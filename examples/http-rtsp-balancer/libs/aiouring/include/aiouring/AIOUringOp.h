//
// Created by sergei on 10.06.22.
//

#ifndef AIOURINGOP_H
#define AIOURINGOP_H

#include <liburing.h>
#include <functional>
#include <optional>
#include <sys/socket.h>

struct AIOUringOp {
    std::optional<std::function<void(io_uring *, __u64)>> submit{std::nullopt};
    bool shutdown{false};
    int shutdownCode{0};
    static AIOUringOp ShutdownUring(int code = 0);
    static AIOUringOp Nop();
    static AIOUringOp Read(int fd, void *buf, size_t buf_size, __u64 offset = 0);
    static AIOUringOp Write(int fd, void *buf, size_t buf_size, __u64 offset = 0);
    static AIOUringOp Accept(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags = 0);
    static AIOUringOp Close(int fd);
    static AIOUringOp Connect(int fd, const struct sockaddr *addr, socklen_t addrlen);
    static AIOUringOp Shutdown(int fd, int how = SHUT_RDWR);
};

#endif //AIOURINGOP_H
