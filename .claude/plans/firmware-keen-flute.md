# METRONOME — standalone stage metronome firmware for the FM-1

## Context

The owner plays several songs in a row, each at a different BPM, and wants a metronome that keeps them as
presets on the piano keys. Research (2026-11-19):

- The Firmware Hub catalog (269 firmwares) has **no dedicated metronome firmware**. It is only a side feature
  elsewhere: h1 (built-in metronome), Frodhes (tap tempo), BEETLE BLUE (setlists in Live Mode, a drum tracker).
  A GitHub search for "fm1 metronome" found nothing.
- Our own RHYTHM (`firmware/rhythm/`) has a METRO page (meter, subdivisions, tap tempo, gap click, LAT) but no
  presets. Its click synth, sample-accurate scheduling and tap tempo are the parts to reuse.

Decisions made (2026-11-19): **a separate firmware** (not RHYTHM v0.3); each key slot holds a **full preset**;
v0.1 = **SETLIST + GRID + CLASSIC** with one screen theme (pendulum). Other themes (conductor, cute characters)
and more click sounds come later, after measuring how much flash they take.

## Identity, licence, storage

- `firmware/metronome/`, name `METRONOME`, GPL-3.0-or-later (copies RHYTHM's GPL click/tap code).
- Identity `FM-1_59` (dev `FM-1_5900`): free in the 2026-11-19 catalog; `docs/dev/jam.md` only listed it as free
  (JAM is on hold), so update that line. Recheck before a Hub listing.
- `APP_STORE_MAP {0xDC000u, 0xCE000u}`: 0xD0000–0xDE000 are all taken by own apps (flash-map). 0xCE000 is
  tentative: run `just emu-rehearse` over the latest backup before any flash and record it in
  `docs/hardware/flash-map.md`.

## Data model (one record, `rec_t`, magic/ver/size like RHYTHM's)

15 slots (one per key, `NKEYS`), each about 32 B:
`used, mode (GRID|CLASSIC), bpm (30–300), bpb (1–9), div (1, 2, 3 triplet, 4), steps[16] (2 bits: off / ghost /
normal / accent), mix[4] (CLASSIC volumes 0–7 for quarter, eighth, triplet, sixteenth), accent_on_1, sound,
name[8] ("SONG 3" by default)`. Plus globals: active slot, LAT, change-on (NOW / NEXT BAR). Saved 3 s after the
last change, through the store layer only.

## Pages (`HOME` cycles; the active slot drives all of them)

**SET (setlist)** — the main page.
- Not editing: keys with a stored slot glow (FX_GLOW), the active one is full (FX_LIT), empty ones are off.
  Tap a stored key: load it. While playing, the switch happens at the next bar (default) or at once.
- `REC` toggles EDIT. In EDIT: tap a key to pick it (creates an empty slot from the current settings),
  tap the same key repeatedly = tap tempo (last 4 taps), `SELECT` ±1 BPM, `OCT-/OCT+` ±5, `SEL`+key clears.
  The screen shows the 15-key strip with each slot's BPM.
- `PLAY` start/stop.

**GRID** — the rhythm pattern on the keys.
- One bar = `bpb × div` steps; a key = a step. Tap cycles off → ghost (dim) → normal (mid) → accent (lit):
  the LED brightness is the accent level. Playhead = blink on the current step.
- More than 15 steps (e.g. 4/4 in sixteenths): two halves, `OCT-/OCT+` flips. Uneven feels (dotted, shuffle)
  are drawn on the sixteenth or triplet grid; a swing knob is for later.
- K1 bpb, K2 div, K3 sound, K4 LAT.

**CLASSIC** — the RW200 workflow, own drawing (no TAMA name or artwork).
- K1–K4 = volume of quarter / eighth / triplet / sixteenth layers, mixed together as on the RW200.
- `SELECT` tempo, `OCT-/OCT+` BEAT (0–9; 0 = no accent), `REC` tap tempo, `PLAY` start/stop.
- Big seven-segment style BPM and beat digits, plus the pendulum.

Screen theme v0.1: a pendulum that swings once per beat, plus beat dots (accent bigger). The theme sits behind a
small interface (`theme_draw(phase, beat, accent)`) so later themes are drop-ins.

## Code layout (reuse first)

| File | Content | From |
|---|---|---|
| `metro.c/.h` | platform-free: tempo → samples per step, the step scheduler (GRID levels, CLASSIC layer mix into one event stream), tap tempo average, slot defaults/validation/clamping | scheduling and tap logic after `firmware/rhythm/app.c` (`app_render` L151, `tap_tempo` L411) |
| `app.c/.h` | pages, input, LEDs (`firmware/common/ledfx`: GLOW/MID/LIT already give three levels), render (click synth copied from RHYTHM), screen, save | `firmware/rhythm/app.c` structure (`lights()` L791, rec_t save) |
| `fm1.toml` | `include = ["../common"]`, identity | `firmware/rhythm/fm1.toml` |
| `tests/basic.txt` + host cmds | expects: slot store/load, tap tempo 4 taps → 120, GRID accents audible at the right samples, switch at the next bar, CLASSIC mix, settings survive restart | RHYTHM's `APP_HOST_EXPECT` / `APP_HOST_CMD` |
| `tests/unit.c` | `metro.c` unit tests (strict C99) | `just rhythm-unit` recipe |
| `LICENSE` | GPL-3.0 | `firmware/rhythm/LICENSE` |

Justfile: `metro-unit`, `metro-emu [CHIP]` (copy the rhythm ones).

## Docs (Traditional Chinese, per AGENTS.md)

- New `docs/dev/metronome.md`: the survey above (dated, `[上游]` catalog), controls table, data model, flash
  address, verification log; add it to `nav` in `zensical.toml`.
- `AGENTS.md` layout row + commands; `docs/hardware/flash-map.md` row for 0xCE000; `docs/dev/jam.md` identity
  note; the own-firmware tables in `README.md` / `docs/index.md` if RHYTHM/DRUMS are listed there.

## Verification

1. `just metro-unit` passes.
2. `just fw-test metronome`: all expects pass; screenshots of SET (normal and EDIT), GRID, CLASSIC.
3. `just fw-build metronome` (Docker/JieLi toolchain; if unavailable in the cloud, say so, and the owner runs it on the Mac).
4. `just metro-emu uc25hq80` and `p25q80`: store three slots, restart, reload, check they are still there and
   that only 0xCE000 is written. Report as `[模擬]`.
5. `just emu-rehearse` over the latest backup (owner's Mac) before any flash proposal.
6. `just docs-check`.
7. No flashing: an install is a separate, owner-approved write with full preflight.

## Progress (before the worker restart, 2026-11-19)

Done, not committed: `firmware/metronome/` (`metro.c/.h`, `app.c/.h`, `fm1.toml`, `words.tsv`, `LICENSE`,
`.gitignore`, `tests/metro_test.c`, `tests/basic.txt`). Changes from the plan, found while building:
the FM-1 has **27 keys** (16 white), so 16 songs on the white keys and GRID up to 16 steps on them, no halves;
black keys tap tempo on every page; `HOME` toggles SET and the song's own page, `SEL` there toggles GRID/CLASSIC;
`ALGO` is the sound (CLICK, WOOD, BEEP); no song names in v0.1 (no text entry).

- unit: 1430 checks pass. `fw-test metronome`: all expects pass (rerun after the last footer-text edit).
- `fw-build metronome`: ok, 616,003 B, identity `FM-1_5900`, app 86,976 B (SDK files taken from JAMTRACK 0.2.3,
  hashes matched `SDK_SHA256`).
- Next: fm1-emu on both chips. The Justfile fetches the macOS build; clone `joakimeriksson/fm1-emu` (public, read
  available) to find a Linux binary or build `fm1-live` with cargo, then save/restart/reload on `uc25hq80` and
  `p25q80`, checking only 0xCE000 is written. Then Justfile recipes, docs, commit, push, PR, `fw-preview`.

## Delivery (owner asked 2026-11-19: build v0.1, open a PR, give a preview link)

1. Commit on `claude/practical-shannon-vf3uvq`, push, open a PR following `.github/pull_request_template.md`
   (Try it / Screens / Before the unit).
2. Preview link, per AGENTS.md: `just fw-build metronome`, then `just fw-preview metronome pr-N` (pushes the
   `.fwsc` to the `previews` branch, prints the fm1-emu link). If the JieLi toolchain cannot run in this cloud
   container (gitee.com unreachable → try `just fw-sdk-from PKG`), fall back to `just fw-web metronome`
   (WebAssembly host build) and say in the PR which one the link is.
3. PR body: the link, simulator screenshots, and a step list for the owner to try on the unit (each with what
   should happen), marked not yet flashed.
