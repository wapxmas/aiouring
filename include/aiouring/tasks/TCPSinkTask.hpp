//
// Created by sergei on 21.06.22.
//

#ifndef AIOURING_TCPSINKTASK_HPP
#define AIOURING_TCPSINKTASK_HPP

#include "aiouring/AIOUring.h"
#include <aioutils/uexcept.h>
#include <aioutils/unet.h>
#include <arpa/inet.h>
#include <utility>

using namespace aioutils;

template<int BufferSize = 1048576>
class TCPSinkTask final : public AIOUringTask {
public:
    explicit TCPSinkTask(AIOUring *aioUring, int tcpFrom, int tcpTo,
                         std::optional<std::weak_ptr<int>> notifyfd = std::nullopt)
            : aioUring(aioUring), tcpFrom(tcpFrom),
              tcpTo(tcpTo), notifyfd(std::move(notifyfd)) {}

    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        AWAIT_OP(Read, readFrom, tcpFrom, buffer.data(), buffer.size());

        if(io_result < 0) {
            return TASK_ERROR(fmt::format("Error on tcp read: {}", uexcept::errnoStr(-io_result)));
        }

        if(io_result == 0) {
            // socket closed
            return TASK_RESULT_NONE();
        }

        bytesToWrite = io_result;
        offset = 0;

        AWAIT_OP(Write, writeTo, tcpTo, buffer.data() + offset, bytesToWrite);

        if(io_result < 0) {
            return TASK_ERROR(fmt::format("Error on tcp write: {}", uexcept::errnoStr(-io_result)));
        }

        if(io_result < bytesToWrite) {
            bytesToWrite -= io_result;
            offset += io_result;
            ASYNC_CONTINUE_OP(writeTo);
        }

        ASYNC_CONTINUE_OP(readFrom);

        return TASK_RESULT_NONE();
    }

    TaskFuture finally(int io_result) override {
        if(notifyfd.has_value()) {
            if(auto eventFd = notifyfd->lock()) {
                EVENT_NOTIFY(*eventFd);
            }
        }
        return TASK_RESULT_NONE();
    }
private:
    AIOUring *aioUring{};
    int tcpFrom{};
    int tcpTo{};
    std::optional<std::weak_ptr<int>> notifyfd{};
    std::array<char, BufferSize> buffer{};
    int bytesToWrite{};
    int offset{};
};

#endif //AIOURING_TCPSINKTASK_HPP
