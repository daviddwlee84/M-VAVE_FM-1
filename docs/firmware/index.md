# 韌體地圖

FM-1 同一時間只跑一套主要韌體，換韌體就是重刷。可以先在 [fm1-emu 線上模擬器](https://joakimeriksson.github.io/fm1-emu/)試玩，再決定哪些值得刷進真機。

這是依用途整理的個人選擇，不是 GitHub stars 排名。版本與查核日期見[資源地圖](../resources.md)。部分專案還在早期開發或只有有限的實機驗證，不代表每個都適合刷進日常使用的機器。

## 音樂類

| 韌體 | 變成什麼 | 備註 |
|---|---|---|
| [SLOOP 2.8](https://github.com/isod89/sloop-fm1) | 四軌 groovebox：3 synth + 1 drum，12 種 engine | **首選**，支援新晶片。見 [SLOOP](sloop/index.md) |
| [SLOOP 3](https://github.com/isod89/sloop3-fm1) | 重寫的新一代 groovebox：4 synth + 1 drum | public beta，必須先裝 m-bootloader。見 [SLOOP 3](sloop/sloop3.md) |
| [Melodee](https://github.com/keremimo/melodee) | 多 engine synth + sequencer，和弦與 song mode | 有 Bluetooth MIDI `[調研]` |
| [Clipper](https://github.com/isod89/clipper-fm1) | 偏 sound design 的實驗性合成器，源自 SLOOP | 有 web editor `[調研]` |
| [X0X](https://github.com/charlesvestal/fm1-x0x) | 909、808、兩台 303 的 groovebox | techno / acid |
| [zp12](https://github.com/zvenson/zp12) | SP-12 風格的 lo-fi sampling 鼓機 | hip-hop |
| [Felucca](https://github.com/hugelton/Felucca) | 多 engine 合成器 | 多數社群韌體的源頭與硬體層 |
| [FM-1 SID](https://github.com/joakimeriksson/fm1-sid) | Commodore 64 SID 合成器 | 跑在 fm1-dock 上 |

## 控制器與工具類

| 韌體 | 變成什麼 | 用途 |
|---|---|---|
| [CTL-1](https://github.com/charlesvestal/fm1-ctl) | MIDI controller，自帶 General MIDI 音色 | 控制 DAW、外部合成器；也適合 FM-1 MIDI → Python → 任意 API |
| [Trireme](https://github.com/ignis32/trireme_fm1) | Vial 相容的 USB HID macro keyboard | 快捷鍵、桌面自動化，不需要 host daemon |
| [RemoteFM1](https://github.com/Quixotic7/RemoteFM1) | 把 FM-1 變成面板，韌體在電腦、手機或瀏覽器上跑 | 不重刷就換韌體；見[搭配手機與平板](../hardware/mobile.md) |
| [Macropad Patch](https://github.com/smememe/fm1-macropad-patch) | 把原廠 V15 patch 成 MIDI macro pad | host 端自訂所有按鍵行為 |
| [MIDI Processor](https://midihax.github.io/fm1-midi-processor/html/index.html) | MIDI filter、remap、monitor | MIDI routing、debug `[調研]` |

## Agent 類

| 韌體 | 變成什麼 |
|---|---|
| [FM-1 Agent Deck](https://github.com/kevinelliott/fm-1-agent-deck) | Claude Code、Codex 等 CLI agent 的實體控制台，整合 Herdr 與 tmux |

細節見[用 FM-1 控制 agent](../agentic/agent-deck.md)。

## 開發用的 base

想自己寫韌體時的起點。這個 repo 目前還沒有建置環境。

| 專案 | 定位 | License |
|---|---|---|
| [felucca-bsp](https://github.com/hugelton/felucca-bsp) | 基礎 BSP / HAL | MPL-2.0 |
| [fm1-base](https://github.com/charlesvestal/fm1-base) | BSP 之上的 platform、host simulator 與 template | MPL-2.0 |
| [dinghy](https://github.com/zvenson/dinghy) | 最小的可運作韌體 template | GPL-3.0 |
| [fm1_open](https://github.com/m-vave/fm1_open) | M-VAVE 官方開放韌體與 binary SDK | GPLv3 + SDK linking exception |
| [fm1-nes](https://github.com/Keitark/fm1-nes) | JieLi SDK 原生開發範例 | Apache-2.0 |

控制器類的新專案，`fm1-base` 比直接 fork 一整套 GPL 的 Felucca / SLOOP 合適：它已處理螢幕、按鍵、MIDI、storage，而且 MPL-2.0 讓自己的應用程式可以選別的 license。它還是 beta，API 可能變動。`[調研]`

## 怎麼選

- 只想做音樂：SLOOP 2.8。
- 想接 DAW 當鍵盤 / 控制器：CTL-1。
- 想要 Bluetooth MIDI：留在原廠，或 CTL-1、Melodee。SLOOP 不支援。
- 想研究 agent 控制台：先在模擬器玩 Agent Deck，不用刷。
- 換韌體前先看[多韌體切換](switching.md)與[注意事項](../getting-started/cautions.md)。
