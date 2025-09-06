#if defined(ARDUINO) || defined(ESP_PLATFORM) || defined(ESP32) || defined(ESP8266)
#define E7_PLATFORM_ESP 1
#include <Arduino.h>
#else
#define E7_PLATFORM_DESKTOP 1
// Desktop platform - no Arduino headers needed
#endif
#include "e7-switcher/parser.h"
#include "e7-switcher/logger.h"
#include "e7-switcher/data_structures.h"
#include <stdexcept>
#include <arpa/inet.h>
#include <algorithm>

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

Reader::Reader(const std::vector<uint8_t>& data) : data_(data), p_(0) {}

void Reader::_need(size_t n) {
    if (p_ + n > data_.size()) {
        throw std::out_of_range("Not enough data in buffer");
    }
}

uint8_t Reader::u8() {
    _need(1);
    return data_[p_++];
}

uint16_t Reader::u16() {
    _need(2);
    uint16_t val = data_[p_] | (data_[p_ + 1] << 8);
    p_ += 2;
    return val;
}

uint32_t Reader::u32() {
    _need(4);
    int32_t val = data_[p_] | (data_[p_ + 1] << 8) | (data_[p_ + 2] << 16) | (data_[p_ + 3] << 24);
    p_ += 4;
    return val;
}

std::vector<uint8_t> Reader::take(size_t n) {
    _need(n);
    std::vector<uint8_t> sub(data_.begin() + p_, data_.begin() + p_ + n);
    p_ += n;
    return sub;
}

std::string Reader::lp_string_max(size_t max_len, const std::string& encoding) {
    uint8_t len = u8();
    if (len > max_len) {
        throw std::out_of_range("String length exceeds max_len");
    }
    std::vector<uint8_t> str_bytes = take(len);
    if (max_len - len > 0) {
        take(max_len - len);
    }
    // remove null bytes
    str_bytes.erase(std::remove(str_bytes.begin(), str_bytes.end(), '\0'), str_bytes.end());
    return std::string(str_bytes.begin(), str_bytes.end());
}

