#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>

namespace e7_switcher {

class Reader {
public:
    Reader(const std::vector<uint8_t>& data);

    uint8_t u8();
    uint16_t u16();
    uint32_t u32();
    std::vector<uint8_t> take(size_t n);
    std::string lp_string_max(size_t max_len, const std::string& encoding = "utf-8");
    std::string ip_reversed();

private:
    void _need(size_t n);

    const std::vector<uint8_t>& data_;
    size_t p_;
};

struct PhoneLoginRecord {
    int32_t session_id;
    int32_t user_id;
    std::vector<uint8_t> communication_secret_key;
    std::vector<uint8_t> app_secret_key;
    uint8_t comm_enc_mode;
    uint8_t app_enc_mode;
    std::vector<uint8_t> server_time;
    uint16_t block_len_bytes;
    std::vector<uint8_t> block_a;
    std::vector<uint8_t> block_b;
    std::vector<uint8_t> gateway_domain;
    uint16_t gateway_port;
    uint8_t gateway_proto;
    std::vector<uint8_t> standby_gateway_domain;
    uint16_t standby_gateway_port;
    uint8_t standby_gateway_proto;
    std::vector<uint8_t> tcp_server_ip;
    uint16_t tcp_server_port;
    uint8_t tcp_server_proto;
    uint16_t heartbeat_secs;
    uint16_t reply_timeout_secs;
    std::vector<uint8_t> token;
    std::vector<uint8_t> nickname;
};

PhoneLoginRecord parse_phone_login(const std::vector<uint8_t>& payload);

struct ProtocolPacket {
    uint16_t start_flag;
    uint16_t length;
    uint16_t version;
    uint16_t cmd;
    uint32_t session;
    uint16_t serial;
    uint8_t direction;
    uint8_t err_code;
    uint16_t control_attr;
    uint32_t user_id;
    uint32_t timestamp;
    std::vector<uint8_t> raw_header;
    std::vector<uint8_t> payload;
    std::vector<uint8_t> crc;
};

struct LightStatus {
    int wifi_power;
    bool switch_state;
    int remaining_time;
    int open_time;
    int auto_closing_time;
    bool is_delay;
    int online_state;

    std::string to_string() const;
};


ProtocolPacket parse_protocol_packet(const std::vector<uint8_t>& payload);

LightStatus parse_light_status(const std::vector<uint8_t>& payload);

} // namespace e7_switcher
