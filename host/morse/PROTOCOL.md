# Morse wire protocol

Between `firmware/morse` on the FM-1 and the host (`host/morse`). Two layers, both
over USB-MIDI (fm1-emu's virtual cable works the same) and, from firmware 0.5, Bluetooth MIDI
(fm1-base's own stack; SysEx in BLE-MIDI packets, keying with real BLE-MIDI timestamps both ways):

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
| `44` | PROG | firmware 0.4+: a chunk of the progress, the answer to PROG_REQ (see Progress) |
| `45` | PROG_DONE | firmware 0.4+: the answer to a PROG_LOAD: `00` loaded, `01` declined on the device, `02` refused (a chunk missing or out of turn, CRC, contents) |
| `43` | CHAR | ASCII: what the device read from the key (KEY page), or what was typed (TYPE page: a letter, or from firmware 0.2 a phrase such as `<SK> `, several characters in one frame; in COMPOSE mode the whole line when it is sent). `20` (space) ends a word; `*` is a pattern that is no character |

### Host → device

| Type | Name | Payload |
|---|---|---|
| `01` | HELLO_ACK | `ver`. The answer to HELLO **and to every HEARTBEAT**: the device counts the host as gone 10 s after its last frame |
| `02` | TEXT | ASCII, at most 60 characters: played as received, at the device's hearing speed. Characters without a code are skipped. From firmware 0.2, characters between `<` and `>` are run together as one prosign (`<SK>` is `...-.-`) |
| `03` | STOP | stop playing the received text |
| `04` | PROG_REQ | firmware 0.4+: send the progress. Nothing changes on the device |
| `05` | PROG_LOAD | firmware 0.4+: a chunk of progress to load. Once all have come and check out, the device shows what it got and asks its owner (`PLAY` yes, `HOME` no), then answers PROG_DONE |

CHAR is sent only while the host counts as present (it answered within 10 s).

## Progress

Firmware 0.4 and later; `host/morse/progress.py`, and the same code in fm1-morse `web/index.html` (MORSE on the web, which also shows CHAR and sends TEXT / STOP).

The record: a format byte, then `learn.c`'s `ln_blob`, 259 bytes little-endian with no padding (round u16,
streak u16, open u8, answered u32, right u32, then for each of the 41 characters in Koch order its recognise and
send boxes, u8 each, then for each its two last-answered rounds, u16 each, `FFFF` never), then in format 2 the
settings, then a CRC-16/CCITT-FALSE (poly `1021`, init `FFFF`) of every byte before it, low byte first.

| Format | Firmware | Bytes |
|---|---|---|
| `01` | 0.4-0.6 send and take it; 0.7 takes it (the progress alone, its settings left as they are) | 1 + 259 + 2 = 262 |
| `02` | 0.7 sends it and takes it | 1 + 259 + 12 + 2 = 274 |

The settings in format 2, 12 bytes (`app.c` `set_blob`, its `set_t`): hearing WPM, spacing (Farnsworth) WPM, keyer
(0 straight, 1 iambic A, 2 iambic B), paddles swapped, flags (bit 0 sound off, bit 1 Bluetooth off, bits 2-4 the
start page: 0 MENU, 1 PRACTICE, 2 LISTEN, 3 KEY, 4 RHYTHM, 5 TYPE, 6 CHART; bits 5-7 zero), cards a round, learn by
(0 sound, 1 sight), look (bits 0-1 KEY's view, 2 TYPE shows the code, 3-4 TYPE's layout, 5 LISTEN hides the text,
6 TYPE composes, 7 strict judging), pitch u16 Hz, sending WPM, LISTEN's program. The device refuses settings its
`set_valid` would not keep. A host sends format 2 only to firmware 0.7 and later (from the HELLO), and format 1 to
older firmware or for a file without settings.

On the wire it is cut into chunks of 49 bytes: 6 frames in either format, each `<index> <count = 06>` and the chunk 7-bit packed (for
each 7 bytes, a byte of their high bits, bit j for byte j, then the 7 bytes' low 7 bits). The device sends one frame
per main-loop pass; the host sends them about 30 ms apart. A load must come in order from index 0; a gap starts it
over and is answered `02`.

The file (`morse backup`): JSON, `"format": "fm1-morse-progress"`, `"version": 2` (host 0.7; 1 before, still
read), the round, streak, open, answered and right, for each character `recognise`, `send` (0..5),
`last_recognise`, `last_send` (a round or `null`), and from firmware 0.7 `"settings"` with named values
(`hear_wpm`, `spacing_wpm`, `send_wpm`, `pitch_hz`, `keyer`, `paddles`, `sound`, `bluetooth`, `start_page`, `cards`,
`learn_by`, `judging`, `key_view`, `type_layout`, `type_shows_code`, `type_composes`, `listen_hides_text`,
`listen_program`; `progress.py` `Settings.to_dict`). Missing settings take their defaults; an unknown one is
refused. Both the host and the device refuse a record that does not make sense (open outside 5..41, a box over 5, an
answer after the current round, more right than answered).

## Shared vectors

The same bytes are in `host/morse/protocol.py` (`VECTORS`) and in
`firmware/morse/tests/basic.txt`; `host/tests/test_morse.py` checks they match.

| Name | Bytes |
|---|---|
| hello (firmware 0.7, USB) | `F0 7D 4D 4F 01 41 01 00 07 00 F7` |
| hello_ack | `F0 7D 4D 4F 01 01 01 F7` |
| char_k | `F0 7D 4D 4F 01 43 4B F7` |
| text_cq | `F0 7D 4D 4F 01 02 43 51 F7` |
| stop | `F0 7D 4D 4F 01 03 F7` |
| prog_req | `F0 7D 4D 4F 01 04 F7` |
| prog_fresh_0, prog_fresh_5, prog_load_0..5, prog_v2_0..5 | in `progress.py` `VECTORS`: a fresh record's first and last chunk as firmware 0.7 sends them (format 2), the sample backup loaded in format 1, and with settings in format 2 |

## Versions

The protocol byte changes only for an incompatible change. A new frame type is gated on
the firmware version the HELLO reports.

Firmware 0.5 adds no frame: the same frames go over Bluetooth, and replies go back by the
transport the host's HELLO_ACK came on. The transport byte of HELLO / HEARTBEAT says which one
(the host logs it, prefers USB when it hears both, and the web page shows it).

Firmware 0.7 adds no frame either: PROG and PROG_LOAD carry format 2 (the settings after the progress), and still
take format 1 in.
