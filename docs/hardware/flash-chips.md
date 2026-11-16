# 兩種 Flash 晶片

2026 年後期出貨的 FM-1 換了 flash 晶片。硬體規格幾乎一樣，真正的差別在韌體相容性與救援風險。

| | 早期 FM-1 | 2026 年後期起 |
|---|---|---|
| 型號 | Puya **P25Q80H** | UCUN **UC25HQ80** |
| JEDEC ID | `0x856014` | `0xB36014` |
| M-Flasher 顯示 | `0x856014 (old)` | `0xB36014 (new)` |
| 容量 | 1 MiB | 1 MiB |
| 介面 | SPI NOR | SPI NOR |

沒有證據顯示新晶片讓合成器變快、音色變多或音質變好。`[調研]`

## 為什麼換一顆 flash 會出問題

基本的讀取、erase、page program 指令兩者相容。問題出在韌體的晶片辨識：舊程式只認得 Puya 的 JEDEC ID，遇到 `0xB36014` 就當成未知晶片而拒絕寫入。

實際出現過的結果：

| 現象 | 後果 |
|---|---|
| 韌體不認得 JEDEC ID | 存 preset、project 失敗 |
| 只存到 RAM | 關機後音色與 project 消失 |
| update loader 不認得新晶片 | 無法更新到修正版 |
| bootloader 不完全相容 | 可能無法開機，需要低階救援 |

SLOOP 2.8.5 的 release note 寫的就是這件事：新晶片的機器「存檔與更新現在可以用了，之前被當成未知晶片」。

## 買之前能知道嗎

不能。目前沒有可靠的公開型號或 SKU 可以在購買前判斷。有社群回報新機螢幕下方印有 M-VAVE logo，但那不是經過驗證的對應關係，不能只靠外觀、顏色或購買日期判斷。`[調研]`

可靠的方法只有一個：拿到機器後用 M-Flasher 進 download mode 讀 `FLASH CHIP`。這是讀取操作，不需要先刷任何東西。見[備份原廠與確認晶片](../getting-started/backup-and-chip.md)。

## 各韌體對新晶片的支援

| 韌體 | 新晶片 | 依據 |
|---|---|---|
| 原廠 | 正常 | 出廠即如此 |
| SLOOP 2.8.5 | 支援 | `[上游]` release note |
| Clipper 1.3.1 | 支援 | `[調研]` |
| Melodee 1.6.1 | 支援 | `[調研]` |
| m-bootloader 2.4 | 測試很少，有啟動失敗回報 | `[上游]` README |
| Agent Deck | 沒找到明確的實機驗證 | `[調研]` |
| 其他 | 未知，自己確認 | — |

韌體在模擬器裡能開機，不代表存檔、更新與實機都沒問題。

## 常見說法對照

有一支介紹新舊晶片的影片（[FM-1 New VS Old Flash Chip](https://www.youtube.com/watch?v=VNFZqDnTFYs)）。我看的是 Gemini 的摘要，不是影片本身，所以以下是 `[調研]` 對 `[上游]` 的比對。

| 影片的說法（摘要） | 和上游一致嗎 |
|---|---|
| 螢幕下方的 logo 不能判斷晶片，有 logo 的也可能是舊晶片 | 一致。唯一可靠的是 M-Flasher 讀 `FLASH CHIP` |
| 新舊晶片硬體規格相同，差別只在晶片 ID | 一致 |
| 在 Firmware Hub 篩選支援新晶片的韌體 | 一致。見 [The Firmware Hub](../firmware/hub.md)。影片說 157 款相容、73 款不相容；我 2026-11-17 從目錄算到 152 與 71（另有 5 個不檢查晶片） |
| 不支援的韌體通常改兩行程式碼就能相容 | 合理。問題就是 JEDEC ID 的白名單 |
| 新晶片刷到不支援的舊韌體「可能導致設備損壞」 | **說得太重。** 上游與 Hub 描述的後果是存檔只留在 RAM、關機消失，以及之後的更新可能失敗，不是硬體損壞。麻煩在於可能卡在那個韌體上，要靠 download mode 救回來 |
| 無論新舊晶片都建議裝 bootloader 來防變磚 | **和上游不一致。** 見下 |

### 「裝 bootloader 防變磚」為什麼不適用我這台

這句話有一半是對的：m-bootloader 讓「韌體刷壞」之後能從按鍵進 recovery，不必靠額外硬體，這是真的保護。

但安裝它本身就是對 boot 區的寫入，而 m-bootloader 作者自己在 README 寫：新晶片測試很少，一台裝完無法啟動、原因不明，另一台正常，結論是 `0xB36014` 的機器 install it at your own risk。影片的「無論新舊都建議裝」沒有帶這個保留。

所以它防的是一種磚（韌體壞掉），代價是冒另一種磚（bootloader 壞掉）的風險；後者在新晶片上還沒有足夠的資料，而且只有 USB_KEY 能救。對舊晶片機，裝它是合理的。對我這台，維持[暫緩](../watch.md)。

另外，沒有 m-bootloader 也不是毫無退路：SLOOP 與 Agent Deck 執行中都能由 M-Flasher 透過 USB MIDI 切進 download mode，Dinghy / Felucca 系的韌體按住 `OCT-` 開機有自己的 USB rescue。`[實機]` 前者我已實際用過多次。

## 可以先在模擬器試

fm1-emu 能模擬新晶片：網址加 `?chip=uc25hq80`，或本機 `fm1-live --flash-chip uc25hq80`。只認得 Puya 的韌體在那個模式下會把存檔留在 RAM。做一樣的操作（改音色、Save、重開）比較兩種晶片，就能在刷機前抓到一部分問題。見 [fm1-emu 模擬器](../agentic/emulator.md)。

## 如果拿到新版

不需要為此退換貨。只是順序要保守：先完整備份、確認目標韌體支援新晶片，m-bootloader 先不要裝。
