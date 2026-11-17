"""A Morse firmware's learning progress (firmware 0.4 and later): the bytes on the wire, and a
JSON file a person can read and edit.

On the wire (PROTOCOL.md, "Progress"): a format byte, the record (firmware/morse/learn.c
``ln_blob``: little-endian, no padding), a CRC-16/CCITT-FALSE of both; cut into chunks of 49
bytes, each sent as ``<index> <count>`` and the chunk with its high bits gathered in front of
each 7 bytes. The device takes a backup only after its owner presses PLAY on it.
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field

from . import protocol as P

ORDER = "KMURESNAPTLWI.JZ=FOY,VG5/Q92H38B?47C1D60X"  # learn.c LN_ORDER (LCWO's Koch order)
N = len(ORDER)
GROUP = 5
TOP = 5
NEVER = 0xFFFF
FORMAT = 1
BLOB_N = 2 + 2 + 1 + 4 + 4 + N * 2 + N * 2 * 2
WIRE_N = 1 + BLOB_N + 2
CHUNK = 49
FRAMES = (WIRE_N + CHUNK - 1) // CHUNK
JSON_FORMAT = "fm1-morse-progress"


class ProgressError(ValueError):
    pass


@dataclass
class Progress:
    round: int = 0
    streak: int = 0
    open: int = GROUP
    answered: int = 0
    right: int = 0
    # per character in ORDER: [recognise, send] boxes 0..5, and the round each was last answered (None: never)
    box: list[list[int]] = field(default_factory=lambda: [[0, 0] for _ in range(N)])
    last: list[list[int | None]] = field(default_factory=lambda: [[None, None] for _ in range(N)])

    def check(self) -> None:
        """What the firmware's ln_valid checks, so a file it would refuse is refused here first."""
        if not GROUP <= self.open <= N:
            raise ProgressError(f"open must be {GROUP}..{N}, not {self.open}")
        if self.right > self.answered:
            raise ProgressError("right is more than answered")
        if not (0 <= self.round <= 0xFFFF and 0 <= self.streak <= 0xFFFF):
            raise ProgressError("round and streak are 0..65535")
        for i, (b, l) in enumerate(zip(self.box, self.last)):
            for d in (0, 1):
                if not 0 <= b[d] <= TOP:
                    raise ProgressError(f"{ORDER[i]}: box {b[d]} is not 0..{TOP}")
                if l[d] is not None and not 0 <= l[d] <= self.round:
                    raise ProgressError(f"{ORDER[i]}: last answered in round {l[d]}, after round {self.round}")

    def level(self, i: int) -> int:
        return (self.box[i][0] + self.box[i][1]) // 2


def crc16(b: bytes) -> int:
    c = 0xFFFF
    for x in b:
        c ^= x << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) if c & 0x8000 else c << 1
            c &= 0xFFFF
    return c


def blob(p: Progress) -> bytes:
    p.check()
    out = bytearray()
    out += p.round.to_bytes(2, "little") + p.streak.to_bytes(2, "little") + bytes([p.open])
    out += p.answered.to_bytes(4, "little") + p.right.to_bytes(4, "little")
    for b in p.box:
        out += bytes(b)
    for l in p.last:
        for v in l:
            out += (NEVER if v is None else v).to_bytes(2, "little")
    assert len(out) == BLOB_N
    return bytes(out)


def unblob(b: bytes) -> Progress:
    if len(b) != BLOB_N:
        raise ProgressError(f"{len(b)} bytes, not {BLOB_N}")
    g = lambda o, n: int.from_bytes(b[o:o + n], "little")  # noqa: E731
    p = Progress(round=g(0, 2), streak=g(2, 2), open=b[4], answered=g(5, 4), right=g(9, 4))
    o = 13
    p.box = [[b[o + 2 * i], b[o + 2 * i + 1]] for i in range(N)]
    o += 2 * N
    p.last = []
    for i in range(N):
        pair = [g(o + 4 * i, 2), g(o + 4 * i + 2, 2)]
        p.last.append([None if v == NEVER else v for v in pair])
    p.check()
    return p


def pack7(b: bytes) -> bytes:
    out = bytearray()
    for i in range(0, len(b), 7):
        part = b[i:i + 7]
        out.append(sum(((x >> 7) & 1) << j for j, x in enumerate(part)))
        out += bytes(x & 0x7F for x in part)
    return bytes(out)


