# Switcher E7 Control

A cross-platform library for controlling Switcher E7 devices from both ESP32 and desktop platforms (Mac/Linux).

## Features

- Control Switcher E7 devices
- Cross-platform compatibility (ESP32 and Mac/Linux)
- Platform-independent logging system
- JSON parsing for both platforms
- AES encryption support

## Building for ESP32

This project uses PlatformIO for ESP32 builds.

1. Install PlatformIO (if not already installed)
2. Open the project in PlatformIO
3. Build and upload to your ESP32 device:

```bash
pio run -t upload
```

## Building for Mac/Linux

The project uses CMake for desktop builds.

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler
- OpenSSL development libraries
- nlohmann_json library

### Installation of dependencies

#### macOS (using Homebrew)

```bash
brew install openssl cmake
brew install nlohmann-json
```

#### Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install cmake libssl-dev
sudo apt-get install nlohmann-json3-dev
```

### Building

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

### Initialization

Initialize the logger at the beginning of your application:

```cpp
#include "logger.h"

int main() {
    e7_switcher::Logger::initialize();
    // Your code here
    return 0;
}
```

### Logging

```cpp
#include "logger.h"

void some_function() {
    auto& logger = e7_switcher::Logger::instance();
    logger.info("This is an info message");
    logger.debugf("Debug with formatting: %d", 42);
}
```

### JSON Parsing

```cpp
#include "json_helpers.h"

void parse_json_example(const std::string& json_str) {
    std::vector<e7_switcher::Device> devices;
    if (e7_switcher::extract_device_list(json_str, devices)) {
        // Process devices
    }
    
    bool is_rest_day;
    if (e7_switcher::extract_is_rest_day(json_str, is_rest_day)) {
        // Use is_rest_day value
    }
}
```

## License

[Your license information here]
