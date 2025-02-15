//
// Created by Stepan Usatiuk on 09.12.2024.
//

#include "Helpers.hpp"

#include <fcntl.h>
#include <poll.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "Exception.h"
#include "Options.h"
#include "stuff.hpp"

void Helpers::poll_wait(int fd, bool write, int timeout) {
    pollfd p;
    p.fd      = fd;
    p.events  = write ? POLLOUT : POLLIN;
    p.revents = 0;
    if (poll(&p, 1, timeout) < 0) {
        if (errno == EINTR)
            return;
        throw ErrnoException("Could not poll");
    }
    if (!(p.revents & (write ? POLLOUT : POLLIN))) {
        throw ErrnoException("Could not poll (timeout?)");
    }
}

void Helpers::init_nonblock(int fd) {
    int r = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    if (r < 0) {
        throw ErrnoException("Could not set nonblocking mode");
    }
}

bool Helpers::SSL_write(SSL* ctx, int fd, const std::vector<uint8_t>& buf) {
    for (size_t written = 0; written < buf.size();) {
        size_t written_now = 0;
        int    ret;
        while ((ret = SSL_write_ex(ctx, buf.data() + written, buf.size() - written, &written_now)) <= 0) {
            int err = SSL_get_error(ctx, ret);
            if (err == SSL_ERROR_WANT_WRITE) {
                poll_wait(fd, true);
                continue;
            }

            throw OpenSSLException("write failed");
        }
        written += written_now;
    }
    return true;
}

std::vector<uint8_t> Helpers::SSL_read_n(SSL* ctx, int fd, size_t n) {
    size_t               nread    = 0;
    size_t               read_now = 0;
    std::vector<uint8_t> buf(n);
    int                  ret;

    while (nread < n) {
        while ((ret = SSL_read_ex(ctx, buf.data() + nread, n, &read_now)) <= 0) {
            int err = SSL_get_error(ctx, ret);
            if (err == SSL_ERROR_WANT_READ) {
                Helpers::poll_wait(fd, false);
                continue;
            }
            throw OpenSSLException("read failed");
        }
        nread += read_now;
    }
    return buf;
}

MsgWrapper Helpers::SSL_read_msg(SSL* ctx, int fd) {
    auto      hdrBuf = SSL_read_n(ctx, fd, sizeof(MsgHeader));
    MsgHeader hdr;
    memcpy(&hdr, hdrBuf.data(), sizeof(hdr));
    auto data = SSL_read_n(ctx, fd, hdr.len);
    return {hdr.id, std::move(data)};
}

void Helpers::SSL_send_msg(SSL* ctx, int fd, const MsgWrapper& buf) {
    MsgHeader header{};
    header.id  = buf.id;
    header.len = checked_cast<uint32_t>(buf.data.size());
    std::vector<uint8_t> hdrBuf(sizeof(MsgHeader));
    memcpy(hdrBuf.data(), &header, sizeof(MsgHeader));
    SSL_write(ctx, fd, hdrBuf);
    SSL_write(ctx, fd, buf.data);
}
