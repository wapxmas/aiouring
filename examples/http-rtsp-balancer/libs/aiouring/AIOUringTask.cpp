//
// Created by sergei on 06.06.22.
//

#include "include/aiouring/AIOUringTask.h"

AIOUringTask::AIOUringTask() {
    taskfd = std::make_shared<int>(eventfd(0, EFD_SEMAPHORE));
}

void AIOUringTask::setUringId(int id) {
    uringId = id;
}

int AIOUringTask::getUringId() const {
    return uringId;
}

std::weak_ptr<int> AIOUringTask::getTaskfd() const {
    return taskfd;
}

void AIOUringTask::setState(const AIOUringTask::TaskState newState) {
    taskState = newState;
}

AIOUringTask::TaskState AIOUringTask::getState() const {
    return taskState;
}

bool AIOUringTask::init() {
    return true;
}

void AIOUringTask::free() {
    if(*taskfd > 0) {
        close(*taskfd);
    }
}

AIOUringTask::TaskFuture AIOUringTask::finally(int io_result) {
    return futureEmpty();
}

AIOUringTask::TaskFuture AIOUringTask::futureEmpty() {
    return std::make_tuple(std::nullopt, std::nullopt);
}

AIOUringTask::TaskFuture AIOUringTask::futureOp(AIOUringOp op) {
    return std::make_tuple(std::optional{op}, std::nullopt);
}

AIOUringTask::TaskFuture AIOUringTask::futureResult(std::any result) {
    return std::make_tuple(std::nullopt, std::optional{result});
}

void AIOUringTask::setUring(io_uring *newRing) {
    ring = newRing;
}

void AIOUringTask::setClassName(std::string className) {
    this->className = std::move(className);
}

std::string AIOUringTask::getClassName() const {
    return className;
}

void AIOUringTask::asyncReset() {
    asyncStep = nullptr;
}

void AIOUringTask::setTaskFinal() {
    finalization = true;
    asyncReset();
}

bool AIOUringTask::isTaskFinal() {
    return finalization;
}
