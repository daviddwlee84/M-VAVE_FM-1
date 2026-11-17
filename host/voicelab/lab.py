"""Render every test word with every speech option into build/voicelab/<engine>/<slug>.wav.

Engines whose tools are missing are skipped with a note (`just voice-vendor` fetches and builds them). The reference
voice (macOS `say`, Samantha) is also the source the LPC and ADPCM options encode: it is fine for listening on this
Mac, but a deck to share needs a voice whose output may be redistributed (Kokoro, Apache-2.0).
"""

from __future__ import annotations

import json
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

from . import adpcm, arpabet, wav

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/voicelab"
BIN = OUT / "bin"
TMP = OUT / "tmp"
VENDOR = ROOT / "vendor"
HERE = Path(__file__).resolve().parent
MOONSHINE = VENDOR / "moonshine/micro"
WIZARD = VENDOR / "python_wizard/python_wizard"
DX386 = VENDOR / "386dx-engine"
CMUDICT = VENDOR / "cmudict/cmudict.dict"
EMU = ROOT / "tools/fm1-emu/fm1-live"
SPELLBOX = ROOT / "downloads/spellbox-0.3.fwsc"
SAY_VOICE = "Samantha"


@dataclass
class Engine:
    key: str
    name: str
    where: str
    licence: str
    shared: str = ""            # data every word shares (a dictionary, a voice), in words
    shared_kb: float = 0.0
    size_note: str = ""         # what the per-word bytes count
    items: dict[str, dict] = field(default_factory=dict)   # slug -> {"wav": rel path, "bytes": int | None}
    skipped: str = ""


ENGINES = [
    Engine("say", "macOS say（Samantha）", "電腦：連線時用 USB audio 從 FM-1 喇叭播", "Apple（只能自用）",
           size_note="不存進機器"),
    Engine("klatt-ipa", "Klatt 共振峰（Moonshine）＋ CMUdict 音素", "機器上合成；音素由電腦先查好", "MIT（CMUdict BSD）",
           size_note="一個音素 1 byte"),
    Engine("klatt-g2p", "Klatt 共振峰（Moonshine）＋ 它自己的 G2P", "機器上：直接念拼字", "MIT",
           shared="G2P 字典", shared_kb=150, size_note="只存拼字"),
    Engine("dx386", "386DX 引擎：eSpeak NG 前端＋Klatt", "機器上：任意英文", "GPL-3.0",
           shared="eSpeak 英文資料", shared_kb=305, size_note="只存拼字"),
    Engine("lpc25", "LPC，25 ms 一幀（TMS5220 原規格）", "電腦先編碼（python_wizard），機器解碼（firmware/common/lpc.c）",
           "解碼器 MIT", size_note="LPC 位元流"),
    Engine("lpc20", "LPC，20 ms 一幀", "同上（python_wizard 在 10 ms 反而更差，見 docs/dev/tts.md）", "解碼器 MIT",
           size_note="LPC 位元流"),
    Engine("adpcm11", "IMA ADPCM 11 kHz（預錄）", "電腦先編碼，機器解碼", "免費", size_note="4 bit 一個樣本"),
    Engine("adpcm16", "IMA ADPCM 16 kHz（預錄）", "同上", "免費", size_note="4 bit 一個樣本"),
    Engine("spellbox", "SPELLBOX 的 SPEAK（fm1-emu）", "FM-1 韌體本身：12k 字典＋規則 → LPC", "GPL-3.0",
           shared="字典＋音素庫", size_note="（韌體內建）"),
]


