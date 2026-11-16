# CTL-1

[CTL-1](https://github.com/charlesvestal/fm1-ctl)（Charles Vestal，MPL-2.0）把 FM-1 變成 MIDI controller：琴鍵送音符，旋鈕送 CC，另有 arpeggiator 與四軌 Euclidean 鼓 sequencer。USB 與 Bluetooth 都送，TRS 進來的 MIDI 會轉送出去，機器自己也用 General MIDI 音色發聲，所以什麼都不接也能彈。它建立在 [fm1-base](../../dev/index.md) 上。

**狀態（2026-11-17）：已刷進實機（新晶片 `0xB36014`），接 iPad 試過，能用。**

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

## 接 iPad 時聲音重疊的處理 { #ipad-audio }

我接 iPad 時遇到兩件事 `[實機]`，原因與解法來自上游手冊與 fm1-base 的文件 `[上游]`：

**1. CTL-1 自己的音色和 iPad 的聲音疊在一起。** CTL-1 內建 128 個 General MIDI 音色，你彈什麼它就跟著發聲。關掉它：

1. 按 **`EDIT`** 開 Sound 頁。
2. **KNOB 1（Sound）** 轉到 **Off**。
3. 順便把 **KNOB 2（MIDI in）** 設成 **Ignored**，這樣 iPad 送回來的 MIDI 也不會觸發它的音色。
4. 按 **`SAVE`** 存起來，否則重開機會回到 On。

**2. iPad 的聲音輸出自動跑到 FM-1 的喇叭。** 這是設計如此：fm1-base 的韌體預設同時是 USB audio 的輸入與**輸出**裝置（「a computer plays through the FM-1」），iPadOS 一看到 USB 音訊輸出就會切過去。iPad 的聲音會從 FM-1 的喇叭與耳機孔出來，音量由 **MASTER** 控制。

怎麼處理看你要哪一種：

| 想要 | 做法 |
|---|---|
| 聲音就從 FM-1 出（接耳機到 FM-1） | 維持現狀，只把上面的 Sound 關掉。這其實是最簡單的單線配置 |
| 聲音留在 iPad 的喇叭或 iPad 接的音響 | **改用 Bluetooth MIDI，不接 USB。** CTL-1 一開機 Bluetooth 就開著，裝置名稱是 `FM-1`，從音樂 app 的 Bluetooth MIDI 設定連線（GarageBand：Settings → Advanced → Bluetooth MIDI Devices）。沒有 USB 就沒有 USB audio，iPad 的輸出不會被搶走 |
| 用 USB 但不要 USB audio | CTL-1 沒有這個選項。上游只提供「有無 Bluetooth」兩個套件，USB audio 都是開的。要關得自己用 fm1-base 重新 build（`usb_audio_out = false`） |

iPadOS 接著 USB 音訊裝置時能不能手動把輸出切回內建喇叭，我沒有確認，各 app 的行為也不同（AUM 可以自己選硬體輸出）。

**MASTER 旋鈕**：設成 Volume（預設）時控制 FM-1 發出的音量，包含 iPad 經 USB 送過來的聲音。也可以在 Sound 頁用 KNOB 3 把它改成送某個 CC。

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
| **實機**，2026-11-17 03:41 | `just fm1-switch ctl`：寫入 140 個 sector（`0x04000`–`0x8FFFF`），驗證通過，自動重開，USB MIDI 正常 `[實機]` |

`0xFC000` 是 SLOOP 也在用的設定 sector（見[多韌體切換](../switching.md)），CTL-1 存設定時會覆寫它。SLOOP 的曲子在 `0x97000` 與 `0x9F000`，不受影響。

## 安裝

```sh
just fm1-fetch-more      # 已做過：下載並驗證
just fm1-switch ctl      # download mode → 備份 → 安裝（輸入 yes）→ 重開
```

M-Flasher 列出的寫入範圍如果超出韌體區，先不要輸入 `yes`。

## 裝了之後要確認的

- [x] 螢幕、琴鍵正常；電腦上看得到 USB MIDI 裝置；接 iPad 能用
- [ ] 旋鈕送出 CC（用 `mido` 或任何 MIDI monitor 看）
- [ ] Bluetooth MIDI 能被 Mac 或 iPad 找到
- [ ] `SAVE` 後關機再開，設定還在（新晶片必做）
- [ ] 切回 SLOOP 後曲子與設定的狀況
