#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace e7_switcher {

class Writer {
public:
    Writer(std::vector<uint8_t>& data);

    void u8(uint8_t b);
    void u16(uint16_t s);
    void u32(uint32_t i);
    void put(const std::vector<uint8_t>& d);
    void put(const uint8_t* d, size_t n);
    void put_constant(uint8_t b, size_t n);

private:
    void _need(size_t n);

    std::vector<uint8_t>& data_;
    size_t p_;
};

std::vector<uint8_t> build_login_payload(
    const std::string& account,
    const std::string& password
);

std::vector<uint8_t> build_device_list_payload(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key
);

std::vector<uint8_t> build_device_control_payload(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id,
    const std::vector<uint8_t>& device_pwd,
    int on_or_off
);

std::vector<uint8_t> build_device_query_payload(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id
);

} // namespace e7_switcher
