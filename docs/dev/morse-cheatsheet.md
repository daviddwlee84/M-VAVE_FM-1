# 摩斯電碼機操作速查

韌體 0.8（0.3 以前沒有的操作標了「0.4」，藍牙是「0.5」，開機頁是「0.7」，`SAVE` 是「0.8」；0.1 和 0.2 的差別見文末）。英文版、可以印成 A4 的單頁：[MORSE cheatsheet](https://daviddwlee84.github.io/fm1-morse/web/cheatsheet.html)（Hub 卡片的 cheatsheet 連到這裡）；沒有 FM-1 也能[在瀏覽器裡試](https://daviddwlee84.github.io/fm1-morse/web/try/)。學習的步驟見[用 MORSE 學摩斯](morse-learn.md)。設計與原理見[摩斯電碼機](morse.md)。

## 任何頁面

| 控制 | 作用 |
|---|---|
| `HOME` | 叫出選單；在選單裡再按一次回到上一頁。0.7 起開機先進選單（SETTINGS 的 START PAGE 可以改），這時按 `HOME` 進 PRACTICE |
| `ALGO` 旋鈕 | 直接換頁 |
| `SAVE` | 0.8：設定立刻存。沒按的話，最後一次改動 3 秒後才自動存，這之前關機改動就沒了。有還沒存的設定時 `SAVE` 燈亮、頁名後面有琥珀色 `*` |
| `FX` | 聲音開／關。關掉時 `FX` 燈亮，改看琴鍵燈和畫面上的燈號 |
| `K1` | 聽的速度：字元 5–40 WPM（預設 20） |
| `K2` | 間隔（Farnsworth）：字和字之間拉開，5 到字元速度（預設 8） |
| `K3` | 音高 300–1200 Hz（預設 600）。收到的訊號高一個大三度 |
| `K4` | 發報速度 5–40 WPM（預設 10）：keyer 的節奏、KEY 頁解碼器的起點、RHYTHM 的目標（0.4 起 PRACTICE 的打碼卡不看它） |
| `MASTER` | 音量 |

轉速度時畫面上方會顯示它的意思，例如「SEND 10 WPM DIT 120MS DAH 360MS」。旋鈕和 SETTINGS 的改動都是**馬上生效**；存進 flash（關機再開還在）是另一件事：最後一次改動 3 秒後自動存，或按 `SAVE`（0.8）立刻存。等 3 秒是為了轉旋鈕時不要每一格都寫一次 flash。不知道怎麼設的話，到 SETTINGS 選 PRESET，見[速度](morse.md#speed)。

## PRACTICE（閃卡）

| 控制 | 作用 |
|---|---|
| `PLAY` | 開始一回合；題目播完後再按是重播 |
| `SELECT`（開始畫面） | 0.4：切換「一回合」和「TEST 分級測驗」（下兩組各考一次，答對的可以標成已會） |
| `E4` `F4` `G4` `A4` | 四選一的題目：四個亮燈的鍵對應畫面上四個選項（由左到右） |
| `SELECT` + `SEL` | 也可以轉 `SELECT` 選、按 `SEL` 確定（閃爍的是目前選的） |
| 琴鍵（電鍵） | 「打出來」的題目：畫面出現 `your turn` 後照設定的 keyer 打。0.4 起看你自己的節奏（劃約 3 個點長），快慢都可以 |
| `SEL` | 打的題目不會：算錯，顯示答案 |
| 任一琴鍵 | 新字：看完聽完，下一張；答案畫面：不等，直接下一張 |
| `PLAY`（新字） | 再聽一次（沒聽清楚時） |
| `SEQ` | 0.4，SOUND 模式：新字和答案畫面偷看點劃，換卡就藏回去。「聽了選」的題目不能看 |
| `HOME` | 離開，已答的部分會存 |

0.4 打碼卡的結果頁：`RIGHT`／`ALMOST`／`NO`、評級（GREAT／GOOD／CLOSE／OFF 和分數）、`READ AS` 解碼器讀成的字、SHOULD（標準節奏）對 YOU（你按的）的長短條，判反的那一下是紅色，下面一行是要改什麼。ALMOST 不升盒也不歸零，回合最後再問一次。開始畫面的 `NEXT GROUP WAITS ON` 是擋住下一組的字。判定細節見[打碼卡怎麼判](morse.md#judging)。

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
| `REC` | 0.4：進入 EDIT（改的是副本，標題顯示 `EDIT *`） |

EDIT 裡（0.4）：

| 控制 | 作用 |
|---|---|
| `OCT-` / `OCT+`、`K1` | 這個字的等級 0–5；0 是「忘了」，會重新當新字出現 |
| `K2` | 開放的組數 ±1 |
| `FX` | 全部退一盒（生疏時） |
| 按住 `SEL` 2 秒 | 全部歸零 |
| `PLAY` / `REC` | 問「SAVE CHANGES?」：再按 `PLAY` 存，`HOME` 不存 |

任何改進度的問題（SAVE CHANGES?、MARK THESE KNOWN?、LOAD THIS BACKUP?）都只認 `PLAY`（是）和 `HOME`（否），其他鍵沒作用。

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
| JUDGING | 0.4：BY RHYTHM（預設，看自己的長短比例）或 STRICT（照發報速度，0.3 的判法） |
| TYPE KEYS | TYPE 頁的排列 |
| BLUETOOTH | 0.5：ON（預設）或 OFF，關機後保留。旁邊會寫 NOT CONNECTED、CONNECTED，或 TALKING（電腦端程式或網頁正透過藍牙連著）。項目超過一頁，往下轉會捲動 |
| START PAGE | 0.7：開機進哪一頁：MENU（預設）、PRACTICE、LISTEN、KEY、RHYTHM、TYPE、CHART。0.6 以前固定進 PRACTICE。不是 MENU 時不要直接降回 0.6（見[備份與還原](morse.md#backup)） |

畫面右上角：`PC` 代表有會回應 MORSE 的程式連著（`morse` 指令列或 MORSE on the web，USB 或藍牙都算；Vail 只收按鍵、不回應，所以不會亮），0.6 起拔線後半秒內消失、走藍牙的在藍牙斷線時消失，其他情況是 10 秒沒消息才消失；`BT`（0.5）灰色代表藍牙開著、等電腦連上，藍色代表已經連上；`MUTE` 代表靜音。

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
just morse-backup progress.json  # 0.4：學習進度存成 JSON（FM-1 上什麼都不變）；0.7 起連設定一起
just morse-restore progress.json # 0.4：載回去；FM-1 會先問，按 PLAY 才載入；檔案有設定、韌體 0.7 以上時設定也換
PYTHONPATH=host uv run python -m morse --bluetooth send cq   # 0.5：先打開 Mac 的藍牙 MIDI 視窗，等 FM-1 連上
```

0.5 起不接線也行：Mac 的「音訊 MIDI 設定」→ 顯示 MIDI 工作室 → 藍牙 → `FM-1 XXXX` 的 Connect。FM-1 重開後要再按一次（見[藍牙](morse.md#bluetooth)）。

不想裝 Python 的話，用 Chrome 或 Edge 開 [MORSE on the web](https://daviddwlee84.github.io/fm1-morse/web/)：看 FM-1 讀到的字、送字給它播、備份和還原，都不用安裝。

## 0.1 和 0.2 的差別

0.1 只有一個速度（K1、K2），沒有 LISTEN、RHYTHM 和 PRESET。TYPE 頁要按住 `OCT-` 才是數字和符號，`OCT+` 是刪字，沒有清空，`PLAY` 也停不下來。
