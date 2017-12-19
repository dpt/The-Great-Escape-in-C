BUGS
====

Unfixed
-------
Up next:

* Nondeterminism! The game does not run the same way twice. The game's as random as the original. Needs explaining.

Queue:

* The guard who marches above main gate seems to spawn too late compared with the original game.
* Dogs and guards seem to be able to catch the hero though the fence. Doesn't happen in the original.
    * `collision: -> solitary` says the debug log
    * _Possibly_ fixed in 6-Jul-17 fix... nope, still happens.
* The macOS UI game window scales to half its expected size if Cmd-Number is pressed twice in sequence. This is related somehow to 'Supports high-res backing' retina support flag.
* I've seen the hero glide off on his knees through a fence when wire snipping is complete.
* Have seen NPCs get stuck at night in hero's bedroom.
* Have seen guards and prisoners getting bunched up in doorway.
* Reset the game when a character is in the hero's bedroom - character is not reset. Original bug perhaps.
* Address Sanitiser reports overwrites walking past end of character defs when plotting.
* Address Sanitiser reports Global buffer overflow in render_mask_buffer.
    e.g. 0 bytes to the right of global variable 'interior_mask_21'
* Thread Sanitiser reports multiple problems. Needs locking in various places.
* Guards & commandant don't always catch the hero on contact.
* Enter 'N' to cancel key defs repeatedly and stamp_handler will assert.

Fixed
-----
1. Masking code producing garbled output.
2. Red cross parcel displayed in the wrong place in the room (but can be picked up in the correct place).
    * Wrong sized enum.
3. Door transitions offset a bit in one direction.
4. Automatic player control not working.
5. No vischars moving.
    * Stoves/crates do not appear (same issue).
6. White square blob on side of huts.
    * Random incorrect byte in respective tile.
7. Using items without associated handlers causing a crash.
8. Couldn't push stove back to its normal position.
9. Have seen inconsistency in bed states.
    * Fixed in 899f5454.
10. Route finding going wrong in automatic.
    * Believed fixed - C&P error.
11. Characters rendering too high up when outside.
    * Fixed 15-Jun-17.
12. No patrolling characters etc. outside.
    * Fixed 15-Jun-17.
13. External door keys not working.
    * Fixed 24-Jun-17.
14. `ASSERT_ITEM_VALID(item)` firing in `item_discovered()`.
    * Fixed 25-Jun-17: Assert in wrong position.
15. macOS UI gaining Tabs menu entries + other unexpected items.
    * Mostly fixed 25-Jun-17: Needs explicitly disabling.
16. Once in solitary hero would get stuck. Commandant appears but hero doesn't follow him out.
    * Fixed 25-Jun-17.
17. Couldn't drop items outside - they vanished.
    * _Seems_ fixed 25-Jun-17.
18. `spawn_character()` would occasionally lockup.
    * _Seems_ fixed 25-Jun-17.
19. Thrown into solitary randomly sometimes and similarly _not_ thrown into solitary sometimes, e.g. when a hostile catches the hero.
    * Fixed 6-Jul-17: Inverted sense.
20. Bell ringing and morale were dropping to zero at night: `searchlight_caught()` was firing.
    * Fixed 11-Jul-17: Incorrect conversion.
21. Searchlight not appearing.
    * Fixed XX-Jul-17: Missed ops in converted code.
22. Full screen mode doesn't obey window sizing restrictions - game ends up partly off-screen.
    * Fixed XX-Aug-17.
23. NULL deref in `get_target` when standing up during breakfast.
    * Fixed XX-Aug-17: Original game derefs `NULL`, walks into ROM.
24. Sat down NPC prisoners vanish at end of breakfast.
    * Fixed 9-Aug-17: `character_sit_sleep_common()` was assigning `route_WANDER` to `route->index`, not zero.
25. There's an assert in the rendering code when an item is encountered outside.
    * Fixed 27-Sep-17: Use 8 bytes of buffering at the end of `window_buf` as in the original.
26. There's an assert in `masked_sprite_plotter_24_wide_vischar` (and others) when character rendering.
    * Fixed 27-Sep-17: Fixed incorrect asserts.
27. The red flag activates unexpectedly when the hero is wandering. Leave the game running long or quickly enough and you'll see it.
    * Fixed 19-Dec-19: There was stray code in `in_permitted_area` in the original game which, while it remained side effect-free in the original, when converted it ended up meddling with route.step. In the most obvious instance this pushed route.step over from 15 to 16 when the hero was wandering which made later code decide that he should have been in the exercise yard. The flag would turn red, the hero would halt his automatic perambulations and promptly get thrown into solitary.
    ```
     $9F77 LD C,(HL)     ; load route.step
     $9F78 BIT 7,A       ; if (route.index & route_REVERSED) C++;
     $9F7A JR Z,$9F7D    ;
     $9F7C INC C         ; C never gets used again
    ```

