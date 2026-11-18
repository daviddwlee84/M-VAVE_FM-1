# SPDX-License-Identifier: GPL-3.0-or-later
"""THEORY's backup record, both ways: the wire bytes the FM-1 sends and takes (firmware/theory/app.c prog_fill,
srs.c sr_blob) and a JSON file a person can read.

Wire (218 bytes, then cut into 49-byte chunks, each 7-bit packed in a PROG / PROG_LOAD frame):
    format 1 | settings 12 | progress (srs.c, little-endian: round u16, streak u16, n u8, open u8, answered u32,
    right u32, box[n] u8, last[n] u16; 0xFFFF never asked) | CRC-16 of all before it, low byte first

Items are named by course.h's order, which only ever grows at the end: scale:C, relmin:C, ... earq:aug."""

from __future__ import annotations

import json
import struct
from dataclasses import dataclass, field

from . import protocol as P

FORMAT = 1
KEYS = ["C", "G", "F", "D", "Bb", "A", "Eb", "E", "Ab", "B", "Db", "F#"]
TOP = 5
NEVER = 0xFFFF
SET_FIELDS = ["tonic", "mode", "sevenths", "labels", "mapping", "keymode", "oct", "start", "cards", "speed"]
SET_N = 12
CHUNK = 49


def course() -> tuple[list[str], list[int]]:
    """course.h's items in their order, and where each group ends."""
    items, ends = [], []
    for g, k in enumerate(KEYS):
        items.append(f"scale:{k}")
        if g:
            items.append(f"keysig:{k}")
        items += [f"relmin:{k}", f"chords:{k}", f"eardeg:{k}"]
        if g == 0:
            items += ["earq:maj", "earq:min"]
        if g == 3:
            items.append("earq:dim")
        if g == 5:
            items.append("earq:aug")
        ends.append(len(items))
    return items, ends


ITEMS, GROUP_ENDS = course()
N = len(ITEMS)
BLOB_N = 14 + 3 * N
RECORD_N = 1 + SET_N + BLOB_N + 2
FRAMES = (RECORD_N + CHUNK - 1) // CHUNK


class ProgressError(ValueError):
    pass


def default_settings() -> dict[str, int]:
    return {k: 0 for k in SET_FIELDS}


@dataclass
class Progress:
    settings: dict[str, int] = field(default_factory=default_settings)
    round: int = 0
    streak: int = 0
    open: int = GROUP_ENDS[0]
    answered: int = 0
    right: int = 0
    box: list[int] = field(default_factory=lambda: [0] * N)
    last: list[int] = field(default_factory=lambda: [NEVER] * N)

    def check(self) -> None:
        """The firmware's own checks (app.c set_valid, srs.c sr_valid): a record it would refuse is refused here."""
        s = self.settings
        ok = (-9 <= s["tonic"] <= 12 and s["mode"] < 9 and s["sevenths"] <= 1 and s["labels"] < 2 and s["mapping"] < 2
              and s["keymode"] <= 1 and -2 <= s["oct"] <= 2 and s["start"] <= 5
              and (s["cards"] == 0 or 8 <= s["cards"] <= 24) and s["speed"] < 3)
        if not ok:
            raise ProgressError(f"settings out of range: {s}")
        if not (GROUP_ENDS[0] <= self.open <= N) or self.right > self.answered:
            raise ProgressError("open or answered out of range")
        for i in range(N):
            if self.box[i] > TOP or (self.last[i] != NEVER and self.last[i] > self.round):
                raise ProgressError(f"{ITEMS[i]}: box {self.box[i]}, last {self.last[i]} (round {self.round})")


def to_bytes(p: Progress) -> bytes:
    p.check()
    s = p.settings
    sb = bytes([s["tonic"] & 0xFF, s["mode"], s["sevenths"], s["labels"], s["mapping"], s["keymode"], s["oct"] & 0xFF,
                s["start"], s["cards"], s["speed"], 0, 0])
    blob = struct.pack("<HHBBII", p.round, p.streak, N, p.open, p.answered, p.right)
    blob += bytes(p.box) + b"".join(struct.pack("<H", v) for v in p.last)
    body = bytes([FORMAT]) + sb + blob
    c = P.crc16(body)
    return body + bytes([c & 0xFF, c >> 8])


