"""Several backends at once: hc and classic Herdr are separate runtimes, and the agents
may live in either. IDs get the backend's name in front ("hc:t7", "herdr:wV:p1")."""

from __future__ import annotations

import time

from ..model import Agent
from .base import BackendError


class MultiBackend:
    def __init__(self, backends: list):
        if not backends:
            raise BackendError("no backend available")
        self.backends = {b.name: b for b in backends}
        self.name = "+".join(self.backends)[:8]

    def _split(self, agent_id: str):
        name, _, inner = agent_id.partition(":")
        if name not in self.backends:
            raise BackendError(f"no backend for {agent_id}")
        return self.backends[name], inner

    def list_agents(self) -> list[Agent]:
        out = []
        for name, b in self.backends.items():
            try:
                agents = b.list_agents()
            except BackendError:
                continue  # one runtime down does not hide the other's agents
            for a in agents:
                a.id = f"{name}:{a.id}"
                out.append(a)
        return out

    def read_lines(self, agent_id, n):
        b, i = self._split(agent_id)
        return b.read_lines(i, n)

    def keys(self, agent_id, keys):
        b, i = self._split(agent_id)
        b.keys(i, keys)

    def send_text(self, agent_id, text):
        b, i = self._split(agent_id)
        b.send_text(i, text)

    def interrupt(self, agent_id):
        b, i = self._split(agent_id)
        return b.interrupt(i)

    def focus(self, agent_id):
        b, i = self._split(agent_id)
        return b.focus(i)

    def wait_change(self, timeout: float) -> bool:
        end = time.time() + timeout
        step = max(timeout / (2 * len(self.backends)), 0.02)
        while True:
            for b in self.backends.values():
                if b.wait_change(step):
                    return True
            if time.time() >= end:
                return False

    def close(self) -> None:
        for b in self.backends.values():
            b.close()
