# 自製韌體環境

目標是把 **改 code → host simulator → fm1-emu → 批准 → 實機** 整條鏈跑通，再談功能。基礎是 [fm1-base](https://github.com/charlesvestal/fm1-base)（MPL-2.0，beta）：它在 felucca-bsp 之上提供螢幕、按鍵、LED、音訊、MIDI、storage，以及跑同一份 app 的 host simulator。

**狀態（2026-11-17）：host simulator 這一段已跑通；實機套件的 build 卡在 Docker 拉不到 image，還沒產出 `.fwsc`。**

## 結構

```text
third_party/fm1-base/      git submodule（它自己再帶 felucca-bsp 於 bsp/）
firmware/hello/            fm1-base template 的原樣複本，用來驗證整條鏈
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
| `just fw-build [app]` | build 實機套件 `firmware/<app>/build/*.fwsc` | **失敗**，見下 |

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

## 實機 build 目前卡在哪

`just fw-build hello` 在啟動 toolchain 容器時失敗：

```text
Unable to find image 'debian:bookworm-slim' locally
docker: Error response from daemon: Get "https://registry-1.docker.io/v2/":
  proxyconnect tcp: dial tcp …:7890: connect: connection refused
build: cannot start the toolchain container
```

Docker daemon 設了一個連不上的 proxy，所以拉不到 `debian:bookworm-slim`。這是本機 Docker 的網路設定，不是 fm1-base 的問題。修好 proxy（或先手動 `docker pull --platform linux/amd64 debian:bookworm-slim`）之後再跑 `just fw-build hello`。toolchain 與 SDK 都已下載好，不必重抓。

在那之前，`.fwsc` 之後的每一步（fm1-emu 兩種晶片、實機）都還沒驗證。

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

- [ ] 修好 Docker 的 proxy，跑通 `just fw-build hello`
- [ ] `just emu-run firmware/hello/build/template.fwsc` 兩種晶片各一次，改設定、重開、確認保存
- [ ] 批准後刷進實機，按鍵 → USB MIDI → host 端 `mido` 腳本收得到
- [ ] 開第一個自己的 app：最小的 MIDI controller / macro pad（按鍵送 MIDI、螢幕顯示 layer）
- [ ] 依 [Agent Deck 試用](../firmware/agent-deck/index.md)的心得，決定 agent 控制台是 fork Agent Deck 還是在 fm1-base 上重寫
