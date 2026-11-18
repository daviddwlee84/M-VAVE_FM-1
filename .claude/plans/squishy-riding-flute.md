# MORSE 0.5：藍牙 MIDI（peripheral）

## Context 背景

0.4 已經刷進實機，網頁 Live 也能用了。你要開始做 0.5 藍牙，研究在 `docs/dev/morse.md#bluetooth`。

**目標**：不接線，讓 Mac（之後也包括 iPad 和手機的 app）透過藍牙使用這些功能：
- Vail；
- MORSE on the web：Live、備份、還原；
- `just morse-send` / `morse-type` / `morse-backup`。

**範圍**：
- 只做 **peripheral**，也就是 FM-1 被電腦連。
- 兩台 FM-1 互打（`ble_central`）要等有第二台，留在 backlog。
- 用 velocity 帶 mark 長度的備案，等實機量過抖動再決定要不要做。

## 已確認的事實（讀過程式）

- **Conductor 的做法可以照抄**（`firmware/conductor/app.c:242-266`、`app.h:16-19`）：
  - `APP_BLE_STORE` 加上空的 `app_ble_load` / `app_ble_save`；
  - `ble_in()` 用 `hl_sx_packet` 把封包組回 frame，再交給 `host_frame(…, HL_BLE)`；
  - `plat_ble_enable(on)` 開關藍牙，`plat_ble_connected()` 給 header 用。
- **MORSE 跟 Conductor 不一樣的地方**：MORSE 在 **1 kHz 的 `app_tick`** 裡讀 `plat_midi_in`（`firmware/morse/app.c:474`），把 note 當成收訊按鍵。藍牙送來的 SysEx 封包也會出現在同一個佇列裡，但 `host_frame` 會呼叫 `hl_send` 和 `prog_in`，不應該在 tick 裡跑。
- **收訊時間**（`third_party/fm1-base/include/plat.h:107-110`）：
  - 沒開 `ble_midi_time` 時，note 的時間是「讀到的時候」，同一個 connection event 裡的訊息會擠在一起。對 60 ms 的點來說這樣不行。
  - 開了以後，`plat_midi_in_t24()` 給的是送出端的時間（`plat_ticks24`，24 tick 等於 1 µs）。
- **送出**：`plat_midi_send` 可以在 tick 裡呼叫，own stack 會帶真實的時間戳記。
- **host simulator 已經有藍牙替身**：`ble connect|disconnect`、`midi BLE …`、`expect_out BLE …`（`third_party/fm1-base/host/plat_host.c:20-33`）。
- **`set_t.look` 的 8 個 bit 都用掉了。** `mute` 只用到 bit 0。

## 1. 韌體（fm1-morse submodule，FW_MINOR 5）

**`fm1.toml`**：加 `ble = true`、`ble_stack = "own"`、`ble_diag = false`、`ble_midi_time = true`。

**`app.h`**：加 `#define APP_BLE_STORE`，註解照 Conductor（只放 RAM，不寫 flash）。

**`app.c`**：

1. **空的 `app_ble_load` / `app_ble_save`**，照 Conductor。
2. **`app_tick` 的 MIDI 迴圈分流**：
   - **note**：照舊設定 `rx_midi`。來自 `PORT_BLE` 的 note，用 `plat_midi_in_t24()` 換算回 `plat_ms` 的時間（`now - (plat_ticks24() - t24) / 24000`），再交給 `ev_push(SRC_RX, …, t)`。
   - **SysEx 封包**（CIN 4–7，來自 `PORT_BLE`）：放進一個 SPSC ring（大約 256 個 `uint32_t`，tick 寫、主迴圈讀，寫法照 `ev_push`）。
   - 收訊用的 `rx_down` 判斷保持原樣。
3. **`app_frame` 新增 `ble_in()`**：把 ring 裡的封包交給 `hl_sx_packet(&ble_rx, …)`，組好的 frame 交給 `host_frame(f, n, HL_BLE)`。之後 HELLO、Live、`T_PROG*` 備份還原都直接走藍牙，因為 `hl_send` 會回到 ack 來的那條路。
4. **送出按鍵**：`midi_key()` 改成 `plat_midi_send(…, TO_USB | (ble_on() ? TO_BLE_HOST : 0))`。
5. **設定**：SETTINGS 加第 13 項 **BLUETOOTH ON/OFF**，`SET_ITEMS` 改成 13。
   - 存在 `SET.mute` 的 bit 1，1 代表關，預設 0（開）。所以**存檔格式不變**，0.4 的存檔可以直接讀。
   - `set_valid` 改成 `mute <= 3`。原本用到 `mute` 的地方改成 `& 1`。
   - 開關時呼叫 `plat_ble_enable`。開機時照設定呼叫一次。
