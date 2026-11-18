# THEORY v0.2：MENU、TODAY、CHART、CHORD、進度備份

## Context 背景

THEORY 0.1（WHEEL）已經在實機上（commit `1a8f7c6`）。v0.2 要加上「學」的部分。本人 2026-11-18 決定：

- **TODAY**：間隔複習的閃卡回合，四種卡片都要。
  - 樂理：音階、調號、關係小調。
  - 樂理：級數和弦。
  - 聽音：級數。
  - 聽音：和弦性質。
- **CHART**：熟練度圖。
- **CHORD 頁**：本人提的「自由按鍵、判斷是什麼和弦、ChoralRoot 風格顯示」。
- **開機頁**比照 Morse：預設是 MENU，SETTINGS 可以改。第一次開機（包括從 0.1 升級）先請使用者選偏好的開機頁，選完存起來。
- **進度備份**比照 Morse 自己寫：hostlink SysEx，可讀的 JSON，電腦端 CLI 加一個網頁。
- 本人問「卡片混在一起會不會效果不好」。答案寫進設計：
  - 交錯練習主要幫助分辨相似的類別（Brunmair & Richter 2019 `[調研]`；Wong 2021），新內容要先集中介紹（Carter & Grahn 2016）。
  - 所以回合採用「主題式 + 交錯」：新解鎖的調先集中出，每回合分成樂理段和聽音段，段內不同的調交錯；開始畫面可以選只練一個主題（FOCUS）。

設計和照搬的對象是 Morse（同一個作者，GPL）：`firmware/morse/learn.c`、`app.c` 的 PRACTICE / CHART / ASK、host 的 progress 協定、網頁的 port 搜尋。

## 頁面與流程

`ALGO` 切換頁面，`HOME` 在 MENU 和上一頁之間切換，和 Morse 一樣。頁面依序是 MENU、TODAY、WHEEL、CHORD、CHART、SETTINGS。

- **第一次開機**：設定紀錄裡「開機頁」那個 byte 是 0（新機器，或從 0.1 升級上來）時，先顯示 `START PAGE?` 選單。`SELECT` 選，`PLAY` 確認，選完存起來；之後開機直接進那一頁。
- **SETTINGS**：開機頁（MENU / TODAY / WHEEL / CHORD / CHART）、每回合張數（10–30，預設 15）、cadence 的速度。
- 設定照 Morse 的做法原地沿用 0.1 的 `rsv` byte，不升 `REC_VER`：`rsv[0]` 是開機頁（0 = 還沒選），`rsv[1]` 是每回合張數（0 = 15），`rsv[2]` 是速度。0.1 的設定可以保留，裝回 0.1 也讀得懂。

## TODAY

**項目**：一共 63 個，依解鎖順序列在表格裡，只能往後加，不能改順序，備份才對得上。

| 種類 | 題目 | 怎麼作答 | 每個調一項 |
|---|---|---|---|
| SCALE | `D major scale` | 依序彈 7 個音，不分八度。每彈對一個音就拼在畫面上，彈錯當下就算錯 | 12 個大調 |
| KEYSIG | `E major: key signature?` | 按出所有升降的音，順序不拘，可以一次按或分開按。按到不該按的音當下就算錯 | 11 個（C 沒有） |
| RELMIN | `relative minor of Bb?` | 按出它 | 12 |
| CHORDS | `IV in Eb` | 按出和弦音，不分八度、不分轉位。每次隨機一個級數 | 12 |
| EAR_DEG | 畫面寫調名；先放 I–IV–V–I，再放一個音 | 按出那個音。box 0–1 只出 1、3、5 級，之後出 7 個級數 | 12 |
| EAR_Q | 聽一個和弦（根音隨機） | 用 E4 F4 G4 A4 從已開放的大、小、減、增裡選（同 Morse 的 `CHOICE_KEY`），也可以用 `SELECT` + `SEL` | 大、小、減、增各一項 |

**解鎖**：
- 以調為單位分組，順序是 C、G、F、D、Bb、A、Eb、E、Ab、B、Db、F#（ABRSM 式的螺旋）。
- 第一組是 C，加上 EAR_Q 的大、小。開到 D 時加減和弦，開到 A 時加增和弦。
- 已開放的項目都到 box 2 以上，才開下一組。

**回合**：
- 新卡（每回合最多 3 張）先集中出，不打散。
- 接著是樂理段，再來是聽音段，段內打散，同一個調不連續出現。
- 弱卡（box 0–1）在回合末再出一次；答錯和偏慢的卡也在回合末重問。
- FOCUS 選項（開始畫面轉 `SELECT`）：ALL / THEORY / EAR。

