TODOs
=====

Core
----
- [ ] Fix core game bugs (see BUGS.md).
- [ ] Fix analyser issues (vars assigned to but never read).
- [x] Stop the core overwriting shared writable room defs to make things appear.

macOS
-----
- [ ] macOS UI - retina resolutions ought to be recognised/supported. [low]
- [ ] Clicky beeper audio ought to be checked out. Could just need filtering.
- [ ] Audio performance ought to be checked out. Avoid bitfifo or just optimise its storage? (e.g. runlength encode the waveform)
- [x] Ensure that the game sleeps properly. Look into macOS timing stuff.
- [ ] Perhaps replace the OpenGL front end with SpriteKit.
- [ ] TV simulator shader (greyscale, scanlines, blurring, etc.)
- [ ] Add a help page or user guide. Keymap. Include solutions.
- [ ] Real joystick support.

Windows
-------
- [ ] Windows UI - bring to parity with macOS UI.

General
-------
- [ ] Huge tidy-up.
  - [ ] astyle / clang-format the source.
- [ ] Fix all the 'FUTURE' issues.
- [ ] Acknowledge everyone's copyright everywhere: The Great Escape (C) Ocean etc. etc.

