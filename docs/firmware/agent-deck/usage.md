# Agent Deck 操作速查

整理自上游 README 的 Controls reference，對應韌體 0.7。`[上游]`，除非另外標示。我自己的試用與刷機紀錄在[概觀](index.md)。

## 啟動

```sh
just agtdeck-run        # FM-1 已刷 Agent Deck 並接上 USB；預設用 Herdr backend
just agtdeck-doctor     # 連不上時先跑這個：檢查 MIDI 埠、設定、agent CLI、Herdr
```

螢幕右上角從 `NO HOST` 變成 `HOST` 就是連上了。daemon 停掉或 USB 拔掉會回到 `NO HOST`，重新接上會自動重新同步。

macOS 上這台裝置的 MIDI 名稱可能仍顯示成 `Felucca`（系統快取了之前 SLOOP 的名稱，USB 產品名其實是 `FM-1 AGTDECK`）。daemon 認得，不影響使用。`[實機]`

## 琴鍵與 LED

27 個琴鍵各是目前這一頁的一個 agent slot，按下去就選取那個 agent。LED 表示狀態：

| LED | 狀態 | 意思 |
|---|---|---|
| 不亮 | EMPTY | 這個 slot 沒有 agent |
| 微亮 | IDLE | 在等輸入 |
| 恆亮 | WORKING | 忙碌中 |
| 閃爍 2 Hz | BLOCKED | **需要你**：等批准或有問題要回答 |
| 快閃約 6 Hz | ERROR | agent 或 backend 回報錯誤 |
| 恆亮 2 秒後轉微亮 | DONE | 剛完成一輪 |
| 慢速脈動 1 Hz | UNKNOWN | 有 agent 但狀態無法分類 |

功能鍵的燈也會反映選取中的 agent：WORKING 時 `PLAY` 亮，BLOCKED 時 `REC` 閃，開了 auto-approve 時 `ARP` 亮。

`OCT-` / `OCT+` 換 slot 頁，兩頁共 54 個 slot。

## 最常用的幾個動作

| 想做的事 | 按 |
|---|---|
| 選一個 agent | 它的琴鍵，或轉 **SELECT** |
| 跳到下一個需要我的 agent | 轉 **KNOB 4** |
| 批准目前的提示 | **`REC`** |
| 批准並且不再問 | **`SAVE`** |
| 拒絕 | **`FX`**（送 Esc） |
| 中斷 | **`ENV`**（送 Ctrl-C） |
| 按 Enter | **`SEL`** |
| 在 agent 的選單裡移動 | **KNOB 2**（上下）、**KNOB 3**（左右），再按 `SEL` |
| 看這個 agent 的輸出 | **`EDIT`**（同時在電腦上聚焦那個 pane）；再按切換 OUTPUT / INFO / DIFF |
| 回列表 | **`HOME`**；在列表上再按一次切換「列表 / 方塊」 |
| 看所有待批准的 | **`GLO`** |

## 全部控制項

動作由電腦上的 daemon 對**選取中的** agent 執行。

| 控制 | 動作 |
|---|---|
| 琴鍵 | 選 slot（空的也可以選） |
| **SELECT** | 在 agent 之間移動選取 |
| **`PLAY`** | 開始 / 繼續；在純 shell 上：在裡面啟動預設 agent；在**空 slot** 上：開一個新 agent |
| **`REC`** | 批准 |
| **`SAVE`** | 批准且不再問（畫面上有那個選項時） |
| **`FX`** | 拒絕（Esc） |
| **`ENV`** | 中斷（Ctrl-C） |
| **`SEL`** | Enter |
| **KNOB 2** / **KNOB 3** | 上下 / 左右方向鍵 |
| **`LFO`** | 切換 agent 的 mode（Shift+Tab） |
| **PRESETS** | 選一個預設 prompt（顯示在底部那一行） |
| **`SEQ`** | 送出選好的預設 prompt。對 BLOCKED 的 agent 會先排隊，等它有空再送 |
| **`ARP`** | 切換這個 agent 的 auto-approve。預設關閉，daemon 重啟後不保留 |
| **KNOB 1** | 往回捲 DETAIL 的輸出 |
| **KNOB 4** | 跳到下一個需要你的 agent（BLOCKED → ERROR → DONE） |
| **`EDIT`** | DETAIL 頁 |
| **`HOME`** / **`GLO`** | GRID 頁 / APPROVALS 頁 |
| **ALGORITHM** | 輪流切換 GRID → DETAIL → APPROVALS → MENU → ABOUT |
| **`OCT-` / `OCT+`** | 上 / 下一頁 slot |
| **MASTER** | 提示音音量 |

