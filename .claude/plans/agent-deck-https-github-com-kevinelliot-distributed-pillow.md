# 自製 FM-1 agent 控制台：設計 + 原型

## Context 背景

Agent Deck 已刷過實機，用下來有兩個痛點：shell pane 佔掉大半琴鍵、只綁 classic Herdr / tmux。市面上有 SlashVibe、VibeDeck、AgentDeck 等競品，也有 codex-midi 這種「假裝成 Codex Micro」的路線。目標是在 **fm1-base** 上寫自己的 agent 控制台韌體 + host daemon，這一輪做到「設計文件 + 原型」：host simulator 與 fm1-emu 跑通，**不刷實機**。

## 調研結論（決定設計的事實）

| 問題 | 結論 | 來源 |
|---|---|---|
| Codex Micro 模擬可行嗎 | 可行但不當主線：靠 `NODE_OPTIONS --require` 注入 ChatGPT.app、私有 `v.oai.*` 協定、只認一個 app build、狀態只以 RGB 顏色傳來（單色 LED 無法還原）、只有 6 個 task、只限 macOS 桌面版 Codex | scf4/codex-midi 原始碼 |
| Codex 的正規介面 | `codex app-server`（官方文件、JSON-RPC、有 approval request 與 turn 事件）與 hooks。列為**之後的第三個 backend**，取代 Codex Micro 模擬 | learn.chatgpt.com/docs/app-server |
| hc（herdr-core 0.1.21） | JSON-RPC 2.0 over unix socket、`events.subscribe`（含 `seq`/`missed`）、狀態直接來自 OSC 7501（`idle/working/done/blocked/error` + `kind=permission/question`），Claude Code 與 Codex 原生會送。沒有 focus pane | herdr-core repo、本機 `hc schema` |
| classic Herdr 0.9.5 | 本 session 就在它底下（`HERDR_ENV=1`），有 `pane.focus`、`events.subscribe`，狀態靠偵測 + 畫面比對 | herdr socket-api 文件 |
| LED | 全部 27 鍵 + 14 鈕都是**單色**，可用 off / glow / mid / lit / breath + 軟體閃爍；**只有 `PLAY` 有紅綠雙色** | fm1-base `include/plat.h:77-94` |
| 螢幕 | 240×240 RGB565 全彩，`cv_*` 繪圖、多組字型 | fm1-base `device/gfx.c` |
| 協定 | fm1-base 讓 app 收 SysEx（`APP_SYSEX`，≤640 B）並可送 SysEx；Agent Deck 塞 CC 的作法只是 Dinghy 的限制 | `plat.h:391-409, 624-628` |
| 測試 | host simulator 腳本可注入 `midi`/`sysex`，並 `expect_sysex`、`leds` 檢查輸出 | fm1-base `host/plat_host.c` |

所以：**LED 負責「急不急」（亮度與動態），螢幕負責「是什麼」（顏色與文字），聲音負責「不用看也知道」。**

## 設計

工作名 **Conductor**（指揮一群 agent；名字可改）。

```text
FM-1 (firmware/conductor, fm1-base app)
   ⇅ USB-MIDI SysEx（0x7D 非商用 ID）+ 琴鍵 Note / 按鈕 CC
host/conductor (Python daemon)
   ├─ HcBackend      → hc unix socket（JSON-RPC, events.subscribe）
   ├─ HerdrBackend   → classic Herdr socket
   └─ FakeBackend    → 測試用（之後：CodexAppServerBackend）
```

### 狀態對應

| 狀態 | 琴鍵 LED | 螢幕色 | 提示音（FM 合成） |
|---|---|---|---|
| EMPTY | off | — | — |
| IDLE | glow | 白/灰 | — |
| WORKING | breath | 藍 | — |
| DONE（未讀） | lit，選取後轉 glow | 綠 | 短和弦 |
| BLOCKED | 2 Hz 閃 | 琥珀 | 上行兩音，`nag` 間隔重提 |
| ERROR | 雙閃 | 紅 | 低音 |
| UNKNOWN | mid | 灰 | — |

- `PLAY` 雙色 LED 當**總覽燈**：任何 BLOCKED/ERROR → 紅，全部正常 → 綠。
- 螢幕配色沿用 Codex Micro 的白/藍/綠/琥珀/紅，降低學習成本。
- 提示音用 `MASTER` 控音量，是 FM-1 獨有、競品沒有的回饋。

### 控制與版面

- 琴鍵 = slot，**預設只放 agent，shell 不佔鍵**（解決 Agent Deck 的頭號問題）；slot 對應由 host 記住。
- 按鈕沿用 Agent Deck 的肌肉記憶：`REC` approve、`FX` deny、`ENV` interrupt、`SEL` Enter、`PLAY` continue、`EDIT` 看輸出（classic Herdr 下同時 focus pane）、`HOME` GRID、`GLO` APPROVALS、KNOB 4 跳下一個需要處理的。
- 畫面 MVP：GRID、DETAIL（輸出尾端）、APPROVALS；`NO HOST` 狀態。

