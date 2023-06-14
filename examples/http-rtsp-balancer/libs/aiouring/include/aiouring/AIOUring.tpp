
#include "AIOUring.h"
#include <cxxabi.h>

template<typename T, typename... Args>
requires Derived<T, AIOUringTask> && IsFinal<T> && AIOUringTaskTrait<T>
T *AIOUring::newTask(Args... args) {
    if(!setupPassed)
    {
        throw AIOUringException("You have to setup() firstly.");
    }
    T* newTask = new T{args...};
    newTask->setUringId(getInstanceId());
    newTask->setUring(&ring);

    {
        int status;
        char * demangled = abi::__cxa_demangle(typeid(T).name(),nullptr,nullptr,&status);
        if(demangled != nullptr) {
            newTask->setClassName(demangled);
            free(demangled);
        } else {
            newTask->setClassName(typeid(T).name());
        }
    }

    if(!newTask->init()) {
        freeTask(newTask);
        throw AIOUringException("Failed to init new task.");
    }
    if(*(newTask->getTaskfd().lock()) < 0) {
        freeTask(newTask);
        throw AIOUringException(fmt::format("Failed to create eventfd: code: "
                                           "{}, message: ", errno, std::strerror(errno)));
    }
    return newTask;
}

template<Derived<AIOUringTask> T>
requires AIOUringTaskTrait<T>
void AIOUring::freeTask(T *task) {
    using enum AIOUringTask::TaskState;

    task->setState(Done);

    try {
        task->free();
    } catch (std::exception &e) {
        kklogging::ERROR(fmt::format("task->free(): {}", e.what()));
    }

    delete task;
}

template<Derived<AIOUringTask> T>
requires AIOUringTaskTrait<T>
void AIOUring::pushTask(T *task) {
    using enum AIOUringTask::TaskState;
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_nop(sqe);
    sqe->user_data = reinterpret_cast<__u64>(task);
    task->setState(Running);
}
