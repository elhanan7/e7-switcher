#pragma once
#include <string>
#include <memory>

namespace e7_switcher {

class Logger {
public:
    virtual ~Logger() = default;
    
    virtual void debug(const std::string& message) = 0;
    virtual void info(const std::string& message) = 0;
    virtual void warning(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
    
    // Format string with variadic arguments (similar to printf)
    virtual void debugf(const char* format, ...) = 0;
    virtual void infof(const char* format, ...) = 0;
    virtual void warningf(const char* format, ...) = 0;
    virtual void errorf(const char* format, ...) = 0;
    
    // Singleton access
    static Logger& instance();
    static void initialize();

private:
    static std::unique_ptr<Logger> instance_;
};

} // namespace e7_switcher
