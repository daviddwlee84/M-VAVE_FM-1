# 摩斯電碼機：FM-1 自製韌體 + host 計畫

## Context 背景

想把 FM-1 做成摩斯電碼機：能發報、解碼成文字、把文字播成摩斯、有逼逼聲（sidetone）和靜音、有學習模式，將來兩台互傳。

調查結果（2026-11-17）：

- **沒有前例**：Firmware Hub 的 244 個套件、FM1 Dev Wiki、GitHub 都沒有摩斯相關的 FM-1 作品，也沒有兩台 FM-1 互傳的功能。相關的只有 SPELLBOX（琴鍵當 A–Z 鍵盤）和 MIDIMON（BLE central + USB host）。
- **放得下**：app slot 581,564 B，`.data+.bss` 96 KB。template 不含無線電是 77 KB；own BLE stack 加 41–46 KB，central 再加 13–22 KB。估計完整功能約 150–180 KB、RAM 約 40 KB。**學習模式放在韌體裡，不用移到 host。**
- **兩台互連**：
  - 藍牙直連：API 支援（`ble_central = true`），但只在模擬中測過，沒在兩台真機上跑過。
  - USB 線直連：不可行。FM-1 不供 VBUS，要外接有電的 OTG hub，而且 host 模式不收 SysEx。
  - 經過電腦：最可靠。
- **Vail 的 MIDI 規格** `[上游]`（vail-adapter `docs/MIDI_INTEGRATION_SPEC.md`）：channel 1；note 0 = 直鍵或已排好時序的按鍵，1 = dit paddle，2 = dah paddle；按下送 `90 nn 7F`，放開送 `80 nn 00`。

- **Vail 已經有完整的訓練工具**（training.vailmorse.com）：Vail CW School（進度統計、遊戲）、Hear It Type It、Send Practice（明寫支援 USB CW key 和 MIDI adapter）、Free Practice、Morse Mailbox（和朋友非同步互傳）、QSO Simulator 等。

使用者的決定：

- 暫時沒有第二台機器：直連要設計、要在模擬器測，實機測試延後。
- 第一版做單機核心 + 學習。
- **深度訓練不重造輪子**，交給 Vail，FM-1 當它的 MIDI 鍵。韌體裡的學習主打**隨身練習**，體驗像背單字或學日文五十音：短回合、閃卡、間隔複習、字表看進度。
- host 要三項：Vail 相容、FM-1 當 Mac 的摩斯鍵盤、電腦打字 → FM-1 播出。
- 做成獨立韌體 `firmware/morse/`，不併進 Conductor。

## 設計重點 Design

**線路上傳的是按鍵動作，不是文字。** 每次 key-down／key-up 送 `90 00 7F`／`80 00 00`（ch1 note 0），和 Vail 的格式一模一樣，收的一方自己解碼。好處：

- USB、兩種 BLE stack、USB host、經電腦轉送都能走；
- Vail 和任何 DAW 都直接聽得懂；
- 不在 ITU 表裡的符號也照樣傳得過去。

**文字另走 hostlink SysEx**（`F0 7D 4D 4F ver type … F7`，header 是 "MO"），只用在和自己的 host 之間：

- device → host：`T_CHAR`（解碼出的字元）、HELLO／heartbeat（沿用 `hl_tick`）
- host → device：`T_TEXT`（要播的字串）、`HELLO_ACK`

Vail 會忽略這些 SysEx。

**時序架構：**

- `APP_TICK`（1 kHz）：讀按鍵 → keyer 狀態機 → 開關 tone 的 gate，按鍵事件連同時間戳寫進 ring buffer。
- main loop：解碼、送 MIDI、畫面。
- 實機每個 mark 會被量長約 7 ms（debounce 造成）。只在 device build 扣掉這個常數；模擬器沒有 debounce，不扣。

**模式（v0.1–v0.2）：**

| 模式 | 內容 |
|---|---|
| KEY 自由發報 | 琴鍵發報。直鍵：任一鍵；paddle（iambic A/B）：左半邊 = dit、右半邊 = dah，可對調。即時解碼，大字顯示目前的 ·−，下方是捲動文字。同時從 USB 送 note 0。收到的 note 0 用另一個音高播放、解碼，文字顯示成另一個顏色 |
| TYPE 字母鍵盤 | 27 鍵 = A–Z + 空白；按住 OCT- 時切成數字和標點。按一鍵就播出那個字母，`PLAY` 播整行並送出。螢幕畫出鍵位對照 |
| PRACTICE 今日練習 | 一回合約 20 張閃卡，2–5 分鐘。內容是到期要複習的卡，加幾張新卡。卡片有四種：① 聽 → 選（4 個選項，對應的 4 顆琴鍵亮燈）② 聽 → 打（照打回去）③ 看字 → 打（畫面出字母，打出它的碼，附點劃比回饋）④ 看碼 → 選（畫面出 ·−，選字母；靜音時用）。回合結束顯示正確率、升級幾張、連續天數改成連續回合數 |
| CHART 字表 | 像五十音表的格狀字表，每個字依熟練度上色。選一個字就播出並顯示它的碼，也當成查表和新字預覽 |
| 設定 | WPM、Farnsworth 有效速度、音高、keyer 模式、paddle 對調、靜音、每回合張數 |

