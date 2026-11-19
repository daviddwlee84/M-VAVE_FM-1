# 節奏練習機 RHYTHM

把 FM-1 變成隨身的節奏練習機：節拍器加上「聽了之後 tap 回來」的節奏盲聽，逐音告訴你早了或晚了幾 ms，並指出最該改的一件事。玩法比照[摩斯電碼機](morse.md)的 RHYTHM drill；樂理的部分在[樂理練習機](theory.md)。0.3 再加上三頁：不聽 click 打穩拍子（PULSE）、聽出快了還是慢了（JUDGE）、速度術語和拍號的考題（QUIZ）。

**狀態（2026-11-19）：**

- **v0.1（METRO + ECHO）**：在 host simulator、本人 Mac 上的 device build 與 fm1-emu 兩種晶片驗證過 `[模擬]`，見 [v0.1 驗證](#v01-verify)。還沒刷進實機。本人在 emulator 試用的感想是「也還可以」。
- **v0.2（譜面、ECHO 的 READ 模式、ENDLESS）**：在 host simulator、Mac 上的 device build 與 fm1-emu 兩種晶片驗證過 `[模擬]`，見 [v0.2](#v02)。還沒刷進實機。
- **v0.3（PULSE、JUDGE、QUIZ）**：只在雲端環境的 host simulator 與 `just rhythm-unit` 驗證過 `[模擬]`，見 [v0.3](#v03)。device build、fm1-emu 和疊在備份上的預演都還沒做，要在本人的 Mac 上跑。

- 韌體在 `firmware/rhythm/`，GPL-3.0-or-later，名稱 RHYTHM。
- 識別碼 `FM-1_54`。2026-11-18 對照 Hub 的 catalog（12:01 UTC 版）沒人用。原本暫定的 `FM-1_52` 已經被 RemoteFM1、JAMTRACK、QUADRO 用掉（RemoteFM1 的 pattern 正好吃到 `FM-1_5200`）。
- 設定存在 `0xD2000`（`APP_STORE_MAP {0xDC000, 0xD2000}`）。附近已經有人用：THEORY 0.2 的 `0xD8000` 與 `0xD6000`（閃卡進度）、Morse 的 `0xDA000`、面板校正的 `0xDC000`；GUITAR 預定用 `0xD4000`。原本選的 `0xD6000` 和 THEORY 0.2 撞到，第一次存檔就會蓋掉 THEORY 的進度，所以改了。`0xD2000` 在 THEORY 0.2 之後的備份裡是不是空的，**刷機前要用 `just emu-rehearse` 確認**。
- 為什麼另做一個韌體而不是放進 THEORY：本人 2026-11-18 決定 RHYTHM 與 GUITAR 都各自是獨立韌體。

## 操作 { #controls }

六頁，`HOME` 依序切換 METRO → ECHO → ENDLESS → PULSE → JUDGE → QUIZ。設定停手 3 秒後自動存。0.3 的三頁的操作在 [v0.3](#v03)。

| 控制 | METRO | ECHO | ENDLESS（0.2） |
|---|---|---|---|
| `PLAY` | 開始 / 停止 | 新的一題 / 停止 | 開始 / 結束 |
| `REC` | tap tempo（照拍子按，取最後 4 下） | 同一題再來一次 | |
| `SEL` | | HEAR（盲聽）/ READ（看譜）切換（0.2） | |
| `SELECT` | BPM ±1（30–240） | 同左 | 起始 BPM |
| `OCT-` / `OCT+` | BPM ±5 | 同左 | 同左（遊戲中不能改） |
| `K1` | 拍號 2/4、3/4、4/4 | 同左（停止時） | 同左 |
| `K2` | 細分：OFF、8 分、3 連音、16 分 | 難度 1–8（見下表） | 起始難度 |
| `K3` | Gap click：OFF、1+1、2+1、2+2、4+2、4+4（響幾小節 + 靜音幾小節） | 1 或 2 小節 | |
| `K4` | LAT：每一下 tap 要扣掉的延遲（ms，-30 到 +90） | 同左 | 同左 |
| 任一琴鍵 | tap：畫面顯示這一下離最近的拍子幾 ms | tap 回答 | tap 回答 |

### METRO

一般的節拍器，第一拍是高音的重音。開了 gap click 之後，靜音的小節裡畫面只寫 `GAP: KEEP TAPPING`，拍子格也不亮，琴鍵燈也不閃，你繼續在任一鍵上 tap。每一下都和「如果節拍器還在響」的拍點比，最近 8 下畫在 EARLY / LATE 的刻度上（中間灰色那一格是 GOOD 的範圍），下面寫最後一下、平均、以及只算靜音小節的平均（`GAP`）。回到有聲的小節時，看平均有沒有跑掉，就知道內在的時鐘準不準。

### ECHO

1. `PLAY`：count-in 一小節（數字 1 2 3 4）。
2. **LISTEN**：放一到兩小節的節奏（低一點的木頭聲），節拍器以小聲的 click 繼續。畫面只有拍子格與 playhead，**不顯示音符**，是盲聽。
3. 再 count-in 一小節。
4. **YOUR TURN**：在任一鍵上把剛剛的節奏 tap 回來，節拍器小聲地繼續。
5. **結果**：分數（%），每個音一格，綠色 GOOD、黃色 OK、紅色差太多、紅框是漏掉。下面的白線是你實際 tap 的位置，灰線連到它該在的地方。再下面是平均誤差、GOOD 的數量、漏掉與多按的次數，以及這題的 Kodály 唸法（例如 `TA TI-TI REST TA`）。footer 是提示。

`REC` 重來同一題，`PLAY` 換一題。

**READ 模式**（0.2，`SEL` 切換）：畫面一開始就顯示譜，不先放給你聽。count-in 一小節之後直接照譜 tap，計分方式相同。這就是視譜練習。

### 譜面 { #notation }

0.2 起，ECHO 的結果畫面、READ 模式和 ENDLESS 都會畫出單線的打擊樂譜 `[模擬]`：

- 實心符頭、往上的符桿；同一拍裡的八分與十六分用 beam 連起來，和印出來的譜一樣。
- TA-A 是空心符頭，休止符畫成四分休止符。附點八分有附點和半截的第二條 beam。三連音上面標 `3`。SYN-CO-PA 是旗子八分、四分、旗子八分。
- 線的下面有拍點，小節之間畫小節線。
- 結果畫面裡，符頭依結果上色（綠 GOOD、黃 OK、紅差太多、紅色空心是漏掉）。你實際 tap 的位置是線下的白線，灰線連到它該在的地方。

一小節時每拍約 53 px，十六分很清楚。兩小節時每拍約 26 px，十六分的符頭會縮成 4 px 寬，擠但還看得出來。

### ENDLESS { #endless }

0.2 新增，玩法類似「節奏天國」的 call and response，中間不停：

1. `PLAY`：count-in 一小節。
2. 放一小節節奏（**LISTEN**，看不到譜），接著下一小節換你在任一鍵上 tap 回來（**YOUR TURN**；這一小節的第一拍是比較大聲的 click，琴鍵燈微亮）。然後馬上接下一句。
3. 每 4 回合是一段。一段結束後重新 count-in，**BPM 加 6**；每兩段**難度加 1**（到 8 為止）。
4. **血條** 100 起算，每回合結算一次：
    - 漏掉一個音 -12、多按一下 -8、紅色 -6、黃色 -2，一回合最多扣 40，所以最少要連錯三回合才會輸。
    - 整回合全部 GOOD 而且沒有多按，回 +6。
5. 血條歸零就結束，分數是撐過的回合數。最佳紀錄存在設定裡（`BEST`），重開機也還在。

畫面上有血條、回合數、目前的 BPM、難度、BEST，以及上一回合的譜（依結果上色）、你的 tap 位置、GOOD 數、血條的增減和提示。

## 難度 { #levels }

每拍 12 tick，所以 8 分、16 分、3 連音都落在整數 tick 上。cell 依 [theory.md](theory.md) 的 Kodály 順序加進來，新的 cell 出現的機會是舊的 3 倍；一題不會以休止符開頭，不會連兩個休止符，2 拍的 cell 不會跨小節。

| 難度 | 新加的 cell | 寫法（host 測試的 `pattern`） |
|---|---|---|
| 1 | TA（四分）、TI-TI（兩個八分） | `q`、`ee` |
| 2 | REST（四分休止） | `r` |
| 3 | TA-A（二分） | `h` |
| 4 | TIKA-TIKA（四個十六分） | `ssss` |
| 5 | TI-TIKA、TIKA-TI | `ess`、`sse` |
| 6 | TIM-KA（附點八分 + 十六分） | `ds` |
| 7 | SYN-CO-PA（八分、四分、八分，2 拍） | `syn` |
| 8 | TRI-O-LA（三連音） | `trip` |

## 評分與提示 { #scoring }

- 每個音的誤差 = tap 的時間 − 該在的時間（ms，正的是晚）。
- **GOOD**：±min(35 ms, 拍長的 7%)；**OK**：兩倍；更遠是紅的。離得超過半拍就不算是在回答這個音。
- tap 和音的配對用 dynamic programming（最小總誤差的 alignment），所以漏掉一個音或多按一下，後面的音不會全部錯位。
- 分數 = (2 × GOOD + OK) / (2 × (音數 + 多按的次數))。
- 提示只給一個，依序檢查：

| 提示 | 條件 | 第二行 |
|---|---|---|
| `MISSED NOTES` | 漏了 2 個以上 | `PLAY: HEAR IT AGAIN` |
| `EXTRA TAPS` | 多按 2 下以上 | `ONE TAP A NOTE` |
| `SYNCOPATION SQUARED` | 切分音（SYN-CO-PA 中間那個）和周圍的音相比偏了超過 OK 的範圍：被拉回正拍 | `HOLD THE OFF-BEAT` |
| `LATE AFTER RESTS` | 休止符後的第一個音比其他音平均晚超過 GOOD 的範圍 | `COUNT THROUGH THE REST` |
| `UNEVEN 16THS` | 完整 tap 出來的 TIKA-TIKA，間距和平均差超過一個十六分的 12%（至少 8 ms） | `SAY TI-KA-TI-KA` |
| `RUSHING` / `DRAGGING` | 整體平均偏早 / 偏晚超過 GOOD 範圍的 60% | `WAIT FOR THE CLICK` / `LEAN INTO THE BEAT` |
| `MISSED NOTES` / `EXTRA TAPS` | 漏了或多按 1 個 | |
| `GOOD` / `CLOSE` | 全部 GOOD / 其他 | |

門檻都是先訂的數字，要等實機試過再調。

## 時間怎麼量 { #timing }

- **時鐘是音訊的 sample 數。** render 裡每個 click 與音符都排在自己的 sample 上，不是排在下一個 block（那樣會有最多 5.8 ms 的抖動）。
- **按鍵用 1 kHz 的 `app_tick` 讀**，每一下的時間 = 最後一個 block 的第一個 sample + 那之後過了幾 ms。
- 在 FM-1 上，喇叭聽到的聲音比 render 晚幾個 block，按鍵的 debounce 也讓按下晚約 1.6 ms 才看到（[Morse 的註解](morse.md)，`[上游]` fm1-base）。這些都是固定的偏移，由 `K4` 的 LAT 扣掉。**預設是 0，實機上要校正**：在 METRO 跟著 click tap 8 下以上，把 `K4` 轉到 `AVG` 接近 0。估計會在 +5 到 +15 ms 之間 `[調研]`，未量。
- 只評按下（onset），不評放開（時值）。

## 程式結構 { #code }

| 檔案 | 內容 |
|---|---|
| `rhythm.c` / `.h` | platform-free 的核心：cell 表、文字 ↔ 節奏、依難度產生、onset 的時間、DP 配對、評分、提示。C99，不依賴 fm1-base |
| `app.c` | 三個頁面、render（click 與音符的合成）、`app_tick` 的按鍵時間、譜面（`draw_staff`）、畫面、LED、設定 |
| `tests/rhythm_test.c` | `just rhythm-unit`：解析、產生器的規則（8 個難度 × 400 題）、時間換算、每一種提示都由它說的那種錯誤觸發 |
| `tests/basic.txt` | `just fw-test rhythm`：host simulator 的劇本 |

simulator 的劇本有幾個 app 自己的指令：`pattern q_ee_r_q` 指定 ECHO 的下一題，`autotap OFFSET [SKIP] [I MS]` 讓下一輪自動在每個音上 tap（可以整體偏移、漏掉第 SKIP 個、把第 I 個再移 MS），`metrotap N OFFSET` 在 METRO 的接下來 N 拍上 tap，`endtap OFFSET [SKIP_ROUND]` 把 ENDLESS 這一段的每個回合都 tap 出來。這些都走真的 `app_tick` 按鍵路徑；按鍵的時間記成劇本排定的那個 sample，不受 1 ms tick 的量化影響。

## v0.1 驗證 { #v01-verify }

2026-11-18，雲端環境，全部 `[模擬]`：

- `just rhythm-unit`：43,688 個檢查全部通過。
- `just fw-test rhythm`：70 個 expect 全部通過。內容：ECHO 準時 tap 得 100% 與 `GOOD`；整體早 45 ms 得 `RUSHING`、晚 45 ms 得 `DRAGGING`；只有休止符後晚 60 ms 得 `LATE AFTER RESTS`；漏一個音，其他三個仍然 GOOD；90 BPM 的切分音晚 150 ms 得 `SYNCOPATION SQUARED`；難度 4、兩小節的隨機題準時 tap 得 100%。METRO 有聲音、tap 偏 +20 ms 量到 +20、gap 小節裡沒有聲音（peak < -60 dB）而且靜音小節的 tap 照樣算、tap tempo 4 下得 120 BPM、設定重開後還在。
- 2026-11-18 在本人的 Mac 上，用獨立的 worktree 驗證第一版 `[模擬]`：
    - **`just fw-build rhythm` 在 link 時失敗**：device 上沒有 libgcc 的 64-bit 除法（`__divdi3`、`__udivdi3`）。host simulator 不會發現這種問題。現在 `app.c` 的 device 端改成只用 32-bit 整數與 float 運算（`q8_mul`、`smp_ms`）；build 也提醒，64-bit 乘法放在 load 旁邊可能弄壞暫存器。
    - 修正後的版本可以 build。套件 616,003 B，app 100,416 B，RAM 的 `.data`+`.bss` 是 39,600 / 98,304 B。
    - fm1-emu 的 `uc25hq80` 與 `p25q80`：開機進 ECHO；改 BPM 3 秒後存檔，重開後設定還在；METRO 與 ECHO 一輪都跑得起來。
    - 疊在 `2026-11-18-pre-theory-0.2` 備份上預演：開機 0 次寫入，存檔只寫設定所在的 sector，沒有碰 `0xD8000`、`0xDA000`、`0xDC000`、`0xFC000`。
    - **找到兩個衝突，已經改掉**：identity `FM-1_52` 已經有人用，改成 `FM-1_54`；設定原本放 `0xD6000`，和 THEORY 0.2 的閃卡進度撞到，改成 `0xD2000`。
- 2026-11-18 在 Mac 上驗證修正後的 `56d345e` `[模擬]`：
    - 不用任何 patch，`just fw-build rhythm` 就可以 link。套件 616,003 B，identity `FM-1_5400`，SHA-256 `33fdab70eb30656e3984d1d806b72ca10ab184a1ad06530e80c571b14495a4d9`。build 的檢查顯示「no 64-bit multiply inlined beside a load」。
    - fm1-emu 的兩種晶片：開機 0 次寫入；改 BPM 後存檔 3 次 erase/program；重開後設定還在。
    - `0xD2000`–`0xD5FFF` 在 `backups/` 的 27 份備份裡全是 `0xFF`。疊在 `2026-11-18-pre-theory-0.2` 上預演：存檔只寫 `0xD2000`，`0xD6000`、`0xD8000`、`0xDA000`、`0xDC000`、`0xFC000` 都沒有碰。
- **還沒做的**：還沒有 THEORY 0.2 之後的備份。刷機前先備份一次，確認 `0xD2000` 仍然是空的；刷機要本人逐次同意。`fm1.toml` 還沒有 `install` 與 `screenshot`，release build 會拒絕，上 Hub 前要補。

## v0.2 { #v02 }

2026-11-18 本人在 emulator 試用 v0.1 後的回饋：想要譜面（四分、八分這種），也想要像「節奏天國」那樣一句接一句的無限模式，用血條或錯幾次就輸，越來越快。0.2 照這兩點做：

- 譜面（[上面](#notation)），ECHO 的 READ 模式。
- ENDLESS（[上面](#endless)）。
- 設定的 record 沿用 v0.1 的大小與版本：v0.1 的 4 個保留 byte 變成 `read`、`best`。所以 v0.1 存的設定在 0.2 照樣讀得到，`read` 與 `best` 從 0 開始。

驗證（2026-11-18，雲端環境，全部 `[模擬]`）：

- `just rhythm-unit`：43,688 個檢查全部通過（核心沒變）。
- `just fw-test rhythm`：91 個 expect 全部通過。v0.1 的全部項目之外，還有：
    - READ 模式：count-in 後照譜準時 tap 得 100%。
    - 四種譜面的截圖（`notation_1.png` 到 `notation_4.png`）。
    - ENDLESS：第一段 4 回合都準時，血條維持 100，進到第二段而且 BPM 從 90 變 96；之後都不 tap，第 7 回合輸掉，`BEST` 是 6。
    - 重開機後 `BEST` 還在。
- 2026-11-18 在 Mac 上驗證 `7e373f7` `[模擬]`：
    - **`fw-build` 失敗**：譜面的 `st_head` 和 fm1-base `device/storage.c` 的 `st_head` 撞名（見[開發環境](index.md)的警告）。現在譜面的函式都改叫 `staff_*`，ENDLESS 的狀態改叫 `rh_en`。
    - 只改名字的版本：套件 616,003 B，app 104,296 B，`.data`+`.bss` 41,008 / 98,304 B。
    - fm1-emu 的 `uc25hq80` 與 `p25q80`：`HOME` 依序切換三頁；READ 模式畫得出譜；ENDLESS 的血條、回合數和上一回合的譜都有出現；設定重開後還在。
    - 疊在 `2026-11-18-pre-theory-0.2` 上預演：存檔只寫 `0xD2000`。
- 2026-11-18 在 Mac 上驗證改名後的 `df535d3` `[模擬]`：
    - 不用任何 patch 就可以 build。套件 616,003 B，SHA-256 `7efc1107e6b2cc1f9414d37b45ff9bf18bd4931f4bb53a32ee2abe171a9fae82`。
    - fm1-emu 的兩種晶片：ENDLESS 都能正常跑，沒有 fault。
- **還沒做的**：刷進實機。刷之前要先備份，刷機要本人逐次同意。

## v0.3 { #v03 }

2026-11-19 本人想要的：

- 一種「指揮練習器」：給一個速度之後盲打，看準不準。簡單版先聽預備拍，難的版本連預備拍都沒有，只知道 BPM。
- 一種像電影《進擊的鼓手》（Whiplash）那樣聽一段，回答快了還是慢了。
- 速度術語的樂理考題（義大利文速度用詞對應幾 BPM）。
- 聽拍號。

這些都放進 RHYTHM，變成三頁（本人 2026-11-19 決定，不另做韌體）。

### 操作 { #controls-v03 }

| 控制 | PULSE | JUDGE | QUIZ |
|---|---|---|---|
| `PLAY` | 開始 / 停止（盲打中按停止，會用已經打的來算） | 開始一輪 / 停止 | 開始 / 停止 |
| `REC` | | 再聽一次這一題 | 再聽一次（HEAR、METER） |
| `SEL` | 預備拍 1、2、4 小節（GUIDED） | 每題的速度固定在 BPM，或在 BPM 的 ±15 % 內變動 | |
| `SELECT`、`OCT±` | BPM | BPM | |
| `K1` | 拍號 | 拍號 | |
| `K2` | GUIDED / COLD | TEMPO / FEEL / DRIFT | 題型：TERM>BPM、BPM>TERM、HEAR、CHANGE、METER |
| `K3` | 盲打 2、4、8、16 小節 | | 難度 1–3 |
| `K4` | LAT（GUIDED 才扣；COLD 只看相對時間，不扣） | | |
| 琴鍵 | 任一鍵 tap | 低的半邊 = SLOWER / DRAGGING，高的半邊 = FASTER / RUSHING（正中間那一鍵不算） | 鍵盤分成四段，由低到高是 A B C D |

琴鍵的燈會標出答案的位置：JUDGE 的低半邊微亮、高半邊亮一點，QUIZ 的四段亮度交替。

### PULSE：盲打 { #pulse }

- **GUIDED**：先放 1、2 或 4 小節的預備拍（可以跟著打，不計分），接著 click 停掉，你在任一鍵上繼續打每一拍。拍點照預備拍的格子延伸下去，所以和 METRO 的 gap 一樣，比的是絕對位置。
- **COLD**：什麼都不放，畫面上只有 BPM。你按下的第一下就是第一拍，之後照你心裡的速度打完設定的小節數。比的是速度，不是位置。
- 盲打的時候畫面只顯示已經打了幾下，不顯示拍子在哪裡，燈也不亮。
- 打完最後一拍就結算。如果拖拍，最後一拍會晚到，所以結算會等你，最多等到原本長度的 1/4。
- 結算後 click 會回來一小節，在「如果節拍器一直在響」的位置（COLD 是從你的第一下算起），讓你聽到自己差多少。

結果畫面：

- **你的速度**：所有 tap 對拍號（第幾拍）做最小平方直線，斜率就是你的 BPM。旁邊是和目標差幾 %（+ 是快）。
- **GOOD 幾拍**、**JITTER**（tap 離那條直線的標準差，ms：你自己穩不穩，和速度對不對無關）、**TREND**（後半段的速度比前半段快或慢幾 %：漸快 / 漸慢）。
- GUIDED 再加 **MEAN**（平均早晚）和 **END**（最後一拍的位置，等於整段漂了多少）。
- **逐拍的圖**：每一拍的誤差隨時間畫成點（±150 ms），可以看到是一路漂走，還是忽快忽慢。
- **分布圖**：誤差的直方圖，每格一個 GOOD 寬度（120 BPM 時是 35 ms），中間那格綠色。

每一下 tap 算在第幾拍，是用「你自己的拍子」（tap 之間間隔的中位數）去數的，不是用目標的格子。所以速度差了 5 %、打了 64 拍，即使最後整整差了 3 拍，還是一拍一拍對得上。一拍按兩下會算成多按，漏一拍會算成漏掉，後面的不會錯位。

提示（依序檢查，門檻都是先訂的，要等實機調）：

| 提示 | 條件 |
|---|---|
| `MISSED BEATS` | 漏掉的拍數 ≥ max(2, 拍數 / 8) |
| `EXTRA TAPS` | 多按 2 下以上 |
| `SPEEDING UP` / `SLOWING DOWN` | TREND 超過 ±3 % |
| `RUSHING` / `DRAGGING` | 整體速度差超過 ±2 % |
| `UNSTEADY` | JITTER 超過 GOOD 範圍的 3/4 |
| `RUSHING` / `DRAGGING` | 速度對，但平均提早或延遲超過 GOOD 的範圍（GUIDED） |
| `STEADY` / `CLOSE` | 全部 GOOD / 其他 |

最佳紀錄是 |速度差 %| 最小的一次，GUIDED、COLD 分開存，至少要有九成的拍子有打到才算。

### JUDGE：快了還是慢了 { #judge }

三種題型：

| 題型 | 放什麼 | 你回答 | 變化量 d 的起點 |
|---|---|---|---|
| **TEMPO** | 兩小節 click，空一拍，再兩小節 click，速度變成 BPM × (1 ± d) | 後半段 FASTER 或 SLOWER | 8 % |
| **FEEL** | 一小節 click 當預備，之後兩小節 click 繼續，上面有一個「鼓手」打八分音符，每一下都提早或延遲 d ms | 鼓手 RUSHING 或 DRAGGING | 60 ms |
| **DRIFT** | 一小節照 BPM，接著三小節每拍間隔逐漸變，到最後一拍是 BPM × (1 ± d) | 漸快（FASTER）或漸慢（SLOWER） | 12 % |

難度用 **2-down 1-up staircase** 自動調整：連對兩題 d 乘 0.7，錯一題 d 除以 0.7；轉向（reversal）兩次之後改成 0.84，步子變細。轉向 10 次，或滿 40 題就結束。**閾值**是最後 6 次轉向的幾何平均，大約是你答對 71 % 的那個 d。結果畫面上畫出每一題的 d（對數刻度，綠對紅錯）和閾值線。三種題型的最佳（最小）閾值分開存。

模擬的聽者（d 小於某個值時正確率從 50 % 線性升到 98 %）跑 200 輪，量出來的閾值平均落在聽者 71 % 點的 1.1 倍以內 `[模擬]`（`just rhythm-unit` 會印出來）。

`SEL` 讓每一題的速度在 BPM 的 ±15 % 內隨機，這樣就不能靠記住 BPM 來回答。

### QUIZ：速度術語與拍號 { #quiz }

五種題型，每題四選一，在鍵盤的四段（A 最低到 D 最高）上回答。答錯的那一項，這一輪後面會比較常出（權重最多 9 倍）。答完會顯示正確答案；BPM>TERM 與 HEAR 也會列出每個選項的 BPM 範圍。最長連對（`BEST STREAK`）會存起來。

| 題型 | 題目 | 選項 |
|---|---|---|
| `TERM>BPM` | 一個速度術語 | 四個 BPM，只有一個落在它的範圍內 |
| `BPM>TERM` | 一個 BPM | 四個術語，只有一個的範圍包含它 |
| `HEAR` | 放兩小節 click（BPM 不顯示） | 四個術語 |
| `CHANGE` | 速度變化的術語，或它的意思（隨機一個方向） | 四個意思或四個術語 |
| `METER` | 放三小節的八分音符：小節第一拍有重音和低音、每組的第一個是 click、其他是輕的 sub | 四個拍號 |

**BPM 範圍**依[英文維基百科 Tempo 的 Basic tempo markings](https://en.wikipedia.org/wiki/Tempo#Basic_tempo_markings)（2026-11-19 讀取）`[上游]`。那一頁自己也說這些數字是 4/4 的粗略值，因作曲家與作品而異。[中文維基百科「速度 (音樂)」](https://zh.wikipedia.org/zh-tw/%E9%80%9F%E5%BA%A6_(%E9%9F%B3%E6%A8%82))的數字差很多，而且沒有列來源，所以沒有用：

| 術語 | 中文（中文維基的用詞） | 英文維基（本韌體用的） | 中文維基 | 難度 1 |
|---|---|---|---|---|
| Larghissimo | 極端地緩慢 | 24 以下 | 40 以下 | |
| Grave | 莊嚴的慢 | 24–40 | — | |
| Largo | 廣板 | 40–66 | 40–60 | ✓ |
| Lento | 緩板 | 40–60 | 40–45 | |
| Larghetto | 小廣板 | 44–66 | 60–66 | |
| Adagio | 慢板 | 44–66 | 66–76 | ✓ |
| Adagietto | 小慢板 | 46–80 | — | |
| Andante | 行板 | 56–108 | 76–108 | ✓ |
| Andantino | 小行板 | 80–108 | — | |
| Moderato | 中板 | 108–120 | 90–115 | ✓ |
| Allegretto | 小快板 | 112–120 | 104–112 | |
| Allegro moderato | 適度、中速的快板 | 116–120 | — | |
| Allegro | 快板 | 120–156 | 120–168 | ✓ |
| Vivace | 活潑的 | 156–176 | 約 140 | ✓ |
| Presto | 急板 | 168–200 | 168–200 | ✓ |
| Prestissimo | 最急板 | 200 以上 | 約 200–208 | |

範圍彼此重疊（例如 50 BPM 同時在 Largo、Lento、Larghetto、Adagio、Adagietto 裡），所以錯的選項一定排除「其實也對」的：TERM>BPM 的錯誤選項不在題目術語的範圍裡；BPM>TERM 與 HEAR 的錯誤選項，範圍都不包含那個 BPM。`just rhythm-unit` 對每一種題型、每個難度各出 3,000 題，確認恰好一個選項是對的。難度 1 只考打勾的 7 個，難度 2、3 考全部 16 個。HEAR 的開放端點分別取 18–24 與 200–230 BPM。

**速度變化**（CHANGE）分成六類，同一類的不會同時出現在選項裡（例如 ritardando 和 rallentando）：

| 類 | 術語（畫面上的意思） |
|---|---|
| 逐漸變快 | Accelerando（GRADUALLY FASTER） |
| 立刻變快 | Più mosso（MORE MOTION: FASTER）、Stretto（PRESSING FASTER） |
| 逐漸變慢 | Ritardando（GRADUALLY SLOWER）、Rallentando（SLOWING DOWN）、Allargando（BROADER, SLOWER） |
| 立刻變慢 | Meno mosso（LESS MOTION: SLOWER）、Ritenuto（HELD BACK AT ONCE） |
| 回原速 | A tempo（BACK TO THE TEMPO）、Tempo primo（THE FIRST TEMPO） |
| 自由 | Rubato（FREE, STOLEN TIME） |

難度 1 是前 6 個（accelerando、ritardando、a tempo、più mosso、meno mosso、rubato），難度 2、3 是全部 11 個。

**拍號**（METER）：

| 拍號 | 一小節的八分音符 | 分組 | 難度 |
|---|---|---|---|
| 3/4 | 6 | 2+2+2 | 1 |
| 4/4 | 8 | 2+2+2+2 | 1 |
| 6/8 | 6 | 3+3 | 1 |
| 5/4 | 10 | 2+2+2+2+2 | 1 |
| 7/8 | 7 | 2+2+3 | 2 |
| 9/8 | 9 | 3+3+3 | 2 |

2/4 對 4/4、6/8 對 12/8 只差在怎麼寫，用聽的分不出來，所以不考。3/4 和 6/8 都是六個八分音符，差在分組（三組兩個或兩組三個），這是最經典的一題。難度 3 拿掉小節第一拍的高音重音，只留低音，要更仔細聽。速度是四分音符 80–120 BPM 隨機。

### 設定的 record { #record-v2 }

0.3 的 record 是第 2 版（40 bytes，原本 20 bytes）。舊的欄位位置不變，新的加在後面：`pmode`、`pbars`、`pcount`、`jtype`、`jrand`、`qtype`、`qlevel`、各頁的最佳紀錄。讀到 v1（0.1、0.2 存的）時，舊設定照用，新欄位用預設值；下次存檔就寫成 v2。仍然只用 `0xD2000` 這一個物件，沒有新的 flash 區域。

### 程式結構 { #code-v03 }

- `rhythm.c`：`rh_pulse`（拍號推算、直線、TREND、JITTER、直方圖、提示），`rh_stair_*`（staircase），`rh_judge_seq`、`rh_clicks_seq`、`rh_meter_seq`（每一個聲音的 µs），`RH_TERMS`、`RH_CHANGES`、`RH_METERS` 與 `rh_quiz_make`（排除也對的選項）。仍然是 platform-free 的 C99，沒有 libm（平方根、幾何平均自己算）。
- `app.c`：transport 多了 `TR_SEQ`，一串聲音各自排在自己的 sample 上（JUDGE 的 FEEL、DRIFT 不在 tick 的格子上）。tap 的佇列記錄是哪一個鍵，JUDGE 和 QUIZ 用它來分辨答案。
- simulator 劇本的新指令：`pulsetap OFFSET [DRIFT_X10 [SKIP]]`（PULSE 的每一拍，每拍多偏 DRIFT/10 ms）、`judgeans right|wrong`、`judgeseq RRW...`（整輪一次答完）、`quizans right|wrong`、`v1record BPM`（寫一個 0.2 的 record，測升級）。

### 驗證 { #v03-verify }

2026-11-19，雲端環境，全部 `[模擬]`：

- `just rhythm-unit`：474,693 個檢查全部通過。新的部分包括：
    - PULSE：準時、快 3 %、慢 4 %、前半準後半快 8 %、整體提早 60 ms、±30 ms 抖動、漏一拍、一拍按兩下、拍子之後多按，以及 3,000 組隨機 tap 不越界、直方圖加總等於有打的拍數。
    - staircase：模擬聽者收斂（每種題型 200 輪）；一直答對會降到下限。
    - 三種 JUDGE 的聲音時間；METER 的重音與分組數。
    - QUIZ：每個 BPM 在每個難度都至少有三個術語不包含它；每題恰好一個對的選項；答錯的項目變常出。
- `just fw-test rhythm`：189 個 expect 全部通過（v0.2 的全部保留）。新的部分：
    - PULSE GUIDED：預備拍有聲音；準時打得 100.0 BPM、`STEADY`；快 3 % 得 `+3.0%`、`RUSHING`；慢 4 % 又漏一拍，得 `-4.0%`、漏 1 拍；結算後 click 回來一小節。
    - PULSE COLD：沒有預備拍，第一下開始，得 `STEADY`。
    - JUDGE TEMPO 的一整輪（轉向 10 次，閾值 6.1 %，存成最佳）；FEEL 用真的琴鍵回答；DRIFT 與 SEL 的隨機速度。
    - QUIZ 五種題型都出得了題，HEAR 和 METER 有聲音，答對、答錯、鍵盤分區回答都正確。
    - 重開機後 0.3 的設定與最佳紀錄還在；寫一個 v1 的 record 再開機，BPM、ENDLESS 的 BEST 照舊，新欄位是預設值。
- 截圖在 `firmware/rhythm/build/host/`：`pulse_*.png`、`judge_*.png`、`quiz_*.png`。

**還沒做的（要在本人的 Mac 上）**：

1. `just fw-build rhythm`：0.3 只用 32-bit 整數與 float，`rhythm.c` 沒有 64-bit 運算，但要 link 過才算數。
2. `just emu-run` 在 `uc25hq80` 與 `p25q80`：每一頁跑一輪，改設定、等 3 秒存檔、重開、設定還在。
3. `just emu-rehearse` 疊在最新的備份上：存檔只寫 `0xD2000`。
4. 實機（要本人逐次同意）：

| 步驟 | 應該看到 |
|---|---|
| PULSE GUIDED，100 BPM，跟著預備拍之後盲打 4 小節 | 預備拍之後 click 停掉；打完最後一拍就出結果；一小節之後 click 回來 |
| PULSE COLD | 按 PLAY 沒有聲音，第一下開始；結果的 BPM 和你心裡的速度相近 |
| JUDGE TEMPO 一整輪 | 大約 25–40 題後出現閾值，記進 device-log，之後拿來調起點與下限 |
| JUDGE FEEL | 低半邊鍵答 DRAGGING、高半邊答 RUSHING；起點 60 ms 應該一聽就知道 |
| QUIZ METER，難度 1 | 3/4 和 6/8 聽得出分組不同 |
| 換頁再回來、重開機 | 各頁的設定和 BEST 還在 |

## 之後 { #next }

照 2026-11-18 的 RHYTHM / GUITAR 設計提案：

- 實機校正延遲，調整 GOOD / OK 的範圍與提示的門檻，以及 ENDLESS 的扣血數字。
- PICK：聽了之後，從四個小譜面選一個（E4–A4 作答）。
- Leitner 回合：項目 = (cell, 速度階)，速度 60 → 80 → 100 → 120，box 到 2 才開新的 cell（比照 Morse 的 Koch 方式）。
- 6/8、兩手節奏（左半鍵與右半鍵各一個聲部）、要不要評時值。
- 0.3 之後：實機調 PULSE 的提示門檻與 JUDGE 的起點、下限；JUDGE 用真的節奏型（不只是 click 和八分音符）；METER 加入沒有重音、只靠節奏型判斷的難度；QUIZ 加 Leitner。
- 穩定後比照 fm1-morse 拆成獨立 repo 上 Hub。
