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
- [Resource map](docs/resources.md) — upstream repos and current versions
- [Device log](docs/device-log.md) — the state of my own unit

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
