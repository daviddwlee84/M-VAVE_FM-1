# MORSE 0.4：練習判定輔助、進度管理與匯出、使用指南、截圖

## Context 背景

0.3 已發布並送上 Hub。實機使用後的回饋：

1. **PRACTICE 打得像卻判錯。** 原因在 `app.c` 的 `own_reset()`：每張卡都把解碼器重設成 SEND SPEED（預設 10 WPM：dit 120 ms，dah 要 ≥240 ms）。使用者是模仿聽到的 20 WPM 節奏在打（dah 約 180 ms），結果 M 被讀成 I、T 被讀成 E。`mo_dec_mark` 的「兩個 mark 相差兩倍以上就重讀」只救得了 dit 和 dah 混合的字。
2. 想要能**歸零、往回退**進度。
3. 想**匯出／匯入進度**，像 SLOOP 匯出歌曲那樣；換韌體或刷回原廠後不用重學。
4. 有經驗的人想**跳過已知的字**，三種方式都要：CHART 手動標記、開放到第 n 組、分級測驗。**任何手動修改都要先確認 SAVE 才寫入**，避免誤觸。
5. docs 要有一頁**用 MORSE 學摩斯的使用指南**。
6. fm1-morse repo 要補**截圖**：menu、每個主要功能。

使用者已選定：寬鬆判定加上答錯時的對照圖；匯出用 CLI 加 Web MIDI 網頁；跳過已知的三種方式都做，且都要確認。

版本規劃：韌體 0.4，host 0.4.0。BLE 從 0.4 延到 0.5，roadmap 跟著改。

## 1. 練習判定輔助

**手速延續**
- `morse.c` 新增 `mo_dec_restart(d)`：只清掉 `pat`、`prev` 和統計，保留 `dit_avg`、`dah_avg`、`cgap`。
- 練習中換卡改用 `mo_dec_restart`，學到的手速就會帶到下一張卡。只有開始新回合、或改了 SEND SPEED 時，才用 `own_reset()` 整個重設。
- 回合的第一張卡，起始 dit 取 SEND 和 HEAR 兩個速度 dit 的中間值。理由：使用者會模仿聽到的速度。

**寬鬆判定**
- `morse.c` 新增純函式 `mo_judge(mark[], gap[], n, want_code, dit_est) → MJ_OK / MJ_COUNT / MJ_DAH_SHORT / MJ_DIT_LONG / MJ_SPLIT / MJ_OTHER`。
- 規則：
  - 元素數必須相同。
  - dit 和 dah 混合的字：答案裡「應為 dah 的 mark」最短的那個，要比「應為 dit 的 mark」最長的那個長 1.5 倍以上。只比相對長短，不管絕對速度。
  - 只有一種元素的字（E T I M S O H 5 0）：和延續下來的手速比，界線放寬到 1.6 × dit。
- 判定用的資料取自已經在記錄的 `tim.live`（`app.c` 約 580 行，每個 mark 和字內 gap 的毫秒數）。
- 字被切開也要處理：解碼器先吐出前半個字時，若前半是預期碼型的前綴，就再等下一段。超過預期的元素數，或停頓超過字間 gap，才判定，原因記為 `MJ_SPLIT`。
- `set_t.look` 的 bit 7 目前沒用，拿來當「STRICT 判定」開關，放在 SET 頁，預設寬鬆。`set_valid` 跟著放寬，所以**存檔格式不用改**。

**答錯時的對照圖**
- PS_RESULT 畫兩列長短條：上面 SHOULD（預期碼型，用你的手速排出時間），下面 YOU（你實際的 mark 和 gap，同一個比例尺）。
- 下面一行提示，對應 `MJ_*`：dah 太短、dit 太長、字中間停太久被切開、多了或少了一個元素。文字沿用 RHYTHM 的 `H1`／`H2` 風格（`app.c` 約 906 行）。
- 判對但有一項接近界線時，給一行小提示（例如「dah 偏短」）。
- 畫長短條重用 KEY TIMING 的 `tim_row()`（約 1715 行）。

## 2. 進度管理：CHART 編輯模式（所有修改都要確認）

CHART 頁按 `REC` 進入 **EDIT**。所有改動先存在 RAM 裡的草稿 `ln_state_t draft`，畫面頂端顯示 `EDIT — unsaved`。

