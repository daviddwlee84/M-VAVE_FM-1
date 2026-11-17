# 接口與連線

## 各接口能做什麼

| 接口 | 能做 | 不能做 |
|---|---|---|
| **USB-C** | 充電、USB MIDI in/out、韌體更新、USB audio（依韌體）、自訂 HID 裝置（依韌體）、USB host（只有少數韌體，見下） | 供電給接上的裝置 |
| **3.5 mm TRS MIDI IN** | 接收其他設備的 note、CC、clock | 不是 MIDI OUT |
| **3.5 mm 音訊輸出** | 立體聲類比輸出到耳機、mixer、錄音介面 | 不是 audio in，也不是 MIDI |
| **Bluetooth MIDI** | 硬體本身有無線電（SoC 內建 Bluetooth 與 WiFi）。原廠、CTL-1、Melodee、RemoteFM1、ChoralRoot 會用 | 要韌體開啟才有。SLOOP 不支援，它不開無線電。Agent Deck 也不支援：它的基底 Dinghy 完全沒有藍牙程式 `[上游]` |

## 沒有音訊輸入：能不能當 sampler { #no-audio-in }

FM-1 **沒有任何類比音訊輸入**，沒有麥克風，3.5 mm 孔只是輸出。所以不能像一般 sampler 那樣把線接進來錄。`[上游]`

聲音要進到 FM-1，只能**從電腦或手機用數位方式送進去**，而且要韌體支援：

| 方式 | 例子 | 狀態 |
|---|---|---|
| 從網頁上傳 sample 檔案 | SLOOP 的 [Web Editor](../firmware/sloop/web-editor.md)（4 個 slot，可以丟長錄音進去 CHOP）、官方 Workbench、Mini-Grain、ECHOFIELD 的 companion web app | `[上游]` |
| 讓 FM-1 當 USB 音效卡的**輸出端**，收主機播的聲音 | CTL-1（iPad 的聲音會從 FM-1 播出來，我實測過）、FM1 Audio Router | `[實機]` CTL-1 |
| 收主機的 USB 音訊並錄成 sample | ECHOFIELD 的說明寫 "your own samples (or recorded over USB)"，但沒說是哪個方向 | 沒查證 `[調研]` |

所以「手機或網頁 + FM-1」的組合可以做到 sampler 的效果：在手機或電腦上錄好、剪好，再上傳到支援 sample 的韌體。ChoralRoot 做不到，它的聲音全是合成的，沒有 sample 引擎（GRAIN 引擎是對它自己生成的素材做 granular，不能載入外部 sample），USB 音訊也只有輸出到主機的方向。`[上游]` `[實機]`

Firmware Hub 上找 sampler 類韌體：在 [catalog](https://thefirmwarehub.com/) 搜尋 sample、granular。

## USB-C：device，不是 host

FM-1 是 USB device。USB 鍵盤直接插在 FM-1 上不會動，因為兩邊都是 device，中間需要一個 host：電腦、手機或 USB MIDI host box。

**例外（更正 2026-11-17）**：ChoralRoot 0.31 起有 **Options → USB Host**，打開後 USB-C 變成 USB MIDI host，可以直接接一台 USB MIDI 鍵盤，或用一條 USB-C 線把兩台 FM-1 互接。限制：FM-1 不供電，鍵盤要透過有供電的 hub 或 OTG 轉接頭；只支援 full-speed；開著時電腦、USB audio、更新都不能用；每次開機都回到一般 device 模式，不會存。`[上游]`（[release notes](https://github.com/Quixotic7/ChoralRootFM1/blob/main/docs/releases/0.31.md)）其他韌體預設仍是 device。

反過來，電腦對 FM-1 可以做的事很多：

- 接收琴鍵與旋鈕送出的 MIDI note / CC。
- 送 MIDI 回去控制合成器或 sequencer。
- 用支援 USB audio 的韌體直接數位錄音。
- 透過雙向 MIDI / SysEx 自訂螢幕內容、LED 狀態與 host 控制協定（Agent Deck 就是這樣做的）。
- 用自訂韌體讓 FM-1 變成 HID 鍵盤 / 滑鼠（Trireme）。

注意不同韌體的 MIDI mapping 不同。不能假設每個實體按鍵在原廠韌體下都會送出獨立事件。`[調研]`

## 兩台 FM-1 互連 { #two-units }

有沒有辦法讓兩台 FM-1 直接互傳？硬體和 fm1-base 都留了路，但 2026-11-17 為止沒有找到有人在兩台實機上做過的紀錄。`[上游]` `[調研]`

| 方式 | 怎麼接 | 限制 | 狀態 |
|---|---|---|---|
| **藍牙直連** | 一台開 `ble_central = true` 去掃描、連上另一台（它以 `FM-1 XXXX` 廣播 BLE MIDI） | fm1-base 自己的 BLE stack（`ble_stack = "own"`）一次只能有一條藍牙 MIDI 連線，連著另一台時電腦就不能走藍牙（USB 還可以）；JieLi 的 stack 可以兩條同時連，但會丟掉收到的 SysEx。central 的 connection interval 是 11.25 ms | fm1-base 只在模擬的無線電上測過（`docs/bt/BLE-DEVICES-DESIGN.md`、`tests/bt/ble_f2f_test.c`），沒有在兩台實機上測過 `[上游]` |
| **USB-C 線** | 一台切成 USB host（fm1-base `usb_host = true`，或 ChoralRoot 的 Options → USB Host） | FM-1 的 USB 口不供電；兩台都有電池，所以被接的那台可能不需要，但 FM-1 在沒有 VBUS 時會不會被列舉，沒有文件。fm1-base 的 host 模式收不到 SysEx，也不送 SysEx，note 和 CC 可以；它的 host 輸出路徑還沒在實機驗證 | ChoralRoot 0.31 的 release notes 說可以用 USB-C 線把兩台互接；[MIDIMON](https://github.com/ignis32/midimonitor_fm1) 的作者說接的裝置要有自己的電源（「例如另一台 FM-1」），並用它 debug 另一台 `[上游]` |
| **經過電腦** | 兩台都接電腦（USB 或藍牙），由電腦轉送；也可以經網路，例如 Vail | 要有電腦 | 最可靠 |
| TRS MIDI | — | FM-1 只有 MIDI IN，沒有 OUT，做不到雙向 | 不可行 |

要讓同一個程式走得過以上每一條路，最穩的是只用 note on / off 這類一般 MIDI 訊息，不要依賴 SysEx。自製的[摩斯電碼機](../dev/morse.md)就是這樣設計：線上只傳按鍵的按下與放開。

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
