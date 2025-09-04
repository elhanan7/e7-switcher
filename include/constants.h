#pragma once

#include <cstdint>

namespace e7_switcher {

// Protocol constants
constexpr int HEADER_SIZE = 0x28; // 40
constexpr int CRC_TAIL_SIZE = 4;
constexpr uint16_t MAGIC1 = 0xF0FE; // header[0:2] (LE -> fe f0 on wire)
constexpr int SOCKET_BUFFER_SIZE = 4096;
constexpr uint16_t PROTO_VER_2 = 0x3202;
constexpr uint16_t PROTO_VER_3 = 0x0503;
constexpr uint16_t MAGIC2 = 0xFEF0; // header[0x26:0x28] (LE -> f0 fe on wire)

// Server configuration
const char* const IP_HUB = "47.91.75.117";
constexpr int PORT_HUB = 9091;

// AES configuration
const char* const AES_KEY_2_50 = "OGEseetime201800";
const char* const AES_KEY_NATIVE = "OGE201600000000000000000";
const char* const AES_KEY_PRIMARY = "OGE201601234567890123456";

// Default device configuration
const unsigned char DEFAULT_BUILD_VERSION[] = "16\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
const unsigned char DEFAULT_SHORT_APP_ID[] = "PAMDS001";
const unsigned char DEFAULT_PACKAGE_VERSION[] = "\x07\x14";
const unsigned char DEFAULT_MAC[] = "\x00\x00\x00\x00\x00\x00";
const unsigned char DEFAULT_BSSID[] = "\x00\x00\x00\x00\x00\x00";

// Command codes
constexpr uint16_t CMD_LOGIN = 0x1102;
constexpr uint16_t CMD_DEVICE_LIST = 0x1343;
constexpr uint16_t CMD_HEARTBEAT = 0x1517;
constexpr uint16_t CMD_DEVICE_CONTROL = 0x0201;
constexpr uint16_t CMD_DEVICE_QUERY = 0x0301;
constexpr uint16_t CMD_AC_IR_CONFIG_QUERY = 0x1A04;

} // namespace e7_switcher
