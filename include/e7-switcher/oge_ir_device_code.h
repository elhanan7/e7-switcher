#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <array>

namespace e7_switcher {
// Forward declare IRKey
struct IRKey;

struct OgeIRDeviceCode;

// Factory function from JSON string - now implemented in json_helpers.cpp
OgeIRDeviceCode parse_oge_ir_device_code(const std::string& json_str);

// --- IRKey -----------------------------------------------------------------
struct IRKey {
    std::string key;
    std::optional<std::string> para;
    std::string hex_code;
};

// --- OgeIRDeviceCode -------------------------------------------------------
struct OgeIRDeviceCode {
    // Constants
    static const std::array<std::string,5> modes;
    static const std::array<std::string,4> fans;
    static const std::array<std::string,4> swings;
    static const std::array<std::string,4> swing_key1;

    // Metadata
    std::string brand_name;
    std::string edit_time;
    std::string file_type;
    int         ir_device_type = 0;
    std::string ir_set_feature;
    std::string ir_set_id;
    std::string ir_set_state_masks;
    bool        is_reviewed = false;
    int         key_count = 0;
    std::string local_analyse_para;
    int         on_off_type = 0;
    std::string protocol;
    std::string protocol_para;
    int         wind_dirction_type = 0;

    // State
    int fan_speed   = 0;
    int last_action_type = 0;
    int mode        = 0;
    int power       = 0;
    int swing       = 0;
    int switch_state= 0;
    int temperature = 0;

    // Keys
    std::vector<IRKey> ir_key_list;
    mutable std::unordered_map<std::string, const IRKey*> index;

    // Helpers
    void ensure_index() const;
    const IRKey* code_by_key(const std::string& key) const;

    // Logic
    const IRKey* ir_code() const;
    const IRKey* ir_code_with_on_prefix();
    const IRKey* swing_ir_code();
    const IRKey* swing_special_ir_code() const;
    const IRKey* switch_ir_code();
    std::string protocol_para_for(const IRKey* bean) const;

private:
    static std::optional<std::string> mode_token_for(int mode);
    static std::optional<std::string> fan_token_for(int fan_speed);
    static std::optional<std::string> swing_token_for(int swing);
};

std::string get_ac_control_code(int mode, int fan_speed, int swing, int temperature, int power, const OgeIRDeviceCode& resolver);

}