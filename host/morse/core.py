"""What the host does with the device's frames: answer HELLO and each HEARTBEAT (the device
counts the host as gone 10 s after its last frame), hand the characters it read to a typer,
and send text to play."""

from __future__ import annotations

import time

from . import FIRMWARE_KNOWN
from . import progress as G
from . import protocol as P


class Host:
    def __init__(self, link, typer=None, lower: bool = True, log=None):
        self.link = link
        self.typer = typer
        self.lower = lower
        self.log = log or (lambda s: None)
        self.firmware: tuple[int, int] | None = None
        self.connected = False
        self.transport: int | None = None  # P.TR_USB / P.TR_BLE, from HELLO and HEARTBEAT
        self.received = ""
        self._asm: G.Assembler | None = None
        self.progress: G.Progress | None = None
        self.prog_error: str | None = None
        self.prog_done: int | None = None

    def on_frame(self, data: bytes) -> None:
        try:
            m = P.parse(data)
        except P.ProtocolError as e:
            self.log(f"ignored: {e}")
            return
        if m is None:
            return
        if m.type in (P.T_HELLO, P.T_HEARTBEAT):
            if m.type == P.T_HELLO:
                fw = P.firmware_of(m)
                if fw and fw != self.firmware:
                    self.firmware = fw
                    newer = fw > FIRMWARE_KNOWN
                    self.log(f"FM-1 Morse firmware {fw[0]}.{fw[1]}" + (" (newer than this host knows)" if newer else ""))
            tr = P.transport_of(m)
            if tr != self.transport:
                self.transport = tr
                self.log("via " + ("Bluetooth" if tr == P.TR_BLE else "USB"))
            self.link.send(P.hello_ack())
            self.connected = True
        elif m.type == P.T_PROG and self._asm is not None:
            try:
                p = self._asm.feed(m.payload)
            except G.ProgressError as e:
                self.prog_error = str(e)
                self._asm = None
                return
            if p is not None:
                self.progress = p
                self._asm = None
        elif m.type == P.T_PROG_DONE and m.payload:
            self.prog_done = m.payload[0]
        elif m.type == P.T_CHAR:
            s = P.chars_of(m).replace(P.UNKNOWN, "")
            if not s:
                return
            self.received += s
            if self.typer:
                self.typer.type(s.lower() if self.lower else s)

    def send_text(self, text: str) -> int:
        frames = P.text(text)
        for f in frames:
            self.link.send(f)
        return len(frames)

    def stop(self) -> None:
        self.link.send(P.stop())

    def pump(self, timeout: float = 0.1) -> None:
        for f in self.link.poll(timeout):
            self.on_frame(f)

    def can_backup(self) -> bool:
        return self.firmware is not None and self.firmware >= P.PROG_FIRMWARE

    def backup(self, timeout: float = 5.0) -> G.Progress:
        """The FM-1's progress (nothing changes on it)."""
        self.progress, self.prog_error, self._asm = None, None, G.Assembler()
        self.link.send(P.prog_req())
        end = time.time() + timeout
        while time.time() < end and self.progress is None and self.prog_error is None:
            self.pump(0.1)
        if self.prog_error:
            raise G.ProgressError(self.prog_error)
        if self.progress is None:
            raise TimeoutError("the FM-1 sent no progress")
        return self.progress

    def restore(self, p: G.Progress, timeout: float = 120.0, gap: float = 0.03) -> int:
        """Send p; the FM-1 asks its owner (PLAY yes, HOME no). Returns P.PD_* (TimeoutError if no answer)."""
        self.prog_done = None
        for f in G.frames(p):
            self.link.send(f)
            self.pump(gap)
        end = time.time() + timeout
        while time.time() < end and self.prog_done is None:
            self.pump(0.1)
        if self.prog_done is None:
            raise TimeoutError("no answer from the FM-1 (nobody pressed PLAY or HOME?)")
        return self.prog_done
