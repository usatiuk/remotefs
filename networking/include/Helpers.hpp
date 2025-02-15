

#ifndef NETWORKING_HELPERS_H
#define NETWORKING_HELPERS_H

#include <cstdint>
#include <deque>
#include <functional>
#include <vector>

#include "Options.h"
#include "stuff.hpp"

#include <openssl/ssl.h>

using MsgIdType = uint64_t;

struct MsgWrapper {
    MsgIdType            id;
    std::vector<uint8_t> data;
};

struct MsgHeader {
    uint64_t id;
    uint64_t len;
} __attribute__((packed));

namespace Helpers {
void poll_wait(int fd, bool write, int timeout = checked_cast<int>(Options::get<size_t>("timeout")) * 1000);
bool SSL_write(SSL* ctx, int fd, const std::vector<uint8_t>& buf);
void init_nonblock(int fd);
std::vector<uint8_t> SSL_read_n(SSL* ctx, int fd, size_t n);
MsgWrapper           SSL_read_msg(SSL* ctx, int fd);
void                 SSL_send_msg(SSL* ctx, int fd, const MsgWrapper& buf);
} // namespace Helpers

#endif
