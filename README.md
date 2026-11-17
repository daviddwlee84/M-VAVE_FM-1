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
- [Resource map](docs/resources.md) — upstream repos and current versions
- [Device log](docs/device-log.md) — the state of my own unit

## Our own firmware + host

Built here on [fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0), MIT-licensed.
Each one is a firmware for the FM-1 plus, where it needs one, a program on the computer.

| Project | What it is | Firmware | Host | Status |
|---|---|---|---|---|
| **Conductor** | The FM-1 as a desk console for coding agents: one agent per key, urgency on the key LEDs, state and output on the screen (Chinese included), chimes, approve / deny / interrupt from the buttons, push-to-talk dictation, USB or Bluetooth | [`firmware/conductor/`](firmware/conductor/) | [`host/conductor/`](host/conductor/) (Python daemon for [herdr-core](https://github.com/herdrdev/herdr-core) `hc` and classic Herdr) | Firmware 0.3, host 0.3.2; on my unit since 2026-11-17 |
| `hello` | fm1-base's template, unmodified: the toolchain smoke test | [`firmware/hello/`](firmware/hello/) | — | Builds and boots in the emulator |
| Shared modules | `hostlink` (SysEx link to a host over USB / BLE), `ledfx` (LED patterns), `chime` (FM notification voice) | [`firmware/common/`](firmware/common/) | — | Used by Conductor |

Conductor's docs (Traditional Chinese): [design](docs/agentic/conductor.md),
[cheatsheet](docs/agentic/conductor-cheatsheet.md),
[changelog](docs/agentic/conductor-changelog.md); wire protocol:
[`host/conductor/PROTOCOL.md`](host/conductor/PROTOCOL.md).

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
