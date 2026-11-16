# iOS 音樂 app

把 FM-1 接到 iPad 或 iPhone 時會用到的 app。連結與價格取自 App Store 的搜尋 API（美國區，2026-11-17），價格會變。每個 app 的用途說明是我的一般認識，沒有逐一和 FM-1 實測。要用哪個韌體去控制它們，見[搭配手機與平板](mobile.md)。

## 主機與路由

| App | 開發者 | 價格 | 用途 |
|---|---|---|---|
| [AUM - Audio Mixer](https://apps.apple.com/us/app/aum-audio-mixer/id1055636344) | Kymatica AB | $24.99 | AUv3 主機、mixer、音訊與 MIDI 路由。把 FM-1 的 MIDI 分送給各個音源、同時錄 FM-1 的 USB audio，通常就靠它 |
| [Loopy Pro](https://apps.apple.com/us/app/loopy-pro-looper-daw-sampler/id1492670451) | A Tasty Pixel | 免費（內購） | looper、sampler、AUv3 主機 |
| [GarageBand](https://apps.apple.com/us/app/garageband/id408709785) | Apple | 免費 | 入門 DAW，內建音源可以用外接鍵盤彈 |
| [StreamByter](https://apps.apple.com/us/app/streambyter/id1398712641) | Soft Audio | 免費 | MIDI 處理器：改 channel、過濾、remap。補 SLOOP 送出的 MIDI 不夠彈性的地方 |

## 音源與取樣

| App | 開發者 | 價格 | 用途 |
|---|---|---|---|
| [AudioKit Synth One](https://apps.apple.com/us/app/audiokit-synth-one-synthesizer/id1371050497) | AudioKit Pro | 免費 | 合成器 |
| [GeoShred](https://apps.apple.com/us/app/geoshred/id1064769019) | Wizdom Music | $16.49 | 物理建模吉他等音色，主打觸控與 MPE。FM-1 沒有 MPE，接上去只是一般鍵盤 |
| [Koala Sampler](https://apps.apple.com/us/app/koala-sampler-beat-maker/id1449584007) | Mazbox | $4.99 | 取樣與做 beat。可以直接取 FM-1 的 USB audio |

## Looper

| App | 開發者 | 價格 | 用途 |
|---|---|---|---|
| [Quantiloop Pro](https://apps.apple.com/us/app/quantiloop-pro-live-looper/id1156213359) | Stephan Marx | $14.99 | 現場 looper，可用 MIDI 控制。另有[免費版](https://apps.apple.com/us/app/quantiloop-live-looper/id1268082518) |

## 讓網頁工具在 iOS 上能用

Safari 沒有 Web MIDI，所以 SLOOP Web Editor、各家瀏覽器 installer 在 iOS 上不能直接開。需要帶 Web MIDI 的瀏覽器 app：

| App | 開發者 | 價格 | 備註 |
|---|---|---|---|
| [MIDIWeb Browser](https://apps.apple.com/us/app/midiweb-browser/id6757226617) | 5of12 LLP | 免費 | m-bootloader 的文件提到用 MIDIWeb app 從 iOS 安裝韌體 `[上游]` |
| [Web MIDI Browser](https://apps.apple.com/us/app/web-midi-browser/id953846217) | Takashi Mizuhiki | 免費 | 較早的同類 app |

我沒有確認這兩個 app 對 SysEx 的支援程度，而 editor 與 installer 都需要 SysEx。

## FM-1 專用

| App | 取得 | 用途 |
|---|---|---|
| RemoteFM1 | [TestFlight beta](https://testflight.apple.com/join/e6A47Xap) | 在 iOS 上跑 FM-1 韌體，FM-1 當面板；也是 AUv3。需要 FM-1 上裝 [RemoteFM1](../firmware/remotefm1/index.md) |

## 接線

- USB-C 的 iPad / iPhone：USB-C 資料線直連。
- Lightning 機型：Lightning to USB camera adapter。
- 免線：FM-1 上的韌體要有 Bluetooth MIDI（CTL-1、RemoteFM1、Melodee、原廠）。SLOOP 沒有。

## 還沒試

我還沒把 FM-1 接上任何 iOS 裝置。試過之後把實際可用的組合記在[使用心得](../notes/index.md)。
