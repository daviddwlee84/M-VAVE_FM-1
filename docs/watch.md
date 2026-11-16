# 上游追蹤

暫緩中的事項，以及每次檢查上游的紀錄。只往下加，不改寫舊紀錄。檢查時先跑 `just fm1-versions`。

## 暫緩中

| 事項 | 暫緩原因 | 解除條件 |
|---|---|---|
| 安裝 m-bootloader | 我的機器是新晶片 `0xB36014`，上游測試很少；沒有 USB_KEY 可救 boot 區 | 見 [SLOOP 3](firmware/sloop/sloop3.md) 的「解除暫緩的條件」 |
| SLOOP 3 | 只能透過 m-bootloader 的 MIDI MODE 安裝 | 同上 |
| m-apps slots + m-card | 需要硬體改裝，上游尚未成熟 | 上游有完整的 FM-1 實機驗證 |

## 檢查紀錄

### 2026-11-17

| 專案 | 版本 | 發布日 |
|---|---|---|
| m-flasher | v0.12.1 | 2026-11-15 |
| m-bootloader | v2.4 | 2026-11-13 |
| sloop-fm1 | v2.8.5 | 2026-11-04 |
| sloop3-fm1 | v3.3.7-beta（prerelease） | 2026-11-16 |
| fm1-emu | v0.5.1 | 2026-11-14 |
| fm-1-agent-deck | v0.7 | 2026-11-14 |
| fm1-ctl | v1.0.3 | 2026-11-06 |
| fm1-base | 沒有 release | — |

**m-bootloader 對 `0xB36014`** `[上游]`：README 仍寫 "Little tested on the FM-1's newer flash chip"。一台裝 1.1 / 1.2 後無法啟動、原因不明；另一台（單一使用者回報）跑 1.3 與 2.0 正常，選單與 download mode 可用。原文結論：With `0xB36014` … install it at your own risk。

**Agent Deck 對新晶片**：上游沒有實機回報。我在 fm1-emu 的 `uc25hq80` 模式看到它開機並寫入 flash `[模擬]`，見[模擬器實測](agentic/emulator.md#results)。

**結論**：維持暫緩。m-bootloader 沒有新證據；Agent Deck 可以進行 host 模擬，實刷前再做一次 preflight。
