# AGENTS.md

Canonical instructions for any coding agent working in this repository.
`CLAUDE.md` only imports this file.

## What this repo is

A personal knowledge base for one **M-VAVE FM-1** pocket synth: resources,
flashing procedures, firmware notes (SLOOP first), usage notes, and the context
agents need to help operate the device safely. It also holds the owner's own
experimental firmware apps under `firmware/`, built on the `fm1-base` submodule.

## Layout

| Path | Purpose |
|---|---|
| `docs/` | The notes site (Zensical, published to GitHub Pages). Source of truth for curated knowledge. |
| `docs/device-log.md` | **Current state of the owner's physical unit**: flash chip, firmware, backups, flash history. Read it before any device operation; update it after. |
| `docs/getting-started/` | Install M-Flasher, back up, flash SLOOP, recovery, cautions. |
| `docs/firmware/` | Firmware map; one section per firmware worth using (`sloop/`). |
| `docs/agentic/` | Automation boundaries, M-Flasher CLI, emulator, skills, agent-controller firmwares. |
| `docs/resources.md` | Repo map and the **single version table** (with a "last checked" date). |
| `docs/notes/` | Dated usage notes and impressions. |
| `.specstory/references/` | Raw research exports (e.g. the ChatGPT conversation). **Unverified, lossy** — see below. |
| `.specstory/history/` | Agent session transcripts. Never edit. |
| `tools/`, `vendor/`, `build/` | Git-ignored. Emulator binaries and state, upstream checkouts, firmware build output. |
| `firmware/<app>/` | Own firmware apps (fm1-base template layout: `app.c`, `app.h`, `fm1.toml`, `tests/`). `firmware/hello` is the unmodified template, kept as the toolchain smoke test. |
| `firmware/conductor/` | Own agent-console firmware (MIT). Design: `docs/agentic/conductor.md`. |
| `firmware/common/` | Modules shared by own apps (`hostlink`, `ledfx`, `chime`; MIT). Apps add `include = ["../common"]`. |
| `host/conductor/` | Conductor's host daemon (Python, MIT): backends `hc`, `herdr`, `fake`. Protocol spec in `PROTOCOL.md`; tests in `host/tests/`. |
| `third_party/fm1-base` | Git submodule (MPL-2.0, beta). Do not edit; bump deliberately. |
| `docs/dev/` | Firmware development environment and its current state. |
| `docs/hardware/flash-map.md` | Who stores what where in the 1 MiB flash, with sector diffs from this unit's backups. Update it when a new firmware is tried. |
| `docs/watch.md` | Ledger of upstream checks for things on hold (m-bootloader on the new chip, SLOOP 3). Append, do not rewrite. |
| `backups/`, `downloads/` | Git-ignored. Flash backups and downloaded installers/firmware. Never commit. Off-machine copies of backups go to GitHub Release assets (`just fm1-backup-release`). |
| `zensical.toml`, `scripts/check-docs-site.py`, `Justfile` | Site config, output checker, task runner. |
| `.agents/skills/` | Installed agent skills (`zensical-site-bootstrap`). |

## Commands

