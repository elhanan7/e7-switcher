#include "e7_switcher_client.h"
#include "constants.h"
#include "packets.h"
#include "parser.h"
#include "crypto.h"
#include "base64_decode.h"
#include "logger.h"
#include "compression.h"
#include "json_helpers.h"

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
#include <cstdint>

namespace e7_switcher {

E7SwitcherClient::E7SwitcherClient(const std::string& account, const std::string& password)
    : host_(IP_HUB), port_(PORT_HUB), timeout_(5), sock_(-1), session_id_(0), user_id_(0) {
    connect();
    login(account, password);
}

E7SwitcherClient::~E7SwitcherClient() {
    if (sock_ != -1) {
        ::close(sock_);
    }
}

void E7SwitcherClient::connect() {
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
    
    // Connect the stream message handler to the socket
    stream_.connect(sock_);
}

void E7SwitcherClient::close() {
    if (sock_ != -1) {
        stream_.disconnect();
        ::close(sock_);
        sock_ = -1;
    }
}

PhoneLoginRecord E7SwitcherClient::login(const std::string& account, const std::string& password) {
    std::vector<uint8_t> login_packet = build_login_payload(account, password);
    stream_.send_message(login_packet);
    ProtocolMessage received_message = stream_.receive_message();
    std::vector<uint8_t> decrypted_payload = decrypt_hex_ecb_pkcs7(received_message.payload, AES_KEY_2_50);
    PhoneLoginRecord login_data = parse_phone_login(decrypted_payload);

    session_id_ = login_data.session_id;
    user_id_ = login_data.user_id;
    communication_secret_key_ = login_data.communication_secret_key;
    Logger::instance().infof("Phone login successful with session ID: %d", login_data.session_id);
    return login_data;
}

const std::vector<Device>& E7SwitcherClient::list_devices() {
    if (!devices_) {
        std::vector<uint8_t> pkt = build_device_list_payload(session_id_, user_id_, communication_secret_key_);
        stream_.send_message(pkt);
        ProtocolMessage received_message = stream_.receive_message();
        std::string payload_str(received_message.payload.begin(), received_message.payload.end());
        size_t start = payload_str.find("{");
        size_t end   = payload_str.rfind("}");
        if (start == std::string::npos || end == std::string::npos || end <= start)
            throw std::runtime_error("No valid JSON found in response");
        std::string json_str = payload_str.substr(start, end - start + 1);
        json_str.erase(std::remove(json_str.begin(), json_str.end(), '\0'), json_str.end());

        std::vector<Device> devices;
        if (!extract_device_list(json_str, devices)) {
            Logger::instance().error("Failed to extract device list from JSON");
            throw std::runtime_error("Failed to extract device list from JSON");
        }
        devices_ = devices;
    }
    return devices_.value();
}

void E7SwitcherClient::control_switch(const std::string& device_name, const std::string& action) {
    Logger::instance().debug("Start of control_device");
    const std::vector<Device>& devices = list_devices();
    Logger::instance().debug("Got device list");
    auto it = std::find_if(devices.begin(), devices.end(), [&](const Device& d) { return d.name == device_name; });
    if (it == devices.end()) throw std::runtime_error("Device not found");

    if (it->type != "0F04") throw std::runtime_error("Device type not supported");

    std::vector<unsigned char> enc_pwd_bytes = base64_decode(it->visit_pwd);
    std::vector<uint8_t> dec_pwd_bytes = decrypt_hex_ecb_pkcs7(
        enc_pwd_bytes, std::string(communication_secret_key_.begin(), communication_secret_key_.end()));
    int on_or_off = (action == "on") ? 1 : 0;

    std::vector<uint8_t> control_packet = build_switch_control_payload(
        session_id_, user_id_, communication_secret_key_, it->did, dec_pwd_bytes, on_or_off);

    Logger::instance().infof("Sending control command to \"%s\"...", device_name.c_str());
    stream_.send_message(control_packet);                 // send
    (void)stream_.receive_message();  // ignore ack, but drain it
    Logger::instance().infof("Control command sent to \"%s\"", device_name.c_str());

    // async status response
    ProtocolMessage response = stream_.receive_message();
    Logger::instance().infof("Received response from \"%s\"", device_name.c_str());
}

void E7SwitcherClient::control_ac(const std::string& device_name, const std::string& action, int mode, int temperature, int fan_speed, int swing, int operationTime) {
    const std::vector<Device>& devices = list_devices();
    auto it = std::find_if(devices.begin(), devices.end(), [&](const Device& d) { return d.name == device_name; });
    if (it == devices.end()) throw std::runtime_error("Device not found");

    if (it->type != "0E01") throw std::runtime_error("Device type not supported");

    std::vector<unsigned char> enc_pwd_bytes = base64_decode(it->visit_pwd);
    std::vector<uint8_t> dec_pwd_bytes = decrypt_hex_ecb_pkcs7(
        enc_pwd_bytes, std::string(communication_secret_key_.begin(), communication_secret_key_.end()));

    const OgeIRDeviceCode& resolver = get_ac_ir_config(device_name);
    std::string control_str = get_ac_control_code(mode, fan_speed, swing, temperature, action == "on", resolver);
    
    std::vector<uint8_t> control_packet = build_ac_control_payload(
        session_id_, user_id_, communication_secret_key_, it->did, dec_pwd_bytes, control_str, operationTime);

    Logger::instance().infof("Sending control command to \"%s\"...", device_name.c_str());
    stream_.send_message(control_packet);                // send
    (void)stream_.receive_message();  // ignore ack, but drain it
    Logger::instance().infof("Control command sent to \"%s\"", device_name.c_str());

    // async status response
    ProtocolMessage response = stream_.receive_message();
    Logger::instance().debugf("Response: %d", response.err_code);
    Logger::instance().infof("Received response from \"%s\"", device_name.c_str());
}

LightStatus E7SwitcherClient::get_light_status(const std::string& device_name) {
    const std::vector<Device>& devices = list_devices();
    auto it = std::find_if(devices.begin(), devices.end(), [&](const Device& d) { return d.name == device_name; });
    if (it == devices.end()) throw std::runtime_error("Device not found");

    if (it->type != "0F04") throw std::runtime_error("Device type not supported");

    std::vector<uint8_t> query_packet = build_device_query_payload(
        session_id_, user_id_, communication_secret_key_, it->did);

    stream_.send_message(query_packet);
    (void)stream_.receive_message(); // drain ack
    ProtocolMessage response = stream_.receive_message();

    return parse_light_status(response.payload);
}

OgeIRDeviceCode E7SwitcherClient::get_ac_ir_config(const std::string &device_name)
{
    // Check if the device code is already in the cache
    auto cache_it = ir_device_code_cache_.find(device_name);
    if (cache_it != ir_device_code_cache_.end()) {
        Logger::instance().infof("Using cached IR device code for \"%s\"", device_name.c_str());
        return cache_it->second;
    }

    // Not in cache, fetch from server
    Logger::instance().infof("Fetching IR device code for \"%s\"", device_name.c_str());
    const std::vector<Device>& devices = list_devices();
    auto it = std::find_if(devices.begin(), devices.end(), [&](const Device& d) { return d.name == device_name; });
    if (it == devices.end()) throw std::runtime_error("Device not found");

    if (it->type != "0E01") throw std::runtime_error("Device type not supported");

    std::string ac_code_id = parse_ac_status(it->work_status_bytes).code_id;

    std::vector<uint8_t> query_packet = build_ac_ir_config_query_payload(
        session_id_, user_id_, communication_secret_key_, it->did, ac_code_id);

    stream_.send_message(query_packet);
    ProtocolMessage response = stream_.receive_message();

    // drop the first 3 bytes of the payload, to use as compressed data
    std::vector<uint8_t> gz_data = response.payload;
    gz_data.erase(gz_data.begin(), gz_data.begin() + 3);
    std::vector<uint8_t> data = decompress_data(gz_data);
    // convert to string
    std::string data_str(data.begin(), data.end());
    OgeIRDeviceCode irCodeResolver = parse_oge_ir_device_code(data_str);

    // Store in cache for future use
    ir_device_code_cache_[device_name] = irCodeResolver;
    Logger::instance().infof("Cached IR device code for \"%s\"", device_name.c_str());

    return irCodeResolver;
}

} // namespace e7_switcher
