# Live 鼓機（設計）

把 FM-1 變成一個可以帶著走的鼓手：選曲風、選鼓組、按 `PLAY` 就有 groove。想換段時，它在**樂句的最後一小節**自己放過門。3.5 mm 輸出接 mixer，不管旁邊是 MODX、吉他還是任何樂器都能一起玩。它和 Yamaha PSR、BeatBuddy、Boss RC-10R 的對照，以及為什麼先做鼓而不是整個伴奏機，見[伴奏機與 live 鼓機（探索）](jam.md)。

**狀態（2026-11-19）：** 只有設計，沒有程式，也沒有排進度。

- 獨立的韌體，不是 [RHYTHM](rhythm.md) 的一頁（本人 2026-11-19 決定）。預定放在 `firmware/drums/`，名稱暫定 DRUMS，正式名稱發布前對照 Hub 再定。
- 授權 GPL-3.0-or-later，因為要搬 RHYTHM 的 transport。
- 識別碼暫定 `FM-1_58`：2026-11-19 對照 Hub 的 catalog 沒人用。
- 存檔在 `0xD0000`（`APP_STORE_MAP {0xDC000, 0xD0000}`），上面一格 `0xD2000` 是 RHYTHM。刷機前用 `just emu-rehearse` 確認這一格在最新的備份裡是空的。

## 使用情境 { #uses }

| 情境 | 需要什麼 |
|---|---|
| 接 mixer，和任何樂器一起玩 | 3.5 mm 立體聲輸出（見 [I/O](../hardware/io.md)）、每一軌的音量與左右 |
| 和 MODX 或鍵盤 jam | 跟著外部的 MIDI clock 跑，或自己送 clock |
| 練吉他 | 雙手不用離開吉他：song mode 自己換段、自己放過門；也可以用 MIDI 踏板觸發 |
| FM-1 自己玩 | 琴鍵當 finger drum pad 現場加打 |

## 一個 style 裡有什麼 { #model }

綜合 PSR、RC-10R 與 BeatBuddy：

| 段落 | 說明 |
|---|---|
| `INTRO` | 1–2 小節，可以關掉 |
| `MAIN A`–`D` | 同一個曲風的四種變化，通常越後面越熱鬧 |
| `FILL` | 每個 MAIN 一個專屬的過門，另外還有幾個共用的（短、長、只有 tom） |
| `TRANSITION` | 按住時一直循環的過渡段（BeatBuddy） |
| `BREAK` | 一小節的停頓，只留 crash 或什麼都不留 |
| `ENDING` | 放完就停 |

- 鼓組和 style 分開選：任何節奏型都可以配任何鼓組。
- **Song（v0.3）**：幾段排在一起，每段有小節數，例如 `intro 2 | A 8 | B 8 | A 8 | ending`。每段的最後一小節自動放過門，練吉他時不用動手。

## 時間規則 { #timing }

transport 搬 RHYTHM 的 `firmware/rhythm/app.c`：`tr`、`tick_at`、`q8_mul`、`beat_q8_at`，以及 `app_render` 裡逐 sample 的排程。時鐘就是音訊的 sample 數，不會飄。差別在每拍改成 **24 tick**（RHYTHM 是 12）：32 分音符的 flam 放得下，MIDI clock 的 24 PPQN 也剛好一個 tick 對一個 pulse。

**過門什麼時候放**，由設定 `FILL AT` 決定：

| `FILL AT` | 按下過門之後 | 參考 |
|---|---|---|
| `NOW` | 從下一個 16 分音符放到這小節結束 | PSR |
| `BAR` | 下一小節整小節放 | Polly、RHYTM-SEC-1 |
| `PHRASE` | 等到這個樂句（4 / 8 / 16 小節）的最後一小節才放 | **沒有產品這樣做** |

其他規則：

- 按住（`TRANSITION`）：放 transition 並一直循環，放開後在這小節結束時換到下一個 MAIN（BeatBuddy）。
- 換 MAIN：在小節尾或樂句尾換，看設定；`AUTO FILL` 開著就先放過門。
- 所有要求都先排隊，到邊界才生效；後來的要求蓋掉前面的。
- 過門一律寫成一小節。從小節中間開始時只放剩下的部分。

## 操作 { #controls }

照實際的面板排（面板圖見 [Conductor 速查](../agentic/conductor-cheatsheet.md)）：左邊是 `SELECT`、`PRESETS`、`ALGORITHM` 與 `OCT-` / `OCT+`；右邊是 `K1`–`K4`，以及兩排按鈕 `FX SEL ENV LFO EDIT GLO` 和 `HOME SAVE ARP SEQ PLAY REC`。