- **間隔複習（Leitner 盒）**：每個字、每個方向（認字、發報）各有一個盒號 0–5。答對升一盒，答錯回到 0。盒 n 每 2ⁿ 個回合複習一次。**FM-1 沒有 RTC**，關機後不知道過了幾天，所以排程用「回合數」而不是日期。之後接 host 時可以讓 host 送時間，改用真正的日期，但不在這次範圍。
- **新字順序**：用 Koch／LCWO 的 41 字順序（K M U R E S N A P T L W I . J Z = F O Y , V G 5 / Q 9 2 H 3 8 B ? 4 7 C 1 D 6 0 X），每 5 個字一組，像五十音的「行」。某一組的字都升到盒 2 以上，才開放下一組。字母一律用設定的字元速度播放（Koch 的原則是用聽的，不數點劃）。
- **時序計算**：dit = 1200/WPM ms；Farnsworth 把 PARIS 的 19 個間隔單位拉長。
- **隨身**：開機直接進「今日練習」，回合可以隨時中斷，進度照存。
- **靜音**：不出聲，改用 LED（`ledfx`）和螢幕上的燈條同步閃。在不能出聲的場合，練習會自動改出「看碼 → 選」和看燈號的卡。
- **交給 Vail 的部分**：單字、呼號、Q code、QSO 模擬、長篇抄收、和人互傳（Morse Mailbox、Repeater）。這些要靠 Vail 相容才行，所以它是 0.1 的必要條件，不是附加功能。
- **按鍵配置**：`HOME` 叫出模式選單，`FX` 切靜音，`PLAY` 重播或送出，`SELECT` 移動選項，K1 調 WPM、K2 調 Farnsworth、K3 調音高，MASTER 調音量。細節寫進速查頁。

## 版本與範圍 Roadmap

| 版本 | 內容 | 需要刷機 |
|---|---|---|
| 韌體 0.1 | KEY、TYPE、設定（存檔）、sidetone／靜音、USB MIDI（Vail 相容）收發、hostlink `T_CHAR`／`T_TEXT` | 是 |
| 韌體 0.2 | PRACTICE 閃卡回合 + CHART 字表，Leitner 盒號和回合計數存檔 | 是 |
| host 0.2.x | `host/morse/`：`morse type`（FM-1 當 Mac 鍵盤）、`morse send`（電腦打字 → FM-1 播出）、`--simulate` | 否（只用 0.1 的協定） |
| 韌體 0.3 | own BLE stack：peripheral（Mac／iPad／Vail 走藍牙）+ central 直連另一台 FM-1。直連的時序抖動對策（velocity 帶長度 + 播放緩衝）等量過再定。直連的實機測試等到有第二台 | 是 |

這次核准後做到**韌體 0.2 + host**，全部在模擬器和 fm1-emu 裡驗證。刷進實機是另一件事：照 AGENTS.md 做 preflight，由使用者逐次核准。

## 檔案 Files

新增：

- `firmware/morse/`（MIT，自己寫，不複製 hello 的 MPL tone 程式碼）
  - `fm1.toml`：`include = ["../common"]`、`usb_product = "MORSE FM-1"`、`id_prefix = "FM-1_13"`；0.1–0.2 不開 BLE
  - `app.h`：`APP_TICK`、`APP_SYSEX`、`APP_HOST_EXPECT`、`APP_STORE_MAP {0xDC000 panel, 0xDA000 morse}`（`0xDA000` 是候選位址，要用 emu-rehearse 確認）
  - `app.c`：UI、模式、輸入、render、存檔、MIDI。和 Conductor 一樣用 `#include` 把其他 `.c` 收成一個 compilation unit
  - `morse.c/.h`：純邏輯，不依賴平台，host 上可直接編譯。內容：碼表（字母、數字、標點、AR/SK/BT/KN）、編碼、自適應解碼器、iambic A/B keyer、文字 → 時序排程、Farnsworth
  - `tone.c/.h`：兩個 voice（自己發的、收到的），相位累加 sine，5 ms raised-cosine ramp，靜音旗標
  - `learn.c/.h`：Leitner 盒（每個字 × 兩個方向）、以回合數排程、組別解鎖、組一回合的卡、選項產生（干擾選項挑碼型相近的）。存檔約 41 × 2 個盒號再加回合計數，遠小於一個 object 的 3840 B 上限
  - `tests/basic.txt`、`tests/morse_test.c`、`README.md`
