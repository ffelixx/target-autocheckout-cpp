#include "../../include/utils/FileReader.h"
#include <fstream>
#include <sstream>

namespace Utils {

std::vector<std::string> FileReader::readLines(const std::string& filepath) {
    std::vector<std::string> lines;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        return lines;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line != "\r") {
            // Remove carriage return if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
    }

    file.close();
    return lines;
}

std::string FileReader::readFile(const std::string& filepath) {
    std::ifstream file(filepath);

    if (!file.is_open()) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return buffer.str();
}

} // namespace Utils
