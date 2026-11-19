# Tasks for the notes site and for the FM-1 itself. Run `just` to list them.
#
# Device recipes follow the safety tiers in AGENTS.md:
#   read-only   fm1-find, fm1-info, fm1-backup, fm1-fetch, fm1-verify-backups, fm1-versions,
#               emu-* (the emulator never touches the unit)
#   mode switch fm1-download-mode, fm1-restart
#   write       fm1-flash, fm1-switch   (owner only; M-Flasher asks for a typed "yes")
# There is deliberately no recipe for bootloader, restore or run-ram.

m_flasher := env("M_FLASHER", "/Applications/M-Flasher.app/Contents/MacOS/M-Flasher")
m_flasher_version := "0.12.1"
sloop_version := "2.8.5"
fm1_emu_version := "0.5.1"
agtdeck_version := "0.7"
ctl_version := "1.0.3"
choralroot_tag := "v0.41"
choralroot_pkg := "choralroot-0.41.fwsc"
remote_version := "0.3.6"
jamtrack_version := "0.2.3"
polly_version := "0.6.4"
cord_version := "1.0.3"
# firmwares that speak (docs/dev/tts.md): listened to in the emulator only
speak37_version := "1.0"
spellbox_version := "0.3"
dx386_version := "2.0"
chimera_version := "0.8.0"
evilgenius_version := "0.1.4"

default:
    @just --list

# ---- docs -------------------------------------------------------------------

# Build the site in strict mode.
docs-build:
    uv run --locked --all-groups zensical build --strict

# Build, then verify the generated output (llms.txt, Markdown copies, search).
docs-check: docs-build
    uv run --locked --all-groups python scripts/check-docs-site.py

# Live preview. `just docs-serve 8769` picks another port.
docs-serve port="8000" host="127.0.0.1":
    uv run --locked --all-groups zensical serve --dev-addr "{{host}}:{{port}}"

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

# Download the other firmwares I try (CTL-1, ChoralRoot, RemoteFM1, and the backing-band ones in docs/firmware/jam.md) and verify them against the release digests.
fm1-fetch-more:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p downloads && cd downloads
    fetch() { # repo tag asset
      gh release download "$2" -R "$1" -p "$3" --clobber
      want=$(gh api "repos/$1/releases/tags/$2" --jq ".assets[] | select(.name==\"$3\") | .digest" | sed 's/^sha256://')
      got=$(shasum -a 256 "$3" | cut -d' ' -f1)
      if [ -n "$want" ] && [ "$want" = "$got" ]; then echo "ok   $3  $got"; else echo "FAIL $3: got $got, release says ${want:-<no digest>}" >&2; exit 1; fi
    }
    fetch charlesvestal/fm1-ctl "v{{ctl_version}}" "ctl-{{ctl_version}}.fwsc"
    fetch Quixotic7/ChoralRootFM1 "{{choralroot_tag}}" "{{choralroot_pkg}}"
    fetch Quixotic7/RemoteFM1 "v{{remote_version}}" "remote-{{remote_version}}.fwsc"
    fetch Philip-Bos/jamtrack-fm1 "v{{jamtrack_version}}" "jamtrack-{{jamtrack_version}}.fwsc"
    fetch charlesvestal/fm1-polly "v{{polly_version}}" "polly-{{polly_version}}.fwsc"
    fetch chasedeanda/cord-fm1 "v{{cord_version}}" "cord-{{cord_version}}.fwsc"

