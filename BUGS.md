BUGS
====

Unfixed
-------
* The naughty flag activates unexpectedly when the hero is wandering. Leave the game running long/quickly enough and you'll see it.
	* I've fixed some stuff, but it's still not right.
* The guard who marches above main gate seems to spawn too late compared with the original game.
* Dogs and guards seem to be able to catch the hero though the fence. Doesn't happen in the original.
	* `collision: -> solitary` says the debug log
	* _Possibly_ fixed in 6jul17 fix... nope, still happens.
* Nondeterminism! The game does not run the same way twice. The game's as random as the original. Needs explaining.
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

Fixed
-----
* Masking code producing garbled output.
* Red cross parcel displayed in the wrong place in the room (but can be picked up in the correct place).
	* Wrong sized enum.
* Door transitions offset a bit in one direction.
* Automatic player control not working.
* No vischars moving.
  * Stoves/crates do not appear (same issue).
* White square blob on side of huts.
	* Random incorrect byte in respective tile.
* Using items without associated handlers causing a crash.
* Couldn't push stove back to its normal position.
* Have seen inconsistency in bed states.
	* Fixed in 899f5454.
* Route finding going wrong in automatic.
	* Believed fixed - C&P error.
* Characters rendering too high up when outside.
	* Fixed 15jun17.
* No patrolling characters etc. outside.
	* Fixed 15jun17.
* External door keys not working.
	* Fixed 24jun17.
* `ASSERT_ITEM_VALID(item)` firing in `item_discovered()`.
	* Fixed 25jun17: Assert in wrong position.
* macOS UI gaining Tabs menu entries + other unexpected items.
	* Mostly fixed 25jun17: Needs explicitly disabling.
* Once in solitary hero would get stuck. Commandant appears but hero doesn't follow him out.
	* Fixed 25jun17.
* Couldn't drop items outside - they vanished.
	* _Seems_ fixed 25jun17.
* `spawn_character()` would occasionally lockup.
	* _Seems_ fixed 25jun17.
* Thrown into solitary randomly sometimes and similarly _not_ thrown into solitary sometimes, e.g. when a hostile catches the hero.
	* Fixed 6jul17: Inverted sense.
* Bell ringing and morale were dropping to zero at night: `searchlight_caught()` was firing.
	* Fixed 11jul17: Incorrect conversion.
* Searchlight not appearing.
	* Fixed XXjul17: Missed ops in converted code.
* Full screen mode doesn't obey window sizing restrictions - game ends up partly off-screen.
	* Fixed XXaug17.
* NULL deref in `get_target` when standing up during breakfast.
	* Fixed XXaug17: Original game derefs `NULL`, walks into ROM.
* Sat down NPC prisoners vanish at end of breakfast.
	* Fixed 9aug17: `character_sit_sleep_common()` was assigning `route_WANDER` to `route->index`, not zero.
* There's an assert in the rendering code when an item is encountered outside.
	* Fixed 27sep17: Use 8 bytes of buffering at the end of `window_buf` as in the original.
* There's an assert in `masked_sprite_plotter_24_wide_vischar` (and others) when character rendering.
	* Fixed 27sep17: Fixed incorrect asserts.
