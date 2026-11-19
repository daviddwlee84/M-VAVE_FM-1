# Live drum machine — design page only (PR #8, no implementation)

## Context
PR daviddwlee84/M-VAVE_FM-1#8 explores backing-band / arranger options; the owner's own JAM is on
hold (2026-11-19). The owner tried JAMTRACK, Polly and CORD in their **web emulators** (not on the
unit) and now wants a design, not code, for a **pure live drum machine**: a standalone firmware,
synthesized drums first, sample kits if the flash budget allows, played into a mixer through the
3.5 mm output so any instrument can jam with it. Use cases: jamming with the MODX / keyboard,
guitar practice, and playing it on the FM-1 itself.

## Deliverables
1. **`docs/firmware/jam.md`**: impressions came from the web emulators → tag them `[模擬]` and link
   the three pages (`chasedeanda.github.io/cord-fm1/play/`, `philip-bos.github.io/jamtrack-fm1/emulator/`,
   `charlesvestal.github.io/fm1-polly/emu/`), also in the compare table ("try in the browser").
2. **New `docs/dev/drums.md`** (zh-TW, layout like `docs/dev/rhythm.md`): status, model, timing
   rules, controls, sound, content, sync, code structure, flash, roadmap, open questions. Add to `nav`
   after the JAM page ("Live 鼓機（設計）").