- `host/morse/`（MIT，Python，mido + python-rtmidi）：`__main__.py`、`protocol.py`（測試向量和 `basic.txt` 共用）、`device.py`（找 `MORSE FM-1` 的 port）、`typing.py`（Mac 上用 CGEvent 打 Unicode 字串，需要 Accessibility 權限）。測試放 `host/tests/test_morse_*.py`
- `docs/dev/morse.md`（設計、可行性調查、路線圖、版本紀錄）、`docs/dev/morse-cheatsheet.md`

重用：

- `firmware/common/hostlink.{c,h}`：`hl_init`、`hl_send`、`hl_tick`
- `firmware/common/ledfx.{c,h}`
- `fm1-base`：`plat_keys`、`plat_buttons`、`plat_enc`、`plat_ms`、`cv_*`、`plat_store_*`、`plat_midi_send`、`plat_midi_in`
- 參考 Conductor：`input()` 的邊緣偵測、`draw_header`／`main_area`／`draw_footer` 的分條繪製
- 參考 hello：設定存檔加 magic 檢查的寫法
- host 的 port 搜尋參考 `host/conductor/device.py`，但不動 Conductor

修改：

- `zensical.toml`：「自製韌體」改成一個 section，底下放 `dev/index.md` 和兩個新頁
- `docs/hardware/io.md`：新增「兩台 FM-1 互連」一節（`[上游]`）
- `Justfile`：
  - `morse-unit`：C 單元測試
  - `morse-emu [CHIP]`
  - `morse-type`、`morse-send TEXT`、`morse-sim`
  - `fw-test morse` 沿用現有 recipe
- `README.md`、`docs/index.md`：在「自己的韌體 + host」表加一行
- `AGENTS.md`：layout 表和 commands 加上 morse
- 有了 emu-rehearse 的結果後：更新 `docs/hardware/flash-map.md`、`docs/firmware/switching.md`

## Verification 驗證

1. `just morse-unit`：
   - 碼表全部字元來回轉換；
   - 解碼器在 5／15／25／35 WPM、±20% 抖動下的正確率；
   - PARIS = 50 單位、Farnsworth 間隔；
   - iambic B 的 squeeze 序列；
   - Leitner：答對升盒、答錯歸零、盒 n 每 2ⁿ 回合到期、組別解鎖條件、一回合的卡組成（到期卡優先，新卡有上限）、靜音時只出看碼的卡。
2. `just fw-test morse`（`tests/basic.txt`）：
   - `key … down`／`wait`／`up` 之後 `expect text "PARIS"`；
   - `expect_out USB 90 00 7F`；
   - TYPE 的 `tapkey` 產生正確時序的 note；
   - `midi USB 90 00 7F` 會出現收到的文字；
   - 靜音時 `expect peak_db_max`；
   - `sysex` 帶 `T_TEXT` 會播出，打出的字會產生 `expect_sysex` `T_CHAR`；
   - 跑完一個練習回合（用腳本答題），`reboot` 後設定、盒號和回合數都還在；中途離開的回合也有存到；`expect store_writes` 有上限（只在回合結束和改設定後存）；
   - 每個畫面都 `shot`。
3. `just cond-test` 繼續通過，新的 host 測試也通過。`morse --simulate` 端到端跑一次。
4. `just fw-build morse`：記錄 app 大小和 RAM。
5. `just emu-run` 在 `uc25hq80` 和 `p25q80` 各跑一次：存檔 → 重開 → 讀回（`[模擬]`）。
6. `just emu-rehearse firmware/morse/build/morse.fwsc <最新備份>`：列出會改寫哪些 sector，確認 `0xDA000` 沒有和別人衝突。
7. 用 `just morse-emu` 的虛擬 USB 線在 Chrome 開 Vail 的 Repeater 和 training 站的 Send Practice，確認 note 0 被當成按鍵（`[模擬]`）。Vail 網頁會不會忽略 velocity，也從它的原始碼確認。
8. `just docs-check`。

實機刷入不在這次自動執行的範圍。做之前先備份目前 Conductor 的狀態（最新的備份是 pre-conductor），提出完整 preflight，等使用者核准。
