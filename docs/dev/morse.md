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
- **發報速度**（K4）：你手打的速度，用來設定 iambic keyer 的節奏、KEY 頁解碼器的起點、RHYTHM 頁和 TIMING 檢視的目標長度。新手的手通常比耳朵慢，所以這個要另外設低一點。0.4 起 PRACTICE 的打碼卡不再用它判斷（見[判定](#judging)）。

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
- **一回合怎麼出題**（0.4）：先排新字和到期的卡（盒號低、等最久的優先）。其中盒號 0–1 的弱卡（包括剛介紹的新字）會在回合後半再出一次，重複那次答對不會再升盒。還有空位，才用沒到期的卡補滿，最弱的優先。0.3 每張卡一回合只出一次，空位直接用熟字補，所以新的一組開放後，一回合裡大部分還是 K U R E 這些熟字（2026-11-18 試用時的回饋 `[實機]`）。
- **新字一次介紹幾個**：開放一組 5 個字時，每回合最多介紹 3 個，所以會分兩回合介紹完。後一回合又出現 NEW LETTER，是同一組剩下的字，不是又開了一組。
- **回合數**：做完一回合，或答過題後中途離開（`HOME`、換頁），都算一回合。
- **用回合數，不用日期**：FM-1 沒有關機後還在走的時鐘，所以排程數的是「做了幾回合」。之後接電腦時可以讓 host 送日期，改成真正的天數，但還沒做。
- **新字按 Koch 順序**（LCWO 的 41 字：K M U R E S N A P T L W I . J Z = F O Y , V G 5 / Q 9 2 H 3 8 B ? 4 7 C 1 D 6 0 X），5 個一組，像五十音的一「行」。一回合最多 3 個新字，每個新字先播兩次給你聽，然後才考。開著的字全部「認得」到 2 號盒以上、「打得出」到 1 號盒以上，才開下一組。
- **先選擇，後回想**：「認得」的卡在 0–2 號盒是聽完從四個裡選（四個選項對應 E4 F4 G4 A4 四個亮燈的鍵，干擾選項挑碼型相近的），3 號盒以上改成聽完直接打出來。
- 回合結束顯示答對、答錯、升了幾盒，以及連續幾回合正確率 80% 以上（streak）。中途按 `HOME` 離開，已答的部分照樣存。

### 打碼卡怎麼判 { #judging }

0.3 以前，打碼卡（看字打碼、聽了照打）是用**發報速度**判斷長短：解碼器從 SEND 的 dit 起算，長過 2 個 dit 才算 dah。預設 SEND 10 WPM 時，dah 要按到 240 ms 以上。但 PRACTICE 播給你聽的是 20 WPM，dah 只有 180 ms。照著聽到的節奏打，劃就被當成點：M（−−）讀成 I、T 讀成 E。

這個情況 2026-11-18 在實機上碰到 `[實機]`：打到 ROUND 14 還停在 K M U R E。開下一組要每個字的「打得出」至少到 1 號盒，可是這些卡一直被判錯、回到 0 號盒。

0.4 改成**看答案自己的節奏**（SET 頁 `JUDGING: BY RHYTHM`，預設）：

- **點和劃都有的字**（A、K、R…）：答案裡應該是劃的那幾下，最短的一個至少要是應該是點的最長那一下的 1.5 倍。只比相對長短，快慢都可以。
- **只有一種元素的字**（E T I M S O H 5 0）：答案裡沒有東西可以比，所以和「你的手速」比：劃至少 1.6 個 dit，點最多 2.4 個 dit。手速從前幾張卡延續下來，答對時會往你的實際長度修正。每回合第一張卡還沒有手速可參考，就用聽的速度和發報速度兩個 dit 的中間值（START 是 90 ms）。
- **數量要對**：K 打成 −··（D）一樣判錯。
- **差一點（ALMOST）**：比例在 1.2 到 1.5 倍之間（只有一種元素的字則是界線外 20% 以內），盒號不升也不歸零，回合最後再問一次。
- **字中間停太久**：還沒打滿就停頓時，會多等一下（至少 900 ms）才判斷，不會被切成兩個字。停頓超過 2.5 個點長，最多只算 ALMOST，並提示「run it together」。
- **評級**：每張打碼卡都有，0–100 分，依劃點比是否接近 3:1、字內間隔是否接近 1 個點、同種元素長度是否一致來算。GREAT ≥ 85，GOOD ≥ 65，CLOSE ≥ 40，其餘是 OFF。評級和對錯分開算。
- **結果頁**：SHOULD 是照你這次的點長排出的標準節奏，YOU 是你實際按的，兩列用同一個比例尺。判反的那一下標紅，停太久的間隔畫紅底線。下面一行提示要改什麼。結果頁另外顯示解碼器讀成的字（`READ AS`）。

`JUDGING: STRICT` 是 0.3 的判法（解碼器照 SEND 的速度讀，讀出的字對了才算對），適合想練固定速度的人。

### 先用聽的：學習方式設定 { #by-sound }

**預設是「SOUND」**：出題只用聲音，畫面上**不畫點劃**，字表只顯示熟練度，靜音時燈號照真實節奏閃，也不畫成 ·−。

原因：Koch 法、LCWO、CW Academy 這類教學都主張把每個字的聲音當成一個整體記，不要看點劃圖。看圖學的人聽到訊號時會先數幾點幾劃再查表，速度常卡在每分鐘 10–15 字，之後得花時間改掉。`[調研]`（這是教學界的普遍說法，我還沒找到量化研究。）同樣的理由，字元速度預設 20 WPM，用 Farnsworth 把字和字之間拉開到有效 8 WPM，而不是把每個字放慢。

這是「學認字」的規則。「練發報」時看自己打出的長短是有用的回饋，所以 KEY 頁的 CODE／TIMING 檢視、RHYTHM 頁、TYPE 頁的「鍵上點劃」都和這個設定分開，用的時候自己打開。

0.4 起 SOUND 模式也能偷看：新字和答案畫面按 `SEQ` 顯示點劃，換卡就藏回去。這是 2026-11-18 試用時的需求：新字只聽一兩次，怕把相近的字聽錯 `[實機]`。考「認得」的題目時不能看，不然就變成看圖選字。footer 也改成寫出 `PLAY again`（0.3 寫「any key: next」，但 `PLAY` 其實一直可以重播）。

「SIGHT」是另一個選項：顯示點劃，閃卡在低盒號時用「看碼選字」，KEY 頁畫出正在打的碼。切過去時畫面會提示「SOUND FIRST IS FASTER LATER」。

| | SOUND（預設） | SIGHT |
|---|---|---|
| 認得的卡，0–2 號盒 | 聽 → 四選一 | 看點劃 → 四選一 |
| 認得的卡，3 號盒以上 | 聽 → 打出來 | 聽 → 打出來 |
| 打得出的卡 | 看字母 → 打出來 | 同左 |
| 靜音時認得的卡 | 看燈號節奏 → 四選一 | 看點劃 → 四選一 |
| 畫面上的點劃 | 不顯示 | 新字、答案、KEY 頁、字表都顯示 |

### 進度：跳過、退回、歸零 { #progress }

**手動修改都要確認才寫入**：螢幕會問「SAVE CHANGES?」「MARK THESE KNOWN?」「LOAD THIS BACKUP?」，只有按 `PLAY` 才寫入，按 `HOME` 就放棄。問的時候其他鍵和旋鈕都沒作用，也不會把琴鍵當成電鍵。

- **CHART 編輯**：在 CHART 按 `REC` 進入 EDIT。所有改動先存在一份副本，標題會顯示 `EDIT *`。
    - `OCT-`／`OCT+` 或 `K1`：游標所在字的等級（兩個方向的盒號一起改）。降到 0 就是「忘了」，那個字會重新當新字出現。在還沒開放的字上調高，會一路開放到它那一組。
    - `K2`：開放的組數 ±1。新開放的字直接放在解鎖門檻（認得 2 號盒、打得出 1 號盒）；關掉的字變回新字。
    - `FX`：全部退一盒，覺得生疏時用，每個字都會早點回來複習。
    - 按住 `SEL` 2 秒：全部歸零。
    - `PLAY` 或 `REC`：問要不要存；`HOME` 不存就離開。編輯中 `ALGO` 不能換頁。
- **分級測驗**：PRACTICE 開始畫面轉 `SELECT` 選 TEST，`PLAY` 開始。還沒開放的下兩組（10 個字）各考一次「聽了選」。結束時列出答對的字，問要不要標成已會（認得 3 號盒、打得出 2 號盒）。整組都答對就開放那一組。測驗不算回合，也不影響 streak。
- **開始畫面的提示**：`NEXT GROUP WAITS ON …` 列出還沒到門檻、擋住下一組的字。

### 備份與還原 { #backup }

進度存在 FM-1 的 `0xDA000`（見[存檔](#flash)）。換韌體時不一定保得住：2026-11-18 實測從 SLOOP 2.8.5 切回來時還在，刷回原廠 V15 就沒了 `[實機]`。換韌體前先備份。0.4 起有兩種方式：

| 方式 | 怎麼做 |
|---|---|
| 網頁（Chrome、Edge） | [MORSE on the web](https://daviddwlee84.github.io/fm1-morse/web/)（fm1-morse 的 `web/index.html`，舊網址 `web/progress.html` 會轉過去）：Connect、Download the progress；還原時選檔案、Send it to the FM-1 |
| 指令列 | `just morse-backup FILE.json`、`just morse-restore FILE.json` |

檔案是 JSON，列出每個字兩個方向的盒號（0–5）和最後作答的回合，可以手改。FM-1 收到後先檢查 CRC 和內容是否合理，再顯示回合數、開放幾個字、答過幾題，按 `PLAY` 才載入。協定見 [`host/morse/PROTOCOL.md`](https://github.com/daviddwlee84/M-VAVE_FM-1/blob/main/host/morse/PROTOCOL.md) 的 Progress 一節。

- **USB 或藍牙**：0.4 只能走 USB；0.5 也能走[藍牙](#bluetooth)，網頁會顯示走哪一條。網頁找機器時不看 port 名稱，而是看哪個 port 在送 HELLO，因為 macOS 會沿用這個 USB 裝置第一次接上時的名字（這台叫 `Felucca`）`[實機]`；藍牙的 port 叫 `FM-1 XXXX`。走藍牙時，網頁等備份回來的時間從 5 秒放寬到 15 秒。
- **入口**：fm1-morse README 的最上面，以及 Hub 卡片：`fmhub.json` 的 `editor` 欄位（「a web editor for its sounds or settings」`[上游]`）2026-11-18 填了這個網址。
- **Live**（2026-11-18 加入，不需要新韌體）：網頁顯示 FM-1 讀到的字（`T_CHAR`，在 KEY 頁發報或在 TYPE 頁打字）和按鍵燈號（note 0），也能送字給 FM-1 播（`T_TEXT`、`T_STOP`），等於網頁版的 `morse-send`，加上只顯示、不打進系統的 `morse-type`。網頁會回應 HELLO，所以 FM-1 會顯示 `PC`。送字在實機上試過 `[實機]`。

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
| `just morse-backup FILE`、`just morse-restore FILE` | 學習進度的備份與還原（韌體 0.4 起，見[備份與還原](#backup)） |
| `--bluetooth`（放在子指令前） | 先打開 Mac 的藍牙 MIDI 視窗，等 FM-1 從藍牙說 HELLO（韌體 0.5，見[藍牙](#bluetooth)） |

解碼在 FM-1 上做，host 只收結果（`T_CHAR`），所以畫面上看到的和打進電腦的一定一樣。host 會印出走的是 USB 還是 Bluetooth；兩條都有時用 USB。host 每收到一次 HEARTBEAT 就回一個 HELLO_ACK；FM-1 超過 10 秒沒收到 host 的東西就當成斷線。

## 兩台互傳

現況和選項見[接口與連線](../hardware/io.md#two-units)。這台軟體的設計是**線路上傳按鍵動作（note on / off），不傳文字**，收的一方自己解碼，像真的電報線。好處是同一套格式可以走 USB、兩種藍牙 stack、USB host、經電腦轉送，Vail 也聽得懂。

我手上只有一台，所以直連排在下一版（見[路線圖](#roadmap)），先用電腦和 Vail 跟人互傳。

## 藍牙（0.5） { #bluetooth }

2026-11-18 做好，host simulator 和 fm1-emu 都驗證過，16:20 刷進實機（進度保留）。同一天在實機上用藍牙連上 Mac：`morse-send`、進度備份都能用，按鍵的時間戳記和 USB 一樣準 `[實機]`。

### 能做什麼

FM-1 當 BLE-MIDI peripheral，和 USB 同時開著：

- Mac 不接線就能用 Vail、[MORSE on the web](https://daviddwlee84.github.io/fm1-morse/web/)（Live、備份、還原）、`just morse-*`。
- 按鍵（Vail 格式的 note 0）同時從 USB 和藍牙送出。
- HELLO、`T_CHAR`、`T_PROG*` 走 host 回答的那條路。

兩台 FM-1 互打（`ble_central`）要等有第二台（見[路線圖](#roadmap)）。

### 怎麼連

1. FM-1 的 *SETTINGS > BLUETOOTH* 是 ON（預設）。header 的 `BT` 灰色代表等待中，藍色代表已經有電腦連上。
2. Mac：「音訊 MIDI 設定」→ *視窗 > 顯示 MIDI 工作室* → 藍牙按鈕 → `FM-1 XXXX` 旁邊的 *Connect*，不用配對碼（Conductor 在 2026-11-17 實測過 `[實機]`）。
3. 指令列加 `--bluetooth`（例如 `PYTHONPATH=host uv run python -m morse --bluetooth send cq`），會打開那個視窗，等 FM-1 從藍牙說 HELLO。這個視窗和 Conductor 用的是同一個 `conductor.blelink`。
4. 網頁照常按 Connect。狀態會顯示 `over Bluetooth` 或 `over USB`（看 HELLO 的最後一個 byte）。

USB 和藍牙都連著時：
- host 找 port 時優先用 USB（`sniff_port`）；
- 網頁用先聽到 HELLO 的那條路；
- 按鍵兩邊都送。

### 怎麼做的

照 Conductor 的寫法，有一個地方不同：
1. **`fm1.toml`**：`ble = true`、`ble_stack = "own"`、`ble_diag = false`、`ble_midi_time = true`。
2. **`APP_BLE_STORE`**：`app_ble_load`／`app_ble_save` 什麼都不存。own stack 用 flash 晶片的 unique ID 產生位址，Mac 和 iOS 不配對也能連，所以**藍牙不寫 flash**（fm1-emu 兩種晶片都驗證過）。
3. **開機要不要開藍牙**：用 `APP_BLE_AT_BOOT`。裝置上這一步在 `app_init` 之前執行，所以 `morse_ble_off()` 自己把紀錄讀進一份副本。
4. **開關存在哪**：`SET.mute` 的 bit 1（bit 0 是靜音）。**存檔格式不變**，0.4 的紀錄直接讀，舊紀錄的這個 bit 是 0，所以藍牙是開的。
5. **和 Conductor 不同的地方**：MORSE 在 1 kHz 的 `app_tick` 讀 MIDI，因為收訊按鍵要準。藍牙送來的 SysEx 封包（CIN 4–7）先放進一個 256 格的 ring（`bx`），由主迴圈的 `ble_in()` 用 `hl_sx_packet` 組回 frame，再交給 `host_frame(…, HL_BLE)`。`host_frame` 會送 frame、也可能改進度，所以不在 tick 裡跑。
6. **收訊按鍵的時間**：開了 `ble_midi_time` 後，同一個 connection event 裡的 note 會一起到，但各自帶著送出端的時間（`plat_midi_in_t24`）。MORSE 把每個 note 的變化都當成一個事件，時間是 `now − 已經過了多久`，最多往回 500 ms，而且不早於上一個事件。所以一個短點的 on 和 off 就算同一個 tick 到，長度也不會變。
7. **送出**：`plat_midi_send(…, TO_USB | TO_BLE_HOST)`，藍牙關掉時只送 USB。own stack 送出時會帶真實的 13-bit 毫秒時間戳記 `[上游]`（`docs/bt/BLE-STACK.md` §6）。

### 時間準不準

這是摩斯特有的問題：點在 20 WPM 只有 60 ms。BLE 的 connection interval 大約 7.5–15 ms，按鍵事件會晚 0–15 ms 才送到，**誤差可能達到一個點的 25%**。

- **送出（2026-11-18 實測）** `[實機]`：TYPE 頁用 20 WPM 播字（點 60 ms、劃 180 ms，FM-1 自己計時，當作標準答案）。USB 和藍牙同時連著，Mac 上用 python-rtmidi 同時錄兩個 port，每個 note 記兩種時間：到達時間（`perf_counter`），以及 CoreMIDI 的封包時間戳記（rtmidi 的 delta）。

    | 46 個 mark、34 個字內停頓 | 點（60） | 劃（180） | 停頓（60） |
    |---|---|---|---|
    | USB，到達時間 | sd 0.1，最差 ±0.3 ms | sd 0.2，±0.8 | sd 0.1，±0.4 |
    | 藍牙，到達時間 | sd 1.7，最差 ±5.5 ms | sd 4.6，±16 | sd 2.8，±9.5 |
    | 藍牙，CoreMIDI 時間戳記 | sd 0.3，最差 ±1 ms | sd 0.4，±1 | sd 0.4，±1 |

    **CoreMIDI 照 FM-1 送的 BLE-MIDI 時間戳記排時間，結果和 USB 一樣準**，誤差在 1 ms 以內（13-bit 時間戳記的精度就是 1 ms）。照到達時間算，最差會差 16 ms，大約是一個點的 25%，和研究時的預估相同。第一輪錄音有幾個劃只有 106–123 ms，兩種時間都一樣短，比較像是播放真的被下一個鍵打斷了，不是傳輸造成的；第二輪每個字都等播完再按，就沒有再出現。
- **接收端用哪一種時間**：用 CoreMIDI 時間戳記的程式（python-rtmidi、DAW）不會抖。**Chrome 的 Web MIDI 也是** `[實機]`：MORSE on the web 的 *Timing* 區，2026-11-18 在 TYPE 頁播 20 WPM：

    | | 短按（60） | 長按（180） | 字內停頓（60） |
    |---|---|---|---|
    | 藍牙，`event.timeStamp` | 平均 60.2，sd 0.4 | 180.2，sd 0.4 | 60.2，sd 0.4 |
    | 藍牙，網頁收到的時間 | 60.4，sd 3.1 | 180.6，sd 3.2 | 59.9，sd 2.5 |
    | USB，`event.timeStamp` | 60.1，sd 0.1 | 180.3，sd 0.1 | 60.1，sd 0.1 |

    所以網頁上的 Vail 只要用 `event.timeStamp`，藍牙就和 USB 一樣準；它實際用哪一個沒看程式碼，但 Vail 的 Decoder 把 FM-1 的發報量成 19.9 WPM（設定是 20）`[實機]`。
- **收進**：FM-1 照送出端的時間解碼（見上面第 6 點）`[模擬]`，模擬器的藍牙替身沒有時間戳記，所以只驗證了程式路徑。
- **用 velocity 帶長度的備案**：接收端照時間戳記排時就不需要。只有在接收端用到達時間、又會誤判時才考慮（見 [Backlog](#backlog)）。

### 網頁直接連藍牙（Web Bluetooth） { #web-bluetooth }

MORSE on the web 的 *Over Bluetooth, directly*（2026-11-18 加入，韌體 0.5 起）：Chrome／Edge 用 Web Bluetooth 掃描 BLE-MIDI 的 service（`03B80E5A-EDE8-4B33-A751-6CE34EC4C700`，FM-1 的廣播裡有它），選 `FM-1 XXXX` 就連上，不用到系統設定按 Connect。

- 網頁自己讀寫 BLE-MIDI 封包：SysEx 切成 20 byte 的封包送出（不知道協商到的 MTU，用最小的）；收進時把跨封包的 SysEx 和 note 組回來，每個事件用 13-bit 的 BLE-MIDI 時間戳記換算成網頁的時間（每個封包裡最新的那個事件最接近送出的時刻，取最小的延遲當兩個時鐘的差）。Timing 區照樣能用。
- 這條連線只有網頁能用，Vail 和指令列看不到；要給 Vail 用就用 *Connect*（經過系統的藍牙 MIDI）。
- macOS 上系統已經用藍牙 MIDI 連著時，Chrome 會不會共用那條連線還沒試 `[調研]`。
- 封包的切法和解法用 node 測過（20 byte 以內、跨封包的 SysEx、running status、同一個封包裡兩個 note 的間隔、13-bit 時間戳記進位）。
- **實機** `[實機]`（2026-11-18，韌體 0.6）：Chrome 的配對視窗列出 `FM-1 3399`，Pair 之後狀態寫 `MORSE 0.6 connected over Bluetooth`，Live 收到 TYPE 頁的字和片語。Timing（22 WPM，點 54 ms、劃 162 ms）：照網頁換算的 BLE-MIDI 時間戳記，sd 0.4–0.5 ms；照通知到達的時間，sd 18–20 ms，比經過 CoreMIDI 的到達時間（sd 約 3）抖得多，所以這條路一定要用時間戳記。

### 瀏覽器

| 平台 | 網頁（Vail、MORSE on the web）走藍牙 |
|---|---|
| macOS Chrome / Edge | 在「音訊 MIDI 設定」連上後，藍牙 MIDI 就是一般的 CoreMIDI port，Web MIDI 應該看得到 `[調研]`。MORSE on the web 靠 HELLO 找 port |
| Android Chrome | 有 Web MIDI；藍牙 MIDI 裝置要先由系統或某個 app 連上 `[調研]` |
| iPad / iPhone | Safari 沒有 Web MIDI，網頁用不了；要用支援藍牙 MIDI 的 app `[調研]` |

### 已知的限制

- **一次只連一個 host**：own stack 同時只有一條連到電腦的 BLE MIDI 連線 `[上游]`。iPad 連著時，Mac 就要走 USB。
- **macOS 不會自己連回來** `[實機]`：FM-1 重開後要到藍牙 MIDI 視窗再按一次 Connect（Conductor 的經驗），可以用 `--bluetooth` 打開那個視窗。
- **大小**：開發版 173,744 B，比 0.4 多 35.7 KB；RAM `.data+.bss` 38,572 B（見[大小](#size)）。
- **電池**：藍牙開著的耗電還沒量。不用時到 SETTINGS 關掉，下次開機也維持關閉。
- **模擬器沒有無線電**：fm1-emu 只能驗證開關、存檔、不寫 flash；連線本身要在實機上測。

### 驗證

已做 `[模擬]`：
- host simulator：藍牙替身連上後 HELLO 走 BLE；從 BLE 送 HELLO_ACK（傳輸 `1`）、`T_TEXT`（播出 E）、`T_PROG_REQ`（`T_PROG` 從 BLE 回來）；KEY 頁按鍵同時送 USB 和 BLE；從 BLE 收到的 T 解得出來；BLUETOOTH OFF 之後按鍵只走 USB，存檔、重開機後還是 OFF；藍牙不增加 store 寫入。
- fm1-emu 兩種晶片：疊在實機 0.4 前的備份上，關掉藍牙、重開讀回 OFF、再打開，ROUND 17 的紀錄保留，開機 0 次寫入。

實機 `[實機]`（2026-11-18）：
- Mac 用藍牙連上 `FM-1 3399`（port 叫 `FM-1 3399 Bluetooth`），拔掉 USB：`just morse-send paris` 播得出來；`just morse-backup` 走藍牙 1.7 秒，內容和走 USB 時相同。
- 抖動：見上面的[時間準不準](#bluetooth)，CoreMIDI 時間戳記的誤差 ≤ 1 ms。
- 0.5 拔線後 header 的 `PC` 要大約 10 秒才消失：FM-1 要等 10 秒沒收到電腦的訊息（`HOSTLINK_TIMEOUT_MS`），才判定電腦離線。0.6 改成 USB 連續 0.5 秒收不了 SysEx（拔線後匯流排 suspend，`plat_sysex_room()` 是 0；USB session 的號碼不會變，所以原本的檢查抓不到）就判定離線，2026-11-18 實機確認：接上 USB、`PC` 亮起後拔線，半秒內消失，接著線時幾乎沒看到閃 `[實機]`。判斷本身有單元測試（`just fw-common-test`：停 0.499 秒不算、0.5 秒算、中間恢復就重新計時）；模擬器的 `usb unplug` 把 session 歸零，走的是原本的檢查，所以拔線後真的會 suspend 這件事要在實機上確認。

    **會不會常常閃**：`plat_sysex_room()` 是 0 的情況只有四種：沒有 USB 設定、匯流排 suspend（拔線、Mac 系統睡眠）、正在串流一個 frame（MORSE 不用），以及送出的 ring 滿了。電腦接著時，macOS 的 USB-MIDI driver 每毫秒都會讀 IN endpoint，就算沒有程式開著 port 也一樣，所以 ring 只會滿幾毫秒（例如送備份的 6 個 frame）。要連續 0.5 秒都是 0，只有拔線、Mac 睡眠，或電腦卡住不讀。真的掉了，FM-1 下一秒就送 HELLO，程式回應後 `PC` 又會亮起來。刷 0.6 後要在實機上看：拔線時 `PC` 會不會馬上消失，接著線時會不會閃。

- **MORSE on the web**（Chrome）經藍牙連上，狀態寫 `MORSE 0.5 connected over Bluetooth`，Live 顯示 FM-1 讀到的字。
- **Vail**（vailmorse.com 的 Repeater）：只用藍牙連著，不用任何設定，FM-1 的 KEY 頁發報就出現在 Vail 上。Decoder 房間解得出 TYPE 頁播的字和片語，速度顯示 19.9 WPM。
- **Vail 的 Decoder 每個字母都分開**：這是 Farnsworth 間隔造成的，不是 Vail 的問題。START（20/8）時字和字之間要停 890 ms，以 20 WPM 的點（60 ms）來算是 15 個點，超過字間的 7 個點，所以 Vail 把每個字母都當成一個詞。BUILDING（20/13）停 435 ms（約 7 個點），剛好在界線上；QSO（22/18）大約 4 個點，就會連成詞。實測 `[實機]`：20/13、20/14 還是分開，22/18 就連起來了。要給別人聽時，選 QSO 或 FAST，或把 `K2` 調到接近字元速度。
- **網頁比 Vail 早出字，不是作弊，但也不是解碼**：TYPE 頁按下鍵時，FM-1 就把那個字（`T_CHAR`）送給電腦，然後才開始播，所以網頁上的字比聲音早出現。Vail 是從按鍵的時間自己解碼，要等字間的停頓才知道一個字結束。KEY 頁送的才是 FM-1 解出來的字，也要等字間的停頓。
- 拔掉 USB 只用電池，用藍牙連續使用一段時間，沒有斷線（還沒正式計時）。

- **USB 和藍牙都連著時**：0.5 的網頁用先聽到 HELLO 的那條路，常常是藍牙；本人在 FM-1 上關掉藍牙，網頁才改走 USB `[實機]`。網頁（2026-11-18 起）改成兩條都有時用 USB：聽到藍牙的 HELLO 後再等 1.3 秒，看 USB 有沒有。限制：已經有程式從藍牙回應 FM-1 的話，FM-1 只在藍牙上送 HEARTBEAT，USB 那邊沒有聲音，要等那個程式關掉 10 秒後才會兩邊都送 HELLO。

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

### 大小 { #size }

`just fw-build morse`（2026-11-17）：

| 版本 | app | `.data+.bss` |
|---|---|---|
| 0.1 | 109,508 B | 27,180 / 98,304 B |
| 0.2 | 127,712 B | 28,444 / 98,304 B |
| 0.3 | 127,880 B | 28,444 / 98,304 B |
| 0.4 | 138,076 B（release） | 29,084 / 98,304 B |
| 0.5（藍牙，release） | 173,748 B | 38,572 / 98,304 B |
| 0.6（release） | 173,844 B | 38,572 / 98,304 B |

上限是 581,564 B；RAM code 5,576 / 32,768 B（0.5）；套件 616,003 B，開發版 identity `FM-1_4100`。0.5 開了 own BLE stack（peripheral），多 35.7 KB；之後加 central 大約再多 13–22 KB（fm1-base 的數字），仍然很寬裕。

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
| 0.4：判定、評級、ALMOST、手動編輯、分級測驗、備份格式 | `just morse-unit` | 3016 項全過（例如：M 用 180/180 ms 判對、5 WPM 的手回答 20 WPM 的題目判對、K 打成 −·· 判錯） |
| 0.4：整個 app（SEND 10 時用 20 WPM 的手速答 M、T 判對，STRICT 判錯；錯的對照圖；ALMOST；CHART 編輯取消／存檔／按住歸零；確認時其他鍵無效；分級測驗；備份匯出向量、載入要按 PLAY、缺片拒收；重開讀回） | `just fw-test morse` | 全過 |
| 0.4：host 和網頁的備份格式 | `just morse-test`；網頁的程式碼用 node 跑 | host 56 項全過；網頁產生的位元組和 host 的向量相同 |
| 0.4 刷進實機 | `fm1_install.py`（USB-MIDI，fast install） | 成功；ROUND 16、開放 10 個字的紀錄保留 `[實機]` |
| 0.4 的備份：指令列和網頁 | `just morse-backup`；Chromium 開 `web/progress.html` | 兩者下載的內容相同；網頁要用「誰在送 HELLO」找 port，因為 macOS 把這台叫 `Felucca` `[實機]` |
| 0.4：疊在實機 0.2 紀錄的備份上、改設定、重開讀回 | `emu_rehearse.py` + fm1-emu `uc25hq80`、`p25q80` | ROUND 5 的紀錄保留，改過的速度重開後讀回 `[模擬]` |
| 0.5：藍牙替身上的 HELLO、HELLO_ACK、`T_TEXT`、`T_PROG`、按鍵送出和收進、BLUETOOTH 開關存檔重開、store 寫入不變 | `just fw-test morse` | 全過；`--no-ble` 的 build 也編得過、除了藍牙那段都通過 `[模擬]` |
| 0.5：host 的傳輸判斷 | `just morse-test` | 22 項全過 |
| 0.5 刷進實機 | `fm1_install.py`（USB-MIDI，fast install） | 成功；ROUND 29、開放 20 個字的紀錄保留 `[實機]` |
| 0.5：疊在實機 0.4 前的備份上，BLUETOOTH 關、重開讀回、再開 | `emu_rehearse.py` + fm1-emu `uc25hq80`、`p25q80` | ROUND 17 的紀錄保留；改設定時存一筆紀錄（4 次 erase／program），開機 0 次 `[模擬]` |

## 版本紀錄

韌體 X.Y（`FW_MINOR` in `firmware/morse/app.c`），host X.Y.Z（`__version__` in `host/morse/__init__.py`，加上 `FIRMWARE_KNOWN`），規則和 Conductor 一樣。

| 版本 | 日期 | 要刷機 | 內容 |
|---|---|---|---|
| 韌體 0.1、host 0.1.0 | 2026-11-17 | 是（22:54 刷入，`c3b5cd6`） | 第一版：KEY、TYPE、PRACTICE、CHART、SETTINGS，兩種學習方式，Vail 格式的 MIDI，host 的 `type`／`send`／`listen`。原本規劃分成 0.1（核心）和 0.2（閃卡），因為一起做完、都還沒刷，合併成 0.1 |
| 韌體 0.6、host 0.6.0 | 2026-11-18 | 是（17:00 前用 USB-MIDI 裝 release build `FM-1_41000600`，fm1-morse `60f76cf`，release [v0.6](https://github.com/daviddwlee84/fm1-morse/releases/tag/v0.6)） | 拔線後 `PC` 在 0.5 秒內消失（hostlink：USB 收不了 SysEx 就判定電腦離開，`firmware/common` 同步），實機確認，接著線時沒看到閃 `[實機]`；網頁：*Over Bluetooth, directly*（[Web Bluetooth](#web-bluetooth)）、USB 和藍牙都在時用 USB、*Timing* 區、自己的解碼器（字元速度、劃長、停頓幾個點、整體 WPM）、*With Vail* 和 PRESET 的說明、*Only show it*（只讀進度不存檔）、備份檔名加上時分 |
| 韌體 0.5、host 0.5.0 | 2026-11-18 | 是（16:20 用 USB-MIDI 裝 release build `FM-1_41000500`，fm1-morse `a75c29e`，release [v0.5](https://github.com/daviddwlee84/fm1-morse/releases/tag/v0.5)） | [藍牙](#bluetooth)：fm1-base own stack 的 BLE-MIDI peripheral，和 USB 同時用；SETTINGS 加 BLUETOOTH 開關（存在紀錄裡，格式不變）；header 顯示 `BT`；SETTINGS 頁可以捲動；收訊按鍵照送出端的時間解碼；host 印出走哪條路、兩條都有時優先 USB、`--bluetooth` 打開 Mac 的藍牙 MIDI 視窗；網頁顯示走哪條路 |
| 韌體 0.4、host 0.4.0 | 2026-11-18 | 是（05:20 用 USB-MIDI 裝 release build `FM-1_41000400`，fm1-morse `323d79b`） | 實機回饋（ROUND 14 還停在第一組）：打碼卡改用答案自己的節奏判斷（[判定](#judging)），手速跨卡延續，評級和 SHOULD／YOU 對照圖，ALMOST；SET 頁的 JUDGING 可以切回 STRICT；CHART 編輯、分級測驗、開始畫面列出擋住下一組的字，SOUND 模式下新字和答案可以按 `SEQ` 偷看點劃，弱卡在同一回合重複出現、熟字只在有空位時補，[手動修改都要按 PLAY 確認](#progress)；進度的[備份與還原](#backup)（`T_PROG_*`、host 的 `backup`／`restore`、網頁）。藍牙順延到 0.5 |
| 韌體 0.3 release（`FM-1_41000300`） | 2026-11-18 | 是（Hub 測試時用 USB-MIDI 裝過三次） | fm1-morse 的第一個 release [v0.3](https://github.com/daviddwlee84/fm1-morse/releases/tag/v0.3)：identity 改成 `FM-1_41`，授權改成 GPL-3.0-or-later，程式和 0.3 相同 |
| 韌體 0.3、host 0.3.0 | 2026-11-18 | 是（02:12 刷入，`02b0ea2`） | TYPE 打開鍵上點劃時，鍵盤從 86 px 拉高到 126 px、上方輸入行縮成一行，黑鍵和白鍵都同時放得下碼和字母（0.2 黑鍵只剩碼、白鍵的碼壓到字母，數字頁的 `0` 放不下）。藍牙順延到 0.4 |
| 韌體 0.2、host 0.2.0 | 2026-11-18 | 是（00:14 刷入，`54ff440`） | 實機試用後的回饋：發報速度和聽的速度分開（K4），PRESET 推薦組合，轉速度時顯示 dit／dah 毫秒；KEY 頁 TEXT／CODE／TIMING 三種顯示；RHYTHM 頁（練長短、提示、建議速度）；LISTEN 頁（連續播放五種節目）；TYPE 頁翻三頁（字母、數字符號、片語）、三種排列、鍵上點劃、LIVE／COMPOSE、`PLAY` 可停止、`REC` 刪字和清空；prosign `<SK>`；解碼器遇到長短差 2 倍時回頭修正；靜音和改音高不再重設解碼器。藍牙順延到 0.3 |

## 獨立 repo 和上架 { #release }

2026-11-18 為了上架 [The Firmware Hub](../firmware/hub.md#listing)，韌體搬到 [daviddwlee84/fm1-morse](https://github.com/daviddwlee84/fm1-morse)（用 `git subtree split` 保留歷史），在本 repo 以 submodule 引回 `firmware/morse`。

- **授權**：2026-11-18 從 MIT 改成 GPL-3.0-or-later，讓拿去改的人分享出來的版本也要開源。fm1-base 的 MPL-2.0 和 `common/` 的 MIT 都可以併入 GPL 作品；SLOOP 就是 GPL-3.0 加 felucca-bsp 的前例。之後的藍牙用 fm1-base 自己的 stack，不會連結 JieLi 的閉源函式庫，所以避開了 GPL 不能連結閉源庫的問題。`57ea786` 以前的版本以 MIT 公開，仍可依 MIT 使用。套件內容不受影響（改授權前後的 `.fwsc` 完全相同）。
- **新 repo 的內容**：根目錄有 GPL-3.0 `LICENSE`、`fmhub.json`（`kinds: other`、`sounds: none`，卡片圖是 `docs/screen.png`）、英文 README（含 Hub 要求說明的「其他韌體的資料」）、鎖定版本的 fm1-base submodule，以及自帶一份 `common/`（hostlink、ledfx）。
- **identity 前綴改成 `FM-1_41`**：原本的 `FM-1_13` 和 Hub 上 FM-1 Midi-Mania 的 `FM-1_137` 開頭相同。目錄裡沒有任何 `FM-1_41x`，但送審前仍要用 Hub 的 #check 確認。開發版是 `FM-1_4100`，release 版是 `FM-1_41` 加 XXYYZZ（0.3 是 `FM-1_41000300`）。存檔位置不變，進度會保留。
- **build**：在本 repo 用 `third_party/fm1-base` build（`just fw-test morse`、`just morse-release 0.3`）。子 repo 裡的那份 fm1-base 是給它單獨 build 用的，`fw-setup` 不會去抓。
- **改程式的流程**：在 `firmware/morse` 裡 commit 並推到 fm1-morse，再到本 repo commit 新的 submodule 指標。**要先 commit 指標，再 build**：`just fw-build`、`just morse-release` 都會先跑 `fw-setup`，而它的 `git submodule update` 會把 `firmware/morse` 切回本 repo 記錄的 commit（detached HEAD）。2026-11-18 就因為這樣，build 出來的 0.4 套件裡是舊的出題邏輯，在模擬器裡看到第一回合只有 6 張卡才發現。

上架還要做的：

1. 在實機上走 USB-MIDI 更新路徑測三件事：從 Morse 換到別的韌體（2026-11-18 通過，Morse 0.3 → SLOOP）、從別的韌體裝 Morse（2026-11-18 通過，SLOOP → Morse release 0.3 `FM-1_41000300`）、從原廠 V15 裝 Morse（2026-11-18 通過，015 → Morse release，fast install）。三項都在新晶片 `0xB36014` 的機器上做。
2. 發一個附 `morse-0.3.fwsc` 的 GitHub release：2026-11-18 發布 [v0.3](https://github.com/daviddwlee84/fm1-morse/releases/tag/v0.3)（tag 在 `748402f`），附 `morse-0.3.fwsc`（SHA-256 `1c563fda…c0f1b3`，GitHub 的 asset digest 相符）和 `firmware.json`。
3. 用 #check 確認 identity：2026-11-18 本人確認 `FM-1_41000300` 是空的；KB 的「Check your file」讀得到 `fmhub.json`，回報 Fine，卡片圖 240×240、169 KB。
4. 送審：2026-11-18 本人用 Hub 的送審表單送出（Hub 的紀錄是 UTC 2026-11-17 19:03）。自動檢查 8 項全過（原始碼、`LICENSE`、`fmhub.json`、sounds、release、identity、update loader、兩種晶片），狀態是 waiting for Sven，等維護者審核。
5. 社群清單：2026-11-18 送了兩個最熱門的清單，都是一筆一個 PR。
    - [awesome-fm-1 #30](https://github.com/cicloid/awesome-fm-1/pull/30)（93 stars，放 *Not a synth* 段）
    - [aroum/fm1-custom-fw #36](https://github.com/aroum/fm1-custom-fw/pull/36)（424 stars，放 1.1.4 的第 126 筆，和 CTL-1、MIDIMON 同段）

    fm1-emu、RemoteFM1、FMDB 會自動跟著 Hub 或 GitHub，不用送。fm1-editor.com 等 Hub 上架後看它會不會自己出現。整套流程整理成 repo 的 skill `fm1-hub-listing`。
6. 新版本：2026-11-18 發布 [v0.6](https://github.com/daviddwlee84/fm1-morse/releases/tag/v0.6)（tag 在 `60f76cf`，`morse-0.6.fwsc` SHA-256 `d65b6df5…8fd7e`，刷進實機的那份，digest 相符）。之前同一天發了 [v0.5](https://github.com/daviddwlee84/fm1-morse/releases/tag/v0.5)（tag 在 `a75c29e`，0.4 沒有單獨發 release），附 `morse-0.5.fwsc`（SHA-256 `49433190…154457`，就是刷進實機的那份，GitHub 的 asset digest 相符）和 `firmware.json`。Hub 會跟著 GitHub 的最新 release。`fmhub.json` 的說明加上藍牙。

## 路線圖 { #roadmap }

| 版本 | 內容 |
|---|---|
| 0.5 | [藍牙](#bluetooth) peripheral：Mac、iPad、手機上的 Vail 和 MORSE on the web 走藍牙。已做好，等實機驗證和量抖動 |
| 0.7 | 兩台 FM-1 互打：`ble_central`，一台掃描、選另一台、連上。等有第二台 |
| 之後 | host 送日期，讓間隔複習改成天數；USB-C 直連（見[接口與連線](../hardware/io.md#two-units)） |

### Backlog { #backlog }

想做、但目前刻意不做的，連同理由記在這裡，免得忘記。

| 項目 | 為什麼現在不做 | 什麼時候做、怎麼做 |
|---|---|---|
| **更多符號進閃卡**：`' ! ( ) & : ; + - _ " $ @`，以及 `<AR>` `<SK>` `<KN>` 等 prosign（2026-11-18 試用時提出） | 課程是 LCWO 的 41 字 Koch 課表，通聯和考試幾乎只用這些。這些符號很少用，prosign 在 LISTEN 的 QSO 節目和 TYPE 的片語頁裡聽得到。每個字的盒號存在大小固定的紀錄裡（`LN_N = 41`，`ln_state_t`），加字就要改存檔格式：韌體的 `REC_VER` 升到 2 並寫升級程式，備份格式（`PROG_FORMAT`、host 的 `progress.py`、網頁）也要升版，舊備份要能讀 | 等學完 41 個字。做法：在 Koch 順序之後加第 9、10 組（常用的 prosign 一組，其他符號一組），prosign 的卡片用 `<..>` 連在一起播 |
| **用 velocity 帶 mark 的長度** | 2026-11-18 實測，CoreMIDI 照時間戳記排，誤差 ≤ 1 ms，不需要 `[實機]` | 只在 Chrome 或 Vail 被證實用到達時間、而且會誤判時才做：note off 的 velocity 帶上長度（10 ms 一格），收方也是 MORSE 時用它 |
