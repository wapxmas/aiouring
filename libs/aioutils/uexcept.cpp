#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <kklogging/kklogging.h>
#include "include/aioutils/uexcept.h"

namespace aioutils::uexcept {
    std::string errnoStr(int num)
    {
        auto errStr = strerror(num);

        if(errStr == nullptr)
        {
            return "Unknown errono: " + std::to_string(num);
        }

        return errStr;
    }

    void logErrno(const std::string &message, int errorNumber) {
        std::ostringstream logString;

        logString << message << ", errorno: " << errno << ", message: " << std::
        strerror(errorNumber < 0 ? errorNumber : errno) << std::endl;

        kklogging::ERROR(logString.str());
    }
}