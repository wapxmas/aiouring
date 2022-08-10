//
// Created by sergei on 20.06.22.
//

#ifndef AIOURING_TCPLISTENINGTASK_HPP
#define AIOURING_TCPLISTENINGTASK_HPP

#include "aiouring/AIOUring.h"
#include <aioutils/uexcept.h>
#include <aioutils/unet.h>
#include <arpa/inet.h>

template<typename TAcceptTask>
requires Derived<TAcceptTask, AIOUringTask> &&
         IsFinal<TAcceptTask> && AIOUringTaskTrait<TAcceptTask>
class TCPListeningTask final : public AIOUringTask {
public:
    explicit TCPListeningTask(AIOUring *aioUring, int tcpListeningPort, int maxBacklogConnections)
            :
            aioUring(aioUring),
            tcpListeningPort(tcpListeningPort),
            maxBacklogConnections(maxBacklogConnections) {}

    TaskFuture poll(int io_result) override {
        using namespace aioutils;

        ASYNC_IO;

        tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if(tcpSocket < 0) {
            return TASK_ERROR(fmt::format("Error on creating socket: {}", uexcept::errnoStr(errno)));
        }

        unet::setSocketReuseOptions(tcpSocket);

        unet::setTcpKeepAliveCfg(tcpSocket, unet::TcpKeepAliveConfig{
                .keepidle = 10,
                .keepcnt = 5,
                .keepintvl = 1
        });

        serviceAddr.sin_family = AF_INET;
        serviceAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serviceAddr.sin_port = htons(tcpListeningPort);

        if(bind(tcpSocket, reinterpret_cast<struct
                sockaddr *>(&serviceAddr), sizeof(serviceAddr)) != 0)
        {
            return TASK_ERROR(fmt::format("Error on binding port {}: {}",
                                          tcpListeningPort, uexcept::errnoStr(errno)));
        }

        if(listen(tcpSocket, maxBacklogConnections) < 0) {
            return TASK_ERROR(fmt::format("Error on listening socket: {}",
                                          uexcept::errnoStr(errno)));
        }

        AWAIT_OP(Accept, acceptClient, tcpSocket, reinterpret_cast<struct
                sockaddr *>(&client_addr), &sockaddr_in_len);

        if(io_result < 0)
        {
            kklogging::ERROR(fmt::format("Error on accepting tcp connection: {}",
                                         uexcept::errnoStr(-io_result)));
        }

        aioUring->pushTask(aioUring->newTask<TAcceptTask>(aioUring, io_result, client_addr));

        ASYNC_CONTINUE_OP(acceptClient);

        return TASK_RESULT_NONE();
    }

private:
    AIOUring *aioUring{nullptr};
    int tcpListeningPort{-1};
    int maxBacklogConnections{-1};
    sockaddr_in serviceAddr{};
    sockaddr_in client_addr{};
    socklen_t sockaddr_in_len =
            sizeof(struct sockaddr_in);
    int tcpSocket{-1};
};
#endif //AIOURING_TCPLISTENINGTASK_HPP
