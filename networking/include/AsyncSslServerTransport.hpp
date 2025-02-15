//
// Created by Stepan Usatiuk on 15.12.2024.
//

#ifndef SERVERMESSAGEPUMP_HPP
#define SERVERMESSAGEPUMP_HPP

#include "AsyncSslTransport.hpp"

class AsyncSslServerTransport : public AsyncSslTransport {
public:
    AsyncSslServerTransport(SSL_CTX* ssl_ctx, int fd, int client_id) : AsyncSslTransport(ssl_ctx, fd), _client_id(client_id) {}

    // Null if finished
    std::shared_ptr<MsgWrapper> get_msg();

protected:
    void handle_message(std::shared_ptr<MsgWrapper> msg) override;
    void handle_fail() override;

    void before_entry() override;

private:
    int _client_id;

    std::deque<std::shared_ptr<MsgWrapper>> _msgs;
    std::mutex                              _msgs_mutex;
    std::condition_variable                 _msgs_condition;
};


#endif // SERVERMESSAGEPUMP_HPP
