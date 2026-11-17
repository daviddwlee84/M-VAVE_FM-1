# 裝置端語音合成（TTS）

問題：FM-1 能不能在機器上把英文單字念出來，拿來做背單字 app（構想見[背單字 app](vocab.md)）？

**結論：可以。** 這台的硬體比一般跑小型 TTS 的 MCU 寬裕，社群也已經有五個會講話的韌體。但「好聽」和「放得下」要取捨：

- **聲音最清楚的是預錄**（ADPCM），每字 3.8–5.5 KB，只放得下幾十到一百字。
- **字數幾乎沒上限的是共振峰合成**（Klatt），每字只存約 5 B 的音素，但聲音像機器人，th/s、sh/s 這類對比常常聽不出來。
- **中間是 LPC**（Speak & Spell 的格式），每字約 110 B，可以放兩三千字。用 python_wizard 編碼的結果偏差，編碼器的品質決定一切。

下面的數字來自 2026-11-18 的試聽實驗（`host/voicelab`，見[自己跑一次](#lab)）。

## 硬體預算

| 項目 | 數值 | 來源 |
|---|---|---|
| MCU | JieLi AC791N，雙核，360 MHz，單精度 FPU | `third_party/fm1-base/include/plat.h`、[Developers' Wiki](https://sbegueria.github.io/fm1-dev-wiki/Hardware-Overview.html) `[上游]` |
| app 的 RAM | `.data+.bss` 96 KB，加上 336 KB 的 pool | fm1-base README `[上游]` |
| app 韌體槽 | 581,564 B（`APP_SLOT`）。Morse 0.3 用了 127,880 B | fm1-base `bsp/tools/fm1pkg.py` `[上游]` |
| 音訊輸出 | `app_render` 每次 256 frames，約 44,117.647 Hz 立體聲；喇叭與 3.5 mm 耳機孔 | `bsp/docs/BOARD_IO.md` `[上游]` |
| 沒有的 | 麥克風、RTC、SD 卡（m-card 還是 prototype） | [io.md](../hardware/io.md)、Developers' Wiki |

8 kHz、11.025 kHz、22.05 kHz 的語音都要在 `app_render` 裡升頻到 44.1 kHz：11.025 kHz 剛好 ×4，8 kHz 要做分數倍的插值（`firmware/voicelab` 用線性插值）。

## 社群裡會講話的韌體 {#community}

| 韌體 | 做法 | Licence | 我的觀察 |
|---|---|---|---|
| [Speak37](https://github.com/genkigenki/Speak37) 1.0 | build 時用 espeak-ng 念，自己的編碼器轉成 TMS5220 式 LPC。FM-1 版 2576 字。作者用語音辨識量過：**10 ms 一幀比 25 ms 多保住一倍的字** | GPL-3.0 | 模擬器兩種晶片都開機；本人在 `just emu-run` 裡試玩，覺得「還湊合」（2026-11-18）`[模擬]` |
| [SPELLBOX](https://github.com/pjac1810/spellbox) 0.3 | Speak & Spell 復刻：LPC、12,000 字的發音字典、字尾規則、字母轉音規則（字典外約 83% 音素正確）。琴鍵 F3..F#5 是 A..Z，G5 是 Enter | GPL-3.0 | 模擬器兩種晶片都開機；SPEAK 模式念測試字，Whisper 只聽懂 1/20 `[模擬]` |
| [386DX](https://github.com/seajaysec/386-DX-FM1-KaraokeBot) 2.0 | eSpeak NG 英文前端 + Klatt，資料編進韌體，96 KB RAM 內。引擎另有 [386-DX-ENGINE](https://github.com/seajaysec/386-DX-ENGINE) | GPL-3.0 | 在 Mac 上 build 引擎：英文資料約 305 KB（phondata 164 KB、en_dict 132 KB） |
| [Chimera Vocal](https://github.com/shan-sherif/fm-1-chimera-vocal) 0.8.0 | 聲道物理模型，CMUdict 約 480 字，可從瀏覽器用 SysEx 送字 | MIT（引擎） | 模擬器兩種晶片都開機 `[模擬]` |
| [EVIL GENIUS](https://github.com/Quixotic7/evilgenius) 0.1.4 | SAM（1982 年的 Software Automatic Mouth），54 字或自己打 | MPL-2.0；SAM 本身授權不明 | 模擬器開機時寫 6 次 flash（它自己的資料）`[模擬]` |

下載與驗證：`just voice-fetch`（release digest 比對），然後 `just emu-run downloads/speak37-fm1-1.0.fwsc` 之類直接玩。這些只拿來在模擬器裡聽，沒有打算刷進機器。

沒有任何社群韌體在做字卡、背單字或 Anki（Firmware Hub 目錄，2026-11-18 查核）。

## 通用 TTS 為什麼放不下 `[調研]`

| 引擎 | 大小 | 結論 |
|---|---|---|
| eSpeak NG（完整） | ESP32 最小範例約 1.4 MB flash | 放不下；386DX 只留英文、壓到約 305 KB 才放進來 |
| Flite（cmu_us_kal） | 約 2.3 MB | 放不下 |
| SVOX Pico | 資料 0.7–1.4 MB，RAM 1 MB 以上 | 放不下 |
| Espressif esp-tts | 只有中文 | 不適用 |
| Piper / Kokoro / MeloTTS | 數十到數百 MB | 只能在電腦上用，拿來產生預錄音 |
| [Moonshine Klatt](https://github.com/moonshine-ai/moonshine/tree/main/micro/klatt-tts) | 程式幾十 KiB，G2P 字典約 150 KiB（可省），RAM 幾 KiB；C++17，約 2,000 行 | **放得下**，MIT |

JieLi AC79 SDK 只有雲端 TTS；它的提示音是 MP3 檔。fm1-base 沒有連結 JieLi 的任何音訊函式庫 `[調研]`。

## 試聽結果 {#results}

22 個測試項目（20 個單字、2 個句子，挑容易混淆的音：think/sink、three/tree、ship/sip、very/berry、light/right…）。「Whisper」是語音辨識模型 `base.en` 聽對的數量，**只拿來互相比較**，單字本來就難辨識，macOS 的 `say` 也只有 18/22。

| 方案 | 每字 | 300 KB 放得下 | 共用資料 | Whisper | Licence |
|---|---|---|---|---|---|
| macOS `say`（參考，連線時經 USB audio 播） | — | — | — | 18/22 | Apple，只能自用 |
| ADPCM 16 kHz（預錄） | 5,514 B | 55 字 | — | 18/22 | 免費 |
| ADPCM 11 kHz（預錄） | 3,800 B | 80 字 | — | 17/22 | 免費 |
| Klatt + CMUdict 音素 | 5 B | 6 萬字 | 合成器幾十 KiB | 11/22 | MIT（CMUdict BSD） |
| Klatt + 它自己的 G2P | 6 B（拼字） | 2.4 萬字 | G2P 字典約 150 KiB | 11/22 | MIT |
| LPC 25 ms（python_wizard 編碼） | 111 B | 2,767 字 | — | 9/22 | 解碼器 MIT |
| LPC 20 ms（python_wizard 編碼） | 140 B | 2,192 字 | — | 8/22 | 解碼器 MIT |
| 386DX（eSpeak NG + Klatt） | 6 B（拼字） | 放不進 300 KB | 英文資料約 305 KB | 8/22 | GPL-3.0 |
| SPELLBOX 的 SPEAK（音素拼接 LPC） | — | — | 韌體內建 | 1/20 `[模擬]` | GPL-3.0 |

### 試聽頁的 ✓ / ✗ 怎麼看 {#scoring}

每個播放鍵下面的一行是 **TTS → STT 的來回測試**：把念出來的音檔交給語音辨識模型 Whisper（`base.en`），看它寫出什麼字。

- **✓ 的條件不是逐字 exact match**：大小寫、標點不算，數字換成英文（「3.」算 three），目標字只要**以完整的字**出現在辨識結果裡就算對（「the world.」算 world）。句子要整串照順序出現。反過來，「Alright」不算 right、「3」不算 tree。
- **✗ 後面接的是 Whisper 實際聽到的字**，可以直接看出錯成什麼：think → 「Thank you」、ship → 「Shit」，就是把 th、sh 聽錯了。
- **它只是一個會偏心的聽眾**：Whisper 會往常見的字和片語猜，單獨一個字又沒有上下文，所以連 macOS `say` 的 ship、sip 都被聽成「Shit」「Sit」。音檔太短或太怪時，它會產生一串無意義的數字（像「2 3 8 5 9 8 8…」），那也算 ✗。
- 所以分數是**下限，也只適合拿來互相比較**：同一個字，哪個方案比較常被聽對。它量的是「機器聽不聽得懂」，不是「好不好聽」；最後還是以自己的耳朵為準。

ADPCM 也聽不清的字：think、sip、berry、judge、rhythm。Klatt 的 think/sink、three/tree、ship/sip 全錯，**最小對比正是背單字最需要的**，這是 Klatt 最大的缺點。

### LPC 編碼的發現

- 我們自己的 LPC 解碼器（`firmware/common/lpc.c`）和 python_wizard 附的參考 player 逐幀比對，能量一致；差異只在雜訊激發。所以分數低是**編碼器**的問題，不是解碼器。
- python_wizard 的命令列會把 pre-emphasis 關掉（設定檔預設是開），打開 `-p` 後 Whisper 從 6/20 進步到 7/20。
- python_wizard 用 10 ms 一幀**反而更差**（4/20）：它的音高偵測在短窗格上不穩。這和 Speak37 用自己的編碼器得到「10 ms 比 25 ms 好一倍」的結論相反，代表要做 LPC 就得像 Speak37、SPELLBOX 那樣自己寫編碼器。

### 自製解碼器在 FM-1 上的狀況 {#voicelab-firmware}

`firmware/voicelab` 是一個驗證用的 fm1-base app：琴鍵念測試字，PRESET 切換 LPC / ADPCM，解碼在音訊中斷裡邊播邊做。

- host simulator：`tests/basic.txt` 全部 `ok`，錄出的 WAV 被 Whisper 聽成和 Mac 上一樣的結果（LPC 的 think 聽成「Thank you」，ADPCM 的 vocabulary 正確）。
- 實機套件：image 161,592 B（含 20 字的 LPC 與 ADPCM 資料），RAM `.data+.bss` 25,284 B / 98,304 B。
- fm1-emu：`uc25hq80` 與 `p25q80` 都在按鍵時出聲，0 次 flash 寫入 `[模擬]`。
- 沒有刷進實機。

## 怎麼選

| 情境 | 建議 |
|---|---|
| 接著電腦用 | 電腦的 TTS（macOS `say` 或 Kokoro）經 USB audio 從 FM-1 的喇叭播：音質最好，flash 一個 byte 都不用 |
| 離線、字少（核心 50–100 字） | ADPCM 11 kHz |
| 離線、字多 | Klatt 念全部，**加上**拼字或 KK 音標顯示，彌補最小對比聽不出來的問題；常錯的字可以另外放 ADPCM |
| 想要 Speak & Spell 味道 | LPC，但要先寫好自己的編碼器 |

預錄的來源聲音如果要散佈（放進公開 repo 或 release），要用允許散佈的模型，例如 Kokoro（Apache-2.0）；macOS `say` 的聲音只能自用，所以 `firmware/voicelab/gen/` 與 `build/voicelab/` 都不進 git。

## 會唱歌嗎？（初音未來那種） {#singing}

**用的是同一個家族的技術，FM-1 上已經有人做了。** 初音未來是 Yamaha VOCALOID 的歌聲合成（singing voice synthesis）：輸入是「音符＋歌詞」而不是一般文字，每個音節的音高和長度由樂譜決定。它的聲音來自配音員錄的大量音素片段（現在多半改用神經網路），聲音庫要好幾百 MB，放不進 FM-1。

但 FM-1 上的語音合成大多是 **source-filter 模型**：聲帶的振動（決定音高）和口腔的共鳴（決定是哪個音）分開算。所以改音高很便宜，把聲帶的頻率換成琴鍵的音高就好：

| 韌體 | 怎麼唱 | Licence |
|---|---|---|
| [Speak37](https://github.com/genkigenki/Speak37) | 每個琴鍵用自己的音高念下一個字，按和弦就有和聲，CHOIR 一個音三個聲部；SONGS 唱民謠、CHURCH 一個鍵唱一個音節 | GPL-3.0 |
| [386DX](https://github.com/seajaysec/386-DX-FM1-KaraokeBot) | 卡拉 OK 機器人：eSpeak NG + Klatt，一個音節放在一到多個音符上，母音跨音符延長、滑音 | GPL-3.0 |
| [MIKU-FM1](https://github.com/MichaelAPerry/MIKU-FM1) 0.8 | 用日文唱你寫的歌（73 個假名音節，含子音），即時合成的共振峰聲音、不用取樣，配 FM 和弦、bass、鼓；畫面上是初音未來的同人像素角色（Piapro 角色授權，限非商業） | GPL-3.0 `[上游]` |
| [SPELLBOX](https://github.com/pjac1810/spellbox) | SING：選一個字，用琴鍵的音高唱，母音按著就延長 | GPL-3.0 |

音質是 Speak & Spell 或機器人唱歌的等級，不是 VOCALOID。Moonshine Klatt 也有 `f0_start`、`f0_end`、`f0_scale`（音高）和 `formant_scale`（聲道長短，可變成小孩聲），要逐音符唱只需要再加一個「這個音節用這個音高」的介面。

## Licence 注意事項

- GPL-3.0 的引擎（eSpeak NG、386DX、Speak37、SPELLBOX）放進 app 就讓整個 app 變 GPL-3.0。另有一說是 GPL 程式和 JieLi 閉源的 Bluetooth 函式庫不能合法一起散佈 `[調研]`；不開 Bluetooth（或用 fm1-base 自己的 `ble_stack = "own"`）可以避開。
- voicelab 只 commit MIT 程式碼。386DX 引擎留在 `vendor/`（git-ignored），`host/voicelab/dx386_say.c` 是我們自己寫、只呼叫它公開 API 的小程式；連結後的執行檔是 GPL，只留在 `build/`。
- LPC 係數表取自 python_wizard（MIT），它的來源是 MAME。

## 自己跑一次 {#lab}

```sh
just voice-vendor   # Moonshine Klatt、python_wizard、CMUdict、386DX 引擎，固定 commit，放進 vendor/
just voice-fetch    # 社群韌體（SPELLBOX 欄位要用）
just voice-lab      # 每個方案念每個測試字 → build/voicelab/<engine>/*.wav
just voice-score    # Whisper 聽一遍，分數寫上頁面（第一次會下載模型）
just voice-open     # 打開 build/voicelab/index.html 試聽
just voice-fw       # 測試字放進 firmware/voicelab，跑 host simulator
just fw-build voicelab && just emu-run firmware/voicelab/build/voicelab.fwsc
```

測試字在 `host/voicelab/words.txt`。程式結構：`lab.py`（各方案）、`score.py`（Whisper）、`page.py`（試聽頁）、`fwdata.py`（韌體資料），C 的部分是 `firmware/common/lpc.c`、`firmware/common/adpcm.c`。
