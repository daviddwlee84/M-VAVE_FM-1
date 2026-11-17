"""End-to-end against a running hc core, touching only its own terminals.

    uv run --group host python host/tests/integration_hc.py

Starts two fake agents labelled conductor-test=1, watches them through HcBackend and the
text stand-in, approves one and denies the other from the "device", checks what keys
each fake agent got, then kills both. Nothing else in hc is read for keys or written to.
"""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from conductor import protocol as P  # noqa: E402
from conductor.backends.hc import HcBackend  # noqa: E402
from conductor.core import Conductor  # noqa: E402
from conductor.device import SimLink  # noqa: E402
from conductor.slots import SlotMap  # noqa: E402

LABEL = {"conductor-test": "1"}
FAKE = Path(__file__).with_name("fake_agent.py")


def hc(*args) -> str:
    return subprocess.run(["hc", *args], check=True, capture_output=True, text=True).stdout


def main() -> int:
    tmp = Path(tempfile.mkdtemp(prefix="conductor-"))
    names = ["cond-fake-1", "cond-fake-2"]
    for n in names:
        hc("run", "--bg", "--no-parent", "--rm", "--name", n, "--label", "conductor-test=1",
           "--", sys.executable, str(FAKE), str(tmp / f"{n}.log"))
    ok = True
    try:
        be = HcBackend(labels=LABEL)
        sim = SimLink(interactive=False, out=open(tmp / "sim.txt", "w"))
        c = Conductor(backend=be, send=sim.send, slots=SlotMap(tmp / "slots.json"), log=lambda *_: None)

        def pump(seconds: float):
            end = time.time() + seconds
            while time.time() < end:
                for f in sim.poll():
                    c.on_frame(f)
                if be.wait_change(0.1):
                    c.refresh()

        deadline = time.time() + 5
        while time.time() < deadline and len(sim.slots) < 2:  # until both have reported a status
            pump(0.3)
        print("slots:", {s: (v["name"], v["state"]) for s, v in sim.slots.items()})
        assert len(sim.slots) == 2, "both fake agents on keys, shells left out"
        # wait for both to block (they work 3 s first)
        deadline = time.time() + 10
        while time.time() < deadline and not all(v["state"] == P.S_BLOCKED for v in sim.slots.values()):
            pump(0.3)
        assert all(v["state"] == P.S_BLOCKED for v in sim.slots.values()), sim.slots
        print("both BLOCKED")
        s1 = c.slots.slot_of(next(a.id for a in c.agents.values() if a.name == "cond-fake-1"))
        s2 = c.slots.slot_of(next(a.id for a in c.agents.values() if a.name == "cond-fake-2"))
        for s, cmd in ((s1, "approve"), (s2, "deny")):
            sim.press(f"sel {s}")
            sim.press(cmd)
            outs = [c.on_frame(f) for f in sim.poll()]
            print(cmd, "->", [o.text for o in outs if o])
        pump(6.0)
        states = {v["name"]: v["state"] for v in sim.slots.values()}
        print("after:", states)
        log1 = (tmp / "cond-fake-1.log").read_text()
        log2 = (tmp / "cond-fake-2.log").read_text()
        print("fake-1 got:", log1.strip())
        print("fake-2 got:", log2.strip())
        ok &= states.get("cond-fake-1") == P.S_DONE and "blocked-key b'\\r'" in log1
        ok &= states.get("cond-fake-2") == P.S_IDLE and "blocked-key b'\\x1b'" in log2
        # continue on the done one
        sim.press(f"sel {s1}")
        sim.press("cont")
        outs = [c.on_frame(f) for f in sim.poll()]
        print("cont ->", [o.text for o in outs if o])
        time.sleep(1.0)
        ok &= "prompt b'continue" in (tmp / "cond-fake-1.log").read_text()
        be.close()
    finally:
        for t in json.loads(hc("ls", "--json", "--label", "conductor-test=1")).get("terminals", []):
            subprocess.run(["hc", "kill", t["address"]], capture_output=True)
    print("PASS" if ok else "FAIL", tmp)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
