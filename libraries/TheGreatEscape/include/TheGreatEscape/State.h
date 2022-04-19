/**
 * State.h
 *
 * This file is part of "The Great Escape in C".
 *
 * This project recreates the 48K ZX Spectrum version of the prison escape
 * game "The Great Escape" in portable C code. It is free software provided
 * without warranty in the interests of education and software preservation.
 *
 * "The Great Escape" was created by Denton Designs and published in 1986 by
 * Ocean Software Limited.
 *
 * The original game is copyright (c) 1986 Ocean Software Ltd.
 * The original game design is copyright (c) 1986 Denton Designs Ltd.
 * The recreated version is copyright (c) 2012-2020 David Thomas
 */

#ifndef STATE_H
#define STATE_H

/* ----------------------------------------------------------------------- */

#include <setjmp.h>
#include <stddef.h>

#include "C99/Types.h"

#include "ZXSpectrum/Spectrum.h"

#include "TheGreatEscape/Types.h"
#include "TheGreatEscape/SuperTiles.h"
#include "TheGreatEscape/Tiles.h"

/* ----------------------------------------------------------------------- */

#define MASK_BUFFER_WIDTHBYTES    (4)
#define MASK_BUFFER_ROWBYTES      (MASK_BUFFER_WIDTHBYTES * 8) /* one row of UDGs */
#define MASK_BUFFER_HEIGHT        (5)
#define MASK_BUFFER_LENGTH        (MASK_BUFFER_ROWBYTES * MASK_BUFFER_HEIGHT)

#define TILE_BUF_WIDTH            (24)
#define TILE_BUF_HEIGHT           (17)
#define TILE_BUF_LENGTH           (TILE_BUF_WIDTH * TILE_BUF_HEIGHT)

#define WINDOW_BUF_WIDTH          (24 * 8)
#define WINDOW_BUF_HEIGHT         (17)
#define WINDOW_BUF_LENGTH         (WINDOW_BUF_WIDTH * WINDOW_BUF_HEIGHT)

#define MAP_BUF_WIDTH             (7)
#define MAP_BUF_HEIGHT            (5)
#define MAP_BUF_LENGTH            (MAP_BUF_WIDTH * MAP_BUF_HEIGHT)

#define MAX_ROOMDEF_OBJECT_BOUNDS (4)

// 7 == max interior mask refs (roomdef_30 uses this many)
#define MAX_INTERIOR_MASK_REFS    (7)

#define LOCKED_DOORS_LENGTH       (11)

/* ----------------------------------------------------------------------- */

/**
 * Holds the current state of the game.
 */
struct tgestate
{
  /* ------------------------------------------------------------------------
   * State variables additional to the original game.
   * --------------------------------------------------------------------- */

  /**
   * Dimensions of the ZX Spectrum screen in UDGs, e.g. 32x24.
   *
   * This is copied from the virtual ZX Spectrum as startup.
   */
  int             width, height;

  /**
   * Dimensions of the game window in UDGs, e.g. 24x17.
   */
  int             columns, rows;

  /**
   * Dimensions of the game window in supertiles, e.g. 7x5.
   *
   * This is rounded up to allow for a buffer region around the edge. e.g. 7x5
   * supertiles would occupy the area of 28x20 UDGs - which is larger than the
   * game window.
   */
  int             st_columns, st_rows;

  /**
   * Virtual ZX Spectrum hardware we're driving.
   */
  zxspectrum_t   *speccy;

  /**
   * Non-local jump buffer initialised by tge_main() then jumped to when
   * squash_stack_goto_main() is called. This happens when transition() or
   * enter_room() is called.
   */
  jmp_buf         jmpbuf_main;

  /**
   * tile_buf's length in bytes.
   */
  size_t          tile_buf_size;

  /**
   * window_buf's stride in bytes.
   */
  size_t          window_buf_stride;

  /**
   * window_buf's length in bytes.
   */
  size_t          window_buf_size;

  /**
   * map_buf's length in bytes.
   */
  size_t          map_buf_size;

  /**
   * The current vischar pointer (was register IY in the original game).
   */
  vischar_t      *IY;

  /**
   * Shadow bytes overlaid on certain room definitions by set/get_roomdef().
   */
  uint8_t         roomdef_shadow_bytes[16];


  /* ------------------------------------------------------------------------
   * State variables as per the original, ordered by memory location.
   * --------------------------------------------------------------------- */

