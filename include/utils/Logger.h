#ifndef LOGGER_H
#define LOGGER_H

#include <string>

namespace Utils {

class Logger {
public:
    static void info(const std::string& message);
    static void success(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
};

} // namespace Utils

#endif // LOGGER_H
