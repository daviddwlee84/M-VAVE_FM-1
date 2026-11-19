# METRONOME v0.2 計畫

## Context 背景

v0.1 已在實機（2026-11-20），本人試用後的回饋：
1. 想要古典速度術語（Largo、Allegro…）和 BPM 的對照。
2. 拍號 UX：SET 上顯示「4/4 QUARTERS」或「CLASSIC BEAT 5」，兩種寫法不一致。本人後來發現 GRID 的 K1/K2 能表達各種拍子，**決定保留 GRID / CLASSIC 各自的拍數設計，不統一**；另外問到要不要支援 6/8。
3. 歌名延到很後面。改成需要「演出完一鍵全部清空」。
4. Hub 的 identity 檢查：`FM-1_59` 被判格式錯誤，是因為它要的是韌體實際回報的 3–8 位數字（`FM-1_5900`、release `FM-1_59000200`）。2026-11-20 拿 catalog 的 269 個 regex 比對過，`FM-1_5900`、`FM-1_59000100`、`FM-1_59000200` 都沒有人用（只有萬用的 `FM-1_\d{3,8}`）。**identity 不用改。**

本人的決定：SELECT 加速、速度術語「顯示 + 跳段旋鈕」、全部清空用「SET EDIT 長按 SEL 2 秒，PLAY 確認」。

record 的格式不變（`REC_VER 1`、460 B），實機上 v0.1 存的歌直接沿用，不用 migration。`FW_MINOR` 改成 2。

## Changes 變更

在 branch `claude/metronome-0.2` 上做（已經從 main 開好）。

### 1. 速度術語（`metro.c` / `metro.h`，平台無關，可以單元測試）
- `const char *mt_marking(int bpm)`，表格不重疊，用 Maelzel 節拍器刻度。Wikipedia 有重疊的地方（Andante／Moderato）用這個刻度切開，60–65 補上 LARGHETTO：
  `30–39 LARGHISSIMO · 40–59 LARGO · 60–65 LARGHETTO · 66–75 ADAGIO · 76–107 ANDANTE · 108–119 MODERATO · 120–167 ALLEGRO · 168–199 PRESTO · 200–300 PRESTISSIMO`
- `int mt_marking_jump(int bpm, int dir)`：跳到下一段（或上一段）的典型速度：35, 50, 63, 70, 92, 112, 132, 184, 208。如果不在典型值上，往下轉會先回到本段的典型值。
- 顯示位置：SET／GRID 左上角大字 BPM 下方，原本的「BPM」改成「BPM · ALLEGRO」（太寬就換行到拍號那行上面）。CLASSIC 的 `TEMPO` 標籤旁邊也顯示。
- 操作：**SET 頁 K2**＝跳段（K1 還是換歌時機）。toast 和語音說「ALLEGRO 132」。換段時用 `bpm_by` 同一條路徑（`tr_follow(FOLLOW_EDIT)`，不掉拍）。

### 2. SELECT 加速（`app.c` `input()`）
- 一個 frame 內轉了 |e| ≥ 3 格就乘 5（或依 frame 間隔判斷），慢轉維持 ±1。OCT 在 SET／GRID 還是 ±5。

### 3. 拍號顯示的 UX（`app.c` `draw_tempo_left`、`meter_say`、`draw_steps`）
- 兩種模式用同一種格式：GRID 顯示 `4/4 · QUARTERS`，CLASSIC 顯示 `BEAT 5 · CLASSIC`（BEAT 0 顯示 `NO ACCENT · CLASSIC`）。SET 的歌格裡加一個小字標出是 G 還是 C。
- **6/8 的回答**：GRID 的 `bpb=2, div=3` 在聲音上就是 6/8（每拍是附點四分，分成三格）。所以 div=3 時拍號改顯示成複拍子：1→3/8、2→6/8、3→9/8、4→12/8，BPM 指的是附點四分。K2 的名稱保留 TRIPLETS。「數 6 下」那種 6/8（拍子單位是八分）需要新增「拍子單位」欄位，會改到 record，放進 roadmap，v0.2 不做。
- GRID 頁尾的提示加上 `K1: BEATS  K2: DIV`，讓人找得到（本人這次就沒發現）。

### 4. 全部清空（`app.c` `key_set`／`input`）
- SET EDIT 裡按住 SEL 2 秒、期間沒按琴鍵 → 畫面和語音顯示「CLEAR ALL? PLAY = YES」。
- 這時按 PLAY 才清空，而且不會啟動播放；按其他鍵或過 5 秒就取消。
- 清空後是 `defaults()`（只剩第 1 首，120 BPM 4/4），播放中就停止，3 秒後存檔。
- 原本「SEL + 白鍵＝清掉一首」不變：只要按住 SEL 的期間有按琴鍵，就不會觸發全部清空。

### 5. 文件
- `docs/dev/metronome.md`：操作表、v0.2 段落（速度術語表標 `[上游]` 並連到 zh.wikipedia 的〈速度 (音樂)〉，Maelzel 刻度的切法寫明是自己定的）、v0.2 驗證、roadmap（歌名移到最後；新增「拍子單位八分，6/8 數 6、7/8」和延遲校正）。identity 那行補上 Hub 的 3–8 位數規則。
- 同樣的 identity 規則寫進 `.agents/skills/fm1-hub-listing` 的相關段落（要的是韌體回報的完整數字，不是 prefix）。
- `fm1.toml` 的 description，`AGENTS.md` 的 metronome 列加上 v0.2。

### 6. 測試
- `firmware/metronome/tests/metro_test.c`：`mt_marking` 每段的邊界（39/40、59/60…、299/300），`mt_marking_jump` 雙向、在端點夾住。
- `tests/basic.txt`：新增 expect（`app_expect` 加 `marking` 和 `confirm`）：K2 跳段讓 BPM 照順序走；div=3 時 label 是 6/8（用 shot 確認）；長按 SEL 2 秒 → confirm=1 → PLAY → used==1、bpm 120、running 0；按其他鍵會取消；SELECT 快轉一次 +5 以上。重開機後的檢查保留。
- 截圖更新到 `docs/assets/metronome/`。

## Verification 驗證
1. `just metro-unit`、`just fw-test metronome` 全過；`just fw-build metronome`（記下大小、SHA）。
2. fm1-emu 兩種晶片 headless（`FM1_BENCH_KEYS`）：開機 0 次寫入；改一首存檔只寫 `0xCE000`；重開讀回。另外要確認 v0.1 的 record 讀得進來：疊在 `backups/2026-11-20-pre-metronome-0.1` 上不行，因為那份裡 `0xCE000` 是空的。刷機前的 preflight 會做新的完整備份，裡面有本人用 v0.1 存的歌。把 v0.2 疊在那份上預演，確認那些歌和 BPM 都還在。
3. 開 PR，照 template：`just fw-preview metronome pr-N`，附截圖和實機試用清單。
4. 刷機另外請本人批准，走完整 preflight，由本人用 `fm1_install.py` 刷。
