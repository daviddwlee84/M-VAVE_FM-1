"""Classic Herdr: newline-delimited JSON on the server's unix socket.

`agent.list` returns agents only (no shells), with Herdr's own status
(idle / working / blocked / done / unknown), detected or reported by integrations.
"""

from __future__ import annotations

import json
import os
import socket
import threading
import time

from ..model import Agent, display_name, kind_of, state_of
from .base import BackendError, tail

EVENTS = ["pane.agent_status_changed", "pane.agent_detected", "pane.created", "pane.closed", "pane.exited"]


def socket_path(explicit: str | None = None, session: str | None = None) -> str:
    if explicit:
        return explicit
    if session:
        return os.path.expanduser(f"~/.config/herdr/sessions/{session}/herdr.sock")
    return os.environ.get("HERDR_SOCKET_PATH") or os.path.expanduser("~/.config/herdr/herdr.sock")


class HerdrBackend:
    name = "herdr"

    def __init__(self, path: str | None = None, session: str | None = None):
        self.path = socket_path(path, session)
        if not os.path.exists(self.path):
            raise BackendError(f"no Herdr socket at {self.path}")
        self._n = 0
        self._lock = threading.Lock()
        self._changed = threading.Event()
        threading.Thread(target=self._follow, daemon=True).start()

    def _connect(self) -> socket.socket:
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.connect(self.path)
        return s

    def call(self, method: str, params: dict | None = None):
        with self._lock:
            self._n += 1
            rid = f"cond-{self._n}"
        s = self._connect()
        try:
            s.settimeout(5.0)
            s.sendall((json.dumps({"id": rid, "method": method, "params": params or {}}) + "\n").encode())
            buf = b""
            while not buf.endswith(b"\n"):
                chunk = s.recv(65536)
                if not chunk:
                    break
                buf += chunk
        finally:
            s.close()
        msg = json.loads(buf or b"{}")
        if "error" in msg:
            err = msg["error"]
            raise BackendError(f"{method}: {err.get('message', err) if isinstance(err, dict) else err}")
        return msg.get("result", {})

    def _follow(self):
        while True:
            try:
                s = self._connect()
                s.sendall(
                    (
                        json.dumps(
                            {
                                "id": "cond-events",
                                "method": "events.subscribe",
                                "params": {"subscriptions": [{"type": t} for t in EVENTS]},
                            }
                        )
                        + "\n"
                    ).encode()
                )
                self._changed.set()
                f = s.makefile("rb")
                for _ in f:
                    self._changed.set()
            except OSError:
                pass
            time.sleep(2.0)

    def wait_change(self, timeout: float) -> bool:
        hit = self._changed.wait(timeout)
        self._changed.clear()
        return hit

    def list_agents(self) -> list[Agent]:
        res = self.call("agent.list")
        out = []
        for a in res.get("agents", []):
            title = a.get("terminal_title_stripped") or ""
            cwd = a.get("foreground_cwd") or a.get("cwd") or ""
            name = display_name(title, os.path.basename(cwd.rstrip("/")) or a["pane_id"])
            out.append(
                Agent(
                    id=a["pane_id"],
                    name=name,
                    kind=kind_of(a.get("agent")),
                    state=state_of(a.get("agent_status")),
                    cwd=cwd,
                )
            )
        return out

    def read_lines(self, agent_id: str, n: int) -> list[str]:
        res = self.call("pane.read", {"pane_id": agent_id, "source": "recent", "lines": n, "strip_ansi": True})
        return tail((res.get("read") or {}).get("text") or "", n)

    def keys(self, agent_id: str, keys: list[str]) -> None:
        self.call("pane.send_keys", {"pane_id": agent_id, "keys": keys})

    def send_text(self, agent_id: str, text: str) -> None:
        self.call("agent.prompt", {"target": agent_id, "text": text})  # Herdr refuses a blocked agent

    def interrupt(self, agent_id: str) -> str:
        self.keys(agent_id, ["esc"])
        return "esc"

    def focus(self, agent_id: str) -> bool:
        self.call("pane.focus", {"pane_id": agent_id})
        return True

    def close(self) -> None:
        pass
