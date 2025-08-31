#include "communication.h"
#include "constants.h"
#include "packets.h"
#include "parser.h"
#include "crypto.h"
#include "base64_decode.h"
#include <ArduinoJson.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

using namespace std;

namespace e7_switcher {

namespace {
inline uint16_t le16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
}

// -------------------------------------------------------------------------------------
// ctor/dtor/connect as before (unchanged) except we clear inbuf_ on connect
// -------------------------------------------------------------------------------------

E7Connection::E7Connection(const std::string& host, int port, int timeout) 
    : host_(host), port_(port), timeout_(timeout), sock_(-1), session_id_(0), user_id_(0) {}

E7Connection::~E7Connection() {
    if (sock_ != -1) {
        ::close(sock_);
    }
}

void E7Connection::connect() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) throw std::runtime_error("Failed to create socket");

    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, host_.c_str(), &serv_addr.sin_addr) <= 0)
        throw std::runtime_error("Invalid address/ Address not supported");

    if (::connect(sock_, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        throw std::runtime_error("Connection Failed");

    // default short timeout (seconds)
    struct timeval tv{};
    tv.tv_sec  = timeout_;
    tv.tv_usec = 0;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    inbuf_.clear();
}

void E7Connection::close() {
    if (sock_ != -1) {
        ::close(sock_);
        sock_ = -1;
    }
}

// -------------------------------------------------------------------------------------
// New: send_message
// -------------------------------------------------------------------------------------
void E7Connection::send_message(const std::vector<uint8_t>& data) {
    if (sock_ == -1) throw std::runtime_error("Not connected");
    const uint8_t* p = data.data();
    size_t left = data.size();
    while (left > 0) {
        ssize_t n = ::send(sock_, p, left, 0);
        if (n < 0) throw std::runtime_error("Send failed");
        p += n;
        left -= static_cast<size_t>(n);
    }
}

// -------------------------------------------------------------------------------------
// New: receive_message (streaming, framed by header/payload/crc)
// -------------------------------------------------------------------------------------
std::vector<uint8_t> E7Connection::receive_message(int timeout_ms) {
    if (sock_ == -1) throw std::runtime_error("Not connected");

    // Temporarily extend SO_RCVTIMEO for this call; restore after.
    struct timeval oldtv{}, newtv{};
    socklen_t optlen = sizeof(oldtv);
    getsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, &oldtv, &optlen);
    newtv.tv_sec  = timeout_ms / 1000;
    newtv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&newtv, sizeof newtv);

    std::vector<uint8_t> out;

    // Try extracting if already buffered
    if (try_extract_one_packet(out)) {
        // Restore old timeout and return
        setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&oldtv, sizeof oldtv);
        return out;
    }

    // Keep reading until a full packet is available or timeout hits
    const size_t READ_CHUNK = 4096;
    std::vector<uint8_t> tmp;
    tmp.resize(READ_CHUNK);

    while (true) {
        ssize_t n = ::recv(sock_, tmp.data(), READ_CHUNK, 0);
        if (n < 0) {
            // respect socket's SO_RCVTIMEO (errno == EAGAIN/EWOULDBLOCK typically)
            setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&oldtv, sizeof oldtv);
            throw std::runtime_error("Receive timeout or error");
        } else if (n == 0) {
            setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&oldtv, sizeof oldtv);
            throw std::runtime_error("Peer closed connection");
        }
        inbuf_.insert(inbuf_.end(), tmp.begin(), tmp.begin() + n);

        if (try_extract_one_packet(out)) {
            setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&oldtv, sizeof oldtv);
            return out;
        }
        // otherwise, loop to read more bytes
    }
}

// -------------------------------------------------------------------------------------
// Internal: try to parse exactly one packet from inbuf_
// Returns true and fills 'out' if a full packet is available.
// -------------------------------------------------------------------------------------
bool E7Connection::try_extract_one_packet(std::vector<uint8_t>& out) {
    // Search for start-of-header (FE F0)
    size_t i = 0;
    while (true) {
        // need at least header size to proceed
        if (inbuf_.size() - i < HEADER_SIZE) {
            // drop garbage before i (if any), but keep partial header in buffer
            if (i > 0) inbuf_.erase(inbuf_.begin(), inbuf_.begin() + i);
            return false;
        }

        // Look for start marker
        uint16_t maybe_marker = le16(&inbuf_[i]);
        if (maybe_marker == MAGIC1) {
            // Verify header tail markers present (we have >= HEADER_SIZE here)
            uint16_t maybe_tail = le16(&inbuf_[i + 38]);
            if (maybe_tail == MAGIC2) {
                // Parse total length (little-endian) from bytes [2..3]
                uint16_t total_len = le16(&inbuf_[i + 2]);

                // Sanity check: header+crc minimum
                if (total_len < HEADER_SIZE + CRC_TAIL_SIZE) {
                    // corrupt; skip one byte and continue searching
                    ++i;
                    continue;
                }

                // If the full packet isn't yet buffered, wait for more bytes
                if (inbuf_.size() - i < total_len) {
                    // keep what's before i? it's not a valid header, but i points to a plausible header start
                    if (i > 0) inbuf_.erase(inbuf_.begin(), inbuf_.begin() + i);
                    return false;
                }

                // Extract packet
                out.assign(inbuf_.begin() + i, inbuf_.begin() + i + total_len);
                // Erase consumed bytes (including any junk before header)
                inbuf_.erase(inbuf_.begin(), inbuf_.begin() + i + total_len);
                return true;
            } else {
                // Not a valid header end; move forward one byte
                ++i;
            }
        } else {
            ++i;
        }
    }
}

