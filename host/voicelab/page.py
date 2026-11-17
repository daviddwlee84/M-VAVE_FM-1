"""build/voicelab/index.html: every engine in one table, a player per cell, sizes, and Whisper's score if run."""

from __future__ import annotations

import html
import json

from .lab import OUT, slug

KB = 1024


def _engine_stats(e: dict) -> tuple[float | None, int]:
    """Mean bytes per single word (sentences left out), and how many words fit in 300 KB after the shared data."""
    sizes = [it["bytes"] for it in e["items"].values() if it.get("bytes") and " " not in it["text"]]
    if not sizes:
        return None, 0
    mean = sum(sizes) / len(sizes)
    room = 300 * KB - e.get("shared_kb", 0) * KB
    return mean, int(room // mean) if room > 0 else 0


def render() -> str:
    res = json.loads((OUT / "results.json").read_text())
    sc_path = OUT / "scores.json"
    sc = json.loads(sc_path.read_text()) if sc_path.exists() else {}
    scores: dict = sc.get("scores", {})
    model: str = sc.get("model", "")
    engines = [e for e in res["engines"] if not e.get("skipped")]
    words = res["words"]

    head = []
    for e in res["engines"]:
        mean, fit = _engine_stats(e)
        s = scores.get(e["key"], {})
        acc = f"{sum(v['ok'] for v in s.values())}/{len(s)}" if s else "—"
        shared = f"{e['shared']} ≈ {e['shared_kb']:.0f} KB" if e.get("shared_kb") else (e.get("shared") or "—")
        head.append(
            f"<tr><td><b>{html.escape(e['name'])}</b><br><code>{e['key']}</code></td>"
            f"<td>{html.escape(e['where'])}</td><td>{html.escape(e['licence'])}</td>"
            f"<td class=n>{'%.0f B' % mean if mean else html.escape(e['size_note'])}</td>"
            f"<td>{html.escape(shared)}</td><td class=n>{fit or '—'}</td><td class=n>{acc}</td>"
            f"<td>{html.escape(e.get('skipped') or '')}</td></tr>")

    rows = []
    for w in words:
        sl = slug(w)
        cells = []
        for e in engines:
            it = e["items"].get(sl, {})
            if "wav" not in it:
                cells.append(f"<td class=err title='{html.escape(it.get('error', ''))}'>—</td>")
                continue
            hit = scores.get(e["key"], {}).get(sl)
            mark = ""
            if hit:
                mark = f"<div class={'ok' if hit['ok'] else 'bad'}>{'✓' if hit['ok'] else '✗'} {html.escape(hit['heard'])}</div>"
            size = f"<div class=sz>{it['bytes']} B</div>" if it.get("bytes") else ""
            cells.append(f"<td><audio controls preload=none src='{it['wav']}'></audio>{size}{mark}</td>")
        rows.append(f"<tr><th>{html.escape(w)}</th>{''.join(cells)}</tr>")
    cols = "".join(f"<th>{html.escape(e['name'])}</th>" for e in engines)

    return f"""<!doctype html><html lang=zh-Hant><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1"><title>FM-1 Voice Lab</title>
<style>
:root{{--bg:#fff;--fg:#1d1d1f;--mute:#6e6e73;--line:#d2d2d7;--ok:#1a7f37;--bad:#c4302b}}
@media (prefers-color-scheme:dark){{:root{{--bg:#1c1c1e;--fg:#f5f5f7;--mute:#a1a1a6;--line:#3a3a3c;--ok:#4ac26b;--bad:#ff6b5e}}}}
body{{background:var(--bg);color:var(--fg);font:14px/1.45 -apple-system,"PingFang TC",sans-serif;margin:16px}}
h1{{font-size:20px}} p{{color:var(--mute);max-width:70em}}
table{{border-collapse:collapse;margin:12px 0}} td,th{{border:1px solid var(--line);padding:6px 8px;vertical-align:top;text-align:left}}
.n{{text-align:right;white-space:nowrap}} .sz{{color:var(--mute);font-size:12px}} .ok{{color:var(--ok);font-size:12px}}
.bad{{color:var(--bad);font-size:12px}} .err{{color:var(--mute)}} audio{{width:150px;height:30px}}
.wrap{{overflow-x:auto}} code{{font-size:12px;color:var(--mute)}}
</style>
<h1>FM-1 Voice Lab</h1>
<p>同一組測試字，用每一種發音方案各念一次。LPC、ADPCM 的來源是 macOS <code>say</code>（Samantha），只供這台 Mac 試聽；
真的要做字卡時要換成可以散佈的聲音（例如 Kokoro，Apache-2.0）。「300 KB 可放」以單字平均大小、扣掉共用資料計算。
「Whisper」是語音辨識模型 {html.escape(model)} 聽懂的比例（<code>just voice-score</code>），只拿來互相比較。
說明與結論：<code>docs/dev/tts.md</code>。</p>
<div class=wrap><table><tr><th>方案</th><th>在哪裡跑</th><th>Licence</th><th>每字平均</th><th>共用資料</th>
<th>300 KB 可放</th><th>Whisper</th><th>略過原因</th></tr>{''.join(head)}</table></div>
<p>每格下面：每字大小，以及 Whisper 聽到的字。✓ = 目標字以完整的字出現在辨識結果裡（不分大小寫、標點，數字換成英文；不是逐字 exact match），✗ 後面是它實際聽成什麼。Whisper 會往常見字猜（ship → Shit），音檔太怪時會冒出一串數字，所以分數只是下限，拿來比較同一個字在不同方案的表現。詳見 <code>docs/dev/tts.md#scoring</code>。</p>
<div class=wrap><table><tr><th></th>{cols}</tr>{''.join(rows)}</table></div>
<p>在模擬器裡自己玩這些會講話的社群韌體：<code>just voice-fetch</code> 之後
<code>just emu-run downloads/speak37-fm1-1.0.fwsc</code>、<code>spellbox-0.3.fwsc</code>、<code>386dx-2.0.fwsc</code>、
<code>chimera-0.8.0.fwsc</code>、<code>evilgenius-0.1.4.fwsc</code>。</p>
</html>"""


def write() -> None:
    (OUT / "index.html").write_text(render(), encoding="utf-8")
    print(f"page: {OUT / 'index.html'}")
