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
  /* ADDITIONAL VARIABLES (those not in the original game) */

  int             width;      /* real screen width in UDGs e.g. 32 */
  int             height;     /* real screen height in UDGs e.g. 24 */

  int             columns;    /* game window width in UDGs e.g. 23 */
  int             rows;       /* game window height in UDGs e.g. 16 */
  
  int             tb_columns; // 24
  int             tb_rows;    // 17
  
  int             st_columns; /* supertiles columns (normally 7) */
  int             st_rows;    /* supertiles rows (normally 5) */

  ZXSpectrum_t   *speccy;

  jmp_buf         jmpbuf_main;


  /* REGISTER VARIABLES */
  vischar_t      *IY;


  /* ORIGINAL VARIABLES (ordered by original game memory location) */

  /** $68A0: Index of the current room, or 0 when outside. */
  room_t          room_index;

  /** $68A1: Holds current door. */
  door_t          current_door;

  /** $69AE: Movable items.
   *
   * Used by setup_movable_items and reset_visible_character. 
   */
  movableitem_t   movable_items[movable_item__LIMIT];

  /**
   * $7612: Character structures.
   *
   * Used by wake_up, breakfast_time, reset_map_and_characters,
   * spawn_characters, get_character_struct and solitary.
   */
  characterstruct_t character_structs[character_structs__LIMIT];

  /**
   * $76C8: Item structs.
   *
   * Used by item_to_itemstruct, find_nearby_item, event_new_red_cross_parcel,
   * accept_bribe, action_red_cross_parcel, action_poison,
   * follow_suspicious_character, character_behaviour, bribes_solitary_food,
   * solitary, is_item_discoverable, is_item_discoverable_interior,
   * mark_nearby_items and get_greatest_itemstruct.
   */
  itemstruct_t    item_structs[item__LIMIT];

  /**
   * $783A: Map locations.
   */
  xy_t            locations[78];

  struct
  {
    /** $7CFC: Queue of message indexes.
     * (Pairs of bytes + terminator). */
    uint8_t       queue[message_queue_LENGTH];
    
    /** $7D0F: Decrementing counter which shows next message when it reaches
     * zero. */
    uint8_t       display_delay;
    
    /** $7D10: Index into the message we're displaying or wiping. */
    uint8_t       display_index;
    
    /** $7D11: Pointer to the next available slot in the message queue. */
    uint8_t      *queue_pointer;
    
    /** $7D13: Pointer to the next message character to be displayed. */
    const char   *current_character;
  }
  messages;

  /** $7F00: A table of 256 bit-reversed bytes.
   * Read by flip_16_masked_pixels and flip_24_masked_pixels only. */
  uint8_t         reversed[256];

  /** $8000: Array of visible characters. */
  vischar_t       vischars[vischars_LENGTH];

