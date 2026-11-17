"""Which agent sits on which key. Remembered across restarts, so an agent comes back to
the key it had."""

from __future__ import annotations

import json
import os
from pathlib import Path

from .protocol import NSLOTS


class SlotMap:
    def __init__(self, path: str | os.PathLike | None = None):
        self.path = Path(path) if path else None
        self.remembered: dict[str, int] = {}
        self.current: dict[str, int] = {}
        if self.path and self.path.exists():
            try:
                data = json.loads(self.path.read_text())
                self.remembered = {k: int(v) for k, v in data.items() if 0 <= int(v) < NSLOTS}
            except (ValueError, OSError):
                self.remembered = {}

    def assign(self, ids: list[str]) -> int:
        """Place the present agents on slots; returns how many did not fit."""
        present = set(ids)
        cur = {i: s for i, s in self.current.items() if i in present}
        used = set(cur.values())
        hidden = 0
        for i in ids:
            if i in cur:
                continue
            want = self.remembered.get(i)
            if want is not None and want not in used:
                cur[i] = want
            else:
                free = next((s for s in range(NSLOTS) if s not in used), None)
                if free is None:
                    hidden += 1
                    continue
                cur[i] = free
            used.add(cur[i])
        changed = cur != self.current
        self.current = cur
        if changed:
            self.remembered.update(cur)
            self._save()
        return hidden

    def agent_at(self, slot: int) -> str | None:
        for i, s in self.current.items():
            if s == slot:
                return i
        return None

    def slot_of(self, agent_id: str) -> int | None:
        return self.current.get(agent_id)

    def _save(self):
        if not self.path:
            return
        try:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            self.path.write_text(json.dumps(self.remembered, indent=1, sort_keys=True))
        except OSError:
            pass
