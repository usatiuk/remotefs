//
// Created by Stepan Usatiuk on 14.12.2024.
//

#ifndef ASYNCMESSAGECLIENT_HPP
#define ASYNCMESSAGECLIENT_HPP

#include "AsyncSslTransport.hpp"

class AsyncSslClientTransport : public AsyncSslTransport {
public:
    AsyncSslClientTransport(SSL_CTX* ssl_ctx, int fd) : AsyncSslTransport(ssl_ctx, fd) {}

    using SharedMsgPromiseT = std::shared_ptr<std::promise<std::shared_ptr<MsgWrapper>>>;

    std::future<std::shared_ptr<MsgWrapper>> send_msg(std::vector<uint8_t> message);
    std::vector<uint8_t>                     send_msg_and_wait(std::vector<uint8_t> message);

protected:
    void handle_message(std::shared_ptr<MsgWrapper> msg) override;
    void handle_fail() override {
        stop();
        std::exit(EXIT_FAILURE);
    }

    void before_entry() override;

private:
    std::unordered_map<decltype(MsgWrapper::id), SharedMsgPromiseT> _promises;
    uint64_t                                                        _msg_id = 0;
    std::mutex                                                      _promises_mutex;
};

#endif // ASYNCMESSAGECLIENT_HPP
