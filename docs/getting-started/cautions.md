# 注意事項

刷機前讀一遍。每一條背後都有人踩過。

## 一定要做

- **先備份再做任何寫入。** 完整 flash 備份、SHA-256、兩份副本。見[備份](backup-and-chip.md)。
- **先確認 flash 晶片，再選韌體。** 新晶片（`0xB36014`）上，舊韌體可能只把設定留在 RAM。裝之前確認該韌體有寫明支援新晶片。見[兩種 Flash 晶片](../hardware/flash-chips.md)。
- **用資料線，直接接電腦，不經 hub。**
- **從上游 release 下載，核對 hash。** [The Firmware Hub](https://thefirmwarehub.com/) 的一鍵安裝很方便，但它自己聲明只檢查套件格式，不逐一測試硬體相容性或品質。
- **刷完做存檔測試：** Save → 關機 → 開機 → Load。

## 不要做

!!! danger "不要把原廠 016 降回 015"
    016 一執行就會覆寫 preset 區。降回 015 之後會出現亂碼的 preset 名稱與錯誤的音高，不管裝的是哪個 bootloader。官方更新不會還原那一區，只有含原始區域的備份可以。不要為了配合某篇舊教學而降版。

!!! danger "不要把 m-bootloader 當成無風險的第一步"
    它裝在 boot 區，本身就是高風險寫入。在新款 UC25HQ80 晶片上測試很少：上游記錄有一台裝了 1.1 / 1.2 後無法啟動、原因不明；另一台回報 1.3 與 2.0 正常。M-Flasher 讀完 flash 後會主動推薦安裝它，選 **No** 也完全沒問題，之後隨時可以再裝。

- **不要用 `--yes`。** M-Flasher 寫入前會列出變更並要求輸入 `yes`，那是最後一道確認。
- **不要把完整備份當成跨機器的韌體安裝包。** 還原別台的備份時 M-Flasher 會保留這台自己的原廠資料，但那不是一般用法。
- **不要假設還原備份不會動到 bootloader。** 完整還原會連 boot 區一起寫回。若之後裝了 m-bootloader 又想還原舊備份，要用 **Keep this unit's bootloader**（CLI：`--keep-bootloader`）。
- **不要把完整備份 commit 進 public repo。**

## 會把 bootloader 換回原廠的操作

**官方 M-VAVE 更新**與 Baud Girl 的 installer 會把原廠 bootloader 連同完整設定寫回去。裝了 m-bootloader 之後做官方更新，要再重裝一次 m-bootloader。

M-Flasher 自 0.12.0 起安裝韌體時會保留現有的 boot 區。`[調研]`（上游文件的說法是「the unit's boot area is kept」，但我沒有逐版核對是哪一版開始。）

## 切換韌體時

- 不同韌體把資料存在不同位置。M-Flasher 在資料配置不同的韌體之間切換時會警告，殘留資料可能讓新韌體混亂。
- SLOOP 往回降版時，新版存的 project 舊版可能讀不了，而且舊版的 autosave 可能蓋掉進行中的音樂。降版前先用 Web Editor 備份。
- SLOOP 3 可以讀 2.8 的部分 project 與設定，但不要假設格式能雙向相容。`[調研]`

## macOS 相關

- M-Flasher 首次授權只能在本機 GUI 做，不能透過 SSH。
- 進 download mode 後盡快開 M-Flasher，放著約半分鐘機器可能自己離開。
- macOS 15 以前，SLOOP 2.7.1 以前的版本看得到 MIDI 但沒有 USB audio 輸入；2.7.2 起修正。
