//
// Created by Stepan Usatiuk on 14.12.2024.
//

#ifndef MESSAGEPUMP_HPP
#define MESSAGEPUMP_HPP

#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <openssl/ssl.h>

#include "Helpers.hpp"

class AsyncSslTransport {
public:
    AsyncSslTransport(SSL_CTX* ssl_ctx, int fd);
    virtual ~AsyncSslTransport() = 0;
    void run();

    void send_message(std::shared_ptr<MsgWrapper> msg);

    bool is_failed() const { return _failed; }
    bool is_stopped() const { return _stopped; }

    void stop();

protected:
    virtual void handle_message(std::shared_ptr<MsgWrapper> msg) = 0;
    virtual void handle_fail()                                   = 0;

    virtual void before_entry() {}

    std::unique_ptr<SSL, decltype(&SSL_free)> _ssl{nullptr, &SSL_free};
    int                                       _fd;

private:
    void thread_entry();

    std::atomic<bool>       _stopped = 0;
    std::mutex              _stopped_mutex;
    std::condition_variable _stopped_condition;

    std::thread _thread;

    std::mutex                              _to_send_mutex;
    int                                     _to_send_notif_pipe[2];
    std::deque<std::shared_ptr<MsgWrapper>> _to_send;

    std::atomic<bool> _failed;

    AsyncSslTransport(const AsyncSslTransport& other)                = delete;
    AsyncSslTransport(AsyncSslTransport&& other) noexcept            = delete;
    AsyncSslTransport& operator=(const AsyncSslTransport& other)     = delete;
    AsyncSslTransport& operator=(AsyncSslTransport&& other) noexcept = delete;
};

#endif // MESSAGEPUMP_HPP
