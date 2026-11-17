# FM-1 裝置端 TTS 調研 + 語音試聽 spike（背單字 app 的前置）

## Context 背景

你想知道 FM-1 能不能跑極小型的 edge TTS，如果可以，就沿用 Morse 的經驗做一個類似 WordBit 的背單字 app：要有學習曲線（spaced repetition），也要能匯入 Anki。

這一輪依你的選擇做兩件事：
- 把調研寫進 `docs/`
- 做語音試聽，各種方案都聽聽看

app 本身這一輪只寫構想，不寫程式。你目前不打算發佈，licence 等要發佈時再定。不過 **repo 是公開的**，commit 進去就等於公開，所以這一輪只 commit MIT 程式碼。GPL 引擎留在 git-ignored 的 `vendor/`，或是直接下載對方發佈的 `.fwsc` 來聽。

## 調研結論 Findings（會寫進 docs）

**硬體預算**
- JieLi AC791N 雙核，360 MHz，單精度 FPU。
- app 有 96 KB `.data/.bss` 加上 336 KB pool（`third_party/fm1-base/README.md:143-146`）。
- 韌體槽 581,564 B，Morse 0.3 用了 127,880 B，剩大約 450 KB。
- 有喇叭和 3.5 mm 耳機孔，`app_render` 輸出 44.1 kHz。
- 字型只有 ASCII，中文和 KK 音標需要另外做 glyph subset。

**FM-1 上已經有 5 個會講話的韌體**
| 韌體 | 做法 | Licence |
|---|---|---|
| Speak37 | espeak-ng 預先轉成 LPC，FM-1 版收 2576 字 | GPL-3.0 |
| SPELLBOX | LPC + 12k 字典 + 字母轉音規則；只支援舊 flash 晶片 | GPL-3.0 |
| 386DX | eSpeak NG 前端 + Klatt，在 96 KB RAM 內 | GPL-3.0 |
| Chimera Vocal | 聲道物理模型 | MIT |
| EVIL GENIUS | SAM | SAM 本身授權不明 |

沒有任何 FM-1 韌體做字卡、單字或 Anki。

**通用 TTS 都放不下**
- eSpeak NG 約 1.4 MB、Flite 約 2.3 MB、Pico 需要 1 MB 以上 RAM，神經網路 TTS 是 MB 到百 MB 級。
- 放得下的有三條路：
  1. **Moonshine Klatt**：MIT，C++17，程式幾十 KiB，RAM 幾 KiB。每字只存約 10 B 音素，字數幾乎無上限。
  2. **預錄 LPC**：每字約 100 B，大約 2500–3000 字。
  3. **ADPCM 11–16 kHz**：音質最好，但每字 3–5 KB，只夠幾十到一百字。

**排程**
- FSRS-6 從 py-fsrs（MIT）手動 port 成 C。每張卡的狀態約 6–12 B，需要 `expf`/`powf`。
- 機器沒有 RTC，所以日期要在連線時由電腦給。

**Anki 匯入**
- `.apkg` 是 zip。新版是 zstd 壓縮的 `collection.anki21b` 加上 protobuf `meta`。
- 自己寫一個 MIT reader 就夠：`zipfile` + `zstandard` + `sqlite3`。
- 雙向同步用 AnkiConnect。

## 這一輪的步驟 Steps

### Step 1 — 調研文件（`docs/`，繁體中文，依規則加標籤）

1. **新增 `docs/dev/tts.md`「裝置端語音合成」**
   - 硬體預算
   - 引擎比較表與預錄格式比較表（每字 bytes、300 KB 放得下幾字）
   - 社群會講話的韌體
   - JieLi SDK 的發現：AC79 只有雲端 TTS，提示音是 MP3
   - licence 注意事項：GPL 不能和 JieLi 閉源 BLE 函式庫一起散佈
   - 標籤：`[上游]` 附連結，`[調研]` 用於未驗證的項目
   - 試聽結果在 Step 2、3 之後補上