| 操作 | 效果（作用在草稿上） |
|---|---|
| SELECT 選字，K1 轉 | 該字的等級 0–5，認字和發報兩個方向一起改 |
| `OCT+` | 標為「已會」：認字盒 3、發報盒 2 |
| `OCT-` | 「忘了」：兩個方向都回盒 0，排到下回合 |
| K2 轉 | 開放到第 n 組。新開放的字設成剛好達到解鎖門檻（認字盒 2、發報盒 1）；縮回時，被關掉的字歸零 |
| `FX` | 全部往回退一盒（生疏時用，所有字都會提早複習） |
| 按住 `SEL` 2 秒 | 全部歸零，等同新手 |
| `PLAY` | 出現「SAVE CHANGES? PLAY=yes HOME=no」，再按一次 `PLAY` 才寫入 |
| `HOME`／離開頁面 | 有草稿時先問；選 no 就丟掉草稿 |

- `learn.c` 新增純函式：`ln_set_level`、`ln_mark_known`、`ln_forget`、`ln_open_to`、`ln_step_back`、`ln_reset`。每個函式做完都要讓 `ln_valid` 成立。
- 確切的按鍵配置，實作時會對照 CHART 目前已用的鍵再定，結果寫進速查頁。

**分級測驗（PLACEMENT）**
- PRACTICE 開始畫面加一個選項：`SELECT` 選「TEST」，`PLAY` 開始。
- 測試範圍：還沒開放的下兩組，每字出一張 HEAR_CHOOSE，靜音時改出 LIGHT 或 CODE 卡。
- 結果頁列出答對的字，問「MARK THESE KNOWN? PLAY=yes」。確認後，答對的字標為已會；若某組全部答對，就開放那一組。
- 不影響回合數和 streak。
- 實作上用 `ln_build` 的新 flag `LN_PLACE`。

## 3. 匯出／匯入進度

**協定**：hostlink 加三種訊息，要求韌體 ≥0.4，host 用 HELLO 帶的版本號判斷。
- `T_PROG_REQ`（host→dev）：要求匯出。
- `T_PROG`（dev→host）：資料分片。
- `T_PROG_LOAD`（host→dev）：匯入的分片。

資料是 `ln_state_t` 加 `REC_VER` 和 CRC16，用 7-bit 打包，每片 ≤ `HOSTLINK_MAX`（64 B）。

**匯入要在機器上確認**：收齊並檢查 CRC 和 `ln_valid` 後，畫面顯示「LOAD PROGRESS? round N, M letters open — PLAY=yes HOME=no」，按了才寫入。和第 2 節「手動修改要確認」同一個原則。

**host**（`host/morse/`，維持 MIT）：
- `morse backup FILE.json` 和 `morse restore FILE.json`，加 Justfile 的 `morse-backup`、`morse-restore`。
- JSON 格式人看得懂、也可以手改：
  ```json
  {"format": "fm1-morse-progress", "version": 1, "firmware": "0.4", "round": 0, "streak": 0, "open": 0,
   "answered": 0, "right": 0, "chars": {"K": {"recognise": 3, "send": 2, "last": [12, 11]}}}
  ```
- 測試向量由 `protocol.py` 和 `tests/basic.txt` 共用，和現有做法一樣。

**網頁**（fm1-morse repo 的 `web/progress.html`，GPL）：
- 單一檔案，用 Web MIDI（Chrome）找到 MORSE 埠，可以 Backup（下載 JSON）和 Restore（上傳 JSON），JSON 格式和 CLI 相同。
- 用 GitHub Pages 發布。開啟 Pages 是公開動作，做之前會先問你。

**文件**：寫明 Morse 的進度存在 `0xDA000`。實測從 SLOOP 切回來時進度還在，但刷回原廠 V15 就沒了，所以換韌體前先備份。寫進 README 和 `docs/dev/morse.md`。

## 4. 使用指南

新頁 `docs/dev/morse-learn.md`「用 MORSE 學摩斯」，繁體中文，加進 `zensical.toml` nav。內容：

