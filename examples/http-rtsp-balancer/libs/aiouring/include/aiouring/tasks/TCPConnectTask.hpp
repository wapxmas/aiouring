//
// Created by sergei on 21.06.22.
//

#ifndef AIOURING_TCPCONNECTTASK_HPP
#define AIOURING_TCPCONNECTTASK_HPP

#include "aiouring/AIOUring.h"
#include <aioutils/uexcept.h>
#include <arpa/inet.h>
#include <aioutils/unet.h>

#include "ResolveHostTask.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"
#pragma ide diagnostic ignored "UnreachableCode"

using namespace aioutils;

class TCPConnectTask final : public AIOUringTask {
public:
    using TResult = int;

    explicit TCPConnectTask(AIOUring *aioUring, std::string hostname, int tcpPort)
            : aioUring(aioUring), hostname(std::move(hostname)), tcpPort(tcpPort) {}

    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        AWAIT_TASK(resolveHostTask, hostname);

        if(TASK_HAS_ERROR(resolveHostTask)) {
            return TASK_ERROR(TASK_ERROR_TEXT(resolveHostTask));
        } else if(TASK_HAS_OPTIONAL_RESULT(resolveHostTask)) {
            resolveResult = std::move(TASK_OPTIONAL_VALUE(resolveHostTask));
        } else {
            return TASK_ERROR(fmt::format("No ip address for hostname {}", hostname));
        }

        clientAddr = std::get<0>(resolveResult);
        clientAddr.sin_port = htons(tcpPort);

        tcpSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        if(tcpSocket < 0)
        {
            return TASK_ERROR(fmt::format("Failed to create TCP socket: {}", std::string(strerror(errno))));
        }

        unet::setTcpKeepAliveCfg(tcpSocket, unet::TcpKeepAliveConfig{
                .keepidle = 10,
                .keepcnt = 5,
                .keepintvl = 1
        });

        AWAIT_OP(Connect, tcpConnect, tcpSocket, reinterpret_cast<
                struct sockaddr *>(&clientAddr), sizeof(clientAddr));

        if(io_result == 0)
        {
            return TASK_RESULT(tcpSocket);
        }

        socketErrno = io_result;

        AWAIT_OP(Close, socketClose, tcpSocket);

        return TASK_ERROR(fmt::format("Error on tcp connection: {}", uexcept::errnoStr(-socketErrno)));
    }

private:
    AIOUring *aioUring{nullptr};
    TASK_DEF(ResolveHostTask, resolveHostTask);
    std::tuple<sockaddr_in, std::string> resolveResult{};
    sockaddr_in clientAddr{};
    std::string hostname{};
    int tcpPort{};
    int tcpSocket{-1};
    int socketErrno{};
};

#pragma clang diagnostic pop

#endif //AIOURING_TCPCONNECTTASK_HPP