def unpack7(b: bytes) -> bytes:
    out = bytearray()
    i = 0
    while i < len(b):
        hi = b[i]
        i += 1
        for j in range(7):
            if i >= len(b):
                break
            out.append(b[i] | ((hi >> j) & 1) << 7)
            i += 1
    return bytes(out)


def wire(p: Progress) -> bytes:
    body = bytes([FORMAT]) + blob(p)
    c = crc16(body)
    return body + bytes([c & 0xFF, c >> 8])


def frames(p: Progress, type_: int = P.T_PROG_LOAD) -> list[bytes]:
    w = wire(p)
    return [P.frame(type_, bytes([i, FRAMES]) + pack7(w[i * CHUNK:(i + 1) * CHUNK])) for i in range(FRAMES)]


class Assembler:
    """T_PROG frames from the device, in order, into a Progress."""

    def __init__(self):
        self.buf = bytearray()
        self.next = 0

    def feed(self, payload: bytes) -> Progress | None:
        if len(payload) < 3 or payload[1] != FRAMES:
            raise ProgressError("not a progress frame this host knows")
        i = payload[0]
        if i == 0:
            self.buf, self.next = bytearray(), 0
        if i != self.next:
            raise ProgressError(f"chunk {i} came, {self.next} was due")
        self.buf += unpack7(payload[2:])
        self.next += 1
        if self.next < FRAMES:
            return None
        w = bytes(self.buf[:WIRE_N])
        if w[0] != FORMAT:
            raise ProgressError(f"format {w[0]}, this host knows {FORMAT}")
        if crc16(w[:-2]) != w[-2] | w[-1] << 8:
            raise ProgressError("CRC mismatch")
        return unblob(w[1:-2])


# ---- the file
def to_json(p: Progress, firmware: tuple[int, int] | None = None) -> str:
    d = {
        "format": JSON_FORMAT,
        "version": 1,
        "firmware": f"{firmware[0]}.{firmware[1]}" if firmware else None,
        "round": p.round,
        "streak": p.streak,
        "open": p.open,
        "answered": p.answered,
        "right": p.right,
        "chars": {
            ORDER[i]: {"recognise": p.box[i][0], "send": p.box[i][1],
                       "last_recognise": p.last[i][0], "last_send": p.last[i][1]}
            for i in range(N)
        },
    }
    return json.dumps(d, indent=1) + "\n"


def from_json(text: str) -> Progress:
    d = json.loads(text)
    if d.get("format") != JSON_FORMAT:
        raise ProgressError(f"not a {JSON_FORMAT} file")
    if d.get("version") != 1:
        raise ProgressError(f"file version {d.get('version')}, this host knows 1")
    p = Progress(round=int(d["round"]), streak=int(d.get("streak", 0)), open=int(d["open"]),
                 answered=int(d.get("answered", 0)), right=int(d.get("right", 0)))
    chars = d.get("chars", {})
    for c in chars:
        if c not in ORDER:
            raise ProgressError(f"{c!r} is not in the course")
    for i, c in enumerate(ORDER):
        e = chars.get(c, {})
        p.box[i] = [int(e.get("recognise", 0)), int(e.get("send", 0))]
        p.last[i] = [e.get("last_recognise"), e.get("last_send")]
    p.check()
    return p


def summary(p: Progress) -> str:
    learnt = sum(1 for i in range(p.open) if p.box[i][0] >= 2 and p.box[i][1] >= 1)
    return (f"round {p.round}, {p.open} open ({ORDER[:p.open]}), {learnt} past the line, "
            f"{p.answered} answered, {p.right} right, streak {p.streak}")


def sample() -> Progress:
    """The backup the tests load (firmware/morse/tests/basic.txt sends these frames)."""
    p = Progress(round=7, streak=2, open=15, answered=100, right=90)
    for i in range(15):
        p.box[i] = [3, 2]
        p.last[i] = [6, 6]
    return p


# shared with firmware/morse/tests/basic.txt: a fresh record's first and last chunk as the device sends them,
# and the sample backup to load
VECTORS = {
    "prog_fresh_0": P.hexs(frames(Progress(), P.T_PROG)[0]),
    "prog_fresh_5": P.hexs(frames(Progress(), P.T_PROG)[5]),
    **{f"prog_load_{i}": P.hexs(f) for i, f in enumerate(frames(sample()))},
}
