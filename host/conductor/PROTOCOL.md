# Conductor protocol, version 1

SysEx over USB-MIDI between `firmware/conductor` (the FM-1) and `host/conductor`
(the daemon). The same frames are meant to go over Bluetooth MIDI later; nothing
in them depends on the transport.

## Framing

```text
F0 7D 43 4E <ver> <type> <payload...> F7
```

- `7D`: the MIDI non-commercial manufacturer ID. `43 4E` ("CN") keeps the frames
  apart from fm1-base's own `7D 4C ..` frames (log, crash history, stats).
- `<ver>` is `01`. A frame with another version is rejected, not guessed at.
- Every payload byte is 7-bit. Numbers above 127 are two bytes, low 7 bits first
  (`u14`). Text is printable ASCII; anything else becomes `?`.
- Frames stay short (at most 36 bytes), so they fit BLE MIDI packets later.

## Host → device

| Type | Name | Payload |
|---|---|---|
| `01` | HELLO_ACK | `ver` |
| `02` | CLEAR | — (every slot empty; sent before a full table) |
| `03` | SLOT | `slot` (0–26), `state`, `kind`, `flags`, `tag` u14, `age_min` u14, `name` (≤ 20) |
| `04` | DETAIL | `slot`, `line` (0–7), `text` (≤ 28) |
| `05` | META | `total` u14, `hidden` u14 (agents not on a key), `backend` (≤ 8) |
| `06` | FOOTER | `text` (≤ 28): the result of the last action, shown for 3 s |
| `07` | BEEP | `figure` (0 up, 1 done, 2 error, 3 tick) |

States: `0` EMPTY, `1` IDLE, `2` WORKING, `3` BLOCKED, `4` DONE, `5` ERROR, `6` UNKNOWN.
Kinds: `0` other, `1` claude, `2` codex, `3` opencode, `4` gemini, `5` cursor, `6` pi, `7` shell.
Flags: `01` blocked on a question (not a permission), `02` runs on another machine.

The tag is 14 bits of a CRC of the backend's agent ID. The device echoes it in
every ACTION, so the host can tell whether the agent on that key is still the one
the device was showing.

## Device → host

| Type | Name | Payload |
|---|---|---|
| `41` | HELLO | `ver`, `fw_major`, `fw_minor` — once a second until HELLO_ACK |
| `42` | HEARTBEAT | `ver` — once a second after HELLO_ACK |
| `43` | ACTION | `op`, `slot`, `tag` u14 |
| `44` | SELECT | `slot` — the host answers with DETAIL lines for it |

ACTION ops: `1` approve, `2` approve always, `3` deny, `4` interrupt, `5` enter,
`6` continue, `7` focus, `8` up, `9` down, `10` mode (shift+tab),
`11` voice down, `12` voice up.

Voice is not about an agent: the host holds a dictation hotkey on the computer
(default Ctrl+Option, for Wispr Flow or Handy) between `11` and `12`. The device
sends `12` on every release; the host also lets go after 120 s, when the device
says HELLO again, and when it exits. The slot is only used to focus that agent's
pane first.

## Liveness

- The device counts the host as present while any frame arrived in the last
  10 s. The host sends META at least every 3 s as a keepalive.
- On HELLO the host sends HELLO_ACK, CLEAR, every SLOT and META.
- After a new USB session (cable out and in) the device says HELLO again.

## Who decides

The device never decides anything about an agent. It refuses obviously useless
presses locally (approve on a slot that is not BLOCKED) to save a round trip, and
otherwise sends intents. The host re-reads the backend before it presses a key:
the tag must still match the agent on that slot, and the agent's state must fit
the action (approve / deny only when blocked, continue only when idle or done).
There is no auto-approve.

## Shared test vectors

`protocol.py` (`VECTORS`) and `firmware/conductor/tests/basic.txt` use the same
bytes:

| Name | Frame |
|---|---|
| hello_ack | `F0 7D 43 4E 01 01 01 F7` |
| slot_blocked (slot 0, claude, tag `0x123`, 5 min, "fix-tests") | `F0 7D 43 4E 01 03 00 03 01 00 23 02 05 00 66 69 78 2D 74 65 73 74 73 F7` |
| device_approve (slot 0, tag `0x123`) | `F0 7D 43 4E 01 43 01 00 23 02 F7` |
