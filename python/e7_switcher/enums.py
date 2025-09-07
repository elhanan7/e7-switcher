"""
Enumerations for the E7 Switcher Python Client.

This module contains enum classes that represent the various modes and settings
for Switcher devices.
"""

from enum import IntEnum


class ACMode(IntEnum):
    """Air conditioner operation modes."""
    AUTO = 1
    DRY = 2
    FAN = 3
    COOL = 4
    HEAT = 5
    
    def __str__(self):
        return self.name.lower()


class ACFanSpeed(IntEnum):
    """Air conditioner fan speed settings."""
    FAN_LOW = 1
    FAN_MEDIUM = 2
    FAN_HIGH = 3
    FAN_AUTO = 4
    
    def __str__(self):
        if self == ACFanSpeed.FAN_LOW:
            return "low"
        elif self == ACFanSpeed.FAN_MEDIUM:
            return "medium"
        elif self == ACFanSpeed.FAN_HIGH:
            return "high"
        else:
            return "auto"


class ACSwing(IntEnum):
    """Air conditioner swing settings."""
    SWING_OFF = 0
    SWING_ON = 1
    
    def __str__(self):
        return "on" if self == ACSwing.SWING_ON else "off"


class ACPower(IntEnum):
    """Air conditioner power state."""
    POWER_OFF = 0
    POWER_ON = 1
    
    def __str__(self):
        return "on" if self == ACPower.POWER_ON else "off"
