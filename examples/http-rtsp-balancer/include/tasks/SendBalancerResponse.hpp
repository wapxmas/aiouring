//
// Created by sergei on 01.07.22.
//

#ifndef VSBALANCER_SENDBALANCERRESPONSE_HPP
#define VSBALANCER_SENDBALANCERRESPONSE_HPP

#include <aiouring/AIOUring.h>
#include <aiouring/tasks/Http404ResponseTask.hpp>
#include <aiouring/tasks/Http200ResponseTask.hpp>
#include <aiouring/tasks/HttpJsonResponseTask.hpp>
#include "vsbconfig.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"
#pragma ide diagnostic ignored "UnreachableCode"

class SendBalancerResponse final : public AIOUringTask {
public:
    explicit SendBalancerResponse(
            AIOUring *aioUring,
            std::vector<std::string> urlTokens,
            std::vector<char> tcpBuffer,
            int clientSocket)
            : aioUring(aioUring),
              urlTokens(std::move(urlTokens)),
              tcpBuffer(std::move(tcpBuffer)),
              clientSocket(clientSocket) {
        this->urlTokens.erase(this->urlTokens.begin());
        redirects = std::move(vsbconfig::getConfig().redirects);
        redirectsMap = std::move(vsbconfig::getConfig().map);
    }
    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        if(urlTokens.empty()) {
            AWAIT_TASKNL(http404ResponseTask, aioUring, clientSocket);
            return TASK_RESULT_NONE();
        }

        tcpBufferView = std::string_view{tcpBuffer.data(), tcpBuffer.size()};

        if(!uhttp::isContentReady(tcpBufferView)) {
            AWAIT_OP(Read, readClient, clientSocket, opBuffer.data(), opBuffer.size());

            if(io_result < 0)
            {
                return TASK_ERROR(fmt::format("Failed to read TCP: {}", std::string(strerror(-io_result))));
            }

            if(io_result == 0)
            {
                kklogging::INFO("SendBalancerResponse: exit due to client closure.");
                return TASK_RESULT_NONE();
            }

            tcpBuffer.insert(tcpBuffer.end(), opBuffer.begin(), opBuffer.begin() + io_result);

            AWAIT_POLL();
        }

        httpRequest = uhttp::HttpRequest{std::string{tcpBufferView}};
        httpRequest.parse();

        if(urlTokens.at(0) == "info") {
            resultJson["redirects"] = nlohmann::json(redirects);
            AWAIT_TASKNL(httpJsonResponseTask, aioUring, clientSocket, resultJson.dump());
        } else if(urlTokens.at(0) == "records" && urlTokens.size() == 3 &&
            httpRequest.getMethod() == uhttp::HttpRequestMethod::DELETE) {
            AWAIT_LONG_TASK(postgresqlUri, [this](tf::Executor *executor) {
                removeRecord(urlTokens.at(1), urlTokens.at(2));
            });
            AWAIT_TASKNL(http200ResponseTask, aioUring, clientSocket);
        } else {
            AWAIT_TASKNL(http404ResponseTask, aioUring, clientSocket);
        }

        return TASK_RESULT_NONE();
    }
    void removeRecord(const std::string& redirectName, const std::string& recordId) {
        auto redirectPair = redirectsMap.find(redirectName);

        if(redirectPair == redirectsMap.end()) {
            kklogging::WARN(fmt::format("Unknown redirect name: {}", redirectName));
            return;
        }

        auto& redirect = redirectPair->second;

        if(!redirect.postgresql.has_value()) {
            kklogging::WARN(fmt::format("No postgresql config for: {}", redirectName));
            return;
        }

        auto &pgConfig = *redirect.postgresql;

        try {
            pqxx::connection conn{pgConfig.connectionString};
            pqxx::work wrk{conn};

            wrk.exec(fmt::format("delete from {} where uri = '{}'",
                                 wrk.esc(pgConfig.tableName), wrk.esc(recordId)));

            wrk.commit();
        } catch (pqxx::sql_error const &e) {
            kklogging::ERROR(fmt::format("SQL error: {}, Query was: {}", e.what(), e.query()));
        } catch(std::exception const &e) {
            kklogging::ERROR(fmt::format("removeRecord: {}", e.what()));
        }
    }
private:
    AIOUring *aioUring{nullptr};
    TASK_DEF(Http404ResponseTask, http404ResponseTask);
    TASK_DEF(Http200ResponseTask, http200ResponseTask);
    TASK_DEF(HttpJsonResponseTask, httpJsonResponseTask);
    std::vector<std::string> urlTokens{};
    std::vector<char> tcpBuffer{};
    int clientSocket{-1};
    std::string_view tcpBufferView{};
    std::array<char, 4096> opBuffer{};
    std::vector<vsbtypes::BalancerRedirectsConfig> redirects{};
    nlohmann::json resultJson;
    uhttp::HttpRequest httpRequest{};
    vsbtypes::RedirectsMap redirectsMap{};
    vsbtypes::KeyTargets *postgresqlTargets{nullptr};
};

#pragma clang diagnostic pop

#endif //VSBALANCER_SENDBALANCERRESPONSE_HPP
