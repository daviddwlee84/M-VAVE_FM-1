# 節拍器 METRONOME

給現場用的節拍器：一首歌一個白鍵，存好每首歌的速度、拍號、節奏型與音色，演出時按一下就換歌。另外有一頁在琴鍵上點出節奏型（亮度就是輕重音），以及仿照經典 rhythm watch 的四層混音頁。

**狀態（2026-11-20）：** v0.2（速度術語、`SELECT` 加速、6/8 的寫法、全部清空）在 host simulator、device build 與 fm1-emu 兩種晶片驗證過 `[模擬]`，還沒刷進實機，見 [v0.2](#v02)。v0.1 在 host simulator、device build、fm1-emu 兩種晶片與疊在備份上的預演都驗證過 `[模擬]`，2026-11-20 刷進實機，本人試用「感覺效果不錯」`[實機]`。見 [v0.1 驗證](#v01-verify)。

- 韌體在 `firmware/metronome/`，GPL-3.0-or-later，名稱 METRONOME。click 的合成與 transport 沿用 [RHYTHM](rhythm.md)。
- 識別碼 `FM-1_59`（開發版 `FM-1_5900`，release 0.2 是 `FM-1_59000200`）：2026-11-19 對照 Hub 的 catalog 沒人用 `[上游]`；2026-11-20 再把 catalog 裡 269 個韌體的 `ident`（是 regex）拿來比對 `FM-1_5900`、`FM-1_59000100`、`FM-1_59000200`，除了萬用的 `FM-1_\d{3,8}` 都沒有符合 `[上游]`。Hub 的 [identity 檢查](https://thefirmwarehub.com/submit/#check) 要的是**韌體回報的完整數字**，3 到 8 位：只打 `FM-1_59` 會顯示「An identity is FM-1_ and 3 to 8 digits」，那是格式不對，不是被佔用。上 Hub 前用完整的 release identity 再查一次。
- 設定與 16 首歌存在 `0xCE000`（`APP_STORE_MAP {0xDC000, 0xCE000}`）。`0xD0000` 到 `0xDE000` 都被自製 app 與面板校正用掉了，見 [flash 配置](../hardware/flash-map.md)。這台機器 2026-11-20 的備份裡 `0xCE000` 是空的 `[實機]`。
- 為什麼另做一個韌體，不放進 RHYTHM：本人 2026-11-19 決定。RHYTHM 是練習機，這個是上台用的工具，開機就要是歌單。

## 現有的節拍器韌體 { #survey }

2026-11-19 查 [The Firmware Hub](https://thefirmwarehub.com/catalog.json) 的 catalog（269 個韌體）`[上游]`：**沒有專門做節拍器的韌體**。節拍器只是別的韌體的附屬功能：

| 韌體 | 和節拍器有關的部分 |
|---|---|
| [h1](https://github.com/jakehandy/h1) | 合成器，內建 metronome |
| [Frodhes](https://github.com/leonardoMirabal/frodhes) | 電鋼琴、木琴，有 tap tempo 與 MIDI clock sync |
| [BEETLE BLUE](https://github.com/SikkenWorks/BEETLE-BLUE) | 鼓的 tracker，Live Mode 可以排 setlist（是歌單，不是節拍器） |
| 自製 [RHYTHM](rhythm.md) | METRO 頁：拍號、細分、tap tempo、gap click；沒有 preset |

GitHub 上搜 `fm1 metronome` 也沒有結果。

## 操作 { #controls }

兩頁，`HOME` 在 SET 與這首歌的節奏頁（GRID 或 CLASSIC）之間切換。設定停手 3 秒後自動存。黑鍵在每一頁都是 tap tempo（照拍子按，取最後 4 下），也會跟著拍子閃。

| 控制 | SET | GRID | CLASSIC |
|---|---|---|---|
| `PLAY` | 開始 / 停止 | 同左 | 同左 |
| 白鍵 | 選歌（有存的鍵半亮，播放中的全亮） | 小節裡的一個 step，按一下換一級 | tap tempo |
| 黑鍵 | tap tempo；**播放中、不在 EDIT 時不作用**（防誤觸，頁尾提示 `REC: EDIT`） | tap tempo | tap tempo |
| `REC` | EDIT 開 / 關 | tap tempo | tap tempo |
| `SEL` | EDIT 時：按住 `SEL` 再按白鍵，清掉那首歌；按住 2 秒不按鍵，問 `CLEAR ALL?`（[全部清空](#clear-all)） | 切到 CLASSIC | 切到 GRID |
| `SELECT` | BPM ±1（30–300）；轉得快（一格不到 50 ms）一格 ±5 | 同左 | 同左 |
| `OCT-` / `OCT+` | BPM ±5 | BPM ±5 | BEAT ±1（0–9，0 是沒有重音） |
| `ALGO` | 音色：CLICK、WOOD、BEEP | 同左 | 同左 |
| `K1` | 換歌的時機：NEXT BAR / NOW | 拍數 1–9 | 四分音符的音量 0–7 |
| `K2` | 跳到下一個[速度術語](#markings)的典型速度 | 一拍幾格：四分、8 分、3 連音、16 分 | 8 分音符的音量 |
| `K3` | | 拍號 preset（[見下](#presets)） | 3 連音的音量 |
| `K4` | | | 16 分音符的音量 |

### SET：歌單 { #set }

- 16 首歌對應 16 個白鍵，由左到右是 1 到 16。有存的鍵半亮，正在用的那首全亮，沒存的不亮。
- 平常（不在 EDIT）：按有存的鍵就換到那首。播放中預設在**下一小節**才換，等的時候那個鍵會閃，換過去時新的歌從它自己的第一拍開始。`K1` 可以改成 NOW（下一個 tick 立刻換）。按沒存的鍵只會提示 `EMPTY`。
- `REC` 進 EDIT（`REC` 燈亮，畫面寫 `SET: EDIT`）：
    - 按一個鍵就選它。空的鍵會先複製目前這首歌的設定。
    - 同一個鍵照拍子再按幾下就是 tap tempo（選它的那一下算第一下）。
    - 或用 `SELECT`、`OCT-` / `OCT+` 精確調。
    - 按住 `SEL` 再按白鍵就清掉那首。最後一首不能清。
    - 再按一次 `REC` 離開。
- 畫面上半是 BPM、速度術語、拍號與一個擺錘；下半是 16 格，每格寫歌號與 BPM。拍號的寫法兩種模式一致：GRID 是 `4/4  8THS`，3 連音寫成複拍子（`6/8`），CLASSIC 是 `BEAT 5  CLASSIC`（BEAT 0 是 `NO ACCENT  CLASSIC`）。

#### 全部清空 { #clear-all }

演出完要拿來排下一場：在 EDIT 裡按住 `SEL` 2 秒、中間不按琴鍵，頁尾變紅字 `CLEAR ALL?  PLAY = YES`。按 `PLAY` 才清（不會開始播），按其他鍵或 5 秒不理就取消。清完只剩第 1 首（120 BPM、4/4），播放中會停下來，換歌時機（`K1`）保留。按住 `SEL` 期間有按白鍵就是「清一首」，不會問全部清空。

#### 速度術語 { #markings }

BPM 旁邊顯示目前速度屬於哪個古典速度術語。[維基百科〈速度 (音樂)〉](https://zh.wikipedia.org/zh-tw/%E9%80%9F%E5%BA%A6_(%E9%9F%B3%E6%A8%82)) 的範圍會重疊（例如 Andante 76–108、Moderato 90–120）`[上游]`，畫面上只能顯示一個，所以照機械節拍器（Maelzel）刻度切成不重疊的段，60–65 補上 Larghetto。切法是自己定的：

| 術語 | BPM | `K2` 跳到 |
|---|---|---|
| LARGHISSIMO（極緩板） | 30–39 | 35 |
| LARGO（緩板／廣板） | 40–59 | 50 |
| LARGHETTO（甚緩板） | 60–65 | 63 |
| ADAGIO（柔板／慢板） | 66–75 | 70 |
| ANDANTE（行板） | 76–107 | 92 |
| MODERATO（中板） | 108–119 | 112 |
| ALLEGRO（快板） | 120–167 | 132 |
| PRESTO（急板） | 168–199 | 184 |
| PRESTISSIMO（最急板） | 200–300 | 208 |

SET 的 `K2` 每轉一格跳到上一段或下一段的典型速度。不在典型速度上時，先跳到本段的典型速度（140 往下轉是 132，再轉才是 112）。跳完可以再用 `SELECT` 細調。

### GRID：在琴鍵上點節奏型 { #grid }

- 一小節是「拍數 × 一拍幾格」個 step（最多 16 個），一個白鍵一格。
- 每按一下換一級：不響 → ghost（很小聲，鍵微亮）→ 一般（鍵半亮）→ 重音（鍵全亮）→ 不響。**鍵的亮度就是它有多大聲**。
- 播放中正在響的那一格全亮（重音的那格反過來暗一下），畫面上那一格變綠色。
- `K1` 改拍數、`K2` 改一拍幾格時，節奏型會重設成預設（第一拍重音、其他拍一般、拍與拍之間 ghost）。
- 不均分的節奏（附點、shuffle）在 16 分或 3 連音的格子上點。swing 旋鈕之後再做。
- **6/8 等複拍子**：一拍分 3 格（`K2` TRIPLETS）時，拍號寫成複拍子：1 拍是 3/8、2 拍是 6/8、3 拍是 9/8、4 拍是 12/8。這時一拍是附點四分，**BPM 也是附點四分的速度**：譜上寫「八分音符 = 120」的話，要設 40。
- 對節拍器來說，拍號的分母只決定兩件事：click 落在哪種音符上、BPM 指哪種音符。聲音上 6/8 只有兩種讀法，GRID 都已經做得到，不需要多一個「拍子單位」的設定：

    | 想要的 | GRID 怎麼設 | BPM 指的是 |
    |---|---|---|
    | 6/8 數 2 下（快的 6/8、搖擺感） | `K1` 2、`K2` TRIPLETS（畫面寫 `6/8`） | 附點四分 |
    | 6/8 數 6 下（慢的 6/8） | `K1` 6、`K2` QUARTERS，第 4 格按成 NORMAL 或 ACCENT | 八分（畫面會寫 `6/4`，聲音一樣） |
    | 7/8（2+2+3） | `K1` 7、`K2` QUARTERS，第 1、3、5 格重音 | 八分 |

#### 拍號 preset（`K3`） { #presets }

不用懂「2/4 加 3 連音就是 6/8」：`K3` 一格換一種拍號，拍數、一拍幾格和重音一次設好。

| preset | 拍數 × 格 | 重音 | BPM 指的是 |
|---|---|---|---|
| 2/4、3/4、4/4、5/4 | 2–5 × 1 | 第 1 拍 | 四分 |
| 6/8 | 2 × 3 | 第 1 拍，另一拍一般，其他 ghost | 附點四分 |
| 6/8 IN 6 | 6 × 1 | 1 重、4 一般、其他 ghost | 八分 |
| 9/8、12/8 | 3、4 × 3 | 同 6/8 | 附點四分 |
| 5/8 3+2 | 5 × 1 | 1 重、4 一般 | 八分 |
| 7/8 2+2+3 | 7 × 1 | 1 重、3 和 5 一般 | 八分 |

存檔裡沒有記錄「用的是哪個 preset」，畫面是比對目前的拍數、格數和每格的級數反推出來的：以八分為一拍的 preset 顯示它的名稱（`6/8 IN 6`、`7/8 2+2+3`），改過任何一格就變回一般的寫法（`6/4  QUARTERS`）。不在 preset 上的時候，`K3` 第一格會先跳到同樣大小的 preset，沒有就跳到 4/4。

例：4/4 八分音符，把第 2 拍的「and」設成重音、第 3 拍關掉，就是 `A g N A . g N g`。

### CLASSIC：rhythm watch 的版面 { #classic }

照經典 rhythm watch 那種「每種音符一個音量鈕」的用法 `[調研]`（本人小時候用的那台；只參考操作方式，畫面是自己畫的）：

- `K1`–`K4` 是四分、8 分、3 連音、16 分的音量（0–7），可以混在一起，例如四分加小聲的 16 分。
- 四分音符在拍點上響；其他三層在拍與拍之間響。
- BEAT（`OCT-` / `OCT+`）是每幾拍一個重音，0 是不加重音。
- 畫面是七段顯示器風格的 TEMPO 與 BEAT，右邊兩顆燈：紅色是重音，綠色是一般的拍。

## 畫面 { #screens }

host simulator 的截圖 `[模擬]`：

| SET（播放中） | SET: EDIT | 換歌等下一小節 |
|---|---|---|
| ![SET](../assets/metronome/set_playing.png) | ![EDIT](../assets/metronome/set_edit.png) | ![等下一小節](../assets/metronome/set_pending.png) |
| **GRID**（4/4 八分） | **CLASSIC** | **速度術語**（`K2` 跳到 MODERATO） |
| ![GRID](../assets/metronome/grid.png) | ![CLASSIC](../assets/metronome/classic.png) | ![速度術語](../assets/metronome/set_marking.png) |
| **GRID 6/8** | **`K3`：6/8 IN 6** | **`K3`：7/8 2+2+3** |
| ![6/8](../assets/metronome/grid_6_8.png) | ![6/8 IN 6](../assets/metronome/grid_6_8_in_6.png) | ![7/8](../assets/metronome/grid_7_8.png) |
| **全部清空：問 PLAY** | | |
| ![CLEAR ALL?](../assets/metronome/set_clear_ask.png) | | |

## 時間怎麼算 { #timing }

和 [RHYTHM](rhythm.md#timing) 一樣，以 audio 的 sample 數為時鐘，每個 click 落在它自己的 sample 上，不是下一個 block。一拍是 12 個 tick，四分、8 分、3 連音、16 分都剛好落在 tick 上。

播放中的改動交給 render 在某個 tick 切換，所以拍子不會斷：

| 改動 | 什麼時候生效 | 拍子 |
|---|---|---|
| 換歌（NEXT BAR） | 目前這首的下一個小節線 | 新歌從第一拍開始 |
| 換歌（NOW）、EDIT 裡選別首 | 下一個 tick | 新歌從第一拍開始 |
| BPM、step、拍號、音量、音色 | 下一個 tick | 接著原本的拍子數下去 |

## 存檔 { #storage }

一筆 record（`rec_t`，460 B）：magic、版本、目前的頁、正在用的歌、換歌時機，以及 16 首歌。每首歌 28 B：

| 欄位 | 內容 |
|---|---|
| `used` | 有沒有存（鍵會不會亮） |
| `mode` | GRID 或 CLASSIC |
| `bpm` | 30–300 |
| `bpb`、`div` | GRID 的拍數 1–9、一拍幾格 1 / 2 / 3 / 4（乘起來最多 16） |
| `beat` | CLASSIC 的重音間隔 0–9 |
| `sound` | CLICK、WOOD、BEEP |
| `mix[4]` | CLASSIC 四層的音量 0–7 |
| `steps[16]` | GRID 每格的級數 0–3 |

讀回來的 record 不合法（版本不同、數值超出範圍）就用預設值：只有第 1 首，120 BPM、4/4。

## 程式結構 { #code }

| 檔案 | 內容 |
|---|---|
| `metro.c` / `metro.h` | 平台無關：每個 tick 響什麼（GRID 的級數、CLASSIC 的四層）、預設節奏型、record 檢查、tap tempo。`just metro-unit` 在電腦上測 |
| `app.c` | 頁面、輸入、LED（`firmware/common/ledfx` 的 GLOW / MID / LIT 三級）、render、畫面、存檔 |
| `tests/basic.txt` | host simulator 的劇本，`just fw-test metronome` |

## v0.1 驗證 { #v01-verify }

| 項目 | 結果 |
|---|---|
| `just metro-unit` | 1,430 個檢查全過：每種拍號與每拍格數（1–9 拍 × 1–4 格，最多 16 格）每個 step 都在自己的 tick 上、一小節一個重音；3 連音落在 tick 0、4、8；CLASSIC 的四層與 BEAT；不合法的 record；tap tempo 的平均與上下限 |
| `just fw-test metronome` | 全部 `ok`：4/4 一小節一個重音三個一般拍（有聲音，peak -17.4 dBFS）；EDIT 裡在第 2 鍵 tap 4 下得 100 BPM；第 3、4 首用 `SELECT` 與 `OCT-` 設；按住 `SEL` 清掉第 4 首；播放中按第 3 首，**下一小節**才從 120 換到 140；按空的鍵不會換；播放中轉 `SELECT` 立刻跟上；GRID 改成 8 分並改兩格後，一小節的重音、一般、ghost 數量都對；CLASSIC 的 BEAT 3 與 8 分音符層；黑鍵 tap tempo；重開機後 16 首歌、頁面與設定都還在 |
| `just fw-build metronome` | app 86,976 B；`.ram_text` 5,100 / 32,768 B；`.data+.bss` 25,976 / 98,304 B；沒有 soft-double，檢查全部 `ok`；package 616,003 B，identity `FM-1_5900` |
| fm1-emu `uc25hq80`、`p25q80` | 2026-11-20 在 Mac 上 headless 跑 PR #10 的 preview package（SHA-256 `698e08b1…bec75c6fd`）：空白 flash 開機 0 次寫入；`FM1_BENCH_KEYS` 腳本 REC、在 `D4`（第 6 首）tap 4 下、REC，存檔寫 4 次、只改 `0xCE000`；重開 0 次寫入，讀回第 6 首 100 BPM `[模擬]` |
| 疊在備份上預演 | 2026-11-20，備份 `pre-metronome-0.1`（THEORY 0.3，SHA-256 `b09767c3…d119b0d0`），兩種晶片：開機 0 次寫入，進 SET、第 1 首 120 BPM；存檔只改 `0xCE000`，`0xD6000`、`0xD8000`、`0xDA000` 不變 `[模擬]` |
| 實機 | 2026-11-20 本人用 `fm1_install.py` 從 THEORY 0.3 更新，`--info` 回報 `FM-1_5900 [running]`；本人試用：「感覺效果不錯」`[實機]` |

同一個 commit 在 Mac 上本地 build 的 package 和 preview 大小相同，差 43 bytes：檔頭的 CRC 和 `FM1B_BUILD`（fm1-base 的 `build_id()` 把 app 目錄下所有原始碼一起 hash，工作目錄多一個或少一個檔案就會不同）。比對兩份 package 時這是正常的。

前三列是 2026-11-19 在雲端 Linux 容器跑的。device build 用的 SDK 三個檔案是從 JAMTRACK 0.2.3 的 package 取出、核對過 SHA-256，見[開發環境](index.md#sdk-from-package)。

## v0.2 { #v02 }

2026-11-20，本人試用 v0.1 之後的回饋：

| 回饋 | v0.2 |
|---|---|
| 想看古典速度術語 | BPM 旁顯示術語，SET 的 `K2` 跳段，見[速度術語](#markings) |
| SET 上拍號寫法不一致（`4/4 QUARTERS` 與 `CLASSIC BEAT 5`） | 兩種模式用同一種格式。GRID 與 CLASSIC 各自的拍數設計保留（本人決定：GRID 的 `K1`/`K2` 已經能表達各種拍子），GRID 頁尾補上 `K1: BEATS  K2: DIV` |
| 要不要 6/8 | 3 連音寫成複拍子；GRID 的 `K3` 拍號 preset（6/8、6/8 IN 6、9/8、12/8、5/8、7/8 …），不懂樂理也設得出來。分母對節拍器只是標示，見 [GRID](#grid) |
| 歌名放很後面，要能演完一次清空 | [全部清空](#clear-all) |
| `OCT` 原本的 ±5 | 保留；`SELECT` 快轉也有 ±5 |
| 防誤觸 | SET 播放中、不在 EDIT 時，黑鍵不 tap tempo（只顯示提示、不出聲）。EDIT 裡或停止時照舊 |

record 的格式沒變（`REC_VER 1`），v0.1 存的歌直接沿用。

### v0.2 驗證 { #v02-verify }

| 項目 | 結果 |
|---|---|
| `just metro-unit` | 2,319 個檢查全過：多了每個拍號 preset 都合法、比對得回自己、改過一格就不算 preset、12/8 一小節 1 重 3 一般 8 ghost；每個 BPM 剛好屬於一段、段的邊界、典型速度在段內、`K2` 往上往下的走法與兩端 |
| `just fw-test metronome` | 全部 `ok`：`K2` 從 120 走 132 → 184 → 132 → 112；`SELECT` 慢轉 ±1、20 ms 一格 ±5；GRID 2 拍 × 3 格顯示 6/8；`K3` 從 6/8 走到 6/8 IN 6、7/8，往下到 2/4，改過一格後從同大小的 preset 開始；全部清空：按住 `SEL` 時按了鍵不會問、按 `OCT+` 取消、5 秒取消、`PLAY` 清完剩第 1 首 120 BPM 而且沒在播，重開機後還是；SET 播放中按黑鍵 3 下 BPM 不變 |
| `just fw-build metronome` | app 88,744 B；`.data+.bss` 25,992 / 98,304 B；沒有 soft-double；package 616,003 B，identity `FM-1_5900` |
| fm1-emu `uc25hq80`、`p25q80` | headless：空白 flash 開機 0 次寫入；REC、`D4` tap 4 下、REC，存檔寫 4 次、只改 `0xCE000`；重開 0 次寫入、讀回。把 v0.2 疊在 v0.1 存過歌的模擬 flash 上：開機 0 次寫入，第 6 首 100 BPM 還在，顯示 `ANDANTE` `[模擬]` |
| 疊在實機備份上預演 | 2026-11-20，備份 `pre-metronome-0.2`（METRONOME 0.1 用過一陣子，SHA-256 `d0e71df6…252ce2071`），兩種晶片：開機 0 次寫入，本人存的狀態原樣（CLASSIC 頁、第 1 首 76 BPM、BEAT 4，顯示 `ANDANTE`）；存一次檔只寫 `0xCF000`（METRONOME 的存檔區 `0xCE000`–`0xCFFFF` 兩個 sector 輪流寫），THEORY 的 `0xD6000`、`0xD8000` 與 Morse 的 `0xDA000` 不變 `[模擬]` |

## 之後 { #next }

- 畫面主題：擺錘之外，指揮棒的拍子圖形、可愛的角色。先量一張圖要多少 flash 再決定（[CORD](https://github.com/chasedeanda/cord-fm1) 證明放得下圖）。
- 更多音色、swing。
- 匯出／匯入歌單：照 [Morse](morse.md) 的備份做法，歌單存成 JSON。之後可以做一個網頁，在電腦上排好歌單再匯入機器。
- 每首歌的名字（要先想好怎麼輸入文字）：本人 2026-11-20 說放到很後面。曲目順序表演者自己記得，演完用[全部清空](#clear-all)。
- 實機上校正延遲：tap tempo 只看按鍵間隔，不受延遲影響；要比較 click 與 LED 有沒有對齊。
- 和 [RHYTHM](rhythm.md) 共用 click 與 transport（抽到 `firmware/common/`）。
