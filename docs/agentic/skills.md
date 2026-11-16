# Agent Skills

M-VAVE 沒有發布官方的 agent skill。社群有幾套，各有長處，但這個 ecosystem 每週都在變，任何一份 skill 都是「某人在某一天整理的摘要」。這頁比較現有選項，並寫下這個 repo 的做法。

## 社群選項

| 方案 | 內容 | 強項 | 限制 | 在這裡的角色 |
|---|---|---|---|---|
| [maxfridbe/mvave-fm1](https://github.com/maxfridbe/mvave-fm1) | 兩個 Claude Code skills + podman build container | 備份、刷機、救磚、firmware 開發一次涵蓋；誠實寫出驗證範圍 | 部分內容過時或只適用 ORBIT | 知識參考 |
| [kmplngj/Felucca](https://github.com/kmplngj/Felucca) | Felucca fork 的 `AGENTS.md`、六個 skills、CI、hardware gate | 目前最完整的 agent-driven FM-1 開發流程 | 工作流與 Felucca 綁在一起 | 借設計，不照搬 |
| [FM1 Developers' Wiki：AI Skills](https://sbegueria.github.io/fm1-dev-wiki/AI-Skills.html) | 社群 skills 清單、各 skill 與 wiki 的差異 | 帶證據標記，列出 skill 寫錯的地方 | 不是 skill 本身 | 查核來源 |
| [fm1-base](https://github.com/charlesvestal/fm1-base) | MPL-2.0 的 firmware base，附 `felucca-to-fm1-base` skill | 把 Felucca 系 firmware 搬到 fm1-base | beta；只管移植，不是通用管理 skill | 之後寫 firmware 時的技術基礎 |

### maxfridbe/mvave-fm1

兩個 skill：`mvave-fm1-firmware`（備份、以 ORBIT 為例的刷機、還原、unbrick、protocol、flash map、安全規則）與 `mvave-fm1-software-development`（hardware API、memory map、build / package、firmware catalog）。另有 `docker/Containerfile` 與 `mvave-build.sh`，在容器裡用 JieLi toolchain build；刷機一律在 host 上做，不在容器內。`[上游]`

它的 README 自己寫明驗證範圍 `[上游]`：

- **實機驗證過**（一台 FM-1、Linux）：讀取 identity、用 MvaveFM1Unbricker 做兩次讀取比對的完整備份、安裝 ORBIT 0.7.1 release package。
- **CI 驗證過**：映像檔 build 成功，Dinghy、Felucca、ORBIT 都 build 完沒有失敗步驟。
- **沒有驗證**：刷寫容器 build 出來的 package、從 custom firmware 回到原廠、從其他 repo 文件轉述的說法。

FM1 Developers' Wiki 列出 skill 和 wiki 不一致的地方（截至 2026-10-29）：例如 skill 說 SoC 「沒有 FPU」，wiki 說有 single-precision FPU；skill 寫的 OTA loader USB ID 和 wiki 對 Felucca / ORBIT loader 的記錄不同。wiki 也指出 `mvave-fm1-firmware` 是圍繞 ORBIT 寫的，`FM-1_900` identity、OCT− rescue 等是 ORBIT 專屬，用在別的 firmware 要換掉。`[上游]`

### kmplngj/Felucca

2026-11-17 從 GitHub 看到的結構 `[上游]`：

- `AGENTS.md` 是唯一的指示檔，`CLAUDE.md` 只 import 它。
- `.agents/skills/` 有六個 skill：`issue-delivery`、`full-stop`、`full-send`、`pr-testing`、`firmware-watch`、`sol-loop`。
- 硬體相關的改動必須有實機驗證；依 wiki 的整理，tagged release 需要 FM-1 測試報告和 maintainer 核可，agent 寫測試步驟，由人來跑。
- `firmware-watch` 審視其他 firmware 的 commits，把評估過的項目記在 JSON/JSONL ledger。

這些 skill 的細節（lease、worktree、四個 repo 的 issue 盤點）都是為 Felucca 的工作量設計的，對一台機器的筆記 repo 太重。

## 怎麼安裝社群 skill

一般做法（依 maxfridbe README）：把 skill 資料夾複製或 symlink 到 `~/.claude/skills/`（全域）或某個專案的 `.claude/skills/`。skill 本身就是 Markdown，不安裝也能直接讀。其他支援 `SKILL.md` 的 agent client 要放到該 client 自己的 skills 目錄。

!!! warning "裝之前先讀完"
    skill 會把它寫下的「事實」帶進每一個 session。裝之前對照 FM1 Developers' Wiki 的 AI Skills 頁，確認哪些內容已知過時。skill 裡的任何刷寫步驟，仍然受這個 repo 的[安全規則](../getting-started/cautions.md)約束。

這個 repo 目前只帶一個 skill：`.agents/skills/zensical-site-bootstrap`，用來維護這個文件網站，和 FM-1 本身無關。

## 這個 repo 的做法

- **`AGENTS.md` 是 canonical。** 所有 agent 共用同一份規則：證據標記、硬體安全分級、preflight。`CLAUDE.md` 只 import 它，這是向 kmplngj/Felucca 學的。
- **不整包複製社群 skill。** 它們是參考來源，不是規則來源；有用的事實查核後寫進 `docs/`，並標上來源。
- **之後再做一個很薄的 repo-owned skill + 固定 scripts。** 原則是 **Skill decides, script executes**：skill 決定要做什麼、讀結果、向擁有者報告；每個實際動作都由固定、可測試的 script 執行，不讓 LLM 自己拼 flash 位址、USB 指令或 erase 操作。

!!! note "還不存在"
    下面是規劃，不是現況。repo 裡目前沒有 `fm1` skill、沒有 `fm1` CLI wrapper，也沒有 build toolchain（見 `AGENTS.md` 的 Not set up yet）。

預計的結構草圖：

```text
.agents/skills/fm1/
  SKILL.md            # 何時用、先讀 AGENTS.md 與 docs/device-log.md、呼叫哪個 script
scripts/fm1/
  info.sh             # 包 m-flasher find / info，唯讀
  backup.sh           # 完整備份 + SHA-256，寫進 backups/（git-ignored）
  verify-package.sh   # 比對下載的 .fwsc 與 release asset digest
  emu-smoke.sh        # 在 fm1-emu 開機、截圖，回報 emulator pass
  preflight.sh        # 印出 AGENTS.md 要求的四項 preflight，不寫入任何東西
```

刷寫本身不會有 script：firmware write 仍由擁有者在當下的 session 核可，並親自在 M-Flasher 輸入 `yes`。分級見 [Agent 自動化](index.md) 與 [M-Flasher CLI](m-flasher-cli.md)。

## 借來的兩個設計

1. **Hardware gate。** 能自動化的盡量自動化：文件 build、hash 比對、[emulator](emulator.md) 測試。但「在實機上可行」只能由擁有者確認並記進[我的機器](../device-log.md)，emulator pass 不升級成 `[實機]`。
2. **Firmware watch ledger。** 不在每個新 session 重新搜尋所有 repo，而是維護一份結構化的追蹤紀錄：哪個上游、哪個版本、看過什麼、要不要採用。這個 repo 的輕量版就是[資源地圖](../resources.md)的版本表與查核日期；之後可以改成機器可讀的格式，由 agent 定期比對。
