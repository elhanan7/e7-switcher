#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "parser.h"
#include "stream_message.h"
#include "data_structures.h"

namespace e7_switcher {

class E7Connection {
public:
    E7Connection(const std::string& host, int port, int timeout = 5);
    ~E7Connection();

    void connect();
    void close();


    PhoneLoginRecord login(const std::string& account, const std::string& password);
    std::vector<Device> get_device_list(); 
    ProtocolMessage control_device(const std::string& device_name, const std::string& action);
    LightStatus get_light_status(const std::string& device_name);

private:

    std::string host_;
    int port_;
    int timeout_;   // default short timeout (seconds) set on connect()
    int sock_;

    int32_t session_id_;
    int32_t user_id_;
    std::vector<uint8_t> communication_secret_key_;

    // Stream message handler
    StreamMessage stream_;
};

} // namespace e7_switcher
