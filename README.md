# “The Great Escape” Reimplemented

© David Thomas, 2013-2017

16th November 2017

<p align="center">
  <img src="demo.gif" alt="Demo" />
</p>

## Overview
This is a _work in progress_ reimplementation of “[The Great Escape](http://www.worldofspectrum.org/infoseekid.cgi?id=0002125)”: the classic isometric 3D game for the 48K Sinclair ZX Spectrum in which you execute a daring escape from a wartime prison camp. Loosely based on the film of the same name, it was created by [Denton Designs](http://en.wikipedia.org/wiki/Denton_Designs) and published in 1986 by [Ocean Software](http://en.wikipedia.org/wiki/Ocean_Software).

I extracted the graphics and [reverse engineered](https://github.com/dpt/The-Great-Escape/) the game logic from a binary snapshot of the original Spectrum game. Originally written in [Z80](http://en.wikipedia.org/wiki/Zilog_Z80) assembly language, I have translated it into portable C. It now runs without an [emulator](http://fuse-emulator.sourceforge.net/) on macOS and Windows, and eventually could run on mobile platforms and in a web browser.

## Goals of the Project
* Reimplement The Great Escape in portable C code.
	* While being as faithful to the original as possible.
* Fully disassemble and document the original game.
	* My current disassembly is incomplete. Attempting to reimplement the game logic forces through issues and flushes out bugs which enable a complete reimplementation to be made, and the original code fully understood.
* Fix some bugs in the original game.
* Analyse the before-and-after code metrics.
	* How much bigger is the compiled C reimplementation compared to the original game?
	* What can we learn from the original’s tight coding techniques?
* Provide a basis for porting the game to contemporary systems of the ZX Spectrum.
	* Although old ports of the game exist for the [PC](http://www.abandonia.com/en/games/534/Great+Escape,+The.html), [C64](http://www.lemon64.com/?game_id=1090) and [CPC](http://www.amstradabandonware.com/en/gameitems/the-great-escape/1179), retro fans would like to see the game on other contemporary systems too.

Chat
----
[![Join the chat at https://gitter.im/The-Great-Escape/Lobby](https://badges.gitter.im/The-Great-Escape/Lobby.svg)](https://gitter.im/The-Great-Escape/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

## Current State of the Code
### Does it build and run?
Yes!

### What works?

- Main menu
- Input
  - Keyboard + Sinclair joystick (keys 6/7/8/9/0)
  - Kempston joystick (mapped to arrow keys)
- Bell ringer
- Score keeping
- Character moves and animates correctly
- Doors
- Boundaries
- Transitions
- Room drawing
- Exterior drawing, scrolling, masking
- Items can be picked up, used and dropped
- Masking
- AI
- Movable items (stoves, crates)
- Sound effects and menu music (macOS only at the moment)

Non-core:

- UI front-ends preserve their aspect ratio
- Keys for window scaling
- Speed control

### What doesn't?

- Various other divergences from the original to be fixed

## Current Builds
- Xcode build - works. This is my default build so is most likely to be up-to-date.
- Windows build - works - needs Visual Studio 2015.
- Makefile build - works, but runs the game for 100,000 iterations in headless mode.

### Build Status
| Branch     | Windows | macOS |
|------------|---------|-------|
| **master** | [![AppVeyor build status](https://ci.appveyor.com/api/projects/status/2uxw47bwpmdpw0a8/branch/master?svg=true)](https://ci.appveyor.com/project/dpt/the-great-escape-in-c/) | [![Travis CI build status](https://travis-ci.org/dpt/The-Great-Escape-in-C.svg?branch=master)](https://travis-ci.org/dpt/The-Great-Escape-in-C) |

### Xcode Build
Open up the Xcode project `platform/osx/The Great Escape.xcodeproj` and build that using ⌘B. Run using ⌘R.

### Windows Build
Open up the Visual Studio solution `platform/windows/TheGreatEscape/TheGreatEscape.sln` and build that using F7. Run using F5.

### Makefile Build
The Makefile build compiles the code using clang, and offers some other handy options, like running an [analysis](http://clang-analyzer.llvm.org/) pass, generating [ctags](http://ctags.sourceforge.net/), running the source through [splint](http://www.splint.org/) and reformatting the source code through [astyle](http://astyle.sourceforge.net/).

```
dave@macbook platform/generic $ make
Usage:
  build		Build
  clean		Clean a previous build
  analyze	Perform a clang analyze run
  lint		Perform a lint run
  tags		Generate tags
  cscope	Generate cscope database
  astyle	Run astyle on the source
  docs		Generate docs

MODE=<release|debug>
```

To build:

```
cd platform/generic
make build
```

Or `MODE=release make build` to make the release version of the code.

The Makefile-based build presently links against a stub `main()` which does nothing, so does not provide useful runnable code yet.

## Components
### `TheGreatEscape`
This is the main game reimplemented in a single (static) library.

### `ZXSpectrum`
Defines an interface to a virtual ZX Spectrum to which the game talks, replacing the bare-metal `IN` and `OUT` instructions and providing a screen to draw to.

## Source Layout
```
./
    include/            - public headers
        TheGreatEscape/
        ZXSpectrum/
    libraries/          - sources
        TheGreatEscape/
            include/    - private headers
        ZXSpectrum/
    platform/           - platform-specific source
        generic/        - generic Makefile build environment
        osx/            - Xcode build environment
        windows/        - Windows build environment
```

# Further (Planned) Changes
## Variable resolution
The goal in this project, other than an understanding the original game, is to bring the escapade to modern platforms. Those platforms feature various modern marvels including high resolution, high density screens and capacitive touch screens.

Bearing in mind the various new devices on which the game will be run we must allow for changes to screen resolution, screen density and input devices. So some hard-coded constants for the dimensions of the screen will be replaced with references to variables kept in the game state.

This is tricky as at some point we’ll need to dispense with the Spectrum’s cunning/weird screen memory layout.

# Related Links
[Porting Chuckie Egg](http://marklomas.net/ch-egg/articles/porting-ch-egg.htm)

