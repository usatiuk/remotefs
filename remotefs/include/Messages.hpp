//
// Created by Stepan Usatiuk on 12.12.2024.
//

#ifndef MESSAGES_HPP
#define MESSAGES_HPP

#include <cstdint>
#include <variant>

#include "SerializableStruct.hpp"
#include "Serialize.hpp"

enum class FileType { NONE, DIRECTORY, REG_FILE, SYMLINK, END };

#define LOGIN_REQ(FIELD)                                                                                               \
    FIELD(std::string, username)                                                                                       \
    FIELD(std::string, password)
DECLARE_SERIALIZABLE(LoginReq, LOGIN_REQ)
DECLARE_SERIALIZABLE_END
#undef LOGIN_REQ

#define LOGIN_REPLY(FIELD)
DECLARE_SERIALIZABLE(LoginReply, LOGIN_REPLY)
DECLARE_SERIALIZABLE_END
#undef LOGIN_REPLY

#define GETATTR_REQ(FIELD) FIELD(std::string, path)
DECLARE_SERIALIZABLE(GetattrReq, GETATTR_REQ)
DECLARE_SERIALIZABLE_END
#undef GETATTR_REQ

#define GETATTR_REPLY(FIELD)                                                                                           \
    FIELD(FileType, type)                                                                                              \
    FIELD(uint64_t, mode)                                                                                              \
    FIELD(uint64_t, links)                                                                                             \
    FIELD(uint64_t, size)
DECLARE_SERIALIZABLE(GetattrReply, GETATTR_REPLY)
DECLARE_SERIALIZABLE_END
#undef GETATTR_REPLY

#define READDIR_REQ(FIELD) FIELD(std::string, path)
DECLARE_SERIALIZABLE(ReaddirReq, READDIR_REQ)
DECLARE_SERIALIZABLE_END
#undef READDIR_REQ

#define READDIR_REPLY(FIELD) FIELD(std::vector<std::string>, path)
DECLARE_SERIALIZABLE(ReaddirReply, READDIR_REPLY)
DECLARE_SERIALIZABLE_END
#undef READDIR_REPLY

#define OPEN_REQ(FIELD) FIELD(std::string, path)
DECLARE_SERIALIZABLE(OpenReq, OPEN_REQ)
DECLARE_SERIALIZABLE_END
#undef OPEN_REQ

#define OPEN_REPLY(FIELD) FIELD(int, ok)
DECLARE_SERIALIZABLE(OpenReply, OPEN_REPLY)
DECLARE_SERIALIZABLE_END
#undef OPEN_REPLY

#define READ_REQ(FIELD)                                                                                                \
    FIELD(std::string, path)                                                                                           \
    FIELD(int64_t, off)                                                                                                \
    FIELD(uint64_t, len)
DECLARE_SERIALIZABLE(ReadReq, READ_REQ)
DECLARE_SERIALIZABLE_END
#undef READ_REQ

#define READ_REPLY(FIELD) FIELD(std::vector<uint8_t>, data)
DECLARE_SERIALIZABLE(ReadReply, READ_REPLY)
DECLARE_SERIALIZABLE_END
#undef READ_REPLY

#define WRITE_REQ(FIELD)                                                                                               \
    FIELD(std::string, path)                                                                                           \
    FIELD(int64_t, off)                                                                                                \
    FIELD(uint64_t, len)                                                                                               \
    FIELD(std::vector<uint8_t>, data)
DECLARE_SERIALIZABLE(WriteReq, WRITE_REQ)
DECLARE_SERIALIZABLE_END
#undef WRITE_REQ

#define WRITE_REPLY(FIELD) FIELD(int, len)
DECLARE_SERIALIZABLE(WriteReply, WRITE_REPLY)
DECLARE_SERIALIZABLE_END
#undef WRITE_REPLY

#define CREATE_REQ(FIELD)                                                                                              \
    FIELD(std::string, path)                                                                                           \
    FIELD(int, mode)
DECLARE_SERIALIZABLE(CreateReq, CREATE_REQ)
DECLARE_SERIALIZABLE_END
#undef CREATE_REQ

#define CREATE_REPLY(FIELD) FIELD(int, ok)
DECLARE_SERIALIZABLE(CreateReply, CREATE_REPLY)
DECLARE_SERIALIZABLE_END
#undef CREATE_REPLY

#define CHMOD_REQ(FIELD)                                                                                               \
    FIELD(std::string, path)                                                                                           \
    FIELD(int, mode)
DECLARE_SERIALIZABLE(ChmodReq, CHMOD_REQ)
DECLARE_SERIALIZABLE_END
#undef CHMOD_REQ

#define CHMOD_REPLY(FIELD) FIELD(int, ok)
DECLARE_SERIALIZABLE(ChmodReply, CHMOD_REPLY)
DECLARE_SERIALIZABLE_END
#undef CHMOD_REPLY

#define MKDIR_REQ(FIELD)                                                                                               \
    FIELD(std::string, path)                                                                                           \
    FIELD(int, mode)
DECLARE_SERIALIZABLE(MkdirReq, MKDIR_REQ)
DECLARE_SERIALIZABLE_END
#undef MKDIR_REQ

#define MKDIR_REPLY(FIELD) FIELD(int, ok)
DECLARE_SERIALIZABLE(MkdirReply, MKDIR_REPLY)
DECLARE_SERIALIZABLE_END
#undef MKDIR_REPLY

