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
 * The recreated version is copyright (c) 2012-2019 David Thomas
 */

#ifndef STATE_H
#define STATE_H

#include <setjmp.h>
#include <stdint.h>

#include "TheGreatEscape/Types.h"

#include "TheGreatEscape/TheGreatEscape.h"

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

  struct
  {
    /**
     * $7CFC: The queue of message indices.
     *
     * Each is a two-byte value. Terminated by a single message_QUEUE_END
     * byte (0xFF).
     */
    uint8_t       queue[message_queue_LENGTH];

    /**
     * $7D0F: A decrementing counter which shows the next message when it
     * reaches zero.
     */
    uint8_t       display_delay;

    /**
     * $7D10: The index into the message we're displaying or wiping.
     */
    uint8_t       display_index;

    /**
     * $7D11: A pointer to the next available slot in the message queue.
     */
    uint8_t      *queue_pointer;

    /**
     * $7D13: A pointer to the next message character to be displayed.
     */
    const char   *current_character;
  }
  messages;

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

#define MASK_BUFFER_WIDTHBYTES 4
#define MASK_BUFFER_ROWBYTES   (MASK_BUFFER_WIDTHBYTES * 8) /* one row of UDGs */
#define MASK_BUFFER_HEIGHT     5
#define MASK_BUFFER_LENGTH     (MASK_BUFFER_ROWBYTES * MASK_BUFFER_HEIGHT)

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
   * $81BE: The current room definition's dimensions index.
   *
   * This is an index into roomdef_dimensions[].
   *
   * Written only by setup_room.
   */
  uint8_t         roomdef_dimensions_index;

  /**
   * $81BF: The current room definition's count of bounds.
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
  bounds_t        roomdef_object_bounds[4];

  /**
   * $81D6: Indices of interior doors.
   */
  doorindex_t     interior_doors[4];

  // 7 == max interior mask refs (roomdef_30 uses this many). hoist this elsewhere.
