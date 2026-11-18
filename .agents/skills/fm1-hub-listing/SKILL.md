---
name: fm1-hub-listing
description: 'Get an own M-VAVE FM-1 firmware (fm1-base) listed on The Firmware Hub and the community lists: its own repository with LICENSE and fmhub.json, a free FM-1_ identity, a release with the .fwsc, the three hardware tests over the USB-MIDI update, the submission form, then awesome-fm-1 and aroum/fm1-custom-fw. Use when the owner wants to publish, release or list a firmware from this repo (firmware/<app>), or asks what the Hub needs. Every firmware write, restore and public step needs the owner, per AGENTS.md.'
---

# Listing an FM-1 firmware on The Firmware Hub

Written from listing MORSE (github.com/daviddwlee84/fm1-morse) on 2026-11-18. The Hub's rules change: read
https://thefirmwarehub.com/fmhub.md and https://thefirmwarehub.com/submit/ again before relying on this. The
narrative, with the quotes and dates, is in `docs/firmware/hub.md#listing`, `docs/dev/morse.md#release` and
`docs/firmware/switching.md#paths`; the hardware steps are in `docs/device-log.md`.

## When to use

- The owner wants a firmware from `firmware/<app>/` on The Firmware Hub, or released publicly.
- The owner asks what the Hub needs, or why a submission's check fails.
- Not for installing other people's firmwares (see `just fm1-switch`), and not for SMK-37 or VIBE FM builds.

## Who does what

AGENTS.md's tiers hold throughout. The agent may build, test, back up, switch modes (say so first), read, and draft.
The owner approves each firmware write and runs it (they type M-Flasher's `yes` / fm1_install's `y`; never pass
`--yes`); a restore is the owner's alone, in M-Flasher's window. Creating a public repo, pushing to it, publishing a
release, opening PRs on other people's repositories and submitting to the Hub are public acts: do them only after
the owner says so for that step.

## The Hub's requirements (as of 2026-11-18)

Six, all needed ("Untested firmwares are not listed"):

1. Source public on GitHub, browsable at the tag, `LICENSE` in the repository root (GPL-3.0, MPL-2.0, MIT and other
   open licences are accepted).
2. A GitHub release with the `.fwsc` attached. The Hub follows the newest release with a `.fwsc` within three hours
   once listed; a 0.x version gets a beta tag.
3. An `FM-1_<number>` identity no other firmware uses ("Longer numbers with your own part are safer").
4. The update loader in the package (fm1-base's packages have it: the check says "Felucca's").
5. `fmhub.json` in the repository root, default branch. No file, no listing.
6. Tested on a real FM-1: installed from stock V15, installed from another firmware, and another firmware installed
   from it.

The Hub reads `fmhub.json`, not fm1-base's `firmware.json` (attach the latter to releases anyway; RemoteFM1 and
M-Flasher may use it). The `chip` field (new / old / any) is read from the package, not declared.

## Steps

### 1. Its own repository

The Hub keys one card to one repository root, so an app living in `firmware/<app>/` moves out:

```sh
git subtree split --prefix=firmware/<app> -b <app>-split     # history kept
git clone --branch <app>-split . /tmp/<repo> && cd /tmp/<repo> && git checkout -b main
```

- Copy in what it includes from `firmware/common/` (into `common/`, `include = ["common"]` in fm1.toml), add
  fm1-base as a submodule pinned to the commit it was built with, and `LICENSE`.
