//
// Created by sergei on 30.06.22.
//

#ifndef AIOUTILS_UHTTP_HPP
#define AIOUTILS_UHTTP_HPP

#include <utility>
#include <unordered_map>
#include <optional>

#include "utext.h"

using namespace aioutils;

namespace uhttp {
    using namespace std::string_view_literals;

    enum class HttpHeaderType {
        Unknown,
        ContentLength,
        ContentType,
        Host,
        Allow,
        Connection
    };

    enum class HttpRequestMethod {
        UNKNOWN,
        GET,
        POST,
        DELETE,
        OPTIONS
    };

    enum class HttpResponseClass {
        Unknown,
        C1xx [[maybe_unused]],
        C2xx,
        C3xx [[maybe_unused]],
        C4xx,
        C5xx [[maybe_unused]]
    };

    static constexpr const char * const ContentTypeJson = "application/json";

    static constexpr const char * const HttpResponse200Text = "HTTP/1.1 200 OK\r\n\r\n";
    static constexpr const char * const HttpResponse400Text = "HTTP/1.1 400 Bad Request\r\n\r\n";
    static constexpr const char * const HttpResponse404Text = "HTTP/1.1 404 Not Found\r\n\r\n";
    static constexpr const char * const HttpResponse500Text = "HTTP/1.1 500 Internal Server Error\r\n\r\n";

    namespace {
        constexpr auto endHeaderSeq = "\r\n\r\n"sv;
        constexpr auto reqResDelimiters = "\r\n";

        const std::unordered_map<std::string, HttpHeaderType> knownHeaders = {
                {"content-length", HttpHeaderType::ContentLength},
                {"content-type", HttpHeaderType::ContentType},
                {"host", HttpHeaderType::Host},
                {"allow", HttpHeaderType::Allow},
                {"connection", HttpHeaderType::Connection},
        };

        const std::unordered_map<std::string, HttpRequestMethod> knownRequestMethods = {
                {"get", HttpRequestMethod::GET},
                {"post", HttpRequestMethod::POST},
                {"delete", HttpRequestMethod::DELETE},
                {"options", HttpRequestMethod::OPTIONS}
        };
    }

    static std::optional<HttpRequestMethod> stringToHttpRequestMethod(const std::string &method);

    class ParseHttpException : public std::exception {
    public:
        explicit ParseHttpException(std::string message)
                : message(std::move(message))
        {

        }
        [[nodiscard]] const char* what() const noexcept override {
            return message.data();
        }
    private:
        std::string message;
    };

    class HttpHeader {
    public:
        HttpHeader() = default;
        explicit HttpHeader(const std::string& name, const std::string& value = {})
                : name{name}, value(value), originalValue(value)
        {
            rtspHeaderType = getHeaderType(name);
        }

        std::string toRaw()
        {
            if(value.empty())
            {
                return name;
            }

            return std::string{name + ": " + value};
        }

        HttpHeaderType getType()
        {
            return rtspHeaderType;
        }

        void setName(const std::string& newName)
        {
            name = newName;
            rtspHeaderType = getHeaderType(newName);
        }

        void setValue(std::string newValue)
        {
            value = std::move(newValue);
        }

        void setOriginalValue()
        {
            value = originalValue;
        }

        [[nodiscard]] std::string getValue() const
        {
            return value;
        }
    private:
        std::string name{}, value{}, originalValue{};
        HttpHeaderType rtspHeaderType{HttpHeaderType::Unknown};

        static HttpHeaderType getHeaderType(std::string name) {
            STRTOLOWER(name);

            auto header = knownHeaders.find(name);

            if(header == knownHeaders.end())
            {
                return HttpHeaderType::Unknown;
            }

            return header->second;
        }
    };

    class HttpResponse {
    public:
        using DataType = std::vector<char>;

        explicit HttpResponse(std::string data = {})
                : rawResponse(std::move(data))
        {

        }

        bool isEmpty()
        {
            return rawResponse.empty();
        }

