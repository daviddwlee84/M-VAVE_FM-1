# CUBE：FM-1 上的魔術方塊計時器、打亂、公式庫與虛擬方塊

## Context 背景

本人想在 FM-1 上做一個三階魔術方塊的 app，體驗參考 ccTimer / cstimer / thecube.guru 的計時器，再加上 FM-1 才有的東西（琴鍵轉方塊、燈提示下一步、K1–K4 轉視角）。2026-11-27 決定：

- **四塊一次做完**（v0.1 就包含）：TIMER、SCRAMBLE、ALGS（查公式 + 出題）、虛擬 CUBE。
- **打亂預設 random-state**（WCA 做法：從約 4.3×10¹⁹ 種狀態均勻挑一種，再求一條約 18–21 步的路徑），**settings 可切 random-move**（長度可調）。可行性要先驗證（見 M1）。
- 公式集：**完整 OLL 57 / PLL 21、2-look OLL/PLL、F2L 41、初學者層先法 LBL**。
- 鍵位：**多種 mapping 都要有，讓使用者在 settings 選**：白鍵順時針 + 右邊黑鍵逆時針（預設）、cstimer 雙手式、`SEL` 當 prime 修飾鍵，再加一個 CUSTOM（逐鍵指定）。
- `K1` / `K2` / `K3` 轉 x / y / z 軸的視角，`K4` 在幾個斜看的 preset 視角間切換。

和現有的 own app 一樣：獨立韌體，放在 `firmware/cube/`（名稱暫定 CUBE，發布前對照 Hub 再定），GPL-3.0-or-later（因為 include THEORY 的 `srs.c`）。要上 The Firmware Hub 時再像 Morse / METRONOME 一樣拆成自己的 repo。

## 硬體與平台限制（已查）

- 畫面 240×240 彩色，透過 `third_party/fm1-base/device/gfx.c` 的 canvas（`cv_begin` / `cv_rect` / `cv_line` / `cv_text`，分 band 畫，`APP_CANVAS_PX` 限制）。**沒有多邊形填色**：app 自己寫 scanline 的四邊形填色（在 band 內 clip）。
- FPU 只有 float（`-mfprev1`），沒有 libm：用 `fastmath.h` 的 `fm_sinf` / `fm_cosf`；build 會檢查 soft-double。
- 輸入：27 鍵（16 白 11 黑，`PLAT_WHITE_KEYS` / `PLAT_BLACK_KEYS`）、`B_*` 14 個按鈕、7 個無限旋鈕（`EN_SELECT/ALGO/PRESET/K1..K4`，`plat_enc` 給 detent）。
- 存檔：`plat_store_save/load`，每個物件最多 3840 B（`PLAT_STORE_MAX`），每物件佔兩個 4 KiB sector。
- App 槽 581,564 B；現有 app 約 100–132 KB，所以**約 400 KB 可給 const 表格**（XIP flash）。RAM `.data+.bss` 98,304 B，現有 app 用 25–40 KB。
- 有第二顆核心（`third_party/fm1-base/docs/SECOND-CORE.md`，`plat_cpu2_set`），可以拿來在背景求解打亂。

## 架構

`firmware/cube/`（照 fm1-base template 與 DRUMS / GUITAR 的格局）：

