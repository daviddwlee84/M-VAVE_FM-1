# Merge PR #8, then DRUMS v0.1 as the next PR with a clickable fm1-emu preview

## Context
PR daviddwlee84/M-VAVE_FM-1#8 (backing-band / drum machine exploration, docs only) is done; the owner
asked to merge it and build DRUMS v0.1 from `docs/dev/drums.md` as a new PR whose page carries an
fm1-emu link to try it in the browser. The preview needs a real `.fwsc` (`just fw-preview` pushes it to
the `previews` branch; link format in `Justfile` fw-preview). This container can fetch the JieLi
toolchain (`pkgman.jieliapp.com` → aliyuncs, reachable) but the network blocks gitee.com, where the
pinned AC79 SDK lives; the owner chose to open gitee.com for this environment.
No device operation in this task (no flash, no M-Flasher).

## Steps
1. **Merge PR #8** — DONE (merge commit `619e1c8`). Next: restart `claude/laughing-cori-ktq3wc` from the
   new `origin/main` (its PR merged; only merged history on it).
2. **SDK files** (updated after the worker restart): the environment is already `unrestricted`
   (`get_environment`), yet gitee.com resets the connection → gitee itself blocks this egress; an
   environment change cannot fix it, so no `update_environment`.
   A `ble = false` build needs only three SDK files, pinned by SHA-256 in fm1-base `tools/build.py`
   `SDK_SHA256` (`uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`; `tools/setup_toolchain.sh` puts them in
   `.toolchain/ac79-sdk`). Plan: extract them from an fm1-base-built package already downloaded and
   digest-checked (JAMTRACK / CORD `.fwsc`), using fm1-base's own package tools (`tools/fm1pkg_make.py` format),
   and accept them **only if all three SHA-256 equal `SDK_SHA256`**; put them in `.toolchain/ac79-sdk/`
   (git-ignored by fm1-base) or `$AC79_SDK`. If that fails: ask the owner — a GitHub Actions workflow that
   builds and publishes the preview (runners reach gitee, as zvenson/fm1-build does), or build on the Mac.
3. **Tools**: `uv tool install rust-just` (or run recipe bodies directly); `apt-get install libraqm-dev`
   + Pillow build deps (Pillow is built from source, `pyproject.toml`); `just fw-setup` (fm1-base and
   morse submodules, `uv sync`; `python-rtmidi` needs `libasound2-dev`); `just fw-toolchain` (~1.2 GB,
   native on Linux x86-64, no Docker). Smoke test: `just fw-test hello`, `just fw-build hello`.
   Linux fm1-emu 0.5.1 (digest-checked earlier) into `tools/fm1-emu/` for `emu-shot`.
