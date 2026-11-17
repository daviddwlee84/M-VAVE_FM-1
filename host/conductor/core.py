"""The daemon's heart: backend state in, SysEx frames out; device intents in, keys out.

The device decides nothing about agents. It sends intents (ACTION op, slot, tag) and the
host checks them against a fresh read of the backend before it presses any key: the tag
must still name the agent on that slot, and the agent must be in a state the action
makes sense for (approve only when blocked).
"""

from __future__ import annotations

import time
from collections.abc import Callable
from dataclasses import dataclass, field

from . import protocol as P
from .backends.base import Backend, BackendError
from .model import KIND_SHORT, Agent
from .slots import SlotMap

# Keys per agent kind. "enter" picks the highlighted choice, which is "Yes" in the
# Claude Code and Codex permission menus. approve_always is only sent where a key is
# configured; these defaults are [調研] until checked against a real session.
DEFAULT_CONFIG = {
    "approve_keys": {"default": ["enter"]},
    "approve_always_keys": {"claude": ["2"], "codex": ["a"]},
    "deny_keys": {"default": ["esc"]},
    "continue_text": "continue",
    "detail_every": 2.0,
    "meta_every": 3.0,
    "full_every": 30.0,  # resend every slot now and then: heals a device that missed or got other frames
    # the dictation hotkey ARP holds down (Wispr Flow, Handy, ...): keys as macOS names
    "voice_keys": ["ctrl", "option"],
    "voice_mode": "hold",  # hold: push-to-talk; toggle: one press and release per button press
    "voice_focus": True,  # focus the selected agent's pane first, where the backend can
    "voice_max_hold": 120.0,
}

KIND_WORD = {v: k for k, v in {"claude": P.K_CLAUDE, "codex": P.K_CODEX, "opencode": P.K_OPENCODE}.items()}


def _keys_for(table: dict, kind: int) -> list[str] | None:
    return table.get(KIND_WORD.get(kind, "?"), table.get("default"))


@dataclass
class Outcome:
    ok: bool
    text: str


