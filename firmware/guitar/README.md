# firmware/guitar

GUITAR, a pocket guitar neck and chord book for the M-VAVE FM-1, built on fm1-base. v0.1 has two pages: NECK, a
key's scale on frets 0-12 (the whole neck, or one CAGED / three-notes-per-string position at a time) on guitar,
bass, ukulele or mandolin in eight tunings; SHAPE, chord diagrams (fingers, barres, open and muted strings) found by
search, easiest first. Verified in the host simulator and fm1-emu, not yet on the unit. Design and controls (Traditional Chinese):
[docs/dev/guitar.md](../../docs/dev/guitar.md).

Licence: GPL-3.0-or-later (`LICENSE`): it includes THEORY's `theory.c`, `synth.c` and `gtext.c` from `../theory`.
Package identity `FM-1_55` (dev `FM-1_5500`; free in the Hub's catalog of 2026-11-18). Settings: one record at
`0xD4000` through fm1-base's store.

| File | What it is |
|---|---|
| `app.c`, `app.h`, `fm1.toml` | The app: NECK and SHAPE, input, lights, the strum and scale run, the settings record, the simulator's checks. |
| `fret.h`, `fret.c` | Tunings, where a key's notes lie, CAGED and NPS positions, chord shape search with fingering and ranking. C99, no platform calls, no libc, no floating point; needs `theory.h`. |
| `tests/basic.txt` | The host simulator's script (`just fw-test guitar`). |
| `tests/fret_test.c` | Unit tests for the core: textbook positions, every key and scale, 29 open and barre chords, every chord on every tuning checked playable. |

```sh
just guitar-unit      # the core's unit tests on this computer
just fw-test guitar   # the app in the host simulator (screenshots in build/host/)
just fw-build guitar  # the device package, build/guitar.fwsc
just guitar-emu       # that package in fm1-emu
```

Conventions worth knowing before using the core:

- Strings count from the lowest-pitched one: string 0 is a guitar's low E. A re-entrant tuning (ukulele) keeps its
  order on the neck, so its string 0 is the high G; the chord search then does not ask for the root in the bass.
- A position is a fret mask per string (`uint32_t`, bit f = fret f, frets 0..24).
- Statics are prefixed `fr_` / `FR_` so the file can be `#include`d into fm1-base's unity build.
