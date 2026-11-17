"""Conductor: the host daemon for the FM-1 coding-agent controller (firmware/conductor).

Versions (docs/agentic/conductor-changelog.md): the firmware is X.Y (FW_MAJOR.FW_MINOR in
firmware/conductor/app.c, reported in its HELLO); this daemon is X.Y.Z, X.Y the newest
firmware it was made with, Z its host-only updates. The protocol byte (protocol.VERSION)
changes only for an incompatible change.
"""

__version__ = "0.3.2"
FIRMWARE_KNOWN = (0, 3)  # the newest firmware this daemon knows
