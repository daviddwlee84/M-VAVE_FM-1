# SPDX-License-Identifier: MIT
"""DRUMS' RUSTY kit: a few one-shots of Karoryfer's Big Rusty Drums (CC0-1.0), made small enough for the FM-1.

Fetches the pinned commit of github.com/sfzinstruments/karoryfer.big-rusty-drums (only the files below, into
build/drums-samples/), and for each lane: mixes the close mic with some of the overheads, starts at the hit, cuts it
short with a fade, normalises, resamples to 22,050 Hz mono and encodes IMA ADPCM in firmware/common/adpcm's block
format. Writes firmware/drums/samples.h with every source file's SHA-256.

  uv run --no-project --with soundfile --with numpy --with scipy python host/drums/make_samples.py
"""
from __future__ import annotations

import hashlib
import subprocess
import sys
from pathlib import Path

import numpy as np
import soundfile as sf
from scipy.signal import resample_poly

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "host"))
from voicelab import adpcm  # noqa: E402

REPO = "https://github.com/sfzinstruments/karoryfer.big-rusty-drums"
COMMIT = "f07ce00df34a46b6b08375be56fe116cf15782bc"
CACHE = ROOT / "build" / "drums-samples" / "rusty"
OUT = ROOT / "firmware" / "drums" / "samples.h"
RATE = 22050

# lane (groove.h order), the close mic's file, how long to keep (s), how much of the overheads
LANES = [
    ("KICK", "Samples/kick_24/kick/kick/k_vl12_rr1.flac", 0.45, 0.35),
    ("SNARE", "Samples/snare_14/center/top/sn_center_vl8_rr1.flac", 0.40, 0.45),
    ("RIM", "Samples/snare_14/sidestick/top/sn_ss_vl3_rr1.flac", 0.16, 0.30),
    ("CLAP", None, 0, 0),
    ("HAT", "Samples/hihat_14/tc/cl/ht_tc_vl6_rr1.flac", 0.20, 0.30),
    ("OPEN HAT", "Samples/hihat_14/open/cl/ht_open_vl5_rr1.flac", 0.80, 0.30),
    ("RIDE", "Samples/ride_22/rd/cl/rd_vl8_rr1.flac", 1.00, 0.30),
    ("CRASH", "Samples/crash_17/cr/cl/cr_vl4_rr1.flac", 1.30, 0.30),
    ("TOM HI", "Samples/tom_14/center/cl/t14_vl5_rr1.flac", 0.55, 0.35),
    ("TOM MID", "Samples/tom_18/center/cl/t18_vl6_rr1.flac", 0.60, 0.35),
    ("TOM LO", "Samples/tom_22/center/cl/t22_vl6_rr1.flac", 0.70, 0.35),
    ("COWBELL", None, 0, 0),
    ("SHAKER", None, 0, 0),
]


def overhead(path: str) -> str | None:
    """the same hit's overhead file: the mic folder (kick, top, cl) is 'oh'"""
    parts = path.split("/")
    parts[-2] = "oh"
    return "/".join(parts)


def fetch(paths: list[str]) -> None:
    if not (CACHE / ".git").exists():
        CACHE.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(["git", "clone", "-q", "--filter=blob:none", "--no-checkout", REPO, str(CACHE)], check=True)
    subprocess.run(["git", "-C", str(CACHE), "checkout", "-q", COMMIT, "--", *paths], check=True)


def load(path: Path) -> tuple[np.ndarray, int]:
    x, sr = sf.read(path, dtype="float32", always_2d=True)
    return x.mean(axis=1), sr


def one(close: str, keep: float, oh_mix: float) -> tuple[np.ndarray, list[tuple[str, str]]]:
    x, sr = load(CACHE / close)
    used = [(close, hashlib.sha256((CACHE / close).read_bytes()).hexdigest())]
    oh = overhead(close)
    if oh and (CACHE / oh).exists():
        y, sr2 = load(CACHE / oh)
        if sr2 == sr:
            n = min(len(x), len(y))
            x = x[:n] + oh_mix * y[:n]
            used.append((oh, hashlib.sha256((CACHE / oh).read_bytes()).hexdigest()))
    peak = np.max(np.abs(x))
    start = int(np.argmax(np.abs(x) > peak * 0.02))   # the hit: the first sample over -34 dB of the peak
    start = max(0, start - int(0.001 * sr))
    x = x[start:start + int(keep * sr)]
    fade = int(len(x) * 0.35)                          # the last third fades out
    x[-fade:] *= np.linspace(1.0, 0.0, fade) ** 2
    x = resample_poly(x, RATE, sr)
    x = x / np.max(np.abs(x)) * 0.9
    return x, used


def main() -> None:
    files = []
    for _, close, _, _ in LANES:
        if close:
            files += [close, overhead(close)]
    fetch(files)
    out = [
        "/* SPDX-License-Identifier: CC0-1.0 */",
        "/* DRUMS' RUSTY kit: one-shots from Karoryfer's Big Rusty Drums (CC0-1.0, Karoryfer Samples),",
        f" * {REPO} at {COMMIT}.",
        f" * Made by host/drums/make_samples.py: close mic plus some overheads, cut short, {RATE} Hz mono, IMA ADPCM",
        " * (firmware/common/adpcm's blocks). Generated; do not edit. The sources and their SHA-256: */",
    ]
    table, total = [], 0
    for i, (name, close, keep, oh_mix) in enumerate(LANES):
        if not close:
            table.append("    { 0, 0, 0 },")
            continue
        x, used = one(close, keep, oh_mix)
        pcm = [int(v) for v in np.clip(np.round(x * 32767), -32768, 32767)]
        data = adpcm.encode(pcm)
        total += len(data)
        for path, sha in used:
            out.append(f"/*   {sha}  {path} */")
        sym = f"SMP_{i}"
        rows = [", ".join(f"0x{b:02x}" for b in data[k:k + 24]) for k in range(0, len(data), 24)]
        out.append(f"static const uint8_t {sym}[{len(data)}] = {{  /* {name}: {len(pcm)} samples */")
        out += ["    " + r + "," for r in rows]
        out.append("};")
        table.append(f"    {{ {sym}, {len(data)}u, {len(pcm)}u }},")
        print(f"{name:9s} {len(pcm) / RATE:5.2f} s  {len(data):6d} B")
    out.append("static const struct { const uint8_t *p; uint32_t bytes, samples; } SMP[GV_LANES] = {")
    out += table
    out.append("};")
    out.append(f"#define SMP_RATE {RATE}u")
    out.append(f"#define SMP_BYTES {total}u")
    OUT.write_text("\n".join(out) + "\n")
    print(f"total {total} B -> {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
