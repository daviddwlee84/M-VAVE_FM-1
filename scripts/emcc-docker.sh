#!/bin/sh
# emcc from a pinned Emscripten image, for fm1-base's `tools/build.py APP --web` on a computer without emsdk
# (just fw-web APP puts this first on PATH as `emcc`). The repo and the temp dir are mounted at their own paths, so
# the absolute paths build.py passes work inside the container.
exec docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp -v "$HOME:$HOME" -v /private/tmp:/private/tmp -v /tmp:/tmp \
  -w "$PWD" emscripten/emsdk:3.1.74 emcc "$@"