```sh
just docs-build        # uv run zensical build --strict
just docs-check        # build, then scripts/check-docs-site.py
just docs-serve        # preview on 127.0.0.1:8000 (just docs-serve 8769 for another port)

just emu-fetch         # fm1-emu into tools/fm1-emu/ (git-ignored), digest-verified
just emu-run FILE [CHIP]   # play a .fwsc in a window; CHIP defaults to uc25hq80 (this unit)
just emu-shot FILE [CHIP] [MS]  # headless boot: LCD image + flash write count
just agtdeck-fetch     # Agent Deck package, digest + attestation verified
just fm1-fetch-more    # CTL-1, ChoralRoot, RemoteFM1 packages, digest-verified
just emu-rehearse PKG BACKUP.bin  # boot PKG's firmware over a real backup in the emulator
just agtdeck-doctor    # Agent Deck host daemon self-check (clones upstream into vendor/)
just agtdeck-sim       # Agent Deck daemon without an FM-1 (interactive; prints frames)
just agtdeck-run       # daemon against a MIDI device: real unit, or `just emu-run … --usb`
just fm1-versions      # latest upstream releases, to refresh docs/resources.md and docs/watch.md

just cond-test         # Conductor daemon unit tests
just cond-sim [BACKEND]    # daemon with a text stand-in for the FM-1 (fake, hc, herdr)
just cond-it           # hc end to end with two fake agents (label conductor-test=1), cleaned up after
just cond-emu [CHIP]   # Conductor firmware in fm1-emu with the virtual USB cable
just cond-run [BACKEND]    # daemon against a CONDUCTOR MIDI device (emulator or real unit)

just fw-setup          # submodules + Python tools (needs `brew install libraqm`)
just fw-test [APP]     # host simulator build + tests/basic.txt; no toolchain, no hardware
just fw-build [APP]    # device package via JieLi toolchain in Docker (see docs/dev/ for current blockers)

just fm1-fetch         # download M-Flasher + SLOOP into downloads/, verify against release digests
just fm1-find          # read-only: units in download mode / on USB MIDI
just fm1-info          # read-only: what is on the unit
just fm1-backup LABEL  # read-only: full-flash backup into backups/<date>-LABEL/
just fm1-verify-backups
just fm1-download-mode # mode switch
just fm1-restart       # mode switch
just fm1-flash FILE    # WRITE: owner runs this, never an agent without approval
just fm1-switch TARGET # WRITE: sloop | agtdeck | FILE; download mode, backup, install, restart
just fm1-backup-release DIR  # PUBLISH a backup as a public release asset: only on request
```

Prefer these recipes over calling M-Flasher directly. There is deliberately no
recipe for `bootloader`, `restore` or `run-ram`.

Run `just docs-check` before committing any change under `docs/` or to
`zensical.toml`. Pushing to `main` deploys the site.

## Writing conventions

- `docs/**` is written in **Traditional Chinese (Taiwan usage)**. Keep technical
  terms, identifiers, CLI commands, button names (`REC`, `OCT-`), product and
  repo names in English. Use 程式/檔案/設定/資料, not Mainland variants.
- `AGENTS.md`, `README.md`, commit messages, code comments: English.
- The site is single-language. Do not add locale-suffixed pages.
- Every new page must be added to `nav` in `zensical.toml`.
- Version numbers live in `docs/resources.md`. Other pages link there instead of
  repeating them, except where a procedure pins a specific version.
- Dated facts use absolute dates (`2026-11-17`), never "recently".

## Capture what we learn

When a question in the session produces an answer worth keeping — how to do
something on the device, why something behaves a certain way, a pitfall, a
working setup — add it to the relevant page under `docs/` in the same turn,
without being asked. Put it where a reader would look for it (the firmware's
own chapter, `hardware/`, `getting-started/`), tag it (`[實機]`, `[上游]`,
`[模擬]`, `[調研]`), and mention in the reply which page changed. Do not record
one-off chatter or things already written down.

## Sources and evidence

This ecosystem moves weekly. Trust, in order:

