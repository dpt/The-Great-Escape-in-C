# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

C port of the ZX Spectrum game "The Great Escape", translated routine by routine from a Z80 disassembly (SkoolKit control file in the separate `The-Great-Escape` repo). Goal is exact behaviour match with the original, so code deliberately mirrors the original structure. See `CONVERSION.md` for conversion rules and `NOTES.md` for coordinate systems and projection maths. `BUGS.md` and `TODO.md` track known issues.

## Build

- macOS/Windows/Linux CMake: `./build.sh` (configures and builds in `build/`, uses Ninja if present). Manual: `mkdir build && cd build && cmake .. && cmake --build .`
- Option `-DTGE_SAVES=OFF` disables load/save.
- Headless test build (runs 100,000 main loop iterations then exits): `make -C platform/generic TARGET=osx-any-clang MODE=release build` (see header of `platform/generic/Makefile`).
- Xcode: `platform/osx/TheGreatEscape.xcodeproj` (CI uses `xcodebuild -scheme Debug`).
- RISC OS: `riscos-ci-build.sh` (GCCSDK + CMake). Emscripten/SDL: scripts in `platform/generic/`.
- CI definitions: `.github/workflows/ci.yml`. No unit test suite or linter exists. Headless build is the only automated check.

## Architecture

- `libraries/TheGreatEscape/` is the game as one static library. Public header `include/TheGreatEscape/TheGreatEscape.h`; private headers in `libraries/TheGreatEscape/include/`.
  - `Engine/` game logic, in original game order. Function comments carry the original Z80 hex address (e.g. `$697D`).
  - `Data/` large tables (tiles, sprites, maps, room defs, masks, music) split out as `static const` blobs.
  - `Extend/Save.c` save/load, additions not in original.
- All original globals live in a single `tgestate` state struct, passed as first argument to every function except leaf functions. Instances created and destroyed via `tge_create()` / `tge_destroy()`.
- The virtual Spectrum and the front-end hosts live in the separate `ZXGame` repo (sibling directory; override with `-DZXGAME_DIR=`). Game-to-host interface is `zxgame_ops_t` (`ZXGame/Game.h`); TGE supplies `tge_game_ops` in `Engine/GameOps.c`. The Spectrum library replaces Z80 `IN`/`OUT`, and provides screen memory, border, keyboard, Kempston and speaker. It converts Spectrum screen memory to 4bpp or 32bpp. The game talks only to this interface, never to the host platform.
- `platform/{osx,windows,riscos,generic}/` hold front ends (UI, sound, timing) that drive the library. `generic/` has the headless `main.c` and SDL `sdl3main.c`. The osx, windows and riscos apps are thin: the hosts are in ZXGame.

## Conventions

- Code is C90 (`C_STANDARD 90`), built with `-Wall -Wextra`. Keep it C90-clean (declarations at top of block, no `//` comments).
- Loops often use `do .. while` to match the original. Multi-entry-point Z80 routines are modelled by ending a function with a call to the next one. Keep this shape when changing logic.
- Keep original-address doc comments (`$XXXX: description`) on functions.
- Untracked files at repo root (`thread.h`, `wavout.c`, `win32sound.c`, scratch dirs) are the user's experiments, not part of the build.
