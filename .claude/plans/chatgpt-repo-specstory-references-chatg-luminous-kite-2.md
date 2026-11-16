# M-VAVE FM-1：Phase 2 系列工作計畫

## Context 背景

Phase 1 已完成：筆記站上線、`AGENTS.md`、`Justfile` 捷徑、原廠與 SLOOP 2.8.5 兩份完整備份（本機 + GitHub Release）、SLOOP 2.8.5 在實機通過存檔測試。這台機器是**新晶片 `0xB36014`**、stock bootloader。

這一階段要回答並落實你問的五件事，方向已確認：m-bootloader **只追蹤、不裝**；自製韌體**放在這個 repo 的子目錄**（必要時 fm1-base 相關部分另開 repo 再以 submodule 引入）；第一個方向是 **Agent 控制台 / MIDI controller**，但**先試用 Agent Deck**。

## 先回答你的問題

**m-bootloader 現在不建議裝，而且「有整機備份」不等於不怕。** 備份只有在機器還能進 download mode 時才用得上。boot 區壞掉時機器不開機，上游寫明這種情況要靠 USB_KEY dongle（Raspberry Pi Pico）強制進 ROM download mode，才輪得到還原備份。上游對 `0xB36014` 的原文是 install it at your own risk：一台裝 1.1/1.2 後無法啟動、原因不明，一台回報 1.3 與 2.0 正常。你手上的 Raspberry Pi 3 不能替代 Pico（USB_KEY 是 RP2040 的實作）。

**依賴關係：**

| 項目 | 需要 m-bootloader？ | 依據 |
|---|---|---|
| SLOOP 3 | **需要**，只能經 MIDI MODE 安裝 | 上游 README |
| Agent Deck | **不需要**，Dinghy fork，一般 `.fwsc` | 上游 README |
| fm1-emu | 不需要，完全不碰實機 | — |
| 自製韌體（fm1-base） | 不需要，產出 `.fwsc` | 上游 README |

所以不裝 m-bootloader 只擋住 SLOOP 3 一項，其他都能做。

**Justfile 目前夠用。** 現階段不需要另寫 CLI wrapper。等出現「多步驟且要強制檢查」的流程（例如切換韌體）再加，而且先加成 just recipe，不夠才抽成 script。

## 做法：四條工作線，依序進行

### A. fm1-emu 本機化（零實機風險，先做）

上游有 macOS arm64 的預編譯 release（`fm1-emu-0.5.1-macos-arm64.tar.gz`），不需要 Rust 建置。

- `Justfile` 新增：`emu-fetch`（下載並驗 digest，解到 gitignore 的 `tools/fm1-emu/`）、`emu-run <fwsc> [chip]`（包 `fm1-live --fwsc`，`chip=uc25hq80` 時帶 `--flash-chip uc25hq80`，`--flash` 指到 `tools/emu-state/` 讓存檔跨次保留）。確切旗標以解壓後的 `--help` 與 README 為準。
- 在模擬器（新晶片模式）實測並記錄：SLOOP 2.8.5（對照實機行為）、SLOOP 3（只在模擬器玩）、Agent Deck 0.7。每個做「改設定 → 存 → 重開 → 還在嗎」。
- 更新 `docs/agentic/emulator.md`：把「本 repo 尚無本機 fm1-emu」改成實際用法，新增一張「模擬器實測」表，標 `[實機]` 以外的新標記 `[模擬]`（同步寫進 `AGENTS.md` 與 `docs/index.md` 的證據標記說明）。

### B. Agent Deck：先模擬、再實刷

1. **Host 模擬（不刷機）**：把 `kevinelliott/fm-1-agent-deck` clone 到 gitignore 的 `vendor/`，跑上游的 `python3 host/agtdeck.py --backend auto --simulate`，接你實際的 Herdr session，確認 agent 狀態同步是否堪用。auto-approve 保持關閉。
2. **模擬器裡跑韌體**：用 A 的 `emu-run` 開 `agtdeck-0.7.fwsc`（`uc25hq80`），確認開機與設定保存。上游沒有新晶片的實機回報，這一步是實刷前唯一能做的相容性檢查。
3. **實刷（需你逐次批准）**：preflight 四項 → 你執行 `just fm1-flash vendor/…/agtdeck-0.7.fwsc` → 實測按鍵、LED、螢幕、daemon 硬體模式。先用 `gh attestation verify` 驗證套件來源（上游提供）。
4. **切回 SLOOP**：先試 `just fm1-flash downloads/sloop-2.8.5.fwsc`，看曲子是否還在（上游明說這點沒測過）。不在的話由你在 M-Flasher GUI 還原 `2026-11-17-sloop-2.8.5` 備份。兩種結果都記進 `docs/firmware/switching.md`，這是那頁目前缺的實機資料。
5. 新增 `docs/firmware/agent-deck/`（概觀、host daemon 設定、Herdr 整合心得），把 `docs/agentic/agent-deck.md` 的實用部分搬過去並互連。

切換流程走過一次、確認步驟固定之後，才把它包成 `just fm1-switch <名稱>`（檢查備份存在 → 顯示 preflight → 呼叫 `fm1-flash`）。還原備份仍不做成 recipe。

### C. 自製韌體環境（vibe firmware）

目標是把 **改 code → host simulator → fm1-emu → 批准 → 實機** 整條鏈跑通，再談功能。

