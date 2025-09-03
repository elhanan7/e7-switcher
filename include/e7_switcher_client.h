#pragma once

#include "message_stream.h"
#include "data_structures.h"
#include "parser.h"
#include <string>
#include <vector>
#include <cstdint>

namespace e7_switcher {

class E7SwitcherClient {
public:
    E7SwitcherClient(const std::string& account, const std::string& password);
    ~E7SwitcherClient();

    // Connection management
    void connect();
    void close();
    
    // Device operations
    std::vector<Device> list_devices();
    void control_switch(const std::string& device_name, const std::string& action);
    LightStatus get_light_status(const std::string& device_name);

private:
    // Connection properties
    std::string host_;
    int port_;
    int timeout_;
    int sock_;
    
    // Authentication properties
    int32_t session_id_;
    int32_t user_id_;
    std::vector<uint8_t> communication_secret_key_;
    
    // Stream message handler
    MessageStream stream_;
    
    // Internal methods
    PhoneLoginRecord login(const std::string& account, const std::string& password);
};

} // namespace e7_switcher