3. **`docs/dev/jam.md`**: shrink "方向 B" to a summary + link to `drums.md`; keep the product survey there.
4. **`AGENTS.md`** "Not set up yet": one line pointing at `docs/dev/drums.md` (design only).
5. `just docs-check` (here: `uv run --locked --only-group docs …`, rtmidi needs ALSA), commit, push to
   `claude/laughing-cori-ktq3wc` (updates PR #8).

## Design content for `docs/dev/drums.md`
**Status**: design only, not scheduled. Standalone firmware, working name DRUMS (check the Hub before
naming). GPL-3.0-or-later (lifts RHYTHM's transport). Identity `FM-1_58` (free on 2026-11-19),
store `0xD0000` (verify blank with `just emu-rehearse`).

**Model** (BeatBuddy / RC-10R / PSR merged):
- Style = INTRO, MAIN A–D, FILL per MAIN (+ a few shared fills), TRANSITION, BREAK, ENDING.
- Kit chosen separately from style (any style × any kit).
- Optional **song** (v0.3): parts with bar counts, e.g. `intro 2 | A 8 | B 8 | A 8 | ending`; fills
  land on the last bar of each part automatically → hands-free guitar practice.

**Timing rules** (transport from `firmware/rhythm/app.c`: `tr`, `tick_at`, `q8_mul`, `beat_q8_at`,
the per-sample scheduler in `app_render`; but 24 ticks per beat so 32nd flams and MIDI clock's 24 PPQN
map 1:1):
- `FILL AT` setting: `NOW` (from the next 16th to the bar end, PSR), `BAR` (next bar, Polly / RHYTM-SEC-1),
  `PHRASE` (last bar of the current 4 / 8 / 16-bar phrase — the gap no product fills).
- Hold = transition loops until release, then switches at the bar end (BeatBuddy).
- MAIN change: at bar or phrase end, with `AUTO FILL` on/off. Requests queue; a later one replaces.
- Fill started mid-bar plays only its tail; bar-length fills are written as one bar.

**Controls** (real panel: left SELECT / PRESETS / ALGORITHM, OCT-/+; right K1–K4, rows FX SEL ENV LFO
EDIT GLO / HOME SAVE ARP SEQ PLAY REC; keys F3–G5, 11 black at key indices 1,3,5,8,10,13,15,17,20,22,25):
- `PLAY`: stopped → intro (if on) then MAIN A; running → ending then stop; hold → stop now.
- `REC`: stopped → tap tempo (RHYTHM's `tap_tempo`); running → the one BeatBuddy button
  (tap fill, hold transition, double-tap ending). Same gestures from a MIDI footswitch CC over TRS / BLE.
- Left keys F3–E4 = performance: whites MAIN A–D, BREAK, INTRO-again, ENDING; blacks fills 1–3,
  transition, crash-stop. Right keys F4–G5 = 15 finger-drum pads (kick, snare, rim, clap, CH, OH, ride,
  crash, 3 toms, cowbell, tambourine, shaker, …), velocity if `APP_TOUCH` timing is good enough (to check).
  `SEL` + pad = mute / unmute that lane (drops).
- `ALGORITHM` genre group, `SELECT` style, `PRESETS` kit; `K1` BPM, `K2` swing, `K3` energy (ghost notes,
  hat density), `K4` humanize; `OCT-/+` BPM ±1; `ARP` AUTO FILL; `SEQ` FILL AT mode / phrase length;
  `FX` hold = build-up (filter + kick drop); `HOME` pages (PLAY, MIX levels/pan per lane, KIT tune/decay,
  SONG, SETTINGS); `SAVE` user song / settings (also autosave after 3 s, RHYTHM's dirty pattern).
- LEDs (`firmware/common/ledfx`): current MAIN lit, queued MAIN/fill blinking, pads flash when their
  lane plays, PLAY green running.

**Sound**:
- Synth kits first (808, 909, electro, lo-fi): kick sine sweep, snare noise + tone, hats filtered
  noise / FM metal, clap, toms, rim, cowbell. ~10–12 voices, choke groups (OH cut by CH), per-lane
  level/pan, stereo out, optional low-end boost off for line out.
- Samples: `firmware/common/adpcm` (IMA 4-bit). Budget math: app slot 581,564 B, RHYTHM-size app ~104 KB
  → roughly 400 KB free; 4-bit at 22.05 kHz = 11.2 KB/s; a 12-piece kit ≈ 5 s ≈ 56 KB → 5–6 kits.
  Cymbals lose top end at 22 kHz → hybrid: sample kick/snare/toms, synthesize hats/cymbals, or 32 kHz
  for cymbals. Samples must live in the app (const arrays), never in the flash data area (AGENTS rule).
  Decide with a voicelab-style spike that measures size and CPU in fm1-emu (`--bench`).
- Sources: CC0 packs (to find) or own recordings.

**Content**: styles as text grids → generated C via fm1-base `gen`; event = 2 B (tick, lane, velocity
level); a style ≈ 18 bars ≈ 1 KB → 50 styles ≈ 50 KB. Patterns written by hand; Magenta Groove MIDI
Dataset (CC BY 4.0, used by Polly, has beat/fill labels `[調研]`) as a source with attribution. Genre
list draft: rock 8/16, pop, ballad, funk, shuffle/blues, swing/jazz, bossa, samba, reggae, hip-hop, lo-fi,
house, DnB, 6/8, 3/4 waltz, 5/4, 7/8.

**Sync and outputs**:
- 3.5 mm stereo out to a mixer (`docs/hardware/io.md`).
- MIDI clock out (0xF8 at 24 PPQN, start/stop) over USB/BLE — needs a computer/host in between.
- Clock in from USB/BLE/TRS: MODX's 5-pin MIDI OUT → TRS adapter → FM-1 follows the MODX directly
  (MODX DIN ports `[調研]`, verify against Yamaha's spec page). No firmware here does clock yet.
- MIDI note out per lane (GM drum map) for a DAW, later.

**Code structure**: `firmware/drums/` — `groove.c` (platform-free: style, song, request queue, boundary
logic, pattern → events; unit-tested with a `just drums-unit`), `kit.c` (synth voices), optional
`sample.c` (adpcm), `app.c` (UI, transport, store), `content/*.txt`. Test cases: fill at every 16th of a
bar under each FILL AT mode, hold/release transition timing, double-tap vs tap, 3/4 and 6/8, phrase
lengths, MAIN + FILL queued together, clock-in jitter.

**Roadmap**: v0.1 synth kit ×2, ~10 styles, MAIN A/B, FILL (NOW/BAR/PHRASE), tap tempo, REC button
gestures, pads; v0.2 MAIN C/D, INTRO/ENDING/BREAK/TRANSITION, AUTO FILL, mix page, sample spike;
v0.3 song mode, MIDI clock in/out, footswitch CC; v0.4 more kits/styles, practice extras (gap bars,
tempo ramp — overlap with RHYTHM).

**Open questions**: name; sample vs synth after the spike; touch velocity timing; whether practice
features live here or in RHYTHM.

## Verification
- Docs build strict + `scripts/check-docs-site.py` pass; new page in nav.
- Claims tagged: `[模擬]` for the web-emulator impressions, `[上游]`/`[調研]` for products and specs.
