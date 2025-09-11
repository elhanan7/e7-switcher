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
    std::vector<uint8_t> work_status_bytes;
};

struct SwitchStatus {
    int wifi_power;
    bool switch_state;
    int remaining_time;
    int open_time;
    int auto_closing_time;
    bool is_delay;
    int online_state;

    std::string to_string() const;
};

// AC Mode enum
enum class ACMode {
    AUTO = 1,
    DRY  = 2,
    FAN  = 3,
    COOL = 4,
    HEAT = 5
};

// AC Fan Speed enum
enum class ACFanSpeed {
    FAN_LOW    = 1,
    FAN_MEDIUM = 2,
    FAN_HIGH   = 3,
    FAN_AUTO   = 4
};

// AC Swing enum
enum class ACSwing {
    SWING_OFF = 0,
    SWING_ON  = 1
};

// AC Power enum
enum class ACPower {
    POWER_OFF = 0,
    POWER_ON  = 1
};

std::string ac_mode_to_string(ACMode mode);
std::string ac_fan_speed_to_string(ACFanSpeed fan_speed);
std::string ac_swing_to_string(ACSwing swing);
std::string ac_power_to_string(ACPower power);

struct ACStatus {
    int wifi_power;
    float temperature;
    std::vector<uint8_t> ac_data; // 4 bytes of AC data
    ACPower power_status;
    ACMode mode;
    int ac_temperature;
    ACFanSpeed fan_speed;
    ACSwing swing;
    int temperature_unit;
    int device_type;
    std::string code_id;
    int last_time;
    int open_time;
    int auto_closing_time;
    int is_delay;
    int online_state;

    std::string to_string() const;
};

struct BoilerStatus {
    bool switch_state;
    float power;
    float electricity;
    int remaining_time;
    int open_time;
    int auto_closing_time;
    bool is_delay;
    int direction_equipment;
    int online_state;

    std::string to_string() const;
};



} // namespace e7_switcher
