#ifndef CONFIGLOADER_H
#define CONFIGLOADER_H

#include <string>
#include <vector>
#include "../models/Profile.h"

namespace Utils {

class ConfigLoader {
public:
    static std::string loadWebhook(const std::string& configPath);
    static std::vector<Models::Profile> loadProfiles(const std::string& profilePath);
};

} // namespace Utils

#endif // CONFIGLOADER_H
