//
// Created by sergei on 07.06.22.
//

#include "include/aiouring/AIOUring.h"

using namespace aioutils;

#define SQ_CQ_MIN_NUMBER 4096

AIOUring::AIOUring(std::optional<int> iouringBackend, bool useSQPoll) :
        iouringBackend{iouringBackend}, useSQPoll{useSQPoll} {
    instanceId = idGenerator.fetch_add(1);
}

void AIOUring::executeLongTask(AIOUringLongTask longTask) {
    if(longTask.task.has_value())
    {
        executor.silent_async([=, this]() {
            try {
                (*longTask.task)(&executor);
            } catch (std::exception &e) {
                kklogging::ERROR(fmt::format("During long running task: {}", e.what()));
            }
            if(longTask.eventfd.has_value()) {
                eventfd_write(*longTask.eventfd, 1L);
            }
        });
    }
}

std::tuple<bool, int> AIOUring::processCQE(io_uring_cqe *cqe) {
    auto *task = static_cast<AIOUringTask *>(
            reinterpret_cast<void *>(cqe->user_data));

    try {
        AIOUringTask::TaskFuture taskFuture = AIOUringTask::futureEmpty();

        if(!task->isTaskFinal()) {
            taskFuture = task->poll(cqe->res);
        } else {
            taskFuture = task->finally(cqe->res);
        }

        std::optional<AIOUringOp> taskOp = std::get<0>(taskFuture);

        if(!taskOp.has_value())
        {
            try {
                std::optional<std::any> &result = std::get<1>(taskFuture);
                if(result.has_value()) {
                    auto error = std::any_cast<std::runtime_error>(*result);
                    kklogging::ERROR(fmt::format("{}: {}", task->getClassName(), error.what()));
                }
            } catch(...) {};

            if(!task->isTaskFinal()) {
                task->setTaskFinal();
                auto op = AIOUringOp::Nop();
                (*op.submit)(&this->ring, cqe->user_data);
            } else {
                freeTask(task);
            }
        }
        else
        {
            auto op = *taskOp;

            if(op.shutdown)
            {
                return std::make_tuple(false, op.shutdownCode);
            }

            if(op.submit.has_value())
            {
                (*op.submit)(&this->ring, cqe->user_data);
            }
            else
            {
                kklogging::ERROR("No submit function for operation.");
            }
        }
    }
    catch(std::exception &e) {
        kklogging::ERROR(fmt::format("Uncaught exception during poll/finally(), task {}: {}",
                                     task->getClassName(), e.what()));
        return std::make_tuple(false, 1);
    }

    return std::make_tuple(true, 0);
}

int AIOUring::run() {
    kklogging::INFO("IO_URING has started.");
    while(true)
    {
        auto result = io_uring_submit_and_wait(&ring, 1);
        struct io_uring_cqe *cqe;
        unsigned head;
        unsigned count = 0;

        if(result < 0)
        {
            kklogging::ERROR("io_uring_submit_and_wait failed: " + uexcept::errnoStr(-result));
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        io_uring_for_each_cqe(&ring, head, cqe) {
            ++count;

            if(cqe->user_data == 0)
            {
                kklogging::ERROR("cqe->user_data == 0");
                continue;
            }

            auto res = processCQE(cqe);

            if(!std::get<0>(res)) {
                kklogging::WARN("IO_URING shutdown.");
                return std::get<1>(res);
            }
        }

        io_uring_cq_advance(&ring, count);
    }
}

void AIOUring::setup() {
    if(useSQPoll)
    {
        useSQPoll = ulinux::linuxKernelNotLessThan(5, 11);
    }

    if(useSQPoll)
    {
        params.flags |= IORING_SETUP_SQPOLL;
        params.sq_thread_idle = 2000;
    }

    if(!useSQPoll)
    {
        kklogging::INFO("SqlPoll is disabled");
    }

    if(iouringBackend.has_value())
    {
        params.flags |= IORING_SETUP_ATTACH_WQ;
        params.wq_fd = *iouringBackend;
    }

    if (io_uring_queue_init_params(SQ_CQ_MIN_NUMBER, &ring, &params) < 0) {
        if(errno == EPERM)
        {
            throw AIOUringException("Operation not permitted. TRY ROOT or disable SQPOLL by setting "
                                    "RTSP_PROXY_IOURING_USE_SQPOLL=false. If it is run from docker try --privileged.");
        }
        else
        {
            THROW_ERRNO(AIOUringException, "During io_uring_queue_init_params");
        }
    }

    if (!(params.features & IORING_FEAT_FAST_POLL)) {
        throw AIOUringException("IORING_FEAT_FAST_POLL not available in the kernel. "
                                "It is available starting from 5.7 kernel version.");
    }

    if(useSQPoll)
    {
        if (!(params.features & IORING_FEAT_SQPOLL_NONFIXED)) {
            throw AIOUringException("IORING_FEAT_SQPOLL_NONFIXED not available in the kernel. "
                                    "It is available starting from 5.11 kernel version.");
        }
    }

    setupPassed = true;
}

int AIOUring::getInstanceId() const {
    return instanceId;
}

