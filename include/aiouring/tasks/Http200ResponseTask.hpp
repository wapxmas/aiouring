//
// Created by sergei on 15.07.22.
//

#ifndef AIOURING_HTTP200RESPONSETASK_HPP
#define AIOURING_HTTP200RESPONSETASK_HPP

#include "aiouring/AIOUring.h"
#include <aioutils/uhttp.hpp>
#include "TCPWrite.hpp"

using namespace aioutils;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"
#pragma ide diagnostic ignored "UnreachableCode"

class Http200ResponseTask final : public AIOUringTask {
public:
    explicit Http200ResponseTask(AIOUring *aioUring, int clientSocket) :
            aioUring(aioUring), clientSocket(clientSocket) {}
    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        httpResponse = uhttp::HttpResponse{uhttp::HttpResponse200Text};
        httpResponse.parse();

        responseData = httpResponse.toRaw();

        AWAIT_TASKNL(tcpWrite, clientSocket, responseData.data(), responseData.size());

        return TASK_RESULT_NONE();
    }
private:
    AIOUring *aioUring{nullptr};
    TASK_DEF(TCPWrite, tcpWrite);
    int clientSocket{-1};
    uhttp::HttpResponse httpResponse{};
    uhttp::HttpResponse::DataType responseData{};
};

#pragma clang diagnostic pop

#endif //AIOURING_HTTP200RESPONSETASK_HPP
