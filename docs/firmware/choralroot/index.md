# ChoralRoot

[ChoralRoot FM-1](https://github.com/Quixotic7/ChoralRootFM1)（Quixotic7，GPL-3.0）是仿 Telepathic Instruments Orchid 的和弦樂器：鍵盤左端決定「要哪種和弦」，其餘的鍵彈根音，機器幫你組和弦、配置聲部，並把和弦名稱顯示在螢幕上。另有跟著和弦走的 bass、演奏模式（strum、arpeggio、pattern）、效果、錄和弦的 looper、七個合成引擎。建立在 Felucca 之上，與 Telepathic Instruments 及 M-VAVE 無關。

**狀態（2026-11-17）：只在瀏覽器試玩。暫時不刷進實機，原因見下。**

- **先在瀏覽器玩**：<https://quixotic7.github.io/ChoralRootFM1/emu/>（同一份韌體，有聲音，不需要 FM-1）
- 手冊：[docs/MANUAL.md](https://github.com/Quixotic7/ChoralRootFM1/blob/main/docs/MANUAL.md)

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

## 為什麼暫時不刷

ChoralRoot 本身支援新晶片（上游 0.29.1 起接受 `0xB36014`，Hub 也這樣標）。問題在**從 SLOOP 切過去**：

- 上游文件記載過一台跑過 Sloop 的 FM-1 裝 ChoralRoot 後掉進晶片的 ROM boot mode（黑畫面），把舊資料（`0x97000…`、`0xFC000…`）清掉才回來。它的 installer 因此**拒絕**在 Sloop 上安裝。`[上游]`
- 那兩個位址正是我這台 SLOOP 存曲子與設定的地方（見[多韌體切換](../switching.md)）。
- 原因是兩者都源自 Felucca、共用同一套儲存格式與位址，ChoralRoot 開機時會去讀 SLOOP 留下的資料。上游的調查認為新版已處理，但結論仍標為未完全釐清。
- 上游建議的切換方式是 M-Flasher **加 m-bootloader**。我沒有 m-bootloader。
- 我用 `just emu-rehearse` 試著預演，但 ChoralRoot 的套件帶了韌體區以外的內容，預演出來是 `FONTS MISSING`，不能當證據。`[模擬]`

掉進 ROM boot mode 本身救得回來（M-Flasher 還看得到機器，可以還原備份），所以不是變磚的風險，但需要我在 GUI 手動還原。

**想刷的話比較穩的路徑**：先還原原廠 015 的備份（原廠 → ChoralRoot 是上游支援的路徑），再裝 ChoralRoot。代價是 SLOOP 的曲子要靠 `pre-…` 備份還原回來。這要等我決定之後再做。

## 已準備好的

`downloads/choralroot-0.41-smk37.1.fwsc` 已下載並驗證（這個 release 的 tag 帶 `smk37` 是因為同時發布了 SMK-37 版，這個檔案是 FM-1 用的）。在 fm1-emu 的空白 flash 上兩種晶片都能開機，各 66 次寫入。`[模擬]`
