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

  int             columns;    /* game window width in UDGs e.g. 24 */
  int             rows;       /* game window height in UDGs e.g. 16 */

  tileindex_t    *tile_buf;   /* replacing direct access to $F0F8 .. $F40F? */
  uint8_t        *window_buf; /* replacing direct access to $F290 .. $FE8F? game window buffer */

  ZXSpectrum_t   *speccy;

  jmp_buf         jmpbuf_main;


  /* ORIGINAL VARIABLES */

  /** $68A0: Index of the current room, or 0 when outside. */
  room_t          room_index;

  /** $68A1: Holds current door. */
  uint8_t         current_door;

  /** $7CFC: Queue of message indexes.
   * (Pairs of bytes + terminator). */
  uint8_t         message_queue[message_queue_LENGTH];
  /** $7D0F: Decrementing counter which shows next message when it reaches
   * zero. */
  uint8_t         message_display_counter;
  /** $7D10: Index into the message we're displaying or wiping. */
  uint8_t         message_display_index;
  /** $7D11: Pointer to the head of the message queue. */
  uint8_t        *message_queue_pointer;
  /** $7D13: Pointer to the next message character to be displayed. */
  const char     *current_message_character;

  /** $7F00: A table of 256 bit-reversed bytes. */
  uint8_t         reversed[256];

  /** $8000: Array of visible characters. */
  vischar_t       vischars[vischars_LENGTH];

  /** $8100: Mask buffer. */
  uint8_t         mask_buffer[160];

  /** $81A0: (unknown) */
  uint16_t        word_81A0;

  /** $81A2: Likely a screen buffer pointer. */
  uint16_t        word_81A2;

  /** $81A4: Saved position.
   * Used by various places in the code. */
  pos_t           saved_pos;

  /** $81AC: Used by masked sprite plotters. */
  uint16_t        bitmap_pointer;
  /** $81AE: Used by masked sprite plotters. */
  uint16_t        mask_pointer;
  /** $81B0: Used by masked sprite plotters. */
  uint16_t        foreground_mask_pointer;

  /** $81B2: (unknown) Used by masked sprite plotters. */
  tinypos_t       tinypos_81B2;
  
  /** $81B5: (unknown) */
  uint8_t         map_position_related_1;
  /** $81B6: (unknown) */
  uint8_t         map_position_related_2;

  /** $81B7: Controls character left/right flipping. */
  uint8_t         flip_sprite;

  /** $81B8: Player's map position. */
  tinypos_t       player_map_position;

  /** $81BB: Map position. Used when drawing tiles. */
  uint8_t         map_position[2];

  /** $81BD: Searchlight state. */
  uint8_t         searchlight_state;

  /** $81BE: Index into roomdef_bounds[]. */
  uint8_t         roomdef_bounds_index;

  /** $81BF: Copy of current room boundaries. */
  uint8_t         roomdef_object_bounds[4 * 4];

  /** $81CF: Length of next field. */
  uint8_t         roomdef_ntbd; // first byte probably count of TBD (6 == max bytes of TBD)

  /** $81D0: Room def data nature of which T.B.D. */
  uint8_t         roomdef_tbd[6]; // (6 == max bytes of TBD)

  /** $81D6: door related stuff. */
  uint8_t         door_related[4];

  /** $81DA: indoor_mask_data. */
  uint8_t         indoor_mask_data[1 + 8 * 7]; // first byte is count // likely partial array of eightbyte_t

  /** $8213: (unknown)
   * This is written to by sub_DC41() but I can find no evidence that it's
   * ever read from. */
  uint8_t         possibly_holds_an_item;

  /** $8214: Item bitmap height.
   * Used by sub_DC41() and sub_DD02(). */
  uint8_t         item_height;

  /** $8215: Items which the player is holding. */
  item_t          items_held[2];

  /** $8217: Character index. */
  character_t     character_index;

  /** $A12F: Game counter.
   * Used to wave flag, time lock picking and wire snipping lockouts. */
  uint8_t         game_counter;

  /** $A130: Bell ringing. */
  bellring_t      bell;

  /** $A132: Score digits. */
  char            score_digits[5];

  /** $A137: Player is in breakfast (flag: 0 or 255). */
  uint8_t         player_in_breakfast;

  /** $A138: Set by in_permitted_area.
   * Used by follow_suspicious_player. */
  uint8_t         red_flag;

  /** $A139: Countdown until CPU control of the player is assumed.
   * When it becomes zero, control is assumed. It's usually set to 31 by input
   * events. */
  uint8_t         automatic_player_counter;

  /** $A13A: (unknown) */
  uint8_t         morale_1;
  /** $A13B: (unknown) */
  uint8_t         morale_2;
  /** $A13C: Morale 'score'. Ranges morale_MIN..morale_MAX. */
  uint8_t         morale;

  /** $A13D: Clock. Incremented once every 64 ticks of game_counter. */
  uint8_t         clock;

  /** $A13E: (unknown) */
  uint8_t         byte_A13E;

  /** $A13F: Player is in bed (flag: 0 or 255). */
  uint8_t         player_in_bed;

  /** $A140: Displayed morale.
   * Lags behind actual morale while the flag moves towards slowly to its
   * target. */
  uint8_t         displayed_morale;

  /** $A141: Pointer to the screen address where the morale flag was last
   * plotted. */
  uint8_t        *moraleflag_screen_address;

  /** $A143: Pointer to door (in gates_and_doors[]) in which bit 7 is cleared
   * when picked. */
  uint8_t        *ptr_to_door_being_lockpicked;

  /** $A145: Game time until player control is restored.
   * e.g. when picking a lock or cutting wire. */
  uint8_t         player_locked_out_until;

  /** $A146: Day or night time (flag: 0 or 255). */
  uint8_t         day_or_night;

  /** $A263: Current contents of red cross parcel. */
  item_t          red_cross_parcel_current_contents;

  /** $A7C6: (unknown) */
  uint8_t         used_by_move_map;

  /** $A7C7: (unknown) */
  uint8_t         plot_game_screen_x;

  /** $AB66: (unknown) */
  uint8_t         zoombox_x;
  /** $AB67: (unknown) */
  uint8_t         zoombox_horizontal_count;
  /** $AB68: (unknown) */
  uint8_t         zoombox_y;
  /** $AB69: (unknown) */
  uint8_t         zoombox_vertical_count;

  /** $AB6A: Stored copy of game screen attribute, used to draw zoombox. */
  attribute_t     game_screen_attribute;

  /** $AE75: (unknown) */
  uint8_t         searchlight_related;

  /** $AE76: (unknown) */
  uint8_t         searchlight_coords[2];

  /** $AF8E: Bribed character. */
  character_t     bribed_character;

  /** $B837: (unknown) - Used by mask_stuff. */
  uint8_t         byte_B837;
  /** $B838: (unknown) - Used by mask_stuff. */
  uint8_t         byte_B838;
  /** $B839: (unknown) - Used by mask_stuff. */
  uint8_t         word_B839;

  /** $C41A: (unknown) - Indexes something. */
  uint8_t        *word_C41A;

  /** $C891: A countdown until any food item is discovered. */
  uint8_t         food_discovered_counter;

  /** $DD69: Item attributes. */
  attribute_t     item_attributes[item__LIMIT];

  /** $F05D: Gates and doors. */
  uint8_t         gates_and_doors[11];

  /** $F06B: Key definitions. */
  keydefs_t       keydefs;

  /* $F075: static_tiles_plot_direction - removed. */

  /** $F445: Indexes inputroutines[]. */
  inputdevice_t   chosen_input_device;

  /** $F541: Music channel indices. */
  uint16_t        music_channel0_index, music_channel1_index;

  /** $FF58: Map tiles. */
  maptile_t       maptiles_buf[MAPX / 4 * MAPY / 4];

  /** $EDD3: Start addresses for game screen. */
  uint16_t        game_screen_start_addresses[128];
};

#endif /* STATE_H */

