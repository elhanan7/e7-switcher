#!/usr/bin/env python3
"""
Example usage of the E7 Switcher Python client.

This example demonstrates how to use the E7 Switcher Python client
to control Switcher devices.
"""

import sys
import argparse
from e7_switcher import E7SwitcherClient, ACMode, ACFanSpeed, ACSwing


def main():
    """Run the example."""
    parser = argparse.ArgumentParser(description="E7 Switcher Python Client Example")
    parser.add_argument("--account", required=True, help="Switcher account username")
    parser.add_argument("--password", required=True, help="Switcher account password")
    
    subparsers = parser.add_subparsers(dest="command", help="Command to execute")
    
    # List devices command
    list_parser = subparsers.add_parser("list", help="List all devices")
    
    # Switch commands
    switch_status_parser = subparsers.add_parser("switch-status", help="Get switch status")
    switch_status_parser.add_argument("--device", required=True, help="Device name")
    
    switch_on_parser = subparsers.add_parser("switch-on", help="Turn switch on")
    switch_on_parser.add_argument("--device", required=True, help="Device name")
    switch_on_parser.add_argument("--time", type=int, default=0, help="Auto-off timer in seconds (0 for no timer)")
    
    switch_off_parser = subparsers.add_parser("switch-off", help="Turn switch off")
    switch_off_parser.add_argument("--device", required=True, help="Device name")
    switch_off_parser.add_argument("--time", type=int, default=0, help="Auto-off timer in seconds (0 for no timer)")
    
    # AC commands
    ac_status_parser = subparsers.add_parser("ac-status", help="Get AC status")
    ac_status_parser.add_argument("--device", required=True, help="Device name")
    
    ac_on_parser = subparsers.add_parser("ac-on", help="Turn AC on")
    ac_on_parser.add_argument("--device", required=True, help="Device name")
    ac_on_parser.add_argument("--mode", choices=["auto", "cool", "heat", "fan", "dry"], 
                             default="cool", help="AC mode")
    ac_on_parser.add_argument("--temp", type=int, default=20, 
                             help="Temperature (16-30)")
    ac_on_parser.add_argument("--fan", choices=["low", "medium", "high", "auto"], 
                             default="medium", help="Fan speed")
    ac_on_parser.add_argument("--swing", choices=["on", "off"], 
                             default="on", help="Swing setting")
    
    ac_off_parser = subparsers.add_parser("ac-off", help="Turn AC off")
    ac_off_parser.add_argument("--device", required=True, help="Device name")

    # Boiler commands
    boiler_status_parser = subparsers.add_parser("boiler-status", help="Get boiler status")
    boiler_status_parser.add_argument("--device", required=True, help="Device name")

    boiler_on_parser = subparsers.add_parser("boiler-on", help="Turn boiler on")
    boiler_on_parser.add_argument("--device", required=True, help="Device name")
    boiler_on_parser.add_argument("--time", type=int, default=0, help="Auto-off timer in seconds (0 for no timer)")

    boiler_off_parser = subparsers.add_parser("boiler-off", help="Turn boiler off")
    boiler_off_parser.add_argument("--device", required=True, help="Device name")
    boiler_off_parser.add_argument("--time", type=int, default=0, help="Auto-off timer in seconds (0 for no timer)")
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    try:
        # Create client
        client = E7SwitcherClient(args.account, args.password)
        
        if args.command == "list":
            devices = client.list_devices()
            print(f"Found {len(devices)} devices:")
            for device in devices:
                device_type = "Switch" if device["type"] == "0F04" else "AC" if device["type"] == "0E01" else "Unknown"
                status = "Online" if device["online"] else "Offline"
                print(f"  - {device['name']} ({device_type}): {status}")
        
        elif args.command == "switch-status":
            status = client.get_switch_status(args.device)
            state = "ON" if status["switch_state"] else "OFF"
            print(f"Switch {args.device} is {state}")
            print(f"  WiFi Power: {status['wifi_power']}")
            print(f"  Remaining Time: {status['remaining_time']} seconds")
        
        elif args.command == "switch-on":
            print(f"Turning ON switch: {args.device}")
            client.control_switch(args.device, True, args.time)
            print("Command sent successfully")
        
        elif args.command == "switch-off":
            print(f"Turning OFF switch: {args.device}")
            client.control_switch(args.device, False, args.time)
            print("Command sent successfully")
        
        elif args.command == "ac-status":
            status = client.get_ac_status(args.device)
            power = "ON" if status["power_status"] == 1 else "OFF"
            mode_map = {1: "AUTO", 2: "DRY", 3: "FAN", 4: "COOL", 5: "HEAT"}
            fan_map = {1: "LOW", 2: "MEDIUM", 3: "HIGH", 4: "AUTO"}
            swing = "ON" if status["swing"] == 1 else "OFF"
            
            print(f"AC {args.device} is {power}")
            if status["power_status"] == 1:
                print(f"  Mode: {mode_map.get(status['mode'], 'Unknown')}")
                print(f"  Temperature: {status['ac_temperature']}°C")
                print(f"  Fan Speed: {fan_map.get(status['fan_speed'], 'Unknown')}")
                print(f"  Swing: {swing}")
        
        elif args.command == "ac-on":
            print(f"Turning ON AC: {args.device}")
            
            # Convert string arguments to enums
            mode_map = {
                "auto": ACMode.AUTO,
                "cool": ACMode.COOL,
                "heat": ACMode.HEAT,
                "fan": ACMode.FAN,
                "dry": ACMode.DRY
            }
            
            fan_map = {
                "low": ACFanSpeed.FAN_LOW,
                "medium": ACFanSpeed.FAN_MEDIUM,
                "high": ACFanSpeed.FAN_HIGH,
                "auto": ACFanSpeed.FAN_AUTO
            }
            
            swing = ACSwing.SWING_ON if args.swing == "on" else ACSwing.SWING_OFF
            
            client.control_ac(
                args.device, 
                True,
                mode_map[args.mode],
                args.temp,
                fan_map[args.fan],
                swing
            )
            print("Command sent successfully")
        
        elif args.command == "ac-off":
            print(f"Turning OFF AC: {args.device}")
            client.control_ac(args.device, False, ACMode.COOL, 20, ACFanSpeed.FAN_MEDIUM, ACSwing.SWING_ON)
            print("Command sent successfully")

        elif args.command == "boiler-status":
            status = client.get_boiler_status(args.device)
            state = "ON" if status["switch_state"] else "OFF"
            print(f"Boiler {args.device} is {state}")
            print(f"  Power: {status['power']} W")
            print(f"  Energy: {status['electricity']} kWh")
            print(f"  Remaining Time: {status['remaining_time']} minutes")

        elif args.command == "boiler-on":
            print(f"Turning ON boiler: {args.device}")
            client.control_boiler(args.device, True, args.time)
            print("Command sent successfully")

        elif args.command == "boiler-off":
            print(f"Turning OFF boiler: {args.device}")
            client.control_boiler(args.device, False, args.time)
            print("Command sent successfully")
    
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