**評分**：
- 從可以作答那一刻開始計時。
- 錯，或用了 `SEQ` 偷看：box 回到 0。
- 對但慢（超過門檻，或聽音卡重播兩次以上）：box 不動，回合末再問。
- 對：box +1。
- 又快又對，而且 box ≥ 1：box +2。

**畫面**：
- 開始畫面：`ROUND n`、到期 / 新卡 / 已開放的數量、streak、「下一個調還差哪些項目」、`PLAY: START`。
- 卡片：題目，答到一半時的進度（彈對的音拼出來）。
- 結果：`RIGHT` / `SLOW` / `NO`，加上拼好的答案，並播出答案的聲音。
- 回合結束：統計，以及有沒有開放新的調。

**LED 不帶彈**（guidance hypothesis）：作答時只有按住的鍵亮；選擇題亮出 4 個選項鍵；答完之後才亮出答案。

**按鍵**：琴鍵作答（照樣發聲）；`PLAY` 開始或重播；`SEL` 不會；`SEQ` 偷看（算錯）。存檔時機和 Morse 一樣：回合結束、中途離開回合時。

## CHART

- 五度圈上每個調的顏色代表它所有項目的平均 box，色階沿用 Morse 的 `level_rgb`；還沒開放的調是暗的。
- 轉 `SELECT` 移動游標，旁邊列出那個調各種卡片的 box 條，下方是 EAR_Q 四項。
- v0.2 只能看，不能編輯。

## CHORD 頁

自由彈，全畫面顯示：
- **大字和弦名**（L 字型，延伸音用小一號的字）。
- **組成音**：拼對音名，並各自標出它是和弦裡的 R、3、b3、5、b7、7、9…。
- 在 WHEEL 設定的調裡是第幾級，有沒有出調。
- **其他讀法**：例如 `also Am7/C`。
- **27 鍵的小鍵盤圖**，標出按住的鍵。
- **最近 4 個和弦的級數軌跡**：例如 `I  V  vi  IV`。

## 核心與聲音

- `theory.c` 加兩個函式，並寫 unit test：
  - `th_identify_all()`：列出依偏好排序的所有候選讀法，`th_identify` 改成取它的第一個。
  - `th_tone_label()`：從 fifths 位移得到 R、b3、3、5、b5、#5、6、bb7、b7、7、9、4、#11 這類標籤。
  - 減類和弦（dim、dim7、m7b5）和增類和弦已經認得。dim7 和 aug 是對稱和弦，CHORD 頁的「其他讀法」會列出其他根音的讀法（例如 `Ddim7/B`）。
  - 選配：和弦表補上 `6/9`、`7b9`、`7#9`、`13`（13 允許省略 11 和 5），music21 的對照 vector 跟著加，辨認的偏好順序放在現有和弦之後。
- **新的 `srs.c` / `srs.h`**（GPL，platform-free）：
  - 從 Morse `learn.c` 的 `ln_build`、`ln_answer`、`ln_finish`、`ln_due`、`ln_rand`、blob 和驗證照搬，把「字元 + 方向」換成「項目編號」。
  - 新增：組別的結尾表、分段（樂理 / 聽音）、同一個調不連續、FOCUS 篩選、Easy +2、`static` 暫存區（避免堆疊用太多）。
  - 寫 unit test `tests/srs_test.c`，由 `just theory-unit` 一起跑。
- **`synth.c` 加 `synth_score()` / `synth_score_busy()`**：
  - 主迴圈寫入一段「時間、音、長度」的樂譜，render 依 sample 數消化。
  - 保持單一寫入者，不需要 tick，也不受重畫影響。
  - 用來播 cadence 加目標音，以及答案的聲音。

## 進度備份（比照 Morse）

- **協定**：
  - 框架是 `F0 7D 54 48 01 <type> … F7`，id `54 48`（"TH"）沒有人用。
  - FM-1 送出：HELLO `41`、HEARTBEAT `42`、PROG `44`、PROG_DONE `45`。電腦送出：HELLO_ACK `01`、PROG_REQ `04`、PROG_LOAD `05`。
  - 紀錄的格式是：format byte + 設定 + 進度 + CRC-16/CCITT-FALSE，切成 49 B 的 chunk，每 7 B 打包成 7-bit，大約 5 個 frame。
  - 載入時 FM-1 先在螢幕問（`PLAY` 是 / `HOME` 否），損壞的資料拒收。
  - 韌體端用 `firmware/common/hostlink`（`hl_init` / `hl_tick` / `hl_match` / `hl_send`）。`APP_SYSEX` 寫法照 Morse 的 `host_frame` 和 `prog_*`。
