#include "e7-switcher/oge_ir_device_code.h"
#include "e7-switcher/json_helpers.h"
#include "e7-switcher/logger.h"

#include <stdexcept>

namespace e7_switcher {
// --- Static token arrays ---------------------------------------------------
const std::array<std::string,5> OgeIRDeviceCode::modes      {"aa","ad","aw","ar","ah"};
const std::array<std::string,4> OgeIRDeviceCode::fans       {"f1","f2","f3","f0"};
const std::array<std::string,4> OgeIRDeviceCode::swings     {"d0","d1","d2","d3"};
const std::array<std::string,4> OgeIRDeviceCode::swing_key1 {"FUN_d0","FUN_d1","FUN_d2","FUN_d3"};

// IRKey parsing is now handled in json_helpers.cpp

// --- Private helpers -------------------------------------------------------
std::optional<std::string> OgeIRDeviceCode::mode_token_for(int mode) {
    if (mode >= 1 && mode <= (int)modes.size()) return modes[mode-1];
    return std::nullopt;
}
std::optional<std::string> OgeIRDeviceCode::fan_token_for(int fan_speed) {
    if (fan_speed >= 1 && fan_speed <= (int)fans.size()) return fans[fan_speed-1];
    return std::nullopt;
}
std::optional<std::string> OgeIRDeviceCode::swing_token_for(int swing) {
    if (swing >= 0 && swing < (int)swings.size()) return swings[swing];
    return std::nullopt;
}

// --- Index helpers ---------------------------------------------------------
void OgeIRDeviceCode::ensure_index() const {
    if (!index.empty()) return;
    for (const auto& k : ir_key_list) index.emplace(k.key, &k);
}
const IRKey* OgeIRDeviceCode::code_by_key(const std::string& key) const {
    ensure_index();
    auto it = index.find(key);
    return it == index.end() ? nullptr : it->second;
}

// --- Logic -----------------------------------------------------------------
const IRKey* OgeIRDeviceCode::ir_code() const {
    try {
        auto m = mode_token_for(mode);
        if (!m) return nullptr;
        auto f = fan_token_for(fan_speed);

        if (f) if (auto* b = code_by_key(*m + std::to_string(temperature) + "_" + *f)) return b;
        if (auto* b = code_by_key(*m + std::to_string(temperature))) return b;
        if (f) if (auto* b = code_by_key(*m + "_" + *f)) return b;
        if (auto* b = code_by_key(*m)) return b;

        for (const auto& k : ir_key_list) if (k.key.find(*m) != std::string::npos) return &k;
        return nullptr;
    } catch (const std::exception& e) {
        Logger::instance().warningf("Exception in OgeIRDeviceCode::ir_code: %s", e.what());
        return nullptr;
    } catch (...) {
        Logger::instance().warning("Unknown exception in OgeIRDeviceCode::ir_code");
        return nullptr;
    }
}

const IRKey* OgeIRDeviceCode::ir_code_with_on_prefix() {
    try {
        if (fan_speed == 0) fan_speed = 1;
        auto m = mode_token_for(mode);
        auto f = fan_token_for(fan_speed);
        if (!m) return nullptr;

        for (const auto& k : {
            "on_" + *m + std::to_string(temperature) + "_" + (f?*f:""),
            "on_" + *m + std::to_string(temperature),
            "on_" + *m + "_" + (f?*f:""),
            "on_" + *m
        }) {
            if (auto* b = code_by_key(k)) return b;
        }
        return nullptr;
    } catch (const std::exception& e) {
        Logger::instance().warningf("Exception in OgeIRDeviceCode::ir_code_with_on_prefix: %s", e.what());
        return nullptr;
    } catch (...) {
        Logger::instance().warning("Unknown exception in OgeIRDeviceCode::ir_code_with_on_prefix");
        return nullptr;
    }
}

const IRKey* OgeIRDeviceCode::swing_ir_code() {
    try {
        if (power == 0) if (auto* b = code_by_key("off")) return b;

        auto m = mode_token_for(mode);
        auto f = fan_token_for(fan_speed);
        auto s = swing_token_for(swing);
        if (!m || !s) return ir_code();

        if (on_off_type == 1 && switch_state == 1) {
            for (const auto& k : {
                f ? "on_" + *m + std::to_string(temperature) + "_" + *f + "_" + *s : "",
                "on_" + *m + std::to_string(temperature) + "_" + *s,
                f ? "on_" + *m + "_" + *f + "_" + *s : "",
                "on_" + *m + "_" + *s,
                f ? "on_" + *m + std::to_string(temperature) + "_" + *f : "",
                "on_" + *m + std::to_string(temperature),
                f ? "on_" + *m + "_" + *f : "",
                "on_" + *m
            }) if (!k.empty()) if (auto* b = code_by_key(k)) return b;
        }

        for (const auto& k : {
            f ? *m + std::to_string(temperature) + "_" + *f + "_" + *s : "",
            *m + std::to_string(temperature) + "_" + *s,
            f ? *m + "_" + *f + "_" + *s : "",
            *m + "_" + *s
        }) if (!k.empty()) if (auto* b = code_by_key(k)) return b;

        return ir_code();
    } catch (const std::exception& e) {
        Logger::instance().warningf("Exception in OgeIRDeviceCode::swing_ir_code: %s", e.what());
        return nullptr;
    } catch (...) {
        Logger::instance().warning("Unknown exception in OgeIRDeviceCode::swing_ir_code");
        return nullptr;
    }
}

const IRKey* OgeIRDeviceCode::swing_special_ir_code() const {
    try {
        if (swing < 0 || swing >= (int)swing_key1.size()) return nullptr;
        return code_by_key(swing_key1[swing]);
    } catch (const std::exception& e) {
        Logger::instance().warningf("Exception in OgeIRDeviceCode::swing_special_ir_code: %s", e.what());
        return nullptr;
    } catch (...) {
        Logger::instance().warning("Unknown exception in OgeIRDeviceCode::swing_special_ir_code");
        return nullptr;
    }
}

const IRKey* OgeIRDeviceCode::switch_ir_code() {
    if (power != 0) {
        if (on_off_type == 1 && switch_state == 1) return ir_code_with_on_prefix();
        return ir_code();
    }
    if (auto* b = code_by_key("off")) return b;
    return ir_code();
}

std::string OgeIRDeviceCode::protocol_para_for(const IRKey* bean) const {
    if (!bean || !bean->para.has_value() || bean->para->empty()) return protocol_para;
    return *bean->para;
}

std::string get_ac_control_code(int mode, int fan_speed, int swing, int temperature, int power, const OgeIRDeviceCode &resolver)
{
    OgeIRDeviceCode mutated_resolver = resolver;
    mutated_resolver.mode = mode;
    mutated_resolver.fan_speed = fan_speed;
    mutated_resolver.swing = swing;
    mutated_resolver.temperature = temperature;
    mutated_resolver.power = power;
    const IRKey* ir_key = mutated_resolver.swing_ir_code();
    if (!ir_key) {
        throw std::runtime_error("Failed to get IR key");
    }
    return resolver.protocol_para + "|" + ir_key->hex_code;
}
// OgeIRDeviceCode parsing is now handled in json_helpers.cpp

// Factory function is now implemented in json_helpers.cpp

}