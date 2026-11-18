# firmware/theory

THEORY, a pocket music theory trainer for the M-VAVE FM-1, built on fm1-base. v0.1 is the WHEEL page: the circle
of fifths with the key's chords marked; play a chord and it is named, with its Roman numeral in the key
(secondary dominants, borrowed chords, inversions); or let the white keys play the key's own chords, voiced to
move as little as possible. Verified in the host simulator and fm1-emu, and running on the owner's unit since 2026-11-18. Design and controls
(Traditional Chinese): [docs/dev/theory.md](../../docs/dev/theory.md).

Licence: GPL-3.0-or-later (`LICENSE`). Package identity `FM-1_49` (dev `FM-1_4900`). Settings: one record at
`0xD8000` through fm1-base's store.

| File | What it is |
|---|---|
| `app.c`, `app.h`, `fm1.toml` | The app: the WHEEL page, input, lights, the settings record, the simulator's checks. |
| `theory.h`, `theory.c` | Spelling on the line of fifths, 12 scales, key signatures, the circle of fifths, 23 chord qualities, Roman numerals and Nashville numbers (naming and reading, secondary dominants), naming a chord from MIDI notes, the nearest close voicing. C99, no platform calls, no libc, no floating point. |
| `synth.h`, `synth.c` | Eight voices of a two-operator FM electric piano (after `../common/chime`), fed by a one-writer event ring. |
| `gtext.h`, `gtext.c` | Text with sharp, flat, diminished and half-diminished signs drawn as small bitmaps (the fonts are ASCII). |
| `tests/basic.txt` | The host simulator's script (`just fw-test theory`). |
| `tests/theory_test.c` | Unit tests for the core: hand-written answers, round trips, every diatonic chord in every key and inversion, random voicings. |
| `tests/vectors.h` | Generated from music21 by `host/theory/make_vectors.py`; do not edit. |

```sh
just theory-unit      # the core's unit tests on this computer
just fw-test theory   # the app in the host simulator (screenshots in build/host/)
just fw-build theory  # the device package, build/theory.fwsc
just theory-emu       # that package in fm1-emu
just theory-vectors   # regenerate tests/vectors.h (uv fetches music21 10.5.2)
```

Conventions worth knowing before using the core:

- A spelled note is an `int` f on the line of fifths: C = 0, G = 1, F = -1, F# = 6, Gb = -6. Pitch class is
  `7f mod 12`; `+7` adds a sharp; `+12` is the next enharmonic spelling.
- Roman numerals measure accidentals against the major scale on the same tonic, minor keys included: in A minor,
  G is `bVII` and G#dim is `viio`.
- Output is ASCII (`#`, `b`, `o`, `/o`) unless the includer defines `TH_GLYPH_SHARP`, `TH_GLYPH_FLAT`,
  `TH_GLYPH_DIM` and `TH_GLYPH_HDIM` first; the app sets them to `gtext`'s marker bytes.
- Static helpers and tables are prefixed `th_` / `TH_` so the file can be `#include`d into fm1-base's unity build
  beside an app's own code.
