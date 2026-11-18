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
- [Resource map](docs/resources.md) — upstream repos and current versions
- [Device log](docs/device-log.md) — the state of my own unit

## Our own firmware + host

Built here on [fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0). Conductor and the shared modules are MIT; the Morse, THEORY and GUITAR firmwares are GPL-3.0-or-later (Morse's host program here is MIT).
Each one is a firmware for the FM-1 plus, where it needs one, a program on the computer.

| Project | What it is | Firmware | Host | Status |
|---|---|---|---|---|
| **Conductor** | The FM-1 as a desk console for coding agents: one agent per key, urgency on the key LEDs, state and output on the screen (Chinese included), chimes, approve / deny / interrupt from the buttons, push-to-talk dictation, USB or Bluetooth | [`firmware/conductor/`](firmware/conductor/) | [`host/conductor/`](host/conductor/) (Python daemon for [herdr-core](https://github.com/herdrdev/herdr-core) `hc` and classic Herdr) | Firmware 0.3, host 0.3.2; ran on my unit 2026-11-17, now switched to Morse |
| **Morse** | A pocket Morse code machine: key on the piano keys (straight key or iambic paddles) and read what you sent, type letters on the keys and hear them, flash cards with spaced repetition (by sound by default; keyed answers judged by their own rhythm), a placement test, progress you can edit, back up and restore, silent mode with the lights, USB or Bluetooth MIDI in the Vail adapter's format | [`firmware/morse/`](firmware/morse/) | [`host/morse/`](host/morse/) (types what you key into the computer; sends text for the FM-1 to play) | Firmware 0.8 (SAVE saves at once), host 0.8.0; on my unit 2026-11-17 to 2026-11-18, now switched to THEORY (its progress stays at `0xDA000`); [learning with it](docs/dev/morse-learn.md) |
| **THEORY** | A pocket music theory trainer. v0.1, the WHEEL: the circle of fifths with the key's chords marked; play a chord and it is named with its Roman or Nashville number in the key (secondary dominants, borrowed chords, inversions); or the white keys play the key's own chords, voiced to move least; nine modes, every key spelled right. 0.2 adds flash cards a round a day (scales, key signatures, chords by degree, degrees and qualities by ear), a page naming whatever you play, progress on the circle, backup. 0.3 explains each kind of card and lets you play new ones along, lights answer notes one by one (they keep sounding), asks less of the same, adds PITCH (absolute pitch games), a round of one key from the chart, and a latch mode for naming chords | [`firmware/theory/`](firmware/theory/) | [`host/theory/`](host/theory/) (progress backup / restore; a [web page](docs/assets/theory/progress.html) too) | Firmware 0.2 on my unit since 2026-11-18 (Morse's progress kept for switching back); 0.3 built and tested in the simulator and emulator, host 0.3.0; [design](docs/dev/theory.md) |
| **GUITAR** | A pocket guitar neck and chord book. v0.1: NECK, a key's scale on frets 0-12, whole or one CAGED / NPS position at a time, on guitar, bass, ukulele or mandolin in eight tunings; play a note and every place it lies lights up. SHAPE: chord diagrams with fingers and barres found by search, easiest first; play a chord to see its shapes, PLAY strums it | [`firmware/guitar/`](firmware/guitar/) | — | Firmware 0.1, simulator and fm1-emu only; [design](docs/dev/guitar.md) |
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
