#pragma once

#include "message_stream.h"
#include "data_structures.h"
#include "parser.h"
#include "oge_ir_device_code.h"
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace e7_switcher {

class E7SwitcherClient {
public:
    // Device type constants
    static constexpr const char* DEVICE_TYPE_AC = "0E01";
    static constexpr const char* DEVICE_TYPE_SWITCH = "0F04";
    
public:
    E7SwitcherClient(const std::string& account, const std::string& password);
    ~E7SwitcherClient();
    
    // Device operations
    const std::vector<Device>& list_devices();
    void control_switch(const std::string& device_name, const std::string& action);
    void control_ac(const std::string& device_name, const std::string& action,
                    ACMode mode, int temperature, ACFanSpeed fan_speed,
                    ACSwing swing, int operation_time = 0);
    SwitchStatus get_switch_status(const std::string& device_name);
    ACStatus get_ac_status(const std::string& device_name);

private:
    std::optional<std::vector<Device>> devices_;
    
    // Authentication properties
    int32_t session_id_;
    int32_t user_id_;
    std::vector<uint8_t> communication_secret_key_;
    
    // Stream message handler
    MessageStream stream_;
    
    OgeIRDeviceCode get_ac_ir_config(const std::string& device_name);
    // Cache for IR device codes
    std::unordered_map<std::string, OgeIRDeviceCode> ir_device_code_cache_;
    
    // Internal methods
    PhoneLoginRecord login(const std::string& account, const std::string& password);
    
    // Helper method to find and validate a device
    const Device& find_device_by_name_and_type(
        const std::string& device_name, const std::string& expected_type);
};

} // namespace e7_switcher
