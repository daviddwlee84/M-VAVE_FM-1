"""A pretend coding agent for integration tests: reports its status with OSC 7501 (as
Claude Code and Codex do), asks for a permission, and logs every key it gets.

    hc run --bg --name cond-fake-1 --label conductor-test=1 -- python3 host/tests/fake_agent.py LOG

Cycle: working 3 s -> blocked (permission) until a key -> working 2 s -> done.
Enter / 1 / y approves, Esc denies (-> idle). It starts the cycle again on "continue".
"""

from __future__ import annotations

import os
import select
import sys
import termios
import time
import tty


def status(state: str, **kv) -> None:
    extra = "".join(f":{k}={v}" for k, v in kv.items())
    sys.stdout.write(f"\x1b]7501;state={state}:app=fake{extra}\x1b\\")
    sys.stdout.flush()


def say(text: str) -> None:
    sys.stdout.write(text + "\r\n")
    sys.stdout.flush()


def main() -> None:
    log = open(sys.argv[1] if len(sys.argv) > 1 else os.devnull, "a", buffering=1)
    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    tty.setraw(fd)
    try:
        while True:
            status("working")
            say("working on it...")
            time.sleep(3)
            status("blocked", kind="permission")
            say("Do you want to run `make test`?")
            say("> 1. Yes   3. No (esc)")
            key = os.read(fd, 16)
            log.write(f"blocked-key {key!r}\n")
            if key in (b"\r", b"\n", b"1", b"y"):
                say("approved")
                status("working")
                time.sleep(2)
                status("done")
                say("all done")
            else:
                say("denied")
                status("idle")
            buf = b""
            while b"\r" not in buf and b"\n" not in buf:  # a prompt, ended by Enter
                if select.select([fd], [], [], 0.5)[0]:
                    ch = os.read(fd, 64)
                    if ch == b"\x03":
                        return
                    buf += ch
            log.write(f"prompt {buf!r}\n")
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old)


if __name__ == "__main__":
    main()