| 控制 | 作用 |
|---|---|
| `PLAY` | 停止中：先放 intro（有開的話），再進 `MAIN A`。播放中：放 ending 後停。按住：立刻停 |
| `REC` | 停止中：tap tempo（RHYTHM 的 `tap_tempo`）。播放中是「BeatBuddy 鈕」：按一下過門、按住 transition、按兩下 ending |
| `ALGORITHM` | 曲風分類 |
| `SELECT` | 分類裡的 style |
| `PRESETS` | 鼓組 |
| `K1`–`K4` | BPM、swing、energy（ghost note 與 hi-hat 密度）、humanize |
| `OCT-` / `OCT+` | BPM ±1 |
| `ARP` | `AUTO FILL` 開關 |
| `SEQ` | `FILL AT` 與樂句長度 |
| `FX`（按住） | build-up：濾波往上掃並拿掉 kick，放開時回來 |
| `SEL` + 右半邊的琴鍵 | 那一軌靜音 / 取消，做 drop 用 |
| `HOME` | 換頁：PLAY → MIX（每一軌的音量與左右）→ KIT（音高、長度）→ SONG → SETTINGS |
| `SAVE` | 存自己的 song；設定停手 3 秒後自動存（RHYTHM 的做法） |

**琴鍵（F3–G5，27 個）：**

| 區域 | 白鍵 | 黑鍵 |
|---|---|---|
| 左半 F3–E4（演奏控制） | `MAIN A`、`B`、`C`、`D`、`BREAK`、`INTRO`、`ENDING` | 過門 1–3、`TRANSITION`、crash 收在拍上（stop-time） |
| 右半 F4–G5（15 個 pad） | kick、snare、rim、clap、closed hat、open hat、ride、crash、三個 tom、cowbell、tambourine、shaker 等 | |

右半邊的 pad 可以設定要不要對齊拍子。力度要用 fm1-base 的 `APP_TOUCH`，但它的時間精度還沒量過；RHYTHM 量拍子用的是 1 kHz tick 裡的 `plat_keys`。

**外接踏板：** 「BeatBuddy 鈕」的三個手勢也可以從 MIDI CC 觸發。踏板從 TRS MIDI IN 或 Bluetooth MIDI 進來，練吉他時用腳踩。

**燈（`firmware/common/ledfx`）：** 現在的 MAIN 亮著；排隊中的 MAIN 或過門閃；每個 pad 在它那一軌發聲時亮一下；播放中 `PLAY` 亮綠燈。

## 聲音 { #sound }

### 合成鼓組（先做）

- kick：正弦波往下掃頻。
- snare：噪音加一個音高。
- hi-hat 與鈸：濾過的噪音或 FM 金屬聲。
- 另有 clap、tom、rim、cowbell。
- 鼓組：808、909、electro、lo-fi。
- 約 10–12 個 voice；open hat 會被 closed hat 切掉（choke group）。
- 每一軌有自己的音量與左右，立體聲輸出。
- 低音補償（給 FM-1 的小喇叭用）可以關掉，接 mixer 時不需要。

程式小，授權單純，沒有素材來源的問題。

### Sample 鼓組（評估過再決定）

用 `firmware/common/adpcm` 的 IMA ADPCM 解碼（4 bit）。粗算可以放幾套：

| 項目 | 數字 |
|---|---|
| app slot | 581,564 B（[語音合成](tts.md)） |
| 程式本身 | 約 100–150 KB（RHYTHM v0.2 是 104 KB） |
| 可以給 sample 的空間 | 約 400 KB |
| 4 bit、22.05 kHz | 11.2 KB / 秒 |
| 一套 12 件的鼓組 | 約 5 秒，約 56 KB |
| 可以放 | **5–6 套** |

