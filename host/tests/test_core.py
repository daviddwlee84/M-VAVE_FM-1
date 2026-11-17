from conductor import protocol as P
from conductor.backends.fake import FakeBackend
from conductor.backends.hc import HcBackend
from conductor.core import Conductor
from conductor.device import SimLink
from conductor.model import Agent
from conductor.slots import SlotMap


def setup(tmp_path, agents=None):
    be = FakeBackend(agents if agents is not None else FakeBackend.demo().list_agents())
    sim = SimLink(interactive=False, out=open(tmp_path / "sim.txt", "w"))
    c = Conductor(backend=be, send=sim.send, slots=SlotMap(tmp_path / "slots.json"), log=lambda *_: None)
    for f in sim.poll():  # the device's HELLO
        c.on_frame(f)
    return be, sim, c


def press(sim, c, cmd):
    sim.press(cmd)
    outs = [c.on_frame(f) for f in sim.poll()]
    return [o for o in outs if o]


def test_hello_gives_full_table(tmp_path):
    be, sim, c = setup(tmp_path)
    assert len(sim.slots) == 5
    assert {v["name"] for v in sim.slots.values()} >= {"fix-tests", "refactor-api"}


def test_approve_only_when_blocked(tmp_path):
    be, sim, c = setup(tmp_path)
    s_blocked = c.slots.slot_of("f1")
    s_working = c.slots.slot_of("f2")
    press(sim, c, f"sel {s_working}")
    (out,) = press(sim, c, "approve")
    assert not out.ok and out.text == "NOT WAITING"
    press(sim, c, f"sel {s_blocked}")
    (out,) = press(sim, c, "approve")
    assert out.ok
    assert ("f1", "keys", ["enter"]) in be.sent


def test_stale_tag_is_refused(tmp_path):
    be, sim, c = setup(tmp_path, [Agent("a", "one", P.K_CLAUDE, P.S_BLOCKED)])
    s = c.slots.slot_of("a")
    press(sim, c, f"sel {s}")
    # the agent leaves and another one takes its key before the press arrives
    be.remove("a")
    be.add(Agent("b", "two", P.K_CLAUDE, P.S_BLOCKED))
    sim.q.put(P.device_action(P.OP_APPROVE, s, sim.slots[s]["tag"]))
    outs = [c.on_frame(f) for f in sim.poll()]
    assert outs[-1].text in ("CHANGED, LOOK AGAIN", "EMPTY SLOT")
    assert not any(k == "keys" for _, k, _ in be.sent)


def test_always_needs_configured_kind(tmp_path):
    be, sim, c = setup(tmp_path, [Agent("o", "oc", P.K_OPENCODE, P.S_BLOCKED)])
    press(sim, c, f"sel {c.slots.slot_of('o')}")
    (out,) = press(sim, c, "always")
    assert not out.ok and "ALWAYS" in out.text


def test_continue_refuses_busy(tmp_path):
    be, sim, c = setup(tmp_path)
    press(sim, c, f"sel {c.slots.slot_of('f2')}")
    (out,) = press(sim, c, "cont")
    assert out.text == "BUSY"
    press(sim, c, f"sel {c.slots.slot_of('f3')}")
    (out,) = press(sim, c, "cont")
    assert out.ok and ("f3", "text", "continue") in be.sent


def test_slots_survive_restart(tmp_path):
    be, sim, c = setup(tmp_path)
    before = dict(c.slots.current)
    c2 = Conductor(backend=be, send=lambda f: None, slots=SlotMap(tmp_path / "slots.json"))
    c2.refresh()
    assert c2.slots.current == before


def test_only_changes_are_resent(tmp_path):
    be, sim, c = setup(tmp_path)
    sent = []
    c.send = sent.append
    c.refresh()
    assert not [f for f in sent if P.parse(f).type == P.T_SLOT]
    be.set_state("f2", P.S_DONE)
    c.refresh()
    slots = [f for f in sent if P.parse(f).type == P.T_SLOT]
    assert len(slots) == 1 and slots[0][6] == c.slots.slot_of("f2")  # F0 7D 43 4E ver type SLOT


