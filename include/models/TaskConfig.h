#ifndef TASKCONFIG_H
#define TASKCONFIG_H

#include <string>

namespace Models {

struct TaskConfig {
    std::string pid;
    int quantity;
    std::string profile;
    int delay;
    std::string cookie;
};

} // namespace Models

#endif // TASKCONFIG_H
