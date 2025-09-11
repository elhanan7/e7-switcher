# E7 Switcher Library

A cross-platform library for controlling Switcher smart home devices from ESP32 and desktop platforms (Mac/Linux/Windows), with support for x86_64 and aarch64.

Switcher smart home devices are manufactured by [Switcher](https://switcher.co.il/).

This repo is not affiliated with the Switcher product, it is meant to help give some extra automation capabilities to Switcher devices.

## Features

- Control Switcher devices (switches, AC units, boilers)
- Cross-platform compatibility: ESP32, Mac, Linux, and Windows
- CPU architectures: x86_64 and aarch64
- Easy integration with PlatformIO and CMake projects
- Python bindings for easy integration with Python projects

## Installation

### Using PlatformIO

Add the library to your `platformio.ini` file using the PlatformIO registry package name:

```ini
lib_deps =
    elhanan7/e7-switcher
```

Note: If you need the latest, unreleased changes, you can depend directly on the Git repository instead. Prefer the registry package for stability.

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

### Using Python Bindings

The library provides Python bindings using pybind11, allowing you to control Switcher devices from Python.

#### Installation

Install from PyPI (recommended):

```bash
pip install e7-switcher
```

Install from source (optional):

```bash
# From the repository root
pip install build
python -m build
pip install dist/e7-switcher-1.0.6-py3-none-any.whl
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

### For Python Bindings
- Python 3.6 or higher
- pybind11 (automatically fetched during build)
- pip and setuptools

## Usage

### Basic Usage

```cpp
#include "e7-switcher/e7_switcher_client.h"
#include "e7-switcher/logger.h"

// Initialize the logger (debug messages are disabled by default)
e7_switcher::Logger::initialize(); // Default log level is INFO
auto& logger = e7_switcher::Logger::instance();

// To enable debug messages:
// e7_switcher::Logger::initialize(e7_switcher::LogLevel::DEBUG);

// Create client with your credentials
e7_switcher::E7SwitcherClient client{"your_account", "your_password"};

// List all devices
const auto& devices = client.list_devices();
for (const auto& device : devices) {
    logger.infof("Device: %s (Type: %s)", device.name.c_str(), device.type.c_str());
}

// Control a switch (optional auto-off timer in seconds)
client.control_switch("Your Switch Name", "on", 0);  // action: "on"/"off", operation_time seconds

// Get switch status
e7_switcher::SwitchStatus status = client.get_switch_status("Your Switch Name");
logger.infof("Switch status: %s", status.to_string().c_str());

// Control a boiler (optional auto-off timer in seconds)
client.control_boiler("Your Boiler Name", "on", 1800);  // turn on for 1800 seconds (30 minutes)

// Get boiler status
e7_switcher::BoilerStatus boiler = client.get_boiler_status("Your Boiler Name");
logger.infof("Boiler status: %s", boiler.to_string().c_str());

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

### Python Usage

```python
from e7_switcher import E7SwitcherClient, ACMode, ACFanSpeed, ACSwing

# Create client with your credentials
client = E7SwitcherClient("your_account", "your_password")

# List all devices
devices = client.list_devices()
for device in devices:
    print(f"Device: {device['name']}, Type: {device['type']}")

# Control a switch (optional auto-off timer in seconds)
client.control_switch("Your Switch Name", True, 0)    # Turn on, no timer
client.control_switch("Your Switch Name", False, 600)  # Turn off with 600-second timer

# Get switch status
status = client.get_switch_status("Your Switch Name")
print(f"Switch is {'ON' if status['switch_state'] else 'OFF'}")

# Control a boiler (optional auto-off timer in seconds)
client.control_boiler("Your Boiler Name", True, 1800)  # Turn on for 1800 seconds (30 minutes)

# Get boiler status
boiler = client.get_boiler_status("Your Boiler Name")
print(f"Boiler is {'ON' if boiler['switch_state'] else 'OFF'}; power={boiler['power']}W; energy={boiler['electricity']}kWh")

# Control an AC unit
client.control_ac(
    "Your AC Name",
    True,                  # Turn on
    ACMode.COOL,           # Mode
    22,                    # Temperature
    ACFanSpeed.FAN_MEDIUM, # Fan speed
    ACSwing.SWING_ON       # Swing
)

# Get AC status
status = client.get_ac_status("Your AC Name")
print(f"AC is {'ON' if status['power_status'] == 1 else 'OFF'}")
```

#### Fluent AC Control (Python)

You can use a fluent, chainable interface to control AC devices. The fluent builder initializes from the current AC status and lets you set properties in a readable manner before executing with `do()`.

```python
from e7_switcher import E7SwitcherClient

client = E7SwitcherClient("your_account", "your_password")

# Start from current state, set target state fluently, then execute
client.control_ac_fluent("Your AC Name").cool().fan_low().temperature(22).on().do()

# Flexible setters: strings, enums, ints, and booleans
client.control_ac_fluent("Bedroom").mode("heat").fan("HIGH").swing_off().timer(30).on().do()

# Using enums explicitly
from e7_switcher import ACMode, ACFanSpeed, ACSwing
client.control_ac_fluent("Office").mode(ACMode.DRY).fan(ACFanSpeed.FAN_AUTO).swing(ACSwing.SWING_ON).do()
```

Available chainable methods include:

- Power: `on()`, `off()`, `power(value)`
- Mode: `mode(value)`, `auto()`, `dry()`, `fan_mode()`, `cool()`, `heat()`
- Temperature: `temperature(value)`
- Fan speed: `fan(value)`, `fan_low()`, `fan_medium()`, `fan_high()`, `fan_auto()`
- Swing: `swing(value)`, `swing_on()`, `swing_off()`
- Timer: `operation_time(seconds)`, `timer(seconds)`
- Execute: `do()`

Notes:

- The fluent API accepts Python enums (from `e7_switcher.enums`), strings (e.g., `"cool"`, `"FAN_HIGH"`), integers, and booleans where applicable.
- The Python enums are converted internally to the native `_core` enums for the underlying call, so you can use the Pythonic API without worrying about low-level details.

## Examples

The library includes examples for both ESP32 desktop and Python platforms:

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

### Python Example

A Python example for controlling devices using the Python bindings:

```bash
python python/examples/example_usage.py --account your_account --password your_password list
python python/examples/example_usage.py --account your_account --password your_password switch-status --device "Your Switch Name"
python python/examples/example_usage.py --account your_account --password your_password ac-on --device "Your AC Name" --mode cool --temp 22 --fan medium --swing on
python python/examples/example_usage.py --account your_account --password your_password boiler-status --device "Your Boiler Name"
python python/examples/example_usage.py --account your_account --password your_password boiler-on --device "Your Boiler Name" --time 30
python python/examples/example_usage.py --account your_account --password your_password boiler-off --device "Your Boiler Name"
```

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.
