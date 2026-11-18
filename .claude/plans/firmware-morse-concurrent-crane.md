# THEORY v0.3：TODAY 教學、點亮式托盤、更隨機、PITCH、能開練的 CHART、CHORD 點亮模式

## Context 背景

THEORY 0.2 已經在實機上（commit `cf80948`）。本人 2026-11-18 的試用回饋記在 `docs/dev/theory.md` 的「v0.2 試用心得」：
- TODAY 對新手來說不知道在做什麼。
- 和弦題想要逐一點亮、可以撤回、而且持續發聲。
- 題目常重複。
- 想要單獨練絕對音感（三種玩法）。
- CHART 不知道要做什麼。
- CHORD 想要單指點亮模式。

本次決定：
- 多音題的送出方式兩種都做，SETTINGS 切換，預設 PLAY 送出。
- CHORD 點亮模式用 `REC` 記進軌跡並清空，`PLAY` 依序彈一遍軌跡，長按 `PLAY` 清空軌跡。本人補充：要和原本的方式互補，也能在 SETTINGS 設定。我的理解是加一個 AUTO 選項，停手一段時間就自動記下；`REC` 在任何模式都有效。
- 用 `ARP` 切換 HOLD / LATCH。
- 節奏和旋律聽寫留到 v0.4。

## 共用：點亮式托盤（latch tray）

TODAY 的多音題、PITCH 的「一團」、CHORD 的 LATCH 模式共用同一套程式，寫在 `app.c` 新的一節。
- **點亮**：按下立刻切換那個音的亮滅；亮著的音持續發聲（`synth_note_on`，熄掉時 `note_off`）。
- **擦到不算**：放開時如果按不到 `BRUSH_MS`（50 ms），把這次切換還原，沿用現在的過濾。
- **怎麼比對**：TODAY 和 PITCH 依音名比，不分八度；CHORD 依實際的鍵比，因為轉位要看低音。最多亮 8 個音（synth 只有 8 個 voice），再多就跳出提示。
- **送出**：
  - `PLAY` 一律可以送出。
  - SETTINGS 的 `SUBMIT` 設成 AUTO 時，亮的音數等於答案的音數、而且 1 秒沒變動，就自動判斷。
  - `SEL`：托盤有音時清空；托盤空的時候才是「不會」。
- 熄掉又點亮算「修正」，評分記為 SLOW，和現在一樣。
- 單音題（RELMIN、EAR_DEG）照舊，放開時判斷，答錯可以再試一次。

## TODAY

**教學**：
- **第一回合**（round 0）的開始畫面，用三行說明一回合是什麼：先看新東西、照著亮的鍵彈，再作答。
- **每種卡片第一次出現**，先出一張「怎麼玩」說明卡。判斷方式是這種技能的項目從來沒被問過（看 `st.last`），所以不必多存資料。
- **SHOW 卡改成「照著彈」**：
  - 亮出答案的鍵並播出聲音；照亮的鍵彈完（音階要照順序）才進下一張。`SEL` 跳過。
  - 這是教，不是考，所以 LED 可以帶彈；ASK 卡照舊不帶彈。
- **題目改成完整句子**，例如：
  - 音階：`Play the D major scale, D up, one note at a time`
  - 和弦：`Play chord IV of Eb major, any order`
  - 聽音：`Listen: Eb major, then one note. Play it`
  - box 0–1 的卡多一行提示。
  - 需要一個換行的文字函式，放在 `gtext.c` 的 `gtext_wrap`。
- footer 的按鍵說明寫成句子，例如 `PLAY check · SEL clear · SEQ show me`。

**更隨機**：
- **同一回合不重複題目**：
  - CHORDS 和 EAR_DEG 的級數、EAR_Q 的根音，同一回合裡從「還沒用過的」抽（shuffle bag）。
  - 用到的記錄存在 `pr` 裡，以項目為單位：級數用 7-bit mask，根音用 12-bit mask。
- **項目太少時改出變化題**：`sr_build` 加第三輪。
  - 當開放的項目填不滿一回合，用「內容會變」的項目補：CHORDS、EAR_DEG、EAR_Q；SCALE 到 box 2 以上會變成下行。
  - 補上的卡標成 `again`，所以答對不升 box，答錯照樣回 0。
