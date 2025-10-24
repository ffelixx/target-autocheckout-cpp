#include "../../include/utils/ProxyParser.h"
#include <sstream>
#include <vector>

namespace Utils {

std::string Proxy::getProxyUrl() const {
    return "http://" + username + ":" + password + "@" + host + ":" + port;
}

Proxy ProxyParser::parse(const std::string& proxyString) {
    Proxy proxy;
    std::istringstream ss(proxyString);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(ss, token, ':')) {
        tokens.push_back(token);
    }

    if (tokens.size() >= 4) {
        proxy.host = tokens[0];
        proxy.port = tokens[1];
        proxy.username = tokens[2];
        proxy.password = tokens[3];
    }

    return proxy;
}

} // namespace Utils
