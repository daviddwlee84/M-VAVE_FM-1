# firmware/theory

The start of a pocket music theory trainer for the M-VAVE FM-1: so far only its platform-free core. There is no
`app.c` or `fm1.toml` yet, so `just fw-test theory` does not apply. Design (Traditional Chinese):
[docs/dev/theory.md](../../docs/dev/theory.md).

Licence: GPL-3.0-or-later.

| File | What it is |
|---|---|
| `theory.h`, `theory.c` | Spelling on the line of fifths, 12 scales, key signatures, the circle of fifths, 23 chord qualities, Roman numerals and Nashville numbers (naming and reading, secondary dominants), naming a chord from MIDI notes, the nearest close voicing. C99, no platform calls, no libc, no floating point. |
| `tests/theory_test.c` | Unit tests: hand-written answers, round trips, every diatonic chord in every key and inversion, random voicings. |
| `tests/vectors.h` | Generated from music21 by `host/theory/make_vectors.py`; do not edit. |

```sh
just theory-unit      # build and run the tests on this computer
just theory-vectors   # regenerate tests/vectors.h (uv fetches music21 10.5.2)
```

Conventions worth knowing before using it:

- A spelled note is an `int` f on the line of fifths: C = 0, G = 1, F = -1, F# = 6, Gb = -6. Pitch class is
  `7f mod 12`; `+7` adds a sharp; `+12` is the next enharmonic spelling.
- Roman numerals measure accidentals against the major scale on the same tonic, minor keys included: in A minor,
  G is `bVII` and G#dim is `viio`.
- Output is ASCII (`#`, `b`, `o`, `/o`). An app with glyphs for sharp, flat, diminished and half-diminished
  defines `TH_GLYPH_SHARP`, `TH_GLYPH_FLAT`, `TH_GLYPH_DIM` and `TH_GLYPH_HDIM` before including `theory.c`.
- Static helpers and tables are prefixed `th_` / `TH_` so the file can be `#include`d into fm1-base's unity build
  beside an app's own code.