#define MASK_BUFFER_WIDTH 32 // BUT is the buffer 32 wide? The draw buffer is only 24 wide!
  /** $8100: Mask buffer. */
  // TODO: Dynamically allocate.
  uint8_t         mask_buffer[5 * MASK_BUFFER_WIDTH];

  /** $81A2: Output screen pointer. Used by masked sprite plotters. */
  uint8_t        *screen_pointer;

  /** $81A4: Saved position (on map).
   * Used by various places in the code. */
  pos_t           saved_pos;

  /** $81AC: Input bitmap pointer. Used by masked sprite plotters. */
  const uint8_t  *bitmap_pointer;
  /** $81AE: Input mask pointer. Used by masked sprite plotters. */
  const uint8_t  *mask_pointer;
  /** $81B0: Input foreground mask pointer. Used by masked sprite plotters. */
  const uint8_t  *foreground_mask_pointer;

  /** $81B2: (unknown) Used by masked sprite plotters.
   * Written by setup_item_plotting, setup_vischar_plotting.
   * Read by mask_stuff, guards_follow_suspicious_character. */
  tinypos_t       tinypos_81B2;

  /** $81B5: (unknown) */
  // assigned in called_from_main_loop_3, setup_item_plotting, setup_vischar_plotting
  xy_t            map_position_related;

  /** $81B7: Used by masked sprite plotters to flip characters left/right.
   * Bit 7 controls flipping, the remainder is an index, but the index is never read.
   * Assigned from vischar->mi.sprite_index, but only used to test the flip flag. */
  spriteindex_t   sprite_index;

  /** $81B8: Hero's map position. */
  tinypos_t       hero_map_position;

  /** $81BB: Map position (on screen).
   * Used when drawing tiles. */
  // assigned in reset_outdoors
  // suspect that this is a centering value
  xy_t            map_position;

  /** $81BD: Searchlight state. Might be a counter in fact. */
  uint8_t         searchlight_state;

  /** $81BE: Index into roomdef_bounds[]. */
  uint8_t         roomdef_bounds_index;
  /** $81BF: Current room bounds count. */
  uint8_t         roomdef_object_bounds_count;
  /** $81C0: Copy of current room def's additional bounds (allows for four
   * room objects). */
  bounds_t        roomdef_object_bounds[4];

  /** $81D6: Doors. */
  door_t          doors[4];

   // 7 == max interior mask refs (roomdef_30 uses this many). hoist this elsewhere.
