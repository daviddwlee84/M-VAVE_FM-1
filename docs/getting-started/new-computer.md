# 換電腦

裝置相關的東西幾乎都不在 git 裡：`backups/`、`downloads/`、`tools/`、`.venv`、submodule，還有裝在 `/Applications` 的 M-Flasher。新 clone 的 repo 這些全都沒有。2026-11-28 換到新的 Intel Mac 時，就是在要刷 SLOOP 時才發現這件事。

## 怎麼發現少了什麼

```sh
just doctor
```

它不連網、不碰機器，只檢查這台電腦，列出少了什麼、怎麼補。Claude Code 每次開 session 都會透過 `.claude/settings.json` 的 SessionStart hook 自動執行 `scripts/doctor.py --brief`：全部都在就不印東西，少了東西就把清單放進 agent 的 context。其他 agent 照 `AGENTS.md` 的規定，開工前自己跑 `just doctor`。

## 自動補齊

```sh
just setup
```

只下載，每個檔案都比對 release 的 SHA-256 digest，不碰機器。它會做這些事：

| 步驟 | 內容 |
|---|---|
| `git submodule update --init --recursive` | `third_party/fm1-base`、`firmware/morse`、`firmware/metronome` |
| `uv sync` | Python 環境（`morse-backup` 這類 recipe 要用） |
| `just fm1-fetch` | M-Flasher 的 dmg、SLOOP package 和 SLOOP 編輯器 |
| `just emu-fetch` | 只在 Apple silicon Mac 上跑，原因見下面 |
| `just fm1-backup-pull` | 把所有 `backup-*` GitHub Release 下載到 `backups/<名稱>/`，`progress-*`（進度 JSON）下載到 `backups/progress/`，逐檔比對 digest，最後跑 `fm1-verify-backups` |

Agent 看到 doctor 回報有缺東西，會直接跑 `just setup`，不用先問。

## 只有本人能做的

- **M-Flasher**：`just fm1-install-flasher` 會打開 dmg，把 app 拖進 Applications，然後在 GUI 開一次，完成第一次授權（見[安裝 M-Flasher](install-m-flasher.md)）。這一步不能透過 SSH 做。dmg 是 universal binary，Intel 和 Apple silicon 都能用 `[實機]`（2026-11-28 用 `file` 確認過）。
- `brew install libraqm`（`just fw-test` 需要）、Docker（`just fw-build` 需要）、`gh auth login`。

## 備份只有一部分跟著走

- 上傳過 GitHub Release 的備份，`just setup` 會抓下來。`docs/device-log.md` 裡標著「尚未上傳」的備份，只存在當初做備份的那台電腦上。doctor 會列出有幾份。
- 換電腦後要刷韌體，**先在新電腦上重新做一份 `just fm1-backup`**。Release 上的備份都比機器現在的狀態舊。
- 存在機器裡的進度（Morse、THEORY、METRONOME）可以用 `just morse-backup` / `theory-backup` / `metro-backup` 存成 JSON。這幾個只需要 `uv` 和 USB 線，不需要 M-Flasher。存好的檔案在 `backups/progress/`。要留一份在這台電腦以外，用 `just fm1-progress-release backups/progress/<檔名>.json`，它會上傳成公開的 Release `progress-<檔名>`，換電腦時 `just setup` 會再抓回來。

## Intel Mac 跑不了 fm1-emu

fm1-emu 0.5.1 只出 macOS arm64、Linux 和 Windows 版，沒有 macOS x86_64 版 `[上游]`（[releases](https://github.com/joakimeriksson/fm1-emu/releases)），Intel 上的 Rosetta 也跑不了 arm64。在 Intel Mac 上 `emu-*` recipe 都不能用。替代方式：用網頁版模擬器（`just fw-preview`），或改在 Apple silicon 的電腦上跑。自己 build 的韌體在實刷前要在兩種晶片的模擬器上各跑一次，這條規則不變；如果做不到，要在回報裡寫清楚，不能直接略過。
