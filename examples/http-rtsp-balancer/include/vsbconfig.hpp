//
// Created by sergei on 19.06.22.
//

#ifndef VS_BALANCER_VSCONFIG_HPP
#define VS_BALANCER_VSCONFIG_HPP

#include <memory>
#include <aioutils/uenv.h>
#include <aioutils/utext.h>
#include <filesystem>
#include <pqxx/pqxx>
#include "nlohmann/json.hpp"
#include "vsbtypes.h"

using namespace aioutils;

namespace vsbconfig {

    const char *configurationFilePath = "etc/vs-balancer.json";
    const char *envPrefix = "VS_BALANCER";

    struct MainConfiguration {
        int port{1558};
        int maxBacklog{128};
        std::vector<vsbtypes::BalancerRedirectsConfig> redirects{};
        std::vector<vsbtypes::PostgresqlConn> postgresql{};
        vsbtypes::RedirectsMap map{};

        static MainConfiguration& instance() {
            static std::unique_ptr<MainConfiguration> config = std::make_unique<MainConfiguration>();
            return *config;
        }
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MainConfiguration, port, maxBacklog, redirects, postgresql)

    inline MainConfiguration getConfig() {
        return MainConfiguration::instance();
    }

    static void checkType(std::string basicString);

    static void checkTemplates(std::vector<std::string> vector1);

    static void checkTargets(const std::vector<std::string>& vector1);

    static void fillRedirect(vsbtypes::BalancerRedirectsConfig &rc, MainConfiguration &config);

    static void migrateDatabase(MainConfiguration &config);

    static void migratePostgresql(vsbtypes::PostgresqlConn &conn);

    static void rewriteWithEnvironment(MainConfiguration &config);

    static void initConfig() {

        auto &config = MainConfiguration::instance();

        if(!std::filesystem::exists(configurationFilePath)) {
            throw std::runtime_error("Configuration file doesn't exist.");
        }

        std::ifstream t(configurationFilePath);
        std::stringstream buffer;
        buffer << t.rdbuf();

        if(buffer.str().empty()) {
            throw std::runtime_error("configuration file is empty");
        }

        auto jsonConfig = nlohmann::json::parse(buffer.str());

        config = jsonConfig.get<MainConfiguration>();

        rewriteWithEnvironment(config);

        for(auto &r : config.redirects) {
            if(r.name.empty()) {
                throw std::runtime_error("redirect name is empty");
            } else if(r.type.empty()) {
                throw std::runtime_error("redirect type is empty");
            } else if(r.templates.empty()) {
                throw std::runtime_error("redirect templates is empty");
            } else if(r.targets.empty()) {
                throw std::runtime_error("redirect targets is empty");
            }
            checkType(r.type);
            checkTemplates(r.templates);
            checkTargets(r.targets);
            fillRedirect(r, config);
        }

        migrateDatabase(config);
    }

    static void rewriteWithEnvironment(MainConfiguration &config) {
        uenv::setVariableFromEnvironment(fmt::format("{}_PORT", envPrefix), config.port, true);
        uenv::setVariableFromEnvironment(fmt::format("{}_MAX_BACKLOG", envPrefix), config.maxBacklog, true);

        for(auto &r : config.redirects) {
            std::string targetsStr{};
            uenv::setVariableFromEnvironment(fmt::format("{}_{}_TARGETS", envPrefix, r.name), targetsStr);
            utext::trim(targetsStr);
            if(!targetsStr.empty()) {
                r.targets.clear();
                utext::tokenizeByStr(targetsStr, r.targets, ",", true);
                for(auto &t : r.targets) {
                    utext::trim(t);
                }
            }
        }

        for(auto &p : config.postgresql) {
            std::string connectionString{};
            uenv::setVariableFromEnvironment(fmt::format("{}_{}_PGCONN", envPrefix, p.redirectName), connectionString);
            utext::trim(connectionString);
            if(!connectionString.empty()) {
                p.connectionString = connectionString;
            }
        }
    }

    static void migrateDatabase(MainConfiguration &config) {
        for(auto &red : config.map) {
            if(red.second.postgresql.has_value()) {
                migratePostgresql(*red.second.postgresql);
            }
        }
    }

