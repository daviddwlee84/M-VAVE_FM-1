"""Morse wire protocol: SysEx frames between the FM-1 firmware (firmware/morse) and the host.

Every frame is ``F0 7D 4D 4F <ver> <type> <payload...> F7``. ``7D`` is the MIDI
non-commercial manufacturer ID; ``4D 4F`` ("MO") keeps us apart from fm1-base's own
``7D 4C ..`` frames and Conductor's ``7D 43 4E``. All payload bytes are 7-bit. The keying
itself is not in these frames: it is MIDI note 0 on channel 1 (the Vail adapter's format).
See ``PROTOCOL.md``.
"""

from __future__ import annotations

from dataclasses import dataclass

HEADER = bytes([0x7D, 0x4D, 0x4F])
VERSION = 1

# host -> device
T_HELLO_ACK = 0x01
T_TEXT = 0x02
T_STOP = 0x03
T_PROG_REQ = 0x04  # firmware 0.4 and later: send the progress (progress.py)
T_PROG_LOAD = 0x05  # a chunk of progress to load (the device asks its owner first)

# device -> host
T_HELLO = 0x41
T_HEARTBEAT = 0x42
T_CHAR = 0x43
T_PROG = 0x44  # a chunk of the progress
T_PROG_DONE = 0x45  # the answer to a load: PD_*
PD_LOADED, PD_DECLINED, PD_BAD = 0, 1, 2
PROG_FIRMWARE = (0, 4)  # the first firmware with T_PROG_*

TR_USB, TR_BLE = 0, 1

TEXT_MAX = 60  # characters in one TEXT frame (the device's player queues 63)

# the characters the firmware knows (firmware/morse/morse.c MO_TABLE), and the space
MORSE_CHARS = set("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?'!/()&:;=+-_\"$@ ")
PROSIGN_OPEN, PROSIGN_CLOSE = "<", ">"  # characters between them are run together (<SK>), firmware 0.2 and later
UNKNOWN = "*"  # what the device's decoder sends for a pattern that is no character

# MIDI keying, as the Vail adapter sends it (vail-adapter docs/MIDI_INTEGRATION_SPEC.md)
KEY_NOTE = 0
KEY_ON = bytes([0x90, KEY_NOTE, 0x7F])
KEY_OFF = bytes([0x80, KEY_NOTE, 0x00])


class ProtocolError(ValueError):
    pass


@dataclass
class Msg:
    type: int
    payload: bytes


def frame(type_: int, payload: bytes = b"") -> bytes:
    if any(b > 0x7F for b in payload):
        raise ProtocolError("payload must be 7-bit")
    return bytes([0xF0, *HEADER, VERSION, type_, *payload, 0xF7])


def parse(data: bytes) -> Msg | None:
    """A frame (with or without F0 / F7) into a Msg; None when it is not ours."""
    if data[:1] == b"\xF0":
        data = data[1:]
    if data[-1:] == b"\xF7":
        data = data[:-1]
    if data[:3] != HEADER:
        return None
    if len(data) < 5:
        raise ProtocolError("short frame")
    if data[3] != VERSION:
        raise ProtocolError(f"protocol version {data[3]}, this host speaks {VERSION}")
    return Msg(data[4], bytes(data[5:]))


def clean_text(text: str) -> str:
    """What the device can play: upper case, the characters it has a code for, single spaces."""
    out = []
    for ch in text.upper():
        if ch in ("\n", "\t"):
            ch = " "
        if ch not in MORSE_CHARS and ch not in (PROSIGN_OPEN, PROSIGN_CLOSE):
            continue
        if ch == " " and (not out or out[-1] == " "):
            continue
        out.append(ch)
    return "".join(out).strip()


# ---- host -> device
def hello_ack() -> bytes:
    return frame(T_HELLO_ACK, bytes([VERSION]))


def text(s: str) -> list[bytes]:
    """TEXT frames for s, cut at TEXT_MAX (a space kept between the pieces)."""
    s = clean_text(s)
    frames = []
    while s:
        piece, s = s[:TEXT_MAX], s[TEXT_MAX:]
        frames.append(frame(T_TEXT, piece.encode("ascii")))
    return frames


def stop() -> bytes:
    return frame(T_STOP)


def prog_req() -> bytes:
    return frame(T_PROG_REQ)


# ---- device -> host (built here for tests and the simulator)
def device_hello(fw_major: int, fw_minor: int, transport: int = TR_USB) -> bytes:
    return frame(T_HELLO, bytes([VERSION, fw_major, fw_minor, transport]))


def device_heartbeat(transport: int = TR_USB) -> bytes:
    return frame(T_HEARTBEAT, bytes([VERSION, transport]))


def device_char(c: str) -> bytes:
    return frame(T_CHAR, c.encode("ascii"))


def firmware_of(m: Msg) -> tuple[int, int] | None:
    if m.type == T_HELLO and len(m.payload) >= 3:
        return m.payload[1], m.payload[2]
    return None


def transport_of(m: Msg) -> int:
    """USB unless a HELLO (4th byte) or HEARTBEAT (2nd byte) says Bluetooth (firmware 0.5)."""
    i = 3 if m.type == T_HELLO else 1 if m.type == T_HEARTBEAT else None
    return m.payload[i] if i is not None and len(m.payload) > i else TR_USB


def chars_of(m: Msg) -> str:
    if m.type != T_CHAR:
        return ""
    return "".join(chr(b) for b in m.payload if 0x20 <= b < 0x7F)


def hexs(b: bytes) -> str:
    return " ".join(f"{x:02X}" for x in b)


# shared with firmware/morse/tests/basic.txt: the same bytes, by the name in its comments
VECTORS = {
    "hello": (device_hello(0, 6), "F0 7D 4D 4F 01 41 01 00 06 00 F7"),
    "hello_ack": (hello_ack(), "F0 7D 4D 4F 01 01 01 F7"),
    "char_k": (device_char("K"), "F0 7D 4D 4F 01 43 4B F7"),
    "text_cq": (text("cq")[0], "F0 7D 4D 4F 01 02 43 51 F7"),
    "stop": (stop(), "F0 7D 4D 4F 01 03 F7"),
    "prog_req": (prog_req(), "F0 7D 4D 4F 01 04 F7"),
}
