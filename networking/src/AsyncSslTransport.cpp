//
// Created by Stepan Usatiuk on 14.12.2024.
//

#include "AsyncSslTransport.hpp"

#include <fcntl.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <poll.h>
#include <unistd.h>

#include <iomanip>

#include "Logger.h"
#include "Serialize.hpp"
#include "stuff.hpp"

AsyncSslTransport::AsyncSslTransport(SSL_CTX* ssl_ctx, int fd) : _ssl(SSL_new(ssl_ctx), &SSL_free), _fd(fd) {
    SSL_set_fd(_ssl.get(), _fd);
    pipe(_to_send_notif_pipe);
    Helpers::init_nonblock(_fd);
}

AsyncSslTransport::~AsyncSslTransport() {
    stop();
    _thread.join();
}

void AsyncSslTransport::stop() {
    std::unique_lock lock(_stopped_mutex);
    _stopped_condition.notify_all();
    _stopped = true;
    write(_to_send_notif_pipe[1], "1", 1);
}

void AsyncSslTransport::send_message(std::shared_ptr<MsgWrapper> msg) {
    std::unique_lock lock(_to_send_mutex);
    _to_send.push_back(std::move(msg));
    write(_to_send_notif_pipe[1], "1", 1);
}

void AsyncSslTransport::thread_entry() {
    try {
        before_entry();

        bool                 sending = false;
        std::vector<uint8_t> to_send_buf{};
        size_t               cur_sent = 0;

        bool                 reading_msg = false; // False if reading header, true if message
        std::vector<uint8_t> read_buf;
        size_t               cur_read{};
        uint64_t             msg_id  = 0;
        size_t               msg_len = sizeof(MsgHeader); // Message length to read if reading message, or header len
        read_buf.resize(msg_len);

        while (!_stopped) {
            if (!sending) {
                std::shared_ptr<MsgWrapper> to_send_now{};

                std::unique_lock lock(_to_send_mutex);
                auto             next = _to_send.begin();
                if (next != _to_send.end()) {
                    to_send_now = *next;
                    _to_send.erase(next);
                }

                if (to_send_now) {
                    MsgHeader header{};
                    header.id  = htobe64(to_send_now->id);
                    header.len = htobe64(to_send_now->data.size());
                    to_send_buf.resize(sizeof(MsgHeader) + to_send_now->data.size());
                    memcpy(to_send_buf.data(), &header, sizeof(MsgHeader));
                    memcpy(to_send_buf.data() + sizeof(MsgHeader), to_send_now->data.data(), to_send_now->data.size());
                    sending = true;
                    Logger::log(
                            Logger::RemoteFs, [&](std::ostream& os) { os << "Started sending message " << to_send_now->id; },
                            Logger::DEBUG);
                }
            }

            while (sending) {
                size_t written_now = 0;
                int    ret;
                if ((ret = SSL_write_ex(_ssl.get(), to_send_buf.data() + cur_sent, to_send_buf.size() - cur_sent,
                                        &written_now)) <= 0) {
                    int err = SSL_get_error(_ssl.get(), ret);
                    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                        break;
                    }

                    throw OpenSSLException("write failed");
                }
                Logger::log(
                        Logger::RemoteFs,
                        [&](std::ostream& os) {
                            os << "Written " << written_now;
                            if (Logger::en_level(Logger::RemoteFs, Logger::TRACE)) {
                                os << ": ";
                                for (size_t i = 0; i < written_now; i++) {
                                    os << std::setw(2) << std::setfill('0') << std::hex
                                       << (int) to_send_buf[i + cur_sent] << " ";
                                }
                            }
                        },
                        Logger::DEBUG);
                cur_sent += written_now;

                if (cur_sent == to_send_buf.size()) {
                    Logger::log(Logger::RemoteFs, [&](std::ostream& os) { os << "Finished sending"; }, Logger::DEBUG);
                    to_send_buf.resize(0);
                    cur_sent = 0;
                    sending  = false;
                }
            }

            while (true) {
                int    ret;
                size_t read_now = 0;
                if ((ret = SSL_read_ex(_ssl.get(), read_buf.data() + cur_read, msg_len, &read_now)) <= 0) {
                    int err = SSL_get_error(_ssl.get(), ret);
                    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                        break;
                    }

                    throw OpenSSLException("read failed");
                }
                Logger::log(
                        Logger::RemoteFs,
                        [&](std::ostream& os) {
                            os << "Read " << read_now;
                            if (Logger::en_level(Logger::RemoteFs, Logger::TRACE)) {
                                os << ": ";
                                for (size_t i = 0; i < read_now; i++) {
                                    os << std::setw(2) << std::setfill('0') << std::hex << (int) read_buf[i + cur_read]
                                       << " ";
                                }
                            }
                        },
                        Logger::DEBUG);
                cur_read += read_now;

                if (cur_read == msg_len) {
                    if (reading_msg) {
                        handle_message(std::make_shared<MsgWrapper>(msg_id, std::move(read_buf)));
                        reading_msg = false;
                        msg_len     = sizeof(MsgHeader);
                        Logger::log(
                                Logger::RemoteFs,
                                [&](std::ostream& os) { os << "Finished receiving message " << msg_id; },
                                Logger::DEBUG);
                    } else {
                        MsgHeader hdr;
                        memcpy(&hdr, read_buf.data(), sizeof(hdr));
                        reading_msg = true;
                        msg_len     = be64toh(hdr.len);
                        msg_id      = be64toh(hdr.id);
                        Logger::log(
                                Logger::RemoteFs,
                                [&](std::ostream& os) { os << "Started receiving message " << msg_id; }, Logger::DEBUG);
                    }

                    read_buf.clear();
                    read_buf.resize(msg_len);
                    cur_read = 0;
                }
            }

            pollfd fds[2];

            fds[0].fd     = _fd;
            fds[0].events = POLLIN;
            if (sending)
                fds[0].events |= POLLOUT;
            fds[0].revents = 0;

            fds[1].fd      = _to_send_notif_pipe[0];
            fds[1].events  = POLLIN;
            fds[1].revents = 0;

            Logger::log(Logger::RemoteFs, [&](std::ostream& os) { os << "Waiting"; }, Logger::DEBUG);


            if (poll(fds, 2, checked_cast<int>(Options::get<size_t>("timeout")) * 1000) < 0) {
                if (errno == EINTR)
                    return;
                throw ErrnoException("Could not poll");
            }

            if (!(fds[0].revents & POLLOUT) && !(fds[1].revents & POLLIN) && !(fds[0].revents & POLLIN)) {
                throw ErrnoException("Could not poll (timeout?)");
            }

            if (fds[1].revents & POLLIN) {
                char temp;
                Logger::log(
                        Logger::RemoteFs, [&](std::ostream& os) { os << "Received message on pipe"; }, Logger::DEBUG);
                read(_to_send_notif_pipe[0], &temp, 1);
            }

            // Logger::log(
            //         Logger::Server,
            //         [&](std::ostream& os) {
            //             os << "Sent " << (*next)->id << " with " << (*next)->data.size() << " bytes: ";
            //             if (Logger::en_level(Logger::Server, Logger::TRACE))
            //                 for (unsigned char i: (*next)->data)
            //                     os << (int) i << " ";
            //         },
            //         Logger::DEBUG);
            // Helpers::SSL_send_msg(_ssl, _fd, **next);
        }
    } catch (std::exception& e) {
        _failed = true;
        Logger::log(Logger::RemoteFs, e.what(), Logger::ERROR);
        handle_fail();
    }
    SSL_shutdown(_ssl.get());
    OPENSSL_thread_stop();
}

// void SSLMessagePump::receiver_entry() {
//     try {
//         before_receiver();
//
//         while (!_stopped) {
//             auto msg = std::make_shared<MsgWrapper>(Helpers::SSL_read_msg(_ssl, _fd));
//             Logger::log(
//                     Logger::Server,
//                     [&](std::ostream& os) {
//                         os << "Received " << msg->id << " with " << msg->data.size() << " bytes: ";
//                         if (Logger::en_level(Logger::Server, Logger::TRACE))
//                             for (unsigned char i: msg->data)
//                                 os << (int) i << " ";
//                     },
//                     Logger::DEBUG);
//             handle_message(msg);
//         }
//     } catch (std::exception& e) {
//         _failed = true;
//         Logger::log(Logger::Server, e.what(), Logger::ERROR);
//         handle_fail();
//     }
// }


void AsyncSslTransport::run() {
    _thread = std::thread([&]() { thread_entry(); });
}
