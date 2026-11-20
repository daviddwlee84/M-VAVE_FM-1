# METRONOME v0.4 計畫：即時連線（USB + 藍牙）、網頁舞台頁與指揮動畫

## Context 背景

METRONOME 0.3 在 2026-11-19 23:00 UTC 自動上線 The Firmware Hub（catalog id `metronome`，八項檢查全綠，沒等 Sven 按）。本人想做 v0.4，選的範圍是：

- 即時連線；
- USB 和藍牙一起做；
- 網頁要有跟著拍子動的畫面、網頁出聲時機器靜音、從網頁遙控；
- 依照 GRID 的拍號畫一個指揮家在指揮的動畫。

之前定的設計原則照舊：不每拍送 MIDI；機器只在狀態改變時送 STATE（`46`），網頁用自己的 audio clock 推算每一拍，剩下的固定延遲用校正處理。藍牙的部分沿用 Morse 0.5 以後的作法：韌體端是 `ble_stack = "own"` 加 hostlink，網頁端是 Web Bluetooth。

## Step 0 上架收尾（先做，與 v0.4 分開 commit）

- **fm1-metronome**（submodule，main 上直接 commit 並 push，Hub 三小時內自動跟上）：
  - `web/try/index.html` 改成 `https://joakimeriksson.github.io/fm1-emu/?fw=metronome`，照 `firmware/morse/web/try/index.html` 的寫法；
  - 刪掉 `web/try/metronome.fwsc`，`Justfile` 的 `metro-release` 不再複製到 web/try；
  - `fm1.toml` 的 credits 改成指向 M-VAVE_FM-1 的 `firmware/rhythm`；
  - `fmhub.json` 的 `play` 不改，因為它指向的 try 頁本身已經改用 Hub 的 package。
- **superproject**：
  - 把已經改好的 `.agents/skills/fm1-hub-listing/SKILL.md`（sounds 勾選框）和 `docs/dev/metronome.md` 一起 commit。
  - skill 補一條：2026-11-19 METRONOME 雖然表單寫「Sven 親自看」，結果還是自動上線；catalog id 取自 repo 名；catalog 的 `chip` 寫 `new`，檢查頁寫 both chips。
  - `docs/dev/metronome.md` 的「發佈與上架」補上「上線」那一行，「之後」刪掉 Hub 那條。
  - bump submodule 指標，走 branch 加 PR。

## v0.4 變更

版本：`FW_MINOR 4`、host 0.4.0、release 0.4（identity `FM-1_59000400`）。`MT_PROTO` 維持 1，新功能用 HELLO 帶的 fw minor ≥ 4 判斷，舊網頁連 0.4 不受影響。record 格式不變（`REC_VER 1`），實機上的歌單沿用。

### 1. 協定（`app.c` 的協定註解、`host/fm1_metronome/protocol.py`、`web/setlist.js` 的 `T`）

- **STATE `0x46`**（機器 → 網頁），只在有電腦連線時送（`hl_host()`），而且只在 `app_frame` 送，不在 render 送。payload 先 pack7，大約 46 B，小於 `HOSTLINK_MAX` 64。內容：
  - 序號、flags：running、muted、有沒有 pending、switch NOW 或 NEXT BAR；
  - active 和 pending 的歌號；
  - **正在播放的那首歌的 28 B**，用 `sl_fill` 同一種排法（抽成 `song_put(d, s)` 共用）；
  - anchor：一個 tick 和它在機器上的 sample 時間（u32，44.1 kHz）；
  - 送出當下的 `now_smp()`。
- **送出的時機**：
  - 開始：anchor 是 `(0, tr.start)`；
  - 停止；
  - `tr_follow` 排定切換時（anchor 用 `pend_tick`/`pend_at`，網頁可以預先知道下一小節的歌）；
  - `tr.switches` 變化，也就是切換真的發生時；
  - BPM 或歌的設定被 `edited()` 改到時；
  - 播放中每 1 秒送一次，讓網頁追時鐘漂移（晶振差 100 ppm，一分鐘約 6 ms）。
  
  用一個 `st_dirty` 旗標合併同一 frame 裡的多次改變。
- **CTRL `0x06`**（網頁 → 機器）：op 加參數。
  - op：START、STOP、PICK n（照機器上的 NEXT BAR／NOW 設定）、BPM n、MUTE 0/1、STATE?（要一份 STATE）。
  - 每個 op 都走按鍵已經在用的函式（PLAY 的處理、`song_pick`、`bpm_by` 的同一條路徑、`tr_follow(FOLLOW_EDIT)`），所以畫面、LED、存檔的行為和實體按鍵一樣。
  - 機器在 `ui.ask` 或 CLEAR ALL 確認中時，只接受 STOP 和 STATE?。
  - 空的歌位不接受 PICK。

