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

# Control a switch
client.control_switch("Living Room Switch", True)  # Turn on
client.control_switch("Living Room Switch", False)  # Turn off

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

### Command-line interface

The package includes a command-line example in the `examples` directory:

```bash
# List all devices
python examples/example_usage.py --account your_account --password your_password list

# Get switch status
python examples/example_usage.py --account your_account --password your_password switch-status --device "Living Room Switch"

# Turn on a switch
python examples/example_usage.py --account your_account --password your_password switch-on --device "Living Room Switch"

# Turn on an AC with custom settings
python examples/example_usage.py --account your_account --password your_password ac-on --device "Bedroom AC" --mode cool --temp 22 --fan high --swing on
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
- `control_switch(device_name: str, turn_on: bool) -> None`: Control a switch device
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
