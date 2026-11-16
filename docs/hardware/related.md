# 相關硬體

和 FM-1 共用晶片、韌體生態或工具的其他機器，以及救援與擴充用的小硬體。我只有一台 FM-1，這頁除了標 `[實機]` 的以外都是 `[上游]`，查核於 2026-11-17。

## 同一顆晶片的機器

| 機器 | 和 FM-1 的關係 | 韌體 |
|---|---|---|
| **Fame MiniFM** | FM-1 的換牌版本。M-Flasher 把它當成 FM-1，兩者的官方套件可以互刷（chip key、資料配置、boot 區相同），但互刷後機器會自報成另一個型號，M-UPGRADE 只接受那個型號的套件 | 和 FM-1 相同 |
| **M-VAVE SMK-37 Pro / Elite** | 37 鍵鍵盤，裡面是 FM-1 的晶片：flash 配置、音訊、更新路徑相同，只有面板接線不同。16 個 RGB pad、8 個旋鈕、4 個 fader、pitch / mod wheel、螢幕 | Firmware Hub 上有 26 個；多數是用 [fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37) 從 FM-1 韌體移植 |
| **MKE-P37**、**Donner Starrykey 37 Play** | 和 SMK-37 共用硬體 | 同 SMK-37 |

M-Flasher、m-bootloader、fm1-emu 都同時支援 FM-1 與 SMK-37。SMK-37 的套件不能刷進 FM-1，M-Flasher 會拒絕型號不符的套件。

對我的意義：如果之後想要有真正鍵盤、pad 與 fader 的版本，SMK-37 是同一套工具鏈能直接沿用的選擇。

## Firmware Hub 收錄的其他機器

和 FM-1 沒有技術關係，只是同一個目錄也收它們的韌體或 mod：

| 機器 | Hub 上有什麼 |
|---|---|
| Elektron Model:Cycles / Model:Samples | 6 個 mod |
| Erica Synths LXR-02 | 2026-10-23 起開源的韌體與 mod |

## VIBE FM Mk1（尚未上市）

Firmware Hub 背後的 Designburg（德國漢堡，Sven Trogus）正在做的自有硬體。它的頁面寫 **Coming 2027**，會走 crowdfunding，目前只能登記名單。規格與價格未公布，平台頁提到有一台 VIBE Box prototype。它**與 M-VAVE 無關**，網站自己寫明 not affiliated with M-VAVE。

比硬體更具體的是軟體平台，現在就能用：

| 東西 | 是什麼 | 狀態 |
|---|---|---|
| [VIBE FM SDK](https://github.com/Vibe-FM-SDK/vibe-fm-sdk) | 用 C 寫一個樂器，編成一個 `.wasm`，在瀏覽器、DAW、VIBE 硬體上跑同一個檔案 | Apache-2.0，2026-10-27 建立 |
| VIBE FM Playground / Web Player | 瀏覽器裡的 VIBE FM | 可用 |
| VIBE FM Plugin | VST3 / AU / CLAP | 免費，open beta |
| Converter | 把有原始碼的 FM-1 韌體轉成 VIBE 的 `.wasm`，保留 FM-1 的 240×240 畫面與控制 | 可用；只有開源韌體能轉 |
| [VIBE FM Link](https://github.com/zvenson/vibe-link) | 刷在 FM-1 上，把它變成 VIBE 的控制器 | GPL-3.0 |

和 FM-1 的相容性：

| 層次 | 能共用嗎 |
|---|---|
| 聲音引擎與 sequencer 的 C 程式碼 | 可以移植 |
| 介面 | 要適配。原生 VIBE 是 480×320 加 pad；轉換的 FM-1 韌體維持 240×240 |
| 韌體檔 | **不通用。** FM-1 是 `.fwsc`（原生機器碼），VIBE 是 `.wasm`。不能把 VIBE 的檔案刷進 FM-1 |

「VIBE FM Plugin 的 host 是閉源」這點來自 ChatGPT 的調研，我沒有在網站上找到明文，標 `[調研]`。

### 這個模式被驗證了嗎

我的看法：**需求有訊號，商業模式還沒被驗證。**

- 有訊號的部分：SLOOP 在 Hub 上約 11,500 次安裝，228 個韌體，M-VAVE 自己也跟進開放。有一群人願意在便宜硬體上玩自製韌體。
- 還沒發生的部分：VIBE 硬體沒出貨，crowdfunding 還沒開始，沒有售價，沒有規格。「會安裝免費韌體」和「會付錢買一台新機器」是兩回事。
- 它自己也承認風險：平台頁寫小團隊無法在工廠、價格、行銷上和大廠競爭，所以把賭注放在「一個能跑在各家硬體上的標準」，而不只是一台機器。

真正要觀察的是有沒有**別的廠商**採用它的 runtime。只有它自己一台機器的話，它就是另一台開放韌體的合成器。

對我目前的機器沒有影響。如果之後要寫自己的韌體，值得留意的是 SDK：同一份 C 程式碼除了 FM-1 還能在瀏覽器與 DAW 裡跑。

## 救援與擴充用的小硬體

| 硬體 | 用途 | 我有嗎 |
|---|---|---|
| **Raspberry Pi Pico**（RP2040）做的 USB_KEY | bootloader 壞掉時強制進 ROM download mode。[FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter) | 沒有。這是我暫緩 m-bootloader 的原因之一，見[上游追蹤](../watch.md)。Raspberry Pi 3 不能替代 |
| Flipper Zero | 另一種救援途徑（[flipper-fm1-recovery](https://github.com/merthsoft/flipper-fm1-recovery)） | 沒有 |
| [m-card](https://github.com/jvitkauskas/m-card) | 接在螢幕排線上的 microSD 擴充板，搭配 m-apps 做多韌體 slot | 沒有；上游標為未測試的 prototype |
| TRS-to-DIN MIDI 轉接線 | 讓外部鍵盤從 3.5 mm MIDI IN 進來（Type A 或 B，要試） | — |
| USB-C 資料線 | 刷機與 USB MIDI。只能充電的線不行，也不要經過 hub | 有 `[實機]` |

## 官方產品線

M-VAVE（公司網站 [cuvave.com](https://www.cuvave.com/products)）的其他產品我沒有整理。和這個 repo 有關的只有 FM-1 與 SMK-37 系列。
