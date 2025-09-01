#ifdef E7_PLATFORM_ESP

#include <Arduino.h>
#include <WiFi.h>
#include <mbedtls/aes.h>

#include "client.h"
#include "communication.h"   // Device struct
#include "hebcal_is_rest_day.h"
#include "secrets.h"
#include "logger.h"

const char *tz             = "IST-2IDT,M3.4.4/26,M10.5.0";

// If your board's LED is different, change these:
const int  LED_PIN         = 2;      // Blue LED on many ESP32 devkits
const bool LED_ACTIVE_HIGH = true;   // Set to false if your LED is active-low
// ===========================

static inline void ledWrite(bool on) {
  digitalWrite(LED_PIN, (on ^ !LED_ACTIVE_HIGH) ? HIGH : LOW);
}

bool ensureWifi(uint32_t timeout_ms = 10000) {
  if (WiFi.status() == WL_CONNECTED) return true;

  WiFi.disconnect();
  WiFi.begin(E7_SWITCHER_WIFI_SSID, E7_SWITCHER_WIFI_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

void do_rest_day_action_if_needed(bool is_rest_day) {
  if (!is_rest_day) return;

  auto& logger = e7_switcher::Logger::instance();
  
  if (!ensureWifi()) {
    logger.warning("[rest_day] WiFi not connected, skipping control command.");
    return;
  }

  // Create a new client each 8-minute cycle and send ON
  e7_switcher::E7Client client{std::string(E7_SWITCHER_ACCOUNT), std::string(E7_SWITCHER_PASSWORD)};
  logger.infof("[rest_day] Sending ON to \"%s\"...", E7_SWITCHER_DEVICE_NAME);
  client.control_device(E7_SWITCHER_DEVICE_NAME, "on");
  logger.info("[rest_day] Control command sent.");
}


void print_device_status() {
  auto& logger = e7_switcher::Logger::instance();
  e7_switcher::E7Client client{std::string(E7_SWITCHER_ACCOUNT), std::string(E7_SWITCHER_PASSWORD)};
  e7_switcher::LightStatus status = client.get_light_status(E7_SWITCHER_DEVICE_NAME);

  logger.infof("Device status: %s", status.to_string().c_str());
}

const int E7_AUTOSHUTDOWN_TIME_SECONDS = 40 * 60;

// Variables to track light status and timing
e7_switcher::LightStatus last_known_status;
unsigned long next_status_check_ms = 0;
bool status_initialized = false;

// Returns milliseconds until next check is needed
unsigned long handle_auto_shutdown() {
    auto& logger = e7_switcher::Logger::instance();
    e7_switcher::E7Client client{std::string(E7_SWITCHER_ACCOUNT), std::string(E7_SWITCHER_PASSWORD)};
    e7_switcher::LightStatus status = client.get_light_status(E7_SWITCHER_DEVICE_NAME);
    
    // Store the latest status
    last_known_status = status;
    status_initialized = true;
    
    logger.infof("Device status: %s", status.to_string().c_str());

    if (status.switch_state == 0) {
      // Light is off, check again in a bit less than auto-shutdown time for safety (85% of the time)
      unsigned long check_interval_ms = (unsigned long)(E7_AUTOSHUTDOWN_TIME_SECONDS * 0.85) * 1000UL;
      logger.infof("[auto shutdown] Light is off, will check again in %lu seconds", check_interval_ms / 1000UL);
      return check_interval_ms;
    }

    if (status.open_time > E7_AUTOSHUTDOWN_TIME_SECONDS) {
      logger.infof("[auto shutdown] Sending OFF to \"%s\"...", E7_SWITCHER_DEVICE_NAME);
      client.control_device(E7_SWITCHER_DEVICE_NAME, "off");
      logger.info("[auto shutdown] Control command sent.");
      // Check again in 30 seconds to confirm it turned off
      return 30UL * 1000UL;
    } else {
      // Calculate time remaining until shutdown needed
      int seconds_remaining = E7_AUTOSHUTDOWN_TIME_SECONDS - status.open_time;
      // Add a small buffer (10 seconds) to ensure we don't check too late
      unsigned long next_check_ms = (seconds_remaining > 10) ? 
                                   (seconds_remaining - 10) * 1000UL : 
                                   10UL * 1000UL; // Minimum 10 seconds
      
      logger.infof("[auto shutdown] Light on for %d seconds, will check again in %lu seconds", 
                   status.open_time, next_check_ms / 1000UL);
      
      return next_check_ms;
    }
}

// Timing
const unsigned long REST_DAY_INTERVAL_MS = 10UL * 60UL * 1000UL;  // every 10 minutes
const unsigned long BLINK_INTERVAL_MS = 1000UL;               // 1 Hz blink when not rest_day
const unsigned long AUTO_SHUTDOWN_INTERVAL_MS = 2UL * 60UL * 1000UL; // every 2 minutes

unsigned long last_rest_day_check_ms = 0;
unsigned long last_blink_ms = 0;
unsigned long last_auto_shutdown_ms = 0;

bool is_rest_day = false;
bool blink_state = false;

// Fail compilation if secrets.h isn't filled in
static_assert(std::string_view{E7_SWITCHER_ACCOUNT} != "REPLACE_ME", "E7_SWITCHER_ACCOUNT is REPLACE_ME");
static_assert(std::string_view{E7_SWITCHER_WIFI_SSID} != "REPLACE_ME", "E7_SWITCHER_PASSWORD is REPLACE_ME");

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait for native USB */ }
  
  // Initialize the logger
  e7_switcher::Logger::initialize();

  pinMode(LED_PIN, OUTPUT);
  ledWrite(false);

  auto& logger = e7_switcher::Logger::instance();
  logger.info("Booting…");
  logger.infof("Connecting to WiFi SSID: %s", E7_SWITCHER_WIFI_SSID);

  WiFi.mode(WIFI_STA);
  ensureWifi();
  if (WiFi.status() == WL_CONNECTED) {
    logger.infof("WiFi connected. IP: %s", WiFi.localIP().toString().c_str());
  } else {
    logger.warning("WiFi not connected (will keep trying periodically).");
  }

  configTzTime(tz, "pool.ntp.org");

  // Initial time fetch (non-fatal if it fails; we'll retry on each cycle)
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 5000)) {
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    logger.infof("Current time: %s", buf);
  } else {
    logger.warning("Failed to obtain time (will retry later).");
  }

  // Initial rest_day check
  std::optional<bool> is_rest_day_opt = hebcal_is_rest_day_in_jerusalem();
  if (!is_rest_day_opt.has_value()) {
    logger.warning("Failed to check rest day status (will retry later).");
  } else {
    logger.infof("Initial is_rest_day: %s", is_rest_day_opt.value() ? "true" : "false");
  }

  is_rest_day = is_rest_day_opt.value_or(false);
  

  // Apply initial LED state
  if (is_rest_day) {
    ledWrite(true);     // solid ON
  } else {
    ledWrite(false);    // start blink from OFF
  }

  // If rest_day at boot, perform action now
  do_rest_day_action_if_needed(true);
  print_device_status();

  last_auto_shutdown_ms = millis();
  last_blink_ms = last_auto_shutdown_ms;
  last_rest_day_check_ms = last_auto_shutdown_ms;
  next_status_check_ms = last_auto_shutdown_ms; // Initialize next check time
}

