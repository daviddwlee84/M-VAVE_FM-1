"""What the daemon knows about an agent, whichever backend it came from."""

from __future__ import annotations

import time
import zlib
from dataclasses import dataclass, field

from . import protocol as P

STATE_BY_WORD = {
    "idle": P.S_IDLE,
    "working": P.S_WORKING,
    "blocked": P.S_BLOCKED,
    "done": P.S_DONE,
    "error": P.S_ERROR,
    "unknown": P.S_UNKNOWN,
}
STATE_NAMES = {v: k.upper() for k, v in STATE_BY_WORD.items()} | {P.S_EMPTY: "EMPTY"}

KIND_BY_WORD = {
    "claude": P.K_CLAUDE,
    "codex": P.K_CODEX,
    "opencode": P.K_OPENCODE,
    "gemini": P.K_GEMINI,
    "cursor": P.K_CURSOR,
    "cursor-agent": P.K_CURSOR,
    "pi": P.K_PI,
    "shell": P.K_SHELL,
    "zsh": P.K_SHELL,
    "bash": P.K_SHELL,
    "fish": P.K_SHELL,
    "sh": P.K_SHELL,
}
KIND_SHORT = {
    P.K_OTHER: "??",
    P.K_CLAUDE: "CL",
    P.K_CODEX: "CX",
    P.K_OPENCODE: "OC",
    P.K_GEMINI: "GM",
    P.K_CURSOR: "CU",
    P.K_PI: "PI",
    P.K_SHELL: "SH",
}


def state_of(word: str | None) -> int:
    return STATE_BY_WORD.get((word or "").lower(), P.S_UNKNOWN)


def kind_of(word: str | None) -> int:
    """'claude', 'claude-code', 'Claude Code', '/opt/homebrew/bin/codex' all count."""
    w = (word or "").strip().lower().rsplit("/", 1)[-1]
    if w in KIND_BY_WORD:
        return KIND_BY_WORD[w]
    head = w.replace("_", "-").replace(" ", "-").split("-", 1)[0]
    return KIND_BY_WORD.get(head, P.K_OTHER) if head != "sh" else P.K_OTHER


def kind_of_argv(argv: list[str] | None) -> int:
    """A wrapper in front (specstory run claude, npx codex): the first known agent in argv."""
    for a in argv or []:
        k = kind_of(a)
        if k not in (P.K_OTHER, P.K_SHELL):
            return k
    return P.K_OTHER


def display_name(title: str | None, fallback: str) -> str:
    """A name the device can show (printable ASCII): leading status marks ("●", "✳")
    dropped; a title that is mostly not ASCII (CJK, emoji) gives way to the fallback,
    usually the working directory's name."""
    t = (title or "").strip()
    if t and not t[0].isascii() and not t[0].isalnum():
        t = t[1:].lstrip()  # one leading status mark
    if not t:
        return fallback
    ascii_n = sum(1 for ch in t if ch.isascii() and ch.isprintable())
    return t if ascii_n >= 0.8 * len(t) else fallback


def tag_of(agent_id: str) -> int:
    """A 14-bit tag the device echoes back, so the host knows an ACTION is for the
    agent the device was showing (and not one that took the slot since)."""
    return zlib.crc32(agent_id.encode()) & 0x3FFF or 1


@dataclass
class Agent:
    id: str  # the backend's address for it (hc `t7` / `machine/t7`, Herdr pane id)
    name: str
    kind: int = P.K_OTHER
    state: int = P.S_UNKNOWN
    question: bool = False  # blocked on a question rather than a permission
    remote: bool = False
    cwd: str = ""
    since: float = field(default_factory=time.time)  # entered this state

    @property
    def tag(self) -> int:
        return tag_of(self.id)

    @property
    def flags(self) -> int:
        return (P.F_QUESTION if self.question else 0) | (P.F_REMOTE if self.remote else 0)

    def age_min(self, now: float | None = None) -> int:
        return int(((now or time.time()) - self.since) // 60)
