# E7 Switcher Library

A cross-platform library for controlling Switcher smart home devices from both ESP32 and desktop platforms (Mac/Linux).

## Features

- Control Switcher devices (switches, AC units, etc.)
- Cross-platform compatibility (ESP32 and Mac/Linux)
- Easy integration with PlatformIO and CMake projects
- Python bindings for easy integration with Python projects

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

### Using Python Bindings

The library provides Python bindings using pybind11, allowing you to control Switcher devices from Python.

#### Installation

```bash
# From the repository root
cd python
pip install .

# To specify a Python version
PYTHON_VERSION=3.9 pip install .
```

Alternatively, you can use the provided build script:

```bash
# From the repository root
cd python
./build_bindings.sh --install

# To specify a Python version
./build_bindings.sh --python-version 3.9 --install
```

You can also build the Python bindings using CMake:

```bash
mkdir build && cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
make
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

### Python Usage

```python
from e7_switcher import E7SwitcherClient, ACMode, ACFanSpeed, ACSwing

# Create client with your credentials
client = E7SwitcherClient("your_account", "your_password")

# List all devices
devices = client.list_devices()
for device in devices:
    print(f"Device: {device['name']}, Type: {device['type']}")

# Control a switch
client.control_switch("Your Switch Name", True)  # Turn on
client.control_switch("Your Switch Name", False)  # Turn off

# Get switch status
status = client.get_switch_status("Your Switch Name")
print(f"Switch is {'ON' if status['switch_state'] else 'OFF'}")

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
# First install the Python package
cd python
pip install .

# Then run the example
python examples/example_usage.py --account your_account --password your_password list
python examples/example_usage.py --account your_account --password your_password switch-status --device "Your Switch Name"
python examples/example_usage.py --account your_account --password your_password ac-on --device "Your AC Name" --mode cool --temp 22 --fan medium --swing on
```

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.