- 22.05 kHz 只到 11 kHz，鈸會悶。所以做成混合的：kick、snare、tom 用 sample，hi-hat 與鈸用合成；或鈸改用 32 kHz。
- sample 只能放在程式裡（const 陣列），不能放在 flash 的資料區（[AGENTS.md](https://github.com/daviddwlee84/M-VAVE_FM-1/blob/main/AGENTS.md) 不准自己算 flash 位址）。
- 要先做一個像 [voicelab](tts.md) 那樣的小實驗：放一套鼓組進去，在 fm1-emu 用 `--bench` 量程式大小與 CPU，再決定。
- 素材：要找 CC0 的 sample 包，或自己錄。

## 節奏型 { #content }

- 用文字格線寫，build 時經 fm1-base 的 `gen = [...]` 轉成 C 表。
- 一個事件 2 byte（tick、哪一軌、力度等級）。一個 style 約 18 小節、約 1 KB，50 個 style 約 50 KB。
- 自己寫，或從 Google Magenta 的 Groove MIDI Dataset 取。它是 CC BY 4.0（Polly 用的就是它），要標出處。據說資料裡有分 beat 與 fill，還沒查證 `[調研]`。
- 曲風清單（草稿）：rock 8beat / 16beat、pop、ballad、funk、shuffle / blues、swing / jazz、bossa nova、samba、reggae、hip-hop、lo-fi、house、drum & bass、6/8、3/4 waltz、5/4、7/8。

格式草稿（`.` 是空格，數字是力度等級；每拍 4 格，實際存成 24 tick）：

```text
style   rock8   "Rock 8beat"   time=4/4   group=rock
main A  kick    9... .... 9.7. ....
main A  snare   .... 9... .... 9...
main A  hat     6.4. 6.4. 6.4. 6.4.
fill A  snare   .... .... 9797 9999
fill A  tom1    .... 9.9. .... ....
fill A  crash   ....                     # 接到下一小節的第一拍
```

## 同步與輸出 { #sync }

- **音訊**：3.5 mm 立體聲輸出到 mixer 或錄音介面。
- **跟 MODX 的 clock 跑**：MODX 背後有 5-pin 的 MIDI IN / OUT，經轉接頭接到 FM-1 的 TRS MIDI IN，FM-1 就能直接跟著 MODX，不用電腦。要把 MODX 的 MIDI I/O 設成 MIDI（不是 USB）。這來自論壇與第三方轉載的手冊，還沒對照 Yamaha 官方手冊 `[調研]`（[Yamaha Synth 論壇](https://yamahasynth.com/community/modx-series-synthesizers/midi-routing-usb-din5-midi-out)）。
- **FM-1 當主控**：送 0xF8（24 PPQN）與 start / stop。走 USB 或 Bluetooth，中間要有電腦或 USB host，因為 FM-1 沒有 MIDI OUT 接頭。
- **跟外部 clock 跑**：clock 可以從 USB、Bluetooth 或 TRS 進來，用 `now_smp()` 量間隔，算出每個 tick 的長度。
- **MIDI note 輸出**：每一軌照 General MIDI 的鼓譜送到 DAW（之後再做）。
- 這個 repo 裡還沒有任何韌體處理 MIDI clock。

## 程式結構 { #code }

| 檔案 | 內容 |
|---|---|
| `groove.c` / `groove.h` | 不碰平台：style、song、要求佇列、邊界判斷、pattern 展開成事件。在電腦上測（`just drums-unit`） |
| `kit.c` | 合成鼓聲 |
| `sample.c` | ADPCM 鼓組（實驗之後才決定要不要） |
| `app.c` / `app.h` | 畫面、按鈕、transport、存檔 |
| `content/*.txt` | 內建的 style 與鼓組參數 |

`groove.c` 要測的情況：

- 在一小節的每一個 16 分音符按過門，三種 `FILL AT` 各自落在哪裡。
- 按住與放開 transition 的時間點。
- 按一下和按兩下的分辨。
- 3/4、6/8，以及樂句長度 4 / 8 / 16。
- MAIN 和過門同時排隊時的先後。
- 外部 clock 有抖動時。

## 進度 { #roadmap }

| 版本 | 內容 |
|---|---|
| v0.1 | 兩套合成鼓組、約 10 個 style、`MAIN A` / `B`、過門（`NOW` / `BAR` / `PHRASE`）、tap tempo、`REC` 的三個手勢、右半邊的 pad |
| v0.2 | `MAIN C` / `D`、`INTRO`、`ENDING`、`BREAK`、`TRANSITION`、`AUTO FILL`、MIX 頁、sample 實驗 |
| v0.3 | song mode、MIDI clock 進出、外接踏板的 CC |
| v0.4 | 更多鼓組與 style；練習功能（靜音小節、慢慢加速），這部分和 RHYTHM 重疊 |

每一版照[開發環境](index.md)的流程：host simulator 的 `fw-test`；fm1-emu 兩種晶片都要做存檔、重開、讀回；疊在最新備份上預演；最後才問要不要刷。

## 還沒決定的事 { #open }

- 正式名稱。
- 做完 sample 實驗後，要不要放 sample 鼓組。
- 琴鍵力度（`APP_TOUCH`）的時間精度夠不夠拿來打鼓。
- 練習功能放這裡還是放 RHYTHM。
