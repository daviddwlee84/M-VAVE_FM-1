"""A Morse firmware's learning progress (firmware 0.4 and later) and, from firmware 0.7, its
settings: the bytes on the wire, and a JSON file a person can read and edit.

On the wire (PROTOCOL.md, "Progress"): a format byte, the record (firmware/morse/learn.c
``ln_blob``: little-endian, no padding), in format 2 (firmware 0.7) the settings' 12 bytes, a
CRC-16/CCITT-FALSE of all before it; cut into chunks of 49 bytes, each sent as ``<index> <count>``
and the chunk with its high bits gathered in front of each 7 bytes. Both formats are 6 chunks.
The device takes a backup only after its owner presses PLAY on it.
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
FORMAT = 2                 # what firmware 0.7 sends; 1 (the progress alone) is what 0.4-0.6 send and take
SETTINGS_FIRMWARE = (0, 7)
BLOB_N = 2 + 2 + 1 + 4 + 4 + N * 2 + N * 2 * 2
SET_N = 12
WIRE_N = {1: 1 + BLOB_N + 2, 2: 1 + BLOB_N + SET_N + 2}
CHUNK = 49
FRAMES = (WIRE_N[2] + CHUNK - 1) // CHUNK
assert FRAMES == (WIRE_N[1] + CHUNK - 1) // CHUNK
JSON_FORMAT = "fm1-morse-progress"
JSON_VERSION = 2           # 2 adds "settings"; 1 (host 0.4-0.6) is read too

# the settings' names in the file, and their values (firmware/morse/app.c set_t and its enums)
KEYERS = ["straight", "iambic_a", "iambic_b"]
PAGES = ["menu", "practice", "listen", "key", "rhythm", "type", "chart"]   # START PAGE: PG_MENU .. PG_CHART
KEY_VIEWS = ["text", "code", "timing"]
TYPE_LAYOUTS = ["white_black", "a_to_z", "koch"]
LISTEN_PROGRAMS = ["koch", "words", "qso", "abbreviations", "text"]       # content.h LP_*


class ProgressError(ValueError):
    pass


@dataclass
class Settings:
    """firmware/morse/app.c set_t, field by field (set_blob's order)."""
    wpm: int = 20
    eff: int = 8
    keyer: int = 0
    swap: int = 0
    flags: int = 0             # bit 0 sound off, bit 1 Bluetooth off, bits 2-4 the start page
    cards: int = 20
    learn: int = 0
    look: int = 0              # bits 0-1 KEY view, 2 TYPE shows code, 3-4 TYPE layout, 5 LISTEN hides text,
    pitch: int = 600           # 6 TYPE composes, 7 strict judging
    send: int = 10
    prog: int = 0

    def check(self) -> None:
        """What the firmware's set_valid checks."""
        ok = (5 <= self.wpm <= 40 and 5 <= self.eff <= self.wpm and 0 <= self.keyer < len(KEYERS)
              and self.swap in (0, 1) and 0 <= self.flags < 0x20 and (self.flags >> 2 & 7) < len(PAGES)
              and 10 <= self.cards <= 30 and self.learn in (0, 1) and 300 <= self.pitch <= 1200
              and 0 <= self.look <= 0xFF and (self.look & 3) < len(KEY_VIEWS) and (self.look >> 3 & 3) < len(TYPE_LAYOUTS)
              and 5 <= self.send <= 40 and 0 <= self.prog < len(LISTEN_PROGRAMS))
        if not ok:
            raise ProgressError(f"settings the firmware would refuse: {self}")

    def blob(self) -> bytes:
        self.check()
        return bytes([self.wpm, self.eff, self.keyer, self.swap, self.flags, self.cards, self.learn, self.look,
                      self.pitch & 0xFF, self.pitch >> 8, self.send, self.prog])

    @classmethod
    def unblob(cls, b: bytes) -> "Settings":
        if len(b) != SET_N:
            raise ProgressError(f"{len(b)} settings bytes, not {SET_N}")
        s = cls(wpm=b[0], eff=b[1], keyer=b[2], swap=b[3], flags=b[4], cards=b[5], learn=b[6], look=b[7],
                pitch=b[8] | b[9] << 8, send=b[10], prog=b[11])
        s.check()
        return s

    def to_dict(self) -> dict:
        bit = lambda v, n: bool(v >> n & 1)  # noqa: E731
        return {
            "hear_wpm": self.wpm, "spacing_wpm": self.eff, "send_wpm": self.send, "pitch_hz": self.pitch,
            "keyer": KEYERS[self.keyer], "paddles": "left_dah" if self.swap else "left_dit",
            "sound": not bit(self.flags, 0), "bluetooth": not bit(self.flags, 1),
            "start_page": PAGES[self.flags >> 2 & 7], "cards": self.cards,
            "learn_by": "sight" if self.learn else "sound", "judging": "strict" if bit(self.look, 7) else "rhythm",
            "key_view": KEY_VIEWS[self.look & 3], "type_layout": TYPE_LAYOUTS[self.look >> 3 & 3],
            "type_shows_code": bit(self.look, 2), "type_composes": bit(self.look, 6),
            "listen_hides_text": bit(self.look, 5), "listen_program": LISTEN_PROGRAMS[self.prog],
        }

    @classmethod
    def from_dict(cls, d: dict) -> "Settings":
        dflt = cls().to_dict()
        unknown = set(d) - set(dflt)
        if unknown:
            raise ProgressError(f"unknown settings: {', '.join(sorted(unknown))}")
        v = {**dflt, **d}

        def pick(name, names):
            if v[name] not in names:
                raise ProgressError(f"{name} must be one of {', '.join(names)}, not {v[name]!r}")
            return names.index(v[name])

        s = cls(wpm=int(v["hear_wpm"]), eff=int(v["spacing_wpm"]), send=int(v["send_wpm"]), pitch=int(v["pitch_hz"]),
                keyer=pick("keyer", KEYERS), swap=pick("paddles", ["left_dit", "left_dah"]),
                flags=(0 if v["sound"] else 1) | (0 if v["bluetooth"] else 2) | pick("start_page", PAGES) << 2,
                cards=int(v["cards"]), learn=pick("learn_by", ["sound", "sight"]),
                look=pick("key_view", KEY_VIEWS) | bool(v["type_shows_code"]) << 2 | pick("type_layout", TYPE_LAYOUTS) << 3
                | bool(v["listen_hides_text"]) << 5 | bool(v["type_composes"]) << 6
                | (pick("judging", ["rhythm", "strict"])) << 7,
                prog=pick("listen_program", LISTEN_PROGRAMS))
        s.check()
        return s


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
    settings: Settings | None = None   # firmware 0.7 sends them; None: a backup from 0.4-0.6

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


def wire(p: Progress, fmt: int | None = None) -> bytes:
    """Format 2 when p has settings (unless fmt=1 asks for the progress alone), else 1."""
    if fmt is None:
        fmt = 2 if p.settings is not None else 1
    if fmt == 2 and p.settings is None:
        raise ProgressError("format 2 needs settings")
    body = bytes([fmt]) + blob(p) + (p.settings.blob() if fmt == 2 else b"")
    c = crc16(body)
    return body + bytes([c & 0xFF, c >> 8])


def format_for(p: Progress, firmware: tuple[int, int] | None) -> int:
    """What to send a device: format 2 only to firmware 0.7 and later, and only with settings."""
    return 2 if p.settings is not None and firmware is not None and firmware >= SETTINGS_FIRMWARE else 1


def frames(p: Progress, type_: int = P.T_PROG_LOAD, fmt: int | None = None) -> list[bytes]:
    w = wire(p, fmt)
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
        fmt = self.buf[0] if self.buf else None
        if fmt not in WIRE_N:
            raise ProgressError(f"format {fmt}, this host knows 1 and 2")
        w = bytes(self.buf[:WIRE_N[fmt]])
        if crc16(w[:-2]) != w[-2] | w[-1] << 8:
            raise ProgressError("CRC mismatch")
        p = unblob(w[1:1 + BLOB_N])
        if fmt == 2:
            p.settings = Settings.unblob(w[1 + BLOB_N:-2])
        return p


# ---- the file
def to_json(p: Progress, firmware: tuple[int, int] | None = None) -> str:
    d = {
        "format": JSON_FORMAT,
        "version": JSON_VERSION,
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
    if p.settings is not None:
        d["settings"] = p.settings.to_dict()
    return json.dumps(d, indent=1) + "\n"


def from_json(text: str) -> Progress:
    d = json.loads(text)
    if d.get("format") != JSON_FORMAT:
        raise ProgressError(f"not a {JSON_FORMAT} file")
    if d.get("version") not in (1, JSON_VERSION):
        raise ProgressError(f"file version {d.get('version')}, this host knows 1 and {JSON_VERSION}")
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
    if d.get("settings") is not None:
        p.settings = Settings.from_dict(d["settings"])
    p.check()
    return p


def summary(p: Progress) -> str:
    learnt = sum(1 for i in range(p.open) if p.box[i][0] >= 2 and p.box[i][1] >= 1)
    s = (f"round {p.round}, {p.open} open ({ORDER[:p.open]}), {learnt} past the line, "
         f"{p.answered} answered, {p.right} right, streak {p.streak}")
    if p.settings is not None:
        t = p.settings.to_dict()
        s += f"; settings: hear {t['hear_wpm']}/{t['spacing_wpm']}, send {t['send_wpm']}, start page {t['start_page']}"
    return s


def sample() -> Progress:
    """The backup the tests load (firmware/morse/tests/basic.txt sends these frames)."""
    p = Progress(round=7, streak=2, open=15, answered=100, right=90)
    for i in range(15):
        p.box[i] = [3, 2]
        p.last[i] = [6, 6]
    return p


def sample_v2() -> Progress:
    """The sample with settings (format 2): IAMBIC B at 25/25, send 18, starting on KEY."""
    p = sample()
    p.settings = Settings(wpm=25, eff=25, keyer=2, send=18, flags=3 << 2)
    return p


# shared with firmware/morse/tests/basic.txt: a fresh record's first and last chunk as firmware 0.7 sends them
# (format 2, the default settings), the sample backup to load as 0.4-0.6 took it (format 1), and with settings
VECTORS = {
    "prog_fresh_0": P.hexs(frames(Progress(settings=Settings()), P.T_PROG)[0]),
    "prog_fresh_5": P.hexs(frames(Progress(settings=Settings()), P.T_PROG)[5]),
    **{f"prog_load_{i}": P.hexs(f) for i, f in enumerate(frames(sample()))},
    **{f"prog_v2_{i}": P.hexs(f) for i, f in enumerate(frames(sample_v2()))},
}
