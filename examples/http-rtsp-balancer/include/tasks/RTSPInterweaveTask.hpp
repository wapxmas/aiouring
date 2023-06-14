//
// Created by sergei on 09.08.22.
//

#ifndef HP_IO_URING_APPS_RTSPINTERWEAVETASK_HPP
#define HP_IO_URING_APPS_RTSPINTERWEAVETASK_HPP

#include "RTSPSinkTask.hpp"

class RTSPInterweaveTask final : public AIOUringTask {
public:
    explicit RTSPInterweaveTask(AIOUring *aioUring, int tcpFrom, int tcpTo,
                                std::vector<std::string> rewriteHost,
                                std::string targetName,
                                sockaddr_in client_addr)
            : aioUring(aioUring), tcpFrom(tcpFrom), tcpTo(tcpTo),
              rewriteHost(std::move(rewriteHost)), targetName(std::move(targetName)),
              client_addr(client_addr) {}
    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        aioUring->pushTask(aioUring->newTask<RTSPSinkTask<>>(
                aioUring, tcpFrom, tcpTo, rewriteHost,
                targetName, client_addr, this->getTaskfd()));
        aioUring->pushTask(aioUring->newTask<TCPSinkTask<>>(
                aioUring, tcpTo, tcpFrom, this->getTaskfd()));

        //wait at least one task to make work done
        AWAIT_EVENT(*this->getTaskfd().lock());

        return TASK_RESULT_NONE();
    }
private:
    AIOUring *aioUring{};
    int tcpFrom{};
    int tcpTo{};
    std::vector<std::string> rewriteHost{};
    std::string targetName{};
    sockaddr_in client_addr{};
};

#endif //HP_IO_URING_APPS_RTSPINTERWEAVETASK_HPP
