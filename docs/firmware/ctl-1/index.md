# CTL-1

[CTL-1](https://github.com/charlesvestal/fm1-ctl)（Charles Vestal，MPL-2.0）把 FM-1 變成 MIDI controller：琴鍵送音符，旋鈕送 CC，另有 arpeggiator 與四軌 Euclidean 鼓 sequencer。USB 與 Bluetooth 都送，TRS 進來的 MIDI 會轉送出去，機器自己也用 General MIDI 音色發聲，所以什麼都不接也能彈。它建立在 [fm1-base](../../dev/index.md) 上。

**狀態（2026-11-17）：套件已下載驗證，模擬器預演通過，還沒刷進實機。**

## 為什麼想試

想接 iPad 控制音樂 app 時它是首選：SLOOP 的旋鈕不送 CC、也沒有 Bluetooth，CTL-1 兩者都有。見[搭配手機與平板](../../hardware/mobile.md)與 [iOS 音樂 app](../../hardware/ios-apps.md)。

## 控制項 `[上游]`

| 控制 | 作用 |
|---|---|
| 琴鍵 | 音符，含 velocity 與 aftertouch（從鍵本身讀）；設了 scale 時白鍵是音階、黑鍵是該音階的和弦 |
| KNOB 1–4 | 該頁的四個值；在 Play 頁是四個 CC，共四個 bank |
| `HOME` `SEL` `ARP` `SEQ` `EDIT` `GLO` | 六個頁面：Play、Keys、Arp、Sequencer、Sound、Setup |
| SELECT | 頁面內的子頁：Play 的 bank、Sequencer 的 track |
| ALGORITHM / PRESETS / MASTER | MIDI channel / program change（音色跟著換）/ 音量 |
| `FX` | 下一個 bank；按住時 KNOB 1–4 設定該 bank 的 CC 編號 |
| `ENV` / `LFO` | sustain（點兩下鎖定）/ mod wheel |
| `PLAY` / `REC` / `SAVE` | start / stop、panic、儲存 |
| `OCT-` / `OCT+` | 八度 |

開機畫面顯示 Bank A 預設是 CC 74、CC 71、CC 1、CC 7。`[模擬]`

完整說明：[上游手冊](https://charlesvestal.github.io/fm1-ctl/manual/)。[瀏覽器試玩](https://charlesvestal.github.io/fm1-ctl/emu/)。

## 兩個套件

| 檔案 | 內容 |
|---|---|
| `ctl-1.0.3.fwsc` | USB、TRS、Bluetooth MIDI。我下載的是這個 |
| `ctl-usb-1.0.3.fwsc` | 同上但沒有 Bluetooth |

## 在我的機器上會動到什麼

| 來源 | 結果 |
|---|---|
| Firmware Hub 目錄 | 標為認得新晶片（從套件判讀） |
| `just emu-shot`，兩種晶片 | 都開機，各 12 次 flash 寫入 `[模擬]` |
| `just emu-rehearse`，蓋在我真實的 SLOOP 備份上 | 開到 Play 頁。第一次開機只改寫 `0xDE000`–`0xDFFFF`，在我的映像裡原本是空的 `[模擬]` |
| 原始碼 `app.h` | 設定存在 `0xFC000`，面板校正 `0xDC000`，Bluetooth 設定 `0xDE000` `[上游]` |

`0xFC000` 是 SLOOP 也在用的設定 sector（見[多韌體切換](../switching.md)），CTL-1 存設定時會覆寫它。SLOOP 的曲子在 `0x97000` 與 `0x9F000`，不受影響。

## 安裝

```sh
just fm1-fetch-more      # 已做過：下載並驗證
just fm1-switch ctl      # download mode → 備份 → 安裝（輸入 yes）→ 重開
```

M-Flasher 列出的寫入範圍如果超出韌體區，先不要輸入 `yes`。

## 裝了之後要確認的

- [ ] 螢幕、琴鍵、旋鈕正常；電腦上看得到 USB MIDI 裝置
- [ ] 旋鈕送出 CC（用 `mido` 或任何 MIDI monitor 看）
- [ ] Bluetooth MIDI 能被 Mac 或 iPad 找到
- [ ] `SAVE` 後關機再開，設定還在（新晶片必做）
- [ ] 切回 SLOOP 後曲子與設定的狀況
