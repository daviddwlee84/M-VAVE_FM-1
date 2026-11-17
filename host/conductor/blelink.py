"""Bluetooth MIDI on macOS: getting the FM-1 connected again after it restarts.

macOS does not reconnect a BLE MIDI device by itself when the device comes back
(2026-11-17, on the owner's Mac). CoreMIDI's public MIDIBluetoothConnection.h suggests
connecting with CoreBluetooth and calling MIDIBluetoothDriverActivateAllConnections;
tried the same day, it returned 0 but left the known device offline, with or without
MIDI notifications on and a CoreMIDI client with its run loop. So the daemon opens
macOS's own Bluetooth MIDI window (CoreAudioKit's CABTLEMIDIWindowController, the one in
Audio MIDI Setup) and waits: one click on Connect.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

SOURCE = Path(__file__).with_name("btpanel.swift")
CACHE = Path(os.environ.get("XDG_CACHE_HOME", Path.home() / ".cache")) / "conductor"


def panel_binary() -> Path | None:
    """btpanel.swift built once (swiftc, from the Xcode command line tools); None without it."""
    out = CACHE / "btpanel"
    if out.exists() and out.stat().st_mtime >= SOURCE.stat().st_mtime:
        return out
    swiftc = shutil.which("swiftc")
    if not swiftc:
        return None
    CACHE.mkdir(parents=True, exist_ok=True)
    r = subprocess.run([swiftc, "-O", "-o", str(out), str(SOURCE)], capture_output=True, text=True)
    return out if r.returncode == 0 and out.exists() else None


def open_panel() -> subprocess.Popen | None:
    """The Bluetooth MIDI window, or Audio MIDI Setup when the helper cannot be built."""
    if sys.platform != "darwin":
        return None
    exe = panel_binary()
    if exe:
        return subprocess.Popen([str(exe)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(["open", "-a", "Audio MIDI Setup"], check=False)
    return None


def fresh_ports() -> list[str]:
    """MIDI inputs as a new process sees them. A process's own CoreMIDI client may keep an
    old list without a run loop, so ask a fresh one."""
    r = subprocess.run(
        [sys.executable, "-c", "import mido; print('\\n'.join(mido.get_input_names()))"],
        capture_output=True, text=True,
    )
    return [ln for ln in r.stdout.splitlines() if ln.strip()]


def wait_for_port(timeout: float = 300.0, every: float = 1.0) -> str | None:
    end = time.time() + timeout
    while time.time() < end:
        ports = fresh_ports()
        if ports:
            return ports[0]
        time.sleep(every)
    return None


def main(argv=None) -> int:
    """python -m conductor.blelink: open the window and wait for a port."""
    p = open_panel()
    print("[ble] Bluetooth MIDI window open: click Connect next to FM-1 XXXX")
    port = wait_for_port()
    if p and p.poll() is None:
        p.terminate()
    if not port:
        print("[ble] no MIDI port appeared", file=sys.stderr)
        return 1
    print(f"[ble] connected: {port}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
