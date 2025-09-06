#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>
#include "e7-switcher/e7_switcher_client.h"
#include "e7-switcher/logger.h"
#include "e7-switcher/secrets.h"

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
        logger.info("  ./switcher-e7 switch-status            - Get switch status");
        logger.info("  ./switcher-e7 switch-on                - Turn switch on");
        logger.info("  ./switcher-e7 switch-off               - Turn switch off");
        logger.info("  ./switcher-e7 ac-status                - Get AC status");
        logger.info("  ./switcher-e7 ac-on                    - Turn AC on");
        logger.info("  ./switcher-e7 ac-off                   - Turn AC off");
            return 1;
    }
    
    std::string command = argv[1];
    
    try {
        // Create client
        E7SwitcherClient client{std::string(E7_SWITCHER_ACCOUNT), std::string(E7_SWITCHER_PASSWORD)};
        
        if (command == "switch-status") {
            logger.info("Getting switch status...");
            SwitchStatus status = client.get_switch_status(E7_SWITCHER_DEVICE_NAME);
            logger.infof("Switch status: %s", status.to_string().c_str());
        } 
        else if (command == "switch-on") {
            logger.infof("Turning ON switch: %s", E7_SWITCHER_DEVICE_NAME);
            client.control_switch(E7_SWITCHER_DEVICE_NAME, "on");
            logger.info("Command sent successfully");
        } 
        else if (command == "switch-off") {
            logger.infof("Turning OFF switch: %s", E7_SWITCHER_DEVICE_NAME);
            client.control_switch(E7_SWITCHER_DEVICE_NAME, "off");
            logger.info("Command sent successfully");
        } 
        else if (command == "ac-status") {
            logger.info("Getting AC status...");
            ACStatus status = client.get_ac_status(E7_SWITCHER_AC_DEVICE_NAME);
            logger.infof("AC status: %s", status.to_string().c_str());
        }
        else if (command == "ac-on") {
            logger.infof("Turning ON AC: %s", E7_SWITCHER_AC_DEVICE_NAME);
            client.control_ac(E7_SWITCHER_AC_DEVICE_NAME, "on", ACMode::COOL, 20, ACFanSpeed::FAN_MEDIUM, ACSwing::SWING_ON);
            logger.info("Command sent successfully");
        } 
        else if (command == "ac-off") {
            logger.infof("Turning OFF AC: %s", E7_SWITCHER_AC_DEVICE_NAME);
            client.control_ac(E7_SWITCHER_AC_DEVICE_NAME, "off", ACMode::COOL, 20, ACFanSpeed::FAN_MEDIUM, ACSwing::SWING_ON);
            logger.info("Command sent successfully");
        }
        else {
            logger.warning("Unknown command. Use 'switch-status', 'switch-on', 'switch-off', 'ac-status', 'ac-on', or 'ac-off'");
            return 1;
        }
    } 
    catch (const std::exception& e) {
        logger.errorf("Error: %s", e.what());
        return 1;
    }
    
    return 0;
}