        void parse()
        {
            std::string_view buffer(rawResponse.data(), rawResponse.size());

            auto headersEndsPos = buffer.find(endHeaderSeq);

            if(headersEndsPos == std::string_view::npos)
            {
                throw ParseHttpException("A headers end sequence was not found.");
            }

            std::string headersData = {rawResponse.data(), headersEndsPos};

            std::vector<std::string> tokens{};

            utext::tokenizeByAny(headersData, tokens, reqResDelimiters, true);

            if(tokens.empty())
            {
                throw ParseHttpException("Response doesn't contain any line: " + headersData);
            }

            auto& responseLine = tokens[0];

            std::vector<std::string> responseLineTokens{};

            utext::tokenizeByAny(responseLine, responseLineTokens, " ", true);

            if(responseLineTokens.size() < 3)
            {
                throw ParseHttpException("Invalid response line: " + responseLine);
            }

            httpVersion = responseLineTokens[0];
            statusCode = responseLineTokens[1];

            if(statusCode[0] >= '1' && statusCode[0] <= '5')
            {
                responseClass = static_cast<HttpResponseClass>(statusCode[0] - '0');
            }

            for(std::size_t i = 2; i < responseLineTokens.size(); i++)
            {
                if(!reasonPhrase.empty())
                {
                    reasonPhrase += " ";
                }

                reasonPhrase += responseLineTokens[i];
            }

            for(std::size_t i = 1; i < tokens.size(); i++)
            {
                auto& token = tokens[i];

                if(token.empty())
                {
                    continue;
                }

                auto pos = token.find_first_of(':');

                if(pos == std::string::npos)
                {
                    auto header = HttpHeader{token};

                    if(header.getType() != HttpHeaderType::Unknown)
                    {
                        headers[header.getType()] = header;
                    }

                    continue;
                }

                auto name = token.substr(0, pos);
                auto vPos = pos + 1;
                auto value = utext::ltrim_copy(token.substr(vPos, token.size() - vPos));

                auto header = HttpHeader{name, value};

                if(header.getType() != HttpHeaderType::Unknown)
                {
                    headers[header.getType()] = header;
                }
            }

            std::size_t contentStartPos = headersEndsPos + endHeaderSeq.size();

            if(rawResponse.size() > contentStartPos)
            {
                content = rawResponse.substr(contentStartPos);
            }

            setConnectionClose();
        }

        void setContentLength(uint64_t length)
        {
            if(length <= 0)
            {
                headers.erase(HttpHeaderType::ContentLength);
                return;
            }

            HttpHeader header{"Content-Length", std::to_string(length)};
            headers[header.getType()] = header;
        }

        DataType toRaw()
        {
            std::ostringstream oss;

            setContentLength(content.size());

            oss << httpVersion << " " << statusCode << " " << reasonPhrase << "\r\n";

            for(auto& header : headers)
            {
                oss << header.second.toRaw() << "\r\n";
            }

            oss << "\r\n";

            if(!content.empty())
            {
                oss << content;
            }

            auto rawData = oss.str();

            std::vector<char> buffer{};

            buffer.insert(buffer.end(), rawData.begin(), rawData.end());

            return buffer;
        }

        void setAllow(const std::string &value)
        {
            HttpHeader header{"Allow", value};
            headers[header.getType()] = header;
        }

        void setContentType(const std::string &value)
        {
            HttpHeader header{"Content-Type", value};
            headers[header.getType()] = header;
        }

        void setContent(const std::string &newContent)
        {
            content = newContent;
        }

        void setConnectionKeepAlive()
        {
            HttpHeader header{"Connection", "keep-alive"};
            headers[header.getType()] = header;
        }

        void setConnectionClose()
        {
            HttpHeader header{"Connection", "close"};
            headers[header.getType()] = header;
        }

        HttpResponseClass getResponseClass()
        {
            return responseClass;
        }

    private:
        std::string rawResponse{}, httpVersion{}, statusCode{}, reasonPhrase{}, content{};
        std::unordered_map<HttpHeaderType, HttpHeader> headers{};
        HttpResponseClass responseClass{HttpResponseClass::Unknown};
    };

    class HttpRequest {
    public:
        explicit HttpRequest(std::string data = {})
                : rawRequest(std::move(data))
        {

        }

        void parse()
        {
            std::string_view buffer(rawRequest.data(), rawRequest.size());

            auto headersEndsPos = buffer.find(endHeaderSeq);

            if(headersEndsPos == std::string_view::npos)
            {
                throw ParseHttpException("A headers end sequence was not found.");
            }

            std::string headersData = {rawRequest.data(), headersEndsPos};

            std::vector<std::string> tokens;

            utext::tokenizeByAny(headersData, tokens, reqResDelimiters, true);

            if(tokens.empty())
            {
                throw ParseHttpException("Request doesn't contain any line: " + rawRequest);
            }

            auto& requestLine = tokens[0];

            std::vector<std::string> requestLineTokens;

            utext::tokenizeByAny(requestLine, requestLineTokens, " ", true);

            if(requestLineTokens.size() != 3)
            {
                throw ParseHttpException("Request line is invalid: " + requestLine);
            }

            rawMethod = requestLineTokens[0];
            rawUri = requestLineTokens[1];
            version = requestLineTokens[2];

            auto methodOpt = stringToHttpRequestMethod(rawMethod);

            if(!methodOpt.has_value())
            {
                throw ParseHttpException("Unknown http method: " + rawMethod);
            }

            method = methodOpt.value();

            for(std::size_t i = 1; i < tokens.size(); i++)
            {
                auto& token = tokens[i];

                if(token.empty())
                {
                    continue;
                }

                auto pos = token.find_first_of(':');

                if(pos == std::string::npos)
                {
                    auto header = HttpHeader{token};

                    if(header.getType() != HttpHeaderType::Unknown)
                    {
                        headers[header.getType()] = header;
                    }

                    continue;
                }

                auto name = token.substr(0, pos);
                auto vPos = pos + 1;
                auto value = utext::ltrim_copy(token.substr(vPos, token.size() - vPos));

                auto header = HttpHeader{name, value};

                if(header.getType() != HttpHeaderType::Unknown)
                {
                    headers[header.getType()] = header;
                }
            }

            std::size_t contentStartPos = headersEndsPos + endHeaderSeq.size();

            if(rawRequest.size() > contentStartPos)
            {
                content = rawRequest.substr(contentStartPos);
            }
        }