static void ip4_to_string(const uint8_t ip[4], char* out, size_t n) {
    // writes "A.B.C.D"
    snprintf(out, n, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

std::string Reader::ip_reversed() {
    char buf[16]; // max "255.255.255.255" + NUL
    std::vector<uint8_t> ip_bytes = take(4);
    uint8_t rev[4] = { ip_bytes[3], ip_bytes[2], ip_bytes[1], ip_bytes[0] };
    ip4_to_string(rev, buf, sizeof(buf));
    return std::string{buf};
}

PhoneLoginRecord parse_phone_login(const std::vector<uint8_t>& payload) {
    Reader r(payload);
    PhoneLoginRecord rec;

    rec.session_id = r.u32();
    rec.user_id = r.u32();
    rec.communication_secret_key = r.take(32);
    rec.app_secret_key = r.take(32);
    rec.comm_enc_mode = r.u8();
    rec.app_enc_mode = r.u8();
    rec.server_time = r.take(4);

    uint16_t tmp_short = r.u16();
    rec.block_len_bytes = tmp_short * 2;
    rec.block_a = r.take(rec.block_len_bytes);
    rec.block_b = r.take(rec.block_len_bytes);

    rec.gateway_domain = r.take(32);
    rec.gateway_port = r.u16();
    rec.gateway_proto = r.u8();

    rec.standby_gateway_domain = r.take(32);
    rec.standby_gateway_port = r.u16();
    rec.standby_gateway_proto = r.u8();

    rec.tcp_server_ip = r.take(4);
    rec.tcp_server_port = r.u16();
    rec.tcp_server_proto = r.u8();

    rec.heartbeat_secs = r.u16();
    rec.reply_timeout_secs = r.u16();

    rec.token = r.take(32);
    rec.nickname = r.take(32);

    return rec;
}

ProtocolMessage parse_protocol_packet(const std::vector<uint8_t>& payload) {
    ProtocolMessage packet;
    Reader r(payload);

    packet.start_flag = r.u16();
    packet.length = r.u16();
    packet.version = r.u16();
    packet.cmd = r.u16();
    packet.session = r.u32();
    packet.serial = r.u16();
    packet.direction = r.u8();
    packet.err_code = r.u8();
    packet.control_attr = r.u16();
    packet.user_id = r.u32();
    packet.timestamp = r.u32();

    size_t header_size = 40;
    size_t crc_size = 4;
    if (payload.size() < header_size + crc_size) {
        throw std::out_of_range("Packet too small");
    }

    packet.raw_header = std::vector<uint8_t>(payload.begin(), payload.begin() + header_size);
    if (packet.length > header_size + crc_size) {
        packet.payload = std::vector<uint8_t>(payload.begin() + header_size, payload.begin() + packet.length - crc_size);
    } else {
        packet.payload = {};
    }
    packet.crc = std::vector<uint8_t>(payload.begin() + packet.length - crc_size, payload.begin() + packet.length);

    return packet;
}


SwitchStatus parse_switch_status(const std::vector<uint8_t>& payload) {
    auto& logger = e7_switcher::Logger::instance();
    logger.debugf("Parsing switch status from %d bytes", payload.size());
    Reader r(payload);
    r.take(2); // original cmd
    r.take(2); // original serial
    r.take(4); // original timestamp
    r.take(1); // needs to be 0 or 3
    r.take(2); // length of rest of payload

    r.take(32); // device name

    SwitchStatus status;
    status.online_state = r.u8();
    
    r.take(2); // length of status bytes

    status.wifi_power = r.u8();
    status.switch_state = r.u8();
    status.remaining_time = r.u32();
    status.open_time = r.u32();
    status.auto_closing_time = r.u32();
    status.is_delay = r.u8();

    return status;
}

ACStatus parse_ac_status_from_query_payload(const std::vector<uint8_t>& payload) {
    auto& logger = e7_switcher::Logger::instance();
    Reader r(payload);
    r.take(2); // original cmd
    r.take(2); // original serial
    r.take(4); // original timestamp
    r.take(1); // needs to be 0 or 3
    r.take(2); // length of rest of payload

    r.take(32); // device name

    int online_state = r.u8();
    
    int status_bytes_len = r.u16(); // length of status bytes
    std::vector<uint8_t> status_bytes = r.take(status_bytes_len);
    ACStatus status = parse_ac_status_from_work_status_bytes(status_bytes);
    status.online_state = online_state;

    return status;
}

ACStatus parse_ac_status_from_work_status_bytes(const std::vector<uint8_t>& work_status_bytes)
{
    auto& logger = e7_switcher::Logger::instance();
    logger.debugf("Parsing AC status from %d bytes", work_status_bytes.size());
    
    ACStatus status;
    Reader r(work_status_bytes);
    
    if ((work_status_bytes.size() != 32) && (work_status_bytes.size() != 30)) {
        logger.error("AC status payload size is not 32 bytes or 30 bytes");
        return status;
    }
    
    status.wifi_power = r.u8();
    status.temperature = r.u16() / 10.0;
    // ac_data: [on_or_off_code, mode, temperature, fan_code * 16 + swing_code]
    status.ac_data = r.take(4);
    status.power_status = static_cast<ACPower>(status.ac_data[0]);
    status.mode = static_cast<ACMode>(status.ac_data[1]);
    status.ac_temperature = status.ac_data[2];
    status.fan_speed = static_cast<ACFanSpeed>(status.ac_data[3] / 16);
    status.swing = static_cast<ACSwing>(status.ac_data[3] % 16);
    status.temperature_unit = r.u8();
    status.device_type = r.u8();
    
    // Read code ID (8 bytes string)
    std::vector<uint8_t> code_id_bytes = r.take(8);
    // Remove null bytes and convert to string
    code_id_bytes.erase(std::remove(code_id_bytes.begin(), code_id_bytes.end(), '\0'), code_id_bytes.end());
    status.code_id = std::string(code_id_bytes.begin(), code_id_bytes.end());
    
    // Check if online_state is 3 (this should be set before calling this function)
    // For now, we'll assume it's not 3 and read the remaining fields
    status.last_time = r.u32();
    status.open_time = r.u32();
    status.auto_closing_time = r.u32();
    status.is_delay = r.u8() & 255;
    
    return status;
}

} // namespace e7_switcher
