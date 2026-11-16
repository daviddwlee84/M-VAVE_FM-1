# 我的機器

這台 FM-1 目前的狀態與操作歷史。任何對實機的操作，做之前先讀這頁，做完更新這頁。這頁的內容都是 `[實機]`。

## 現況

| 項目 | 值 |
|---|---|
| 型號 | M-VAVE FM-1 |
| Flash 晶片 / JEDEC ID | **`0xB36014` (new)**，UCUN UC25HQ80，1 MiB |
| Bootloader | stock (M-VAVE) |
| 出廠韌體版本 | FM-1 015 (stock)，firmware check OK |
| 目前韌體 | **SLOOP 2.8.5**（2026-11-17 刷入；M-Flasher 顯示套件識別為 `FM-1_900`） |
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

## 下載的檔案

放在本機的 `downloads/`（gitignore）。SHA-256 已與 GitHub release asset 的 digest 比對一致。

| 日期 | 檔案 | SHA-256 | 來源 |
|---|---|---|---|
| 2026-11-17 | `M-Flasher-0.12.1-macos.dmg` | `ccb469a1ceead508e50a42b6c02946ea0d0caee18a9d06ec830c268ee8717192` | [m-flasher v0.12.1](https://github.com/jvitkauskas/m-flasher/releases/tag/v0.12.1) |
| 2026-11-17 | `sloop-2.8.5.fwsc` | `b0446effb840740a3aa5c386ce004e79334e4f833d48d82a9eb5edbfde92ae67` | [sloop-fm1 v2.8.5](https://github.com/isod89/sloop-fm1/releases/tag/v2.8.5) |
| 2026-11-17 | `sloop-editor-2.8.5.html` | `25fe6043084b6d6bdd0529699e2b8cf6f4ba186f7d04801c2564882dabb76f03` | 同上 |

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

## Phase 1 驗收

- [x] 原廠韌體版本已記錄
- [x] M-Flasher 已安裝，首次授權完成
- [x] `FLASH CHIP` / JEDEC ID 已記錄
- [x] 完整 flash 備份已存，SHA-256 已記錄
- [x] 備份另存一份到這台電腦以外的地方（GitHub Release）
- [x] SLOOP 2.8.5 已刷入
- [x] Save → 關機 → 開機 → Load 測試通過
