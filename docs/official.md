# 官方資源

M-VAVE 官方提供的東西。**我目前一個都沒用**：備份、刷機、切換全部走社群的 M-Flasher，韌體也都是社群的。這頁是為了知道官方有什麼、什麼時候會需要它。

連結多數取自上游 repo 的 README 與 Firmware Hub（2026-11-17），我沒有逐一開啟確認內容，官方網站的頁面是動態載入的。

## 清單

| 資源 | 是什麼 | 我用到了嗎 |
|---|---|---|
| [產品頁](https://www.cuvave.com/product?id=fm-1)（cuvave.com）、[m-vave.com 下載頁](https://www.m-vave.com/download) | 產品資訊、官方韌體與工具的下載 | 沒有 |
| 原廠韌體 `FM-1.fwsc` | 出廠與官方更新的韌體。我的機器出廠是 **015**；官方較新的是 016（8-part multitimbral 等） | 015 只存在我的原廠備份裡。沒裝過 016 |
| **M-UPGRADE**（又稱 M-Updater） | 官方的更新工具，PC 與 Mac。會重寫韌體與原廠 preset | 沒有。M-Flasher 可以直接安裝官方的 `.fwsc` |
| [fm1_open](https://github.com/m-vave/fm1_open) | 官方的**開放韌體** FM1 Open V14：8 軌、8 個 sample slot、16 個效果示範。app 是 GPL-3.0 加 SDK linking exception；Firmware Hub 註明它的聲音引擎是閉源的 binary SDK。build 需要 Windows 與 JieLi 的 Windows toolchain | 沒有 |
| [Workbench](https://open.m-vave.com/fm1-workbench) | 官方的網頁工具：更新韌體、瀏覽器內預覽、管理 project、sample 與 DX7 音色、下載 Source + SDK | 沒有 |
| [官方論壇](https://forum.m-vave.com) | 公告與討論 | 沒有 |

原廠韌體和 FM1 Open 是兩個不同的東西：前者是機器出廠的閉源韌體，後者是官方後來釋出原始碼的另一套韌體。

## 什麼時候會需要官方的東西

- **想完整回到原廠**：我會還原自己的 015 備份（M-Flasher GUI 的 RESTORE FULL FLASH），不需要官方工具。沒有備份的人才需要 M-UPGRADE。
- **想試官方的新功能**：016 或 FM1 Open V14。注意 016 一執行就會覆寫 preset 區，之後降回 015 會有亂碼與音高錯誤，見[注意事項](getting-started/cautions.md)。
- **某個社群韌體要求先回原廠**：有些 installer 只接受從原廠或特定韌體安裝（例如 ChoralRoot 的 installer 拒絕在 SLOOP 上安裝）。用 M-Flasher 走 download mode 可以繞過這個限制，但繞過不等於安全，見 [ChoralRoot](firmware/choralroot/index.md)。
- **保固或送修**：還原原廠備份。

## 官方更新會做的事

- 官方更新與 M-UPGRADE 會把**原廠 bootloader** 寫回去。裝了 m-bootloader 的人更新後要重裝。
- M-UPGRADE 只接受和機器自報型號相符的套件。
- 社群韌體把 USB 裝置名稱改掉之後，M-UPGRADE 可能找不到機器。SLOOP 的文件建議這時用 Clipper installer 頁面上的 Return to the official firmware。

## 為什麼都用社群方案

- M-Flasher 能做完整 flash 備份與還原，M-UPGRADE 不能。
- M-Flasher 走晶片 ROM 的 download mode，韌體壞了也能用。
- 我的機器是新晶片，社群工具與 Firmware Hub 對新晶片的標示比官方資料清楚。
- 我要的功能（groovebox、agent 控制台、MIDI controller）都在社群韌體裡。

背景見[生態是怎麼來的](ecosystem.md)。