def slug(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


def load_words(path: Path = HERE / "words.txt") -> list[str]:
    return [ln.strip() for ln in path.read_text().splitlines() if ln.strip() and not ln.lstrip().startswith("#")]


def run(cmd: list, **kw) -> subprocess.CompletedProcess:
    return subprocess.run([str(c) for c in cmd], check=True, capture_output=True, text=True, **kw)


# ---- tools -----------------------------------------------------------------------------------------------------

def build_tools() -> dict[str, str]:
    """Compile the renderers this lab needs into build/voicelab/bin -> {engine key: why it is skipped}."""
    BIN.mkdir(parents=True, exist_ok=True)
    skip: dict[str, str] = {}
    run(["cc", "-O2", "-Wall", "-Wextra", "-I", ROOT / "firmware/common", HERE / "lpc_decode.c", "-o", BIN / "lpc_decode"])
    if (MOONSHINE / "klatt-tts").is_dir():
        k, g = MOONSHINE / "klatt-tts", MOONSHINE / "g2p"
        srcs = sorted((k / "src").glob("*.cc")) + sorted((g / "src").glob("*.cc"))
        inc = [f"-I{p}" for p in (k / "include", k / "include/tts", k / "src", g / "include", g / "include/g2p", g / "src")]
        run(["clang++", "-std=c++17", "-O2", *inc, *srcs, HERE / "klatt_render.cpp", "-o", BIN / "klatt_render"])
    else:
        skip["klatt-ipa"] = skip["klatt-g2p"] = "vendor/moonshine 不在（just voice-vendor）"
    if not CMUDICT.exists():
        skip["klatt-ipa"] = "vendor/cmudict 不在（just voice-vendor）"
    objs = sorted((DX386 / "build/obj").glob("*.o"))
    if objs:
        run(["cc", "-O2", "-DDX_HOST=1", f"-I{DX386 / 'src'}", "-Wno-everything", HERE / "dx386_say.c", *objs, "-lm",
             "-o", BIN / "dx386_say"])
    else:
        skip["dx386"] = "vendor/386dx-engine 還沒 build（just voice-vendor）"
    if not WIZARD.exists():
        skip["lpc25"] = skip["lpc10"] = "vendor/python_wizard 不在（just voice-vendor）"
    if not (EMU.exists() and SPELLBOX.exists()):
        skip["spellbox"] = "需要 fm1-emu（just emu-fetch）與 SPELLBOX（just voice-fetch）"
    return skip


# ---- engines ---------------------------------------------------------------------------------------------------

def ref_aiff(text: str) -> Path:
    TMP.mkdir(parents=True, exist_ok=True)
    p = TMP / f"{slug(text)}.aiff"
    if not p.exists():
        run(["say", "-v", SAY_VOICE, "-o", p, text])
    return p


def resample(src: Path, dst: Path, rate: int) -> Path:
    dst.parent.mkdir(parents=True, exist_ok=True)
    run(["ffmpeg", "-loglevel", "error", "-y", "-i", src, "-ar", str(rate), "-ac", "1", "-c:a", "pcm_s16le", dst])
    return dst


def r_say(text: str, out: Path) -> int | None:
    resample(ref_aiff(text), out, 22050)
    return None


def r_klatt_ipa(text: str, out: Path, cmu: dict) -> int | None:
    ipa, missing = arpabet.line_ipa(text, cmu)
    if missing:
        raise RuntimeError(f"CMUdict has no {missing}")
    out.parent.mkdir(parents=True, exist_ok=True)
    run([BIN / "klatt_render", "ipa", ipa, out])
    return arpabet.phone_count(text, cmu) + max(0, len(text.split()) - 1)


def r_klatt_g2p(text: str, out: Path) -> int | None:
    out.parent.mkdir(parents=True, exist_ok=True)
    run([BIN / "klatt_render", "text", text, out])
    return len(text)


def r_dx386(text: str, out: Path) -> int | None:
    out.parent.mkdir(parents=True, exist_ok=True)
    run([BIN / "dx386_say", text, out])
    return len(text)


# python_wizard settings that Whisper scored best on the test words (2026-11-18): pre-emphasis on (its command line
# turns the settings' default off), a 3-frame analysis window for frames shorter than 25 ms.
LPC_FLAGS = {25: ["-p"], 20: ["-p", "-w", "3"]}


def r_lpc(text: str, out: Path, frame_ms: int) -> int | None:
    w8 = resample(ref_aiff(text), TMP / f"{slug(text)}-8k.wav", 8000)
    hexs = run([sys.executable, WIZARD, "-T", "tms5220", "-F", str(frame_ms), "-f", "hex",
                *LPC_FLAGS.get(frame_ms, ["-p"]), w8]).stdout
    data = bytes.fromhex(hexs)
    lpc_bin, raw = TMP / f"{slug(text)}-{frame_ms}.lpc", TMP / f"{slug(text)}-{frame_ms}.raw"
    lpc_bin.write_bytes(data)
    run([BIN / "lpc_decode", lpc_bin, frame_ms, raw])
    wav.raw_to_wav(raw, out, 8000)
    return len(data)


def r_adpcm(text: str, out: Path, rate: int) -> int | None:
    src = resample(ref_aiff(text), TMP / f"{slug(text)}-{rate}.wav", rate)
    pcm, _ = wav.read(src)
    enc = adpcm.encode(pcm)
    wav.write(out, adpcm.decode(enc, len(pcm)), rate)
    return len(enc)


# SPELLBOX in the emulator: PRESET (encoder 6) +7 = SPEAK, the word typed on the alphabetical keys (F3 = A ...
# F#5 = Z), PLAY speaks it. The audio from PLAY on is the word.
_NOTES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
EMU_RATE = 44118


def r_spellbox(text: str, out: Path) -> int | None:
    letters = [c for c in text.lower() if c.isalpha()]
    if len(letters) != len(text) or len(letters) > 16:
        raise RuntimeError("SPEAK 只測單字（句子要用 OCT+ 當空白，這裡不測）")
    ev, t = ["3000:e6:+7"], 3500
    for c in letters:
        m = 53 + ord(c) - ord("a")
        k = f"{_NOTES[m % 12]}{m // 12 - 1}"
        ev += [f"{t}:{k}@100", f"{t + 80}:{k}^"]
        t += 200
    play = t + 1200
    ev += [f"{play}:PLAY", f"{play + 100}:PLAY^"]
    work = TMP / "emu" / slug(text)
    rec = work / "rec"
    if work.exists():
        for p in sorted(work.rglob("*"), reverse=True):
            p.unlink() if p.is_file() else p.rmdir()
    (work / "work").mkdir(parents=True)
    env = {"FM1_BENCH_KEYS": ",".join(ev), "PATH": "/usr/bin:/bin"}
    subprocess.run([str(EMU), "--fwsc", str(SPELLBOX), "--flash-chip", "uc25hq80", "--flash", str(work / "x.flash"),
                    "--midi", "none", "--shot", str(play + 3500), "--record", str(rec), "--lcd-only"],
                   cwd=work, env=env, check=True, capture_output=True)
    import array
    a = array.array("h", (rec / "audio.raw").read_bytes())
    mono = list(a[::2])[play * EMU_RATE // 1000:]
    # trim the silence after the word
    end = len(mono)
    while end > 0 and abs(mono[end - 1]) < 200:
        end -= 1
    wav.write(out, mono[:end + EMU_RATE // 10], EMU_RATE)
    return None


# ---- the lab ---------------------------------------------------------------------------------------------------

def build(only: set[str] | None = None) -> list[Engine]:
    skip = build_tools()
    cmu = arpabet.load(CMUDICT) if CMUDICT.exists() else {}
    words = load_words()
    renderers = {
        "say": r_say,
        "klatt-ipa": lambda t, o: r_klatt_ipa(t, o, cmu),
        "klatt-g2p": r_klatt_g2p,
        "dx386": r_dx386,
        "lpc25": lambda t, o: r_lpc(t, o, 25),
        "lpc20": lambda t, o: r_lpc(t, o, 20),
        "adpcm11": lambda t, o: r_adpcm(t, o, 11025),
        "adpcm16": lambda t, o: r_adpcm(t, o, 16000),
        "spellbox": r_spellbox,
    }
    engines = [e for e in ENGINES if not only or e.key in only]
    for e in engines:
        if e.key in skip:
            e.skipped = skip[e.key]
            print(f"skip {e.key}: {e.skipped}")
            continue
        print(f"{e.key} ...", flush=True)
        for text in words:
            out = OUT / e.key / f"{slug(text)}.wav"
            try:
                n = renderers[e.key](text, out)
                e.items[slug(text)] = {"text": text, "wav": f"{e.key}/{slug(text)}.wav", "bytes": n}
            except (RuntimeError, subprocess.CalledProcessError) as err:
                msg = getattr(err, "stderr", None) or str(err)
                e.items[slug(text)] = {"text": text, "error": str(msg).strip()[:200]}
    done = {e.key: e.__dict__ for e in engines}
    prev = OUT / "results.json"
    if only and prev.exists():   # a partial run keeps the other engines' results
        old = {e["key"]: e for e in json.loads(prev.read_text())["engines"]}
        done = {e.key: done.get(e.key) or old.get(e.key) or e.__dict__ for e in ENGINES if e.key in done or e.key in old}
    prev.write_text(json.dumps({"words": words, "engines": list(done.values())}, ensure_ascii=False, indent=1))
    return engines