- 第一次開機選 START preset。
- 每天一回合 PRACTICE。
- 通勤時用 LISTEN 背景聽。
- RHYTHM 練手。
- TYPE 和 KEY 自由玩。
- 什麼時候換 BUILDING／QSO preset。
- 判錯時怎麼看對照圖。
- 生疏了、已經會、換韌體時怎麼處理進度（CHART EDIT、TEST、backup）。
- 進階交給 Vail，FM-1 當它的鍵。

同時更新：
- `docs/index.md`、`README.md`：在「自己的韌體」那一行連到這頁。
- `docs/dev/morse-cheatsheet.md`：加上 0.4 的新操作。
- fm1-morse 的 README：加英文的 "Learning with MORSE" 小節。

## 5. 截圖

- `tests/basic.txt` 已經會產生約 25 張 `shot`。補上缺的：card_hear_key、card_see_key、result_wrong（含對照圖）、chart_edit、save_confirm、placement、type_letters、listen 播放中。
- 新增 `just morse-shots`：跑模擬器後，把挑好的 PNG 放大 2 倍（nearest neighbour），複製到 fm1-morse 的 `docs/screens/`。
- fm1-morse README 加一個 Screens 小節，用表格排：menu、practice（各種卡）、listen、key 的三種 view、rhythm、type 的 layout 和 code、chart 和 edit、settings。
- docs 的指南頁連到 GitHub 上的圖，不複製進這個 repo。

## 檔案 Files

- fm1-morse（submodule）：
  - `morse.c/h`（`mo_dec_restart`、`mo_judge`）
  - `learn.c/h`（編輯函式、`LN_PLACE`）
  - `app.c`（FW_MINOR 4；判定、對照圖、CHART EDIT、確認對話、TEST、`T_PROG*`、STRICT 設定）
  - `tests/basic.txt`、`tests/morse_test.c`
  - `web/progress.html`、`docs/screens/`、README、`fmhub.json` 不變
- 這個 repo：
  - `host/morse/`（`protocol.py`、`core.py`、`__main__.py`、`__init__.py` 改成 0.4.0、`PROTOCOL.md`）、`host/tests/test_morse.py`
  - Justfile（`morse-backup`、`morse-restore`、`morse-shots`）
  - `docs/dev/morse.md`（0.4 版本列、roadmap 的 BLE 改成 0.5）、`morse-learn.md`（新）、`morse-cheatsheet.md`、`zensical.toml`、`docs/index.md`、`README.md`、AGENTS.md（新的 Justfile 指令）

## Verification 驗證

1. `just morse-unit`：
   - `mo_judge`：M 用 180/180 ms 判對；I 用 60/60 ms 判對；dit 和 dah 對比不足判錯；切開、元素數不對時給出對的 reason。
   - `mo_dec_restart` 保留平均值。
   - `ln_*` 的編輯函式做完後 `ln_valid` 都成立；`ln_open_to` 往前開、往回縮都正確；`LN_PLACE` 的回合只出未開放的字。
2. `just fw-test morse`：
   - 腳本手（sendkeys）在 SEND=10 的設定下，用 20 WPM 的手速回答 M 和 T：寬鬆判對，STRICT 判錯，錯時畫出對照圖（shot）。
   - CHART EDIT：改了按 HOME，選 no 後 `store_writes` 不變；選 yes 才寫入；`reboot` 後還在。
   - TEST 回合。
   - `T_PROG_REQ` 會吐出向量；`T_PROG_LOAD` 先要求確認，CRC 錯就拒收。
3. `just morse-test`：host 向量測試；`morse backup` 和 `restore` 在 SimLink 上來回一次，資料不變。
4. `just morse-emu` 加 `--usb`：CLI backup → 改進度 → restore，在 `uc25hq80` 和 `p25q80` 各做一次存檔、重開、讀回。網頁在 Chrome 上對 emulator 的虛擬埠做一次 backup 和 restore（`[模擬]`）。
5. `just fw-build morse`，記錄大小。`just docs-check`。
6. 實機：照 AGENTS.md 的 preflight，用 USB-MIDI 更新到 0.4（你核准後由你執行），確認進度保留，再實際用 backup 和 restore 跑一次。v0.4 release、Hub 更新、開啟 Pages 都是公開動作，到時候一步一步問你。
