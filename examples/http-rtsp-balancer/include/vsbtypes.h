//
// Created by sergei on 29.06.22.
//

#ifndef VSBALANCER_VSBTYPES_H
#define VSBALANCER_VSBTYPES_H

#include <vector>
#include <string>

namespace vsbtypes {

    struct BalancerRedirectsConfig {
        std::string name{};
        std::vector<std::string> targets{};
        std::string type{};
        std::vector<std::string> templates{};
    };

    struct BalancerTarget {
        std::string host{};
        int port{};
        std::string newUri{};
    };

    struct PostgresqlConn {
        std::string redirectName{};
        std::string connectionString{};
        std::string tableName{};
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PostgresqlConn, redirectName, connectionString, tableName)

    using TemplateTokens = std::vector<std::string>;

    struct BalancerRedirect {
        std::string name{};
        std::string type{};
        std::vector<BalancerTarget> targets{};
        std::vector<TemplateTokens> templates{};
        std::optional<PostgresqlConn> postgresql{std::nullopt};

        BalancerTarget getNextTarget(uint32_t iterator) {
            return targets[iterator % targets.size()];
        }
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BalancerRedirectsConfig, name, targets, type, templates)

    using KeyTargetMap = std::unordered_map<std::string, BalancerTarget>;
    using KeyTargets = std::unordered_map<std::string, KeyTargetMap>;
    using RedirectsMap = std::unordered_map<std::string, BalancerRedirect>;
}

#endif //VSBALANCER_VSBTYPES_H
