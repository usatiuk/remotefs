//
// Created by stepus53 on 11.12.24.
//

#include "Client.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <openssl/err.h>

#include "Exception.h"
#include "Helpers.hpp"
#include "Logger.h"

// From https://wiki.openssl.org/index.php/Simple_TLS_Server
static std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> create_context() {
    const SSL_METHOD* method;
    SSL_CTX*          ctx;

    method = TLS_client_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        throw OpenSSLException("Unable to create SSL context");
    }

    return {ctx, &SSL_CTX_free};
}

Client::Client(uint16_t port, std::string ip, std::string cert_path, std::string key_path) :
    _port(port), _ip(ip), _cert_path(cert_path), _key_path(key_path), _ssl_ctx(create_context()) {
    SSL_CTX_set_verify(_ssl_ctx.get(), SSL_VERIFY_PEER, nullptr);
    if (SSL_CTX_load_verify_locations(_ssl_ctx.get(), cert_path.c_str(), nullptr) <= 0) {
        throw OpenSSLException("Unable to read certificate file");
    }
}

void Client::run() {
    protoent* proto = getprotobyname("tcp");
    if (proto == NULL) {
        throw ErrnoException("Could not get TCP protocol info");
    }

    _sock = socket(AF_INET, SOCK_STREAM, proto->p_proto);
    sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(_port);

    if (inet_pton(AF_INET, _ip.c_str(), &addr.sin_addr) <= 0)
        throw ErrnoException("inet_pton()");

    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
#ifdef APPLE
    addr.sin_len = sizeof(struct sockaddr_in),
#endif

    if (connect(_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) throw ErrnoException("connect()");

    _transport.emplace(_ssl_ctx.get(), _sock);
    _transport->run();
}
