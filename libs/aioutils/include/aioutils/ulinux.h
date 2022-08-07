//
// Created by sergei on 08.06.22.
//

#ifndef AIOUTILS_ULINUX_H
#define AIOUTILS_ULINUX_H

#define CALL_RETRY(retvar, expression) do { \
    (retvar) = (expression); \
} while ((retvar) == -1 && errno == EINTR)

namespace aioutils::ulinux {
    bool linuxKernelNotLessThan(int major, int minor);
}
#endif //AIOUTILS_ULINUX_H