#define MAX_INTERIOR_MASK_REFS 7
  /** $81DA: Interior mask data count. */
  uint8_t         interior_mask_data_count;
  /** $81DB: Interior mask data values. */
  mask_t          interior_mask_data[MAX_INTERIOR_MASK_REFS];

  /** $8214: Item bitmap height.
   * Used by setup_item_plotting() and item_visible(). */
  uint8_t         item_height;

  /** $8215: Items which the hero is holding. */
  item_t          items_held[2];

  /** $8217: Character index. */
  character_t     character_index;

  /** $A12F: Game counter.
   * Used to wave flag, time lock picking and wire snipping lockouts. */
  gametime_t      game_counter;

  /** $A130: Bell ringing. */
  bellring_t      bell;

  /** $A132: Score digits. */
  char            score_digits[5];

  /** $A137: The hero is in breakfast (flag: 0 or 255). */
  uint8_t         hero_in_breakfast;

  /** $A138: Set by in_permitted_area.
   * Used by follow_suspicious_character. */
  uint8_t         red_flag;

  /** $A139: Countdown until CPU control of the hero is assumed.
   * When it becomes zero, control is assumed. It's usually set to 31 by
   * input events. */
  uint8_t         automatic_player_counter;

  /** $A13A: (unknown) -- if nonzero then player control is inhibited (process_player_input, set_hero_target_location) */
  uint8_t         morale_1;
  /** $A13B: (unknown) -- if nonzero then player control is inhibited */
  uint8_t         morale_2;
  /** $A13C: Morale 'score'. Ranges morale_MIN..morale_MAX. */
  uint8_t         morale;

  /** $A13D: Clock. Incremented once every 64 ticks of game_counter. */
  eventtime_t     clock;

  /** $A13E: (unknown) (flag: 0 or 255). */
  // set to 0xFF when move_characters is entered, and only then
  uint8_t         entered_move_characters;

  /** $A13F: The hero is in bed (flag: 0 or 255). */
  uint8_t         hero_in_bed;

  /** $A140: Displayed morale.
   * Lags behind actual morale while the flag moves towards to its target. */
  uint8_t         displayed_morale;

  /** $A141: Pointer to the screen address where the morale flag was last
   * plotted. */
  uint8_t        *moraleflag_screen_address;

  /** $A143: Pointer to a door in gates_and_doors[] in which bit 7 is cleared
   * when picked. */
  door_t         *ptr_to_door_being_lockpicked;

  /** $A145: Game time when player control is restored.
   * e.g. when picking a lock or cutting wire. */
  gametime_t      player_locked_out_until;

  /** $A146: Day or night time (flag: day is 0, night is 255). */
  uint8_t         day_or_night;

  /** $A263: Current contents of red cross parcel. */
  item_t          red_cross_parcel_current_contents;

  /** $A7C6: An index used only by move_map(). */
  uint8_t         move_map_y;

  /** $A7C7: Game window plotting offset. */
  xy_t            game_window_offset;

  /** $AB66: Zoombox parameters. */
  struct
  {
    uint8_t       x;
    uint8_t       width;
    uint8_t       y;
    uint8_t       height;
  }
  zoombox;

  /** $AB6A: Stored copy of game screen attribute, used to draw zoombox. */
  attribute_t     game_window_attribute;

  struct
  {
    /** $AD29: Searchlight movement data. */
    searchlight_state_t states[3];

    /** $AE75: (unknown) */
    uint8_t             related;

    /** $AE76: Coordinates of searchlight when hero is caught. */
    xy_t                caught_coord;
  }
  searchlight;

  /** $AF8E: Bribed character. */
  character_t     bribed_character;

  /** $C41A: Pseudo-random number generator index. */
  uint8_t         prng_index;

  /** $C891: A countdown until any food item is discovered. */
  uint8_t         food_discovered_counter;

  /** $DD69: Item attributes. */
  attribute_t     item_attributes[item__LIMIT];

  /** $E121 .. $E363: (Formerly) Self-modified locations. */
  uint8_t         self_E121; // masked_sprite_plotter_24_wide: height loop in right shift case = clipped_height & 0xFF
  uint8_t         self_E1E2; // masked_sprite_plotter_24_wide: height loop in left shift case = clipped_height & 0xFF
  uint8_t         self_E2C2; // masked_sprite_plotter_16_wide_left: height loop = clipped height & 0xFF
  uint8_t         self_E363; // masked_sprite_plotter_16_wide_right: height loop = clipped height & 0xFF

  /* Note that these adjacent chunks actually overlap in the original game 
   * but I've divided them here for clarity. */

  /** $E188 .. $E290: (Formerly) Self-modified disabled instructions. */
  uint8_t         enable_E188; // 24 case
  uint8_t         enable_E259; // 24 case
  uint8_t         enable_E199; // 24 case
  uint8_t         enable_E26A; // 24 case
  uint8_t         enable_E1AA; // 24 case
  uint8_t         enable_E27B; // 24 case
  uint8_t         enable_E1BF; // 24 case
  uint8_t         enable_E290; // 24 case

  uint8_t         enable_E319; // 16 case masked_sprite_plotter_16_wide_left
  uint8_t         enable_E32A; // 16 case masked_sprite_plotter_16_wide_left
  uint8_t         enable_E340; // 16 case masked_sprite_plotter_16_wide_left
  uint8_t         enable_E3C5; // 16 case masked_sprite_plotter_16_wide_right
  uint8_t         enable_E3D6; // 16 case masked_sprite_plotter_16_wide_right
  uint8_t         enable_E3EC; // 16 case masked_sprite_plotter_16_wide_right

  /** $EDD3: Start addresses for game screen (usually 128). */
  uint16_t       *game_window_start_offsets;

  /** $F05D: Gates and doors. */
  door_t          gates_and_doors[11];

  /** $F06B: Key definitions. */
  keydefs_t       keydefs;

  /* $F075: static_tiles_plot_direction - removed. */

  /** $F445: Indexes inputroutines[]. */
  inputdevice_t   chosen_input_device;

  /** $F541: Music channel indices. */
  uint16_t        music_channel0_index;
  uint16_t        music_channel1_index;


  /* replacing direct access to $F0F8 .. $F28F. 24 x 17. */
  tileindex_t    *tile_buf;

  /* replacing direct access to $F290 .. $FE8F. 24 x 16 x 8. */
  uint8_t        *window_buf; // tilerow_t?

  // $FE90 - 200 bytes unaccounted for

  /* replacing direct access to $FF58 .. $FF7A. 7 x 5. */
  supertileindex_t      *map_buf;
};

#endif /* STATE_H */

