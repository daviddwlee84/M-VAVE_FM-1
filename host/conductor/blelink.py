"""Bluetooth MIDI connections without Audio MIDI Setup (macOS 13 and later).

CoreMIDI's public MIDIBluetoothConnection.h describes the steps: find the peripheral
with CoreBluetooth, connect to it, call MIDIBluetoothDriverActivateAllConnections so
CoreMIDI takes it over as a MIDI device, then let CoreBluetooth's own connection go.
MIDIBluetoothDriverDisconnect(uuid) ends CoreMIDI's.

macOS does not reconnect a BLE MIDI device by itself after the device restarts, so the
daemon calls connect() when it finds no port. The terminal app needs the Bluetooth
permission (macOS asks the first time).
"""

from __future__ import annotations

import asyncio
import ctypes
import json
import sys
import time
from pathlib import Path

MIDI_SERVICE = "03b80e5a-ede8-4b33-a751-6ce34ec4c700"  # the BLE-MIDI service
NAME_PREFIX = "FM-1"  # fm1-base names it "FM-1 XXXX" from the flash part's ID
_CM = "/System/Library/Frameworks/CoreMIDI.framework/CoreMIDI"
_CF = "/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation"


class BleError(RuntimeError):
    pass


def _coremidi():
    if sys.platform != "darwin":
        raise BleError("Bluetooth MIDI connections: macOS only")
    cm = ctypes.cdll.LoadLibrary(_CM)
    if not hasattr(cm, "MIDIBluetoothDriverActivateAllConnections"):
        raise BleError("this macOS has no MIDIBluetoothConnection API (needs 13 or later)")
    cm.MIDIBluetoothDriverActivateAllConnections.restype = ctypes.c_int32
    cm.MIDIBluetoothDriverDisconnect.restype = ctypes.c_int32
    cm.MIDIBluetoothDriverDisconnect.argtypes = [ctypes.c_void_p]
    return cm


def _cfstring(s: str):
    cf = ctypes.cdll.LoadLibrary(_CF)
    cf.CFStringCreateWithCString.restype = ctypes.c_void_p
    cf.CFStringCreateWithCString.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_uint32]
    return cf.CFStringCreateWithCString(None, s.encode(), 0x08000100)  # kCFStringEncodingUTF8


def _ports() -> list[str]:
    import mido

    return mido.get_input_names()


def _matches(device, adv, prefix: str) -> bool:
    name = (adv.local_name or device.name or "").strip()
    return name.startswith(prefix) and (MIDI_SERVICE in [u.lower() for u in adv.service_uuids] or not adv.service_uuids)


async def _connect(prefix: str, scan_s: float):
    from bleak import BleakClient, BleakScanner

    dev = await BleakScanner.find_device_by_filter(lambda d, a: _matches(d, a, prefix), timeout=scan_s)
    if dev is None:
        return None
    before = set(_ports())
    async with BleakClient(dev, timeout=10.0):
        status = _coremidi().MIDIBluetoothDriverActivateAllConnections()
        if status != 0:
            raise BleError(f"MIDIBluetoothDriverActivateAllConnections: OSStatus {status}")
        end = time.time() + 6.0
        while time.time() < end:  # CoreMIDI registers the device: its port shows up
            names = _ports()
            if any((dev.name or "") and (dev.name in n) for n in names) or set(names) - before:
                break
            await asyncio.sleep(0.25)
    # leaving the block lets CoreBluetooth's connection go; CoreMIDI keeps its own (step 6)
    return dev


def connect(prefix: str = NAME_PREFIX, scan_s: float = 8.0, memo: Path | None = None) -> str | None:
    """Find an advertising FM-1 and hand it to CoreMIDI. Its name, or None if none was
    found. The peripheral's CoreBluetooth UUID is remembered in memo, for disconnect()."""
    try:
        dev = asyncio.run(_connect(prefix, scan_s))
    except BleError:
        raise
    except Exception as e:  # bleak: no permission, Bluetooth off, connection refused
        raise BleError(f"{type(e).__name__}: {e}") from e
    if dev is None:
        return None
    if memo:
        try:
            memo.parent.mkdir(parents=True, exist_ok=True)
            memo.write_text(json.dumps({"uuid": dev.address, "name": dev.name}))
        except OSError:
            pass
    return dev.name or dev.address


def disconnect(uuid: str | None = None, memo: Path | None = None) -> str:
    """End CoreMIDI's Bluetooth connection to the FM-1 (the one connect() remembered)."""
    if not uuid and memo and memo.exists():
        uuid = json.loads(memo.read_text()).get("uuid")
    if not uuid:
        raise BleError("no device remembered: connect with `just cond-ble connect` first, or pass its UUID")
    status = _coremidi().MIDIBluetoothDriverDisconnect(_cfstring(uuid))
    if status != 0:
        raise BleError(f"MIDIBluetoothDriverDisconnect: OSStatus {status}")
    return uuid


def main(argv=None) -> int:
    import argparse
    import os

    ap = argparse.ArgumentParser(prog="conductor.blelink", description="Bluetooth MIDI connect / disconnect for the FM-1")
    ap.add_argument("action", choices=["connect", "disconnect"])
    ap.add_argument("--prefix", default=NAME_PREFIX, help="advertised name to look for (default: FM-1)")
    ap.add_argument("--uuid", help="disconnect: this CoreBluetooth UUID instead of the remembered one")
    args = ap.parse_args(argv)
    memo = Path(os.environ.get("CONDUCTOR_HOME", Path.home() / ".config" / "conductor")) / "ble.json"
    try:
        if args.action == "connect":
            name = connect(args.prefix, memo=memo)
            if not name:
                print("no FM-1 advertising Bluetooth MIDI (is BLE grey on its screen? OCT+ turns it on)", file=sys.stderr)
                return 1
            print(f"[ble] connected: {name}; MIDI ports: {_ports()}")
        else:
            print(f"[ble] disconnected {disconnect(args.uuid, memo)}")
    except BleError as e:
        print(f"[ble] {e}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