@dataclass
class Conductor:
    backend: Backend
    send: Callable[[bytes], None]
    slots: SlotMap = field(default_factory=SlotMap)
    config: dict = field(default_factory=lambda: dict(DEFAULT_CONFIG))
    log: Callable[[str], None] = print

    agents: dict = field(default_factory=dict)
    hidden: int = 0
    selected: int | None = None
    _slot_frames: dict = field(default_factory=dict)
    _detail: list = field(default_factory=list)
    _detail_at: float = 0.0
    _meta_at: float = 0.0
    _meta: bytes = b""
    _full_at: float = 0.0
    device_seen: float = 0.0
    voice: object = None  # a VoiceKey, or None: the button does nothing

    # ------------------------------------------------------------ backend -> device
    def refresh(self) -> None:
        now = time.time()
        if now - self._full_at >= self.config.get("full_every", 30.0):
            self._full_at = now
            self._slot_frames.clear()
        fresh = {a.id: a for a in self.backend.list_agents()}
        for a in fresh.values():
            old = self.agents.get(a.id)
            if old and old.state == a.state:
                a.since = old.since
        self.agents = fresh
        self.hidden = self.slots.assign(sorted(fresh, key=lambda i: fresh[i].since))
        for s in range(P.NSLOTS):
            aid = self.slots.agent_at(s)
            a = fresh.get(aid) if aid else None
            f = (
                P.slot(s, a.state, a.kind, a.flags, a.tag, a.age_min(now), a.name)
                if a
                else P.slot(s, P.S_EMPTY, 0, 0, 0, 0, "")
            )
            if self._slot_frames.get(s) != f:
                self._slot_frames[s] = f
                self.send(f)
        m = P.meta(len(fresh), self.hidden, self.backend.name)
        if m != self._meta or now - self._meta_at >= self.config["meta_every"]:
            self._meta, self._meta_at = m, now
            self.send(m)
        if self.selected is not None and now - self._detail_at >= self.config["detail_every"]:
            self.send_detail()

    def send_detail(self, force: bool = False) -> None:
        self._detail_at = time.time()
        aid = self.slots.agent_at(self.selected) if self.selected is not None else None
        lines: list[str] = []
        if aid:
            try:
                lines = self.backend.read_lines(aid, P.DETAIL_LINES)
            except BackendError as e:
                lines = [f"read failed: {e}"]
        lines = (lines + [""] * P.DETAIL_LINES)[: P.DETAIL_LINES]
        if self.selected is not None and (lines != self._detail or force):
            self._detail = lines
            for i, ln in enumerate(lines):
                self.send(P.detail(self.selected, i, ln))

    def resync(self) -> None:
        """The device (re)started: send it everything."""
        self._slot_frames.clear()
        self._meta = b""
        self._detail = []
        self.send(P.hello_ack())
        self.send(P.clear())
        self.refresh()

    # ------------------------------------------------------------ device -> host
    def on_frame(self, data: bytes) -> Outcome | None:
        try:
            m = P.parse(data)
        except P.ProtocolError as e:
            self.log(f"[device] {e}")
            return None
        if m is None:
            return None
        self.device_seen = time.time()
        if m.type == P.T_HELLO:
            self.log(f"[device] hello, protocol {m.payload[0] if m.payload else '?'}")
            if self.voice:
                self.voice.let_go()  # the device restarted: its button is not held any more
            self.resync()
        elif m.type == P.T_SELECT and m.payload:
            self.selected = m.payload[0] if m.payload[0] < P.NSLOTS else None
            self.send_detail(force=True)
        elif m.type == P.T_ACTION:
            out = self.act(P.parse_action(m))
            self.log(f"[action] {out.text}")
            self.send(P.footer(out.text))
            return out
        return None

    def act_voice(self, action: P.Action) -> Outcome:
        """Not about an agent: keys pressed on this computer, for a dictation app."""
        if not self.voice:
            return Outcome(False, "NO VOICE KEY SET")
        try:
            if action.op == P.OP_VOICE_UP:
                return Outcome(True, self.voice.release() or "VOICE")
            aid = self.slots.agent_at(action.slot)
            if aid and self.config.get("voice_focus") and self.voice.held_since is None:
                try:
                    self.backend.focus(aid)
                except BackendError:
                    pass
            return Outcome(True, self.voice.press())
        except OSError as e:
            return Outcome(False, str(e)[: P.FOOTER_MAX])

    def act(self, action: P.Action) -> Outcome:
        if action.op in (P.OP_VOICE_DOWN, P.OP_VOICE_UP):
            return self.act_voice(action)
        aid = self.slots.agent_at(action.slot)
        if aid is None:
            return Outcome(False, "EMPTY SLOT")
        # a fresh read: the device may be a second behind
        try:
            self.refresh()
        except BackendError as e:
            return Outcome(False, f"BACKEND: {e}"[: P.FOOTER_MAX])
        a: Agent | None = self.agents.get(aid)
        if a is None or self.slots.agent_at(action.slot) != aid or a.tag != action.tag:
            return Outcome(False, "CHANGED, LOOK AGAIN")
        label = f"{KIND_SHORT.get(a.kind, '??')} {a.name}"
        op = action.op
        try:
            if op in (P.OP_APPROVE, P.OP_APPROVE_ALWAYS, P.OP_DENY):
                if a.state != P.S_BLOCKED:
                    return Outcome(False, "NOT WAITING")
                table = {
                    P.OP_APPROVE: "approve_keys",
                    P.OP_APPROVE_ALWAYS: "approve_always_keys",
                    P.OP_DENY: "deny_keys",
                }[op]
                keys = _keys_for(self.config[table], a.kind)
                if not keys:
                    return Outcome(False, "NO ALWAYS FOR THIS KIND")
                self.backend.keys(aid, keys)
                word = {P.OP_APPROVE: "OK", P.OP_APPROVE_ALWAYS: "ALWAYS", P.OP_DENY: "NO"}[op]
                return Outcome(True, f"{word} {label}"[: P.FOOTER_MAX])
            if op == P.OP_INTERRUPT:
                return Outcome(True, f"STOP {self.backend.interrupt(aid)}"[: P.FOOTER_MAX])
            if op == P.OP_CONTINUE:
                if a.state not in (P.S_IDLE, P.S_DONE):
                    return Outcome(False, "BUSY")
                self.backend.send_text(aid, self.config["continue_text"])
                return Outcome(True, f"SENT {label}"[: P.FOOTER_MAX])
            if op == P.OP_FOCUS:
                return Outcome(True, "FOCUSED") if self.backend.focus(aid) else Outcome(False, "NO FOCUS HERE")
            keys = {P.OP_ENTER: ["enter"], P.OP_UP: ["up"], P.OP_DOWN: ["down"], P.OP_MODE: ["shift+tab"]}.get(op)
            if keys:
                self.backend.keys(aid, keys)
                return Outcome(True, keys[0].upper())
        except BackendError as e:
            return Outcome(False, str(e)[: P.FOOTER_MAX])
        return Outcome(False, f"UNKNOWN OP {op}")
