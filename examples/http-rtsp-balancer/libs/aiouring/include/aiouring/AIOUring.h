//
// Created by sergei on 07.06.22.
//

#ifndef AIOURING_H
#define AIOURING_H

#include <atomic>
#include <liburing.h>
#include <fmt/core.h>
#include <aioutils/ulinux.h>
#include <aioutils/uexcept.h>
#include <kklogging/kklogging.h>
#include <thread>
#include <taskflow/taskflow.hpp>
#include <sys/eventfd.h>
#include <tuple>

#include "AIOUringTask.h"
#include "AIOUringLongTask.h"

class AIOUringException : public std::exception {
public:
    explicit AIOUringException(std::string message)
            : message(std::move(message))
    {

    }
    [[nodiscard]] const char* what() const noexcept override {
        return message.data();
    }
private:
    std::string message;
};

class AIOUring {
public:
    explicit AIOUring(std::optional<int> iouringBackend = std::nullopt, bool useSQPoll = false);

    [[nodiscard]] int getInstanceId() const;

    void setup();
    int run();
    void executeLongTask(AIOUringLongTask longTask);

    template<typename T, typename... Args>
    requires Derived<T, AIOUringTask> && IsFinal<T> && AIOUringTaskTrait<T>
    T* newTask(Args... args);

    template<Derived<AIOUringTask> T>
    requires AIOUringTaskTrait<T>
    void freeTask(T* task);

    template<Derived<AIOUringTask> T>
    requires AIOUringTaskTrait<T>
    void pushTask(T* task);

private:
    inline static std::atomic<int> idGenerator{0};
    io_uring_params params{};
    io_uring ring{};
    bool setupPassed{false};
    int instanceId{-1};
    std::optional<int> iouringBackend{std::nullopt};
    bool useSQPoll{false};
    tf::Executor executor{};

    std::tuple<bool, int> processCQE(io_uring_cqe *cqe);
};

#include "AIOUring.tpp"

#endif //AIOURING_H
