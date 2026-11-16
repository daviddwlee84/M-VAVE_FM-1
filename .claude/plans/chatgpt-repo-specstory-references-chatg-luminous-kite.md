# M-VAVE FM-1 知識庫 repo：Phase 1 建置計畫

## Context 背景

這個 repo 目前幾乎是空的（`README.md` 只有標題、已裝好 `zensical-site-bootstrap` skill、`.specstory/` 未追蹤）。目標是把它變成 FM-1 的個人知識庫：

- 把 ChatGPT 調研（`.specstory/references/chatgpt-FM1刷機建議-20261117-0058.md`，1576 行）整理成可長期維護的筆記站（Zensical + GitHub Pages）。
- 提供 agent 需要的 context（`AGENTS.md`），讓之後任何 agent 都能安全地協助刷機、查資料、寫筆記。
- Phase 1 驗收：**裝 M-Flasher → 備份原廠 → 刷 SLOOP 2.8.5**，由 agent 陪跑實機完成並留下紀錄。

已確認的決定：docs 用**繁體中文**（術語留英文）；Pages **一路做到上線**；驗收採 **agent 陪跑實機**；`.specstory/` **全部追蹤**。

### 調研檔的兩個已知缺陷（影響做法）

1. **所有 code block 在匯出時遺失**（指令、路徑、目錄結構、SysEx 都是空白），`<Cite>` 也無法還原。→ 每一條指令都必須從上游文件重新取得，不能憑調研內文補寫。
2. 內容是二手整理。我已用 `gh api` 抽查，以下屬實：M-Flasher `v0.12.1`（有 `M-Flasher-0.12.1-macos.dmg`）、m-bootloader `v2.4`、SLOOP `v2.8.5`（`sloop-2.8.5.fwsc`，README 明載支援 UC25HQ80）、SLOOP 3 `v3.3.7-beta`。另發現調研沒寫到的細節：SLOOP 執行中進 download mode 是**同時按住 `OCT-` + `OCT+` 5 秒**；回原廠走 M-UPGRADE + V16。

## 做法

### 1. Repo 基礎與 agent context

- `AGENTS.md`（英文，canonical）：
  - repo 用途、目錄地圖、docs 撰寫慣例（zh-TW、台灣用語、術語/識別字留英文）、build/preview 指令。
  - **來源優先序**：上游 repo/release → FM1 Developers' Wiki → 本 repo docs → `.specstory/references/`（raw、未驗證）。
  - **證據標記**：頁面內區分「已在我的機器驗證 / 上游文件記載 / 調研轉述未驗證」。
  - **Hardware safety tiers**（取自調研第 5 節權限設計）：`find`/`info`/`backup`、下載、emulator = 可自動；切 download mode = 可自動但先告知；刷 application firmware = 每次需使用者明確批准、不得用 `--yes`；bootloader 安裝、`restore`、erase = 禁止自行執行。
  - 刷寫前 preflight 清單（備份存在且有 SHA-256、晶片 ID、firmware 檔 hash、目標韌體支援該晶片）。
  - 裝置現況一律以 `docs/device-log.md` 為準；備份與下載檔放 gitignored 目錄。
- `CLAUDE.md`：只有一行 `@AGENTS.md`。
- `README.md`：簡介 + 站台連結 + 快速入口。
- `.gitignore`：加 `site/`、`.venv/`、`backups/`、`downloads/`（完整 flash 備份含裝置特定資料，不進 public repo）。
- `Justfile`：`docs-build`、`docs-check`、`docs-serve port host`。
- 暫不做自己的 thin skill 與 CLI wrapper（調研建議等開始改韌體再做），在 docs 的 roadmap 記為後續階段。

### 2. Zensical 站台

依 `.agents/skills/zensical-site-bootstrap/SKILL.md` 的流程：

1. `scripts/init-docs-site.py --target-dir <repo> --site-name 'M-VAVE FM-1 Notes' --repo-slug daviddwlee84/M-VAVE_FM-1 --site-url https://daviddwlee84.github.io/M-VAVE_FM-1/ --ci --dry-run`，看過計畫再正式執行。
2. `uv lock` → `uv sync --locked --only-group docs` → `zensical build --strict` → `scripts/check-docs-site.py`。
3. **偏離 skill 預設之處**：skill 只驗證過英文站。這裡改成單一語言 zh-TW（調整 `zensical.toml` 的 language），不做多語、不做 fallback；若 checker 或搜尋因中文出問題，就地修正並在 AGENTS.md 記下。
4. 明確 `nav`，`docs_dir` 只含 `docs/`，`.specstory/` 不進站台。

### 3. Docs 內容（繁中）

