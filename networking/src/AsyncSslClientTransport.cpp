//
// Created by Stepan Usatiuk on 14.12.2024.
//

#include "AsyncSslClientTransport.hpp"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "Logger.h"

std::future<std::shared_ptr<MsgWrapper>> AsyncSslClientTransport::send_msg(std::vector<uint8_t> message) {
    auto promise = std::make_shared<std::promise<std::shared_ptr<MsgWrapper>>>();

    decltype(_msg_id) id;
    {
        std::lock_guard lock(_promises_mutex);
        id = _msg_id++;
        _promises.emplace(id, promise);
    }

    send_message(std::make_shared<MsgWrapper>(id, std::move(message)));

    return promise->get_future();
}

std::vector<uint8_t> AsyncSslClientTransport::send_msg_and_wait(std::vector<uint8_t> message) {
    auto future = send_msg(std::move(message));
    return future.get()->data;
}

void AsyncSslClientTransport::before_entry() {
    Logger::log(Logger::RemoteFs, [&](std::ostream& os) { os << "Connecting"; }, Logger::INFO);

    int r;
    while ((r = SSL_connect(_ssl.get())) <= 0) {
        ERR_print_errors_fp(stderr);
        int err = SSL_get_error(_ssl.get(), r);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            Helpers::poll_wait(_fd, err == SSL_ERROR_WANT_WRITE);
            continue;
        }
        throw OpenSSLException("SSL_connect() failed");
    }
    Logger::log(Logger::RemoteFs, [&](std::ostream& os) { os << "Connected"; }, Logger::INFO);
}

void AsyncSslClientTransport::handle_message(std::shared_ptr<MsgWrapper> msg) {
    std::lock_guard lock(_promises_mutex);
    auto            future_it = _promises.find(msg->id);

    if (future_it == _promises.end()) {
        Logger::log(Logger::RemoteFs, "Could not find future for msg with id " + std::to_string(msg->id),
                    Logger::ERROR);
        return;
    }

    future_it->second->set_value(msg);
    _promises.erase(future_it);
}
