# 搭配手機與平板

FM-1 接手機有三種不同的玩法，容易混在一起：

| 玩法 | 誰在跑韌體 | 手機的角色 |
|---|---|---|
| **A. 手機當 editor / 錄音機** | FM-1 自己（例如 SLOOP） | 觸控螢幕上的編輯器、mixer、錄音 |
| **B. 手機當主機，FM-1 當面板** | 手機（RemoteFM1） | 跑韌體、發聲、大螢幕；FM-1 只負責鍵、旋鈕、LED、小螢幕 |
| **C. FM-1 當 MIDI 控制器** | FM-1（CTL-1 等） | 跑自己的音樂 app（AUM、GarageBand…），FM-1 送 MIDI 給它 |

你看到的「FM-1 + Android」組合多半是其中一種。這頁都是 `[上游]`，我自己還沒接過手機。查核於 2026-11-17。

## 連線的基本事實

- FM-1 是 USB **device**，手機當 **host**。USB-C 對 USB-C 的資料線，或 OTG 轉接頭。
- Bluetooth MIDI 要看韌體：原廠、CTL-1、Melodee、RemoteFM1 有；**SLOOP 沒有**，只能走 USB。
- SLOOP 的 USB audio 支援 48 kHz，是為了手機（很多手機與 app 只收 48 kHz）。手機錄起來失真時，把選單裡的 **USB RATE** 設成 44.1 kHz 試試。

## A. SLOOP + 手機：把 Web Editor 當觸控螢幕

SLOOP 的 [Web Editor](../firmware/sloop/web-editor.md) 在手機上能做的事：

- **LIVE 頁**：為了從電腦、平板或手機控制整場演出而設計。PLAY / REC、BPM、section A–D 切換、mixer（音量、mute、solo、filter、pan、send）、27 個 punch-in effect 做成 pad、琴鍵與鼓 pad、scene、playlist。這最接近「多了一塊觸控螢幕」。
- **編輯**：Sound、Sequencer（含 piano roll）、Samples、Drum synth 等所有頁面。
- **備份**：把整台機器上的東西存成一個檔案、或放回去。
- **錄音**：FM-1 是 class-compliant 的 USB audio 輸入，手機上的 sampler 或錄音 app 可以直接錄 master 輸出，和 editor 共用同一條線。

Android 的用法：

