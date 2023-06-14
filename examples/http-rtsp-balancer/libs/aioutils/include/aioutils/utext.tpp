#include <locale>
#include <iomanip>

namespace aioutils::utext {
    template<class ContainerT>
    void tokenizeByStr(const std::string &str, ContainerT &tokens,
                       const std::string &delimiter, bool trimEmpty) {
        std::string::size_type pos, lastPos = 0, length = str.length();

        using value_type = typename ContainerT::value_type;
        using size_type = typename ContainerT::size_type;

        while (lastPos < length + 1) {
            pos = str.find(delimiter, lastPos);

            if (pos == std::string::npos) {
                pos = length;
            }

            if (pos != lastPos || !trimEmpty)
            {
                tokens.push_back(value_type(str.data() + lastPos,
                                            static_cast<size_type>(pos - lastPos)));
            }

            lastPos = pos + delimiter.size();
        }
    }

    template<class ContainerT>
    void tokenizeByAny(const std::string &str, ContainerT &tokens,
                       const std::string &delimiters, bool trimEmpty) {
        std::string::size_type pos, lastPos = 0, length = str.length();

        using value_type = typename ContainerT::value_type;
        using size_type = typename ContainerT::size_type;

        while (lastPos < length + 1) {
            pos = str.find_first_of(delimiters, lastPos);

            if (pos == std::string::npos) {
                pos = length;
            }

            if (pos != lastPos || !trimEmpty)
            {
                tokens.push_back(value_type(str.data() +
                                            lastPos, static_cast<size_type>(pos - lastPos)));
            }

            lastPos = pos + 1;
        }
    }

    template< typename T >
    std::string intToHex( T i )
    {
        std::stringstream stream;
        stream << std::setfill ('0')
               << std::setw(sizeof(T) * 2)
               << std::hex << i;
        return stream.str();
    }
}