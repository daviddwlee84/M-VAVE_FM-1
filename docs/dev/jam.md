# 伴奏機 JAM（構想）

把 FM-1 變成口袋版的 arranger：選一個調、一個 style，按 `PLAY` 就有鼓、bass、和弦，你在上面 solo。和 Yamaha PSR 一樣，style 有 `MAIN A`–`D` 四種變化，可以有前奏、尾奏，也可以叫它**在這一段的最後一小節放過門**再換段。一首歌由幾個段落組成，每段有自己的和弦進行。

現成韌體的比較在[伴奏類韌體](../firmware/jam.md)。那裡的結論是：JAMTRACK 有 solo 音色和「這一輪結尾的過門」，Polly 有段落、前奏與尾奏，但兩個都沒有 MAIN A–D，也沒有「這一段第幾小節放過門」。這一頁是補上這些的設計。

**狀態（2026-11-19）：** 構想，還沒有程式。

- 韌體預定放在 `firmware/jam/`，名稱暫定 JAM。Hub 上已經有 JAMTRACK，正式名稱發布前再定。
- 授權 GPL-3.0-or-later，因為要用 [THEORY](theory.md) 的 `theory.c`、`synth.c` 與 [RHYTHM](rhythm.md) 的 `rhythm.c`。
- 識別碼暫定 `FM-1_58`：2026-11-19 對照 Hub 的 catalog 沒人用，也沒有別人的 pattern 吃到它（`FM-1_56`、`FM-1_57` 都有人用）。發布前再查一次。
- 設定與自己的歌存在 `0xD0000`（`APP_STORE_MAP {0xDC000, 0xD0000}`）。上面一格 `0xD2000` 是 RHYTHM，`0xD4000` 是 GUITAR。這一格在最新的備份裡是不是空的，**刷機前要用 `just emu-rehearse` 確認**。
- THEORY 原本排在 v0.6 的 GROOVE（FM-1 彈 bass 和鼓，你彈和弦）改由 JAM 來做。THEORY 的[和弦進行目錄](theory.md#content)兩邊共用。

## 用起來是什麼樣子 { #play }

1. 開機停在 PLAY 頁：畫面中間是現在的和弦，下面是這一段的和弦進行與 playhead，角落是 style、MAIN、BPM、調。
2. `SELECT` 選 style，按住 `SEL` 再按一個琴鍵選調，`PRESET` 選一首內建的歌（或「只有一個進行」的 jam）。
3. `PLAY`：有設 intro 就先放前奏，然後進 `MAIN A`。
4. 琴鍵全部拿來 solo；`ALGO` 換 solo 音色，`OCT-` / `OCT+` 移八度。琴鍵燈顯示這個和弦的和弦音（亮）與音階（暗）。
5. 按 `FILL`：**這一段的最後一小節**放過門，然後進下一段。快到段尾才按，就從按下的那一拍放到小節結束（PSR 的做法）。
6. 按 `MAIN B`：同樣在段尾（或小節尾，看設定）放過門，然後換成 B。
7. 再按 `PLAY`：放尾奏，放完停。

### 段落與過門的時間規則 { #timing }

播放的位置用 RHYTHM 的 transport 計：每拍 12 tick（`RH_TPB`），樣本精確。所有「要求」都先記下來，到下一個邊界才生效：

| 要求 | 什麼時候生效 | 過門 |
|---|---|---|
| `FILL`（段尾） | 這一段這一輪的最後一小節 | 放這個 MAIN 的 fill，接著下一段 |
| `FILL`（在最後一小節裡按） | 立刻，從現在這一拍到小節結束 | 同上 |
| 按住 `FILL` | 立刻，到小節結束，**不換段** | 放完回原本的 MAIN |
| `MAIN X` | 設定 `SWITCH`：`BAR`（PSR 式，下一小節換）或 `SECTION`（段尾換） | `AUTO FILL` 開的話先放 fill |
| `NEXT`（跳下一段） | 段尾 | 依 `AUTO FILL` |
| `ENDING` | 下一小節 | 放尾奏，然後停 |
| `BREAK` | 下一小節，一小節的停頓 | 沒有 |

過門的 pattern 一律寫成一小節；從中間開始時，只放從那一拍到小節尾的部分。

## 歌、段落與 style { #model }

兩層資料，分開寫：

- **Song**：段落的順序，每段有自己的和弦進行、長度、用哪一個 MAIN。和弦用羅馬數字寫，直接交給 `th_prog_parse`，所以換調只是改 tonic。
- **Style**：每個 MAIN 的鼓、bass、和弦伴奏 pattern，加上每個 MAIN 的 fill、intro、ending。伴奏 pattern 不寫絕對音高，而是寫「和弦的第幾個音」（根音、三音、五音、七音、高八度、下一個和弦的經過音），播放時依現在的和弦算出音高，再用 `th_voice_near` 讓和弦移動最少。這和 PSR 用 C 大調寫 style、播放時再轉成實際和弦的道理一樣。

一首歌的文字格式（草稿；build 時經 fm1-base 的 `gen = [...]` 轉成 C 表）：

```text
song    "Ballad in C"  key=C  bpm=84  style=pop8
intro   I  V/vi  vi  IV                 bars=4  main=A
verse   I  V  vi  iii  IV  I  IV  V     bars=8  main=A  x2
chorus  IV  V  iii  vi  IV  V  I  I     bars=8  main=C  x2  fill=end
ending  IV  V  I                        bars=4
```

一個 style 的 pattern（草稿；`.` 是十六分音符的空格，數字是 velocity 等級）：

```text
style   pop8   "Pop 8beat"  time=4/4
main A  kick   9...  ....  9.5.  ....
main A  snare  ....  9...  ....  9...
main A  hat    5.3.  5.3.  5.3.  5.3.
main A  bass   R... ..R.  5... ....      # R 根音、5 五音
main A  comp   C... ..C.  .... C...      # C 整個和弦
fill A  snare  ....  ....  9.7.  9799
```

存檔裡只放自己改過的歌（段落表一首約幾十 byte，fm1-base 的一個 store 物件最多 3,840 byte）。內建的歌與 style 在程式裡。

## 聲音 { #voices }

fm1-base 沒有 synth，聲音都要自己在 `app_render` 裡算（44.1 kHz、每塊 256 frame）。分四組，各自有固定的 voice 數，不互相搶：

| 組 | 做法 | 來源 |
|---|---|---|
| 鼓 | kick（正弦掃頻）、snare（噪音加音高）、hat（濾過的噪音或 FM 金屬聲）、clap、tom、rim，共 6 voice | 新寫 `drum.c` |
| bass | 單音，FM 或鋸齒波加濾波 | 新寫，參考 `synth.c` |
| 和弦伴奏 | THEORY 的 2-operator FM 電鋼，8 voice | `firmware/theory/synth.c` |
| solo | 單音 lead（帶 glide）、方波、風琴、電鋼 | 新寫 `lead.c` |

注意：

- FM-1 的喇叭幾乎出不了低音。`synth.c` 已經替低音加了音量與亮度，bass 和 kick 也要一樣。用耳機或線路輸出時要能關掉。
- CPU 是兩核 360 MHz、單精度 FPU，要在 fm1-emu 用 `--bench` 量 ISR 佔多少，四組一起響的時候是最壞情況。
- 以後可以改用 sample 的鼓（JAMTRACK 是這樣做的），用 `firmware/common/adpcm` 解碼，要先看 code 空間夠不夠。

## 操作（草稿） { #controls }

琴鍵留給 solo，演奏的控制都在按鈕上。實際排法要在 emulator 的面板上對過按鈕位置再定。

| 控制 | PLAY 頁 |
|---|---|
| `PLAY` | 開始（有 intro 先放 intro）；播放中再按：尾奏後停 |
| `REC` | tap tempo；按住：`SYNC START` 開關（v0.3） |
| `SEQ` | 跳下一段（段尾） |
| `ARP` | `FILL`（見[時間規則](#timing)）；按住：立刻放 |
| `FX`、`ENV`、`LFO`、`EDIT` | `MAIN A`–`D` |
| `GLO` | `BREAK` |
| `SELECT` | style |
| `PRESET` | 內建的歌 / jam |
| `ALGO` | solo 音色 |
| `SEL` + 琴鍵 | 調（tonic） |
| `OCT-` / `OCT+` | solo 的八度 |
| `K1`–`K4` | BPM、伴奏與 solo 的音量比、swing、伴奏密度 |
| `HOME` | 換頁：PLAY → SONG（編段落）→ SETTINGS |

SETTINGS：`AUTO FILL` 開關、`SWITCH`（`BAR` / `SECTION`）、低音補償、MIDI 輸出。

### 和弦辨識（v0.3） { #acmp }

PSR 的 ACMP：琴鍵分成兩半，低的 12 個鍵（F3–E4）彈和弦，THEORY 的 `th_identify` 認出是什麼和弦，伴奏跟著轉；高的 15 個鍵 solo。另外一個「一指」模式：只按根音是大三和弦，加按一個黑鍵是小三和弦。`SYNC START` 開著時，第一次在左半邊按下和弦就開始。

## 程式結構 { #code }

| 檔案 | 內容 |
|---|---|
| `arr.c` / `arr.h` | 不碰平台的 arranger：段落、要求佇列、邊界判斷、pattern 展開成音符事件。全部可以在電腦上測（`just jam-unit`） |
| `drum.c`、`lead.c`、bass | 聲音 |
| `app.c` / `app.h` | 畫面、按鈕、transport（從 `firmware/rhythm/app.c` 搬過來：`tick_at`、`q8_mul`、tap tempo）、存檔 |
| `content/*.txt` | 內建的歌與 style，build 時產生 C 表 |
| 共用 | `../theory/theory.c`、`../theory/synth.c`、`../rhythm/rhythm.c`、`../common/ledfx`（播放中的拍子燈、排隊中的 fill 閃燈） |

`arr.c` 的測試重點：任何時間點按下的 `FILL` 都落在正確的小節；`MAIN` 與 `FILL` 同時排隊時的先後；段落長度不是 4 的倍數；3/4 與 6/8；intro 只有一小節時按 `FILL`。

## 進度 { #roadmap }

| 版本 | 內容 |
|---|---|
| v0.1 | 一個調、幾個 style、內建 jam（只有一個進行）、`MAIN A` / `B`、`FILL`（段尾與立刻）、一個 solo 音色、琴鍵燈顯示和弦音 |
| v0.2 | 歌：段落各自的和弦進行、intro、ending、`MAIN C` / `D`、`AUTO FILL`、`BREAK`、SONG 頁編段落並存檔 |
| v0.3 | `SYNC START`、和弦辨識（分割琴鍵）、一指和弦 |
| v0.4 | MIDI 輸出：鼓、bass、和弦、solo 分 channel，送 clock，可以接 DAW；接 MODX 要電腦或 USB host 在中間，TRS 只有 MIDI IN（見 [I/O](../hardware/io.md)） |

每一版照 [開發環境](index.md) 的流程：host simulator 的 `fw-test`、fm1-emu 兩種晶片（含存檔、重開、讀回）、疊在最新備份上預演，最後才問要不要刷。

## 還沒決定的事 { #open }

- 正式名稱。
- 鼓用合成還是 sample。
- 按鈕的實際排法（上面是草稿）。
- 內建要放哪些歌：只放和弦進行（不放旋律）就沒有版權問題；THEORY 的進行目錄（王道進行、4536251、卡農、axis、andalusian）可以直接用。
