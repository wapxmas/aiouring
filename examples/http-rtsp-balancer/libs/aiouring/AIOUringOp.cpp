#include "include/aiouring/AIOUringOp.h"

AIOUringOp AIOUringOp::ShutdownUring(int code) {
    return AIOUringOp {
            .shutdown = true,
            .shutdownCode = code
    };
}

AIOUringOp AIOUringOp::Nop() {
    return AIOUringOp {
        .submit = [=](io_uring *ring, __u64 ptrTask) {
            struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
            io_uring_prep_nop(sqe);
            sqe->user_data = ptrTask;
        }
    };
}

AIOUringOp AIOUringOp::Read(int fd, void *buf, size_t buf_size, __u64 offset) {
    return AIOUringOp {
            .submit = [=](io_uring *ring, __u64 ptrTask) {
                struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
                io_uring_prep_read(sqe, fd, buf, buf_size, offset);
                sqe->user_data = ptrTask;
            }
    };
}

AIOUringOp AIOUringOp::Write(int fd, void *buf, size_t buf_size, __u64 offset) {
    return AIOUringOp {
            .submit = [=](io_uring *ring, __u64 ptrTask) {
                struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
                io_uring_prep_write(sqe, fd, buf, buf_size, offset);
                sqe->user_data = ptrTask;
            }
    };
}

AIOUringOp AIOUringOp::Accept(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    return AIOUringOp {
            .submit = [=](io_uring *ring, __u64 ptrTask) {
                struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
                io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
                sqe->user_data = ptrTask;
            }
    };
}

AIOUringOp AIOUringOp::Close(int fd) {
    return AIOUringOp {
            .submit = [=](io_uring *ring, __u64 ptrTask) {
                struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
                io_uring_prep_close(sqe, fd);
                sqe->user_data = ptrTask;
            }
    };
}

AIOUringOp AIOUringOp::Connect(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    return AIOUringOp {
            .submit = [=](io_uring *ring, __u64 ptrTask) {
                struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
                io_uring_prep_connect(sqe, fd, addr, addrlen);
                sqe->user_data = ptrTask;
            }
    };
}

AIOUringOp AIOUringOp::Shutdown(int fd, int how) {
    return AIOUringOp {
            .submit = [=](io_uring *ring, __u64 ptrTask) {
                struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
                io_uring_prep_shutdown(sqe, fd, how);
                sqe->user_data = ptrTask;
            }
    };
}
