#pragma once
/*
 * hebcal_assur.h
 *
 * One-shot check for "Assur Melacha" (Shabbat or Yom Tov) via Hebcal Zmanim API.
 * Uses ArduinoJson instead of nlohmann/json.hpp.
 *
 * Requirements (ESP32 Arduino):
 *   - WiFi connected before calling
 *   - Libraries: HTTPClient, WiFiClient, ArduinoJson
 *
 * Returns:
 *   - std::optional<bool> containing the value if network + parse succeeded
 *   - std::nullopt if failure
 */

#include <stdint.h>
#include <optional>

// GeoNames id for Jerusalem
#define HEBCAL_JERUSALEM_GEONAMEID 281184

// Jerusalem-specific convenience wrapper
std::optional<bool> hebcalIsAssurBemlachaJerusalem(const char* dtIsoOpt = nullptr,
                                    uint32_t timeoutMs = 5000);

// Generic location variant (use different GeoNames IDs if needed)
std::optional<bool> hebcalIsAssurBemlachaByGeonameId(int geonameId,
                                       const char* dtIsoOpt = nullptr,
                                       uint32_t timeoutMs = 5000);