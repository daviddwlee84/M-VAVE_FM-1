# 自己的 agent 控制台：Conductor

Conductor 是我在 [fm1-base](../dev/index.md) 上寫的 agent 控制台：韌體在 `firmware/conductor/`，電腦端的 daemon 在 `host/conductor/`。用過 [Agent Deck](agent-deck.md) 之後，有兩件事想改：shell pane 佔掉大半琴鍵，而且它只接 classic Herdr / tmux。

**狀態（2026-11-17）：0.2 已刷進實機（新晶片 `0xB36014`），開機正常，daemon 連得上，DETAIL 的中文點陣圖在實機顯示 `[實機]`。按鍵、LED、批准流程還沒逐項在實機確認。**

## 一句話的設計

FM-1 的琴鍵 LED 是**單色**的，沒辦法像 Codex Micro 那樣用顏色區分狀態。所以把資訊拆成三層：

| 誰負責 | 表達什麼 | 怎麼表達 |
|---|---|---|
| 琴鍵 LED | 急不急 | 亮度與動態：熄、微亮、中亮、全亮、呼吸、閃爍 |
| 240×240 彩色螢幕 | 是什麼 | 顏色、名稱、狀態字、輸出內容 |
| 提示音 | 不用看也知道 | FM 合成的短音型，`MASTER` 控音量 |

硬體能力 `[上游]`（fm1-base `include/plat.h`）：27 個琴鍵與 14 個按鈕都有 LED，全部單色，每顆可以是 off / glow / mid / lit。**只有 `PLAY` 有紅、綠兩顆 LED。** 閃爍與呼吸都是軟體做的。

## 狀態對應

| 狀態 | 琴鍵 LED | 螢幕色 | 提示音 |
|---|---|---|---|
| 空 | 熄 | — | — |
| IDLE | 微亮 | 白 | — |
| WORKING | 呼吸（微亮 → 中 → 全 → 中，1.6 秒一輪） | 藍 | — |
| BLOCKED（等你） | 2 Hz 閃 | 琥珀 | 上行兩音 |
| DONE（還沒看） | 全亮；選過之後轉微亮 | 綠 | 三音和弦 |
| ERROR | 每 1.2 秒雙閃 | 紅 | 下行兩音 |
| UNKNOWN | 中亮 | 灰 | — |

- **`PLAY` 是總覽燈**：有任何 BLOCKED / ERROR 時亮紅，全部正常時亮綠，沒有 host 時熄。
- 螢幕的顏色沿用 Codex Micro 的白 / 藍 / 綠 / 琥珀 / 紅，看過那台的人不用重學。
- 提示音只在**已知的 agent 換狀態**時響，剛連上、整張表填進來時不響；同時很多個換狀態只響一次。
- host 斷線 10 秒後，螢幕右上角顯示 `NO HOST`，琴鍵全部退成微亮，表示「這些狀態已經不可靠」。

## 控制

一頁式的面板圖與速查見 [Conductor 操作速查](conductor-cheatsheet.md)。

