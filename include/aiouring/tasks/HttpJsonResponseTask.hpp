//
// Created by sergei on 01.07.22.
//

#ifndef AIOURING_HTTPJSONRESPONSETASK_HPP
#define AIOURING_HTTPJSONRESPONSETASK_HPP

#include "aiouring/AIOUring.h"
#include <aioutils/uhttp.hpp>
#include "TCPWrite.hpp"

using namespace aioutils;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"
#pragma ide diagnostic ignored "UnreachableCode"

class HttpJsonResponseTask final : public AIOUringTask {
public:
    explicit HttpJsonResponseTask(AIOUring *aioUring, int clientSocket, std::string json) :
            aioUring(aioUring), clientSocket(clientSocket), json(std::move(json)) {}
    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        httpResponse = uhttp::HttpResponse{uhttp::HttpResponse200Text};
        httpResponse.parse();
        httpResponse.setContentType(uhttp::ContentTypeJson);
        httpResponse.setContent(json);

        responseData = httpResponse.toRaw();

        AWAIT_TASKNL(tcpWrite, clientSocket, responseData.data(), responseData.size());

        return TASK_RESULT_NONE();
    }
private:
    AIOUring *aioUring{nullptr};
    TASK_DEF(TCPWrite, tcpWrite);
    int clientSocket{-1};
    std::string json{};
    uhttp::HttpResponse httpResponse{};
    uhttp::HttpResponse::DataType responseData{};
};

#pragma clang diagnostic pop

#endif //AIOURING_HTTPJSONRESPONSETASK_HPP
