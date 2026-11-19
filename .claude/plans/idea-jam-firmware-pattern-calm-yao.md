# DRUMS v0.2: more variations, fill lengths, intro / break / stop-time, MIX page, sample kit experiment

## Context
DRUMS 0.1 is merged (daviddwlee84/M-VAVE_FM-1#9). The owner asked for v0.2 as written in the roadmap
(`docs/dev/drums.md#next`): MAIN C/D in all ten styles, the free keys F3–D#4 used (short fill, tom fill, BREAK,
INTRO, transition key, stop-time), fill lengths (½ / 1 / 2 bars), plus the MIX page and a sample-kit experiment
(both chosen 2026-11-19). Same flow as 0.1: unit tests, simulator test, device build (native Linux, SDK files via
`just fw-sdk-from`), fm1-emu both chips with save/restart/reload, new PR with a `just fw-preview drums pr-N` link.
No device operation.

## Player (`firmware/drums/groove.c/.h`)
- Parts become: MAIN A–D (`A B C D`), their fills (`a b c d`), TOM fill (`t`), 2-bar fill (`w`), INTRO (`I`),
  BREAK (`K`, default: an empty bar), STOP (`P`, default: kick + crash on the one, then silence), ENDING (`E`).
  Missing C/D fall back to B / b; missing `t`, `w`, `I` come from per-metre generic texts (like `GV_FILL_44`).
- **Multi-bar parts**: a line may hold 1 or 2 bars of steps (all lines of a part equal); `n_bars` per part;
  the player keeps `part_bar` and continues a multi-bar part at the bar boundary before deciding anything new
  (an ENDING request waits for it).
- `sec` 0..3; `groove_main(g, sec)` unchanged semantics; transition release goes to the next MAIN (A→B→C→D→A).
- Fill requests carry a kind and a length: `groove_fill_ex(g, kind /*SEC, TOM*/, len /*HALF, ONE, TWO*/)`;
  `groove_fill` = section fill with the `fill_len` setting.
  - HALF: the fill's steps from the bar's middle (`fill_from = steps/2`, main before it — the existing tail logic);
    NOW+HALF starts at max(next step, middle), or the next bar's middle.
  - TWO: the `w` part over two bars; PHRASE puts it on bars phrase−2, phrase−1 (asked in phrase−1: one-bar fill
    now); BAR: the next two bars; NOW: this bar's tail is its first bar.
  - AUTO FILL on a MAIN change uses the `fill_len` setting.
- `groove_intro_start(g, sec)`: INTRO part(s), then MAIN `sec` with a crash. `groove_break(g)`, `groove_stop_time(g)`:
  the next bar is BREAK / STOP, then the current MAIN with a crash.
- Unit tests (`tests/groove_test.c`): every style has A–D and parses; C/D changes; HALF at every 16th under each
  FILL AT; TWO under PHRASE 4/8/16, BAR, NOW (incl. asked in the last bar); intro → MAIN with crash; break bar
  silent; stop-time hits once; transition cycles A→B→C→D→A; ending waits for a 2-bar fill; 3/4 and 6/8.

## Styles (`styles.h`)
Write MAIN C and D (C: busier hats / ride, D: biggest, crash on the one) and their fills for all ten; a `t` and
`w` where the generic one does not fit (shuffle, 6/8); intros for a few, generic for the rest. Generic texts per
metre for `t`, `w`, `I` in `groove.c`.

