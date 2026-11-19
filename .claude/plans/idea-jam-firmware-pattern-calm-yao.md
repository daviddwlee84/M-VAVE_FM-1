# DRUMS: record the next steps, then merge PR #9

## Context
The owner tried DRUMS 0.1 (daviddwlee84/M-VAVE_FM-1#9) and is happy to merge it. They asked:
(1) is a style only MAIN A/B, can it have more variations (was ENDING put on E4 to leave room?);
(2) editing / adding patterns on the device later (the preset model is good for now);
(3) standard drum MIDI out so the FM-1 can drive another sound source with its own sound muted — record for later;
(4) fills are one bar — other lengths?
Nothing to implement now: answer, write the answers into `docs/dev/drums.md` as the roadmap, then merge.

## Facts from the code (v0.1)
- `groove.h`: `GV_PARTS` = MAIN A, MAIN B, FILL A, FILL B, END; `sec` is 0/1. Adding MAIN C/D = 4 more parts
  (MAIN C, D, FILL C, D), `sec` 0..3; text codes e.g. `C D c d` (C/D not used as part codes yet; lane codes are
  separate). Styles without C/D can fall back (C = B with more hats, or reuse B).
- Keys: F3 MAIN A, F#3 FILL, G3 MAIN B, E4 ENDING; keys 3–10 (G#3 A3 A#3 B3 C4 C#4 D4 D#4) are free — yes, E4
  was chosen to keep F3–D#4 for the design's MAIN C/D, BREAK, INTRO, fills 2–3, transition.
  Proposed map: A3 MAIN C, B3 MAIN D, C4 BREAK, D4 INTRO; G#3 FILL 2 (short), A#3 FILL 3 (tom roll), C#4
  TRANSITION (hold), D#4 crash stop.
- Fill length: every fill is written as one bar; NOW plays its tail. Next: half-bar fills (PSR's Half Bar
  Fill-in: last 2 beats, from the fill's second half), two-bar build fills (a part spanning 2 bars, PHRASE puts it
  on the last two), and the transition already loops for as long as REC is held.
- Store: one object ≤ 3,840 B; an event is 3 B, a part ~20–60 events → a user style (9 parts) ~1–1.5 KB → 2 user
  styles an object; more objects can be added to `APP_STORE_MAP`.
- MIDI: `plat_midi_send(pkt, TO_USB | TO_BLE)`; GM drum map on channel 10 (kick 36, snare 38, rim 37, clap 39,
  closed hat 42, open hat 46, ride 51, crash 49, toms 50/47/43, cowbell 56, shaker 70 / maracas), velocity from
  the step's 1–9; note-off after a few ms; LOCAL setting (FM-1 sound on/off); clock 0xF8 every 4 ticks
  (GV_TPB 96 = 24 PPQN), 0xFA/0xFC. Over USB a computer or host sits between FM-1 and another USB device;
  Bluetooth needs `ble = true`.

## Steps
1. On `claude/laughing-cori-ktq3wc` (PR #9): add a section `## 之後 { #next }` to `docs/dev/drums.md` with the four
   answers above (variations + key map, fill lengths, MIDI out with GM map and LOCAL, on-device editing: copy a
   preset into a USER slot, step edit page with the grid, saved through `APP_STORE_MAP`, presets stay read-only),
   and update the roadmap table (v0.2 MAIN C/D + more fills + intro/break; v0.3 MIDI out/clock + song mode;
   v0.4 on-device editing). Tags: `[上游]` for PSR half-bar fill only where sourced, otherwise plain design text.
2. `just docs-check`; commit; push.
3. Merge PR #9 (merge commit); `just fw-preview drums pr-9 --remove` (AGENTS: once the PR is merged).
   Restart the branch from the new `main`.

## Verification
- docs-check passes; PR #9 merged; `previews` branch no longer has `pr-9/`.
