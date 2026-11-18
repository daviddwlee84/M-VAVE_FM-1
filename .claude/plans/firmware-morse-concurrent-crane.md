# THEORY v0.1：WHEEL（五度圈 + 和弦辨識）

## Context 背景

樂理核心（`firmware/theory/theory.c`，26,661 個檢查）和設計文件（`docs/dev/theory.md`）已經 commit（`d33b979`、`7731b3d`）。本人決定 v0.1 做 **WHEEL**：隨身的樂理參考，不打分數。

本人在 fm1-emu 試用了 Keyboard Academy：「效果還可以，但一直有琴鍵燈提示，所以非常簡單。」這對應到運動學習的 guidance hypothesis：提示太頻繁、太好用時，練習的表現很好，但學的人會依賴提示，拿掉之後就彈不出來。所以 WHEEL 的 LED 只顯示調，不帶彈；之後的卡片也要讓提示逐漸撤掉。這點要寫進設計文件。

已決定（2026-11-18，本人）：
- 名稱 **THEORY**。
- 順級和弦模式的白鍵預設是 **字母 = 根音**：D 大調按 F 是 F#m，按 C 是 C#dim。另一種「C 鍵永遠是 I」放在設定裡。

這一輪只做到 host simulator 和 fm1-emu，不刷實機。實刷另外走 preflight，並要本人批准。

## 機器上的行為

**畫面（240×240）**

- **Header**（y 0–31）：左邊是調名（`D major`、`E dorian`），右邊是調號（`2♯`、`3♭`），角落小字標出琴鍵模式 `NOTES` / `CHORDS`。
- **Main**（y 32–207，分兩個 240×88 的半張畫，因為 canvas 最多 240×144）：
  - **左邊 176×176 是五度圈。** 外圈是 12 個大調（S 字型，半徑約 70），內圈是關係小調，用小寫表示（XS 字型，半徑約 44），圓周用 `fm_sinf` / `fm_cosf` 算點，`cv_pset` 畫。
  - 目前調號所在位置的 c−1、c、c+1，外圈加內圈共 6 格，用 `C_AMB` 標出：這就是調內的 6 個大、小和弦。主音另外用 `C_HI` 框起來。
  - 剛彈的和弦如果是大或小三和弦家族，在它的位置畫圈。這樣副屬和弦、借用和弦會出現在鄰近 6 格之外，一眼看得出是調外的和弦。
  - **右邊 64 px 寬，列出 7 個順級和弦**：級數（T 字型）加和弦名（S 字型）。
- **Footer**（y 208–239）：剛彈的東西。
  - 和弦：用 M 字型顯示名稱（`F#m7/A`），旁邊是級數（`iii7`）和轉位。
  - 單音：音名加級數（`F#4 3`）。
  - 兩個音：音程（`M3`）。
  - 認不出來：列出音名。
  - 轉旋鈕改設定時，footer 暫時顯示 1.5 秒（例如 `MODE DORIAN`）。
- ♯ ♭ ° ø 用自己畫的小 bitmap（`gtext`）。app 在 include `theory.c` 之前把 `TH_GLYPH_*` 定義成 `\x01`–`\x04`，`gtext` 遇到這些位元組就畫 bitmap，其他文字交給 `cv_text`。`fm1.toml` 加 `font_full = ["M"]`，M 字型才有小寫（`m`、`b`）。

**操作**

| 操作 | 作用 |
|---|---|
| 轉 `SELECT` | 沿五度圈換調（調式不變） |
| 按住 `SEL` 再按琴鍵 | 跳到以那個音為主音的調（`th_key_for_pc`） |
| `K1` | 調式：大調、小調、和聲小調、旋律小調、dorian、phrygian、lydian、mixolydian、locrian |
| `K2` | 三和弦或七和弦 |
| `K3` | 順級和弦的標示：和弦名、Roman 或 Nashville |
| `K4` | 和弦鍵的對應：字母 = 根音（預設），或 C = I |
| `PLAY` | 切換琴鍵模式：**NOTES**（彈什麼就辨認什麼）或 **CHORDS**（白鍵彈順級和弦；`PLAY` 亮綠燈） |
| `OCT-` / `OCT+` | 發聲的八度，−2 到 +2，不影響辨認 |

**琴鍵與 LED**