```
docs/
  index.md                      總覽、目前裝置狀態摘要、路線圖（階段 1–6）
  getting-started/
    index.md                    拆箱後最短流程與順序理由
    install-m-flasher.md        macOS 安裝、未簽章首次確認、GUI 首次授權、CLI 位置
    backup-and-chip.md          download mode、讀 FLASH CHIP、完整備份、SHA-256、異地一份
    flash-sloop.md              刷 SLOOP 2.8.5（M-Flasher 與 Web Installer 兩條路）、刷後測試
    recovery.md                 救磚階梯（由輕到重）
    cautions.md                 注意事項：V16 勿降 V15、bootloader 非無風險、完整 restore 僅限同機
  hardware/
    io.md                       USB-C / TRS MIDI IN / Audio OUT / BLE，USB device 非 host
    flash-chips.md              P25Q80H vs UC25HQ80、為何影響相容性、如何辨識
  firmware/
    index.md                    韌體地圖與比較表（音樂類、控制器類、agent 類）
    sloop/index.md              SLOOP 2.8.5 概觀
    sloop/usage.md              操作速查（layer 按鍵、drum/synth/seq、存檔）＋個人心得區
    sloop/web-editor.md         Web editor、備份還原、samples
    sloop/sloop3.md             SLOOP 3 差異與前置條件（需 m-bootloader MIDI MODE）
    switching.md                多韌體切換：M-Flasher 輪刷 / m-bootloader / m-apps slots
  agentic/
    index.md                    agentic workflow 全貌與權限分級
    m-flasher-cli.md            CLI 指令表與自動化邊界（macOS vs Linux）
    emulator.md                 fm1-emu：能力、限制、`mhz=240`、`chip=uc25hq80`
    skills.md                   社群 skills 比較與本 repo 立場（AGENTS.md + 之後的 thin skill）
    agent-deck.md               FM-1 Agent Deck、Trireme、CTL-1 + mido 路線
  resources.md                  repo 地圖（工具/文件/SDK/base）＋版本表，附「查核日期」
  device-log.md                 我的機器：晶片 ID、原廠版本、備份檔名與 SHA-256、刷機歷史
  notes/index.md                使用心得日誌
```

撰寫規則：

- 內容以調研為骨架，但**指令、按鍵組合、路徑一律從上游重取**：`jvitkauskas/m-flasher` 的 `docs/usage.md`、`docs/safety.md` 與 guide 頁；`isod89/sloop-fm1` 的 `README.md`、`SLOOP.md`、`GUIDE.md`；fm1-emu README。取不到的就標「未驗證」而不補寫。
- 版本號集中在 `resources.md` 一張表並標查核日期（2026-11-17），其他頁連過去，避免到處過期。
- 調研中帶不確定性的說法（新晶片 + m-bootloader 的啟動失敗回報、Logo 外觀線索、m-card 未驗證）保留原本的保留語氣。

### 4. 發佈上線

1. 讀 Zensical 官方 Pages guide，把 skill 給的 build-only `docs-check.yml` 擴充為：PR 只 build + check；push 到 `main` 才 deploy（`pages: write`、`id-token: write` 只給 deploy job）。
2. 在 `main` 直接 commit（單人 repo、Pages 需從 main 部署）並 push。
3. `gh api` 開啟 Pages（source = GitHub Actions），等 workflow 跑完。
4. 開 `https://daviddwlee84.github.io/M-VAVE_FM-1/` 確認首頁、一個巢狀頁、搜尋、Copy as Markdown。

### 5. 實機驗收（agent 陪跑）

需要你在場：FM-1 用 USB-C **資料線**接上並開機。

| 步驟 | 誰做 | 內容 |
|---|---|---|
| a | 你 | 開機記下原廠 firmware 版本；確認琴鍵、旋鈕、耳機輸出正常 |
| b | 我 | 下載 `M-Flasher-0.12.1-macos.dmg` 到 `downloads/`，對 release asset digest 驗 SHA-256 |
| c | 你 | 安裝並首次開啟（未簽章確認、管理員密碼、可移除磁碟權限都要在 GUI 完成） |
| d | 我 | CLI：`find` → `download-mode` → `info`，回報 FLASH CHIP / JEDEC ID |
| e | 我 | `backup` 到 `backups/<日期>-stock/`，算 SHA-256；提醒你另存一份到別處 |
| f | 我 | 寫入 `docs/device-log.md` |
| g | 我 | 下載 `sloop-2.8.5.fwsc`、驗 hash、展示 preflight 摘要，**停下等你批准** |
| h | 你批准後 | `m-flasher firmware sloop-2.8.5.fwsc`，不帶 `--yes`；M-Flasher 自己的 `yes` 確認由你輸入（用 `! <command>`） |
| i | 我 + 你 | `restart`；確認 USB MIDI 回來；你做 **Save → 關機 → 開機 → Load** |
| j | 我 | 更新 device-log 與 `notes/`，commit + push |

若步驟 d 讀到的晶片或版本與文件假設不符（例如既非 `0x856014` 也非 `0xB36014`），停在備份完成，不刷。m-bootloader 不在 Phase 1 範圍。

## Verification 驗證

- `uv run --no-sync zensical build --strict` 與 `scripts/check-docs-site.py` 皆通過。
- `just docs-serve` 本地瀏覽：導覽、中文搜尋、巢狀頁 Copy as Markdown 的剪貼簿內容。
- GitHub Actions 的 build 與 deploy job 綠燈，線上網址可開。
- `AGENTS.md` 的安全分級與 `docs/agentic/` 內容一致；`git status` 確認 `backups/`、`downloads/` 未被追蹤。
- 實機：`device-log.md` 有晶片 ID、原廠版本、備份 SHA-256、SLOOP 2.8.5 刷入時間，以及 Save/重開機/Load 的結果。

## 不在這次範圍

m-bootloader 安裝、SLOOP 3、Agent Deck 實刷、自有 thin skill / `fm1` CLI wrapper、fm1-emu 本地建置、firmware 開發環境。這些只在 docs 中記錄為後續階段。
