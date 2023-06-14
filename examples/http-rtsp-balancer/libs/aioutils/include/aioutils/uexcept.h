//
// Created by sergei on 08.06.22.
//

#ifndef AIOUTILS_HPEXCEPT_H
#define AIOUTILS_HPEXCEPT_H

#include <string>

#define THROW_ERRNO(exceptionT, exceptionText) \
    throw exceptionT(exceptionText + std::string(": ") + \
        aioutils::uexcept::errnoStr(errno))

namespace aioutils::uexcept {
    std::string errnoStr(int num);
    void logErrno(const std::string &message, int errorNumber = 0);
}

#endif //AIOUTILS_HPEXCEPT_H
