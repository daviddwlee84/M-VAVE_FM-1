# previews

Firmware packages (`.fwsc`) for trying a pull request in the browser, in fm1-emu
(<https://joakimeriksson.github.io/fm1-emu/>), which loads them from raw.githubusercontent.com (it sends CORS
headers; GitHub release assets do not). Not for installing: a package to install comes from a build of `main`, with
the preflight in `AGENTS.md`.

- `pr-N/APP.fwsc`: the build a pull request's description links. Removed once the PR is merged or closed.
- `main/APP.fwsc`: the build of `main`.

Link: `https://joakimeriksson.github.io/fm1-emu/?package=https://raw.githubusercontent.com/daviddwlee84/M-VAVE_FM-1/previews/pr-N/APP.fwsc&name=APP&chip=uc25hq80`
