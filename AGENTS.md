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
| `firmware/morse/` | Submodule: [fm1-morse](https://github.com/daviddwlee84/fm1-morse), own pocket Morse code machine (GPL-3.0-or-later since 2026-11-18, MIT before; `common/` in it stays MIT; its own repo for The Firmware Hub, with `LICENSE` and `fmhub.json`). Built here with `third_party/fm1-base`; commit inside it, then bump the pointer here. Design: `docs/dev/morse.md`. |
| `firmware/common/` | Modules shared by own apps (`hostlink`, `ledfx`, `chime`, and the speech decoders `lpc`, `adpcm`; MIT). Apps add `include = ["../common"]`. |
| `firmware/theory/` | THEORY, own music theory trainer (GPL-3.0-or-later, identity `FM-1_49`, settings at `0xD8000`). v0.1 is the WHEEL page (circle of fifths, naming a played chord, the key's chords on the white keys), on the owner's unit since 2026-11-18 (installed over Morse 0.6, whose record at `0xDA000` it leaves alone). Its platform-free core `theory.c` (spelling, scales, keys, chords, Roman numerals, voicing) is checked against music21 (`tests/vectors.h`, generated). Design: `docs/dev/theory.md`. |
| `firmware/guitar/` | GUITAR, own guitar neck and chord book (GPL-3.0-or-later, identity `FM-1_55`, settings at `0xD4000`; separate firmware, decided 2026-11-18). v0.1: NECK (a key's scale on frets 0-12, CAGED / NPS positions, eight tunings) and SHAPE (chord diagrams found by search, easiest first). v0.2 (draft PR #3): power chords, SHAPE filters, LEARN (chord course by key in `lesson.c`, Leitner, progress in the same record). Includes THEORY's `theory.c`, `synth.c`, `gtext.c` from `../theory`. Its platform-free core `fret.c` has `just guitar-unit`. Verified in the host simulator and fm1-emu (both chips, rehearsed over a backup); not on the unit yet. Design: `docs/dev/guitar.md`. |
| `firmware/voicelab/` | Spike app (MIT): each key says a test word with the `lpc` / `adpcm` decoders. Not for installing. Its data (`gen/`) is generated and git-ignored. Design: `docs/dev/tts.md`. |
| `firmware/rhythm/` | RHYTHM, own rhythm trainer (GPL-3.0-or-later, identity `FM-1_54`, settings at `0xD2000`). v0.1: METRO (metronome, subdivisions, tap tempo, gap click) and ECHO (hear a bar or two, tap it back, per-onset ms error and a hint); verified in fm1-emu on both chips, not flashed yet. v0.2: one-line percussion notation, ECHO READ mode, ENDLESS (call and response without breaks, health bar, speeds up). Platform-free core `rhythm.c`. Design: `docs/dev/rhythm.md`. |
| `host/conductor/` | Conductor's host daemon (Python, MIT): backends `hc`, `herdr`, `fake`. Protocol spec in `PROTOCOL.md`; tests in `host/tests/`. |
| `host/morse/` | Morse's host (Python, MIT): types what the FM-1 reads into the Mac, sends text for it to play. Protocol in `PROTOCOL.md` (keying is Vail-format MIDI note 0). |
| `host/voicelab/` | Listening lab (Python, MIT): renders test words with every speech option (macOS `say`, Moonshine Klatt, 386DX, LPC, ADPCM, SPELLBOX in fm1-emu) into `build/voicelab/`, scores them with Whisper. GPL engines stay in `vendor/`; generated audio is never committed. |
| `third_party/fm1-base` | Git submodule (MPL-2.0, beta). Do not edit; bump deliberately. |
| `docs/dev/` | Firmware development environment and its current state. |
| `docs/hardware/flash-map.md` | Who stores what where in the 1 MiB flash, with sector diffs from this unit's backups. Update it when a new firmware is tried. |
| `docs/watch.md` | Ledger of upstream checks for things on hold (m-bootloader on the new chip, SLOOP 3). Append, do not rewrite. |
| `backups/`, `downloads/` | Git-ignored. Flash backups and downloaded installers/firmware. Never commit. Off-machine copies of backups go to GitHub Release assets (`just fm1-backup-release`). |
| `zensical.toml`, `scripts/check-docs-site.py`, `Justfile` | Site config, output checker, task runner. |
| `.agents/skills/` | Agent skills, linked from `.claude/skills/`: `zensical-site-bootstrap` (installed) and `fm1-hub-listing` (repo-owned: listing an own firmware on The Firmware Hub and the community lists). |

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
just fw-common-test    # firmware/common unit tests on this computer (hostlink BLE packets)

just morse-unit        # firmware/morse platform-free parts (morse.c, learn.c, content.c) on this computer
just morse-release X.Y # Morse's release package (identity FM-1_41 + XXYYZZ)
just morse-test        # Morse host unit tests (protocol vectors shared with tests/basic.txt)
just morse-emu [CHIP]  # Morse firmware in fm1-emu with the virtual USB cable
just morse-type [--dry-run]  # what the FM-1 reads, typed into the focused Mac app
just morse-send TEXT   # the FM-1 plays TEXT as received
just morse-backup FILE # Morse 0.4+: the learning progress into JSON (read-only on the unit)
just morse-restore FILE  # back onto the unit; it asks on its screen (PLAY yes)
just morse-shots       # simulator screenshots into fm1-morse docs/screens/

just theory-unit       # firmware/theory core on this computer (strict C99, plus music21's spellings)
just theory-emu [CHIP] # THEORY in fm1-emu (after `just fw-build theory`; `just fw-test theory` is its simulator test)
just theory-vectors    # regenerate firmware/theory/tests/vectors.h from music21 (uv fetches it)

just guitar-unit       # firmware/guitar/fret.c and lesson.c on this computer (tunings, positions, chord shape search, LEARN course)
just guitar-emu [CHIP] # GUITAR in fm1-emu (after `just fw-build guitar`; `just fw-test guitar` is its simulator test)

just voice-vendor      # Moonshine Klatt, python_wizard, CMUdict, 386DX engine into vendor/ (pinned commits)
just voice-fetch       # community firmwares that speak (Speak37, SPELLBOX, 386DX, ...), digest-verified, for the emulator
just voice-lab [ENGINE...]  # render host/voicelab/words.txt with every speech option -> build/voicelab/
just voice-score       # Whisper (base.en) as a stand-in listener, onto the page
just voice-open        # the listening page, build/voicelab/index.html
just voice-fw          # the test words into firmware/voicelab (gen/, git-ignored) + its host simulator test

just rhythm-unit       # firmware/rhythm core on this computer (strict C99)
just rhythm-emu [CHIP] # RHYTHM in fm1-emu (after `just fw-build rhythm`; `just fw-test rhythm` is its simulator test)

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

Versions (`docs/agentic/conductor-changelog.md`): firmware X.Y (`FW_MINOR` in
`firmware/conductor/app.c`), host X.Y.Z (`__version__` in
`host/conductor/__init__.py`, plus `FIRMWARE_KNOWN`). A host-only change bumps Z;
a firmware change bumps Y and resets the host to X.Y.0. The protocol byte changes
only for an incompatible change; additions are gated on the firmware version.
Every update adds a changelog row (version, date, reflash yes/no, what, commit),
updates the version in the "Our own firmware + host" tables (`README.md`,
`docs/index.md`), and starts its commit subject with the version.

The daemon presses keys in real agent sessions. When testing, send keys only to
terminals the test started itself (`just cond-it` labels them `conductor-test=1`
and kills them after). Never approve, deny or interrupt the owner's own agents
from a test. There is no auto-approve; do not add one without the owner asking.

## Not set up yet

A repo-owned skill for operating the unit and an `fm1` CLI wrapper (the `Justfile` covers it for now;
`fm1-hub-listing` covers publishing only). `fm1-emu` is a downloaded binary, not a source checkout.
A vocabulary app is at the idea stage (`docs/dev/vocab.md`). A deck written to the flash data area would need
the firmware-development rule "never compute flash addresses or call erase/program directly" changed first,
with the owner's approval; the proposed wording is in that page. THEORY (`docs/dev/theory.md`) has v0.1 but no learning yet;
its imported content is meant to go through the store layer (`APP_STORE_MAP`), which needs no rule change.
