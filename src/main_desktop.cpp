#ifdef E7_PLATFORM_DESKTOP

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>
#include "e7_switcher_client.h"
#include "logger.h"
#include "secrets.h"

using namespace e7_switcher;

// Simple command-line interface for desktop platforms
int main(int argc, char* argv[]) {
    // Initialize the logger
    Logger::initialize();
    auto& logger = Logger::instance();
    
    logger.info("Switcher E7 Desktop Client");
    logger.info("=========================");
    
    // Check if we have the required arguments
    if (argc < 2) {
        logger.info("Usage:");
        logger.info("  ./switcher-e7 status                   - Get device status");
        logger.info("  ./switcher-e7 on                       - Turn device on");
        logger.info("  ./switcher-e7 off                      - Turn device off");
            return 1;
    }
    
    std::string command = argv[1];
    
    try {
        // Create client
        E7SwitcherClient client{std::string(E7_SWITCHER_ACCOUNT), std::string(E7_SWITCHER_PASSWORD)};
        
        if (command == "status") {
            logger.info("Getting device status...");
            SwitchStatus status = client.get_switch_status(E7_SWITCHER_DEVICE_NAME);
            logger.infof("Device status: %s", status.to_string().c_str());
        } 
        else if (command == "ac-status") {
            logger.info("Getting AC status...");
            ACStatus status = client.get_ac_status("Work AC");
            logger.infof("AC status: %s", status.to_string().c_str());
        }
        else if (command == "on") {
            logger.infof("Turning ON device: %s", E7_SWITCHER_DEVICE_NAME);
            client.control_switch(E7_SWITCHER_DEVICE_NAME, "on");
            logger.info("Command sent successfully");
        } 
        else if (command == "off") {
            logger.infof("Turning OFF device: %s", E7_SWITCHER_DEVICE_NAME);
            client.control_switch(E7_SWITCHER_DEVICE_NAME, "off");
            logger.info("Command sent successfully");
        } 
        else if (command == "ac") {
            logger.info("Getting AC IR config...");
            client.control_ac("Work AC", "on", ACMode::COOL, 20, ACFanSpeed::FAN_MEDIUM, ACSwing::SWING_ON);
            logger.info("Command sent successfully");
        }
        else {
            logger.warning("Unknown command. Use 'status', 'on', or 'off'");
            return 1;
        }
    } 
    catch (const std::exception& e) {
        logger.errorf("Error: %s", e.what());
        return 1;
    }
    
    return 0;
}

#endif // E7_PLATFORM_DESKTOP
