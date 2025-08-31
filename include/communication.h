#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "parser.h"

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

class E7Connection {
public:
    E7Connection(const std::string& host, int port, int timeout = 5);
    ~E7Connection();

    void connect();
    void close();

    void send_message(const std::vector<uint8_t>& data);
    std::vector<uint8_t> receive_message(int timeout_ms = 15000); // long, per-call timeout

    PhoneLoginRecord login(const std::string& account, const std::string& password);
    std::vector<Device> get_device_list(); 
    ProtocolPacket control_device(const std::string& device_name, const std::string& action);
    LightStatus get_light_status(const std::string& device_name);

private:
    // Stream helpers
    bool recv_into_buffer_until(size_t min_size, int timeout_ms);
    bool try_extract_one_packet(std::vector<uint8_t>& out);

    std::string host_;
    int port_;
    int timeout_;   // default short timeout (seconds) set on connect()
    int sock_;

    int32_t session_id_;
    int32_t user_id_;
    std::vector<uint8_t> communication_secret_key_;

    // New: incoming stream buffer
    std::vector<uint8_t> inbuf_;
};

} // namespace e7_switcher
