#ifndef PROXYPARSER_H
#define PROXYPARSER_H

#include <string>
#include <vector>

namespace Utils {

struct Proxy {
    std::string host;
    std::string port;
    std::string username;
    std::string password;

    std::string getProxyUrl() const;
};

class ProxyParser {
public:
    static Proxy parse(const std::string& proxyString);
};

} // namespace Utils

#endif // PROXYPARSER_H
