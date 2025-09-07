#include "e7-switcher/e7_switcher_client.h"
#include "e7-switcher/constants.h"
#include "e7-switcher/messages.h"
#include "e7-switcher/parser.h"
#include "e7-switcher/crypto.h"
#include "e7-switcher/base64_decode.h"
#include "e7-switcher/logger.h"
#include "e7-switcher/compression.h"
#include "e7-switcher/json_helpers.h"

#include <algorithm>
#include <stdexcept>

namespace e7_switcher {

E7SwitcherClient::E7SwitcherClient(const std::string& account, const std::string& password)
    : session_id_(0), user_id_(0) {
    stream_.connect_to_server(IP_HUB, PORT_HUB, 5);
    login(account, password);
}

E7SwitcherClient::~E7SwitcherClient() {
}


PhoneLoginRecord E7SwitcherClient::login(const std::string& account, const std::string& password) {
    ProtocolMessage login_message = build_login_message(account, password);
    stream_.send_message(login_message);
    ProtocolMessage received_message = stream_.receive_message();

    if (received_message.err_code != 0) {
        throw std::runtime_error("Login failed with error code: " + std::to_string(received_message.err_code));
    }
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
        ProtocolMessage message = build_device_list_message(session_id_, user_id_, communication_secret_key_);
        stream_.send_message(message);
        ProtocolMessage received_message = stream_.receive_message();
        if (received_message.err_code != 0) {
            throw std::runtime_error("Failed to list devices with error code: " + std::to_string(received_message.err_code));
        }
        std::string json_str(received_message.payload.begin(), received_message.payload.end());

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
    Logger::instance().debug("Got device list");
    const Device& device = find_device_by_name_and_type(device_name, DEVICE_TYPE_SWITCH);

    std::vector<unsigned char> enc_pwd_bytes = base64_decode(device.visit_pwd);
    std::vector<uint8_t> dec_pwd_bytes = decrypt_hex_ecb_pkcs7(
        enc_pwd_bytes, std::string(communication_secret_key_.begin(), communication_secret_key_.end()));
    int on_or_off = (action == "on") ? 1 : 0;

    ProtocolMessage control_message = build_switch_control_message(
        session_id_, user_id_, communication_secret_key_, device.did, dec_pwd_bytes, on_or_off);

    Logger::instance().infof("Sending control command to \"%s\"...", device_name.c_str());
    stream_.send_message(control_message);                 // send
    (void)stream_.receive_message();  // ignore ack, but drain it
    Logger::instance().infof("Control command sent to \"%s\"", device_name.c_str());

    // async status response
    ProtocolMessage response = stream_.receive_message();
    Logger::instance().infof("Received response from \"%s\"", device_name.c_str());
}

void E7SwitcherClient::control_ac(const std::string& device_name, const std::string& action, ACMode mode, int temperature, ACFanSpeed fan_speed, ACSwing swing, int operation_time) {
    const Device& device = find_device_by_name_and_type(device_name, DEVICE_TYPE_AC);

    std::vector<unsigned char> enc_pwd_bytes = base64_decode(device.visit_pwd);
    std::vector<uint8_t> dec_pwd_bytes = decrypt_hex_ecb_pkcs7(
        enc_pwd_bytes, std::string(communication_secret_key_.begin(), communication_secret_key_.end()));

    const OgeIRDeviceCode& resolver = get_ac_ir_config(device_name);
    int power_value = (action == "on") ? static_cast<int>(ACPower::POWER_ON) : static_cast<int>(ACPower::POWER_OFF);
    std::string control_str = get_ac_control_code(
        static_cast<int>(mode), 
        static_cast<int>(fan_speed), 
        static_cast<int>(swing), 
        temperature, 
        power_value, 
        resolver);
    
    ProtocolMessage control_message = build_ac_control_message(
        session_id_, user_id_, communication_secret_key_, device.did, dec_pwd_bytes, control_str, operation_time);

    Logger::instance().infof("Sending control command to \"%s\"...", device_name.c_str());
    stream_.send_message(control_message);                // send
    (void)stream_.receive_message();  // ignore ack, but drain it
    Logger::instance().infof("Control command sent to \"%s\"", device_name.c_str());

    // async status response
    ProtocolMessage response = stream_.receive_message();
    Logger::instance().debugf("Response: %d", response.err_code);
    Logger::instance().infof("Received response from \"%s\"", device_name.c_str());
}

SwitchStatus E7SwitcherClient::get_switch_status(const std::string& device_name) {
    const Device& device = find_device_by_name_and_type(device_name, DEVICE_TYPE_SWITCH);

    ProtocolMessage query_message = build_device_query_message(
        session_id_, user_id_, communication_secret_key_, device.did);

    stream_.send_message(query_message);
    (void)stream_.receive_message(); // drain ack
    ProtocolMessage response = stream_.receive_message();

    return parse_switch_status(response.payload);
}

ACStatus E7SwitcherClient::get_ac_status(const std::string& device_name) {
    const Device& device = find_device_by_name_and_type(device_name, DEVICE_TYPE_AC);

    ProtocolMessage query_message = build_device_query_message(
        session_id_, user_id_, communication_secret_key_, device.did);

    stream_.send_message(query_message);
    (void)stream_.receive_message(); // drain ack
    ProtocolMessage response = stream_.receive_message();

    return parse_ac_status_from_query_payload(response.payload);
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
    const Device& device = find_device_by_name_and_type(device_name, DEVICE_TYPE_AC);

    std::string ac_code_id = parse_ac_status_from_work_status_bytes(device.work_status_bytes).code_id;

    ProtocolMessage query_message = build_ac_ir_config_query_message(
        session_id_, user_id_, communication_secret_key_, device.did, ac_code_id);

    stream_.send_message(query_message);
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

// Helper method implementation
const Device& E7SwitcherClient::find_device_by_name_and_type(const std::string& device_name, const std::string& expected_type) {
    const std::vector<Device>& devices = list_devices();
    auto it = std::find_if(devices.begin(), devices.end(), [&](const Device& d) { return d.name == device_name; });
    if (it == devices.end()) throw std::runtime_error("Device not found");

    if (it->type != expected_type) throw std::runtime_error("Device type not supported");
    
    return *it;
}

} // namespace e7_switcher
