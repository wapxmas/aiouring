//
// Created by sergei on 15.06.22.
//
#ifndef AIOURINGLONGTASK_H
#define AIOURINGLONGTASK_H

#include <optional>
#include <functional>
#include <taskflow/taskflow.hpp>

#define RUN_LONG_TASK(...) \
    aioUring->executeLongTask(AIOUringLongTask { \
        .task = __VA_ARGS__     \
    })

#define AWAIT_LONG_TASK(taskName, ...) \
    ___long_task_begin_##taskName:     \
    aioUring->executeLongTask(AIOUringLongTask { \
        .task = __VA_ARGS__     ,       \
        .eventfd = *this->getTaskfd().lock()    \
    });                                 \
    AWAIT_OP(Read, taskName, *this->getTaskfd().lock(), &eventfdSink, sizeof(eventfd_t))

#define ASYNC_CONTINUE_LONG_TASK(taskName) \
    asyncStep = &&___long_task_begin_##taskName; \
    return futureOp(AIOUringOp::Nop())

struct AIOUringLongTask {
    std::optional<std::function<void(tf::Executor *executor)>> task{std::nullopt};
    std::optional<int> eventfd{std::nullopt};
};

#endif //AIOURINGLONGTASK_H