def from_bytes(b: bytes) -> Progress:
    if len(b) < RECORD_N:
        raise ProgressError(f"{len(b)} bytes, a record is {RECORD_N}")
    b = b[:RECORD_N]
    if b[0] != FORMAT:
        raise ProgressError(f"format {b[0]}, this host knows {FORMAT}")
    c = P.crc16(b[:-2])
    if b[-2] != c & 0xFF or b[-1] != c >> 8:
        raise ProgressError("CRC mismatch: the record is damaged")
    sb = b[1:1 + SET_N]
    s = dict(zip(SET_FIELDS, sb[:10]))
    s["tonic"] = struct.unpack("b", bytes([sb[0]]))[0]
    s["oct"] = struct.unpack("b", bytes([sb[6]]))[0]
    blob = b[1 + SET_N:1 + SET_N + BLOB_N]
    rnd, streak, n, open_, answered, right = struct.unpack("<HHBBII", blob[:14])
    if n != N:
        raise ProgressError(f"{n} items, this host knows {N}")
    p = Progress(s, rnd, streak, open_, answered, right, list(blob[14:14 + N]),
                 [struct.unpack("<H", blob[14 + N + 2 * i:16 + N + 2 * i])[0] for i in range(N)])
    p.check()
    return p


def frames(p: Progress) -> list[bytes]:
    """The PROG_LOAD frames that load p."""
    b = to_bytes(p)
    return [P.frame(P.T_PROG_LOAD, bytes([i, FRAMES]) + P.pack7(b[i * CHUNK:(i + 1) * CHUNK])) for i in range(FRAMES)]


class Assembler:
    """PROG frames from the FM-1, in order, into a Progress."""

    def __init__(self) -> None:
        self.buf = bytearray()
        self.next = 0

    def feed(self, payload: bytes) -> Progress | None:
        if len(payload) < 3 or payload[1] != FRAMES or payload[0] != self.next:
            raise ProgressError(f"chunk {payload[0] if payload else '?'} out of order (want {self.next} of {FRAMES})")
        self.buf += P.unpack7(payload[2:])
        self.next += 1
        return from_bytes(bytes(self.buf)) if self.next == FRAMES else None


def to_json(p: Progress, firmware: str = "0.2") -> str:
    p.check()
    return json.dumps({
        "format": "fm1-theory-progress", "version": 1, "firmware": firmware,
        "settings": p.settings, "round": p.round, "streak": p.streak, "open": p.open,
        "open_up_to": ITEMS[p.open - 1], "answered": p.answered, "right": p.right,
        "items": {ITEMS[i]: {"box": p.box[i], "last": None if p.last[i] == NEVER else p.last[i]} for i in range(N)},
    }, indent=1)


def from_json(text: str) -> Progress:
    d = json.loads(text)
    if d.get("format") != "fm1-theory-progress" or d.get("version") != 1:
        raise ProgressError("not an fm1-theory-progress v1 file")
    p = Progress()
    p.settings = {**default_settings(), **{k: int(v) for k, v in d.get("settings", {}).items() if k in SET_FIELDS}}
    p.round, p.streak, p.open = int(d["round"]), int(d["streak"]), int(d["open"])
    p.answered, p.right = int(d["answered"]), int(d["right"])
    for i, name in enumerate(ITEMS):
        it = d["items"].get(name, {"box": 0, "last": None})
        p.box[i] = int(it["box"])
        p.last[i] = NEVER if it["last"] is None else int(it["last"])
    p.check()
    return p


def summary(p: Progress) -> str:
    return (f"round {p.round}, {p.open} of {N} open (up to {ITEMS[p.open - 1]}), "
            f"{p.answered} answered, {p.right} right, streak {p.streak}")


def vectors() -> dict[str, str]:
    """Frames shared with firmware/theory/tests/basic.txt: a fresh record out, and a record to load."""
    fresh = Progress()
    fr = [P.frame(P.T_PROG, f[6:-1]) for f in frames(fresh)]
    load = Progress(settings={**default_settings(), "tonic": 2, "start": 2, "cards": 12}, round=5, streak=2, open=11,
                    answered=40, right=35)
    for i in range(11):
        load.box[i], load.last[i] = (i % 3) + 1, 4
    hx = lambda b: " ".join(f"{x:02X}" for x in b)
    out = {f"fresh_{i}": hx(f) for i, f in enumerate(fr)}
    out.update({f"load_{i}": hx(f) for i, f in enumerate(frames(load))})
    return out


if __name__ == "__main__":
    for k, v in vectors().items():
        print(k, v)
