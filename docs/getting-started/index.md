# Getting Started 總覽

**先備份原廠 → 確認 flash 晶片 → 刷 SLOOP 2.8.5 → 開始玩。** 第一天只需要做這些，不必先裝 m-bootloader，也不必追求多韌體開機選單。

## 順序

1. **先玩一下原廠韌體。** 確認琴鍵、旋鈕、螢幕、耳機輸出、USB MIDI 都正常，記下開機顯示的韌體版本。先不要更新原廠版本。
2. **[安裝 M-Flasher](install-m-flasher.md)。** 電腦端的備份、刷機、還原工具。
3. **[完整備份並確認晶片](backup-and-chip.md)。** 整顆 1 MiB flash 存下來，記錄晶片 ID 與 SHA-256，另存一份到別處。
4. **[刷 SLOOP 2.8.5](flash-sloop.md)。** 它明確支援新舊兩種晶片。
5. **刷後測試。** 存一個東西、關機、開機、確認讀得回來。

## 為什麼是這個順序

**備份比裝 bootloader 重要。** 原廠映像連同這台機器自己的資料（校正、Bluetooth 位址、原廠音色）未必能從網路完整找回。M-Flasher 在每個 session 第一次寫入前也會自動存一份完整備份，但主動先做一份、確認檔案在、hash 記下來，才算有退路。

**晶片要先確認。** 2026 年後期出貨的 FM-1 換了 flash 晶片，舊韌體可能不認識它，結果是設定只存在 RAM、關機就消失。銷售時沒有標示，只能拿到機器後用 M-Flasher 讀。見[兩種 Flash 晶片](../hardware/flash-chips.md)。

**m-bootloader 不是第一步。** 它讓「韌體刷壞」之後更容易救，但它自己也寫在高風險的 boot 區，而且在新晶片上的實機測試還很少。等熟悉刷機與備份、確認晶片相容之後再考慮。

**SLOOP 2.8.5 而不是 SLOOP 3。** SLOOP 3 是 public beta、幾乎從零重寫，而且必須先裝 m-bootloader。比較見[SLOOP 3](../firmware/sloop/sloop3.md)。

## 需要準備的東西

- USB-C **資料線**（不是只能充電的線），直接接電腦，不要經過 hub。
- macOS 11 以上、Windows 10/11 或 Linux。這份筆記以 macOS 為主。
- 走瀏覽器安裝或 Web Editor 的話需要 Chrome 或 Edge（要 Web MIDI + SysEx）。
- 一個放備份的地方，以及另一個異地存放的地方。

## 這個 repo 的捷徑

Repo 根目錄的 `Justfile` 把上面的步驟包成指令（macOS，需要 [just](https://just.systems/) 與 `gh`）：

| 指令 | 作用 | 等級 |
|---|---|---|
| `just fm1-fetch` | 下載 M-Flasher 與 SLOOP 到 `downloads/`，並與 release digest 比對 | 唯讀 |
| `just fm1-install-flasher` | 下載後開啟 dmg（拖進 Applications 要自己做） | 唯讀 |
| `just fm1-gui` | 開 M-Flasher 視窗 | — |
| `just fm1-find` / `just fm1-info` | 找機器 / 讀機器資訊 | 唯讀 |
| `just fm1-backup <label>` | 完整備份到 `backups/<日期>-<label>/` 並印出 SHA-256 | 唯讀 |
| `just fm1-verify-backups` | 用 `.json` 裡的 SHA-256 檢查所有備份 | 唯讀 |
| `just fm1-download-mode` / `just fm1-restart` | 進 / 離開 download mode | 模式切換 |
| `just fm1-flash <package.fwsc>` | 安裝韌體。會先問一次，M-Flasher 再要求輸入 `yes` | 寫入，本人執行 |
| `just sloop-editor` | 用 Chrome 開離線版 SLOOP Web Editor | — |

刻意沒有 bootloader、restore、`run-ram` 的捷徑。等級的定義見[工作流與權限](../agentic/index.md)。

## 驗收清單

- [ ] 原廠韌體版本已記錄
- [ ] M-Flasher 已安裝，首次授權完成
- [ ] `FLASH CHIP` / JEDEC ID 已記錄
- [ ] 完整 flash 備份已存，SHA-256 已記錄，另有一份副本
- [ ] SLOOP 2.8.5 已刷入，開機看到 SLOOP logo
- [ ] Save → 關機 → 開機 → Load 測試通過
- [ ] [我的機器](../device-log.md)已更新
