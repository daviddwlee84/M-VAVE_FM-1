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
| 2026-11-17 | CTL-1 1.0.3 → ChoralRoot 0.41 | `just fm1-switch <檔案>` | 成功。寫入 152 個 sector 到 `0x9EFFF`，**蓋掉了 SLOOP 的 project sector** |
| 2026-11-28 | Morse 0.8 release → SLOOP 2.8.5 | `just fm1-switch sloop`（新的 Intel Mac） | 成功。寫入 140 個 sector（`0x04000`–`0x8FFFF`），CRC 相符；macOS 的 MIDI port 名稱還是 `MORSE FM-1`：這台新 Mac 第一次看到這個 USB 裝置時它是 Morse，macOS 就一直沿用這個名字，見[MIDI port 的名稱](../agentic/conductor.md#port-name) `[實機]` |
| 2026-11-25 | METRONOME 0.5 dev → Morse 0.8 release | `just fm1-switch firmware/morse/build/morse-0.8.fwsc` | 成功。寫入 43 個 sector（`0x04000`–`0x2EFFF`）；第一次 download-mode 逾時，重跑就成功 |
| 2026-11-18 | 原廠 015 → Morse 0.3 release | `fm1_install.py`（USB-MIDI，Hub 測試 3）；原廠是本人用 M-Flasher GUI 還原的 | 成功，fast install |
| 2026-11-18 | Morse 0.3 → SLOOP 2.8.5 | `fm1_install.py`（USB-MIDI，Hub 測試 1） | 成功，classic install；SLOOP 的曲子還在 |
| 2026-11-18 | SLOOP 2.8.5 → Morse 0.3 release（`FM-1_41000300`） | `fm1_install.py`（USB-MIDI，Hub 測試 2） | 成功，fast install；Morse 的紀錄在 SLOOP 期間沒被動到 |
| 2026-11-18 | Morse 0.2 → 0.3（自製） | `just fm1-switch firmware/morse/build/morse.fwsc` | 成功。寫入 32 個 sector（`0x04000`–`0x23FFF`），和預演一致 |
| 2026-11-18 | Morse 0.1 → 0.2（自製） | `just fm1-switch firmware/morse/build/morse.fwsc` | 成功。寫入 32 個 sector（`0x04000`–`0x23FFF`），學習紀錄 `0xDA000` 沒被寫，和預演一致 |
| 2026-11-17 | Conductor 0.3 → Morse 0.1（自製） | `just fm1-switch firmware/morse/build/morse.fwsc` | 成功。寫入 30 個 sector（`0x04000`–`0x21FFF`），和模擬器預演一致 |
| 2026-11-17 | ChoralRoot 0.41 → Conductor 0.1（自製） | `just fm1-switch firmware/conductor/build/conductor.fwsc` | 成功。寫入 140 個 sector（`0x04000`–`0x8EFFF`、`0x91000`–`0x91FFF`）。ChoralRoot 在 `0x93000`–`0x9EFFF` 的映像殘留還在，Conductor 不讀它 |
| 2026-11-17 | SLOOP 2.8.5 → CTL-1 1.0.3 | `just fm1-switch ctl` | 成功 |
| 2026-11-17 | Agent Deck 0.7 → SLOOP 2.8.5 | 同上，只裝韌體、不還原備份 | 寫入成功、能開機，**SLOOP 的曲子都還在** |

### SLOOP ↔ Agent Deck 互切不會丟曲子，但設定 sector 是共用的

比對刷 Agent Deck 前後的兩份完整映像，只有這些 sector 不同：

| 區域 | 位址 | 數量 | 內容 |
|---|---|---|---|
| 韌體區 | `0x04000`–`0x8FFFF` | 140 | 韌體本身 |
| 設定 sector | `0xFC000` | 1 | Agent Deck 把自己的設定寫在這裡，**蓋掉了 SLOOP 原本存在這裡的設定** |

!!! warning "更正（2026-11-17）"
    我原本寫 `0xFC000` 是「原本空著的區域」，那是錯的。M-Flasher 的 flash map 把它標為 free，但比對備份後發現 SLOOP 把設定存在那裡（開頭是 `FELU`），Agent Deck 寫入後變成自己的資料（`KDGA`），刷回 SLOOP 之後那個 sector 仍是 Agent Deck 的內容。

SLOOP 在我的機器上實際寫過的 sector（和原廠備份比對）：

| 位址 | 內容 | Agent Deck 來回之後 |
|---|---|---|
| `0x97000` | project | 沒變 |
| `0x9F000` | autosave | 沒變 |
| `0xFC000` | 設定 | **被 Agent Deck 蓋掉，尚未被 SLOOP 寫回** |
| `0xFD000` | 另一份設定（內容與 `0xFC000` 大小相同） | 沒變 |
| `0xFE000` | autosave 的另一份 | SLOOP 回來後有更新 |

曲子在 `0x97000` 與 `0x9F000`，所以都還在。設定有兩份，我推測 SLOOP 是讀到 `0xFD000` 那份才沒有明顯異狀，但這是推測。如果發現 SLOOP 的某些全域設定（燈光、SYNC、MIDI channel）跑掉了，原因就在這裡。

結論不變：SLOOP ↔ Agent Deck 來回只需要重裝 `.fwsc`，曲子不會丟。但「一個 byte 都沒碰」不成立，設定 sector 是共用的。

這個結論只對這兩個韌體、這兩個版本成立。`[實機]`

### 其他韌體會動到哪裡

Felucca 系的韌體（SLOOP、ChoralRoot、Dinghy 系的 Agent Deck）與 fm1-base 系的（CTL-1）大多沿用同一套儲存位址，所以會互相踩到。以下是把韌體蓋在我真實的 SLOOP 備份上、在模擬器裡第一次開機後改寫的 sector `[模擬]`：

| 韌體 | 第一次開機改寫 | 對我的影響 |
|---|---|---|
| CTL-1 1.0.3 | `0xDE000`–`0xDFFFF` | 在我的映像裡原本是空的。存設定時還會寫 `0xFC000` |
| RemoteFM1 0.3.6 | `0xF2000`–`0xF3FFF` | `0xF2000` 有原廠留下的資料，會被蓋掉 |
| ChoralRoot 0.41 | 沒有改寫；正常開機 | **韌體映像本身較大（到 `0x9EFFF`），安裝時就會蓋掉 SLOOP 在 `0x97000` 的 project。** 上游另記載過舊版在 SLOOP 資料上掉進 ROM boot mode，見 [ChoralRoot](choralroot/index.md) |
| Morse 0.1（自製） | 開機不寫；改設定或玩完一回合才寫 `0xDA000`–`0xDBFFF` `[模擬]`；2026-11-17 已刷進實機 | 韌體映像 `0x04000`–`0x92FFF`。和目前機上的 Conductor 只差 27 個 sector（`0x04000`–`0x1EFFF`）。預演疊在 `2026-11-17-2052-pre-conductor` 備份上，見 [Morse](../dev/morse.md#flash) |
| Conductor 0.1（自製） | 沒有改寫，重開再讀回也是 0 次寫入 `[模擬]`；2026-11-17 已刷進實機 | 韌體映像 `0x04000`–`0x92FFF`，不到 `0x97000`。只讀 panel calibration（`0xDC000`），不碰 `0xFC000`。預演是疊在 `2026-11-17-0413-pre-choralroot-0.41` 備份上做的，見 [Conductor](../agentic/conductor.md#flash) |

### 先在模擬器預演

```sh
just emu-rehearse downloads/ctl-1.0.3.fwsc backups/<目錄>/<檔名>.bin
```

它把套件的韌體區蓋在一份真實備份上（和 M-Flasher 對 SLOOP、Agent Deck 做的事一樣），在模擬器裡開機。可以看新韌體遇到舊資料時會不會正常啟動；結束後比對 `tools/emu-state/rehearse-<名稱>.before` 與 `.flash` 就知道它改寫了哪些 sector。

它會印出套件映像的範圍，以及這個範圍蓋到了目前韌體的哪些資料 sector。大多數韌體是 `0x04000`–`0x92FFF`，ChoralRoot 到 `0x9EFFF`。

限制：套件映像的範圍取自模擬器鋪出來的內容；M-Flasher 對某個套件實際寫多大，要看它自己列的計畫。模擬器也不是實機。

順帶一提：模擬器直接吃實機備份的格式。把備份複製成 `--flash` 檔再開，跑的就是那份備份裡的韌體與資料，我的 SLOOP 曲子在模擬器裡照樣出現。`[模擬]`

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

具名目標：`sloop`、`agtdeck`、`ctl`、`choralroot`、`remote`，其他韌體把 `.fwsc` 的路徑傳進去。後三個用 `just fm1-fetch-more` 下載。清單也會列出 `downloads/` 與 `firmware/*/build/` 裡現有的套件。

它依序做：確認沒有模擬器佔著 USB MIDI → 切 download mode → 備份到 `backups/<日期時間>-pre-<套件名>/` → 呼叫 M-Flasher 安裝（列出變更，等你輸入 `yes`）→ 重開機。屬於寫入等級，由本人執行。

目標韌體已經裝著的時候，M-Flasher 會回報 `Nothing to write` 然後直接重開，不會問 `yes`。`[實機]`

每次切換都會留一份刷前備份在本機，不會自動上傳。

## 兩種寫入路徑：download mode 與 USB-MIDI 更新 { #paths }

2026-11-18 為了 Hub 的上架測試，兩條路都在實機上走過 `[實機]`：

| | `just fm1-switch`（M-Flasher，download mode） | `fm1_install.py`（USB-MIDI 更新，和 Hub 網頁安裝同一套） |
|---|---|---|
| 誰負責寫 | 晶片的 download mode（`WL82 UBOOT1.00`），M-Flasher 先把自己的 loader 載進 RAM | 正在跑的韌體讀取新套件的開頭，啟動套件附的 update loader（暫放在 `0xE0000`），由它寫入 |
| 步驟 | 切 download mode → 讀整顆 flash 做備份 → 列出要寫的 sector → 輸入 `yes` → 寫 → 整顆 CRC32 驗證 → 重開 | 確認套件 → 輸入 `y` → 進更新模式 → 寫 → 重開 → 確認新的 identity |
| 寫多少 | 只寫和現在不同的 sector | 看 loader：fm1-base 的 loader 只寫有變動的（fast install）；SLOOP 2.8.5 附的較舊，整包重寫（classic install） |
| 自動備份 | 有 | 沒有，要先自己 `just fm1-backup` |
| 現在的韌體壞了 | 照樣能用，只要 boot 區沒壞 | 不能用，現在的韌體要懂更新指令 |
| 中斷後 | 重跑 | loader 停在更新模式，重跑就從中斷處繼續（fm1-base 的說明） |
| 感覺 | 步驟多、比較慢 | 步驟少、比較順 |

**怎麼選**：

- **開發、或不確定新韌體能不能開機時**：用 `fm1-switch`。它會自動備份、整顆驗證，而且不依賴現在跑的韌體。
- **兩個都正常的韌體之間切換**：用 USB-MIDI 就好，這也是一般使用者從 Hub 網頁安裝時走的路。

兩條路在 AGENTS.md 都算韌體寫入，一樣要先核准、由本人執行。

```sh
uv run --locked --all-groups python third_party/fm1-base/tools/fm1_install.py --info        # 唯讀：現在的 identity
uv run --locked --all-groups python third_party/fm1-base/tools/fm1_install.py PACKAGE.fwsc  # 寫入：本人執行
```

## 之後想做的

- 切換後自動確認裝置回到 USB MIDI、並提示要不要啟動對應的 host 程式（Agent Deck 的 daemon、SLOOP 的 Web Editor）。
- 備份累積多了之後的整理方式。
