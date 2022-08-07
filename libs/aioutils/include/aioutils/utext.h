#ifndef AIOUTILS_UTEXT_H
#define AIOUTILS_UTEXT_H

#include <string>
#include <algorithm>
#include <cctype>
#include <vector>

#define STRTOLOWER(x) std::transform (x.begin(), x.end(), x.begin(), [](unsigned char c){ return std::tolower(c); })
#define STRTOUPPER(x) std::transform (x.begin(), x.end(), x.begin(), [](unsigned char c){ return std::toupper(c); })

namespace aioutils::utext {
    void ltrim(std::string &s);
    void rtrim(std::string &s);
    void trim(std::string &s);
    std::string ltrim_copy(std::string s);
    std::string rtrim_copy(std::string s);
    std::string trim_copy(std::string s);
    std::string replaceString(std::string subject, const
    std::string& search, const std::string& replace);
    void replaceStringInPlace(std::string& subject, const
    std::string& search, const std::string& replace);
    std::string join(const std::vector<std::string> &lst, const std::string &delim);
    void replaceAll(std::string& str, const std::string& from, const std::string& to);
    template<class ContainerT>
    void tokenizeByStr(const std::string &str, ContainerT &tokens,
                              const std::string &delimiter = " ", bool trimEmpty = false);
    template<class ContainerT>
    void tokenizeByAny(const std::string &str, ContainerT &tokens,
                              const std::string &delimiters = " ", bool trimEmpty = false);
    template< typename T >
    std::string intToHex( T i );
    std::string genUUID();
}

#include "utext.tpp"

#endif //AIOUTILS_UTEXT_H
