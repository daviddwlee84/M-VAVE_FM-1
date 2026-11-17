"""The two ends a daemon can talk to: a real FM-1 over MIDI (USB or Bluetooth, the same
CoreMIDI port either way), or a text stand-in on the terminal."""

from __future__ import annotations

import queue
import sys
import threading
import time

from . import protocol as P
from .model import KIND_SHORT, STATE_NAMES

PORT_HINTS = ("CONDUCTOR",)


def sniff_port(mido, names: list[str], seconds: float = 2.5) -> str | None:
    """The input that speaks our protocol: the device says HELLO or HEARTBEAT every
    second, whatever macOS calls the port (it keeps a firmware's old name, e.g.
    "Felucca", after a switch)."""
    found: list[str] = []
    ports = []
    for n in names:
        def hear(msg, n=n):
            if msg.type == "sysex" and bytes(msg.data[:3]) == P.HEADER and n not in found:
                found.append(n)
        try:
            ports.append(mido.open_input(n, callback=hear))
        except (OSError, IOError):
            continue
    end = time.time() + seconds
    while time.time() < end and not found:
        time.sleep(0.05)
    for p in ports:
        p.close()
    return found[0] if found else None


def find_port(names: list[str], want: str | None) -> str | None:
    if want:
        return next((n for n in names if want.lower() in n.lower()), None)
    return next((n for n in names if any(h.lower() in n.lower() for h in PORT_HINTS)), None)


class MidiLink:
    """SysEx in and out through mido. Over USB or BLE MIDI the port looks the same; only
    its name differs (macOS shows a Bluetooth device once it is connected in Audio MIDI
    Setup > MIDI Studio > Bluetooth)."""

    def __init__(self, port: str | None = None):
        import mido  # only needed for a real device

        ins, outs = mido.get_input_names(), mido.get_output_names()
        name_in, name_out = find_port(ins, port), find_port(outs, port)
        if not name_in and not port:
            name_in = sniff_port(mido, ins)  # not by name: by what it says
            name_out = name_in if name_in in outs else None
        if not name_in or not name_out:
            raise RuntimeError(f"no Conductor on MIDI: no port named CONDUCTOR and none saying HELLO (inputs: {ins})")
        self.name = name_in
        self.q: queue.Queue[bytes] = queue.Queue()
        self.out = mido.open_output(name_out)
        self.inp = mido.open_input(name_in, callback=self._on_msg)
        self._mido = mido

    def _on_msg(self, msg):
        if msg.type == "sysex":
            self.q.put(bytes([0xF0, *msg.data, 0xF7]))

    def send(self, frame: bytes) -> None:
        self.out.send(self._mido.Message("sysex", data=list(frame[1:-1])))

    def poll(self) -> list[bytes]:
        out = []
        while not self.q.empty():
            out.append(self.q.get_nowait())
        return out


class SimLink:
    """The device's half in text: keeps the slot table the frames describe, prints it,
    and turns typed commands into the frames the firmware would send."""

    HELP = "commands: sel N | approve | always | deny | int | enter | cont | focus | voice | unvoice | show | quit"

    def __init__(self, out=sys.stdout, interactive: bool = True):
        self.out = out
        self.slots: dict[int, dict] = {}
        self.meta = ""
        self.footer = ""
        self.detail: dict[int, str] = {}
        self.selected = 0
        self.q: queue.Queue[bytes] = queue.Queue()
        self.dirty = False
        self.q.put(P.device_hello())
        if interactive:
            print(self.HELP, file=self.out)
            threading.Thread(target=self._stdin, daemon=True).start()

    def send(self, frame: bytes) -> None:  # host -> "device"
        m = P.parse(frame)
        if not m:
            return
        p = m.payload
        if m.type == P.T_CLEAR:
            self.slots.clear()
        elif m.type == P.T_SLOT:
            s = p[0]
            if p[1] == P.S_EMPTY:
                self.slots.pop(s, None)
            else:
                self.slots[s] = {
                    "state": p[1],
                    "kind": p[2],
                    "flags": p[3],
                    "tag": p[4] | p[5] << 7,
                    "age": p[6] | p[7] << 7,
                    "name": p[8:].decode("ascii", "replace"),
                }
        elif m.type == P.T_META:
            meta = f"{p[0] | p[1] << 7} agents, {p[2] | p[3] << 7} hidden, {p[4:].decode()}"
            if meta == self.meta:
                return  # the periodic keepalive: nothing new to print
            self.meta = meta
        elif m.type == P.T_FOOTER:
            self.footer = p.decode("ascii", "replace")
            print(f"  >> {self.footer}", file=self.out)
            return
        elif m.type == P.T_DETAIL:
            self.detail[p[1]] = p[2:].decode("ascii", "replace")
            return  # shown by "show"
        self.dirty = True

    def render(self) -> None:
        if not self.dirty:
            return
        self.dirty = False
        print(f"--- {self.meta}", file=self.out)
        for s in sorted(self.slots):
            v = self.slots[s]
            mark = ">" if s == self.selected else " "
            print(
                f"{mark}{s:2d} {STATE_NAMES.get(v['state'], '?'):8s} {KIND_SHORT.get(v['kind'], '??')} "
                f"{v['name']:20s} {v['age']}m",
                file=self.out,
            )

    def press(self, cmd: str) -> None:
        parts = cmd.split()
        if not parts:
            return
        c = parts[0]
        ops = {
            "approve": P.OP_APPROVE,
            "always": P.OP_APPROVE_ALWAYS,
            "deny": P.OP_DENY,
            "int": P.OP_INTERRUPT,
            "enter": P.OP_ENTER,
            "cont": P.OP_CONTINUE,
            "focus": P.OP_FOCUS,
            "voice": P.OP_VOICE_DOWN,
            "unvoice": P.OP_VOICE_UP,
        }
        if c == "sel" and len(parts) > 1:
            self.selected = int(parts[1])
            self.q.put(P.device_select(self.selected))
            self.dirty = True
        elif c in ops:
            tag = self.slots.get(self.selected, {}).get("tag", 0)
            self.q.put(P.device_action(ops[c], self.selected, tag))
        elif c == "show":
            self.dirty = True
            for i in sorted(self.detail):
                print(f"   | {self.detail[i]}", file=self.out)
        elif c == "quit":
            raise SystemExit(0)
        else:
            print(self.HELP, file=self.out)

    def _stdin(self):
        for line in sys.stdin:
            try:
                self.press(line.strip())
            except SystemExit:
                import os

                os._exit(0)

    def poll(self) -> list[bytes]:
        self.render()
        out = []
        while not self.q.empty():
            out.append(self.q.get_nowait())
        return out
