#include <Arduino.h>
#include <WiFi.h>
#include "e7-switcher/e7_switcher_client.h"
#include "e7-switcher/logger.h"
#include "e7-switcher/secrets.h"

// WiFi credentials should be defined in secrets.h
// #define E7_SWITCHER_WIFI_SSID "your_wifi_ssid"
// #define E7_SWITCHER_WIFI_PASSWORD "your_wifi_password"
// #define E7_SWITCHER_ACCOUNT "your_account"
// #define E7_SWITCHER_PASSWORD "your_password"
// #define E7_SWITCHER_DEVICE_NAME "your_device_name"

// LED pin configuration
const int LED_PIN = 2;  // Blue LED on many ESP32 devkits

// Function to ensure WiFi connection
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

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* wait for native USB */ }
  
  // Initialize the logger
  e7_switcher::Logger::initialize(e7_switcher::LogLevel::INFO);
  auto& logger = e7_switcher::Logger::instance();
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  logger.info("Switcher E7 ESP32 Example");
  logger.info("========================");
  
  // Connect to WiFi
  logger.infof("Connecting to WiFi SSID: %s", E7_SWITCHER_WIFI_SSID);
  WiFi.mode(WIFI_STA);
  
  if (ensureWifi()) {
    logger.infof("WiFi connected. IP: %s", WiFi.localIP().toString().c_str());
    
    // Create client and get device status
    e7_switcher::E7SwitcherClient client{std::string(E7_SWITCHER_ACCOUNT), std::string(E7_SWITCHER_PASSWORD)};
    
    // List all devices
    logger.info("Listing devices...");
    const auto& devices = client.list_devices();
    logger.infof("Found %d devices", devices.size());
    
    for (const auto& device : devices) {
      logger.infof("Device: %s (Type: %s)", device.name.c_str(), device.type.c_str());
    }
    
    // Get status of the configured device
    logger.infof("Getting status for device: %s", E7_SWITCHER_DEVICE_NAME);
    try {
      e7_switcher::SwitchStatus status = client.get_switch_status(E7_SWITCHER_DEVICE_NAME);
      logger.infof("Device status: %s", status.to_string().c_str());
      
      // Turn on the LED if the device is on
      digitalWrite(LED_PIN, status.switch_state ? HIGH : LOW);
    } 
    catch (const std::exception& e) {
      logger.errorf("Error getting device status: %s", e.what());
    }
  } 
  else {
    logger.error("Failed to connect to WiFi");
  }
}

void loop() {
  // Simple blink pattern to indicate the program is running
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(1900);  // Total 2-second cycle
}