- **結構**：`firmware/<app>/`（本 repo 子目錄）放 app；`third_party/fm1-base` 以 git submodule 引入（它自己用 `--recursive` 帶 felucca-bsp）。fm1-base 的 `tools/build.py APP_DIR` 本來就支援 app 在外部目錄。若實作時發現它的 template/release workflow 假設 app 必須是獨立 repo，就改成另開 repo 再 submodule 進來，並在該步驟回報。
- **兩段式 build**：
  - host simulator 與 browser emulator：只需要本機 C 編譯器 / emcc，Mac 可直接跑，是日常迭代的主迴圈。
  - 實機 `.fwsc`：需要 JieLi pi32v2 toolchain，上游只支援 Linux x86-64，Mac 要走 Docker。先確認 fm1-base 自己的建置文件與 `LICENSING.md`（每個 build 要帶三個 SDK 檔），再決定用 Docker 還是 GitHub Actions 產出 artifact。**先不承諾本機一定能 build 實機套件**，這是此線最大的不確定點。
- **第一個 app**：最小的 MIDI controller / macro pad（按鍵送 MIDI、螢幕顯示 layer、host 端 Python `mido` 接收並呼叫 Herdr）。它同時是 Agent 控制台的前置練習。之後依 B 的試用心得決定：fork Agent Deck（GPL-3.0、Dinghy 系），或在 fm1-base 上重寫（MPL-2.0，app 可自選 license）。
- **Agent 規則**：`AGENTS.md` 新增 firmware 開發一節：先 host test、再 `emu-run` 兩種晶片、自製韌體每次實刷都要批准、`.fwsc` 產物放 gitignore 的 `build/`、自製韌體必須用 fm1-base 既有的 storage 層（它處理新晶片 JEDEC ID），不得自己寫 flash 位址。
- **Skills**：fm1-base 內建一個 `skill/felucca-to-fm1-base`；評估後決定是否引用。本 repo 自己的 thin skill 等這條鏈實際跑過兩三次、流程穩定後再寫，內容是「怎麼開新 app、怎麼跑測試、怎麼提 preflight」，不抄社群 skill。
- 文件：新增 `docs/dev/`（環境、build、測試、第一個 app 的紀錄）。

### D. m-bootloader 與 SLOOP 3：只追蹤

- 新增 `docs/watch.md`：一張 ledger，每次檢查記日期、上游版本、對 `0xB36014` 的新回報、結論。初始條目寫今天的狀態。
- `Justfile` 新增 `fm1-versions`：用 `gh api` 列出 `docs/resources.md` 版本表中各 repo 的最新 release，與表內版本並排，方便更新。
- 解除暫緩的條件寫明在 `docs/firmware/sloop/sloop3.md`：（1）上游 README 不再把新晶片標為 little tested，或有多個成功回報；且（2）手上有驗證過可用的 USB_KEY（Pico）。兩者都滿足前不裝。
- SLOOP 3 這段期間只在模擬器體驗（A）。

## 順序與相依

```
A（emu）──┬─▶ B（Agent Deck）──▶ fm1-switch recipe
          └─▶ C（自製韌體）
D（追蹤）獨立，隨 A 一起做
```

A 與 D 不碰實機，可以一次做完。B 的第 3 步與 C 的實刷都需要你在場並逐次批准。

## 會改到的檔案

- `Justfile`：`emu-fetch`、`emu-run`、`fm1-versions`，之後 `fm1-switch`、firmware build/test recipes
- `.gitignore`：`/tools/`、`/vendor/`、`/build/`
- `.gitmodules` + `third_party/fm1-base`（C）
- `firmware/<app>/`（C，新）
- `AGENTS.md`：`[模擬]` 標記、firmware 開發規則、新目錄
- `docs/`：`agentic/emulator.md`、`firmware/switching.md`、`firmware/sloop/sloop3.md`、`firmware/agent-deck/*`（新）、`dev/*`（新）、`watch.md`（新）、`device-log.md`、`resources.md`
- `zensical.toml`：nav 新增頁面
- `.github/workflows/docs.yml`：checkout 不帶 submodule（docs build 不需要），確認 path filter 不被 `firmware/` 觸發

沿用既有的：`fm1-fetch` 的「下載 + 對 release digest」寫法（`Justfile`）、`fm1-flash` 的確認機制、`scripts/check-docs-site.py`。

## Verification 驗證

- A：`just emu-fetch` digest 相符；`just emu-run downloads/sloop-2.8.5.fwsc uc25hq80` 開得起來、存檔跨次保留；模擬實測表填好。
- B：simulate 模式下 Herdr 的 agent 狀態出現在 daemon 輸出；實刷後 `just fm1-find` 看得到裝置、daemon 硬體模式連得上；切回 SLOOP 後的曲子狀態有記錄。
- C：template app 在 host simulator 跑起來；產出的 `.fwsc` 在 `emu-run` 兩種晶片都開機並保存設定；實機刷入後按鍵 → MIDI → host 腳本收得到。
- D：`just fm1-versions` 輸出與 `resources.md` 一致或已更新；`watch.md` 有初始條目。
- 每次改 docs：`just docs-check` 通過，push 後 Pages workflow 綠燈。
- 每次實機操作：`docs/device-log.md` 有對應紀錄。

## 不在這次範圍

安裝 m-bootloader、實刷 SLOOP 3、USB_KEY / Pico 硬體、m-card / m-apps、硬體自動化測試台、把自製韌體提交到 Firmware Hub。
