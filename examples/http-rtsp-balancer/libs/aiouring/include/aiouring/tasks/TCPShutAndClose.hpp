//
// Created by sergei on 22.06.22.
//

#ifndef AIOURING_TCPSHUTANDCLOSE_HPP
#define AIOURING_TCPSHUTANDCLOSE_HPP

#include "aiouring/AIOUring.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"
#pragma ide diagnostic ignored "UnreachableCode"

class TCPShutAndClose final : public AIOUringTask {
public:
    explicit TCPShutAndClose(std::initializer_list<int> sockets)
            : sockets{sockets} {}

    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        if(!sockets.empty()) {

            socketValue = sockets.front();

            if(socketValue >= 0) {
                AWAIT_OP(Shutdown, shutSocket, socketValue);
                AWAIT_OP(Close, closeSocket, socketValue);
            }

            sockets.pop_front();

            AWAIT_POLL();
        }

        return TASK_RESULT_NONE();
    }
private:
    std::deque<int> sockets{};
    int socketValue{-1};
};

#pragma clang diagnostic pop

#endif //AIOURING_TCPSHUTANDCLOSE_HPP
