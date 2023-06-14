//
// Created by sergei on 30.06.22.
//

#ifndef VSBALANCER_BALANCERACCEPTTASK_HPP
#define VSBALANCER_BALANCERACCEPTTASK_HPP

#include <aiouring/AIOUring.h>
#include <aiouring/tasks/TCPListeningTask.hpp>
#include <aiouring/tasks/TCPConnectTask.hpp>
#include <aiouring/tasks/TCPInterweaveTask.hpp>
#include <aiouring/tasks/TCPShutAndClose.hpp>
#include <utility>
#include <aioutils/uhttp.hpp>

#include "tasks/FindTargetTask.hpp"
#include "tasks/SendBalancerResponse.hpp"
#include "tasks/RTSPInterweaveTask.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"
#pragma ide diagnostic ignored "UnreachableCode"

class BalancerAcceptTask final : public AIOUringTask {
public:
    explicit BalancerAcceptTask(AIOUring *aioUring, int clientSocket, sockaddr_in client_addr)
            : aioUring(aioUring), clientSocket(clientSocket), client_addr(client_addr) {}
    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        AWAIT_OP(Read, readClient, clientSocket, opBuffer.data(), opBuffer.size());

        if(io_result < 0)
        {
            return TASK_ERROR(fmt::format("Failed to read TCP: {}", std::string(strerror(-io_result))));
        }

        if(io_result == 0)
        {
            kklogging::INFO("BalancerAcceptTask: exit due to client closure.");
            return TASK_RESULT_NONE();
        }

        tcpBuffer.insert(tcpBuffer.end(), opBuffer.begin(), opBuffer.begin() + io_result);

        tcpBufferView = std::string_view{tcpBuffer.begin(), tcpBuffer.end()};

        newLinePos = tcpBufferView.find("\r\n");

        if(newLinePos == std::string_view::npos) {
            ASYNC_CONTINUE_OP(readClient);
        }

        newLineIter = std::next(tcpBuffer.begin(), newLinePos);

        requestHeader = std::string{tcpBuffer.begin(), newLineIter};

        utext::tokenizeByStr(requestHeader, requestTokens, " ", true);

        if(requestTokens.size() != 3) {
            kklogging::WARN(fmt::format("Unknown protocol header: {}", requestHeader));
            return TASK_RESULT_NONE();
        }

        utext::tokenizeByStr(requestTokens[1], queryTokens, "?", true);
        utext::tokenizeByStr(queryTokens[0], urlTokens, "/", true);

        if(urlTokens.size() > 1 && urlTokens.at(0).find(':') != std::string::npos) {
            hostTokens.push_back(urlTokens.at(0));
            hostTokens.push_back(urlTokens.at(1));
            urlTokens.erase(urlTokens.begin());
            urlTokens.erase(urlTokens.begin());
        }

        if(!urlTokens.empty() && urlTokens.at(0) == "balancer") {
            AWAIT_TASKNL(sendBalancerResponse, aioUring, urlTokens,
                         tcpBuffer, clientSocket);
            return TASK_RESULT_NONE();
        }

        AWAIT_TASK(findTargetTask, aioUring, requestTokens, urlTokens, queryTokens);

        if(TASK_HAS_ERROR(findTargetTask)) {

            kklogging::ERROR(fmt::format("Error on targeting: {}",
                                         TASK_ERROR_TEXT(findTargetTask)));
            return TASK_RESULT_NONE();
        }

        if(!TASK_HAS_OPTIONAL_RESULT(findTargetTask)) {

            kklogging::ERROR(fmt::format("No target found for {}", requestHeader));

            return TASK_RESULT_NONE();
        }

        tcpTarget = TASK_OPTIONAL_VALUE(findTargetTask);

        replaceUri(hostTokens.empty() ? tcpTarget.newUri :
            fmt::format("{}//{}{}",
                        hostTokens.at(0),
                        hostTokens.at(1),
                        tcpTarget.newUri));

        AWAIT_TASK(tcpConnectTask, aioUring, tcpTarget.host, tcpTarget.port);

