#include "e7-switcher/logger.h"
#include <cstdarg>
#include <cstdio>
#include <memory>

#if defined(ARDUINO) || defined(ESP_PLATFORM) || defined(ESP32) || defined(ESP8266)
#define E7_PLATFORM_ESP 1
#include <Arduino.h>
#else
#define E7_PLATFORM_DESKTOP 1
#include <iostream>
#endif

namespace e7_switcher {

std::unique_ptr<Logger> Logger::instance_;

#ifdef E7_PLATFORM_ESP
// ESP32 implementation using Serial
class ESPLogger : public Logger {
private:
    LogLevel log_level_ = LogLevel::INFO;
public:
    ESPLogger() {
        Serial.begin(115200);
        while (!Serial) {
            ; // Wait for serial port to connect
        }
    }

    void debug(const std::string& message) override {
        if (log_level_ <= LogLevel::DEBUG) {
            Serial.print("[DEBUG] ");
            Serial.println(message.c_str());
        }
    }
    
    void set_log_level(LogLevel level) override {
        log_level_ = level;
    }

    void info(const std::string& message) override {
        Serial.print("[INFO] ");
        Serial.println(message.c_str());
    }

    void warning(const std::string& message) override {
        Serial.print("[WARNING] ");
        Serial.println(message.c_str());
    }

    void error(const std::string& message) override {
        Serial.print("[ERROR] ");
        Serial.println(message.c_str());
    }

    void debugf(const char* format, ...) override {
        if (log_level_ <= LogLevel::DEBUG) {
            Serial.print("[DEBUG] ");
            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            Serial.println(buffer);
            va_end(args);
        }
    }

    void infof(const char* format, ...) override {
        Serial.print("[INFO] ");
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        Serial.println(buffer);
        va_end(args);
    }

    void warningf(const char* format, ...) override {
        Serial.print("[WARNING] ");
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        Serial.println(buffer);
        va_end(args);
    }

    void errorf(const char* format, ...) override {
        Serial.print("[ERROR] ");
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        Serial.println(buffer);
        va_end(args);
    }
};

#else
// Linux/Mac implementation using stdout/stderr
class StdLogger : public Logger {
private:
    LogLevel log_level_ = LogLevel::INFO;
public:
    void debug(const std::string& message) override {
        if (log_level_ <= LogLevel::DEBUG) {
            std::cout << "[DEBUG] " << message << std::endl;
        }
    }
    
    void set_log_level(LogLevel level) override {
        log_level_ = level;
    }

    void info(const std::string& message) override {
        std::cout << "[INFO] " << message << std::endl;
    }

    void warning(const std::string& message) override {
        std::cerr << "[WARNING] " << message << std::endl;
    }

    void error(const std::string& message) override {
        std::cerr << "[ERROR] " << message << std::endl;
    }

    void debugf(const char* format, ...) override {
        if (log_level_ <= LogLevel::DEBUG) {
            std::cout << "[DEBUG] ";
            va_list args;
            va_start(args, format);
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            std::cout << buffer << std::endl;
            va_end(args);
        }
    }

    void infof(const char* format, ...) override {
        std::cout << "[INFO] ";
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        std::cout << buffer << std::endl;
        va_end(args);
    }

    void warningf(const char* format, ...) override {
        std::cerr << "[WARNING] ";
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        std::cerr << buffer << std::endl;
        va_end(args);
    }

    void errorf(const char* format, ...) override {
        std::cerr << "[ERROR] ";
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        std::cerr << buffer << std::endl;
        va_end(args);
    }
};
#endif

void Logger::initialize(LogLevel level) {
#ifdef E7_PLATFORM_ESP
    instance_ = std::make_unique<ESPLogger>();
#else
    instance_ = std::make_unique<StdLogger>();
#endif
    instance_->set_log_level(level);
}

Logger& Logger::instance() {
    if (!instance_) {
        initialize();
    }
    return *instance_;
}

} // namespace e7_switcher
