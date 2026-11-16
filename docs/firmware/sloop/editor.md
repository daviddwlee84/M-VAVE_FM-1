# SLOOP Editor（內嵌 2.8.5）

這個站自帶一份 **SLOOP Web Editor 2.8.5**，版本和我機器上的 SLOOP 相同。FM-1 用 USB 接上，用 **Chrome 或 Edge** 開，按 **Connect** 並允許 MIDI。

[全螢幕開啟 Editor](../../assets/sloop/sloop-editor-2.8.5.html){ .md-button .md-button--primary }

<iframe src="../../assets/sloop/sloop-editor-2.8.5.html" title="SLOOP Web Editor 2.8.5" allow="midi" loading="lazy" style="width:100%;height:78vh;border:1px solid var(--md-default-fg-color--lightest);border-radius:6px;"></iframe>

內嵌的畫面比較窄，實際操作建議用上面的按鈕全螢幕開。各頁的用途見 [Web Editor](web-editor.md)，接手機的用法見[搭配手機與平板](../../hardware/mobile.md)。

## 為什麼自己放一份

- **版本固定。** 上游的[線上 editor](https://isod89.github.io/sloop-fm1/webapp/editor/) 會跟著 SLOOP 更新。這份固定在 2.8.5，和機器上的韌體一致。SLOOP 升級後要一起換。
- **手機上方便。** 它是 https 的網頁，Android 的 Chrome 可以直接用 Web MIDI。下載到手機本機的 HTML 檔不行。
- 檔案和 `downloads/sloop-editor-2.8.5.html` 相同，就是 SLOOP 2.8.5 release 附的那個（SHA-256 `25fe6043…dabb76f03`，已對過 release digest）。

## 要知道的事

- **不能用 Safari / iOS 的一般瀏覽器**，它們沒有 Web MIDI。
- **同時只能有一個東西用 MIDI。** 關掉 DAW、M-Flasher、其他 editor 分頁，否則 Connect 找不到機器。Agent Deck 的 daemon 也要停掉。
- **sound library 存在瀏覽器裡，而且每個網址各一份。** 這裡存的音色不會出現在上游的線上 editor 或本機離線檔裡，反之亦然。要搬移用 Export / Import 或 backup 檔。
- 清除瀏覽器資料會清掉那份 library。重要的東西存成檔案。
- 我還沒有實際用這一頁連過機器，只確認檔案能載入。`[調研]` 等級，用過再更新。

## 授權

SLOOP 與它的 editor 由 isod89 以 **GPL-3.0** 釋出，原始碼在 [isod89/sloop-fm1](https://github.com/isod89/sloop-fm1)（v2.8.5）。這裡是未修改的原檔，僅為了自己使用方便而重新散布。