## Sounds (`kit.c/.h`)
- `kit_set_level(lane, gain)` (MIX), applied in `kit_trigger`.
- **Sample experiment**: a third kit `RUSTY` from Karoryfer's *Big Rusty Drums* (CC0-1.0,
  github.com/sfzinstruments/karoryfer.big-rusty-drums): kick, snare, rim, closed / open hat, ride, crash, 3 toms;
  clap, cowbell, shaker stay synthesised (STUDIO's).
  - `host/drums/make_samples.py` (MIT): fetches the chosen WAVs (sparse git checkout or raw; LFS may need
    media.githubusercontent.com), records each source path + SHA-256, trims (fade-out, ≤ ~0.6 s, cymbals ≤ ~1.2 s),
    normalises, resamples to 22,050 Hz mono, encodes IMA ADPCM in `firmware/common/adpcm`'s block format, writes
    `firmware/drums/samples.h` (committed: CC0, regenerable; header comment names the source and hashes).
    Budget target ≤ ~120 KB.
  - `kit.c`: a `KT_SAMPLE` voice: `adpcm` decode at 22.05 kHz, linear upsample ×2, velocity gain; include
    `../common/adpcm.c` into kit.c (only user). `kit_test.c` covers the sample lanes.
  - Measure and record: package / app size before–after, `.data+.bss`, ISR % in `--bench` with all lanes, WAVs from
    the simulator. If the source cannot be fetched: stop the experiment, keep the rest, say so.

## App (`app.c`, `app.h`, `fm1.toml`, `words.tsv`)
- Keys F3–E4: F3 A, F#3 FILL, G3 B, G#3 SHORT (½-bar section fill), A3 C, A#3 TOM fill, B3 D, C4 BREAK,
  C#4 TRANSITION (hold; the tick also queues key-ups), D4 INTRO (stopped: start with the intro), D#4 STOP-TIME,
  E4 ENDING. Stopped: any MAIN key starts in that MAIN.
- `ALGORITHM` = FILL LEN (½ / 1 / 2). `HOME` = PLAY ↔ MIX page.
- **MIX page**: 13 lanes as level bars with names; a pad (or `SELECT`) picks the lane, `K1` its level (0–10, 8 =
  0 dB), `K2` the kit (as PRESETS); levels saved. PLAY page unchanged otherwise.
- PLAY page: four MAIN boxes; status words INTRO / BREAK / STOP / FILL ½ / FILL 2; settings line adds FILL LEN.
- Record v2 (`DR02`): + `fill_len`, `levels[13]`; a v1 record falls back to defaults (0.1 was never installed).
  `FW_MINOR 2`.
- `tests/basic.txt`: MAIN C/D keys, G#3 half fill, A#3 tom fill, C4 break, D#4 stop-time, D4 intro start, C#4
  hold/release, ALGO fill len 2 with PHRASE, MIX page level change → saved → reboot → kept, RUSTY kit peak, shots.

## Docs and publishing
- `docs/dev/drums.md`: v0.2 section (controls table, screens in `docs/assets/drums/`, sample experiment numbers and
  verdict `[模擬]`, CC0 credit), roadmap row done; `AGENTS.md` row; `fm1.toml` description; credit line for the
  samples. `just docs-check`.
- Branch `claude/laughing-cori-ktq3wc` (already at `main`); commit, push, PR (template), `just fw-preview drums pr-N`
  with the link in the PR.

## Progress (after a worker restart)
Done and uncommitted in the working tree: player (`groove.c/.h`, 468 unit checks pass), ten styles with C/D,
RUSTY samples (`host/drums/make_samples.py`, `samples.h` 68,873 B, kit 156 checks pass), app v0.2 (keys, MIX page,
record v2), `tests/basic.txt` all `ok` (93.9 s simulated), screens reviewed.
Remaining: `just fw-build drums` (sizes vs 0.1: app 104,748 B), fm1-emu both chips (play, MIX level change, wait,
restart, level kept, writes only `0xD0000`; `--bench` ISR % on RUSTY), docs (`docs/dev/drums.md` v0.2 section +
sample numbers + CC0 credit, `docs/assets/drums/` shots, `AGENTS.md` row, `fm1.toml` description/credits,
`words.tsv`), `just docs-check`, commit, push, PR, `just fw-preview drums pr-N`, link in the PR.

## Verification
- `just drums-unit`, `just fw-test drums` all pass; `just fw-build drums` checks ok, sizes recorded;
  fm1-emu on `uc25hq80` and `p25q80`: play, change a MIX level, wait > 3 s, restart, level kept, only `0xD0000`
  written; `--bench` ISR % with the sample kit; preview link serves the same SHA-256 as the build.
