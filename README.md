# "The Great Escape" Reimplemented

© David Thomas, 2013-2014.

23rd January 2014

## Overview
This is a _work in progress_ reimplementation of "[The Great Escape](http://www.worldofspectrum.org/infoseekid.cgi?id=0002125)", the classic isometric 3D game for the Sinclair ZX Spectrum 48K in which you execute a daring escape from a wartime prison camp. Loosely based on the film of the same name, it was created by [Denton Designs](http://en.wikipedia.org/wiki/Denton_Designs) and published in 1986 by [Ocean Software](http://en.wikipedia.org/wiki/Ocean_Software).

The game's graphics were extracted and logic was reverse engineered from a binary snapshot of the original Spectrum game. Originally written in [Z80](http://en.wikipedia.org/wiki/Zilog_Z80) assembly language, I am now translating it into portable C with the goal of eventually making it run without an [emulator](http://fuse-emulator.sourceforge.net/) across a range of modern computers, including in-browser via [Emscripten](https://emscripten.org/).

This document describes the process by which the disassembly is being converted from [SkoolKit](http://pyskool.ca/?page_id=177) format into, hopefully, functioning and portable C code.

## Goals of the Project
* Accurately reimplement The Great Escape in portable C code.
* Fully disassemble and document the original game. The existing disassembly is incomplete. Attempting to reimplement the game logic forces through issues which enable a complete reimplementation to be made, and the original code fully understood.
* Fix a few bugs in the original game.
* Analyse the before-and-after code metrics. How much bigger is the compiled C reimplementation compared to the original game? What can we learn from the original's tight coding techniques?
* Provide a basis for porting the game to contemporary systems of the ZX Spectrum. Although old ports of the game exist for the [PC](http://www.abandonia.com/en/games/534/Great+Escape,+The.html), [C64](http://www.lemon64.com/?game_id=1090) and [CPC](http://www.amstradabandonware.com/en/gameitems/the-great-escape/1179), retro fans would like to see the game on other contemporary systems too.

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
        osx/            - Xcode build environment for OS X
```

## Current State of the Code
### Does it build?
The build will complete but with numerous warnings. Many of the currently implemented routines have chunks of code absent, as-yet unused variables, or do not correctly 'join up' with others.

Code which does not build is disabled using `#if 0`. This keeps as much of the working code visible to the compiler as possible, allowing mistakes to be caught early.

### Does it run?
Sort of! The Xcode build will run to the (very early) main menu code and then get stuck in a loop at that point waiting on keyboard input.

There isn’t yet any screen, sound or input handling code hooked up to the `ZXSpectrum` library, so even if the game could run further there wouldn’t be anything to see.

## Current Builds
Only the OS X hosted builds are currently being worked on.

### OS X
There are two ways to build the code on OS X: Makefile-based or Xcode-based:

#### Makefile
The Makefile build compiles the code using clang, and offers some other handy options, like running an [analysis](http://clang-analyzer.llvm.org/) pass, generating [ctags](http://ctags.sourceforge.net/), running the source through [splint](http://www.splint.org/) and reformatting the source code through [astyle](http://astyle.sourceforge.net/).
```
dave@macbook platform/generic $ make
Usage:
  build		Build
  analyze	Perform a clang analyze run
  lint		Perform a lint run
  tags		Generate tags
  astyle	Run astyle on the source

MODE=<release|debug>
```

To build:
```
cd platform/generic
make build
```

Or `MODE=release make build` to make the release version of the code.

The Makefile-based build presently links against a stub `main()` which does nothing, so does not provide useful runnable code yet.

#### Xcode
Open up the Xcode project `platform/osx/The Great Escape.xcodeproj` and build that using ⌘B.

## The Conversion Process
My previous work building the SkoolKit control file, [TheGreatEscape.ctl](../reverse/TheGreatEscape.ctl), is used as the source for the conversion. It contains my interpretation of the game logic in C-style pseudocode with occasional Z80 instructions embedded. The pseudocode is copied out to `TheGreatEscape.c` where it can be marshalled into syntactically correct C (or as close as is practically possible prior to rewriting).

Each function is reimplemented and placed in original game order in the main `TheGreatEscape.c` file. Reimplemented routines are modelled as closely as possible, given language constraints, on the original game code. For example, loops which are idiomatically written as `for` loops in regular C are typically written as `do .. while` loops here, as that most closely matches the original code.

Sometimes it isn't possible to closely match the original code, as it uses techniques unavailable in C. For example, some routines have multiple entry points, the last of which falls through from the end of the function into a final chunk of common code. We can't replicate that in C, so in these cases the routine ends with a final call out to the following function.

The pseudocode contains Z80 instructions which don't map well onto C's language features (rotates, register exchanges, and so on). These are left commented out until the point where they can be rewritten as C. (A possible option here is to write the Z80 instructions as macros, and have the macros expand out to equivalent C, in the same way that the [RISC OS port of Chuckie Egg](http://homepages.paradise.net.nz/mjfoot/riscos.htm) was done.)

Functions are defined in the same order as in the original game code. Comments preceding the function list the hex address of the routine being implemented and a description of the routine. Javadoc-style comments are added too, to describe the arguments. For example:
```
/**
 * $697D: Setup movable item.
 *
 * \param[in] state       Pointer to game state.
 * \param[in] movableitem Pointer to movable item.
 * \param[in] character   Character.
 */
```

Initially the content of the re-formed but non-working routine is wrapped in `#if 0` .. `#endif` markers. Slowly, parts are replaced with functioning code and these markers are pushed further down the routine until they coincide and can be deleted.

Finally, to make the code work as a library it must be supplemented with routines to create and destroy instances, and initialise its state. These currently are `tge_create()` and `tge_destroy()`.

### Data
Game bitmaps, masks, tiles, maps and other large data blobs are factored out to separate source files to keep the main code in `TheGreatEscape.c` as clean as possible.

That said, data should be scoped as tightly as possible. If only one function uses that data, move it into the body of the function and ideally mark it `static const`.

Since the data is not fully defined in the control file, it is instead copied across from the .skool file and reformed into acceptable C structures.

Room definitions are decoded to C source using a small Python script called `DecodeRooms.py`.

### Other necessary changes
I’ve already mentioned about having to alter the code to simulate fall-though. Other changes are also required to enable the code to run.

In the original game **global variables** are clumped together in a few places across the memory map. These are gathered together into a single `tgestate` context structure. A pointer to this state structure is passed to all functions, save for leaf functions, as the first argument.

In addition to being neater than having variables scattered everywhere, this allows the game to be multiply instantiated: multiple copies of the game can run from a single instance of the game code. It also makes it a bit easier to serialise the game’s state.

**Strings** are stored in ASCII rather than the custom font encoding the original game used (the game font uses the same glyph for ‘0’ and ‘O’). The font bitmap data remains encoded as per the game but an `ascii_to_font[]` table is introduced to convert from ASCII to the font encoding.

### Feeding changes back into .ctl form
At this point the .ctl disassembly is unavoidably incomplete. It’s still full of mysteries and, no doubt, mistakes. Having to reimplement the game in concrete C forces many questions which invalidate the existing interpretation of the original code.

New discoveries ought to be fed back into the SkoolKit-format [TheGreatEscape.ctl](../reverse/TheGreatEscape.ctl) file. At the very least function names must kept in sync.

At some point a whole other pass will need to be made over the .ctl file to bring across **all** changes.

### Obstacles
When a high-level language is compiled into assembly language a series of passes are made over the code to reduce it down. Names are lost, variables are assigned to registers, optimisations are carried out, etc. In converting The Great Escape to C we're performing a manual decompilation of the game's assembly language and so reconstructing a form of source code which may not have existed to begin with.

I'm unsure how much of the original game code was hand-assembled. Likely it was all of it, though some sections have a different texture/flavour.

#### Encapsulating structures
Structures are created to encapsulate the data used by the game. C’s structures are regular, whereas the assembly language game is free to play fast and loose with the ‘rules’: copying sub-structures around, etc.

Perhaps the most ubiquitous of the structures is the `vischar`: a *visible character*. Much of the game code is concerned with walking the `vischars[]` array (located at $8000 in the original game).

#### Making sense of registers (register "de-allocation")
The register HL, for example, might be used multiple times within a routine with distinct meanings. These have to be identified and recovered. You might see multiple declarations of HL: e.g. `uint8_t *HL`, `uint16_t HLfoo` and `const uint8_t *HLbar`, within the same routine, identifying the separate uses to which the original register was put.

This happens in conjunction with PUSH and POP-ing of registers to stack, which is resolved by renaming nested variables and deleting the PUSH/POP ops.

#### Bank switching
The Z80 has two banks of registers. HL, for example, has a counterpart register called HL'. Registers are switched between with `EX AF,AF’`, or `EXX` instructions.

For a code sequence where obvious delimiting EX/EXX instructions are present, variables between the two points are renamed to, e.g. `HLdash` and then the EX and EXX instructions are deleted.

#### Self-modifying code
Many of the original functions would poke values directly into the instruction stream. These have to be recognised and each one factored out into its own variable. If the self modification is only within the routine, it can become a local. Otherwise it has to be placed into the state structure.

#### Piecemeal structure access
Much of the code points HL to a structure member, loads it, increments HL, loads more, increments HL, and so on. This resulting set of interleaved `HL++` or `HL--` ops gets in the way of 'actual work'. This is resolved by adjusting the (often offsetted) initial HL pointer to point to the base of the structure and using normal structure references for each access.

#### Code which never returns
The original code will squash the stack and jump directly to the main game loop certain situations. The C standard library functions `longjmp` and `setjmp` are used to simulate this. In the code these are marked with `NEVER_RETURNS;` which is an informational and safety macro.

## Coding style
When editing the code:

* Follow the existing style of the code.
* Use spaces, not tabs. Indent by two spaces.
* Format source lines to 80 columns wide (to allow two or three side-by-side diffs).
* Order functions and data by the address of the original.
* Order #includes consistently (list in 'depth' order, C stdlib first, etc.).
* Name `functions_like_this`.
* Name `constants_LIKE_THIS`.
* Suffix type names with `_t`.
* Use Javadoc/Doxygen style comments throughout.
* Aim for strict C89 code (to maximise portability).
* Don't use global variables. Place any 'globals' in `tgestate_t`.
* Separate logic and data. Place large data blobs in external source files.

### Editing tips
I usually edit the C code side-by-side in a three-pane configuration in Vim. I have `TheGreatEscape.c` open alongside `TheGreatEscape.skool` and `TheGreatEscape.ctl`. This lets me see the original Z80, the interpreted logic and the reimplemented C simultaneously.

# (Major) TODOs
* Fix coordinate order in pos structures: (y,x) -> (x,y).

# Further (Planned) Changes
## Variable resolution
The goal in this project, other than an understanding the original game, is to bring the escapade to modern platforms. Those platforms feature various modern marvels including high resolution, high density screens and capacitive touch screens.

Bearing in mind the various new devices on which the game will be run we must allow for changes to screen resolution, screen density and input devices. So some hard-coded constants for the dimensions of the screen will be replaced with references to variables kept in the game state.

This is tricky as at some point we’ll need to dispense with the Spectrum’s cunning/weird screen memory layout.

# Sargasso
* I'm undecided yet whether platform drivers ought to be written as new implementations of `ZXSpectrum`, or callbacks from within it.
* Many of the game's data structures are largely, but not entirely, const. For example, the room definitions have object indices poked into them when characters get in and out of bed, or sit down to eat.

# Related Links
[Porting Chuckie Egg](http://marklomas.net/ch-egg/articles/porting-ch-egg.htm)
