"""
E7 Switcher Python Client

A high-level Python wrapper for the E7 Switcher library.
"""

from typing import List, Dict, Union
import logging

from . import _core
from .enums import ACMode, ACFanSpeed, ACSwing, ACPower

# Module logger
_log = logging.getLogger(__name__)


class E7SwitcherClient:
    """
    Python client for controlling Switcher devices.
    
    This class provides a Pythonic interface to the E7 Switcher library,
    allowing you to control Switcher devices such as switches and air conditioners.
    """
    
    def __init__(self, account: str, password: str):
        """
        Initialize a new Switcher client.
        
        Args:
            account: The account username for the Switcher service
            password: The password for the Switcher service
        
        Raises:
            RuntimeError: If connection or authentication fails
        """
        self._client = _core.E7SwitcherClient(account, password)
    
    @staticmethod
    def _to_core_enum(core_enum_cls, value):
        """
        Convert a Python-side enum/int/string to the corresponding _core enum.
        Accepts:
          - Already-correct _core enum -> returned as-is
          - Python IntEnum from python.e7_switcher.enums -> converted by name
          - Int value -> converted by value
          - String name -> converted by member name
        """
        # If it's already the correct _core enum type
        if isinstance(value, core_enum_cls):
            return value
        
        # If it looks like an Enum with a name (e.g., our Python IntEnum), try by name
        name = getattr(value, 'name', None)
        if isinstance(name, str) and hasattr(core_enum_cls, name):
            return getattr(core_enum_cls, name)
        
        # Try by string member name directly
        if isinstance(value, str):
            # Support both exact and upper-case names
            if hasattr(core_enum_cls, value):
                return getattr(core_enum_cls, value)
            upper_name = value.upper()
            if hasattr(core_enum_cls, upper_name):
                return getattr(core_enum_cls, upper_name)
            raise ValueError(f"Invalid enum name '{value}' for {core_enum_cls.__name__}")
        
        # Try by numeric value (works for IntEnum and ints)
        try:
            int_value = int(value)
            return core_enum_cls(int_value)
        except Exception as exc:
            raise ValueError(f"Invalid value '{value}' for enum {core_enum_cls.__name__}") from exc
    
    def list_devices(self) -> List[Dict[str, Union[str, bool, int]]]:
        """
        Get a list of all available devices.
        
        Returns:
            A list of device dictionaries, each containing device information
            
        Raises:
            RuntimeError: If the device list cannot be retrieved
        """
        return self._client.list_devices()
    
    def control_switch(self, device_name: str, turn_on: bool, operation_time: int = 0) -> None:
        """
        Control a switch device.
        
        Args:
            device_name: The name of the switch device
            turn_on: True to turn the switch on, False to turn it off
            operation_time: Optional auto-off timer in seconds (0 for no timer)
            
        Raises:
            RuntimeError: If the device is not found or the command fails
            ValueError: If the device is not a switch
        """
        action = "on" if turn_on and turn_on != "off" else "off"
        self._client.control_switch(device_name, action, operation_time)
    
    def control_ac(self, 
                  device_name: str, 
                  turn_on: bool, 
                  mode: ACMode = ACMode.COOL, 
                  temperature: int = 20, 
                  fan_speed: ACFanSpeed = ACFanSpeed.FAN_MEDIUM, 
                  swing: ACSwing = ACSwing.SWING_ON, 
                  operation_time: int = 0) -> None:
        """
        Control an air conditioner device.
        
        Args:
            device_name: The name of the AC device
            turn_on: True to turn the AC on, False to turn it off
            mode: The AC operation mode
            temperature: The target temperature (16-30)
            fan_speed: The fan speed setting
            swing: The swing setting
            operation_time: Optional timer in seconds (0 for no timer)
            
        Raises:
            RuntimeError: If the device is not found or the command fails
            ValueError: If the device is not an AC or parameters are invalid
        """
        action = "on" if turn_on and turn_on != "off" else "off"
        core_mode = self._to_core_enum(_core.ACMode, mode)
        core_fan_speed = self._to_core_enum(_core.ACFanSpeed, fan_speed)
        core_swing = self._to_core_enum(_core.ACSwing, swing)
        self._client.control_ac(
            device_name,
            action,
            core_mode,
            temperature,
            core_fan_speed,
            core_swing,
            operation_time,
        )
    
    def control_boiler(self, device_name: str, turn_on: bool, operation_time: int = 0) -> None:
        """
        Control a boiler device.
        
        Args:
            device_name: The name of the boiler device
            turn_on: True to turn the boiler on, False to turn it off
            operation_time: Optional auto-off timer in seconds (0 for no timer)
        """
        action = "on" if turn_on and turn_on != "off" else "off"
        self._client.control_boiler(device_name, action, operation_time)
    
    def get_switch_status(self, device_name: str) -> Dict[str, Union[bool, int]]:
        """
        Get the status of a switch device.
        
        Args:
            device_name: The name of the switch device
            
        Returns:
            A dictionary containing the switch status information
            
        Raises:
            RuntimeError: If the device is not found or the status cannot be retrieved
            ValueError: If the device is not a switch
        """
        return self._client.get_switch_status(device_name)
    
    def get_ac_status(self, device_name: str) -> Dict[str, Union[str, int, float, bool]]:
        """
        Get the status of an air conditioner device.
        
        Args:
            device_name: The name of the AC device
            
        Returns:
            A dictionary containing the AC status information
            
        Raises:
            RuntimeError: If the device is not found or the status cannot be retrieved
            ValueError: If the device is not an AC
        """
        return self._client.get_ac_status(device_name)
    
    def get_boiler_status(self, device_name: str) -> Dict[str, Union[bool, int, float]]:
        """
        Get the status of a boiler device.
        
        Args:
            device_name: The name of the boiler device
        
        Returns:
            A dictionary containing the boiler status information
        """
        return self._client.get_boiler_status(device_name)
    
    def control_ac_fluent(self, device_name: str) -> "ACFluentControl":
        """Start a fluent AC control sequence for the given device."""
        return ACFluentControl(self, device_name)


