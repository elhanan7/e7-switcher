#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>
#include <map>
#include <vector>
#include <sstream>
#include "e7-switcher/e7_switcher_client.h"
#include "e7-switcher/logger.h"
#include "e7-switcher/secrets.h"

using namespace e7_switcher;

// Helper function to parse command line arguments into a map
std::map<std::string, std::string> parse_args(int argc, char* argv[], int start_idx) {
    std::map<std::string, std::string> args;
    for (int i = start_idx; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.substr(0, 2) == "--") {
            std::string key = arg.substr(2);
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                args[key] = argv[i + 1];
                i++;
            } else {
                args[key] = "true";
            }
        }
    }
    return args;
}

// Helper function to convert string to enum
ACMode string_to_ac_mode(const std::string& mode) {
    if (mode == "cool") return ACMode::COOL;
    if (mode == "heat") return ACMode::HEAT;
    if (mode == "fan") return ACMode::FAN;
    if (mode == "dry") return ACMode::DRY;
    if (mode == "auto") return ACMode::AUTO;
    throw std::invalid_argument("Invalid AC mode: " + mode + ". Valid options are: cool, heat, fan, dry, auto");
}

// Helper function to convert string to enum
ACFanSpeed string_to_fan_speed(const std::string& speed) {
    if (speed == "low") return ACFanSpeed::FAN_LOW;
    if (speed == "medium") return ACFanSpeed::FAN_MEDIUM;
    if (speed == "high") return ACFanSpeed::FAN_HIGH;
    if (speed == "auto") return ACFanSpeed::FAN_AUTO;
    throw std::invalid_argument("Invalid fan speed: " + speed + ". Valid options are: low, medium, high, auto");
}

