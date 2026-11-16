#!/usr/bin/env python3
"""Build an emulator flash image that rehearses installing a firmware over a real backup.

For SLOOP and Agent Deck, M-Flasher wrote only the firmware area (0x04000-0x92FFF) on
my unit and left everything else as it was. This does the same on a copy: take a
full-flash backup, replace the firmware area with the package's, and write the result
as an fm1-live --flash file. Booting it in the emulator shows how the new firmware
reacts to the data the old one left behind, without touching the unit.

Limits: only the firmware area is taken from the package. A package that carries
more than that (ChoralRoot shows FONTS MISSING this way) cannot be rehearsed
faithfully, and the emulator is not the hardware.

usage: emu_rehearse.py PRISTINE.flash BACKUP.bin OUT.flash
  PRISTINE.flash  what fm1-live writes for the package on an empty flash
"""
import sys

FW_START, FW_END = 0x04000, 0x93000


def main():
    pristine = open(sys.argv[1], "rb").read()
    backup = open(sys.argv[2], "rb").read()
    if len(pristine) != len(backup) or len(backup) != 1 << 20:
        sys.exit("both images must be 1 MiB")
    image = backup[:FW_START] + pristine[FW_START:FW_END] + backup[FW_END:]
    open(sys.argv[3], "wb").write(image)
    print(f"firmware area 0x{FW_START:05X}-0x{FW_END - 1:05X} from the package, the rest from the backup")


if __name__ == "__main__":
    main()
