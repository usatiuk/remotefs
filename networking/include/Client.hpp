//
// Created by stepus53 on 11.12.24.
//

#ifndef TCPCLIENT_HPP
#define TCPCLIENT_HPP

#include <cstdint>
#include <mutex>
#include <string>
#include <memory>
#include <optional>

#include <openssl/ssl.h>

#include "AsyncSslClientTransport.hpp"

class Client {
public:
    Client(uint16_t port, std::string ip, std::string cert_path, std::string key_path);

    void run();

    AsyncSslClientTransport& transport() { return *_transport; }

protected:
    uint16_t    _port;
    std::string _ip;

    std::string _cert_path;
    std::string _key_path;

    std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> _ssl_ctx;
    std::unique_ptr<SSL, decltype(&SSL_free)>         _ssl{nullptr, &SSL_free};

    int    _sock;
    size_t _msg_id = 0;

private:
    std::optional<AsyncSslClientTransport> _transport;
};

#endif // TCPCLIENT_HPP