// Helper function to convert string to enum
ACSwing string_to_swing(const std::string& swing) {
    if (swing == "on") return ACSwing::SWING_ON;
    if (swing == "off") return ACSwing::SWING_OFF;
    throw std::invalid_argument("Invalid swing option: " + swing + ". Valid options are: on, off");
}

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
        logger.info("  ./switcher-e7 switch-status --device <device_name>                - Get switch status");
        logger.info("  ./switcher-e7 switch-on --device <device_name> [--time <seconds>] - Turn switch on (optional auto-off timer in seconds)");
        logger.info("  ./switcher-e7 switch-off --device <device_name> [--time <seconds>] - Turn switch off (optional auto-off timer in seconds)");
        logger.info("  ./switcher-e7 ac-status --device <device_name>                    - Get AC status");
        logger.info("  ./switcher-e7 ac-on --device <device_name> [--mode <mode>] [--temp <temperature>] [--fan <speed>] [--swing <on|off>]  - Turn AC on");
        logger.info("  ./switcher-e7 ac-off --device <device_name>                       - Turn AC off");
        logger.info("  ./switcher-e7 boiler-status --device <device_name>                - Get boiler status");
        logger.info("  ./switcher-e7 boiler-on --device <device_name> [--time <seconds>] - Turn boiler on (optional auto-off timer in seconds)");
        logger.info("  ./switcher-e7 boiler-off --device <device_name>                   - Turn boiler off");
        logger.info("");
        logger.info("Options:");
        logger.info("  --device    Device name (required)");
        logger.info("  --mode      AC mode: cool, heat, fan, dry, auto (default: cool)");
        logger.info("  --temp      Temperature: 16-30 (default: 20)");
        logger.info("  --fan       Fan speed: low, medium, high, auto (default: medium)");
        logger.info("  --swing     Swing: on, off (default: on)");
        logger.info("  --time      Auto-off timer in seconds (default: 0)");
        return 1;
    }
    
    std::string command = argv[1];
    
    // Parse command line arguments
    auto args = parse_args(argc, argv, 2);
    
    // Check if device name is provided
    if (!args.count("device")) {
        logger.error("Error: --device parameter is required");
        logger.info("Run without arguments to see usage instructions");
        return 1;
    }
    
    try {
        // Create client
        E7SwitcherClient client{std::string(E7_SWITCHER_ACCOUNT), std::string(E7_SWITCHER_PASSWORD)};
        
        // Get device name from command line
        std::string device_name = args["device"];
        
        if (command == "switch-status") {
            logger.infof("Getting switch status for device: %s", device_name.c_str());
            SwitchStatus status = client.get_switch_status(device_name);
            logger.infof("Switch status: %s", status.to_string().c_str());
        } 
        else if (command == "switch-on") {
            logger.infof("Turning ON switch: %s", device_name.c_str());
            int op_time = 0;
            if (args.count("time")) {
                try {
                    op_time = std::stoi(args["time"]);
                    if (op_time < 0) op_time = 0;
                } catch (...) {
                    logger.warning("Invalid --time value, defaulting to 0");
                    op_time = 0;
                }
            }
            client.control_switch(device_name, "on", op_time);
            logger.info("Command sent successfully");
        } 
        else if (command == "switch-off") {
            logger.infof("Turning OFF switch: %s", device_name.c_str());
            int op_time = 0;
            if (args.count("time")) {
                try {
                    op_time = std::stoi(args["time"]);
                    if (op_time < 0) op_time = 0;
                } catch (...) {
                    logger.warning("Invalid --time value, defaulting to 0");
                    op_time = 0;
                }
            }
            client.control_switch(device_name, "off", op_time);
            logger.info("Command sent successfully");
        } 
        else if (command == "ac-status") {
            logger.infof("Getting AC status for device: %s", device_name.c_str());
            ACStatus status = client.get_ac_status(device_name);
            logger.infof("AC status: %s", status.to_string().c_str());
        }
        else if (command == "ac-on") {
            // Parse AC parameters with proper error handling
            ACMode mode = ACMode::COOL;
            int temp = 20;
            ACFanSpeed fan_speed = ACFanSpeed::FAN_MEDIUM;
            ACSwing swing = ACSwing::SWING_ON;
            
            if (args.count("mode")) {
                mode = string_to_ac_mode(args["mode"]);
            }
            
            if (args.count("temp")) {
                try {
                    temp = std::stoi(args["temp"]);
                    if (temp < 16 || temp > 30) {
                        throw std::out_of_range("Temperature must be between 16 and 30");
                    }
                } catch (const std::invalid_argument&) {
                    throw std::invalid_argument("Invalid temperature value: " + args["temp"] + ". Must be a number between 16 and 30");
                } catch (const std::out_of_range&) {
                    throw std::out_of_range("Temperature must be between 16 and 30");
                }
            }
            
            if (args.count("fan")) {
                fan_speed = string_to_fan_speed(args["fan"]);
            }
            
            if (args.count("swing")) {
                swing = string_to_swing(args["swing"]);
            }
            
            logger.infof("Turning ON AC: %s", device_name.c_str());
            client.control_ac(device_name, "on", mode, temp, fan_speed, swing);
            logger.info("Command sent successfully");
        } 
        else if (command == "ac-off") {
            logger.infof("Turning OFF AC: %s", device_name.c_str());
            client.control_ac(device_name, "off", ACMode::COOL, 20, ACFanSpeed::FAN_MEDIUM, ACSwing::SWING_ON);
            logger.info("Command sent successfully");
        }
        else if (command == "boiler-status") {
            logger.infof("Getting boiler status for device: %s", device_name.c_str());
            BoilerStatus status = client.get_boiler_status(device_name);
            logger.infof("Boiler status: %s", status.to_string().c_str());
        }
        else if (command == "boiler-on") {
            logger.infof("Turning ON boiler: %s", device_name.c_str());
            int op_time = 0;
            if (args.count("time")) {
                try {
                    op_time = std::stoi(args["time"]);
                    if (op_time < 0) op_time = 0;
                } catch (...) {
                    logger.warning("Invalid --time value, defaulting to 0");
                    op_time = 0;
                }
            }
            client.control_boiler(device_name, "on", op_time);
            logger.info("Command sent successfully");
        }
        else if (command == "boiler-off") {
            logger.infof("Turning OFF boiler: %s", device_name.c_str());
            client.control_boiler(device_name, "off");
            logger.info("Command sent successfully");
        }
        else {
            logger.warning("Unknown command. Use 'switch-status', 'switch-on', 'switch-off', 'ac-status', 'ac-on', 'ac-off', 'boiler-status', 'boiler-on', or 'boiler-off'");
            return 1;
        }
    } 
    catch (const std::exception& e) {
        logger.errorf("Error: %s", e.what());
        return 1;
    }
    
    return 0;
}
