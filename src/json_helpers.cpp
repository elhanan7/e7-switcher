#include "json_helpers.h"
#include <algorithm>
#include <stdexcept>

#if defined(ARDUINO) || defined(ESP_PLATFORM) || defined(ESP32) || defined(ESP8266)
#define E7_PLATFORM_ESP 1
#include <ArduinoJson.h>
#else
#define E7_PLATFORM_DESKTOP 1
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace e7_switcher {

#ifdef E7_PLATFORM_ESP
// ESP32 implementation using ArduinoJson

bool extract_device_list(const std::string& json_str, std::vector<Device>& devices) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json_str);
    
    if (error) {
        return false;
    }
    
    if (!doc["DevList"].is<JsonArray>()) {
        return false;
    }
    
    JsonArray dev_list = doc["DevList"].as<JsonArray>();
    devices.clear();
    
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
    
    return true;
}

bool extract_is_rest_day(const std::string& json_str, bool& is_rest_day) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json_str);
    
    if (error) {
        return false;
    }
    
    if (!doc["IsRestDay"].is<int>()) {
        return false;
    }
    
    is_rest_day = doc["IsRestDay"].as<int>() == 1;
    return true;
}

#else
// Linux/Mac implementation using nlohmann/json

bool extract_device_list(const std::string& json_str, std::vector<Device>& devices) {
    try {
        json j = json::parse(json_str);
        
        if (!j.contains("DevList")) {
            return false;
        }
        
        devices.clear();
        for (const auto& item : j["DevList"]) {
            Device dev;
            dev.name     = item["DeviceName"].get<std::string>();
            dev.ssid     = item["APSSID"].get<std::string>();
            dev.mac      = item["DMAC"].get<std::string>();
            dev.type     = item["DeviceType"].get<std::string>();
            dev.firmware = item["FirmwareMark"].get<std::string>() + " " + item["FirmwareVersion"].get<std::string>();
            dev.online   = item["OnlineStatus"].get<int>() == 1;
            dev.line_no  = item["LineNo"].get<int>();
            dev.line_type= item["LineType"].get<int>();
            dev.did      = item["DID"].get<int>();
            dev.visit_pwd= item["VisitPwd"].get<std::string>();
            devices.push_back(dev);
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool extract_is_rest_day(const std::string& json_str, bool& is_rest_day) {
    try {
        json j = json::parse(json_str);
        
        if (!j.contains("IsRestDay")) {
            return false;
        }
        
        is_rest_day = j["IsRestDay"].get<int>() == 1;
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

#endif

} // namespace e7_switcher
