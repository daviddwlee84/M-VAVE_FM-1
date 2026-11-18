# SPDX-License-Identifier: GPL-3.0-or-later
"""THEORY's computer side: the wire, the record, JSON, and backup / restore against a stand-in FM-1.
    just theory-test"""

from pathlib import Path

import pytest

from theory import progress as G
from theory import protocol as P
from theory.__main__ import Session

ROOT = Path(__file__).resolve().parents[2]


def test_wire_basics():
    assert P.crc16(b"123456789") == 0x29B1          # CRC-16/CCITT-FALSE check value
    data = bytes(range(256))
    assert P.unpack7(P.pack7(data)) == data
    assert all(b < 0x80 for b in P.pack7(data))
    f = P.frame(P.T_PROG_REQ)
    assert f == bytes([0xF0, 0x7D, 0x54, 0x48, 0x01, 0x04, 0xF7])
    assert P.parse(f).type == P.T_PROG_REQ
    assert P.parse(bytes([0xF0, 0x7D, 0x4D, 0x4F, 0x01, 0x41, 0xF7])) is None   # Morse's, not ours
    with pytest.raises(P.ProtocolError):
        P.frame(0x05, bytes([0x80]))


def test_course_matches_the_firmware():
    assert G.N == 63 and G.GROUP_ENDS[0] == 6 and G.GROUP_ENDS[-1] == 63
    assert G.ITEMS[0] == "scale:C" and G.ITEMS[4:6] == ["earq:maj", "earq:min"]
    assert G.ITEMS.index("earq:dim") == 21 and G.ITEMS.index("earq:aug") == 32   # as tests/srs_test.c
    assert G.ITEMS[-1] == "eardeg:F#" and "keysig:C" not in G.ITEMS
    assert G.RECORD_N == 218 and G.FRAMES == 5


def test_record_round_trip_and_checks():
    p = G.Progress(round=9, streak=3, open=16, answered=50, right=44)
    p.box[3], p.last[3] = 4, 8
    p.settings["tonic"], p.settings["oct"] = -3, -1
    b = G.to_bytes(p)
    assert len(b) == G.RECORD_N
    assert G.from_bytes(b) == p
    bad = bytearray(b)
    bad[30] ^= 1
    with pytest.raises(G.ProgressError, match="CRC"):
        G.from_bytes(bytes(bad))
    p.box[0] = 6
    with pytest.raises(G.ProgressError):
        G.to_bytes(p)


def test_json_round_trip():
    p = G.Progress(round=4, open=11, answered=10, right=9)
    p.box[7], p.last[7] = 2, 3
    text = G.to_json(p)
    assert '"keysig:G"' in text and '"format": "fm1-theory-progress"' in text
    assert G.from_json(text) == p
    with pytest.raises(G.ProgressError):
        G.from_json('{"format": "fm1-morse-progress", "version": 1}')


def test_vectors_are_in_the_firmware_test():
    script = (ROOT / "firmware/theory/tests/basic.txt").read_text()
    for name, hexframe in G.vectors().items():
        assert hexframe in script, f"{name} is not in firmware/theory/tests/basic.txt"


class StandIn:
    """An FM-1 running THEORY, as far as the backup goes."""

    def __init__(self, record: G.Progress, answer: int = P.DONE_LOADED):
        self.record, self.answer = record, answer
        self.out: list[bytes] = [P.frame(P.T_HELLO, bytes([1, 0, 2, 0]))]
        self.loaded: G.Progress | None = None
        self._rx = bytearray()

    def send(self, frame: bytes) -> None:
        m = P.parse(frame)
        if m.type == P.T_PROG_REQ:
            b = G.to_bytes(self.record)
            self.out += [P.frame(P.T_PROG, bytes([i, G.FRAMES]) + P.pack7(b[i * G.CHUNK:(i + 1) * G.CHUNK]))
                         for i in range(G.FRAMES)]
        elif m.type == P.T_PROG_LOAD:
            self._rx += P.unpack7(m.payload[2:])
            if m.payload[0] == G.FRAMES - 1:
                self.loaded = G.from_bytes(bytes(self._rx))
                self.out.append(P.frame(P.T_PROG_DONE, bytes([self.answer])))

    def poll(self, timeout: float = 0.1) -> list[bytes]:
        out, self.out = self.out, []
        return out


def test_backup_and_restore_against_a_stand_in():
    p = G.Progress(round=7, open=11, answered=30, right=25)
    dev = StandIn(p)
    s = Session(dev, log=lambda *_: None)
    assert s.wait_connected(1.0) and s.firmware == (0, 2)
    assert s.backup(1.0) == p
    q = G.Progress(round=12, open=16, answered=60, right=50)
    assert s.restore(q, 1.0) == P.DONE_LOADED and dev.loaded == q
    dev2 = StandIn(p, answer=P.DONE_DECLINED)
    s2 = Session(dev2, log=lambda *_: None)
    assert s2.restore(q, 1.0) == P.DONE_DECLINED