- **NOTES 模式**：每個琴鍵發出自己的音，`th_identify` 辨認目前按住的所有鍵。一次按和弦的過程中，記住音最多的那一組，全部放開後畫面繼續顯示它。
- **CHORDS 模式**：
  - 白鍵的字母決定級數，`th_diatonic` 算出和弦。
  - `th_voice_near` 依前一個和弦選就近的轉位，範圍 G3–E5，另外加一個低八度的根音當 bass。
  - 放開琴鍵時，關掉這個鍵觸發的所有音。黑鍵在 v0.1 不出聲。
- **LED**（用 `firmware/common/ledfx`）：調內的音 glow，主音 mid，按住的鍵 lit。CHORDS 模式下，白鍵 glow，I 級的鍵 mid。
- 按鍵的音同時從 USB-MIDI 送出（`plat_midi_send`，note on / off），可以錄進 DAW。

**聲音**：新的 `synth.c`，從 `common/chime` 的 2-op FM 做法改寫，標示出處。
- 8 個 voice，不夠時搶走最舊的。
- 128 格的 phase increment 表在 `app_init` 用 `fm_exp2f` 算好，render 只用 `fm_sin_turns`。
- envelope：attack 3 ms，接著指數 decay 到 sustain，FM index 隨 envelope 變小（音色變暗，像電鋼琴）。note off 之後約 200 ms release。音量依 velocity（開 `APP_TOUCH`）。
- note event 用單一寫入者的 ring buffer 傳遞：只有主迴圈（`app_input_only` / `app_frame`）寫，render 在每個 block 開頭讀完。
- 混音每個 voice 約 0.12，總線過 `fm_tanhf` 軟限幅，再乘上 `master_q12`，然後直接寫進 `lr`（覆蓋，不是相加）。

**存檔**
- `enum { OBJ_PANEL, OBJ_THEORY, OBJ_NOBJ }`，`APP_STORE_MAP { 0xDC000u, 0xD8000u }`。
- `0xD8000` / `0xD9000` 在這台機器的 23 份備份裡都沒被寫過，而且不和 Morse 的 `0xDA000`（Morse 0.6 的學習進度在那裡）或面板校正的 `0xDC000` 重疊。
- 紀錄格式：magic `TH01`、版本、長度，加上調、調式、三或七和弦、標示方式、對應方式、琴鍵模式、八度。設定改了 3 秒後存檔，和 Morse 一樣。讀回時 magic 或範圍不對就用預設值。

## 要新增或修改的檔案

**新增在 `firmware/theory/`**
- `fm1.toml`：
  - `name = "THEORY"`、`id_prefix = "FM-1_49"`（dev 版是 `FM-1_4900`）。這個前綴查過 Hub catalog 沒人用，也避開了 LOOM-8 佔用的 1x。
  - `usb_product = "THEORY FM-1"`、`include = ["../common"]`、`words = "words.tsv"`、`font_full = ["M"]`、`ble = false`。
  - `licence = "GPL-3.0-or-later"`，以及 tagline、description、credits（fm1-base、chime 的做法、music21 只是測試工具）。
- `app.h`：`APP_NAME`、`APP_TOUCH`、`APP_STORE_MAP`、物件的 enum，以及 `#ifdef PLAT_HOST` 裡的 `APP_HOST_EXPECT` / `APP_HOST_CMD` / `APP_HOST_FAILS`。
- `app.c`：
  - fm1-base 的必要 hook：`app_init`、`app_render`、`app_panic`、`app_ui_init`、`app_frame`、`app_input_only`、`app_say`。
  - 狀態、輸入（`plat_keys` 遮罩比對、`plat_buttons`、`plat_enc`，沒用到的 encoder 也每個 frame 讀掉）、存檔、`R_HEAD` / `R_MAIN` / `R_FOOT` 局部重畫（照 Morse 的 `draw()`）。
  - host simulator 的 `app_expect`：`tonic`、`mode`、`keymode`、`sevenths`、`labels`、`mapping`、`oct`、`voices`、`lit_keys`、`dim_keys`、`glow_keys`。
  - host simulator 的 `app_host_cmd`：`expect_text chord|roman|footer|key TEXT`，寫法照 Morse 的 `expect_text`（`firmware/morse/app.c` 約 3322 行）。
