"""herdr-core (`hc`): JSON-RPC 2.0 over `hc connect` (the API on stdin/stdout).

Status comes from the programs themselves (OSC 7501: idle / working / done / blocked /
error), which Claude Code and Codex send natively; hc never guesses from the screen.
A terminal that never reported a status is a shell unless its foreground program is a
known agent.
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import threading
import time

from .. import protocol as P
from ..model import Agent, display_name, kind_of, kind_of_argv, state_of
from .base import BackendError, tail


class _Rpc:
    """One `hc connect` process: requests in, replies (and notifications) out."""

    def __init__(self, hc: str, on_note=None):
        self.p = subprocess.Popen(
            [hc, "connect"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
        )
        self.lock = threading.Lock()
        self.next_id = 0
        self.on_note = on_note

    def call(self, method: str, params: dict | None = None):
        with self.lock:
            self.next_id += 1
            rid = self.next_id
            assert self.p.stdin and self.p.stdout
            self.p.stdin.write(json.dumps({"jsonrpc": "2.0", "id": rid, "method": method, "params": params or {}}) + "\n")
            self.p.stdin.flush()
            while True:
                line = self.p.stdout.readline()
                if not line:
                    raise BackendError("hc connect closed")
                msg = json.loads(line)
                if msg.get("id") == rid:
                    break
                if "method" in msg and self.on_note:
                    self.on_note(msg)
        if "error" in msg:
            err = msg["error"]
            kind = (err.get("data") or {}).get("kind") or err.get("message")
            raise BackendError(f"{method}: {kind}")
        return msg.get("result")

    def notes(self):
        assert self.p.stdout
        for line in self.p.stdout:
            try:
                yield json.loads(line)
            except json.JSONDecodeError:
                continue

    def close(self):
        try:
            self.p.kill()
        except OSError:
            pass


class HcBackend:
    name = "hc"

    def __init__(self, hc: str | None = None, labels: dict | None = None, show_shells: bool = False):
        self.hc = hc or shutil.which("hc") or os.path.expanduser("~/.local/bin/hc")
        if not os.path.exists(self.hc):
            raise BackendError("hc not found")
        self.labels = labels or {}
        self.show_shells = show_shells
        self.hidden_shells = 0
        self.rpc = _Rpc(self.hc)
        self.rpc.call("hello", {"generation": 1})
        self._changed = threading.Event()
        self._events_ok = False
        self._ev_thread = threading.Thread(target=self._follow, daemon=True)
        self._ev_thread.start()

    # events: any change only marks us dirty; list_agents is the truth
    def _follow(self):
        while True:
            ev = None
            try:
                ev = _Rpc(self.hc)
                ev.call("hello", {"generation": 1})
                ev.call("events.subscribe", {"labels": self.labels} if self.labels else {})
                self._events_ok = True
                self._changed.set()
                for msg in ev.notes():
                    if msg.get("method") in ("event", "missed"):
                        self._changed.set()
            except (BackendError, OSError, ValueError):
                pass
            finally:
                self._events_ok = False
                if ev:
                    ev.close()
            time.sleep(2.0)  # the core restarted or went away: subscribe again

    def wait_change(self, timeout: float) -> bool:
        hit = self._changed.wait(timeout)
        self._changed.clear()
        return hit

    @staticmethod
    def _agent(t: dict, show_shells: bool) -> Agent | None:
        if t.get("state") != "live":
            return None
        st = t.get("status") or None
        fg = (t.get("foreground") or {}).get("name") or ""
        app = (st or {}).get("app") or ""
        argv = (t.get("foreground") or {}).get("argv") or t.get("argv")
        kind = kind_of(app) or kind_of(fg) or kind_of_argv(argv)
        if st is None and kind in (P.K_OTHER, P.K_SHELL) and not show_shells:
            return None
        if st is None and kind == P.K_OTHER:
            kind = P.K_SHELL
        labels = t.get("labels") or {}
        cwd = (t.get("foreground") or {}).get("cwd") or t.get("cwd") or ""
        fallback = os.path.basename(cwd.rstrip("/")) or fg or t["address"]
        name = t.get("name") or display_name((st or {}).get("title") or t.get("title") or labels.get("where"), fallback)
        state = state_of(st.get("state")) if st else (P.S_IDLE if kind == P.K_SHELL else P.S_UNKNOWN)
        return Agent(
            id=t["address"],
            name=name,
            kind=kind,
            state=state,
            question=bool(st and st.get("kind") == "question"),
            remote="/" in t["address"],
            cwd=(t.get("foreground") or {}).get("cwd") or t.get("cwd") or "",
        )

    def list_agents(self) -> list[Agent]:
        params = {"labels": self.labels} if self.labels else {}
        res = self.rpc.call("terminal.list", params)
        out = []
        live = 0
        for t in res.get("terminals", []):
            live += t.get("state") == "live"
            a = self._agent(t, self.show_shells)
            if a:
                out.append(a)
        self.hidden_shells = live - len(out)  # shown at start-up, so an empty list is not a mystery
        return out

    def read_lines(self, agent_id: str, n: int) -> list[str]:
        res = self.rpc.call("terminal.read", {"terminal": agent_id, "lines": n})  # the screen plus n of history
        return tail(res.get("text", ""), n)

    def keys(self, agent_id: str, keys: list[str]) -> None:
        self.rpc.call("terminal.keys", {"terminal": agent_id, "keys": keys})

    def send_text(self, agent_id: str, text: str) -> None:
        self.rpc.call("terminal.send", {"terminal": agent_id, "text": text})  # refuses a blocked one (no force)

    def interrupt(self, agent_id: str) -> str:
        res = self.rpc.call("terminal.interrupt", {"terminal": agent_id})
        return res.get("outcome", "?")

    def focus(self, agent_id: str) -> bool:
        return False  # the core has no panes to focus; the Herdr app does

    def close(self) -> None:
        self.rpc.close()
