# 救援

這頁是「怎麼做」。各種狀況為什麼發生、怎麼判斷自己在哪一級，見[變磚的原理與分級](bricking.md)。

依問題程度由輕到重。越下面越需要額外的東西。這些方法都依賴機器當下的 bootloader、韌體與 USB 狀態，不保證成功。

| 情況 | 做法 | 需要 |
|---|---|---|
| 安裝到一半中斷 | 機器停在 update mode，在 installer 再按一次 **INSTALL** | 瀏覽器 installer |
| SLOOP 開不起來 | 關機，**只按住 `OCT-`** 開機進 *SLOOP USB RESCUE*，重新安裝 | SLOOP installer |
| SLOOP / Felucca 還在跑，但想整個重刷 | 同時按住 **`OCT-` + `OCT+`** 5 秒進 download mode，用 M-Flasher | M-Flasher |
| 原廠韌體還在跑 | M-Flasher 的 **DOWNLOAD MODE** | M-Flasher |
| 韌體壞了，已裝 m-bootloader | 按住 **`PLAY`** 開機進 recovery menu；選單在韌體無法啟動時也會自己開，並先進 MIDI mode，可直接用瀏覽器 installer 重裝 | m-bootloader |
| 同上，想用 M-Flasher | 按住 **`REC`** 開機進 download mode | m-bootloader + M-Flasher |
| 沒畫面，但電腦看得到 `WL82 UBOOT1.00` | 機器已在 ROM download mode，用 M-Flasher 還原備份或重裝韌體 | M-Flasher + 備份 |
| USB 完全沒反應 | 需要硬體救援：RP2040 做的 USB_KEY（[FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter)） | Raspberry Pi Pico |

## 幾個要知道的事

**損壞的韌體常會自己掉進 download mode。** 所以「黑畫面」不等於磚。先接上電腦開 M-Flasher 看它找不找得到。

**原廠 bootloader + 壞掉的韌體，沒有按鍵可以救。** 這正是 m-bootloader 存在的理由，也是沒有它時硬體 USB_KEY 是最後手段的原因。

**m-bootloader 不是遠端重開機。** 韌體死了、USB MIDI 也沒在跑的時候，電腦沒辦法透過 USB 叫它進 recovery，要人去按鍵。

**還原備份是回到某個韌體「連同它的資料」最乾淨的方式。** 單純安裝韌體只寫套件裡的內容，不會清掉前一個韌體留下的資料；不同韌體把設定存在不同位置，殘留資料可能讓新韌體行為怪異。

**螢幕上的錯誤訊息** `[上游]`

- `BAD CONFIG`：1.3 以前的 m-bootloader 不接受原廠 016 寫入的 boot 設定，每次開機都停在選單。選單裡的 **Boot app** 仍可啟動韌體；安裝新版 m-bootloader 可修正。
- `FAILED -5`：Melodee 的 *Return to official V15*、或某些情況下的 M-UPGRADE，因為 boot 區與套件不同而拒絕。boot 區與韌體都沒被動到。改用 M-Flasher。
- `APP BAD CRC` / `APP INCOMPLETE` / `APP NOT FOUND`：m-bootloader 啟動前檢查沒過，會自動開 recovery menu。

## 延伸閱讀

- [FM1 Developers' Wiki：Recovery](https://sbegueria.github.io/fm1-dev-wiki/Recovery.html)
- [m-bootloader README](https://github.com/jvitkauskas/m-bootloader)
- [M-Flasher：Safety](https://github.com/jvitkauskas/m-flasher/blob/main/docs/safety.md)
- [MvaveFM1Unbricker](https://github.com/Quixotic7/MvaveFM1Unbricker)
