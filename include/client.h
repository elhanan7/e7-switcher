#pragma once

#include "communication.h"
#include <string>
#include <vector>

namespace e7_switcher {

class E7Client {
public:
    E7Client(const std::string& account, const std::string& password);

    std::vector<Device> list_devices();
    void control_device(const std::string& device_name, const std::string& action);

private:
    E7Connection conn_;
};

} // namespace e7_switcher
