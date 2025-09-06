#include "e7-switcher/hebcal_is_rest_day.h"

#if defined(ARDUINO) || defined(ESP_PLATFORM) || defined(ESP32) || defined(ESP8266)
#define E7_PLATFORM_ESP 1
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>

#include <cstring>          // strlen
#include <optional>

static String build_hebcal_url(int geonameId, const char* dtIsoOpt) {
  // HTTP only (no TLS), per request.
  String url = "http://www.hebcal.com/zmanim?cfg=json&im=1&geonameid=";
  url += String(geonameId);

  if (dtIsoOpt && dtIsoOpt[0]) {
    // Minimal URL encode for '+' in timezone offsets (e.g., +03:00 -> %2B03:00)
    String dt;
    dt.reserve(strlen(dtIsoOpt) + 8);
    for (const char* p = dtIsoOpt; *p; ++p) {
      if (*p == '+') dt += "%2B";
      else           dt += *p;
    }
    url += "&dt=";
    url += dt;
  }
  return url;
}

static std::optional<bool> parse_is_rest_day_from_body(const String& body) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    return std::nullopt;
  }

  if (!doc["status"].is<JsonObject>()) {
    return std::nullopt;
  }

  JsonObject status = doc["status"];
  if (!status["isAssurBemlacha"].is<bool>()) {
    return std::nullopt;
  }

  return status["isAssurBemlacha"].as<bool>();
}

std::optional<bool> hebcal_is_rest_day_by_geoname_id(int geonameId,
                                       const char* dtIsoOpt,
                                       uint32_t timeoutMs) {
  const String url = build_hebcal_url(geonameId, dtIsoOpt);

  WiFiClient net;  // Plain HTTP
  HTTPClient http;

  if (!http.begin(net, url)) {
    return std::nullopt;
  }

  http.setTimeout(timeoutMs);
  const int code = http.GET();
  if (code != 200) {
    http.end();
    return std::nullopt;
  }

  // Read full body (endpoint is small enough for ESP32 RAM in typical cases)
  const String body = http.getString();
  http.end();

  return parse_is_rest_day_from_body(body);
}

std::optional<bool> hebcal_is_rest_day_in_jerusalem(const char* dtIsoOpt,
                                    uint32_t timeoutMs) {
  return hebcal_is_rest_day_by_geoname_id(HEBCAL_JERUSALEM_GEONAMEID,
                                          dtIsoOpt, timeoutMs);
}

#else // Desktop implementation
#define E7_PLATFORM_DESKTOP 1
#include <string>
#include <optional>

// Desktop stub implementations - these functions will return std::nullopt on desktop
std::optional<bool> hebcal_is_rest_day_by_geoname_id(int geonameId,
                                       const char* dtIsoOpt,
                                       uint32_t timeoutMs) {
  // Desktop stub implementation - always returns std::nullopt
  return std::nullopt;
}

std::optional<bool> hebcal_is_rest_day_in_jerusalem(const char* dtIsoOpt,
                                    uint32_t timeoutMs) {
  // Desktop stub implementation - always returns std::nullopt
  return std::nullopt;
}
#endif