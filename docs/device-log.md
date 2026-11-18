# 我的機器

這台 FM-1 目前的狀態與操作歷史。任何對實機的操作，做之前先讀這頁，做完更新這頁。這頁的內容都是 `[實機]`。

## 現況

| 項目 | 值 |
|---|---|
| 型號 | M-VAVE FM-1 |
| Flash 晶片 / JEDEC ID | **`0xB36014` (new)**，UCUN UC25HQ80，1 MiB |
| Bootloader | stock (M-VAVE) |
| 出廠韌體版本 | FM-1 015 (stock)，firmware check OK |
| 目前韌體 | **THEORY 0.3 dev build（`FM-1_4900`，只有 USB，commit `92267f6`）**（2026-11-18 23:00 前後從 `a0a14e9` 那版重刷，fast install；22:35 前後從 THEORY 0.2 更新，fast install；設定和閃卡進度保留；Morse 的紀錄留在 `0xDA000`）。之前是 THEORY 0.2 dev build（2026-11-18 21:30 前後從 Morse 0.8 用 USB-MIDI 更新，fast install；THEORY 0.1 的設定沿用，閃卡進度從頭開始；Morse 的紀錄 ROUND 30 留在 `0xDA000`）。之前是 Morse 0.8 release build（`FM-1_41000800`，藍牙；2026-11-18 19:55 前後從 Morse 0.7 用 USB-MIDI 更新（fast install），學習紀錄 ROUND 31 和設定保留；THEORY 的存檔留在 `0xD8000`）。之前是 Morse 0.7 release build（`FM-1_41000700`，19:45 前後從 THEORY 0.1 更新）。之前是 THEORY 0.1 dev build（`FM-1_4900`，只有 USB，2026-11-18 19:00 前後從 Morse 0.6 更新）。再之前是 Morse 0.6 release build（`FM-1_41000600`，藍牙；2026-11-18 從 0.4 → 0.5（16:20）→ 0.6 用 USB-MIDI 更新，學習紀錄保留；macOS 上的 USB MIDI port 名稱仍是 `Felucca`）。0.3 release 是 Hub 測試 3 從原廠 015 裝的，進度當時從 ROUND 1 重新開始。之前依序是原廠 015 → SLOOP 2.8.5 → Agent Deck 0.7 → SLOOP 2.8.5 → CTL-1 1.0.3 → ChoralRoot 0.41 → Conductor 0.1 → 0.2 → 0.3 → Morse 0.1 → 0.2 → 0.3 → SLOOP 2.8.5 → Morse 0.3 release → 原廠 015（還原）→ Morse 0.3 release → Morse 0.4 → Morse 0.5 → Morse 0.6 → THEORY 0.1 → Morse 0.7 → Morse 0.8 → THEORY 0.2 → THEORY 0.3 |
| SLOOP 的曲子 | **已不在機器上**（`0x97000` 被 ChoralRoot 的韌體蓋掉）。完整保存在 `2026-11-17-0341-pre-ctl-1.0.3` 備份裡；回 SLOOP 要在 M-Flasher GUI 還原那份備份 |
| m-bootloader | 未安裝 |
| 主機 | macOS，M-Flasher 0.12.1 |

## 這代表什麼

- 這台是**新晶片**機。裝任何自訂韌體前都要確認它寫明支援 UC25HQ80，見[兩種 Flash 晶片](hardware/flash-chips.md)。SLOOP 2.8.5 有支援。
- m-bootloader 在這顆晶片上測試很少，暫不安裝。這也表示 SLOOP 3 暫時不刷。
- 出廠是 015，不是 016，所以沒有 016 覆寫 preset 區的問題。不要用 M-UPGRADE 升到 016，除非確定要。

## 備份

備份檔放在本機的 `backups/`（gitignore，不進 git），異地副本用 `just fm1-backup-release <目錄>` 上傳成這個 repo 的 Release 附件。這個 repo 是 public，所以附件任何人都能下載；這是我自己的決定，映像只適用於這台機器。

