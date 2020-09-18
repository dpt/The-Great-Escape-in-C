TODOs
=====

Core
----
- [x] Fix core game bugs (see BUGS.md).
- [ ] Fix analyser issues (vars assigned to but never read).
- [x] Stop the core overwriting shared writable room defs to make things appear.
- [ ] Made the code buildable with sdcc.
- [ ] Remove the infinite for(;;) loops which will stop a multitasking cooperative RISC OS version being possible.

macOS
-----
- [ ] macOS UI - retina resolutions ought to be recognised/supported. [low]
- [ ] Clicky beeper audio ought to be checked out. Could just need filtering.
- [ ] Audio performance ought to be checked out. Avoid bitfifo or just optimise its storage? (e.g. runlength encode the waveform)
- [x] Ensure that the game sleeps properly. Look into macOS timing stuff.
- [ ] Perhaps replace the OpenGL front end with SpriteKit.
- [x] TV simulator (shader) (greyscale, scanlines, blurring, etc.)
- [ ] Add a help page or user guide. Keymap. Include solutions.
- [ ] Real joystick support.
- [ ] Fix tiny icon in about box.
- [ ] When running in Retina resolution the screen image is not pixel aligned.

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
- [x] Acknowledge everyone's copyright everywhere: The Great Escape (C) Ocean etc. etc.
- [ ] Extract the sound and graphics from an image of the original game, rather than embedding it all.
- [ ] Versioning.

