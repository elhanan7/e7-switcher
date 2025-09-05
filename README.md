# Switcher E7 Library

A cross-platform library for controlling Switcher E7 smart home devices from both ESP32 and desktop platforms (Mac/Linux).

## Features

- Control Switcher E7 devices (switches, AC units, etc.)
- Cross-platform compatibility (ESP32 and Mac/Linux)
- Platform-independent logging system
- JSON parsing for both platforms
- AES encryption support
- Easy integration with PlatformIO and CMake projects

## Installation

### Using PlatformIO

Add the library to your `platformio.ini` file:

```ini
lib_deps = 
    https://github.com/elhanan7/e7-switcher.git
```

### Using CMake

There are multiple ways to include this library in your CMake project:

#### Option 1: Using FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    e7-switcher
    GIT_REPOSITORY https://github.com/elhanan7/e7-switcher.git
    GIT_TAG main  # or specific tag/commit
)
FetchContent_MakeAvailable(e7-switcher)

# Link with your target
target_link_libraries(your_target PRIVATE e7-switcher)
```

#### Option 2: Using find_package

First, install the library:

```bash
git clone https://github.com/elhanan7/e7-switcher.git
cd e7-switcher
mkdir build && cd build
cmake ..
make
sudo make install
```

Then in your CMakeLists.txt:

```cmake
find_package(e7-switcher REQUIRED)
target_link_libraries(your_target PRIVATE e7-switcher::e7-switcher)
```

## Dependencies

### For ESP32
- Arduino framework
- ArduinoJson (v7.0.4 or higher)
- zlib-PIO

### For Desktop
- CMake 3.10 or higher
- C++17 compatible compiler
- OpenSSL development libraries
- nlohmann_json library (v3.11.2 or higher)
- zlib

## Usage

### Basic Usage

```cpp
#include "e7_switcher_client.h"
#include "logger.h"

// Initialize the logger
e7_switcher::Logger::initialize();
auto& logger = e7_switcher::Logger::instance();

// Create client with your credentials
e7_switcher::E7SwitcherClient client{"your_account", "your_password"};

// List all devices
const auto& devices = client.list_devices();
for (const auto& device : devices) {
    logger.infof("Device: %s (Type: %s)", device.name.c_str(), device.type.c_str());
}

// Control a switch
client.control_switch("Your Switch Name", "on");  // or "off"

// Get switch status
e7_switcher::SwitchStatus status = client.get_switch_status("Your Switch Name");
logger.infof("Switch status: %s", status.to_string().c_str());

// Control an AC unit
client.control_ac(
    "Your AC Name",
    "on",                        // "on" or "off"
    e7_switcher::ACMode::COOL,  // COOL, HEAT, FAN, DRY, AUTO
    22,                          // temperature
    e7_switcher::ACFanSpeed::FAN_MEDIUM,  // FAN_LOW, FAN_MEDIUM, FAN_HIGH, FAN_AUTO
    e7_switcher::ACSwing::SWING_ON        // SWING_OFF, SWING_ON, SWING_HORIZONTAL, SWING_VERTICAL
);
```

## Examples

The library includes examples for both ESP32 and desktop platforms:

### ESP32 Example

A simple example showing how to connect to WiFi and control Switcher devices from an ESP32.

```bash
cd examples/esp32_example
pio run -t upload
```

### Desktop Example

A command-line example for controlling devices from desktop platforms.

```bash
cd examples/desktop_example
pio run
# Or with CMake:
mkdir build && cd build
cmake .. -DBUILD_EXAMPLES=ON
make
./e7-switcher-desktop-example status  # Get device status
./e7-switcher-desktop-example on      # Turn device on
./e7-switcher-desktop-example off     # Turn device off
```

## Configuration

Create a `secrets.h` file with your credentials:

```cpp
#pragma once

#define E7_SWITCHER_ACCOUNT "your_account"
#define E7_SWITCHER_PASSWORD "your_password"
#define E7_SWITCHER_DEVICE_NAME "your_device_name"

// For ESP32 only
#define E7_SWITCHER_WIFI_SSID "your_wifi_ssid"
#define E7_SWITCHER_WIFI_PASSWORD "your_wifi_password"
```

## License

[Your license information here]
