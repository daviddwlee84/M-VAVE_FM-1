# 節奏練習機 RHYTHM

把 FM-1 變成隨身的節奏練習機：節拍器加上「聽了之後 tap 回來」的節奏盲聽，逐音告訴你早了或晚了幾 ms，並指出最該改的一件事。玩法比照[摩斯電碼機](morse.md)的 RHYTHM drill；樂理的部分在[樂理練習機](theory.md)。

**狀態（2026-11-18）：v0.1（METRO + ECHO）只在 host simulator 驗證過 `[模擬]`。還沒有 device build，沒有在 fm1-emu 跑過，也沒有刷進實機。**

- 韌體在 `firmware/rhythm/`，GPL-3.0-or-later，名稱 RHYTHM。
- 識別碼 `FM-1_54`。2026-11-18 對照 Hub 的 catalog（12:01 UTC 版）沒人用。原本暫定的 `FM-1_52` 已經被 RemoteFM1、JAMTRACK、QUADRO 用掉（RemoteFM1 的 pattern 正好吃到 `FM-1_5200`）。
- 設定存在 `0xD2000`（`APP_STORE_MAP {0xDC000, 0xD2000}`）。附近已經有人用：THEORY 0.2 的 `0xD8000` 與 `0xD6000`（閃卡進度）、Morse 的 `0xDA000`、面板校正的 `0xDC000`；GUITAR 預定用 `0xD4000`。原本選的 `0xD6000` 和 THEORY 0.2 撞到，第一次存檔就會蓋掉 THEORY 的進度，所以改了。`0xD2000` 在 THEORY 0.2 之後的備份裡是不是空的，**刷機前要用 `just emu-rehearse` 確認**。
- 為什麼另做一個韌體而不是放進 THEORY：本人 2026-11-18 決定 RHYTHM 與 GUITAR 都各自是獨立韌體。

## 操作 { #controls }

兩頁，`HOME` 切換。設定停手 3 秒後自動存。

| 控制 | METRO | ECHO |
|---|---|---|
| `PLAY` | 開始 / 停止 | 新的一題 / 停止 |
| `REC` | tap tempo（照拍子按，取最後 4 下） | 同一題再來一次 |
| `SELECT` | BPM ±1（30–240） | 同左 |
| `OCT-` / `OCT+` | BPM ±5 | 同左 |
| `K1` | 拍號 2/4、3/4、4/4 | 同左（停止時） |
| `K2` | 細分：OFF、8 分、3 連音、16 分 | 難度 1–8（見下表） |
| `K3` | Gap click：OFF、1+1、2+1、2+2、4+2、4+4（響幾小節 + 靜音幾小節） | 1 或 2 小節 |
| `K4` | LAT：每一下 tap 要扣掉的延遲（ms，-30 到 +90） | 同左 |
| 任一琴鍵 | tap：畫面顯示這一下離最近的拍子幾 ms | tap 回答 |

### METRO

一般的節拍器，第一拍是高音的重音。開了 gap click 之後，靜音的小節裡畫面只寫 `GAP: KEEP TAPPING`，拍子格也不亮，琴鍵燈也不閃，你繼續在任一鍵上 tap。每一下都和「如果節拍器還在響」的拍點比，最近 8 下畫在 EARLY / LATE 的刻度上（中間灰色那一格是 GOOD 的範圍），下面寫最後一下、平均、以及只算靜音小節的平均（`GAP`）。回到有聲的小節時，看平均有沒有跑掉，就知道內在的時鐘準不準。

### ECHO

1. `PLAY`：count-in 一小節（數字 1 2 3 4）。
2. **LISTEN**：放一到兩小節的節奏（低一點的木頭聲），節拍器以小聲的 click 繼續。畫面只有拍子格與 playhead，**不顯示音符**，是盲聽。
3. 再 count-in 一小節。
4. **YOUR TURN**：在任一鍵上把剛剛的節奏 tap 回來，節拍器小聲地繼續。
5. **結果**：分數（%），每個音一格，綠色 GOOD、黃色 OK、紅色差太多、紅框是漏掉。下面的白線是你實際 tap 的位置，灰線連到它該在的地方。再下面是平均誤差、GOOD 的數量、漏掉與多按的次數，以及這題的 Kodály 唸法（例如 `TA TI-TI REST TA`）。footer 是提示。

`REC` 重來同一題，`PLAY` 換一題。

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
| `app.c` | 兩個頁面、render（click 與音符的合成）、`app_tick` 的按鍵時間、畫面、LED、設定 |
| `tests/rhythm_test.c` | `just rhythm-unit`：解析、產生器的規則（8 個難度 × 400 題）、時間換算、每一種提示都由它說的那種錯誤觸發 |
| `tests/basic.txt` | `just fw-test rhythm`：host simulator 的劇本 |

simulator 的劇本有幾個 app 自己的指令：`pattern q_ee_r_q` 指定 ECHO 的下一題，`autotap OFFSET [SKIP] [I MS]` 讓下一輪自動在每個音上 tap（可以整體偏移、漏掉第 SKIP 個、把第 I 個再移 MS），`metrotap N OFFSET` 在 METRO 的接下來 N 拍上 tap。這些都走真的 `app_tick` 按鍵路徑。

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

## 之後 { #next }

照 2026-11-18 的 RHYTHM / GUITAR 設計提案（v0.2 起）：

- 實機校正延遲，調整 GOOD / OK 的範圍與提示的門檻。
- 小譜面：READ（看譜 tap）、PICK（聽了從四個小譜面選一個）。
- Leitner 回合：項目 = (cell, 速度階)，速度 60 → 80 → 100 → 120，box 到 2 才開新的 cell（比照 Morse 的 Koch 方式）。
- 6/8、兩手節奏（左半鍵與右半鍵各一個聲部）、要不要評時值。
- 穩定後比照 fm1-morse 拆成獨立 repo 上 Hub。
