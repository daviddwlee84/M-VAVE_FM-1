# 吉他指板與和弦書（GUITAR）

把 FM-1 變成隨身的吉他指板與和弦圖：選一個調，指板上標出音階的每個位置，可以一次只看一個把位（CAGED、3NPS）；彈一個和弦，就畫出它的按法，最簡單的排第一。靈感來自 [ChordRoot](https://chordroot.com/)，但只借想法，不抓它的資料。

**狀態（2026-11-18）：v0.1（NECK + SHAPE）在 host simulator 和 fm1-emu 兩種晶片上驗證過，也疊在實機備份上預演過 `[模擬]`；還沒刷進實機。** 驗證結果見 [v0.1 驗證](#v01-verify)。

- 韌體在 `firmware/guitar/`，GPL-3.0-or-later，名稱 GUITAR，識別碼 `FM-1_55`（開發版 `FM-1_5500`）。原本選的 `FM-1_53` 和 Hub 上 LSDJ Burrito 的 `FM-1_537…` 開頭相同（嚴格說格式不會撞，但 Morse 遇到 `FM-1_13` 對 `FM-1_137` 時也照這個原則換掉），所以改成 `FM-1_55`：2026-11-18 對照 Hub catalog（12:01 UTC 版）沒有人用 `[上游]`。上 Hub 前仍要用 #check 再查一次。
- 和 THEORY、RHYTHM 分開，是獨立的韌體（2026-11-18 本人決定，見[設計提案](#background)）。直接 `include` THEORY 的 `theory.c`、`synth.c`、`gtext.c`，所以授權跟著 THEORY 是 GPL-3.0-or-later。
- 設定存在 `0xD4000`（見[存檔](#flash)）。
- 指板核心 `fret.c` 的 `just guitar-unit` 有 299,610 個檢查，全部通過。

## 背景 { #background }

2026-11-18 的調研（Firmware Hub catalog、GitHub、一般搜尋，`[調研]`）：FM-1 上沒有任何顯示吉他指板、和弦圖或音階指法的韌體。現有的吉他韌體（fm1-guitar、Riffsmith、FoMni、ChoralRoot、Strumline）都是讓 FM-1「聽起來或彈起來像吉他」。其他平台的參考：ChordRoot（和弦圖、no barre 簡易版、tunings）、Tuitar（ESP32 上的指板與找音遊戲）、Gingoduino（MIT，微控制器上的 Fretboard 模組）。

包裝方式本人選了「RHYTHM 與 GUITAR 都另做韌體」：各自有自己的 `FM-1_` identity 與設定 sector，穩定之後再比照 fm1-morse 拆成獨立 repo 上 Hub。

## 頁面 { #pages }

用 `HOME` 在兩頁之間切換。

| 頁面 | 做什麼 | 版本 |
|---|---|---|
| **NECK** | 一個調的音階在指板 0–12 品上的所有位置；整個指板，或一次一個把位（CAGED 5 個、3NPS 7 個、五聲音階 2NPS 5 個） | v0.1 |
| **SHAPE** | 和弦圖：手指編號、barre、空弦 ○、悶音 ×、根音標色，最簡單的按法排第一 | v0.1 |
| BOX | 音階指法速查：5 品放大，每個點寫手指編號 | 之後 |
| FIND | 找音練習（Leitner，項目 = 弦 × 品區） | 之後 |

### NECK { #neck }

![NECK：G 大調的 CAGED E 把位](../assets/guitar/neck-caged-e.png){ width="240" } ![NECK：音名標示](../assets/guitar/neck-names.png){ width="240" } ![NECK：彈 G4，指板上所有的 G4 圈起來](../assets/guitar/neck-note.png){ width="240" }

host simulator 的截圖 `[模擬]`。左：G 大調、CAGED 的 E 把位（2–5 品），把位外的調內音是小灰點。中：整個指板，標示換成音名。右：按住 G4，指板上每個 G 都圈起來，footer 顯示 `G4` 和級數 `1`。

- **方向**：高音弦在上，和 tab 一樣。左邊一欄是每根弦的空弦音名（依調拼寫）。
- **點**：調內的音；主音較亮、較大。在把位模式下，把位外的調內音縮成小灰點。
- **按住琴鍵**：那些音（不分八度）在指板上的每個位置都畫白圈；放開就消失，footer 留著剛才的音名或和弦名與級數。
- **把位超出 12 品**時，指板自動往右捲，左下角標出第一格的品數。

| 操作 | 作用 |
|---|---|
| 轉 `SELECT` | 沿五度圈換調 |
| 按住 `SEL` 再按一個琴鍵 | 跳到以那個音為主音的調 |
| `K1` | 音階：THEORY 的 12 種（大調、各調式、和聲／旋律小調、大小五聲、blues） |
| `K2` | 標示：點、音名、級數（`b3`、`#4`，相對於同主音的大調） |
| `K3` | 把位：整個指板 → CAGED C、A、G、E、D → 3NPS 1–7（五聲音階是 2NPS 1–5；blues 沒有 NPS） |
| `K4` | 樂器與 tuning（兩頁共用） |
| `PLAY` | 把目前把位的音由低到高彈一遍，指板上跟著標出正在響的那一格（再按一次停） |
| `OCT-` / `OCT+` | 琴鍵發聲的八度 |

### SHAPE { #shape }

![SHAPE：C 的開放和弦](../assets/guitar/shape-c.png){ width="240" } ![SHAPE：Bbm 的 A 型 barre](../assets/guitar/shape-bbm.png){ width="240" } ![SHAPE：烏克麗麗的 C](../assets/guitar/shape-uke.png){ width="240" }

host simulator 的截圖 `[模擬]`。左：C 的第一個按法 `x32010`，手指 3、2、1。中：Bbm 的 A 型 barre（第 1 品）。右：換成烏克麗麗，C 變成 `0003`。

- **圖**：直的弦（低音弦在左），橫的品，最上面是上弦枕；按法在 5 品以上時上弦枕換成細線，左邊標起始品數。
- **點**裡是手指編號（1 食指到 4 小指）；根音用亮色。barre 畫成一條橫槓。弦上方是 ○（空弦）或 ×（不彈）。圖下面是每根弦實際的音名。
- **右欄**：和弦名、組成音、第幾個按法（`1 / 8`）、`OPEN` / `BARRE n` / `FRET n`、省略五音時標 `NO 5TH`，以及按法的文字寫法（`x32010`）。

| 操作 | 作用 |
|---|---|
| 彈一個和弦 | 辨認出來（THEORY 的 `th_identify`），畫出它的按法 |
| 按住 `SEL` 再按一個琴鍵 | 根音換成那個音 |
| 轉 `SELECT` | 下一個按法 |
| `K1` | 和弦性質：常用的排前面（大、小、7、m7、maj7、sus4、sus2、5、dim、aug、m7b5、dim7、6、m6、7sus4、add9、madd9、9、maj9、m9、mMaj7、aug7、augMaj7） |
| `K2` | 根音，一格半音（拼法跟大調一樣：Bb、F#、Db） |
| `K4` | 樂器與 tuning |
| `PLAY` | 照這個按法的實際音高，從低音弦往上 strum |

LED：NECK 亮調內的音（主音 mid、其他 glow），SHAPE 亮和弦的組成音（根音 mid）；按住的鍵 lit；`PLAY` 播放時亮綠燈。設定在最後一次操作 3 秒後存起來。

## 指板核心 fret.c { #core }

`firmware/guitar/fret.c`：C99，不呼叫平台、不用 libc、不用浮點數，和 `theory.c` 一樣可以在電腦上單獨測（`just guitar-unit`）。

### Tuning

每種 tuning 是一行：弦數加上每根空弦的 MIDI 音。弦從音最低的那根算起（吉他的低音 E 是 0 號弦），畫面再決定怎麼擺。

| 名稱 | 空弦 |
|---|---|
| STANDARD | E2 A2 D3 G3 B3 E4 |
| DROP D | D2 A2 D3 G3 B3 E4 |
| DADGAD | D2 A2 D3 G3 A3 D4 |
| OPEN G | D2 G2 D3 G3 B3 D4 |
| BASS | E1 A1 D2 G2 |
| BASS 5 | B0 E1 A1 D2 G2 |
| UKULELE | G4 C4 E4 A4（reentrant：0 號弦是高音 G） |
| MANDOLIN | G3 D4 A4 E5（每組複弦當一根） |

### 把位 { #positions }

- **CAGED**：每個把位是一個 4 品寬的窗（食指到小指）。窗的位置從「同調號的大調主音在 0 號弦上的品數 r」算：C 型 r+4、A 型 r+6、G 型 r+9、E 型 r−1、D 型 r+1（取 0–11 品）。C 大調在標準調弦上就是 C 型 0–3 品、A 型 2–5、G 型 5–8、E 型 7–10、D 型 9–12。窗裡的調內音全部標出；夾在兩根弦之間、窗裡沒有的音（例如 C 大調 G 型的 B），用較高那根弦往回一品的位置補上（食指往後伸）。小調與調式用它的關係大調的窗，所以 A 小調和 C 大調的五個把位相同。
- **NPS**：從音階的第 n 個音在 0 號弦上的位置（0–11 品）開始，往上每根弦固定放 3 個音（七聲音階）或 2 個音（五聲音階）。如果某根弦會落到 0 品以下，整組往上一個八度。
- 單元測試：15 個調號 × 12 種音階 × 每個把位，**把位範圍內每個調內音剛好出現一次**、沒有調外音；NPS 每根弦的音數正確；CAGED 窗最多 5 品寬。另外對照教科書上的把位：G 大調 E 型、C 大調 C 型／E 型／G 型、G 大調 3NPS 第 1 把位、A 小調五聲第 1 把位。

### 和弦按法的搜尋 { #search }

不抄和弦表，用搜尋產生：

1. 對每個起始品 b（1–12），每根弦可以是 ×、空弦（如果是和弦音）、或 b 到 b+3 品之間的和弦音。逐弦往下搜，一個和弦約幾百個組合。
2. 留下來的要符合：每個音都是和弦音；組成音都在（完全五度可以省略）；最低音是根音（reentrant 的烏克麗麗不要求）；至少 4 根弦有聲（4 弦樂器至少 3 根）；跨度不超過 4 品。
3. **手指**：不用 barre 時，從最低的品往上、同一品先低音弦，依序給 1、2、3、4 指。超過 4 指時試 barre：食指壓在最低品，從最低那根有這一品的弦壓到最高那根，中間每根弦都要按在這一品以上；壓不了就丟掉。這個規則對 C、G、D、Am、E、F barre 都得出教科書的指法。
4. **排序**（分數越低越前面）：每根要按的手指 +2、每根空弦 −1、低音側悶音 +2、高音側悶音 +6、中間悶音 +10、barre +6、起始品數、省略五音 +3、兩根弦同一個音 +4、最低兩個音只差二度或四度 +4（例如 `x00230` 雖然少一指，聽起來不如 `x02230`）。
5. 留最前面 8 個，去掉重複的。

**對照測試**：26 個吉他手先學的開放和弦（C、A、G、E、D、Am、Em、Dm、各種 7、m7、maj7、sus4、sus2）每一個都被搜到，而且排在前三；3 個 barre 和弦（F `133211`、Bm `x24432`、Fm `133111`）在前八。8 種 tuning × 13 個根音 × 23 種和弦性質的每個結果都再檢查一次：和弦音、根音在低音、手指數、跨度、排序。標準調弦上，四個音以下的和弦每一個都至少有一個按法（九和弦有些找不到 4 品內的按法）。

在電腦上（`-O0`）一個和弦的搜尋約 0.5 ms，只在換和弦或換 tuning 時跑一次。

排序的權重是憑經驗調的。實際用起來覺得哪個按法排錯，把它加進 `tests/fret_test.c` 的 `OPEN` 表再調權重。

## 存檔 { #flash }

設定是一筆 `rec_t`（`GT01`，約 24 bytes），透過 fm1-base 的 store（`APP_STORE_MAP {0xDC000, 0xD4000}`）存在 `0xD4000`。選這裡是因為：`0xD6000` 是 THEORY 0.2 的閃卡進度、`0xD8000` 是 THEORY 的設定、`0xDA000` 是 Morse、`0xDC000` 是面板校正，而 `0xFC000` 是 SLOOP、Agent Deck、CTL-1 共用的設定區。

疊在 `2026-11-18-pre-theory-0.2` 備份上預演（兩種晶片，`[模擬]`）：開機 0 次寫入；存檔只改 `0xD4000` 一個 sector；`0xD6000`、`0xD8000`（THEORY）、`0xDA000`（Morse）、`0xFC000` 都和備份相同。備份本身的 `0xD4000`–`0xD5FFF` 全是 `0xFF`。這份備份是裝 THEORY 0.2 之前做的，THEORY 0.2 的閃卡進度在 `0xD6000`，GUITAR 不碰那裡。

## v0.1 驗證 { #v01-verify }

2026-11-18，全部 `[模擬]`。後三列在本人的 Mac 上跑（PR 的 worktree）。

| 項目 | 怎麼驗證 | 結果 |
|---|---|---|
| 指板核心 | `just guitar-unit` | 299,610 個檢查全部通過 |
| 畫面、辨認、聲音、存檔 | `just fw-test guitar`（`tests/basic.txt`） | 全部 ok：G 大調開機、琴鍵燈、單音 `G4 1`、和弦 `C IV`、CAGED E 與 3NPS 的格子、捲動、音名與級數標示、`PLAY` 跑音階、換調換音階、D 小調五聲 2NPS、`SEL` 加琴鍵；SHAPE 的 C `x32010` 指法 `032010`、strum 5 個聲部、彈 Am 得到 `x02210`、F、B7 `x21202`、Bbm、Gm、烏克麗麗 C `0003`；存檔、重開讀回 |
| 實機套件 | `just fw-build guitar`（本人的 Mac，dev build `FM-1_5300`，改 identity 之前） | app 125,944 B（槽 581,564 B），RAM `.data+.bss` 29,052 / 98,304 B，`.ram_text` 5,100 / 32,768 B；沒有 soft-double，`firmware/guitar` 沒有 compiler warning；套件 616,003 B |
| 兩種晶片：改設定、重開讀回 | fm1-emu `uc25hq80`、`p25q80` headless，`FM1_BENCH_KEYS` 轉 `SELECT` 到 A major、按 `HOME` 到 SHAPE | 每次存檔寫 3 次；重開 0 次寫入，讀回 SHAPE 頁與 A major |
| 疊在實機備份上 | `emu_rehearse.py` 疊在 `2026-11-18-pre-theory-0.2`，兩種晶片 | 開機 0 次寫入；存檔只寫 `0xD4000`（見[存檔](#flash)） |

刷進實機之前要照 AGENTS.md：兩種晶片都跑過存檔與重開、`emu-rehearse` 報告寫入的 sector、完整備份、本人逐次同意。

## 之後 { #next }

- 換成 `FM-1_55` 之後重做一次 `fw-build`（只差 identity），刷實機前照 AGENTS.md 做完整 preflight。
- release 版要在 `fm1.toml` 補 `install`、`screenshot`（build 會提醒）。
- 指板方向可以反過來（低音弦在上）、左手吉他鏡像。
- BOX 頁：5 品放大的音階指法，每個點寫手指編號。
- SHAPE 的轉位與 slash chord（`C/E`）、jazz 的 shell voicing（目前要求根音在低音）。
- FIND 找音練習，和 THEORY 的 TODAY 共用 Leitner 回合。
- 網頁匯入自訂 tuning。
