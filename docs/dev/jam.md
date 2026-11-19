# 伴奏機與 live 鼓機（探索）

想要的是「挑一個調、挑一個節奏型就開始玩」，而且像 Yamaha PSR 那樣，可以說「這一段結尾給我一個過門」。這一頁整理市售產品怎麼做這件事，以及在 FM-1 上自己做的兩個方向：完整的伴奏機（JAM），或只做鼓的 live 鼓機。FM-1 上已經有的韌體在[伴奏與 live 鼓機類韌體](../firmware/jam.md)。

**狀態（2026-11-19）：** 只是探索，沒有程式，也沒有排進度。試過 JAMTRACK、Polly、CORD 之後，本人決定自己的 JAM **先放著**：現成的已經能玩，差的部分還不值得現在做。這一頁留著，等哪天想做時從這裡開始。

## 市售產品怎麼做 { #products }

除了標 `[上游]` 的以外，下面的資料來自第三方轉載的使用手冊、評測與零售商頁面 `[調研]`，沒有對照原廠 PDF。各型號的細節不一樣。

### Arranger keyboard：Yamaha PSR / Genos

- 一個 style 有 `MAIN A`–`D` 四種變化、四個 fill-in、一個 `BREAK`，另外有 `INTRO` 與 `ENDING`。這是 [PSR-SX900 / SX700 手冊](https://www.manualowl.com/m/Yamaha/PSR-SX900/Manual/521161?page=59)寫的。
- `AUTO FILL IN` 開著時，演奏中按 `MAIN A`–`D` 任一個，先放過門再進那一段；`BREAK` 是一小節，放完回 MAIN。
- 據 [Tyros 的手冊](https://www.manualslib.com/manual/196986/Yamaha-Tyros.html?page=34)，按下現在這一段的 MAIN 鈕，會放一個過門再回到同一段。
- `INTRO` 只放一次，再交給你選好的 MAIN；`ENDING` 放一次就停。
- `SYNC START`：左手在伴奏區一按就開始。`ACMP`：左手的和弦被辨識出來，伴奏跟著轉。
- 小一點的型號（PSR-E 系列）只有一個 `MAIN/AUTO FILL` 鈕，在 A、B 之間切換。新一點的機種還有 Half Bar Fill-in 選項。這兩點只見於[論壇](https://forum.psrtutorial.com/index.php?msg=529781)。
- 舊型號的說明：[PSR-1100](https://www.manualsdir.com/manuals/230092/yamaha-1100.html?page=69)、[PSR-E433](https://www.manualowl.com/m/Yamaha/PSR-E433/Manual/288825?page=41)、[PSR-A300](https://www.manualowl.com/m/Yamaha/PSR-A300/Manual/152648?page=54)。

所以「在這段結尾放過門」的做法是：在想換段的那一小節按一下 MAIN。過門的長度是一小節（或半小節），**什麼時候放由你按的時間決定**，機器不知道你的段落有幾小節。Korg Pa、Roland E-X / BK 系列也是同樣的架構，這裡沒有逐一查。

### 口袋 arranger：Yamaha QY70

[Polly](https://github.com/charlesvestal/fm1-polly) 就是照 QY70 做的：style 加上 Groove A / B、Intro / Fill / Ending，一首歌由多個段落組成，每段有自己的和弦進行 `[上游]`。

### 只有鼓：Singular Sound BeatBuddy

吉他手腳踩的鼓機，整套操作只靠一個踏板 `[調研]`（[BeatBuddy 使用手冊](https://manuals.online/singular-sound/drum/beatbuddy/user-manual/22trz)、[Sound On Sound 評測](https://www.soundonsound.com/reviews/singular-sound-beatbuddy)）：

| 操作 | 結果 |
|---|---|
| 踩一下（停止中） | 放 intro（可以關掉），接著進第一段的主節奏 |
| 演奏中踩一下 | 過門，一小節以內；一段最多可以有 8 個 |
| 踩住 | transition：可以好幾小節，一直循環到放開，然後在這小節結束時進下一段 |
| 踩兩下 | outro，放完停 |

一首「歌」由幾段組成，每段有主節奏、過門、transition。這正好是 FM-1 上 live 鼓機想要的樣子，而且一個按鈕就夠。

### 只有鼓：Boss RC-10R 與 DR-01S

- **RC-10R Rhythm Loop Station**：looper 加鼓機。每個 rhythm 有 Intro、Pattern 1、Pattern 1 Fill、Pattern 2、Pattern 2 Fill、Ending。踩一下先放 intro 再進 Pattern 1，踩住換到比較熱鬧的 Pattern 2，換的時候自動加過門，演奏中踩一下也能加。超過 280 個 rhythm、16 套鼓，可以匯入自己的 SMF。資料來自[Boss 的發表消息](https://sonicstate.com/amped/2019/07/15/boss-introduces-rc-10r-rhythm-loop-station/)與[評測](https://www.pressreader.com/australia/guitarist/20200403/283064121440481)。
- **DR-01S Rhythm Partner**：給木吉他手的節奏機，99 個 pattern。不能排 step，但可以選每個 pattern 用哪些樂器、每個樂器用哪個 variation；有 count-in 和 fade-out。過門的資料沒查到。見[Roland 的發表消息](https://www.roland.com/us/company/press_releases/2017/BOSS-INTRODUCES-DR-01S-RHYTHM-PARTNER/)。

### 練習用的 app：iReal Pro

和弦譜加上自動伴奏，有約 50 種 style（部分要另外買）。練習功能有：段落循環、任意移調、一直加速、自動繞調，每個和弦還會建議可以用的音階。見 [App Store 頁面](https://apps.apple.com/gb/app/ireal-pro/id298206806)。它是「練習 jam」的參考，JAMTRACK 的鍵燈提示走的也是同一個方向。

### 對照：MODX

本人用 MODX 頂多做到「8 小節後換一個節奏型」，沒有過門這一層。

### 共通的地方

| | PSR | QY70 / Polly | BeatBuddy | RC-10R | JAMTRACK |
|---|---|---|---|---|---|
| 變化的數量 | MAIN A–D | Groove A / B | 每首歌好幾段 | Pattern 1 / 2 | A / B |
| 過門什麼時候放 | 按下時，放到小節尾 | 下一小節 | 按下時，一小節以內 | 換段時自動，或按下時 | 這一輪結尾 |
| 前奏、尾奏 | 有 | 有 | 有 | 有 | 沒有 |
| 和弦 | 左手辨識 | 和弦進行或辨識 | 不需要 | 不需要 | 寫好的級數 |

沒有一個產品是「我說這段 8 小節，你自己在第 8 小節放過門」。PSR 和 BeatBuddy 都是讓你在對的時間按；RC-10R 和 JAMTRACK 是換段時自動放。

## 方向 A：完整的伴奏機 JAM { #arranger }

鼓、bass、和弦伴奏，加上你的 solo。和 PSR 一樣有 `MAIN A`–`D`、`INTRO`、`ENDING`、`BREAK`；一首歌由幾段組成，每段有自己的和弦進行。

- **歌**：段落的順序，每段有自己的和弦進行（羅馬數字，交給 THEORY 的 `th_prog_parse`）、長度、用哪一個 MAIN。
- **Style**：每個 MAIN 的鼓、bass、和弦伴奏 pattern，以及每個 MAIN 的 fill、intro、ending。伴奏 pattern 寫「和弦的第幾個音」，播放時依現在的和弦算出音高，再用 `th_voice_near` 讓和弦移動最少。這和 PSR 用 C 大調寫 style 的道理一樣。
- **過門**：`FILL` 在這一段的最後一小節放；在最後一小節才按，就從按下的那一拍放到小節尾；按住是立刻放、不換段。換 MAIN 時可以設定在這一小節尾換或段尾換。
- **提示**：琴鍵燈顯示和弦音（亮）與音階（暗），下一個和弦的根音閃，這兩點分別來自 JAMTRACK 與 CORD。
- **之後**：`SYNC START`、分割琴鍵做和弦辨識（THEORY 的 `th_identify`，像 Polly）、MIDI 輸出。

文字格式的草稿：

```text
song    "Ballad in C"  key=C  bpm=84  style=pop8
intro   I  V/vi  vi  IV                 bars=4  main=A
verse   I  V  vi  iii  IV  I  IV  V     bars=8  main=A  x2
chorus  IV  V  iii  vi  IV  V  I  I     bars=8  main=C  x2  fill=end
ending  IV  V  I                        bars=4
```

工作量最大的是聲音（鼓、bass、和弦、solo 四組 voice 都要自己寫，fm1-base 沒有 synth）與內容（每個 style 要寫四組 MAIN 加過門）。現成的 JAMTRACK 與 Polly 已經涵蓋大部分，這是先放著的原因。THEORY 原本規劃的 GROOVE 也併到這裡。

## 方向 B：live 鼓機 { #drums }

只做鼓。比完整的伴奏機簡單很多，又剛好補上現有韌體的空缺：FM-1 上的鼓機大多是 step sequencer，偏 live 的 RHYTM-SEC-1 用的是產生出來的 pattern，沒有固定的段落（見[韌體頁](../firmware/jam.md#drums)）。專用的話，CPU 與程式空間都可以給鼓，操作也直覺：選曲風、選鼓組、按 `PLAY`。輸出接 mixer，任何樂器都能一起玩。

主要的想法：

- 一個按鈕照 BeatBuddy 的方式用：按一下過門、按住換段、按兩下結束。
- 新的一點：過門可以設定成等到 4 / 8 / 16 小節樂句的最後一小節才放。

完整設計在 [Live 鼓機（設計）](drums.md)（2026-11-19）。

## 如果哪天要做 { #later }

- 先從**方向 B** 開始（[設計](drums.md)）：範圍小、補的是現有韌體的空缺、用得到的機會也多。之後若要加 bass 與和弦，就往方向 A 長。
- 授權：用到 THEORY 或 RHYTHM 的程式就是 GPL-3.0-or-later。
- 識別碼：2026-11-19 對照 Hub 的 catalog，`FM-1_58`、`FM-1_59` 沒人用（`FM-1_56`、`FM-1_57` 都有人用了）。之後 `FM-1_58` 給了 DRUMS、`FM-1_59` 給了 [METRONOME](metronome.md)，動工時要另找。
- 存檔：`0xD0000` 是自製 app 還沒用的下一格（上面是 RHYTHM 的 `0xD2000`）。刷機前用 `just emu-rehearse` 確認是空的。
- 流程照[開發環境](index.md)：host simulator、fm1-emu 兩種晶片、疊在最新備份上預演，最後才問要不要刷。
