# The Firmware Hub

[The Firmware Hub](https://thefirmwarehub.com/) 是目前最大的 FM-1 社群韌體目錄：列出韌體、在瀏覽器裡一鍵安裝（Chrome / Edge，Web MIDI）、連到模擬器試玩，並附救援教學。安裝不需要帳號。

以下數字來自它的 `catalog.json`，查核於 **2026-11-17**（目錄自己標的更新時間是 2026-11-16 18:01 UTC）。`[上游]`

## 有什麼

FM-1 共 **228** 個韌體：

| 類別 | 數量 |
|---|---|
| synth | 89 |
| groove（groovebox） | 76 |
| other（控制器、工具、遊戲等） | 26 |
| drums | 16 |
| tracker | 11 |
| chords | 8 |

安裝次數最多的幾個：SLOOP（約 11,500）、JIANT FM、Skiff、Felucca、Melodee、OberVave、FMini-1、Clipper。很多條目是這幾個主幹的衍生版。

除了 FM-1，Hub 也收 M-VAVE SMK-37、Elektron Model:Cycles、Erica Synths LXR-02 的韌體，並有工具區（模擬器、editor、備份與救援、自己 build）、Knowledge Base 與 creator 名單。

## 新晶片篩選：對我這台最重要的功能

Hub 會從每個套件裡讀出它認得哪些 flash 晶片，並提供篩選 **Saves on newer FM-1s**。我的機器是新晶片 `0xB36014`，裝東西前先看這一欄。

| Hub 的標示 | 意思 | 數量 |
|---|---|---|
| Every FM-1, the newer chip too | 套件認得新晶片 | 152 |
| The first FM-1s only | 只認得舊晶片：在新機上存檔只留在 RAM，更新也可能失敗 | 71 |
| Every FM-1: it does not check the chip | 不檢查晶片；Hub 註明他們沒在新晶片上試過 | 5 |

我在用或考慮過的：

| 韌體 | Hub 版本 | 新晶片 |
|---|---|---|
| SLOOP | 2.8.5 | 認得 |
| Felucca | 1.8.2 | 認得 |
| Melodee | 1.6.1 | 認得 |
| Clipper | 1.3.1 | 認得 |
| X0X | 1.2.2 | 認得 |
| CTL-1 | 1.0.3 | 認得 |
| AGTDECK（Agent Deck） | 0.6.3 | 認得（Hub 版本比上游 release 的 0.7 舊） |
| SLOOP DX7 Banks、SLOOP Floyd FM、X0X-RMX、Felucca [Salt] | — | **只認舊晶片**，不要裝 |

衍生版不一定跟上主幹：SLOOP 本身支援新晶片，但基於舊版 SLOOP 的幾個 fork 還沒有。

!!! warning "「認得晶片」不等於「測過」"
    這一欄是從套件內容判讀出來的，不是實機測試結果。Hub 自己聲明它不跑測試也不做 review，只檢查檔案是不是 FM-1 套件。還是要自己備份、刷完做存檔測試。

## 我怎麼用它

- **找東西、看新晶片標示**：用 Hub。
- **下載與安裝**：仍然從韌體作者自己的 GitHub release 下載、對 digest，再用 `just fm1-flash`。這樣版本最新（例如 Agent Deck），而且寫入前有 M-Flasher 的檢查與自動備份。
- **試玩**：Hub 的 Try 與 [fm1-emu](../agentic/emulator.md) 是同一個模擬器。
- **救援教學**：Hub 的 [FM-1 rescue](https://thefirmwarehub.com/tools/rescue/) 依「你看到什麼」分步驟寫，和[救援](../getting-started/recovery.md)互補。

Hub 上標「Built from source」的套件有 build attestation（由共用的 [zvenson/fm1-build](https://github.com/zvenson/fm1-build) 產生）。那代表檔案確實是從那個 commit build 出來的，不代表韌體安全。

## 官方網站

M-VAVE 官方的產品頁與下載在 [cuvave.com](https://www.cuvave.com/product?id=fm-1)（Cuvave 是 M-VAVE 的公司網站）。官方韌體 `FM-1.fwsc` 與更新工具 M-UPGRADE 從那裡的 Download 取得。頁面內容是 JavaScript 動態載入的，我沒有逐項核對上面目前列的版本。
