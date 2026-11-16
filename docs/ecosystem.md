# 生態是怎麼來的

FM-1 的社群韌體看起來很多，但整個生態非常新。這頁整理它的來歷、官方與社群各自做了什麼、以及幾個容易混淆的東西彼此的關係。日期是各 GitHub repo 的建立日期（2026-11-17 用 GitHub API 查），可以當成時間順序的參考，不等於專案真正開始的日子。`[上游]`

## 時間線

| 日期 | 事件 | 誰 |
|---|---|---|
| 2026-07-19 | [FM-1-RE](https://github.com/AL-255/FM-1-RE)：逆向工程原廠韌體與協定 | 社群 |
| 2026-07-25 | [fm1-custom-fw](https://github.com/aroum/fm1-custom-fw)：資源目錄、SysEx 協定文件 | 社群 |
| 2026-10-02 | [Felucca](https://github.com/hugelton/Felucca)：開源的多 engine 合成器韌體與硬體層 | Hügelton Instruments |
| 2026-10-04 | [SLOOP](https://github.com/isod89/sloop-fm1) | isod89 |
| 2026-10-07 | [MvaveFM1Unbricker](https://github.com/Quixotic7/MvaveFM1Unbricker)：備份與救援 | 社群 |
| 2026-10-09 | [Dinghy](https://github.com/zvenson/dinghy)：最小的韌體 template | zvenson |
| 2026-10-12 | [fm1-emu](https://github.com/joakimeriksson/fm1-emu)：晶片層級模擬器 | 社群 |
| **2026-10-13** | **[fm1_open](https://github.com/m-vave/fm1_open)：M-VAVE 官方開放韌體與 SDK** | **官方** |
| 2026-10-17 | [M-Flasher](https://github.com/jvitkauskas/m-flasher)、[fm1-base](https://github.com/charlesvestal/fm1-base) | 社群 |
| 2026-11-07 | [VIBE FM Link](https://github.com/zvenson/vibe-link) | zvenson |

也就是說：自製韌體的爆發集中在 2026 年 10 月之後，到我拿到機器時只有六週左右。Firmware Hub 上的 228 個韌體大多是這段時間出現的。

## 是官方開放的，還是使用者逆向的

**兩者都有，而且是社群先。**

- **先是逆向。** 7 月就有人逆向原廠韌體與更新協定。10 月初的 Felucca 提供了第一個完整的開源硬體層（現在獨立成 [felucca-bsp](https://github.com/hugelton/felucca-bsp)），後來大多數韌體都是從它或它的衍生物（Dinghy、SLOOP）長出來的。Felucca 的 README 明說它與 M-VAVE 無關、未經其背書。
- **官方隨後跟進。** M-VAVE 在 10 月中發布 `fm1_open`：一份 GPLv3（附 SDK linking exception）的開放韌體「FM1 Open V14」，含 binary SDK、8 軌、8 個 sample slot、16 個效果示範，以及一個網頁版的 [Workbench](https://open.m-vave.com/fm1-workbench)（更新韌體、預覽、管理 project 與 sample）。它的 build 需要 Windows 與 JieLi 的 Windows toolchain。
- **晶片本身有公開資料。** SoC 是 JieLi AC791N，JieLi 有公開的 AC79 SDK 與 toolchain。社群韌體 build 時用的就是 JieLi 的 clang，加上從 SDK 取用的幾個檔案。這是逆向能進展這麼快的原因之一。

所以 M-VAVE 不是「一開始就設計成開放平台」，而是在社群已經做出成果之後，選擇開放自己的韌體原始碼與 SDK，而不是封鎖。

## 為什麼這麼快

- **門檻被幾個基礎專案壓低了。** Felucca / felucca-bsp 解決硬體層，Dinghy 與 fm1-base 提供可以直接複製的 template，fm1-emu 讓人不用實機就能測，Web MIDI installer 讓安裝變成按一個按鈕，Firmware Hub 提供目錄與散布。
- **大量 fork。** 很多韌體是 Felucca 或 SLOOP 的衍生版，改 engine、改介面、加音色。這也是為什麼「主幹支援新晶片、衍生版不支援」的情況很常見。
- **AI 輔助寫程式。** Firmware Hub 自己的說法是「now anyone can vibe code firmware」。不少專案的 README 直接寫明是用 AI 協作完成的，有些（例如 Trireme）也坦白沒有逐項手測。這解釋了數量，也是品質參差的原因。
- **機器便宜、硬體不差。** 240×240 螢幕、27 個帶 LED 的鍵、7 個 encoder、雙核心、USB / BLE / TRS MIDI，適合當通用的小型開發板。

## SLOOP 是移植來的嗎

不是從別的平台移植的現成產品，是為 FM-1 寫的。但它不是從零開始：

- **建立在 Felucca 之上。** SLOOP 的 credits 第一條就是 Felucca：engine、sequencer、editor、installer 都源自它，之後也持續把 Felucca 新版的功能帶進來。
- **聲音引擎借用既有的開源 DSP。** FM6 是 Dexed 的 msfa（Google 與 Pascal Gauthier，Apache-2.0）；PHASE 來自 CrispyZebra；VOICE 來自 klattsch；取樣來自 CC0 的 Versilian Studios 與 Sonic Pi。
- **介面概念**參考 teenage engineering 的 pocket operator 與 EP-133、Elektron 的 step entry、Akai MPC 的 swing 與 note repeat。
- **社群回饋。** 每一版的更新很多來自使用者在 issue 裡提的需求，以及其他韌體（X0X、Melodee、JIANT）的作法。

這個生態的常態就是互相借用：GPL-3.0 讓大家能直接拿彼此的程式碼，credits 裡互相致謝。

## 官方韌體與 M-UPGRADE：我用到了嗎

**沒有。** 我的機器出廠是 015，我沒有用過 M-UPGRADE，也沒有裝過官方的更新。

| 東西 | 是什麼 | 我的狀況 |
|---|---|---|
| 原廠韌體 015 | 出廠時機器上的韌體 | 只存在於原廠備份裡 |
| 原廠韌體 016 / `FM-1.fwsc` | 官方較新的韌體（8-part multitimbral 等） | 沒裝。注意 016 會覆寫 preset 區，見[注意事項](getting-started/cautions.md) |
| M-UPGRADE | 官方的更新工具（PC / Mac） | 沒用過。M-Flasher 可以直接安裝官方的 `.fwsc` |
| FM1 Open V14 | 官方的**開放原始碼**韌體，和上面的原廠韌體是不同的東西 | 沒裝 |
| Workbench | 官方網頁工具：更新、模擬預覽、管理 project | 沒用過 |

要回到原廠，我會還原自己的 015 備份，而不是用 M-UPGRADE 裝 016。

## VIBE FM Mk1 是什麼

**不是 FM-1，也不是 FM-1 的模擬器。** 它是 Firmware Hub 那群人（zvenson / designburg）正在籌備的**另一台硬體**：一台「官方支援開放韌體」的機器，目前還在預告與登記名單階段（網站寫 crowdfunding、join the list）。`[上游]`，我沒有找到出貨日期或規格。

網頁上那個畫面是 **VIBE FM Web Player**：在瀏覽器裡的 VIBE FM。它長得和 FM-1 不一樣，是因為它畫的是 VIBE 的面板（16 個 pad、較長的鍵盤、DATA 旋鈕、transport 鍵）。它能跑「轉換過的」FM-1 韌體，例如截圖裡的 JIANT FM 3.0；同樣的東西也有 VST 版。

和 [fm1-emu](agentic/emulator.md) 的差別：

| | fm1-emu | VIBE FM Web Player |
|---|---|---|
| 模擬什麼 | FM-1 的晶片與面板 | VIBE FM 這台（尚未上市的）機器 |
| 跑什麼 | 原封不動的 FM-1 `.fwsc` | 為 VIBE 轉換過的韌體 |
| 用途 | 刷機前驗證、開發測試 | 試玩、在 DAW 裡當樂器 |

**VIBE FM Link** 則是一個刷在 FM-1 上的韌體，把 FM-1 變成 VIBE（Web Player、VST 或未來的實機）的**控制器**：琴鍵彈音、七個 encoder 對應 VIBE 的 encoder、每個按鍵對應 VIBE 的按鍵，FM-1 的螢幕還會顯示 VIBE 那邊韌體的畫面。裝了它之後 FM-1 自己不發聲。它也只是送標準 MIDI CC，所以任何 DAW 都能 learn。

所以方向是「FM-1 去控制 VIBE」，不是「別的 controller 控制 FM-1」。Web Player 本身也接受其他 MIDI controller。

對我來說目前沒有用處：VIBE 硬體還不存在，而 Web Player 用滑鼠或任何 MIDI 鍵盤就能玩。