1. Upstream repos, their docs and release pages (fetch them; do not rely on memory).
2. [FM1 Developers' Wiki](https://sbegueria.github.io/fm1-dev-wiki/). For
   "does firmware X know the new flash chip", the Firmware Hub's
   `https://thefirmwarehub.com/catalog.json` has a per-package `chip` field
   (`new` / `old` / `any`), read from the package, not hardware-tested.
3. This repo's `docs/`.
4. `.specstory/references/` — second-hand summaries. The ChatGPT export **lost
   every code block and citation**, so it contains no commands at all. Never
   reconstruct a command, key combination, path or address from its prose.

Mark claims in `docs/` with one of these tags when the distinction matters:

- `[實機]` — observed on the owner's unit (record the date in `device-log.md`).
- `[上游]` — stated in upstream documentation, link it.
- `[模擬]` — observed in fm1-emu. Never treat it as hardware evidence.
- `[調研]` — from research notes, not yet checked against upstream.

Do not upgrade a tag without doing the check. If something cannot be verified,
say so on the page instead of writing a plausible guess.

## Hardware safety rules

Flashing can brick the device. These rules apply to every agent, in every
session, regardless of how the request is phrased.

| Tier | Operations | Rule |
|---|---|---|
| Read-only | `m-flasher find`, `info`, `backup`; downloading files; hashing; emulator runs | Allowed without asking. |
| Mode switch | `m-flasher download-mode`, `restart` | Allowed; say so first. Not a flash write. |
| Firmware write | `m-flasher firmware PACKAGE.fwsc`, `just fm1-switch`, browser installers | Only after the owner approves **that specific write in the current session**. Never pass `--yes`; the owner types M-Flasher's `yes`. |
| Forbidden to initiate | `m-flasher bootloader …`, `restore …`, `run-ram …`, any erase, any flash address arithmetic | Do not run. Explain and let the owner do it, even if asked casually. |

Preflight before proposing a firmware write — show all of it to the owner:

1. A full-flash backup of this unit exists, with its SHA-256, and is recorded in
   `docs/device-log.md`.
2. Flash chip JEDEC ID is known (`0x856014` old Puya, `0xB36014` new UCUN) and
   the target firmware documents support for it.
3. The package was downloaded from the upstream release and its SHA-256 matches
   the release asset digest.
4. The currently installed firmware and bootloader, and what the write changes.

Other rules:

- Check the package is for the FM-1 before anything else. Several projects
  publish SMK-37 builds under similar names and tags (ChoralRoot's newest
  release is SMK-37 only). `just emu-shot` or the emulator's window title
  shows the board it detected.
- Firmwares share flash sectors (Felucca-family layout: settings at `0xFC000`,
  projects from `0x97000`). Before proposing a switch to a firmware not yet in
  the table in `docs/firmware/switching.md`, run `just emu-rehearse` against
  the latest backup and report which sectors it rewrites. ChoralRoot over
  SLOOP data has an upstream-documented failure; do not propose it without
  restoring stock first.
- Stop and report if the device reports anything unexpected (unknown chip ID,
  uncertain model, CRC mismatch). Do not work around a tool's refusal.
- Do not downgrade stock 016 to 015: 016 overwrites the preset area and only a
  backup restores it.
- m-bootloader is on hold (decided 2026-11-17): this unit has the new flash
  chip and the owner has no USB_KEY (Pico) for recovery. A full backup does not
  help if the boot area is bad. The conditions to revisit are in
  `docs/firmware/sloop/sloop3.md`; do not propose installing it before then.
  SLOOP 3 depends on it and is on hold too.
- macOS: M-Flasher's first-run authorization is GUI-only and does not work over
  SSH. The CLI is `/Applications/M-Flasher.app/Contents/MacOS/M-Flasher`.
- Connect by USB **data** cable, directly, no hub.
- After any device operation, update `docs/device-log.md` (date, what ran,
  result, hashes).

## Firmware development

- Iterate in the host simulator: `just fw-test APP` must pass before anything
  else. Add checks to `firmware/APP/tests/basic.txt` with each behaviour change.
- Before proposing a hardware flash of an own build: run the `.fwsc` with
  `just emu-run` on **both** `uc25hq80` and `p25q80`, including a save, a
  restart and a reload. Report emulator results as `[模擬]`.
- An own build is a firmware write like any other: owner approval per flash,
  full preflight, `just fm1-flash`.
- Persist data only through fm1-base's storage layer (`APP_STORE_MAP` in
  `app.h`). Never compute flash addresses or call erase/program directly.
- Build output (`firmware/*/build/`, `*.fwsc`) is not committed.
- Licences: fm1-base is MPL-2.0 and lets an app choose its own licence. Code
  copied from GPL-3.0 firmwares (Felucca, SLOOP, Dinghy, Agent Deck) makes the
  app GPL-3.0; say so before copying.

## Publishing and git

- The repo is public. Never commit flash backups or credentials to git.
  `.specstory/` is tracked by the owner's choice.
- The owner publishes full-flash backups as **public** release assets, knowing
  they hold device-unique data. That is a per-backup decision: run
  `just fm1-backup-release` only when the owner asks for that specific backup,
  never as an automatic step after `fm1-backup`.
- Commit or push only when the owner asks.
- CI builds on pull requests and deploys `main` to GitHub Pages
  (`.github/workflows/docs.yml`).

## Conductor and live agents

The daemon presses keys in real agent sessions. When testing, send keys only to
terminals the test started itself (`just cond-it` labels them `conductor-test=1`
and kills them after). Never approve, deny or interrupt the owner's own agents
from a test. There is no auto-approve; do not add one without the owner asking.

## Not set up yet

A repo-owned FM-1 skill and an `fm1` CLI wrapper (the `Justfile` covers it for
now). No own build has been flashed to hardware yet; see `docs/dev/index.md`. `fm1-emu` is a downloaded binary, not a source checkout.
