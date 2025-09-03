#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace e7_switcher {

struct Device {
    std::string name;
    std::string ssid;
    std::string mac;
    std::string type;
    std::string firmware;
    bool online;
    int line_no;
    int line_type;
    int did;
    std::string visit_pwd;
};

struct LightStatus {
    int wifi_power;
    bool switch_state;
    int remaining_time;
    int open_time;
    int auto_closing_time;
    bool is_delay;
    int online_state;

    std::string to_string() const;
};

} // namespace e7_switcher
