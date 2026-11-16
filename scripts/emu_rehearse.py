#!/usr/bin/env python3
"""Build an emulator flash image that rehearses installing a firmware over a real backup.

M-Flasher writes a package's image and leaves the rest of the flash as it was (for
SLOOP, Agent Deck and CTL-1 on my unit: 0x04000-0x92FFF). This does the same on a
copy: take a full-flash backup, lay the package's image over it, and write the result
as an fm1-live --flash file. Booting it in the emulator shows how the new firmware
reacts to the data the old one left behind, without touching the unit.

Limits: the package image is taken as the contiguous run from 0x04000 in what the
emulator lays down; whether M-Flasher writes exactly that range for a given
package is only known from its own plan. The emulator is not the hardware.

usage: emu_rehearse.py PRISTINE.flash BACKUP.bin OUT.flash
  PRISTINE.flash  what fm1-live writes for the package on an empty flash
"""
import sys

FW_START = 0x04000
SECTOR = 4096
BLANK = b"\xff" * SECTOR


def main():
    pristine = open(sys.argv[1], "rb").read()
    backup = open(sys.argv[2], "rb").read()
    if len(pristine) != len(backup) or len(backup) != 1 << 20:
        sys.exit("both images must be 1 MiB")
    # the package's image: the contiguous run from the start of the firmware area
    # (0x92FFF for most firmwares, further for larger ones such as ChoralRoot)
    end = FW_START
    while end < len(pristine) and pristine[end:end + SECTOR] != BLANK:
        end += SECTOR
    image = backup[:FW_START] + pristine[FW_START:end] + backup[end:]
    open(sys.argv[3], "wb").write(image)
    lost = [a for a in range(0x93000, end, SECTOR) if backup[a:a + SECTOR] != BLANK]
    print(f"package image 0x{FW_START:05X}-0x{end - 1:05X} laid over the backup")
    if lost:
        print("it covers data the installed firmware had stored at:", ", ".join(f"0x{a:05X}" for a in lost))


if __name__ == "__main__":
    main()
