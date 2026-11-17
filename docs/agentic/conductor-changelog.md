# Conductor 版本紀錄

[Conductor](conductor.md) 是韌體加 host daemon 的一套組合。每次更新記在這裡，最新的在上面。

## 版本號怎麼看

| 零件 | 版本 | 在哪裡 |
|---|---|---|
| 韌體 | **X.Y**（Conductor 的版本就是這個） | `firmware/conductor/app.c` 的 `FW_MAJOR` / `FW_MINOR`；裝置的 HELLO 會回報，daemon 啟動時印出 `firmware X.Y` |
| host daemon | **X.Y.Z**：X.Y 是它搭配的最新韌體，Z 是只改 host 的更新 | `host/conductor/__init__.py` 的 `__version__`；啟動時印出 `[conductor] host X.Y.Z` |
| 協定 | 一個數字，目前 `1` | frame 的第 5 個 byte（`protocol.py` 的 `VERSION`、`app.c` 的 `CN_VER`） |

規則：

- **只改 host**：Z 加一，**不用刷**，重開 `just cond-run` 就好。
- **有改韌體**：Y 加一，host 也跟著變成 X.Y.0，**要刷**，照[實刷準備](conductor.md#flash)的檢查走。
- **協定**只在不相容的改動才加一（frame 格式變了、舊的 host 會讀錯）。新增 frame 類型、在 frame 尾端加可選欄位都不算：host 依韌體版本決定要不要用，韌體看到不認識的 frame 就忽略。
- 新的 host 要能接舊的韌體。韌體比 host 新時，daemon 會提醒更新 host。

## 紀錄

| 版本 | 日期 | 要刷？ | 改了什麼 | commit |
|---|---|---|---|---|
| host 0.3.2 | 2026-11-17 | 否 | 版本號與這一頁；`ARP` 語音鍵只在 macOS 啟用；韌體比 host 新時提醒 | （這次） |
| host 0.3.1 | 2026-11-17 | 否 | FM-1 重開後找不到 port：自動彈出 macOS 的藍牙 MIDI 視窗，按 Connect 後自動接上。純 CoreBluetooth 的做法試過不行 | `bf65a55` |
| **0.3** | 2026-11-17 | **是** | 藍牙 MIDI（fm1-base own stack，不寫 flash）；`OCT+` 開關藍牙；HELLO / HEARTBEAT 帶傳輸方式；host 優先 USB、藍牙上 DETAIL 送文字 | `b7e0564` |
| host 0.2.2 | 2026-11-17 | 否 | 收到 HELLO 前不送東西、不執行按鍵（重開 daemon 後才拿得到韌體版本） | `13c5352` |
| host 0.2.1 | 2026-11-17 | 否 | DETAIL 去掉 Codex 的點字 logo、輸入框與狀態列；缺字顯示 `?` | `d7ebaad` |
| **0.2** | 2026-11-17 | **是** | DETAIL 點陣圖（中文看得到）、去掉 Claude Code 的介面、KNOB 1 往回捲、`SEQ` 說明頁、修正 DETAIL 第 7–8 行被截掉 | `5add525` |
| host 0.1.1 | 2026-11-17 | 否 | 聽協定找 port（macOS 沿用 `Felucca` 這個舊名稱）；每 30 秒重送整張表 | `13fc88a` |
| **0.1** | 2026-11-17 | **是** | 第一版：琴鍵 = agent、LED 急迫度、彩色螢幕、提示音、`PLAY` 總覽燈、`ARP` 語音鍵、hc / classic Herdr / fake backend | `d45ce67` |

## 協定的演進（都還是 `1`）

| 韌體 | 加了什麼 | 舊的一方怎麼辦 |
|---|---|---|
| 0.2 | host → 裝置 `08` DETAIL_BMP；SELECT 尾端的 `scroll` | host 對 0.1 送文字；0.1 收到 `08` 會忽略 |
| 0.3 | HELLO 尾端的 `transport`、HEARTBEAT 尾端的 `transport` | 沒有這個 byte 就當 USB |