def test_hc_terminal_mapping():
    t = {
        "address": "box/t7",
        "state": "live",
        "labels": {},
        "status": {"state": "blocked", "app": "codex", "kind": "question"},
        "foreground": {"name": "codex", "cwd": "/w"},
    }
    a = HcBackend._agent(t, show_shells=False)
    assert (a.kind, a.state, a.question, a.remote) == (P.K_CODEX, P.S_BLOCKED, True, True)
    shell = {"address": "t5", "state": "live", "labels": {}, "status": None, "foreground": {"name": "zsh"}}
    assert HcBackend._agent(shell, show_shells=False) is None
    assert HcBackend._agent(shell, show_shells=True).kind == P.K_SHELL


def test_multi_backend_merges_and_routes():
    from conductor.backends.multi import MultiBackend

    a, b = FakeBackend([Agent("t7", "in-hc", P.K_CLAUDE, P.S_BLOCKED)]), FakeBackend([Agent("wV:p1", "in-herdr", P.K_CODEX, P.S_IDLE)])
    a.name, b.name = "hc", "herdr"
    m = MultiBackend([a, b])
    assert m.name == "hc+herdr"
    assert sorted(x.id for x in m.list_agents()) == ["hc:t7", "herdr:wV:p1"]
    m.keys("herdr:wV:p1", ["enter"])
    assert b.sent == [("wV:p1", "keys", ["enter"])] and a.sent == []


def test_display_name():
    from conductor.model import display_name

    assert display_name("●Tadronaut-Mono-Repo: done", "x") == "Tadronaut-Mono-Repo: done"
    assert display_name("✳ audit-sizing", "x") == "audit-sizing"
    assert display_name("投資研究 | Invest", "repo") == "repo"
    assert display_name("", "repo") == "repo"


def test_kind_names_as_agents_report_them():
    from conductor.model import kind_of, kind_of_argv

    assert kind_of("claude-code") == P.K_CLAUDE
    assert kind_of("/opt/homebrew/bin/codex") == P.K_CODEX
    assert kind_of("specstory") == P.K_OTHER
    assert kind_of_argv(["specstory", "run", "claude"]) == P.K_CLAUDE
    t = {"address": "t1b", "state": "live", "labels": {}, "title": "●zhouhanru: done",
         "status": {"app": "claude-code", "state": "done"},
         "foreground": {"name": "specstory", "argv": ["specstory", "run", "claude"], "cwd": "/Users/x"}}
    a = HcBackend._agent(t, show_shells=False)
    assert (a.kind, a.state, a.name) == (P.K_CLAUDE, P.S_DONE, "zhouhanru: done")


class _Keys:
    def __init__(self):
        self.events = []

    def post(self, code, down, flags):
        self.events.append((code, down, flags))


def test_voice_hold_presses_and_always_lets_go(tmp_path):
    from conductor.voice import VoiceKey

    be, sim, c = setup(tmp_path)
    keys = _Keys()
    c.voice = VoiceKey(["ctrl", "option"], poster=keys)
    press(sim, c, f"sel {c.slots.slot_of('f2')}")
    (out,) = press(sim, c, "voice")
    assert out.text == "LISTENING"
    assert keys.events == [(0x3B, True, 0x40000), (0x3A, True, 0xC0000)]
    assert ("f2", "focus", None) in be.sent  # the agent's pane first, so the words land there
    (out,) = press(sim, c, "unvoice")
    assert keys.events[2:] == [(0x3A, False, 0x40000), (0x3B, False, 0)]
    # held, then the device restarts: let go
    press(sim, c, "voice")
    sim.q.put(P.device_hello())
    for f in sim.poll():
        c.on_frame(f)
    assert keys.events[-1] == (0x3B, False, 0) and c.voice.held_since is None


def test_voice_times_out_and_toggle_mode():
    from conductor.voice import VoiceKey

    keys = _Keys()
    v = VoiceKey(["f19"], max_hold=5, poster=keys)
    v.press()
    v.tick(v.held_since + 6)
    assert keys.events[-1] == (0x50, False, 0) and v.held_since is None
    t = VoiceKey(["option", "space"], mode="toggle", poster=_Keys())
    t.press()
    assert [d for _, d, _ in t.poster.events] == [True, True, False, False]
    assert t.release() == ""


def test_device_lines_ascii_and_wrap():
    from conductor.core import device_lines

    out = device_lines(["╭─ Bash ─╮", "❯ 1. Yes", "", "x" * 50, "中文 ok"], 36)
    assert out[0] == "+- Bash -+" and out[1] == "> 1. Yes"
    assert out[2] == "x" * 36 and out[3].startswith("  x")
    assert out[4] == "?? ok"
    assert all(all(0x20 <= ord(c) < 0x7F for c in ln) for ln in out)