void loop() {
  unsigned long now = millis();

  if (now - last_rest_day_check_ms >= REST_DAY_INTERVAL_MS) {
    last_rest_day_check_ms = now;

    // Keep Wi-Fi connected (non-blocking-ish with small timeout inside ensureWifi)
    auto& logger = e7_switcher::Logger::instance();
    bool wifiOK = ensureWifi();
    if (wifiOK) {
      logger.infof("[cycle] WiFi OK. IP: %s", WiFi.localIP().toString().c_str());
    } else {
      logger.warning("[cycle] WiFi not connected.");
      delay(1000);
      return;
    }

    // Refresh time (helps if NTP hadn't synced yet)
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 3000)) {
      char buf[32];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
      logger.infof("[cycle] Time: %s", buf);
    } else {
      logger.warning("[cycle] Failed to obtain time.");
    }

    // Check is_rest_day status
    std::optional<bool> new_rest_day_status_opt = hebcal_is_rest_day_in_jerusalem();
    bool new_is_rest_day = new_rest_day_status_opt.value_or(false); // Assign value if present, else false
    logger.infof("[cycle] is_rest_day: %s", new_is_rest_day ? "true" : "false");

    // If status changed, update LED behavior immediately
    if (new_is_rest_day != is_rest_day) {
      is_rest_day = new_is_rest_day;
      if (is_rest_day) {
        ledWrite(true);           // solid ON
      } else {
        blink_state = false;       // reset blink phase
        ledWrite(false);
        last_blink_ms = now;
      }
    } else {
      // Keep current behavior (solid / blinking)
      is_rest_day = new_is_rest_day;
    }

    // Do the control action if rest_day
    do_rest_day_action_if_needed(is_rest_day);
  }

  // 2) LED behavior: solid when rest_day; 1 Hz blink when not rest_day
  if (is_rest_day) {
    ledWrite(true);  // make sure it stays solid on
  } else {
    if (now - last_blink_ms >= BLINK_INTERVAL_MS) {
      last_blink_ms = now;
      blink_state = !blink_state;
      ledWrite(blink_state);
    }
  }

  // auto-shutdown
  if (!is_rest_day)
  {
    // Check if it's time to query the light status
    if (!status_initialized || now >= next_status_check_ms)
    {
      last_auto_shutdown_ms = now;
      unsigned long next_check_interval = handle_auto_shutdown();
      next_status_check_ms = now + next_check_interval;
      
      auto& logger = e7_switcher::Logger::instance();
      logger.infof("[auto shutdown] Next check in %lu seconds", next_check_interval / 1000UL);
    }
  }

  // Small idle delay to keep loop snappy without busy-spinning
  delay(5);
}

#endif // E7_PLATFORM_ESP
