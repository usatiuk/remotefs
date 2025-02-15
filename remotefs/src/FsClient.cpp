//
// Created by Stepan Usatiuk on 12.12.2024.
//

#include "FsClient.hpp"

#include "Client.hpp"
#include "Options.h"
#include "stuff.hpp"

#include <fuse.h>
#include <iostream>

#include <sys/statvfs.h>
#include <unistd.h>

#include "Logger.h"
#include "Messages.hpp"
#include "Serialize.hpp"

static Client*         client;
static AsyncSslClientTransport* asyncTransport;

template<typename R, typename M>
R call(M msg) {
    auto ret = asyncTransport->send_msg_and_wait(Serialize::serialize(AnyMsgT{msg}));

    auto deserialized = Serialize::deserialize<AnyMsgT>(ret);
    if (!std::holds_alternative<R>(deserialized)) {
        if (std::holds_alternative<ErrorReply>(deserialized)) {
            throw Exception("Error when reading: " + std::get<ErrorReply>(deserialized).error);
        } else {
            throw Exception("Unexpected reply from server");
        }
    }
    return std::get<R>(deserialized);
}

static int rfsGetattr(const char* path, struct stat* stbuf) {
    try {
        memset(stbuf, 0, sizeof(struct stat));
        if (strcmp(path, "/") == 0) {
            stbuf->st_mode  = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }

        auto ret = call<GetattrReply>(GetattrReq{path});
        switch (ret.type) {
            case FileType::NONE:
                return -ENOENT;
                break;
            case FileType::DIRECTORY:
                stbuf->st_mode = S_IFDIR;
                break;
            case FileType::REG_FILE:
                stbuf->st_mode = S_IFREG;
                break;
            case FileType::SYMLINK:
                stbuf->st_mode = S_IFLNK;
                break;
            default:
                return -ENOENT;
        }

        stbuf->st_mode |= checked_cast<mode_t>(ret.mode);
        stbuf->st_size  = checked_cast<off_t>(ret.size);
        stbuf->st_nlink = checked_cast<nlink_t>(ret.links);

        return 0;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsReaddir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
    try {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        auto ret = call<ReaddirReply>(ReaddirReq{path});

        for (auto const& e: ret.path) {
            filler(buf, e.c_str(), NULL, 0);
        }

        return 0;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsOpen(const char* path, struct fuse_file_info* fi) {
    try {
        auto ret = call<OpenReply>(OpenReq{path});

        if (ret.ok != 1) {
            return -ENOENT;
        }

        return 0;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsRead(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    try {
        auto   ret        = call<ReadReply>(ReadReq{path, offset, size});
        size_t reallyRead = std::min(ret.data.size(), size);
        std::memcpy(buf, ret.data.data(), reallyRead);
        return checked_cast<int>(reallyRead);
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsWrite(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    try {
        auto ret = call<WriteReply>(WriteReq{path, offset, size, std::vector<uint8_t>(buf, buf + size)});
        return ret.len;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsCreate(const char* path, mode_t mode, struct fuse_file_info* fi) {
    try {
        auto ret = call<CreateReply>(CreateReq{std::string(path), static_cast<int>(mode)});
        return ret.ok;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsMkdir(const char* path, mode_t mode) {
    try {
        auto ret = call<MkdirReply>(MkdirReq{std::string(path), static_cast<int>(mode)});
        return ret.ok;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsRmdir(const char* path) {
    try {
        auto ret = call<RmdirReply>(RmdirReq{std::string(path)});
        return ret.ok;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsUnlink(const char* path) {
    try {
        auto ret = call<UnlinkReply>(UnlinkReq{std::string(path)});
        return ret.ok;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsTruncate(const char* path, off_t size) {
    try {
        auto ret = call<TruncateReply>(TruncateReq{std::string(path), size});
        return ret.res;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsRename(const char* path, const char* newPath) {
    try {
        auto ret = call<RenameReply>(RenameReq{std::string(path), newPath});
        return ret.ok;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsUtimens(const char* path, const struct timespec time[2]) {
    try {
        auto ret =
                call<UTimensReply>(UTimensReq{path, time[0].tv_sec, time[0].tv_nsec, time[1].tv_sec, time[1].tv_nsec});
        return ret.ok;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsUtime(const char* path, struct utimbuf* time) {
    timespec timens[2] = {
            {
                    time->actime,
                    0,
            },
            {
                    time->modtime,
                    0,
            },
    };

    return rfsUtimens(path, timens);
}

static int rfsStatfs(const char* path, struct statvfs* stats) {
    try {
        auto ret         = call<StatfsReply>(StatfsReq{path});
        stats->f_frsize  = checked_cast<decltype(stats->f_frsize)>(ret.frsize);
        stats->f_bsize   = checked_cast<decltype(stats->f_bsize)>(ret.blksize);
        stats->f_blocks  = checked_cast<decltype(stats->f_blocks)>(ret.blocks);
        stats->f_bfree   = checked_cast<decltype(stats->f_bfree)>(ret.bfree);
        stats->f_bavail  = checked_cast<decltype(stats->f_bavail)>(ret.bavail);
        stats->f_files   = checked_cast<decltype(stats->f_files)>(ret.files);
        stats->f_ffree   = checked_cast<decltype(stats->f_ffree)>(ret.ffree);
        stats->f_favail  = checked_cast<decltype(stats->f_favail)>(ret.favail);
        stats->f_namemax = checked_cast<decltype(stats->f_namemax)>(ret.namemax);
        return ret.ok;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

static int rfsChmod(const char* path, mode_t mode) {
    try {
        auto ret = call<ChmodReply>(ChmodReq{std::string(path), static_cast<int>(mode)});
        return ret.ok;
    } catch (std::exception& e) {
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        return -EIO;
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

static struct fuse_operations ops = {
        .getattr  = rfsGetattr,
        .mkdir    = rfsMkdir,
        .unlink   = rfsUnlink,
        .rmdir    = rfsRmdir,
        .rename   = rfsRename,
        .chmod    = rfsChmod,
        .truncate = rfsTruncate,
        .utime    = rfsUtime,
        .open     = rfsOpen,
        .read     = rfsRead,
        .write    = rfsWrite,
        .statfs   = rfsStatfs,
        .readdir  = rfsReaddir,
        .create   = rfsCreate,
        .utimens  = rfsUtimens,
};

#pragma GCC diagnostic pop

std::thread keep_alive_thread;

static void keep_alive() {
    while (!asyncTransport->is_stopped()) {
        try {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            call<KeepAliveReply>(KeepAliveReq{});
        } catch (std::exception& e) {
            Logger::log(Logger::RemoteFs, std::string("Keepalive error: ") + e.what(), Logger::ERROR);
        }
    }
}

void FsClient::run() {
    client = new Client(checked_cast<uint16_t>(Options::get<size_t>("port")), Options::get<std::string>("ip"),
                           Options::get<std::string>("ca_path"), Options::get<std::string>("pk_path"));

    client->run();
    asyncTransport  = &client->transport();
    keep_alive_thread = std::thread(keep_alive);

    Logger::log(
            Logger::RemoteFs,
            [&](std::ostream& os) {
                os << "Trying to log in with " << Options::get<std::string>("username") << " "
                   << Options::get<std::string>("password");
            },
            Logger::INFO);

    call<LoginReply>(LoginReq{Options::get<std::string>("username"), Options::get<std::string>("password")});

    char        arg1[] = "";
    char        arg2[] = "-o";
    std::string arg3   = "uid=" + std::to_string(getuid());
    char        arg4[] = "-o";
    std::string arg5   = "gid=" + std::to_string(getgid());
    auto        arg6   = Options::get<std::string>("path");
    char        arg8[] = "-f";

    int   argc   = 7;
    char* argv[] = {arg1, arg2, arg3.data(), arg4, arg5.data(), arg6.data(), arg8};
    std::cout << static_cast<int>(fuse_main(argc, argv, &ops, nullptr));
}
