# ChoralRoot

[ChoralRoot FM-1](https://github.com/Quixotic7/ChoralRootFM1)（Quixotic7，GPL-3.0）是仿 Telepathic Instruments Orchid 的和弦樂器：鍵盤左端決定「要哪種和弦」，其餘的鍵彈根音，機器幫你組和弦、配置聲部，並把和弦名稱顯示在螢幕上。另有跟著和弦走的 bass、演奏模式（strum、arpeggio、pattern）、效果、錄和弦的 looper、七個合成引擎。建立在 Felucca 之上，與 Telepathic Instruments 及 M-VAVE 無關。

**狀態（2026-11-17）：已刷進實機（新晶片 `0xB36014`），開機正常。** 代價如預期：SLOOP 的曲子已不在機器上，要回 SLOOP 得還原備份，見「刷之前要知道的」。 手冊：[docs/MANUAL.md](https://github.com/Quixotic7/ChoralRootFM1/blob/main/docs/MANUAL.md)

完整的上手教學見 [ChoralRoot 教學](tutorial.md)。

## 不刷機怎麼玩 { #emulate }

由簡單到接近實機：

| 方式 | 怎麼做 | 手感 |
|---|---|---|
| **1. 上游的瀏覽器模擬器** | <https://quixotic7.github.io/ChoralRootFM1/emu/> | 滑鼠與電腦鍵盤。零設定，有聲音 |
| **2. 本機 fm1-emu** | `just emu-run downloads/choralroot-0.41.fwsc`。視窗標題要是 `FM-1 · choralroot-0.41` | 同上，但用真實的 240 MHz 時脈、延遲較低，存檔留在本機 |
| **3. 本機 fm1-emu + 實機當鍵盤** | 實機裝著 [CTL-1](../ctl-1/index.md) 接 USB，再開方式 2 | **實機的 27 個鍵一對一對應 ChoralRoot 的鍵。** 最接近實際演奏，不用刷 ChoralRoot |
| **4. RemoteFM1 的播放器** | [remotefm1.com](https://remotefm1.com/)，它的韌體庫裡有 `choralroot-0.41.rfm1` | 瀏覽器裡用原生速度跑（不是模擬晶片） |
| **5. RemoteFM1 + 實機當面板** | 實機刷 [RemoteFM1](../remotefm1/index.md)，在播放器選 Play on my FM-1 | 鍵、旋鈕、LED、螢幕全部是實機的。要刷 RemoteFM1 |

### 方式 3 的設定

原理 `[上游]`：`fm1-live` 會把電腦上所有 MIDI 輸入送進模擬機器的 MIDI IN；ChoralRoot 的 **Options → MIDI Keys** 可以指定一個 channel，該 channel 上的音符 53–79 會「按下」機器的 F3–G5 這 27 個鍵。CTL-1 八度為 0 時最低鍵正好是 F3，channel 預設是 1。

1. 實機裝 CTL-1，USB 接 Mac。在 CTL-1 上按 `EDIT`，把 KNOB 1（Sound）關掉，避免它自己的音色疊上去。
2. **先確認實機已接上這台 Mac 的 USB**，再執行 `just emu-run downloads/choralroot-0.41.fwsc`。啟動訊息的 `MIDI in:` 那一行要列出實機（macOS 可能仍顯示舊名稱 `Felucca`）。如果是 `MIDI in: no inputs`，表示啟動當下電腦上沒有 MIDI 裝置（例如機器接在 iPad 上、或走藍牙），按 Esc 關掉、接好再開。`[實機]`
3. 在模擬器視窗裡進 ChoralRoot 的 Options（`GLO`，電腦鍵盤的數字鍵之一），把 **MIDI Keys** 設成 **ch 1**。
4. 在實機上按住 A#3（MAJ）再按 D4，模擬器應該彈出 D 大三和弦。

限制：只有琴鍵會對應過去。旋鈕與功能鍵（LOCK、BASS、REC…）仍要在模擬器視窗裡用滑鼠或電腦鍵盤操作。CTL-1 的八度要維持在 0，否則音符範圍對不上。

這個組合我還沒實際接起來試，以上是依兩邊文件推出來的設定。`[調研]`

## 怎麼彈 `[上游]`

27 個鍵，F3 到 G5。左邊七個是 **chord block**，其餘 20 個（C4 到 G5）是根音。

| 鍵 | ChoralRoot 的功能 | 用法 |
|---|---|---|
| 黑鍵 F#3、G#3、A#3 | **DIM、MIN、MAJ** | 按住時決定和弦種類 |
| G#3 + A#3 一起按 | **SUS** | 一根手指按兩個黑鍵 |
| 白鍵 F3、G3、A3、B3 | **6、m7、M7、9** | 延伸音，按住時加上，可以疊 |
| C4 到 G5 | 根音 | 單按是單音；chord block 有鍵按著時是和弦 |

所以就是你說的：左手按和弦性質，右手按根音。chord 鍵要在根音按下去**之前**就按著。

五分鐘上手：

1. 按住 MAJ（A#3）再按 D4：D 大三和弦。螢幕顯示 `DM` 與組成音。
2. 按住 MIN（G#3）加 m7（G3）再按 D4：Dm7。
3. **LOCK**（面板印的是 `LFO`）：把 chord block 鎖住，空出左手。鎖住後點 MIN 就一直是小和弦，點 9 加九音。
4. 按住和弦轉 **KNOB 1**：改 voicing（最低音上移八度，或最高音下移）。
5. 轉 **PRESETS** 換音色；按住 `GLO`（OPT）轉 PRESETS 換引擎（FM6、FM TONE、VA、CZ-1、P5、WHEEL、GRAIN）。
6. 點 `ENV`（BASS）：每個和弦下面加 bass。
7. 點 `REC` 錄 loop，彈幾個和弦，再點 `REC` 結束。loop 記的是和弦不是聲音，所以換音色會跟著變。
8. 卡住時同時按 `OCT-` + `OCT+`：PANIC，所有音關掉。

面板上印的標示多半換了工作：`SEL` 是 KEY、`ARP` 是 PERF、`ENV` 是 BASS、`LFO` 是 LOCK、`GLO` 是 OPT、`PLAY` 是 LOOP。琴鍵沒有 velocity（FM-1 的鍵感測不到），用固定值。

## 音色：七個引擎，不只一種 { #sounds }

剛裝好時會覺得音色單一，是因為 **PRESETS 只在目前這個引擎裡轉**。換引擎要**按住 `GLO`（OPT）再轉 PRESETS**，轉到想要的引擎後放開。`[上游]`

| 引擎 | 是什麼 | 內建音色數 |
|---|---|---|
| **FM6** | 六運算子 FM，能載入 DX7 音色。開機預設是這個（`TINE EP`） | 25 |
| **FM TONE** | 四運算子 FM，Digitone 風格 | 16 |
| **VA** | virtual analog：四個振盪器、可變形濾波器、四組 envelope 與 LFO | 25 |
| **CZ-1** | Casio CZ-1 的 phase distortion，含原廠 64 個音色 | 64 |
| **P5** | Prophet-5 模型 | 115 |
| **WHEEL** | 九支 drawbar 的 tonewheel 風琴 | 5 |
| **GRAIN** | granular 合成 | 8 |

合計 258 個內建音色。bass 是另一個獨立聲部：**ALGORITHM** 換 bass 音色，**按住 `GLO` 轉 ALGORITHM** 換 bass 的引擎。每個引擎還能存自己做的音色，`EDIT` 進編輯器。

## 當成 MIDI 和弦控制器：送 MIDI 出去、自己不發聲 { #midi-out }

可以。ChoralRoot 彈出的和弦會從 USB MIDI（開了的話還有 Bluetooth）送出去，所以能拿它的和弦邏輯去驅動 iPad 或電腦上的任何音源。`[上游]`

| 送出的東西 | Options 裡的設定 | 預設 channel | 內容 |
|---|---|---|---|
| Perform | MIDI Perform | ch 1 | 和弦聲部實際彈的音：整個和弦，或 performance mode 拆出來的琶音 |
| Bass | MIDI Bass | ch 2 | bass 線 |
| Raw Chord | MIDI Raw Chord | Off | 不經 performance mode 的原始和弦塊。Orchid 用 ch 3 |

另外預設會送 MIDI clock，loop 開始與停止時送 start / stop。

**讓它自己不發聲**：手冊裡沒有像 CTL-1 那樣的 Sound On/Off 或 local off 選項。最直接的做法是把 **MASTER 旋鈕轉到底**，MIDI 照常送出。我還沒在機器上確認轉到底是否完全無聲。`[調研]`

接 iPad 時和 CTL-1 的差別：

| | CTL-1 | ChoralRoot |
|---|---|---|
| 送的 MIDI | 你按的音、旋鈕的 CC | **組好的和弦**、bass、clock |
| 關掉自己的聲音 | Sound 頁有開關 | 沒有開關，用 MASTER |
| iPad 的聲音會不會被導到 FM-1 | 會（它是 USB audio 輸出） | 手冊說電腦**不能**透過 FM-1 播放聲音，所以應該不會。未實測 |
| Bluetooth MIDI | 開機就開 | 預設關，Options → Bluetooth 設 On，下次開機生效 |
| 旋鈕送 CC | 四個 bank | Options → MIDI CC 設成 Out 或 In + Out 後，轉音色參數時會送 |

在接收端：把一個音源放在 ch 1 收和弦、另一個放 ch 2 收 bass（AUM 可以分別路由）。很多 DAW 把 MIDI note 60 叫 C3，ChoralRoot 叫 C4，所以看起來會差一個八度，音其實一樣。

也可以反過來：Options → **MIDI Roots** 指定一個 channel，外接鍵盤上的每個音都當成根音，chord block 仍在 FM-1 上按。這樣就有一整排鍵盤可以彈和弦。

## 刷之前要知道的

ChoralRoot 本身支援新晶片（上游 0.29.1 起接受 `0xB36014`，Hub 也這樣標）。問題在**從 SLOOP 切過去**：

- **它會蓋掉 SLOOP 的曲子。** ChoralRoot 的韌體映像比其他韌體大：佔 `0x04000`–`0x9EFFF`，其他韌體只到 `0x92FFF`。模擬器先預測到這點，2026-11-17 M-Flasher 在實機上列出的寫入計畫證實了：`Writes 152 sectors (608 KiB): 0x04000-0x8EFFF, 0x91000-0x91FFF, 0x93000-0x9EFFF` `[實機]`。多出來的部分蓋到 `0x97000`，那正是我這台 SLOOP 存 project 的 sector（見[多韌體切換](../switching.md)）。裝了 ChoralRoot 再切回 SLOOP，曲子要從備份還原。`[模擬]`
- 上游文件記載過一台跑過 Sloop 的 FM-1 裝 ChoralRoot 後掉進晶片的 ROM boot mode（黑畫面），把舊資料（`0x97000…`、`0xFC000…`）清掉才回來。它的 installer 因此**拒絕**在 Sloop 上安裝。兩者都源自 Felucca、共用儲存位址，ChoralRoot 開機時會讀到 SLOOP 留下的資料。`[上游]`
- 上游建議的切換方式是 M-Flasher **加 m-bootloader**。我沒有 m-bootloader。

我的預演結果 `[模擬]`：用 `just emu-rehearse` 把 ChoralRoot 0.41 蓋在我真實的 SLOOP 備份上，在新晶片模式下**正常開到演奏畫面**，15 秒內沒有任何 flash 寫入，也沒有進 safe mode。這和上游的事故不矛盾（那是較舊的版本），但它只是模擬，而且只測了開機。

掉進 ROM boot mode 本身救得回來（M-Flasher 還看得到機器，可以還原備份），所以不是變磚的風險，但需要我在 GUI 手動還原。

**實機結果（2026-11-17）** `[實機]`：我照常 `just fm1-switch` 刷入，機器正常開機，USB 產品名變成 `ChoralRoot FM-1`。當時 flash 裡同時有 SLOOP 的 autosave、CTL-1 的設定等殘留，上游記載的事故在這台、這個版本沒有發生。

**回 SLOOP 的方式**：不能用 `just fm1-switch sloop`，那只會得到沒有曲子的 SLOOP。要在 M-Flasher 的 GUI 用 **RESTORE FULL FLASH** 還原 `backups/2026-11-17-0341-pre-ctl-1.0.3/`（SLOOP 加上最新的曲子）。

## 已準備好的

`downloads/choralroot-0.41.fwsc`（release `v0.41`）已下載並驗證。在 fm1-emu 的空白 flash 上兩種晶片都能開機。`[模擬]`

!!! warning "更正（2026-11-17）：我一開始下載錯了套件"
    ChoralRoot 的最新 release 是 `v0.41-smk37.1`，那是 **SMK-37** 版；FM-1 版在 `v0.41`。我先前下載了 SMK-37 的檔案，所以 `just emu-run` 開出來的是 SMK-37 的面板（標題列寫 `SMK-37 · …`），先前記錄的「66 次寫入」與 `FONTS MISSING` 也都是那個錯的套件造成的。已換成正確的檔案並重做所有測試。

    教訓：模擬器視窗的標題列會顯示它認出的機型，是 `FM-1` 才對。M-Flasher 也會拒絕型號不符的套件，所以即使沒發現也不會刷進去，但不該靠最後一道防線。
