#include "../../include/utils/Logger.h"
#include <iostream>

// ANSI color codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"

namespace Utils {

void Logger::info(const std::string& message) {
    std::cout << BLUE << message << RESET << std::endl;
}

void Logger::success(const std::string& message) {
    std::cout << GREEN << message << RESET << std::endl;
}

void Logger::warning(const std::string& message) {
    std::cout << YELLOW << message << RESET << std::endl;
}

void Logger::error(const std::string& message) {
    std::cout << RED << message << RESET << std::endl;
}

} // namespace Utils
