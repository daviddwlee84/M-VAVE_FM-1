# firmware/theory

THEORY, a pocket music theory trainer for the M-VAVE FM-1, built on fm1-base. Pages: MENU; TODAY, flash cards a
round a day (scales, key signatures, chords by degree, degrees and chord qualities by ear) in Leitner boxes; WHEEL,
the circle of fifths with the key's chords marked, naming what is played; CHORD, a whole screen about whatever is
played (its tones, other readings, the progression as numbers); CHART, progress on the circle; SETTINGS. Progress
backs up to a computer (`host/theory`, or `docs/assets/theory/progress.html`). 0.1 runs on the owner's unit since
2026-11-18; 0.2 is verified in the host simulator and fm1-emu. Design and controls (Traditional Chinese):
[docs/dev/theory.md](../../docs/dev/theory.md).

Licence: GPL-3.0-or-later (`LICENSE`). Package identity `FM-1_49` (dev `FM-1_4900`). Through fm1-base's store:
the settings at `0xD8000`, the flash cards' progress at `0xD6000`.

| File | What it is |
|---|---|
| `app.c`, `app.h`, `fm1.toml` | The app: the pages, input, lights, the records, the backup's SysEx (`F0 7D 54 48`), the simulator's checks. |
| `srs.h`, `srs.c` | Leitner boxes counted in rounds, for any list of items (after Morse's `learn.c`): new items shown first, segments, no key twice in a row, focus, groups, grades, backup blob. No platform, no libc. |
| `course.h` | The flash cards' 63 items in the order they open. Append only: progress and backups name items by place. |
| `theory.h`, `theory.c` | Spelling on the line of fifths, 12 scales, key signatures, the circle of fifths, 23 chord qualities, Roman numerals and Nashville numbers (naming and reading, secondary dominants), naming a chord from MIDI notes, the nearest close voicing. C99, no platform calls, no libc, no floating point. |
| `synth.h`, `synth.c` | Eight voices of a two-operator FM electric piano (after `../common/chime`), fed by a one-writer event ring. |
| `gtext.h`, `gtext.c` | Text with sharp, flat, diminished and half-diminished signs drawn as small bitmaps (the fonts are ASCII). |
| `tests/basic.txt` | The host simulator's script (`just fw-test theory`); its backup frames come from `host/theory/progress.py`. |
| `tests/theory_test.c` | Unit tests for the core: hand-written answers, round trips, every diatonic chord in every key and inversion, readings, random voicings. |
| `tests/srs_test.c` | Unit tests for the rounds: the course, grades, interleaving, focus, groups, backup. |
| `tests/vectors.h` | Generated from music21 by `host/theory/make_vectors.py`; do not edit. |

```sh
just theory-unit      # the core's and the rounds' unit tests on this computer
just fw-test theory   # the app in the host simulator (screenshots in build/host/)
just theory-test      # the computer side's tests
just theory-backup FILE / just theory-restore FILE
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