def test_detail_scrolls_back(tmp_path):
    be, sim, c = setup(tmp_path)
    be.read_lines = lambda aid, n: [f"line {i}" for i in range(30)]
    sim.q.put(P.device_select(0))
    for f in sim.poll():
        c.on_frame(f)
    assert sim.detail[7] == "line 29"
    sim.q.put(P.device_select(0, scroll=2))  # two detents: eight lines back
    for f in sim.poll():
        c.on_frame(f)
    assert sim.detail[0] == "line 14" and sim.detail[7] == "line 21"


CLAUDE_SCREEN = """\
⏺ I re-tested it in hc; it looks fine now.
✻ Worked for 1m 17s · done 6:46 PM
─── History 54/54 ──────────────────────
❯ 好像在 hc 裡面跑 codex 會 crash
──────────────────────────────────────
  [Opus 5.5 ◑ high] │ zhouhanru │ ⏱️  40m │ Cost $0.477
  Context    █░░░░░░░░░ 6% (60k/1.0M)
  Approx RAM ████░░░░░░ 8.9 GB / 24 GB (37%)
  1 rules | 11 hooks
  ✓ Bash ×9
  Tokens 612k (in: 24, out: 5k, cache: 607k)
  ⏵⏵ bypass permissions on (shift+tab to cycle)""".splitlines()


def test_strip_chrome_keeps_the_conversation():
    from conductor.core import strip_chrome

    assert strip_chrome(CLAUDE_SCREEN) == CLAUDE_SCREEN[:2]


def test_strip_chrome_keeps_a_permission_menu():
    from conductor.core import strip_chrome

    screen = ["⏺ Bash(make test)", "╭────────────────╮", "│ Do you want to proceed?", "──────────",
              "❯ 1. Yes", "  2. Yes, and don't ask again", "  3. No", "  ⏵⏵ bypass permissions on (shift+tab to cycle)"]
    out = strip_chrome(screen)
    assert out[-1] == "  3. No" and "❯ 1. Yes" in out


def test_detail_as_pictures_for_newer_firmware(tmp_path):
    from conductor.render import PACKED, LineRenderer, unpack7

    be, sim, c = setup(tmp_path)
    c.renderer = LineRenderer()
    be.read_lines = lambda aid, n: ["請把錯誤訊息重貼一次", "> 1. Yes"]
    sent = []
    c.send = sent.append
    c.on_frame(P.device_hello(0, 2))
    c.on_frame(P.device_select(0))
    pics = [P.parse(f) for f in sent if P.parse(f).type == P.T_DETAIL_BMP]
    assert len(pics) == P.DETAIL_LINES
    assert all(len(m.payload) == 2 + PACKED for m in pics)
    assert all(len(f) <= 640 + 2 for f in sent)  # fm1-base: 640 bytes between F0 and F7
    assert sum(unpack7(pics[0].payload[2:])) > 50  # the Chinese line has ink
    # firmware 0.1 still gets text
    sent.clear()
    c.on_frame(P.device_hello(0, 1))
    c.on_frame(P.device_select(0))
    assert not [f for f in sent if P.parse(f).type == P.T_DETAIL_BMP]


CODEX_SCREEN = [
    "  >_ OpenAI Codex (v0.169.0)",
    "  permissions: YOLO mode",
    " " * 120 + "⠈⢿⣿⣿⣦⡀⠈⠛⠿⣿⣿⣿⣿⣄⡀  ⢀⣠⣼⣿⣿",
    "• Ran make test, 12 passed",
    "",
    "› Ask Codex to do anything",
    "",
    "  GPT-6.1-Sol xhigh · Standard · Ready · Context 100% left · ~",
    "  ← for agents · ? for shortcuts",
]


def test_codex_screen():
    from conductor.core import strip_chrome, tui_lines

    out = tui_lines(strip_chrome(CODEX_SCREEN))
    assert out == ["  >_ OpenAI Codex (v0.169.0)", "  permissions: YOLO mode", "* Ran make test, 12 passed"]


def test_missing_glyphs_become_question_marks():
    from conductor.render import LineRenderer

    r = LineRenderer()
    assert r.clean("ok 中文 \U0001F9EA") == "ok 中文 ?"
