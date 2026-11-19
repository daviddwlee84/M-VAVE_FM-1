# RHYTHM v0.3: PULSE, JUDGE, TERMS

## Context

The owner asked (2026-11-19) whether RHYTHM has a "conductor trainer": set a tempo, then tap blind for N bars and see
accuracy, in two difficulties (a count-in you can hear first / nothing but the BPM number); a Whiplash-style "were you
rushing or dragging?" listening test; and a theory quiz on Italian tempo markings and their BPM.

Today RHYTHM (firmware/rhythm, v0.2, not on the unit yet) has METRO / ECHO / ENDLESS. The only blind-tap thing is
METRO's gap click (N bars on, N bars off, mean ms). None of the three asks exist. Owner decisions:

- New pages in RHYTHM v0.3 (not a new firmware). `HOME` cycles METRO → ECHO → ENDLESS → PULSE → JUDGE → TERMS.
- JUDGE has all three question types: TEMPO, FEEL, DRIFT, each an adaptive staircase that ends in a threshold.
- Tempo-term BPM table follows the **English Wikipedia** "Basic tempo markings" list (fetched 2026-11-19); the
  zh-tw page's numbers differ and are noted in the docs.

## Design

### 1. PULSE (blind tapping, conductor trainer)

Controls (when stopped): `K2` mode **GUIDED / COLD**; `K3` blind bars 2, 4, 8, 16; `K1` meter as elsewhere;
`SELECT`/`OCT±` BPM; `K4` LAT (shared). `SEL` (GUIDED only) count-in bars 1 / 2 / 4. `PLAY` start/stop.

- **GUIDED**: count-in bars clicked (tap along if you like, not scored), then the click stops and you tap every beat
  for the blind bars. Reference grid = the count-in's grid continued (absolute phase known).
- **COLD**: silent. Screen shows only the BPM. Your first tap is beat 1 (sets phase); tap bars × bpb beats. Reference
  grid anchored at the first tap.
- Ends after the last expected beat + half a beat (or `PLAY`).
- After the end, an optional one-bar "reveal" click at the true tempo continuing from your last expected beat, so you
  hear where it should have been (on by default; `REC` replays it).

Result screen metrics (platform-free, in `rhythm.c` as `rh_pulse_*`):
- per-beat error vs grid (ms), matched nearest-beat (each tap ≤ one beat, missed / extra counted, reusing the DP
  alignment idea of `rh_score`, on a grid of plain quarter onsets — can likely just build a pattern of `q`s and call
  `rh_score` directly for grades, GOOD count, missed/extra, mean, score);
- **tempo**: least-squares slope of tap time vs beat index → your BPM and its error in % (for COLD this is the main
  number); **drift**: error at the end minus at the start from the fit, ms/bar; **jitter**: SD of the inter-tap
  intervals (ms) and of residuals from the fit;
- **histogram** of errors (e.g. 9 bins of ±GOOD/… up to ±150 ms, plus under/over), drawn as bars; **drift plot**:
  error per beat as dots over time with the fit line; colours as ECHO (`C_GOOD/C_OK/C_BAD`).
- hint: RUSHING (tempo > +2 %), DRAGGING (< −2 %), DRIFTING, UNSTEADY (jitter high), GOOD — thresholds as named
  constants, "to be tuned on the unit" like v0.1.
- best (lowest |tempo %| with ≥ 90 % beats answered) per mode saved.

Float math only (device has no 64-bit divide; follow `q8_mul`/`smp_ms` pattern in app.c).

### 2. JUDGE (Whiplash: faster/slower, rushing/dragging)

