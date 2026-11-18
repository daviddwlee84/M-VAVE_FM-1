# MORSE：Hub 卡片的「Play in browser」與 cheatsheet 頁面

## Context 背景

Hub 上很多韌體有 ▶ Play in browser 按鈕（例：Rainbow），MORSE 沒有。MORSE 的 cheatsheet 現在連到 README 的 `#controls`，只有 31 行，讀起來不方便。

查過的事實：
- Hub 的 `catalog.json` 裡，按鈕來自 `play` 欄位，cheatsheet 來自 `cheat`。兩者都可以寫在 `fmhub.json`（允許的 9 個欄位裡就有 `play`）。MORSE 目前是 `play: null`，`cheat` 指向 README。
- 大多數韌體的 `play` 指向 **fm1-emu 的網頁版**（WebAssembly，跑的就是 `.fwsc` 本身）：
  - 直接連：`https://joakimeriksson.github.io/fm1-emu/?fw=<hub id>`，例如 conductor、argo、clavioline。fm1-emu 會讀 Hub 的目錄，把 Hub 新上架的韌體列為 newcomers，所以 `?fw=morse` 應該能直接找到，而且會跟著 Hub 的最新版（0.5 → 0.6）。
  - 包成自己的頁面：Rainbow、Skiff、Tuna 等都做了 `/try/`。fm1-emu 的 `?embed=1` 只顯示機器本身，專門給別的網站用 iframe 嵌入。
- 本機的 `just emu-run` 已經在 fm1-emu 上跑過 MORSE `[模擬]`，可以開機。

## 做法

### 1. Try 頁：`fm1-morse/web/try/index.html`（新檔）

- iframe 嵌入 `https://joakimeriksson.github.io/fm1-emu/?fw=morse&embed=1`，版本跟著 Hub，不用自己放 `.fwsc`。
- 旁邊放一小段「先試這些」：
  - 開機後按哪幾個鍵進 KEY 頁打一個字；
  - LISTEN 頁怎麼聽；
  - 電腦鍵盤怎麼對應琴鍵（照 fm1-emu 自己的說明連過去，不自己猜）。
- 寫明限制：
  - 模擬器沒有藍牙；
  - 進度存在這個瀏覽器裡，不是實機；
  - 要接 Vail 或備份進度得用實機，連到 `../`（MORSE on the web）。
- 視覺跟 `web/index.html` 一致，沿用同一套 `:root` 色票和深色模式。
- 加一個「直接開 fm1-emu 全頁」的連結，iframe 不能用時可以改點它。

### 2. Cheatsheet 頁：`fm1-morse/web/cheatsheet.html`（新檔，英文）

- 內容從 `docs/dev/morse-cheatsheet.md`（157 行中文，比 README 完整）翻成英文，因為 Hub 的讀者大多看英文：
  - 每一頁的按鍵（KEY、LEARN、LISTEN、TYPE、SETTINGS）；
  - header 的符號（`BT`、`PC`）；
  - 設定項目；
  - prosign 表。
- 單頁、可列印成 A4（`@media print`），深色模式和 `web/index.html` 一樣。
- README `## Controls` 保留精簡版，加一行連到這頁。中文的 `morse-cheatsheet.md` 保留，頂端加一行連到英文版。

### 3. `fmhub.json`

```json
"play":  "https://daviddwlee84.github.io/fm1-morse/web/try/",
"cheat": "https://daviddwlee84.github.io/fm1-morse/web/cheatsheet.html"
```

另外：
- `web/index.html` 的頁首加兩個連結：Try in the browser、Cheatsheet。
- Hub 會在約 3 小時內讀到 `fmhub.json` 的修改，不用重新送審。

### 4. 文件與 skill

- `.agents/skills/fm1-hub-listing/SKILL.md` 的 fmhub.json 一節補上：
  - `play` 的兩種做法：直接連 `fm1-emu/?fw=<hub id>`，或用自己的 `/try/` 頁 iframe 嵌入 `&embed=1`；
  - 別的網站的 package 用 `?package=URL&name=…`，但需要 https 加 CORS，GitHub release 的檔案不行，GitHub Pages 可以；
  - `cheat` 最好指向一個 GitHub Pages 的單頁，不要只指向 README 的錨點。
- `docs/firmware/hub.md`：同樣的事實寫成中文，標 `[上游]`（fm1-emu 頁面寫的參數），以及上線後的結果。
- `docs/dev/morse.md` 的 release 一節：記錄 play 和 cheat 上線。

## Verification 驗證

1. 本機用 `python3 -m http.server` 開 `web/try/` 和 `web/cheatsheet.html`，用 playwright-cli 截圖：
   - iframe 裡 MORSE 能開機，畫面出現 MORSE 主選單；
   - cheatsheet 在亮色、深色和列印預覽下都正常；
   - 手機寬度沒有橫向捲動。
2. 確認 `?fw=morse` 在 fm1-emu 上真的解析到 MORSE（未登錄的話會落到預設韌體）。如果不行，改用 `?package=` 加上一個有 CORS 的位置，並先回報給你。
3. `python3 -m json.tool fmhub.json`；之後由你在 Hub KB 跑 "Check your file"。
4. `just docs-check`。
5. commit 和 push（fm1-morse 和 superproject 的 pointer）都等你說。push fm1-morse 之後 GitHub Pages 才會上線，Hub 也才會讀到新的 `fmhub.json`。