| 日期 | 內容 | 檔名 | SHA-256 | 異地副本 |
|---|---|---|---|---|
| 2026-11-17 | 原廠完整 flash（015，stock bootloader） | `backups/2026-11-17-stock/fm1-20261117-011632.bin` + `.json` | `c6a4790266bdb9e97500c20f50a98b80f0d334d8a219e1744bfc8e656a44fd38` | [GitHub Release](https://github.com/daviddwlee84/M-VAVE_FM-1/releases/tag/backup-2026-11-17-stock)（2026-11-17，digest 相符） |
| 2026-11-17 | SLOOP 2.8.5 裝好並玩過之後的完整 flash（含當時的曲子與設定） | `backups/2026-11-17-sloop-2.8.5/fm1-20261117-014153.bin` + `.json` | `783060c08ac0727916e997dfc0b295a7469a45afe0ab4d460a4d5bc9bfc018b2` | [GitHub Release](https://github.com/daviddwlee84/M-VAVE_FM-1/releases/tag/backup-2026-11-17-sloop-2.8.5)（2026-11-17，digest 相符） |
| 2026-11-17 | 刷 Agent Deck **之前**的 SLOOP 2.8.5（含當時最新的曲子） | `backups/2026-11-17-pre-agtdeck/fm1-20261117-023458.bin` + `.json` | `074cf36077e4e689aa2c4aae57baa7386428a73afd667ca042864714777546bc` | GitHub Release |
| 2026-11-17 | Agent Deck 0.7 刷入後 | `backups/2026-11-17-agtdeck-0.7/fm1-20261117-023846.bin` + `.json` | `8345795a8cfa07afc71bc196c46683e2e35916e677b8fc807cee3ac648b8767d` | 尚未上傳 |

M-Flasher 在兩次寫入前各自動存了一份：`~/Documents/M-Flasher/backups/fm1-20261117-023538.bin`、`fm1-20261117-024717.bin`。`pre-agtdeck` 已上傳到 [GitHub Release](https://github.com/daviddwlee84/M-VAVE_FM-1/releases/tag/backup-2026-11-17-pre-agtdeck)（digest 相符）。
| 2026-11-17 | 刷 CTL-1 之前的 SLOOP 2.8.5 | `backups/2026-11-17-0341-pre-ctl-1.0.3/fm1-20261117-034125.bin` + `.json` | `8c9267e87a320a1e4cd592a9261907d978b8f3fdb94eaf15e685afae267b4451` | 尚未上傳 |
| 2026-11-17 | 刷 ChoralRoot 之前的 CTL-1（仍含 SLOOP 的 project sector） | `backups/2026-11-17-0413-pre-choralroot-0.41/fm1-20261117-041350.bin` + `.json` | `8834d0188eb309c7229fb3ad9e89a76a349eca546accb666b8a741509a66eb10` | 尚未上傳 |
| 2026-11-17 | 刷 Conductor 之前的 ChoralRoot 0.41 | `backups/2026-11-17-pre-conductor/fm1-20261117-183840.bin` + `.json` | `81f048bb0e8bea174ccb84df4de2555c891b087651083710c41c428a0e7ba1e1` | 尚未上傳 |
| 2026-11-17 22:52 | 刷 Morse 之前的 Conductor（identity `FM-1 120`；依 [Conductor 版本紀錄](agentic/conductor-changelog.md) 是 0.3） | `backups/2026-11-17-pre-morse/fm1-20261117-225206.bin` + `.json` | `fbcb47909c37828ec92dd890d3a80b26ab61899f59d1743013ad9bd4250ca0ef` | 尚未上傳 |

`fm1-switch` 在寫入前又自動備份了一份 `backups/2026-11-17-2253-pre-morse/fm1-20261117-225404.bin`，SHA-256 和上面這份相同；M-Flasher 另存 `~/Documents/M-Flasher/backups/fm1-20261117-225412.bin`。

| 日期 | 內容 | 檔名 | SHA-256 | 異地副本 |
|---|---|---|---|---|
| 2026-11-18 00:12 | 刷 Morse 0.2 之前的 Morse 0.1（含學習紀錄：玩過 3 回合、聽 15/14 WPM） | `backups/2026-11-18-pre-morse-0.2/fm1-20261118-001247.bin` + `.json` | `35457c78575749cdaed88cea833f5f8a45dd7cbabd79a6e8762f8677a338ada4` | 尚未上傳 |
| 2026-11-18 02:12 | 刷 Morse 0.3 之前的 Morse 0.2（含學習紀錄：ROUND 5、聽 20/8） | `backups/2026-11-18-pre-morse-0.3/fm1-20261118-021205.bin` + `.json` | `64e28fc9f9219abbcc8453d62f0f5132d980c27664c4626dd49f2424c06cc60f` | 尚未上傳 |
| 2026-11-18 02:33 | Hub 測試 1（Morse → SLOOP，走 USB-MIDI）之前的 Morse 0.3 開發版（`FM-1_1300`） | `backups/2026-11-18-pre-hub-test-1/fm1-20261118-023344.bin` + `.json` | `c2b315877b9a7e0edc59c308c866c166db68c2005a8aa000c82789cd732dfc3c` | 尚未上傳 |
| 2026-11-18 02:39 | Hub 測試 2（SLOOP → Morse release，走 USB-MIDI）之前的 SLOOP 2.8.5（`0xDA000` 仍有 Morse 的紀錄） | `backups/2026-11-18-pre-hub-test-2/fm1-20261118-023901.bin` + `.json` | `deacf5153e17b52a6675a429bf063cb76e29ffff01d87de3214cd824d4151464` | 尚未上傳 |
| 2026-11-18 02:42 | Hub 測試 3 之前的 Morse 0.3 release（`FM-1_41000300`，含學習紀錄）：測完要回到這裡就還原這份 | `backups/2026-11-18-pre-hub-test-3/fm1-20261118-024232.bin` + `.json` | `5601847d2505efa80d85ec611a0813f2854bb9f40c91cb49d47d65299f7e50b5` | 尚未上傳 |
| 2026-11-18 04:47 | 刷 Morse 0.4 之前的 Morse 0.3 release（`FM-1_41000300`，含學習紀錄：ROUND 15、S N A P T 剛解鎖） | `backups/2026-11-18-pre-morse-0.4/fm1-20261118-044757.bin` + `.json` | `73a530d20bc6409488a28717a2f5c5817a7083c023dc88f657ecd37fcc82602b` | 尚未上傳 |
| 2026-11-18 18:56 | 刷 THEORY 0.1（dev build）之前的 Morse 0.6 release（`FM-1_41000600`，`0xDA000` 有學習紀錄，`0xD8000`–`0xD9FFF` 是空的） | `backups/2026-11-18-pre-theory-0.1/fm1-20261118-185652.bin` + `.json` | `5b62a3daeed2f23c7198f4f0cf56f02aaf432355727d78e6f1e8586beed879ea` | 尚未上傳 |
| 2026-11-18 19:38 | 刷 Morse 0.7 之前的 THEORY 0.1 dev build（`FM-1_4900`；`0xDA000` 是 Morse 0.6 的學習紀錄 ROUND 30，`0xD8000`–`0xD9FFF` 是 THEORY 的存檔） | `backups/2026-11-18-pre-morse-0.7/fm1-20261118-193823.bin` + `.json` | `4c3d061d886c58271e26bf05822ba9f6ff957ce551a95d338d93a05cd1712e85` | 尚未上傳 |
| 2026-11-18 19:50 | 刷 Morse 0.8 之前的 Morse 0.7 release（`FM-1_41000700`；`0xDA000` 的紀錄 ROUND 31，設定 START PAGE 是 PRACTICE、HEAR 20/8） | `backups/2026-11-18-pre-morse-0.8/fm1-20261118-195019.bin` + `.json` | `d5470954e001d8cea58acffce5ca338e008451acedb04a9dd98cdd38e4bfb31f` | 尚未上傳 |
| 2026-11-18 21:20 | 刷 THEORY 0.2（dev build）之前的 Morse 0.8 release（`FM-1_41000800`；`0xDA000` 的紀錄 ROUND 30；`0xD8000` 有 THEORY 0.1 的設定） | `backups/2026-11-18-pre-theory-0.2/fm1-20261118-212058.bin` + `.json` | `d388c6c48d6d4b26c75dc0a3747c0049f0c999ef31b52edda4cfd54eef6dc874` | 尚未上傳 |
| 2026-11-18 22:30 | 刷 THEORY 0.3（dev build）之前的 THEORY 0.2 dev build（`FM-1_4900`；`0xD6000` 有 0.2 的閃卡進度 round 1、`0xD8000` 設定、`0xDA000` Morse 的紀錄） | `backups/2026-11-18-pre-theory-0.3/fm1-20261118-223033.bin` + `.json`；閃卡進度另存 `backups/progress/theory-2026-11-18-pre-0.3.json` | `eab8cd547c228a7f746a46b866bc719ab1176c854dd4771b654a4613762fa681` | 尚未上傳 |
| 2026-11-18 22:56 | 重刷 THEORY 0.3（commit `92267f6`）之前的 THEORY 0.3（commit `a0a14e9`） | `backups/2026-11-18-pre-theory-0.3b/fm1-20261118-225644.bin` + `.json`；閃卡進度另存 `backups/progress/theory-2026-11-18-pre-0.3b.json` | `e382d5b321129973d9019f69f755d0fdf4f1b40c66950b1995262c0da1fbda7e` | 尚未上傳 |

## 下載的檔案

放在本機的 `downloads/`（gitignore）。SHA-256 已與 GitHub release asset 的 digest 比對一致。

| 日期 | 檔案 | SHA-256 | 來源 |
|---|---|---|---|
| 2026-11-17 | `M-Flasher-0.12.1-macos.dmg` | `ccb469a1ceead508e50a42b6c02946ea0d0caee18a9d06ec830c268ee8717192` | [m-flasher v0.12.1](https://github.com/jvitkauskas/m-flasher/releases/tag/v0.12.1) |
| 2026-11-17 | `sloop-2.8.5.fwsc` | `b0446effb840740a3aa5c386ce004e79334e4f833d48d82a9eb5edbfde92ae67` | [sloop-fm1 v2.8.5](https://github.com/isod89/sloop-fm1/releases/tag/v2.8.5) |
| 2026-11-17 | `sloop-editor-2.8.5.html` | `25fe6043084b6d6bdd0529699e2b8cf6f4ba186f7d04801c2564882dabb76f03` | 同上 |
| 2026-11-17 | `fm1-emu-0.5.1-macos-arm64.tar.gz` | `b5df55e1450210ae88be4ddfa903e608fa495a58bf72775d30047308e48b9895` | [fm1-emu v0.5.1](https://github.com/joakimeriksson/fm1-emu/releases/tag/v0.5.1) |
| 2026-11-17 | `agtdeck-0.7.fwsc` | `c0d0b43e688ca0f7f9911dce9ee0bdffb00b1127a66bc4629a934e087a97eacf` | [fm-1-agent-deck v0.7](https://github.com/kevinelliott/fm-1-agent-deck/releases/tag/v0.7)，build attestation 驗證通過 |

## 操作歷史

| 日期 | 操作 | 結果 |
|---|---|---|
| 2026-11-17 | 下載並驗證 M-Flasher 0.12.1 與 SLOOP 2.8.5 | hash 相符 |
| 2026-11-17 | 安裝 M-Flasher，GUI 按 DOWNLOAD MODE | 成功。原廠 015 可由 USB MIDI 切進 download mode；整顆讀取 1.3 s（813 KB/s），晶片 CRC32 檢查通過 |
| 2026-11-17 | M-Flasher 詢問是否安裝 m-bootloader | 選 No（新晶片，暫不安裝） |
| 2026-11-17 | BACKUP FULL FLASH | 已存，SHA-256 與 `.json` 內記錄一致；原始檔在 `~/Documents/M-Flasher/backups`，另複製到 repo 的 `backups/` |
| 2026-11-17 | GUI：FLASH FIRMWARE `sloop-2.8.5.fwsc` → REVIEW & FLASH | 計畫：寫 143 個 sector（572 KiB），`0x04000`–`0x92FFF`，只在 FIRMWARE 區，BOOT 區未動。01:23 完成，143 sectors written, read back and verified |
| 2026-11-17 | 拔線、關機再開機 | 進入 SLOOP；存檔測試通過（存檔後重開機內容還在） |
| 2026-11-17 | `just fm1-find`（SLOOP 執行中） | 回報 `midi Felucca (running; download-mode switches it)`：SLOOP 在 USB MIDI 上的名稱是 Felucca，M-Flasher 認得它 |
| 2026-11-17 | `just fm1-backup-release 2026-11-17-stock` | 原廠備份上傳為 Release `backup-2026-11-17-stock`，asset digest 與本機 SHA-256 一致 |
| 2026-11-17 | SLOOP 執行中進 download mode，`just fm1-backup sloop-2.8.5` | 成功。讀取 1.3 s（801 KB/s），CRC32 相符；M-Flasher 回報 firmware `SLOOP 2.8.5`、app OK、bootloader stock |
| 2026-11-17 | `just fm1-backup-release 2026-11-17-sloop-2.8.5` | 上傳為 Release `backup-2026-11-17-sloop-2.8.5`，digest 相符 |
| 2026-11-17 | `just fm1-download-mode`（SLOOP 執行中、已停止播放） | 成功，不需要按鍵。`just fm1-backup pre-agtdeck` 完成 |
| 2026-11-17 | 本人執行 `just fm1-flash downloads/agtdeck-0.7.fwsc` | 計畫：140 sectors（560 KiB）`0x04000`–`0x8FFFF`，BOOT 區未動。寫入完成，晶片 CRC32 相符 |
| 2026-11-17 | `just fm1-restart` | 機器已在執行。USB 產品名 `FM-1 AGTDECK`；macOS 的 MIDI 名稱仍顯示 `Felucca`（系統快取） |
| 2026-11-17 | Agent Deck 執行中 `just fm1-download-mode` → `fm1-backup agtdeck-0.7` → `fm1-restart` | 全部成功。Agent Deck 也能由 USB MIDI 切進 download mode，不需要按鍵 |
| 2026-11-17 | Agent Deck 執行中 `just fm1-download-mode`，本人執行 `just fm1-flash downloads/sloop-2.8.5.fwsc` | 計畫：140 sectors（560 KiB）`0x04000`–`0x8FFFF`。寫入完成，晶片 CRC32 相符。`just fm1-restart` 後機器在 USB MIDI 上 |
| 2026-11-17 | 刷回 SLOOP 後，之前的曲子是否還在 | **還在。** 只重裝韌體、沒有還原備份 |
| 2026-11-17 | 比對 `pre-agtdeck` 與 `agtdeck-0.7` 兩份映像（4 KiB sector） | 141 個 sector 不同：韌體區 `0x04000`–`0x8FFFF` 的 140 個，加上 `0xFC000` 一個（Agent Deck 自己的設定）。SLOOP 存資料的區域完全沒被動到 |
| 2026-11-17 | 測試 `just fm1-switch sloop`（SLOOP 已安裝） | 切 download mode、備份、M-Flasher 回報 `Nothing to write`、重開，全程沒有寫入 |
| 2026-11-17 | 更正先前的紀錄 | `0xFC000` 不是空的：它是 SLOOP 的設定 sector，被 Agent Deck 覆寫，刷回 SLOOP 後尚未寫回。曲子所在的 `0x97000`、`0x9F000` 沒變。詳見[多韌體切換](firmware/switching.md) |
| 2026-11-17 | 下載並驗證 `ctl-1.0.3.fwsc`、`choralroot-0.41-smk37.1.fwsc`、`remote-0.3.6.fwsc` | digest 相符；模擬器兩種晶片都開機。都還沒刷進實機 |
| 2026-11-17 | 本人執行 `just fm1-switch ctl` | 第一次用這個捷徑實際寫入。download mode → 備份 → 140 sectors（`0x04000`–`0x8FFFF`）→ CRC32 相符 → 自動重開，全程只輸入一次 `yes` |
| 2026-11-17 | CTL-1 接 iPad | 能用。iPad 的聲音輸出自動改到 FM-1（USB audio out），且 CTL-1 自己的 GM 音色會疊上去；處理方式見 [CTL-1](firmware/ctl-1/index.md#ipad-audio) |
| 2026-11-17 | 更正：ChoralRoot 套件下載錯誤 | 先前下載的是 SMK-37 版（`v0.41-smk37.1`），已刪除並改下載 FM-1 版 `choralroot-0.41.fwsc`（`8da57076…1ce0652`，digest 相符）。錯的檔案從未刷進機器 |
| 2026-11-17 | 本人執行 `just fm1-switch downloads/choralroot-0.41.fwsc` | M-Flasher 的計畫：`Writes 152 sectors (608 KiB): 0x04000-0x8EFFF, 0x91000-0x91FFF, 0x93000-0x9EFFF`。寫入後機器重開，USB 產品名 `ChoralRoot FM-1`。M-Flasher 自動備份 `fm1-20261117-041838.bin` |
| 2026-11-17 | ChoralRoot 在新晶片實機、且 flash 裡有 SLOOP 與 CTL-1 殘留資料的狀態下 | 開得起來、USB MIDI 正常。上游記載的「在 Sloop 資料上掉進 ROM boot」在這台、這個版本沒有發生 |
| 2026-11-17 18:38 | agent 執行 `just fm1-download-mode`、`fm1-info`、`fm1-backup pre-conductor`（刷 Conductor 前的 preflight） | 切換成功（`WL82 UBOOT1.00`）。`info`：model FM-1、bootloader stock (M-VAVE)、firmware Felucca、app OK、flash `0xB36014`，晶片 CRC32 與讀取相符。備份 SHA-256 `81f048bb…7ba1e1`，`fm1-verify-backups` ok。機器停在下載模式，等本人刷寫 |
| 2026-11-17 18:40 | 本人執行 `just fm1-switch firmware/conductor/build/conductor.fwsc`（SHA-256 `de182723…0502f3`，commit `d45ce67`） | 先自動備份 `backups/2026-11-17-1839-pre-conductor/fm1-20261117-184005.bin`（SHA-256 與 18:38 那份相同，`81f048bb…7ba1e1`）。M-Flasher 的計畫：`Writes 140 sectors (560 KiB): 0x04000-0x8EFFF, 0x91000-0x91FFF`，安裝範圍 `0x04000-0x92FFF`。寫入後全部驗證通過（晶片 CRC32 相符），M-Flasher 自動備份 `fm1-20261117-184013.bin`，機器重開 |
| 2026-11-17 | Conductor 在實機開機 | 螢幕正常；macOS 的 MIDI port 名稱沿用 `Felucca`，daemon 照名字找不到 → 改成聽協定找 port。裝置 0.4 秒內送出 Conductor 的 HELLO，本人的 daemon 連上，裝置改送 HEARTBEAT，SELECT 有回傳。Conductor 不寫 flash，資料區沒有變動（模擬器預演結果，實機尚未用備份比對） |
| 2026-11-17 18:59 | 本人執行 `just fm1-switch firmware/conductor/build/conductor.fwsc`（0.2，SHA-256 `48a052f9…faea6d`，commit `5add525`） | 先自動備份 `backups/2026-11-17-1859-pre-conductor/fm1-20261117-185927.bin`（Conductor 0.1 的狀態，SHA-256 `71dd8a47533b3bda6da1d001ad59989cdcbb2c69dde280dff240cf5aed9c1c32`）。M-Flasher 的計畫：`Writes 22 sectors (88 KiB): 0x04000-0x19FFF`，驗證通過，自動備份 `fm1-20261117-185935.bin`。重開後 daemon 回報 `firmware 0.2`；輔助使用權限的警告消失 |
| 2026-11-17 22:51 | agent 執行 `just fm1-download-mode`、`fm1-info`、`fm1-backup pre-morse`（刷 Morse 0.1 前的 preflight） | 切換成功（`WL82 UBOOT1.00`）。`info`：model FM-1、bootloader stock (M-VAVE)、firmware `FM-1 120`（Conductor）、app OK、flash `0xB36014`，晶片 CRC32 與讀取相符。備份 SHA-256 `fbcb4790…250ca0ef`。用這份備份預演：Morse 和它不同的是 `0x04000`–`0x21FFF` 的 30 個 sector，全在韌體區；`0xDA000`–`0xDBFFF` 是空的。機器留在 download mode 等本人執行寫入 |
| 2026-11-17 22:54 | 本人執行 `just fm1-switch firmware/morse/build/morse.fwsc`（Morse 0.1，SHA-256 `dfb23a70…27c9a`，commit `c3b5cd6`） | 先自動備份 `backups/2026-11-17-2253-pre-morse/`（SHA-256 同 22:52 那份）。M-Flasher 的計畫：`Writes 30 sectors (120 KiB): 0x04000-0x21FFF`、`Installs FM-1_1300 (0x04000-0x92FFF)`，和預演一致；寫入後晶片 CRC32 相符，自動重開。重開後 `fm1-find` 顯示 `Felucca (running)`；`python -m morse send` 依協定找到 port，收到 Morse 的 HELLO（韌體 0.1）並送出文字 |
| 2026-11-17 22:56–23:00 | Morse 0.1 實機測試：本人操作，agent 在電腦端記錄 MIDI 與 `T_CHAR`（4 分鐘） | KEY 頁直鍵發報解碼正確（S、H、O、T…），按鍵以 `90 00 7F`／`80 00 00` 送出；TYPE 頁每按一字送出 `T_CHAR`，播放時序精準（dit 60 ms、Farnsworth 字間 652 ms），K1 調速生效（之後 dit 57 ms）。一次 SOS 讀成 `S M T S`：O 的 dah 之間隔 146、175 ms，和字間（169–224 ms）一樣長。看到一次 9 ms 的按鍵彈跳：解碼器忽略了，MIDI 照送。本人回饋：VVV「好像有聽到」、PRACTICE 玩了一回合「還可以」、TYPE「不錯」；初學打 SOS 成功率不高，希望看到自己打的點劃 → 做進韌體 0.2 |
| 2026-11-18 00:10 | agent 執行 `just fm1-download-mode`、`fm1-info`、`fm1-backup pre-morse-0.2`（刷 Morse 0.2 前的 preflight） | 切換前機器送出 Morse 0.1 的 HELLO。第一次 `fm1-info` 卡住超過 2 分鐘、沒有輸出（中止）；重試時 M-Flasher 重新載入 loader，1.3 s 讀完：model FM-1、bootloader stock、firmware `FM-1 130`（Morse）、app OK、flash `0xB36014`，CRC32 相符。備份 SHA-256 `35457c78…338ada4`。用這份備份預演 0.2：不同的是 `0x04000`–`0x23FFF` 的 32 個 sector，全在韌體區；0.1 的紀錄（`0xDA000`–`0xDBFFF`）升級後保留（ROUND 4、聽 15/14、發報速度補成 10）`[模擬]` |
| 2026-11-18 00:14 | 本人執行 `just fm1-switch firmware/morse/build/morse.fwsc`（Morse 0.2，SHA-256 `fc3dd404…556b0`，commit `54ff440`） | 先自動備份 `backups/2026-11-18-0013-pre-morse/`（SHA-256 同 00:12 那份）。M-Flasher 的計畫：`Writes 32 sectors (128 KiB): 0x04000-0x23FFF`、`Installs FM-1_1300 (0x04000-0x92FFF)`，和預演一致；寫入後 CRC32 相符，自動重開，M-Flasher 另存 `fm1-20261118-001409.bin`。重開後 `python -m morse send` 收到 HELLO（韌體 0.2）並送出 `<SK>` |
| 2026-11-18 | Morse 0.2 實機試用（本人） | 開機仍是 ROUND 4：升級保留學習進度。RHYTHM、KEY 三種顯示、TYPE 翻頁和片語、LISTEN、PRESET「感覺都還不錯」。TYPE 打開鍵上點劃時黑鍵的字母被擠掉 → 韌體 0.3 修正（還沒刷） |
| 2026-11-18 02:11 | agent 執行 `just fm1-download-mode`、`fm1-info`、`fm1-backup pre-morse-0.3`（刷 Morse 0.3 前的 preflight） | 切換前機器送出 Morse 0.2 的 HELLO。`info`：model FM-1、bootloader stock、firmware `FM-1 130`、app OK、flash `0xB36014`，CRC32 相符。備份 SHA-256 `64e28fc9…c06cc60f`。用這份備份預演 0.3：不同的是 `0x04000`–`0x23FFF` 的 32 個 sector，全在韌體區；紀錄（ROUND 5）升級後保留 `[模擬]` |
| 2026-11-18 02:12 | 本人執行 `just fm1-switch firmware/morse/build/morse.fwsc`（Morse 0.3，SHA-256 `64bfa57d…f8d4ea`，commit `02b0ea2`） | 先自動備份 `backups/2026-11-18-0212-pre-morse/`（SHA-256 同 02:12 那份）。M-Flasher 的計畫：`Writes 32 sectors (128 KiB): 0x04000-0x23FFF`，和預演一致；寫入後 CRC32 相符，自動重開，M-Flasher 另存 `fm1-20261118-021254.bin`。重開後 HELLO 回報 0.3；`fm1_install.py --info`（USB-MIDI 唯讀詢問）回答 `FM-1_1300 [running]` |
| 2026-11-18 02:33 | agent 執行 `just fm1-download-mode`、`fm1-backup pre-hub-test-1`、`fm1-restart`（Hub 測試 1 的準備） | 備份 SHA-256 `c2b31587…732dfc3c`，重開回 Morse 0.3 |
| 2026-11-18 02:40 | Hub 測試 1：本人執行 `fm1_install.py downloads/sloop-2.8.5.fwsc`（USB-MIDI 更新路徑，和 Hub 網頁安裝同一套），機器上是 Morse 0.3（`FM-1_1300`） | 成功：`checking the package (18 reads)` → `switching to update mode` → `the loader reads and writes the whole package (classic install)` → `done: the FM-1 runs FM-1_900`。重開進 SLOOP，之前的曲子還在。證明可以從 Morse 換到別的韌體 |
| 2026-11-18 02:39 | agent 執行 `just fm1-download-mode`（SLOOP 執行中）、`fm1-backup pre-hub-test-2`、`fm1-restart`，`fm1_install.py --info` | 備份 SHA-256 `deacf515…d4151464`，M-Flasher 回報 firmware `SLOOP 2.8.5`；重開回 SLOOP，`--info` 回答 `FM-1_900 [running]`。備份裡 `0xDA000`–`0xDBFFF` 的 Morse 紀錄沒被 SLOOP 動到 |
| 2026-11-18 02:41 | Hub 測試 2：本人執行 `fm1_install.py firmware/morse/build/morse-0.3.fwsc`（Morse release，`FM-1_41000300`，SHA-256 `1c563fda…c0f1b3`），機器上是 SLOOP 2.8.5 | 成功：`checking the package (30 reads)` → `switching to update mode` → `the loader writes only the sectors that changed (fast install)` → `done: the FM-1 runs FM-1_41000300`。證明可以從別的韌體裝 Morse。本人覺得比 `fm1-switch` 順，比較見[兩種寫入路徑](firmware/switching.md#paths) |
| 2026-11-18 02:42 | agent 執行 `just fm1-download-mode`、`fm1-backup pre-hub-test-3`、`fm1-restart`，`fm1_install.py --info` | M-Flasher 回報 firmware `FM-1 410`；備份 SHA-256 `5601847d…f7e50b5`；重開回 Morse，`--info` 回答 `FM-1_41000300 [running]`。原廠備份重算 SHA-256 仍是 `c6a47902…44fd38` |
| 2026-11-18 02:45 | 本人用 M-Flasher GUI 還原 `backups/2026-11-17-stock/fm1-20261117-011632.bin`（RESTORE FULL FLASH） | 原廠開機；`fm1-find` 顯示 `midi FM-1 (running)`，`fm1_install.py --info` 回答 `FM-1_015 [running] port: FM-1` |
| 2026-11-18 02:46 | agent 執行 `just fm1-download-mode`（原廠執行中）、`fm1-backup pre-hub-test-3-stock`、`fm1-restart` | M-Flasher 回報 bootloader stock、firmware `FM-1 015 (stock)`；備份 SHA-256 `c6a47902…44fd38`，**和 11/17 的原廠備份逐 sector 完全相同**：還原是完整的。重開回原廠 |
| 2026-11-18 02:48 | Hub 測試 3：本人執行 `fm1_install.py firmware/morse/build/morse-0.3.fwsc`，機器上是原廠 015（`FM-1_015`，port `FM-1`） | 成功：`checking the package (34 reads)` → `switching to update mode` → `the loader writes only the sectors that changed (fast install)` → `done: the FM-1 runs FM-1_41000300`。PRACTICE 從 ROUND 1 開始（原廠備份裡沒有 Morse 紀錄）。Hub 要求的三項實機測試全部通過 |
| 2026-11-18 04:47 | agent 執行 `fm1_install.py --info`、`just fm1-download-mode`、`fm1-backup pre-morse-0.4`、`fm1-restart`（刷 Morse 0.4 前的 preflight） | `--info` 回答 `FM-1_41000300 [running]`；flash `0xB36014`；備份 SHA-256 `73a530d2…2e9b556`，`0xDA000`–`0xDBFFF` 有 Morse 的紀錄；重開回 Morse 0.3。0.4 的紀錄格式沒變，[模擬] 疊在 0.2／0.3 紀錄上升級都保留 |
| 2026-11-18 05:20 | 本人執行 `fm1_install.py firmware/morse/build/morse-0.4.fwsc`（Morse 0.4 release build，`FM-1_41000400`，SHA-256 `3bc1b23b…7e471f`，fm1-morse `323d79b`），機器上是 Morse 0.3 release | 成功：`checking the package (30 reads)` → `switching to update mode` → `the loader writes only the sectors that changed (fast install)` → `done: the FM-1 runs FM-1_41000400` |
| 2026-11-18 05:25 | agent 執行 `just morse-backup`（唯讀）；用 Playwright 的 Chromium 開 `web/progress.html` 下載進度（唯讀） | 指令列：`round 16, 10 open (KMURESNAPT), 4 past the line, 169 answered, 132 right`，**升級後學習紀錄保留**，存成 `backups/progress/2026-11-18-after-0.4.json`。網頁第一版找不到機器：macOS 把這台的 MIDI port 叫做 `Felucca`（第一次接上時的韌體名），網頁只找名字有 MORSE 的 port；改成聽哪個 port 送出 HELLO 後，網頁下載的 JSON 和指令列的完全相同 |
| 2026-11-18 16:11 | agent 執行 `just morse-backup`（唯讀，刷 Morse 0.5 前） | `round 29, 20 open (KMURESNAPTLWI.JZ=FOY), 15 past the line, 451 answered, 386 right, streak 5`，存成 `backups/progress/2026-11-18-pre-0.5.json`。FM-1 已經和另一個程式連著（只送 HEARTBEAT、不送 HELLO），host 0.5.0 原本因為不知道韌體版本而拒絕備份，改成照樣嘗試 |
| 2026-11-18 16:12 | agent 執行 `just fm1-download-mode`、`fm1-backup pre-morse-0.5`、`fm1-restart`（刷 Morse 0.5 前的 preflight） | flash `0xB36014`；備份 SHA-256 `d8f8889e…a0d44`；重開回 Morse 0.4。fm1-emu 兩種晶片上把 0.5 release build 疊在這份備份上預演：讀到 ROUND 30、開放 20 個字，0 次寫入 `[模擬]` |
| 2026-11-18 16:15 | 本人要求後，agent 執行 `just --yes fm1-backup-release 2026-11-18-pre-morse-0.5` | 公開 release [`backup-2026-11-18-pre-morse-0.5`](https://github.com/daviddwlee84/M-VAVE_FM-1/releases/tag/backup-2026-11-18-pre-morse-0.5)：整片 flash 備份、M-Flasher 的 JSON、學習進度 JSON；上傳後的 digest 和本機的 SHA-256 相同 |
| 2026-11-18 16:20 | 本人執行 `fm1_install.py firmware/morse/build/morse-0.5.fwsc`（Morse 0.5 release build，`FM-1_41000500`，SHA-256 `49433190…154457`，fm1-morse `a75c29e`），機器上是 Morse 0.4 | 第一次在 `[y/N]` 輸入時 Python 報 `UnicodeDecodeError`（終端機送了非 UTF-8 的位元組，大概是輸入法），還沒寫入就結束。第二次成功：`checking the package (30 reads)` → `switching to update mode` → `the loader writes only the sectors that changed (fast install)` → `done: the FM-1 runs FM-1_41000500` |
| 2026-11-18 16:22 | agent 執行 `fm1_install.py --info`、`just morse-backup`（唯讀） | `FM-1_41000500 [running] port: Felucca`；進度和刷機前相同（round 29、20 open、451 answered、386 right、streak 5），存成 `backups/progress/2026-11-18-after-0.5.json` |
| 2026-11-18 16:23–16:29 | 本人在 Mac 的藍牙 MIDI 視窗連上 `FM-1 3399`、拔掉 USB；agent 執行 `just morse-send`、`just morse-backup`（唯讀），用 python-rtmidi 錄 TYPE 頁播出的按鍵時間 | 藍牙 port `FM-1 3399 Bluetooth`；`morse-send paris` 播出；進度備份走藍牙 1.7 秒，內容和走 USB 時相同。插回 USB 後兩條同時錄：照 CoreMIDI 時間戳記算，藍牙的點、劃、停頓誤差 ≤ 1 ms（和 USB 一樣），照到達時間算最差 ±16 ms。拔線後 `PC` 大約 10 秒才消失（host 逾時） |
| 2026-11-18（16:29 之後） | 本人只用藍牙（沒接 USB）：Chrome 開 MORSE on the web，以及 vailmorse.com 的 Repeater | 網頁顯示 `MORSE 0.5 connected over Bluetooth`，Live 收到 FM-1 讀到的字；Vail 不用設定就收到 FM-1 的發報。拔線用電池時沒有斷線 |
| 2026-11-18 16:55 前後 | 本人執行 `fm1_install.py firmware/morse/build/morse-0.6.fwsc`（Morse 0.6 release build，`FM-1_41000600`，SHA-256 `d65b6df5…8fd7e`，fm1-morse `60f76cf`），機器上是 Morse 0.5 | 成功：fast install，`done: the FM-1 runs FM-1_41000600`；17:02 `--info` 回答 `FM-1_41000600 [running] port: Felucca` |
| 2026-11-18 17:00 前後 | 本人試網頁和 Vail | 網頁的 *Over Bluetooth, directly*（Web Bluetooth）配對 `FM-1 3399` 成功，Live 和 Timing 都能用；USB 和藍牙都在時網頁走 USB。Vail Decoder：20/13、20/14 字母還是分開，22/18 連成詞 |
| 2026-11-18 17:10 前後 | 本人測 0.6 的拔線 | 用 USB 連上、`PC` 亮起後拔線，`PC` 半秒內消失；接著線時幾乎沒看到 `PC` 閃 |
| 2026-11-18 18:55 前後 | agent 執行 `fm1_install.py --info`、`just morse-backup`（唯讀，刷 THEORY 0.1 前） | `FM-1_41000600 [running] port: Felucca`。進度 JSON 兩次都沒存到：host 在 `Felucca` 上等不到 HELLO（大概有別的程式連著 FM-1）。學習紀錄在下一列的整片備份裡（`0xDA000`） |
| 2026-11-18 18:56 | agent 執行 `just fm1-download-mode`、`fm1-info`、`fm1-backup pre-theory-0.1`、`fm1-restart`、`fm1_install.py --info`（刷 THEORY 0.1 前的 preflight） | bootloader stock、firmware `FM-1 410`、flash `0xB36014`；備份 SHA-256 `5b62a3da…879ea`；重開回 Morse 0.6（`FM-1_41000600 [running]`）。fm1-emu 兩種晶片上把 THEORY 0.1 dev build（SHA-256 `0d86cc42…41ebcd`）疊在這份備份上預演：開機 0 次寫入，存檔只寫 `0xD8000`，`0xDA000` 的 Morse 紀錄和 `0xFC000` 都沒變 `[模擬]` |
| 2026-11-18 19:00 前後 | 本人執行 `fm1_install.py firmware/theory/build/theory.fwsc`（THEORY 0.1 dev build，`FM-1_4900`，SHA-256 `0d86cc42…41ebcd`，從 `c7a7084` 上未 commit 的工作目錄 build），機器上是 Morse 0.6 | 成功：`checking the package (30 reads)` → `switching to update mode` → `the loader writes only the sectors that changed (fast install)` → `done: the FM-1 runs FM-1_4900`。本人試用：「好像還可以」。19:02 agent 想用 `--info` 再確認時，FM-1 已不在 USB 上（沒有任何 MIDI input），所以以安裝程式最後的回報為準 |
| 2026-11-18 19:37–19:40 | agent 執行 `fm1_install.py --info`、`just fm1-download-mode`、`fm1-info`、`fm1-backup pre-morse-0.7`、`fm1-restart`（刷 Morse 0.7 前的 preflight） | `FM-1_4900 [running] port: Felucca`（19:02 不在 USB 上，是線沒接好；本人重接後就在）；bootloader stock、firmware `FM-1 490`、flash `0xB36014`；備份 SHA-256 `4c3d061d…712e85`，和刷 THEORY 前的備份相比，`0x93000` 以上只有 `0xD8000`、`0xD9000` 不同（THEORY 的存檔），`0xDA000` 的 Morse 紀錄沒變；重開回 THEORY。fm1-emu 兩種晶片上把 `morse-0.7.fwsc`（release，SHA-256 `c15541d5…aa45e3d`）疊在這份備份上預演：開機進 MENU、ROUND 30 保留、開機和進 PRACTICE 都是 0 次寫入 `[模擬]` |
| 2026-11-18 19:45 前後 | 本人執行 `fm1_install.py firmware/morse/build/morse-0.7.fwsc`（release `FM-1_41000700`，SHA-256 `c15541d5…aa45e3d`，fm1-morse `52b048e`），機器上是 THEORY 0.1 | 成功：`checking the package (30 reads)` → `switching to update mode` → `the loader writes only the sectors that changed (fast install)` → `done: the FM-1 runs FM-1_41000700`。本人確認：開機進 MENU、進度還在，START PAGE 改了之後關機再開也照設定。觀察：在 SETTINGS 改完馬上關機不會存（設定在最後一次改動 3 秒後才寫入），`SAVE` 鍵沒有作用 |
| 2026-11-18 19:49–19:51 | agent 執行 `fm1_install.py --info`、`just fm1-download-mode`、`fm1-backup pre-morse-0.8`、`fm1-restart`（刷 Morse 0.8 前的 preflight） | `FM-1_41000700 [running] port: Felucca`；bootloader stock、firmware `FM-1 410`、flash `0xB36014`；備份 SHA-256 `d5470954…bfb31f`；重開回 Morse 0.7。fm1-emu 兩種晶片上把 `morse-0.8.fwsc`（release，SHA-256 `76b0c2a0…bd92d5`）疊在這份備份上預演：照 START PAGE 開在 PRACTICE、ROUND 31 保留、開機 0 次寫入 `[模擬]` |
| 2026-11-18 19:55 前後 | 本人執行 `fm1_install.py firmware/morse/build/morse-0.8.fwsc`（release `FM-1_41000800`，SHA-256 `76b0c2a0…bd92d5`，fm1-morse `8467a08`），機器上是 Morse 0.7 | 成功：`checking the package (30 reads)` → `switching to update mode` → `the loader writes only the sectors that changed (fast install)` → `done: the FM-1 runs FM-1_41000800`。本人試用：「目前使用起來效果不錯」（`SAVE` 立刻存、`*` 和 `SAVE` 燈） |
| 2026-11-18 21:19–21:21 | agent 執行 `fm1_install.py --info`、`just morse-backup`、`just fm1-download-mode`、`fm1-info`、`fm1-backup pre-theory-0.2`、`fm1-restart`（刷 THEORY 0.2 前的 preflight） | `FM-1_41000800 [running] port: Felucca`；Morse 進度存成 `backups/progress/2026-11-18-pre-theory-0.2.json`（round 30、20 open、473 answered、406 right、streak 6）；bootloader stock、firmware `FM-1 410`、flash `0xB36014`；備份 SHA-256 `d388c6c4…dc874`；重開回 Morse 0.8。fm1-emu 兩種晶片上把 THEORY 0.2 dev build（SHA-256 `22c7f156…28451`，commit `46b0b41`）疊在這份備份上預演：開機先問 START PAGE（0.1 的設定升級），0 次寫入；選頁後只寫 `0xD8000`，`0xDA000` 和 `0xFC000` 不變 `[模擬]` |
| 2026-11-18 21:30 前後 | 本人執行 `fm1_install.py firmware/theory/build/theory.fwsc`（THEORY 0.2 dev build，`FM-1_4900`，SHA-256 `22c7f156…28451`，commit `46b0b41`），機器上是 Morse 0.8 | 成功：`checking the package (30 reads)` → `switching to update mode` → `the loader writes only the sectors that changed (fast install)` → `done: the FM-1 runs FM-1_4900`。本人試用：「體驗還可以」。21:34 agent 想用 `--info` 再確認時 FM-1 已不在 USB 上，以安裝程式的回報為準 |
| 2026-11-18 22:30 | 刷 THEORY 0.3 前的 preflight。agent 執行：`just theory-backup`（唯讀）、`just fm1-download-mode`、`fm1-info`、`fm1-backup pre-theory-0.3`、`fm1-restart` | `theory-backup`：round 1、63 項開了 6 項、答 6 對 6。`info`：firmware `FM-1 490`、app OK、flash `0xB36014`（新）、M-Flasher 1.3。完整備份 SHA-256 `eab8cd54…762fa681`，`0xD6000`、`0xD8000`、`0xDA000` 都有資料。把 0.3（SHA-256 `12b633be…3e4a51c9`，commit `a0a14e9`）疊在這份備份上預演 `[模擬]`：開機 0 次寫入，進 TODAY，ROUND 1 的進度還在。重開回 THEORY 0.2 |
| 2026-11-18 22:35 前後 | 本人執行 `fm1_install.py firmware/theory/build/theory.fwsc`（THEORY 0.3 dev build，`FM-1_4900`，SHA-256 `12b633be…3e4a51c9`，commit `a0a14e9`），機器上是 THEORY 0.2 | 成功：`checking the package (30 reads)` → `switching to update mode` → `the loader writes only the sectors that changed (fast install)` → `done: the FM-1 runs FM-1_4900`。agent 接著用 `fm1_install.py --info` 確認 `FM-1_4900 [running] port: Felucca`，THEORY 的 HELLO 回報韌體 0.3 |
| 2026-11-18 23:00 前後 | 本人執行 `fm1_install.py firmware/theory/build/theory.fwsc`（THEORY 0.3 dev build，SHA-256 `2b7d5c3f…54dd5bd`，commit `92267f6`：CHART 示範任何調、低音較亮），機器上是 `a0a14e9` 那版 0.3 | 成功：fast install，`done: the FM-1 runs FM-1_4900`。agent 用 `--info` 確認 `FM-1_4900 [running]`，HELLO 回報 0.3 |

## Phase 1 驗收

- [x] 原廠韌體版本已記錄
- [x] M-Flasher 已安裝，首次授權完成
- [x] `FLASH CHIP` / JEDEC ID 已記錄
- [x] 完整 flash 備份已存，SHA-256 已記錄
- [x] 備份另存一份到這台電腦以外的地方（GitHub Release）
- [x] SLOOP 2.8.5 已刷入
- [x] Save → 關機 → 開機 → Load 測試通過
