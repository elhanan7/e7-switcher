"""
E7 Switcher Python Client

A Python wrapper for the E7 Switcher library that allows controlling Switcher devices.
"""

from .client import E7SwitcherClient
from .enums import ACMode, ACFanSpeed, ACSwing, ACPower
from .version import __version__

__all__ = [
    'E7SwitcherClient',
    'ACMode',
    'ACFanSpeed',
    'ACSwing',
    'ACPower',
]
