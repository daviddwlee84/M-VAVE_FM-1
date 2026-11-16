# 備份原廠與確認晶片

這一步只有讀取，不會改寫 flash。做完之前不要刷任何東西。

## 進入 download mode { #download-mode }

晶片的 ROM 內建 USB download mode，韌體壞了也能進。進入方式依機器目前的狀態而定：

| 機器狀態 | 怎麼進 |
|---|---|
| 原廠韌體或 Felucca 正常運作 | 接上 USB，M-Flasher 會在 USB MIDI 上找到它，按 **DOWNLOAD MODE**（CLI：`m-flasher download-mode`）。它送一個 MIDI 訊息讓機器重開進 download mode |
| Felucca、SLOOP 或 Snoop37 執行中 | 同時按住 **`OCT-`** 和 **`OCT+`** 5 秒 |
| 已裝 m-bootloader | 按住 **`REC`** 開機 |
| 韌體已損壞 | 常常會自己掉進 download mode |

原廠 bootloader 加上正常的原廠韌體時，**沒有按鍵組合可以進 download mode**，只能用 M-Flasher 的 **DOWNLOAD MODE**。

!!! note "原廠版本與 DOWNLOAD MODE"
    上游文件寫這個 MIDI 切換對 FM-1 原廠 **015** 與 Felucca 有效。原廠 **016** 是否也能用，上游沒有明寫；如果按了沒反應，先不要亂試，記下現象再查。`[上游]`

進入 download mode 後：

- 機器螢幕停在原本的畫面，Finder 裡不會出現任何東西。系統看到的是一顆空磁碟（`WL82 UBOOT1.00`）。
- M-Flasher 會自己找到它、讀完整顆 flash，顯示型號、韌體與 bootloader。

## 讀晶片資訊

GUI 看 **Device info**，CLI：

```sh
m-flasher find     # download mode 中的機器，以及在 USB MIDI 上執行中的機器
m-flasher info     # 機器上有什麼
```

要記下來的：

- **FLASH CHIP**：`0x856014 (old)` 是 Puya P25Q80H，`0xB36014 (new)` 是 UCUN UC25HQ80。差異見[兩種 Flash 晶片](../hardware/flash-chips.md)。
- 型號、目前韌體版本、bootloader 是原廠還是 m-bootloader。

## 完整備份

GUI 選 **Back up the whole flash**，CLI：

```sh
m-flasher backup [DIR]
```

- 存下 1 MiB 的 flash 映像，加上一個描述它的 `.json`（裡面有 SHA-256，還原時會用來驗證）。
- 不指定目錄時存到家目錄的 `Documents/M-Flasher/backups`。
- 整顆讀取大約 1.2 秒。慢很多的話換一個 USB port。

備份後自己再算一次 hash 並記錄：

```sh
shasum -a 256 <備份檔>.bin
```

!!! danger "備份的處理"
    - **至少兩份**，一份在電腦、一份在別的儲存位置。
    - **不要 commit 進 git。** repo 裡的 `backups/` 已被 gitignore。
    - 完整映像包含這台機器自己的資料（Bluetooth 位址、配對、unit id）。要不要公開是自己的決定：我選擇用 `just fm1-backup-release` 放到這個 public repo 的 Release 當異地副本；不想公開就放 private repo 或其他儲存空間。
    - 這份備份是回到原廠狀態（含原廠音色）的唯一可靠方法。原廠 016 執行後會覆寫 preset 區，官方更新不會還原它。

## 記錄

把晶片 ID、原廠版本、備份檔名、SHA-256、日期填進[我的機器](../device-log.md)。

## 離開 download mode

拔線後關機再開機，或：

```sh
m-flasher restart
```

它從 download mode 重置機器、不碰 flash，並等待韌體重新出現在 USB MIDI 上。

## 下一步

[刷 SLOOP 2.8.5](flash-sloop.md)。
