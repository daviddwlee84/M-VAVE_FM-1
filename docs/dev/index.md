# 自製韌體環境

目標是把 **改 code → host simulator → fm1-emu → 批准 → 實機** 整條鏈跑通，再談功能。基礎是 [fm1-base](https://github.com/charlesvestal/fm1-base)（MPL-2.0，beta）：它在 felucca-bsp 之上提供螢幕、按鍵、LED、音訊、MIDI、storage，以及跑同一份 app 的 host simulator。

**狀態（2026-11-18）：host simulator、實機套件 build、fm1-emu 兩種晶片開機都已跑通；自己的 build 已經刷進實機過（Conductor 0.1、0.2，Morse 0.1–0.3，紀錄見[我的機器](../device-log.md)）。**

## 結構

```text
third_party/fm1-base/      git submodule（它自己再帶 felucca-bsp 於 bsp/）
firmware/hello/            fm1-base template 的原樣複本，用來驗證整條鏈
firmware/conductor/        agent 控制台（見 Conductor 頁）
firmware/morse/            摩斯電碼機（見摩斯電碼機頁）
firmware/common/           app 之間共用的模組（hostlink、ledfx、chime、lpc、adpcm）
firmware/voicelab/         語音解碼器的驗證 app（見裝置端語音合成頁；不刷實機）
firmware/theory/           樂理練習機 THEORY（見樂理練習機頁）
firmware/guitar/           吉他指板與和弦書 GUITAR（見吉他指板與和弦書頁）
firmware/<app>/            之後的 app
tools/jieli/               JieLi toolchain（gitignore）
tools/fw-AC79_AIoT_SDK/    JieLi AC79 SDK，固定 tag（gitignore）
```

這符合上游的建議：把 `template/` 複製到自己的 repo，fm1-base 放在旁邊當 submodule。`tools/build.py APP_DIR` 接受任意位置的 app 目錄。

## 指令

| 指令 | 作用 | 我跑過嗎 |
|---|---|---|
| `just fw-setup` | 初始化 submodule、安裝 Python 工具（`fw` dependency group） | 是 |
| `just fw-test [app]` | build host simulator 並跑 `tests/basic.txt`，產出截圖與 WAV 到 `firmware/<app>/build/host/` | 是，全部 `ok` |
| `just fw-toolchain` | 下載 JieLi toolchain 與 AC79 SDK 到 `tools/`（約 1.2 GB） | 是 |
| `just fw-build [app]` | build 實機套件 `firmware/<app>/build/*.fwsc` | 是，約 17 秒，產出 `template.fwsc`（616 KB，identity `FM-1_1100`） |

## 前置需求（macOS）

- `uv`、`just`、Xcode command line tools（`cc`）。
- **`brew install libraqm`。** fm1-base 的字型產生器（`tools/gen_font.py`）用到 OpenType features，需要 Pillow 帶 libraqm。macOS 的 Pillow wheel 編譯時沒有包含它（`HAVE_RAQM = False`），所以 `pyproject.toml` 設了 `no-binary-package = ["pillow"]`，讓 Pillow 從原始碼對 Homebrew 的 libraqm 編譯。沒裝的話 `gen_font.py` 會報 `not supported without libraqm`。`[實機]` host 端
- 實機 build 另外需要 **Docker**：JieLi toolchain 只有 Linux x86-64，`build.py` 在 macOS 上會自動把每個工具放進 `linux/amd64` 容器執行。

## 兩段式 build

| | Host simulator | 實機套件 |
|---|---|---|
| 指令 | `build.py APP --host` | `build.py APP` |
| 需要 | 本機 C 編譯器 | JieLi toolchain + AC79 SDK + Docker |
| 產出 | `build/host/<name>_host` | `build/<name>.fwsc` |
| 用途 | 日常迭代、邏輯與畫面測試 | 模擬器與實機 |

Host simulator 讀一份腳本（`tests/basic.txt`）注入按鍵與 MIDI、檢查狀態、存截圖與音訊。這是讓 agent 自己驗證改動的主要手段，不需要任何硬體。

## Build 結果

`just fw-build hello` 的輸出（2026-11-17）：app 226,264 B；RAM code 9,296 / 32,768 B；`.data+.bss` 76,452 / 98,304 B；build 自帶的檢查全部 `ok`（沒有 soft-double、register 存取只在 felucca-bsp 內）。

在 fm1-emu 開機 `[模擬]`：

| 晶片 | 結果 |
|---|---|
| `uc25hq80` | 開機到 TEMPLATE 畫面，5 秒內 12 次 flash erase / program |
| `p25q80` | 同上，12 次 |

兩種晶片行為一致。這只是開機與寫入次數，「改設定 → 重開 → 讀回」還沒做。

!!! note "之前卡住的地方"
    第一次 build 失敗是因為 Docker daemon 的 proxy 連不上，拉不到 `debian:bookworm-slim`（`build: cannot start the toolchain container`）。修好 Docker 的網路後就正常。遇到同樣訊息先查 `docker pull --platform linux/amd64 debian:bookworm-slim` 能不能成功。

## 預設的 template 帶 Bluetooth

`firmware/hello/fm1.toml` 是 `ble = true`，會用到 SDK 裡 JieLi 的 Bluetooth library（binary only）。如果第一次 build 想降低變數，可以加 `--no-ble`（產出 `NAME-usb.fwsc`）。

## Release 與另開 repo 的問題

fm1-base template 附的 release workflow（透過共用的 [zvenson/fm1-build](https://github.com/zvenson/fm1-build) 產生 build attestation，Firmware Hub 才會標 built from source）假設的是**獨立的 firmware repo**：app 在 `app/`、fm1-base 在 `base/`、根目錄有 `./build.sh`。放在這個筆記 repo 的子目錄不符合那個配置，所以我把複本裡的 `.github/` 拿掉了。

結論：在本機迭代、刷自己的機器，子目錄就夠。**等某個 app 要正式 release 時，再把它搬到獨立 repo，並以 submodule 引回這裡。**

## 自製韌體的規則

寫在 `AGENTS.md`，重點：

1. 先 `just fw-test`，再用 `just emu-run` 在**兩種晶片**各跑一次，才提實刷。
2. 存檔只走 fm1-base 的 storage 層，它處理新舊晶片的 JEDEC ID。不自己寫 flash 位址。
3. 自製韌體每一次實刷都要我批准，和裝別人的韌體一樣。
4. build 產物不進 git。

## 下一步

- [x] 跑通 `just fw-build hello`
- [x] `just emu-shot firmware/hello/build/template.fwsc` 兩種晶片都開機
- [ ] `just emu-run firmware/hello/build/template.fwsc` 兩種晶片各一次，改設定、重開、確認保存
- [ ] 批准後刷進實機，按鍵 → USB MIDI → host 端 `mido` 腳本收得到
- [ ] 開第一個自己的 app：最小的 MIDI controller / macro pad（按鍵送 MIDI、螢幕顯示 layer）
- [x] 依 [Agent Deck 試用](../firmware/agent-deck/index.md)的心得，決定 agent 控制台是 fork Agent Deck 還是在 fm1-base 上重寫：**在 fm1-base 上重寫**，見 [Conductor](../agentic/conductor.md)（`firmware/conductor`，共用模組在 `firmware/common`）
