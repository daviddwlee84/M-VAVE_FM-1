# Tasks for the notes site and for the FM-1 itself. Run `just` to list them.
#
# Device recipes follow the safety tiers in AGENTS.md:
#   read-only   fm1-find, fm1-info, fm1-backup, fm1-fetch, fm1-verify-backups, fm1-versions,
#               emu-* (the emulator never touches the unit)
#   mode switch fm1-download-mode, fm1-restart
#   write       fm1-flash   (owner only; M-Flasher asks for a typed "yes")
# There is deliberately no recipe for bootloader, restore or run-ram.

m_flasher := env("M_FLASHER", "/Applications/M-Flasher.app/Contents/MacOS/M-Flasher")
m_flasher_version := "0.12.1"
sloop_version := "2.8.5"
fm1_emu_version := "0.5.1"
agtdeck_version := "0.7"

default:
    @just --list

# ---- docs -------------------------------------------------------------------

# Build the site in strict mode.
docs-build:
    uv run --locked --group docs zensical build --strict

# Build, then verify the generated output (llms.txt, Markdown copies, search).
docs-check: docs-build
    uv run --locked --group docs python scripts/check-docs-site.py

# Live preview. `just docs-serve 8769` picks another port.
docs-serve port="8000" host="127.0.0.1":
    uv run --locked --group docs zensical serve --dev-addr "{{host}}:{{port}}"

# ---- setup ------------------------------------------------------------------

# Download M-Flasher and SLOOP into downloads/ and verify them against the release digests.
fm1-fetch:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p downloads && cd downloads
    fetch() { # repo tag asset
      gh release download "$2" -R "$1" -p "$3" --clobber
      want=$(gh api "repos/$1/releases/tags/$2" --jq ".assets[] | select(.name==\"$3\") | .digest" | sed 's/^sha256://')
      got=$(shasum -a 256 "$3" | cut -d' ' -f1)
      if [ -n "$want" ] && [ "$want" = "$got" ]; then echo "ok   $3  $got"; else echo "FAIL $3: got $got, release says ${want:-<no digest>}" >&2; exit 1; fi
    }
    fetch jvitkauskas/m-flasher "v{{m_flasher_version}}" "M-Flasher-{{m_flasher_version}}-macos.dmg"
    fetch isod89/sloop-fm1 "v{{sloop_version}}" "sloop-{{sloop_version}}.fwsc"
    fetch isod89/sloop-fm1 "v{{sloop_version}}" "sloop-editor-{{sloop_version}}.html"

# Open the M-Flasher disk image (drag the app to Applications yourself).
fm1-install-flasher: fm1-fetch
    open "downloads/M-Flasher-{{m_flasher_version}}-macos.dmg"

# Open the M-Flasher window.
fm1-gui:
    open -a M-Flasher

# Open the offline SLOOP web editor in Chrome.
sloop-editor:
    open -a "Google Chrome" "downloads/sloop-editor-{{sloop_version}}.html"

# ---- emulator (never touches the unit) ---------------------------------------

# Download fm1-emu (macOS arm64) into tools/fm1-emu/ and verify it against the release digest.
emu-fetch:
    #!/usr/bin/env bash
    set -euo pipefail
    a="fm1-emu-{{fm1_emu_version}}-macos-arm64.tar.gz"
    mkdir -p downloads tools/fm1-emu
    gh release download "v{{fm1_emu_version}}" -R joakimeriksson/fm1-emu -p "$a" -D downloads --clobber
    want=$(gh api "repos/joakimeriksson/fm1-emu/releases/tags/v{{fm1_emu_version}}" --jq ".assets[] | select(.name==\"$a\") | .digest" | sed 's/^sha256://')
    got=$(shasum -a 256 "downloads/$a" | cut -d' ' -f1)
    [ -n "$want" ] && [ "$want" = "$got" ] || { echo "FAIL $a: got $got, release says ${want:-<no digest>}" >&2; exit 1; }
    tar -xzf "downloads/$a" -C tools/fm1-emu --strip-components=1
    xattr -d com.apple.quarantine tools/fm1-emu/fm1-live tools/fm1-emu/fm1-emu 2>/dev/null || true
    tools/fm1-emu/fm1-live --version

# Download Agent Deck's package and verify its digest and build attestation.
agtdeck-fetch:
    #!/usr/bin/env bash
    set -euo pipefail
    a="agtdeck-{{agtdeck_version}}.fwsc"
    mkdir -p downloads
    gh release download "v{{agtdeck_version}}" -R kevinelliott/fm-1-agent-deck -p "$a" -D downloads --clobber
    want=$(gh api "repos/kevinelliott/fm-1-agent-deck/releases/tags/v{{agtdeck_version}}" --jq ".assets[] | select(.name==\"$a\") | .digest" | sed 's/^sha256://')
    got=$(shasum -a 256 "downloads/$a" | cut -d' ' -f1)
    [ -n "$want" ] && [ "$want" = "$got" ] || { echo "FAIL $a: got $got, release says ${want:-<no digest>}" >&2; exit 1; }
    gh attestation verify "downloads/$a" --repo kevinelliott/fm-1-agent-deck \
      --signer-workflow zvenson/fm1-build/.github/workflows/build.yml >/dev/null
    echo "ok   $a  $got  (attestation verified)"

