# 刷 SLOOP 2.8.5

[SLOOP](https://github.com/isod89/sloop-fm1) 把 FM-1 變成四軌 groovebox。2.8.5 這版修正了新款 UC25HQ80 晶片上無法存檔與更新的問題，所以新舊機都能用。韌體本身的介紹見 [SLOOP 概觀](../firmware/sloop/index.md)。

!!! warning "開始之前"
    - [完整備份](backup-and-chip.md)已完成，hash 已記錄。
    - 晶片 ID 是 `0x856014` 或 `0xB36014` 其中之一。都不是就先停下來。
    - SLOOP 仍自稱 beta，自行承擔風險。

有兩條路。兩條都保留既有的 projects、user presets、samples 與設定。

## 方法 A：SLOOP 瀏覽器 installer（上游推薦）

1. 用 **Chrome 或 Edge** 開啟 [SLOOP installer](https://isod89.github.io/sloop-fm1/)。
2. FM-1 用 USB **資料線**直接接上，不經 hub。
3. 按 **INSTALL**，允許 MIDI 存取，等到 *Done*。FM-1 會重開並顯示 SLOOP logo。

Installer 寫入前會檢查套件的 SHA-256，機器上的 update loader 啟動前再檢查一次 CRC。安裝中斷的話機器會停在 update mode，再按一次 INSTALL 會接續完成。

關掉其他會用 MIDI 的程式或分頁（DAW、M-UPGRADE、另一個 editor 分頁），否則 installer 可能找不到機器。

## 方法 B：M-Flasher 安裝 `.fwsc`

適合想走 CLI、或讓 agent 陪跑的情況。

1. 從 [SLOOP releases](https://github.com/isod89/sloop-fm1/releases) 下載 `sloop-2.8.5.fwsc`，比對 SHA-256：

    ```sh
    shasum -a 256 sloop-2.8.5.fwsc
    gh api repos/isod89/sloop-fm1/releases/tags/v2.8.5 \
      --jq '.assets[] | "\(.name) \(.digest)"'
    ```

2. 讓機器進 download mode（見[備份](backup-and-chip.md#download-mode)）。
3. 安裝：

    ```sh
    m-flasher firmware sloop-2.8.5.fwsc
    ```

    它會先列出將要改動的部分，等你輸入 `yes`。**不要加 `--yes`。**

4. 寫完後 `m-flasher restart`，或加 `--restart` 讓它寫完自動重開。

M-Flasher 安裝韌體時的行為：

- 只寫入套件裡的 flash 映像，boot 區保留不動。
- 只寫有差異的 4 KiB sector，寫完用晶片的 CRC32 或讀回比對。
- 這個 session 第一次寫入前會自動存一份完整備份。
- 套件必須是給這個型號的，且能用這台機器的 chip key 解密，否則拒絕。
- 安裝約 10 秒。

## 刷完之後

1. **拔掉再插回 USB 一次**，電腦才會重新辨識出 USB audio 輸入。
2. 按住 **`HOME`** 進選單 → SYSTEM → **ABOUT**，確認版本。
3. **存檔測試**（新晶片尤其要做）：做一小段 pattern → Save → 關機 → 開機 → 確認內容還在。
4. 開 [Web Editor](../firmware/sloop/web-editor.md) 按 Connect，確認連得上，順手存一份 editor backup。
5. 更新[我的機器](../device-log.md)。

## 回到原廠

兩種做法，擇一：

- **還原 M-Flasher 的完整備份**（最乾淨，連原廠音色與資料一起回來）。屬於高風險操作，請本人執行，見[注意事項](cautions.md)。
- **M-VAVE 的 M-UPGRADE** 加官方 **V16** 的 `FM-1.fwsc`。先用 Web Editor 存備份，關掉其他用 MIDI 的程式。M-UPGRADE 找不到機器時，可改用 [Clipper installer](https://isod89.github.io/clipper-fm1/webapp/installer/) 頁面上的 **Return to the official firmware**（不需要安裝 Clipper）。

## 出問題

見[救援](recovery.md)。最常用的一招：SLOOP 開不起來時，**只按住 `OCT-` 開機**進入 *SLOOP USB RESCUE*，再安裝一次。
