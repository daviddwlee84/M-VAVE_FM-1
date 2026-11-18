# 樂理練習機（FM-1）：設計文件 + 樂理核心

## Context 背景

使用者想把 Morse（閃卡 + Leitner rounds）和背單字構想（FSRS、Anki）的經驗，延伸成 FM-1 上的**樂理、鍵盤、聽力練習機**：五度圈、和弦進行、各調音階與琶音，偏流行編曲，但古典基本功也要有。使用者小時候在 Yamaha 練過聽音，想加入相對音感、絕對音感，以及**節奏聽寫**。節奏聽寫和 Morse 的盲聽節奏很像，題型有兩種：看小譜面選出聽到的是哪一個，或盲聽之後把它彈出來。

已決定的事（2026-11-18，本人回答）：

- **v0.1 的體驗是 WHEEL**：五度圈探索，加上和弦辨識。
- **教材格式**：音階、和弦、調號由程式生成；和弦進行用羅馬數字 TSV；旋律用 **Alda 子集**。要有**網頁**可以匯入教材、備份進度，比照 Morse。
- **這一輪的範圍**：設計文件，加上**開始寫 platform-free 的樂理核心**（C）和 unit test。WHEEL 的 app 本體、synth、畫面下一輪才做。

## 研究結論（寫進文件，附來源）

**有人做過（2026-11-18 查）**

- **Shed**（`Luisnvb/fm1-shed`）：FM-1 上的 jazz 聽力訓練。
  - 有 9 種 drill，先放 cadence 再出題，一回合 20 題，弱項較常出現。
  - 建在 FM1 Open SDK 上，GPL-3.0 + SDK exception。
  - **沒有 release、不在 Hub**，2026-10-23 之後沒有更新，音名拼法沒有定義。
- **FM1 Keyboard Academy**（`chmess/fm1-keyboard-academy`）：初學者課程。
  - 流程是 listen → watch → play → review，按鍵 LED 帶彈，正確率 80% 解鎖下一課，附 30 首旋律。
  - 有 `.fwsc` v0.1.2，不在 Hub。
- 其他 FM-1 作品：
  - Kris Jangle：聽旋律再彈出來。
  - Midi-Mania：音符落下的節奏遊戲。
  - Flyfish：練一首曲子。
  - 和弦樂器類（ChoralRoot、fm1-chord、CTL-1、FoMni）是用來彈的，不是用來教的。
- 其他平台：Functional Ear Trainer、Melodics、Simply Piano、Synthesia（wait mode）、Casio、LUMI（按鍵發光）、Launchkey（scale / chord mode）；開源的有 OpenEar（MIT）、Solfege、LenMus（GPL）。微控制器上只有 Gingoduino（MIT，樂理引擎）。
- **空缺**：
  - 跨天的 SRS，同一套排程同時管樂理、聽力、節奏、手上技巧。
  - 五度圈和和弦辨識，當隨身參考。
  - 流行與華語的和弦進行（王道進行、4536251、卡農）配 Nashville number。
  - 15 個調號都拼對音名。
  - 網頁備份。

**Best practice → 設計原則**

1. 用彈的來回答。
2. 先短暫集中，再交錯（Carter & Grahn 2016、Wong 2021）。
3. 短回合（約 20 張卡）。
4. 聽力先放 cadence 建立調性（Functional Ear Trainer、Bruce Arnold、Chenette）。
5. 調性螺旋（ABRSM）：從 C 往五度圈兩側展開。
6. 知識交給 SRS；手上技巧看實際表現評分，速度階梯要練到、甚至超過目標速度（Allingham & Wöllner 2022），錯誤點集中練（Duke 2009）。
7. 每個階段同時包含樂理、鍵盤、聽力、流行應用。
8. 寫文件時另外查核兩件事，查不到就標 `[調研]` 或寫明是本人的經驗：成人絕對音感訓練的研究（例如 Wong et al. 2020），以及 Yamaha 音樂教室聽音課的做法。

**機器條件**（`plat.h`、`gfx.c`）

- 琴鍵 27 鍵（F3–G5，MIDI 53–79），每鍵一顆 LED。
- 螢幕 240×240 彩色，沒有畫圓的 primitive，字型只有 ASCII，♯♭ 要自己做 bitmap。
- fm1-base 沒有 synth，`common/chime` 的 2-op FM 可以當起點。
- 有 `fm1.toml gen = [...]`（build-time generator）。
- 一個 store object 最多 3,840 B。
- 韌體槽 581,564 B，Morse 用了約 138 KB。

