# Flash 配置圖

FM-1 的 flash 是 1 MiB，分成 256 個 4 KiB 的 sector。誰把什麼放在哪裡，決定了換韌體時什麼會留下、什麼會被蓋掉。這頁把各個來源的說法和我自己備份的比對放在一起。

## 總覽

| 位址 | 內容 | 來源 |
|---|---|---|
| `0x00000`–`0x03FFF` | **boot head**：bootloader 與 boot 設定（16 KiB）。壞了就開不了機 | `[上游]` M-Flasher、Unbricker |
| `0x04000`–`0x92FFF` | **韌體區**。多數韌體（原廠 V15、SLOOP、Agent Deck、CTL-1）佔這段 | `[實機]` M-Flasher 的寫入計畫 |
| `0x93000`–`0xE8FFF` | 原廠 V15 的**設定區**。社群韌體也把資料放在這裡面 | `[上游]` |
| `0xE9000` | Bluetooth 資料（配對） | `[上游]` |
| `0xEA000`–`0xFBFFF` | 原廠音色（factory voices） | `[上游]` |
| `0xFC000`–`0xFEFFF` | M-Flasher 的圖上標為 free；Felucca 系拿來放全域設定 | `[實機]` |
| `0xFF000`–`0xFFFFF` | key / MAC，裝置自己的資料 | `[上游]` |

M-Flasher 的 flash map 用顏色標的就是這些：BOOT `00–03`、FIRMWARE `04–92`、SETTINGS `93–E8`、PAIRINGS `E9`、SOUNDS `EA–FB`、free `FC–FE`、MAC `FF`。

## 韌體區不一定只到 `0x92FFF`

| 韌體 | 佔用範圍 | 來源 |
|---|---|---|
| SLOOP 2.8.5、Agent Deck 0.7、CTL-1 1.0.3 | `0x04000`–`0x92FFF`（實際寫入到 `0x8FFFF`） | `[實機]` |
| Conductor 0.1（自製，fm1-base） | `0x04000`–`0x92FFF`（實際寫入 `0x04000`–`0x8EFFF`、`0x91000`–`0x91FFF`） | `[實機]` M-Flasher 的計畫：140 sectors |
| Morse 0.1 / 0.2（自製，fm1-base） | `0x04000`–`0x92FFF`（實際寫入：Conductor 0.3 → 0.1 是 `0x04000`–`0x21FFF`，0.1 → 0.2 是 `0x04000`–`0x23FFF`） | `[實機]` M-Flasher 的計畫：30、32 sectors |
| **ChoralRoot 0.41** | `0x04000`–**`0x9EFFF`** | `[實機]` M-Flasher 的計畫：152 sectors |
| **原廠 V16** | 到 **`0xA8FFF`** | `[上游]` Unbricker |

超出 `0x92FFF` 的韌體會直接蓋掉別人放在 `0x93000` 之後的資料。

## Felucca 系韌體的資料位置

Felucca、SLOOP、ChoralRoot 以及 Dinghy、fm1-base 系的韌體大多沿用同一套位址 `[上游]`：

| 位址 | 內容 |
|---|---|
| `0x97000` 起 | project（曲子） |
| `0x9F000` | SLOOP 的 autosave（和 ChoralRoot 的 FM6 bank 是同一個物件編號） |
| `0xA0000` 起 | samples |
| `0xDA000`–`0xDBFFF` | 自製 [Morse](../dev/morse.md#flash) 的設定與學習進度（這台機器的備份裡這兩個 sector 都是空的） |
| `0xDC000` 起 | user preset bank；CTL-1 與 fm1-base 的 app（含 Conductor）用來放面板校正 |
| `0xDE000` | CTL-1 的 Bluetooth 設定 |
| `0xE0000` | 更新時暫存 update loader |
| `0xF2000`–`0xF3FFF` | RemoteFM1 第一次開 Bluetooth 時寫入 |
| `0xFC000` | 全域設定。**SLOOP、Agent Deck、CTL-1 都寫這裡** |
| `0xFE000` | SLOOP autosave 的另一份 |

因為位址共用，韌體之間會互相踩到對方的資料，也會讀到對方留下的東西。

## 我的機器上實際發生的 `[實機]`

用各階段的完整備份逐 sector 比對：

| 動作 | 改變的 sector |
|---|---|
| 原廠 015 → 裝 SLOOP 並玩過 | 韌體區，加上 `0x97000`、`0x9F000`、`0xFC000`、`0xFD000`、`0xFE000` |
| → Agent Deck | 韌體區，加上 `0xFC000`（蓋掉 SLOOP 的設定） |
| → 回 SLOOP | 韌體區，加上 `0xFE000`。曲子還在 |
| → CTL-1 | 韌體區，加上 `0xDE000`、`0xDF000`、`0xFC000`、`0xFD000` |
| → ChoralRoot | `0x04000`–`0x9EFFF`，蓋掉了 `0x97000` 的 SLOOP project（已執行，尚未用備份比對確認其他 sector） |
| → Conductor | 韌體區 140 個 sector（`0x04000`–`0x8EFFF`、`0x91000`–`0x91FFF`）。開機後不寫任何資料（預演 `[模擬]`；實機尚未用備份比對） |

原廠出廠時 `0x93000`、`0xE9000`–`0xF2000`、`0xFA000`、`0xFB000` 有資料，`0xFC000`–`0xFE000` 是空的。

## 自己比對兩份備份

```python
S = 4096
a = open("backups/A/x.bin", "rb").read()
b = open("backups/B/y.bin", "rb").read()
print([hex(i * S) for i in range(256) if a[i * S:(i + 1) * S] != b[i * S:(i + 1) * S]])
```

## 這對操作的意義

- **只重裝韌體會保留資料區**，所以 SLOOP ↔ Agent Deck 來回曲子還在。
- **韌體比較大的（ChoralRoot、V16）會吃掉資料區**，回去時要還原備份。
- **共用的 `0xFC000` 每換一次就被蓋一次**，各韌體的全域設定不會跨韌體保留。
- **原廠音色在 `0xEA000`–`0xFBFFF`**，有些韌體會寫這一段。只有原廠備份救得回來。
- 更完整的後果見[變磚的原理與分級](../getting-started/bricking.md)與[多韌體切換](../firmware/switching.md)。
