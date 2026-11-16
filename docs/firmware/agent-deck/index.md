# Agent Deck

[FM-1 Agent Deck](https://github.com/kevinelliott/fm-1-agent-deck) 把 FM-1 變成 CLI coding agent 的實體控制台。這頁是我自己的試用紀錄；它是什麼、有哪些替代方案，見[用 FM-1 控制 agent](../../agentic/agent-deck.md)。

**狀態（2026-11-17）：host daemon 的模擬模式已接上我的 Herdr；韌體只在模擬器裡跑過，還沒刷進實機。**

## 不刷機就能試的部分

```sh
just agtdeck-doctor     # 檢查 Python、MIDI、設定、agent CLI、Herdr
just agtdeck-sim        # 沒有 FM-1 也能跑：印出 frame，從 stdin 讀指令，輸入 quit 離開
just agtdeck-fetch      # 下載 .fwsc，驗 digest 與 build attestation
just emu-run downloads/agtdeck-0.7.fwsc     # 在 fm1-emu 裡開韌體（新晶片模式）
```

`agtdeck-doctor` 與 `agtdeck-sim` 會先把上游 v0.7 clone 到 `vendor/fm-1-agent-deck`（gitignore）。

## 讓模擬器裡的韌體接上真的 daemon

`just emu-run` 單獨開只會停在 `NO HOST`，因為沒有 daemon。開兩個 terminal：

```sh
# terminal 1：模擬器，加 --usb 插上虛擬 USB 線
just emu-run downloads/agtdeck-0.7.fwsc uc25hq80 --usb

# terminal 2：daemon 的硬體模式（不是 --simulate），會找到名為 "FM-1 AGTDECK (emu)" 的 MIDI 裝置
just agtdeck-run
```

我確認過的：模擬器視窗開著並帶 `--usb` 時，macOS 上會出現 MIDI 輸入與輸出 `FM-1 AGTDECK (emu)`，daemon 沒有回報找不到裝置。畫面是否真的顯示出 agent grid 我沒看到（那要看視窗），請自己確認。headless 的 `emu-shot` 不會建立這個虛擬裝置。`[模擬]`

`just agtdeck-run` 用 repo 的 `fw` dependency group 提供 `mido` 與 `python-rtmidi`，不必另外 `pip install`。接實機時也是同一個指令。

`just agtdeck-sim` 會每秒重印全部 slot，pane 多的時候輸出很洗版，那是模擬模式把每個 frame 印出來的正常行為。

## 試用結果

| 項目 | 結果 | 標記 |
|---|---|---|
| `--doctor` | Herdr socket 找到、`claude` / `codex` / `opencode` CLI 找到；唯一的 FAIL 是沒裝 `mido`（模擬模式不需要，接實機才需要） | `[實機]` host 端 |
| `--simulate --backend herdr` | 連上 Herdr，`HELLO_ACK proto=6`，列出我目前所有的 pane 並辨識種類（`claude`、`codex`、`shell`） | `[實機]` host 端 |
| 韌體在 fm1-emu 開機 | 兩種晶片都到 `WAITING FOR THE HOST`，都有 flash 寫入 | `[模擬]` |
| 套件來源 | release digest 相符，`gh attestation verify` 通過 | — |
| 刷進實機 | 還沒做 | — |

## 發現

- **Pane 太多。** 我的 Herdr 當下有 48 個 pane，其中很多是 shell。Agent Deck 一頁 27 鍵、共兩頁 54 個 slot，放得下，但 shell 佔掉大半的鍵。上游設定檔裡我沒找到「只顯示 agent、隱藏 shell」的選項（`--print-config` 列出的欄位中沒有），這是之後自己改或自己寫時要解決的第一件事。
- **Slot 會被記住。** daemon 把 slot 對應存在 `~/.config/agtdeck/slots.json`，agent 重啟後還在同一個鍵上。
- **auto-approve 預設有 7 條 never-pattern、沒有 allow-list。** 維持不開。
- **接實機需要** `pip install mido python-rtmidi`。還沒裝。

## 實刷前要做的

- [ ] 在 `just agtdeck-sim` 裡實際操作一輪（`select`、`approve`、`deny`、`interrupt`），確認對 Herdr 的動作符合預期
- [ ] 用上面的兩個 terminal 讓模擬器接上 daemon，看過 grid 與各個畫面
- [ ] SLOOP 目前的狀態有備份（已有 `2026-11-17-sloop-2.8.5`；之後有新曲子要再備一次）
- [ ] preflight 四項，見[工作流與權限](../../agentic/index.md)
- [ ] 知道切回 SLOOP 的方法，見[多韌體切換](../switching.md)

## 風險

- 上游沒有新晶片 `0xB36014` 的實機回報。模擬器裡的寫入行為與舊晶片一致，但那只是 `[模擬]`。
- 上游明說：從別的韌體切過來再切回去，原本韌體存的 project 是否還在**沒有測過**。所以刷之前一定要有 SLOOP 的完整備份。
- 它是獨立韌體，裝上去之後 FM-1 就不是合成器了，要重刷才回得來。
