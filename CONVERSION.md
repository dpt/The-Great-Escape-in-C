# “The Great Escape” Reimplemented

© David Thomas, 2013-2017

## The Conversion Process
My previous work building the SkoolKit control file, [TheGreatEscape.skool](https://github.com/dpt/The-Great-Escape/blob/master/TheGreatEscape.skool), is used as the source for the conversion. It contains my interpretation of the game logic in C-style pseudocode with occasional Z80 instructions embedded. The pseudocode was copied out to `TheGreatEscape.c` where it was marshalled into syntactically correct C (or as close as is practically possible prior to rewriting).

Each function was reimplemented and placed (in original game order) in the main `TheGreatEscape.c` file. Reimplemented routines were modelled as closely as possible, given language constraints, on the original game code. For example, loops which are idiomatically written as `for` loops in regular C were typically written as `do .. while` loops here, as that most closely matches the original code.

The pseudocode contains Z80 instructions which don’t map well onto C’s language features (rotates, register exchanges, and so on). These are left commented out until the point where they can be rewritten as C. (A possible option here is to write the Z80 instructions as macros, and have the macros expand out to equivalent C, in the same way that the [RISC OS port of Chuckie Egg](http://homepages.paradise.net.nz/mjfoot/riscos.htm) was done.)

It wasn't always possible to exactly match the original code, as it uses techniques unavailable in C. For example, some of the original routines have multiple entry points; some fall through from the end of the function into a final chunk of common code. I can’t replicate that in C, so in these cases the routine ends with a final call out to the following function.

Functions were defined in the same order as in the original game code. Comments preceding the function list the hex address of the routine being implemented and a description of the routine. Javadoc-style comments are added too, to describe the arguments. For example:

```
/**
 * $697D: Setup movable item.
 *
 * \param[in] state       Pointer to game state.
 * \param[in] movableitem Pointer to movable item.
 * \param[in] character   Character.
 */
```

Initially the content of the re-formed but non-working routine was wrapped in `#if 0` .. `#endif` markers. As conversion progressed, parts were replaced with functioning code and these markers were pushed further down the routine until they coincide and can be deleted.

Finally, to make the code work as a library it was supplemented with routines to create and destroy instances, and initialise its state. These are `tge_create()` and `tge_destroy()`.

### Data
Game bitmaps, masks, tiles, maps and other large data blobs were factored out to separate source files to keep the main code in `TheGreatEscape.c` as clean as possible.

Data was scoped as tightly as possible. If only one function uses that data it was moved into the body of the function and marked `static const`.

Room definitions were decoded to C source using a small Python script called `DecodeRooms.py`.

### Other necessary changes
I’ve already mentioned about having to alter the code to simulate fall-though. Other changes were also required to enable the code to run.

In the original game **global variables** are clumped together in a few places across the memory map. These were gathered together into a single `tgestate` context structure. A pointer to this state structure is passed to all functions, save for leaf functions, as the first argument.

In addition to being neater than having variables scattered everywhere, this allows the game to be multiply instantiated: multiple copies of the game can run from a single instance of the game code. It also makes it a bit easier to serialise the game’s state.

**Strings** were changed to ASCII rather than the custom font encoding the original game used (the game font uses the same glyph for ‘0’ and ‘O’). The font bitmap data remains encoded as per the game but an `ascii_to_font[]` table is introduced to convert from ASCII to the font encoding.

### Feeding changes back into SkoolKit form
At this point in the conversion the disassembly is unavoidably incomplete. It’s still full of mysteries and, no doubt, mistakes. Having to reimplement the game in concrete C forces many questions which invalidate the existing interpretation of the original code.

New discoveries ought to be fed back into the SkoolKit-format [TheGreatEscape.skool](https://github.com/dpt/The-Great-Escape/blob/master/TheGreatEscape.skool) file. At the very least function names must kept in sync.

(Originally I used a {incomplete, blocks only} .ctl file then later tured it into a fully-formed .skool file).

Feeding changes back into the .skool file is an ongoing process as the conversion continues.

### Obstacles
When a high-level language is compiled into assembly language a series of passes are made over the code to reduce it down. Names are lost, variables are assigned to registers, optimisations are carried out, etc. In converting The Great Escape to C we’re performing a manual decompilation of the game’s assembly language and so reconstructing a form of source code which didn't ever exist to begin with.

I’m unsure how much of the original game code was hand-assembled. Likely it was all of it, though some sections have a different texture/flavour.

#### Encapsulating structures
Structures were created to encapsulate the data used by the game. C’s structures are regular, whereas the assembly language game is free to play fast and loose with the ‘rules’: copying sub-structures around, etc.

Perhaps the most ubiquitous of the structures is the `vischar`: a *visible character*. Much of the game code is concerned with walking the `vischars[]` array (located at $8000 in the original game) to make the characters work.

#### Making sense of registers (“register de-allocation” or “variable creation”)
The register HL, for example, might be used multiple times within a routine with distinct meanings. These uses have to be identified and turned into individual variables. You might see multiple declarations of HL: e.g. `uint8_t *HL`, `uint16_t HLfoo` and `const uint8_t *HLbar`, within the same routine, identifying the separate uses to which the original register was put.

When a register's use is determined and its holding variable is renamed (or split) I suffix a `/* was XX */` comment to remember the register to which it was originally assigned. This is invaluable when comparing routines with the disassembly.

This happens in conjunction with PUSH and POP-ing of registers to stack, which is resolved by renaming nested variables and deleting the PUSH/POP ops.

#### Register bank switching
The Z80 has two banks of registers. HL, for example, has a counterpart register called HL’. Registers are switched between with `EX AF,AF’`, or `EXX` instructions.

For a code sequence where obvious delimiting EX/EXX instructions are present, variables between the two points were renamed to, e.g. `HLdash` and then the EX and EXX instructions were deleted.

#### Self-modifying code
Many of the original functions would poke values directly into the instruction stream. These were recognised and each one factored out into its own variable. If the self modification is only within the routine, it can become a local variable. Otherwise it has to be placed into the state structure.

#### Piecemeal structure access
Much of the code points HL to a structure member, loads it, increments HL, loads more, increments HL, and so on. This resulting set of interleaved `HL++` or `HL--` ops gets in the way of ‘actual work’. This is resolved by adjusting the (often offsetted) initial HL pointer to point to the base of the structure and using normal structure references for each access.

#### Code which never returns
The original code will squash the stack and jump directly to the main game loop in certain situations. The C standard library functions `longjmp` and `setjmp` are used to simulate this. In the converted code these were marked with `NEVER_RETURNS;` which is an informational and safety macro.

## Coding style
If editing the code you should:

* Follow the existing style of the code.
* Use spaces, not tabs. Indent by two spaces.
* Format source lines to 80 columns wide (to allow two or three side-by-side diffs).
* Order functions and data by the address of the original.
* Order #includes consistently (list in ‘depth’ order, C stdlib first, etc.).
* Name `functions_like_this`.
* Name `constants_LIKE_THIS`.
* Suffix type names with `_t`.
* Use Javadoc/Doxygen style comments throughout.
* Aim for strict C89 code (to maximise portability).
* Don’t use global variables. Place any ‘globals’ in `tgestate_t`.
* Separate logic and data. Place large data blobs in external source files.

Note that I use C++-style `//` lines for 'rough' comments and C-style '/*' blocks for statements that I'm happier about.

### Editing tips
During the conversion I usually edited the C code side-by-side in a three-pane configuration in Vim. I would have `TheGreatEscape.c` open alongside `TheGreatEscape.skool` and `TheGreatEscape.ctl`. This let me see the original Z80, the interpreted logic and the reimplemented C simultaneously.

---

# Sargasso
* I’m undecided yet whether platform drivers ought to be written as new implementations of `ZXSpectrum`, or callbacks from within it.
* Many of the game’s data structures are largely, but not entirely, const. For example, the room definitions have object indices poked into them when characters get in and out of bed, or sit down to eat.

