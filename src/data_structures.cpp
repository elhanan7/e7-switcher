#include "data_structures.h"

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

} // namespace e7_switcher
