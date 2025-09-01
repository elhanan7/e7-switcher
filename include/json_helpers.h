#pragma once

#include <string>
#include <vector>
#include "communication.h"  // For Device struct

namespace e7_switcher {

// Extract device list from JSON response
bool extract_device_list(const std::string& json_str, std::vector<Device>& devices);

// Extract is_rest_day from JSON response
bool extract_is_rest_day(const std::string& json_str, bool& is_rest_day);

} // namespace e7_switcher
