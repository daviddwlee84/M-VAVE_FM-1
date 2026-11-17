"""Whisper as a stand-in listener: what a speech recogniser hears in each rendering, and whether it is the word.

A rough, repeatable intelligibility number to put beside your own ears (Speak37 chose its 10 ms LPC frames the same
way). Needs faster-whisper (`just voice-score` adds it); the model downloads once into the Hugging Face cache.
Isolated words are hard for any recogniser, so compare engines with each other, not with 100 %.
"""

from __future__ import annotations

import json
import re

from .lab import OUT, TMP, resample
from . import wav

MODEL = "base.en"


NUMBERS = {"0": "zero", "1": "one", "2": "two", "3": "three", "4": "four", "5": "five", "6": "six", "7": "seven",
           "8": "eight", "9": "nine", "10": "ten"}


def tokens(s: str) -> list[str]:
    return [NUMBERS.get(t, t) for t in re.sub(r"[^a-z0-9' ]+", " ", s.lower()).split()]


def heard_ok(target: str, heard: str) -> bool:
    """The target's words appear, in order and together, in what was heard ("3." is three, "the world." has world)."""
    t, h = tokens(target), tokens(heard)
    return any(h[i:i + len(t)] == t for i in range(len(h) - len(t) + 1)) if t else False


def _audio16k(path):
    """The rendering at 16 kHz as float32, decoded here (ffmpeg + wave): faster-whisper's own decoder (PyAV) is
    version-sensitive."""
    import numpy as np
    pcm, _ = wav.read(resample(path, TMP / "asr16k.wav", 16000))
    return np.asarray(pcm, dtype=np.float32) / 32768.0


def score() -> dict:
    from faster_whisper import WhisperModel  # optional dependency

    res = json.loads((OUT / "results.json").read_text())
    model = WhisperModel(MODEL, device="cpu", compute_type="int8")
    out: dict[str, dict] = {}
    for e in res["engines"]:
        hits = total = 0
        for sl, it in e["items"].items():
            if "wav" not in it:
                continue
            segs, _ = model.transcribe(_audio16k(OUT / it["wav"]), language="en", beam_size=5,
                                       condition_on_previous_text=False, vad_filter=False)
            heard = " ".join(s.text for s in segs).strip()
            ok = heard_ok(it["text"], heard)
            out.setdefault(e["key"], {})[sl] = {"heard": heard, "ok": ok}
            hits += ok
            total += 1
        print(f"{e['key']:10s} {hits}/{total}")
    (OUT / "scores.json").write_text(json.dumps({"model": MODEL, "scores": out}, ensure_ascii=False, indent=1))
    return out
