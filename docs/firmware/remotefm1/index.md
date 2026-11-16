# RemoteFM1

[RemoteFM1](https://github.com/Quixotic7/RemoteFM1)（Quixotic7，GPL-3.0）是一個「自己幾乎什麼都不做」的韌體。它把每個鍵、按鈕、encoder 與 MASTER 旋鈕送給主機，再把主機送回來的東西顯示在 LED 與 240×240 螢幕上。真正的 FM-1 韌體在電腦、手機或瀏覽器上用全速執行，把實機當成面板。

**狀態（2026-11-17）：套件已下載驗證，模擬器裡開得起來，還沒刷。**

## 和其他韌體比的特點

| | 一般韌體（SLOOP、CTL-1…） | RemoteFM1 |
|---|---|---|
| 韌體在哪裡跑 | FM-1 上 | 主機上（瀏覽器、手機 app、VST） |
| 換韌體 | 重刷 | 在主機上點一下 |
| 受限於 FM-1 的 CPU 與記憶體 | 是 | 否 |
| 不接主機 | 完整功能 | 仍是一台 DX7、MIDI controller 與 looper |
| 聲音從哪出 | FM-1 | 主機（或經 USB audio 回到 FM-1） |

所以它不是 CTL-1 的替代品，兩者目的不同：**CTL-1 是把 FM-1 當成別的音源的控制器；RemoteFM1 是把 FM-1 當成「在主機上跑的 FM-1 韌體」的面板。** 想在不重刷的情況下試很多韌體，或想用手機當主機時才需要它。

## 主機 `[上游]`

| 主機 | 取得 |
|---|---|
| 瀏覽器 | [remotefm1.com](https://remotefm1.com/)，列了約 160 個韌體 |
| Android | [APK](https://github.com/Quixotic7/RemoteFM1Android/releases/latest)，自行安裝 |
| iPhone / iPad | [TestFlight beta](https://testflight.apple.com/join/e6A47Xap)，也是 AUv3 |
| Mac | [FM1VST](https://github.com/Quixotic7/FM1VST)（VST3 / AU） |

連線走 USB 或 Bluetooth MIDI。Bluetooth 頻寬有限，上游說螢幕與聲音只能擇一。更多見[搭配手機與平板](../../hardware/mobile.md)。

## 不接主機時

螢幕顯示 `REMOTE FM1 / WAITING FOR HOST`。此時它是：

- 一台 DX7（24-bit，Yamaha 的 256 個 ROM 音色加自己的 cartridge）。
- **MIDI REMOTE**：`GLO` + `FX` 把整個面板變成純 MIDI controller，鍵送 note、按鈕與 encoder 送 CC。再按一次離開，關機後不保留。
- 一個 looper。

## 兩個套件

| 檔案 | 內容 |
|---|---|
| `remote-0.3.6.fwsc` | USB + Bluetooth。我下載的是這個 |
| `remote-usb-0.3.6.fwsc` | 沒有無線電 |

## 在我的機器上會動到什麼

- Hub 目錄標為認得新晶片。fm1-emu 兩種晶片都開機，各 9 次寫入。`[模擬]`
- 蓋在我的 SLOOP 備份上預演時，第一次開機改寫 `0xF2000`–`0xF3FFF`。其中 `0xF2000` 在我的映像裡有資料，是原廠留下的、SLOOP 沒動過的區域。SLOOP 的曲子不受影響，但那塊原廠資料之後只能從原廠備份還原。`[模擬]`

## 安裝

```sh
just fm1-switch remote
```

上游的救援方式：按住 `REC` 開機進晶片的 update mode。
