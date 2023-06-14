#ifndef AIOURING_RESOLVEHOSTTASK_HPP
#define AIOURING_RESOLVEHOSTTASK_HPP

#include <fmt/core.h>
#include "aiouring/AIOUring.h"
#include <kklogging/kklogging.h>
#include <netdb.h>
#include <arpa/inet.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-label"
#pragma ide diagnostic ignored "UnreachableCode"

class ResolveHostTask final : public AIOUringTask {
public:
    using TResult = std::optional<std::tuple<sockaddr_in, std::string>>;

    explicit ResolveHostTask(std::string hostname)
            : hostname(std::move(hostname)) {}

    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        host = static_cast<gaicb *>(calloc(1, sizeof(struct gaicb)));
        hint = static_cast<addrinfo *>(calloc(1, sizeof(struct addrinfo)));

        if(host == nullptr || hint == nullptr)
        {
            throw std::runtime_error("Out of memory");
        }

        hint[0].ai_family = AF_INET;
        hint[0].ai_socktype = SOCK_STREAM;
        hint[0].ai_protocol = IPPROTO_TCP;

        host[0].ar_name = hostname.c_str();
        host[0].ar_request = hint;

        sig.sigev_notify = SIGEV_THREAD;
        sig.sigev_value.sival_int = *this->getTaskfd().lock();
        sig.sigev_notify_function = &ResolveHostTask::NotifyTask;

        getaddrinfo_a(GAI_NOWAIT, &host, 1, &sig);

        AWAIT_EVENT(*this->getTaskfd().lock());

        int gaiRet = gai_error(host);

        if(gaiRet != 0)
        {
            return TASK_ERROR(fmt::format("GAI error, code {}: {}, {}",
                                          gaiRet, gai_strerror(gaiRet), host->ar_name));
        }

        for(addrinfo *rp = host->ar_result; rp != nullptr; rp = rp->ai_next)
        {
            if(rp->ai_family == AF_INET)
            {
                char ipAddr[INET6_ADDRSTRLEN];
                sockaddr_in sockaddrv4{};
                sockaddrv4 = *(reinterpret_cast<sockaddr_in *>(rp->ai_addr));
                const char *addrText = inet_ntop(rp->ai_family, &sockaddrv4
                        .sin_addr, ipAddr, sizeof(ipAddr));
                auto tpl = std::make_tuple(sockaddrv4, std::string{addrText});
                return TASK_RESULT(std::make_optional(tpl));
            }
        }

        return TASK_RESULT(std::nullopt);
    }

    static void NotifyTask(sigval value) {
        EVENT_NOTIFY(value.sival_int);
    }

    void free() override {
        AIOUringTask::free();
        freeaddrinfo(host->ar_result);
        ::free(reinterpret_cast<void *>(const_cast<
                addrinfo *>(host->ar_request)));
        ::free(host);
    }
private:
    std::string hostname{};
    gaicb *host{};
    addrinfo *hint{};
    sigevent sig{};
};

#pragma clang diagnostic pop

#endif //AIOURING_RESOLVEHOSTTASK_HPP