1. 用 Chrome 連網開一次[線上 editor](https://isod89.github.io/sloop-fm1/webapp/editor/)，或這個站[內嵌的 2.8.5](../firmware/sloop/editor.md)。
2. 選 **Install the app**（或 Chrome 選單 → Install app / Add to Home screen）。之後不需要網路。
3. FM-1 用 USB 接上，按 Connect，允許 MIDI。

!!! warning "Android 上不要用下載的單一 HTML 檔"
    Android 的 Chrome 不允許下載的檔案使用 MIDI。用上面的 app（PWA）方式。離線 HTML 檔是給電腦用的。

## B. RemoteFM1：手機跑韌體，FM-1 當面板

[RemoteFM1](https://github.com/Quixotic7/RemoteFM1) 是一個「自己幾乎什麼都不做」的 FM-1 韌體：它把每個鍵、按鈕、encoder、MASTER 旋鈕送給主機，再把主機送回來的東西顯示在 LED 與 240×240 螢幕上。主機（電腦、手機、瀏覽器）用全速跑一個 FM-1 韌體，把真的 FM-1 當成它的面板。

這帶來幾件事：

- **不用重刷就能換韌體**，點一下就切。
- 可以跑 FM-1 自己的 CPU 與記憶體裝不下的東西。
- 手機的大螢幕與喇叭 / 耳機，加上實體的鍵與旋鈕。

| 主機 | 取得方式 |
|---|---|
| 瀏覽器 | [remotefm1.com](https://remotefm1.com/)（FM-1 Player，列了約 160 個韌體） |
| Android | 從 GitHub Releases 下載 [APK](https://github.com/Quixotic7/RemoteFM1Android/releases/latest) 自行安裝（不在 Play Store）。需要 Android 10 以上、arm64 |
| iPhone / iPad | [TestFlight beta](https://testflight.apple.com/join/e6A47Xap)，也是 AUv3 plugin |
| Mac | [FM1VST](https://github.com/Quixotic7/FM1VST)（VST3 / AU 與桌面 app） |

連線走 USB 或 Bluetooth MIDI。沒接主機時它自己也是一台樂器：DX7、MIDI controller、looper。

要知道的限制：

- **Bluetooth 的頻寬有限**：上游說螢幕與聲音只能擇一。走 USB 比較完整。
- **已知問題**：部分 Samsung / Qualcomm 裝置（上游舉 Galaxy Tab A9+）透過 FM-1 的 USB audio 發聲會失敗，還可能讓 FM-1 從 USB 掉線。改用手機自己的喇叭、耳機或藍牙。
- **Android 側載**：Samsung 要先暫時關掉 Auto Blocker。
- 並非所有韌體都有 `.rfm1` 版本。上游說約 70 多個以 `.rfm1` 執行，其餘在模擬器裡跑。
- Firmware Hub 的目錄把 RemoteFM1 0.3.6 標為認得新晶片（從套件判讀，不是實機測試）。

## C. 當 iPad 上音樂 app 的 MIDI 控制器

想用 FM-1 控制 AUM、GarageBand、GeoShred、Koala、AudioKit Synth One、Quantiloop、StreamByter 這類 app 時，重點不在 app，而在 **FM-1 上裝的韌體送出什麼 MIDI**。FM-1 是 class-compliant 的 USB MIDI 裝置，iPadOS 的 CoreMIDI 不需要 driver，任何吃 MIDI 輸入的 app 都看得到它。各 app 怎麼對應（MIDI learn、channel 設定）要各自設定，我沒有逐一測過。

| FM-1 上的韌體 | 送出什麼 | 適合 |
|---|---|---|
| **[CTL-1](https://github.com/charlesvestal/fm1-ctl)** | 專門的 MIDI controller：琴鍵（含 velocity 與 aftertouch、scale、黑鍵和弦）、KNOB 1–4 送 CC（四個 bank，CC 編號可改）、program change、sustain、mod wheel、arp、四軌 Euclidean 鼓 sequencer。USB 與 **Bluetooth MIDI** 都有，自己也有 General MIDI 音色 | **首選。** 旋鈕能對應 app 的參數，藍牙可以免線 |
| **SLOOP** | 琴鍵一定會送（track 1–3 在 channel 1–3、drum 在 10）；設 GLO → MIDI 2 → OUT = SEQ 後連 sequencer、arp、roll 也送。**旋鈕不送 CC**，沒有 Bluetooth | 把 SLOOP 當 sequencer 去驅動 app 的音源，或單純當鍵盤 |
| **RemoteFM1**（不接 host） | 內建 MIDI REMOTE 模式（`GLO` + `FX`）：整個面板變成純 MIDI controller，鍵送 note、按鈕與 encoder 送 CC。USB 與 Bluetooth | 已經裝了 RemoteFM1 時順便用 |
| **原廠韌體** | 有 Bluetooth MIDI；按鍵與旋鈕各送什麼我沒有查 `[調研]` | — |

幾個實際會遇到的點：

- **接線**：USB-C iPad 用 USB-C 資料線直接接。Lightning 的機型要 Lightning to USB camera adapter。
- **CTL-1 的聲音路由** `[實機]`：接上 USB 後 iPad 的聲音輸出會自動改到 FM-1，而且 CTL-1 自己的 GM 音色會疊上去。關掉內建音色、或改用 Bluetooth 避開 USB audio 的做法見 [CTL-1](../firmware/ctl-1/index.md#ipad-audio)。
- **ChoralRoot 的聲音路由** `[實機]`（Mac 上確認）：只有輸入（`ChoralRoot In`，6 channel），沒有輸出，所以 iPad 的聲音不可能從 FM-1 出來。見 [ChoralRoot](../firmware/choralroot/index.md#midi-out)。
- **SLOOP 的聲音路由**：SLOOP 同時是 USB audio **輸入**裝置（FM-1 的聲音進 iPad）。iPad 接上後可能把它選成錄音來源，這對 AUM、Koala 取樣是好事；但也要確認 iPad 的**輸出**沒有被改掉。這點我沒有實機確認。RemoteFM1 的文件則明說 FM-1 插上 USB 後會自動成為手機的聲音輸出。
- **雙向**：AUM 或 StreamByter 可以把 MIDI 送回 FM-1。SLOOP 會收 note、CC、clock（channel 對應見[接口與連線](io.md)），所以可以讓 iPad 當 clock master，或用 iPad 上的 sequencer 彈 SLOOP 的音色。
- **StreamByter** 這類 MIDI 處理器正好補 SLOOP 的不足：把收到的 note 轉 channel、過濾、remap 再送給音源。
- **GeoShred** 以 MPE 與觸控演奏為主，FM-1 的鍵沒有 MPE，接上去只是一般鍵盤。

如果主要目的是控制 iPad 上的 app，我會刷 CTL-1；如果是讓 SLOOP 和 iPad 一起玩（SLOOP 出鼓與 bass，iPad 出別的音色並錄音），維持 SLOOP 就可以。CTL-1 在 Firmware Hub 的目錄標為認得新晶片，還沒在我的機器上試過。

## iOS 比較難嗎

比我原本以為的好，但確實分兩種情況：

| | Android | iOS / iPadOS |
|---|---|---|
| 瀏覽器裡的 Web MIDI | Chrome 支援，所以 SLOOP Web Editor、各家瀏覽器 installer 都能用 | Safari **不支援 Web MIDI**。網頁版的 editor 與 installer 不能直接用 |
| 繞過的方法 | 不需要 | 用帶 Web MIDI 的第三方瀏覽器 app。m-bootloader 的文件提到用 MIDIWeb app 從 iOS 安裝韌體；RemoteFM1 的文件說在這類 app 裡整個網站都能跑，並在 iPhone 13 Pro Max 上測過 |
| 原生 app | RemoteFM1 APK（側載） | RemoteFM1（TestFlight），還多了 AUv3，可以放進 iOS 的 DAW |
| USB audio | 視機型，有已知問題 | 插上 USB 後 FM-1 會自動成為手機的聲音輸出；Lightning 機型需要 Lightning to USB camera adapter |
| 自己寫 app | — | [FM1Kit](https://github.com/ocean/FM1Kit)：Swift + CoreMIDI 的工具包 |

所以：**走網頁的玩法（A）在 iOS 比較麻煩，走原生 app 的玩法（B）iOS 反而有 AUv3 這個優勢。**

## 對我的機器

- 目前裝的是 SLOOP，所以玩法 A 現在就能試，不用刷任何東西。
- 玩法 B 要先刷 RemoteFM1。它不需要 m-bootloader，流程和刷 Agent Deck 一樣：先在 fm1-emu 的新晶片模式跑過，再 `just fm1-switch <檔案>`。
- 還沒試過的都標在[使用心得](../notes/index.md)的待辦裡。