## 一次處理多個

**按住 `GLO` 再按** `REC` / `FX` / `ENV` / `SEQ` / `ARP`：對所有適用的 agent 做同一件事。批准或拒絕全部 BLOCKED、中斷全部 WORKING、把預設 prompt 送給全部 IDLE / DONE、關掉所有 auto-approve。

**按住功能鍵再點琴鍵**（游標不動，按住時相關的鍵會亮起來提示）：

| 組合 | 動作 |
|---|---|
| `REC` + 鍵 | 批准那個 agent |
| `FX` + 鍵 | 拒絕那個 agent |
| `ENV` + 鍵 | 中斷那個 agent |
| `SEL` + 鍵 N | 回答第 N 個選項（輸入數字 1–9） |
| `SEQ` + 鍵 N | 把第 N 個預設 prompt 送給選取中的 agent |
| `PLAY` + 鍵 | 開始 / 繼續那個 agent；空鍵則在那裡開新 agent |
| `SAVE` + 鍵 | 把選取中的 agent 搬到那個鍵（已有則交換） |
| `LFO` + 鍵 | 標記 / 取消標記。有標記時，`GLO` + 動作只作用在標記的那幾個 |
| `ARP` + 鍵 | 切換那個 agent 的 auto-approve |
| `SEQ` 按住約 1 秒 | repeat mode：agent 每完成一輪就再送一次選好的 prompt |

`SAVE` + 鍵可以用來整理版面：把真正的 agent 搬到前面，shell 留在後面。slot 配置會存在 `~/.config/agtdeck/slots.json`。

## 畫面

五個頁面，用 **ALGORITHM** 輪流切換：

| 頁面 | 內容 |
|---|---|
| **GRID** | 所有 agent 的列表或方塊。一列是：狀態色邊條、slot 編號、狀態圖示、名稱、種類（`CL` Claude Code、`CX` Codex、`OC` OpenCode、`SH` shell…）、在該狀態的時間、旗標、context 用量條 |
| **DETAIL** | 選取 agent 的輸出、INFO、DIFF |
| **APPROVALS** | 等你批准的清單；沒有時顯示 ALL CLEAR |
| **MENU** | SELECT 移動、`SEL` 選、`FX` 返回。可開新 agent、送 prompt、看 event log、關 agent、改設定 |
| **ABOUT** | 韌體與協定版本、host 是否連線、backend、uptime |

顏色就是狀態：琥珀色是在等你，綠色是完成，紅色是壞了，白色是工作中，灰色是閒置。

列上的旗標：`A` auto-approve、`R` repeat、`+` 已標記、`Q` 有排隊的 prompt。

## Auto-approve

預設關閉，建議維持關閉。開了之後 daemon 會自動批准該 agent 的提示，但會先比對 `auto_never` 清單（預設含 `rm -r` / `-f`、`git push`、`git reset --hard`、`sudo`、`--force`、`drop table`、`mkfs` / `dd`），命中就不批准並發出警示。上游自己說明這是文字比對，不是安全保證。

## 設定檔

`~/.config/agtdeck/config.json`，不存在時用預設值。`python3 vendor/fm-1-agent-deck/host/agtdeck.py --print-config` 會印出完整預設。常改的：

| 欄位 | 用途 |
|---|---|
| `presets` | 預設 prompt 清單（預設 9 個），也可以是多步驟的 pipeline |
| `default_agent` | `PLAY` 在空 slot 上啟動哪一種 agent |
| `projects` | 開新 agent 時可選的目錄 |
| `auto_never` | auto-approve 絕不自動批准的字樣 |
| `nag_minutes` | agent 一直 BLOCKED 時每隔幾分鐘再提醒 |
| `follow` | 有 agent 被 block 時選取自動跳過去 |

## 我遇到的

- Herdr 裡 shell pane 也會佔 slot，我的 50 個 pane 大半是 shell，真正的 agent 被擠到後面。沒有隱藏 shell 的設定，目前只能用 `SAVE` + 鍵手動整理，或用 **KNOB 4** 直接跳到需要處理的 agent。`[實機]` host 端
- 完整說明見上游 [README](https://github.com/kevinelliott/fm-1-agent-deck#controls-reference) 與[一頁式 cheat sheet](https://kevinelliott.github.io/fm-1-agent-deck/cheatsheet.html)。
