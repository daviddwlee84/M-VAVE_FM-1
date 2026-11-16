# M-Flasher CLI

M-Flasher 的 GUI 執行檔本身就是 CLI：帶參數執行就進入命令列模式。agent 應該用 CLI，不要去模擬滑鼠點 GUI。以下指令與選項取自上游 [M-Flasher usage 文件](https://github.com/jvitkauskas/m-flasher/blob/main/docs/usage.md)的 *Command line* 一節。`[上游]` 版本見[資源](../resources.md)。

## 指令與權限等級

權限等級的定義見[工作流與權限](index.md#tiers)。

| 指令 | 上游說明 | 權限等級 |
|---|---|---|
| `m-flasher find` | the units in download mode (and those running, on USB MIDI) | 唯讀 |
| `m-flasher download-mode` | switch a running unit to download mode over USB MIDI | 模式切換 |
| `m-flasher restart` | leave download mode: the unit resets and starts its firmware | 模式切換 |
| `m-flasher info` | what is on the unit | 唯讀 |
| `m-flasher backup [DIR]` | 存 1 MiB flash 映像與描述它的 `.json` | 唯讀 |
| `m-flasher restore BACKUP.bin` | 寫回與備份不同的部分 | **禁止發起** |
| `m-flasher firmware PACKAGE.fwsc` | 安裝 `.fwsc` 套件，保留 boot 區 | **韌體寫入**（每次需批准） |
| `m-flasher bootloader [--stock PACKAGE.fwsc | --file UBOOT.boot]` | 只寫 boot 區（16 KiB） | **禁止發起** |
| `m-flasher run-ram IMAGE --addr 0x01C28000 [--call FC60]... [--read ADDR:LEN -o OUT]` | 上傳程式到晶片 RAM 執行，不寫 flash | **禁止發起** |

選項：

| 選項 | 用途 |
|---|---|
| `--device PATH` | 接了不只一台時指定 |
| `--board fm1|smk37` | 型號不確定時指定 |
| `--backup-dir DIR` | 備份目錄 |
| `--yes` | 跳過寫入前的互動確認。**在這個 repo 裡禁止使用** |
| `--keep-bootloader` | restore 時不寫 boot 區 |
| `--restart` | restore、firmware、bootloader 寫完後啟動機器 |

寫入類指令會先列出變更，再要求輸入 `yes`（或 `--yes`）。那個 `yes` 是我和機器之間的最後一道確認，所以 agent 永遠不加 `--yes`。

`run-ram` 雖然不寫 flash，但會在晶片上執行任意程式碼，所以歸在禁止發起。見[多韌體切換](../firmware/switching.md)。

## macOS 執行檔

```sh
/Applications/M-Flasher.app/Contents/MacOS/M-Flasher info
```

方便起見設個 alias（這是我自己的設定，不是上游提供的）：

```sh
alias m-flasher='/Applications/M-Flasher.app/Contents/MacOS/M-Flasher'
```

從 Terminal 執行時，它會用和 GUI 一樣的系統對話框要密碼，或用 `sudo` 執行。**不能透過 SSH**：macOS 只在螢幕上詢問是否允許存取可移除磁碟。`[上游]` Linux 直接是 `m-flasher info`（tarball 版是 `./m-flasher info`）。

!!! warning "首次開 GUI 時的 m-bootloader 建議"
    在原廠 bootloader 的機器上，GUI 視窗讀完 flash 後會主動建議安裝 m-bootloader，每台問一次。選 **Yes** 會先顯示變更讓你確認，選 **No** 機器維持原樣，之後可用 **Flash bootloader** 再裝。`[上游]` m-bootloader 目前不在範圍內，而且在新晶片上測試很少，所以選 **No**。CLI 是否也會出現這個詢問，上游沒有寫。

## M-Flasher 自己會檢查什麼

摘自上游 [Safety](https://github.com/jvitkauskas/m-flasher/blob/main/docs/safety.md) 文件。`[上游]` 這些是工具內建的，agent 不需要也不應該自己重做，但也**不能繞過**它的拒絕。

**寫入前**

- 連上時讀整顆 flash，與晶片自己算的 CRC32 比對，不符就什麼都不寫。
- 從 flash 判斷型號；不確定時會說並詢問，另一型號的套件或 bootloader 會被拒絕。
- 韌體套件必須是這個型號的，且必須能用這台的 chip key 解密。
- 套件的 boot 區只有在真的能開機時才會寫。
- 在沒測過的 flash 晶片上裝 m-bootloader 會警告；chip key 損壞時拒絕裝 m-bootloader。
- 每個 session 第一次寫入前自動存一份完整備份，並確保它落到磁碟上之後才 erase。
- 要還原的備份會用它 `.json` 裡的 SHA-256 驗證，不符就拒絕。

**寫入中**：只寫有變動的 4 KiB sector，且只在該動作允許的範圍內；保留這台的原廠資料（校正值、Bluetooth 位址）。

**寫入後**：寫過的全部用晶片 CRC32 或讀回比對驗證；不符會重寫一次，還是不符就停下來報告。

它**不會**幫你處理的：套件以外的區域不會被清掉；不同韌體的資料配置不同時只會警告；原廠 USR 音色只有備份能還原。

## 不用碰機器能做到多少 { #hands-free }

關鍵是怎麼進 download mode。`[上游]`

| 機器目前狀態 | 進 download mode | 能不能全自動 |
|---|---|---|
| 原廠韌體或 Felucca 正常執行 | `m-flasher download-mode`（USB MIDI 訊息） | 可以 |
| SLOOP、Felucca 或 Snoop37 執行中 | 按住 `OCT-` + `OCT+` 5 秒 | 不行，要人按 |
| 已裝 m-bootloader | 按住 `REC` 開機 | 不行，要人按 |
| 韌體損壞 | 常常自己掉進去 | 看運氣 |
| 韌體與 bootloader 都壞 | USB_KEY dongle | 需要額外硬體 |

!!! note "原廠 016 與 download-mode"
    上游寫這個 MIDI 切換適用於 M-VAVE 自己的韌體「FM-1 015, SMK-37 Pro and Elite 016」與 Felucca。文中的 016 指的是 SMK-37，**沒有寫 FM-1 016**。FM-1 016 能不能用，要實機試過才知道；沒反應就停下來記錄，不要亂試。

所以我的機器刷成 SLOOP 之後，每次進 download mode 都要我按鍵。離開 download mode 則一律可以用 `m-flasher restart`，或寫入時加 `--restart`。

## macOS 與 Linux

| | macOS | Linux |
|---|---|---|
| 首次授權 | 本機 GUI：管理員密碼 + 允許存取可移除磁碟 | 套件安裝 udev rule 與 SCSI generic（`sg`）driver |
| 透過 SSH 操作 | 不行（首次授權只在螢幕上） | 可以，以一般使用者執行 |
| 無人值守 / 遠端 agent | 要先在本機完成授權 | 比較適合 `[調研]` |

macOS 上進 download mode 後要盡快啟動 M-Flasher，留給系統磁碟 driver 約半分鐘後機器可能自己離開 download mode。`[上游]`

## 時間

- 整顆讀取約 1.2 秒，韌體安裝約 10 秒。`[上游]`
- 有些 USB controller 慢很多（例如 AMD 600 系列晶片組：讀取約 4.5 秒）。log 會顯示讀取速度並建議換 port。

agent 設 timeout 時以這些數字的數倍為準；明顯更慢就換 port，不要重試寫入。

## 範例：先備份再刷入

假設機器正在跑原廠韌體、用資料線直接接 Mac、已設好上面的 alias。檔名是示意。

```sh
# 1. 唯讀：找機器
m-flasher find

# 2. 模式切換（先告訴本人）
m-flasher download-mode

# 3. 唯讀：晶片、型號、目前韌體與 bootloader
m-flasher info

# 4. 唯讀：完整備份，自己再算一次 hash
m-flasher backup backups/2026-11-17-stock
shasum -a 256 backups/2026-11-17-stock/*.bin

# 5. 唯讀：核對套件 hash 與 release asset digest
shasum -a 256 downloads/PACKAGE.fwsc
```

```text
==== 停止：agent 把 preflight 四項列給本人，等本人批准這一次寫入 ====
```

```sh
# 6. 韌體寫入：只在批准後執行，不加 --yes。
#    M-Flasher 列出要改的 sector 後，由本人輸入 yes。
m-flasher firmware downloads/PACKAGE.fwsc --restart
```

寫完後做實機存檔測試，並把備份檔名、hash、套件 hash、結果記進[我的機器](../device-log.md)。完整的手動版流程見[備份原廠與確認晶片](../getting-started/backup-and-chip.md)。
