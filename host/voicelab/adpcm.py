"""IMA ADPCM (4 bits a sample), the plainest "store the recording" codec: encode, decode, and its size.

A decoder on the FM-1 is a few dozen lines and no RAM to speak of. One block = a 4-byte header (predictor, step
index) then the nibbles, as IMA/DVI ADPCM in WAV does; BLOCK samples a block.
"""

from __future__ import annotations

INDEX = (-1, -1, -1, -1, 2, 4, 6, 8)
STEPS = (
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871,
    5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
    27086, 29794, 32767,
)
BLOCK = 505  # samples a block: 1 in the header + 504 nibbles (256-byte blocks, as WAV's IMA ADPCM at 8-22 kHz)


def _clamp(v: int, lo: int, hi: int) -> int:
    return lo if v < lo else hi if v > hi else v


def _step(code: int, pred: int, idx: int) -> tuple[int, int]:
    step = STEPS[idx]
    diff = step >> 3
    if code & 4:
        diff += step
    if code & 2:
        diff += step >> 1
    if code & 1:
        diff += step >> 2
    pred = _clamp(pred - diff if code & 8 else pred + diff, -32768, 32767)
    return pred, _clamp(idx + INDEX[code & 7], 0, 88)


def encode(pcm: list[int]) -> bytes:
    out = bytearray()
    for b in range(0, len(pcm), BLOCK):
        blk = pcm[b:b + BLOCK]
        pred, idx = blk[0], 0
        out += int(pred).to_bytes(2, "little", signed=True) + bytes((idx, 0))
        nib: list[int] = []
        for s in blk[1:]:
            step, diff, code = STEPS[idx], s - pred, 0
            if diff < 0:
                code, diff = 8, -diff
            if diff >= step:
                code |= 4
                diff -= step
            if diff >= step >> 1:
                code |= 2
                diff -= step >> 1
            if diff >= step >> 2:
                code |= 1
            pred, idx = _step(code, pred, idx)
            nib.append(code)
        if len(nib) % 2:
            nib.append(0)
        out += bytes(nib[i] | (nib[i + 1] << 4) for i in range(0, len(nib), 2))
    return bytes(out)


def decode(data: bytes, n: int) -> list[int]:
    """n samples back from encode()'s bytes."""
    out: list[int] = []
    blk_bytes = 4 + (BLOCK - 1 + 1) // 2
    for b in range(0, len(data), blk_bytes):
        blk = data[b:b + blk_bytes]
        pred, idx = int.from_bytes(blk[0:2], "little", signed=True), blk[2]
        out.append(pred)
        for byte in blk[4:]:
            for code in (byte & 15, byte >> 4):
                if len(out) >= n:
                    return out
                pred, idx = _step(code, pred, idx)
                out.append(pred)
    return out[:n]
