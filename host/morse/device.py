"""The two ends the host can talk to: a real FM-1 running firmware/morse over MIDI (USB, or
fm1-emu's virtual cable), or a text stand-in on the terminal."""

from __future__ import annotations

import queue
import sys
import threading
import time

from . import protocol as P

PORT_HINTS = ("MORSE",)


def sniff_port(mido, names: list[str], seconds: float = 2.5) -> str | None:
    """The input that speaks our protocol: the device says HELLO or HEARTBEAT every second,
    whatever macOS calls the port (it keeps a firmware's old name, e.g. "Felucca", after a
    switch)."""
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
    """SysEx in and out through mido; the keying notes are counted too (key-downs seen)."""

    def __init__(self, port: str | None = None):
        import mido  # only needed for a real device

        ins, outs = mido.get_input_names(), mido.get_output_names()
        name_in, name_out = find_port(ins, port), find_port(outs, port)
        if not name_in and not port:
            name_in = sniff_port(mido, ins)
            name_out = name_in if name_in in outs else None
        if not name_in or not name_out:
            raise RuntimeError(f"no Morse FM-1 on MIDI: no port named MORSE and none saying HELLO (inputs: {ins})")
        self.name = name_in
        self.q: queue.Queue[bytes] = queue.Queue()
        self.key_downs = 0
        self.out = mido.open_output(name_out)
        self.inp = mido.open_input(name_in, callback=self._on_msg)
        self._mido = mido

    def _on_msg(self, msg):
        if msg.type == "sysex":
            self.q.put(bytes([0xF0, *msg.data, 0xF7]))
        elif msg.type == "note_on" and msg.note == P.KEY_NOTE and msg.velocity:
            self.key_downs += 1

    def send(self, frame: bytes) -> None:
        self.out.send(self._mido.Message("sysex", data=list(frame[1:-1])))

    def poll(self, timeout: float = 0.1) -> list[bytes]:
        out = []
        try:
            out.append(self.q.get(timeout=timeout))
            while True:
                out.append(self.q.get_nowait())
        except queue.Empty:
            pass
        return out

    def close(self) -> None:
        self.inp.close()
        self.out.close()


class SimLink:
    """A text stand-in for the FM-1: it says HELLO and a HEARTBEAT every second, each line
    typed on the terminal is what the FM-1 read from the key (a CHAR per character, then a
    space), and what the host sends is printed. Lines can also come from a list (tests)."""

    def __init__(self, lines: list[str] | None = None, out=None, beat: float = 1.0):
        self.q: queue.Queue[bytes] = queue.Queue()
        self.sent: list[bytes] = []
        self.out = out if out is not None else sys.stdout
        self.name = "simulated FM-1"
        self.beat = beat
        self.acked = False
        self._last = 0.0
        from . import progress as G

        self.progress = G.Progress()      # what it has learnt
        self.owner_says = P.PD_LOADED     # the owner's answer when a backup comes in (PLAY / HOME)
        self._asm = G.Assembler()
        self.q.put(P.device_hello(*_firmware()))
        if lines is None:
            threading.Thread(target=self._read_stdin, daemon=True).start()
        else:
            for line in lines:
                self._feed(line)

    def _feed(self, line: str) -> None:
        for ch in P.clean_text(line):
            self.q.put(P.device_char(ch))
        self.q.put(P.device_char(" "))

    def _read_stdin(self) -> None:
        for line in sys.stdin:
            self._feed(line)

    def send(self, frame: bytes) -> None:
        self.sent.append(frame)
        m = P.parse(frame)
        if m and m.type == P.T_HELLO_ACK:
            self.acked = True
        elif m and m.type == P.T_TEXT:
            print(f"[FM-1 plays] {m.payload.decode('ascii')}", file=self.out, flush=True)
        elif m and m.type == P.T_STOP:
            print("[FM-1 stops]", file=self.out, flush=True)
        elif m and m.type == P.T_PROG_REQ:
            from . import progress as G

            for f in G.frames(self.progress, P.T_PROG):
                self.q.put(f)
        elif m and m.type == P.T_PROG_LOAD:
            from . import progress as G

            try:
                p = self._asm.feed(m.payload)
            except G.ProgressError:
                self.q.put(P.frame(P.T_PROG_DONE, bytes([P.PD_BAD])))
                return
            if p is not None:
                if self.owner_says == P.PD_LOADED:
                    self.progress = p
                self.q.put(P.frame(P.T_PROG_DONE, bytes([self.owner_says])))

    def poll(self, timeout: float = 0.1) -> list[bytes]:
        now = time.time()
        if now - self._last >= self.beat:
            self._last = now
            self.q.put(P.device_heartbeat() if self.acked else P.device_hello(*_firmware()))
        out = []
        try:
            out.append(self.q.get(timeout=timeout))
            while True:
                out.append(self.q.get_nowait())
        except queue.Empty:
            pass
        return out

    def close(self) -> None:
        pass


def _firmware() -> tuple[int, int]:
    from . import FIRMWARE_KNOWN

    return FIRMWARE_KNOWN