#define RMDIR_REQ(FIELD) FIELD(std::string, path)
DECLARE_SERIALIZABLE(RmdirReq, RMDIR_REQ)
DECLARE_SERIALIZABLE_END
#undef RMDIR_REQ

#define RMDIR_REPLY(FIELD) FIELD(int, ok)
DECLARE_SERIALIZABLE(RmdirReply, RMDIR_REPLY)
DECLARE_SERIALIZABLE_END
#undef RMDIR_REPLY

#define UNLINK_REQ(FIELD) FIELD(std::string, path)
DECLARE_SERIALIZABLE(UnlinkReq, UNLINK_REQ)
DECLARE_SERIALIZABLE_END
#undef UNLINK_REQ

#define UNLINK_REPLY(FIELD) FIELD(int, ok)
DECLARE_SERIALIZABLE(UnlinkReply, UNLINK_REPLY)
DECLARE_SERIALIZABLE_END
#undef UNLINK_REPLY

#define TRUNCATE_REQ(FIELD)                                                                                            \
    FIELD(std::string, path)                                                                                           \
    FIELD(long, size)
DECLARE_SERIALIZABLE(TruncateReq, TRUNCATE_REQ)
DECLARE_SERIALIZABLE_END
#undef TRUNCATE_REQ

#define TRUNCATE_REPLY(FIELD) FIELD(int, res)
DECLARE_SERIALIZABLE(TruncateReply, TRUNCATE_REPLY)
DECLARE_SERIALIZABLE_END
#undef TRUNCATE_REPLY

#define RENAME_REQ(FIELD)                                                                                              \
    FIELD(std::string, path)                                                                                           \
    FIELD(std::string, newPath)
DECLARE_SERIALIZABLE(RenameReq, RENAME_REQ)
DECLARE_SERIALIZABLE_END
#undef RENAME_REQ

#define RENAME_REPLY(FIELD) FIELD(int, ok)
DECLARE_SERIALIZABLE(RenameReply, RENAME_REPLY)
DECLARE_SERIALIZABLE_END
#undef RENAME_REPLY

#define UTIMENS_REQ(FIELD)                                                                                             \
    FIELD(std::string, path)                                                                                           \
    FIELD(int64_t, asecs)                                                                                              \
    FIELD(int64_t, ans)                                                                                                \
    FIELD(int64_t, msecs)                                                                                              \
    FIELD(int64_t, mns)
DECLARE_SERIALIZABLE(UTimensReq, UTIMENS_REQ)
DECLARE_SERIALIZABLE_END
#undef UTIMENS_REQ

#define UTIMENS_REPLY(FIELD) FIELD(int, ok)
DECLARE_SERIALIZABLE(UTimensReply, UTIMENS_REPLY)
DECLARE_SERIALIZABLE_END
#undef UTIMENS_REPLY

#define STATFS_REQ(FIELD) FIELD(std::string, path)
DECLARE_SERIALIZABLE(StatfsReq, STATFS_REQ)
DECLARE_SERIALIZABLE_END
#undef STATFS_REQ

#define STATFS_REPLY(FIELD)                                                                                            \
    FIELD(int, ok)                                                                                                     \
    FIELD(uint64_t, frsize)                                                                                            \
    FIELD(uint64_t, blksize)                                                                                           \
    FIELD(uint64_t, blocks)                                                                                            \
    FIELD(uint64_t, bfree)                                                                                             \
    FIELD(uint64_t, bavail)                                                                                            \
    FIELD(uint64_t, files)                                                                                             \
    FIELD(uint64_t, ffree)                                                                                             \
    FIELD(uint64_t, favail)                                                                                            \
    FIELD(uint64_t, namemax)
DECLARE_SERIALIZABLE(StatfsReply, STATFS_REPLY)
DECLARE_SERIALIZABLE_END
#undef STATFS_REPLY

#define KEEPALIVE_REQ(FIELD)
DECLARE_SERIALIZABLE(KeepAliveReq, KEEPALIVE_REQ)
DECLARE_SERIALIZABLE_END
#undef RENAME_REQ

#define KEEPALIVE_REPLY(FIELD)
DECLARE_SERIALIZABLE(KeepAliveReply, KEEPALIVE_REPLY)
DECLARE_SERIALIZABLE_END
#undef KEEPALIVE_REPLY

#define ERROR_REPLY(FIELD) FIELD(std::string, error)
DECLARE_SERIALIZABLE(ErrorReply, ERROR_REPLY)
DECLARE_SERIALIZABLE_END
#undef ERROR_REPLY

using AnyMsgT = std::variant<ErrorReply, KeepAliveReq, KeepAliveReply, LoginReq, LoginReply, GetattrReq, GetattrReply,
                             ReaddirReq, ReaddirReply, OpenReq, OpenReply, ReadReq, ReadReply, WriteReq, WriteReply,
                             CreateReq, CreateReply, MkdirReq, MkdirReply, RmdirReq, RmdirReply, UnlinkReq, UnlinkReply,
                             TruncateReq, TruncateReply, RenameReq, RenameReply, UTimensReq, UTimensReply, StatfsReply,
                             StatfsReq, ChmodReq, ChmodReply>;

#endif // MESSAGES_HPP
