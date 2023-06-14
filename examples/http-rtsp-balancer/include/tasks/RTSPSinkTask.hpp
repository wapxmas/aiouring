//
// Created by sergei on 09.08.22.
//

#ifndef HP_IO_URING_APPS_RTSPSINKTASK_HPP
#define HP_IO_URING_APPS_RTSPSINKTASK_HPP

#include <aiouring/AIOUring.h>
#include <aioutils/uexcept.h>
#include <aioutils/unet.h>
#include <aioutils/uhttp.hpp>
#include <arpa/inet.h>

#include <utility>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"
#pragma ide diagnostic ignored "UnreachableCode"

template<int BufferSize = 1048576>
class RTSPSinkTask final : public AIOUringTask {
public:
    explicit RTSPSinkTask(AIOUring *aioUring, int tcpFrom, int tcpTo,
                         std::vector<std::string> rewriteHost,
                         std::string targetName,
                         sockaddr_in client_addr,
                         std::optional<std::weak_ptr<int>> notifyfd = std::nullopt)
            : aioUring(aioUring), tcpFrom(tcpFrom), tcpTo(tcpTo),
                rewriteHost(std::move(rewriteHost)),
                targetName(std::move(targetName)),
                client_addr(client_addr),
                notifyfd(std::move(notifyfd)) {}

    TaskFuture poll(int io_result) override {
        using namespace aioutils;

        ASYNC_IO;

        tcpBufferView = std::string_view{tcpBuffer.data(), tcpBuffer.size()};

        if(!uhttp::isContentReady(tcpBufferView)) {
            AWAIT_OP(Read, readFrom, tcpFrom, buffer.data(), buffer.size());

            if(io_result < 0) {
                return TASK_ERROR(fmt::format("Error on tcp read: {}", uexcept::errnoStr(-io_result)));
            }

            if(io_result == 0) {
                // socket closed
                return TASK_RESULT_NONE();
            }

            tcpBuffer.insert(tcpBuffer.end(), buffer.begin(), buffer.begin() + io_result);

            AWAIT_POLL();
        }

        if(!rewriteHost.empty())
        {
            replaceRtspUri();
        }

        bytesToWrite = tcpBuffer.size();
        offset = 0;

        AWAIT_OP(Write, writeTo, tcpTo, tcpBuffer.data() + offset, bytesToWrite);

        if(io_result < 0) {
            return TASK_ERROR(fmt::format("Error on tcp write: {}", uexcept::errnoStr(-io_result)));
        }

        if(io_result < bytesToWrite) {
            bytesToWrite -= io_result;
            offset += io_result;
            ASYNC_CONTINUE_OP(writeTo);
        }

        tcpBuffer.clear();

        AWAIT_POLL();

        return TASK_RESULT_NONE();
    }

    void replaceRtspUri() {

        if(rewriteHost.size() != 2)
        {
            return;
        }

        auto newLinePos = tcpBufferView.find("\r\n");

        if(newLinePos == std::string_view::npos) {
            kklogging::ERROR(fmt::format("Rtsp headers: no new line found, "
                                         "but content ready. Content: {}",
                                         tcpBufferView));
            return;
        }

        auto newLineIter = std::next(tcpBuffer.begin(), newLinePos);
        auto requestHeader = std::string{tcpBuffer.begin(), newLineIter};

        std::vector<std::string> requestTokens{};
        utext::tokenizeByStr(requestHeader, requestTokens, " ", true);

        if(requestTokens.size() != 3) {
            kklogging::ERROR(fmt::format("Rtsp request is invalid, "
                                         "but content ready. Content: {}",
                                         tcpBufferView));
            return;
        }

        if(requestTokens[1].find("://") == std::string::npos) {
            kklogging::ERROR(fmt::format("Rtsp request URI is invalid, "
                                         "but content ready. Content: {}",
                                         tcpBufferView));
            return;
        }

        std::vector<std::string> uriTokens{};
        utext::tokenizeByStr(requestTokens[1], uriTokens, "/", true);

        //protocol + host + balancer marker
        if(uriTokens.size() < 2)
        {
            kklogging::ERROR(fmt::format("Rtsp request unexpected balancer URI, "
                                         "but content ready. Content: {}",
                                         tcpBufferView));
            return;
        }

        uriTokens.erase(uriTokens.cbegin());
        uriTokens.erase(uriTokens.cbegin());

        if(!uriTokens.empty() && uriTokens.at(0) == targetName)
        {
            uriTokens.erase(uriTokens.cbegin());
        }

        auto newUri = "/" + utext::join(uriTokens, "/") +
                (requestTokens[1].back() == '/' ? "/" : "");

        requestTokens[1] = fmt::format("{}//{}{}",
                                       rewriteHost.at(0),
                                       rewriteHost.at(1),
                                       newUri);

        auto newRequestHeader = utext::join(requestTokens, " ");

        std::vector<char> newTcpBuffer{};

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
    std::vector<std::string> rewriteHost{};
    std::string targetName{};
    sockaddr_in client_addr{};
    std::optional<std::weak_ptr<int>> notifyfd{};
    std::array<char, BufferSize> buffer{};
    std::vector<char> tcpBuffer{};
    int bytesToWrite{};
    int offset{};
    std::string_view tcpBufferView{};
};

#pragma clang diagnostic pop

#endif //HP_IO_URING_APPS_RTSPSINKTASK_HPP
