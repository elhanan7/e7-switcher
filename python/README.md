# E7 Switcher Python Client

Python bindings for the E7 Switcher library, allowing you to control Switcher devices such as switches and air conditioners.

## Installation

### Prerequisites

- CMake (>= 3.10)
- C++ compiler with C++17 support
- Python (>= 3.6)
- OpenSSL
- nlohmann_json
- ZLIB

### Installing from source

```bash
# Clone the repository
git clone https://github.com/yourusername/e7-switcher.git
cd e7-switcher/python

# Install the package
pip install .

# Or to specify a Python version
PYTHON_VERSION=3.9 pip install .
```

## Usage

### Basic usage

```python
from e7_switcher import E7SwitcherClient, ACMode, ACFanSpeed, ACSwing

# Create a client
client = E7SwitcherClient("your_account", "your_password")

# List all devices
devices = client.list_devices()
for device in devices:
    print(f"Device: {device['name']}, Type: {device['type']}")

# Control a switch (optional auto-off timer in seconds)
client.control_switch("Living Room Switch", True, 0)   # Turn on, no timer

# Get switch status
status = client.get_switch_status("Living Room Switch")
print(f"Switch is {'ON' if status['switch_state'] else 'OFF'}")

# Control an AC
client.control_ac(
    "Bedroom AC",
    True,                  # Turn on
    ACMode.COOL,           # Mode
    20,                    # Temperature
    ACFanSpeed.FAN_MEDIUM, # Fan speed
    ACSwing.SWING_ON       # Swing
)

# Get AC status
status = client.get_ac_status("Bedroom AC")
print(f"AC is {'ON' if status['power_status'] == 1 else 'OFF'}")
```

### Fluent AC Control

You can control AC devices using a fluent, chainable interface. The builder initializes from the device's current AC status and lets you compose changes before executing with `do()`.

```python
from e7_switcher import E7SwitcherClient, ACMode, ACFanSpeed, ACSwing

client = E7SwitcherClient("your_account", "your_password")

# Start from current state, set target state fluently, then execute
client.control_ac_fluent("Bedroom AC").cool().fan_low().temperature(22).on().do()

# Flexible setters: strings, enums, ints, and booleans
client.control_ac_fluent("Living Room AC").mode("heat").fan("HIGH").swing_off().timer(30).on().do()

# Using enums explicitly
client.control_ac_fluent("Office AC").mode(ACMode.DRY).fan(ACFanSpeed.FAN_AUTO).swing(ACSwing.SWING_ON).do()
```

Available chainable methods include:

- Power: `on()`, `off()`, `power(value)`
- Mode: `mode(value)`, `auto()`, `dry()`, `fan_mode()`, `cool()`, `heat()`
- Temperature: `temperature(value)`
- Fan speed: `fan(value)`, `fan_low()`, `fan_medium()`, `fan_high()`, `fan_auto()`
- Swing: `swing(value)`, `swing_on()`, `swing_off()`
- Timer: `operation_time(seconds)`, `timer(seconds)`
- Execute: `do()`

### Command-line interface

A CLI is installed with the package, exposed as `e7-switcher`.
Environment variables `E7_ACCOUNT` and `E7_PASSWORD` can be used instead of flags.

```bash
# Show help
e7-switcher --help

# List all devices
e7-switcher --account your_account --password your_password list

# Switch control (timer is in seconds)
e7-switcher --account your_account --password your_password switch --device "Living Room Switch" on
e7-switcher --account your_account --password your_password switch --device "Living Room Switch" off
e7-switcher --account your_account --password your_password switch --device "Living Room Switch" on --timer 30

# Switch status
e7-switcher --account your_account --password your_password switch-status --device "Living Room Switch"

# AC control
e7-switcher --account your_account --password your_password ac --device "Bedroom AC" on --mode cool --temp 22 --fan high --swing on
e7-switcher --account your_account --password your_password ac --device "Bedroom AC" off

# AC status
e7-switcher --account your_account --password your_password ac-status --device "Bedroom AC"
```

## API Reference

### E7SwitcherClient

The main client class for interacting with Switcher devices.

#### Constructor

```python
E7SwitcherClient(account: str, password: str)
```

- `account`: The account username for the Switcher service
- `password`: The password for the Switcher service

#### Methods

- `list_devices() -> List[Dict[str, Union[str, bool, int]]]`: Get a list of all available devices
- `control_switch(device_name: str, turn_on: bool, operation_time: int = 0) -> None`: Control a switch device (operation_time in seconds)
- `control_ac(device_name: str, turn_on: bool, mode: ACMode = ACMode.COOL, temperature: int = 20, fan_speed: ACFanSpeed = ACFanSpeed.FAN_MEDIUM, swing: ACSwing = ACSwing.SWING_ON, operation_time: int = 0) -> None`: Control an air conditioner device
- `get_switch_status(device_name: str) -> Dict[str, Union[bool, int]]`: Get the status of a switch device
- `get_ac_status(device_name: str) -> Dict[str, Union[str, int, float, bool]]`: Get the status of an air conditioner device

### Enumerations

- `ACMode`: Air conditioner operation modes (AUTO, DRY, FAN, COOL, HEAT)
- `ACFanSpeed`: Air conditioner fan speed settings (FAN_LOW, FAN_MEDIUM, FAN_HIGH, FAN_AUTO)
- `ACSwing`: Air conditioner swing settings (SWING_OFF, SWING_ON)
- `ACPower`: Air conditioner power state (POWER_OFF, POWER_ON)

## Building wheels

To build a wheel package:

```bash
cd python
pip install build
python -m build
```

This will create both source distribution and wheel packages in the `dist` directory.

## Python version compatibility

The Python version used for building can be configured by setting the `PYTHON_VERSION` environment variable:

```bash
PYTHON_VERSION=3.9 pip install .
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.
