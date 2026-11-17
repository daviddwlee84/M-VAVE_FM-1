"""The contract every backend keeps. The daemon only talks to backends through it."""

from __future__ import annotations

from typing import Protocol

from ..model import Agent


class BackendError(RuntimeError):
    pass


class Backend(Protocol):
    name: str  # shown on the device (8 characters at most)

    def list_agents(self) -> list[Agent]:
        """Every agent now, shells left out unless the backend was asked for them."""
        ...

    def read_lines(self, agent_id: str, n: int) -> list[str]:
        """The last n non-empty lines of the agent's screen."""
        ...

    def keys(self, agent_id: str, keys: list[str]) -> None:
        """Press named keys: enter, esc, up, down, shift+tab, ctrl+c, 1, y, ..."""
        ...

    def send_text(self, agent_id: str, text: str) -> None:
        """Type text and press Enter (a prompt). Refuses a blocked agent."""
        ...

    def interrupt(self, agent_id: str) -> str:
        """Stop the current turn; returns a short word for what happened."""
        ...

    def focus(self, agent_id: str) -> bool:
        """Show the agent's pane on the computer. False if the backend cannot."""
        ...

    def wait_change(self, timeout: float) -> bool:
        """Block up to timeout seconds; True as soon as something may have changed."""
        ...

    def close(self) -> None: ...


def tail(text: str, n: int) -> list[str]:
    lines = [ln.rstrip() for ln in text.splitlines()]
    lines = [ln for ln in lines if ln.strip()]
    return lines[-n:]