2. **新增 `docs/dev/vocab.md`「背單字 app 構想（草案）」**
   - **WordBit 對照**：開機就出現一張卡（取代鎖屏）、四個按鈕評分（Again/Hard/Good/Easy）、四選一、拼字（A–Z 放在琴鍵上，同 SPELLBOX 的排法）、自動播放的瀏覽模式。
   - **學習曲線**：FSRS 的遺忘曲線 R(t) 加上下次複習日；統計頁顯示每日複習量和卡片狀態。
   - **內容來源與 licence**：
     - 內建 NGSL（CC BY-SA 4.0，資料另放並附 LICENSE）
     - ECDICT 釋義（MIT），用 OpenCC `s2twp` 轉成台灣繁體
     - 自己的 Anki deck
     - 考試字表（多益、全民英檢、大考）只放本機 git-ignored 的 `decks/`，不進公開 repo
   - **中文與 KK**：只把 deck 用到的字做成 subset，用 Cubic 11（OFL-1.1）。2500 字 1-bit 約 40 KB。
   - **字卡傳輸**：
     - v1 編進韌體映像，換 deck 就要重刷，只寫韌體區。
     - 你同意「可選擇使用 flash 資料區」：
       - 這條路要先修改 AGENTS.md 的「不直接 erase/program」規則。
       - 規則草稿寫在文件裡：只能用 `plat_data_*`，寫在一段事先宣告、並且在最新備份裡確認是空的 sector 範圍；要先跑 `just emu-rehearse`，也要更新 `flash-map.md`。
       - 這一輪**不改規則**，等真的要做時你再核准規則的文字。
   - **同步**：連線時電腦送日期給機器，機器回傳複習紀錄，再透過 AnkiConnect `answerCards`/`insertReviews` 寫回 Anki。
   - **flash 預算**與分階段 roadmap。
3. **其他文件與設定**
   - `zensical.toml` 的 nav 加入這兩頁。
   - `docs/resources.md` 的版本表加上 Step 2 下載的韌體。
   - 修正過時描述：`docs/dev/index.md:5`「還沒刷進實機」，以及 `AGENTS.md` 的 "No own build has been flashed"。實際上 Conductor 0.1/0.2、Morse 0.1–0.3 都已經刷過，見 `docs/device-log.md`。
   - `AGENTS.md`：Layout 和 Commands 加上 voicelab 相關的列。

### Step 2 — 在模擬器裡試聽社群的會講話韌體（不碰實機）

- **新增 `just voice-fetch`**
  - 照 `Justfile` 裡 `fm1-fetch-more` 的 `fetch()` 模式：`gh release download` 之後比對 release digest。
  - 對象是 Speak37（FM-1 版）、386DX、Chimera Vocal、EVIL GENIUS，SPELLBOX 有 release 才抓。
  - 版本號放進 Justfile 變數。實作時用 `gh release view` 確認 tag。沒有 digest 或找不到 FM-1 版的，就回報，不加進來。
- 依 AGENTS.md，每個都先用 `just emu-shot` 確認偵測到的是 FM-1 板，不是 SMK-37。
- 你用 `just emu-run downloads/X.fwsc` 實際玩。SPELLBOX 用 `p25q80` 晶片。
- 先確認 fm1-live 視窗會出聲。不會的話，在 tts.md 記下這個限制。
- 你的感想寫進 tts.md，標 `[模擬]`。

### Step 3 — Mac 上的語音實驗室 `host/voicelab/`（Python，MIT）

**測試字**：`host/voicelab/words.txt`，約 20 個字加 2 句，挑容易混淆的音：
- think/sink、three/tree、ship/sip、very/berry、light/right
- world、thought、measure、judge、rhythm
- vocabulary、pronunciation、comfortable、schedule、necessary
- 句子："Practice makes perfect."

**各方案的輸出**一律是 `build/voicelab/<engine>/<word>.wav`（git-ignored）：
1. **參考音**
   - macOS `say`，等於連線時用 USB audio 播的音質。
   - Kokoro（Apache-2.0）用 `uv run --with` 選用，因為要下載約 330 MB，預設不跑。
2. **Klatt（Moonshine，MIT）**
   - pinned commit clone 到 `vendor/moonshine`，用 CMake 做 host build。
   - 寫一個小的 C++ driver（`host/voicelab/klatt_render.cpp`，MIT，會 commit），呼叫 `BeginIpa`，並且也拿 `BeginText` 的內建 G2P 來比較。
   - IPA 來源是 CMUdict（BSD，抓到 `vendor/`），在 Python 裡把 ARPAbet 轉成 IPA。
3. **386DX 引擎（GPL-3.0）**
   - 依它的 README，clone 到 `vendor/`，加上 espeak-ng 1.52.0，用 `DX_HOST` build。
   - 它是唱歌引擎，用單一平音念字。
   - GPL 的程式碼和 driver 都只留在 `vendor/`，不 commit。
4. **LPC（TMS5220 式）**
   - 參考音轉 8 kHz，用 python_wizard（MIT，放 `vendor/`）編成 LPC frames。
   - 解碼器自己寫：**`firmware/common/lpc.{c,h}`（MIT，不依賴平台，host 和裝置共用）**。
   - 輸出 WAV，並記錄每字 bytes。
5. **ADPCM**：IMA 4-bit，11.025 與 16 kHz，用 Python 編碼再解碼，作為「預錄」的音質上限。