class ACFluentControl:
    """
    Fluent builder for AC control.

    Usage:
        client.control_ac_fluent(device).cool().fan_low().temperature(22).on().do()
    """
    def __init__(self, client: "E7SwitcherClient", device_name: str):
        self._client = client
        self._device_name = device_name
        # Initialize from current status
        status = client.get_ac_status(device_name)
        # Map current status to Python enums/values (they align with core values)
        try:
            self._power: ACPower = ACPower(int(status.get("power_status", 0)))
        except Exception as exc:
            _log.warning("Failed to parse 'power_status' from AC status: %r; defaulting to POWER_OFF", status.get("power_status", None))
            self._power = ACPower.POWER_OFF
        try:
            self._mode: ACMode = ACMode(int(status.get("mode", ACMode.COOL)))
        except Exception as exc:
            _log.warning("Failed to parse 'mode' from AC status: %r; defaulting to COOL", status.get("mode", None))
            self._mode = ACMode.COOL
        self._temperature: int = int(status.get("ac_temperature", status.get("temperature", 20)))
        try:
            self._fan_speed: ACFanSpeed = ACFanSpeed(int(status.get("fan_speed", ACFanSpeed.FAN_MEDIUM)))
        except Exception as exc:
            _log.warning("Failed to parse 'fan_speed' from AC status: %r; defaulting to FAN_MEDIUM", status.get("fan_speed", None))
            self._fan_speed = ACFanSpeed.FAN_MEDIUM
        try:
            self._swing: ACSwing = ACSwing(int(status.get("swing", ACSwing.SWING_ON)))
        except Exception as exc:
            _log.warning("Failed to parse 'swing' from AC status: %r; defaulting to SWING_ON", status.get("swing", None))
            self._swing = ACSwing.SWING_ON
        self._operation_time: int = 0
    
    # Power controls
    def on(self):
        self._power = ACPower.POWER_ON
        return self
    
    def off(self):
        self._power = ACPower.POWER_OFF
        return self
    
    def power(self, value: Union[bool, ACPower, str, int]):
        if isinstance(value, ACPower):
            self._power = value
        elif isinstance(value, bool):
            self._power = ACPower.POWER_ON if value else ACPower.POWER_OFF
        elif isinstance(value, str):
            v = value.strip().lower()
            if v in ("on", "power_on", "1", "true"):
                self._power = ACPower.POWER_ON
            elif v in ("off", "power_off", "0", "false"):
                self._power = ACPower.POWER_OFF
            else:
                # try enum member name
                try:
                    self._power = ACPower[value]
                except Exception as exc:
                    raise ValueError(f"Invalid power value: {value}") from exc
        else:
            self._power = ACPower(int(value))
        return self
    
    # Mode controls
    def mode(self, value: Union[ACMode, str, int]):
        if isinstance(value, ACMode):
            self._mode = value
        elif isinstance(value, str):
            v = value.strip().upper()
            self._mode = ACMode[v]
        else:
            self._mode = ACMode(int(value))
        return self
    
    def auto(self):
        self._mode = ACMode.AUTO
        return self
    
    def dry(self):
        self._mode = ACMode.DRY
        return self
    
    def fan_mode(self):
        self._mode = ACMode.FAN
        return self
    
    def cool(self):
        self._mode = ACMode.COOL
        return self
    
    def heat(self):
        self._mode = ACMode.HEAT
        return self
    
    # Temperature
    def temperature(self, value: int):
        self._temperature = int(value)
        return self
    
    # Fan speed controls
    def fan(self, value: Union[ACFanSpeed, str, int]):
        if isinstance(value, ACFanSpeed):
            self._fan_speed = value
        elif isinstance(value, str):
            v = value.strip().upper()
            if not v.startswith("FAN_") and v in ("LOW", "MEDIUM", "HIGH", "AUTO"):
                v = f"FAN_{v}"
            self._fan_speed = ACFanSpeed[v]
        else:
            self._fan_speed = ACFanSpeed(int(value))
        return self
    
    def fan_low(self):
        self._fan_speed = ACFanSpeed.FAN_LOW
        return self
    
    def fan_medium(self):
        self._fan_speed = ACFanSpeed.FAN_MEDIUM
        return self
    
    def fan_high(self):
        self._fan_speed = ACFanSpeed.FAN_HIGH
        return self
    
    def fan_auto(self):
        self._fan_speed = ACFanSpeed.FAN_AUTO
        return self
    
    # Swing controls
    def swing(self, value: Union[ACSwing, bool, str, int]):
        if isinstance(value, ACSwing):
            self._swing = value
        elif isinstance(value, bool):
            self._swing = ACSwing.SWING_ON if value else ACSwing.SWING_OFF
        elif isinstance(value, str):
            v = value.strip().lower()
            if v in ("on", "1", "true"):
                self._swing = ACSwing.SWING_ON
            elif v in ("off", "0", "false"):
                self._swing = ACSwing.SWING_OFF
            else:
                self._swing = ACSwing[v.upper()]
        else:
            self._swing = ACSwing(int(value))
        return self
    
    def swing_on(self):
        self._swing = ACSwing.SWING_ON
        return self
    
    def swing_off(self):
        self._swing = ACSwing.SWING_OFF
        return self
    
    # Operation time / timer
    def operation_time(self, seconds: int):
        self._operation_time = int(seconds)
        return self
    
    def timer(self, seconds: int):
        return self.operation_time(seconds)
    
    # Execute
    def do(self):
        turn_on = (self._power == ACPower.POWER_ON)
        self._client.control_ac(
            self._device_name,
            turn_on,
            self._mode,
            self._temperature,
            self._fan_speed,
            self._swing,
            self._operation_time,
        )
        return True


def control_ac_fluent(self: E7SwitcherClient, device_name: str) -> ACFluentControl:
    """Start a fluent AC control sequence for the given device."""
    return ACFluentControl(self, device_name)


