"""Conductor wire protocol: SysEx frames between the FM-1 firmware and the host.

Every frame is ``F0 7D 43 4E <ver> <type> <payload...> F7``. ``7D`` is the MIDI
non-commercial manufacturer ID; ``43 4E`` ("CN") keeps us apart from fm1-base's own
``7D 4C ..`` frames. All payload bytes are 7-bit. See ``PROTOCOL.md``.
"""

from __future__ import annotations

from dataclasses import dataclass

HEADER = bytes([0x7D, 0x43, 0x4E])
VERSION = 1

# host -> device
T_HELLO_ACK = 0x01
T_CLEAR = 0x02
T_SLOT = 0x03
T_DETAIL = 0x04
T_META = 0x05
T_FOOTER = 0x06
T_BEEP = 0x07

# device -> host
T_HELLO = 0x41
T_HEARTBEAT = 0x42
T_ACTION = 0x43
T_SELECT = 0x44

# agent states (same numbers in firmware/conductor/app.c)
S_EMPTY, S_IDLE, S_WORKING, S_BLOCKED, S_DONE, S_ERROR, S_UNKNOWN = range(7)

# agent kinds
K_OTHER, K_CLAUDE, K_CODEX, K_OPENCODE, K_GEMINI, K_CURSOR, K_PI, K_SHELL = range(8)

# slot flags
F_QUESTION = 0x01  # blocked on a question, not a permission
F_REMOTE = 0x02  # runs on another machine

# ACTION ops
OP_APPROVE = 1
OP_APPROVE_ALWAYS = 2
OP_DENY = 3
OP_INTERRUPT = 4
OP_ENTER = 5
OP_CONTINUE = 6
OP_FOCUS = 7
OP_UP = 8
OP_DOWN = 9
OP_MODE = 10
OP_VOICE_DOWN = 11  # dictation hotkey held (the host presses keys on the computer)
OP_VOICE_UP = 12

OP_NAMES = {
    OP_APPROVE: "approve",
    OP_APPROVE_ALWAYS: "approve_always",
    OP_DENY: "deny",
    OP_INTERRUPT: "interrupt",
    OP_ENTER: "enter",
    OP_CONTINUE: "continue",
    OP_FOCUS: "focus",
    OP_UP: "up",
    OP_DOWN: "down",
    OP_MODE: "mode",
    OP_VOICE_DOWN: "voice_down",
    OP_VOICE_UP: "voice_up",
}

NSLOTS = 27
NAME_MAX = 20
DETAIL_LINES = 8
DETAIL_MAX = 28
FOOTER_MAX = 28


class ProtocolError(ValueError):
    pass


def _u14(v: int) -> bytes:
    v = max(0, min(int(v), 0x3FFF))
    return bytes([v & 0x7F, v >> 7])


def _r14(b: bytes, i: int) -> int:
    return b[i] | (b[i + 1] << 7)


def _text(s: str, n: int) -> bytes:
    out = bytearray()
    for ch in s[:n]:
        c = ord(ch)
        out.append(c if 0x20 <= c < 0x7F else 0x3F)  # '?' for anything not printable ASCII
    return bytes(out)


def frame(t: int, payload: bytes = b"") -> bytes:
    for b in payload:
        if b > 0x7F:
            raise ProtocolError(f"payload byte {b:#x} is not 7-bit")
    return bytes([0xF0]) + HEADER + bytes([VERSION, t]) + payload + bytes([0xF7])


# ---------------------------------------------------------------- host -> device


def hello_ack() -> bytes:
    return frame(T_HELLO_ACK, bytes([VERSION]))


def clear() -> bytes:
    return frame(T_CLEAR)


def slot(index: int, state: int, kind: int, flags: int, tag: int, age_min: int, name: str) -> bytes:
    if not 0 <= index < NSLOTS:
        raise ProtocolError(f"slot {index} out of range")
    return frame(
        T_SLOT,
        bytes([index, state, kind, flags & 0x7F]) + _u14(tag) + _u14(age_min) + _text(name, NAME_MAX),
    )


def detail(index: int, line: int, text: str) -> bytes:
    return frame(T_DETAIL, bytes([index, line]) + _text(text, DETAIL_MAX))


def meta(total: int, hidden: int, backend: str) -> bytes:
    return frame(T_META, _u14(total) + _u14(hidden) + _text(backend, 8))


def footer(text: str) -> bytes:
    return frame(T_FOOTER, _text(text, FOOTER_MAX))


def beep(sound: int) -> bytes:
    return frame(T_BEEP, bytes([sound]))


# ---------------------------------------------------------------- device -> host


@dataclass(frozen=True)
class Message:
    type: int
    payload: bytes


@dataclass(frozen=True)
class Action:
    op: int
    slot: int
    tag: int

    @property
    def name(self) -> str:
        return OP_NAMES.get(self.op, f"op{self.op}")


def parse(data: bytes | list[int]) -> Message | None:
    """Parse one SysEx frame (with or without F0/F7). None if it is not ours."""
    b = bytes(data)
    if b[:1] == b"\xf0":
        b = b[1:]
    if b[-1:] == b"\xf7":
        b = b[:-1]
    if not b.startswith(HEADER) or len(b) < len(HEADER) + 2:
        return None
    if b[len(HEADER)] != VERSION:
        raise ProtocolError(f"protocol version {b[len(HEADER)]}, expected {VERSION}")
    return Message(b[len(HEADER) + 1], b[len(HEADER) + 2 :])


def parse_action(m: Message) -> Action:
    if m.type != T_ACTION or len(m.payload) < 4:
        raise ProtocolError("not an ACTION frame")
    return Action(m.payload[0], m.payload[1], _r14(m.payload, 2))


def device_hello(fw_major: int = 0, fw_minor: int = 1) -> bytes:
    return frame(T_HELLO, bytes([VERSION, fw_major, fw_minor]))


def device_heartbeat() -> bytes:
    return frame(T_HEARTBEAT, bytes([VERSION]))


def device_action(op: int, index: int, tag: int) -> bytes:
    return frame(T_ACTION, bytes([op, index]) + _u14(tag))


def device_select(index: int) -> bytes:
    return frame(T_SELECT, bytes([index]))


def hexs(b: bytes) -> str:
    return " ".join(f"{x:02X}" for x in b)


# Shared test vectors: tests/test_protocol.py checks them here, and
# firmware/conductor/tests/basic.txt feeds the same bytes to the firmware.
VECTORS = {
    "hello_ack": (hello_ack(), "F0 7D 43 4E 01 01 01 F7"),
    "slot_blocked": (
        slot(0, S_BLOCKED, K_CLAUDE, 0, 0x0123, 5, "fix-tests"),
        "F0 7D 43 4E 01 03 00 03 01 00 23 02 05 00 66 69 78 2D 74 65 73 74 73 F7",
    ),
    "device_approve": (device_action(OP_APPROVE, 0, 0x0123), "F0 7D 43 4E 01 43 01 00 23 02 F7"),
}