**試聽頁**：`build/voicelab/index.html`，單字 × 方案的表格，每格有 `<audio>`，附每字 bytes 和「3000 字需要多少 KB」。

**Just recipes**：`just voice-lab` 產生全部檔案，`just voice-open` 打開頁面。

**測試**
- `host/tests/test_voicelab.py`：ARPAbet→IPA 對照、ADPCM round-trip。
- LPC 解碼器的單元測試照 `just fw-common-test` 的模式。

### Step 4 — 裝置端程式路徑驗證 `firmware/voicelab/`（fm1-base app，MIT，不刷機）

- **內容**：用 fm1-base template 結構做一個 app，裝自己的 LPC 和 ADPCM 解碼器。
- **操作**：琴鍵對應測試字，按鈕切換方案。播放走 `app_render`，在 44.1 kHz 上重新取樣（輸出做法參考 `firmware/morse/tone.c`）。
- **Klatt 不放進來**：Moonshine 是 C++17，fm1-base 目前看起來只 build C，實作時要確認。是否 port 成 C，等你聽完再決定。
- **生成資料**：放 `firmware/voicelab/gen/`，`.gitignore` 加上 `firmware/*/gen/`。macOS `say` 的聲音不適合公開散佈。
- **驗證**
  - `just fw-test voicelab`：`tests/basic.txt` 用 host simulator 的 `wav FILE` 錄下來，這就是之前說的「用 host simulator 錄」。
  - `just fw-build voicelab`：回報大小和 `APP_SLOT` 的比較。
  - `just emu-run` 在 `uc25hq80` 和 `p25q80` 上都聽一次。
- 這一輪不刷實機。要在實機上聽是另一次韌體寫入，需要你核准並跑完 preflight。

## 主要會動到的檔案 Files

- **新增**
  - `docs/dev/tts.md`、`docs/dev/vocab.md`
  - `host/voicelab/`（`words.txt`、`lab.py`、`klatt_render.cpp`、`index.html` 樣板）
  - `host/tests/test_voicelab.py`
  - `firmware/common/lpc.{c,h}` 與測試
  - `firmware/voicelab/`（Step 4）
- **修改**
  - `zensical.toml`（nav）
  - `docs/resources.md`（版本表）
  - `docs/dev/index.md`（過時描述）
  - `AGENTS.md`（Layout、Commands、過時描述）
  - `Justfile`（`voice-fetch`、`voice-lab`、`voice-open`，版本變數）
  - `.gitignore`
- **沿用**
  - `Justfile` 的 `fetch()` digest 驗證和 `emu-run`/`emu-shot`
  - `firmware/morse/tone.c` 的輸出方式
  - `firmware/common/README.md` 的模組慣例
  - host simulator 的 `wav` 指令（`third_party/fm1-base/host/plat_host.c:26`）
- **不動**
  - `firmware/morse`：另一個工作正在把它改成 submodule（fm1-morse），目前是 staged 狀態，只拿來參考。
  - working tree 裡還沒 commit 的 Morse 相關修改（`docs/dev/morse.md`、`device-log.md`、`docs/firmware/hub.md` 等）
  - `third_party/fm1-base`
- **協調**
  - `AGENTS.md`、`Justfile`、`firmware/common/README.md` 現在也有那批還沒 commit 的修改。
  - 開工前先確認那批已經 commit。我這邊的修改都只新增段落或列，不改寫既有內容。
  - 背單字 app 之後如果要上 Firmware Hub，可以比照 Morse 拆成獨立 repo 再掛成 submodule，這點寫進 vocab.md 的 roadmap。
- **不會 commit**：GPL 程式碼和生成的語音資料。

## Verification 驗證

1. `just docs-check` 通過，新頁面出現在 nav 裡。
2. `just voice-fetch`：每個檔案都是 `ok <sha256>`。`just emu-shot` 顯示 FM-1 板。
3. `just voice-lab` 產生每個方案 × 每個字的 WAV；`just voice-open` 的頁面可以播放，bytes/字 表格正確。
4. 這幾個都通過：
   - `uv run pytest host/tests/test_voicelab.py`
   - `just fw-common-test`（LPC 解碼器）
   - `just fw-test voicelab`（host simulator 錄出 WAV）
   - `just fw-build voicelab`（大小在 `APP_SLOT` 內）
   - `just emu-run` 兩種晶片都有出聲
5. 你聽完選定方案，結果寫回 `docs/dev/tts.md`，再決定 vocab app v1 用哪種語音、Klatt 要不要 port 成 C。
6. 這一輪沒有任何實機操作，`docs/device-log.md` 不變；commit 等你要求再做。
