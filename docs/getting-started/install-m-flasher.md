# 安裝 M-Flasher

[M-Flasher](https://github.com/jvitkauskas/m-flasher) 是電腦端的工具：備份整顆 flash、還原、安裝 `.fwsc` 韌體、安裝或移除 m-bootloader。它直接跟晶片 ROM 內建的 USB download mode 溝通，所以韌體壞掉時也能用。目前版本見[資源地圖](../resources.md)。

它和 m-bootloader 是兩回事：

| | 裝在哪 | 作用 |
|---|---|---|
| M-Flasher | 電腦 | 備份、刷機、還原、救援 |
| m-bootloader | FM-1 裡的 boot 區 | 開機時的 recovery menu、從按鍵進 download mode、啟動前檢查韌體 |

## macOS

需要 macOS 11 以上，Apple silicon 與 Intel 都支援。

1. 從 [releases](https://github.com/jvitkauskas/m-flasher/releases) 或[下載頁](https://jvitkauskas.github.io/m-apps/flasher/)取得 `M-Flasher-<版本>-macos.dmg`。
2. 開啟 dmg，把 M-Flasher 拖進 Applications。
3. 第一次開啟會被 macOS 擋下，因為它沒有 Apple 簽章：到 **System Settings → Privacy & Security → Open Anyway**。
4. app 找到機器時會出現兩個系統對話框：
    - 要求輸入密碼。只有管理員能把 USB 裝置從 macOS 的 disk driver 手上拿走。
    - 詢問是否允許 M-Flasher 存取 removable volume（那就是 FM-1），選 **Allow**。只會問一次。

!!! note "接上後盡快開 app"
    機器進 download mode 後如果一直留給 macOS 的 disk driver，大約半分鐘後可能自己離開 download mode。

!!! warning "首次授權不能透過 SSH"
    removable volume 的詢問只會出現在螢幕上。第一次一定要在本機圖形介面完成，之後 CLI 才能順利用。

### 驗證下載檔

Release 頁面每個 asset 都有 SHA-256 digest。下載後比對：

```sh
shasum -a 256 M-Flasher-0.12.1-macos.dmg
gh api repos/jvitkauskas/m-flasher/releases/latest \
  --jq '.assets[] | select(.name | test("macos")) | "\(.name) \(.digest)"'
```

### Command line

同一個執行檔帶參數就是 CLI：

```sh
/Applications/M-Flasher.app/Contents/MacOS/M-Flasher info
```

可以設 alias：

```sh
alias m-flasher='/Applications/M-Flasher.app/Contents/MacOS/M-Flasher'
```

CLI 一樣會跳出系統密碼對話框，或用 `sudo` 執行。完整指令見 [M-Flasher CLI](../agentic/m-flasher-cli.md)。

## Windows 與 Linux

- **Windows 10/11**：解壓 `M-Flasher-…-windows.zip` 執行 `M-Flasher.exe`。未簽章，SmartScreen 選 **More info → Run anyway**。找到機器時會要求管理員權限。Microsoft Defender 偶爾會誤判新版本。
- **Linux**（x86-64 或 ARM64，glibc 2.35+）：`.deb` / `.rpm` 套件會一併裝好 udev rule 與 SCSI generic driver，一般使用者就能執行 `m-flasher`，不需要 root。這也是 Linux 比較適合 headless 或遠端自動化的原因。

細節以上游的 [Using M-Flasher](https://github.com/jvitkauskas/m-flasher/blob/main/docs/usage.md) 為準。

## 也有網頁版的燒錄工具，為什麼還用 M-Flasher { #web-installers }

有。很多韌體附網頁 installer，用 Chrome 或 Edge 打開、接上 FM-1、按 Install 就好，電腦不用裝任何東西：

| 工具 | 做什麼 |
|---|---|
| [官方 Workbench](https://open.m-vave.com/fm1-workbench) | 更新原廠韌體、管理 project 與 sample（見[官方資源](../official.md)） |
| 各韌體自己的 installer | 例如 [ChoralRoot](https://quixotic7.github.io/ChoralRootFM1/webapp/installer/)、Melodee、FMDj；Firmware Hub 的 `catalog.json` 有 `install` 欄位的就是 |
| SLOOP 的 [Web Editor](../firmware/sloop/web-editor.md) | 在 SLOOP 之間升級 |

**原理不同**，這是重點。`[上游]` 網頁 installer 走的是 **Web MIDI**：它跟**機器上正在跑的韌體**對話，請那個韌體自己把新韌體寫進 flash（OTA）。M-Flasher 走的是晶片 ROM 內建的 **USB download mode**，完全不靠現在的韌體。

| | 網頁 installer（Web MIDI） | M-Flasher（ROM download mode） |
|---|---|---|
| 要裝東西 | 不用，Chrome / Edge 即可 | 要裝 app，第一次要授權 |
| 機器要能正常開機 | **要**，現在的韌體得聽得懂它的更新協定 | 不用，黑畫面也能用 |
| 備份 | 只備份那個韌體認得的資料（設定、音色、loop），存成 JSON 之類的檔案 | **整顆 1 MiB flash**，可以完整還原 |
| 能從哪個韌體裝過去 | 看 installer 的規則。ChoralRoot 的會拒絕從 SLOOP、Felucca 0.x 安裝 | 任何狀態 |
| 晶片檢查 | 由各 installer 自己決定 | 讀 JEDEC ID，並列出寫入計畫給你確認 |
| 中途斷掉 | 新一點的 installer 有續傳；舊的可能讓機器黑掉，要靠 M-Flasher 救 | 可以重來 |

**我這台用 M-Flasher**，原因：

- 整機備份與還原只有它做得到。這個 repo 的切換流程（`just fm1-switch`）每次寫入前都先整機備份。
- 它有 CLI，`Justfile` 可以把流程包起來，agent 也能做唯讀檢查。
- 網頁 installer 壞了要回頭用 M-Flasher 救，反過來不需要。

網頁 installer 適合的情況：不在自己的電腦前、只是要升級同一個韌體的新版本，而且那個韌體的作者只提供網頁 installer。用之前一樣先用 M-Flasher 做整機備份。

## 下一步

[備份原廠與確認晶片](backup-and-chip.md)。
