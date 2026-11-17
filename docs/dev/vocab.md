# 背單字 app（構想草案）

把 FM-1 變成像 [WordBit](https://play.google.com/store/apps/details?id=net.wordbit.entw) 那樣、隨手拿起來背幾個字的口袋機：有發音、有學習曲線（spaced repetition），能匯入 Anki。延續[摩斯電碼機](morse.md)的字卡經驗。

**狀態（2026-11-18）：只有構想與語音試聽，還沒寫 app。** 發音方案的比較見[裝置端語音合成](tts.md)。

**v1 的發音（2026-11-18 決定）**：離線用 Klatt，同時顯示拼字與 KK 音標；接電腦時改用電腦的 TTS，經 USB audio 從 FM-1 喇叭播。本人聽完試聽頁的感想是「可能確實差強人意」，所以保留升級的路：常錯的字另外放 ADPCM、自己寫 LPC 編碼器、或等 m-card（microSD）可用後改成預錄。

## WordBit 的體驗，對到 FM-1 上 `[調研]`

WordBit 的核心（Play 商店頁打不開，資料來自第三方介紹）：

- 每次解鎖手機就跳出一張卡或一題選擇題。
- 單字附例句、KK 音標、TTS 發音。
- 自己把字分成「不會、混淆、會了」等幾堆。
- 依遺忘曲線在 1、7、15、30 天後複習。
- 題型有配對、選擇、拼字。

| WordBit | FM-1 上 |
|---|---|
| 解鎖就出現一張卡 | 開機直接是今天該複習的第一張卡 |
| 不會 / 混淆 / 會了 | 四個按鈕評分 Again / Hard / Good / Easy，同時是 FSRS 的評分 |
| 選擇題 | 四選一，琴鍵選答案（Morse 的認字題已經這樣做） |
| 拼字題 | 琴鍵當字母鍵：F3..F#5 = A..Z，G5 = Enter（和 SPELLBOX 同一套排法），先聽發音再拼 |
| 鎖屏輪播 | 瀏覽模式：放著不動就自動念下一個字並顯示意思 |
| TTS 發音 | `PLAY` 再念一次；方案見[語音合成](tts.md#results) |

## 學習曲線

- **排程用 FSRS-6**。把 [py-fsrs](https://github.com/open-spaced-repetition/py-fsrs)（MIT）的 scheduler 移植成 C，數百行，只需要 `expf`、`powf`（FPU 是單精度，夠用），用 py-fsrs 產生的參考數據測試。官方沒有嵌入式 C 版 `[上游]`。
- **每張卡的狀態約 8 B**：stability、difficulty 量化成 16 bit，加上狀態與到期日。3,000 張卡約 24 KB。
- **畫面上畫遺忘曲線**：評分後顯示這張卡的 R(t) 曲線和下次複習日；統計頁顯示每天複習量與各狀態的卡數。
- **沒有 RTC**：FM-1 關機後不知道過了幾天（只有 179 秒就繞回來的計時器）。接電腦時由電腦送日期；離線時沿用最後一次同步的日期，提供手動「換到下一天」。Morse 用「回合」代替天數，也是同一個原因。

## 內容來源與 licence

| 來源 | 用途 | Licence | 進公開 repo？ |
|---|---|---|---|
| [NGSL](https://www.newgeneralservicelist.com/new-general-service-list)（約 2,800 字） | 內建通用字表 | CC BY-SA 4.0 | 可以，資料另放一個目錄並附 LICENSE |
| [ECDICT](https://github.com/skywind3000/ECDICT) | 釋義、音標、詞頻、考試標記 | MIT（上游來源的授權沒寫清楚 `[調研]`） | 可以 |
| [OpenCC](https://github.com/BYVoid/OpenCC) `s2twp` | ECDICT 的簡體釋義轉成台灣繁體與用語 | Apache-2.0 | 只是工具 |
| [CMUdict](https://github.com/cmusphinx/cmudict) | 發音（給 Klatt），也能轉成 KK | BSD | 可以 |
| 自己的 Anki deck | 主要內容 | 自己的 | 不放（git-ignored 的 `decks/`） |
| 多益、全民英檢、大考 6000 字等考試字表 | 考試用 | 大考中心限非營利、要標示；LTTC 沒寫授權 `[調研]` | **不行**，只放本機 `decks/` |

## Anki 匯入 {#anki}

- **`.apkg` 是 zip** `[上游]`。有 `meta`（protobuf）時是新格式：zstd 壓縮的 `collection.anki21b`，媒體檔也壓縮；沒有 `meta` 時是舊格式 `collection.anki21` 或 `collection.anki2`（SQLite，不壓縮）。
- **欄位**：`notes.flds`，以 `0x1f` 分隔；欄位名稱在 `fields` 表（新）或 `col.models` JSON（舊）。
- **既有排程**：`cards` 的 `ivl`、`factor`、`due`、`reps`、`lapses`；FSRS 的記憶狀態在 `cards.data` 的 JSON（`s`、`d`）。匯入時直接拿來當 FM-1 上的初始狀態。
- **讀取方式**：自己寫一個 MIT 的小 reader（`zipfile` + `zstandard` + `sqlite3` + 很小的 protobuf 解碼）。官方的 `anki` 套件是 AGPL，只能當選用依賴。
- **雙向同步**：Anki Desktop 開著時用 [AnkiConnect](https://git.sr.ht/~foosoft/anki-connect)（GPL，HTTP API）：`cardsInfo` 讀、`answerCards` 或 `insertReviews` 把 FM-1 上的複習寫回去。`answerCards` 會不會把時間記成同步當下，還沒驗證 `[調研]`。

## 畫面：中文與 KK 音標

FM-1 的字型只有 ASCII，KK 音標的 `ə ɪ æ θ ð ʃ ŋ` 也不在裡面。

- **只把 deck 用到的字做成 subset 字型**：用 [Cubic 11](https://github.com/ACh-K/Cubic-11)（繁體中文 11×11 點陣，OFL-1.1）。2,500 個字 1-bit 約 40 KB，KK 符號另外幾十個。
- 卡片文字存成 glyph 編號（中文 2 B 一字），由電腦在匯入時決定。
- Conductor 的做法（電腦把每行畫成 240×16 的點陣，549 B 一行）適合連線即時顯示，不適合離線存幾千張卡。

## flash 預算（3,000 字）

| 項目 | 大小 |
|---|---|
| app 程式 + 字型（Morse 0.3 約 128 KB） | 約 130 KB |
| Klatt 合成器 | 幾十 KiB |
| 單字 + 音素 + 中文釋義（glyph 編號），不含例句 | 約 120 KB |
| 加上例句與例句翻譯 | 約 300 KB |
| 中文 subset 字型 | 約 40 KB |
| FSRS 狀態 | 約 24 KB（存在資料區，A/B 兩份） |

不含例句的版本可以整個編進 581,564 B 的韌體槽；含例句就需要下面的「資料區」或壓縮（fm1-base 有 `tools/lz4blk.py`）。

## 字卡怎麼進機器 {#delivery}

1. **v1：編進韌體映像。** 換 deck = 重新 build + 刷寫，每次都要本人同意；只寫韌體區（和 Morse 升級一樣），不碰 SLOOP 的資料，完全符合現有的安全規則。
2. **選用：app 自己寫 flash 資料區。** 用 USB 隨時推送新 deck，不必重刷。這條路**要先修改 AGENTS.md** 的「不直接 erase/program」規則，等真的要做時再請本人核准。規則草稿：
    - 只能用 fm1-base 的 `plat_data_erase` / `plat_data_write`（有 `PLAT_DATA_OK` 邊界檢查），不能自己算位址以外的範圍。
    - deck 區是 app.h 裡宣告的固定 sector 範圍，必須是最新備份裡確認為空、且不和 SLOOP 的 project / sample、Morse 的 `0xDA000` 重疊的 sector。
    - 改 deck 區之前先跑 `just emu-rehearse`，並更新 [flash 配置圖](../hardware/flash-map.md)。
    - 寫入的資料先算 CRC，寫完讀回比對，失敗就當作沒有 deck。
3. **只在連線時串流**：電腦即時送卡片，排程直接用 Anki 的。完全不寫 flash，但離開電腦就不能用；可以當作第一個試做版。

fm1-base 的 resource region（`resources = FILE`）不適合拿來放可替換的 deck：build 會把資料的長度與 CRC 編進韌體，換資料仍然要重 build。

## Roadmap

1. 語音方案定案（聽完 `just voice-open` 的試聽頁）。如果選 Klatt，要先把 Moonshine 的 C++ 移植成 C（fm1-base 的 app 是 C）。
2. 電腦端：Anki `.apkg` reader、NGSL + ECDICT + OpenCC 的內建字表產生器、subset 字型產生器、FSRS 參考數據。
3. 韌體 v1：開機卡片、四鍵評分、選擇題、拼字、遺忘曲線、FSRS，deck 編進映像。
4. 同步：日期、複習紀錄、AnkiConnect 寫回。
5. 之後如果要上 Firmware Hub，比照 Morse 拆成獨立 repo 再掛成 submodule（見 [Hub](../firmware/hub.md)）。
