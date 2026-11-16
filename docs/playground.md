# 線上試玩

不用刷機、不用安裝，在瀏覽器裡就能玩的地方。用 Chrome 或 Edge；要接 MIDI 鍵盤或實機時瀏覽器會要求 MIDI 權限。網址都在 2026-11-17 確認有回應，我實際玩過的只有標 `[實機]` 的。

## 通用的：一個網站玩很多韌體

| 網站 | 是什麼 | 特點 |
|---|---|---|
| [fm1-emu](https://joakimeriksson.github.io/fm1-emu/) | 晶片層級的 FM-1 模擬器，選一個韌體按 Start | 原封不動地跑 `.fwsc`。Firmware Hub 的 Try、RemoteFM1 的 Emulated 模式用的都是它 |
| [FM-1 Player（remotefm1.com）](https://remotefm1.com/) | 列了 240 多個韌體的播放器 | 帶圓點的是原生版本（較快、延遲低），其餘用 fm1-emu。可搭配 [RemoteFM1](firmware/remotefm1/index.md) 把實機當面板 |
| [The Firmware Hub](https://thefirmwarehub.com/) | 韌體目錄，每張卡片有 Try | 可篩選「Saves on newer FM-1s」。見 [The Firmware Hub](firmware/hub.md) |
| [VIBE FM Playground](https://play.thefirmwarehub.com/) | VIBE FM 的網頁版 | **不是 FM-1**，是另一台尚未上市的機器的面板；跑的是為它轉換過的 FM-1 韌體。見[相關硬體](hardware/related.md) |
| [fm1-emu：SMK-37](https://joakimeriksson.github.io/fm1-emu/smk37/) | SMK-37 鍵盤的模擬器 | 同晶片的另一台機器 |

fm1-emu 的網址參數：

| 參數 | 作用 | 範例 |
|---|---|---|
| `?fw=<id>` | 直接開某個韌體 | [`?fw=sloop`](https://joakimeriksson.github.io/fm1-emu/?fw=sloop)、`felucca`、`melodee`、`clipper`、`orbit` |
| `&chip=uc25hq80` | 用新款 flash 晶片（和我的機器一樣） | [`?fw=sloop&chip=uc25hq80`](https://joakimeriksson.github.io/fm1-emu/?fw=sloop&chip=uc25hq80) |
| `?package=<URL>` | 開清單上沒有的套件 | 需要 https 與 CORS |
| `&embed=1` | 只顯示機器，適合嵌入 | — |

## 各韌體自己的試玩頁

| 韌體 | 試玩 | 相關頁面 |
|---|---|---|
| **ChoralRoot** | [模擬器](https://quixotic7.github.io/ChoralRootFM1/emu/)；[device manager 模擬模式](https://quixotic7.github.io/ChoralRootFM1/webapp/installer/manager.html?sim=1) | [ChoralRoot](firmware/choralroot/index.md)、[教學](firmware/choralroot/tutorial.md) |
| **CTL-1** | [模擬器](https://charlesvestal.github.io/fm1-ctl/emu/) | [CTL-1](firmware/ctl-1/index.md) |
| **Agent Deck** | [自動播放的展示](https://kevinelliott.github.io/fm-1-agent-deck/play/?fw=agtdeck&script=demo.txt)；[cheat sheet](https://kevinelliott.github.io/fm-1-agent-deck/cheatsheet.html) | [Agent Deck](firmware/agent-deck/index.md) |
| **SLOOP** | [fm1-emu 裡的 SLOOP](https://joakimeriksson.github.io/fm1-emu/?fw=sloop&chip=uc25hq80)；[Web Editor](https://isod89.github.io/sloop-fm1/webapp/editor/)（要接實機）；[Editor 的 mock 模式](https://isod89.github.io/sloop-fm1/webapp/editor/?mock=1)（模擬一台機器，我沒試過） | [SLOOP](firmware/sloop/index.md)、[站內的 Editor 2.8.5](firmware/sloop/editor.md) |
| **X0X** | [模擬器](https://charlesvestal.github.io/fm1-x0x/emu/) | [韌體地圖](firmware/index.md) |
| **FM-1 SID** | [C64 SID 播放頁](https://joakimeriksson.github.io/fm1-emu/fm1-sid/) | — |
| **fm1-base template** | [模擬器](https://charlesvestal.github.io/fm1-base/emu/) | [自製韌體環境](dev/index.md) |
| Felucca、Melodee、Clipper | 用 fm1-emu 的 `?fw=felucca`、`melodee`、`clipper` | — |

## 官方的

| 網站 | 是什麼 |
|---|---|
| [FM1 Workbench](https://open.m-vave.com/fm1-workbench) | M-VAVE 官方的網頁工具，含官方開放韌體的瀏覽器預覽。見[官方資源](official.md) |

## 本機的對應

線上模擬器有延遲、存檔留在瀏覽器裡。想要低延遲、或想用實機備份當起點，用本機的 fm1-emu：

```sh
just emu-run downloads/choralroot-0.41.fwsc          # 開視窗玩，預設新晶片模式
just emu-rehearse <套件> <備份.bin>                   # 把套件蓋在實機備份上開機
```

見 [fm1-emu 模擬器](agentic/emulator.md)。

## 怎麼選

- 只是想聽聽某個韌體：**remotefm1.com**，韌體最多，原生版本的反應最好。
- 想確認某個韌體在新晶片上會不會存檔：**fm1-emu** 加 `&chip=uc25hq80`，或本機 `just emu-run`。
- 想用實機的鍵去彈模擬器裡的韌體：見 [ChoralRoot：不刷機怎麼玩](firmware/choralroot/index.md#emulate)。
- VIBE FM Playground 看起來最漂亮，但它模擬的不是我這台機器。
