<!-- For a PR that changes a firmware (firmware/APP). Delete what does not apply. AGENTS.md: "Pull requests". -->

## What changes for the player

<!-- In a few lines: what is new or different on the FM-1, and why (the owner's feedback, a bug). -->

## 1. Try it (how it plays)

- fm1-emu link: <!-- just fw-build APP && just fw-preview APP pr-N: the package itself in the browser, no account.
  Refresh it after new commits; remove it (--remove) when the PR is merged or closed. -->

## 2. Screens

<!-- The new or changed screens: the simulator test's shots (firmware/APP/build/host/*.png), copied into
     docs/assets/APP/ if the docs use them, or dragged into this description. -->

## 3. Before it goes on the unit

- [ ] `just fw-test APP` (count):
- [ ] unit tests (count):
- [ ] `just fw-build APP`: size, RAM, no soft-double; package SHA-256:
- [ ] fm1-emu `uc25hq80` and `p25q80`: a save, a restart, a reload (writes):
- [ ] rehearsed over the latest backup (sectors written; other firmwares' records untouched):
- [ ] stored data: format changed? kept across the update?

## 4. On the unit (the owner)

<!-- Steps to try on the FM-1 after the install, each with what should happen. The owner ticks them; the result goes
     into docs/device-log.md. One unit: one PR on it at a time. -->

- [ ] step → expected
