"""What the host does with the device's frames: answer HELLO and each HEARTBEAT (the device
counts the host as gone 10 s after its last frame), hand the characters it read to a typer,
and send text to play."""

from __future__ import annotations

from . import FIRMWARE_KNOWN
from . import protocol as P


class Host:
    def __init__(self, link, typer=None, lower: bool = True, log=None):
        self.link = link
        self.typer = typer
        self.lower = lower
        self.log = log or (lambda s: None)
        self.firmware: tuple[int, int] | None = None
        self.connected = False
        self.received = ""

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
            self.link.send(P.hello_ack())
            self.connected = True
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
