# ChoralRoot 教學：和弦樂器的玩法

ChoralRoot 仿的是 Telepathic Instruments 的 Orchid。它的操作邏輯和一般鍵盤不同，這頁照「先懂觀念、再一步步加東西」的順序走一遍。內容整理自[上游手冊](https://github.com/Quixotic7/ChoralRootFM1/blob/main/docs/MANUAL.md)（0.41）`[上游]`；我的機器已刷入 ChoralRoot，但這些步驟我還沒逐一在機器上確認。

!!! note "螢幕上說的鍵名和面板印的不一樣"
    ChoralRoot 的螢幕用它自己的鍵名。最常遇到的是 **`Hold OPT to change engine`：OPT 就是面板上印著 `GLO` 的那顆鍵。**

    | 螢幕 / 手冊說 | 面板上印的 |
    |---|---|
    | **OPT** | **`GLO`** |
    | KEY | `SEL` |
    | BASS | `ENV` |
    | LOCK | `LFO` |
    | PERF | `ARP` |
    | LOOP | `PLAY` |

    `FX`、`EDIT`、`HOME`、`SAVE`、`SEQ`、`REC` 名稱不變。完整對照在本頁最下面的「面板對照」。

    **為什麼要換名字**：面板上的字是為原廠韌體印的（`SEL`、`ENV`、`LFO`、`ARP` 是原廠合成器的功能），上游手冊直說 "The printed labels belong to the stock firmware"。ChoralRoot 的功能是另一套，所以用它自己的名稱，上游建議貼貼紙。`[上游]` 名稱本身（KEY、BASS、LOCK、PERF、LOOP）應該是沿用 Orchid 的功能名，這點是我的推測，沒有查證。另外 Options 裡有 **Classic** 配置，是 Orchid 原本的 chord 鍵位（SUS 獨立在 C#4、9 在 C4、LOCK 在 B3）；預設的是為 FM-1 鍵盤重排過的配置。

## 觀念：一隻手決定「什麼和弦」，另一隻手決定「哪個音」

一般鍵盤上，彈 Dm7 要同時按 D、F、A、C 四個鍵，換和弦就要換一組指型。Orchid 式的樂器把這件事拆開：

- **和弦性質**（大、小、減、掛留，加不加七音、九音）用一組固定的鍵決定。
- **根音**用另一組鍵彈，一次只要一根手指。
- 機器負責組出和弦、安排聲部，並把和弦名稱顯示在螢幕上。

好處是不需要會指型就能彈出完整的和弦進行，而且換調、換 voicing 都是轉一個旋鈕的事。它是用來找和弦、寫歌、做和聲鋪底的工具，不是用來彈旋律的鍵盤。（關於 Orchid 本身的這段是我的一般認識，沒有查證。）

## 鍵盤配置

27 個鍵，F3 到 G5。**左邊七個是 chord block，其餘 20 個是根音。**

```
黑鍵      DIM   MIN   MAJ         C#4   D#4         F#4  ...
          F#3   G#3   A#3
                \_ SUS _/   （G#3 + A#3 一起按）
白鍵    6     m7    M7    9     C4    D4    E4    F4  ...  G5
        F3    G3    A3    B3    |------- 根音：C4 到 G5 -------|
```

| 鍵 | 名稱 | 加在根音上面的音（半音數） |
|---|---|---|
| F#3 | DIM 減 | 3、6 |
| G#3 | MIN 小 | 3、7 |
| A#3 | MAJ 大 | 4、7 |
| G#3 + A#3 | SUS 掛留 | 5、7 |
| F3 | 6 | 再加 9 |
| G3 | m7（小七度） | 再加 10 |
| A3 | M7（大七度） | 再加 11 |
| B3 | 9 | 再加 14 |

兩條規則：

- **chord 鍵要在根音按下去之前就按著。** 預設的 Play Style 是 Simple，和弦在根音落下的那一刻就定了。
- **延伸音可以疊。** MAJ + 6 + 9 會顯示 `D` `6 9`。疊三個顯示 `JAZZ`，四個全按顯示 `WTF`。
- **同時按兩個種類鍵時，後按的那個算數。** 例外是 G#3 + A#3，永遠是 SUS。

### `JAZZ` 和 `WTF` 是什麼 `[上游]`

不算隱藏彩蛋，是 Orchid 原本的命名規則，ChoralRoot 照搬。它的預設命名（Options → Chord Names = **Orchid**）依「你按了哪些鍵」取名：延伸音一個寫成上標、兩個並排、**三個寫 `JAZZ`、四個寫 `WTF`**。和種類鍵按了幾個無關，只看 6、m7、M7、9 這四個延伸鍵按了幾個。

所以 chord block 七個全按（三個種類鍵 + 四個延伸鍵）一定是 `WTF`。按六個時，如果是三個種類鍵 + 三個延伸鍵就是 `JAZZ`，兩個種類鍵 + 四個延伸鍵還是 `WTF`。

所有組合不用背：

| 延伸鍵按了幾個 | 螢幕（Orchid 命名，根音 D、MAJ） |
|---|---|
| 0 | `DM` |
| 1 | `D` 加小字 `7`、`6`、`M7` 或 `9` |
| 2 | `D` 加兩個小字，例如 `6 9`、`7 9`、`M7 9` |
| 3 | `D` `JAZZ` |
| 4 | `D` `WTF` |

種類有 MAJ、MIN、DIM、SUS 四種，四個延伸鍵開關有 16 種，合計 64 種。每一種的組成音，上游列在 [CHORD-NAMES.md](https://github.com/Quixotic7/ChoralRootFM1/blob/main/docs/CHORD-NAMES.md)。

想看正常的和弦名稱，把 **Options → Chord Names** 改成 **Standard**。它照樂譜上的寫法命名，例如 MAJ + 7 + 6 + 9 會寫成 `D13`。只改顯示，聲音、MIDI 和錄好的 loop 都不變。只有同時按了 m7 和 M7（兩種七度）的和弦，Standard 也找不到名字，仍然顯示 `JAZZ` 或 `WTF`。

| 按住（根音 D） | Orchid（預設） | Standard |
|---|---|---|
| MAJ + 9 | `D` `9` | `Dadd9` |
| MAJ + 7 + 9 | `D` `7 9` | `D9` |
| MAJ + 6 + 9 | `D` `6 9` | `D6/9` |
| MAJ + 7 + 6 + 9 | `D` `JAZZ` | `D13` |
| MIN + 7 + 9 | `Dm` `7 9` | `Dm9` |
| SUS + 7 | `Dsus` `m7` | `D7sus4` |
| DIM + 7 | `Ddim` `7` | `Dm7b5` |
| MAJ + 7 + M7 | `D` `7 M7` | `D` `7 M7` |

### Secret Chords：真正藏起來的三個和弦 `[上游]`

這個比較像彩蛋，預設是關的。打開 **Options → Secret Chords**（Simple 或 All）之後，同時按住兩三個種類鍵會得到第三種和弦：

| 同時按住 | 和弦 | 螢幕 |
|---|---|---|
| F#3 + A#3（DIM + MAJ） | augmented 增和弦 | `D+` |
| F#3 + G#3（DIM + MIN） | 小和弦，五音換成四音 | `Dm` 加紅色小字 `add4` |
| F#3 + G#3 + A#3（三個都按） | power chord（根音、五度、八度） | `D` 加紅色小字 `5` |

延伸鍵可以照常疊上去。關掉時這些組合就是「後按的種類鍵算數」。

螢幕上：大三和弦單獨出現時寫成大寫 M（`DM`），小三和弦是小寫 m（`Dm`）；加了延伸音後大和弦的 M 省略（`D` 加一個小的 `7`）。延伸音用橘色顯示。

## 第一課：彈出和弦

| 想彈 | 按住 | 再按 | 螢幕 |
|---|---|---|---|
| D（單音） | 什麼都不按 | D4 | `D` |
| D 大三和弦 | MAJ（A#3） | D4 | `DM` |
| D 小三和弦 | MIN（G#3） | D4 | `Dm` |
| Dm7 | MIN + m7（G#3 + G3） | D4 | `Dm` 加小 `7` |
| G7（屬七） | MAJ + m7（A#3 + G3） | G4 | `G` 加小 `7` |
| CM7（大七） | MAJ + M7（A#3 + A3） | C4 | `C` 加小 `M7` |
| Dsus | G#3 + A#3（一指按兩鍵） | D4 | `Dsus` |

注意 **m7 鍵加的是小七度**，所以 MAJ + m7 是屬七和弦（G7），不是大七和弦。大七要用 M7 鍵。

**練習：ii–V–I（C 大調）**

1. 按住 MIN + m7，彈 D4 → Dm7
2. 換成 MAJ + m7，彈 G4 → G7
3. 換成 MAJ + M7，彈 C4 → CM7

## 第二課：LOCK，把左手空出來

每次都要按住 chord 鍵很累。**LOCK**（面板上印的是 **`LFO`**）會把 chord block 鎖住：

1. 點 `LFO`，燈亮，螢幕右上出現 `lock`。
2. 按住 MIN 再放開：MIN 保持亮著，接下來彈的每個根音都是小和弦。
3. 沒按住任何 chord 鍵時：
    - 點一個**種類鍵**（DIM / MIN / MAJ）：換成那個種類，延伸音清掉。
    - 點一個**延伸鍵**（6 / m7 / M7 / 9）：開或關那個延伸音，種類不變。
4. 再點 `LFO` 關掉 LOCK。

鎖住之後左手可以去轉旋鈕、按其他功能鍵。

## 第三課：Key Mode，一根手指彈整首歌

這是最省事的模式。點 **KEY**（面板印的是 **`SEL`**），燈亮，螢幕左上顯示 `Key: C`。

現在**單獨按一個根音，機器會彈出那個音在這個調裡該有的和弦**。C 大調下：

| 按 | 得到 |
|---|---|
| C | C |
| D | Dm |
| E | Em |
| F | F |
| G | G |
| A | Am |
| B | Bdim |

- 調外的音會彈成鄰近音階音上的 sus 和弦。
- 按住 chord 鍵可以**臨時覆蓋**音階的選擇（例如在 C 大調裡硬彈一個 D 大三和弦）。
- 琴鍵的燈會顯示這個調：音階內的音微亮，主音較亮，調外的音不亮。

**換調**：按住 KEY 並同時按一個根音，那個音就成為主音。要小調就同時按 MIN。然後 `OCT-` 關閉 layer。

按住 KEY 會開 **KEY layer**：KNOB 1 選主音、KNOB 2 選音階（共 23 種：大小調、各種 mode、五聲、藍調…）、KNOB 3 移調（±24 半音）。

### 用 C 的指法彈 F：只要 Transpose，不用改主音 `[上游]`

主音（Tonic）和移調（Transpose）是兩件事：

- **Tonic** 決定「哪個鍵是一級」，也就是你的指法以哪個調為準。
- **Transpose** 把彈出來的所有音整體移高或移低，指法不變。

想用 C 大調的指法、聽到的是 F 大調（按 C 鍵得到一級和弦 F）：**主音留在 C，只把 Transpose 設成 +5**。

1. 按住 `SEL`（KEY）打開 KEY layer。
2. KNOB 1 確認主音是 C、KNOB 2 是 Major。
3. KNOB 3 轉到 **+5**。
4. `OCT-` 關閉 layer。

之後按 C 鍵響的是 F、按 G 鍵響的是 C、按 A 鍵響的是 Dm。螢幕上的和弦名稱寫的是**聽到的**（`FM`），琴鍵的燈也顯示實際發出的音。Options 打開 Numerals 的話，級數寫的是**你按的**，所以按 C 永遠是 `I`。

如果反過來把主音設成 F、Transpose 留 0，聽到的一樣是 F 大調，但你要用 F 大調的指法彈（按 F 鍵才是一級）。兩個同時設（主音 F、Transpose +5）就會變成 B♭ 大調，通常不是你要的。

Transpose 在 Key Mode 關掉時也有效。要關機後還在，**按住 `SAVE`**（全部存檔）：主音、音階、Transpose 都會存進去；沒存的話下次開機回到上次存的狀態。哪天發現和弦全部不在對的調上，先檢查 KEY layer 的 KNOB 3 有沒有歸零。

**練習：流行進行 I–V–vi–IV（C 大調）**

開 Key Mode，依序按 C4、G4、A4、F4。四根手指，四個和弦。

## 第四課：Voicing

同一個和弦可以有不同的排列。**按住和弦轉 KNOB 1**：

- 往上一格：最低音上移八度。
- 往下一格：最高音下移八度。
- 範圍 -12 到 +12，螢幕顯示 `voicing +2`。

這個設定會套用到之後所有的和弦，包括正在播放的 loop。`OCT-` / `OCT+` 則是把根音整組移八度（-2 到 +2）。

## 第五課：加 bass

點 **BASS**（面板印的是 **`ENV`**）。每個和弦下面會自動加一個 bass 音，螢幕右上顯示 `Bass`。

- **ALGORITHM**：換 bass 的音色（轉到底是 off）。
- **KNOB 2**：bass 的音域，-2 到 +4 八度。
- 按住 BASS 轉 ALGORITHM：bass 的音量。
- bass 是單音的獨立聲部，有自己的引擎。

## 第六課：Performance mode，讓它幫你彈

點 **PERF**（面板印的是 **`ARP`**）開關。按住 PERF 開 PERF layer，用 **SELECT** 選模式：

| 模式 | 效果 |
|---|---|
| **Strum** | 和弦音一個接一個掃出來，像刷吉他 |
| **Slop** | 時間不整齊的 strum |
| **Arpeggiate** | 依 tempo 輪流彈和弦音 |
| **Pattern** | 12 種節奏型（pulse、alberti、waltz、stride…） |
| **Harp** | 跨好幾個八度的快速琶音 |

在 layer 裡 KNOB 1 是速度（Rate 或 Division）、KNOB 2 是順序（上、下、上下、照彈的順序、隨機）、KNOB 3 與 KNOB 4 依模式而定。layer 外面 KNOB 3 直接調該模式的主要參數。

琶音在放開鍵時會停。要讓它持續：**LATCH**（按住 `GLO` 再點 `LFO`）。

## 第七課：Looper，錄的是和弦不是聲音

1. 點 **`REC`**。燈亮，訊息顯示 `rec: play to start`。
2. 彈幾個和弦。第一個和弦落下時開始錄。
3. 再點 `REC`（或 `PLAY`）：`loop recorded`，立刻開始循環。
4. **LOOP**（面板印的是 **`PLAY`**）：停止 / 播放。
5. **按住 `SAVE`**：存起來，否則關機就沒了。

重點：loop 記的是「哪個根音、哪種和弦、什麼時候、多長」，播放時重新經過樂器。所以 loop 播放中你可以：

- 轉 PRESETS 換音色，loop 跟著變。
- 轉 KNOB 1 改 voicing。
- 開關 bass、換 performance mode。
- 改 tempo：只改速度，不改音高。

按住 `REC`：undo 最後一層。按住 `GLO` 再點 `REC`：切換錄音模式（Overwrite、Overdub…）。

## 換音色

- **PRESETS**：同一個引擎的下一個音色。只轉這個會覺得音色很單一，因為它不會跨引擎。
- **按住 OPT（`GLO`）轉 PRESETS**：換引擎。有 FM6、FM TONE、VA、CZ-1、P5、WHEEL、GRAIN，合計 258 個內建音色，見[七個引擎](index.md#sounds)。
- 想用機器上沒有的音色：把它當 MIDI 和弦控制器，見[送 MIDI 出去](index.md#midi-out)。
- **`EDIT`**：進音色編輯器。

## 面板對照

面板上印的字大多換了工作：

| 印的 | ChoralRoot | 點一下 | 按住 |
|---|---|---|---|
| `FX` | FX | 效果開關 | FX layer |
| `SEL` | **KEY** | Key Mode 開關 | KEY layer |
| `ENV` | **BASS** | bass 開關 | BASS layer |
| `LFO` | **LOCK** | LOCK 開關 | LOCK layer |
| `EDIT` | EDIT | 音色編輯器 | 引擎選擇 |
| `GLO` | **OPT** | Options | 搭配旋鈕：第二功能 |
| `HOME` | HOME | 回主畫面 / 下一個畫面 | 畫面選單 |
| `SAVE` | SAVE | 存音色 | **存全部** |
| `ARP` | **PERF** | performance 開關 | PERF layer |
| `SEQ` | SEQ | loop 編輯器 | 節拍器 / 鼓 |
| `PLAY` | **LOOP** | 播放 / 停止 loop | PLAY 選單 |
| `REC` | REC | 錄音 | undo 一層 |

layer 的通則：點一下是開關，按住約半秒是打開 layer（放開後仍開著，按鈕會閃）。`OCT-`、`OCT+` 或 `HOME` 關閉 layer。

## 卡住的時候

- **`OCT-` + `OCT+` 同時按**：PANIC，所有音關掉。
- 這兩個鍵**加 `HOME` 按住 5 秒**：進 update mode。平常不要按這個組合。
- 琴鍵沒有力度感應，音量是固定值（在 Options 裡設）。

## 建議的練習順序

1. 第一課的 ii–V–I，確認看得懂螢幕上的和弦名稱。
2. 開 LOCK，鎖住 MIN + m7，隨便彈根音，聽小七和弦的平行移動。
3. 開 Key Mode，彈 I–V–vi–IV；換一個調再彈一次。
4. 加 bass，轉 KNOB 1 聽 voicing 的差別。
5. PERF 選 Strum，再換 Arpeggiate。
6. 錄一個四小節的 loop，播放中換音色與 performance mode。

## 沒寫到的

手冊還有很多：三種 Play Style（Simple、Advanced、Free）、Classic 配置（Orchid 原本的鍵位）、鼓、loop 編輯器、音色編輯器、MIDI out 三個 channel、USB audio 三組立體聲。需要時查[上游手冊](https://github.com/Quixotic7/ChoralRootFM1/blob/main/docs/MANUAL.md)。
