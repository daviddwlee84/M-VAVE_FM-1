# MORSE 0.7：開機頁設定、備份帶設定、Hub 的 Play in browser 與 cheatsheet

## Context 背景

你提了三件事：
1. Hub 卡片要有 ▶ Play in browser（像 Rainbow）。
2. cheatsheet 不要只連到 README。
3. 開機不要直接進 PRACTICE。改成在 SETTINGS 選開機頁，預設 MENU，而且匯出備份時這個設定也要保留。

第 3 項順便讓 Try 頁更好用：第一次在模擬器裡打開的人，會先看到選單。

## A. 韌體 0.7（fm1-morse submodule）

### A1. 開機頁 START PAGE

現況：`app_init` 寫死 `ui.page = PG_PRACTICE; ui.back = PG_PRACTICE`（`app.c:3117`）。

**存在哪裡**：`SET.mute` 的 bit 2–4（3 bit 可以放 0..7）。值就是頁面的 enum：`PG_MENU = 0`，`PG_PRACTICE`、`PG_LISTEN`、`PG_KEY`、`PG_RHYTHM`、`PG_TYPE`、`PG_CHART` 依序往下，不含 `PG_SET`。
- 記錄的版面（`REC_VER 1`）不變，0.6 的存檔可以直接讀。
- 舊存檔的這幾個 bit 都是 0，升級後**開機會進 MENU**，正好就是你要的預設值。
- 欄位改名：`mute` 改成 `flags`，註解寫清楚每個 bit。新增 `start_pg()` helper，寫法跟 `muted()` / `bt_off()` 一樣。

**驗證規則**：
- `set_valid` 和 `morse_ble_off` 原本檢查 `mute <= 3`，改成：bit 5–7 必須是 0，而且開機頁的值合法。
- **降版的注意事項**：0.6 只接受 `mute <= 3`。開機頁不是 MENU 的存檔裝回 0.6，整筆記錄會被重設，**進度也會跟著消失**。
  - 開機頁是 MENU 時，記錄和 0.6 的完全相同，降版沒有問題。
  - 這一點寫進 README 和 morse.md：降版前先備份，或先把開機頁改回 MENU。

**設定頁**：
- 新增第 14 項 START PAGE，`SET_ITEMS` 改成 14。
- 選項依序是 MENU、PRACTICE、LISTEN、KEY、RHYTHM、TYPE、CHART，OCT-/OCT+ 循環切換，`set_changed()` 照常延遲存檔。
- 選單捲動照 BLUETOOTH 那一項的做法調整。

**開機**：`ui.page = ui.back = start_pg()`。
- 開機頁是 MENU 時，`ui.back` 設成 PRACTICE。這樣按 HOME 會回到 PRACTICE，選單的游標也停在第一項。
- `pr.ps = PS_START` 照舊。

### A2. 備份格式 2：進度加上設定

現況：`prog_fill` 只放 format `01`、`ln_blob`（259 B）和 CRC，總共 262 B，切成 6 個 frame（`app.c:1170`、`prog_in` 在 `:1222`）。

**格式 2**：format `02`、`ln_blob`、**設定 13 B**、CRC。
- 設定 13 B 是逐欄寫出的 little-endian，不用 `memcpy` struct，避開 padding。
- 總共 275 B，還是 6 個 frame（6 × 49 = 294）。`PROG_FRAMES` 不變。

**送出**：一律送格式 2。

**接收**：
- 先看第一個 byte。`01` 照舊只載入進度；`02` 先驗 CRC，再檢查 `set_valid`。
- 確認畫面寫 `PROGRESS + SETTINGS`。按 PLAY 同意之後，套用設定，跟開機時一樣重新初始化：
  - keyer、player、`timing_apply`；
  - `plat_ble_enable`；
  - 存檔一次。
- 抽一個 `settings_apply()`，給 `app_init` 和載入共用。

**版本**：
- `FW_MINOR` 改成 7。
- 協定不加新的 frame type，只多一個 format，改由 HELLO 回報的版本決定：只有 ≥ 0.7 的韌體才收得到格式 2。
- roadmap 原本的 0.7（central）順延成 0.8。

### A3. `tests/basic.txt`

- 全新開機進 MENU。
- 0.6 的存檔（`mute = 0` 或 `2`）開機也進 MENU，BT 狀態保留。
- START PAGE 改成 KEY，`store_writes` 加 1，重開機後進 KEY。
- 新增 `shot settings_start.png`。
- 設成 KEY 之後 PROG_REQ 送出的備份是格式 2；寫一組新向量 `prog_v2_*`。
- 格式 1 的舊向量 `prog_load_*` 還能載入，設定不變。
- 格式 2 載入、按 PLAY 同意之後，設定（開機頁、WPM）跟著換，按 HOME 拒絕就不變。
- `mute` 不合法（bit 5 是 1）的存檔回到預設值。
- HELLO 向量改成 `…00 07 00 F7`。

## B. host 與網頁

**`host/morse/progress.py`**（0.7.0，`FIRMWARE_KNOWN (0,7)`）：
- 解析格式 1 和 2。JSON 改成 `"version": 2`，多一個 `"settings"` 物件，欄位有名字：
  - `hear_wpm`、`spacing_wpm`、`send_wpm`、`pitch_hz`；
  - `keyer`、`paddles_swapped`、`sound`、`bluetooth`；
  - `cards`、`learn_by`、`judging`、`type_layout`、`key_view`、`start_page`；
  - `look` 的其他 bit 也照樣列出來。
