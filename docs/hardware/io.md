# 接口與連線

## 各接口能做什麼

| 接口 | 能做 | 不能做 |
|---|---|---|
| **USB-C** | 充電、USB MIDI in/out、韌體更新、USB audio（依韌體）、自訂 HID 裝置（依韌體） | 當 USB host |
| **3.5 mm TRS MIDI IN** | 接收其他設備的 note、CC、clock | 不是 MIDI OUT |
| **3.5 mm 音訊輸出** | 立體聲類比輸出到耳機、mixer、錄音介面 | 不是 audio in，也不是 MIDI |
| **Bluetooth MIDI** | 硬體本身有無線電（SoC 內建 Bluetooth 與 WiFi）。原廠、CTL-1、Melodee、RemoteFM1、ChoralRoot 會用 | 要韌體開啟才有。SLOOP 不支援，它不開無線電 |

## USB-C：device，不是 host

FM-1 是 USB device。USB 鍵盤直接插在 FM-1 上不會動，因為兩邊都是 device，中間需要一個 host：電腦、手機或 USB MIDI host box。

反過來，電腦對 FM-1 可以做的事很多：

- 接收琴鍵與旋鈕送出的 MIDI note / CC。
- 送 MIDI 回去控制合成器或 sequencer。
- 用支援 USB audio 的韌體直接數位錄音。
- 透過雙向 MIDI / SysEx 自訂螢幕內容、LED 狀態與 host 控制協定（Agent Deck 就是這樣做的）。
- 用自訂韌體讓 FM-1 變成 HID 鍵盤 / 滑鼠（Trireme）。

注意不同韌體的 MIDI mapping 不同。不能假設每個實體按鍵在原廠韌體下都會送出獨立事件。`[調研]`

## TRS MIDI IN

需要 **TRS-to-DIN MIDI 轉接線**。沒聲音時試另一種型式（Type A / Type B）。

SLOOP 下的 channel 對應：

| MIDI channel | 播放 |
|---|---|
| 1、2、3 | synth track 1、2、3 |
| 10 | drum track |
| 其他 | 目前選取的 track |

可以在 GLO → MIDI 改。MIDI clock in 可從 USB 或 TRS 進來（GLO → SYSTEM → SYNC）。

## USB audio（SLOOP）

SLOOP 下 FM-1 同時是一個名為 **"Felucca"** 的 USB audio 輸入：44.1 或 48 kHz、16-bit stereo、class compliant，不需要 driver。錄到的是 master 輸出，跟耳機聽到的一樣。MIDI、Web Editor、installer 都走同一條線。

- 音量：按住 `HOME` 進選單 → **USB AUDIO**。`MASTER` 跟著 master 旋鈕；`FULL` 是固定滿音量，適合沒有輸入增益的錄音介面。
- 安裝後第一次要拔插一次，電腦才會重新辨識成 MIDI + audio 裝置。

## 用電腦接收 MIDI 的最小範例

確認電腦看到的 MIDI port（需要 `pip install mido python-rtmidi`）：

```python
import mido

print(mido.get_input_names())
with mido.open_input(mido.get_input_names()[0]) as port:
    for msg in port:
        print(msg)
```

這是通用的 `mido` 用法，不是某個上游專案的指令。port 名稱依韌體而定。
