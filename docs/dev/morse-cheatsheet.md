# 摩斯電碼機操作速查

韌體 0.2 / 0.3（機器上還是 0.1 的話，差別見文末）。設計與原理見[摩斯電碼機](morse.md)。

## 任何頁面

| 控制 | 作用 |
|---|---|
| `HOME` | 叫出選單；在選單裡再按一次回到上一頁 |
| `ALGO` 旋鈕 | 直接換頁 |
| `FX` | 聲音開／關。關掉時 `FX` 燈亮，改看琴鍵燈和畫面上的燈號 |
| `K1` | 聽的速度：字元 5–40 WPM（預設 20） |
| `K2` | 間隔（Farnsworth）：字和字之間拉開，5 到字元速度（預設 8） |
| `K3` | 音高 300–1200 Hz（預設 600）。收到的訊號高一個大三度 |
| `K4` | 發報速度 5–40 WPM（預設 10）：keyer 的節奏、解碼器的起點、RHYTHM 的目標 |
| `MASTER` | 音量 |

轉速度時畫面上方會顯示它的意思，例如「SEND 10 WPM DIT 120MS DAH 360MS」。設定改完 3 秒後自動存。不知道怎麼設的話，到 SETTINGS 選 PRESET，見[速度](morse.md#speed)。

## PRACTICE（閃卡）

| 控制 | 作用 |
|---|---|
| `PLAY` | 開始一回合；題目播完後再按是重播 |
| `E4` `F4` `G4` `A4` | 四選一的題目：四個亮燈的鍵對應畫面上四個選項（由左到右） |
| `SELECT` + `SEL` | 也可以轉 `SELECT` 選、按 `SEL` 確定（閃爍的是目前選的） |
| 琴鍵（電鍵） | 「打出來」的題目：畫面出現 `your turn` 後照設定的 keyer 打 |
| `SEL` | 打的題目不會：算錯，顯示答案 |
| 任一琴鍵 | 新字：看完聽完，下一張；答案畫面：不等，直接下一張 |
| `HOME` | 離開，已答的部分會存 |

## LISTEN（收聽）

| 控制 | 作用 |
|---|---|
| `PLAY` | 開始／停止 |
| `SELECT` | 換節目：KOCH GROUPS、WORDS、QSO、ABBREVIATIONS、SAYINGS |
| `SEQ` | 顯示／隱藏正在播的字（隱藏時這一行播完才顯示） |
| `SEL` | 隱藏時偷看這一行 |

## KEY（自由發報）

| 控制 | 作用 |
|---|---|
| 琴鍵 | 直鍵：任一鍵。iambic：左半邊 F3–F4 是 dit，右半邊 F#4–G5 是 dah（SETTINGS 可對調） |
| `SEQ` | 換顯示：TEXT（字）→ CODE（每個字下面是你打的點劃）→ TIMING（每一下照長短畫成條） |
| `OCT-` | 清掉畫面上的字 |

白字是自己發的，琥珀色是收到的（MIDI 或電腦送來的文字）。上方顯示解碼器聽到的速度、dah:dit 比例（標準 3.0）和字內停頓（GAP，標準 1.0）；偏離太多時變成琥珀色。TIMING 檢視最上面的灰條是目標 dit 和 dah 的長度，紅線是會把一個字切成兩個的長停頓。接著電腦時，按鍵會經 USB 送出（Vail 格式），解出的字也會送給 host。

## RHYTHM（練長短）

| 控制 | 作用 |
|---|---|
| 琴鍵 | 照目標打 |
| `SELECT` | 換練習：DITS、DAHS、DIT DAH、SOS、FREE |
| `PLAY` | 重來（清掉統計） |
| `K4` | 改目標速度 |

框是目標長度，條是你按的長度：綠是好、黃是偏了、紅是變成另一種了。下面的提示看最近 16 下。

## TYPE（字母鍵盤）

| 控制 | 作用 |
|---|---|
| `OCT-` / `OCT+` | 翻頁：字母 → 數字和符號 → 片語 |
| `REC` | 刪最後一個字；按住 0.6 秒清空整行 |
| `PLAY` | 播出整行；正在播時按是停止 |
| `SEL` | LIVE（按一個字就播出、送出）／COMPOSE（打字時不出聲，`PLAY` 才送整行） |
| `SEQ` | 鍵上顯示點劃（查表用）。0.3 起打開時鍵盤會變高、上方只留一行，碼和字母都看得到 |

預設排列（SETTINGS 的 TYPE KEYS 可改成「A–Z 由左到右」或「Koch 順序」）：

| 頁 | 白鍵 | 黑鍵 |
|---|---|---|
| 字母 | A B C D E F G H I J K L M N O P | Q R S T U V W X Y Z，最右邊是空白 |
| 數字和符號 | `1 2 3 4 5 6 7 8 9 0 . , ? / = +` | `- ( ) ' " : ; @ ! & $` |
| 片語 | CQ DE K `<KN>` `<AR>` `<SK>` `<BT>` 73 TU RST 5NN QTH NAME OP UR ES | GM GE FB R QRZ? QSL `<BK>` HW? PSE AGN QRS |

片語的意思會顯示在畫面上，也列在[片語表](morse.md#phrases)。

## CHART（字表）

| 控制 | 作用 |
|---|---|
| `SELECT` | 移動 |
| `PLAY` / `SEL` | 聽這個字 |

顏色：深紅是剛學、黃是一半、綠是熟了（兩張卡盒號的平均）；幾乎全黑的格子是還沒開的字。

## SETTINGS

`SELECT` 選項目，`OCT-` / `OCT+`（或 `SEL`）改值。

| 項目 | 值 |
|---|---|
| PRESET | START、BUILDING、QSO、FAST；速度和這幾組都不同時顯示 CUSTOM |
| HEAR SPEED | 聽的字元速度（同 K1） |
| SPACING | 間隔（同 K2） |
| SEND SPEED | 發報速度（同 K4） |
| PITCH | 音高 |
| KEYER | STRAIGHT、IAMBIC A、IAMBIC B |
| PADDLES | LEFT DIT（左 dit 右 dah）或 LEFT DAH |
| SOUND | ON 或 OFF（看燈） |
| CARDS | 每回合 10–30 張 |
| LEARN BY | SOUND（預設，建議）或 SIGHT（顯示點劃）。原因見[先用聽的](morse.md#by-sound) |
| TYPE KEYS | TYPE 頁的排列 |

## 燈

| 燈 | 意思 |
|---|---|
| 所有琴鍵全亮／半亮 | 自己在發／收到訊號（靜音時就看這個） |
| 四個琴鍵亮、一個閃 | 四選一的選項，閃的是 `SELECT` 選中的 |
| `FX` 亮 | 靜音 |
| `PLAY` 綠 | 電腦端程式連著 |

## 電腦端

```sh
just morse-type            # 解出的字打進 Mac 目前的視窗（要 Accessibility 權限）
just morse-type --dry-run  # 只印出來
just morse-send cq cq de fm1 k   # FM-1 當成收到的播出來（"73 <SK>" 這樣的 prosign 0.2 起可以）
```

## 0.1 和 0.2 的差別

0.1 只有一個速度（K1、K2），沒有 LISTEN、RHYTHM 和 PRESET。TYPE 頁要按住 `OCT-` 才是數字和符號，`OCT+` 是刪字，沒有清空，`PLAY` 也停不下來。
