#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "parser.h"

namespace e7_switcher {

/**
 * Creates a complete ProtocolMessage with header, payload, and CRC
 * 
 * @param cmd_code Command code for the message
 * @param session Session ID
 * @param serial Serial number
 * @param control_attr Control attribute
 * @param direction Direction (usually 1 for client to server)
 * @param errcode Error code (usually 0)
 * @param user_id User ID
 * @param payload Payload data
 * @param communication_secret_key Key used for CRC calculation
 * @param is_version_2 Whether to use protocol version 2 (true) or 3 (false)
 * @return A complete ProtocolMessage object
 */
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
    bool is_version_2 = false
);

ProtocolMessage build_login_message(
    const std::string& account,
    const std::string& password
);

ProtocolMessage build_device_list_message(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key
);

ProtocolMessage build_switch_control_message(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id,
    const std::vector<uint8_t>& device_pwd,
    int on_or_off,
    int operation_time = 0
);

ProtocolMessage build_boiler_control_message(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id,
    const std::vector<uint8_t>& device_pwd,
    int on_or_off,
    int operation_time = 0
);


ProtocolMessage build_device_query_message(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id
);

ProtocolMessage build_ac_ir_config_query_message(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id,
    std::string ac_code_id
);

ProtocolMessage build_ac_control_message(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id,
    const std::vector<uint8_t>& device_pwd,
    const std::string& control_str,
    int operation_time = 0
);


} // namespace e7_switcher
