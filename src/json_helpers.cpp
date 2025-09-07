#include "e7-switcher/json_helpers.h"
#include "e7-switcher/base64_decode.h"
#include <algorithm>
#include <stdexcept>

#if defined(ARDUINO) || defined(ESP_PLATFORM) || defined(ESP32) || defined(ESP8266)
#define E7_PLATFORM_ESP 1
#include <ArduinoJson.h>
#else
#define E7_PLATFORM_DESKTOP 1
#include <nlohmann/json.hpp>
#include "e7-switcher/oge_ir_device_code.h"
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
        auto work_status_b64 = item["WorkStatus"].as<std::string>();
        dev.work_status_bytes = base64_decode(work_status_b64);
        devices.push_back(dev);
    }
    
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
            auto work_status_b64 = item["WorkStatus"].get<std::string>();
            dev.work_status_bytes = base64_decode(work_status_b64);
            devices.push_back(dev);
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

#endif

// --- OgeIRDeviceCode JSON parsing functions ---------------------------------

#ifdef E7_PLATFORM_ESP
// ESP32 implementation using ArduinoJson

// Internal helper function - not exposed in header
static bool parse_ir_key(const void* json_obj, IRKey& k) {
    const JsonObject& j = *static_cast<const JsonObject*>(json_obj);
    
    if (j["Key"].is<const char*>()) k.key = j["Key"].as<std::string>();
    else k.key = "";
    
    if (j["Para"].is<const char*>()) k.para = j["Para"].as<std::string>();
    else k.para = std::nullopt;

    if (j["HexCode"].is<const char*>()) k.hex_code = j["HexCode"].as<std::string>();
    else k.hex_code = "";
    
    return true;
}

OgeIRDeviceCode parse_oge_ir_device_code(const std::string& json_str) {
    OgeIRDeviceCode d;
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json_str);
    
    if (error) {
        return d; // Return empty object on error
    }
    
    auto get_or_empty = [&](const char* k) -> std::string {
        return doc[k].is<const char*>() ? doc[k].as<std::string>() : "";
    };
    
    auto get_or_int = [&](const char* k, int def) -> int {
        return doc[k].is<int>() ? doc[k].as<int>() : def;
    };
    
    auto get_or_bool = [&](const char* k, bool def) -> bool {
        return doc[k].is<bool>() ? doc[k].as<bool>() : def;
    };
    
    d.brand_name         = get_or_empty("BrandName");
    d.edit_time          = get_or_empty("EditTime");
    d.file_type          = get_or_empty("FileType");
    d.ir_device_type     = get_or_int("IRDeviceType", 0);
    d.ir_set_feature     = get_or_empty("IRSetFeature");
    d.ir_set_id          = get_or_empty("IRSetID");
    d.ir_set_state_masks = get_or_empty("IRSetStateMasks");
    d.is_reviewed        = get_or_bool("IsReviewed", false);
    d.key_count          = get_or_int("KeyCount", 0);
    d.local_analyse_para = get_or_empty("LocalAnalysePara");
    d.on_off_type        = get_or_int("OnOffType", 0);
    d.protocol           = get_or_empty("Protocol");
    d.protocol_para      = get_or_empty("ProtocolPara");
    d.wind_dirction_type = get_or_int("WindDirctionType", 0);
    
    d.fan_speed        = doc["fanSpeed"].is<int>() ? doc["fanSpeed"].as<int>() : 
                         (doc["fan_speed"].is<int>() ? doc["fan_speed"].as<int>() : 0);
    d.last_action_type = doc["lastActionType"].is<int>() ? doc["lastActionType"].as<int>() : 
                         (doc["last_action_type"].is<int>() ? doc["last_action_type"].as<int>() : 0);
    d.mode             = doc["mode"].is<int>() ? doc["mode"].as<int>() : 0;
    d.power            = doc["power"].is<int>() ? doc["power"].as<int>() : 0;
    d.swing            = doc["swing"].is<int>() ? doc["swing"].as<int>() : 0;
    d.switch_state     = doc["switchState"].is<int>() ? doc["switchState"].as<int>() : 
                         (doc["switch_state"].is<int>() ? doc["switch_state"].as<int>() : 0);
    d.temperature      = doc["temperature"].is<int>() ? doc["temperature"].as<int>() : 0;
    
    d.ir_key_list.clear();
    if (doc["IRKeyList"].is<JsonArray>()) {
        JsonArray key_list = doc["IRKeyList"].as<JsonArray>();
        for (JsonObject key_obj : key_list) {
            IRKey key;
            parse_ir_key(&key_obj, key);
            d.ir_key_list.push_back(key);
        }
    }
    
    d.index.clear();
    return d;
}

