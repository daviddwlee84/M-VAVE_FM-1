# SLOOP 3

[SLOOP 3](https://github.com/isod89/sloop3-fm1) 不是小改版。玩起來像 SLOOP 2.8，但幾乎每一行都是新寫的：音訊引擎、sequencer、介面。目前是 **public beta**，還沒刷，這頁是之後的準備。

兩者是分開的 repo、installer 與 release。SLOOP 3 不會寫入 SLOOP 2.8 的資料。

## 差異

| | SLOOP 2.8 | SLOOP 3 |
|---|---|---|
| 狀態 | stable | public beta |
| Tracks | 3 synth + 1 drum | 4 synth + 1 drum |
| 2.8 的作品 | — | 第一次啟動時會開啟 2.8 的 projects、user presets |
| 安裝 | 瀏覽器 installer 或 M-Flasher | **只能透過 m-bootloader 的 MIDI MODE** |

以下數字來自調研，還沒對過上游：`[調研]`

| | SLOOP 2.8 | SLOOP 3 |
|---|---|---|
| Engines | 12 | 13 |
| 最大 pattern 長度 | 64 steps | 128 steps |
| 音訊運算 | fixed-point | 32-bit float |
| Audio block | 256 samples | 64 samples |
| Polyphony | 8 個共用 synth voices | 32 個共用 voices |
| Sample slicing | sample / GRAIN | 獨立的 SLICE engine |

## 安裝前置條件

SLOOP 3 的 installer 是一個導引精靈，步驟大致是：

1. 確認需求：FM-1、USB 資料線、Chrome 或 Edge。
2. **裝 m-bootloader（一次）。** 檢查方式：按住 `PLAY` 開機看有沒有 recovery menu。沒有的話用 M-Flasher 進 download mode、**完整備份**、再安裝 m-bootloader。
3. 連線：按住 `PLAY` 開機，選 MIDI MODE，接上 USB。
4. 安裝，約一分鐘。

**回到 SLOOP 2.8：** 用 M-Flasher 還原裝 SLOOP 3 之前的備份。

## 我為什麼還沒刷

- 它必須先裝 m-bootloader，而 m-bootloader 在新款 UC25HQ80 晶片上的實機驗證很少（見[注意事項](../../getting-started/cautions.md)）。SLOOP 3 的精靈推薦它，不代表可以跳過相容性確認。
- beta 版更新很快，幾天內就有多個修正版。
- fm1-emu 目前跑不了它（清單裡沒有，套件是 `.area` 不是 `.fwsc`），所以連模擬器試玩都還不行。`[模擬]` 2026-11-17

## 解除暫緩的條件

我這台是新晶片 `0xB36014`，2026-11-17 決定只追蹤、不裝。兩個條件**都**滿足才重新考慮：

1. 上游 m-bootloader README 不再把新晶片標為 little tested，或有多個成功回報。
2. 手上有一個驗證過能讓這台 FM-1 進 download mode 的 USB_KEY（Raspberry Pi Pico；Raspberry Pi 3 不行）。

為什麼備份不夠：boot 區壞掉時機器不會開機，備份只有在還能進 download mode 時才用得上，而那時只剩 USB_KEY 這條路。追蹤紀錄在[上游追蹤](../../watch.md)。

## 刷之前要先完成

- [ ] 確認我這台的晶片（見[我的機器](../../device-log.md)）
- [ ] 查 m-bootloader 最新版在該晶片上的回報
- [ ] SLOOP 2.8 的 editor backup 與 M-Flasher 完整備份都做好
- [ ] 讀完 [m-bootloader README](https://github.com/jvitkauskas/m-bootloader) 的 Status 一節
