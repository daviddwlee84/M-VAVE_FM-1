# Conductor firmware

The FM-1 as a desk console for coding agents. The keys are agents, their lights
say how urgent each one is, the screen says what it is, a chime says it without
looking. The host daemon in `host/conductor` talks to hc or Herdr.

Design and status (Traditional Chinese): `docs/agentic/conductor.md`.
Protocol: `host/conductor/PROTOCOL.md`. Shared modules: `firmware/common/`.

```sh
just fw-test conductor     # host simulator + tests/basic.txt
just fw-build conductor    # build/conductor.fwsc (USB only, no Bluetooth yet)
just cond-emu              # in fm1-emu with the virtual USB cable; then `just cond-run hc`
```

Not flashed to hardware yet. A flash is a firmware write: owner approval and the
preflight in `AGENTS.md`.

MIT. Built on [fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0).