6. **header**：照 Conductor 顯示 `BT`。正在廣播時是灰色，連上後是藍色，跟 USB 的 host 標示並排。
7. **`#if FM1B_BLE` 包起來**，`--no-ble` 也能 build。

**`tests/basic.txt`**（host simulator）新增：
- `ble connect` 後，HELLO 從 BLE 送出，傳輸 byte 是 `01`；
- 從 BLE 送 HELLO_ACK 和 `T_TEXT`，裝置會播出來；
- `T_PROG_REQ` 走 BLE，回傳的 `T_PROG` frame 也走 BLE；
- KEY 頁按鍵時，`expect_out BLE 90 00 …`；
- BLUETOOTH OFF 之後，按鍵不送到 BLE；這個設定存檔、重開機後還在；
- 藍牙相關的動作不增加 store 寫入。

**版本和協定**：`FW_MINOR` 改成 5，HELLO 向量改成 `05`。協定本身不變，HELLO 本來就帶傳輸 byte。

## 2. host（`host/morse/`，0.5.0）

- `__init__.py` 改成 0.5.0 / (0,5)，`protocol.py` 的 HELLO 向量和測試跟著改。
- `device.py`：找 port 時，USB 和藍牙都有就用 USB，跟 Conductor 的 `sniff_port` 一樣。找不到 port 時，提示到藍牙 MIDI 視窗按 Connect，並重用 `host/conductor/blelink.py` 打開那個視窗（同一個 repo，MIT）。
- CLI 印出目前走的是 USB 還是 Bluetooth（從 HELLO 的傳輸 byte 判斷）。

## 3. 網頁（fm1-morse `web/index.html`）

- 連線狀態顯示「via Bluetooth / USB」，從 HELLO 最後一個 byte 判斷。
- 說明一行：Mac 要先在「音訊 MIDI 設定 → 藍牙」連上。
- 備份走藍牙會比較慢，6 個 frame 應該還是只要幾秒。逾時改成依傳輸決定。

## 4. 文件

- `docs/dev/morse.md`：
  - 藍牙一節從「研究」改成「0.5」，寫實作和怎麼連；
  - 版本列加 0.5，大小和 RAM 用 build 出來的數字；
  - roadmap 和 backlog 留下 central 和 velocity 備案；
  - 驗證表。
- `morse-cheatsheet.md`：加 BLUETOOTH 設定和 header 的 `BT`。
- `morse-learn.md`：「接電腦」段落加藍牙。
- fm1-morse README：加 Bluetooth 小節，截圖加 `settings_bt`。
- AGENTS.md 不用改，沒有新的 Justfile 指令。

## Verification 驗證

1. `just morse-unit`、`just fw-test morse`（加上上面的 BLE 測試）、`just morse-test`。
2. `just fw-build morse` 會 build 出含藍牙的版本，記錄大小和 RAM（預估 +41–46 KB）。**注意**：submodule pointer 要先 commit，再 build，避開已知的 fw-setup 陷阱。
3. fm1-emu 在 `uc25hq80` 和 `p25q80` 上都跑一次：開機、改設定、存檔、重開、讀回，USB 照常，藍牙不增加寫入 `[模擬]`。模擬器沒有無線電，藍牙本身要到實機才能測。
4. 實機（**每次刷機都要你核准**，preflight 照 AGENTS.md，由你輸入 `yes`）：
   - 確認進度還在。
   - Mac 用藍牙連上 `FM-1 XXXX`，`just morse-send`、MORSE on the web（Live 和備份）、Vail 都能用。
   - **量抖動**：用 KEY 頁打同一串字，比較 USB 和藍牙兩邊 Web MIDI `timeStamp` 的間隔。我會寫一個小量測腳本或網頁 debug 區，記錄每個 mark 的長度。
   - 拔線只用電池，連打 10 分鐘。
   - 結果寫進 `device-log.md`，以及 morse.md 的 `[實機]` 標記。
5. `just docs-check`。commit、push、release 都等你說。
