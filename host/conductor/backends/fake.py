"""An in-memory backend for tests and for trying the device without real agents."""

from __future__ import annotations

import threading

from .. import protocol as P
from ..model import Agent
from .base import BackendError


class FakeBackend:
    name = "fake"

    def __init__(self, agents: list[Agent] | None = None):
        self.agents: dict[str, Agent] = {a.id: a for a in (agents or [])}
        self.sent: list[tuple[str, str, object]] = []  # (agent id, what, detail)
        self._changed = threading.Event()

    @classmethod
    def demo(cls) -> FakeBackend:
        return cls(
            [
                Agent("f1", "fix-tests", P.K_CLAUDE, P.S_BLOCKED),
                Agent("f2", "refactor-api", P.K_CODEX, P.S_WORKING),
                Agent("f3", "docs-pass", P.K_CLAUDE, P.S_DONE),
                Agent("f4", "spike-ui", P.K_OPENCODE, P.S_IDLE),
                Agent("f5", "remote-train", P.K_CODEX, P.S_ERROR, remote=True),
            ]
        )

    # test helpers
    def set_state(self, agent_id: str, state: int, question: bool = False) -> None:
        a = self.agents[agent_id]
        a.state, a.question = state, question
        self._changed.set()

    def add(self, agent: Agent) -> None:
        self.agents[agent.id] = agent
        self._changed.set()

    def remove(self, agent_id: str) -> None:
        self.agents.pop(agent_id, None)
        self._changed.set()

    # the contract
    def list_agents(self) -> list[Agent]:
        return [Agent(**vars(a)) for a in self.agents.values()]

    def _get(self, agent_id: str) -> Agent:
        if agent_id not in self.agents:
            raise BackendError(f"no agent {agent_id}")
        return self.agents[agent_id]

    def read_lines(self, agent_id: str, n: int) -> list[str]:
        a = self._get(agent_id)
        lines = [f"{a.name} ({a.cwd or '~'})", f"state {a.state}"]
        if a.state == P.S_BLOCKED:
            lines += ["Do you want to run npm test?", "> 1. Yes", "  2. Yes, don't ask again", "  3. No"]
        return lines[-n:]

    def keys(self, agent_id: str, keys: list[str]) -> None:
        self._get(agent_id)
        self.sent.append((agent_id, "keys", list(keys)))

    def send_text(self, agent_id: str, text: str) -> None:
        a = self._get(agent_id)
        if a.state == P.S_BLOCKED:
            raise BackendError("blocked: answer it first")
        self.sent.append((agent_id, "text", text))

    def interrupt(self, agent_id: str) -> str:
        self._get(agent_id)
        self.sent.append((agent_id, "interrupt", None))
        return "stopped"

    def focus(self, agent_id: str) -> bool:
        self._get(agent_id)
        self.sent.append((agent_id, "focus", None))
        return True

    def wait_change(self, timeout: float) -> bool:
        hit = self._changed.wait(timeout)
        self._changed.clear()
        return hit

    def close(self) -> None:
        pass
