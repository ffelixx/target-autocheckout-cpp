#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <string>
#include <map>
#include <memory>
#include "../utils/ProxyParser.h"

namespace Network {

struct HttpResponse {
    int statusCode;
    std::string body;
    std::map<std::string, std::string> headers;
};

class HttpClient {
private:
    Utils::Proxy proxy;
    std::string cookie;
    void* curl;

public:
    HttpClient(const Utils::Proxy& proxy, const std::string& cookie);
    ~HttpClient();

    HttpResponse get(const std::string& url, const std::map<std::string, std::string>& headers);
    HttpResponse post(const std::string& url, const std::map<std::string, std::string>& headers, const std::string& body);
    HttpResponse put(const std::string& url, const std::map<std::string, std::string>& headers, const std::string& body);
    HttpResponse del(const std::string& url, const std::map<std::string, std::string>& headers);

private:
    void setupCurl();
    std::map<std::string, std::string> getDefaultHeaders() const;
};

} // namespace Network

#endif // HTTPCLIENT_H