| 檔案 | 內容 |
|---|---|
| `cube.c/.h` | 平台無關的方塊模型：cubie 表示（角/邊的位置與方向）+ 54 facelet 轉換；所有記號 `U D L R F B`（含 `'`、`2`）、`M E S`、wide `u d l r f b` / `Rw`、`x y z`；parse / print / invert；`cube_solved`、`cube_f2l_solved`、`cube_ll_solved_auf`（只差 U 層轉動）；random-move 產生器（不連續同面、不同軸抵消，長度可設） |
| `solve.c/.h` | random-state：產生合法隨機狀態（角/邊 parity 一致、方向和為 0），兩階段（Kociemba two-phase）求解，反過來就是打亂。表格見 M1 |
| `tools/gen_tables.c` → `gen/tables.h` | 在電腦上產生 move / pruning 表，git-ignored（照 `firmware/voicelab/gen/` 的做法），`just cube-tables` 產生，`fw-build cube` 前先跑 |
| `algs.h` | 公式資料，一行一個（set、名稱、主公式、備用公式），**append only**（SRS 進度用 index），跟 THEORY `course.h` 同規則 |
| `stats.c/.h` | WCA 規則的 mo3 / ao5 / ao12 / ao50 / ao100（去頭去尾、DNF 算最慢、兩個 DNF 則 DNF）、best、mean |
| `view.c/.h` | 3D 投影（正交投影，旋轉矩陣由 yaw/pitch/roll），以 27 個 cubie 為單位依深度排序畫（painter's），所以轉層動畫也能畫；2D 展開圖（net）；LL 俯視 case 圖（頂面 + 四邊一排 sticker） |
| `keymap.c/.h` | 鍵位 preset 與 CUSTOM 表：27 鍵 → move id |
| `app.c/.h` | 頁面、輸入、燈、聲音、存檔 |
| `fm1.toml` | `include = ["../common", "../theory"]`（`ledfx.c`、`chime.c`；`srs.c`），identity 見下 |
| `tests/basic.txt`、`tests/cube_test.c`、`tests/vectors.h` | simulator 情境與平台無關部分的 unit test |
| `host/cube/make_vectors.py` | 用一個現成的 Python 魔術方塊函式庫（uv 抓，例如 `kociemba` 的 facelet 字串或 `pycuber`）產生參考 vectors：給定公式後的 54 facelet，跟 `theory-vectors` 同一個做法 |

沿用：THEORY 的 `srs.c`（Leitner，GUITAR 的 LEARN 也這樣 include）、`firmware/common/chime.c`（嗶聲）與 `ledfx.c`（燈）、DRUMS / METRONOME 的畫面分區（header 32 px / main / footer 32 px、toast）與 `tcat` / `tint` 小工具。

## 頁面與操作

`HOME` 開 MENU：TIMER、CUBE、ALGS、DRILL、STATS、SETTINGS（`SELECT` 旋鈕選、`PLAY` 進）。

**TIMER（含 SCRAMBLE，ccTimer 的體驗）**
- 上方顯示打亂公式（自動換行），下方小張 net 圖（打亂後的樣子，由 `cube.c` 算出來，不用相機）。
- 開始：預設「按住任一鍵 0.3 秒變綠、放開開始」；settings 可改成 **Stackmat 式：同時按住最左 `F3` 與最右 `G5`**（像計時墊的兩個手掌位置）。停止：任何鍵。
- WCA inspection（可關）：15 秒，8 / 12 秒嗶聲，超過 15 秒自動 +2，超過 17 秒 DNF。
- 結束後：`K1` 切 OK / +2 / DNF，`SEL` 刪除這一筆；自動產生下一個打亂（random-state 在背景先算好，見 M1）。
- 畫面顯示 ao5 / ao12 / best。

**CUBE（虛擬方塊）**
- 琴鍵轉方塊（依 settings 的鍵位 preset），轉層有短動畫與 click 聲；按同一鍵兩下 = X2。
- `K1` / `K2` / `K3` 轉 x / y / z 視角（每 detent 15°，只轉相機，不改方塊記號）；`K4` 切 preset 視角：UFR（標準）、UFL、DFR（看底）、UBR、UBL（看背面）……
- `REC`：套用一個新打亂（random-state / random-move），**燈亮下一步要按的鍵**，跟著按就打亂完成；`PLAY`：開始計時，轉回 solved 自動停（虛擬 solve 計時）。
- 鍵位裡的 UNDO（預設 `E5`）退一步；`SEL` + UNDO 重設成 solved。
- footer 顯示最近幾步的記號。

**ALGS（公式庫）**
- `SELECT` 選 set（LBL / 2-look OLL / 2-look PLL / OLL / PLL / F2L），`PRESET` 旋鈕選 case。
- 每個 case：case 圖（**由程式把公式反過來套到 solved 上自動畫**，不手繪；OLL/PLL 用 LL 俯視圖，F2L / LBL 用 3D 視角）、名稱（例如 `OLL 21`、`T-perm`）、主公式與備用公式。
- `PLAY`：在虛擬方塊上看公式一步一步播放（燈同步亮鍵位）。

**DRILL（出題）**，選 set 與子集合後三種模式：
1. **RECOGNIZE**（閃卡）：出 case 圖，想公式，`PLAY` 翻面看答案，再用鍵評分（again / hard / good），進度用 `srs.c` 的 Leitner 回合存檔。
2. **REAL**（實體方塊，就是「按公式打亂後再用對應公式轉回來」）：顯示 setup 打亂（公式的反向，前後加隨機 AUF），照著在實體方塊上打亂，然後用 TIMER 的方式計時解這個 case，記每個 case 的平均時間。
3. **VIRTUAL**：虛擬方塊設成那個 case，用琴鍵做出公式，`cube_ll_solved_auf` / `cube_f2l_solved` 判定成功並計時；可開「燈提示下一步」。