        std::string toRaw()
        {
            std::ostringstream oss;

            oss << rawMethod << " " << rawUri << " " << version << "\r\n";

            for(auto& header : headers)
            {
                oss << header.second.toRaw() << "\r\n";
            }

            oss << "\r\n";

            return oss.str();
        }

        std::string getRawUri()
        {
            return rawUri;
        }

        std::string getOriginalRequest()
        {
            return rawRequest;
        }

        [[nodiscard]] std::string getContent() const
        {
            return content;
        }

        [[nodiscard]] std::optional<std::pair<std::string, std::string>> extractHost() const
        {
            auto httpHost = getHost();

            if(!httpHost.has_value())
            {
                return std::nullopt;
            }

            std::vector<std::string> tokens{};

            utext::tokenizeByStr(*httpHost, tokens, ":", true);

            if(tokens.empty() || tokens.size() > 2)
            {
                return std::nullopt;
            }

            auto pair = std::pair{tokens[0], std::string{}};

            if(tokens.size() > 1)
            {
                pair.second = tokens[1];
            }

            return pair;
        }

        [[nodiscard]] std::optional<std::string> getHost() const
        {
            return getHeaderValue(HttpHeaderType::Host);
        }

        [[nodiscard]] std::optional<std::string> getHeaderValue(HttpHeaderType headerType) const
        {
            auto hostIt = headers.find(headerType);

            if(hostIt == headers.end())
            {
                return std::nullopt;
            }

            auto value = hostIt->second.getValue();

            if(!value.empty())
            {
                return value;
            }

            return std::nullopt;
        }

        bool isKeepAlive()
        {
            auto it = headers.find(HttpHeaderType::Connection);

            if(it == headers.end())
            {
                return false;
            }

            auto &header = it->second;

            auto value = header.getValue();

            STRTOLOWER(value);

            if(value == "keep-alive")
            {
                return true;
            }

            return false;
        }

        HttpRequestMethod getMethod()
        {
            return method;
        }

    private:
        std::string rawRequest{}, rawMethod{}, version{}, rawUri{}, content{};
        HttpRequestMethod method{};
        std::unordered_map<HttpHeaderType, HttpHeader> headers{};
    };

    static bool isHeadersReady(std::string_view buffer) {
        return buffer.find(endHeaderSeq) != std::string_view::npos;
    }

    static bool isHeadersReady(const char *data, size_t size) {
        return isHeadersReady({data, size});
    }

    static bool isContentReady(std::string_view buffer) {

        if(!isHeadersReady(buffer))
        {
            return false;
        }

        auto headersEndsPos = buffer.find(endHeaderSeq);

        if(headersEndsPos == std::string_view::npos)
        {
            return false;
        }

        std::string headersData = {buffer.data(), headersEndsPos};

        std::vector<std::string> tokens{};

        utext::tokenizeByAny(headersData, tokens, reqResDelimiters, true);

        if(tokens.empty())
        {
            return false;
        }

        for(std::size_t i = 1; i < tokens.size(); i++)
        {
            auto& token = tokens[i];

            if(token.empty())
            {
                continue;
            }

            auto pos = token.find_first_of(':');

            if(pos == std::string::npos)
            {
                continue;
            }

            auto name = token.substr(0, pos);
            auto vPos = pos + 1;
            auto value = utext::ltrim_copy(token.substr(vPos, token.size() - vPos));

            auto rtspHeader = HttpHeader{name, value};

            switch (rtspHeader.getType()) {
                case HttpHeaderType::ContentLength:
                    try
                    {
                        return buffer.size() >= (std::stoi(value) + headersEndsPos + endHeaderSeq.size());
                    }
                    catch(std::exception &e)
                    {
                        return false;
                    }
            }
        }

        return true;
    }

    static bool isContentReady(const char *data, size_t size) {
        return isContentReady({data, size});
    }

    static std::optional<HttpRequestMethod> stringToHttpRequestMethod(const std::string &method)
    {
        auto methodLC = method;

        STRTOLOWER(methodLC);

        auto it = knownRequestMethods.find(methodLC);

        if(it == knownRequestMethods.end())
        {
            return std::nullopt;
        }

        return it->second;
    }
}

#endif //AIOUTILS_UHTTP_HPP