- `synth.h` / `synth.c`：8 voice 的 FM 電鋼琴，event ring，`synth_active()`。
- `gtext.h` / `gtext.c`：♯ ♭ ° ø 的 1-bit bitmap（16 px 和 24 px 兩種大小），`gtext()` 和 `gtext_w()`。
- `words.tsv`：螢幕閱讀器的字，第一行是 `# url:`，只能用 ASCII（格式照 `firmware/voicelab/words.tsv`）。
- `tests/basic.txt`：host simulator 腳本，內容見下面的驗證。
- `.gitignore`：`build/`、`web/words/`（同 `firmware/conductor/.gitignore`）。
- `LICENSE`：GPL-3.0 全文，從 `firmware/morse/LICENSE` 複製。

**沿用、不改**：`theory.c` / `theory.h`、`firmware/common/ledfx`、fastmath。

**其他**
- `Justfile`：
  - `theory-emu chip="uc25hq80"`：用 `just emu-run firmware/theory/build/theory.fwsc`。
  - `theory-unit` 不變。
- 文件：
  - `docs/dev/theory.md`：
    - 狀態改成 v0.1。
    - Keyboard Academy 試用心得，加上「提示要逐漸撤掉」的原則，來源是 guidance hypothesis（Salmoni, Schmidt & Walter 1984）。只讀到二手資料，例如 [Frontiers 2010](https://www.frontiersin.org/articles/10.3389/fpsyg.2010.00190/full)，所以標 `[調研]`。
    - WHEEL 最後定下的操作（`K4` = 對應方式）。
    - synth、識別碼 `FM-1_49`、存檔位置 `0xD8000`。
    - 驗證結果標 `[模擬]`。
  - `docs/hardware/flash-map.md`：加一列 THEORY 的 `0xD8000–0xD9FFF`，附 rehearse 的結果。
  - `firmware/theory/README.md`：說明現在是 app 了。
  - `AGENTS.md`：layout 那一列（現在是 app，識別碼 `FM-1_49`）和指令（`fw-test theory`、`theory-emu`）。
- 不 commit，除非本人要求。

## Verification 驗證

1. **`just theory-unit`**：核心沒變，26,661 個檢查照樣全過。
2. **`just fw-test theory`**：host simulator，`tests/basic.txt` 全部 `ok`，截圖輸出到 `firmware/theory/build/host/`。內容：
    - 開機截圖；`expect tonic 0`。
    - `turn SELECT 2` → `tonic 2`，`expect_text key D major`，截圖。
    - NOTES 模式下 `key 7/11/14 down`（C4、E4、G4）→ `expect_text chord C`、`roman I`。先 `peakreset` 再 `expect peak_db_min -40`，確認有聲音。
    - C/E 的轉位，以及 D7 → `V7/V`。
    - 單音 → `footer` 顯示音名和級數；放開所有鍵後，footer 保留剛才的和弦。
    - `lit_keys` / `glow_keys` 的位元遮罩符合 D 大調音階。
    - `tap PLAY` → `keymode 1`；`tapkey w4`（C4）在 D 大調是 `C#dim`，`tapkey` D 鍵是 `D`；`voices` 是 4。
    - `turn K4 1` → C = I：`tapkey w4` 是 `D`。
    - 按住 `SEL` 再按 Eb 鍵 → `tonic -3`。
    - `turn K1` → 調式變了，順級和弦的列表也跟著變。
    - `turn K2` → `roman IM7`。
    - 等 3 秒 → `expect store_writes >0`；`reboot` → 所有設定都讀回。
    - 截圖：五度圈、CHORDS 模式、♯♭ glyph。
3. **`just fw-build theory`**：build 出 `firmware/theory/build/theory.fwsc`，自帶的檢查要全部 ok（沒有 soft-double，大小在 581,564 B 以內）。
4. **fm1-emu 兩種晶片**（`uc25hq80`、`p25q80`）：
    - `just emu-shot` 開機，記下寫入次數。
    - 用 `FM1_BENCH_KEYS` 腳本轉 `SELECT`、切 `PLAY`、等待存檔，再用同一個 `--flash` 檔重開，截圖確認調和模式都讀回。
5. **`just emu-rehearse`**：疊在最新的實機備份上（`backups/` 裡最新的那份，目前是 `2026-11-18-pre-morse-0.5`）。回報開機和存檔時各寫了哪些 sector：預期只有 `0xD8000` / `0xD9000`，而且 `0xDA000` 的 Morse 紀錄要完整保留。
6. **`just docs-check`** 通過。
7. 回報結果之後，如果本人想刷實機，再另外提出完整的 preflight（備份與 SHA-256、晶片 `0xB36014`、套件 SHA-256、目前是 Morse 0.6 和這次寫入會改變什麼），等本人批准。
