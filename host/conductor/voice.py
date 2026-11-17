"""A dictation hotkey held down from the FM-1: the daemon presses keys on this Mac as
if from the keyboard, so Wispr Flow, Handy or macOS Dictation start listening.

Hold mode (push-to-talk): key down while the device's button is held, up on release.
Toggle mode: a quick press and release on the button's press (for apps that toggle).
The keys are always let go: on release, after max_hold seconds, when the device
restarts, and when the daemon exits. Needs Accessibility permission for the app the
daemon runs in (System Settings > Privacy & Security > Accessibility).
"""

from __future__ import annotations

import ctypes
import sys
import time

_FW = "/System/Library/Frameworks/{0}.framework/{0}"

# macOS virtual key codes and the event flags each modifier sets
KEYCODES = {
    "ctrl": 0x3B, "control": 0x3B, "rctrl": 0x3E,
    "option": 0x3A, "alt": 0x3A, "ropt": 0x3D, "roption": 0x3D,
    "cmd": 0x37, "command": 0x37, "rcmd": 0x36,
    "shift": 0x38, "rshift": 0x3C,
    "fn": 0x3F,
    "space": 0x31, "return": 0x24, "escape": 0x35, "tab": 0x30,
    "f5": 0x60, "f6": 0x61, "f13": 0x69, "f14": 0x6B, "f15": 0x71, "f16": 0x6A, "f17": 0x40,
    "f18": 0x4F, "f19": 0x50,
}
FLAGS = {
    "ctrl": 0x40000, "control": 0x40000, "rctrl": 0x40000,
    "option": 0x80000, "alt": 0x80000, "ropt": 0x80000, "roption": 0x80000,
    "cmd": 0x100000, "command": 0x100000, "rcmd": 0x100000,
    "shift": 0x20000, "rshift": 0x20000,
    "fn": 0x800000,
}


class MacKeys:
    """CGEventPost through ctypes: no extra Python package needed."""

    def __init__(self):
        if sys.platform != "darwin":
            raise OSError("voice key: macOS only")
        cg = ctypes.cdll.LoadLibrary(_FW.format("ApplicationServices"))
        cf = ctypes.cdll.LoadLibrary(_FW.format("CoreFoundation"))
        cg.CGEventCreateKeyboardEvent.restype = ctypes.c_void_p
        cg.CGEventCreateKeyboardEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint16, ctypes.c_bool]
        cg.CGEventSetFlags.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        cg.CGEventPost.argtypes = [ctypes.c_uint32, ctypes.c_void_p]
        cf.CFRelease.argtypes = [ctypes.c_void_p]
        self.cg, self.cf = cg, cf

    def post(self, keycode: int, down: bool, flags: int) -> None:
        e = self.cg.CGEventCreateKeyboardEvent(None, keycode, down)
        self.cg.CGEventSetFlags(e, flags)
        self.cg.CGEventPost(0, e)  # kCGHIDEventTap: seen like the real keyboard
        self.cf.CFRelease(e)


def accessibility_trusted() -> bool | None:
    """True / False on macOS; None where it cannot be asked."""
    if sys.platform != "darwin":
        return None
    try:
        ax = ctypes.cdll.LoadLibrary(_FW.format("ApplicationServices"))
        ax.AXIsProcessTrusted.restype = ctypes.c_bool
        return bool(ax.AXIsProcessTrusted())
    except OSError:
        return None


class VoiceKey:
    def __init__(self, keys: list[str], mode: str = "hold", max_hold: float = 120.0, poster=None):
        bad = [k for k in keys if k.lower() not in KEYCODES]
        if bad or not keys:
            raise ValueError(f"voice keys not known: {bad or keys}")
        self.keys = [k.lower() for k in keys]
        self.mode = mode
        self.max_hold = max_hold
        self.poster = poster
        self.held_since: float | None = None

    def _poster(self):
        if self.poster is None:
            self.poster = MacKeys()
        return self.poster

    def _down(self):
        p, flags = self._poster(), 0
        for k in self.keys:
            flags |= FLAGS.get(k, 0)
            p.post(KEYCODES[k], True, flags)

    def _up(self):
        p = self._poster()
        flags = 0
        for k in self.keys:
            flags |= FLAGS.get(k, 0)
        for k in reversed(self.keys):
            flags &= ~FLAGS.get(k, 0)
            p.post(KEYCODES[k], False, flags)

    def press(self) -> str:
        if self.mode == "toggle":
            self._down()
            self._up()
            return "VOICE"
        if self.held_since is None:
            self._down()
            self.held_since = time.time()
        return "LISTENING"

    def release(self) -> str:
        if self.mode == "toggle":
            return ""
        self.let_go()
        return "VOICE OFF"

    def let_go(self) -> None:
        """Never leave the keys down."""
        if self.held_since is not None:
            self.held_since = None
            self._up()

    def tick(self, now: float | None = None) -> None:
        if self.held_since is not None and (now or time.time()) - self.held_since > self.max_hold:
            self.let_go()