#else
    // Linux/Mac implementation using nlohmann/json

    // Internal helper function - not exposed in header
    static bool
    parse_ir_key(const void *json_obj, IRKey &k)
{
    const json& j = *static_cast<const json*>(json_obj);
    
    if      (j.contains("Key"))   k.key = j.at("Key").get<std::string>();
    else                          k.key = "";
    
    if      (j.contains("Para"))  k.para = j.at("Para").get<std::string>();
    else                          k.para = std::nullopt;
    
    if      (j.contains("HexCode")) k.hex_code = j.at("HexCode").get<std::string>();
    else                              k.hex_code = "";
    
    return true;
}

OgeIRDeviceCode parse_oge_ir_device_code(const std::string& json_str) {
    OgeIRDeviceCode d;
    try {
        json j = json::parse(json_str);
        
        auto get_or_empty = [&](const char* k) -> std::string {
            return j.contains(k) && !j.at(k).is_null() ? j.at(k).get<std::string>() : "";
        };
        
        auto get_or_int = [&](const char* k, int def) -> int {
            return j.contains(k) && j.at(k).is_number() ? j.at(k).get<int>() : def;
        };
        
        auto get_or_bool = [&](const char* k, bool def) -> bool {
            return j.contains(k) && j.at(k).is_boolean() ? j.at(k).get<bool>() : def;
        };
        
        d.brand_name         = get_or_empty("BrandName");
        d.edit_time          = get_or_empty("EditTime");
        d.file_type          = get_or_empty("FileType");
        d.ir_device_type     = get_or_int("IRDeviceType", 0);
        d.ir_set_feature     = get_or_empty("IRSetFeature");
        d.ir_set_id          = get_or_empty("IRSetID");
        d.ir_set_state_masks = get_or_empty("IRSetStateMasks");
        d.is_reviewed        = get_or_bool("IsReviewed", false);
        d.key_count          = get_or_int("KeyCount", 0);
        d.local_analyse_para = get_or_empty("LocalAnalysePara");
        d.on_off_type        = get_or_int("OnOffType", 0);
        d.protocol           = get_or_empty("Protocol");
        d.protocol_para      = get_or_empty("ProtocolPara");
        d.wind_dirction_type = get_or_int("WindDirctionType", 0);
        
        d.fan_speed       = j.value("fanSpeed", j.value("fan_speed", 0));
        d.last_action_type= j.value("lastActionType", j.value("last_action_type", 0));
        d.mode            = j.value("mode", 0);
        d.power           = j.value("power", 0);
        d.swing           = j.value("swing", 0);
        d.switch_state    = j.value("switchState", j.value("switch_state", 0));
        d.temperature     = j.value("temperature", 0);
        
        d.ir_key_list.clear();
        if (j.contains("IRKeyList") && j.at("IRKeyList").is_array()) {
            for (const auto& key_obj : j.at("IRKeyList")) {
                IRKey key;
                parse_ir_key(&key_obj, key);
                d.ir_key_list.push_back(key);
            }
        }
        
        d.index.clear();
    } catch (const std::exception& e) {
        // Return empty object on error
    }
    
    return d;
}

#endif

} // namespace e7_switcher
