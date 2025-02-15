//
// Created by Stepan Usatiuk on 12.12.2024.
//

#include "FsServer.hpp"

#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include "Acl.hpp"
#include "Exception.h"
#include "Logger.h"
#include "Messages.hpp"
#include "Options.h"
#include "Serialize.hpp"
#include "Server.hpp"
#include "stuff.hpp"

static uint32_t parse_ip(const std::string& ip_str) {
    std::istringstream iss(ip_str);
    std::string        part;
    int                ctr = 0;

    uint8_t ip_res[4];

    while (std::getline(iss, part, '.')) {
        if (ctr == 4) {
            throw Exception("Unknown address format");
        }
        if (!part.empty())
            ip_res[ctr++] = (unsigned char) strtol(part.c_str(), NULL, 10);
    }

    uint32_t res;
    memcpy(&res, ip_res, sizeof(res));
    return res;
}

static ACL acl;

class RemoteFsServer : public Server {

private:
    std::vector<uint8_t> handle_auth(ClientCtx& context, AnyMsgT msg) {
        return Serialize::serialize(std::visit(
                [&](auto&& arg) -> AnyMsgT {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, LoginReq>) {
                        Logger::log(Logger::RemoteFs, "Authenticating " + arg.username, Logger::INFO);
                        if (acl.authorize(arg.username, arg.password)) {
                            context.client_name = arg.username;
                            return LoginReply{};
                        } else {
                            throw Exception("Invalid username or password");
                        }
                    } else
                        throw Exception(std::string("Unexpected message type: ") + typeid(T).name());
                },
                msg));
    }

