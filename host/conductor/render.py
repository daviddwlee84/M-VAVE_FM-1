"""Lines of any script drawn on the host, sent to the device as 1-bit pictures.

The FM-1's fonts are ASCII only, and a CJK font would not fit in the firmware, so the
host draws each DETAIL line (Chinese, Japanese, symbols) with a system font into a
240 x 16 bitmap and the device only paints it. One line packs into 549 bytes of 7-bit
SysEx: within fm1-base's 640-byte frame.
"""

from __future__ import annotations

import os

W, H = 240, 16
BITS = W * H
PACKED = (BITS + 6) // 7  # 549

FONT_CANDIDATES = [
    "/System/Library/Fonts/STHeiti Medium.ttc",  # Traditional and Simplified Chinese, Latin
    "/System/Library/Fonts/Hiragino Sans GB.ttc",
    "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
]


def pack7(bits: list[int]) -> bytes:
    """A bit stream cut into 7-bit bytes, first bit highest. Pixel i is bit
    (6 - i % 7) of byte i // 7: the device reads it in place, no unpacking."""
    out = bytearray(PACKED)
    for i, b in enumerate(bits):
        if b:
            out[i // 7] |= 1 << (6 - i % 7)
    return bytes(out)


class LineRenderer:
    def __init__(self, font_path: str | None = None, size: int = 14):
        from PIL import ImageFont  # Pillow is in the host dependency group

        path = font_path or next((f for f in FONT_CANDIDATES if os.path.exists(f)), None)
        if not path:
            raise OSError("no font with CJK glyphs found; set detail_font in config.json")
        self.font = ImageFont.truetype(path, size)
        self.path = path

    def width(self, text: str) -> float:
        return self.font.getlength(text)

    def wrap(self, text: str, width: int = W - 4) -> list[str]:
        """Break a line where it would run past the screen, by drawn width (a CJK
        character is about twice a Latin one)."""
        out, cur = [], ""
        for ch in text:
            if self.width(cur + ch) > width and cur:
                out.append(cur.rstrip())
                cur = "  " + ch if ch != " " else "  "
            else:
                cur += ch
        out.append(cur.rstrip())
        return out

    def bitmap(self, text: str) -> bytes:
        from PIL import Image, ImageDraw

        img = Image.new("L", (W, H), 0)
        ImageDraw.Draw(img).text((2, H // 2), text, font=self.font, fill=255, anchor="lm")
        px = img.load()
        return pack7([1 if px[x, y] >= 110 else 0 for y in range(H) for x in range(W)])


def unpack7(data: bytes) -> list[int]:
    return [(data[i // 7] >> (6 - i % 7)) & 1 for i in range(BITS)]
