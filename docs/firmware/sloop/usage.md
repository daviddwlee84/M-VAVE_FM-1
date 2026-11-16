# SLOOP 操作速查

整理自上游 README，對應 2.8。我自己的心得放在最下面。

## 60 秒做出第一個 beat

1. **`ALGORITHM`** 轉到 track **4**（橘色，drums）。白鍵是 16 個鼓聲；**`PRESETS`** 選 kit（試試 *808* 或 *BOOMBAP*）。
2. 按 **`REC`**，照自己的速度隨意打一段。打的時候按住 **`OCT-`** 是 ghost note，**`OCT+`** 是重擊。
3. **最後一小節結束後的第一拍按 `REC`。** loop 收起來，它的長度決定 tempo，打擊點對齊格線，立刻開始播放。
4. 播放中再按 **`REC`**：疊錄。按住 **`ARP`** 再按住 hi-hat 鍵是 hat roll。
5. **`ALGORITHM`** 轉到 track **1**，**`REC`**，彈一段 bass。按住 **`SEL`** 再按這首歌的調；在 track 2 按住 `SEL` 把 **KNOB 1** 轉到 *7TH*，每個白鍵就變成一個和弦。
6. 按住 **`FX`** 再按任一鍵是 punch-in effect；繼續按著 `FX`，轉 **KNOB 2** 是 DUST，**KNOB 3** 是 DUCK。
7. 按錯了？按住 **`EDIT`** 再按 **`OCT-`**：undo。

想要固定 tempo 或預備拍：開始前在 REC 畫面轉 KNOB 1 與 KNOB 3。

## 控制項

| 控制 | 作用 |
|---|---|
| **MASTER** | 音量 |
| **SELECT** | HOME 與 layer 內：tempo；頁面上：同組的上 / 下一頁；DRUMS：grid / kit |
| **ALGORITHM** | 選 track：1、2、3（synth）、4（drums） |
| **PRESETS** | 選取 track 的音色，或 drum kit |
| **KNOB 1–4** | 螢幕底部四個 dial 顯示的東西，各有顏色 |
| **`OCT-` / `OCT+`** | 八度（同時按：歸零）；drum track 上按住：ghost / 重擊 |
| **`FX` `SEL` `ENV` `LFO` `EDIT` `GLO`** | 輕點：開頁面；按住 `FX`、`SEL`、`EDIT`、`GLO`：layer |
| **`HOME`** | TRACKS 畫面；按住：選單；layer 按住時點它：鎖定 layer |
| **`SAVE`** | TRACKS 上：SONG 畫面；其他地方：SAVE 頁面；按住：song layer |
| **`ARP` `SEQ`** | 輕點：頁面；按住：note repeat / steps |
| **`PLAY`** | 四軌一起開始 / 停止；燈號每拍閃一次 |
| **`REC`** | 播放中：立刻錄 / 停；停止時：arm；按住約 2 秒：清除 track |
| **`EDIT` + `OCT-` / `OCT+`** | undo / redo |

`SEL` 是上排第二顆鍵（`FX` 與 `ENV` 之間），不是 SELECT 旋鈕。

## Layers

按住功能鍵時，白鍵與旋鈕的工作：

| 按住 | 白鍵 | KNOB 1 · 2 · 3 · 4 |
|---|---|---|
| **`FX`** punch | 27 個 punch-in effect（黑鍵也有） | FILTER · DUST · DUCK · track filter |
| **`EDIT`** erase | 播放中擦掉某個聲音或音符；黑鍵：即時 drum mute | SHIFT · LENGTH ×2 / ½ · TRANSPOSE |
| **`ARP`** roll | note repeat，會錄成 ratchet | RATE |
| **`SEQ`** steps | 該頁的 16 個 step；按住某 step 可設 level、ratchet、nudge、parameter lock、fill 條件 | SOUND / NOTE · DIV · SWING · LENGTH |
| **`SEL`** key | 歌曲的調 | CHORD · SCALE · KEYS · TRANSPOSE |
| **`GLO`** mix | 1–4 mute、5–8 solo、9 fill、10 下一小節 fill、16 tap tempo | track 1–4 的音量 |
| **`SAVE`** song | 1–4 播放 section A–D、5–8 把 loop 存進去、13 loop / song、14 錄下 song、16 chain | — |

## 存檔與歌曲

- **Autosave**：停止後 2.5 秒自動保存，開機回到上次狀態。
- **Section A–D**：`SAVE` + 鍵 5–8 存入，`SAVE` + 鍵 1–4 在下一小節切換播放。按著 `SAVE` 連點幾個 section 是 quick chain。
- **錄下整首歌**：`SAVE` + 鍵 14。song mode 下 `PLAY` 播整條 chain。
- 機器上只放一首歌，多首用 [Web Editor](web-editor.md) 的 backup 檔。

## 選單（按住 `HOME`）

四個區：**SCREEN**（COLOR、ZOOM、KNOB ACCEL）、**LIGHTS**（LIGHTS、KEYS、NOTES）、**AUDIO**（LOWCUT、USB AUDIO、USB RATE）、**SYSTEM**（HARDWARE CALIBRATION、ABOUT）。`SELECT` 換區，KNOB 1–3 設定該區各列，`PRESETS` 移動游標，`OCT+` 進入，`OCT-` 關閉。

這些是機器的設定，不屬於 project：載入 project 不會改它們，backup 會包含它們。

- 黑色機身看不清楚標示：LIGHTS 調亮按鍵背光。
- 版本：SYSTEM → ABOUT。

## 常見狀況

| 現象 | 原因與處理 |
|---|---|
| synth track 黑鍵沒聲音 | 該 track 在 chord 或 scale 模式。按住 `SEL`，KNOB 1 CHORD 設 OFF、KNOB 3 KEYS 設 OFF |
| 錄進去的音被對齊到格線 | 依 track 的 DIV 量化。REC 畫面把 **QUANT** 關掉（KNOB 4），或把 DIV 設成 1/32 |
| 音多的時候有音淡出 | CPU 到極限，SLOOP 一次淡掉一個 voice。減少同時按住的音或換輕一點的 engine |
| MIDI IN 沒反應 | 換另一種 TRS 轉接型式（A / B），檢查 channel |
| 沒有 USB audio 輸入 | 拔插一次。見[接口與連線](../../hardware/io.md) |
| installer / editor 找不到機器 | Chrome 或 Edge、資料線、不經 hub、允許 MIDI；關掉其他用 MIDI 的程式再重新整理 |

## 我的心得

還沒有。實際玩過之後記在[使用心得](../../notes/index.md)，穩定下來的結論再搬到這裡。