4. **Identity check**: re-read `thefirmwarehub.com/catalog.json`; use `FM-1_58` if still free.
5. **Write `firmware/drums/`** (GPL-3.0-or-later; transport adapted from `firmware/rhythm/app.c`):
   - `fm1.toml` (name/title DRUMS, `id_prefix = "FM-1_58"`, `include = ["../common"]`, `o2 = ["kit.c"]`,
     `ble = false`, Hub metadata like rhythm's), `app.h` (`APP_TICK`, `enum {OBJ_PANEL, OBJ_DRUMS, OBJ_NOBJ}`,
     `APP_STORE_MAP {0xDC000u, 0xD0000u}`, host expect/cmd hooks as in rhythm).
   - `groove.c/.h` — platform-free, no float, no 64-bit division: 24 ticks per beat; style tables (lanes ×
     16th/8th-triplet grid per bar, parsed from text strings at init: `"x..x..x."` with velocity digits);
     player state: section A/B, request queue (later replaces earlier), `FILL AT` = NOW (next 16th → bar end,
     plays the fill's tail) / BAR / PHRASE (last bar of a 4/8/16-bar phrase), AUTO FILL on MAIN change,
     transition (loop fill while held, switch at bar end after release), ending (one bar, then stop);
     `groove_tick(g, tick, out_events)`. Unit tests `firmware/drums/tests/unit.c` + recipe `just drums-unit`:
     fill pressed at every 16th under each mode, phrase 4/8/16, 3/4 and 6/8, A/B + fill queued together,
     transition hold/release, ending.
   - `kit.c` — synth voices (12-voice pool; LFSR noise as in `common/lpc.c`, one-pole/SVF filters,
     exponential envelopes, `fm_sinf`/`fm_exp2f` from fastmath, soft clip `fm_tanhf` as `theory/synth.c`):
     kick (sine pitch sweep), snare (tone + filtered noise), clap (noise bursts), rim, closed/open hat (choke),
     ride, crash, 3 toms, cowbell, shaker. Two kits as parameter tables: `808` and `STUDIO`. Per-lane level.
   - Styles (~10): Rock 8, Pop 16, Funk, Shuffle, Disco, Hip-hop, Bossa, Reggae one-drop, Waltz 3/4,
     6/8 ballad — each MAIN A, MAIN B, FILL A, FILL B; one shared ending.
   - `app.c` — transport (`tr`, `q8_mul`, `tick_at`, `beat_q8_at`, per-sample scheduler, `now_smp`) from
     rhythm; key timestamps in `app_tick` for pads (sample-accurate trigger); rec/dirty autosave (3 s) pattern;
     hold detection as `theory/app.c` (`btn_at`, `long_done`); `ledfx`.
     Controls v0.1: `PLAY` start / ending (hold = stop now); `REC` stopped = tap tempo, running = tap fill,
     hold transition, double-tap ending (tap resolves after ~300 ms); keys F3 / G3 = MAIN A / B, F#3 = fill,
     E4 = ending; F4–G5 = 15 pads; `SEL` + pad = mute lane; `SELECT` style, `PRESETS` kit, `K1` BPM, `K2` swing,
     `K3` FILL AT, `K4` phrase length; `OCT-/+` BPM ±1; `ARP` AUTO FILL. Screen: header (style, kit, BPM),
     big MAIN A/B, bar.beat, phrase bar with the fill bar marked, queued request, footer hints.
     LEDs: current MAIN lit, queued blinking, pads flash with their lane, PLAY green.
   - `tests/basic.txt`: boot shot, play, section/bar expects, each FILL AT, transition, double-tap ending,
     tap tempo, kit/style change, mute, audio peak checks, save → reboot → reload; shots for docs.
6. **Verify**: `just drums-unit`; `just fw-test drums`; `just fw-build drums` (size, RAM, no soft-double,
   no `__divdi3`); fm1-emu on `uc25hq80` and `p25q80`: boot shot, scripted `FM1_BENCH_KEYS` play + settings
   change + wait > 3 s, then reboot on the same `--flash` and check the setting reloaded, flash writes only at
   `0xD0000`/`0xD1000`; `--bench` CPU while all lanes play. No backup here → `emu-rehearse` listed for the owner.
7. **Docs**: `docs/dev/drums.md` status/controls/verification `[模擬]` with shots in `docs/assets/drums/`;
   `AGENTS.md` layout row + commands (`drums-unit`, `drums-emu`); `Justfile` recipes; `docs/hardware/flash-map.md`
   (`0xD0000` DRUMS); own-firmware tables in `README.md` / `docs/index.md` if they list apps;
   `docs/dev/index.md` note that `fw-build` runs natively on Linux x86-64. `just docs-check`.
8. **Commit, push, PR** (template: Try it / Screens / Before the unit / On the unit steps), then
   `just fw-preview drums pr-N` (pushes `previews`, owner asked for the link) and put the link in the PR body.

## Verification
- Unit + simulator tests pass; device build succeeds with size report; emulator boots, plays, saves and
  reloads on both chips; preview link opens the package in fm1-emu; docs-check passes.
