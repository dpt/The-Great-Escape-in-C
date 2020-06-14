# “The Great Escape” Ported to C

© David Thomas, 2013-2020

21st April 2020

<p align="center">
  <img src="demo.gif" alt="Demo" />
</p>


## Overview
This is a largely complete port to C of “[The Great Escape](http://www.worldofspectrum.org/infoseekid.cgi?id=0002125)”: the classic isometric 3D game for the 48K Sinclair ZX Spectrum in which you execute a daring escape from a wartime prison camp. Very loosely based on the film of the same name, it was created by [Denton Designs](http://en.wikipedia.org/wiki/Denton_Designs) and published in 1986 by [Ocean Software](http://en.wikipedia.org/wiki/Ocean_Software).

[Over here](https://github.com/dpt/The-Great-Escape/) I reverse engineered the original game from a binary snapshot of the Spectrum version, decoding the graphics, data tables and all of the logic. Originally written in [Z80](http://en.wikipedia.org/wiki/Zilog_Z80) assembly language, I have translated it into portable C code where now builds and runs _exactly like the original_ on macOS, Windows and RISC OS but without the need of an [emulator](http://fuse-emulator.sourceforge.net/). Eventually it could run on mobile platforms and in a web browser.


## Goals of the Project
* Reimplement The Great Escape in portable C code.
	* While being as accurate a recreation of the original as possible.
* Fully disassemble, document and understand the original game.
	* Attempting to reimplement the game logic 100% flushes out bugs which enable a complete reimplementation to be made, and the original code fully understood.
* Fix some bugs in the original game.
* Analyse the before-and-after metrics.
	* How much bigger and slower is the compiled C reimplementation compared to the original game?
	* What can we learn from the original’s tight coding techniques?
* Provide a basis for porting the game to contemporary systems of the ZX Spectrum.
	* Although old ports of the game exist for the [PC](http://www.abandonia.com/en/games/534/Great+Escape,+The.html), [C64](http://www.lemon64.com/?game_id=1090) and [CPC](http://www.amstradabandonware.com/en/gameitems/the-great-escape/1179), retro fans would like to see the game on [other contemporary systems](http://atariage.com/forums/topic/239167-new-game-great-escape/) too.


## Chat
[![Join the chat at https://gitter.im/The-Great-Escape/Lobby](https://badges.gitter.im/The-Great-Escape/Lobby.svg)](https://gitter.im/The-Great-Escape/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)


## Running the Game
The port is an exact recreation of the ZX Spectrum version of The Great Escape, including the input device selection menu. It builds and runs on macOS, Windows and RISC OS presently. The macOS version is currently the "best" featuring the menu music and sound effects which the Windows port lacks.

When you start the game hit '0' to start - this will let you define your preferred keys. You could also choose '3' for Sinclair (keys 6/7/8/9/0). The macOS port will let you choose Kempston and use the arrow keys and '.' for fire.

There are various other controls which vary by OS. On macOS consult the menus for keyboard shortcuts.

The front-ends attempt to always preserve the game's aspect ratio and snap to whole pixels.


## Current Builds
- Xcode build - works. This is my default build so is most likely to be up-to-date.
- Windows build - works. This needs Visual Studio 2015. Lacks sound.
- RISC OS build - works. Lacks sound.
- Makefile build - works, but non-graphical. Runs the game for 100,000 iterations of the main loop then stops.

There is an Emscripten build too which runs in the browser, however the structure of the game is not exactly amenable to the browser world. This may take some invasive changes to make it work well. The RISC OS build has a similar issues.

### Build Status
| Branch     | Windows | macOS |
|------------|---------|-------|
| **master** | [![AppVeyor build status](https://ci.appveyor.com/api/projects/status/2uxw47bwpmdpw0a8/branch/master?svg=true)](https://ci.appveyor.com/project/dpt/the-great-escape-in-c/) | [![Travis CI build status](https://travis-ci.org/dpt/The-Great-Escape-in-C.svg?branch=master)](https://travis-ci.org/dpt/The-Great-Escape-in-C) |

### Xcode Build
Open up the Xcode project `platform/osx/The Great Escape.xcodeproj` and build that using ⌘B. Run using ⌘R.

### Windows Build
Open up the Visual Studio solution `platform/windows/TheGreatEscape/TheGreatEscape.sln` and build that using F7. Run using F5.

### RISC OS Build
If you're nutty enough to want to build the RISC OS version then you should probably talk to me first.

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
        riscos/         - RISC OS build environment
        windows/        - Windows build environment
```


# Related Links
[Porting Chuckie Egg](http://marklomas.net/ch-egg/articles/porting-ch-egg.htm)
