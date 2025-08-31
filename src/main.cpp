#include <Arduino.h>
#include <WiFi.h>
#include <mbedtls/aes.h>

#include "client.h"
#include "communication.h"   // Device struct
#include "hebcal_assur.h"
#include "secrets.h"

const char *tz             = "IST-2IDT,M3.4.4/26,M10.5.0";

// If your board’s LED is different, change these:
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

void doAssurActionIfNeeded(bool isAssur) {
  if (!isAssur) return;

  if (!ensureWifi()) {
    Serial.println("[assur] WiFi not connected, skipping control command.");
    return;
  }

  // Create a new client each 8-minute cycle and send ON
  e7_switcher::E7Client client{std::string(E7_SWITCHER_ACCOUNT), std::string(E7_SWITCHER_PASSWORD)};
  Serial.printf("[assur] Sending ON to \"%s\"...\n", E7_SWITCHER_DEVICE_NAME);
  client.control_device(E7_SWITCHER_DEVICE_NAME, "on");
  Serial.println("[assur] Control command sent.");
}

// Timing
const unsigned long CHECK_INTERVAL_MS = 8UL * 60UL * 1000UL;  // every 8 minutes
const unsigned long BLINK_INTERVAL_MS = 1000UL;               // 1 Hz blink when not assur

unsigned long lastCheckMs = 0;
unsigned long lastBlinkMs = 0;

bool isAssur = false;
bool blinkState = false;

// Fail compilation if secrets.h isn’t filled in
static_assert(std::string_view{E7_SWITCHER_ACCOUNT} != "REPLACE_ME", "E7_SWITCHER_ACCOUNT is REPLACE_ME");
static_assert(std::string_view{E7_SWITCHER_WIFI_SSID} != "REPLACE_ME", "E7_SWITCHER_PASSWORD is REPLACE_ME");

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait for native USB */ }

  pinMode(LED_PIN, OUTPUT);
  ledWrite(false);

  Serial.println("Booting…");
  Serial.printf("Connecting to WiFi SSID: %s\n", E7_SWITCHER_WIFI_SSID);

  WiFi.mode(WIFI_STA);
  ensureWifi();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi not connected (will keep trying periodically).");
  }

  configTzTime(tz, "pool.ntp.org");

  // Initial time fetch (non-fatal if it fails; we’ll retry on each cycle)
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 5000)) {
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf("Current time: %s\n", buf);
  } else {
    Serial.println("Failed to obtain time (will retry later).");
  }

  // Initial assur check
  std::optional<bool> isAssurOpt = hebcalIsAssurBemlachaJerusalem();
  if (!isAssurOpt.has_value()) {
    Serial.println("Failed to check assur (will retry later).");
  } else {
    Serial.printf("Initial assurBemelacha: %s\n", isAssurOpt.value() ? "true" : "false");
  }
  

  // Apply initial LED state
  if (isAssur) {
    ledWrite(true);     // solid ON
  } else {
    ledWrite(false);    // start blink from OFF
  }

  // If assur at boot, perform action now
  // doAssurActionIfNeeded(isAssur);
  doAssurActionIfNeeded(true);

  lastCheckMs = millis();
  lastBlinkMs = millis();
}

void loop() {
  unsigned long now = millis();

  // 1) Every 8 minutes: ensure WiFi, refresh time, check assur, and act if needed
  if (now - lastCheckMs >= CHECK_INTERVAL_MS) {
    lastCheckMs = now;

    // Keep Wi-Fi connected (non-blocking-ish with small timeout inside ensureWifi)
    bool wifiOK = ensureWifi();
    if (wifiOK) {
      Serial.printf("[cycle] WiFi OK. IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("[cycle] WiFi not connected.");
    }

    // Refresh time (helps if NTP hadn’t synced yet)
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 3000)) {
      char buf[32];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
      Serial.printf("[cycle] Time: %s\n", buf);
    } else {
      Serial.println("[cycle] Failed to obtain time.");
    }

    // Check assur status
    std::optional<bool> newAssurOpt = hebcalIsAssurBemlachaJerusalem();
    bool newAssur = newAssurOpt.value_or(false); // Assign value if present, else false
    Serial.printf("[cycle] assurBemelacha: %s\n", newAssur ? "true" : "false");

    // If status changed, update LED behavior immediately
    if (newAssur != isAssur) {
      isAssur = newAssur;
      if (isAssur) {
        ledWrite(true);           // solid ON
      } else {
        blinkState = false;       // reset blink phase
        ledWrite(false);
        lastBlinkMs = now;
      }
    } else {
      // Keep current behavior (solid / blinking)
      isAssur = newAssur;
    }

    // Do the control action if assur
    doAssurActionIfNeeded(isAssur);
  }

  // 2) LED behavior: solid when assur; 1 Hz blink when not assur
  if (isAssur) {
    ledWrite(true);  // make sure it stays solid on
  } else {
    if (now - lastBlinkMs >= BLINK_INTERVAL_MS) {
      lastBlinkMs = now;
      blinkState = !blinkState;
      ledWrite(blinkState);
    }
  }

  // Small idle delay to keep loop snappy without busy-spinning
  delay(5);
}
