# 我的機器

這台 FM-1 目前的狀態與操作歷史。任何對實機的操作，做之前先讀這頁，做完更新這頁。這頁的內容都是 `[實機]`。

## 現況

| 項目 | 值 |
|---|---|
| 型號 | M-VAVE FM-1 |
| Flash 晶片 / JEDEC ID | **`0xB36014` (new)**，UCUN UC25HQ80，1 MiB |
| Bootloader | stock (M-VAVE) |
| 出廠韌體版本 | FM-1 015 (stock)，firmware check OK |
| 目前韌體 | **Morse 0.3（自製）**（2026-11-18 02:12 刷入，套件識別 `FM-1_1300`，USB 產品名 `MORSE FM-1`；macOS 的 MIDI port 仍顯示 `Felucca`）。之前依序是原廠 015 → SLOOP 2.8.5 → Agent Deck 0.7 → SLOOP 2.8.5 → CTL-1 1.0.3 → ChoralRoot 0.41 → Conductor 0.1 → 0.2 → 0.3 → Morse 0.1 → 0.2 |
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

## Phase 1 驗收

- [x] 原廠韌體版本已記錄
- [x] M-Flasher 已安裝，首次授權完成
- [x] `FLASH CHIP` / JEDEC ID 已記錄
- [x] 完整 flash 備份已存，SHA-256 已記錄
- [x] 備份另存一份到這台電腦以外的地方（GitHub Release）
- [x] SLOOP 2.8.5 已刷入
- [x] Save → 關機 → 開機 → Load 測試通過
