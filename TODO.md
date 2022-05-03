TODOs
=====

Last update: 2022-05-01

Core
----
- [ ] Fix analyser issues (vars assigned to but never read).
- [ ] Remove the infinite `for(;;)` loops which will stop a multitasking cooperative RISC OS version being possible.
- [x] Fix core game bugs (see BUGS.md).
- [x] Stop the core overwriting shared writable room defs to make things appear.

macOS
-----
- [ ] Clicky beeper audio ought to be checked out. Could just need filtering.
- [ ] Audio performance ought to be checked out. Avoid bitfifo or just optimise its storage? (e.g. runlength encode the waveform)
- [ ] Perhaps replace the OpenGL front end with SpriteKit or Metal.
- [ ] Add a help page or user guide. Keymap. Include solutions.
- [ ] Real joystick support.
- [ ] Fix tiny icon in about box.
- [x] When running in Retina resolution the screen image is not pixel aligned.
- [x] Retina resolutions ought to be recognised/supported. [low]
- [x] Ensure that the game sleeps properly. Look into macOS timing stuff.
- [x] TV simulator (shader) (greyscale, scanlines, blurring, etc.)

Windows
-------
- [ ] Windows UI - bring to parity with macOS UI.

SDL
---
- [ ] Add a touch handler to make it useful on mobiles.

RISC OS
-------
- [x] Port.

General
-------
- [ ] Huge tidy-up.
  - [ ] astyle / clang-format the source.
- [ ] Fix all the 'FUTURE' issues.
- [ ] Extract the sound and graphics from an image of the original game, rather than embedding it all.
- [ ] Versioning.
- [x] Acknowledge everyone's copyright everywhere: The Great Escape (C) Ocean etc. etc.

Ideas
-----
- [ ] Made the code buildable with sdcc.

