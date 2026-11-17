"""Mono 16-bit WAV in and out (the standard library's wave), and raw s16le."""

from __future__ import annotations

import array
import wave
from pathlib import Path


def read(path: Path) -> tuple[list[int], int]:
    with wave.open(str(path), "rb") as w:
        if w.getsampwidth() != 2:
            raise ValueError(f"{path}: not 16-bit")
        ch, rate = w.getnchannels(), w.getframerate()
        a = array.array("h", w.readframes(w.getnframes()))
    return list(a[::ch]), rate


def write(path: Path, pcm: list[int], rate: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(array.array("h", pcm).tobytes())


def raw_to_wav(raw: Path, wav_path: Path, rate: int, channels: int = 1) -> None:
    """s16le samples (channels interleaved; the first one kept) as a mono WAV."""
    a = array.array("h", raw.read_bytes())
    write(wav_path, list(a[::channels]), rate)
