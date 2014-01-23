# "The Great Escape" Reimplemented

© David Thomas, 2013-2014.

18th January 2014 +

[[ Temporary notes look like this. ]]

## Overview
This is a _work in progress_ reimplementation of "[The Great Escape](http://www.worldofspectrum.org/infoseekid.cgi?id=0002125)", the classic isometric 3D game for the Sinclair ZX Spectrum 48K in which you execute a daring escape from a wartime prison camp. Loosely based on the film of the same name, it was created by [Denton Designs](http://en.wikipedia.org/wiki/Denton_Designs) and published in 1986 by [Ocean Software](http://en.wikipedia.org/wiki/Ocean_Software).

The game's graphics were extracted and logic was reverse engineered from a binary snapshot of the original Spectrum game. Originally written in Z80 assembly language, I am now rewriting it in portable C with the goal of eventually making it run without an [emulator](http://fuse-emulator.sourceforge.net/) across a range of modern computers, including in-browser via [Emscripten](https://emscripten.org).

This document describes the process by which the disassembly is being converted from [SkoolKit](http://pyskool.ca/?page_id=177) format into, hopefully, functioning and portable C code.

## Other Goals of the Project
* Fully disassemble and document the original game, explaining its magic.
	* The existing disassembly is incomplete. Attempting to reimplement the game logic forces through issues which enable a complete reimplementation to be made, and the original code fully understood.
* Fix some bugs in the original game.
* Analyse the before-and-after code metrics.
	* How much bigger is the compiled C reimplementation compared to the original game?
	* What can we learn from the original's tight coding techniques?
 * Provide a basis for porting the game to contemporary systems of the ZX Spectrum. (Old ports of the game exist for the [PC](http://www.abandonia.com/en/games/534/Great+Escape,+The.html), [C64](http://www.lemon64.com/?game_id=1090) and [CPC](http://www.amstradabandonware.com/en/gameitems/the-great-escape/1179)).

## Components
### `TheGreatEscape`
This is the main game reimplemented in a single (static) library.

### `ZXSpectrum`
Defines an interface to a virtual ZX Spectrum to which the game talks, replacing the bare-metal `IN` and `OUT` instructions and providing a screen to draw to.

## Source Layout
```
./
    include/
        TheGreatEscape/ - public headers of TheGreatEscape library
        ZXSpectrum/     - public headers of ZXSpectrum library
    libraries/
        TheGreatEscape/ - source of TheGreatEscape library
            include/... - private headers of TheGreatEscape library
        ZXSpectrum/     - source of ZXSpectrum library
    platform/
        generic/        - build environment for a generic Unix
        osx/            - build environment for OS X
        (other platforms would be 'windows', 'ios', etc.)
```

## Current State of the Code
### Does it build?
The build will complete but with numerous warnings. Many of the currently implemented routines have chunks of code absent, as-yet unused variables, or do not correctly 'join up' with others.

Code which does not build is disabled using `#if 0`. This keeps as much of the working code visible to the compiler as possible, allowing mistakes to be caught as early as possible.

### Does it run?
Sort of! The Xcode build can reach the (very early) main menu code and then gets stuck in a loop at that point waiting on keyboard input.

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
My previous work building the SkoolKit control file, [TheGreatEscape.ctl](../reverse/TheGreatEscape.ctl), is used as the source for the conversion. It contains my interpretation of the game logic in C-style pseudocode with occasional Z80 instructions embedded. The pseudocode is copied out to the main `TheGreatEscape.c` where it can be marshalled into syntactically correct C (or as close as is practically possible).

Each function is reimplemented and placed in original game order in the main `TheGreatEscape.c` file. Reimplemented routines are modelled as closely as possible, given language constraints, on the original game code. For example, loops which are idiomatically written as 'for' loops in regular C are typically written as 'do .. while' loops here, as that most closely matches the original code.

Sometimes it isn't possible to closely match the original code, as it uses techniques unavailable in C. For example, some routines have multiple entry points, the last of which falls through from the end of the function into a final chunk of common code. We can't replicate that in C, so in these cases the routine ends with a final call out to the following function.

The pseudocode contains Z80 instructions which don't map well onto C's language features (rotates, register exchanges, and so on). These are left commented out until the point where they can be rewritten as C. [[ An option here is to write the Z80 instructions as macros, and have the macros expand out to equivalent C, in the same way that the [RISC OS port of Chuckie Egg](http://homepages.paradise.net.nz/mjfoot/riscos.htm) was done.]]

Functions are defined in the same order as in the original game code. Leading comments which precede the function list the hex address of the routine being implemented and a description of the routine. Javadoc-style comments are added too, to describe the arguments. For example:
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

Since the data is not defined in the control file, it is instead copied across from the .skool file and reformed into acceptable C structures.

Room definitions are decoded using a small Python script. [[ Provide the script. ]]

### Necessary changes
I’ve already mentioned about having to alter the code to simulate fall-though. Other changes are also required to enable the code to run.

Global variables—which in the original are clumped together in a few places across the memory map—are gathered together into a single `tgestate` context structure. A pointer to this is passed to all functions, save for leaf functions, as the first argument. In addition to being neater than having variables scattered everywhere, this allows the game to be multiply instantiated (multiple copies of the game can run from a single instance of the game code).

Strings are stored in ASCII rather than the custom font encoding the original game used (the game font uses the same glyph for ‘0’ and ‘O’). The font bitmap data remains encoded as per the game but an `ascii_to_font[]` table is introduced to convert from ASCII to the font encoding.

### Feeding changes back into .ctl form
The .ctl disassembly is unavoidably incomplete, still full of mysteries and, no doubt, mistakes. Having to reimplement the game in concrete C forces many questions which cause changes in interpretation of the original code. Discoveries ought to be fed back into the SkoolKit-format [TheGreatEscape.ctl](../reverse/TheGreatEscape.ctl) file (at the very least function names must kept in sync).

At some point another pass will need to be made over the .ctl file to bring across **all** changes.

### Obstacles
When a high-level language is compiled into assembly language a series of passes are made over the code to reduce it down. Names are lost, variables are assigned to registers, optimisations are carried out, etc. In converting The Great Escape to C we're performing a manual decompilation of the game's assembly language, introducing syntactic elements. [[ Reconstructing source which may not have existed anyway. ]]

I'm unsure how much of the original game code was hand-assembled. Likely it was all of it, though some sections have a different texture/flavour to it.

#### Encapsulating structures
Structures are created to encapsulate the data used by the game. Perhaps the most critical is the `vischar`: a *visible character*. As a type this is called `vischar_t` and is one of the most critical structures in the game.

#### Making sense of registers (register "de-allocation")
The register HL, for example, might be used multiple times within a routine with distinct meanings. These have to be identified and recovered. You might see, `uint8_t *HL`, `uint16_t HLfoo` and `const uint8_t *HLbar`, within the same routine, identifying the separate uses to which the original register was put.

This happens in conjunction with PUSH and POP-ing of registers to stack, which is resolved by renaming nested variables and deleting the PUSH/POP ops.

#### Bank switching
The Z80 has two banks of registers. HL, for example, has a counterpart register called HL'. Registers are switched between with EX or EXX instructions.

For a code sequence where obvious delimiting EX or EXX instructions are present, variables between the two points are renamed to, e.g. HLdash then the EX and EXX instructions are deleted.

#### Self-modifying code
Many of the original functions would poke values directly into the instruction stream. These have to be recognised and each one factored out into its own variable. If the self modification is only within the routine, it can become a local. Otherwise it has to be placed into the state structure.

#### Piecemeal structure access
Much of the code points HL to a structure member, loads it, increments HL, loads more, increments HL, ... This resulting set of interleaved HL++ or HL-- ops gets in the way of 'actual work'. This is resolved by adjusting the initial HL pointer to point to the base of the structure and introducing offsets or named references for each member accessed.

### Editing tips
I usually edit the C code side-by-side in a three-pane configuration. I have `TheGreatEscape.c` open alongside `TheGreatEscape.skool` and `TheGreatEscape.ctl`. This lets me see the original Z80, the interpreted logic and the reimplemented C simultaneously.

## Coding style
* Follow the existing style of the code.
* Use spaces not tabs. Indent by two spaces.
* Format source lines to 80 columns wide (to allow side-by-side diffs).
* Order functions and data by address of the original.
* Order #includes consistently (list in 'depth' order, C stdlib first, etc.).
* Name `functions_like_this`.
* Suffix type names with `_t`.
* Use Javadoc/Doxygen style comments throughout.
* Aim for strict C89 code (to maximise portability).
 * Don't use global variables. Place any 'globals' in `tgestate_t`.
* Separate logic and data. Place large data blobs in external source files.

## Further Changes
### Variable resolution
Of course the goal in this project, aside from understanding the original game, is to bring the compelling escapade to modern platforms. Those platforms have various different high resolution screens, capacitive touch screens, motion sensors and other modern marvels.

Bearing in mind the various device contexts in which the game will be run we must allow for changes to screen resolution, screen density and input devices. So some hard-coded constants for the dimensions of the screen will be replaced with references to variables kept in the game state.

## Other stuff / Sargasso / Aide memoires
Scope data as tightly as possible - move it in to a function if only that function uses it.

I'm undecided yet whether platform drivers ought to be written as new implementations of ZXSpectrum, or callbacks from within it.

Many of the game's data structures are largely, but not entirely, const. For example, the room definitions have object indices poked into them.

The original code uses a ‘squash stack and jump to main’ long jump in certain situations. In the code these are marked with `NEVER_RETURNS;`. The C standard library functions `longjmp` and `setjmp` are used to simulate this.

# TODOs
* Fix coordinate order in pos structures: (y,x) -> (x,y).

# Links
[Porting Chuckie Egg](http://marklomas.net/ch-egg/articles/porting-ch-egg.htm)
