//
// Created by sergei on 30.06.22.
//

#ifndef VSBALANCER_FINDTARGETTASK_HPP
#define VSBALANCER_FINDTARGETTASK_HPP

#include <aiouring/AIOUring.h>
#include "vsbconfig.hpp"
#include "vsbutils.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"
#pragma ide diagnostic ignored "UnreachableCode"

class FindTargetTask final : public AIOUringTask {
public:
    using TResult = std::optional<vsbtypes::BalancerTarget>;

    explicit FindTargetTask(
            AIOUring *aioUring,
            std::vector<std::string> requestTokens,
            std::vector<std::string> urlTokens,
            std::vector<std::string> queryTokens)
            :
            aioUring(aioUring),
            requestTokens(std::move(requestTokens)),
            urlTokens(std::move(urlTokens)),
            queryTokens(std::move(queryTokens))
            {
                redirectsMap = std::move(vsbconfig::getConfig().map);
            }

    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        if(urlTokens.empty()) {
            kklogging::WARN(fmt::format("No url tokens: {}", requestTokens[1]));
            return TASK_RESULT(std::nullopt);
        }

        redirectPair = redirectsMap.find(urlTokens[0]);

        if(redirectPair == redirectsMap.end()) {
            kklogging::WARN(fmt::format("Unknown redirect name: {}", urlTokens[0]));
            return TASK_RESULT(std::nullopt);
        }

        urlTokens.erase(urlTokens.begin());

        redirect = &redirectPair->second;

        if(redirect->type == "mem-uri") {
            target = getMemUriTarget();
        } else if (redirect->type == "postgresql-uri") {
            if(redirect->postgresql.has_value()) {
                AWAIT_LONG_TASK(postgresqlUri, [this](tf::Executor *executor) {
                    setPostgresqlUriTarget();
                });
            } else {
                kklogging::ERROR(fmt::format("No postgresql configuration for \"{}\" redirect.", redirect->name));
            }
        }

        if(!target.has_value()) {
            kklogging::WARN(fmt::format("No template/target has been picked for: {}", requestTokens[1]));
            return TASK_RESULT(std::nullopt);
        }

        return TASK_RESULT((std::make_optional(vsbtypes::BalancerTarget {
                .host = target->host,
                .port = target->port,
                .newUri = "/" + utext::join(urlTokens, "/") + (
                        queryTokens.size() > 1 ? fmt::format("?{}", queryTokens[1]) : "")
        })));
    }

    void setPostgresqlUriTarget() {
        if(redirect == nullptr) {
            kklogging::ERROR("redirect == nullptr");
            return;
        }

        auto uniqueKey = vsbutils::getUniqueUriParts(
                redirect->templates, urlTokens);

        if(!uniqueKey.has_value()) {
            kklogging::WARN(fmt::format("No unique key for {}", requestTokens[1]));
            return;
        }

        auto &pgConfig = *redirect->postgresql;

        try {
            pqxx::connection conn{pgConfig.connectionString};

            {
                pqxx::work wrk{conn};
                pqxx::result r{};

                r = {wrk.exec(fmt::format("select host from {} where uri = '{}'",
                                          wrk.esc(pgConfig.tableName), wrk.esc(*uniqueKey)))};

                wrk.commit();

                if(!r.empty()) {
                    setTargetFromHostString(r.front()["host"].c_str(), *uniqueKey, redirect->name);
                }
            }

            if(!target.has_value()) {
                pqxx::work wrk{conn};
                pqxx::result r{};

                std::vector<std::string> hosts{};

                std::transform(redirect->targets.begin(), redirect->targets.end(), std::back_inserter(hosts),
                               [&](vsbtypes::BalancerTarget &tg) {
                                   return fmt::format("'{}'", wrk.esc(fmt::format("{}:{}", tg.host, tg.port)));
                               });

                std::string hostsStr = utext::join(hosts, ", ");

                r = {wrk.exec(fmt::format(
                        "insert into {0} values(default, '{1}', (select host from (select host, sum(loadf) as loadf_sum from ("
                        "    select host, count(host) as loadf from {0} as fristu where host IN ({2}) group by host"
                        "    union all"
                        "    select host, 0 as loadf from unnest(ARRAY[{2}]::text[]) as host"
                        ") x group by x.host order by loadf_sum limit 1) x)) on conflict (uri) do update set uri = EXCLUDED.uri returning host"
                        , wrk.esc(pgConfig.tableName), wrk.esc(*uniqueKey), hostsStr))};

                wrk.commit();

                if(!r.empty()) {
                    setTargetFromHostString(r.front()["host"].c_str(), *uniqueKey, redirect->name);
                }
            }
        } catch (pqxx::sql_error const &e) {
            kklogging::ERROR(fmt::format("SQL error: {}, Query was: {}", e.what(), e.query()));
        } catch(std::exception const &e) {
            kklogging::ERROR(fmt::format("setPostgresqlUriTarget: {}", e.what()));
        }
    }

    void setTargetFromHostString(const std::string& host,
                                 const std::string& key,
                                 const std::string& redirectName) {

        std::vector<std::string> tokens{};

        utext::tokenizeByStr(host, tokens, ":", true);

        if(tokens.size() != 2) {
            throw std::runtime_error(fmt::format("No host tokens in {}.", host));
        }

        auto balancerTarget = vsbtypes::BalancerTarget {
                .host = tokens[0],
                .port = std::stoi(tokens[1])
        };

        target = std::make_optional(balancerTarget);
    }

    std::optional<vsbtypes::BalancerTarget> getMemUriTarget() {
        static vsbtypes::KeyTargets keyTargets{};
        static uint32_t memIterator{0};

        if(redirect == nullptr) {
            kklogging::ERROR("redirect == nullptr");
            return std::nullopt;
        }

        auto uniqueKey = vsbutils::getUniqueUriParts(
                redirect->templates, urlTokens);

        if(uniqueKey.has_value()) {
            auto nameRedirectIter = keyTargets.find(redirect->name);

            if(nameRedirectIter == keyTargets.end()) {
                keyTargets.insert({redirect->name, {}});
                nameRedirectIter = keyTargets.find(redirect->name);
            }

            auto &nameRedirect = nameRedirectIter->second;

            auto keyRedirectIter = nameRedirect.find(*uniqueKey);

            if(keyRedirectIter == nameRedirect.end()) {
                nameRedirect.insert({*uniqueKey, redirect->getNextTarget(memIterator++)});
                keyRedirectIter = nameRedirect.find(*uniqueKey);
            }

            return std::make_optional(keyRedirectIter->second);
        } else {
            kklogging::WARN(fmt::format("No unique key for {}", requestTokens[1]));
            return std::nullopt;
        }
    }

private:
    AIOUring *aioUring{};
    vsbtypes::RedirectsMap redirectsMap{};
    vsbtypes::RedirectsMap::iterator redirectPair{};
    std::vector<std::string> requestTokens{};
    std::vector<std::string> urlTokens{};
    std::vector<std::string> queryTokens{};
    vsbtypes::BalancerRedirect *redirect{};
    std::optional<vsbtypes::BalancerTarget> target{std::nullopt};
};

#pragma clang diagnostic pop

#endif //VSBALANCER_FINDTARGETTASK_HPP