        if(TASK_HAS_ERROR(tcpConnectTask)) {

            kklogging::ERROR(fmt::format("Error on connection: {}",
                                         TASK_ERROR_TEXT(tcpConnectTask)));
            return TASK_RESULT_NONE();
        }

        if(!TASK_HAS_RESULT(tcpConnectTask))
        {
            kklogging::ERROR("No socket on connection.");
            return TASK_RESULT_NONE();
        }

        targetSocket = TASK_RESULT_VALUE(tcpConnectTask);

        AWAIT_TASKNL(tcpWrite, targetSocket, tcpBuffer.data(), tcpBuffer.size());

        if(TASK_HAS_ERROR(tcpWrite)) {
            kklogging::ERROR(fmt::format("Socket write error: {}", TASK_ERROR_TEXT(tcpWrite)));
        }

        tcpBuffer.clear();

        if(hostTokens.empty()) {
            // http protocol
            AWAIT_TASK(tcpInterweaveTask, aioUring, clientSocket, targetSocket);
        } else {
            // rtsp protocol (rewrite uri)
            AWAIT_TASK(rtspInterweaveTask, aioUring, clientSocket, targetSocket,
                       hostTokens, urlTokens.at(0), client_addr);
        }

        return TASK_RESULT_NONE();
    }

    TaskFuture finally(int io_result) override {
        ASYNC_IO;

        AWAIT_TASKNL(tcpShutAndClose, clientSocket, targetSocket);

        return TASK_RESULT_NONE();
    }

    void replaceUri(std::string newUri) {
        std::vector<char> newTcpBuffer{};
        std::vector<std::string> tokens{};

        utext::tokenizeByStr(requestHeader, tokens);

        if(tokens.size() != 3) {
            kklogging::ERROR(fmt::format("replaceUri: invalid tokens for {}", requestHeader));
            return;
        }

        tokens[1] = std::move(newUri);

        auto newRequestHeader = utext::join(tokens, " ");

        newTcpBuffer.insert(newTcpBuffer.end(),
                            std::make_move_iterator(newRequestHeader.begin()),
                            std::make_move_iterator(newRequestHeader.end()));

        auto xff = fmt::format("\r\nX-Forwarded-For: {}", inet_ntoa(client_addr.sin_addr));

        newTcpBuffer.insert(newTcpBuffer.end(),
                            std::make_move_iterator(xff.begin()),
                            std::make_move_iterator(xff.end()));

        newTcpBuffer.insert(newTcpBuffer.end(),
                            std::make_move_iterator(newLineIter),
                            std::make_move_iterator(tcpBuffer.end()));
        tcpBuffer = std::move(newTcpBuffer);
    }
private:
    AIOUring *aioUring{nullptr};
    int clientSocket{-1};
    sockaddr_in client_addr{};
    TASK_DEF(TCPConnectTask, tcpConnectTask);
    TASK_DEF(TCPInterweaveTask, tcpInterweaveTask);
    TASK_DEF(RTSPInterweaveTask, rtspInterweaveTask);
    TASK_DEF(TCPShutAndClose, tcpShutAndClose);
    TASK_DEF(TCPWrite, tcpWrite);
    TASK_DEF(FindTargetTask, findTargetTask);
    TASK_DEF(SendBalancerResponse, sendBalancerResponse);
    int targetSocket{-1};
    std::array<char, 4096> opBuffer{};
    std::vector<char> tcpBuffer{};
    std::vector<char>::iterator newLineIter{};
    std::string_view tcpBufferView{};
    std::string_view::size_type newLinePos{};
    std::string requestHeader{};
    vsbtypes::BalancerTarget tcpTarget{};
    std::vector<std::string> requestTokens{};
    std::vector<std::string> urlTokens{};
    std::vector<std::string> queryTokens{};
    std::vector<std::string> hostTokens{};
};

#pragma clang diagnostic pop

#endif //VSBALANCER_BALANCERACCEPTTASK_HPP
