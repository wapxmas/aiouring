//
// Created by sergei on 06.06.22.
//

#ifndef AIOURINGTASK_H
#define AIOURINGTASK_H

#include <concepts>
#include <type_traits>
#include <exception>
#include <string>
#include <optional>
#include <unistd.h>
#include <sys/eventfd.h>
#include <any>
#include <tuple>
#include <liburing.h>
#include <variant>
#include <memory>

#include "AIOUringOp.h"
#include "AIOUringLongTask.h"

#define ASYNC_IO \
    constexpr bool ___async_function{true}; \
    if(asyncStep != nullptr) \
    { \
        goto *this->asyncStep; \
    }

#define ASYNC_RESET() \
    if(___async_function) { \
        this->asyncReset(); \
    }

#define AWAIT_OP_NL(opName, ...) \
    if(___async_function) {      \
        return futureOp(AIOUringOp::opName(__VA_ARGS__)); \
    }

#define AWAIT_OP(opName, labelName, ...) \
    ___op_begin_##labelName:                   \
    asyncStep = &&___op_end_##labelName;       \
    AWAIT_OP_NL(opName, ## __VA_ARGS__)  \
    ___op_end_##labelName:

#define AWAIT_CONTINUE_OP(labelName) \
    goto ___op_begin_##labelName

#define ASYNC_CONTINUE_OP(labelName) \
    asyncStep = &&___op_begin_##labelName; \
    return futureOp(AIOUringOp::Nop());

#define TASK_RESULT(result) \
    futureResult(std::make_any<TResult>((result)))

#define TASK_RESULT_NONE() \
    TASK_RESULT(std::monostate{})

#define TASK_ERROR(error_text) \
    futureResult(std::make_any<std::runtime_error>(error_text))

#define TASK_HAS_OPTIONAL_RESULT(taskName) \
    (taskName##_result.has_value() && \
        (*taskName##_result).has_value())

#define TASK_HAS_RESULT(taskName) \
    (taskName##_result.has_value())

#define TASK_RESULT_VALUE(taskName) \
    (*taskName##_result)

#define TASK_OPTIONAL_VALUE(taskName) \
    (*taskName##_result).value()

#define TASK_HAS_ERROR(taskName) \
    taskName##_error.has_value()

#define TASK_ERROR_TEXT(taskName) \
    (*taskName##_error).what()

#define AIOURING_SHUTDOWN(code) \
    return futureOp(AIOUringOp::ShutdownUring(code))

#define TASK_DEF(task_class, taskName) \
    std::optional<task_class::TResult> taskName##_result{std::nullopt}; \
    std::optional<std::runtime_error> taskName##_error{std::nullopt};\
    TaskFuture ___task_future_##taskName{};                          \
    std::optional<AIOUringOp> ___task_ok_##taskName{std::nullopt};    \
    std::optional<std::any> ___task_result_##taskName{std::nullopt}; \
    task_class *taskName{nullptr}

#define AWAIT_TASKNL2(taskName, lbSuffix, ...) \
    AWAIT_TASK_INT(taskName, lbSuffix, __VA_ARGS__)

#define AWAIT_TASKNL(taskName, ...) \
    AWAIT_TASKNL2(taskName, __COUNTER__, __VA_ARGS__)

#define AWAIT_TASK(taskName, ...) \
    AWAIT_TASK_INT(taskName, , __VA_ARGS__)

#define AWAIT_TASK_INT(taskName, lbSuffix, ...) \
    ___task_create_##taskName##lbSuffix:    \
    if(___async_function) {       \
        if(aioUring == nullptr) {  \
            throw std::runtime_error(fmt::format("{}: aioUring is null", this->getClassName())); \
        }                         \
        this->taskName = aioUring->newTask<std::remove_pointer_t<decltype(taskName)>>(__VA_ARGS__); \
    }                             \
    ___task_begin_##taskName##lbSuffix:     \
    ___task_future_##taskName = this->taskName->poll(io_result);                                \
    ___task_ok_##taskName = std::get<0>(___task_future_##taskName);                             \
    if(___task_ok_##taskName.has_value()) {                                                     \
        asyncStep = &&___task_begin_##taskName##lbSuffix;                                                 \
        return ___task_future_##taskName;                                                       \
    } else {                      \
        aioUring->freeTask(this->taskName);                                                      \
        this->taskName = nullptr; \
    }                             \
    ___task_result_##taskName = std::get<1>(___task_future_##taskName);                         \
    taskName##_result = std::nullopt;                                                           \
    taskName##_error = std::nullopt;                                                            \
    if(___task_result_##taskName.has_value()) {                                                 \
        if(!std::is_same<typename std::remove_pointer_t<decltype(taskName)>::TResult, std::monostate>()) {  \
            try {                 \
                taskName##_result = std::make_optional<typename std::remove_pointer_t<decltype(taskName)>::TResult>( \
                    std::any_cast<typename std::remove_pointer_t<decltype(taskName)>::TResult>(*___task_result_##taskName)); \
            } catch(...) {}       \
        }                         \
        if(!taskName##_result.has_value()) {                                                    \
            try {                 \
                taskName##_error = std::make_optional<std::runtime_error>(                      \
                    std::any_cast<std::runtime_error>(*___task_result_##taskName));             \
            } catch(...) {}       \
        }                         \
    }

#define ASYNC_CONTINUE_TASK(taskName) \
    asyncStep = &&___task_create_##taskName; \
    return futureOp(AIOUringOp::Nop());

#define ASYNC_LOOP(loopName) \
    ___async_loop_begin_##loopName:

#define AWAIT_LOOP(loopName) \
    asyncStep = &&___async_loop_begin_##loopName; \
    return futureOp(AIOUringOp::Nop());

#define AWAIT_POLL() \
    ASYNC_RESET()    \
    return futureOp(AIOUringOp::Nop())

#define AWAIT_EVENT_INT(fdname, cnt) \
    if(___async_function) {          \
        asyncStep = &&___await_event_##cnt; \
    }                                \
    return futureOp(AIOUringOp::Read(fdname, &eventfdSink, sizeof(eventfd_t))); \
    ___await_event_##cnt:

#define AWAIT_EVENT2(fdname, cnt) \
    AWAIT_EVENT_INT(fdname, cnt)

#define AWAIT_EVENT(fdname) \
    AWAIT_EVENT2(fdname, __COUNTER__)

#define EVENT_NOTIFY_ASYNC_INT(fdname, cnt) \
    if(___async_function) {          \
        asyncStep = &&___event_notify_##cnt; \
    }                                \
    return futureOp(AIOUringOp::Write((fdname), &eventfdValue, sizeof(eventfd_t))); \
    ___event_notify_##cnt:

#define EVENT_NOTIFY_ASYNC2(fdname, cnt) \
    EVENT_NOTIFY_ASYNC_INT(fdname, cnt)

#define EVENT_NOTIFY_ASYNC(fdname) \
    EVENT_NOTIFY_ASYNC2(fdname, __COUNTER__)

#define EVENT_NOTIFY(eventfd) \
    eventfd_write((eventfd), 1L)

template<class T, class U>
concept Derived = std::is_base_of<U, T>::value;

template<class T, class U>
concept IsSameAs = std::is_same<U, T>::value;

template<class T>
concept IsFinal = std::is_final<T>::value;

class IOUringTaskException : public std::exception {
public:
    explicit IOUringTaskException(std::string message)
            : message(std::move(message))
    {

    }
    [[nodiscard]] const char* what() const noexcept override {
        return message.data();
    }
private:
    std::string message;
};

class AIOUringTask {
public:
    using TResult = std::monostate;

    enum class TaskState {
        New,
        Running,
        Done
    };

    using TaskFuture = std::tuple<std::optional<AIOUringOp>, std::optional<std::any>>;

    AIOUringTask();
    void setUringId(int id);
    [[nodiscard]] int getUringId() const;
    virtual ~AIOUringTask() = default;
    [[nodiscard]] std::weak_ptr<int> getTaskfd() const;
    void setState(TaskState newState);
    [[nodiscard]] TaskState getState() const;
    void setUring(io_uring *newRing);
    void setClassName(std::string className);
    [[nodiscard]] std::string getClassName() const;
    void asyncReset();
    void setTaskFinal();
    bool isTaskFinal();
    static TaskFuture futureEmpty();
    static TaskFuture futureResult(std::any result);
    static TaskFuture futureOp(AIOUringOp op);
    virtual bool init();
    virtual void free();
    virtual TaskFuture finally(int io_result);
    [[nodiscard]] virtual TaskFuture poll(int io_result) = 0;
protected:
    void *asyncStep{nullptr};
    io_uring *ring{nullptr};
    eventfd_t eventfdSink{};
    eventfd_t eventfdValue{1};
private:
    std::shared_ptr<int> taskfd{};
    int uringId{-1};
    TaskState taskState{TaskState::New};
    std::string className{};
    bool finalization{false};
};

template <typename T>
concept AIOUringTaskTrait = requires(T c) {
    c.free();
    c.asyncReset();
    c.setUringId(int{});
    c.setState(AIOUringTask::TaskState{});
    c.setUring((io_uring*){});
    c.setClassName(std::string{});
    c.setTaskFinal();
    { c.init() } -> std::same_as<bool>;
    { c.getTaskfd() } -> std::same_as<std::weak_ptr<int>>;
    { c.finally(int{}) } -> std::same_as<AIOUringTask::TaskFuture>;
    { c.isTaskFinal() } -> std::same_as<bool>;
};


#endif //AIOURINGTASK_H
