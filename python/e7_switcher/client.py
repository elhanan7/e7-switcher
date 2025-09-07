"""
E7 Switcher Python Client

A high-level Python wrapper for the E7 Switcher library.
"""

from typing import List, Dict, Optional, Union
import enum

from . import _core
from .enums import ACMode, ACFanSpeed, ACSwing, ACPower


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
    
    def list_devices(self) -> List[Dict[str, Union[str, bool, int]]]:
        """
        Get a list of all available devices.
        
        Returns:
            A list of device dictionaries, each containing device information
            
        Raises:
            RuntimeError: If the device list cannot be retrieved
        """
        return self._client.list_devices()
    
    def control_switch(self, device_name: str, turn_on: bool) -> None:
        """
        Control a switch device.
        
        Args:
            device_name: The name of the switch device
            turn_on: True to turn the switch on, False to turn it off
            
        Raises:
            RuntimeError: If the device is not found or the command fails
            ValueError: If the device is not a switch
        """
        action = "on" if turn_on else "off"
        self._client.control_switch(device_name, action)
    
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
            operation_time: Optional timer in minutes (0 for no timer)
            
        Raises:
            RuntimeError: If the device is not found or the command fails
            ValueError: If the device is not an AC or parameters are invalid
        """
        action = "on" if turn_on else "off"
        self._client.control_ac(device_name, action, mode, temperature, fan_speed, swing, operation_time)
    
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
