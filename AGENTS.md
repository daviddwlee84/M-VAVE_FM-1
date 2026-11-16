# AGENTS.md

Canonical instructions for any coding agent working in this repository.
`CLAUDE.md` only imports this file.

## What this repo is

A personal knowledge base for one **M-VAVE FM-1** pocket synth: resources,
flashing procedures, firmware notes (SLOOP first), usage notes, and the context
agents need to help operate the device safely. It is **not** a firmware source
tree. There is no application code; the only build is the documentation site.

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
just fm1-versions      # latest upstream releases, to refresh docs/resources.md and docs/watch.md

just fm1-fetch         # download M-Flasher + SLOOP into downloads/, verify against release digests
just fm1-find          # read-only: units in download mode / on USB MIDI
just fm1-info          # read-only: what is on the unit
just fm1-backup LABEL  # read-only: full-flash backup into backups/<date>-LABEL/
just fm1-verify-backups
just fm1-download-mode # mode switch
just fm1-restart       # mode switch
just fm1-flash FILE    # WRITE: owner runs this, never an agent without approval
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

## Sources and evidence

This ecosystem moves weekly. Trust, in order:

1. Upstream repos, their docs and release pages (fetch them; do not rely on memory).
2. [FM1 Developers' Wiki](https://sbegueria.github.io/fm1-dev-wiki/).
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
| Firmware write | `m-flasher firmware PACKAGE.fwsc`, browser installers | Only after the owner approves **that specific write in the current session**. Never pass `--yes`; the owner types M-Flasher's `yes`. |
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

## Not set up yet

Firmware build toolchain, local `fm1-emu` checkout, a repo-owned FM-1 skill and
an `fm1` CLI wrapper. They are described as later phases in
`docs/agentic/`; do not assume they exist.
