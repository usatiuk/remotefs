//
// Created by Stepan Usatiuk on 15.12.2024.
//

#include "AsyncSslServerTransport.hpp"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "Logger.h"

void AsyncSslServerTransport::handle_message(std::shared_ptr<MsgWrapper> msg) {
    std::lock_guard lock(_msgs_mutex);
    _msgs.emplace_back(msg);
    _msgs_condition.notify_all();
}


void AsyncSslServerTransport::handle_fail() {
    _msgs_condition.notify_all();
}

void AsyncSslServerTransport::before_entry() {
    int r;
    while ((r = SSL_accept(_ssl.get())) <= 0) {
        ERR_print_errors_fp(stderr);
        int err = SSL_get_error(_ssl.get(), r);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            Helpers::poll_wait(_fd, err == SSL_ERROR_WANT_WRITE);
            continue;
        }
        throw OpenSSLException("SSL_accept() failed");
    }
    Logger::log(Logger::RemoteFs, "Client " + std::to_string(_client_id) + " connected\n", Logger::INFO);
}

std::shared_ptr<MsgWrapper> AsyncSslServerTransport::get_msg() {
    std::unique_lock lock(_msgs_mutex);
    _msgs_condition.wait(lock, [&] { return !_msgs.empty() || is_failed() || is_stopped(); });

    if (is_failed())
        return nullptr;

    auto ret = _msgs.begin();

    auto real_ret = *ret;

    _msgs.erase(_msgs.begin());

    return real_ret;
}
