#ifndef FILEREADER_H
#define FILEREADER_H

#include <string>
#include <vector>

namespace Utils {

class FileReader {
public:
    static std::vector<std::string> readLines(const std::string& filepath);
    static std::string readFile(const std::string& filepath);
};

} // namespace Utils

#endif // FILEREADER_H