# Play a .fwsc in a window. chip: uc25hq80 (this unit, default) or p25q80. Saves persist per package+chip.
emu-run package chip="uc25hq80" *args:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p tools/emu-state
    state="tools/emu-state/$(basename "{{package}}" .fwsc)-{{chip}}.flash"
    exec tools/fm1-emu/fm1-live --fwsc "{{package}}" --flash-chip "{{chip}}" --flash "$state" {{args}}

# Headless boot for `ms` emulated ms; writes the LCD to tools/emu-state/<name>-<chip>.ppm and reports flash writes.
emu-shot package chip="uc25hq80" ms="6000":
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p tools/emu-state/work
    name="$(basename "{{package}}" .fwsc)-{{chip}}"
    pkg="$(cd "$(dirname "{{package}}")" && pwd)/$(basename "{{package}}")"
    cd tools/emu-state
    ../fm1-emu/fm1-live --fwsc "$pkg" --flash-chip "{{chip}}" --flash "$name.flash" --midi none \
      --shot "{{ms}}" --lcd-only 2>&1 | grep -i -E "^flash:|fault|error" || true
    mv work/lcd.ppm "$name.ppm" && echo "LCD: tools/emu-state/$name.ppm"

# ---- upstream watch -----------------------------------------------------------

# Latest upstream releases, to compare with the version table in docs/resources.md.
fm1-versions:
    #!/usr/bin/env bash
    set -euo pipefail
    for r in jvitkauskas/m-flasher jvitkauskas/m-bootloader isod89/sloop-fm1 isod89/sloop3-fm1 \
             joakimeriksson/fm1-emu kevinelliott/fm-1-agent-deck charlesvestal/fm1-base charlesvestal/fm1-ctl; do
      printf '%-32s %s\n' "$r" "$(gh api "repos/$r/releases" --jq '.[0] | "\(.tag_name)  \(.published_at[0:10])\(if .prerelease then "  prerelease" else "" end)"' 2>/dev/null || echo '(no releases)')"
    done

# ---- device: read-only ------------------------------------------------------

# List units in download mode and units running on USB MIDI.
fm1-find:
    "{{m_flasher}}" find

# Show what is on the unit (needs download mode).
fm1-info:
    "{{m_flasher}}" info

# Full-flash backup into backups/<date>-<label>/, then print its SHA-256.
fm1-backup label="backup":
    #!/usr/bin/env bash
    set -euo pipefail
    dir="backups/$(date +%Y-%m-%d)-{{label}}"
    mkdir -p "$dir"
    "{{m_flasher}}" backup "$dir"
    shasum -a 256 "$dir"/*.bin
    echo "Record this in docs/device-log.md and keep a copy off this machine."

# Check every backup image against the SHA-256 stored in its .json.
fm1-verify-backups:
    #!/usr/bin/env bash
    set -euo pipefail
    shopt -s nullglob
    rc=0
    for bin in backups/*/*.bin; do
      want=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["sha256"])' "${bin%.bin}.json")
      got=$(shasum -a 256 "$bin" | cut -d' ' -f1)
      if [ "$want" = "$got" ]; then echo "ok   $bin"; else echo "FAIL $bin" >&2; rc=1; fi
    done
    exit $rc

# ---- device: mode switch ----------------------------------------------------

# Restart a running unit into download mode over USB MIDI (stock 015, Felucca).
fm1-download-mode:
    "{{m_flasher}}" download-mode

# Leave download mode and start the firmware. Does not touch the flash.
fm1-restart:
    "{{m_flasher}}" restart

# ---- device: write (owner only) ---------------------------------------------

# Install a .fwsc. M-Flasher shows the plan and waits for a typed "yes"; never add --yes.
[confirm("This writes firmware to the FM-1. A verified backup exists and the package supports this flash chip?")]
fm1-flash package:
    "{{m_flasher}}" firmware "{{package}}"

# ---- off-machine copies -----------------------------------------------------

# `dir` is the folder name under backups/ (a path to it also works).
# Upload one backups/<dir> (.bin + .json) as assets of a GitHub Release tagged backup-<dir>.
# The repo is public, so the image is downloadable by anyone: the owner's decision, per backup.
[confirm("Publish this full-flash backup as a PUBLIC release asset?")]
fm1-backup-release dir:
    #!/usr/bin/env bash
    set -euo pipefail
    name=$(basename "{{dir}}")   # accept a bare name or a path like backups/<name>/
    d="backups/$name"; tag="backup-$name"
    test -d "$d" || { echo "no such directory: $d" >&2; exit 1; }
    sums=$(cd "$d" && shasum -a 256 *.bin)
    notes=$(printf 'Full-flash backup of my own FM-1 (1 MiB), made with M-Flasher.\nDevice-specific: restore only to this unit. See docs/device-log.md.\n\nSHA-256:\n%s\n' "$sums")
    if gh release view "$tag" >/dev/null 2>&1; then
      gh release upload "$tag" "$d"/*.bin "$d"/*.json --clobber
    else
      gh release create "$tag" "$d"/*.bin "$d"/*.json --title "Backup $name" --notes "$notes" --latest=false
    fi
    gh release view "$tag" --json url,assets --jq '.url, (.assets[] | "\(.name) \(.digest)")'