  /**
   * $68A0: The index of the hero's current room, or room_0_OUTDOORS when
   * he's outside.
   */
  room_t          room_index;

  /**
   * $68A1: Holds the current index into doors[] and optionally a
   * door_REVERSE flag.
   *
   * Read by is_door_locked, door_handling_interior.
   */
  doorindex_t     current_door;

  /**
   * $69AE: Holds the current state of the three movable items (stoves and
   * crates).
   *
   * Used by setup_movable_items and reset_visible_character.
   */
  movableitem_t   movable_items[movable_item__LIMIT];

  /**
   * $7612: Holds the current state of every character in the game.
   *
   * Used by wake_up, end_of_breakfast, reset_map_and_characters,
   * spawn_characters and solitary.
   */
  characterstruct_t character_structs[character_structs__LIMIT];

  /**
   * $76C8: Holds the current state of every item in the game.
   *
   * Used by find_nearby_item, event_new_red_cross_parcel,
   * accept_bribe, action_red_cross_parcel, action_poison,
   * follow_suspicious_character, character_behaviour, target_reached,
   * solitary, is_item_discoverable, is_item_discoverable_interior,
   * mark_nearby_items and get_greatest_itemstruct.
   */
  itemstruct_t    item_structs[item__LIMIT];

  /**
   * $7CFC: Holds the variables related to on-screen messages.
   */
  messages_t      messages;

  /**
   * $7F00: A table of 256 bit-reversed bytes.
   *
   * Read by flip_16_masked_pixels and flip_24_masked_pixels only.
   */
  uint8_t         reversed[256];

  /**
   * $8000: Holds the current state of every visible character in the game.
   */
  vischar_t       vischars[vischars_LENGTH];

  /**
   * $8100: A buffer used when plotting to cut away foreground layers from
   * visible characters and items.
   */
  uint8_t         mask_buffer[MASK_BUFFER_LENGTH];

  /**
   * $81A2: Points to where to begin plotting in the window buffer.
   *
   * This is used by the masked sprite plotters.
   */
  uint8_t        *window_buf_pointer;

  /**
   * $81A4: A scratch pad for holding map positions.
   *
   * Used by various places in the code.
   */
  union
  {
    mappos16_t    pos16;
    mappos8_t     pos8;
  }
  saved_mappos;

  /**
   * $81AC: Points to where to start reading bitmap data from.
   *
   * This is used by the masked sprite plotters.
   */
  const uint8_t  *bitmap_pointer;

  /**
   * $81AE: Points to where to start reading mask data from.
   *
   * This is used by the masked sprite plotters.
   */
  const uint8_t  *mask_pointer;

  /**
   * $81B0: Points to where to start reading foreground mask data from.
   *
   * This is used by the masked sprite plotters.
   */
  const uint8_t  *foreground_mask_pointer;

  /**
   * $81B2: Another scratch pad for holding map positions.
   *
   * This is used by render_mask_buffer, to receive the map position of the
   * current visible character or item, and by
   * guards_follow_suspicious_character which uses it as a scratch pad.
   *
   * Written by setup_item_plotting, setup_vischar_plotting.
   * Read by render_mask_buffer, guards_follow_suspicious_character.
   */
  mappos8_t       mappos_stash;

  /**
   * $81B5: The current visible character's (or item's) screen position.
   *
   * This is in the same coordinate space as map_position.
   *
   * Written by restore_tiles, setup_item_plotting, setup_vischar_plotting.
   * Read by render_mask_buffer.
   */
  pos8_t          isopos;

  /**
   * $81B7: Used by the masked sprite plotters to flip characters left to
   * right.
   *
   * Bit 7 controls the flip. The remainder is an index, but it's never used.
   *
   * Assigned from vischar->mi.sprite_index, but only used to test the flip
   * flag.
   */
  spriteindex_t   sprite_index;

  /**
   * $81B8: The hero's map position.
   */
  mappos8_t       hero_mappos;

  /**
   * $81BB: An offset into the map used when drawing tiles.
   *
   * When the offset is (0,0) the highest, leftmost point of the map appears
   * at the top-left of the game window. Increasing the offsets will scroll
   * the map leftwards and upwards relative to the game window.
   */
  pos8_t          map_position;

