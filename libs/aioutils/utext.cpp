#include <vector>
#include <random>

#include "include/aioutils/utext.h"

namespace aioutils::utext {
    // trim from start (in place)
    void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    // trim from end (in place)
    void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    // trim from both ends (in place)
    void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }

    // trim from start (copying)
    std::string ltrim_copy(std::string s) {
        ltrim(s);
        return s;
    }

    // trim from end (copying)
    std::string rtrim_copy(std::string s) {
        rtrim(s);
        return s;
    }

    // trim from both ends (copying)
    std::string trim_copy(std::string s) {
        trim(s);
        return s;
    }

    std::string replaceString(std::string subject, const
    std::string& search, const std::string& replace) {

        size_t pos = 0;

        while((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }

        return subject;
    }

    void replaceStringInPlace(std::string& subject, const
    std::string& search, const std::string& replace) {

        size_t pos = 0;

        while((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
    }

    std::string join(const std::vector<std::string> &lst, const std::string &delim)
    {
        std::string result;

        for(const auto &s : lst) {
            if(!result.empty())
            {
                result += delim;
            }
            result += s;
        }

        return result;
    }

    void replaceAll(std::string& str, const std::string& from, const std::string& to) {
        if(from.empty())
        {
            return;
        }

        size_t start_pos = 0;

        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }

    std::string genUUID() {
        static std::random_device dev{};
        static std::mt19937 rng{dev()};

        std::uniform_int_distribution<int> dist{0, 15};

        const char *v = "0123456789abcdef";
        const int dash[] = { 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0 };

        std::string res{};

        for (const int &i : dash) {
            if (i == 1) {
                res += "-";
            }
            res += v[dist(rng)];
            res += v[dist(rng)];
        }

        return res;
    }
}