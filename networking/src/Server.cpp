//
// Created by Stepan Usatiuk on 09.12.2024.
//

#ifndef TCPSERVER_IPP
#define TCPSERVER_IPP

#include "Server.hpp"

#include <memory>
#include <string>
#include <thread>

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
#include <openssl/ssl.h>

#include "Exception.h"
#include "Helpers.hpp"
#include "Logger.h"

// From https://wiki.openssl.org/index.php/Simple_TLS_Server
static std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> create_context() {
    const SSL_METHOD* method;
    SSL_CTX*          ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        throw OpenSSLException("Unable to create SSL context");
    }

    return {ctx, &SSL_CTX_free};
}

static void configure_context(SSL_CTX* ctx, const std::string& cert_path, const std::string& key_path) {
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, cert_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw OpenSSLException("Unable to read certificate file");
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        throw OpenSSLException("Unable to read private key file");
    }
}

Server::Server(uint16_t port, uint32_t ip, std::string cert_path, std::string key_path) :
    _port(port), _ip(ip), _cert_path(std::move(cert_path)), _key_path(std::move(key_path)), _ssl_ctx(create_context()) {
    configure_context(_ssl_ctx.get(), _cert_path, _key_path);
}

void Server::process_req(int conn_fd) {
    _req_in_progress.fetch_add(1);
    std::thread proc([=, this] {
        int id = _total_req.fetch_add(1);

        Logger::log(Logger::RemoteFs, "Client " + std::to_string(id) + " connecting\n", Logger::INFO);

        ClientCtx context{{}, {_ssl_ctx.get(), conn_fd, id}, {}};

        try {
            Helpers::init_nonblock(conn_fd);

            context.transport.run();

            for (;;) {
                auto msg = context.transport.get_msg();

                if (!msg)
                    break;

                std::thread msg_proc([&context, msg, this] {
                    auto ret = this->handle_message(context, std::move(msg->data));
                    context.transport.send_message(std::make_shared<MsgWrapper>(msg->id, std::move(ret)));
                });
                msg_proc.detach();
            }
        } catch (std::exception& e) {
            Logger::log(Logger::RemoteFs, std::string("Error: ") + e.what(), Logger::ERROR);
        }

        close(conn_fd);
        _req_in_progress.fetch_sub(1);
        std::lock_guard<std::mutex> lock(_req_in_progress_mutex);
        Logger::log(Logger::RemoteFs, "Client " + std::to_string(id) + " finished\n", Logger::INFO);
        _req_in_progress_cond.notify_all();
    });
    proc.detach();
}

void Server::run() {
    protoent* proto = getprotobyname("tcp");
    if (proto == NULL) {
        throw ErrnoException("Could not get TCP protocol info");
    }

    int         sock = socket(AF_INET, SOCK_STREAM, proto->p_proto);
    sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(_port);
    addr.sin_addr   = {_ip};
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
#ifdef APPLE
    addr.sin_len = sizeof(struct sockaddr_in),
#endif

    Logger::log(
            Logger::RemoteFs,
            [&](std::ostream& os) {
                os << "Listening on ";
                for (int i = 0; i < 4; i++) {
                    os << static_cast<int>(reinterpret_cast<uint8_t*>(&_ip)[i]);
                    if (i != 3)
                        os << ".";
                }
                os << ":" << _port;
            },
            Logger::INFO);

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw ErrnoException("Could not bind");
    }

    if (listen(sock, 1) < 0) {
        throw ErrnoException("Could not listen");
    }

    Helpers::init_nonblock(sock);

    try {
        // while (!Signals::is_stopped()) {
        while (true) {
            try {
                Helpers::poll_wait(sock, false, -1);
                int conn = accept(sock, nullptr, nullptr);
                if (conn == -1) {
                    throw ErrnoException("accept");
                    continue;
                }
                process_req(conn);
            } catch (std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    Logger::log(Logger::RemoteFs, "Exiting", Logger::INFO);
    {
        std::unique_lock lock(_req_in_progress_mutex);
        _req_in_progress_cond.wait(lock, [&] { return _req_in_progress == 0; });
    }

    close(sock);
}


#endif // TCPSERVER_IPP
