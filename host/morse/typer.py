"""Typing on the computer: each character as a Unicode key event (macOS, CGEvent through
ctypes; no extra package). Needs Accessibility permission for the terminal that runs it
(System Settings > Privacy & Security > Accessibility)."""

from __future__ import annotations

import ctypes
import sys

_FW = "/System/Library/Frameworks/{0}.framework/{0}"


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


class MacTyper:
    def __init__(self):
        if sys.platform != "darwin":
            raise OSError("typing on the computer: macOS only (use --dry-run elsewhere)")
        cg = ctypes.cdll.LoadLibrary(_FW.format("ApplicationServices"))
        cf = ctypes.cdll.LoadLibrary(_FW.format("CoreFoundation"))
        cg.CGEventCreateKeyboardEvent.restype = ctypes.c_void_p
        cg.CGEventCreateKeyboardEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint16, ctypes.c_bool]
        cg.CGEventKeyboardSetUnicodeString.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_uint16)]
        cg.CGEventPost.argtypes = [ctypes.c_uint32, ctypes.c_void_p]
        cf.CFRelease.argtypes = [ctypes.c_void_p]
        self.cg, self.cf = cg, cf

    def type(self, text: str) -> None:
        for ch in text:
            buf = (ctypes.c_uint16 * 1)(ord(ch))
            for down in (True, False):
                e = self.cg.CGEventCreateKeyboardEvent(None, 0, down)
                self.cg.CGEventKeyboardSetUnicodeString(e, 1, buf)
                self.cg.CGEventPost(0, e)  # kCGHIDEventTap: seen like the real keyboard
                self.cf.CFRelease(e)


class PrintTyper:
    """--dry-run: what would be typed, on the terminal."""

    def __init__(self, out=None):
        self.out = out if out is not None else sys.stdout
        self.typed = ""

    def type(self, text: str) -> None:
        self.typed += text
        print(text, end="", file=self.out, flush=True)
