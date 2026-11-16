# SLOOP 概觀

[SLOOP](https://github.com/isod89/sloop-fm1) 把 FM-1 變成一台用雙手即興演奏的四軌 groovebox：三軌 synth 加一軌 drum。沒有內建 pattern，沒有東西要載入，聽到的都是自己彈出來的。

安裝見[刷 SLOOP 2.8.5](../../getting-started/flash-sloop.md)。這頁是規格與概念，實際操作見[操作速查](usage.md)。

## 規格（2.8）

| | |
|---|---|
| Tracks | 3 synth（共用 8 voices）+ drums（16 個聲音、6 voices） |
| Engines | 12 種：analog、4-op FM、6-op FM（FM6，可匯入 DX7 patch）、phase distortion、三振盪器、tonewheel organ、formant voice、granular、lo-fi chip、sampler、physical models（PHYS）、noise |
| 音色 | 153 個 preset、32 個 user preset、27 格 FM6 patch bank |
| Drum kits | 37 組（5 組取樣、32 組合成），外加自製的 USR1–USR4 與 SYN1–SYN4 |
| Sequencer | 每軌 64 steps，各自的長度與 division；parameter locks、micro timing、fills、swing |
| 錄音 | 即時錄音、overdub；free take（速度跟著你）或固定 tempo |
| 效果 | 27 個 punch-in effect；master DUST、DUCK、DJ filter、limiter；每軌 drive、slicer、chorus / delay / reverb send |
| 記憶 | autosave、undo / redo、一首歌 4 個 section（A–D） |
| Samples | 4 個 slot，每個約 7.4 秒 |
| Audio | 44.1 kHz fixed-point DSP；USB audio 輸入（master 輸出，16-bit stereo） |
| MIDI | USB in/out、TRS MIDI IN、MIDI clock in |
| 不支援 | Bluetooth MIDI；USB 鍵盤直插 |

## 核心概念

**每個功能鍵都是一個 layer。** 按住它，16 個白鍵與四個旋鈕的工作就換了，螢幕會顯示現在是什麼。輕點則是開它的頁面。按住 layer 鍵再點 `HOME` 可以把它鎖住，空出雙手。

**顏色對應 track。** 藍色是 track 1 與 KNOB 1，綠色 2，黃色 3，橘色 4（drums）。白色是你正在碰的東西，紅色是錄音中。

**Autosave。** 停止後放 2.5 秒，內容就存下來；開機時會回到上次離開的狀態。

**機器上只有一首歌。** 4 個 section（A–D）。更多首歌用 [Web Editor](web-editor.md) 存成 backup 檔。

## 狀態

SLOOP 2.8 被作者標為 stable、tried and tested，但 README 仍寫 "Still a beta"，自行承擔風險。更新時 projects、presets、samples、設定都會保留。

## 上游文件

- [README](https://github.com/isod89/sloop-fm1/blob/main/README.md)：功能、安裝、MIDI、troubleshooting
- [SLOOP.md](https://github.com/isod89/sloop-fm1/blob/main/SLOOP.md)：完整手冊
- [GUIDE.md](https://github.com/isod89/sloop-fm1/blob/main/GUIDE.md)：教學
- [Installer](https://isod89.github.io/sloop-fm1/) · [Web Editor](https://isod89.github.io/sloop-fm1/webapp/editor/)
