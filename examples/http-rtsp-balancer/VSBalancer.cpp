#include <mimalloc-new-delete.h>
#include <fmt/core.h>
#include <aiouring/AIOUring.h>
#include <kklogging/kklogging.h>
#include <aioutils/uexcept.h>
#include <aioutils/utext.h>
#include "vsbconfig.hpp"
#include "VSBalancer.h"
#include "tasks/BalancerAcceptTask.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"
#pragma ide diagnostic ignored "UnreachableCode"

class MainTask final : public AIOUringTask {
public:
    explicit MainTask(AIOUring *aioUring)
        : aioUring(aioUring) {
        config = vsbconfig::getConfig();
    }

    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        AWAIT_TASK(tcpListeningTask,
                   aioUring,
                   config.port,
                   config.maxBacklog);

        if(TASK_HAS_ERROR(tcpListeningTask)) {
            kklogging::ERROR(fmt::format("TCPListeningTask: {}", TASK_ERROR_TEXT(tcpListeningTask)));
            AIOURING_SHUTDOWN(1);
        }

        kklogging::WARN("TCPListeningTask completed successfully.");

        return TASK_RESULT_NONE();
    }
private:
    AIOUring *aioUring{nullptr};
    vsbconfig::MainConfiguration config{};
    TASK_DEF(TCPListeningTask<BalancerAcceptTask>, tcpListeningTask);
};

bool configure();

int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGXCPU, SIG_IGN);

    if(!configure()) {
        return EXIT_FAILURE;
    }

    try
    {
        AIOUring aioUring{};

        aioUring.setup();

        aioUring.pushTask(aioUring.newTask<MainTask>(&aioUring));

        return aioUring.run();
    }
    catch(std::exception &e)
    {
        kklogging::ERROR("Uring: " + std::string(e.what()));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

bool configure() {
    kklogging::INFO(VSBalancer_DescVer);

    try {
        vsbconfig::initConfig();
    } catch(std::exception &e) {
        kklogging::ERROR(fmt::format("configure: {}", e.what()));
        return false;
    }

    auto config = vsbconfig::getConfig();

    kklogging::INFO("Defaults have been set to: ");
    kklogging::INFO(fmt::format("port={}", config.port));
    kklogging::INFO(fmt::format("maxBacklog={}", config.maxBacklog));
    kklogging::INFO("Redirects:");
    for(auto &r : config.redirects) {
        kklogging::INFO(fmt::format("name: {}", r.name));
        kklogging::INFO(fmt::format("targets: {}", aioutils::utext::join(r.targets, ", ")));
        kklogging::INFO(fmt::format("type: {}", r.type));
        kklogging::INFO(fmt::format("templates: {}", aioutils::utext::join(r.templates, ", ")));
        kklogging::INFO("----------");
    }
    kklogging::INFO("Postgresql:");
    for(auto &p : config.postgresql) {
        kklogging::INFO(fmt::format("redirect name: {}", p.redirectName));
        kklogging::INFO(fmt::format("table name: {}", p.tableName));
        kklogging::INFO("----------");
    }
    return true;
}

#pragma clang diagnostic pop