# 用 FM-1 控制 agent

FM-1 和 agent 可以往兩個方向接：

1. **FM-1 控制 coding agent**：琴鍵、LED、螢幕變成 Claude Code、Codex 等 CLI agent 的實體控制台。
2. **agent 操作 FM-1 做音樂**：agent 透過 MIDI / SysEx 寫入音色、pattern、編曲。

## FM-1 Agent Deck

[FM-1 Agent Deck](https://github.com/kevinelliott/fm-1-agent-deck)（kevinelliott，GPL-3.0）是一套獨立韌體，把 FM-1 變成 coding agent 的桌上控制台。以下都來自它的 README `[上游]`。

**它是取代合成器的韌體，不是 SLOOP 上的 plugin。** 刷著 Agent Deck 時 FM-1 不發聲；刷回原廠或其他韌體才回到樂器。它不讀其他韌體的 projects，其他韌體的資料能不能撐過一趟 Agent Deck 沒有測過，所以先備份。

### 硬體怎麼對應

| FM-1 | Agent Deck |
|---|---|
| 27 個琴鍵 | 每鍵一個 agent slot（2 頁共 54 個），按下選取 |
| 琴鍵 LED | 狀態：off EMPTY、dim IDLE、solid WORKING、2 Hz 閃 BLOCKED（等你處理）、快閃 ERROR |
| 240×240 螢幕 | GRID（列表或方塊）、DETAIL（OUTPUT / INFO / DIFF）、APPROVALS、MENU、ABOUT |
| `REC` / `SAVE` | approve / approve 並不再詢問 |
| `FX` / `ENV` | deny（Esc）/ interrupt（Ctrl-C） |
| `PLAY` | continue；在空 slot 上開新 agent |
| `PRESETS` + `SEQ` | 選一個 prompt preset 送出；對 BLOCKED agent 會排入佇列 |
| `ARP` | 這個 agent 的 auto-approve 開關 |
| `K4` | 跳到下一個需要你處理的 agent |
| `GLO` 按住 + `REC` / `FX` … | 一次套用到所有適用的 agent |

裝置上不能打字：自由輸入的 prompt 還是要在 terminal 裡打，裝置只送 preset。

### 架構

```text
FM-1 (Agent Deck firmware)
   ⇅  USB-MIDI channel 16
host/agtdeck.py  (電腦上的 Python daemon)
   ⇅  Herdr socket API (newline JSON)   或   tmux CLI
Herdr  /  tmux  →  Claude Code、Codex、opencode ...
```

daemon 每秒輪詢 backend，只把變動送給 FM-1；FM-1 每秒回 heartbeat，螢幕 header 顯示 `HOST OK` 或 `NO HOST`。只支援 USB-MIDI，沒有 Bluetooth。

**Herdr 是比較好的 backend。** Herdr 原生回報每個 pane 的 agent 狀態（Idle / Working / Blocked / Done），tmux backend 則是看指令與畫面文字的 heuristic，會誤判。README 記錄 2026-11-08 對 Herdr 0.9.5 實測過 discovery、狀態、approve、deny、interrupt、prompt 與 `agent.start`；其他 Herdr 版本可能改了 socket method 名稱。我平常就用 Herdr，所以 Herdr 整合是這個專案對我最有意思的部分。

### 不刷機就能試

模擬模式（不需要 FM-1），指令照 README：

```sh
git clone https://github.com/kevinelliott/fm-1-agent-deck.git
cd fm-1-agent-deck

python3 host/agtdeck.py --backend auto --simulate
sh host/test_simulate.sh        # 開三個用完即丟的 tmux session 自我測試
```

只看 Herdr backend：`python3 host/agtdeck.py --backend herdr --simulate`（要有正在跑的 Herdr）。出問題時 `python3 host/agtdeck.py --doctor` 會逐項檢查。

真的刷了 Agent Deck 並接上 USB 之後的硬體模式：

```sh
pip install mido python-rtmidi
python3 host/agtdeck.py --backend auto
```

線上播放器：[在瀏覽器裡跑 Agent Deck](https://kevinelliott.github.io/fm-1-agent-deck/play/?fw=agtdeck&script=demo.txt)，是 fm1-emu 跑真正的韌體 package，按 **Start** 播一段腳本化的 session。

!!! note "0.7 改名"
    0.7 把專案的檔案與指令從 `agentdeck` 改成 `agtdeck`：package `agtdeck-0.7.fwsc`、daemon `host/agtdeck.py`、設定在 `~/.config/agtdeck`。網路上舊文章裡的 `agentdeck.py` 已經不存在。

### 狀態與風險

README 的 Status（2026-11）寫得很誠實：作者的 FM-1 實機跑的是 **0.6.2**，用 daemon 接真的 Herdr session；最新的 **0.7** 只差版本字串與 daemon 檔名，在 GitHub 上 build 並簽章，**只在 emulator 跑過，還沒上實機**。prompt queue、pipelines、auto-approve 規則多半是對 stub backend 測的，預期會有粗糙處。版本見[資源地圖](../resources.md)。

README 說它的 base（Dinghy 0.5）在程式碼裡同時接受舊 Puya（`0x856014`）和新 UCUN（`0xB36014`）的 JEDEC ID。但我**沒有找到在新款 UC25HQ80 晶片上實機跑 Agent Deck 的明確回報** `[調研]`。我的晶片見[我的機器](../device-log.md)，晶片差異見 [Flash 晶片](../hardware/flash-chips.md)。

!!! warning "auto-approve 保持關閉"
    `ARP` 開啟後 daemon 會自動 approve 這個 agent 的 prompt。它有 `auto_never` 清單（`rm -r`、`git push`、`sudo` 等），但 README 自己說這只是文字 pattern match，不是 sandbox，換個寫法的危險指令照樣通過。auto-approve 預設關閉、daemon 重啟後也會忘掉；我會一直讓它關著，避免誤觸按鍵就讓 agent 改檔案或執行指令。

## 不刷 Agent Deck 的替代方案

| 方案 | 怎麼做 | 取捨 |
|---|---|---|
| [Trireme](https://github.com/ignis32/trireme_fm1) | Vial 相容的 USB HID 鍵盤 / macro pad，按鍵送快捷鍵 | 不需要 host daemon；但沒有 agent 狀態回饋。作者標明是 vibe-coded、沒逐一手測 `[上游]` |
| [CTL-1](https://github.com/charlesvestal/fm1-ctl) + Python `mido` | FM-1 送 MIDI，自己寫 script 轉成任意 agent API 呼叫 | 最彈性，CTL-1 還有自己的 GM 音色；狀態回顯要自己做 `[調研]` |
| 保留音樂韌體，用它的 MIDI 輸出 | SLOOP 等韌體照常發聲，host daemon 接它的 MIDI | 不必換韌體；能拿到哪些按鍵、旋鈕、螢幕回饋，取決於該韌體的 MIDI protocol `[調研]` |

## 自己寫一個：Conductor

在 fm1-base 上重寫的版本：預設只讓 agent 佔琴鍵、hc 與 classic Herdr 兩個 backend、真正的 SysEx 協定、提示音。見[自己的 agent 控制台](conductor.md)。

那頁也記錄了另外兩條路的調研：

- **偽裝成 Codex Micro**（[scf4/codex-midi](https://github.com/scf4/codex-midi)）：要注入 ChatGPT.app、協定是逆向工程、狀態只以 RGB 顏色傳來，不適合單色 LED。見[為什麼不走 Codex Micro](conductor.md#codex-micro)。
- **hc（herdr-core）vs classic Herdr**：兩個獨立的 runtime；hc 的狀態由 agent 自己回報（OSC 7501）。見[對照表](conductor.md#backends)。

## agent 操作 FM-1 做音樂

SLOOP 的 [web editor](../firmware/sloop/web-editor.md) 透過 USB MIDI 和機器即時同步，protocol 記在 SLOOP repo 的 `web/EDITOR_PROTOCOL.md` `[上游]`。這是讓 agent 寫入音色與 pattern 最直接的介面，但目前沒有針對 2.8.x 的現成 agent 工具。

社群有兩個 editor skill，都是 MIT、中文 README `[上游]`：

| Skill | 目標韌體 | 平台 | 格式 |
|---|---|---|---|
| [fm1-sloop-editor-2026](https://github.com/hunterxiong2026/fm1-sloop-editor-2026) | SLOOP **2.4.1** | Windows（`ctypes` + `winmm.dll` 直送 SysEx） | DeepSeek Harness skill（`~/.dsh/skills/`） |
| [fm1-felucca-editor-2026](https://github.com/hunterxiong2026/fm1-felucca-editor-2026) | Felucca **v1.5** | 只支援 Windows | Claude skill（`SKILL.md`） |

兩者都綁定舊版韌體的 SysEx 參數表，也不能在 macOS 上直接跑。適合當「agent 透過 MIDI 控制 FM-1」的參考實作，**不要拿來對 SLOOP 2.8.x 或 SLOOP 3 寫入 project**。

## 不刷機的試用清單

- [ ] 在[線上播放器](https://kevinelliott.github.io/fm-1-agent-deck/play/?fw=agtdeck&script=demo.txt)看完 demo，翻一遍 GRID、DETAIL、APPROVALS
- [ ] clone repo，跑 `--simulate` 對自己的 Herdr session
- [ ] 跑 `sh host/test_simulate.sh`，看 tmux heuristic 的結果
- [ ] 用 `--doctor` 確認 Herdr socket 找得到
- [ ] 在本機 [fm1-emu](emulator.md) 開 `agtdeck-0.7.fwsc`
- [ ] 讀完 README 的 Arrival day 與 Risk notes，再決定要不要刷；刷之前照[注意事項](../getting-started/cautions.md)與 `AGENTS.md` 的 preflight