## 交付 1：設計文件 `docs/dev/theory.md`（繁體中文，構想草案）

章節順序比照 `docs/dev/vocab.md`：

1. 定位和狀態列（2026-11-18：構想 + 樂理核心）。
2. **有沒有人做過**：表格，加上和 Shed、Keyboard Academy 的差異。
3. **學習方法**：上面的原則，附來源。
4. **課綱**：7 個階段，每階段有樂理、鍵盤、聽力、節奏、流行五部分。主題 × 調的矩陣依五度圈解鎖，像 Koch 的字組。
5. **頁面**：用 `ALGO` 切換，比照 Morse。每頁附 ASCII 草圖：
   - **WHEEL**（v0.1）：五度圈；轉 `SELECT` 換調，或按一個琴鍵直接跳到那個調。畫面顯示調號、關係小調、I–vii° 的 Roman 與 Nashville；LED 亮出音階、主音最亮。彈一個和弦，畫面說出名字、轉位、在這個調裡的級數（含 `bVII`、`V7/V`）。白鍵按下去聽順級和弦，轉位用 nearest voice。
   - **TODAY**：交錯的 SRS 回合。
   - **DRILL**：音階和琶音，節拍器、LED 帶彈、速度階梯、時間均勻度條狀圖。
   - **GROOVE**：流行和弦進行伴奏，`OCT±` 沿五度圈移調。
   - **LISTEN**：被動聆聽。
   - **CHART**：五度圈熱圖。
   - **SETTINGS**。
   - 再附按鍵對應表。
6. **卡片種類**：
   - 樂理：調號、五度圈關係、拼音階、組和弦、級數。
   - 聽音：
     - **相對音感**（主線）：cadence 之後聽級數、調內音程、和弦性質、和弦進行（7 個白鍵代表 I–vii）、bass。
     - **絕對音感**（選用的「固定音名」線）：沒有調性脈絡的單音，每題換音色和音域，題與題之間放干擾聲。
   - **節奏**：
     - 聽 1–2 小節，從 4 個小譜面選一個；4 個選項用 E4 F4 G4 A4 作答，同 Morse 的 `CHOICE_KEY`。
     - 盲聽之後在任一鍵 tap 回來，依 onset 時間評分，沿用 Morse RHYTHM 的經驗。
     - 看譜面 tap 出來，先有 count-in。
   - **旋律**：聽 3–8 個音再彈回來；看小五線譜彈出來。
   - **鍵盤**：音階、琶音、cadence 與轉位、移調，自動評分。
   - 評分對應到 Again / Hard / Good / Easy。
7. **小譜面怎麼畫**：
   - 節奏用單線譜，旋律用五線譜。
   - 符頭、符桿、符尾、beam、休止符、附點、連結線用小 bitmap 加線段，依時值程式產生。
   - 4 個節奏選項每個約 40 px 高，放得進 176 px 的主畫面。
8. **節奏也用程式生成**：
   - 依 Kodály 式的節奏單元（cell）分級：ta、ti-ti、休止、ta-a、tika-tika、附點、切分、三連音、連結線。
   - 選項中的錯誤答案只改一個 cell，比照 Morse 的 `ln_alike`。
   - 時值用每拍 12 tick（可以表示三連音）。
9. **機器限制怎麼處理**：
   - 27 鍵：判斷時不分八度；左手 F3–E4，右手 F4–G5。
   - LED、畫圓、♯♭ glyph。
   - synth：8 voice FM EP、bass、click，音符依 sample 數在 render 裡排程。
10. **內容表示，三層**：
    - 裝置上用 line of fifths 程式生成。
    - 羅馬數字 TSV 經 `gen` 轉成 C 表。
    - 旋律用 Alda 子集，自己寫的 Python parser 在 build 時轉成 2 B/事件的 bytecode，同時輸出 `.mid` 試聽。MIDI 只在電腦端用 mido 匯入。
    - Alda 官方工具（EPL-2.0）只拿來選用對照。
