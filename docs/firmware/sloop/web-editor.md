# SLOOP Web Editor

[Web Editor](https://isod89.github.io/sloop-fm1/webapp/editor/) 在 **Chrome 或 Edge** 開啟，FM-1 接上 USB 後按 **Connect**。它會即時跟著機器：轉機器上的旋鈕，editor 也跟著動。

## 頁面

| 頁面 | 內容 |
|---|---|
| **Sound** | 選取 track 的所有參數、engine 與 preset；FM6 track 有 FM6 面板，可 Import / Export DX7 SysEx |
| **Sequencer** | steps（tile 或 piano roll）、track 長度；drum track 是 16 聲音 × steps 的 grid。可 Export / Import MIDI 檔 |
| **Tracks** | 四條 channel strip |
| **Song** | section A–D 的順序、小節數、重複次數 |
| **Library** | user presets 與 preset 檔 |
| **Samples** | sample slots、檔案、**CHOP**（丟一段任意長度的錄音進來切） |
| **Drum synth** | 自製的合成鼓組 SYN1–SYN4，每個參數即時在機器上聽到 |
| **Song & sections** | section A–D 與 **Backup** |
| **Settings** | global、master（DUST、DUCK、FILT、ROLL）、drums |
| **LIVE** | 從電腦、平板或手機控制整場演出：mixer、punch-in pads、scenes、playlist |
| **Theme** | 自訂螢幕配色 |

## Backup 與還原

**Song & sections → Save a backup** 把機器上的所有東西寫成一個檔案：進行中的音樂、projects、user presets、samples、設定、FM6 bank。**Restore from a file** 全部放回去（先停止播放）。

這是 SLOOP 層級的備份，跟 M-Flasher 的完整 flash 備份不同：

| | Editor backup | M-Flasher 完整備份 |
|---|---|---|
| 內容 | SLOOP 的音樂與設定 | 整顆 1 MiB flash，含 bootloader 與原廠資料 |
| 用途 | 換歌、SLOOP 升降版前保存作品 | 回到某個韌體連同其狀態、救援 |
| 可跨機 | 可以 | 不建議 |

SLOOP 升降版、或要回原廠之前，兩種都做。

## 離線使用

- **Download the editor**：存成單一 HTML 檔（每個 release 也附 `sloop-editor-<版本>.html`），不需要網路，雙擊在 Chrome / Edge 開啟。它的 sound library 存在瀏覽器裡，跟線上版是分開的。
- **Install the app**：裝成 PWA，Android 也可以。Android 請用 app，因為 Android 的 Chrome 不允許下載的檔案使用 MIDI。

## 給想讓 agent 操作 SLOOP 的人

Editor 與機器之間的協定有文件：[web/EDITOR_PROTOCOL.md](https://github.com/isod89/sloop-fm1/blob/main/web/EDITOR_PROTOCOL.md)。這是寫 host 端工具或讓 agent 透過 SysEx 編輯 pattern 的正確起點。

社群的 [SLOOP Editor Skill](https://github.com/hunterxiong2026/fm1-sloop-editor-2026) 是針對較舊的 SLOOP 2.3 協定、偏 Windows 的 Python 工具，適合當參考實作，不建議直接拿來對 2.8.5 寫入 project。`[調研]`
