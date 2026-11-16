# 多韌體切換

FM-1 沒有像 GRUB 那樣的開機選單可以選 SLOOP、Melodee 或 Agent Deck。目前有三條路：

| 方案 | 怎麼切換 | 要改硬體嗎 | 評價 |
|---|---|---|---|
| **M-Flasher 輪刷** | 接電腦，重刷另一套韌體或還原備份 | 否 | **現在最實際** |
| **m-bootloader** | recovery menu、重裝韌體、啟動 RAM app | 否 | 是救援與開發工具，不是多系統選單 |
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

## m-apps slots 與 m-card

[m-card](https://github.com/jvitkauskas/m-card) 是一塊接在螢幕 FFC 排線上的 microSD 擴充板，不需要焊接主板，但擴充板本身要自己組。[m-apps](https://github.com/jvitkauskas/m-apps) 是從 RAM 執行的程式集合，其中的 `slots` 提供 firmware slots。

概念流程：開機 → m-bootloader 選單 → SD CARD → launcher → SLOTS → 選一個映像 → 寫回內部 flash → 重開。文件描述切換約 30 秒，不是即時的。`[調研]`

限制：`[調研]`

- m-card 仍標示為未測試的 PCB prototype。
- FM-1 上完整的 picker + slots 流程還沒有充分的實機驗證。

結論：先觀望，不急著拆機。

## `run-ram`

`m-flasher run-ram` 可以把一個小程式上傳到晶片 RAM 執行，不寫 flash，關機再開就恢復原狀。適合開發者測試自己的小程式，**不能**用來把完整的 SLOOP 或 Agent Deck 當成第二套常駐系統。它會在晶片上執行任意程式碼，在這個 repo 的規則裡屬於 agent 不得自行執行的操作。

## 之後想做的

等真的常在音樂韌體與 Agent Deck 之間切換時，包一個 host 端的 `fm1 switch <名稱>` 工作流：檢查備份、顯示 preflight、等批准、刷入、驗證。比改裝 microSD 實際。目前還沒做。