// -------------------------------------------------------------------------------------
// High-level operations now use send_message / receive_message
// -------------------------------------------------------------------------------------

PhoneLoginRecord E7Connection::login(const std::string& account, const std::string& password) {
    std::vector<uint8_t> login_packet = build_login_payload(account, password);
    send_message(login_packet);
    std::vector<uint8_t> response = receive_message();

    ProtocolPacket parsed_response = parse_protocol_packet(response);
    std::vector<uint8_t> decrypted_payload = decrypt_hex_ecb_pkcs7(parsed_response.payload, AES_KEY_2_50);
    PhoneLoginRecord login_data = parse_phone_login(decrypted_payload);

    session_id_ = login_data.session_id;
    user_id_ = login_data.user_id;
    communication_secret_key_ = login_data.communication_secret_key;
    Serial.printf("Phone login successful with session ID: %d\n", login_data.session_id);
    return login_data;
}

std::vector<Device> E7Connection::get_device_list() {
    std::vector<uint8_t> pkt = build_device_list_payload(session_id_, user_id_, communication_secret_key_);
    send_message(pkt);
    std::vector<uint8_t> response = receive_message(); // ignore (ack)

    ProtocolPacket parsed_response = parse_protocol_packet(response);
    std::string payload_str(parsed_response.payload.begin(), parsed_response.payload.end());
    size_t start = payload_str.find("{");
    size_t end   = payload_str.rfind("}");
    if (start == std::string::npos || end == std::string::npos || end <= start)
        throw std::runtime_error("No valid JSON found in response");
    std::string json_str = payload_str.substr(start, end - start + 1);
    json_str.erase(std::remove(json_str.begin(), json_str.end(), '\0'), json_str.end());

    JsonDocument doc;
    deserializeJson(doc, json_str);
    JsonArray dev_list = doc["DevList"].as<JsonArray>();

    std::vector<Device> devices;
    for (JsonObject item : dev_list) {
        Device dev;
        dev.name     = item["DeviceName"].as<std::string>();
        dev.ssid     = item["APSSID"].as<std::string>();
        dev.mac      = item["DMAC"].as<std::string>();
        dev.type     = item["DeviceType"].as<std::string>();
        dev.firmware = item["FirmwareMark"].as<std::string>() + " " + item["FirmwareVersion"].as<std::string>();
        dev.online   = item["OnlineStatus"].as<int>() == 1;
        dev.line_no  = item["LineNo"].as<int>();
        dev.line_type= item["LineType"].as<int>();
        dev.did      = item["DID"].as<int>();
        dev.visit_pwd= item["VisitPwd"].as<std::string>();
        devices.push_back(dev);
    }
    return devices;
}

ProtocolPacket E7Connection::control_device(const std::string& device_name, const std::string& action) {
    Serial.printf("Start of control_device\n");
    std::vector<Device> devices = get_device_list();
    Serial.printf("Got device list\n");
    auto it = std::find_if(devices.begin(), devices.end(), [&](const Device& d) { return d.name == device_name; });
    if (it == devices.end()) throw std::runtime_error("Device not found");

    if (it->type != "0F04") throw std::runtime_error("Device type not supported");

    std::vector<unsigned char> enc_pwd_bytes = base64_decode(it->visit_pwd);
    std::vector<uint8_t> dec_pwd_bytes = decrypt_hex_ecb_pkcs7(
        enc_pwd_bytes, std::string(communication_secret_key_.begin(), communication_secret_key_.end()));
    int on_or_off = (action == "on") ? 1 : 0;

    std::vector<uint8_t> control_packet = build_device_control_payload(
        session_id_, user_id_, communication_secret_key_, it->did, dec_pwd_bytes, on_or_off);

    Serial.printf("Sending control command to \"%s\"...\n", device_name.c_str());
    send_message(control_packet);                 // send
    (void)receive_message();  // ignore ack, but drain it
    Serial.printf("Control command sent to \"%s\"\n", device_name.c_str());

    // async status response
    std::vector<uint8_t> response = receive_message();
    Serial.printf("Received response from \"%s\"\n", device_name.c_str());
    return parse_protocol_packet(response);
}

LightStatus E7Connection::get_light_status(const std::string& device_name) {
    std::vector<Device> devices = get_device_list();
    auto it = std::find_if(devices.begin(), devices.end(), [&](const Device& d) { return d.name == device_name; });
    if (it == devices.end()) throw std::runtime_error("Device not found");

    if (it->type != "0F04") throw std::runtime_error("Device type not supported");

    std::vector<uint8_t> query_packet = build_device_query_payload(
        session_id_, user_id_, communication_secret_key_, it->did);

    send_message(query_packet);
    (void)receive_message(); // drain ack
    std::vector<uint8_t> response = receive_message();

    std::vector<uint8_t> payload = parse_protocol_packet(response).payload;
    return parse_light_status(payload);
}

} // namespace e7_switcher