# Download the community firmwares that speak (docs/dev/tts.md) and verify them against the release digests.
# For listening in the emulator (`just emu-run downloads/FILE`); none is planned for the unit.
voice-fetch:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p downloads && cd downloads
    fetch() { # repo tag asset
      gh release download "$2" -R "$1" -p "$3" --clobber
      want=$(gh api "repos/$1/releases/tags/$2" --jq ".assets[] | select(.name==\"$3\") | .digest" | sed 's/^sha256://')
      got=$(shasum -a 256 "$3" | cut -d' ' -f1)
      if [ -n "$want" ] && [ "$want" = "$got" ]; then echo "ok   $3  $got"; else echo "FAIL $3: got $got, release says ${want:-<no digest>}" >&2; exit 1; fi
    }
    fetch genkigenki/Speak37 "v{{speak37_version}}" "speak37-fm1-{{speak37_version}}.fwsc"
    fetch pjac1810/spellbox "v{{spellbox_version}}" "spellbox-{{spellbox_version}}.fwsc"
    fetch seajaysec/386-DX-FM1-KaraokeBot "v{{dx386_version}}" "386dx-{{dx386_version}}.fwsc"
    fetch shan-sherif/fm-1-chimera-vocal "v{{chimera_version}}" "chimera-{{chimera_version}}.fwsc"
    fetch Quixotic7/evilgenius "v{{evilgenius_version}}" "evilgenius-{{evilgenius_version}}.fwsc"

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
    mkdir -p tools/emu-state/work
    state="tools/emu-state/$(basename "{{package}}" .fwsc)-{{chip}}.flash"
    if [ -f "$state" ]; then
      # fm1-live boots the firmware the state file holds and installs the package only into a new one: after a
      # rebuild, lay the package's firmware over the state (its data kept), as an update on the unit would
      fresh="$(mktemp -d)/pristine.flash"
      (cd tools/emu-state && ../fm1-emu/fm1-live --fwsc "$(cd "$OLDPWD" && cd "$(dirname "{{package}}")" && pwd)/$(basename "{{package}}")" \
        --flash-chip "{{chip}}" --flash "$fresh" --midi none --shot 1 --lcd-only >/dev/null 2>&1)
      python3 scripts/emu_rehearse.py "$fresh" "$state" "$state.new" >/dev/null
      if cmp -s "$state" "$state.new"; then rm -f "$state.new"
      else mv "$state.new" "$state"; echo "emu-run: $(basename "{{package}}") laid over the saved state (its data kept)"; fi
    fi
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

# Rehearse a switch in the emulator: lay PACKAGE over a real BACKUP (.bin) as M-Flasher would, then boot it.
# Shows how the new firmware copes with the data the old one left behind. Extra args go to fm1-live.
emu-rehearse package backup chip="uc25hq80" *args:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p tools/emu-state/work
    name="$(basename "{{package}}" .fwsc)"
    pkg="$(cd "$(dirname "{{package}}")" && pwd)/$(basename "{{package}}")"
    bak="$(cd "$(dirname "{{backup}}")" && pwd)/$(basename "{{backup}}")"
    cd tools/emu-state
    rm -f "pristine-$name.flash"
    ../fm1-emu/fm1-live --fwsc "$pkg" --flash-chip "{{chip}}" --flash "pristine-$name.flash" --midi none --shot 1 --lcd-only >/dev/null 2>&1
    python3 ../../scripts/emu_rehearse.py "pristine-$name.flash" "$bak" "rehearse-$name.flash"
    cp "rehearse-$name.flash" "rehearse-$name.before"
    echo "booting $name over $(basename "$bak") (window; Esc quits). State: tools/emu-state/rehearse-$name.flash"
    ../fm1-emu/fm1-live --fwsc "$pkg" --flash-chip "{{chip}}" --flash "rehearse-$name.flash" {{args}}

# ---- Agent Deck host daemon (no flashing) ------------------------------------

# Clone Agent Deck at the pinned tag into vendor/ (git-ignored).
agtdeck-clone:
    test -d vendor/fm-1-agent-deck || git clone -q --depth 1 --branch "v{{agtdeck_version}}" https://github.com/kevinelliott/fm-1-agent-deck.git vendor/fm-1-agent-deck

# Check the daemon's view of this machine: Python, MIDI, config, agent CLIs, Herdr.
agtdeck-doctor backend="herdr": agtdeck-clone
    python3 vendor/fm-1-agent-deck/host/agtdeck.py --backend "{{backend}}" --doctor --no-wait

# Run the daemon without an FM-1: prints frames, reads commands from stdin (`quit` to leave).
agtdeck-sim backend="herdr": agtdeck-clone
    python3 vendor/fm-1-agent-deck/host/agtdeck.py --backend "{{backend}}" --simulate

# ---- own firmware (firmware/<app>, on third_party/fm1-base) -------------------

fm1_base := "third_party/fm1-base"

# One-time: submodules and the Python tools. Needs `brew install libraqm` (Pillow is built against it).
fw-setup:
    git submodule update --init --recursive third_party/fm1-base
    git submodule update --init firmware/morse
    uv sync --locked --all-groups

