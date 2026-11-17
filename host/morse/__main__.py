"""python -m morse: the computer's side of firmware/morse.

  type      what you key on the FM-1 (KEY page) or type on it (TYPE page) is typed into the
            focused app on the computer (macOS; Accessibility permission), or printed (--dry-run)
  send TEXT the FM-1 plays TEXT as received (amber on its KEY page); "-" reads lines from stdin
  listen    print what the FM-1 reads, nothing typed

--simulate puts a text stand-in in place of the FM-1 (lines on stdin are what it "read")."""

from __future__ import annotations

import argparse
import sys
import threading
import time

from . import __version__
from .core import Host
from .device import MidiLink, SimLink
from .typer import MacTyper, PrintTyper, accessibility_trusted


def log(s: str) -> None:
    print(f"[morse] {s}", file=sys.stderr, flush=True)


def open_link(args):
    if args.simulate:
        return SimLink(out=sys.stderr)
    link = MidiLink(args.port)
    log(f"connected to {link.name}")
    return link


def wait_connected(host: Host, seconds: float = 5.0) -> bool:
    end = time.time() + seconds
    while time.time() < end and not host.connected:
        host.pump(0.1)
    return host.connected


def cmd_type(args) -> int:
    if args.dry_run or args.listen:
        typer = PrintTyper()
    else:
        if accessibility_trusted() is False:
            log("this terminal has no Accessibility permission: macOS will drop the keys "
                "(System Settings > Privacy & Security > Accessibility), or use --dry-run")
        typer = MacTyper()
    link = open_link(args)
    host = Host(link, typer, lower=not args.upper, log=log)
    log("listening: key on the FM-1's KEY page or type on its TYPE page (Ctrl-C to stop)")
    try:
        while True:
            host.pump(0.1)
    except KeyboardInterrupt:
        pass
    finally:
        link.close()
    return 0


def cmd_send(args) -> int:
    link = open_link(args)
    host = Host(link, None, log=log)
    if not wait_connected(host):
        log("the FM-1 has not said HELLO: is it running Morse?")
        link.close()
        return 1
    text = " ".join(args.text)
    if text != "-":
        n = host.send_text(text)
        log(f"sent {n} frame(s); the FM-1 plays it at its own speed")
        host.pump(0.3)
        link.close()
        return 0
    stop = threading.Event()

    def keep_alive():
        while not stop.is_set():
            host.pump(0.1)

    t = threading.Thread(target=keep_alive, daemon=True)
    t.start()
    log("type lines to send (Ctrl-D to end)")
    try:
        for line in sys.stdin:
            if line.strip():
                host.send_text(line)
    except KeyboardInterrupt:
        pass
    stop.set()
    t.join(1)
    link.close()
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog="morse", description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", help="MIDI port name (part of it); default: one named MORSE, or one saying HELLO")
    ap.add_argument("--simulate", action="store_true", help="a text stand-in for the FM-1")
    ap.add_argument("--version", action="version", version=f"morse host {__version__}")
    sub = ap.add_subparsers(dest="cmd", required=True)
    t = sub.add_parser("type", help="type what the FM-1 reads into the computer")
    t.add_argument("--dry-run", action="store_true", help="print instead of typing")
    t.add_argument("--upper", action="store_true", help="type capitals (default: lower case)")
    t.set_defaults(fn=cmd_type, listen=False)
    li = sub.add_parser("listen", help="print what the FM-1 reads")
    li.add_argument("--upper", action="store_true")
    li.set_defaults(fn=cmd_type, listen=True, dry_run=True)
    s = sub.add_parser("send", help="the FM-1 plays TEXT")
    s.add_argument("text", nargs="+", help='the text, or "-" for lines from stdin')
    s.set_defaults(fn=cmd_send)
    args = ap.parse_args(argv)
    try:
        return args.fn(args)
    except RuntimeError as e:
        log(str(e))
        return 1


if __name__ == "__main__":
    sys.exit(main())
