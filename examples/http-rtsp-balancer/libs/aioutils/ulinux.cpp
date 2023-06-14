#include <sys/utsname.h>
#include <stdexcept>
#include <vector>

#include "include/aioutils/ulinux.h"
#include "include/aioutils/utext.h"
#include "include/aioutils/uexcept.h"

namespace aioutils::ulinux {
    bool linuxKernelNotLessThan(int major, int minor)
    {
        struct utsname buf{};

        auto res = uname(&buf);

        if(res < 0)
        {
            THROW_ERRNO(std::runtime_error, "On uname");
        }

        auto release = std::string{buf.release};

        if(release.empty())
        {
            throw std::runtime_error{"Release string returned from uname is empty."};
        }

        std::vector<std::string> tokens{};

        utext::tokenizeByStr(release, tokens, ".", true);

        if(tokens.empty())
        {
            throw std::runtime_error{"No token has been parsed from uname release string."};
        }

        if(tokens.size() < 2)
        {
            throw std::runtime_error{"At least 2 tokens must be present in string returned "
                                     "from uname, but got " + std::to_string(tokens.size())};
        }

        int kmajor{}, kminor{};

        try
        {
            kmajor = std::stoi(tokens[0]);
            kminor = std::stoi(tokens[1]);
        }
        catch(...)
        {
            throw std::runtime_error{"Failed to convert a token to integer, full string: " + release};
        }

        if(kmajor < 0 || kminor < 0)
        {
            throw std::runtime_error{"Minor or major kernel versions appeared to be negative, full string: " + release};
        }

        if(kmajor < major || kminor < minor)
        {
            return false;
        }

        return true;
    }
}