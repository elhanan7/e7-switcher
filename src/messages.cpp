#if defined(ARDUINO) || defined(ESP_PLATFORM) || defined(ESP32) || defined(ESP8266)
#define E7_PLATFORM_ESP 1
#include <Arduino.h>
#else
#define E7_PLATFORM_DESKTOP 1
// Desktop platform - no Arduino headers needed
#endif
#include "e7-switcher/messages.h"
#include "e7-switcher/constants.h"
#include "e7-switcher/crc.h"
#include "e7-switcher/crypto.h"
#include "e7-switcher/time_utils.h"
#include "e7-switcher/logger.h"
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>

namespace e7_switcher {

class Writer {
public:
    Writer(std::vector<uint8_t>& data);

    void u8(uint8_t b);
    void u16(uint16_t s);
    void u32(uint32_t i);
    void put(const std::vector<uint8_t>& d);
    void put(const std::string& s);
    void put(const uint8_t* d, size_t n);
    void put_constant(uint8_t b, size_t n);

private:
    void _need(size_t n);

    std::vector<uint8_t>& data_;
    size_t p_;
};

namespace {

std::vector<uint8_t> fixed_len_str(const std::string& s, size_t n) {
    std::vector<uint8_t> b(n, 0);
    std::memcpy(b.data(), s.c_str(), std::min(s.length(), n));
    return b;
}

} // namespace

ProtocolMessage build_protocol_message(
    uint16_t cmd_code,
    int32_t session,
    uint16_t serial,
    uint16_t control_attr,
    uint8_t direction,
    uint8_t errcode,
    int32_t user_id,
    const std::vector<uint8_t>& payload,
    const std::vector<uint8_t>& communication_secret_key,
    bool is_version_2
) {
    // Create a header
    std::vector<uint8_t> header(HEADER_SIZE, 0);
    Writer w(header);
    uint16_t total_len = payload.size() + HEADER_SIZE + CRC_TAIL_SIZE;

    w.u16(MAGIC1); // [0, 2]
    w.u16(total_len); // [2, 4]
    uint16_t version = is_version_2 ? PROTO_VER_2 : PROTO_VER_3;
    w.u16(version); // [4, 6]
    w.u16(cmd_code); // [6, 8]
    w.u32(session); // [8, 12]
    w.u16(serial); // [12, 14]
    w.u8(direction); // [14, 15]
    w.u8(errcode); // [15, 16]
    w.u16(control_attr); // [16, 18]
    w.u32(user_id); // [18, 22]
    w.put_constant(0, 2); // [22, 24] user id is encoded with 6 bytes
    uint32_t ts = now_unix();
    w.u32(ts); // [24, 28]
    w.put_constant(0, 10); // [28, 38] reserved
    w.u16(MAGIC2); // [38, 40]

    // Create the full packet for CRC calculation
    std::vector<uint8_t> packet = header;
    if (!payload.empty()) {
        packet.insert(packet.end(), payload.begin(), payload.end());
    }

    // Calculate CRC
    std::vector<uint8_t> crc = get_complete_legal_crc(packet, communication_secret_key);

    // Create and populate the ProtocolMessage
    ProtocolMessage message;
    message.start_flag = MAGIC1;
    message.length = total_len;
    message.version = version;
    message.cmd = cmd_code;
    message.session = session;
    message.serial = serial;
    message.direction = direction;
    message.err_code = errcode;
    message.control_attr = control_attr;
    message.user_id = user_id;
    message.timestamp = ts;
    message.raw_header = header;
    message.payload = payload;
    message.crc = crc;

    return message;
}

Writer::Writer(std::vector<uint8_t>& data) : data_(data), p_(0) {}

void Writer::u8(uint8_t b) {
    _need(1);
    data_[p_++] = (b & 0xFF);
}
void Writer::u16(uint16_t s) {
    _need(2);
    data_[p_++] = s & 0xFF;
    data_[p_++] = (s >> 8) & 0xFF;
}
void Writer::u32(uint32_t i) {
    _need(4);
    data_[p_++] = i & 0xFF;
    data_[p_++] = (i >> 8) & 0xFF;
    data_[p_++] = (i >> 16) & 0xFF;
    data_[p_++] = (i >> 24) & 0xFF;
}

void Writer::put(const std::vector<uint8_t>& d) {
    _need(d.size());
    std::memcpy(data_.data() + p_, d.data(), d.size());
    p_ += d.size();
}

void Writer::put(const uint8_t* d, size_t n) {
    _need(n);
    std::memcpy(data_.data() + p_, d, n);
    p_ += n;
}

void Writer::put(const std::string& s) {
    _need(s.size());
    std::memcpy(data_.data() + p_, s.c_str(), s.size());
    p_ += s.size();
}

void Writer::put_constant(uint8_t b, size_t n) {
    _need(n);
    std::memset(data_.data() + p_, b, n);
    p_ += n;
}

void Writer::_need(size_t n) {
    if (p_ + n > data_.size()) {
        throw std::out_of_range("Not enough room in buffer");
    }
}

ProtocolMessage build_login_message(
    const std::string& account,
    const std::string& password
) {
    uint16_t serial = 1090;
    uint8_t direction = 1;
    uint8_t errcode = 0;
    uint16_t control_attr = 0x0100;

    std::vector<uint8_t> buf(160, 0);
    Writer w(buf);

    w.u8(1);
    w.put(DEFAULT_BUILD_VERSION, 16);
    w.u8(2);
    w.u8(50);
    
    w.put(DEFAULT_SHORT_APP_ID, 8);
    w.put(DEFAULT_PACKAGE_VERSION, 2);
    w.put(DEFAULT_MAC, 6);
    w.put(DEFAULT_BSSID, 6);
    
    // IP address is not used in the python code, so we can leave it as 0
    w.u32(0);
    w.u8(3);
    std::vector<uint8_t> acc_b = fixed_len_str(account, 32);
    w.put(acc_b);
    
    std::vector<uint8_t> pwd_b = fixed_len_str(password, 32);
    w.put(pwd_b);
    
    w.u32(0); // user id
    int32_t ts = java_bug_seconds_from_now();
    w.u32(ts);
    w.put_constant(0, 32);
    w.put_constant(0x0A, 10);

    std::vector<uint8_t> encrypted = encrypt_to_hex_ecb_pkcs7(buf, AES_KEY_2_50);

    // Use the new build_protocol_message function
    return build_protocol_message(
        CMD_LOGIN,      // cmd_code
        0,              // session
        serial,         // serial
        control_attr,   // control_attr
        direction,      // direction
        errcode,        // errcode
        0,              // user_id
        encrypted,      // payload
        {},             // communication_secret_key
        true            // is_version_2
    );
}

ProtocolMessage build_device_list_message(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key
) {
    return build_protocol_message(
        CMD_DEVICE_LIST,  // cmd_code
        session_id,       // session
        1102,             // serial
        0,                // control_attr
        1,                // direction
        0,                // errcode
        user_id,          // user_id
        {},               // payload (empty)
        communication_secret_key, // communication_secret_key
        false             // is_version_2
    );
}

ProtocolMessage build_switch_control_message(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id,
    const std::vector<uint8_t>& device_pwd,
    int on_or_off
) {
    auto& logger = e7_switcher::Logger::instance();
    logger.debugf("Building device control packet for device %d", device_id);
    std::vector<uint8_t> buf(49, 0);
    Writer w(buf);

    w.u32(device_id);
    w.u32(user_id);
    
    std::vector<uint8_t> padded_pwd = device_pwd;
    padded_pwd.resize(32, 0);
    std::vector<uint8_t> encrypted_pwd = encrypt_to_hex_ecb_pkcs7(padded_pwd, AES_KEY_NATIVE);
    encrypted_pwd.resize(32);
    w.put(encrypted_pwd);

    w.u8(0x0A);
    w.u8(0x06);
    w.u8(0x00);
    w.u8(0x01); // line_type
    w.u8(on_or_off);
    w.u32(0); // closing time
    
    auto message = build_protocol_message(
        CMD_DEVICE_CONTROL, // cmd_code
        session_id,         // session
        1104,               // serial
        0,                  // control_attr
        1,                  // direction
        0,                  // errcode
        user_id,            // user_id
        buf,                // payload
        communication_secret_key, // communication_secret_key
        false               // is_version_2
    );
    
    logger.debugf("Built device control packet for device %d", device_id);
    return message;
}

ProtocolMessage build_device_query_message(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id
) {
    std::vector<uint8_t> buf(4, 0);
    Writer w(buf);

    w.u32(device_id);
    
    return build_protocol_message(
        CMD_DEVICE_QUERY,  // cmd_code
        session_id,        // session
        1104,              // serial
        0,                 // control_attr
        1,                 // direction
        0,                 // errcode
        user_id,           // user_id
        buf,               // payload
        communication_secret_key, // communication_secret_key
        false              // is_version_2
    );
}

ProtocolMessage build_ac_ir_config_query_message(int32_t session_id, int32_t user_id, const std::vector<uint8_t> &communication_secret_key, int32_t device_id, std::string ac_code_id)
{
    std::vector<uint8_t> buf(16, 0);
    Writer w(buf);

    w.u32(user_id);
    w.u32(device_id);

    // convert ac_code_id to unit8_t vector
    std::vector<uint8_t> ac_code_id_bytes(ac_code_id.begin(), ac_code_id.end());
    ac_code_id_bytes.resize(8, 0);
    
    w.put(ac_code_id_bytes);
    
    return build_protocol_message(
        CMD_AC_IR_CONFIG_QUERY, // cmd_code
        session_id,             // session
        1110,                   // serial
        0,                      // control_attr
        1,                      // direction
        0,                      // errcode
        user_id,                // user_id
        buf,                    // payload
        communication_secret_key, // communication_secret_key
        false                   // is_version_2
    );
}

ProtocolMessage build_ac_control_message(int32_t session_id, int32_t user_id,
                                              const std::vector<uint8_t> &communication_secret_key, int32_t device_id, 
                                              const std::vector<uint8_t> &device_pwd, const std::string &control_str,
                                              int operationTime)
{
    size_t buffer_length = control_str.length() + 47;
    std::vector<uint8_t> buf(buffer_length, 0);
    Writer w(buf);

    w.u32(device_id);
    w.u32(user_id);

    std::vector<uint8_t> padded_pwd = device_pwd;
    padded_pwd.resize(32, 0);
    auto& logger = e7_switcher::Logger::instance();
    
    std::vector<uint8_t> encrypted_pwd = encrypt_to_hex_ecb_pkcs7(padded_pwd, AES_KEY_NATIVE);
    encrypted_pwd.resize(32);
    w.put(encrypted_pwd);
    w.u8(1);
    w.u16(control_str.length() + 4);
    w.u32(operationTime);
    w.put(control_str);

    return build_protocol_message(
        CMD_DEVICE_CONTROL,  // cmd_code
        session_id,          // session
        1111,                // serial
        0,                   // control_attr
        1,                   // direction
        0,                   // errcode
        user_id,             // user_id
        buf,                 // payload
        communication_secret_key, // communication_secret_key
        false                // is_version_2
    );
}

} // namespace e7_switcher
