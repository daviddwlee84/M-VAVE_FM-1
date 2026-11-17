# Conductor 操作速查

一頁看完怎麼用 [Conductor](conductor.md)。對應韌體 0.3、協定 v1（2026-11-17）。

## 三步開始

```sh
just cond-emu        # 1. 韌體跑在模擬器（之後換成實機，就是插上 USB）
just cond-run        # 2. daemon：自動接 hc 與 classic Herdr 裡的 agent
                     # 3. 螢幕右上角出現 backend 名稱（hc、herdr、hc+herdr）就是連上了
```

只想試按鍵、不碰真的 session：`just cond-run fake`。**接真的 backend 時，按鍵會真的送給那個 Claude Code / Codex。**

## 面板

<figure class="cond-panel" markdown="0">
<svg viewBox="0 0 760 400" role="img" aria-label="FM-1 面板上每個控制在 Conductor 裡的功能">
<style>
.cond-panel svg{width:100%;height:auto;font-family:var(--md-text-font-family,system-ui),sans-serif}
.cond-panel .box{fill:none;stroke:currentColor;stroke-opacity:.35;stroke-width:1.5}
.cond-panel .lab{fill:currentColor;fill-opacity:.6;font-size:11px}
.cond-panel .fn{fill:currentColor;font-size:13px;font-weight:700}
.cond-panel .ans{fill:#f0a020;fill-opacity:.28;stroke:#f0a020}
.cond-panel .ctl{fill:#4a8cff;fill-opacity:.22;stroke:#4a8cff}
.cond-panel .nav{fill:currentColor;fill-opacity:.08;stroke:currentColor;stroke-opacity:.4}
.cond-panel .voi{fill:#3cc878;fill-opacity:.28;stroke:#3cc878}
.cond-panel .off{fill:none;stroke:currentColor;stroke-opacity:.18;stroke-dasharray:3 3}
.cond-panel .scr{fill:#000;stroke:currentColor;stroke-opacity:.4}
.cond-panel .key{fill:currentColor;fill-opacity:.1;stroke:currentColor;stroke-opacity:.35}
.cond-panel .keyb{fill:currentColor;fill-opacity:.3;stroke:currentColor;stroke-opacity:.35}
.cond-panel text{text-anchor:middle}
</style>
<rect class="box" x="2" y="2" width="756" height="396" rx="18"/>
<!-- left knobs -->
<circle class="ctl" cx="60" cy="58" r="24"/><text class="lab" x="60" y="25">MASTER</text><text class="fn" x="60" y="100">提示音量</text>
<circle class="nav" cx="150" cy="58" r="24"/><text class="lab" x="150" y="25">SELECT</text><text class="fn" x="150" y="100">上下選</text>
<circle class="off" cx="60" cy="164" r="24"/><text class="lab" x="60" y="131">PRESETS</text><text class="lab" x="60" y="206">—</text>
<circle class="nav" cx="150" cy="164" r="24"/><text class="lab" x="150" y="131">ALGORITHM</text><text class="fn" x="150" y="206">換頁</text>
<rect class="off" x="30" y="216" width="58" height="24" rx="6"/><text class="lab" x="59" y="232">OCT-</text>
<rect class="ctl" x="122" y="216" width="58" height="24" rx="6"/><text class="lab" x="151" y="232">OCT+ 藍牙</text>
<!-- screen -->
<rect class="scr" x="228" y="20" width="230" height="222" rx="10"/>
<text x="343" y="48" style="fill:#fff;font-size:14px;font-weight:700">AGENTS</text>
<rect x="240" y="62" width="4" height="16" style="fill:#ffb000"/><text x="350" y="75" style="fill:#ffb000;font-size:12px">F3 CL fix-tests  WAIT</text>
<rect x="240" y="86" width="4" height="16" style="fill:#468cff"/><text x="350" y="99" style="fill:#468cff;font-size:12px">F#3 CX refactor  WORK</text>
<rect x="240" y="110" width="4" height="16" style="fill:#50dc78"/><text x="350" y="123" style="fill:#50dc78;font-size:12px">G3 CL docs  DONE</text>
<text x="343" y="228" style="fill:#888;font-size:11px">底部：上一個動作的結果</text>
<!-- right knobs -->
<circle class="nav" cx="510" cy="50" r="18"/><text class="lab" x="510" y="22">KNOB1</text><text class="fn" x="510" y="84">往回捲</text>
<circle class="ctl" cx="568" cy="50" r="18"/><text class="lab" x="568" y="22">KNOB2</text><text class="fn" x="568" y="84">↑ ↓</text>
<circle class="off" cx="626" cy="50" r="18"/><text class="lab" x="626" y="22">KNOB3</text><text class="lab" x="626" y="84">—</text>
<circle class="nav" cx="684" cy="50" r="18"/><text class="lab" x="684" y="22">KNOB4</text><text class="fn" x="684" y="84">下一個</text>
<!-- buttons row 1 -->
<rect class="ans" x="482" y="104" width="40" height="34" rx="6"/><text class="lab" x="502" y="118">FX</text><text class="fn" x="502" y="133">拒絕</text>
<rect class="ctl" x="526" y="104" width="40" height="34" rx="6"/><text class="lab" x="546" y="118">SEL</text><text class="fn" x="546" y="133">Enter</text>
<rect class="ctl" x="570" y="104" width="40" height="34" rx="6"/><text class="lab" x="590" y="118">ENV</text><text class="fn" x="590" y="133">中斷</text>
<rect class="ctl" x="614" y="104" width="40" height="34" rx="6"/><text class="lab" x="634" y="118">LFO</text><text class="fn" x="634" y="133">mode</text>
<rect class="nav" x="658" y="104" width="40" height="34" rx="6"/><text class="lab" x="678" y="118">EDIT</text><text class="fn" x="678" y="133">細節</text>
<rect class="nav" x="702" y="104" width="40" height="34" rx="6"/><text class="lab" x="722" y="118">GLO</text><text class="fn" x="722" y="133">等你的</text>
<!-- buttons row 2 -->
<rect class="nav" x="482" y="146" width="40" height="34" rx="6"/><text class="lab" x="502" y="160">HOME</text><text class="fn" x="502" y="175">全部</text>
<rect class="ans" x="526" y="146" width="40" height="34" rx="6"/><text class="lab" x="546" y="160">SAVE</text><text class="fn" x="546" y="175">永遠</text>
<rect class="voi" x="570" y="146" width="40" height="34" rx="6"/><text class="lab" x="590" y="160">ARP</text><text class="fn" x="590" y="175">按住說</text>
<rect class="nav" x="614" y="146" width="40" height="34" rx="6"/><text class="lab" x="634" y="160">SEQ</text><text class="fn" x="634" y="175">說明</text>
<rect class="ctl" x="658" y="146" width="40" height="34" rx="6"/><text class="lab" x="678" y="160">PLAY</text><text class="fn" x="678" y="175">繼續</text>
<rect class="ans" x="702" y="146" width="40" height="34" rx="6"/><text class="lab" x="722" y="160">REC</text><text class="fn" x="722" y="175">批准</text>
<text class="lab" x="612" y="206">PLAY 的燈：紅 = 有人在等或出錯，綠 = 都好</text>
<!-- keys -->
<g>
<rect class="key" x="40" y="300" width="38" height="76" rx="8"/><rect class="key" x="82" y="300" width="38" height="76" rx="8"/><rect class="key" x="124" y="300" width="38" height="76" rx="8"/><rect class="key" x="166" y="300" width="38" height="76" rx="8"/><rect class="key" x="208" y="300" width="38" height="76" rx="8"/><rect class="key" x="250" y="300" width="38" height="76" rx="8"/><rect class="key" x="292" y="300" width="38" height="76" rx="8"/><rect class="key" x="334" y="300" width="38" height="76" rx="8"/><rect class="key" x="376" y="300" width="38" height="76" rx="8"/><rect class="key" x="418" y="300" width="38" height="76" rx="8"/><rect class="key" x="460" y="300" width="38" height="76" rx="8"/><rect class="key" x="502" y="300" width="38" height="76" rx="8"/><rect class="key" x="544" y="300" width="38" height="76" rx="8"/><rect class="key" x="586" y="300" width="38" height="76" rx="8"/><rect class="key" x="628" y="300" width="38" height="76" rx="8"/><rect class="key" x="670" y="300" width="38" height="76" rx="8"/>
<rect class="keyb" x="64" y="254" width="30" height="40" rx="8"/><rect class="keyb" x="106" y="254" width="30" height="40" rx="8"/><rect class="keyb" x="148" y="254" width="30" height="40" rx="8"/><rect class="keyb" x="232" y="254" width="30" height="40" rx="8"/><rect class="keyb" x="274" y="254" width="30" height="40" rx="8"/><rect class="keyb" x="358" y="254" width="30" height="40" rx="8"/><rect class="keyb" x="400" y="254" width="30" height="40" rx="8"/><rect class="keyb" x="442" y="254" width="30" height="40" rx="8"/><rect class="keyb" x="526" y="254" width="30" height="40" rx="8"/><rect class="keyb" x="568" y="254" width="30" height="40" rx="8"/><rect class="keyb" x="652" y="254" width="30" height="40" rx="8"/>
</g>
<text class="lab" x="59" y="392">F3</text><text class="lab" x="689" y="392">G5</text>
<text class="fn" x="380" y="392">每個琴鍵 = 一個 agent：按下去選它，燈 = 它急不急</text>
</svg>
<figcaption>琥珀：回答 agent；藍：操作；灰：移動與頁面；綠：語音；虛線：沒用到</figcaption>
</figure>

## 琴鍵的燈

| 燈 | 意思 | 螢幕色 |
|---|---|---|
| 熄 | 這個鍵沒有 agent | — |
| 微亮 | IDLE，閒著 | 白 |
| 呼吸（慢慢亮暗） | WORKING | 藍 |
| **2 Hz 閃** | **BLOCKED：等你批准或回答** | 琥珀 |
| 全亮 | DONE，還沒看；選過之後轉微亮 | 綠 |
| 雙閃 | ERROR | 紅 |
| 中亮 | UNKNOWN | 灰 |

全部退成微亮、右上角 `NO HOST`：daemon 斷了，燈的狀態不可靠。提示音：上行兩音 = 有人在等，三音和弦 = 完成，下行兩音 = 出錯。

## 回答 agent

先選 agent（按它的琴鍵，或轉 SELECT），再按：

| 按 | 動作 | 只在 |
|---|---|---|
| `REC` | 批准（Enter，選預設的 Yes） | BLOCKED |
| `SAVE` | 批准且不再問 | BLOCKED，且設定了按鍵（Claude、Codex） |
| `FX` | 拒絕（Esc） | BLOCKED |
| `ENV` | 中斷這一輪 | 任何時候 |
| `SEL` | Enter | 任何時候 |
| `PLAY` | 送 `continue` | IDLE、DONE |
| `LFO` | 切 mode（Shift+Tab） | 任何時候 |
| KNOB 2 | 選單裡上下移動 | 任何時候 |

選取的 agent 在等你時，`REC` 會閃、`FX` 會亮。daemon 送鍵前會再確認一次那個鍵上還是同一個 agent、它真的在等；不是的話底部會顯示 `CHANGED, LOOK AGAIN` 或 `NOT WAITING`，什麼都不送。

## 移動

| 按 | 動作 |
|---|---|
| 琴鍵 | 選那個 agent |
| SELECT | 在這頁列出的 agent 之間上下移 |
| KNOB 4 | 跳到下一個需要你的：BLOCKED → ERROR → 還沒看的 DONE |
| `HOME` | 全部 agent |
| `GLO` | 只列在等你的 |
| `EDIT` | 選取 agent 的輸出（見下面）；classic Herdr 下同時在電腦上 focus 那個 pane |
| KNOB 1 | 在輸出裡往回捲（逆時針），一格 4 行；換 agent 會回到最底 |
| `SEQ` | 裝置上的說明頁；再按一次回原本的頁 |
| ALGORITHM | 四頁輪流：全部、輸出、等你的、說明 |
| `OCT+` | 藍牙開 / 關（到下次開機為止，不存） |

### 看 agent 在做什麼 { #detail }

`EDIT` 顯示選取 agent 畫面上的對話，最後 8 行：

- daemon 會先去掉 agent 自己的介面：輸入框、狀態列、`bypass permissions` 這類提示。權限選單（`❯ 1. Yes`…）會留著，那正是要你回答的。
- 中文、日文和符號由電腦用系統字型畫成點陣圖再送過去（韌體 0.2 起），裝置本身的字型只有 ASCII。舊韌體收到的是 ASCII 文字，中文會變成 `?`。
- 要看更前面的內容，逆時針轉 KNOB 1。

## 語音輸入：按住 `ARP` { #voice }

**按住 `ARP` 說話，放開結束。** daemon 在這台 Mac 上模擬按住 `Ctrl`+`Option`，和 Wispr Flow 的 push-to-talk 一樣。放開後辨識出來的文字會打進目前 focus 的視窗；能 focus 的 backend（classic Herdr）會先把選取 agent 的 pane 叫到前面。

設定：

1. Wispr Flow（或 Handy）的快捷鍵設成 `Ctrl`+`Option`（或改下面的 `voice_keys` 去配合它）。
2. 跑 daemon 的那個 terminal app 要有**輔助使用**權限：系統設定 → 隱私權與安全性 → 輔助使用。沒有的話 daemon 啟動時會提醒，按鍵也送不到。
3. 別的組合寫在 `~/.config/conductor/config.json`：

```json
{"voice_keys": ["ctrl", "option"], "voice_mode": "hold"}
```

`voice_mode` 是 `hold`（按住說、放開停）或 `toggle`（按一下開始、再按一下停，給用切換式快捷鍵的 app）。按鍵名稱：`ctrl`、`option`、`cmd`、`shift`、`fn`、右側的 `rctrl` / `ropt` / `rcmd` / `rshift`，以及 `space`、`f13`–`f19` 等。不想要這個功能：`just cond-run --no-voice`。

安全：按住超過 120 秒、裝置重開、daemon 結束時，daemon 一定會放開按鍵，不會讓 `Ctrl` 卡住。

## 藍牙 { #bluetooth }

韌體 0.3 起，USB 和藍牙都能接 daemon。藍牙每次開機都會打開，`OCT+` 可以關掉（到下次開機為止）。

1. 第一次：Mac 的「音訊 MIDI 設定」→ 選單「視窗 → 顯示 MIDI 工作室」（`⌘2`）→ 工具列的藍牙圖示 → 找到 `FM-1 XXXX`（我的是 `FM-1 3399`，每台固定），按「連接」。不需要配對碼。注意：預設打開的是「音訊裝置」視窗，那裡和系統設定的「聲音」都不會列 MIDI 裝置。
2. 跑 `just cond-run`。daemon 自己找到藍牙那個 port；USB 也插著時優先用 USB。
3. 標頭右上角：沒有 host 時，`BLE` 灰色 = 廣播中、藍色 = 電腦已連上；有 host 時出現藍色 `BLE` = 正在走藍牙。

斷開與移除：

| 想做的事 | 怎麼做 |
|---|---|
| 斷開連線 | 同一個藍牙設定視窗按 `FM-1 XXXX` 旁的「中斷連線」；或在 FM-1 按 `OCT+` 關掉藍牙（再按一次會重新廣播） |
| 「取消配對」 | 不需要：Conductor 不配對，系統設定的藍牙清單裡不會有它 |
| 移除 MIDI 工作室裡的圖示 | 斷開後，選灰色的 `FM-1 XXXX` 圖示，按工具列的「−」（或 Delete）。下次連接會再出現 |
| port 名稱還是舊韌體的 `Felucca` | FM-1 拔掉 USB 時，在 MIDI 工作室刪掉灰色的 `Felucca` / `FM-1` 圖示，再插上就會用現在的 USB 名稱 `[調研]` |

走藍牙時，`EDIT` 的輸出只送 ASCII 文字（點陣圖一行 558 bytes，藍牙太慢），中文會是 `?`。要看中文就插 USB。

## 沒有 FM-1 時：文字版

`just cond-sim`（或 `just cond-sim auto`）用 terminal 代替裝置：

| 打 | 等於 |
|---|---|
| `sel N` | 按第 N 個琴鍵（0 = F3） |
| `approve` / `always` / `deny` | `REC` / `SAVE` / `FX` |
| `int` / `enter` / `cont` | `ENV` / `SEL` / `PLAY` |
| `focus` | `EDIT` 的 focus |
| `voice` / `unvoice` | 按下 / 放開 `ARP` |
| `show` | 看細節的 8 行 |
| `quit` | 離開 |

## 和 Agent Deck 的差別

按鈕位置大致相同，換過來不用重記。不同的地方：

- shell 不佔琴鍵；一個 agent 固定一個鍵，沒有換頁。
- 沒有 auto-approve、prompt preset、`GLO` + 按鈕的「全部一起」；`ARP` 改成語音。
- 提示音與 `PLAY` 的總覽燈是新的。
