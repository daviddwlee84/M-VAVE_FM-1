# M-VAVE FM-1 Notes

Personal notes on the [M-VAVE FM-1](https://www.m-vave.com/) pocket FM synth:
flashing and recovery, community firmware (starting with SLOOP), usage notes,
and the context coding agents need to help operate the device safely.

**Site:** <https://daviddwlee84.github.io/M-VAVE_FM-1/> (written in Traditional Chinese)

## Start here

- [Getting started](docs/getting-started/index.md) — install M-Flasher, back up the stock flash, flash SLOOP 2.8.5
- [Cautions](docs/getting-started/cautions.md) and [recovery](docs/getting-started/recovery.md)
- [Firmware map](docs/firmware/index.md) and the [SLOOP section](docs/firmware/sloop/index.md)
- [Agentic workflow](docs/agentic/index.md) — what an agent may automate and what needs approval
- [Conductor](docs/agentic/conductor.md) — our own coding-agent console (firmware + host), see below
- [Morse](docs/dev/morse.md) — our own pocket Morse code machine (firmware + host), see below
- [THEORY](docs/dev/theory.md) — our own pocket music theory trainer (firmware + host), see below
- [GUITAR](docs/dev/guitar.md) — our own pocket guitar neck and chord book (firmware), see below
- [CUBE](docs/dev/cube.md) — our own Rubik's cube timer, random-state scrambler, formula book and virtual cube (firmware), see below
- [METRONOME](docs/dev/metronome.md) — our own stage metronome with a web setlist editor ([fm1-metronome](https://github.com/daviddwlee84/fm1-metronome)), see below
- [Resource map](docs/resources.md) — upstream repos and current versions
- [Device log](docs/device-log.md) — the state of my own unit

## Our own firmware + host

Built here on [fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0). Conductor and the shared modules are MIT; the Morse, THEORY, GUITAR, METRONOME and CUBE firmwares are GPL-3.0-or-later (Morse's host program here is MIT).
Each one is a firmware for the FM-1 plus, where it needs one, a program on the computer.

| Project | What it is | Firmware | Host | Status |
|---|---|---|---|---|
| **Conductor** | The FM-1 as a desk console for coding agents: one agent per key, urgency on the key LEDs, state and output on the screen (Chinese included), chimes, approve / deny / interrupt from the buttons, push-to-talk dictation, USB or Bluetooth | [`firmware/conductor/`](firmware/conductor/) | [`host/conductor/`](host/conductor/) (Python daemon for [herdr-core](https://github.com/herdrdev/herdr-core) `hc` and classic Herdr) | Firmware 0.3, host 0.3.2; ran on my unit 2026-11-17, now switched to Morse |
| **Morse** | A pocket Morse code machine: key on the piano keys (straight key or iambic paddles) and read what you sent, type letters on the keys and hear them, flash cards with spaced repetition (by sound by default; keyed answers judged by their own rhythm), a placement test, progress you can edit, back up and restore, silent mode with the lights, USB or Bluetooth MIDI in the Vail adapter's format | [`firmware/morse/`](firmware/morse/) | [`host/morse/`](host/morse/) (types what you key into the computer; sends text for the FM-1 to play) | Firmware 0.8 (SAVE saves at once), host 0.8.0; on my unit since 2026-11-18; [learning with it](docs/dev/morse-learn.md) |
| **THEORY** | A pocket music theory trainer. v0.1, the WHEEL: the circle of fifths with the key's chords marked; play a chord and it is named with its Roman or Nashville number in the key (secondary dominants, borrowed chords, inversions); or the white keys play the key's own chords, voiced to move least; nine modes, every key spelled right. 0.2 adds flash cards a round a day (scales, key signatures, chords by degree, degrees and qualities by ear), a page naming whatever you play, progress on the circle, backup. 0.3 explains each kind of card and lets you play new ones along, lights answer notes one by one (they keep sounding), asks less of the same, adds PITCH (absolute pitch games), a round of one key from the chart, and a latch mode for naming chords | [`firmware/theory/`](firmware/theory/) | [`host/theory/`](host/theory/) (progress backup / restore; a [web page](docs/assets/theory/progress.html) too) | Firmware 0.2 on my unit since 2026-11-18 (Morse's progress kept for switching back); 0.3 built and tested in the simulator and emulator, host 0.3.0; [design](docs/dev/theory.md) |
| **GUITAR** | A pocket guitar neck and chord book. v0.1: NECK, a key's scale on frets 0-12, whole or one CAGED / NPS position at a time, on guitar, bass, ukulele or mandolin in eight tunings; play a note and every place it lies lights up. SHAPE: chord diagrams with fingers and barres found by search, easiest first; play a chord to see its shapes, PLAY strums it. 0.2 (in progress): power chords, a BARRE / OPEN / MOVABLE filter, and LEARN, chords taught key by key (C, then G, D, A and E, then F with barre chords) on Leitner cards | [`firmware/guitar/`](firmware/guitar/) | — | Firmware 0.1, simulator and fm1-emu only; 0.2 simulator only; [design](docs/dev/guitar.md) |
| **METRONOME** | A stage metronome: 16 songs on the white keys, each with its own tempo, meter, step pattern and sound, switched at the next bar; tempo markings (LARGO … PRESTO) beside the BPM, meter presets (6/8, 7/8 2+2+3 …), a rhythm-watch mix page, CLEAR ALL after a show. 0.3: the setlist to a JSON file and back over USB, and a [web editor](https://daviddwlee84.github.io/fm1-metronome/web/) that plans, plays and sends it. 0.4 (released 2026-11-20): a live link over USB or Bluetooth, a page with a conductor beating the meter, the clicks from the computer, the transport. 0.5 (merged, not released): the page sets its clock by round trips, one sound for every song (SET EDIT `K3`) | [`firmware/metronome/`](firmware/metronome/) (submodule: [fm1-metronome](https://github.com/daviddwlee84/fm1-metronome)) | its `host/` (Python) and `web/` | 0.5 (development build) on my unit since 2026-11-20, the live page over USB and Bluetooth checked on it; on [The Firmware Hub](https://thefirmwarehub.com/firmware/metronome/) since 2026-11-19; [design](docs/dev/metronome.md), [cheatsheet](https://daviddwlee84.github.io/fm1-metronome/web/cheatsheet.html) |
| **CUBE** | A Rubik's cube companion. TIMER: WCA inspection, hold a key and let go to start (or both end keys, like a timer's pads), ao5 / ao12 / ao100, with random-state scrambles the FM-1 finds itself (Kociemba's two-phase algorithm) or random-move ones. CUBE: a virtual cube turned from the keys, four layouts, seen from any side (K1-K4), scrambles followed by the key lights. ALGS: beginner's, 2-look, OLL 57, PLL 21, F2L 41, each case drawn from its formula. DRILL: flash cards, your own cube with a setup and a timer, or the virtual one | [`firmware/cube/`](firmware/cube/) | — | Firmware 0.1, verified in the simulator, fm1-emu on both chips and a rehearsal over a backup; not on my unit; [design](docs/dev/cube.md) |
| `hello` | fm1-base's template, unmodified: the toolchain smoke test | [`firmware/hello/`](firmware/hello/) | — | Builds and boots in the emulator |
| Shared modules | `hostlink` (SysEx link to a host over USB / BLE), `ledfx` (LED patterns), `chime` (FM notification voice) | [`firmware/common/`](firmware/common/) | — | Used by Conductor; Morse uses `hostlink` and `ledfx` |

Conductor's docs (Traditional Chinese): [design](docs/agentic/conductor.md),
[cheatsheet](docs/agentic/conductor-cheatsheet.md),
[changelog](docs/agentic/conductor-changelog.md); wire protocol:
[`host/conductor/PROTOCOL.md`](host/conductor/PROTOCOL.md).
Morse's docs: [design](docs/dev/morse.md), [cheatsheet](docs/dev/morse-cheatsheet.md);
wire protocol: [`host/morse/PROTOCOL.md`](host/morse/PROTOCOL.md).

```sh
just cond-run          # daemon: finds the FM-1 over USB or Bluetooth, talks to hc and Herdr
just cond-sim          # the daemon with a text stand-in, no FM-1 needed
just fw-test conductor # firmware in fm1-base's host simulator
just fw-build conductor  # the .fwsc package (flashing: see AGENTS.md, owner approval per write)
```

Versions: firmware X.Y, host X.Y.Z (Z for host-only updates, which need no reflash).
The [development environment](docs/dev/index.md) page covers the toolchain.

## Working on the notes

```sh
just docs-serve     # preview at http://127.0.0.1:8000
just docs-check     # strict build + output checks
```

Requires [uv](https://docs.astral.sh/uv/) and [just](https://just.systems/).
Agents: read [AGENTS.md](AGENTS.md) first, especially the hardware safety rules.

## Disclaimer

Unofficial. Not affiliated with M-VAVE or any firmware author. Flashing custom
firmware is at your own risk; these are one person's notes, not a guarantee.
