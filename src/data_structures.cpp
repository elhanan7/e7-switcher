#include "e7-switcher/data_structures.h"

namespace e7_switcher {

std::string SwitchStatus::to_string() const {
    std::string result;
    result += "{ wifi_power: " + std::to_string(wifi_power) + ", ";
    result += "  switch_state: " + std::to_string(switch_state) + ", ";
    result += "  remaining_time: " + std::to_string(remaining_time) + ", ";
    result += "  open_time: " + std::to_string(open_time) + ", ";
    result += "  auto_closing_time: " + std::to_string(auto_closing_time) + ", ";
    result += "  is_delay: " + std::to_string(is_delay) + ", ";
    result += "  online_state: " + std::to_string(online_state) + " }";
    return result;
}

std::string ACStatus::to_string() const {
    std::string result;
    
    result += "{ wifi_power: " + std::to_string(wifi_power) + ", ";
    result += "  mode: " + ac_mode_to_string(mode) + ", ";
    result += "  ac_temperature: " + std::to_string(ac_temperature) + ", ";
    result += "  ambient_temperature: " + std::to_string(temperature) + ", ";
    result += "  on_or_off: " + ac_power_to_string(power_status) + ", ";
    result += "  fan_speed: " + ac_fan_speed_to_string(fan_speed) + ", ";
    result += "  swing: " + ac_swing_to_string(swing) + " }";
    return result;
}

std::string BoilerStatus::to_string() const {
    std::string result;
    
    result += "{ switch_state: " + std::to_string(switch_state) + ", ";
    result += "  power: " + std::to_string(power) + ", ";
    result += "  electricity: " + std::to_string(electricity) + ", ";
    result += "  remaining_time: " + std::to_string(remaining_time) + ", ";
    result += "  open_time: " + std::to_string(open_time) + ", ";
    result += "  auto_closing_time: " + std::to_string(auto_closing_time) + ", ";
    result += "  is_delay: " + std::to_string(is_delay) + ", ";
    result += "  direction_equipment: " + std::to_string(direction_equipment) + ", ";
    result += "  online_state: " + std::to_string(online_state) + " }";
    return result;
}

std::string ac_mode_to_string(ACMode mode) {
    switch (mode) {
        case ACMode::AUTO: return "auto";
        case ACMode::DRY: return "dry";
        case ACMode::FAN: return "fan";
        case ACMode::COOL: return "cool";
        case ACMode::HEAT: return "heat";
        default: return "unknown";
    }
}

std::string ac_fan_speed_to_string(ACFanSpeed fan_speed) {
    switch (fan_speed) {
        case ACFanSpeed::FAN_LOW: return "low";
        case ACFanSpeed::FAN_MEDIUM: return "medium";
        case ACFanSpeed::FAN_HIGH: return "high";
        case ACFanSpeed::FAN_AUTO: return "auto";
        default: return "unknown";
    }
}

std::string ac_swing_to_string(ACSwing swing) {
    switch (swing) {
        case ACSwing::SWING_OFF: return "off";
        case ACSwing::SWING_ON: return "on";
        default: return "unknown";
    }
}

std::string ac_power_to_string(ACPower power) {
    switch (power) {
        case ACPower::POWER_OFF: return "off";
        case ACPower::POWER_ON: return "on";
        default: return "unknown";
    }
}


} // namespace e7_switcher
