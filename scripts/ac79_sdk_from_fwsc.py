#!/usr/bin/env python3
"""The three AC79 SDK files a USB-only fm1-base build needs, taken out of an fm1-base package (.fwsc).

fm1-base's tools/fm1pkg_make.py puts the SDK's uboot.boot, cfg_tool.bin and cfg/eq_cfg_hw.bin into every package.
Where gitee.com (the SDK's home) cannot be reached, as from some cloud machines, this reverses that packing and keeps
the files only when all three SHA-256 equal tools/build.py's SDK_SHA256: then they are the SDK's own bytes, whoever
built the package. A Bluetooth or WiFi build still needs the whole SDK.

  scripts/ac79_sdk_from_fwsc.py PACKAGE.fwsc [DEST]     (DEST: tools/fw-AC79_AIoT_SDK, what just fw-toolchain uses)
"""
import hashlib
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "third_party" / "fm1-base" / "tools"


def enc(buf, off, size, key=0xFFFF):
    """JieLi ENC stream cipher, in place (its own inverse)"""
    for i in range(size):
        buf[off + i] ^= key & 0xFF
        key = ((key << 1) ^ (0x1021 if key & 0x8000 else 0)) & 0xFFFF


def sfc(buf, size, key):
    for i in range(0, size, 32):
        enc(buf, i, min(size - i, 32), key ^ (i >> 2))


def chipkey(d):
    s = sum(d[:16]) & 0xFF
    s = 0xAA if s >= 0xE0 else 0x55 if s <= 0x10 else s
    return sum(1 << i for i in range(16) if (d[16 + i] ^ d[15 - i]) < s)


def entry(b):
    """a JLFS entry: (offset, size, name)"""
    _, off, size, _, _, _, name = struct.unpack("<HIIBBH16s", bytes(b[2:32]))
    return off, size, name.split(b"\0")[0].decode(errors="replace")


def pinned():
    text = (BASE / "build.py").read_text()
    block = text[text.index("SDK_SHA256 = {"):]
    block = block[:block.index("}")]
    return dict(re.findall(r'"([^"]+)":\s*"([0-9a-f]{64})"', block))


def extract(pkg):
    raw = pkg.read_bytes()
    logical = bytearray()
    for i in range(20):                       # drop the identity's marker bytes
        logical += raw[i * 0x30:i * 0x30 + 0x2F]
    logical += raw[20 * 0x30:]
    hdr = bytearray(logical[:0x40])
    enc(hdr, 0, 0x40)
    nfiles = struct.unpack_from("<H", hdr, 8)[0]
    flash = None
    for k in range(nfiles):
        e = bytearray(logical[0x40 + k * 0x50:0x40 + (k + 1) * 0x50])
        enc(e, 0, 0x50)
        off, ln = struct.unpack_from("<II", e, 8)
        if e[0x40:0x50].split(b"\0")[0] == b"flash.bin":
            flash = bytes(logical[off:off + ln])
    if flash is None:
        raise SystemExit("no flash.bin in the package")
    head = bytearray(flash[:0xA0])
    for o in range(0, 0xA0, 32):
        enc(head, o, 32)
    files, key = {}, None
    for k in range(1, 5):
        off, size, name = entry(head[k * 32:(k + 1) * 32])
        if name == "uboot.boot":
            spl = flash[off:off + size]
            dec = bytearray(spl)              # newer packers store it ENC'd (two streams), older ones plain
            enc(dec, 0, 16)
            enc(dec, 16, size - 16)
            files["uboot.boot"] = (bytes(spl), bytes(dec))
        elif name == "isd_config.ini":
            key = chipkey(flash[off:off + 32])
    region = bytearray(flash[0x4000:])
    sfc(region, len(region), key)
    for k in range(1, 3):
        off, size, name = entry(region[k * 32:(k + 1) * 32])
        if name == "cfg_tool.bin":
            files["cfg_tool.bin"] = (bytes(region[off:off + size]),)
            cfg = off + size
    off, size, name = entry(region[cfg + 32:cfg + 64])
    files["cfg/eq_cfg_hw.bin"] = (bytes(region[cfg + off:cfg + off + size]),)
    return files


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    dest = Path(sys.argv[2] if len(sys.argv) > 2 else ROOT / "tools" / "fw-AC79_AIoT_SDK") / "cpu" / "wl82" / "tools"
    want = pinned()
    got = {}
    for rel, cands in extract(Path(sys.argv[1])).items():
        for c in cands:
            if hashlib.sha256(c).hexdigest() == want.get(rel):
                got[rel] = c
    missing = sorted(set(want) - set(got))
    if missing:
        raise SystemExit(f"not the pinned SDK bytes: {', '.join(missing)} (nothing written)")
    for rel, data in got.items():
        p = dest / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(data)
        print(f"ok   {rel}  {want[rel]}")
    print(f"into {dest}")


if __name__ == "__main__":
    main()
