//
// Created by Stepan Usatiuk on 09.12.2024.
//

#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include <openssl/ssl.h>

#include "AsyncSslServerTransport.hpp"
#include "Helpers.hpp"

struct ClientCtx {
    std::optional<std::string> client_name;
    AsyncSslServerTransport    transport;
    std::mutex                 ctx_mutex;
};

class Server {
public:
    Server(uint16_t port, uint32_t ip, std::string cert_path, std::string key_path);

    void run();

protected:
    uint16_t _port;
    uint32_t _ip;

    std::string _cert_path;
    std::string _key_path;

    std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> _ssl_ctx;

    void process_req(int conn_fd);

    virtual std::vector<uint8_t> handle_message(ClientCtx& client, std::vector<uint8_t> data) = 0;

private:
    std::atomic<int>        _total_req{0};
    std::atomic<int>        _req_in_progress{0};
    std::mutex              _req_in_progress_mutex;
    std::condition_variable _req_in_progress_cond;
};


#endif // TCPSERVER_HPP
