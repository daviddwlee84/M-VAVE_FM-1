# Morse wire protocol

Between `firmware/morse` on the FM-1 and the host (`host/morse`). Two layers, both
over USB-MIDI (fm1-emu's virtual cable works the same):

1. **Keying** — MIDI notes, the [Vail adapter's format](https://github.com/Vail-CW/vail-adapter/blob/master/docs/MIDI_INTEGRATION_SPEC.md),
   so Vail and any MIDI tool understand it without our host.
2. **Text** — our own SysEx frames, for the host program only.

## Keying (notes)

| Direction | Message | Meaning |
|---|---|---|
| device → host | `90 00 7F` | key down (channel 1, note 0) |
| device → host | `80 00 00` | key up |
| host → device | `9n kk vv` (channel 1, any note, `vv` > 0) | received key down |
| host → device | `8n kk vv`, or `9n kk 00` | received key up |

- The device sends its own keying on the **KEY** page (after its keyer: paddles come out
  already timed, as note 0) and what it plays on the **TYPE** page. Not on the practice
  pages.
- Up is always `80 00 00`, never `90 00 00`: Vail's web app reads the command nibble only
  (`static/scripts/inputs.mjs` in Vail-CW/vail_web_repeater), so a note-on of velocity 0
  would be a key-down there.
- Received keying is played on the second voice (a major third up) and decoded into amber
  text. Vail's CC and program change messages (`B0 00 xx`, `B0 01 xx`, `C0 xx`, sent to
  every MIDI output when its page opens) are ignored.

## Text (SysEx)

Every frame is `F0 7D 4D 4F <ver> <type> <payload…> F7`. `7D` is the MIDI non-commercial
ID; `4D 4F` ("MO") keeps it apart from fm1-base's `7D 4C …` and Conductor's `7D 43 4E`.
`ver` is 1. Payload bytes are 7-bit; text is ASCII.

### Device → host

| Type | Name | Payload |
|---|---|---|
| `41` | HELLO | `ver`, firmware major, minor, transport (0 USB, 1 BLE). Once a second until a HELLO_ACK |
| `42` | HEARTBEAT | `ver`, transport. Once a second after a HELLO_ACK |
| `43` | CHAR | ASCII: what the device read from the key (KEY page), or a letter typed (TYPE page). `20` (space) ends a word; `*` is a pattern that is no character |

### Host → device

| Type | Name | Payload |
|---|---|---|
| `01` | HELLO_ACK | `ver`. The answer to HELLO **and to every HEARTBEAT**: the device counts the host as gone 10 s after its last frame |
| `02` | TEXT | ASCII, at most 60 characters: played as received, at the device's speed. Characters without a code are skipped |
| `03` | STOP | stop playing the received text |

CHAR is sent only while the host counts as present (it answered within 10 s).

## Shared vectors

The same bytes are in `host/morse/protocol.py` (`VECTORS`) and in
`firmware/morse/tests/basic.txt`; `host/tests/test_morse.py` checks they match.

| Name | Bytes |
|---|---|
| hello (firmware 0.1, USB) | `F0 7D 4D 4F 01 41 01 00 01 00 F7` |
| hello_ack | `F0 7D 4D 4F 01 01 01 F7` |
| char_k | `F0 7D 4D 4F 01 43 4B F7` |
| text_cq | `F0 7D 4D 4F 01 02 43 51 F7` |
| stop | `F0 7D 4D 4F 01 03 F7` |

## Versions

The protocol byte changes only for an incompatible change. A new frame type is gated on
the firmware version the HELLO reports.