# Build an app's host simulator (no toolchain needed) and run its test script.
fw-test app="hello": fw-setup
    #!/usr/bin/env bash
    set -euo pipefail
    uv run --locked --all-groups python "{{fm1_base}}/tools/build.py" "firmware/{{app}}" --host
    bin=$(ls firmware/{{app}}/build/host/*_host)
    cd "firmware/{{app}}" && "build/host/$(basename "$bin")" tests/basic.txt build/host
    echo "screenshots and audio: firmware/{{app}}/build/host/"

# One-time for device builds: JieLi toolchain and the pinned AC79 SDK into tools/ (about 1.2 GB).
fw-toolchain:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p tools/jieli
    test -e tools/jieli/toolchain || sh "{{fm1_base}}/tools/get_toolchain.sh" "$PWD/tools/jieli"
    test -d tools/fw-AC79_AIoT_SDK || git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
      https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git tools/fw-AC79_AIoT_SDK

# Build an app's device package (firmware/<app>/build/*.fwsc). On macOS the tools run in a linux/amd64 container.
fw-build app="hello": fw-setup fw-toolchain
    JIELI_TOOLCHAIN="$PWD/tools/jieli/toolchain" AC79_SDK="$PWD/tools/fw-AC79_AIoT_SDK" \
      uv run --locked --all-groups python "{{fm1_base}}/tools/build.py" "firmware/{{app}}"

# A package to try in the browser, in fm1-emu (the .fwsc itself, emulated): firmware/APP/build/APP.fwsc (just fw-build
# APP first) onto the `previews` branch as SLOT/APP.fwsc (SLOT: pr-N, or main), pushed; prints the public link.
# PUBLISHES to GitHub. The branch is always one commit, force-pushed (with a lease: a parallel push is not lost), so
# old packages do not pile up in the repo's history. `just fw-preview APP pr-N --remove` takes one off.
fw-preview app slot *flags:
    #!/usr/bin/env bash
    set -euo pipefail
    pkg="$PWD/firmware/{{app}}/build/{{app}}.fwsc"
    src="$(git rev-parse --short HEAD)"
    [ "{{flags}}" = "--remove" ] || test -f "$pkg" || { echo "fw-preview: no $pkg (just fw-build {{app}})"; exit 1; }
    w="$(mktemp -d)/previews"
    git fetch -q origin previews
    lease="$(git rev-parse origin/previews)"
    git worktree add -q --detach "$w" origin/previews
    trap 'git worktree remove --force "$w"' EXIT
    cd "$w"
    if [ "{{flags}}" = "--remove" ]; then
      git rm -q -r --ignore-unmatch "{{slot}}"
    else
      mkdir -p "{{slot}}" && cp "$pkg" "{{slot}}/{{app}}.fwsc" && git add "{{slot}}"
    fi
    git diff --cached --quiet && { echo "fw-preview: nothing changed"; exit 0; }
    tree="$(git write-tree)"
    commit="$(git commit-tree "$tree" -m "previews: packages to try in fm1-emu (last: {{app}} {{slot}} {{flags}} from $src)")"
    git push -q --force-with-lease=previews:"$lease" origin "$commit":refs/heads/previews
    [ "{{flags}}" = "--remove" ] || echo "try it: https://joakimeriksson.github.io/fm1-emu/?package=https://raw.githubusercontent.com/daviddwlee84/M-VAVE_FM-1/previews/{{slot}}/{{app}}.fwsc&name={{app}}&chip=uc25hq80"

# The browser emulator (the same app in WebAssembly) into firmware/APP/build/emu/: open its index.html over http, or
# publish that folder for a PR's "try it" link. emcc from emsdk if installed, else Docker (scripts/emcc-docker.sh).
fw-web app="hello": fw-setup
    #!/usr/bin/env bash
    set -euo pipefail
    if ! command -v emcc >/dev/null; then
      d="$(mktemp -d)"; ln -s "$PWD/scripts/emcc-docker.sh" "$d/emcc"; export PATH="$d:$PATH"
    fi
    uv run --locked --all-groups python "{{fm1_base}}/tools/build.py" "firmware/{{app}}" --web
    echo "fw-web: firmware/{{app}}/build/emu/ (python3 -m http.server -d firmware/{{app}}/build/emu, then open it)"

# ---- Conductor: own agent console (firmware/conductor + host/conductor) ------

# Host daemon unit tests (protocol, slot memory, action checks; no hardware, no backend).
cond-test:
    uv run --locked --all-groups pytest -q

# The daemon against a text stand-in for the FM-1 (backend: fake, auto, hc or herdr).
cond-sim backend="fake" *args:
    PYTHONPATH=host uv run --locked --all-groups python -m conductor --backend "{{backend}}" --simulate {{args}}

# The daemon against a MIDI device named CONDUCTOR: the real unit, or `just cond-emu`.
# auto (default): hc and classic Herdr together.
cond-run backend="auto" *args:
    PYTHONPATH=host uv run --locked --all-groups python -m conductor --backend "{{backend}}" {{args}}

# macOS's Bluetooth MIDI window, opened by itself (click Connect next to FM-1 XXXX); waits for the port.
cond-ble:
    PYTHONPATH=host uv run --locked --all-groups python -m conductor.blelink

# The firmware in the emulator with the virtual USB cable plugged in (pair with `just cond-run`).
cond-emu chip="uc25hq80":
    just emu-run firmware/conductor/build/conductor.fwsc "{{chip}}" --usb

# firmware/common's own tests on this computer (hostlink's Bluetooth SysEx packets, the LPC and ADPCM speech decoders).
fw-common-test:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p build
    cc -Wall -Wextra -I firmware/common -o build/hostlink_test firmware/common/tests/hostlink_test.c
    build/hostlink_test
    cc -Wall -Wextra -I firmware/common -o build/lpc_test firmware/common/tests/lpc_test.c
    build/lpc_test
    cc -Wall -Wextra -I firmware/common -o build/adpcm_test firmware/common/tests/adpcm_test.c
    build/adpcm_test

# hc end to end: two fake agents (label conductor-test=1), approve / deny / continue, then kill them.
cond-it:
    uv run --locked --all-groups python host/tests/integration_hc.py

# Agent Deck in the emulator with the virtual USB cable plugged in (pair with `just agtdeck-run`).
agtdeck-emu chip="uc25hq80": agtdeck-fetch
    just emu-run "downloads/agtdeck-{{agtdeck_version}}.fwsc" "{{chip}}" --usb

# Run the daemon against a MIDI device: a real FM-1 with Agent Deck, or the emulator
# started with `just emu-run downloads/agtdeck-{{agtdeck_version}}.fwsc uc25hq80 --usb`.
agtdeck-run backend="herdr" *args: agtdeck-clone
    uv run --locked --all-groups python vendor/fm-1-agent-deck/host/agtdeck.py --backend "{{backend}}" {{args}}

# ---- Morse: pocket Morse code machine (firmware/morse + host/morse) ----------

# Morse's firmware is its own repo (github.com/daviddwlee84/fm1-morse), a submodule at firmware/morse; it is built
# here with third_party/fm1-base (its own fm1-base submodule is for building it alone, and is not fetched here).
# Release package (identity FM-1_41 + XXYYZZ): just morse-release 0.3
morse-release version: fw-setup fw-toolchain
    JIELI_TOOLCHAIN="$PWD/tools/jieli/toolchain" AC79_SDK="$PWD/tools/fw-AC79_AIoT_SDK" \
      uv run --locked --all-groups python "{{fm1_base}}/tools/build.py" firmware/morse --release "{{version}}"

# morse.c, learn.c and content.c on this computer: code table, timing, player, keyer, decoder, flash cards, content.
morse-unit:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p build
    cc -Wall -Wextra -o build/morse_test firmware/morse/tests/morse_test.c
    build/morse_test

# Host unit tests (protocol vectors shared with firmware/morse/tests/basic.txt, the host's answers).
morse-test:
    uv run --locked --all-groups pytest -q host/tests/test_morse.py

# The firmware in the emulator with the virtual USB cable plugged in (pair with `just morse-type` / `morse-send`).
morse-emu chip="uc25hq80":
    just emu-run firmware/morse/build/morse.fwsc "{{chip}}" --usb

# What you key or type on the FM-1, typed into the focused app (macOS, Accessibility). --dry-run prints it.
morse-type *args:
    PYTHONPATH=host uv run --locked --all-groups python -m morse type {{args}}

# The FM-1 plays TEXT as received ("-": lines from stdin).
morse-send +text:
    PYTHONPATH=host uv run --locked --all-groups python -m morse send {{text}}

# The host against a text stand-in for the FM-1 (lines you type are what it "read").
morse-sim:
    PYTHONPATH=host uv run --locked --all-groups python -m morse --simulate type --dry-run

# The learning progress into a JSON file (firmware 0.4+; read-only on the FM-1).
morse-backup file:
    PYTHONPATH=host uv run --locked --all-groups python -m morse backup "{{file}}"

# A progress JSON file back onto the FM-1: it asks on its screen first (PLAY yes, HOME no).
morse-restore file:
    PYTHONPATH=host uv run --locked --all-groups python -m morse restore "{{file}}"

# The simulator's screenshots, doubled (nearest neighbour), into fm1-morse's docs/screens/ for its README.
morse-shots: (fw-test "morse")
    #!/usr/bin/env bash
    set -euo pipefail
    uv run --locked --all-groups python - <<'PY'
    from pathlib import Path
    from PIL import Image
    src, dst = Path("firmware/morse/build/host"), Path("firmware/morse/docs/screens")
    dst.mkdir(parents=True, exist_ok=True)
    names = """menu boot card_new card_new_peek card_hear card_see_key card_hear_key result_right result_keyed result_keyed_wrong
    result_almost done practice_test placement_ask listen listen_hidden key key_code key_timing rhythm rhythm_good
    rhythm_dahs type type_code type_phrases chart chart_edit chart_save load_ask settings settings_bt settings_start load_settings unsaved""".split()
    for n in names:
        im = Image.open(src / f"{n}.png").convert("RGB")
        im.resize((im.width * 2, im.height * 2), Image.NEAREST).save(dst / f"{n}.png", optimize=True)
    print(f"{len(names)} screens in {dst}")
    PY

# ---- theory: a music theory trainer, so far its platform-free core (firmware/theory, docs/dev/theory.md) ----

# theory.c and srs.c on this computer: spelling, scales, keys, chords, Roman numerals, naming a chord, voicing,
# music21's spellings (tests/vectors.h); then the flash cards' rounds. Strict flags: no double, no libc in fm1-base.
theory-unit:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p build
    cc -std=c99 -Wall -Wextra -pedantic -Wdouble-promotion -Wfloat-equal -Wshadow -o build/theory_test \
      firmware/theory/tests/theory_test.c
    build/theory_test
    cc -std=c99 -Wall -Wextra -pedantic -Wdouble-promotion -Wfloat-equal -Wshadow -o build/srs_test \
      firmware/theory/tests/srs_test.c
    build/srs_test

# THEORY in the emulator (build it first: just fw-build theory). Its simulator test is `just fw-test theory`.
theory-emu chip="uc25hq80":
    just emu-run firmware/theory/build/theory.fwsc "{{chip}}"

# The computer side's tests: the wire, the record, JSON, backup and restore (vectors shared with tests/basic.txt).
theory-test:
    uv run --locked --all-groups pytest -q host/tests/test_theory.py

# THEORY's progress and settings into a JSON file (0.2+; read-only on the FM-1). Finds the unit by what it says.
theory-backup file:
    PYTHONPATH=host uv run --locked --all-groups python -m theory backup "{{file}}"

# A progress JSON file back onto the FM-1: it asks on its screen first (PLAY yes, HOME no).
theory-restore file:
    PYTHONPATH=host uv run --locked --all-groups python -m theory restore "{{file}}"

# Regenerate firmware/theory/tests/vectors.h from music21 (BSD-3-Clause; pinned, fetched by uv, not a project dependency).
theory-vectors:
    uv run --quiet --no-project --with music21==10.5.2 python -I host/theory/make_vectors.py

# ---- guitar: a pocket guitar neck and chord book (firmware/guitar, docs/dev/guitar.md) ----

# fret.c on this computer: tunings, scale positions (CAGED, NPS), chord shapes against the open chords a
# guitarist learns, every chord on every tuning checked playable. Strict flags, as for theory-unit.
guitar-unit:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p build
    cc -std=c99 -Wall -Wextra -pedantic -Wdouble-promotion -Wfloat-equal -Wshadow -Ifirmware/theory \
      -o build/fret_test firmware/guitar/tests/fret_test.c
    build/fret_test

# GUITAR in the emulator (build it first: just fw-build guitar). Its simulator test is `just fw-test guitar`.
guitar-emu chip="uc25hq80":
    just emu-run firmware/guitar/build/guitar.fwsc "{{chip}}"

# ---- voice lab: speech options for a vocabulary app (host/voicelab, docs/dev/tts.md) ----

# Upstream code the lab builds on, at pinned commits, into vendor/ (git-ignored): Moonshine's Klatt TTS (MIT),
# python_wizard (MIT), CMUdict (BSD), and the 386DX engine (GPL-3.0) with eSpeak NG 1.52.0, built for this computer.
voice-vendor:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p vendor
    pin() { # dir url sha [sparse paths...]
      local dir=$1 url=$2 sha=$3; shift 3
      if [ ! -d "vendor/$dir/.git" ]; then
        git init -q "vendor/$dir" && git -C "vendor/$dir" remote add origin "$url"
        if [ $# -gt 0 ]; then git -C "vendor/$dir" sparse-checkout set --no-cone "$@"; fi
      fi
      git -C "vendor/$dir" fetch -q --depth 1 --filter=blob:none origin "$sha"
      git -C "vendor/$dir" checkout -q FETCH_HEAD
      echo "ok   vendor/$dir  $(git -C "vendor/$dir" rev-parse --short HEAD)"
    }
    pin moonshine https://github.com/moonshine-ai/moonshine.git d8d8fa29d05ca67441a7719e70099f9f48917a93 \
      /micro/klatt-tts/ /micro/g2p/ /micro/LICENSE
    pin python_wizard https://github.com/ptwz/python_wizard.git add73ebf3c8fed306225dffd2e0229aee1a38e8f
    pin cmudict https://github.com/cmusphinx/cmudict.git 74790861f652b15e4ac49015a90074ad62a27690
    pin 386dx-engine https://github.com/seajaysec/386-DX-ENGINE.git 595eb7d9e9200e97b7fbc09ff7e1fdb771bf5f11
    test -d vendor/386dx-engine/vendor/espeak-ng || git -c advice.detachedHead=false clone -q --depth 1 \
      --branch 1.52.0 https://github.com/espeak-ng/espeak-ng.git vendor/386dx-engine/vendor/espeak-ng
    test -x vendor/386dx-engine/build/sing || (cd vendor/386dx-engine && uv run --no-project --with ninja python3 tools/build.py)

# Render the test words (host/voicelab/words.txt) with every option; build/voicelab/index.html to listen.
# `just voice-lab lpc10 adpcm16` renders only those engines.
voice-lab *engines:
    PYTHONPATH=host uv run --no-project --with numpy --with scipy python -m voicelab build {{engines}}

# Whisper (base.en) as a stand-in listener: what it hears in each rendering, onto the page.
voice-score:
    PYTHONPATH=host uv run --no-project --with faster-whisper python -m voicelab score

# Open the listening page.
voice-open:
    open build/voicelab/index.html

# The test words into firmware/voicelab (gen/voicedata.h, git-ignored), then its host simulator test (WAVs in
# firmware/voicelab/build/host/). `just fw-build voicelab` makes the device package; `just emu-run` plays it.
voice-fw:
    PYTHONPATH=host uv run --no-project --with numpy --with scipy python -m voicelab firmware
    just fw-test voicelab

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

# M-Flasher shows the plan and waits for a typed "yes". Owner only.
# Switch firmware: download mode -> backup -> install -> restart. No argument lists the targets.
fm1-switch target="":
    #!/usr/bin/env bash
    set -euo pipefail
    case "{{target}}" in
      ""|-h|--help|list)
        echo "usage: just fm1-switch TARGET"
        echo
        echo "named targets:"
        for t in "sloop=downloads/sloop-{{sloop_version}}.fwsc" "agtdeck=downloads/agtdeck-{{agtdeck_version}}.fwsc" \
                 "ctl=downloads/ctl-{{ctl_version}}.fwsc" "choralroot=downloads/{{choralroot_pkg}}" \
                 "remote=downloads/remote-{{remote_version}}.fwsc"; do
          f="${t#*=}"; if [ -f "$f" ]; then st="ready"; else st="not downloaded"; fi
          printf '  %-11s %s  (%s)\n' "${t%%=*}" "$f" "$st"
        done
        echo
        echo "packages on disk (pass the path):"
        ls downloads/*.fwsc firmware/*/build/*.fwsc 2>/dev/null | sed 's/^/  /' || true
        echo
        echo "now installed:"
        "{{m_flasher}}" find | sed 's/^/  /'
        echo "  (docs/device-log.md has the firmware name; SLOOP and Agent Deck both show as Felucca here)"
        exit 0 ;;
      sloop)   pkg="downloads/sloop-{{sloop_version}}.fwsc" ;;
      agtdeck) pkg="downloads/agtdeck-{{agtdeck_version}}.fwsc" ;;
      ctl)        pkg="downloads/ctl-{{ctl_version}}.fwsc" ;;
      choralroot) pkg="downloads/{{choralroot_pkg}}" ;;
      remote)     pkg="downloads/remote-{{remote_version}}.fwsc" ;;
      *)       pkg="{{target}}" ;;
    esac
    test -f "$pkg" || { echo "no such package: $pkg (try: just fm1-fetch / just agtdeck-fetch)" >&2; exit 1; }
    name=$(basename "$pkg" .fwsc)
    units=$("{{m_flasher}}" find)
    echo "$units"
    if echo "$units" | grep -q "(emu)"; then
      echo "An emulated FM-1 is on USB MIDI. Close the emulator first, so the switch cannot go to the wrong device." >&2; exit 1
    fi
    if ! echo "$units" | grep -q "^usb:"; then
      "{{m_flasher}}" download-mode
    fi
    dir="backups/$(date +%Y-%m-%d-%H%M)-pre-$name"
    mkdir -p "$dir"
    "{{m_flasher}}" backup "$dir" | grep -E "firmware|app |Backup saved"
    shasum -a 256 "$dir"/*.bin
    echo "Backup before the switch: $dir (not uploaded; just fm1-backup-release $(basename "$dir") if you want it off this machine)"
    "{{m_flasher}}" firmware "$pkg" --restart

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

# ---- rhythm: RHYTHM, a rhythm trainer (firmware/rhythm, docs/dev/rhythm.md) ----

# rhythm.c on this computer: parsing, the generator's rules, timing, the alignment, grades and hints.
rhythm-unit:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p build
    cc -std=c99 -Wall -Wextra -pedantic -Wdouble-promotion -Wfloat-equal -Wshadow -o build/rhythm_test \
      firmware/rhythm/tests/rhythm_test.c
    build/rhythm_test

# RHYTHM in the emulator (build it first: just fw-build rhythm). Its simulator test is `just fw-test rhythm`.
rhythm-emu chip="uc25hq80":
    just emu-run firmware/rhythm/build/rhythm.fwsc "{{chip}}"

# ---- drums: DRUMS, a live drum machine (firmware/drums, docs/dev/drums.md) ----

# groove.c and kit.c on this computer: the styles parse, every request lands where FILL AT says, every drum sounds and ends.
drums-unit:
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p build
    cc -std=c99 -Wall -Wextra -pedantic -Wdouble-promotion -Wfloat-equal -Wshadow -o build/groove_test \
      firmware/drums/tests/groove_test.c
    build/groove_test
    cc -std=c99 -Wall -Wextra -pedantic -Wdouble-promotion -Wshadow -I{{fm1_base}}/include -Ifirmware/common -o build/kit_test \
      firmware/drums/tests/kit_test.c -lm
    build/kit_test | tail -1

# DRUMS in the emulator (build it first: just fw-build drums). Its simulator test is `just fw-test drums`.
drums-emu chip="uc25hq80":
    just emu-run firmware/drums/build/drums.fwsc "{{chip}}"

# Where gitee.com cannot be reached: the three AC79 SDK files a USB-only build needs, out of an fm1-base package,
# kept only if they match fm1-base's pinned SHA-256 (scripts/ac79_sdk_from_fwsc.py; docs/dev/index.md).
fw-sdk-from package:
    python3 -I scripts/ac79_sdk_from_fwsc.py "{{package}}"
