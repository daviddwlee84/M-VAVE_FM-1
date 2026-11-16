# 多韌體切換

FM-1 沒有像 GRUB 那樣的開機選單可以選 SLOOP、Melodee 或 Agent Deck。目前有幾條路：

| 方案 | 怎麼切換 | 要改硬體嗎 | 評價 |
|---|---|---|---|
| **M-Flasher 輪刷** | 接電腦，重刷另一套韌體或還原備份 | 否 | **現在最實際** |
| **m-bootloader** | recovery menu、重裝韌體、啟動 RAM app | 否 | 是救援與開發工具，不是多系統選單 |
| **RemoteFM1** | 刷一次 RemoteFM1，之後韌體在電腦或手機上跑，FM-1 當面板，點一下就換 | 否 | 另一種思路，見[搭配手機與平板](../hardware/mobile.md)。我還沒試 |
| **m-apps `slots` + m-card** | SD 卡上存多個完整映像，選一個寫回內部 flash 再重開 | **要**（microSD 擴充板） | 進階實驗，尚未成熟 |

## 把電腦當 firmware manager

每套韌體各留一份「裝好並設定好之後」的完整 flash 備份。切換時還原那份備份，回來的不只是韌體，還有它的設定、音色與 project。

建議的本機目錄（都在 gitignore 裡）：

```text
backups/
  2026-11-17-stock/          原廠，最重要的一份
  2026-11-17-sloop-2.8.5/    SLOOP 裝好、設定好之後
downloads/
  sloop-2.8.5.fwsc
```

這個目錄結構是我自己的安排，不是上游工具的要求。

注意：

- 完整還原會連 boot 區與裝置資料一起寫。只用**這台機器自己**的備份。
- 之後若裝了 m-bootloader 又想還原較舊的備份，要用 **Keep this unit's bootloader**（CLI：`--keep-bootloader`），否則 bootloader 會被舊備份蓋回去。
- 還原屬於高風險操作，由本人執行，不交給 agent。見[工作流與權限](../agentic/index.md)。

只是想換一套韌體、不在乎保留前一套的狀態時，直接 `m-flasher firmware` 安裝新的 `.fwsc` 就好。M-Flasher 在資料配置不同的韌體之間切換時會警告。

## 實機紀錄

| 日期 | 從 → 到 | 做法 | 結果 |
|---|---|---|---|
| 2026-11-17 | 原廠 015 → SLOOP 2.8.5 | M-Flasher GUI 安裝 `.fwsc` | 成功，存檔正常 |
| 2026-11-17 | SLOOP 2.8.5 → Agent Deck 0.7 | `fm1-download-mode` → `fm1-backup` → `fm1-flash` → `fm1-restart`，全程不需要按機器上的鍵 | 成功 |
| 2026-11-17 | Agent Deck 0.7 → SLOOP 2.8.5 | 同上，只裝韌體、不還原備份 | 寫入成功、能開機，**SLOOP 的曲子都還在** |

### SLOOP ↔ Agent Deck 互切不會丟曲子（在我的機器上）

比對刷 Agent Deck 前後的兩份完整映像，只有這些 sector 不同：

| 區域 | 位址 | 數量 | 內容 |
|---|---|---|---|
| 韌體區 | `0x04000`–`0x8FFFF` | 140 | 韌體本身 |
| 原本空著的區域 | `0xFC000` | 1 | Agent Deck 自己的設定 |

M-Flasher 的 flash map 把 `0x93000` 以後標為 SETTINGS、PAIRINGS、SOUNDS，SLOOP 的 project、preset、sample 都在那裡，Agent Deck 一個 byte 都沒碰。所以刷回 SLOOP 只需要重裝 `.fwsc`，不必還原備份。

這個結論只對這兩個韌體、這兩個版本成立。別的韌體可能把資料寫在和 SLOOP 重疊的位置，切換前還是要備份；想確認的話，用同樣的方法比對前後兩份備份。`[實機]`

SLOOP 與 Agent Deck 執行中都能由 M-Flasher 透過 USB MIDI 切進 download mode（兩者在 `fm1-find` 都顯示為 `Felucca … download-mode switches it`）。`[實機]`

## m-apps slots 與 m-card

[m-card](https://github.com/jvitkauskas/m-card) 是一塊接在螢幕 FFC 排線上的 microSD 擴充板，不需要焊接主板，但擴充板本身要自己組。[m-apps](https://github.com/jvitkauskas/m-apps) 是從 RAM 執行的程式集合，其中的 `slots` 提供 firmware slots。

概念流程：開機 → m-bootloader 選單 → SD CARD → launcher → SLOTS → 選一個映像 → 寫回內部 flash → 重開。文件描述切換約 30 秒，不是即時的。`[調研]`

限制：`[調研]`

- m-card 仍標示為未測試的 PCB prototype。
- FM-1 上完整的 picker + slots 流程還沒有充分的實機驗證。

結論：先觀望，不急著拆機。

## `run-ram`

`m-flasher run-ram` 可以把一個小程式上傳到晶片 RAM 執行，不寫 flash，關機再開就恢復原狀。適合開發者測試自己的小程式，**不能**用來把完整的 SLOOP 或 Agent Deck 當成第二套常駐系統。它會在晶片上執行任意程式碼，在這個 repo 的規則裡屬於 agent 不得自行執行的操作。

## `just fm1-switch`

兩個方向都實測過之後做成的捷徑：

```sh
just fm1-switch             # 不帶參數：列出可用的目標與目前連著的裝置
just fm1-switch sloop       # 或 agtdeck，或任何 .fwsc 的路徑
```

具名目標目前只有 `sloop` 與 `agtdeck`，其他韌體把 `.fwsc` 的路徑傳進去。清單也會列出 `downloads/` 與 `firmware/*/build/` 裡現有的套件。

它依序做：確認沒有模擬器佔著 USB MIDI → 切 download mode → 備份到 `backups/<日期時間>-pre-<套件名>/` → 呼叫 M-Flasher 安裝（列出變更，等你輸入 `yes`）→ 重開機。屬於寫入等級，由本人執行。

目標韌體已經裝著的時候，M-Flasher 會回報 `Nothing to write` 然後直接重開，不會問 `yes`。`[實機]`

每次切換都會留一份刷前備份在本機，不會自動上傳。

## 之後想做的

- 切換後自動確認裝置回到 USB MIDI、並提示要不要啟動對應的 host 程式（Agent Deck 的 daemon、SLOOP 的 Web Editor）。
- 備份累積多了之後的整理方式。