### 2. 機器端靜音（`app.c`）

- `ui.muted` 只放在 RAM，不存檔。只有網頁能開，**連線斷掉（hostlink 逾時或藍牙斷線）時自動取消**，避免舞台上電腦斷線後機器沒有聲音。
- 實作：`sound()` 照常更新 `tr.nv`，所以測試的計數不變；只是不呼叫 `voice()`。另一個作法是讓 `gain_now` 把增益降到 0。
- 畫面標頭顯示 `MUTE`。LED 和擺錘照常動。

### 3. 藍牙（`fm1.toml`、`app.h`、`app.c`）

- `fm1.toml`：`ble = true`、`ble_stack = "own"`、`ble_diag = false`。依 Morse 的數字，大約多 36 KB flash、9.5 KB RAM，額度還很夠。
- `app.h`：`APP_BLE_STORE`、`APP_BLE_AT_BOOT (!metro_ble_off())`。`app_ble_load` 和 `app_ble_save` 回傳 0（own stack 不寫 flash）。都照 `firmware/morse/app.c:769-799`。
- 開關存在 `rec.rsv` 的 bit 0（1 表示關）。舊的 record 讀起來是 0，也就是藍牙開。`ask_answer` 載入歌單時要保留 `rsv`，因為它會整個換掉 `rec`。
- 操作：**SET EDIT 裡轉 K4**，切換 BLUETOOTH ON/OFF，toast 和語音會說出來。標頭的 `BT` 沒連線時是灰色，連上是藍色（照 Morse 的 `app.c:2227`）。
- 收訊：main loop 用 `plat_midi_in` 讀 `PORT_BLE`，再用 `hl_sx_packet` 組回 frame（`hostlink.h:17-24` 的簡單寫法，不需要 Morse 的 ring）。`host_frame` 加一個 transport 參數，`hl_acked` 才會用對的通道。
- **歌單在藍牙上傳**：`sl_pump` 遇到 `hl_send` 回傳 -1（BLE 佇列滿）時要下一輪重試，不能跳過。網頁送 SET_LOAD 時每個 frame 之間等 30 ms，跟 Morse 一樣。
- 模擬器測不到藍牙這一段。`basic.txt` 照 Morse 的作法加 `ble_frame` 指令（`app_host_cmd`），測 transport 參數和 STATE 走藍牙。

### 4. 網頁（`web/`）

- **`web/link.js`**（新檔）：一個連線物件，介面是 `send(frame)` 加 `onFrame(bytes, timeStamp)`，把 USB 和藍牙兩種路徑包起來。
  - 從 Morse `web/index.html` 搬來：`hunt()` 會分辨 transport，USB 優先，看到藍牙先等 1.3 s 再決定（465-510）；Web Bluetooth 那段是 573-645，包括 `blePackets`、`bleIn`、`bleUnwrap`。
  - 純函式部分匯出給 node 測。
  - 歌單編輯器 `index.html` 也改用它，所以讀取和傳送歌單也能走藍牙。
- **`web/live.js`**（新檔，純邏輯，node 可以測）：
  - 解析 STATE，可以和 `setlist.js` 共用從 `fromRecord` 抽出來的 `songFrom(d)`。
  - 時間對應：機器時間 → `performance.now` 的毫秒 → AudioContext 時間。偏移量取最近 8 秒內「到達時間減機器時間」的最小值（Morse `bleOff` 的作法）。到達時間一定用 `event.timeStamp`（藍牙用 `bleUnwrap` 的時間），不用收到的那一刻。換到 AudioContext 時間用 `ctx.getOutputTimestamp()`。
  - 再加上一個使用者校正量（ms）。
  - 排程：沿用 `index.html` 的 `player`（提前 120 ms 排程）。改成以 STATE 的 anchor 為準；在 anchor 處切換（換歌時 tick 歸 0）；漂移校正每拍最多移 1 ms，避免跳拍。
- **`web/live/index.html`**（新頁，舞台用）：
  - Connect（USB）和 Connect Bluetooth 兩個按鈕；
  - 大字 BPM、速度術語、拍號、小節和拍點；
  - 指揮動畫；
  - 16 首歌的按鈕（PICK），加上 START/STOP、BPM ±1/±5；
  - 「網頁出聲」開關：開的時候送 MUTE 1，關的時候送 MUTE 0；
  - 校正：跟著機器的 click 按空白鍵 8 下，取中位數填進滑桿；
  - 手機寬度可用。不支援的瀏覽器（Safari 和 iOS 沒有 Web MIDI 和 Web Bluetooth）顯示說明。
