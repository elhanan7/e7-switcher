#include "parser.h"
#include <stdexcept>
#include <arpa/inet.h>
#include <algorithm>

namespace e7_switcher {

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

ProtocolPacket parse_protocol_packet(const std::vector<uint8_t>& payload) {
    ProtocolPacket packet;
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

} // namespace e7_switcher
