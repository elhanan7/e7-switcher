#pragma once

#include "communication.h"
#include "data_structures.h"
#include <string>
#include <vector>

namespace e7_switcher {

class E7Client {
public:
    E7Client(const std::string& account, const std::string& password);

    std::vector<Device> list_devices();
    void control_device(const std::string& device_name, const std::string& action);
    LightStatus get_light_status(const std::string& device_name);

private:
    E7Connection conn_;
};

} // namespace e7_switcher
