# 摩斯電碼機（Morse）

把 FM-1 變成隨身的摩斯電碼機：琴鍵當電鍵發報、即時解碼成文字、琴鍵當字母鍵盤把文字播成摩斯、用閃卡隨身練習，也能接電腦和 [Vail](https://vailmorse.com/) 跟網路上的人互傳。韌體在 `firmware/morse/`，電腦端在 `host/morse/`，都是 MIT。

**狀態（2026-11-17）：韌體 0.1、host 0.1.0。單元測試、host simulator、fm1-emu 兩種晶片都驗證過 `[模擬]`；還沒刷進實機。** 操作方式見[操作速查](morse-cheatsheet.md)。

## 有沒有人做過 { #prior-art }

2026-11-17 查過，**沒有**找到 FM-1 上的摩斯作品，也沒有讓兩台 FM-1 互傳訊息的韌體 `[調研]`：

- [Firmware Hub catalog](https://thefirmwarehub.com/catalog.json) 的 244 個套件裡沒有 Morse、CW、keyer（電報意義的）、trainer。
- [FM1 Developers' Wiki](https://sbegueria.github.io/fm1-dev-wiki/) 沒有相關頁面。
- GitHub 搜尋 `fm1 morse`、`m-vave morse`、`fm1-base morse` 都沒有結果（code search 只涵蓋 default branch，可能漏掉）。

相近的作品：SPELLBOX 把琴鍵當 A–Z 鍵盤；MIDIMON（fm1-base）同時當 BLE peripheral、central 與 USB host，作者說用它來 debug 另一台 FM-1。設計時參考的是其他平台的摩斯機：[Morserino-32](https://github.com/oe1wkl/Morserino-32)（模式分類、收發用不同音高）、[LCWO](https://lcwo.net/)（Koch 順序）、Vail（MIDI 格式）。只參考做法，沒有複製程式碼。

## 功能

| 頁面 | 做什麼 |
|---|---|
| **PRACTICE** | 開機直接進來。一回合 20 張閃卡（10–30 可調），幾分鐘做完 |
| **KEY** | 自由發報：琴鍵是電鍵，畫面顯示解出來的字，還有你的速度和 dah:dit 比例。電腦送來的字用琥珀色顯示 |
| **TYPE** | 琴鍵是字母：白鍵 A–P、黑鍵 Q–Z 和空白，按住 `OCT-` 切成數字與符號。按一個鍵就播出那個字 |
| **CHART** | 像五十音表的字表，依熟練度上色 |
| **SETTINGS** | 速度、Farnsworth 間隔、音高、keyer（直鍵、iambic A、iambic B）、paddle 左右、聲音、每回合張數、學習方式 |

電鍵有兩種：**直鍵**是任一琴鍵按下就發；**iambic paddle** 是左半邊琴鍵（F3–F4）發 dit、右半邊（F#4–G5）發 dah，兩邊同時按會交替（squeeze）。Mode B 會記住在一個 element 中途按的另一邊，放開後補發一個。

聲音是兩個 sine voice：自己發的用設定的音高（預設 600 Hz），收到的高一個大三度，聽得出是誰在發。靜音時所有琴鍵的燈會跟著節奏亮（自己發的全亮、收到的半亮），畫面上也有一條燈號。

## 學習：閃卡和間隔複習

體驗參考背單字和學五十音：

- **一個字兩張卡**：「認得它」（聽到或看到，說出是哪個字）和「打得出它」（看到字母，用電鍵打出來）。
- **Leitner 盒**：每張卡在 0–5 號盒。答對升一盒，答錯回 0 號，盒 n 每 2ⁿ 個回合複習一次。答錯的卡會在同一回合最後再問一次，這次答對也不升盒。
- **用回合數，不用日期**：FM-1 沒有關機後還在走的時鐘，所以排程數的是「做了幾回合」。之後接電腦時可以讓 host 送日期，改成真正的天數，但還沒做。
- **新字按 Koch 順序**（LCWO 的 41 字：K M U R E S N A P T L W I . J Z = F O Y , V G 5 / Q 9 2 H 3 8 B ? 4 7 C 1 D 6 0 X），5 個一組，像五十音的一「行」。一回合最多 3 個新字，每個新字先播兩次給你聽，然後才考。開著的字全部「認得」到 2 號盒以上、「打得出」到 1 號盒以上，才開下一組。
- **先選擇，後回想**：「認得」的卡在 0–2 號盒是聽完從四個裡選（四個選項對應 E4 F4 G4 A4 四個亮燈的鍵，干擾選項挑碼型相近的），3 號盒以上改成聽完直接打出來。
- 回合結束顯示答對、答錯、升了幾盒，以及連續幾回合正確率 80% 以上（streak）。中途按 `HOME` 離開，已答的部分照樣存。

### 先用聽的：學習方式設定 { #by-sound }

**預設是「SOUND」**：出題只用聲音，畫面上**不畫點劃**，字表只顯示熟練度，靜音時燈號照真實節奏閃，也不畫成 ·−。

原因：Koch 法、LCWO、CW Academy 這類教學都主張把每個字的聲音當成一個整體記，不要看點劃圖。看圖學的人聽到訊號時會先數幾點幾劃再查表，速度常卡在每分鐘 10–15 字，之後得花時間改掉。`[調研]`（這是教學界的普遍說法，我還沒找到量化研究。）同樣的理由，字元速度預設 20 WPM，用 Farnsworth 把字和字之間拉開到有效 10 WPM，而不是把每個字放慢。

「SIGHT」是另一個選項：顯示點劃，閃卡在低盒號時用「看碼選字」，KEY 頁畫出正在打的碼。切過去時畫面會提示「SOUND FIRST IS FASTER LATER」。

| | SOUND（預設） | SIGHT |
|---|---|---|
| 認得的卡，0–2 號盒 | 聽 → 四選一 | 看點劃 → 四選一 |
| 認得的卡，3 號盒以上 | 聽 → 打出來 | 聽 → 打出來 |
| 打得出的卡 | 看字母 → 打出來 | 同左 |
| 靜音時認得的卡 | 看燈號節奏 → 四選一 | 看點劃 → 四選一 |
| 畫面上的點劃 | 不顯示 | 新字、答案、KEY 頁、字表都顯示 |

## 和 Vail 搭配 { #vail }

單字、呼號、Q code、QSO 模擬、長篇抄收、和人互傳，這些深度訓練交給 Vail，不在韌體裡重做。FM-1 接上電腦就是一個 Vail 認得的 MIDI 電鍵：

- **格式** `[上游]`：channel 1、note 0 是「已經排好時序的按鍵」，按下 `90 00 7F`、放開 `80 00 00`（[vail-adapter 的 MIDI spec](https://github.com/Vail-CW/vail-adapter/blob/master/docs/MIDI_INTEGRATION_SPEC.md)）。FM-1 的 iambic keyer 在機器上就把時序排好，所以一律送 note 0。
- **Vail 網頁怎麼讀** `[上游]`：[vail_web_repeater](https://github.com/Vail-CW/vail_web_repeater) 的 `static/scripts/inputs.mjs` 把 note 0 當直鍵；它只看指令（note on / note off），**不看 velocity**，所以 `90 00 00` 在 Vail 會被當成按下。我們放開一律送 `80 00 00`。Vail 打開頁面時會對所有 MIDI 輸出送 `B0 00 00`、`B0 01 xx`、`C0 xx`（給 Vail adapter 的設定），FM-1 會忽略。
- **Vail 的訓練工具** `[上游]`：[training.vailmorse.com](https://training.vailmorse.com/) 有 Vail CW School（進度統計、遊戲）、Hear It Type It、Send Practice（明寫支援 USB CW key 和 MIDI adapter）、Free Practice、Morse Mailbox（和朋友非同步互傳）、QSO Simulator 等。
- **驗證狀態**：位元組在 host simulator 和 fm1-emu 都確認過 `[模擬]`；**還沒有實際在瀏覽器裡用 FM-1 開 Vail 測過。**

## 電腦端（host）

`host/morse/`（Python，mido + python-rtmidi），協定見 [`host/morse/PROTOCOL.md`](https://github.com/daviddwlee84/M-VAVE_FM-1/blob/main/host/morse/PROTOCOL.md)：

| 指令 | 做什麼 |
|---|---|
| `just morse-type` | 在 FM-1 的 KEY 頁發報（或在 TYPE 頁打字），解出的字打進 Mac 目前的視窗。要給終端機 Accessibility 權限；`--dry-run` 只印出來 |
| `just morse-send 文字` | FM-1 把文字當成「收到的」播出來，用來練抄收。`-` 從 stdin 一行一行送 |
| `just morse-sim` | 不接 FM-1，用文字模擬 |

解碼在 FM-1 上做，host 只收結果（`T_CHAR`），所以畫面上看到的和打進電腦的一定一樣。host 每收到一次 HEARTBEAT 就回一個 HELLO_ACK；FM-1 超過 10 秒沒收到 host 的東西就當成斷線。

## 兩台互傳

現況和選項見[接口與連線](../hardware/io.md#two-units)。這台軟體的設計是**線路上傳按鍵動作（note on / off），不傳文字**，收的一方自己解碼，像真的電報線。好處是同一套格式可以走 USB、兩種藍牙 stack、USB host、經電腦轉送，Vail 也聽得懂。

我手上只有一台，所以直連排在下一版（見[路線圖](#roadmap)），先用電腦和 Vail 跟人互傳。

## 設計

### 時序

- **1 kHz tick**（`APP_TICK`，timer interrupt）：讀琴鍵 → keyer → 開關 sidetone 的 gate、送 MIDI、讀 MIDI in，按鍵變化連同時間戳放進 ring buffer。main loop 解碼、跑頁面和閃卡、畫畫面。
- **Sidetone 的抖動**：gate 每個 audio block 讀一次（256 frames = 5.8 ms），按下到出聲最多差一個 block。
- **琴鍵的 debounce**：按下約 1–2 ms 後看到、放開約 9 ms 後看到，所以直鍵的每個 mark 會被量長約 7 ms（fm1-base `bsp/boards/fm1/fm1_input.h`）。韌體在實機 build 時從直鍵的 mark 扣掉 7 ms 再解碼。fm1-emu 裡看到的是按下 +3 ms、放開 +10 ms，和預期一致 `[模擬]`。
- **解碼器**：分別追蹤 dit、dah 和字間間隔的平均，取中間當門檻，所以跟得上偏離設定 ±30% 的發報速度，也能讀 Farnsworth 拉開的間隔（單元測試：5／15／25／35 WPM 加 ±20% 抖動全對）。

### 存檔 { #flash }

一筆紀錄（設定 + 學習進度，約 280 bytes）放在 `0xDA000`–`0xDBFFF`，透過 fm1-base 的 storage 層存（A/B 兩份、CRC）。在回合結束或離開、改設定 3 秒後才寫。不碰 SLOOP、Agent Deck、CTL-1 共用的 `0xFC000`。這台機器所有備份的 `0xDA000`–`0xDBFFF` 都是空的。

預演 `[模擬]`：把 `firmware/morse/build/morse.fwsc` 疊在 `backups/2026-11-17-2052-pre-conductor/` 上開機、改一個設定、玩完一回合：

| | sector |
|---|---|
| 安裝（和目前機上的 Conductor 不同的部分） | `0x04000`–`0x1EFFF`，27 個；套件映像範圍是 `0x04000`–`0x92FFF` |
| 執行時寫入 | `0xDA000`–`0xDBFFF`，只有這 2 個 |

### 大小

`just fw-build morse`（2026-11-17）：app 109,508 B（上限 581,564）；RAM code 5,100 / 32,768 B；`.data+.bss` 27,180 / 98,304 B；套件 616,003 B，identity `FM-1_1300`。這版沒有開藍牙；加上 own BLE stack 和 central 大約再多 55–70 KB，仍然很寬裕。

## 驗證

| 項目 | 指令 | 結果 |
|---|---|---|
| 碼表、時序、解碼器、keyer、閃卡規則 | `just morse-unit` | 251 項全過 |
| 整個 app（發報、收報、Vail 位元組、host 協定、靜音、iambic B、TYPE、三回合閃卡含真的按鍵作答、重開機讀回） | `just fw-test morse` | 全過 |
| host 協定、和韌體共用的測試向量 | `just morse-test` | 全過 |
| 實機套件 | `just fw-build morse` | build 自帶檢查全過 |
| 兩種晶片：改設定、玩一回合、重開讀回 | fm1-emu `uc25hq80`、`p25q80` | 都讀回「ROUND 2」和改過的設定 `[模擬]` |
| USB MIDI 的實際位元組 | fm1-emu `--usb` 的 trace | 按 K 送出 `90 00 7F`／`80 00 00`、`T_CHAR 'K'`；收到 host 的 `T_TEXT` 會顯示 `[模擬]` |
| host 接模擬器的虛擬 USB 線 | `just morse-emu` + `just morse-send` | 找到 `MORSE FM-1 (emu)`、收到 HELLO、送出文字 `[模擬]` |
| Vail 網頁 | — | 還沒測 |
| 實機 | — | 還沒刷 |

## 版本紀錄

韌體 X.Y（`FW_MINOR` in `firmware/morse/app.c`），host X.Y.Z（`__version__` in `host/morse/__init__.py`，加上 `FIRMWARE_KNOWN`），規則和 Conductor 一樣。

| 版本 | 日期 | 要刷機 | 內容 |
|---|---|---|---|
| 韌體 0.1、host 0.1.0 | 2026-11-17 | 是（還沒刷） | 第一版：KEY、TYPE、PRACTICE、CHART、SETTINGS，兩種學習方式，Vail 格式的 MIDI，host 的 `type`／`send`／`listen`。原本規劃分成 0.1（核心）和 0.2（閃卡），因為一起做完、都還沒刷，合併成 0.1 |

## 路線圖 { #roadmap }

| 版本 | 內容 |
|---|---|
| 0.2 | 藍牙：own BLE stack 的 peripheral（Mac、iPad、手機上的 Vail 走藍牙）和 central（一台 FM-1 掃描並連上另一台）。BLE connection interval 是 11.25 ms，每個按鍵事件會晚 0–11 ms 到；對策想用 note 的 velocity 帶上長度、收的一方加一點播放緩衝，等量過再定。實機要等有第二台 |
| 之後 | host 送日期，讓間隔複習改成天數；USB-C 直連（見[接口與連線](../hardware/io.md#two-units)） |
