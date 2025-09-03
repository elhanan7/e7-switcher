#pragma once

#include <string>
#include <vector>
#include "data_structures.h"  // For Device struct
#include "oge_ir_device_code.h"  // For OgeIRDeviceCode struct

namespace e7_switcher {

// Extract device list from JSON response
bool extract_device_list(const std::string& json_str, std::vector<Device>& devices);

// Extract is_rest_day from JSON response
bool extract_is_rest_day(const std::string& json_str, bool& is_rest_day);

// Parse OgeIRDeviceCode from JSON string
OgeIRDeviceCode parse_oge_ir_device_code(const std::string& json_str);

} // namespace e7_switcher
