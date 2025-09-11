"""
Command Line Interface for the E7 Switcher Python client.

Provides convenient subcommands to list devices, control switches and AC units,
query device status, and more.

Credentials can be provided via flags or environment variables:
- E7_ACCOUNT
- E7_PASSWORD
"""

import argparse
import json
import os
import sys
from typing import Any, Dict
from . import E7SwitcherClient


def _make_client(args: argparse.Namespace) -> E7SwitcherClient:
    account = args.account or os.getenv("E7_ACCOUNT")
    password = args.password or os.getenv("E7_PASSWORD")
    if not account or not password:
        print("Error: account and password are required. Use --account/--password or set E7_ACCOUNT/E7_PASSWORD.", file=sys.stderr)
        sys.exit(2)
    return E7SwitcherClient(account, password)


def _print(obj: Any, fmt: str) -> None:
    if fmt == "json":
        print(json.dumps(obj, indent=2, ensure_ascii=False))
    else:
        # plain
        if isinstance(obj, (dict, list)):
            print(json.dumps(obj, ensure_ascii=False))
        else:
            print(str(obj))


def cmd_list_devices(args: argparse.Namespace) -> int:
    client = _make_client(args)
    devices = client.list_devices()
    _print(devices, args.output)
    return 0


def cmd_switch(args: argparse.Namespace) -> int:
    client = _make_client(args)
    action = args.action.lower()
    turn_on = action == "on"
    client.control_switch(args.device, turn_on, args.timer or 0)
    if args.wait_status:
        status = client.get_switch_status(args.device)
        _print(status, args.output)
    else:
        _print({"device": args.device, "action": action, "timer": args.timer or 0}, args.output)
    return 0


def cmd_switch_status(args: argparse.Namespace) -> int:
    client = _make_client(args)
    status = client.get_switch_status(args.device)
    _print(status, args.output)
    return 0


def cmd_boiler(args: argparse.Namespace) -> int:
    client = _make_client(args)
    action = args.action.lower()
    turn_on = action == "on"
    client.control_boiler(args.device, turn_on, args.timer or 0)
    if args.wait_status:
        status = client.get_boiler_status(args.device)
        _print(status, args.output)
    else:
        _print({"device": args.device, "action": action, "timer": args.timer or 0}, args.output)
    return 0


def cmd_boiler_status(args: argparse.Namespace) -> int:
    client = _make_client(args)
    status = client.get_boiler_status(args.device)
    _print(status, args.output)
    return 0

def cmd_ac(args: argparse.Namespace) -> int:
    client = _make_client(args)
    builder = client.control_ac_fluent(args.device)

    # Apply only provided arguments; otherwise keep current state initialized by the builder
    if args.action is not None:
        if args.action.lower() == "on":
            builder.on()
        else:
            builder.off()
    if args.mode is not None:
        builder.mode(args.mode)
    if args.temp is not None:
        builder.temperature(args.temp)
    if args.fan is not None:
        builder.fan(args.fan)
    if args.swing is not None:
        # accept on/off/true/false etc.
        v = str(args.swing).strip().lower()
        if v in ("on", "1", "true"):
            builder.swing(True)
        elif v in ("off", "0", "false"):
            builder.swing(False)
        else:
            builder.swing(args.swing)
    if args.timer is not None:
        builder.timer(args.timer)

    builder.do()
    if args.wait_status:
        status = client.get_ac_status(args.device)
        _print(status, args.output)
    else:
        _print(
            {
                "device": args.device,
                "action": args.action,
                "mode": args.mode,
                "temp": args.temp,
                "fan": args.fan,
                "swing": args.swing,
                "timer": args.timer,
            },
            args.output,
        )
    return 0


def cmd_ac_status(args: argparse.Namespace) -> int:
    client = _make_client(args)
    status = client.get_ac_status(args.device)
    _print(status, args.output)
    return 0


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(prog="e7-switcher", description="E7 Switcher CLI")
    p.add_argument("--account", help="Account username. Can also be set via E7_ACCOUNT env var.")
    p.add_argument("--password", help="Account password. Can also be set via E7_PASSWORD env var.")
    p.add_argument("-o", "--output", choices=["plain", "json"], default="plain", help="Output format")

    sub = p.add_subparsers(dest="cmd", required=True)

    # list devices
    p_list = sub.add_parser("list", help="List devices")
    p_list.set_defaults(func=cmd_list_devices)

    # switch on/off
    p_sw = sub.add_parser("switch", help="Control a switch")
    p_sw.add_argument("--device", required=True, help="Switch device name")
    p_sw.add_argument("action", choices=["on", "off"], help="Action to perform")
    p_sw.add_argument("--timer", type=int, default=0, help="Auto-off timer in seconds (0 for none)")
    p_sw.add_argument("--wait-status", action="store_true", help="After command, fetch and print status")
    p_sw.set_defaults(func=cmd_switch)

    # switch status
    p_swst = sub.add_parser("switch-status", help="Get switch status")
    p_swst.add_argument("--device", required=True, help="Switch device name")
    p_swst.set_defaults(func=cmd_switch_status)

    # ac on/off with options
    p_ac = sub.add_parser("ac", help="Control an AC")
    p_ac.add_argument("--device", required=True, help="AC device name")
    p_ac.add_argument("action", choices=["on", "off"], nargs='?', help="Action to perform (omit to keep current state)")
    p_ac.add_argument("--mode", help="AC mode: auto|dry|fan|cool|heat")
    p_ac.add_argument("--temp", type=int, help="Target temperature (omit to keep current)")
    p_ac.add_argument("--fan", help="Fan speed: low|medium|high|auto (omit to keep current)")
    p_ac.add_argument("--swing", help="Swing: on|off (omit to keep current)")
    p_ac.add_argument("--timer", type=int, help="Timer in seconds (omit for none)")
    p_ac.add_argument("--wait-status", action="store_true", help="After command, fetch and print status")
    p_ac.set_defaults(func=cmd_ac)

    # ac status
    p_acst = sub.add_parser("ac-status", help="Get AC status")
    p_acst.add_argument("--device", required=True, help="AC device name")
    p_acst.set_defaults(func=cmd_ac_status)

    # boiler on/off with optional timer
    p_bo = sub.add_parser("boiler", help="Control a boiler")
    p_bo.add_argument("--device", required=True, help="Boiler device name")
    p_bo.add_argument("action", choices=["on", "off"], help="Action to perform")
    p_bo.add_argument("--timer", type=int, default=0, help="Auto-off timer in seconds (0 for none)")
    p_bo.add_argument("--wait-status", action="store_true", help="After command, fetch and print status")
    p_bo.set_defaults(func=cmd_boiler)

    # boiler status
    p_bost = sub.add_parser("boiler-status", help="Get boiler status")
    p_bost.add_argument("--device", required=True, help="Boiler device name")
    p_bost.set_defaults(func=cmd_boiler_status)

    return p


def main(argv: Any = None) -> int:
    try:
        parser = build_parser()
        args = parser.parse_args(argv)
        return args.func(args)
    except KeyboardInterrupt:
        return 130
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
