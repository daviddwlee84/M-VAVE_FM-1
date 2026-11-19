# JAM firmware — research page + own arranger design

## Context
Owner wants "pick a key, pick a groove, start playing": built-in chord progressions,
drum patterns, several solo voices, and PSR-style arranger control. MODX only gets to
"switch pattern every 8 bars"; the owner wants a fill at the end of a part, MAIN A-D,
INTRO/ENDING, sync start + chord recognition, and **per-section chord progressions**
("that alone would let me play a lot of songs"). Decision (2026-11-19): do both —
document and try the existing firmwares, and draft our own design.

## Research findings (Firmware Hub catalog.json + upstream READMEs, 2026-11-19)
- **JAMTRACK** (Philip-Bos/jamtrack-fm1 0.2.3, MIT, chip new): 160 jams / 16 genres,
  Nashville-number progressions, key + major/minor, 28 grooves, bass / comping / synth line,
  part A (verse) / B (chorus); SEQ switches part "at the end of the round with a fill",
  hold SEQ = fill now; 3 mono solo leads with glide on the keys; key LEDs show chord and
  scale tones; MIDI clock in, loop-pedal CCs. Closest to the idea; no INTRO/ENDING, 2 parts.
- **Polly** (charlesvestal/fm1-polly 0.6.4, GPL-3.0, chip new): QY70-style arranger, 16 styles
  (+128 with own QY70 ROM), Groove A/B, Intro / Fill / Ending "from the next bar" (HOME + black
  keys 1-5), chord follow, 16 progressions, song = sections, GM sounds (Sonivox EAS), GM
  module over MIDI in. Closest to PSR's model; no auto fill / end-of-section fill documented.
- **CORD** (chasedeanda/cord-fm1 1.0.3, GPL-3.0): 130 editable progression ideas, grooves,
  one-bar fill queue, drop/back on next bar; no dedicated lead voice.
- Also relevant: Pocket Band, PERF-1 (Jam page), HERA (J-6 chord sets), COQUÍ, Blop, MIN-1.
- PSR model (MAIN A-D, AUTO FILL to bar end on MAIN change, BREAK, INTRO/ENDING A-C,
  SYNC START, ACMP chord fingering): from memory → tag `[調研]` until checked against a
  Yamaha manual (fetch one during implementation and upgrade to `[上游]` if it confirms).

Gap none of them fills documented: **fill scheduled at the end of the current section
(N bars)**, MAIN A-D, and per-section progressions together with INTRO/ENDING.

## Repo facts the design relies on (from exploration)
- fm1-base has no synth; apps render 44.1 kHz stereo int32 in `app_render`, 256-frame
  blocks, optional 1 kHz `app_tick` (`APP_TICK`).
- Reuse: `firmware/theory/theory.c` (`th_prog_parse`, `th_diatonic`, `th_voice_near`,
  `th_identify` for chord recognition), `firmware/theory/synth.c` (8-voice FM EP + score
  player), `firmware/rhythm/app.c` sample-accurate transport (`tick_at`, `q8_mul`, 12 TPB,
  tap tempo), `firmware/common/ledfx`, `chime`. Reuse makes JAM GPL-3.0-or-later.
- No drum engine exists here: needs a new synthesized kit (FM/noise) or ADPCM (`common/adpcm`).
- Free store slot: `0xD0000` (next below RHYTHM `0xD2000`); verify blank with
  `just emu-rehearse`. Identity: pick a free `FM-1_` prefix after checking catalog.json
  (`FM-1_52` is taken by JAMTRACK et al.; avoid prefix collisions as with 13/53).
- Name: not "JAMTRACK". Working name **JAM**; final name decided in the design page.
- `docs/dev/theory.md` already plans **GROOVE** (v0.6: FM-1 plays bass and drums under a
  progression) and a progression catalogue (Roman numerals TSV, 2 B/chord). JAM supersedes it:
  move that plan to JAM and leave a pointer in theory.md.
- MIDI: `plat_midi_send` USB/BLE out, `plat_midi_in` (USB/BLE/TRS in, clock arrives). Driving a
  MODX needs a USB host in between; TRS is IN only.

## Work in this task
1. **`docs/firmware/jam.md`** (zh-TW): "伴奏 / arranger 類韌體" comparison — JAMTRACK, Polly, CORD
   (+ short list of others) with a feature table (progressions, sections, fill timing, MAIN
   variations, intro/ending, chord follow, lead voices, licence, chip), `[上游]` tags and links;
   a PSR concept box (`[調研]`); "which to try first" → JAMTRACK (lead + fill), Polly (sections).
   Add to `nav` in `zensical.toml`; link from `docs/firmware/index.md`. Add the three to the
   version table in `docs/resources.md` (date 2026-11-19).
2. **Fetch + emulator trial**: add JAMTRACK, Polly, CORD to `just fm1-fetch-more` (release
   asset digest verify, FM-1 package not SMK-37), `just emu-shot` each on `uc25hq80` and `p25q80`,
   record `[模擬]` results on the page. `just emu-rehearse` needs a backup in `backups/`, which this
   cloud container does not have → list it as a step for the owner on the Mac, plus add each to
   `docs/firmware/switching.md` only after rehearsal.
3. **`docs/dev/jam.md`** design page (layout like `rhythm.md` / `guitar.md`: status, controls,
   features, code structure, flash, roadmap, open questions). Contents:
   - **Model**: Song = INTRO → sections (each: progression in Roman numerals, bar count, MAIN A-D)
     → ENDING. Style = per-MAIN drum/bass/comp patterns written relative to the chord (C-relative,
     re-pitched by root/quality from `theory.c`), plus FILL per MAIN, INTRO, ENDING patterns.
   - **Fill timing**: (a) FILL-AT-END: press anytime → fill plays in the last bar of the current
     section, then the next section/MAIN; (b) FILL-NOW: rest of current bar (PSR); (c) AUTO FILL on
     MAIN change. Quantized to bar boundaries by the transport.
   - **Sync start**, **chord follow**: split point; lower keys → `th_identify`, one-finger mode;
     upper keys → solo voice. Key LEDs show chord tones / scale (ledfx).
   - **Voices**: drums (new `drum.c`: kick/snare/hat/clap/tom, FM + noise), bass (mono), comp
     (`synth.c` EP), solo (mono lead w/ glide, square, organ, EP). Separate voice pools; CPU check in
     emulator.
   - **Controls** draft on the FM-1 (PLAY start/stop, REC tap tempo / sync start, black keys or
     K1-K4 for MAIN A-D, FILL/BREAK/INTRO/ENDING, SELECT style, ALGO voice, HOME pages).
   - **Content pipeline**: progressions + styles as text tables → generated C via fm1-base `gen`.
   - **Roadmap**: v0.1 key + style + preset progression + MAIN A/B + FILL-AT-END/NOW + one lead;
     v0.2 song of sections (own progression each) + INTRO/ENDING + MAIN C/D + AUTO FILL;
     v0.3 sync start + chord follow; v0.4 MIDI out (parts on channels, clock) for a DAW/MODX.
   - Update `docs/dev/theory.md` GROOVE entry to point at JAM; `docs/dev/index.md` list; nav.
4. `just docs-check`; commit on `claude/laughing-cori-ktq3wc` and push (owner asked for this work;
   confirm before push if not).

No firmware code in this task, no device operations.

## Verification
- `just docs-check` passes (strict build + site checker).
- Downloads: SHA-256 equals release asset digest, printed in the page.
- `just emu-shot` images show the FM-1 board and each firmware's first screen on both chips.