- **存檔**：進度存在新的物件 `OBJ_PROGRESS`，位置 `0xD6000`（這台機器的所有備份裡都是空的）。設定紀錄不動。`APP_STORE_MAP` 改成 `{0xDC000, 0xD8000, 0xD6000}`。
- **電腦端**：`host/theory/`（GPL）。
  - `protocol.py`：frame、CRC、pack7、chunk。
  - `progress.py`：blob 和 JSON 互轉。JSON 是 `fm1-theory-progress` v1，項目用穩定的名稱，例如 `scale:D`、`earq:dim`。
  - `device.py`：依協定的 header 找 port，不依名稱，因為 macOS 把這台叫 `Felucca`；收到 HEARTBEAT 也算連上並回 ack；逾時就提示「關掉其他 MIDI 程式」。
  - `__main__.py`：`backup` / `restore`。
  - 通用的部分從 `host/morse`（MIT）複製過來並標示出處。
- **網頁**：`docs/assets/theory/progress.html`（GPL，Web MIDI，只走 USB）。
  - 依 header 找 port；備份是下載 JSON，還原是上傳 JSON 再送出 frame。
  - 從 `docs/dev/theory.md` 用按鈕連過去，做法照 SLOOP editor 頁。
- `Justfile`：`theory-test`（pytest）、`theory-backup FILE`、`theory-restore FILE`。

## 檔案

- **修改**：
  - `firmware/theory/app.c`：分頁、MENU、SETTINGS、第一次開機、TODAY、CHART、CHORD、hostlink、`app_expect` 的新值，以及 host 指令 `answer right|wrong|slow`、`answer_all`、`setcard`。
  - `app.h`（`APP_SYSEX`、物件）、`fm1.toml`、`theory.c` / `theory.h`、`synth.c` / `synth.h`、`tests/basic.txt`、`tests/theory_test.c`。
- **新增**：
  - `firmware/theory/srs.c` / `srs.h`、`course.h`（項目表與組別）、`tests/srs_test.c`。
  - `host/theory/{__init__,__main__,protocol,progress,device}.py`、`host/tests/test_theory.py`。
  - `docs/assets/theory/progress.html`。
- **文件**：
  - `docs/dev/theory.md`：
    - TODAY / CHART / CHORD / MENU 的實際設計和按鍵。
    - 「主題式 + 交錯」的理由和來源。
    - 協定、v0.2 驗證、roadmap。
  - `docs/hardware/flash-map.md`：加 `0xD6000`。
  - `AGENTS.md`：layout 和指令。
  - `README.md`、`docs/index.md`：狀態。
- 韌體版本改成 0.2（HELLO 帶版本）。

## 實作順序（每一步都要通過測試才往下）

1. 核心加 `th_identify_all` / `th_tone_label`，寫 `srs.c` 和 `course.h`，加上 unit test。
2. 分頁骨架：MENU、SETTINGS、第一次開機、設定的 `rsv` byte；WHEEL 照舊。
3. `synth_score`；TODAY；CHART。
4. CHORD 頁。
5. hostlink 加進度協定，接著電腦端和測試，最後網頁。
6. fm1-emu、預演、文件。

## Verification 驗證

- **`just theory-unit`**：核心照樣全過，加上新的 srs 檢查：
  - 新卡不超過 3 張；到期判斷正確。
  - 錯回 0、慢不動、Easy +2。
  - 同一個調不連續。
  - FOCUS 篩選有效。
  - 組別依門檻開放。
  - blob 往返，壞資料被拒。
- **`just fw-test theory`**：
  - 新機器開機出現開機頁選單，選 TODAY 後重開直接進 TODAY；0.1 的紀錄升級後保留調和調式，並出現一次選單。
  - 跑一回合：新卡；`answer right|wrong|slow` 對 box 的影響；回合結束，round +1；進度重開後還在；`store_writes` 有上限。
  - KEYSIG、SCALE 實際按鍵的判斷，包括彈錯當下就算錯。
  - EAR_DEG 播放時 `voices > 0`，播完才能作答。
  - CHART、CHORD 截圖；CHORD 依序彈 C、G、Am、F 後，軌跡是 `I V vi IV`，還有 `also Am7/C`。
  - SysEx：PROG_REQ 送出紀錄，PROG_LOAD 要 `PLAY` 確認，壞資料被拒；測試 vector 和電腦端共用。
- **`just theory-test`**：協定 vector、JSON 往返、壞資料、對模擬裝置的備份和還原。
- **`just fw-build theory`**：在大小上限內，沒有 soft-double。
- **fm1-emu 兩種晶片**：改設定、玩一回合、重開讀回。
- **預演**：疊在最新的實機備份上，預期只寫 `0xD6000` / `0xD8000`，Morse 的 `0xDA000` 不變。
- **`just docs-check`** 通過。
- 刷實機另外走 preflight，等本人批准。不 commit，除非本人要求。