- `gh repo create <owner>/<repo> --public --source . --push` (owner's go-ahead), then in this repo `git rm -r
  firmware/<app>` and `git submodule add <url> firmware/<app>`. Keep `fw-setup` from recursing into the app's own
  fm1-base (init `third_party/fm1-base` recursively and the app non-recursively).
- Device builds must run from inside this repo: the JieLi toolchain runs in Docker with the repo mounted, and an app
  directory outside it (a scratch dir) fails with paths missing their leading slash. Build the submodule in place.

### 2. Identity

- fm1.toml `id_prefix` is `FM-1_` plus digits only (the update tools parse it). Dev builds are prefix + `00`,
  releases prefix + XXYYZZ (0.3 of `FM-1_41` is `FM-1_41000300`); USB answers the short form, `FM-1_` and three digits.
- Pick a prefix whose three-digit short forms nobody uses. Read `https://thefirmwarehub.com/catalog.json` at most
  once a day (the Hub asks) and list the `FM-1_` numbers in it; `FM-1_13` was too close to Midi-Mania's `FM-1_137`.
- Before submitting, the owner checks the full release identity at https://thefirmwarehub.com/submit/#check.
- A new identity installs as a different firmware but keeps the storage (APP_STORE_MAP), so progress survives.

### 3. fm1.toml and the release build

- Add `title` (else firmware.json's name falls back to `usb_manufacturer`), `install` (a page that installs it; the
  README's `#install` is enough), `screenshot` (a square PNG in the repo), `repo`, `licence`. `build.py --release`
  refuses without `install` and `screenshot`.
- If the app has screen-reader words, point `words.tsv`'s `# url:` at where `web/words/*.json` is served (e.g.
  raw.githubusercontent.com of the new repo) and commit `web/words/`.
- Build: `just morse-release X.Y` style recipe, i.e. `build.py firmware/<app> --release X.Y` → `<name>-X.Y.fwsc`.

### 4. fmhub.json

Only these fields: `what`, `author`, `kinds`, `play`, `editor`, `cheat`, `image`, `color`, `sounds` (and `smk37`).
Any other field makes the whole file unused.

- `what`: English, plain, 20-300 characters, no links, no `<>{}` or backticks. Longer is not refused but the card
  cuts it (MORSE's 350 ended at "Vail…"): count it.
- `author` (2-60 characters): the name on the card's "By:". Keep it the same as the form's "Your name".
- `kinds`: an array, first entry counts: synth, groove, drums, chords, tracker, other.
- `image`: a repo path; square, 240-1024 px, PNG/WebP/JPEG, at most 300 KB (a 240 x 240 simulator `shot` works).
- `sounds`: ask the owner; never guess. `"none"` only if every sound is generated in code and the repo holds no
  `.wav`, `.syx`, `.sf2` or sample arrays.
- `python3 -m json.tool fmhub.json`, then the owner runs the KB's "Check your file" (https://thefirmwarehub.com/kb/).
- `play` (the card's "Play in browser"): a URL. Most point at fm1-emu's web build, which plays the release package
  itself and reads the Hub's catalog, so a listed firmware is found by its Hub id with no copy of the package:
  - direct: `https://joakimeriksson.github.io/fm1-emu/?fw=<hub id>` (`&mhz=` sets the clock);
  - better: an own `web/try/` page on the repo's GitHub Pages with an iframe of `…/fm1-emu/?fw=<hub id>&embed=1`
    (only the synth), a few things to try, what the emulator cannot do (Bluetooth, MIDI out), a reload button and a
    full-page link. MORSE: `firmware/morse/web/try/index.html`.
  - `?package=URL&name=` plays a package from elsewhere, but needs https with CORS: GitHub Pages yes, release
    assets no. Do not pin a Hub file with it: the Hub deletes the old package on each release.
  - Before listing there is no Hub id yet: add `play` after the Hub lists the firmware (a card edit, live within
    three hours). Check it in a fresh browser: right after a release fm1-emu can fetch the package a stale cached
    catalog names and say `<NAME>: https://thefirmwarehub.com/fw/<id>-<old>.fwsc: 404`; reloading (or ten
    minutes, the catalog's `max-age=600`) fixes it. Note `fm1-live --shot` presses PLAY by itself unless given keys
    or `FM1_NO_PLAY=1`.
- `cheat`: a URL. A README anchor works, a GitHub Pages single page (printable, `@media print`) reads better.
  MORSE: `web/cheatsheet.html`, translated from the KB's Chinese cheatsheet; keep the two in step.

### 5. README

English: what it does, controls, install, licence, build. The Hub requires saying it if the firmware cannot read
other firmwares' projects or samples; also say where it stores its data (for fm1-base apps: their APP_STORE_MAP
offsets, and that an update stages its loader at `0xE0000`) and which flash chips it was tested on.

### 6. The three hardware tests, over the USB-MIDI update

The Hub's web installer does not use download mode: the running firmware reads the new package and starts its update
loader, which writes. Test that path with fm1-base's tool (one write each, preflight first, the owner runs it):

```sh
uv run --locked --all-groups python third_party/fm1-base/tools/fm1_install.py --info          # read-only
uv run --locked --all-groups python third_party/fm1-base/tools/fm1_install.py PACKAGE.fwsc   # owner runs it
```

`fm1_install.py` makes no backup: before each write, `just fm1-download-mode`, `just fm1-backup <label>`,
`just fm1-restart` (it needs the firmware running), and record the backup in `docs/device-log.md`.

1. **Leave it**: from the app, install another firmware (SLOOP 2.8.5 from `downloads/`, digest-verified).
2. **From another firmware**: from that one, install the app's release package. Check the app's data survived.
3. **From V15**: stock 015 is only in the owner's stock backup (m-vave.com offers 016, which overwrites the preset
   area). The owner restores `backups/2026-11-17-stock/...` in M-Flasher's window; read a backup back and compare it
   with the stock image (identical on 2026-11-18); then install the app's release from stock. The app's data is gone.

Loaders differ: fm1-base's writes only changed sectors ("fast install"); SLOOP 2.8.5's rewrites all ("classic").
Interrupting a write on purpose is in the KB's list but not the submit page's six; skip it on a unit without a
USB_KEY recovery unless the Hub asks.

### 7. Release

```sh
git -C firmware/<app> tag -a vX.Y -m "<NAME> X.Y" && git -C firmware/<app> push origin vX.Y
gh release create vX.Y -R <owner>/<repo> --verify-tag --title "<NAME> X.Y" --notes-file NOTES.md \
  firmware/<app>/build/<name>-X.Y.fwsc firmware/<app>/build/firmware.json
gh api repos/<owner>/<repo>/releases/tags/vX.Y --jq '.assets[] | "\(.name) \(.digest)"'   # = local SHA-256
```

Release the exact file the hardware tests installed. The notes give the identity, SHA-256, chips, licence and how it
was tested.

### 8. Submit (the owner)

The form at https://thefirmwarehub.com/submit/ (fmhub.md still mentions email; the form is the current path):

- "For which device?" M-VAVE FM-1; leave "Which device?" empty (it is for another device).
- The pink dashed box is the **VIBE Box Challenge** (VIBE FM SDK builds using something the FM-1 lacks, rules at
  https://thefirmwarehub.com/lab/rules/). An FM-1 firmware does not enter: leave it unticked, its fields empty.
- "How you tested it": from which firmware, back to which, and how (the tool, the chip).
- "Anything Sven should know": built on, licence, kind and why (a tool that makes sound filed as `other`; the KB says
  tools without sound are not listed), how the release was built (locally, so no built-from-source badge, which needs
  zvenson/fm1-build's workflow), chips tested.

The result page (its address is the owner's only; changes also come by mail) runs eight checks: source, LICENSE,
fmhub.json, sounds, release, identity, update loader, flash chip. All green means "waiting for Sven": the maintainer
reviews listings, but a fully green check with "tested" and "rights" ticked goes live by itself: MORSE was submitted
2026-11-17 19:03 UTC, checked 19:06, and went live automatically at 23:00 ("no click of Sven's"), on the hub with the
next sync. Fixes: push, then "Check again", or send the form again for the same repo (it updates).
After listing: new versions are new releases; card changes are fmhub.json edits (live within three hours). MORSE v0.5,
released 2026-11-18, was in `catalog.json` the same day (`beta: true` for 0.x). The catalog's `chip` said `new` while
the check page said both chips: read from the package, not tested.

### 9. Community lists (owner's go-ahead per PR)

| List | Weight (2026-11-18) | How |
|---|---|---|
| [aroum/fm1-custom-fw](https://github.com/aroum/fm1-custom-fw) | 424 stars, most outside "Add ..." PRs merged | PR: a numbered table row, `**Name by Author**<br>[GitHub](url) \| ...<br>**License:** ...` and a Concept / Features cell; fm1-base tools (CTL-1, MIDIMON) sit in 1.1.4 |
| [cicloid/awesome-fm-1](https://github.com/cicloid/awesome-fm-1) | 93 stars, outside PRs merged | PR per CONTRIBUTING.md: `- [Name](url) - Description.` one line, author and licence, one entry per PR; tools go under *Not a synth* |
| fm1-editor.com firmware list | 75 stars (benny-sparra/fm1-dx7-patch-importer) | an issue there, if it does not pick the Hub up by itself |
| fm1-emu, RemoteFM1 player, FMDB | — | follow the Hub or crawl GitHub: nothing to send |
| FM-1 Firmware Atlas, the Developers' Wiki | 0 / 5 stars; the wiki is not a firmware list | skip |

Announcements happen on r/MVaveFM1. Do not write `@name` in PR text.

## Pitfalls met

- `just fw-build` and `just morse-release` run `fw-setup` first, whose `git submodule update` checks the app
  submodule out at the commit this repo records (detached HEAD): commit inside the app, then commit the new pointer
  here, then build. Otherwise the package is built from the old commit while the host simulator (run on the working
  tree) passes. Check a release in fm1-emu for the change itself before handing it to the owner.

- The Hub's CONDUCTOR is another author's (MammothCheese); names and ids collide across projects: search the
  catalog before naming.
- A python patch that does `open(p, "w").write(open(p).read())` empties the file: the write-open truncates first.
- An app's version bump means every shared vector with the firmware version (HELLO bytes) changes in both the
  firmware test and the host's protocol.
- Licence can change later (the owner is the only author), but versions already published keep theirs; say so in
  the README (MORSE: MIT up to 57ea786, GPL-3.0-or-later after).
