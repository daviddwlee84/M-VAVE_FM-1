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

## 下一步

[備份原廠與確認晶片](backup-and-chip.md)。