| 控制 | 動作 |
|---|---|
| 琴鍵 | 選那個 agent（每個琴鍵固定一個 agent，換頁不會跑掉） |
| **SELECT** | 在目前頁面列出的 agent 之間移動 |
| **KNOB 4** | 跳到下一個需要你的：BLOCKED → ERROR → 還沒看的 DONE |
| `REC` | approve |
| `SAVE` | approve 並不再詢問（只對有設定按鍵的 agent 種類） |
| `FX` | deny（Esc） |
| `ENV` | interrupt |
| `SEL` | Enter |
| `PLAY` | 送 `continue`（只對 IDLE / DONE） |
| `LFO` | 切換 agent 的 mode（Shift+Tab） |
| `ARP`（按住） | 語音輸入：daemon 在電腦上按住 `Ctrl`+`Option`，觸發 Wispr Flow / Handy；放開就停。見[速查](conductor-cheatsheet.md#voice) |
| **KNOB 2** | 上 / 下方向鍵 |
| `EDIT` | DETAIL 頁（classic Herdr 下同時 focus 那個 pane） |
| `HOME` / `GLO` | 全部 agent / 只列等你的 |
| **ALGORITHM** | 輪流切換三個頁面 |

按鈕的位置沿用 Agent Deck，換過來不用重新記。**預設只有 agent 會佔琴鍵，shell 不會**：hc 只放有回報狀態或前景是已知 agent 的 terminal，classic Herdr 用 `agent.list`（它本來就只列 agent）。我的 classic Herdr 當下有 48 個 pane，`agent.list` 是 20 個，27 鍵放得下 `[實機]` host 端。放不下的數量顯示在底部（`+N NOT ON KEYS`）。

琴鍵和 agent 的對應存在 host 的 `~/.config/conductor/slots-<backend>.json`，agent 重開會回到原本的鍵。

## 架構

```text
FM-1  firmware/conductor（fm1-base app，MIT）
   ⇅  USB-MIDI SysEx：F0 7D 43 4E …（協定 v1）
host/conductor（Python daemon）
   ├─ hc      → `hc connect`（JSON-RPC 2.0）、events.subscribe
   ├─ herdr   → classic Herdr 的 unix socket、events.subscribe
   └─ fake    → 測試與 demo
```

協定規格在 `host/conductor/PROTOCOL.md`。幾個決定：

- **用真正的 SysEx。** Agent Deck 把資料塞進 CC，是因為它的基底 Dinghy 把 SysEx 留給更新程式；fm1-base 讓 app 收發 SysEx（`APP_SYSEX`、`plat_sysex_send`）`[上游]`。前綴 `7D 43 4E` 避開 fm1-base 自己的 `7D 4C ..`。
- **裝置只送意圖，host 再確認一次。** 裝置送 `ACTION(op, slot, tag)`；host 送鍵前重新讀一次 backend：tag 要還是那個鍵上的 agent，狀態也要對（approve / deny 只對 BLOCKED，continue 只對 IDLE / DONE）。避免「剛好換了一個 agent 到這個鍵上」時批准錯對象。
- **沒有 auto-approve。**
- **DETAIL 由電腦畫，先去掉 agent 的介面。** Claude Code：從輸入框上方的分隔線切掉，再刪底部的狀態列。Codex：歡迎畫面的點字 logo（`⣿⠿…`，字型裡沒有，會畫成方框）、底部的 `› Ask Codex…` 與 `Context 100% left` 狀態列。字型缺的字顯示成 `?`。2026-11-17 用我真實的 Claude Code 與 Codex session 對照過 `[實機]` host 端。
- **DETAIL 的字由電腦畫。** 裝置的字型只有 ASCII，中文字型也放不進韌體。所以 daemon 先去掉 agent 的介面（輸入框、狀態列），再用系統字型（macOS 的 STHeiti）把每一行畫成 240×16 的點陣圖送過去，一行一個 558 bytes 的 SysEx。韌體 0.1 收到的是 ASCII 文字。
- **韌體不寫 flash。** 所有狀態都在 host；fm1-emu 裡兩種晶片都是 0 次 erase / program `[模擬]`。韌體只登記 fm1-base 必要的 panel calibration 物件（`0xDC000`），不用 SLOOP 的設定 sector `0xFC000`。
- **授權**：`firmware/conductor`、`firmware/common`、`host/conductor` 都是 MIT。沒有複製 Agent Deck（GPL-3.0）的程式碼，只參考它的按鍵配置與行為。

### 為什麼 hc 優先 { #backends }

| | hc（herdr-core 0.1.21） | classic Herdr 0.9.5 |
|---|---|---|
| 狀態從哪來 | agent 自己用 OSC 7501 回報；Claude Code、Codex 原生會送；hc 從不猜 `[上游]` | 程序偵測 + 畫面比對，或 integration 回報 `[上游]` |
| 列出 agent | `terminal.list`；沒回報狀態的當 shell | `agent.list`，只有 agent |
| approve / deny | `terminal.keys`（沒有專用 method） | `pane.send_keys` |
| interrupt | `terminal.interrupt`，會回報結果 | 送 Esc |
| 送 prompt | `terminal.send`，BLOCKED 時拒絕 | `agent.prompt` |
| focus pane | 沒有 | `pane.focus` |
| 遠端機器 | `hc machine add`，遠端 terminal 出現在同一份清單 | 沒有 |

兩個是**各自獨立的 runtime**，不共享 agent。我目前每天用的是 classic Herdr，所以兩個 backend 都做：hc 的狀態比較準、能接遠端機器，classic Herdr 能 focus pane。

## 為什麼不走 Codex Micro { #codex-micro }

[scf4/codex-midi](https://github.com/scf4/codex-midi) 把 MIDI controller 偽裝成 OpenAI × Work Louder 的 Codex Micro。我讀過它的原始碼 `[上游]`，決定不當主線：

- 它用 `NODE_OPTIONS=--require=…` 啟動 ChatGPT.app，在 Electron 的 main process 裡攔截 `node-hid`，插入一個假裝置。每次都要先完全關掉 ChatGPT，再從它的 launcher 開。
- 協定（`v.oai.thstatus`、`v.oai.rgbcfg` 的 64-byte HID report 裡的 JSON-RPC）是逆向工程得來，只對 ChatGPT `26.715.31925` 測過，沒有版本檢查，app 更新後可能**無聲地**失效。
- task 狀態只以 **RGB 顏色**傳給裝置，沒有「thinking」這種狀態字。FM-1 的單色 LED 收到顏色也還原不出狀態，螢幕也拿不到 task 名稱。
- 只有 6 個 task 槽，只控制 macOS 桌面版 ChatGPT 裡的 Codex，不碰 Codex CLI。

要接 Codex，比較穩的路是官方有文件的 [`codex app-server`](https://learn.chatgpt.com/docs/app-server.md)（JSON-RPC，有 approval request 與 turn 事件）`[上游]`。它會是之後的第三個 backend，不需要改韌體。

## USB 與 Bluetooth { #bluetooth }

**韌體 0.3 起支援藍牙**，2026-11-17 在實機上連通 `[實機]`。使用方式見[速查](conductor-cheatsheet.md#bluetooth)。

| 項目 | 做法 |
|---|---|
| BLE stack | fm1-base 自己的 stack（`ble_stack = "own"`）。預設的 JieLi stack 解 BLE MIDI 時把收到的 SysEx 整段跳過（`ble/midi/le_midi.c`：`sysex: skip to the end flag`）`[上游]`；own stack 會把它還原成 USB-MIDI 的 SysEx 封包（CIN 4–7），從 `plat_midi_in` 的 `PORT_BLE` 進來 `[上游]` |
| 收 | `APP_SYSEX` 只收 USB，所以 `hostlink` 的 `hl_sx_packet` 自己把 BLE 的封包組回 frame |
| 送 | frame 切成 USB-MIDI SysEx 封包，`plat_midi_send(…, TO_BLE_HOST)`；佇列放不下整個 frame 就不送（寧可晚，不要送一半） |
| 哪條路 | 沒有 host 時，USB 有電腦就從 USB、藍牙有連線就從藍牙各送一份 HELLO（帶著傳輸方式）；之後只走 HELLO_ACK 來的那條。daemon 兩條都聽到時選 USB |
| 不寫 flash | 位址由 own stack 從 flash 晶片的唯一 ID 算出（每次開機一樣，名稱 `FM-1 XXXX`）；不開配對（fm1-base：macOS、iOS 不配對也能連）。所以 app 接管藍牙的保存（`APP_BLE_STORE`），只放 RAM，不需要 `OBJ_BLE`。fm1-emu 兩種晶片仍是 0 次寫入 `[模擬]` |
| 開關 | 每次開機都開；`OCT+` 關到下次開機（不存設定，所以也不寫 flash） |
| DETAIL | 藍牙上只送 ASCII 文字；點陣圖一行 558 bytes，BLE MIDI 太慢。設定 `detail_pictures_ble: true` 可以強制送圖 |
| 容量 | app 122 KB、RAM `.data+.bss` 38 KB（上限 96 KB）；`ble_diag = false` 省下 2.4 KB RAM |

驗證到哪裡：

| 項目 | 結果 | 標記 |
|---|---|---|
| `just fw-common-test` | 0–20 bytes、558、640 bytes 的 frame 切成封包再組回，內容相同；中間插一個 note-on 會被忽略；超長的 frame 整個丟掉、下一個正常 | — |
| host simulator 的 BLE 替身 | 拔掉 USB、從 BLE 連上後，裝置從 BLE 送出 HELLO（傳輸方式 `01`）；`OCT+` 開關藍牙；0 次 store 寫入 | `[模擬]` |
| fm1-emu 兩種晶片 | 開機、重開都 0 次 erase / program；USB 路徑照常。模擬器不模擬藍牙無線電 | `[模擬]` |
| 實機：Mac 經藍牙連上、daemon 用藍牙 port | 拔掉 USB，在 MIDI 工作室的藍牙設定連上 `FM-1 3399`（不需要配對碼）；CoreMIDI 的 port 名稱是 `FM-1 3399 Bluetooth`，daemon 自己找到並回報 `firmware 0.3, via Bluetooth` | `[實機]` |

## 經過 iOS 或 MIDI routing app { #routing }

Conductor 和 daemon 之間說的**全是 SysEx**（`F0 7D 43 4E …`），琴鍵和按鈕不送 note 或 CC。daemon 要在 Mac 上跑（它要接 hc / Herdr），所以 iPad 上的 routing app（StreamByter、MidiFire、AUM 等）頂多是中繼站。我的建議 `[調研]`，沒有實測：

- **能直接接就直接接。** USB 或藍牙直連 Mac 最單純；中間每多一個 app，就多一個可能丟掉或改寫 SysEx 的地方。
- **藍牙一次只能連一台。** fm1-base 的 own stack 同時只有一條 BLE MIDI 連線 `[上游]`：iPad 用藍牙連著時，Mac 就連不上藍牙。iPad 和 Mac 都要用時，Mac 走 USB。
- **讓 SysEx 原封不動通過。** 很多 routing app 預設會過濾或限制 SysEx。規則用前綴 `F0 7D 43 4E` 放行，其他照你的需求處理；`7D 43 4E` 是我們自己的固定前綴，就是為了好認、好放行。
- **不要把別的來源合併進同一條路。** 兩個來源合流時，SysEx 可能被別的訊息插進中間而損壞。Conductor 的 frame 損壞時會被丟掉，不會誤動作，但會漏掉更新。
- **一台裝置只接一個 daemon。** routing 把同一台 FM-1 分給兩個 daemon，兩邊會搶著送表、各自執行按鍵。
- **想在 iOS 上拿琴鍵觸發別的東西**，現在做不到，因為沒有 note / CC。之後可以加一個選項：琴鍵同時送 channel 16 的 note、按鈕送 CC，讓 StreamByter 這類工具不必懂 SysEx 也能用。

## 共用模組 { #common }

給之後其他韌體用的部分放在 `firmware/common/`（MIT），app 在 `fm1.toml` 加 `include = ["../common"]`，再在 `app.c` include `.c` 檔：

| 模組 | 用途 |
|---|---|
| `hostlink` | app 自己的 SysEx 對話，走 USB 或藍牙：frame 前綴、HELLO 直到 host 回應、之後 HEARTBEAT、host 是否還在、重新插線或藍牙斷線後重新 HELLO；藍牙 SysEx 封包的拆與組 |
| `ledfx` | 單色 LED 的動態：off / glow / mid / lit / 呼吸 / 閃 / 雙閃 / 快閃，每 frame 轉成 `plat_lights` + `plat_lights_mid` |
| `chime` | 兩個 operator 的 FM 小音色加上音符佇列，混進 app 的 render，做提示音 |

說明與用法在 `firmware/common/README.md`。

## 指令

```sh
just cond-test              # daemon 的單元測試（協定、slot 記憶、動作確認）
just fw-common-test         # firmware/common 的單元測試（藍牙 SysEx 封包）
just cond-sim               # daemon 對 fake backend，裝置用 terminal 文字代替
just cond-sim auto          # 同上，接真的 hc 與 classic Herdr（只看不按就不會送任何鍵）
just cond-it                # hc 端到端：開兩個假 agent，approve / deny / continue，再關掉
just fw-test conductor      # 韌體在 host simulator 跑 tests/basic.txt
just fw-build conductor     # 實機套件 firmware/conductor/build/conductor.fwsc
just cond-emu               # 韌體在 fm1-emu，插虛擬 USB
just cond-run               # daemon 接 MIDI 裝置（模擬器或實機）；預設 auto
just cond-run herdr         # 只接 classic Herdr（hc、fake 同理）
```

### 假的還是真的 agent { #real-or-fake }

| 跑法 | agent 從哪來 | 按鍵會送到哪 |
|---|---|---|
| `just cond-run fake`、`just cond-sim` | daemon 記憶體裡寫死的 5 個 demo agent | 哪裡都不送，只記錄 |
| `just cond-it` | 在 hc 裡開兩個假 agent 程序（`host/tests/fake_agent.py`，會送 OSC 7501），測完關掉 | 只送給那兩個假 agent |
| `just cond-run`、`cond-run hc`、`cond-run herdr` | **你真實的 hc / classic Herdr session** | **真的 Claude Code、Codex** |

接真的 backend 時，`REC`、`FX`、`ENV`、`SEL`、`PLAY` 都會真的作用在那個 agent 上。2026-11-17 在 Herdr app（建在 hc 上）裡開的 Claude Code 會自己回報 `app=claude-code` 的狀態，`cond-run hc` 直接顯示成 `CL … DONE` `[實機]` host 端。用 `specstory run claude` 這類 wrapper 啟動也認得：種類從 argv 裡找。

`--backend` 預設是 **`auto`**：hc 與 classic Herdr 哪個在跑就接哪個，兩邊的 agent 合在一起放上琴鍵（標頭顯示 `hc+herdr`）。兩個 runtime 不共享 agent，agent 可能在任一邊。

### MIDI port 叫 `Felucca` { #port-name }

macOS 會記住這台機器之前的 MIDI 名稱，刷了 Conductor 之後 port 仍可能叫 `Felucca`（USB 產品名其實是 `CONDUCTOR FM-1`）`[實機]`。daemon 先找名字含 `CONDUCTOR` 的 port，找不到就聽所有 MIDI 輸入 2.5 秒，誰送出 Conductor 的 HELLO / HEARTBEAT 就用誰。也可以直接指定：`just cond-run hc --port Felucca`。

**同一時間只開一個 daemon。** 兩個 daemon 對著同一台機器時，裝置會收到兩邊的表；daemon 每 30 秒重送一次完整的表，最多 30 秒就會恢復。

### 重開 daemon 後中文變成 `?` { #restart }

韌體版本只寫在裝置的 HELLO 裡，而裝置只在認為 host 不在時才送 HELLO（最後一個 frame 之後 10 秒）。2026-11-17 我停掉 daemon 後馬上重開，新 daemon 只收到 HEARTBEAT，不知道裝置是 0.2，DETAIL 就退回 ASCII `[實機]`。

現在 daemon 在收到 HELLO 前什麼都不送、也不執行任何按鍵，裝置 10 秒內就會判定 host 離線並重新 HELLO。等待時會印 `[device] still attached to the last daemon; waiting for its HELLO (up to 10 s)`；之後出現 `[device] hello, protocol 1, firmware 0.2` 就對了。

### 螢幕顯示 `NO AGENTS` { #no-agents }

daemon 已經連上（標頭有 backend 名稱），但那個 backend 裡沒有 agent。2026-11-17 我第一次用 `just cond-run hc` 就是這樣：我的 hc 裡只有兩個 `zsh`，shell 不佔琴鍵，agent 全在 classic Herdr `[實機]` host 端。daemon 啟動時會印出每個 backend 看到什麼：

```text
[backend] hc: 0 agents, 2 shells not shown (--show-shells to list them)
[backend] herdr: 20 agents
```

用預設的 `auto`，或指定 agent 所在的 backend。想讓 hc 的 shell 也佔鍵，加 `--show-shells`。

### 名稱

螢幕只顯示 ASCII。pane 標題開頭的狀態符號（`●`、`✳`）會拿掉；標題大多不是 ASCII（中文、emoji）時，改用工作目錄的名稱。

`cond-sim` 裡可以打 `sel N`、`approve`、`deny`、`int`、`cont`、`show`、`quit`。

daemon 的設定檔是 `~/.config/conductor/config.json`（可省略），可以改各種 agent 的按鍵，例如：

```json
{"approve_always_keys": {"claude": ["2"], "codex": ["a"]}}
```

## 驗證結果

| 項目 | 結果 | 標記 |
|---|---|---|
| `just cond-test` | 20 項通過 | — |
| `just cond-it`（hc） | 兩個假 agent 上了琴鍵，既有的兩個 zsh 沒有；approve 送到 Enter、deny 送到 Esc、continue 送到文字；結束後只清掉自己的 terminal | `[實機]` host 端 |
| classic Herdr（唯讀） | `agent.list` 20 個 agent、`pane.read` 讀得到輸出；**沒有對 classic Herdr 送過任何鍵** | `[實機]` host 端 |
| `auto`（唯讀） | hc 0 個 agent + 2 個 shell 沒顯示、classic Herdr 20 個，合併上琴鍵，名稱正確 | `[實機]` host 端 |
| `just fw-test conductor` | 全部 `ok`：HELLO、HELLO_ACK、SLOT、REC 送出 approve、對 WORKING 按 REC 被拒、提示音只在換狀態時響、K4、三個頁面、10 秒後 `NO HOST` | `[模擬]` host simulator |
| `just fw-build conductor` | app 85,356 B；RAM `.data+.bss` 26,084 / 98,304 B；build 自帶檢查全部 `ok` | — |
| fm1-emu `uc25hq80` / `p25q80` | 都列舉成 USB `CONDUCTOR FM-1`、每秒 HELLO、收到 HELLO_ACK 後改送 HEARTBEAT、畫面正確；**flash 0 次 erase / program** | `[模擬]` |
| daemon ↔ fm1-emu（虛擬 USB） | daemon 經 CoreMIDI 找到 `CONDUCTOR FM-1 (emu)`，收到 HELLO 並送出整張表 | `[模擬]` |

## 實刷準備 { #flash }

**2026-11-17 18:40 已由我本人刷寫，結果見[我的機器](../device-log.md)。** 以下是刷之前做的檢查。

要刷的套件：

| 項目 | 值 |
|---|---|
| 檔案 | `firmware/conductor/build/conductor.fwsc`（616,003 B，identity `FM-1_1200`，USB 產品名 `CONDUCTOR FM-1`） |
| 來源 | commit `d45ce67`（原本是 `ba67827`，之後 amend 只加了計畫檔與 `.specstory`，原始碼相同），乾淨的工作區重新 build |
| SHA-256 | `de1827239a4a9f395316cf9d5d6b9f02f41abdebcde8197aa0f51b78e0d502f3` |
| 支援的晶片 | `856014`、`B36014`（build 產生的 `firmware.json`；fm1-base 的 `device/fm1_flash_ok.h` 與測試）`[上游]` |

Preflight：

| 檢查 | 結果 | 標記 |
|---|---|---|
| `just fw-test conductor` | 全部 `ok` | `[模擬]` host simulator |
| fm1-emu 兩種晶片：開機 → 重開 → 再讀回 | `uc25hq80`、`p25q80` 都 0 次 erase / program，重開前後 flash 與畫面完全相同（這個韌體沒有東西要存） | `[模擬]` |
| 換韌體預演（疊在最新的實機備份 `2026-11-17-0413-pre-choralroot-0.41` 上開機） | 正常開機、USB 列舉、收得到 host 的 frame；開機後 0 個 sector 被改寫。套件映像 `0x04000`–`0x92FFF`，和備份相比不同的有 124 個 sector（`0x04000`–`0x7FFFF`） | `[模擬]` |
| 退路一：M-Flasher 切下載模式 | 送出 M-Flasher 用的進入更新模式 SysEx（`F0 22 24 35 7D F7`，felucca-bsp `chip/usb.c`）後，USB 斷開再重連，和 SLOOP 2.8.5、Agent Deck 0.7 在同一測試下的反應相同；那兩套都在實機上用 M-Flasher 切走過。模擬器不模擬 ROM 更新模式 | `[模擬]` |
| 退路二：開機時按住 `REC` | fm1-base 開機時偵測到 `REC` 就進入晶片的更新模式（`device/main_fm1.c`），給「韌體能開機但 USB 不通」時用 | `[上游]` |
| 目前的韌體與 bootloader | `fm1-info`：firmware Felucca（即 ChoralRoot 0.41）、bootloader stock (M-VAVE)、app OK、flash `0xB36014` | `[實機]` |
| 完整備份（目前 ChoralRoot 的狀態） | `backups/2026-11-17-pre-conductor/fm1-20261117-183840.bin`，SHA-256 `81f048bb0e8bea174ccb84df4de2555c891b087651083710c41c428a0e7ba1e1`，已記進[我的機器](../device-log.md) | `[實機]` |
| 疊在**這份**備份上預演 | 正常開機、0 個 sector 被改寫；安裝會和現況不同的是 140 個 sector（`0x04000`–`0x91FFF`） | `[模擬]` |

會被改的地方：韌體區 `0x04000`–`0x92FFF`。ChoralRoot 的映像到 `0x9EFFF`，`0x93000`–`0x9EFFF` 留著不動，Conductor 不讀它。Conductor 不寫設定 sector `0xFC000`，也不寫任何資料區。實際寫入範圍以 M-Flasher 刷之前印出的計畫為準。

### 0.2 的檢查 { #flash-0-2 }

0.2 加了 DETAIL 點陣圖、KNOB 1 捲動、說明頁。換成 0.2 一樣是韌體寫入，要我本人批准。

| 項目 | 值 |
|---|---|
| 檔案 | `firmware/conductor/build/conductor.fwsc`（616,003 B，identity `FM-1_1200`） |
| 來源 | commit `5add525`，乾淨的工作區重新 build，兩次 build 的 SHA-256 相同 |
| SHA-256 | `48a052f9aa917a576a505f1f0dd75a4eaa375204371c5660a7932eeb2cfaea6d` |

| 檢查 | 結果 | 標記 |
|---|---|---|
| `just fw-test conductor` | 全部 `ok`，含 KNOB 1 捲動、說明頁、host 畫的中文行（699 個點） | `[模擬]` |
| fm1-emu 兩種晶片：開機 → 重開 → 再讀回 | 都 0 次 erase / program | `[模擬]` |
| 疊在 `2026-11-17-pre-conductor` 備份上開機 | 正常、0 次寫入 | `[模擬]` |
| 558 bytes 的點陣圖 frame 走虛擬 USB | 收得到，DETAIL 頁中文正確 | `[模擬]` |
| 備份 | `2026-11-17-pre-conductor`（ChoralRoot 狀態）；0.1 不寫 flash，所以現在的資料區和那份相同（除了韌體區） | `[實機]` |

步驟和 0.1 相同：關掉模擬器與 daemon，跑 `just fm1-switch firmware/conductor/build/conductor.fwsc`，確認計畫後打 `yes`。

### 0.3 的檢查 { #flash-0-3 }

0.3 加了藍牙。換成 0.3 一樣要我本人批准。

| 項目 | 值 |
|---|---|
| 檔案 | `firmware/conductor/build/conductor.fwsc`（616,003 B，identity `FM-1_1200`，app 122,120 B） |
| 來源 | commit `b7e0564`，乾淨的工作區重新 build，兩次 SHA-256 相同 |
| SHA-256 | `b48d6059b621c86cef3b84b29c2c023c6741ac04c68dc5d892a7ff77f8476609` |

| 檢查 | 結果 | 標記 |
|---|---|---|
| `just fw-test conductor`、`just fw-common-test`、`just cond-test` | 全部通過 | `[模擬]` / 單元測試 |
| fm1-emu 兩種晶片：開機 → 重開 → 再讀回 | 都 0 次 erase / program（藍牙開著） | `[模擬]` |
| 疊在 `2026-11-17-1859-pre-conductor` 備份上開機 | 正常、0 個 sector 被改寫 | `[模擬]` |
| 藍牙實際連線 | 模擬器沒有無線電，**只能在實機確認** | — |

風險：藍牙是新程式碼，最壞的情況是開機後藍牙不正常。fm1-base 寫明「上一次藍牙執行當掉，下次開機就不自動啟動藍牙」`[上游]`，USB 照常可用，`fm1-switch` 也能換回 0.2 或別的韌體。

### 刷的步驟（由我本人執行）

1. 關掉模擬器（`fm1-switch` 看到模擬器在 USB 上會拒絕執行，避免刷錯對象）。
2. FM-1 用 USB **資料線**直接接 Mac，不經過 hub。
3. agent 跑 `just fm1-info` 和 `just fm1-backup pre-conductor`（唯讀；會先把機器切到下載模式），把 SHA-256 記進 device log。
4. 我自己跑：

    ```sh
    just fm1-switch firmware/conductor/build/conductor.fwsc
    ```

    它會再備份一次，然後 M-Flasher 列出寫入計畫；確認範圍在 `0x04000`–`0x92FFF` 附近，再打 `yes`。
5. 重開後螢幕顯示 `AGENTS` / `NO AGENTS`、右上角 `NO HOST`。跑 `just cond-run`，右上角變成 backend 名稱就是連上了。
6. 回 ChoralRoot：`just fm1-switch choralroot`（其他韌體同理）。

刷完後要更新：[我的機器](../device-log.md)、[多韌體切換](../firmware/switching.md)的實機紀錄、[Flash 配置圖](../hardware/flash-map.md)。

## 還沒做 / 下一步

- [ ] approve 的按鍵：預設 Enter（Claude Code 與 Codex 的權限選單預設選「Yes」）。`SAVE`（不再詢問）預設 Claude `2`、Codex `a`，**還沒對真的 session 驗證過** `[調研]`。先用一個用完即丟的 session 試，再把結果寫回這裡。
- [ ] 在 fm1-emu 視窗裡手動按一輪 `REC` / `FX` / `ENV`，對 hc 的假 agent。
- [ ] classic Herdr 的動作（approve / deny / focus）在一個新開的測試 workspace 裡驗證。
- [x] Bluetooth（0.3），見 [USB 與 Bluetooth](#bluetooth)。
- [x] 藍牙在實機連通（2026-11-17）。
- [ ] FM-1 關機再開機後，macOS 會不會自己重新連上藍牙（還沒試）。
- [ ] 不用 GUI 連線：`just cond-ble connect / disconnect`（CoreBluetooth + `MIDIBluetoothDriverActivateAllConnections`），見[速查](conductor-cheatsheet.md#bluetooth)。
- [ ] `codex app-server` backend。
- [ ] 語音鍵（`ARP`）對真的 Wispr Flow 試一次：host 單元測試確認了送出的按鍵與一定會放開，**還沒在有輔助使用權限的 terminal 裡實際觸發過** Wispr Flow。
- [x] 實刷（2026-11-17），見[實刷準備](#flash)。
- [ ] 在實機逐項確認：琴鍵燈的各種狀態、`REC` / `FX` / `ENV` 對 `cond-it` 的假 agent、提示音、`ARP` 語音鍵。
