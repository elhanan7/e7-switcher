#pragma once
/*
 * hebcal_is_rest_day.h
 *
 * One-shot check for rest day (Shabbat or Yom Tov) via Hebcal Zmanim API.
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
std::optional<bool> hebcal_is_rest_day_in_jerusalem(const char* dtIsoOpt = nullptr,
                                    uint32_t timeoutMs = 5000);

// Generic location variant (use different GeoNames IDs if needed)
std::optional<bool> hebcal_is_rest_day_by_geoname_id(int geonameId,
                                       const char* dtIsoOpt = nullptr,
                                       uint32_t timeoutMs = 5000);