- 還原時看韌體版本：≥ 0.7 送格式 2；0.4–0.6，或檔案沒有 settings，就送格式 1，只還原進度，並印一行說明。
- `version: 1` 的舊檔照樣能讀。
- 同一份向量寫進 `VECTORS` 和 `test_morse.py`。

**`web/index.html`**：
- 同樣支援格式 1 和 2，也同樣依版本降級。
- 「Only show it」多顯示一張設定表。
- 頁首加兩個連結：Try in the browser、Cheatsheet。

**`PROTOCOL.md`**：寫進格式 2 的位元組版面、JSON `version` 2，以及降級規則。

## C. Hub：Play in browser 與 cheatsheet

**已經查到的事實**：
- Hub 卡片的按鈕來自 `fmhub.json` 的 `play`，cheatsheet 來自 `cheat`。
- 大多數韌體的 `play` 指向 fm1-emu 網頁版。fm1-emu 讀 Hub 的 `catalog.json`，所以 `?fw=morse` 可以直接找到 MORSE，版本跟著 Hub。
- **你遇到的 404**：我用全新的 headless Chromium 開了 5 次。
  - 第 1 次重現了 `fw/morse-0.3.fwsc: 404`。
  - 後面 4 次都抓到 `morse-0.5.fwsc`，開機進 PRACTICE ROUND 1，100 MHz 是 3.5 倍即時。
  - 原因：fm1-emu 自己的清單還沒收錄 MORSE，套件網址直接取自 Hub 的目錄。拿到 0.3 時期的舊目錄快取，就會去抓已經刪掉的檔案。fm1-emu 的備援也查同一份目錄，所以救不回來。
  - 這是暫時性的，重新整理通常就好；v0.6、v0.7 換版時可能又會短暫出現。
  - 不用 `?package=` 釘死某個 Hub 檔案，因為換版後會永久 404；GitHub release 的檔案也沒有 CORS。

**C1. `web/try/index.html`（新檔）**：
- iframe 嵌入 `https://joakimeriksson.github.io/fm1-emu/?fw=morse&embed=1`。
- 「先試這些」：開機後在 MENU 選 KEY 打字、LISTEN 聽，電腦鍵盤的對應連到 fm1-emu 的說明。
- 限制：模擬器沒有藍牙，進度只存在這個瀏覽器；要接 Vail 和備份請用實機（連到 `../`）。
- 一個「重新載入」按鈕：用 cache-busting 重新載入 iframe，處理上面的 404。另外放一個 fm1-emu 全頁的連結。
- 色票和深色模式沿用 `web/index.html` 的 `:root`。

**C2. `web/cheatsheet.html`（新檔，英文）**：
- 從 `docs/dev/morse-cheatsheet.md`（中文，比 README 完整）翻譯：各頁按鍵、header 的 `BT` / `PC`、全部 14 個設定（含 START PAGE）、prosign 表。
- 單頁，`@media print` 可以印成 A4。
- README `## Controls` 加一行連到這頁；中文 cheatsheet 頂端連到英文版，並補上 START PAGE。

**C3. `fmhub.json`**：
```json
"play":  "https://daviddwlee84.github.io/fm1-morse/web/try/",
"cheat": "https://daviddwlee84.github.io/fm1-morse/web/cheatsheet.html"
```

**C4. skill 和文件**：
- `fm1-hub-listing` SKILL.md 補上：
  - `play` 的兩種做法：`fm1-emu/?fw=<hub id>`，或自己的 `/try/` 頁加 `&embed=1`；
  - `?package=` 需要 https 加 CORS，GitHub Pages 可以，release 檔案不行；
  - 新上架的韌體會因為目錄快取短暫 404；
  - `cheat` 最好指向 GitHub Pages 單頁。
- `docs/firmware/hub.md`：寫中文版，標 `[上游]`，實測結果標 `[模擬]`。
- `docs/dev/morse.md`：
  - 新增 0.7 版本列，大小和 RAM 用 build 出來的數字；
  - roadmap 的 central 改成 0.8；
  - 備份格式 2 和降版注意事項；
  - release 一節記錄 play 和 cheat 上線。
- 其他：`morse-cheatsheet.md`、`morse-learn.md`（開機進 MENU）、README（Firmware 0.7）、`docs/index.md`；`Justfile` 的 `morse-shots` 加 `settings_start`。

## Verification 驗證

1. `just morse-unit`、`just fw-test morse`、`just morse-test` 全部通過。
2. 先 commit submodule pointer，再跑 `just fw-build morse`，記錄大小。fm1-emu 在 `uc25hq80` 和 `p25q80` 上各跑一次：開機進 MENU、改 START PAGE、存檔、重開機、讀回 `[模擬]`。
3. 本機 `http.server` 開 try 頁和 cheatsheet，用 Playwright（快取裡的 Chromium）截圖：iframe 裡 MORSE 開機；cheatsheet 在亮色、深色和手機寬度下都正常。
4. `python3 -m json.tool fmhub.json`；`just docs-check`。
5. **實機**：刷 0.7 是韌體寫入，要你核准，由你輸入 `yes`。
   - 進度要保留，開機進 MENU。
   - 網頁備份出來是 version 2、含設定。
   - 把 START PAGE 設成 KEY 後還原舊的 0.6 備份：進度回來，設定不變。
   - 結果寫進 `device-log.md`。
6. commit、push、v0.7 release 都等你說。push fm1-morse 之後，Pages 和 Hub（大約 3 小時）才會更新。
