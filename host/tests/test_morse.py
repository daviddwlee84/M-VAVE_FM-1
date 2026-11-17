"""host/morse: the protocol's shared vectors (firmware/morse/tests/basic.txt has the same bytes),
text cleaning, and the host's answers, against the simulated FM-1."""

import io

import pytest

from morse import protocol as P
from morse.core import Host
from morse.device import SimLink
from morse.typer import PrintTyper


@pytest.mark.parametrize("name", sorted(P.VECTORS))
def test_shared_vectors(name):
    frame, hexed = P.VECTORS[name]
    assert P.hexs(frame) == hexed


def test_vectors_are_in_the_firmware_test():
    from pathlib import Path

    script = (Path(__file__).parents[2] / "firmware/morse/tests/basic.txt").read_text()
    for name, (_, hexed) in P.VECTORS.items():
        assert hexed in script, f"{name} not in basic.txt"


def test_not_ours_and_wrong_version():
    assert P.parse(bytes([0xF0, 0x7D, 0x43, 0x4E, 0x01, 0x41, 0xF7])) is None  # Conductor's
    with pytest.raises(P.ProtocolError):
        P.parse(bytes([0xF0, 0x7D, 0x4D, 0x4F, 0x09, 0x41, 0xF7]))


def test_clean_text():
    assert P.clean_text("  cq  cq\nde FM1 k ") == "CQ CQ DE FM1 K"
    assert P.clean_text("73 <sk>") == "73 <SK>"
    assert P.clean_text("naïve #1 ~ ok?") == "NAVE 1 OK?"
    assert P.clean_text("") == ""


def test_text_is_cut_and_7bit():
    frames = P.text("paris " * 30)
    assert len(frames) == 3
    for f in frames:
        assert f[:6] == bytes([0xF0, 0x7D, 0x4D, 0x4F, 0x01, P.T_TEXT]) and f[-1] == 0xF7
        assert len(f) - 7 <= P.TEXT_MAX
        assert all(b < 0x80 for b in f[1:-1])
    assert P.text("###") == []


def test_char_round_trip():
    m = P.parse(P.device_char("K"))
    assert P.chars_of(m) == "K"


def test_host_answers_hello_and_types():
    link = SimLink(lines=["cq de fm1"], out=io.StringIO())
    typer = PrintTyper(out=io.StringIO())
    host = Host(link, typer)
    for _ in range(3):
        host.pump(0.01)
    assert link.acked and host.connected
    assert host.firmware == (0, 4)
    assert typer.typed == "cq de fm1 "
    assert P.hello_ack() in link.sent


def test_host_upper_and_unknown_dropped():
    link = SimLink(lines=[], out=io.StringIO())
    typer = PrintTyper(out=io.StringIO())
    host = Host(link, typer, lower=False)
    host.on_frame(P.device_char("A"))
    host.on_frame(P.device_char(P.UNKNOWN))
    host.on_frame(P.device_char("B"))
    assert typer.typed == "AB"


def test_heartbeat_is_answered():
    link = SimLink(lines=[], out=io.StringIO())
    host = Host(link)
    link.sent.clear()
    host.on_frame(P.device_heartbeat())
    assert link.sent == [P.hello_ack()]


def test_send_text():
    out = io.StringIO()
    link = SimLink(lines=[], out=out)
    host = Host(link)
    assert host.send_text("cq cq") == 1
    host.stop()
    assert "[FM-1 plays] CQ CQ" in out.getvalue()
    assert link.sent[-1] == P.stop()


# ---- the progress backup (firmware 0.4)
from morse import progress as G  # noqa: E402


def test_progress_vectors_are_in_the_firmware_test():
    from pathlib import Path

    script = (Path(__file__).parents[2] / "firmware/morse/tests/basic.txt").read_text()
    for name, hexed in G.VECTORS.items():
        assert hexed in script, f"{name} not in basic.txt"


def test_progress_wire_round_trip():
    p = G.sample()
    assert len(G.blob(p)) == G.BLOB_N == 259
    asm = G.Assembler()
    got = None
    for f in G.frames(p, P.T_PROG):
        got = asm.feed(P.parse(f).payload)
    assert got == p
    assert G.unpack7(G.pack7(bytes(range(256)))) == bytes(range(256))


def test_progress_refuses_damage():
    fr = G.frames(G.sample(), P.T_PROG)
    bad = bytearray(fr[-1])
    bad[-3] ^= 1  # inside the CRC
    asm = G.Assembler()
    with pytest.raises(G.ProgressError):
        for f in fr[:-1] + [bytes(bad)]:
            asm.feed(P.parse(f).payload)
    with pytest.raises(G.ProgressError):
        G.Assembler().feed(P.parse(fr[1]).payload)  # out of turn


def test_progress_json_round_trip_and_checks():
    p = G.sample()
    assert G.from_json(G.to_json(p, (0, 4))) == p
    d = G.to_json(p).replace('"open": 15', '"open": 3')
    with pytest.raises(G.ProgressError):
        G.from_json(d)
    with pytest.raises(G.ProgressError):
        G.from_json('{"format": "something else"}')
    assert "15 open" in G.summary(p)


def test_backup_and_restore_against_the_simulator():
    link = SimLink(lines=[], out=io.StringIO(), beat=0.01)
    host = Host(link)
    host.pump(0.05)
    assert host.can_backup()
    assert host.backup(timeout=2) == G.Progress()
    assert host.restore(G.sample(), timeout=2, gap=0) == P.PD_LOADED
    assert link.progress == G.sample()
    assert host.backup(timeout=2) == G.sample()
    link.owner_says = P.PD_DECLINED
    p = G.sample()
    p.round = 9
    assert host.restore(p, timeout=2, gap=0) == P.PD_DECLINED
    assert link.progress.round == 7
