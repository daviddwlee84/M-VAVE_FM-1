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

## 和 remotefm1.com、fm1-emu 的關係

三個網站看起來功能重疊，其實是三層：

| 東西 | 角色 | 誰做的 |
|---|---|---|
| **The Firmware Hub** | **目錄**：有哪些韌體、套件在哪、認不認得新晶片 | zvenson / Designburg |
| **[fm1-emu](../agentic/emulator.md)** | **模擬器引擎**：模擬 FM-1 的晶片，原封不動地跑 `.fwsc` | joakimeriksson |
| **[remotefm1.com](https://remotefm1.com/)（FM-1 Player）** | **播放器**：用上面兩者，再加上自己的原生執行方式與「把實機當面板」 | Quixotic7 |

FM-1 Player 的頁面自己寫明：韌體清單來自 Firmware Hub 的即時目錄（我看到時列了 243 個）；帶圓點的韌體以 `.rfm1` 原生執行（韌體的原始碼編成 WebAssembly）；其餘每一個都能用 fm1-emu 模擬執行，套件從 Hub 指向的位置抓。`[上游]`

所以來源確實是 Hub，模擬的部分是 fm1-emu。Hub 自己的 Try、Agent Deck 的線上展示頁、我本機的 `just emu-run` 用的也都是 fm1-emu。差別在 FM-1 Player 多了原生核心（比較快、延遲低）以及搭配 [RemoteFM1](remotefm1/index.md) 把實機當面板。

## 官方網站

M-VAVE 官方的產品頁與下載在 [cuvave.com](https://www.cuvave.com/product?id=fm-1)（Cuvave 是 M-VAVE 的公司網站）。官方韌體 `FM-1.fwsc` 與更新工具 M-UPGRADE 從那裡的 Download 取得。頁面內容是 JavaScript 動態載入的，我沒有逐項核對上面目前列的版本。

## 自己的韌體怎麼上架 { #listing }

2026-11-18 為了 [Morse](../dev/morse.md) 查的。只讀，沒有送出任何東西。`[上游]`（來源：[submit](https://thefirmwarehub.com/submit/)、[KB](https://thefirmwarehub.com/kb/)、[fmhub.md](https://thefirmwarehub.com/fmhub.md)、`catalog.json`）

- **送審**：填 [submit](https://thefirmwarehub.com/submit/) 頁的表單，不用附檔案，Hub 會自己去讀 repo。每一筆都由 Sven 本人看過，先上 test site 再上正式站。舊文件還寫「寄 email」，表單看起來是比較新的路徑。上架後，Hub 會在三小時內自動抓那個 repo 最新一個附有 `.fwsc` 的 GitHub release；版本是 0.x 或帶 beta 的會標成 beta。
- **六個必要條件**：
  - 開源：source 在 tag 上可以瀏覽，repo 根目錄要有 `LICENSE`。
  - GitHub release 附上 `.fwsc`。
  - identity 要是自己的：送出前用 [#check](https://thefirmwarehub.com/submit/#check) 檢查。
  - 套件裡要有 update loader。
  - repo 根目錄要有 `fmhub.json`。
  - **在實機上測過**："'It builds' or 'it runs in the emulator' is not enough"。要測的有：從原廠 V15 安裝、從另一個韌體安裝、從自己的韌體再裝別的韌體、USB 回應自己的 `FM-1_` identity、刻意中斷一次寫入再繼續；如果會覆寫或讀不了別人的 project，要在 README 寫明。
- **`fmhub.json`**：只允許 9 個欄位（`what`、`author`、`kinds`、`play`、`editor`、`cheat`、`image`、`color`、`sounds`，外加 `smk37`），多一個欄位整個檔案就不採用。新韌體至少要有：
  - `what`：英文，20–300 字元，不能有連結。
  - `kinds`：synth / groove / drums / chords / tracker / other 擇一。
  - `image`：repo 裡的正方形圖，240–1024 px、≤ 300 KB。
- **晶片欄**（new / old / any）由 Hub 自己從套件讀出來，作者不用申報。同樣用 fm1-base 的 CTL-1 被標為 new。
- **收什麼**：KB 寫「It makes music… Games and tools without sound are not listed」，但 AGTDECK、MIDIMON 都以 `other` 上架了。
- **Hub 讀的是 `fmhub.json`**：fm1-base build 出來的 `firmware.json`，在 Hub 的文件裡完全沒提到。
- **一個 repo 一張卡**：圖片路徑、`fmhub.json` 都以 repo 根目錄為準，所以要上架的韌體應該放在獨立 repo（見[自製韌體環境](../dev/index.md)）。

### 社群清單 { #lists }

Hub 之外的韌體清單，2026-11-18 查的。stars 是當天 GitHub 上的數字，「外部 PR」是最近 15 個已關閉、不是維護者自己開的 PR 中被 merge 的比例。`[上游]`

| 清單 | 熱門程度 | 收錄方式與格式 | Morse |
|---|---|---|---|
| [aroum/fm1-custom-fw](https://github.com/aroum/fm1-custom-fw) | 424 stars；外部 PR 11/15 merge | 發 PR。沒有 CONTRIBUTING，README 是編號表格：`**名稱 by 作者**<br>[GitHub](…) \| …<br>**License:** …`，加上 Concept／Features 欄。fm1-base 做的工具（CTL-1、MIDIMON）放 1.1.4，遊戲和非音訊的放 1.3 | [PR #36](https://github.com/aroum/fm1-custom-fw/pull/36)：1.1.4 第 126 筆 |
| [cicloid/awesome-fm-1](https://github.com/cicloid/awesome-fm-1) | 93 stars；外部 PR 15/15 merge | 依 `CONTRIBUTING.md` 發 PR：`- [名稱](url) - 說明.`，一行、寫作者和授權、一個 PR 只加一筆。工具放 *Not a synth* | [PR #30](https://github.com/cicloid/awesome-fm-1/pull/30) |
| [fm1-editor.com](https://fm1-editor.com/firmware/)（benny-sparra/fm1-dx7-patch-importer） | 75 stars | 頁尾寫「缺了哪個韌體就開 issue」 | 等 Hub 上架後看會不會自己出現 |
| fm1-emu、RemoteFM1 的 FM-1 Player、[FMDB](https://stanleygurvich.github.io/FMDB_FM1_FirmwareDatabase/) | — | 自動跟著 Hub 的目錄，或自己爬 GitHub | 不用送 |
| FM-1 Firmware Atlas（wero1414/atlas-fm-1） | 0 stars | 發 PR 改 `data/firmwares.json` | 不送 |
| FM1 Developers' Wiki | 5 stars | 不是韌體清單，只收「帶有開發知識」的專案 | 不送 |

公告通常發在 r/MVaveFM1。這張表也寫在 repo 的 skill `fm1-hub-listing` 裡，兩邊要一起更新。

**實際送審的經驗**（Morse，2026-11-18）：

- **表單**：送審頁的表單（不是 email）。
  - 裝置選 M-VAVE FM-1 時，旁邊的「Which device?」留空，那是選 Another device 時才要填的。
  - 「Your name, as it should appear」最好和 `fmhub.json` 的 `author` 一致，卡片的「By:」用的是後者。
  - **粉紅虛線框是 VIBE Box Challenge 的參賽欄位**（「What it is, in one line」「Which VIBE features does it use?」和參賽勾選）。那個比賽只收用 VIBE FM SDK 做、而且用到 FM-1 沒有的功能（音訊輸入、大螢幕、pads、CV 等）的作品，規則見 [lab/rules](https://thefirmwarehub.com/lab/rules/)。FM-1 韌體不要勾。
  - 測試寫在「How you tested it」（從哪個韌體裝、換回哪個）；建置方式、授權、聲音的說明寫在「Anything Sven should know」。
- **送出後**：會立刻得到一個只有自己有網址的狀態頁，Hub 自動檢查 8 項：
  1. repo 裡的原始碼；
  2. `LICENSE`；
  3. `fmhub.json` 的欄位；
  4. sounds 的聲明；
  5. 附 `.fwsc` 的 release；
  6. 自己的 identity；
  7. update loader；
  8. flash 晶片。

  Morse 8 項全過，狀態是 **waiting for Sven**：等維護者本人審核。之後的變化會寄到填寫的 email。
- **進度**：Morse 照這份清單準備，2026-11-18 完成三項實機測試、發布 v0.3 release、通過 identity 和 `fmhub.json` 檢查，接著送審，也對 awesome-fm-1 和 aroum 各開了一個 PR。過程見 [Morse](../dev/morse.md#release)。這套流程整理成 repo 的 agent skill `fm1-hub-listing`（`.agents/skills/`）。

**注意同名**：Hub 上的 CONDUCTOR 是 MammothCheese 的 groovebox（`MammothCheese/conductor`，GPL-3.0，identity `FM-1_1812`），不是本 repo 的 [Conductor](../agentic/conductor.md)。Hub 和 fm1-emu 上的 id `conductor` 已經被它用了。