- **`sr_spread`**：不讓同一個項目連續出現，優先度高於「不同調」。
- 種子加上 `answered`。

## PITCH（新頁，絕對音感遊戲，不排程）

頁面順序改成 MENU、TODAY、WHEEL、CHORD、CHART、PITCH、SETTINGS。PITCH 排在後面，`rec.start` 的舊值才不會對錯頁；`start` 的上限改成 6。

**音池**：
- 依序加入 C G E A D F B，再加 F# Bb Eb C# G#。等級 L 有 2+L 個音（L = 0–10）。
- 起始等級存在 `rsv[1]`（`pitch`）。
- 等級 4 以上，音會落在隨機的八度。作答依音名，不分八度。

**三種遊戲**，在開始畫面用 `SELECT` 選、`PLAY` 開始。

| 遊戲 | 一局 | 作答 | 調整 |
|---|---|---|---|
| SINGLE 單音 | 20 個音 | 聽一個音，限時 4 秒內按對（畫面上有倒數條）。錯或超時：再播一次正確的音，並亮出它的鍵 | 一局答對 ≥ 90% 升一級（新音先介紹一次），< 60% 降一級 |
| COMBO 一串 | 10 串 | 依序播 n 個音（從 2 個起，最多 8 個），照順序按，按一個判一個 | 對了 n+1，錯了 n−1 並亮出答案；分數是最長的一串 |
| CLUSTER 一團 | 10 團 | 同時播 n 個不同的音（2–5 個），用點亮式托盤找出全部 | 連對兩次 n+1，錯了亮出答案 |

- 結束畫面顯示正確率、平均時間、最長一串。等級變了就存。
- 按鍵：`PLAY` 托盤空的時候重播，托盤有音時送出；`HOME` 離開；`SEL` 清空托盤。

## CHART

- **說明**：畫面上加一行 `your progress, key by key`，和 6 格的色階圖例（`new … known`）。旁邊列出選中那個調的各項 box，以及回合數和正確率。
- **選調開練**：選一個調按 `PLAY`，就開一回只練那個調的 TODAY，進 TODAY 直接開始。
  - 實作上是 `sr_build` 的 focus 多一段：bits 8–15 放「調 + 1」，`sr_in_focus` 跟著改；沒有調的 EAR_Q 不出。
  - 還沒開放的調顯示 `LOCKED` 提示，不開始。

## CHORD

- **切換輸入方式**：`ARP` 在 HOLD 和 LATCH 之間切換。LATCH 時 `ARP` 燈亮，header 標 `LATCH`；這個選擇記在設定的旗標裡。
- **LATCH 模式**：
  - 用托盤，依實際的鍵比對，畫面一直顯示目前亮著的音的辨認結果，不用「這次手勢最多音」那套邏輯。
  - `SEL` 清空。
- **記進軌跡**：
  - `REC` 把目前的和弦記進軌跡，在 LATCH 模式下同時清空。HOLD 模式下 `REC` 也能記下最後顯示的和弦。HOLD 照舊，放開就記。
  - SETTINGS 的 `CHORD LOG` 設成 AUTO 時，亮的音是一個和弦、而且 2 秒沒變，就自動記下但不清空；同一個和弦不會重複記（`hist_push` 已經會去重）。
- **`PLAY`**：
  - 短按：用 `synth_score` 依序彈出軌跡。每個和弦一拍，用 `th_voice_near` 排出彼此最接近的位置，加上根音低音；速度依 `spd`。
  - 長按（≥ 800 ms）：清空軌跡。要在 `input()` 記錄按鈕按下的時間，放開時才決定是短按還是長按。

## 設定與備份

- **`rec_t`**：原地用掉剩下的 `rsv`，不升 `REC_VER`；`set_valid` 跟著加檢查。
  - `rsv[0]` 是 `flags`：bit0 SUBMIT AUTO、bit1 CHORD LOG AUTO、bit2 CHORD LATCH。
  - `rsv[1]` 是 `pitch`（0–10）。