  /**
   * $81BD: The current searchlight state.
   *
   * This is set to searchlight_STATE_SEARCHING (0xFF) when the hero is being
   * searched for. It's set to searchlight_STATE_CAUGHT (0x1F) when the hero
   * is caught in the searchlight. Otherwise it's set to inbetween values
   * when the hero has evaded the searchlight.
   */
  uint8_t         searchlight_state;

  /**
   * $81BE: The current room definition's dimensions.
   *
   * This is an index into roomdef_dimensions[].
   *
   * Written only by setup_room.
   */
  uint8_t         roomdef_dimensions_index;

  /**
   * $81BF: The number of bounds used by the current room definition.
   *
   * Written only by setup_room.
   */
  uint8_t         roomdef_object_bounds_count;

  /**
   * $81C0: A copy of the current room definition's bounds.
   *
   * This allows for up to four room objects.
   *
   * Written only by setup_room.
   */
  bounds_t        roomdef_object_bounds[MAX_ROOMDEF_OBJECT_BOUNDS];

  /**
   * $81D6: Holds the indices of the current room's doors.
   *
   * Written only by setup_doors.
   */
  doorindex_t     interior_doors[4];

  /**
   * $81DA: The number of masks used by the current room definition.
   *
   * Written only by setup_room.
   */
  uint8_t         interior_mask_data_count;

  /**
   * $81DB: A copy of the current room definition's masks.
   *
   * Written only by setup_room.
   */
  mask_t          interior_mask_data[MAX_INTERIOR_MASK_REFS];

  /**
   * $8214: The current item's bitmap height.
   *
   * Used by setup_item_plotting() and item_visible().
   */
  uint8_t         item_height;

  /**
   * $8215: The two items which the hero is holding.
   */
  item_t          items_held[2];

  /**
   * $8217: The index of the current character.
   */
  character_t     character_index;

  /**
   * $A12F: The game counter.
   *
   * Incremented on every call to wave_morale_flag. This is used to animate the
   * morale flag and to time the lock picking and wire cutting player lockouts.
   * Also every time the game counter hits a multiple of 64, main_loop calls
   * dispatch_timed_event which moves the event clock onwards.
   */
  gametime_t      game_counter;

  /**
   * $A130: The number of rings of the bell remaining.
   *
   * Set to 0 for perpetual ringing, or 255 to stop ringing.
   *
   * Used by various event routines.
   */
  bellring_t      bell;

  /**
   * $A132: The digits of the player's current score.
   */
  char            score_digits[5];

  /**
   * $A137: A flag (0 or 255) set if the hero is at breakfast.
   */
  uint8_t         hero_in_breakfast;

  /**
   * $A138: A flag (0 or 255) set if the hero goes out of bounds or off-route.
   *
   * This causes the hero to be followed by hostiles and disables automatic
   * behaviour.
   *
   * Writen by in_permitted_area and reset_game.
   * Read by automatics and guards_follow_suspicious_character.
   */
  uint8_t         red_flag;

  /**
   * $A139: A countdown until CPU control of the hero is assumed.
   *
   * When it becomes zero control is assumed. It's reset to 31 by any input
   * event.
   */
  uint8_t         automatic_player_counter;

  /**
   * $A13A: A flag (0 or 255) set when the hero is sent to solitary.
   *
   * If nonzero then player control is inhibited (process_player_input,
   * set_hero_route).
   *
   * Set by solitary.
   * Reset by charevnt_solitary_ends.
   */
  uint8_t         in_solitary;

  /**
   * $A13B: A flag (0 or 255) set when morale hits zero.
   *
   * If nonzero then player control is inhibited (process_player_input).
   */
  uint8_t         morale_exhausted;

  /**
   * $A13C: The current morale level (0 to 112).
   */
  uint8_t         morale;

  /**
   * $A13D: The game clock (0..139).
   *
   * Incremented once every 64 ticks of game_counter. 100+ is night time.
   */
  eventtime_t     clock;

  /**
   * $A13E: A flag (0 or 255) set when character_index is to be used for
   * character events. Zero for vischar events.
   *
   * Set to 255 only when move_a_character is entered.
   * Set to 0 in set_route, follow_suspicious_character and spawn_character.
   *
   * Causes character events to use character_index, not IY (vischar). This
   * needs to get set ahead of anything which causes a character event.
   */
  uint8_t         entered_move_a_character;

  /**
   * $A13F: A flag (0 or 255) set when the hero is in bed.
   */
  uint8_t         hero_in_bed;