public:
    RemoteFsServer(uint16_t port, uint32_t ip, const std::string& cert_path, const std::string& key_path) :
        Server(port, ip, cert_path, key_path) {}

    std::vector<uint8_t> handle_message(ClientCtx& context, std::vector<uint8_t> data) override {
        try {
            auto msg = Serialize::deserialize<AnyMsgT>(data);
            if (!context.client_name) {
                std::lock_guard lock(context.ctx_mutex);
                if (!context.client_name) {
                    return handle_auth(context, msg);
                }
            }
            return Serialize::serialize(std::visit(
                    [&](auto&& arg) -> AnyMsgT {
                        auto root = std::filesystem::path(Options::get<std::string>("path"));
                        using T   = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, GetattrReq>) {
                            auto path = root.concat(arg.path);

                            if (!std::filesystem::exists(path)) {
                                return GetattrReply{FileType::NONE, 0, 0, 0};
                            } else if (std::filesystem::is_directory(path)) {
                                struct stat buf;
                                stat(path.c_str(), &buf);
                                return GetattrReply{FileType::DIRECTORY, buf.st_mode, buf.st_nlink,
                                                    checked_cast<uint64_t>(buf.st_size)};
                            } else if (std::filesystem::is_regular_file(path)) {
                                struct stat buf;
                                stat(path.c_str(), &buf);
                                return GetattrReply{FileType::REG_FILE, buf.st_mode, buf.st_nlink,
                                                    checked_cast<uint64_t>(buf.st_size)};
                            } else {
                                return GetattrReply{FileType::NONE, 0, 0, 0};
                            }
                        } else if constexpr (std::is_same_v<T, ReaddirReq>) {
                            auto path = root.concat(arg.path);

                            std::vector<std::string> results;
                            for (const auto& entry: std::filesystem::directory_iterator(path)) {
                                results.push_back(entry.path().lexically_relative(root).string());
                            }
                            return ReaddirReply{results};
                        } else if constexpr (std::is_same_v<T, OpenReq>) {
                            auto path = root.concat(arg.path);

                            if (!acl.authorize_path(*context.client_name, arg.path)) {
                                return ErrorReply("Unauthorized path");
                            }

                            if (std::filesystem::exists(path)) {
                                return OpenReply{1};
                            }
                            return OpenReply{0};
                        } else if constexpr (std::is_same_v<T, ReadReq>) {
                            auto path = root.concat(arg.path);

                            if (!acl.authorize_path(*context.client_name, arg.path)) {
                                return ErrorReply("Unauthorized path");
                            }

                            if (std::filesystem::is_regular_file(path)) {
                                std::ifstream        ifs(path, std::ios::binary);
                                std::vector<uint8_t> buf(arg.len);

                                ifs.seekg(arg.off, std::ios::beg);
                                ifs.read(reinterpret_cast<char*>(buf.data()), checked_cast<ssize_t>(arg.len));
                                buf.resize(checked_cast<size_t>(ifs.tellg() - arg.off));

                                return ReadReply{buf};
                            } else {
                                return ReadReply{{}};
                            }
                        } else if constexpr (std::is_same_v<T, WriteReq>) {
                            auto path = root.concat(arg.path);

                            if (!acl.authorize_path(*context.client_name, arg.path)) {
                                return ErrorReply("Unauthorized path");
                            }

                            if (std::filesystem::is_regular_file(path)) {
                                std::fstream ofs(path, std::ios::binary | std::ios::out | std::ios::in);

                                size_t real_write = std::min(checked_cast<size_t>(arg.len), arg.data.size());

                                ofs.seekg(arg.off, std::ios::beg);
                                ofs.write(reinterpret_cast<char*>(arg.data.data()), checked_cast<ssize_t>(real_write));

                                return WriteReply{checked_cast<int>(ofs.tellg() - arg.off)};
                            } else {
                                return WriteReply{-1};
                            }
                        } else if constexpr (std::is_same_v<T, CreateReq>) {
                            auto path = root.concat(arg.path);

                            if (!acl.authorize_path(*context.client_name, arg.path)) {
                                return ErrorReply("Unauthorized path");
                            }

                            if (std::filesystem::exists(path)) {
                                return CreateReply{-1};
                            } else {
                                {
                                    std::ofstream output(path);
                                }
                                chmod(path.c_str(), checked_cast<mode_t>(arg.mode));
                                return CreateReply{0};
                            }
                        } else if constexpr (std::is_same_v<T, ChmodReq>) {
                            auto path = root.concat(arg.path);

                            if (!acl.authorize_path(*context.client_name, arg.path)) {
                                return ErrorReply("Unauthorized path");
                            }

                            if (std::filesystem::exists(path)) {
                                return ChmodReply{-1};
                            } else {
                                int ret = chmod(path.c_str(), checked_cast<mode_t>(arg.mode));
                                return ChmodReply{ret};
                            }
                        } else if constexpr (std::is_same_v<T, MkdirReq>) {
                            auto path = root.concat(arg.path);

                            if (!acl.authorize_path(*context.client_name, arg.path)) {
                                return ErrorReply("Unauthorized path");
                            }

                            if (std::filesystem::exists(path)) {
                                return MkdirReply{-1};
                            } else {
                                std::filesystem::create_directory(path);
                                return MkdirReply{0};
                            }
                        } else if constexpr (std::is_same_v<T, RmdirReq>) {
                            auto path = root.concat(arg.path);

                            if (!acl.authorize_path(*context.client_name, arg.path)) {
                                return ErrorReply("Unauthorized path");
                            }

                            if (!std::filesystem::is_directory(path)) {
                                return RmdirReply{-1};
                            } else {
                                std::filesystem::remove(path);
                                return RmdirReply{0};
                            }
                        } else if constexpr (std::is_same_v<T, UnlinkReq>) {
                            auto path = root.concat(arg.path);

                            if (!acl.authorize_path(*context.client_name, arg.path)) {
                                return ErrorReply("Unauthorized path");
                            }

                            if (!std::filesystem::is_regular_file(path)) {
                                return UnlinkReply{-1};
                            } else {
                                std::filesystem::remove(path);
                                return UnlinkReply{0};
                            }
                        } else if constexpr (std::is_same_v<T, TruncateReq>) {
                            auto path = root.concat(arg.path);

                            if (!acl.authorize_path(*context.client_name, arg.path)) {
                                return ErrorReply("Unauthorized path");
                            }

                            if (!std::filesystem::is_regular_file(path)) {
                                return TruncateReply{-1};
                            } else {
                                std::filesystem::resize_file(path, static_cast<uintmax_t>(arg.size));
                                return TruncateReply{0};
                            }
                        } else if constexpr (std::is_same_v<T, RenameReq>) {
                            auto path    = root.concat(arg.path);
                            auto newPath = root.concat(arg.newPath);

                            if (!acl.authorize_path(*context.client_name, arg.path) ||
                                !acl.authorize_path(*context.client_name, arg.newPath)) {
                                return ErrorReply("Unauthorized path");
                            }

                            if (!std::filesystem::is_regular_file(path)) {
                                return RenameReply{-1};
                            } else {
                                std::filesystem::rename(path, newPath);
                                return RenameReply{0};
                            }
                        } else if constexpr (std::is_same_v<T, UTimensReq>) {
                            auto path = root.concat(arg.path);

                            if (!acl.authorize_path(*context.client_name, arg.path)) {
                                return ErrorReply("Unauthorized path");
                            }

                            timespec time[2] = {
                                    {
                                            arg.asecs,
                                            arg.ans,
                                    },
                                    {
                                            arg.msecs,
                                            arg.mns,
                                    },
                            };
                            int ret = utimensat(AT_FDCWD, path.c_str(), time, 0);
                            return UTimensReply{ret};
                        } else if constexpr (std::is_same_v<T, StatfsReq>) {
                            auto path = root.concat(arg.path);

                            struct statvfs res{};
                            int            ret = statvfs(path.c_str(), &res);

                            return StatfsReply{ret,          res.f_frsize, res.f_bsize, res.f_blocks, res.f_bfree,
                                               res.f_bavail, res.f_files,  res.f_ffree, res.f_favail, res.f_namemax};
                        } else if constexpr (std::is_same_v<T, KeepAliveReq>) {
                            return KeepAliveReply{};
                        } else
                            throw Exception(std::string("Unexpected message type: ") + typeid(T).name());
                    },
                    msg));
        } catch (const std::exception& e) {
            return Serialize::serialize(AnyMsgT{ErrorReply(std::string("Error: ") + e.what())});
        }
    }
};

static std::string read_file(std::string path) {
    std::ifstream ifs(path);

    if (!ifs.is_open()) {
        throw Exception(std::string("Unable to open file ") + path);
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();

    return buffer.str();
}

void FsServer::run() {
    RemoteFsServer server(checked_cast<uint16_t>(Options::get<size_t>("port")),
                          parse_ip(Options::get<std::string>("ip")), Options::get<std::string>("ca_path"),
                          Options::get<std::string>("pk_path"));

    if (Options::get<std::string>("users_path").empty()) {
        throw Exception("Please specify a file with user config: --users_path:<file>");
    }

    auto        users_file = read_file(Options::get<std::string>("users_path"));
    std::string acl_file;

    if (!Options::get<std::string>("acl_path").empty()) {
        acl_file = read_file(Options::get<std::string>("acl_path"));
    }

    acl.load(acl_file, users_file);

    server.run();
}