- **SETTINGS** 的列：START（多了 PITCH）、CARDS、SPEED、SUBMIT、CHORD LOG。
- **備份紀錄**：格式不變（format 1，218 B），只是 settings 的 byte 10、11 不再固定是 0。要一起改的地方：
  - 韌體的 `set_blob` / `set_unblob`。
  - `host/theory/progress.py`：`SET_FIELDS` 加 `flags`、`pitch`，檢查 `start <= 6`、`flags < 8`、`pitch <= 10`。
  - `docs/assets/theory/progress.html`：同樣的改動。
  - 舊的 JSON 沒有這兩個欄位，就當成 0。共用的測試 vector 重新產生，並同步到 `tests/basic.txt`。
- **版本**：韌體 `FW_MINOR 3`（HELLO 帶 0.3）；host 改成 `__version__ "0.3.0"`、`FIRMWARE_KNOWN (0, 3)`。

## 檔案

- **韌體**：
  - `firmware/theory/app.c`：主要的改動，包括托盤、TODAY、PITCH、CHART、CHORD、設定、expect 和 host 指令。
  - 其他：`srs.c` / `srs.h`、`gtext.c` / `gtext.h`、`tests/basic.txt`、`tests/srs_test.c`、`app.h`（如果需要新的 expect）。
- **電腦端**：`host/theory/{progress,__init__}.py`、`host/tests/test_theory.py`、`docs/assets/theory/progress.html`。
- **文件**：
  - `docs/dev/theory.md`：v0.3 的設計、按鍵、PITCH、驗證、roadmap（聽寫移到 v0.4）。
  - `README.md`、`docs/index.md`、`AGENTS.md`：狀態。

## 實作順序（每一步都要通過測試才往下）

1. **`srs.c`**：依調篩選（focus 的 bits 8–15）、變化題補位、不讓同一項目連續出現。同時加 unit test。
2. **點亮式托盤**：放進 TODAY，包括 SUBMIT 兩種模式、`SEL` 清空。
3. **TODAY 的教學和隨機**：說明卡、照著彈的 SHOW 卡、句子和換行、shuffle bag。
4. **CHART**：圖例和選調開練。
5. **CHORD**：LATCH、`ARP`、`REC`、`PLAY` 播放和長按、CHORD LOG。
6. **PITCH** 三種遊戲。
7. **設定旗標和備份欄位**：韌體、host、網頁、vector。
8. **整體驗證**：fm1-emu 兩種晶片、預演、文件。

## Verification 驗證

- **`just theory-unit`**：
  - 依調篩選只出那個調的項目。
  - 只開第一組時，一回合不會同一項目連續出現，而且回合能填滿（有變化題）。
  - 變化題答對不升 box。
  - 原有的檢查全部照過。
- **`just fw-test theory`**，新增的檢查：
  - **TODAY 托盤**：點亮時 `voices` 增加，熄掉時減少；擦到的被還原。
  - **送出**：PLAY 模式不會自動判斷，AUTO 模式等 1 秒後判斷；`SEL` 有音時清空、沒音時算不會。
  - **教學**：第一次出現的說明卡；SHOW 卡要照著彈完才前進。
  - **隨機**：同一回合裡和弦題的級數不重複。
  - **CHART**：在 D 上按 `PLAY`，回合只出 D 的項目；鎖住的調不開始。
  - **CHORD**：`ARP` 切到 LATCH；點 C E G 之後 `REC`，軌跡是 `I` 而且清空；再點 A C E、`REC`，軌跡是 `I vi`；短按 `PLAY` 時 `busy` = 1；長按 `PLAY` 清空軌跡；AUTO 停 2 秒自動記下。
  - **PITCH**：三種遊戲各跑完一局（用 host 指令作答），等級的升降存起來，重開讀回。
  - **設定和備份**：新的設定列；byte 10、11 往返；`start = 6` 開機直接進 PITCH。
- **`just theory-test`**：新欄位往返；舊的 JSON（沒有 flags、pitch）也讀得進來；vector 和韌體測試一致。
- **網頁**：用 node 執行網頁的 JS，和 Python 比對。
- **`just fw-build theory`**：在大小上限內，沒有 soft-double。
- **fm1-emu 兩種晶片**：改設定、玩一局 PITCH、TODAY 一回合，然後重開讀回。
- **預演**：疊在 `2026-11-18-pre-theory-0.2` 的備份上，只寫 `0xD6000` 和 `0xD8000`，Morse 的 `0xDA000` 不變。
- **`just docs-check`** 通過。
- 刷實機另外走 preflight，等本人批准。commit 也要等本人要求。