11. **網頁：匯入教材、備份進度**：
    - Web MIDI 頁，同 Morse 的 `web/index.html`，用 hostlink SysEx。
    - 備份和還原進度。
    - 匯入自己寫的進行或旋律：瀏覽器解析 TSV / Alda 子集 → SysEx → FM-1 問一次（`PLAY` 確認）→ 存進 **store object**。
    - **走 `APP_STORE_MAP`，不需要改 AGENTS.md 的 flash 規則。** 單一 object 3,840 B，大約可以放 100 個進行或短旋律。真的需要更大的 deck，才走 vocab 頁的資料區草案。
    - parser 有 Python（build）和 JS（網頁）兩份，用共用的 test vectors 確保一致（Morse 的 protocol vectors 也是這樣做）。
12. **排程與存檔**：
    - item = (skill, key, variant)，每個約 3 B。
    - v1 沿用 Morse 的 Leitner rounds，之後可以和 vocab 共用 FSRS。
    - store sector 避開 `0xDA000`、`0xDC000`，實作時用 `emu-rehearse` 確認。
13. **內容來源與 licence**：自己寫內容；Hooktheory 不能用；OMT 是 CC BY-SA（只和 GPLv3 相容）；musictheory.net 的課文是 CC BY 4.0；teoria 不能用。
14. **Roadmap**：
    - 本輪：核心 + unit test。
    - v0.1：WHEEL + 和弦辨識（app、synth、畫圓、glyph）。
    - v0.2：TODAY，含樂理卡、級數與和弦性質聽音卡、進度備份網頁。
    - v0.3：節奏與旋律聽寫，含小譜面、節奏生成。
    - v0.4：DRILL。
    - v0.5：GROOVE，加上 TSV/Alda pipeline 和網頁匯入。
    - 之後：獨立 repo，上 Hub。
    - 開工前先在 fm1-emu 試用 Keyboard Academy，並讀 Shed 的 SHED_SPEC。
15. **還沒決定的事**：名稱、app 最後的 licence、要不要和 vocab 共用 SRS。

## 交付 2：樂理核心 `firmware/theory/`（GPL-3.0、C99、沒有 libc、沒有 float）

寫法比照 Morse 的 `learn.c`：platform-free，用 `#include "../theory.c"` 拿到電腦上測。

**`firmware/theory/theory.h` / `theory.c`**

- **Pitch**：spelled pitch class 是 line of fifths 上的位置 `f`（C=0、G=+1、F=−1、F#=+6、Bb=−2）。
  - `th_pc(f)`：`7f mod 12`。
  - `th_letter(f)`：`"FCGDAEB"[(f+1) mod 7]` 換成 C 起算的字母。
  - `th_acc(f)`：floor((f+1)/7)。
  - `th_name(f, buf)`：輸出 `"F#"`、`"Bb"`、`"F##"`。
  - `th_spell_pc(pc, key_f)`：離調中心最近的拼法；D 調的 pc 6 是 F#，Db 調是 Gb。
  - `th_note_name(midi, key_f, buf)`：例如 `"C#4"`。B#、Cb 的八度要處理對。
- **Interval**：用 fifths offset 表示（m2 −5、M2 +2、m3 −3、M3 +4、P4 −1、A4 +6、d5 −6、P5 +1…），可以輸出名稱。
- **Scale**：fifths offset 表，約 12 種：major、自然小調、和聲小調、旋律小調、6 個 mode、大小調五聲音階、blues。
  - `th_scale(tonic_f, scale, out[])`：拼出音階。
  - 調號：`th_keysig(tonic_f, mode)`，回傳升降記號的數量，加上依序排列的記號。
- **Circle**：屬調、下屬調、關係小調、平行調、五度圈上的順序（15 個調號，−7…+7）。
- **Chord**：約 20 種 quality 的 fifths offset 表（maj、m、dim、aug、sus2、sus4、6、m6、7、maj7、m7、mMaj7、m7b5、dim7、add9、9、maj9、m9、7sus4…），加上 ASCII 符號。
  - `th_chord(root_f, q, out[])`：拼出和弦音。
  - `th_diatonic(key, degree, sevenths)`：順級三和弦或七和弦。
