#include "client.h"
#include "constants.h"

namespace e7_switcher {

E7Client::E7Client(const std::string& account, const std::string& password)
    : conn_(IP_HUB, PORT_HUB) {
    conn_.connect();
    conn_.login(account, password);
}

std::vector<Device> E7Client::list_devices() {
    return conn_.get_device_list();
}

void E7Client::control_device(const std::string& device_name, const std::string& action) {
    conn_.control_device(device_name, action);
}

LightStatus E7Client::get_light_status(const std::string& device_name) {
    return conn_.get_light_status(device_name);
}

} // namespace e7_switcher
