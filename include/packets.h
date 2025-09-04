#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace e7_switcher {

std::vector<uint8_t> build_login_payload(
    const std::string& account,
    const std::string& password
);

std::vector<uint8_t> build_device_list_payload(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key
);

std::vector<uint8_t> build_switch_control_payload(
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

std::vector<uint8_t> build_ac_ir_config_query_payload(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id,
    std::string ac_code_id
);

std::vector<uint8_t> build_ac_control_payload(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id,
    const std::vector<uint8_t>& device_pwd,
    const std::string& control_str,
    int operationTime = 0
);


} // namespace e7_switcher