**STATS**：這個 session 的每一筆（時間、懲罰、打亂），`SELECT` 捲動；best / mean / ao5 / ao12 / ao50 / ao100；`SEL` 長按清除 session（畫面上確認）。

**SETTINGS**：打亂方式（RANDOM STATE / RANDOM MOVE）與 random-move 長度（15–30，預設 25）、inspection 開關、開始方式（ANY KEY / TWO HANDS）、鍵位 preset（BLACK PRIME / CSTIMER / SEL PRIME / CUSTOM）、CUSTOM 編輯（按一個鍵、`SELECT` 旋鈕選它的 move）、轉層動畫開關、配色（標準西方配色：白頂綠前）。

### 鍵位 preset

1. **BLACK PRIME（預設）**：白鍵順時針、右邊緊鄰的黑鍵逆時針；同鍵兩下 = X2。

   | 白 | 黑 | | 白 | 黑 |
   |---|---|---|---|---|
   | F3 `L` | F#3 `L'` | | F4 `U` | F#4 `U'` |
   | G3 `D` | G#3 `D'` | | G4 `E` | G#4 `E'` |
   | A3 `B` | A#3 `B'` | | A4 `S` | A#4 `S'` |
   | B3 `x` | — | | B4 `z` | — |
   | C4 `F` | C#4 `F'` | | C5 `r` | C#5 `r'` |
   | D4 `M` | D#4 `M'` | | D5 `u` | D#5 `u'` |
   | E4 `y` | — | | E5 UNDO | — |
   | | | | F5 `R` | F#5 `R'` |
   | | | | G5 保留 | — |

   `x y z` 的逆向：`SEL` + 鍵（BLACK PRIME 也接受 `SEL` 當 prime）。
2. **CSTIMER**：模仿 cstimer 鍵盤的雙手分配（右手半邊管 `R` / `U` / `F'` 等、左手半邊管 `L` / `D` / `F` 等，左右對稱），實作時對照 cstimer 的鍵盤表排到 27 鍵上，並在 `docs/dev/cube.md` 附對照圖。
3. **SEL PRIME**：16 個白鍵 = 16 個順時針動作，按住 `SEL` 變逆時針；黑鍵做 X2。
4. **CUSTOM**：從任一 preset 複製後逐鍵改，存在 settings record 裡（27 bytes）。

每個 preset 在 CUBE 頁 `SEL` 長按會顯示鍵位速查圖（另外做成 `docs/` 的 cheatsheet 頁）。

## 存檔與識別碼

- 三個物件（`APP_STORE_MAP {0xDC000, 0xCC000, 0xCA000, 0xC8000}`，緊接在 METRONOME 的 `0xCE000` 下面；**刷機前用 `just emu-rehearse` 確認這幾個 sector 在最新備份裡是空的**，並更新 `docs/hardware/flash-map.md`）：
  - `OBJ_CUBE`：settings + CUSTOM 鍵位
  - `OBJ_SOLVES`：最近 100 筆（時間 ms、懲罰、打亂以每步 1 byte 存，≤ 3840 B）
  - `OBJ_DRILL`：`srs.c` 的進度 blob + 每個 case 的 REAL 平均時間
- 只透過 `APP_STORE_MAP` / `plat_store_*`，不自己算 flash 位址。
- identity：實作時讀 `https://thefirmwarehub.com/catalog.json`，挑一個沒人用的 `FM-1_6x`（目前 own app 用到 `FM-1_59`），寫進 `fm1.toml` 的註解。

## 實作順序（同一個 PR，每一步都先過 test 再往下）

