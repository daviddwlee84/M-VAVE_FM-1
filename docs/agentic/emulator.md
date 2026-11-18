# fm1-emu 模擬器

[fm1-emu](https://github.com/joakimeriksson/fm1-emu) 模擬的是 FM-1 這顆晶片本身：CPU、螢幕、按鍵、旋鈕、音訊、MIDI、flash。社群韌體的 `.fwsc` 套件**不用改**就能在上面跑，和要裝到實機上的是同一個檔案。`[上游]` 這讓它成為刷機前最便宜的一道檢查。

!!! note "本機狀態（2026-11-17）"
    `fm1-emu` 0.5.1 的 macOS arm64 預編譯版已下載到 `tools/fm1-emu/`（gitignore），用 `just emu-fetch` 取得並對過 release digest。沒有原始碼 checkout，所以需要 checkout 的指令（`cargo run`、`--fw list`）我沒跑過。

## 這個 repo 的用法 { #local }

| 指令 | 作用 |
|---|---|
| `just emu-fetch` | 下載 fm1-emu、驗 digest、解到 `tools/fm1-emu/` |
| `just emu-run <package.fwsc> [chip]` | 開視窗玩。`chip` 預設 `uc25hq80`（我這台），也可給 `p25q80`。存檔留在 `tools/emu-state/<套件>-<chip>.flash`，下次開還在。`fm1-live` 開的是存檔裡的韌體（套件只會裝進新的存檔），所以重新 build 之後，`emu-run` 會先把新套件的韌體區疊上存檔、保留資料區，就像在實機上升級（2026-11-18 起；之前改完韌體再開，跑的其實是舊版） |
| `just emu-shot <package.fwsc> [chip] [ms]` | headless 開機 `ms` 毫秒，把 LCD 存成 PPM，並印出 flash 寫入次數 |
| `just agtdeck-fetch` | 下載 Agent Deck 套件，驗 digest 與 build attestation |
| `just emu-rehearse <package.fwsc> <backup.bin> [chip]` | 把套件的韌體區蓋在一份實機備份上再開機，預演切換。說明與限制見[多韌體切換](../firmware/switching.md) |

**開起來先看兩件事** `[實機]`：

- 視窗標題要以 `FM-1 ·` 開頭。如果是 `SMK-37 ·`，表示套件是 SMK-37 版，下載錯了。
- 啟動訊息的 `MIDI in:` 列出的是啟動當下電腦上的 MIDI 輸入。`no inputs` 表示沒有裝置；要用實機或 MIDI 鍵盤彈，先接好再開。

`emu-run` 後面可以接 `fm1-live` 的其他旗標，例如 `just emu-run downloads/sloop-2.8.5.fwsc uc25hq80 --usb`。

## 模擬器實測 { #results }

2026-11-17，`fm1-live 0.5.1`，headless `--shot`。標記 `[模擬]`：在模擬器裡看到的，不等於實機。

| 韌體 | `p25q80` | `uc25hq80` | 觀察 |
|---|---|---|---|
| SLOOP 2.8.5 | 開機到 TRACKS 畫面 | 開機到 TRACKS 畫面 | 腳本按鍵（`REC`、彈音、`PLAY`）有反應，但 12 秒內兩種晶片都是 0 次 flash 寫入，我沒能用腳本觸發存檔。存檔行為以實機為準（已通過，見[我的機器](../device-log.md)） |
| Agent Deck 0.7 | 開機到 `WAITING FOR THE HOST` | 開機到 `WAITING FOR THE HOST` | 每次開機兩種晶片都是 3 次 flash erase / program，行為一致：它在新晶片模式下確實寫入 flash，沒有退回只存 RAM |
| SLOOP 3 | — | — | **跑不了**：fm1-emu 的韌體清單沒有它，它的套件是給 m-bootloader 的 `.area`，不是 `.fwsc` |

Agent Deck 那一列是目前唯一的新晶片相容性證據，上游沒有 UC25HQ80 的實機回報。它只說明「模擬的新晶片上寫入有發生」，設定是否正確讀回、實機是否正常都還沒驗證。

## 它是什麼

FM-1 的核心是 JieLi AC791N：兩顆 JieLi 自家的 32-bit pi32v2 核心，沒有公開的指令集手冊。fm1-emu 從韌體本身還原出指令集，再用 JieLi toolchain 編的測試程式核對，上面疊上 timer、中斷、flash、240×240 螢幕、按鍵矩陣與 encoder、旋鈕、音訊輸出、MIDI UART 與 USB controller 的模型。熱門程式碼會被編譯成原生碼（瀏覽器裡是 WebAssembly）。`[上游]`

所以它比較像 QEMU 或 Renode 這類硬體模擬器，而不是 Docker。Docker 只隔離執行環境，不模擬別的 CPU；需要可重現的 headless 測試環境時，可以把 fm1-emu 包進 Docker 裡，兩者是內外層的關係。`[調研]`（上游 README 沒有提到 Docker。）

## 在哪裡跑

| 形式 | 用途 |
|---|---|
| [瀏覽器頁面](https://joakimeriksson.github.io/fm1-emu/) | 選韌體、按 Start、用鍵盤滑鼠或 MIDI 鍵盤玩。flash 變更自動存在瀏覽器裡 |
| `fm1-live` | 原生程式（macOS、Linux），有視窗與聲音，也可以 headless |
| `fm1-emu` | batch runner，給測試與除錯：WAV 與螢幕輸出、breakpoint、memory dump |
| DAW plugin | AU、VST3、CLAP，在 DAW 裡當軟體樂器 |

頁面本身不放韌體，啟動時從作者的網站下載。M-VAVE 原廠韌體要自己下載 `FM-1.fwsc` 再在頁面上開啟。上游列出可跑的原廠版本是 V13 與 V15，**沒有提到 016**。`[上游]`

## 瀏覽器 URL 參數

| 參數 | 作用 |
|---|---|
| `?fw=ID` | 指定韌體，也接受 The Firmware Hub 的 id |
| `?chip=uc25hq80` | 換成 2026 年秋季起出貨的新 flash 晶片（JEDEC `B3 60 14`），取代原本的 Puya |
| `&embed=1` | 只顯示機器與控制項，給別的網站嵌入用 |
| `?package=URL` | 播放清單外的套件（https，需開 CORS），`&name=` 命名 |
| `&flash=PATH`、`&script=PATH` | try-out 用：覆蓋 flash sector、從開機播放輸入腳本。只接受頁面自己 origin 的路徑 |

!!! warning "原本的連結用了 `mhz=100`"
    我最早拿到的連結是 `?fw=felucca&mhz=100`。真實晶片跑 240 MHz，100 MHz 只是減輕電腦負擔，**不能代表實機的時序**。上游 README 沒有記載 `mhz` 這個 URL 參數，只有 `fm1-live` 的 `--mhz`，而且它的用途是「電腦跟不上時降低時脈」。要看接近實機的行為，用預設時脈。

## 本機指令

以下全部取自上游 README。`[上游]`

```sh
# release 下載的 fm1-live
./fm1-live --fwsc felucca.fwsc

# 從 checkout build
cargo run --release -p fm1-live -- --fw list           # 列出韌體
node tools/fetch_firmwares.mjs                          # 下載到 reference/firmwares
cargo run --release -p fm1-live -- --fw felucca         # 開視窗、有聲音
cargo run --release -p fm1-live -- --fwsc FM-1.fwsc     # M-VAVE 原廠套件
```

`fm1-live` 的選項與環境變數：

| 選項 / 變數 | 作用 |
|---|---|
| `--flash-chip uc25hq80` | 新 flash 晶片，等同頁面的 `?chip=uc25hq80` |
| `--flash FILE` | flash 內容存在 FILE，跨次執行保留（開始讀、結束寫回） |
| `FM1_FLASH_CUT=N` | 在第 N 次 erase 或 page program 寫到一半時斷電，看韌體怎麼處理寫一半的存檔 |
| `--usb` | 插上 USB 線：host 會 enumerate，USB-MIDI 韌體在電腦上出現為「(emu)」MIDI 裝置（macOS、Linux） |
| `FM1_STRICT=1` | 照實機的方式跑：未對齊存取會 fault、每條指令照量測的 cycle 數、LCD 傳輸照 SPI 時脈。只能 interpret，比較慢 |
| `--shot` | headless 執行；配 `FM1_USB_SEND` 在指定時間送 USB MIDI 並印出回應 |
| `FM1_BENCH_KEYS` | 輸入腳本：`ms:mHEX` 從 DIN 送 MIDI、`ms:id` 按鍵、`ms:eN:clicks` 轉 encoder |
| `--mhz` | 降低時脈。原生 JIT 預設跑真實的 240 MHz |
| `--flaky-core1 RATE` | 第二顆核心偶爾讀錯 RAM，測試韌體退回單核的能力 |

`fm1-emu` batch runner 的確切指令列 README 沒有列出，要看它的 checkout。

## 能驗證什麼、不能驗證什麼

| 項目 | 模擬器 | 備註 |
|---|---|---|
| 韌體能不能開機、畫面對不對 | 可以 | `--shot`、截圖 |
| 按鍵、encoder、MIDI 輸入 | 可以 | `FM1_BENCH_KEYS` |
| 存檔在新舊晶片上是否保留 | 可以 | 見下一節 |
| 寫到一半斷電 | 可以模擬 | `FM1_FLASH_CUT` |
| USB MIDI 枚舉 | 可以 | `--usb`，polling 與 interrupt 都支援 `[上游]` |
| 時序是否接近實機 | 部分 | `FM1_STRICT=1`，仍是模型 |
| bootloader 安裝、ROM download mode | **不行** | README 沒有記載模擬這些；M-Flasher 走的是晶片 ROM，不是韌體 |
| 真正的 flash 寫入 | **不行** | 模擬的 flash 不是實體晶片 |
| 實際音質、電源、硬體穩定性 | **不行** | 要實機與耳朵 |

上游自己也提醒過這件事：FM-1 SID 在還沒於實機上跑過就先釋出，第一批使用者付出了代價，之後又修了幾輪才在實機上正常。`[上游]` 模擬器通過不代表可以跳過[工作流](index.md#flow)裡的人工批准與實機測試。

研究筆記寫「USB controller 的中斷還不完整」，但目前上游 README 寫 controller 已支援 polling 與 interrupt 兩種。研究筆記這一點已經過時。

## 刷機前比較新舊 flash 晶片

新晶片（`0xB36014`）上，只認得 Puya 的韌體會把存檔留在 RAM，關機就沒了。`[上游]` 刷之前可以先在模擬器上看：

1. 同一個套件跑兩次：一次預設（Puya），一次加 `--flash-chip uc25hq80`（或頁面的 `?chip=uc25hq80`）。
2. 每次改一個設定或音色，用韌體自己的 Save。
3. 結束再開（`fm1-live` 用 `--flash FILE`，兩種晶片各用一個檔案），檢查設定還在不在。

`--flash-chip` 與 `--flash` 能否一起用，README 沒有寫組合範例，要實際試。新晶片上存檔消失，就不要刷到新晶片的機器上。我的機器是哪一種晶片見[我的機器](../device-log.md)，背景見[兩種 Flash 晶片](../hardware/flash-chips.md)。

## 和 fm1-base host simulator 的差別

`[調研]`（以下來自研究筆記，尚未對照 fm1-base 上游。）

| | fm1-emu | fm1-base host simulator |
|---|---|---|
| 模擬層 | 晶片（CPU 與周邊） | 應用層，把 app 原生編譯到電腦上 |
| 能跑別人的 `.fwsc` | 可以 | 一般不行 |
| 速度 | 有模擬成本 | 通常較快 |
| 適合 | 黑箱測試、相容性、最終套件驗證 | unit test、UI 與邏輯開發 |

自己寫韌體時兩者互補：host simulator 做快速回歸，fm1-emu 跑最後產出的套件。這個 repo 目前不寫韌體，所以兩個都還沒用上。

## 自己的 app 的瀏覽器版（fm1-base `--web`） { #own-web }

fm1-base 可以把同一個 app 編成 WebAssembly，在瀏覽器裡跑（`tools/build.py APP --web`）。它跑的是 host simulator 那一層，不是 fm1-emu 的整台機器模擬，所以適合試玩畫面和操作，不能拿來驗證 flash 或時序 `[上游]`。

- **指令**：`just fw-web APP`，輸出在 `firmware/APP/build/emu/`。
    - 有裝 emsdk 就用它的 `emcc`；沒有的話，`scripts/emcc-docker.sh` 用固定版本的 `emscripten/emsdk:3.1.74` image 代替。
    - 用 `python3 -m http.server -d firmware/APP/build/emu` 開，按 `Switch on` 開機。
    - 存檔放在瀏覽器的 localStorage。
- **PR 的試玩連結**：把那個資料夾發布成私人的 claude.ai artifact，連結貼在 PR 裡。規則見 `AGENTS.md`。
    - 第一個例子是 2026-11-18 THEORY 0.3 的 PR #5。
    - 連結只代表當時那一版，有新的 commit 要重新發布。
- **名稱衝突**（2026-11-18 遇過）：瀏覽器版把 app 和 `web/emu/emu_web.c` 編在同一個檔案裡。THEORY 原本有一個 `static void ring()`，和 `emu_web.c` 的全域變數 `ring` 撞名，編不過。host simulator 和實機版都沒事，只有 `--web` 會出問題，所以改名成 `circle_ring()`。
    - app 裡全域範圍的名稱最好加上自己的前綴。
- **讀螢幕**：頁面會去找 `reader/words.js`，`--web` 的輸出裡沒有這個檔案，所以內建的讀螢幕用不了，其他功能正常。

