# 資源地圖

FM-1 的開源 ecosystem 依用途分五層。每層一張表：連結、角色、一句評價。各韌體玩起來怎樣、怎麼選，見[韌體地圖](firmware/index.md)，這裡不重複。

## 「權威」在這裡的意思

依序是三種來源：

1. **原廠來源**：M-VAVE 的官方韌體與 SDK。
2. **工具作者的一手實作**：例如 M-Flasher 和 m-bootloader 的 README、release notes、原始碼。
3. **社群交叉查核過的文件**：例如 [FM1 Developers' Wiki](https://sbegueria.github.io/fm1-dev-wiki/)，每個事實都標 `[HW]`、`[SDK]`、`[RE]`、`[CODE]`、`[?]`。

GitHub stars 只代表關注度，**不是刷機安全性的證據**。

!!! warning "The Firmware Hub 不是安全認證"
    [The Firmware Hub](https://thefirmwarehub.com/) 的首頁寫明：它不跑測試也不做審查，只檢查檔案是不是 FM-1 package（2026-11-17 查核）。一鍵安裝很方便，但相容性與品質要自己看各專案的說明。

## A. 刷機、救援、模擬

| 專案 | 角色 | 評價 |
|---|---|---|
| [jvitkauskas/m-flasher](https://github.com/jvitkauskas/m-flasher) | 讀晶片、完整備份、刷機、還原，有 CLI | **必備**，實機操作的主工具。見 [M-Flasher CLI](agentic/m-flasher-cli.md) |
| [jvitkauskas/m-bootloader](https://github.com/jvitkauskas/m-bootloader) | 開機前檢查韌體、recovery menu、USB MIDI 安裝與備份 | SLOOP 3 需要它；新晶片上的實機驗證少，見[注意事項](getting-started/cautions.md) |
| [joakimeriksson/fm1-emu](https://github.com/joakimeriksson/fm1-emu) | 晶片層級 emulator，原封不動跑 `.fwsc`；有[線上版](https://joakimeriksson.github.io/fm1-emu/) | **必備**，試玩與自動化測試首選。見 [Emulator](agentic/emulator.md) |
| [ocean/FM1Kit](https://github.com/ocean/FM1Kit) | Swift toolkit（macOS / iOS） | 想寫 Mac 原生工具時參考；主要測試來自單台機器 `[調研]` |
| [jvitkauskas/m-apps](https://github.com/jvitkauskas/m-apps) | 從 RAM 執行的小程式（FM-1、SMK-37） | 研究用；`run-ram` 屬於 agent 不得主動執行的分級 |

## B. 文件、目錄、官方 SDK

| 資源 | 角色 | 評價 |
|---|---|---|
| [m-vave/fm1_open](https://github.com/m-vave/fm1_open) | M-VAVE 官方開放韌體與 binary SDK | 官方基準，GPLv3 + SDK linking exception |
| [FM1 Developers' Wiki](https://sbegueria.github.io/fm1-dev-wiki/)（[repo](https://github.com/sbegueria/fm1-dev-wiki)） | boot chain、memory map、recovery、update protocol | **技術問題優先查這裡**，有證據標記 |
| [aroum/fm1-custom-fw](https://github.com/aroum/fm1-custom-fw) | resource hub：韌體目錄、拆機、SysEx / OTA 逆向筆記 | 最大的社群入口；GitHub 上沒有宣告 license |
| [cicloid/awesome-fm-1](https://github.com/cicloid/awesome-fm-1) | awesome list（CC0） | 快速找新韌體與工具 |
| [The Firmware Hub](https://thefirmwarehub.com/) | 韌體目錄 + 瀏覽器一鍵安裝 | 方便試玩；只檢查封裝，見上方警告 |

分工：找新韌體看 The Firmware Hub 或 awesome-fm-1；查底層正確性看 Developers' Wiki；查逆向與工具看 aroum/fm1-custom-fw。

## C. 韌體 base

自己寫韌體時的起點；授權與定位比較見[韌體地圖](firmware/index.md)的「開發用的 base」一節。

| 專案 | 角色 | 評價 |
|---|---|---|
| [hugelton/felucca-bsp](https://github.com/hugelton/felucca-bsp) | BSP / HAL（MPL-2.0） | 想自己管上層架構時用 |
| [charlesvestal/fm1-base](https://github.com/charlesvestal/fm1-base) | felucca-bsp 之上的 platform、host simulator、browser emulator | 新控制器專案的首選；beta `[調研]` |
| [hugelton/Felucca](https://github.com/hugelton/Felucca) | 完整多 engine synth（GPL-3.0） | 多數社群韌體的源頭，研究架構用 |
| [zvenson/dinghy](https://github.com/zvenson/dinghy) | 最小可運作的韌體 template | 學 build、更新、硬體；Agent Deck 就是 fork 它 |
| [Keitark/fm1-nes](https://github.com/Keitark/fm1-nes) | JieLi SDK 原生開發範例（Apache-2.0） | 深入 SoC 底層時參考 |

## D. 硬體救援與擴充

| 專案 | 角色 | 評價 |
|---|---|---|
| [Quixotic7/MvaveFM1Unbricker](https://github.com/Quixotic7/MvaveFM1Unbricker) | 透過一般 USB 做完整備份、還原原廠 V15、救 soft brick | 軟體層救援的重要備案 `[上游]`（依 maxfridbe README 的描述） |
| [kurogedelic/FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter) | 用 RP2040 救 hard brick | 最後手段，需要動手接線。見[救援](getting-started/recovery.md) |
| [jvitkauskas/m-card](https://github.com/jvitkauskas/m-card) | microSD 擴充板，不用焊在機器上（CC0） | 硬體擴充，不是救援工具 |

晶片辨識與差異見 [Flash 晶片](hardware/flash-chips.md)。

## E. Agent 相關

| 專案 | 角色 | 評價 |
|---|---|---|
| [kevinelliott/fm-1-agent-deck](https://github.com/kevinelliott/fm-1-agent-deck) | coding agent 的實體控制台，整合 Herdr / tmux | 最值得研究；先用模擬模式。見[用 FM-1 控制 agent](agentic/agent-deck.md) |
| [ignis32/trireme_fm1](https://github.com/ignis32/trireme_fm1) | Vial 相容 USB HID macro keyboard | 不需要 daemon 的快捷鍵方案 |
| [charlesvestal/fm1-ctl](https://github.com/charlesvestal/fm1-ctl) | CTL-1 MIDI controller | MIDI → Python → 任意 API 的基礎 |
| [smememe/fm1-macropad-patch](https://github.com/smememe/fm1-macropad-patch) | 把原廠 V15 patch 成 MIDI macro pad | host 端自訂按鍵行為 |
| [maxfridbe/mvave-fm1](https://github.com/maxfridbe/mvave-fm1) | 兩個 Claude Code skills + podman build | 知識參考，部分過時。見 [Agent Skills](agentic/skills.md) |
| [kmplngj/Felucca](https://github.com/kmplngj/Felucca) | 由 agent 大量參與維護的 Felucca fork | agentic 開發流程的範本 |
| [hunterxiong2026/fm1-sloop-editor-2026](https://github.com/hunterxiong2026/fm1-sloop-editor-2026) | 自然語言編曲，SysEx 寫入 SLOOP 2.4.1 | 只限 Windows、舊版 SLOOP |
| [hunterxiong2026/fm1-felucca-editor-2026](https://github.com/hunterxiong2026/fm1-felucca-editor-2026) | 自然語言編輯 Felucca 1.5 音色與編曲 | 只限 Windows、舊版 Felucca |

## 版本表

**查核日期：2026-11-17**（GitHub API）。這是 repo 裡唯一的版本表，其他頁面連到這裡。

| 專案 | 最新版本 | 發布日 | 備註 |
|---|---|---|---|
| [m-flasher](https://github.com/jvitkauskas/m-flasher/releases) | v0.12.1 | 2026-11-15 | |
| [m-bootloader](https://github.com/jvitkauskas/m-bootloader/releases) | v2.4 | 2026-11-13 | |
| [sloop-fm1](https://github.com/isod89/sloop-fm1/releases) | v2.8.5 | 2026-11-04 | SLOOP 2.8，支援 UC25HQ80 |
| [sloop3-fm1](https://github.com/isod89/sloop3-fm1/releases) | v3.3.7-beta | 2026-11-16 | prerelease |
| [fm1-emu](https://github.com/joakimeriksson/fm1-emu/releases) | v0.5.1 | 2026-11-14 | 本機用的版本，見[模擬器](agentic/emulator.md#local) |
| [fm-1-agent-deck](https://github.com/kevinelliott/fm-1-agent-deck) | v0.7 | 2026-11-14 | 作者實機跑的是 0.6.2 |

一次列出全部：`just fm1-versions`。重新查某個 repo 的最新 release：

```sh
gh api repos/<owner>/<repo>/releases --jq '.[0].tag_name'
```

這會回傳最新一筆 release，**包含 prerelease**（例如 SLOOP 3 的 beta）。改了版本要同時更新上面的查核日期。