    static void migratePostgresql(vsbtypes::PostgresqlConn &conn) {
        try
        {
            pqxx::connection c{conn.connectionString};

            {
                pqxx::work wrk{c};
                wrk.exec(fmt::format("create table if not exists {0}\n"
                                     "(\n"
                                     "    id   uuid default gen_random_uuid() not null\n"
                                     "        constraint {0}_pk\n"
                                     "            primary key,\n"
                                     "    uri  text                           not null\n"
                                     "        unique,\n"
                                     "    host text                           not null\n"
                                     ")", wrk.esc(conn.tableName)));
                wrk.commit();
            }

            {
                pqxx::work wrk{c};
                wrk.exec(fmt::format("create index if not exists {0}_host\n"
                                     "    on {0} (host);",
                                     wrk.esc(conn.tableName)));
                wrk.commit();
            }

            kklogging::INFO(fmt::format("Postgresql migration for \"{}\" redirect is done.", conn.redirectName));
        }
        catch (pqxx::sql_error const &e)
        {
            throw std::runtime_error(fmt::format("SQL error: {}, Query was: {}", e.what(), e.query()));
        }
    }

    static void fillRedirect(vsbtypes::BalancerRedirectsConfig &rc, MainConfiguration &config) {
        std::vector<vsbtypes::BalancerTarget> targets{};
        std::vector<vsbtypes::TemplateTokens> templates{};

        for(auto &target : rc.targets) {
            std::vector<std::string> tokens{};

            utext::tokenizeByStr(target, tokens, ":");

            targets.push_back(vsbtypes::BalancerTarget {
                .host = tokens.at(0),
                .port = std::stoi(tokens.at(1))
            });
        }

        for(auto &tpl : rc.templates) {
            std::vector<std::string> tokens{};
            utext::tokenizeByStr(tpl, tokens, "/", true);
            if(tokens.empty()) {
                throw std::runtime_error(fmt::format("No tokens for template: {}", tpl));
            }
            templates.push_back(tokens);
        }

        if(config.map.find(rc.name) != config.map.end()) {
            throw std::runtime_error(fmt::format("redirect {} already exists", rc.name));
        }

        std::optional<vsbtypes::PostgresqlConn> postgresql{std::nullopt};

        auto connIt = std::find_if(
                config.postgresql.begin(),
                config.postgresql.end(),
                [&](const vsbtypes::PostgresqlConn &conn) {
            return conn.redirectName == rc.name;
        });

        if(connIt != config.postgresql.end()) {
            postgresql = std::make_optional<vsbtypes::PostgresqlConn>(*connIt);
        }

        config.map.insert({rc.name, vsbtypes::BalancerRedirect {
            .name = rc.name,
            .type = rc.type,
            .targets = std::move(targets),
            .templates = std::move(templates),
            .postgresql = postgresql
        }});
    }

    static void checkTargets(const std::vector<std::string>& targets) {
        for(auto &target : targets) {
            std::vector<std::string> tokens{};

            if(target.empty()) {
                throw std::runtime_error("target must not be empty.");
            }

            utext::tokenizeByStr(target, tokens, ":");

            if(tokens.size() != 2) {
                throw std::runtime_error("incorrect format of target, should be host:port");
            }

            int port{};

            try {
                port = std::stoi(tokens.at(1));
            } catch (...) {
                throw std::runtime_error(fmt::format("target port must be a number: {}", target));
            }

            if(port <= 0) {
                throw std::runtime_error(fmt::format("target port must be > 0: {}", target));
            }
        }
    }

    static void checkTemplates(std::vector<std::string> templates) {
        for(auto &tpl : templates) {
            if(tpl.empty()) {
                throw std::runtime_error("template must not be empty.");
            }
            if(tpl.at(0) != '/') {
                throw std::runtime_error("template must start with /");
            }
        }
    }

    static void checkType(std::string type) {
        const static std::unordered_set<std::string> allowed = {"mem-uri", "postgresql-uri"};

        if(allowed.find(type) == allowed.end()) {
            throw std::runtime_error(fmt::format("type {} is not allowed. Allowed types are {}.",
                                                 type, utext::join(std::vector<std::string>(
                                                         allowed.begin(), allowed.end()), ", ")));
        }
    }
}

#endif //VS_BALANCER_VSCONFIG_HPP