`K2` type **TEMPO / FEEL / DRIFT**; `SELECT`/`OCT±` base BPM; `SEL` "RANDOM BPM" on/off (base ± 15 % each trial so
you can't memorise). `PLAY` start a run, `REC` hear the trial again. Answer: any key in the **lower half** of the
keyboard = SLOWER / DRAGGING, **upper half** = FASTER / RUSHING (both choices shown on screen at the left/right).

- **TEMPO**: 2 bars clicks at T, 1 beat gap, 2 bars at T·(1±d). d starts 8 %.
- **FEEL**: steady soft click at T for 2 bars + a snare-ish voice (the existing note voice) playing quarters (later:
  a simple rock pattern) every hit offset by ±d ms. d starts 60 ms. Count-in bar first so the click is established.
- **DRIFT**: 4 bars where the beat interval changes multiplicatively by (1±d/n) each beat (accel/rit), ending at
  T·(1±d). d starts 10 %.
- Staircase: 2-down / 1-up on d, step ×0.7 down / ×1.4 up (geometric), floor/ceiling per type; run ends after 8
  reversals or 40 trials; threshold = geometric mean of the last 6 reversals (≈ 70.7 % correct point). Screen: the
  trial's question, right/wrong flash + LED, d now, reversals, a little staircase plot after the run.
- Saved: best (smallest) threshold per type.
- Platform-free part in `rhythm.c`: `rh_stair_t` (init, answer, done, threshold) and the onset generator for each type
  (sample times of every click/hit for given T, d, sign), so both are unit-tested on the host.

### 3. TERMS (tempo markings quiz)

Table `rh_terms[]` in `rhythm.c` from en.wikipedia "Tempo › Basic tempo markings" (2026-11-19): Larghissimo ≤24,
Grave 24–40, Largo 40–66, Larghetto 44–66, Lento 40–60, Adagio 44–66, Adagietto 46–80, Andante 56–108, Andantino
80–108, Moderato 108–120, Allegretto 112–120, Allegro moderato 116–120, Allegro 120–156, Vivace 156–176, Presto
168–200, Prestissimo ≥200 (plus Chinese gloss kept in docs only; the LCD font is ASCII). Change terms (accelerando,
ritardando, rallentando, a tempo, più/meno mosso, ritenuto, stretto, rubato, allargando, tempo primo) as a second
deck with English meanings.

`K2` question type:
- **TERM → BPM**: shows "ANDANTE", 4 BPM ranges/values; pick the one inside the range.
- **BPM → TERM**: shows "132 BPM", 4 terms.
- **HEAR**: clicks at a BPM drawn from one term's range (BPM not shown), 4 terms.
- **CHANGE**: "rit." → 4 meanings (or the reverse).

Distractor rule (ranges overlap!): a distractor term must have a range that does **not** contain the asked BPM; for
TERM → BPM a distractor value must lie outside the asked term's range. Unit-tested over every term and BPM.
`K3` level: 1 = the 7 core terms (Largo, Adagio, Andante, Moderato, Allegro, Vivace, Presto), 2 = all 16, 3 = + the
change terms. Answer with 4 white keys (C D E F of the lowest octave, coloured by LED); correct/wrong flash, then
next. Scoring: session streak and accuracy; per-term miss counts weight the draw (missed terms come back 3× as
often) — simple, no Leitner (THEORY's `srs.c` can be reused later if wanted).

### Settings record

`REC_VER` 2, larger `rec_t`: add `pulse_mode, pulse_bars, pulse_count, judge_type, judge_rand, terms_type,
terms_level`, best PULSE % ×2, best JUDGE thresholds ×3 (uint16, in 0.1 % / ms), TERMS best streak. `rec_valid`
accepts v1 (old size) and migrates it (copy fields, new ones default) — check how `plat_store_load` reports size in
`third_party/fm1-base` before writing it. Same `0xD2000` object; still one sector, no new flash area.
`PG_N` 6, `rec.page < PG_N`.

## Files

- `firmware/rhythm/rhythm.h`, `rhythm.c`: pulse analysis (fit, histogram, hint), staircase, JUDGE onset generator,
  terms table and question maker with distractor rule.
- `firmware/rhythm/app.c`: three pages (state machines like `echo_*` / `en_*`, events through the existing
  `tr_go_at` / `event_at` / `sound` path — JUDGE's non-grid times need an explicit sample-time event list rather than
  ticks: add a small `ev_smp[]` queue consumed in `app_render`), drawing (`draw_pulse`, `draw_judge`, `draw_terms`),
  input mapping, LEDs, record v2, host checks/commands. Bump `FW_MINOR` 3. Split into `pulse.c`-style files only if
  app.c gets unwieldy (already 1.4k lines; prefer `app_pulse.c`, `app_judge.c`, `app_terms.c` sharing a small
  internal header if fm1.toml/build allows extra sources — check `third_party/fm1-base/tools/build.py`).
- `firmware/rhythm/tests/rhythm_test.c`: fit/histogram on synthetic taps (exact tempo, +3 %, linear drift, jitter),
  staircase converges on a simulated listener with a known threshold, JUDGE onsets (TEMPO ratio, FEEL offset, DRIFT
  ends at T·(1±d)), terms distractors never correct for all terms × BPM.
- `firmware/rhythm/tests/basic.txt`: new host commands `pulsetap OFFSET [DRIFT_MS_PER_BEAT] [SKIP]`,
  `judgeanswer right|wrong|faster|slower`, `termsanswer right|wrong`, expects (`pulse_bpm_x10`, `pulse_jitter`,
  `judge_d`, `judge_done`, `terms_correct` …), screenshots of each page and result.
- `firmware/rhythm/fm1.toml`: description mentions the new pages; `words.tsv` if new spoken words are used.
- `docs/dev/rhythm.md`: v0.3 section (controls table extended, the three pages, metrics, staircase, terms table with
  source + zh-tw differences noted, `[模擬]` verification); `AGENTS.md` layout row for RHYTHM; README/docs index if
  they list RHYTHM features.

## Verification

1. `just rhythm-unit` (strict C99) passes, including the new tests.
2. `just fw-test rhythm`: all old expects still pass (v0.2 behaviour unchanged), new ones pass; look at the shots.
3. v1 record → v2 migration check in the simulator (save with v1 layout, boot, settings kept).
4. Device build (`just fw-build rhythm`) and fm1-emu on `uc25hq80` and `p25q80` with save/restart/reload need the
   owner's Mac (JieLi toolchain/Docker); list them as owner steps in the docs and the PR, with
   `just emu-rehearse` over the latest backup to confirm only `0xD2000` is written.
5. `just docs-check`. Commit on `claude/rhythm-metronome-practice-lytzzu`, push; no PR unless asked.

## Changed during implementation

- The owner added "hear the time signature" mid-way: TERMS became **QUIZ** with a fifth type, METER
  (3/4, 4/4, 6/8, 5/4 at level 1; + 7/8, 9/8; 2/4 vs 4/4 and 6/8 vs 12/8 left out as not tellable by ear).
- Staircase: 10 reversals (not 8), checked against a simulated listener with a graded psychometric function.
- PULSE ends as soon as the last beat is tapped (by your own pulse), else waits while taps keep coming, so a
  dragging run is not cut short.