- **`web/conductor.js`**（新檔）：依照歌的設定選指揮圖形，用 canvas 畫指揮棒尖的軌跡。拍點落在拍上，拍與拍之間是彈起的弧線。圖形的選法：

  | 歌的設定 | 指揮圖形 |
  |---|---|
  | GRID，div 1/2/4 | bpb 拍：1、2、3、4、5（3+2）、6（德式）、7–9 依重音分組 |
  | GRID，div 3（複拍子） | 打附點四分：2/4/3 拍，所以 6/8 打 2、12/8 打 4 |
  | 以八分為一拍的 preset（5/8、7/8、6/8 IN 6） | 依 steps 裡 A/N 的位置分組：5/8 3+2 打兩拍（長短）、7/8 2+2+3 打三拍、6/8 IN 6 打 6 |
  | CLASSIC | `accent_every` n 拍；0 就打 1 拍 |

  `patternFor(song)` 是純函式，用 node 測。動畫不用額外的圖檔。
- `index.html` 和 cheatsheet 加連到 live 頁的連結，cheatsheet 補上 SET EDIT 的 K4 和 MUTE。

### 5. Python 端（`host/fm1_metronome`）

- `protocol.py` 加 `T_STATE`、`T_CTRL` 和 STATE 的解碼器。
- `__main__.py` 加 `watch`：把收到的 STATE 印出來，可以用 `--start`、`--stop` 送 CTRL。這主要是在模擬器和實機上量延遲、做驗證的工具。
- 對應的 Justfile recipe：`metro-watch`。

## 測試與驗證

1. `just metro-unit`、`just fw-test metronome`。`basic.txt` 新增：
   - `expect states N`（STATE 的送出次數）；
   - 開始、停止、換歌（NEXT BAR 和 NOW）、改 BPM 時各送出一份；
   - 第一份 STATE 用 `expect_sysex` 比對整個 frame（模擬器時間固定）；
   - CTRL 的 START/STOP/PICK/BPM/MUTE 都會改變 `running`、`active`、`bpm`、`muted`；
   - `ask` 中只接受 STOP；
   - 靜音時 `n_accent` 照算，但 `peak_db_max` 很低；
   - 模擬連線逾時後自動取消靜音；
   - `ble_frame` 走藍牙那一路；
   - 載入歌單後 `rsv` 的藍牙設定還在。
2. `just metro-test`：
   - pytest 用 `basic.txt` 裡的 STATE 和 CTRL 向量，三方共用；
   - node 測 `live.js`：模擬 USB 和藍牙的 jitter（含成批到達），推算出的拍點誤差小於 1 ms；跨 anchor 換歌、BPM 改變、100 ppm 漂移都要涵蓋；
   - node 也測 `conductor.js` 的 `patternFor`（每種拍號和 preset）和 `link.js` 的 `blePackets`/`bleIn` 來回轉換。
3. `just fw-build metronome`，記下大小和 SHA。fm1-emu 兩種晶片都跑 headless：
   - 開機 0 次寫入、存檔、重開機、讀回；
   - 用虛擬 USB `FM1_USB_SEND` 送 CTRL START，`FM1_USB_TRACE` 看到 STATE；
   - 疊在最新的實機備份上預演，歌單還在 `[模擬]`。
4. 網頁用 Playwright：live 頁和指揮動畫在每種拍號下的截圖、手機 390 px 寬、沒有 JS 錯誤。live 頁接上 fm1-emu 是另一個 origin，沒有 MIDI，所以這裡只測解析、畫面和排程。
5. 開 PR（fm1-metronome 一個，superproject 一個，裡面放文件和指標）。照 template：`just fw-preview metronome pr-N`、附截圖、列出實機試用步驟：
   - USB 和藍牙各自連 live 頁；
   - 網頁出聲時機器是靜音的；
   - 拔掉線後機器恢復出聲；
   - 從網頁換歌時，機器在下一小節換；
   - 量出延遲，寫進 `docs/dev/metronome.md`；
   - 藍牙下傳一次歌單。
6. 刷機要另外經本人批准，走完整的 preflight，由本人用 `fm1_install.py` 刷。之後發 release 0.4，Hub 會自動跟上。

## 文件

- `docs/dev/metronome.md`：v0.4 段落（協定表、時間對應、藍牙、靜音的安全規則、指揮圖形表、驗證、實測延遲）。「之後」刪掉即時連線那條。
- `AGENTS.md` 的 metronome 列和 metro recipes，README（fm1-metronome）的 Controls 和 live 頁。
- `docs/hardware/flash-map.md`：藍牙不寫 flash，不用改，在頁面上註明。
