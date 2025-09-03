#ifdef E7_PLATFORM_DESKTOP

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>
#include "client.h"
#include "logger.h"
#include "secrets.h"

// Simple command-line interface for desktop platforms
int main(int argc, char* argv[]) {
    // Initialize the logger
    e7_switcher::Logger::initialize();
    auto& logger = e7_switcher::Logger::instance();
    
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
        e7_switcher::E7Client client{std::string(E7_SWITCHER_ACCOUNT), std::string(E7_SWITCHER_PASSWORD)};
        
        if (command == "status") {
            logger.info("Getting device status...");
            e7_switcher::LightStatus status = client.get_light_status(E7_SWITCHER_DEVICE_NAME);
            logger.infof("Device status: %s", status.to_string().c_str());
        } 
        else if (command == "on") {
            logger.infof("Turning ON device: %s", E7_SWITCHER_DEVICE_NAME);
            client.control_device(E7_SWITCHER_DEVICE_NAME, "on");
            logger.info("Command sent successfully");
        } 
        else if (command == "off") {
            logger.infof("Turning OFF device: %s", E7_SWITCHER_DEVICE_NAME);
            client.control_device(E7_SWITCHER_DEVICE_NAME, "off");
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
