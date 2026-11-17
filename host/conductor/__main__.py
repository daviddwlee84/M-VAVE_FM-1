"""python -m conductor [--backend auto|hc|herdr|fake] [--simulate] [--port NAME]"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
from pathlib import Path

from .backends.base import BackendError
from .core import DEFAULT_CONFIG, Conductor
from .slots import SlotMap

CONFIG_DIR = Path(os.environ.get("CONDUCTOR_HOME", Path.home() / ".config" / "conductor"))


def make_backend(args):
    if args.backend == "fake":
        from .backends.fake import FakeBackend

        return FakeBackend.demo()

    def hc():
        from .backends.hc import HcBackend

        labels = dict(kv.split("=", 1) for kv in args.label)
        return HcBackend(labels=labels, show_shells=args.show_shells)

    def herdr():
        from .backends.herdr import HerdrBackend

        return HerdrBackend(path=args.herdr_socket, session=args.herdr_session)

    if args.backend == "hc":
        return hc()
    if args.backend == "herdr":
        return herdr()
    # auto: every runtime that is running; agents may be in either
    from .backends.multi import MultiBackend

    found = []
    for make in (hc, herdr):
        try:
            found.append(make())
        except BackendError as e:
            print(f"[backend] skipped: {e}", file=sys.stderr)
    return found[0] if len(found) == 1 else MultiBackend(found)


def report(backend) -> None:
    """What each backend sees, so an empty device is explained here."""
    parts = getattr(backend, "backends", {backend.name: backend})
    for name, b in parts.items():
        try:
            n = len(b.list_agents())
        except BackendError as e:
            print(f"[backend] {name}: {e}")
            continue
        extra = ""
        if getattr(b, "hidden_shells", 0):
            extra = f", {b.hidden_shells} shells not shown (--show-shells to list them)"
        print(f"[backend] {name}: {n} agents{extra}")


def load_config() -> dict:
    cfg = json.loads(json.dumps(DEFAULT_CONFIG))
    f = CONFIG_DIR / "config.json"
    if f.exists():
        cfg.update(json.loads(f.read_text()))
    return cfg


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="conductor", description=__doc__)
    ap.add_argument("--backend", choices=["auto", "hc", "herdr", "fake"], default="auto",
                    help="auto (default): hc and classic Herdr together, whichever are running")
    ap.add_argument("--simulate", action="store_true", help="no FM-1: a text stand-in on this terminal")
    ap.add_argument("--port", help="MIDI port name (part of it); default: one named CONDUCTOR")
    ap.add_argument("--label", action="append", default=[], help="hc: only terminals with this label k=v")
    ap.add_argument("--show-shells", action="store_true", help="hc: shells take keys too")
    ap.add_argument("--herdr-socket")
    ap.add_argument("--herdr-session")
    ap.add_argument("--poll", type=float, default=2.0, help="seconds between full refreshes")
    ap.add_argument("--no-ble-connect", action="store_true",
                    help="no MIDI port: exit, instead of opening the Bluetooth MIDI window and waiting")
    ap.add_argument("--no-voice", action="store_true", help="ARP does not press the dictation hotkey")
    ap.add_argument("--slots", help="slot memory file (default ~/.config/conductor/slots-BACKEND.json)")
    args = ap.parse_args(argv)

    try:
        backend = make_backend(args)
    except BackendError as e:
        print(f"backend: {e}", file=sys.stderr)
        return 2

    if args.simulate:
        from .device import SimLink

        link = SimLink()
    else:
        from .device import MidiLink

        try:
            link = MidiLink(args.port)
        except RuntimeError as e:
            link = None
            if sys.platform == "darwin" and not args.no_ble_connect and not args.port:
                # macOS does not reconnect a Bluetooth MIDI device after it restarts: open its window and wait
                from .blelink import open_panel, wait_for_port

                panel = open_panel()
                print("[ble] no MIDI port. In the Bluetooth MIDI window, click Connect next to FM-1 XXXX "
                      "(or plug in USB). Waiting up to 5 minutes ...")
                port = wait_for_port()
                if panel and panel.poll() is None:
                    panel.terminate()
                if port:
                    print(f"[ble] port appeared: {port}; starting again")
                    # a fresh process: this one's CoreMIDI client may not see the new port
                    os.execv(sys.executable, [sys.executable, "-m", "conductor", *(argv or sys.argv[1:]), "--no-ble-connect"])
            if link is None:
                print(e, file=sys.stderr)
                return 2
        print(f"[midi] {link.name}")

    slots = SlotMap(args.slots or CONFIG_DIR / f"slots-{backend.name}.json")
    cfg = load_config()
    c = Conductor(backend=backend, send=link.send, slots=slots, config=cfg)
    report(backend)
    try:
        from .render import LineRenderer

        c.renderer = LineRenderer(cfg.get("detail_font"), cfg.get("detail_font_size", 14))
    except (ImportError, OSError) as e:
        print(f"[detail] text only (ASCII): {e}", file=sys.stderr)
    if cfg.get("voice_keys") and not args.no_voice:
        from .voice import VoiceKey, accessibility_trusted

        c.voice = VoiceKey(cfg["voice_keys"], cfg.get("voice_mode", "hold"), cfg.get("voice_max_hold", 120.0))
        keys = "+".join(cfg["voice_keys"])
        print(f"[voice] ARP holds {keys} ({c.voice.mode})")
        if accessibility_trusted() is False:
            print("[voice] this terminal has no Accessibility permission: the keys will not arrive. "
                  "System Settings > Privacy & Security > Accessibility", file=sys.stderr)
    last = 0.0
    try:
        while True:
            for f in link.poll():
                c.on_frame(f)
            if c.voice:
                c.voice.tick()
            changed = backend.wait_change(0.1)
            if changed or time.time() - last >= args.poll:
                last = time.time()
                try:
                    c.refresh()
                except BackendError as e:
                    print(f"[backend] {e}", file=sys.stderr)
                    time.sleep(1.0)
    except KeyboardInterrupt:
        pass
    finally:
        if c.voice:
            c.voice.let_go()
        backend.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
