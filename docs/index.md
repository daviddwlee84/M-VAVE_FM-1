# M-VAVE FM-1 筆記

這裡記錄我的 M-VAVE FM-1：刷機與救援、社群韌體（先從 SLOOP 開始）、使用心得，以及讓 coding agent 能安全協助操作這台機器所需的 context。

!!! warning "非官方筆記"
    內容與 M-VAVE 及任何韌體作者無關。刷自訂韌體有風險，這只是個人整理，不是保證。動手前請先讀[注意事項](getting-started/cautions.md)。

## 從哪裡開始

| 我想… | 看這頁 |
|---|---|
| 第一次拿到機器，照順序做 | [Getting Started](getting-started/index.md) |
| 知道哪些事絕對不要做 | [注意事項](getting-started/cautions.md) |
| 機器開不了 | [救援](getting-started/recovery.md) |
| 選一套韌體來玩 | [韌體地圖](firmware/index.md)、[The Firmware Hub](firmware/hub.md) |
| 學 SLOOP 怎麼操作 | [SLOOP](firmware/sloop/index.md) |
| 讓 agent 幫忙刷機或開發 | [Agentic 工作流](agentic/index.md) |
| 自己寫韌體 | [自製韌體環境](dev/index.md) |
| 試 Agent Deck | [Agent Deck 試用](firmware/agent-deck/index.md) |
| 搞懂官方、社群、Hub、VIBE 的關係 | [生態是怎麼來的](ecosystem.md) |
| 找某個 repo 或目前版本 | [資源地圖](resources.md) |
| 查我這台現在的狀態 | [我的機器](device-log.md) |

## 路線圖

| 階段 | 配置 | 狀態 |
|---|---|---|
| 1 | 原廠韌體 + M-Flasher 完整備份 | 完成（2026-11-17），見[我的機器](device-log.md) |
| 2 | SLOOP 2.8.5 + Web Editor，正常玩音樂 | SLOOP 已刷入，開始玩 |
| 3 | fm1-emu + Agent Deck host 模擬（不刷機） | fm1-emu 已在本機可用，見[模擬器](agentic/emulator.md#local) |
| 4 | m-bootloader（先確認晶片相容與復原方案） | 暫緩，只追蹤，見[上游追蹤](watch.md) |
| 5 | SLOOP 3 / Agent Deck / Melodee 輪流刷入 | Agent Deck 0.7 已刷入（2026-11-17），見[操作速查](firmware/agent-deck/usage.md) |
| 6 | m-apps + m-card 多系統切換（需硬體改裝） | 觀望 |

階段 1、2 做完就可以正常玩，後面都是選配。SLOOP 3 必須透過 m-bootloader 的 MIDI MODE 安裝，所以階段 4 是階段 5 的前置條件。

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
