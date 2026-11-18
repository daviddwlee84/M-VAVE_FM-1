# M-VAVE FM-1 筆記

這裡記錄我的 M-VAVE FM-1：刷機與救援、社群韌體（先從 SLOOP 開始）、使用心得，以及讓 coding agent 能安全協助操作這台機器所需的 context。

!!! warning "非官方筆記"
    內容與 M-VAVE 及任何韌體作者無關。刷自訂韌體有風險，這只是個人整理，不是保證。動手前請先讀[注意事項](getting-started/cautions.md)。

## 從哪裡開始

| 我想… | 看這頁 |
|---|---|
| 第一次拿到機器，照順序做 | [Getting Started](getting-started/index.md) |
| 知道哪些事絕對不要做 | [注意事項](getting-started/cautions.md) |
| 機器開不了 | [救援](getting-started/recovery.md)、[變磚的原理與分級](getting-started/bricking.md) |
| 知道換韌體會蓋掉什麼 | [Flash 配置圖](hardware/flash-map.md) |
| 不刷機先試玩 | [線上試玩](playground.md) |
| 選一套韌體來玩 | [韌體地圖](firmware/index.md)、[The Firmware Hub](firmware/hub.md) |
| 學 SLOOP 怎麼操作 | [SLOOP](firmware/sloop/index.md) |
| 讓 agent 幫忙刷機或開發 | [Agentic 工作流](agentic/index.md) |
| 自己寫韌體 | [自製韌體環境](dev/index.md) |
| 用 FM-1 管 coding agent（自製） | [Conductor](agentic/conductor.md)、[操作速查](agentic/conductor-cheatsheet.md) |
| 用 FM-1 學摩斯電碼、跟人互傳（自製） | [用 MORSE 學摩斯](dev/morse-learn.md)、[摩斯電碼機](dev/morse.md)、[操作速查](dev/morse-cheatsheet.md)、[兩台互連](hardware/io.md#two-units) |
| 用 FM-1 學樂理：五度圈、和弦、級數（自製） | [樂理練習機](dev/theory.md) |
| 試 Agent Deck | [Agent Deck 試用](firmware/agent-deck/index.md) |
| 搞懂官方、社群、Hub、VIBE 的關係 | [生態是怎麼來的](ecosystem.md) |
| 找某個 repo 或目前版本 | [資源地圖](resources.md) |
| 查我這台現在的狀態 | [我的機器](device-log.md) |
| 看同晶片的機器、VIBE FM、救援硬體 | [相關硬體](hardware/related.md) |
| 接 Android 或 iPhone | [搭配手機與平板](hardware/mobile.md)、[iOS 音樂 app](hardware/ios-apps.md) |
| 找官方的韌體與工具 | [官方資源](official.md) |

## 路線圖

| 階段 | 配置 | 狀態 |
|---|---|---|
| 1 | 原廠韌體 + M-Flasher 完整備份 | 完成（2026-11-17），見[我的機器](device-log.md) |
| 2 | SLOOP 2.8.5 + Web Editor，正常玩音樂 | SLOOP 已刷入，開始玩 |
| 3 | fm1-emu + Agent Deck host 模擬（不刷機） | fm1-emu 已在本機可用，見[模擬器](agentic/emulator.md#local) |
| 4 | m-bootloader（先確認晶片相容與復原方案） | 暫緩，只追蹤，見[上游追蹤](watch.md) |
| 5 | 輪流刷不同韌體 | 已試過 Agent Deck 0.7、CTL-1 1.0.3、ChoralRoot 0.41，用 `just fm1-switch` 切換，見[多韌體切換](firmware/switching.md) |
| 6 | m-apps + m-card 多系統切換（需硬體改裝） | 觀望 |

階段 1、2 做完就可以正常玩，後面都是選配。SLOOP 3 必須透過 m-bootloader 的 MIDI MODE 安裝，所以階段 4 是階段 5 的前置條件。

## 自製韌體 + host

在 [fm1-base](dev/index.md) 上寫的。Conductor 和共用模組是 MIT；Morse 與 THEORY 的韌體是 GPL-3.0-or-later（Morse 的電腦端仍是 MIT）。每個專案是一套 FM-1 韌體，需要的話再加一個電腦端程式。

| 專案 | 是什麼 | 韌體 | 電腦端 | 狀態 |
|---|---|---|---|---|
| **Conductor** | coding agent 的桌上控制台：一個琴鍵一個 agent，LED 表示急不急，螢幕顯示狀態與輸出（含中文），提示音，按鈕批准 / 拒絕 / 中斷，按住說話的語音輸入，USB 或藍牙 | `firmware/conductor/` | `host/conductor/`（接 hc 與 classic Herdr 的 Python daemon） | 韌體 0.3、host 0.3.2；2026-11-17 在我的機器上用過，目前機器上換成 Morse（換回來：`just fm1-switch firmware/conductor/build/conductor.fwsc`），見[設計](agentic/conductor.md)、[速查](agentic/conductor-cheatsheet.md)、[版本紀錄](agentic/conductor-changelog.md) |
| **Morse** | 隨身摩斯電碼機：琴鍵當電鍵（直鍵或 iambic paddle）即時解碼、練長短的 RHYTHM 頁、琴鍵當字母鍵盤和片語鍵、閃卡間隔複習（預設只用聽的，打碼卡看自己的節奏判斷）、分級測驗與手動調整進度、進度備份還原、連續收聽、靜音看燈、USB 或藍牙 MIDI 用 Vail 的格式 | `firmware/morse/` | `host/morse/`（把發的字打進電腦、電腦送字給 FM-1 播） | 韌體 0.8、host 0.8.0（`SAVE` 立刻存）；2026-11-17 到 11-18 在我的機器上，目前換成 THEORY（學習紀錄留在 `0xDA000`），見[怎麼用它學](dev/morse-learn.md)、[設計](dev/morse.md)、[速查](dev/morse-cheatsheet.md) |
| **THEORY** | 隨身樂理練習機。v0.1 是 WHEEL：五度圈上標出調內和弦；彈和弦就說出它的名稱和在調裡的 Roman / Nashville 級數（副屬、借用、轉位）；或讓白鍵彈調內和弦，自動就近轉位；九種調式，每個調都拼對音名。0.2 加上每天一回合的閃卡（音階、調號、級數和弦、聽音級數與和弦性質）、說出你彈什麼的 CHORD 頁、五度圈熟練度圖、進度備份 | `firmware/theory/` | `host/theory/`（進度備份與還原，另有[網頁](assets/theory/progress.html)） | 韌體 0.2，2026-11-18 起在我的機器上（Morse 的進度保留，可以換回去），見[設計](dev/theory.md) |
| `hello` | fm1-base 的 template 原樣複本，用來驗證工具鏈 | `firmware/hello/` | — | 模擬器開得起來 |
| 共用模組 | `hostlink`（經 USB / 藍牙和電腦的 SysEx 連線）、`ledfx`（LED 動態）、`chime`（FM 提示音） | `firmware/common/` | — | Conductor 在用；Morse 用 `hostlink`、`ledfx` |

## 這台機器是什麼

- 口袋大小的 6-operator FM 合成器，SoC 是 JieLi AC791N，內建 1 MiB SPI NOR flash。
- 27 個發光琴鍵、240×240 彩色螢幕、7 個 encoder 加一個 master 旋鈕、一排功能鍵、喇叭與電池。
- USB-C（MIDI、依韌體支援 audio）、3.5 mm TRS MIDI IN、3.5 mm 音訊輸出、Bluetooth MIDI（依韌體）。細節見[接口與連線](hardware/io.md)。
- 有活躍的開源韌體社群，可以變成 groovebox、鼓機、sampler、MIDI controller，甚至 coding agent 的實體控制台。

## 證據標記

這個生態每週都在變，所以頁面上會標出資訊的可信程度：

- `[實機]`：在我自己的機器上看到的，日期記在[我的機器](device-log.md)。
- `[上游]`：上游 repo 或文件寫的，附連結。
- `[模擬]`：在 fm1-emu 裡看到的，不等於實機。
- `[調研]`：來自調研筆記，還沒對過上游。

沒標的段落預設是 `[上游]`，查核日期見[資源地圖](resources.md)。