  /**
   * $A140: The currently displayed morale level.
   *
   * The displayed morale lags behind actual morale since the flag moves slowly
   * towards its target.
   */
  uint8_t         displayed_morale;

  /**
   * $A141: A pointer to the screen address where the morale flag was last
   * plotted.
   */
  uint8_t        *moraleflag_screen_address;

  /**
   * $A143: A pointer to a door in locked_doors[] in which door_LOCKED is
   * cleared when picked.
   */
  doorindex_t    *ptr_to_door_being_lockpicked;

  /**
   * $A145: The game time when player control is restored when picking a lock or cutting wire.
   */
  gametime_t      player_locked_out_until;

  /**
   * $A146: A flag (0 or 255) set when it's night time.
   */
  uint8_t         day_or_night;

  /**
   * $A263: The current contents of the red cross parcel.
   */
  item_t          red_cross_parcel_current_contents;

  /**
   * $A7C6: An index used only by move_map().
   */
  uint8_t         move_map_y;

  /**
   * $A7C7: The game window plotting offset.
   */
  pos8_t          game_window_offset;

  /**
   * $AB66: Holds zoombox parameters.
   */
  zoombox_t       zoombox;

  /**
   * $AB6A: A stored copy of game screen attribute, used to draw the zoombox.
   */
  attribute_t     game_window_attribute;

  /**
   * $AD29: Holds searchlight parameters.
   */
  searchlight_t   searchlight;

  /**
   * $AF8E: The current bribed character.
   */
  character_t     bribed_character;

  /**
   * $C41A: A pseudo-random number generator index.
   */
  uint8_t         prng_index;

  /**
   * $C891: A countdown until any food item is discovered.
   */
  uint8_t         food_discovered_counter;

  /**
   * $DD69: Holds item attributes.
   */
  attribute_t     item_attributes[item__LIMIT];

  /** */
  spriteplotter_t spriteplotter;

  /**
   * $F05D: Holds the gates and doors which are initially locked.
   *
   * Each entry can have door_LOCKED set to indicate that it's locked.
   * The first five locked doors are exterior doors.
   * The doors 2..8 include interior doors.
   */
  doorindex_t     locked_doors[LOCKED_DOORS_LENGTH];

  /**
   * $F06B: Holds key definitions.
   */
  keydefs_t       keydefs;

  /* Conv: $F075: static_tiles_plot_direction was removed. */

  /**
   * $F0F8..$F28F: Holds the tile buffer.
   *
   * The tile buffer holds one tile index per 8x8 pixel area (UDG) of the
   * window buffer. Tile indices index either into interior_tiles[] or
   * exterior_tiles[]. In the case of exterior_tiles[], which needs indices
   * larger than a byte, the tile buffer's indices only make sense when
   * considered in conjunction with their respective parent supertile (held
   * in map_buf).
   *
   * Its dimensions are 24x17 = 408 total tiles in the buffer.
   *
   * Written by plot_*_tiles and expand_object.
   */
  tileindex_t    *tile_buf;

  /**
   * $F445: The chosen input device.
   *
   * Note: This gets overwritten by window_buf in the original game.
   */
  inputdevice_t   chosen_input_device;

  /**
   * $F541: Holds music channel indices.
   *
   * Note: This gets overwritten by window_buf in the original game.
   */
  uint16_t        music_channel0_index;
  uint16_t        music_channel1_index;

  /**
   * $F290..$FF4F: Holds the window buffer.
   *
   * The window buffer holds the expanded-out version of the tile_buf. It is
   * stored in linear/progressive order unlike the native ZX Spectrum screen.
   * Later it is plotted into the game window area of the native screen with
   * a possible 4-bit shift.
   *
   * Its dimensions are 24x17x8 bytes = 3,264 total bytes in the buffer.
   */
  uint8_t        *window_buf; // should this be a tilerow_t?

  /**
   * $FF58..$FF7A: Holds the map buffer.
   *
   * The map buffer holds one supertile index per 32x32 pixel area of the
   * window buffer. This tells us which tiles to place in tile_buf. Later
   * it's used to decide which offset into exterior_tiles we should use.
   *
   * Its dimensions are 7x5 = 35 total supertiles in the buffer.
   */
  supertileindex_t *map_buf;
};

/* ----------------------------------------------------------------------- */

#endif /* STATE_H */

// vim: ts=8 sts=2 sw=2 et
