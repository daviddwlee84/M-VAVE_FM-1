import pytest

from conductor import protocol as P


@pytest.mark.parametrize("name", sorted(P.VECTORS))
def test_shared_vectors(name):
    frame, hexed = P.VECTORS[name]
    assert P.hexs(frame) == hexed


def test_action_round_trip():
    m = P.parse(P.device_action(P.OP_DENY, 26, 0x3FFF))
    a = P.parse_action(m)
    assert (a.op, a.slot, a.tag, a.name) == (P.OP_DENY, 26, 0x3FFF, "deny")


def test_not_ours_and_wrong_version():
    assert P.parse(bytes([0xF0, 0x7D, 0x4C, 0x47, 0xF7])) is None  # fm1-base's own log frame
    with pytest.raises(P.ProtocolError):
        P.parse(bytes([0xF0, 0x7D, 0x43, 0x4E, 0x09, 0x41, 0xF7]))


def test_text_is_7bit_and_clipped():
    f = P.slot(1, P.S_IDLE, P.K_CODEX, 0, 1, 0, "naïve-" + "x" * 40)
    assert all(b < 0x80 for b in f[1:-1])
    assert len(f) == 1 + 3 + 2 + 8 + P.NAME_MAX + 1
    assert b"na?ve-" in f


def test_slot_range():
    with pytest.raises(P.ProtocolError):
        P.slot(27, P.S_IDLE, 0, 0, 0, 0, "x")