**M1 random-state 可行性 spike（先做，決定 solver 的形式）**
- 自己依 Kociemba two-phase 的公開描述寫（或 port 授權相容 GPL-3.0 的實作；min2phase / muodov kociemba 的授權實作前先查，GPL-2.0-only 不能用）。
- 對稱縮減的 phase 1 pruning（twist×slice、flip×slice，各約 80 KB nibble），phase 2 先用小的 CPerm×MPerm（約 33 KB），目標 const 表格 ≤ 250 KB、RAM 搜尋狀態 < 4 KB。
- 在電腦上：1000 個隨機狀態全部解得出、驗證、步數分布（目標 ≤ 21–22 步）。在 fm1-emu 量一次求解的指令數 `[模擬]`；實機時間之後在 unit 上量 `[實機]`。
- 執行：每完成一次 solve 就在背景（第二核心，或 main loop 分片）先算好下一個打亂，使用者感覺不到等待。
- **若太大或太慢的退路**（spike 結果寫進 `docs/dev/cube.md` 再選）：(a) phase 2 換更小的表、放寬步數上限；(b) 用電腦預先產生的 random-state 打亂庫（例如 2000 個，每個約 21 bytes，約 42 KB）放在 flash，均勻隨機但有限。不論哪個，settings 都保留 RANDOM MOVE。

**M2 方塊模型 + 打亂**：`cube.c`、random-move、`just cube-unit`（群論恆等式：`R4`、`(R U R' U')6`、`M2 U M2 U2 M2 U M2` 等於 identity 等；與 `make_vectors.py` 的參考 facelet 比對；invert 與 parse/print 來回）。

**M3 畫面**：`view.c` 的 3D、net、LL 俯視圖；simulator 截圖檢查。

**M4 TIMER + STATS**：計時狀態機、inspection、懲罰、WCA 平均、存檔。

**M5 CUBE**：鍵位 preset、轉層、動畫、視角、UNDO、跟著燈打亂、虛擬計時。

**M6 ALGS + DRILL**：`algs.h` 全部公式集；unit test 檢查：每個公式解得了自己的 case、F2L 公式不破壞其他已完成的槽、OLL/PLL 不破壞前兩層、OLL 57 / PLL 21 / F2L 41 在 AUF 下兩兩不同且數量對；名稱編號人工對照 speedsolving wiki / SpeedCubeDB 的 case 圖，頁面標 `[調研]`。

**M7 SETTINGS、整合、文件**。

## 要改 / 新增的檔案

- 新增 `firmware/cube/`（上表）、`host/cube/make_vectors.py`
- `Justfile`：`cube-unit`、`cube-tables`、`cube-vectors`、`cube-emu [CHIP]`（照 `drums-unit` / `guitar-emu` 的寫法）
- `.gitignore`：`firmware/cube/gen/`
- `docs/dev/cube.md`（設計、操作、鍵位圖、M1 的結果、驗證表），`docs/dev/cube-cheatsheet.md`（鍵位速查）；加進 `zensical.toml` 的 `nav`
- `docs/hardware/flash-map.md`（新的三個物件）、`AGENTS.md` 的 Layout 表與 Commands
- 不動 `third_party/fm1-base`

## Verification 驗證

1. `just cube-unit`：模型、記號、vectors、random-move 規則、solver（1000 個隨機狀態）、WCA 平均、公式集檢查全過。
2. `just fw-test cube`：`tests/basic.txt` 情境：按住放開開始 / 停止、inspection 的 +2 與 DNF、ao5、打亂顯示、CUBE 每個鍵位 preset 轉面與 UNDO、轉回 solved 自動停錶、K1–K4 視角、ALGS 翻頁、DRILL 三種模式評分、settings 改鍵位後重開還在；截圖給 PR。
3. `just cube-tables && just fw-build cube`：app 大小、`.data+.bss`、沒有 soft-double，記進 `docs/dev/cube.md`。
4. `just cube-emu uc25hq80` 與 `p25q80`：開機、打亂（量 random-state 求解時間 `[模擬]`）、存檔、重開、讀回；記錄只寫了哪些 sector。
5. `just emu-rehearse firmware/cube/build/cube.fwsc backups/<最新>/…bin`：確認不碰 THEORY / Morse / METRONOME 的 sector。
6. `just docs-check`。
7. PR：`just fw-preview cube pr-N` 的試玩連結、截圖、給本人在實機上試的步驟清單。**刷進實機要本人在當次 session 核准**，照 AGENTS.md 的 preflight，之後更新 `docs/device-log.md`。

## 之後（不在這次）

solves 匯出（csTimer 相容 JSON，USB SysEx + 網頁，照 METRONOME 的 backup）、解法提示（用同一個 solver 給「最佳解」）、2×2 / 其他方塊、藍牙智慧方塊（GAN 等用的是一般 GATT 不是 MIDI，own BLE stack 能不能當 central 連要另外調研）。
