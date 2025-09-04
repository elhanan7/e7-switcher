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

class E7SwitcherClient {
public:
    E7SwitcherClient(const std::string& account, const std::string& password);
    ~E7SwitcherClient();

    // Connection management
    void connect();
    void close();
    
    // Device operations
    const std::vector<Device>& list_devices();
    void control_switch(const std::string& device_name, const std::string& action);
    void control_ac(const std::string& device_name, const std::string& action, ACMode mode, int temperature, ACFanSpeed fan_speed, ACSwing swing, int operationTime = 0);
    SwitchStatus get_switch_status(const std::string& device_name);
    OgeIRDeviceCode get_ac_ir_config(const std::string& device_name);

private:
    // Connection properties
    std::string host_;
    int port_;
    int timeout_;
    int sock_;
    std::optional<std::vector<Device>> devices_;
    
    // Authentication properties
    int32_t session_id_;
    int32_t user_id_;
    std::vector<uint8_t> communication_secret_key_;
    
    // Stream message handler
    MessageStream stream_;
    
    // Cache for IR device codes
    std::unordered_map<std::string, OgeIRDeviceCode> ir_device_code_cache_;
    
    // Internal methods
    PhoneLoginRecord login(const std::string& account, const std::string& password);
};

} // namespace e7_switcher