- **Roman numeral**：
  - `th_roman_name(key, chord, buf)`：輸出 `I`、`ii`、`vii°`、`IVM7`、`bVII`、`V7/V`（secondary dominant 用簡單規則判斷）。
  - `th_roman_parse(str, key, &chord)`：用來測 TSV，以及 `IVM7 V7 iii7 vi` 在各調的轉換。Nashville 數字也要能解析。
- **Identify（和弦辨識）**：`th_identify(midi[], n, key, &result)`。
  - 每個 pc 都試當根音，和 quality 表比對 pc-set（七和弦允許省略五音）。
  - 排序規則：完全符合 > 根音就是 bass > 調內和弦 > 比較簡單的 quality。
  - 輸出根音、quality、bass（slash / 轉位）、用調性脈絡拼好的名稱，以及級數。
  - C6 和 Am7 這種同一組音的和弦，由 bass 決定。
- **Voice**：`th_voice_near(prev[], chord, lo, hi, out[])`。在 27 鍵範圍裡選總移動量最小的轉位，給 WHEEL 試聽和之後的 GROOVE 用。

**`firmware/theory/tests/theory_test.c`**：用 Morse 的 `CHECK` 巨集。

- 15 個大調和 15 個小調（自然、和聲）的音階逐音比對：C# 大調要有 E#、B#；Cb 大調要有 Fb；G# 和聲小調要有 F##。
- 所有 7 音音階在所有調都是連續的 7 個不同字母。
- `th_pc` 和名稱 round trip。
- 調號的數量和順序。
- 五度圈關係。
- C 大調、A 小調順級三和弦與七和弦的 Roman 名稱。
- identify 的案例：C/E；Am7 對 C6（看 bass）；G7=V7；D7=V7/V；Bb=bVII；dim7；aug；省略五音的 7。
- `th_spell_pc` 的 F# / Gb。
- 王道進行和 4536251 在 C、Eb、F# 的轉換。
- `th_voice_near` 的總移動量不大於任何其他轉位。

**對照用的 vectors**：`host/theory/make_vectors.py`（GPL-3.0）。

- 用 `uv run --with music21`（BSD-3）產生 `firmware/theory/tests/vectors.h`：所有調 × 音階、和弦 quality × 根音的拼法。
- 檔案 commit 進 repo，測試時不需要網路。
- 如果 music21 和我們的結果不一致，逐項檢查，把結論寫進測試註解。

**其他**：`firmware/theory/README.md`（英文，說明目前只有核心）。這一輪還沒有 `app.c`、`fm1.toml`。

## 要改的檔案

- **新增**：
  - `docs/dev/theory.md`
  - `firmware/theory/theory.h`、`theory.c`、`tests/theory_test.c`、`tests/vectors.h`、`README.md`
  - `host/theory/make_vectors.py`
- `Justfile`：
  - `theory-unit`：`cc -std=c99 -Wall -Wextra -pedantic`，同 `morse-unit`。
  - `theory-vectors`：重新產生 vectors。
- `zensical.toml`：在 `dev/vocab.md` 之後加 `{ "樂理練習機（構想）" = "dev/theory.md" }`。
- `docs/firmware/index.md`：新增「學習類」小節，列 Shed、Keyboard Academy、Kris Jangle、Midi-Mania、Flyfish、MORSE，註明 Hub 上的晶片標示，以及是否上架、有沒有 release。
- `AGENTS.md`：
  - Layout 加 `firmware/theory/` 一列。
  - Commands 加 `just theory-unit`。
  - 「Not set up yet」加一句樂理 app 還在構想階段，網頁匯入走 store layer。
- `docs/dev/vocab.md`：加一行互相連結，提到可能共用 SRS 模組。
- `docs/dev/index.md`：結構圖加 `firmware/theory/`。

不 commit，除非本人要求。

## Verification 驗證

- `just theory-unit`：全部 CHECK 通過，回報 check 數量。
- `just theory-vectors` 能重新產生 `vectors.h`，而且和 commit 的版本一致（`git diff --exit-code`）。
- 核心可以用 `-std=c99 -pedantic -Wdouble-promotion -Wfloat-equal` 乾淨編譯，`grep` 確認沒有 `float`、`double`，也沒有 `#include` libc（測試檔除外）。這樣之後放進 fm1-base 才不會撞到 no-double 規則。
- `just docs-check` 通過，新頁在 nav 裡，連結有效。
- 文件的外部說法都有來源，沒查核的標 `[調研]`。
