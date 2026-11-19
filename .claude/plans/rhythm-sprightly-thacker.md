# RHYTHM v0.3: PULSE, JUDGE, QUIZ — PR with an fm1-emu preview link

## Context

RHYTHM 0.3 (PULSE, JUDGE, QUIZ; commit `45031f6` on `claude/rhythm-metronome-practice-lytzzu`) is built and passes
`rhythm-unit` and `fw-test rhythm` in the host simulator only. The owner (2026-11-19) asked: open the PR and put the
fw-preview link on it. Per AGENTS.md "Publishing and git", a firmware PR follows `.github/pull_request_template.md`:
1. Try it (fm1-emu link from `just fw-preview APP pr-N`), 2. Screens, 3. Before the unit, 4. On the unit.

`fw-preview` needs a real device package (`firmware/rhythm/build/rhythm.fwsc`) from `just fw-build rhythm`, which has
not run yet. `docs/dev/index.md#linux` says a device build works in this cloud Linux container without Docker
(DRUMS 0.1 was built that way), with gitee.com unreachable for the AC79 SDK, fixed by `just fw-sdk-from PKG`.

## Steps

1. **Toolchain** (one-time, ~1.2 GB into git-ignored `tools/`):
   - `just` via `uv tool install rust-just` (not installed here); `libraqm-dev`, `libasound2-dev` via apt if
     `uv sync --all-groups` still cannot build Pillow (it failed earlier; the host build ran with system
     `python3` instead — fall back to that for `build.py` if uv stays broken).
   - JieLi toolchain: `third_party/fm1-base/tools/get_toolchain.sh tools/jieli` (pkgman.jieliapp.com is reachable).
   - AC79 SDK's three files: `git fetch origin previews`, take our own `pr-13/metronome.fwsc` (fm1-base package) into
     a fresh dir under `downloads/`, then `python3 -I scripts/ac79_sdk_from_fwsc.py downloads/<dir>/metronome.fwsc`
     (it writes them only if their SHA-256 match `SDK_SHA256` in build.py).
2. **Device build**: `JIELI_TOOLCHAIN=tools/jieli/toolchain AC79_SDK=tools/fw-AC79_AIoT_SDK python3
   third_party/fm1-base/tools/build.py firmware/rhythm`. Record package size, app size, RAM `.data+.bss`, the
   build's own checks (no soft-double, no 64-bit multiply beside a load), identity `FM-1_5400`, SHA-256.
   If it fails (e.g. a `static` name clashing with `device/*.c`, 64-bit division), fix in `app.c`/`rhythm.c`, re-run
   `rhythm-unit` and `fw-test rhythm`, commit.
3. **Open the PR** (GitHub MCP `create_pull_request`, base `main`, head the branch), title
   `RHYTHM 0.3: PULSE, JUDGE and QUIZ`, body from the template; its number N gives the slot.
4. **Preview**: `just fw-preview rhythm pr-N` (or the recipe's commands by hand if `just` is missing) — PUBLISHES the
   .fwsc to the `previews` branch (one force-with-lease commit, keeps the other slots). Link:
   `https://joakimeriksson.github.io/fm1-emu/?package=https://raw.githubusercontent.com/daviddwlee84/M-VAVE_FM-1/previews/pr-N/rhythm.fwsc&name=rhythm&chip=uc25hq80`.
5. **Fill the PR body** (update_pull_request):
   - What changes for the player: the three pages in a few lines.
   - 1. Try it: the link (+ a `chip=p25q80` variant).
   - 2. Screens: commit a few simulator shots (pulse_steady, pulse_rushing, judge_done, judge_feel, quiz_term_bpm,
     quiz_bpm_term_feed, quiz_meter) into `docs/assets/rhythm/` and reference them by raw GitHub URL; or list them as
     in `firmware/rhythm/build/host/` if committing images is not wanted (default: commit, small PNGs, docs can use them).
   - 3. Checklist: fw-test 189, rhythm-unit 474,693, fw-build numbers + SHA-256 ticked; fm1-emu on both chips and the
     rehearsal over the latest backup left unticked (fm1-emu is a macOS binary and the backups are on the owner's Mac);
     stored data: record v1 → v2, kept (simulator test).
   - 4. On the unit: the steps table from `docs/dev/rhythm.md#v03-verify`.
   - Footer: the Claude Code attribution lines.
6. **Docs**: add the device build result (date, size, SHA-256, `[模擬]` not applicable — build facts) to
   `docs/dev/rhythm.md` v0.3 verification; `just docs-check` (via `uv run --only-group docs`); commit and push.
   Re-run `fw-preview rhythm pr-N` after that commit only if firmware code changed.
7. Ask the owner whether to watch the PR (subscribe_pr_activity).

## Verification

- `build.py` device build exits 0 and prints its checks ok; `rhythm.fwsc` exists.
- `git ls-tree origin/previews` shows `pr-N/rhythm.fwsc` next to the existing slots (main/theory, pr-7, pr-12, pr-13).
- `curl -sI` the raw.githubusercontent URL returns 200.
- PR page shows the template sections filled and the link.
