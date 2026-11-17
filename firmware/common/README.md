# firmware/common

Small modules shared by this repo's fm1-base apps. MIT. Each is a `.h` with the
API and usage, and a `.c` that the app includes into fm1-base's unity build.

| Module | What it does |
|---|---|
| `hostlink` | An app's own SysEx conversation with a program on the computer, over USB or Bluetooth: framing under `F0 7D id0 id1 ver`, HELLO on every transport until the host answers, HEARTBEAT after on the transport it answered by, host presence with a timeout, a new HELLO after a new USB session or a lost Bluetooth link. Bluetooth needs `ble_stack = "own"` (JieLi's stack drops incoming SysEx); `hl_sx_packet` puts the SysEx packets back together. |
| `ledfx` | Per-LED patterns on the single-colour LEDs: off, glow, mid, lit, pulse, blink, double blink, fast. Turns them into `plat_lights` + `plat_lights_mid` once a frame. |
| `chime` | A two-operator FM voice with a short note queue for notification sounds, mixed into the app's render. |

## Using them in an app

`fm1.toml`:

```toml
include = ["../common"]
```

`app.c`, after `#include "fastmath.h"` (chime needs `fm_sinf`, `fm_exp2f`):

```c
#include "hostlink.c"
#include "ledfx.c"
#include "chime.c"
```

They call fm1-base's `plat_*` functions, which are in scope in the unity build.
They keep no flash state.

`just fw-common-test` builds and runs `tests/` on the computer (hostlink's packet
cutting and reassembly). `firmware/conductor` is the first user; its `tests/basic.txt` exercises all three
in the host simulator. `firmware/morse` (now its own repo, a submodule) carries a copy of `hostlink` and `ledfx` in its `common/`; change
them here and copy them there, or the other way round.
