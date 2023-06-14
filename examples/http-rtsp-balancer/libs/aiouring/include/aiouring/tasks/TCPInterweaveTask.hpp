//
// Created by sergei on 21.06.22.
//

#ifndef AIOURING_TCPINTERWEAVETASK_HPP
#define AIOURING_TCPINTERWEAVETASK_HPP

#include "TCPSinkTask.hpp"

class TCPInterweaveTask final : public AIOUringTask {
public:
    explicit TCPInterweaveTask(AIOUring *aioUring, int tcpFrom, int tcpTo)
            : aioUring(aioUring), tcpFrom(tcpFrom), tcpTo(tcpTo) {}
    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        aioUring->pushTask(aioUring->newTask<TCPSinkTask<>>(
                aioUring, tcpFrom, tcpTo, this->getTaskfd()));
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
};

#endif //AIOURING_TCPINTERWEAVETASK_HPP
