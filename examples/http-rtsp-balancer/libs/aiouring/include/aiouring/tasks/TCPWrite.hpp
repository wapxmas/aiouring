//
// Created by sergei on 30.06.22.
//

#ifndef AIOURING_TCPWRITE_HPP
#define AIOURING_TCPWRITE_HPP

#include "aiouring/AIOUring.h"
#include <aioutils/uexcept.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"
#pragma ide diagnostic ignored "UnreachableCode"

using namespace aioutils;

class TCPWrite final : public AIOUringTask {
public:
    explicit TCPWrite(int tcpSocket, char *data, size_t size) :
            tcpSocket(tcpSocket), data(data), size(size) {}
    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        if(size <= 0) {
            return TASK_RESULT_NONE();
        }

        AWAIT_OP(Write, writeTo, tcpSocket, data + offset, size);

        if(io_result < 0) {
            return TASK_ERROR(fmt::format("Error on tcp write: {}", uexcept::errnoStr(-io_result)));
        }

        if(io_result < size) {
            size -= io_result;
            offset += io_result;
            AWAIT_POLL();
        }

        return TASK_RESULT_NONE();
    }
private:
    int tcpSocket{-1};
    char *data{nullptr};
    size_t size{};
    size_t offset{};
};

#pragma clang diagnostic pop

#endif //AIOURING_TCPWRITE_HPP
