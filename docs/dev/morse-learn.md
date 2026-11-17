# 用 MORSE 學摩斯

MORSE 是這個 repo 自己寫的 FM-1 韌體（[fm1-morse](https://github.com/daviddwlee84/fm1-morse)，GPL-3.0-or-later）。目標是把 FM-1 變成隨身的摩斯練習機，用起來像背單字或學五十音：每天幾分鐘的閃卡，加上通勤時的背景收聽。進階的單字、呼號、通聯練習交給 [Vail](https://vailmorse.com/)，FM-1 當它的電鍵。

這頁講怎麼用它學。每個按鍵的作用見[操作速查](morse-cheatsheet.md)，設計和原理見[摩斯電碼機](morse.md)。本頁寫的是韌體 0.4。

![PRACTICE 的開始畫面](https://raw.githubusercontent.com/daviddwlee84/fm1-morse/main/docs/screens/boot.png){ width="240" }
![一張打碼卡的結果](https://raw.githubusercontent.com/daviddwlee84/fm1-morse/main/docs/screens/result_keyed_wrong.png){ width="240" }
![CHART](https://raw.githubusercontent.com/daviddwlee84/fm1-morse/main/docs/screens/chart.png){ width="240" }

## 安裝

- **從 release 裝**：到 [fm1-morse 的 Releases](https://github.com/daviddwlee84/fm1-morse/releases) 下載 `morse-X.Y.fwsc`，用 M-Flasher 或 fm1-base 的 `fm1_install.py` 透過 USB-MIDI 安裝。先備份整片 flash。
- **在這個 repo 裡**：`just fw-build morse` 之後照[切換韌體](../firmware/switching.md)的流程寫入。每一次寫入都要本人核准。

## 第一次

1. 開機後直接進 PRACTICE。到 SETTINGS 確認 **PRESET 是 START**：字元用 20 WPM 播，字和字之間拉開到 8 WPM（Farnsworth）；發報速度是 10 WPM。
2. **LEARN BY 保持 SOUND。** 畫面不畫點劃，每個字當成一個聲音來記。先看圖學的人，之後聽的時候會習慣數點劃，速度容易卡在 10–15 WPM（原因見[先用聽的](morse.md#by-sound)）。
3. 不想出聲的場合按 `FX` 靜音，琴鍵燈會照節奏閃。

## 每天：PRACTICE 一回合

一回合 20 張卡，大約 2–5 分鐘，`PLAY` 開始。

- **新字**：一次最多 3 個，先播兩次給你聽，然後才考。沒聽清楚按 `PLAY` 再聽。真的分不出來（S ··· 和 H ····、U 和 V 這種只差一下的），按 `SEQ` 看一眼點劃，換下一張就藏回去。先聽、不確定才看，記住的才會是聲音。
- **聽了選**：四個亮燈的鍵（`E4` `F4` `G4` `A4`）對應畫面上四個選項。
- **聽了照打、看字打碼**：畫面出現 `your turn` 後，用任一個琴鍵當直鍵打。照聽到的節奏打就好，快慢都可以，重點是**劃大約 3 個點長，字裡的停頓大約 1 個點長**。不會就按 `SEL`。
- **結果頁**：打碼卡會顯示評級（GREAT／GOOD／CLOSE／OFF），以及兩列長短條。上面的 SHOULD 是照你這次的速度排出的標準節奏，下面的 YOU 是你實際按的。判反的那一下標紅，下面一行告訴你要改什麼，例如「dahs short: hold a dah for 3 dits」。
- **差一點（ALMOST）**：長短比例很接近界線時算 ALMOST，不升級也不歸零，回合最後會再問一次。
- **進度怎麼往前**：每個字有兩個盒號，「認得」和「打得出」各一個，0–5。答對升一盒，答錯回 0。開放中的每個字都要「認得」到 2 號盒、「打得出」到 1 號盒，才會開下一組 5 個字。開始畫面的 `NEXT GROUP WAITS ON …` 會列出還差哪些字。
- 排程算的是「回合」不是天數（FM-1 關機後沒有時鐘）。一天一回合最接近原本的設計；一天做很多回合也可以，只是複習的間隔會變短。

!!! note "0.3 卡在第一組"
    0.3 用發報速度（10 WPM，劃要按超過 240 ms）來判打碼卡。照 20 WPM 聽到的節奏打，劃只有 180 ms，M 會被讀成 I、T 讀成 E，所以「打得出」一直歸零，ROUND 14 還停在 K M U R E `[實機]`。0.4 改成看你自己的長短比例。想要 0.3 的判法，到 SETTINGS 把 JUDGING 切成 STRICT。

## 空檔：LISTEN

`PLAY` 開始，持續播放。`SELECT` 換節目：你學到的字組成的 Koch 字組、常用單字、模擬的通聯、CW 縮寫（附意思）、短句。

- 想練抄收就按 `SEQ` 把字藏起來，在腦中抄，下一行開始播時上一行才會顯示。`SEL` 可以偷看。
- 適合通勤時當背景聽，不用看螢幕。

## 練手：RHYTHM、KEY、TYPE

- **RHYTHM**：一組 5 下，練點、劃或混合。每一下都有判斷，最後給提示，也會依你點的平均長度建議一個發報速度（`K4`）。
- **KEY**：自由發報、即時解碼。`SEQ` 可以切成 CODE（每個字下面畫出你打的點劃）或 TIMING（照比例畫長短，看點是不是點、停頓有沒有太長）。
- **TYPE**：琴鍵變成字母、數字符號或通聯片語（`CQ`、`73`、`<SK>`…），按一下就播出來，用來熟悉聲音、查碼。`SEQ` 會在鍵上畫出點劃。

## 什麼時候加快

| 你的狀態 | PRESET |
|---|---|
| 剛開始 | START（聽 20/8、發 10） |
| 大部分的字都認得 | BUILDING（20/13、13） |
| 能和人一般通聯 | QSO（22/18、18） |
| 熟練 | FAST（25/25、22） |

也可以自己調：`K1` 字元速度，`K2` 字和字之間的間隔，`K4` 發報速度。轉的時候畫面會顯示點和劃的毫秒數。速度怎麼選見[速度](morse.md#speed)。

## 已經會一些、生疏了、或想重來

這些都會改到進度，所以螢幕一定會先問，**只有按 `PLAY` 才會寫入**，按 `HOME` 就放棄。

| 情況 | 怎麼做 |
|---|---|
| 已經認得一些字 | PRACTICE 開始畫面轉 `SELECT` 選 **TEST**：下兩組 10 個字各考一次，答對的可以標成已會；整組答對就開那一組 |
| 想直接標某些字 | CHART 按 `REC` 進入 EDIT，`SELECT` 選字，`OCT+` 或 `K1` 調高等級（在還沒開的字上調高會開到它那一組） |
| 一次開到後面的組 | EDIT 裡轉 `K2` |
| 生疏了 | EDIT 裡按 `FX`：全部退一盒，每個字都會早點回來複習 |
| 某個字忘了 | EDIT 裡把它降到 0，它會重新當新字出現 |
| 全部重來 | EDIT 裡按住 `SEL` 2 秒 |

EDIT 改的是副本，按 `PLAY`（或 `REC`）時才問「SAVE CHANGES?」。

## 換韌體之前：備份進度

進度存在 FM-1 的 flash 裡（`0xDA000`）。換成別的韌體不一定保得住：從 SLOOP 2.8.5 切回來時還在，刷回原廠 V15 就沒了 `[實機]`。換韌體前先備份：

- **網頁**：用 Chrome 或 Edge 開 fm1-morse 的 `web/progress.html`，Connect，Download the progress。換回 MORSE 後選檔案，Send it to the FM-1。
- **指令列**：`just morse-backup progress.json`，之後 `just morse-restore progress.json`。

FM-1 收到備份後會顯示回合數、開放幾個字、答過幾題，按 `PLAY` 才載入。備份檔是 JSON，可以直接改每個字的盒號。

## 進階：交給 Vail

接上電腦後，FM-1 在 Vail 眼中就是一個 MIDI 電鍵（[和 Vail 搭配](morse.md#vail)）。單字和呼號的抄收、QSO 模擬、和朋友互傳（Morse Mailbox、Repeater），都到 [training.vailmorse.com](https://training.vailmorse.com/) 練。MORSE 本身負責每天的字母和節奏基礎。

也可以把 FM-1 當成 Mac 的摩斯鍵盤：`just morse-type`，在 KEY 頁打的字會打進目前的視窗。