#define MAX_INTERIOR_MASK_REFS 7

  /**
   * $81DA: Interior mask data count.
   */
  uint8_t         interior_mask_data_count;

  /**
   * $81DB: Interior mask data copies.
   */
  mask_t          interior_mask_data[MAX_INTERIOR_MASK_REFS];

  /**
   * $8214: Item bitmap height.
   *
   * Used by setup_item_plotting() and item_visible().
   */
  uint8_t         item_height;

  /**
   * $8215: Items which the hero is holding.
   */
  item_t          items_held[2];

  /**
   * $8217: Current character index.
   */
  character_t     character_index;

  /**
   * $A12F: Game counter.
   *
   * Used to wave flag, time lock picking and wire cutting lockouts.
   */
  gametime_t      game_counter;

  /**
   * $A130: Bell ringing.
   */
  bellring_t      bell;

  /**
   * $A132: Score digits.
   */
  char            score_digits[5];

  /**
   * $A137: The hero is in breakfast (flag: 0 or 255).
   */
  uint8_t         hero_in_breakfast;

  /**
   * $A138: Set by in_permitted_area.
   *
   * Used by follow_suspicious_character.
   */
  uint8_t         red_flag;

  /**
   * $A139: Countdown until CPU control of the hero is assumed.
   *
   * When it becomes zero, control is assumed. It's usually set to 31 by input
   * events.
   */
  uint8_t         automatic_player_counter;

  /**
   * $A13A: Set to 0xFF when sent to solitary, inhibiting player control. Zero
   * otherwise.
   *
   * If nonzero then player control is inhibited (process_player_input,
   * set_hero_route).
   */
  uint8_t         in_solitary;

  /**
   * $A13B: Set to 0xFF when morale hits zero, inhibiting player control. Zero
   * otherwise.
   */
  uint8_t         morale_exhausted;

  /**
   * $A13C: Morale 'score'. Ranges morale_MIN..morale_MAX.
   */
  uint8_t         morale;

  /**
   * $A13D: Clock. Incremented once every 64 ticks of game_counter.
   */
  eventtime_t     clock;

  /**
   * $A13E: Set to 0xFF when character_index is to be used for character events.
   * Zero for vischar events.
   *
   * Set to 0xFF only when move_a_character is entered.
   * Set to 0x00 in set_route, follow_suspicious_character and spawn_character.
   *
   * Causes character events to use character_index, not IY (vischar). This
   * needs to get set ahead of anything which causes a character event.
   */
  uint8_t         entered_move_a_character;

  /**
   * $A13F: The hero is in bed (flag: 0 or 255).
   */
  uint8_t         hero_in_bed;

  /**
   * $A140: Displayed morale.
   *
   * The displayed morale lags behind actual morale as the flag moves towards
   * its target.
   */
  uint8_t         displayed_morale;

  /**
   * $A141: Pointer to the screen address where the morale flag was last
   * plotted.
   */
  uint8_t        *moraleflag_screen_address;

  /**
   * $A143: Pointer to a door in locked_doors[] in which door_LOCKED is
   * cleared when picked.
   */
  doorindex_t    *ptr_to_door_being_lockpicked;

  /**
   * $A145: Game time when player control is restored.
   * e.g. when picking a lock or cutting wire.
   */
  gametime_t      player_locked_out_until;

  /**
   * $A146: Day or night time (flag: day is 0, night is 255).
   */
  uint8_t         day_or_night;

  /**
   * $A263: Current contents of red cross parcel.
   */
  item_t          red_cross_parcel_current_contents;

  /**
   * $A7C6: An index used only by move_map().
   */
  uint8_t         move_map_y;

  /**
   * $A7C7: Game window plotting offset.
   */
  pos8_t          game_window_offset;

  /**
   * $AB66: Zoombox parameters.
   */
  struct
  {
    uint8_t       x;
    uint8_t       width;
    uint8_t       y;
    uint8_t       height;
  }
  zoombox;

  /**
   * $AB6A: Stored copy of game screen attribute, used to draw zoombox.
   */
  attribute_t     game_window_attribute;

  struct
  {
    /**
     * $AD29: Searchlight movement data.
     */
    searchlight_movement_t states[3];

    /**
     * $AE76: Coordinates of searchlight when hero is caught.
     */
    pos8_t        caught_coord;
  }
  searchlight;

  /**
   * $AF8E: Bribed character.
   */
  character_t     bribed_character;

  /**
   * $C41A: Pseudo-random number generator index.
   */
  uint8_t         prng_index;

  /**
   * $C891: A countdown until any food item is discovered.
   */
  uint8_t         food_discovered_counter;

  /**
   * $DD69: Item attributes.
   */
  attribute_t     item_attributes[item__LIMIT];

  /**
   * $E121..$E363: Self-modified locations in masked sprite plotters.
   */
  uint8_t         self_E121; // masked_sprite_plotter_24_wide_vischar: height loop (in right shift case) = clipped_height & 0xFF
  uint8_t         self_E1E2; // masked_sprite_plotter_24_wide_vischar: height loop (in left shift case)  = clipped_height & 0xFF
  uint8_t         self_E2C2; // masked_sprite_plotter_16_wide_left:    height loop                       = clipped_height & 0xFF
  uint8_t         self_E363; // masked_sprite_plotter_16_wide_right:   height loop                       = clipped_height & 0xFF

  /* Note that these adjacent chunks actually overlap in the original game
   * but I've divided them here for clarity. */

  /**
   * $E188..$E3EC: Sprite clipping control.
   *
   * In the original game these were self-modified instructions.
   */
  uint8_t         enable_24_right_1; // was $E188 - 24 case, rotate right, first  clip
  uint8_t         enable_24_right_2; // was $E259 - 24 case, rotate right, second clip
  uint8_t         enable_24_right_3; // was $E199 - 24 case, rotate right, third  clip
  uint8_t         enable_24_right_4; // was $E26A - 24 case, rotate right, fourth clip

  uint8_t         enable_24_left_1;  // was $E1AA - 24 case, rotate left,  first  clip
  uint8_t         enable_24_left_2;  // was $E27B - 24 case, rotate left,  second clip
  uint8_t         enable_24_left_3;  // was $E1BF - 24 case, rotate left,  third  clip
  uint8_t         enable_24_left_4;  // was $E290 - 24 case, rotate left,  fourth clip

  uint8_t         enable_16_left_1;  // was $E319 - 16 case, rotate left,  first  clip
  uint8_t         enable_16_left_2;  // was $E32A - 16 case, rotate left,  second clip
  uint8_t         enable_16_left_3;  // was $E340 - 16 case, rotate left,  third  clip

  uint8_t         enable_16_right_1; // was $E3C5 - 16 case, rotate right, first  clip
  uint8_t         enable_16_right_2; // was $E3D6 - 16 case, rotate right, second clip
  uint8_t         enable_16_right_3; // was $E3EC - 16 case, rotate right, third  clip

  /**
   * $EDD3: Start addresses for game screen (usually 128).
   */
  uint16_t       *game_window_start_offsets;

  /**
   * $F05D: Initially locked gates and doors.
   *
   * Each entry can have door_LOCKED set to indicate that it's locked.
   * The first five locked doors are exterior doors.
   * The doors 2..8 include interior doors.
   */
  doorindex_t     locked_doors[11];

  /**
   * $F06B: Key definitions.
   */
  keydefs_t       keydefs;

  /* $F075: static_tiles_plot_direction - removed. */

  /**
   * $F445: Chosen input device.
   */
  inputdevice_t   chosen_input_device;

  /**
   * $F541: Music channel indices.
   */
  uint16_t        music_channel0_index;
  uint16_t        music_channel1_index;


  /* replacing direct access to $F0F8 .. $F28F. 24 x 17 = 408 */
  tileindex_t    *tile_buf;

  /* replacing direct access to $F290 .. $FF4F. 24 x 17 x 8 = 3,264 */
  uint8_t        *window_buf; // tilerow_t?

  // $FF50 - 8 bytes unaccounted for .. or that's plotter slack space?

  /* replacing direct access to $FF58 .. $FF7A. 7 x 5 = 35 */
  supertileindex_t *map_buf;
};

#endif /* STATE_H */

// vim: ts=8 sts=2 sw=2 et
