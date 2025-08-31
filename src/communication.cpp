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
using namespace std;

namespace e7_switcher {

E7Connection::E7Connection(const std::string& host, int port, int timeout) 
    : host_(host), port_(port), timeout_(timeout), sock_(-1), session_id_(0), user_id_(0) {}

E7Connection::~E7Connection() {
    if (sock_ != -1) {
        ::close(sock_);
    }
}

void E7Connection::connect() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_);

    if (inet_pton(AF_INET, host_.c_str(), &serv_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address/ Address not supported");
    }

    if (::connect(sock_, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        throw std::runtime_error("Connection Failed");
    }

    struct timeval tv;
    tv.tv_sec = timeout_;
    tv.tv_usec = 0;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

void E7Connection::close() {
    if (sock_ != -1) {
        ::close(sock_);
        sock_ = -1;
    }
}

std::vector<uint8_t> E7Connection::send_and_receive(const std::vector<uint8_t>& data) {
    if (sock_ == -1) {
        throw std::runtime_error("Not connected");
    }

    if (send(sock_, data.data(), data.size(), 0) < 0) {
        throw std::runtime_error("Send failed");
    }

    return receive_internal();
}

std::vector<uint8_t> E7Connection::receive() {
    if (sock_ == -1) {
        throw std::runtime_error("Not connected");
    }

    return receive_internal();
}

std::vector<uint8_t> E7Connection::receive_internal() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::vector<uint8_t> buffer(SOCKET_BUFFER_SIZE);
    int valread = read(sock_, buffer.data(), SOCKET_BUFFER_SIZE);
    if (valread < 0) {
        throw std::runtime_error("Read failed");
    }
    buffer.resize(valread);
    return buffer;
}


PhoneLoginRecord E7Connection::login(const std::string& account, const std::string& password) {
    std::vector<uint8_t> login_packet = build_login_payload(account, password);
    std::vector<uint8_t> response = send_and_receive(login_packet);
    ProtocolPacket parsed_response = parse_protocol_packet(response);

    std::vector<uint8_t> decrypted_payload = decrypt_hex_ecb_pkcs7(parsed_response.payload, AES_KEY_2_50);

    PhoneLoginRecord login_data = parse_phone_login(decrypted_payload);

    session_id_ = login_data.session_id;
    user_id_ = login_data.user_id;
    communication_secret_key_ = login_data.communication_secret_key;

    return login_data;
}

std::vector<Device> E7Connection::get_device_list() {
    std::vector<uint8_t> device_list_packet = build_device_list_payload(session_id_, user_id_, communication_secret_key_);
    std::vector<uint8_t> response = send_and_receive(device_list_packet);
    ProtocolPacket parsed_response = parse_protocol_packet(response);

    std::string payload_str(parsed_response.payload.begin(), parsed_response.payload.end());
    size_t start = payload_str.find("{");
    size_t end = payload_str.rfind("}");
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        throw std::runtime_error("No valid JSON found in response");
    }
    std::string json_str = payload_str.substr(start, end - start + 1);
    json_str.erase(std::remove(json_str.begin(), json_str.end(), '\0'), json_str.end());

    JsonDocument doc;
    deserializeJson(doc, json_str);
    JsonArray dev_list = doc["DevList"].as<JsonArray>();

    std::vector<Device> devices;
    for (JsonObject item : dev_list) {
        Device dev;
        dev.name = item["DeviceName"].as<std::string>();
        dev.ssid = item["APSSID"].as<std::string>();
        dev.mac = item["DMAC"].as<std::string>();
        dev.type = item["DeviceType"].as<std::string>();
        dev.firmware = item["FirmwareMark"].as<std::string>() + " " + item["FirmwareVersion"].as<std::string>();
        dev.online = item["OnlineStatus"].as<int>() == 1;
        dev.line_no = item["LineNo"].as<int>();
        dev.line_type = item["LineType"].as<int>();
        dev.did = item["DID"].as<int>();
        dev.visit_pwd = item["VisitPwd"].as<std::string>();
        devices.push_back(dev);
    }
    return devices;
}

ProtocolPacket E7Connection::control_device(const std::string& device_name, const std::string& action) {
    std::vector<Device> devices = get_device_list();
    auto it = std::find_if(devices.begin(), devices.end(), [&](const Device& d) {
        return d.name == device_name;
    });

    if (it == devices.end()) {
        throw std::runtime_error("Device not found");
    }

    std::vector<unsigned char> enc_pwd_bytes = base64_decode(it->visit_pwd);
    std::vector<uint8_t> dec_pwd_bytes = decrypt_hex_ecb_pkcs7(enc_pwd_bytes, std::string(communication_secret_key_.begin(), communication_secret_key_.end()));
    int on_or_off = (action == "on") ? 1 : 0;

    std::vector<uint8_t> control_packet = build_device_control_payload(session_id_, user_id_, communication_secret_key_, it->did, dec_pwd_bytes, on_or_off);
    send_and_receive(control_packet); // ignore (ack) response
    std::vector<uint8_t> response = receive(); // async resonse
    return parse_protocol_packet(response);
}

LightStatus E7Connection::get_light_status(const std::string& device_name) {
    std::vector<Device> devices = get_device_list();
    auto it = std::find_if(devices.begin(), devices.end(), [&](const Device& d) {
        return d.name == device_name;
    });

    if (it == devices.end()) {
        throw std::runtime_error("Device not found");
    }

    std::vector<uint8_t> query_packet = build_device_query_payload(session_id_, user_id_, communication_secret_key_, it->did);
    send_and_receive(query_packet); // ignore (ack) response
    std::vector<uint8_t> response = receive();
    std::vector<uint8_t> payload = parse_protocol_packet(response).payload;
    return parse_light_status(response);
}
} // namespace e7_switcher