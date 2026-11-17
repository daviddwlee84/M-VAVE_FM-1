# 摩斯電碼機（Morse）

把 FM-1 變成隨身的摩斯電碼機：琴鍵當電鍵發報、即時解碼成文字、琴鍵當字母鍵盤把文字播成摩斯、用閃卡隨身練習，也能接電腦和 [Vail](https://vailmorse.com/) 跟網路上的人互傳。韌體在獨立的 repo [fm1-morse](https://github.com/daviddwlee84/fm1-morse)（GPL-3.0-or-later，以 submodule 放在本 repo 的 `firmware/morse/`），電腦端在本 repo 的 `host/morse/`（MIT）。

**狀態（2026-11-18）：機器上是韌體 0.3（02:12 刷入）。0.1 時已在實機確認發報、解碼、TYPE、PRACTICE、和電腦連線；0.2 的 RHYTHM、KEY 三種顯示、TYPE、LISTEN、PRESET 本人試過覺得不錯，升級後學習進度也還在 `[實機]`。0.3 修了 TYPE 鍵上點劃的排版。** 操作方式見[操作速查](morse-cheatsheet.md)。

## 有沒有人做過 { #prior-art }

2026-11-17 查過，**沒有**找到 FM-1 上的摩斯作品，也沒有讓兩台 FM-1 互傳訊息的韌體 `[調研]`：

- [Firmware Hub catalog](https://thefirmwarehub.com/catalog.json) 的 244 個套件裡沒有 Morse、CW、keyer（電報意義的）、trainer。
- [FM1 Developers' Wiki](https://sbegueria.github.io/fm1-dev-wiki/) 沒有相關頁面。
- GitHub 搜尋 `fm1 morse`、`m-vave morse`、`fm1-base morse` 都沒有結果（code search 只涵蓋 default branch，可能漏掉）。

相近的作品：SPELLBOX 把琴鍵當 A–Z 鍵盤；MIDIMON（fm1-base）同時當 BLE peripheral、central 與 USB host，作者說用它來 debug 另一台 FM-1。設計時參考的是其他平台的摩斯機：[Morserino-32](https://github.com/oe1wkl/Morserino-32)（模式分類、收發用不同音高）、[LCWO](https://lcwo.net/)（Koch 順序）、Vail（MIDI 格式）。只參考做法，沒有複製程式碼。

## 功能

| 頁面 | 做什麼 | 版本 |
|---|---|---|
| **PRACTICE** | 開機直接進來。一回合 20 張閃卡（10–30 可調），幾分鐘做完 | 0.1 |
| **LISTEN** | 一直播下去：你開放的 Koch 字組、常用英文字、QSO 範例、縮寫和意思、諺語。可以顯示正在播的字，或隱藏起來當抄收練習 | 0.2 |
| **KEY** | 自由發報：琴鍵是電鍵，解出來的字、你的速度、dah:dit 比例、字內停頓。三種顯示：TEXT（字）、CODE（每個字下面是你實際打的點劃）、TIMING（每一下照實際長短畫成條） | 0.1；顯示方式 0.2 |
| **RHYTHM** | 練長短：只打 dit、只打 dah、交替、SOS、自由按。每一下和目標長度比對，給提示和建議的發報速度 | 0.2 |
| **TYPE** | 琴鍵是字母，`OCT-`／`OCT+` 翻三頁：字母、數字和符號、片語。鍵上可以顯示點劃；LIVE（按一個字送一個）或 COMPOSE（打好一行再送） | 0.1；翻頁、片語、COMPOSE 0.2 |
| **CHART** | 像五十音表的字表，依熟練度上色 | 0.1 |
| **SETTINGS** | 速度組合（PRESET）、聽的速度、間隔、發報速度、音高、keyer（直鍵、iambic A、iambic B）、paddle 左右、聲音、每回合張數、學習方式、TYPE 的排列 | 0.1；PRESET、發報速度 0.2 |

電鍵有兩種：**直鍵**是任一琴鍵按下就發；**iambic paddle** 是左半邊琴鍵（F3–F4）發 dit、右半邊（F#4–G5）發 dah，兩邊同時按會交替（squeeze）。Mode B 會記住在一個 element 中途按的另一邊，放開後補發一個。

聲音是兩個 sine voice：自己發的用設定的音高（預設 600 Hz），收到的高一個大三度，聽得出是誰在發。靜音時所有琴鍵的燈會跟著節奏亮（自己發的全亮、收到的半亮），畫面上也有一條燈號。

## 速度：WPM 是什麼、該設多少 { #speed }

WPM 是每分鐘幾個字，以 PARIS 這個字（50 個單位）為準：**dit 的長度 = 1200 ÷ WPM 毫秒**。摩斯沒有規定一定要多快，重要的是比例：

| 元素 | 長度 |
|---|---|
| dit | 1 單位 |
| dah | 3 單位 |
| 字裡面的停頓 | 1 單位 |
| 字和字之間 | 3 單位 |
| 字詞之間 | 7 單位 |

| WPM | dit | dah | 大概是 |
|---|---|---|---|
| 5 | 240 ms | 720 ms | 很慢，看得清楚每一下 |
| 10 | 120 ms | 360 ms | 新手手打 |
| 13 | 92 ms | 277 ms | 以前美國業餘無線電 General 考試的速度 |
| 20 | 60 ms | 180 ms | Koch 建議的字元速度 |
| 25 | 48 ms | 144 ms | 熟練的通聯 |

0.2 起分成兩個速度（0.1 只有一個）：

- **聽的速度**（K1，加上 K2 的間隔）：FM-1 播給你聽的速度。Koch 的做法是字本身用 20 WPM 左右播，聽起來是一整個聲音；字和字之間用 **Farnsworth** 拉開（例如「20/8」是字用 20 WPM，整體像 8 WPM），讓你有時間想。
- **發報速度**（K4）：你手打的速度，用來設定 iambic keyer 的節奏、解碼器的起點、RHYTHM 頁和 TIMING 檢視的目標長度。新手的手通常比耳朵慢，所以這個要另外設低一點。

**推薦的組合**（SETTINGS 最上面的 PRESET，轉的時候畫面會說明）：

| PRESET | 聽 | 發 | 適合 |
|---|---|---|---|
| START（預設） | 20/8 | 10 | 剛開始 |
| BUILDING | 20/13 | 13 | 認得大部分的字之後 |
| QSO | 22/18 | 18 | 能和人一般通聯 |
| FAST | 25/25 | 22 | 熟練 |

自己調的方法：到 TYPE 頁按幾個字聽聽看，轉 K1、K2 找到「字聽得出是一整個聲音、字和字之間來得及想」的組合；發報速度到 RHYTHM 頁打幾組 dit，它會依你的 dit 平均長度建議一個 K4 值。這些組合是我依 Koch／Farnsworth 的做法和常見的考試、通聯速度整理的建議 `[調研]`，不是哪個教材的固定值。

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

原因：Koch 法、LCWO、CW Academy 這類教學都主張把每個字的聲音當成一個整體記，不要看點劃圖。看圖學的人聽到訊號時會先數幾點幾劃再查表，速度常卡在每分鐘 10–15 字，之後得花時間改掉。`[調研]`（這是教學界的普遍說法，我還沒找到量化研究。）同樣的理由，字元速度預設 20 WPM，用 Farnsworth 把字和字之間拉開到有效 8 WPM，而不是把每個字放慢。

這是「學認字」的規則。「練發報」時看自己打出的長短是有用的回饋，所以 KEY 頁的 CODE／TIMING 檢視、RHYTHM 頁、TYPE 頁的「鍵上點劃」都和這個設定分開，用的時候自己打開。

「SIGHT」是另一個選項：顯示點劃，閃卡在低盒號時用「看碼選字」，KEY 頁畫出正在打的碼。切過去時畫面會提示「SOUND FIRST IS FASTER LATER」。

| | SOUND（預設） | SIGHT |
|---|---|---|
| 認得的卡，0–2 號盒 | 聽 → 四選一 | 看點劃 → 四選一 |
| 認得的卡，3 號盒以上 | 聽 → 打出來 | 聽 → 打出來 |
| 打得出的卡 | 看字母 → 打出來 | 同左 |
| 靜音時認得的卡 | 看燈號節奏 → 四選一 | 看點劃 → 四選一 |
| 畫面上的點劃 | 不顯示 | 新字、答案、KEY 頁、字表都顯示 |

## 練長短：RHYTHM 頁 { #rhythm }

給還分不出長短的人。每按一下，畫面會畫出目標長度的框和你實際按的長度，顏色是綠（好）、黃（偏了）、紅（變成另一種了），並寫出「DIT 1.3（130 MS）A BIT LONG」這樣的數字：1.3 表示是目標 dit 的 1.3 倍。

| 練習 | 目標 |
|---|---|
| DITS | 五個 dit |
| DAHS | 五個 dah |
| DIT DAH | 交替三次 |
| SOS | `... --- ...` |
| FREE | 自由按，看每一下被當成什麼 |

判斷標準（以發報速度的 dit 為 1）：dit 在 0.5–1.5 算好、1.6–2.0 偏長、超過 2.0 就會被當成 dah；dah 在 2.2–4.0 算好、低於 1.8 會被當成 dit；1.6–2.1 是兩邊都不像的「中間地帶」。最近 16 下的平均會產生一條提示：dit 太長（附上適合你的 K4 速度）、dah 太短或太長、字內停頓太長（超過 1.8）、長短不穩（平均差 0.3 個 dit 以上）、或「GOOD RHYTHM」。

## 收聽：LISTEN 頁 { #listen }

一直播下去，適合放著聽。播的內容都是韌體裡現場組出來的，沒有使用任何人的教材：

| 節目 | 內容 |
|---|---|
| KOCH GROUPS | 你已經開放的字，隨機五個一組，一行五組 |
| WORDS | 最常用的 100 個英文字，一行六個 |
| QSO | 兩個隨機呼號從 CQ、報訊號和名字、地點，到 73、`<SK>` 結束，六行一輪 |
| ABBREVIATIONS | 一個縮寫播兩次，畫面顯示意思（例如 QTH：my location is） |
| SAYINGS | 英文諺語，加上 1844 年第一封電報 WHAT HATH GOD WROUGHT |

`SEQ` 切換「顯示」和「隱藏」。隱藏時只看得到播到第幾個字，這一行播完、下一行開始時才顯示答案，等於抄收練習；`SEL` 可以偷看。更長的教材可以從電腦用 `just morse-send -` 把文字一行一行送進來播，例如 Project Gutenberg 上公有領域的書。

被動地一直聽能幫忙熟悉聲音，但教學上普遍認為主動抄收、回打進步比較快 `[調研]`，所以 LISTEN 是 PRACTICE 的補充。

## 片語、prosign 和實際的用法 { #phrases }

業餘無線電用摩斯通聯（QSO）時，有一套固定的縮寫和 **prosign**。prosign 是幾個字母連在一起當成一個符號打，中間沒有字間停頓，寫成 `<SK>` 這樣。TYPE 頁第三頁的 27 個片語鍵：

| 片語 | 意思 | 片語 | 意思 |
|---|---|---|---|
| CQ | 呼叫任何電台 | GM / GE | 早安 / 晚安 |
| DE | 這裡是 | FB | fine business：很好 |
| K | 換你說 | R | 收到 |
| `<KN>` | 換你說，只限你 | QRZ? | 誰在呼叫我？ |
| `<AR>` | 訊息結束 | QSL | 我確認收到 |
| `<SK>` | 通聯結束 | `<BK>` | 換手（back to you） |
| `<BT>` | 停頓、換段 | HW? | 收得清楚嗎？ |
| 73 | 祝好、再見 | PSE | 請 |
| TU | 謝謝 | AGN | 再一次 |
| RST | 訊號報告 | QRS | 請發慢一點 |
| 5NN | 599 報告的簡寫 | QTH | 我的位置是 |
| NAME | 我的名字是 | OP | 操作員 |
| UR | 你的 | ES | 和 |

**實際上大家怎麼用**：大部分人仍然是**用手即時發**（直鍵或 paddle），對方**用耳朵抄**，這是這項技能的核心。另外也很常見：

- 電腦或「CW 鍵盤」：先打好字放在緩衝區，機器用固定速度送出，一邊送一邊還可以繼續打（type-ahead）。
- 比賽軟體：用 F 鍵送 `CQ TEST`、報告這類事先寫好的片語（macro）。
- 收的一方：有人用軟體解碼（fldigi、CW Skimmer，比賽和 Reverse Beacon Network 大量使用），但通聯本身多半還是靠人耳。`[調研]`

TYPE 頁的 LIVE 對應「打一個送一個」，COMPOSE 對應「先打好再送」，片語鍵對應 macro。

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
- **解碼器**：分別追蹤 dit、dah 和字間間隔的平均，取中間當門檻，所以跟得上偏離設定 ±30% 的發報速度，也能讀 Farnsworth 拉開的間隔（單元測試：5／15／25／35 WPM 加 ±20% 抖動全對）。0.2 起，同一個字裡出現兩下長度差 2 倍以上時，會直接把它們當成一個 dit 一個 dah，並回頭修正前一下的判斷：新手太慢的 dit 被當成 dah，遇到第一個真正的 dah 就會改回來。但如果手打得比發報速度慢很多，連字裡的停頓都會超過切字的門檻，所以發報速度還是要設得接近你的手。
- **實機上看到的** `[實機]`：手打的 O（三個 dah）一次被讀成 M + T，因為 dah 之間停了 146、175 ms，和那次字與字之間的 169–224 ms 一樣長；字裡面的間隔應該只有一個 dit。另一次 dah 之間 124、133 ms 就讀對。也看到一次 9 ms 的按鍵彈跳：解碼器扣掉 debounce 偏差後當成雜訊忽略，但 MIDI 照送（Vail 會看到一下極短的按下）。

### 存檔 { #flash }

一筆紀錄（設定 + 學習進度，約 280 bytes）放在 `0xDA000`–`0xDBFFF`，透過 fm1-base 的 storage 層存（A/B 兩份、CRC）。在回合結束或離開、改設定 3 秒後才寫。不碰 SLOOP、Agent Deck、CTL-1 共用的 `0xFC000`。這台機器所有備份的 `0xDA000`–`0xDBFFF` 都是空的。

0.2 沿用 0.1 的紀錄格式：0.1 留下的空位改放 KEY／TYPE／LISTEN 的顯示選項、發報速度、LISTEN 的節目。從 0.1 升級時設定和進度都保留，發報速度從 0 補成預設的 10（fm1-emu 兩種晶片驗證過 `[模擬]`）。

預演 `[模擬]`：把 `firmware/morse/build/morse.fwsc` 疊在 `backups/2026-11-17-2052-pre-conductor/` 上開機、改一個設定、玩完一回合：

| | sector |
|---|---|
| 安裝（和目前機上的 Conductor 不同的部分） | `0x04000`–`0x1EFFF`，27 個；套件映像範圍是 `0x04000`–`0x92FFF` |
| 執行時寫入 | `0xDA000`–`0xDBFFF`，只有這 2 個 |

### 大小

`just fw-build morse`（2026-11-17）：

| 版本 | app | `.data+.bss` |
|---|---|---|
| 0.1 | 109,508 B | 27,180 / 98,304 B |
| 0.2 | 127,712 B | 28,444 / 98,304 B |
| 0.3 | 127,880 B | 28,444 / 98,304 B |

上限是 581,564 B；RAM code 5,100 / 32,768 B；套件 616,003 B，identity `FM-1_1300`。這兩版都沒有開藍牙；加上 own BLE stack 和 central 大約再多 55–70 KB，仍然很寬裕。

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
| 刷進實機 | `just fm1-switch firmware/morse/build/morse.fwsc` | 寫入 30 個 sector（`0x04000`–`0x21FFF`），和預演一致 `[實機]` |
| 實機和電腦連線 | `python -m morse send`、監聽 script | port 名稱沿用 `Felucca`，host 依協定找到；HELLO、HELLO_ACK、`T_TEXT`、`T_CHAR` 都正常 `[實機]` |
| 實機 KEY 頁 | 本人直鍵發報 | 解碼正確；按鍵以 Vail 格式送出，長度是手打的真實值 `[實機]` |
| 實機 TYPE 頁 | 本人按字 | `T_CHAR` 即時送出；播放 dit 60 ms、Farnsworth 字間 652 ms，和設計一致；K1 調速生效 `[實機]` |
| 實機 PRACTICE | 本人玩了一回合 | 本人回饋「還可以」`[實機]` |
| 實機靜音、iambic | — | 還沒在實機上確認 |
| 0.2：解碼器重讀、prosign、LISTEN 的內容 | `just morse-unit` | 2932 項全過（大部分是逐行檢查產生的節目都播得出來） |
| 0.2：整個 app（三種 KEY 檢視、RHYTHM 三種提示、TYPE 翻頁、片語、COMPOSE、停止、REC 清空、排列方式、LISTEN、PRESET、K4、升級後重開讀回、四回合閃卡） | `just fw-test morse` | 全過 |
| 0.2：從 0.1 的紀錄升級、改設定、重開讀回 | fm1-emu `uc25hq80`、`p25q80` | 設定和進度保留，發報速度補成 10，改過的值重開後讀回 `[模擬]` |
| 0.2：疊在實機 0.1 的備份上 | `emu_rehearse.py` + fm1-emu | 真正的學習紀錄（ROUND 4、聽 15/14）升級後保留 `[模擬]` |
| 0.2 刷進實機 | `just fm1-switch firmware/morse/build/morse.fwsc` | 寫入 32 個 sector（`0x04000`–`0x23FFF`），和預演一致；HELLO 回報 0.2，`<SK>` 送出 `[實機]` |
| 0.2 升級後的進度 | 本人看 PRACTICE 頁 | 還是 ROUND 4：刷機只改寫韌體區，`0xDA000` 的紀錄留著 `[實機]` |
| 0.2 新頁面 | 本人試用 RHYTHM、KEY 顯示、TYPE、LISTEN、PRESET | 「感覺都還不錯」；TYPE 鍵上點劃在黑鍵擠掉字母 → 0.3 修 `[實機]` |
| 0.3：TYPE 鍵上點劃、從 0.2 升級、存檔重開讀回 | `just fw-test morse`、fm1-emu 兩種晶片 | 全過；字母頁和數字頁的碼與字母都放得下 `[模擬]` |

## 版本紀錄

韌體 X.Y（`FW_MINOR` in `firmware/morse/app.c`），host X.Y.Z（`__version__` in `host/morse/__init__.py`，加上 `FIRMWARE_KNOWN`），規則和 Conductor 一樣。

| 版本 | 日期 | 要刷機 | 內容 |
|---|---|---|---|
| 韌體 0.1、host 0.1.0 | 2026-11-17 | 是（22:54 刷入，`c3b5cd6`） | 第一版：KEY、TYPE、PRACTICE、CHART、SETTINGS，兩種學習方式，Vail 格式的 MIDI，host 的 `type`／`send`／`listen`。原本規劃分成 0.1（核心）和 0.2（閃卡），因為一起做完、都還沒刷，合併成 0.1 |
| 韌體 0.3 release（`FM-1_41000300`） | 2026-11-18 | 是（Hub 測試時用 USB-MIDI 裝過三次） | fm1-morse 的第一個 release [v0.3](https://github.com/daviddwlee84/fm1-morse/releases/tag/v0.3)：identity 改成 `FM-1_41`，授權改成 GPL-3.0-or-later，程式和 0.3 相同 |
| 韌體 0.3、host 0.3.0 | 2026-11-18 | 是（02:12 刷入，`02b0ea2`） | TYPE 打開鍵上點劃時，鍵盤從 86 px 拉高到 126 px、上方輸入行縮成一行，黑鍵和白鍵都同時放得下碼和字母（0.2 黑鍵只剩碼、白鍵的碼壓到字母，數字頁的 `0` 放不下）。藍牙順延到 0.4 |
| 韌體 0.2、host 0.2.0 | 2026-11-18 | 是（00:14 刷入，`54ff440`） | 實機試用後的回饋：發報速度和聽的速度分開（K4），PRESET 推薦組合，轉速度時顯示 dit／dah 毫秒；KEY 頁 TEXT／CODE／TIMING 三種顯示；RHYTHM 頁（練長短、提示、建議速度）；LISTEN 頁（連續播放五種節目）；TYPE 頁翻三頁（字母、數字符號、片語）、三種排列、鍵上點劃、LIVE／COMPOSE、`PLAY` 可停止、`REC` 刪字和清空；prosign `<SK>`；解碼器遇到長短差 2 倍時回頭修正；靜音和改音高不再重設解碼器。藍牙順延到 0.3 |

## 獨立 repo 和上架 { #release }

2026-11-18 為了上架 [The Firmware Hub](../firmware/hub.md#listing)，韌體搬到 [daviddwlee84/fm1-morse](https://github.com/daviddwlee84/fm1-morse)（用 `git subtree split` 保留歷史），在本 repo 以 submodule 引回 `firmware/morse`。

- **授權**：2026-11-18 從 MIT 改成 GPL-3.0-or-later，讓拿去改的人分享出來的版本也要開源。fm1-base 的 MPL-2.0 和 `common/` 的 MIT 都可以併入 GPL 作品；SLOOP 就是 GPL-3.0 加 felucca-bsp 的前例。之後的藍牙用 fm1-base 自己的 stack，不會連結 JieLi 的閉源函式庫，所以避開了 GPL 不能連結閉源庫的問題。`57ea786` 以前的版本以 MIT 公開，仍可依 MIT 使用。套件內容不受影響（改授權前後的 `.fwsc` 完全相同）。
- **新 repo 的內容**：根目錄有 GPL-3.0 `LICENSE`、`fmhub.json`（`kinds: other`、`sounds: none`，卡片圖是 `docs/screen.png`）、英文 README（含 Hub 要求說明的「其他韌體的資料」）、鎖定版本的 fm1-base submodule，以及自帶一份 `common/`（hostlink、ledfx）。
- **identity 前綴改成 `FM-1_41`**：原本的 `FM-1_13` 和 Hub 上 FM-1 Midi-Mania 的 `FM-1_137` 開頭相同。目錄裡沒有任何 `FM-1_41x`，但送審前仍要用 Hub 的 #check 確認。開發版是 `FM-1_4100`，release 版是 `FM-1_41` 加 XXYYZZ（0.3 是 `FM-1_41000300`）。存檔位置不變，進度會保留。
- **build**：在本 repo 用 `third_party/fm1-base` build（`just fw-test morse`、`just morse-release 0.3`）。子 repo 裡的那份 fm1-base 是給它單獨 build 用的，`fw-setup` 不會去抓。
- **改程式的流程**：在 `firmware/morse` 裡 commit 並推到 fm1-morse，再到本 repo commit 新的 submodule 指標。

上架還要做的：

1. 在實機上走 USB-MIDI 更新路徑測三件事：從 Morse 換到別的韌體（2026-11-18 通過，Morse 0.3 → SLOOP）、從別的韌體裝 Morse（2026-11-18 通過，SLOOP → Morse release 0.3 `FM-1_41000300`）、從原廠 V15 裝 Morse（2026-11-18 通過，015 → Morse release，fast install）。三項都在新晶片 `0xB36014` 的機器上做。
2. 發一個附 `morse-0.3.fwsc` 的 GitHub release：2026-11-18 發布 [v0.3](https://github.com/daviddwlee84/fm1-morse/releases/tag/v0.3)（tag 在 `748402f`），附 `morse-0.3.fwsc`（SHA-256 `1c563fda…c0f1b3`，GitHub 的 asset digest 相符）和 `firmware.json`。
3. 用 #check 確認 identity：2026-11-18 本人確認 `FM-1_41000300` 是空的；KB 的「Check your file」讀得到 `fmhub.json`，回報 Fine，卡片圖 240×240、169 KB。
4. 用 Hub 的送審表單或 email 送審（本人）。

## 路線圖 { #roadmap }

| 版本 | 內容 |
|---|---|
| 0.4 | 藍牙：own BLE stack 的 peripheral（Mac、iPad、手機上的 Vail 走藍牙）和 central（一台 FM-1 掃描並連上另一台）。BLE connection interval 是 11.25 ms，每個按鍵事件會晚 0–11 ms 到；對策想用 note 的 velocity 帶上長度、收的一方加一點播放緩衝，等量過再定。實機要等有第二台 |
| 之後 | host 送日期，讓間隔複習改成天數；USB-C 直連（見[接口與連線](../hardware/io.md#two-units)） |
