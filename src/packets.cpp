#include "packets.h"
#include "constants.h"
#include "crc.h"
#include "crypto.h"
#include "time_utils.h"
#include <vector>
#include <string>
#include <cstring>

namespace e7_switcher {

namespace {

std::vector<uint8_t> build_header(
    uint16_t payload_len,
    uint16_t cmd_code,
    int32_t session,
    uint16_t serial,
    uint16_t control_attr,
    uint8_t direction,
    uint8_t errcode,
    int32_t user_id,
    bool is_version_2
) {
    std::vector<uint8_t> hdr(HEADER_SIZE, 0);
    uint16_t total_len = payload_len + HEADER_SIZE + CRC_TAIL_SIZE;

    hdr[0] = MAGIC1 & 0xFF;
    hdr[1] = (MAGIC1 >> 8) & 0xFF;
    hdr[2] = total_len & 0xFF;
    hdr[3] = (total_len >> 8) & 0xFF;
    uint16_t version = is_version_2 ? PROTO_VER_2 : PROTO_VER_3;
    hdr[4] = version & 0xFF;
    hdr[5] = (version >> 8) & 0xFF;
    hdr[6] = cmd_code & 0xFF;
    hdr[7] = (cmd_code >> 8) & 0xFF;
    hdr[8] = session & 0xFF;
    hdr[9] = (session >> 8) & 0xFF;
    hdr[10] = (session >> 16) & 0xFF;
    hdr[11] = (session >> 24) & 0xFF;
    hdr[12] = serial & 0xFF;
    hdr[13] = (serial >> 8) & 0xFF;
    hdr[14] = direction & 0xFF;
    hdr[15] = errcode;
    hdr[16] = control_attr & 0xFF;
    hdr[17] = (control_attr >> 8) & 0xFF;
    hdr[18] = user_id & 0xFF;
    hdr[19] = (user_id >> 8) & 0xFF;
    hdr[20] = (user_id >> 16) & 0xFF;
    hdr[21] = (user_id >> 24) & 0xFF;
    uint32_t ts = now_unix();
    hdr[24] = ts & 0xFF;
    hdr[25] = (ts >> 8) & 0xFF;
    hdr[26] = (ts >> 16) & 0xFF;
    hdr[27] = (ts >> 24) & 0xFF;
    hdr[38] = MAGIC2 & 0xFF;
    hdr[39] = (MAGIC2 >> 8) & 0xFF;

    return hdr;
}

std::vector<uint8_t> fixed_len_str(const std::string& s, size_t n) {
    std::vector<uint8_t> b(n, 0);
    std::memcpy(b.data(), s.c_str(), std::min(s.length(), n));
    return b;
}

} // namespace

std::vector<uint8_t> build_login_payload(
    const std::string& account,
    const std::string& password
) {
    uint16_t serial = 1090;
    uint8_t direction = 1;
    uint8_t errcode = 0;
    uint16_t control_attr = 0x0100;

    std::vector<uint8_t> buf(160, 0);
    size_t off = 0;

    buf[off++] = 1;
    std::memcpy(buf.data() + off, DEFAULT_BUILD_VERSION, 16); off += 16;
    buf[off++] = 2;
    buf[off++] = 50;
    std::memcpy(buf.data() + off, DEFAULT_SHORT_APP_ID, 8); off += 8;
    std::memcpy(buf.data() + off, DEFAULT_PACKAGE_VERSION, 2); off += 2;
    std::memcpy(buf.data() + off, DEFAULT_MAC, 6); off += 6;
    std::memcpy(buf.data() + off, DEFAULT_BSSID, 6); off += 6;
    // IP address is not used in the python code, so we can leave it as 0
    off += 4;
    buf[off++] = 3;
    std::vector<uint8_t> acc_b = fixed_len_str(account, 32);
    std::memcpy(buf.data() + off, acc_b.data(), 32); off += 32;
    std::vector<uint8_t> pwd_b = fixed_len_str(password, 32);
    std::memcpy(buf.data() + off, pwd_b.data(), 32); off += 32;
    off += 4; // uid
    int32_t ts = java_bug_seconds_from_now();
    buf[off++] = ts & 0xFF;
    buf[off++] = (ts >> 8) & 0xFF;
    buf[off++] = (ts >> 16) & 0xFF;
    buf[off++] = (ts >> 24) & 0xFF;
    off += 32; // trailing 32 zeros
    for(int i=0; i<10; ++i) buf[off++] = 0x0a;

    std::vector<uint8_t> encrypted = encrypt_to_hex_ecb_pkcs7(buf, AES_KEY_2_50);

    std::vector<uint8_t> header = build_header(encrypted.size(), CMD_LOGIN, 0, serial, control_attr, direction, errcode, 0, true);

    std::vector<uint8_t> packet = header;
    packet.insert(packet.end(), encrypted.begin(), encrypted.end());

    std::vector<uint8_t> crc = get_complete_legal_crc(packet, {});
    packet.insert(packet.end(), crc.begin(), crc.end());

    return packet;
}

std::vector<uint8_t> build_device_list_payload(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key
) {
    std::vector<uint8_t> header = build_header(0, CMD_DEVICE_LIST, session_id, 1102, 0, 1, 0, user_id, false);
    std::vector<uint8_t> crc = get_complete_legal_crc(header, communication_secret_key);
    header.insert(header.end(), crc.begin(), crc.end());
    return header;
}

std::vector<uint8_t> build_device_control_payload(
    int32_t session_id,
    int32_t user_id,
    const std::vector<uint8_t>& communication_secret_key,
    int32_t device_id,
    const std::vector<uint8_t>& device_pwd,
    int on_or_off
) {
    std::vector<uint8_t> buf(49, 0);
    size_t off = 0;

    buf[off++] = device_id & 0xFF;
    buf[off++] = (device_id >> 8) & 0xFF;
    buf[off++] = (device_id >> 16) & 0xFF;
    buf[off++] = (device_id >> 24) & 0xFF;

    buf[off++] = user_id & 0xFF;
    buf[off++] = (user_id >> 8) & 0xFF;
    buf[off++] = (user_id >> 16) & 0xFF;
    buf[off++] = (user_id >> 24) & 0xFF;

    std::vector<uint8_t> padded_pwd = device_pwd;
    padded_pwd.resize(32, 0);
    std::vector<uint8_t> encrypted_pwd = encrypt_to_hex_ecb_pkcs7(padded_pwd, AES_KEY_NATIVE);
    std::memcpy(buf.data() + off, encrypted_pwd.data(), 32); off += 32;

    buf[off++] = 0x0A;
    buf[off++] = 0x06;
    buf[off++] = 0x00;
    buf[off++] = 0x01; // line_type
    buf[off++] = on_or_off & 0xFF;
    off += 4; // closing time

    std::vector<uint8_t> header = build_header(buf.size(), CMD_DEVICE_CONTROL, session_id, 1104, 0, 1, 0, user_id, false);

    std::vector<uint8_t> packet = header;
    packet.insert(packet.end(), buf.begin(), buf.end());

    std::vector<uint8_t> crc = get_complete_legal_crc(packet, communication_secret_key);
    packet.insert(packet.end(), crc.begin(), crc.end());

    return packet;
}

} // namespace e7_switcher
