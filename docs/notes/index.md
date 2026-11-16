# 使用心得

依日期記錄實際玩過之後的想法、踩到的坑、想試的東西。新的放上面。穩定下來的結論搬到對應的頁面（例如 [SLOOP 操作速查](../firmware/sloop/usage.md)），這裡保留當時的脈絡。

## 2026-11-17：開始

- 建立這份筆記。起點是一份 ChatGPT 調研，整理時發現匯出檔的指令全部遺失，所以每條指令都重新對過上游文件。
- 決定的順序：先備份原廠、確認晶片，再刷 SLOOP 2.8.5。m-bootloader 與 SLOOP 3 之後再說。
- 對這台機器的兩個興趣：當口袋 groovebox，以及當 coding agent 的實體控制台（Agent Deck 整合 Herdr）。

## 想試的東西

- [ ] SLOOP：照「60 秒做出第一個 beat」走一遍
- [ ] SLOOP：用 USB audio 錄進電腦
- [ ] SLOOP Web Editor：CHOP 切自己的 sample
- [ ] fm1-emu：在瀏覽器試玩 SLOOP 3、Melodee、Agent Deck
- [ ] Agent Deck：host daemon 的 simulate mode 接 Herdr，不刷機
- [ ] CTL-1 + `mido`：把琴鍵事件轉成 Herdr API 呼叫

## 記錄格式

```markdown
## YYYY-MM-DD：標題

- 做了什麼、用哪個韌體與版本
- 發現、卡住的地方、解法
- 下次想試
```