### 協定原則

- 裝置自己管選取與換頁（反應快），送出的是**意圖**：`ACTION{op, slot, agent_tag}`。
- host 收到後**重新確認**該 agent 仍是同一個、仍在可接受的狀態（例如 approve 只對 `blocked`）才送鍵，避免 slot 剛換人時誤批准。
- host→device：`SNAPSHOT`、`SLOT`、`DETAIL`、`FOOTER`、`BEEP`、`HELLO_ACK`；device→host：`HELLO`、`HEARTBEAT`、`ACTION`、`SELECT`。7-bit 編碼，規格與**共用測試向量**放在 `host/conductor/PROTOCOL.md`，Python 單元測試與韌體 `tests/basic.txt` 都用同一組 hex。
- **沒有 auto-approve**，這一輪不做。

### 授權與安全

- 不複製 Agent Deck（GPL-3.0）的程式碼，只參考行為；自己的程式碼預設 **MIT**（repo 目前無 LICENSE，fm1-base 允許 app 自選）。
- MVP 不存任何設定到 flash（slot 對應存 host 端）。若 fm1-base 強制 `APP_STORE_MAP`，保留但不寫入，並在 emulator 量測寫入次數；template 的 `0xFC000` 與 SLOOP 設定區重疊這點寫進文件。
- 先用 `--no-ble` build，BLE 留到之後（無線是相對 Agent Deck 的加分項）。

## 實作步驟

1. **文件**（先做，`[上游]` / `[調研]` 標記）
   - 新增 `docs/agentic/conductor.md`：上面的設計、狀態對應表、為什麼不走 Codex Micro；加進 `zensical.toml` nav。
   - `docs/agentic/agent-deck.md` 的替代方案補上 codex-midi（機制與風險）、hc vs classic Herdr 對照、`codex app-server`。
   - `docs/dev/index.md` 下一步：勾掉「fork 還是重寫」，連到新頁。
   - `AGENTS.md` Layout 加 `host/` 與 `firmware/conductor/`。
2. **host daemon** `host/conductor/`（stdlib + `mido`，`pyproject.toml` 加 `host` dependency group 含 `pytest`）
   - `model.py`（Agent dataclass、State enum）、`protocol.py`（SysEx encode/decode）、`device.py`（MIDI I/O、heartbeat、重同步）、`backends/{base,fake,hc,herdr}.py`、`__main__.py`（`--backend hc|herdr|fake`、`--simulate` 在 terminal 印出畫面與 LED）。
   - 順序：fake → hc → classic Herdr。hc 走 `events.subscribe`，`missed` 時重抓 snapshot；Herdr 走 `events.subscribe` + `session.snapshot`。
   - 各 kind 的 approve 鍵序（Claude Code / Codex 的權限選單）用 fake agent 與一個用完即丟的真 session 驗證後寫進表格。
   - `tests/fake_agent.py`：一個會送 OSC 7501、讀鍵盤的假 agent，給 hc / Herdr 整合測試用。
3. **韌體** `firmware/conductor/`（從 `firmware/hello` 複製 template）
   - `app_sysex` 收 frame、狀態表、GRID/DETAIL/APPROVALS 繪製、LED 模式、PLAY 總覽燈、FM 提示音。
   - `tests/basic.txt`：注入共用測試向量的 sysex，`expect_sysex` 檢查按鍵產生的 ACTION，`leds` 檢查 BLOCKED 閃爍，`shot` 存畫面。
4. **Justfile**：`cond-sim`（daemon + fake/hc 的 terminal 模擬）、`cond-run BACKEND`（接 MIDI 裝置）、`cond-emu CHIP`（`emu-run firmware/conductor/build/*.fwsc CHIP --usb`）、`cond-test`（pytest）。
5. **整合**：`just fw-build conductor` → `just cond-emu uc25hq80` + `just cond-run hc`，再用 `p25q80` 跑一次；結果寫進新頁，標 `[模擬]`。

## Verification 驗證

- `just cond-test`：protocol round-trip、共用測試向量、backend 對 fake 的 list/act、approve 的狀態重確認。
- `just fw-test conductor` 全部 `ok`，截圖看 GRID / DETAIL / APPROVALS。
- hc 整合：`hc run --bg` 起兩個 `fake_agent.py`（label `conductor-test`），切換 working → blocked → done，`--simulate` 看到正確 LED/顏色，按 approve 後 fake agent 收到正確鍵序；結束時只清掉自己 label 的 terminal。**不對你既有的 agent 送任何鍵。**
- classic Herdr 整合：同上，在一個新開的 test workspace 裡。
- emulator 端到端：兩種晶片都從 `NO HOST` 變 `HOST`、顯示 fake agent、按 `REC` 能 approve；記錄 flash 寫入次數（預期 0 次 app 寫入）。
- `just docs-check` 通過。
- **不刷實機**；之後要刷時照 `AGENTS.md` preflight 另行批准。
