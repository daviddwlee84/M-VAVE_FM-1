#!/usr/bin/env python3
"""What this computer is missing for the FM-1 work: offline, fast, read-only.

Run at the start of every agent session (the SessionStart hook in .claude/settings.json)
and by `just doctor`. It never touches the unit, the network or any file; it only looks.

  --brief   print nothing when everything is in place (for the hook)

The fix for almost everything it reports is `just setup` (downloads only, digest-checked).
"""

from __future__ import annotations

import os
import platform
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
JUSTFILE = ROOT / "Justfile"
DEVICE_LOG = ROOT / "docs" / "device-log.md"


def just_var(name: str) -> str | None:
    m = re.search(rf'^{name}\s*:=\s*(?:env\("[^"]+",\s*)?"([^"]+)"', JUSTFILE.read_text(), re.M)
    return m.group(1) if m else None


def main() -> int:
    brief = "--brief" in sys.argv
    machine = platform.machine()
    is_mac = sys.platform == "darwin"
    missing: list[tuple[str, str]] = []  # (what, fix)
    notes: list[str] = []

    # Repo-side pieces that `just setup` brings back.
    for sub in ("third_party/fm1-base", "firmware/morse", "firmware/metronome"):
        if not (ROOT / sub / ".git").exists():
            missing.append((f"submodule {sub} not checked out", "just setup"))
    if not (ROOT / ".venv").is_dir():
        missing.append(("Python environment (.venv)", "just setup"))

    sloop = just_var("sloop_version")
    mfv = just_var("m_flasher_version")
    if sloop and not (ROOT / f"downloads/sloop-{sloop}.fwsc").exists():
        missing.append((f"downloads/sloop-{sloop}.fwsc", "just setup"))

    # M-Flasher: the only way to back up or switch with download mode. GUI-only first run.
    m_flasher = os.environ.get("M_FLASHER") or just_var("m_flasher") or ""
    if is_mac and not Path(m_flasher).exists():
        missing.append((f"M-Flasher app ({m_flasher})",
                        f"owner: `just fm1-install-flasher` (opens the {mfv} dmg), drag to Applications, "
                        "open it once in the GUI (Open Anyway, password, Allow removable volume)"))

    # Emulator: no macOS x86_64 build upstream.
    emu = ROOT / "tools/fm1-emu/fm1-live"
    if not emu.exists():
        if is_mac and machine == "x86_64":
            notes.append("fm1-emu has no macOS x86_64 build: emu-* recipes do not work on this Intel Mac. "
                         "Use the web emulator (fw-preview / fw-web) or an Apple silicon machine.")
        else:
            missing.append(("fm1-emu (tools/fm1-emu/)", "just setup"))

    # Backups: full-flash images and progress JSON are git-ignored and machine-local.
    bins = sorted((ROOT / "backups").glob("*/*.bin"))
    progress = sorted((ROOT / "backups/progress").glob("*.json"))
    if not bins:
        missing.append(("no full-flash backup on this computer (backups/*/*.bin)",
                        "just setup (pulls the public release backups); a fresh one needs M-Flasher: just fm1-backup LABEL"))
    # Backups the device log lists that are neither here nor in a release.
    only_elsewhere = []
    if DEVICE_LOG.exists():
        for line in DEVICE_LOG.read_text().splitlines():
            if line.startswith("| 20") and "尚未上傳" in line:
                m = re.search(r"`(backups/[^`]+?\.bin)`", line)
                if m and not (ROOT / m.group(1)).exists():
                    only_elsewhere.append(m.group(1))
    if only_elsewhere:
        notes.append(f"{len(only_elsewhere)} backups in docs/device-log.md were never uploaded and are not here "
                     f"(only on the old computer), newest: {only_elsewhere[-1]}")

    # Firmware development, informational.
    if is_mac and shutil.which("brew") and not any(Path(p).exists() for p in
                                                  ("/opt/homebrew/opt/libraqm", "/usr/local/opt/libraqm")):
        notes.append("libraqm not installed (just fw-test needs it): brew install libraqm")
    if not shutil.which("docker"):
        notes.append("docker not found (just fw-build needs it)")
    if not shutil.which("gh"):
        missing.append(("gh CLI", "brew install gh && gh auth login"))

    if brief and not missing and not notes:
        return 0

    head = f"[fm1 doctor] {platform.node()} ({sys.platform} {machine})"
    if not missing:
        print(f"{head}: everything in place; {len(bins)} full-flash backups, {len(progress)} progress files.")
    else:
        print(f"{head}: this computer is not set up for the FM-1 yet (new machine or fresh clone).")
        for what, fix in missing:
            print(f"  missing: {what}\n           fix: {fix}")
        print(f"  have: {len(bins)} full-flash backups, {len(progress)} progress files in backups/")
        print("  Agents: run `just setup` now (downloads only, digest-checked; no device access), "
              "then `just doctor`. Do not operate the unit until M-Flasher works and a fresh backup exists.")
    for n in notes:
        print(f"  note: {n}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
