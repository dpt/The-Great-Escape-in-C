/* TheGreatEscape.c
 *
 * Reimplementation of The Great Escape by Denton Designs.
 *
 * This reimplementation copyright (c) David Thomas, 2012-2013.
 * <dave@davespace.co.uk>.
 */

#include <stdint.h>
#include <string.h>

/* ----------------------------------------------------------------------- */

#define UNKNOWN 1
#define NELEMS(a) ((int) (sizeof(a) / sizeof(a[0])))

/* ----------------------------------------------------------------------- */

// ENUMERATIONS
//

/**
 * Identifiers of game items.
 */
enum item
{
  item_WIRESNIPS,
  item_SHOVEL,
  item_LOCKPICK,
  item_PAPERS,
  item_TORCH,
  item_BRIBE,
  item_UNIFORM,
  item_FOOD,
  item_POISON,
  item_RED_KEY,
  item_YELLOW_KEY,
  item_GREEN_KEY,
  item_RED_CROSS_PARCEL,
  item_RADIO,
  item_PURSE,
  item_COMPASS,
  item__LIMIT,
  item_NONE = 255
};

/**
 * Identifiers of game messages.
 */
enum message
{
  message_MISSED_ROLL_CALL,
  message_TIME_TO_WAKE_UP,
  message_BREAKFAST_TIME,
  message_EXERCISE_TIME,
  message_TIME_FOR_BED,
  message_THE_DOOR_IS_LOCKED,
  message_IT_IS_OPEN,
  message_INCORRECT_KEY,
  message_ROLL_CALL,
  message_RED_CROSS_PARCEL,
  message_PICKING_THE_LOCK,
  message_CUTTING_THE_WIRE,
  message_YOU_OPEN_THE_BOX,
  message_YOU_ARE_IN_SOLITARY,
  message_WAIT_FOR_RELEASE,
  message_MORALE_IS_ZERO,
  message_ITEM_DISCOVERED,
  message_HE_TAKES_THE_BRIBE,
  message_AND_ACTS_AS_DECOY,
  message_ANOTHER_DAY_DAWNS,
  message__LIMIT,
  message_NONE = 255
};

/**
 * Identifiers of objects used to build interiors.
 */
enum interior_object
{
  interiorobject_TUNNEL_0,
  interiorobject_SMALL_TUNNEL_ENTRANCE,
  interiorobject_ROOM_OUTLINE_2,
  interiorobject_TUNNEL_3,
  interiorobject_TUNNEL_JOIN_4,
  interiorobject_PRISONER_SAT_DOWN_MID_TABLE,
  interiorobject_TUNNEL_CORNER_6,
  interiorobject_TUNNEL_7,
  interiorobject_WIDE_WINDOW,
  interiorobject_EMPTY_BED,
  interiorobject_SHORT_WARDROBE,
  interiorobject_CHEST_OF_DRAWERS,
  interiorobject_TUNNEL_12,
  interiorobject_EMPTY_BENCH,
  interiorobject_TUNNEL_14,
  interiorobject_DOOR_FRAME_15,
  interiorobject_DOOR_FRAME_16,
  interiorobject_TUNNEL_17,
  interiorobject_TUNNEL_18,
  interiorobject_PRISONER_SAT_DOWN_END_TABLE,
  interiorobject_COLLAPSED_TUNNEL,
  interiorobject_ROOM_OUTLINE_21,
  interiorobject_CHAIR_POINTING_BOTTOM_RIGHT,
  interiorobject_OCCUPIED_BED,
  interiorobject_WARDROBE_WITH_KNOCKERS,
  interiorobject_CHAIR_POINTING_BOTTOM_LEFT,
  interiorobject_CUPBOARD,
  interiorobject_ROOM_OUTLINE_27,
  interiorobject_TABLE_1,
  interiorobject_TABLE_2,
  interiorobject_STOVE_PIPE,
  interiorobject_STUFF_31,
  interiorobject_TALL_WARDROBE,
  interiorobject_SMALL_SHELF,
  interiorobject_SMALL_CRATE,
  interiorobject_SMALL_WINDOW,
  interiorobject_DOOR_FRAME_36,
  interiorobject_NOTICEBOARD,
  interiorobject_DOOR_FRAME_38,
  interiorobject_DOOR_FRAME_39,
  interiorobject_DOOR_FRAME_40,
  interiorobject_ROOM_OUTLINE_41,
  interiorobject_CUPBOARD_42,
  interiorobject_MESS_BENCH,
  interiorobject_MESS_TABLE,
  interiorobject_MESS_BENCH_SHORT,
  interiorobject_ROOM_OUTLINE_46,
  interiorobject_ROOM_OUTLINE_47,
  interiorobject_TINY_TABLE,
  interiorobject_TINY_DRAWERS,
  interiorobject_DRAWERS_50,
  interiorobject_DESK,
  interiorobject_SINK,
  interiorobject_KEY_RACK,
  interiorobject__LIMIT
};

/**
 * Identifiers of tiles used to draw interior objects.
 */
enum interior_object_tile
{
  interiorobjecttile_MAX = 194,
  interiorobjecttile_ESCAPE = 255
};

/* ----------------------------------------------------------------------- */

// CONSTANTS
//

#define screen_text_start_address   ((uint16_t) 0x50E0)

#define visible_tiles_start_address ((uint16_t) 0xF0F8)
#define visible_tiles_end_address   ((uint16_t) 0xF28F) /* inclusive */
#define visible_tiles_length        (24 * 17)

#define screen_buffer_start_address ((uint16_t) 0xF290)
#define screen_buffer_end_address   ((uint16_t) 0xFF4F) /* inclusive */
#define screen_buffer_length        (24 * 8 * 17)

/* ----------------------------------------------------------------------- */

// TYPES
//

/**
 * The current state of the game.
 */
typedef struct tgestate tgestate_t;

/**
 * A game object.
 */
typedef struct tgeobject tgeobject_t;

/**
 * A game message.
 */
typedef enum message message_t;

/**
 * An object used to build interiors.
 */
typedef enum interior_object object_t;

/**
 * A tile used to draw interior objects.
 */
typedef enum interior_object_tile objecttile_t;

/**
 * A tile index.
 */
typedef uint8_t tileindex_t;

/**
 * An 8-pixel wide row within a tile.
 */
typedef uint8_t tilerow_t;

/**
 * An 8-by-8 tile.
 */
typedef struct { tilerow_t row[8]; } tile_t;

/**
 * A game item.
 */
typedef enum item item_t;

/**
 * A boundary such as a wall or fence.
 */
typedef struct wall
{
  uint8_t a, b;
  uint8_t c, d;
  uint8_t e, f;
}
wall_t;

/* ----------------------------------------------------------------------- */

// EXTERNS
//

extern const tgeobject_t *interior_object_tile_refs[interiorobject__LIMIT];

extern const tile_t interior_tiles[interiorobjecttile_MAX];

extern const uint8_t *game_screen_scanline_start_addresses[128]; // CHECK

/* ----------------------------------------------------------------------- */

// STATIC CONSTS
//

/**
 * $A69E: Bitmap font definition.
 */
static const tile_t bitmap_font[] =
{
  { { 0x00, 0x7C, 0xFE, 0xEE, 0xEE, 0xEE, 0xFE, 0x7C } }, // 0 or O
  { { 0x00, 0x1E, 0x3E, 0x6E, 0x0E, 0x0E, 0x0E, 0x0E } }, // 1
  { { 0x00, 0x7C, 0xFE, 0xCE, 0x1C, 0x70, 0xFE, 0xFE } }, // 2
  { { 0x00, 0xFC, 0xFE, 0x0E, 0x3C, 0x0E, 0xFE, 0xFC } }, // 3
  { { 0x00, 0x0E, 0x1E, 0x3E, 0x6E, 0xFE, 0x0E, 0x0E } }, // 4
  { { 0x00, 0xFC, 0xC0, 0xFC, 0x7E, 0x0E, 0xFE, 0xFC } }, // 5
  { { 0x00, 0x38, 0x60, 0xFC, 0xFE, 0xC6, 0xFE, 0x7C } }, // 6
  { { 0x00, 0xFE, 0x0E, 0x0E, 0x1C, 0x1C, 0x38, 0x38 } }, // 7
  { { 0x00, 0x7C, 0xEE, 0xEE, 0x7C, 0xEE, 0xEE, 0x7C } }, // 8
  { { 0x00, 0x7C, 0xFE, 0xC6, 0xFE, 0x7E, 0x0C, 0x38 } }, // 9
  { { 0x00, 0x38, 0x7C, 0x7C, 0xEE, 0xEE, 0xFE, 0xEE } }, // A
  { { 0x00, 0xFC, 0xEE, 0xEE, 0xFC, 0xEE, 0xEE, 0xFC } }, // B
  { { 0x00, 0x1E, 0x7E, 0xFE, 0xF0, 0xFE, 0x7E, 0x1E } }, // C
  { { 0x00, 0xF0, 0xFC, 0xEE, 0xEE, 0xEE, 0xFC, 0xF0 } }, // D
  { { 0x00, 0xFE, 0xFE, 0xE0, 0xFE, 0xE0, 0xFE, 0xFE } }, // E
  { { 0x00, 0xFE, 0xFE, 0xE0, 0xFC, 0xE0, 0xE0, 0xE0 } }, // F
  { { 0x00, 0x1E, 0x7E, 0xF0, 0xEE, 0xF2, 0x7E, 0x1E } }, // G
  { { 0x00, 0xEE, 0xEE, 0xEE, 0xFE, 0xEE, 0xEE, 0xEE } }, // H
  { { 0x00, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38 } }, // I
  { { 0x00, 0xFE, 0x38, 0x38, 0x38, 0x38, 0xF8, 0xF0 } }, // J
  { { 0x00, 0xEE, 0xEE, 0xFC, 0xF8, 0xFC, 0xEE, 0xEE } }, // K
  { { 0x00, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xFE, 0xFE } }, // L
  { { 0x00, 0x6C, 0xFE, 0xFE, 0xD6, 0xD6, 0xC6, 0xC6 } }, // M
  { { 0x00, 0xE6, 0xF6, 0xFE, 0xFE, 0xEE, 0xE6, 0xE6 } }, // N
  { { 0x00, 0xFC, 0xEE, 0xEE, 0xEE, 0xFC, 0xE0, 0xE0 } }, // P
  { { 0x00, 0x7C, 0xFE, 0xEE, 0xEE, 0xEE, 0xFC, 0x7E } }, // Q
  { { 0x00, 0xFC, 0xEE, 0xEE, 0xFC, 0xF8, 0xEC, 0xEE } }, // R
  { { 0x00, 0x7E, 0xFE, 0xF0, 0x7C, 0x1E, 0xFE, 0xFC } }, // S
  { { 0x00, 0xFE, 0xFE, 0x38, 0x38, 0x38, 0x38, 0x38 } }, // T
  { { 0x00, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xFE, 0x7C } }, // U
  { { 0x00, 0xEE, 0xEE, 0xEE, 0xEE, 0x6C, 0x7C, 0x38 } }, // V
  { { 0x00, 0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6 } }, // W
  { { 0x00, 0xC6, 0xEE, 0x7C, 0x38, 0x7C, 0xEE, 0xC6 } }, // X
  { { 0x00, 0xC6, 0xEE, 0x7C, 0x38, 0x38, 0x38, 0x38 } }, // Y
  { { 0x00, 0xFE, 0xFE, 0x0E, 0x38, 0xE0, 0xFE, 0xFE } }, // Z
  { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // SPACE
  { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30 } }, // FULL STOP
  { { 0x55, 0xCC, 0x55, 0xCC, 0x55, 0xCC, 0x55, 0xCC } }, // UNKNOWN (added)
};

/**
 * A table which maps from ASCII into the in-game glyph encoding.
 */
static const unsigned char ascii_to_font[256] =
{
#define _ 38 // UNKNOWN
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
 36,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _, 37,  _,
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  _,  _,  _,  _,  _,  _,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,  0,
 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
#undef _
};

/* ----------------------------------------------------------------------- */

// FORWARD REFERENCES
//

uint8_t *plot_single_glyph(int A, uint8_t *output);
void wipe_message(tgestate_t *state);
void next_message(tgestate_t *state);

static const char *messages_table[message__LIMIT];

int item_to_bitmask(item_t item);

static const wall_t walls[];

/* ----------------------------------------------------------------------- */

// STRUCTURES
//

/**
 * The current state of the game.
 */
struct tgestate
{
  // ADDITIONAL VARIABLES
  int          columns;    // e.g. 24
  int          rows;       // e.g. 16

  uint8_t     *screen_buf;
  tileindex_t *tile_buf;

  // EXISTING VARIABLES
  uint8_t      indoor_room_index;           // $68A0
  uint8_t      current_door;                // $68A1

  uint8_t      message_queue[19];           // $7CFC
  uint8_t      message_display_counter;     // $7D0F
  uint8_t      message_display_index;       // $7D10
  uint8_t     *message_queue_pointer;       // $7D11
  uint8_t     *current_message_character;   // $7D13

  uint16_t     word_81A4;                   // $81A4
  uint16_t     word_81A6;                   // $81A6
  uint16_t     word_81A8;                   // $81A8

  uint8_t      gates_and_doors[9];          // $F05D


  uint8_t      RAM[65536];
};

/**
 * A game object.
 */
struct tgeobject
{
  uint8_t      width, height;
  uint8_t      data[UNKNOWN];
};

/* ----------------------------------------------------------------------- */

/**
 * $6AB5: Expands an object.
 *
 * \param[in]  state  Game state.
 * \param[in]  index  Object index to expand.
 * \param[out] output Screen location to expand to.
 *
 * \return Nothing.
 */
void expand_object(tgestate_t *state, object_t index, uint8_t *output)
{
  int                columns;
  const tgeobject_t *obj;
  int                width, height;
  int                saved_width;
  const uint8_t     *data;
  int                byte;
  int                val;

  columns     = state->columns;

  obj         = interior_object_tile_refs[index];

  width       = obj->width;
  height      = obj->height;

  saved_width = width;

  data        = &obj->data[0];

  do
  {
    do
    {
expand:
      byte = *data;
      if (byte == interiorobjecttile_ESCAPE)
      {
        byte = *++data;
        if (byte != interiorobjecttile_ESCAPE)
        {
          if (byte >= 128)
            goto run;
          if (byte >= 64)
            goto range;
        }
      }

      if (byte)
        *output = byte;
      data++;
      output++;
    }
    while (--width);

    width = saved_width;
    output += columns - width;
  }
  while (--height);

  return;


run:

  byte = *data++ & 0x7F;
  val = *data;
  do
  {
    if (val > 0)
      *output = val;
    output++;

    if (--width == 0) // ran out of width
    {
      if (--height == 0)
        return;

      output += columns - saved_width; // move to next row
    }
  }
  while (--byte);

  data++;

  goto expand;


range:

  byte = *data++ & 0x0F;
  val = *data;
  do
  {
    *output++ = val++;

    if (--width == 0) // ran out of width
    {
      if (--height == 0)
        return;

      output += columns - saved_width; // move to next row
    }
  }
  while (--byte);

  data++;

  goto expand;
}

/* ----------------------------------------------------------------------- */

/**
 * $6B42: Plot indoor tiles.
 *
 * \param[in] state Game state.
 *
 * \return Nothing.
 */
void plot_indoor_tiles(tgestate_t *state)
{
  int                  rows;
  int                  columns;
  uint8_t             *screen_buf;
  const tileindex_t   *tiles_buf;
  int                  rowcounter;
  int                  columncounter;
  int                  bytes;
  int                  stride;

  rows       = state->rows;
  columns    = state->columns;

  screen_buf = state->screen_buf;
  tiles_buf  = state->tile_buf;

  rowcounter = rows;
  do
  {
    columncounter = columns;
    do
    {
      const tilerow_t *tile_data;

      tile_data = &interior_tiles[*tiles_buf++].row[0];

      bytes  = 8;
      stride = columns;
      do
      {
        *screen_buf = *tile_data++;
        screen_buf += stride;
      }
      while (--bytes);

      screen_buf++; // move to next character position
    }
    while (--columncounter);
    screen_buf += 7 * columns; // move to next row
  }
  while (--rowcounter);
}

/* ----------------------------------------------------------------------- */

/**
 * $7CE9: Given a screen address, return the same position on the next
 * scanline.
 *
 * \param[in] HL Scanline number.
 *
 * \return Subsequent scanline number.
 */
uint16_t get_next_scanline(uint16_t HL) /* stateless */
{
  uint16_t DE;

  HL += 0x0100;
  if (HL & 0x0700)
    return HL; /* line count didn't rollover */

  if ((HL & 0xFF) >= 0xE0)
    DE = 0xFF20;
  else
    DE = 0xF820;

  return HL + DE; /* needs to be a 16-bit add! */
}

/* ----------------------------------------------------------------------- */

/**
 * $7D15: Add a message to the display queue.
 *
 * Conversion note: The original code accepts BC as the message index.
 * However all-but-one of the callers only setup B, not BC. We therefore
 * ignore the second argument here.
 *
 * \param[in] state         Game state.
 * \param[in] message_index message_t to display.
 *
 * \return Nothing.
 */
void queue_message_for_display(tgestate_t *state,
                               message_t   message_index)
{
  uint8_t *HL;

  HL = state->message_queue_pointer; // insertion point pointer?
  if (*HL == message_NONE)
    return;

  /* Are we already about to show this message? */
  HL -= 2;
  if (*HL++ == message_index && *HL++ == 0)
    return;

  /* Add it to the queue. */
  *HL++ = message_index;
  *HL++ = 0;
  *state->message_queue_pointer = HL;
}

/* ----------------------------------------------------------------------- */

/**
 * $7D2F: Indirectly plot a glyph.
 *
 * \param[in] pcharacter Pointer to character to plot.
 * \param[in] output     Where to plot.
 *
 * \return Pointer to next character along.
 */
uint8_t *plot_glyph(const char *pcharacter, uint8_t *output)
{
  return plot_single_glyph(*pcharacter, output);
}

/**
 * $????: Plot a single glyph.
 *
 * \param[in] character Character to plot.
 * \param[in] output    Where to plot.
 *
 * \return Pointer to next character along.
 */
uint8_t *plot_single_glyph(int character, uint8_t *output)
{
  const tile_t    *glyph;
  const tilerow_t *row;
  int              iters;

  glyph = &bitmap_font[ascii_to_font[character]];
  row   = &glyph->row[0];
  iters = 8; // 8 iterations
  do
  {
    *output = *row++;
    output += 256; // next row
  }
  while (--iters);

  return ++output; // return the next character position
}

/* ----------------------------------------------------------------------- */

/**
 * $7D48: Incrementally display queued game messages.
 *
 * \param[in] state Game state.
 *
 * \return Nothing.
 */
void message_display(tgestate_t *state)
{
  uint8_t        A;
  const uint8_t *HL;
  uint8_t       *DE;

  if (state->message_display_counter > 0)
  {
    state->message_display_counter--;
    return;
  }

  A = state->message_display_index;
  if (A == 128)
  {
    next_message(state);
  }
  else if (A > 128)
  {
    wipe_message(state);
  }
  else
  {
    HL = state->current_message_character;
    DE = screen_text_start_address + A;
    plot_glyph(HL, DE);
    state->message_display_index = (intptr_t) DE & 31;
    A = *++HL;
    if (A == 255) // end of string
    {
      state->message_display_counter = 31; // leave the message for 31 turns
      state->message_display_index |= 128; // then wipe it
    }
    else
    {
      state->current_message_character = HL;
    }
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $7D87: Incrementally wipe away any on-screen game message.
 *
 * \param[in] state Game state.
 *
 * \return Nothing.
 */
void wipe_message(tgestate_t *state)
{
  int      index;
  uint8_t *DE;

  index = state->message_display_index;
  state->message_display_index = --index;
  DE = screen_text_start_address + index;
  plot_single_glyph(' ', DE); // plot a SPACE character
}

/* ----------------------------------------------------------------------- */

/**
 * $7D99: Change to displaying the next queued game message.
 *
 * \param[in] state Game state.
 *
 * \return Nothing.
 */
void next_message(tgestate_t *state)
{
  uint8_t    *DE;
  const char *message;

  DE = &state->message_queue[0];
  if (state->message_queue_pointer == DE)
    return;

  // message id is stored in the buffer itself

  message = messages_table[*DE];

  state->current_message_character = (uint8_t *) message; // should i cast away const?
  memmove(state->message_queue, state->message_queue + 2, 16);
  state->message_queue_pointer -= 2;
  state->message_display_index = 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $7DCD: Table of game messages.
 */
static const char *messages_table[message__LIMIT] =
{
  "MISSED ROLL CALL",
  "TIME TO WAKE UP",
  "BREAKFAST TIME",
  "EXERCISE TIME",
  "TIME FOR BED",
  "THE DOOR IS LOCKED",
  "IT IS OPEN",
  "INCORRECT KEY",
  "ROLL CALL",
  "RED CROSS PARCEL",
  "PICKING THE LOCK",
  "CUTTING THE WIRE",
  "YOU OPEN THE BOX",
  "YOU ARE IN SOLITARY",
  "WAIT FOR RELEASE",
  "MORALE IS ZERO",
  "ITEM DISCOVERED",
};

/* ----------------------------------------------------------------------- */

/**
 * $A59C: Return bitmask indicating the presence of required items.
 *
 * \param[in] pitem    Pointer to item.
 * \param[in] previous Previous bitfield.
 *
 * \return Sum of bitmasks.
 */
int have_required_items(const item_t *pitem, int previous)
{
  return item_to_bitmask(*pitem) + previous;
}

/**
 * $A5A3: Return a bitmask indicating the presence of required items.
 *
 * \param[in] item Item.
 *
 * \return COMPASS, PAPERS, PURSE, UNIFORM => 1, 2, 4, 8.
 */
int item_to_bitmask(item_t item)
{
  switch (item)
  {
    case item_COMPASS: return 1;
    case item_PAPERS:  return 2;
    case item_PURSE:   return 4;
    case item_UNIFORM: return 8;
    default: return 0;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $A5BF: Plot a screenlocstring.
 *
 * \param[in] state Game state.
 * \param[in] HL    Pointer to screenlocstring.
 *
 * \return Nothing.
 */
void screenlocstring_plot(tgestate_t *state, uint8_t *HL)
{
  uint8_t *screenaddr;
  int      nbytes;

  screenaddr = state->screen_buf + HL[0] + (HL[1] << 8);
  nbytes     = HL[2];
  HL += 3;
  do
    screenaddr = plot_glyph((const char *) HL++, screenaddr);
  while (--nbytes);
}

/* ----------------------------------------------------------------------- */

/**
 * $B14C: Check the character is inside of bounds.
 *
 * \param[in] state Game state.
 *
 * \return Nothing.
 */
int bounds_check(tgestate_t *state)
{
  uint8_t       B;
  const wall_t *wall;

  if (state->indoor_room_index)
    return indoor_bounds_check(state);

  B = 24; // nwalls (count of walls + fences)
  wall = &walls[0];
  do
  {
    uint16_t min0, max0, min1, max1, min2, max2;

    min0 = wall->a * 8;
    max0 = wall->b * 8;
    min1 = wall->c * 8;
    max1 = wall->d * 8;
    min2 = wall->e * 8;
    max2 = wall->f * 8;

    if (state->word_81A4 >= min0 + 2 &&
        state->word_81A4 <  max0 + 4 &&
        state->word_81A6 >= min1     &&
        state->word_81A6 <  max1 + 4 &&
        state->word_81A8 >= min2     &&
        state->word_81A8 <  max2 + 2)
    {
#if 0
      IY[7] ^= 0x20;
#endif
      return 1; // NZ
    }

    wall++;
  }
  while (--B);
  return 0; // Z
}

/* ----------------------------------------------------------------------- */

#define door_FLAG_LOCKED (1 << 7)

/**
 * $B1D4: Locate current door and queue message if it's locked.
 *
 * \param[in] state Game state.
 *
 * \return 0 => open, 1 => locked.
 */
int is_door_open(tgestate_t *state)
{
  int      mask;
  int      cur;
  uint8_t *door;
  int      iters;

  mask  = 0xFF & ~door_FLAG_LOCKED;
  cur   = state->current_door & mask;
  door  = &state->gates_and_doors[0];
  iters = 9; // NELEMS(gates_and_doors);
  do
  {
    if ((*door & mask) == cur)
    {
      if ((*door & door_FLAG_LOCKED) == 0)
        return 0; // open

      queue_message_for_display(state, message_THE_DOOR_IS_LOCKED);
      return 1; // locked
    }
    door++;
  }
  while (--iters);

  return 0; // open
}

/* ----------------------------------------------------------------------- */

/**
 * $B53E: Walls.
 * $B586: Fences
 */
static const wall_t walls[] =
{
  { 0x6A, 0x6E, 0x52, 0x62, 0x00, 0x0B },
  { 0x5E, 0x62, 0x52, 0x62, 0x00, 0x0B },
  { 0x52, 0x56, 0x52, 0x62, 0x00, 0x0B },
  { 0x3E, 0x5A, 0x6A, 0x80, 0x00, 0x30 },
  { 0x34, 0x80, 0x72, 0x80, 0x00, 0x30 },
  { 0x7E, 0x98, 0x5E, 0x80, 0x00, 0x30 },
  { 0x82, 0x98, 0x5A, 0x80, 0x00, 0x30 },
  { 0x86, 0x8C, 0x46, 0x80, 0x00, 0x0A },
  { 0x82, 0x86, 0x46, 0x4A, 0x00, 0x12 },
  { 0x6E, 0x82, 0x46, 0x47, 0x00, 0x0A },
  { 0x6D, 0x6F, 0x45, 0x49, 0x00, 0x12 },
  { 0x67, 0x69, 0x45, 0x49, 0x00, 0x12 },
  { 0x46, 0x46, 0x46, 0x6A, 0x00, 0x08 },
  { 0x3E, 0x3E, 0x3E, 0x6A, 0x00, 0x08 },
  { 0x4E, 0x4E, 0x2E, 0x3E, 0x00, 0x08 },
  { 0x68, 0x68, 0x2E, 0x45, 0x00, 0x08 },
  { 0x3E, 0x68, 0x3E, 0x3E, 0x00, 0x08 },
  { 0x4E, 0x68, 0x2E, 0x2E, 0x00, 0x08 },
  { 0x46, 0x67, 0x46, 0x46, 0x00, 0x08 },
  { 0x68, 0x6A, 0x38, 0x3A, 0x00, 0x08 },
  { 0x4E, 0x50, 0x2E, 0x30, 0x00, 0x08 },
  { 0x46, 0x48, 0x46, 0x48, 0x00, 0x08 },
  { 0x46, 0x48, 0x5E, 0x60, 0x00, 0x08 },
  { 0x69, 0x6D, 0x46, 0x49, 0x00, 0x08 },
};

/* ----------------------------------------------------------------------- */

#if 0

#define SCREEN_BUFFER_BASE 0xF291

/**
 * $EED3: Plot the game screen.
 *
 * \param[in] state Game state.
 *
 * \return Nothing.
 */
void plot_game_screen(tgestate_t *state)
{
  uint8_t  A;
  uint8_t *SP;

  A = *(&plot_game_screen_x + 1); // meh
  if (A)
    goto unaligned;

  HL = SCREEN_BUFFER_BASE + plot_game_screen_x;
  SP = game_screen_scanline_start_addresses;
  A  = 128;
  do
  {
    DE = *SP++;
    *DE = *HL++; // 23x
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    *DE = *HL++;
    HL++; // skip 24th
  }
  while (--A);

  return;


unaligned:

  HL = SCREEN_BUFFER_BASE + plot_game_screen_x;
  A  = *HL++;
  SP = game_screen_scanline_start_addresses;
  Bdash = 128; // 128 iterations
  do
  {
    DE = *SP++;

    // RRD:
    // T = A & 0x0F;
    // A = (*HL & 0x0F) | (A & 0xF0);
    // *HL = (*HL >> 4) | (T << 4);

    B = 4; // 4 iterations
    do
    {
      C = *HL;
      RRD;
      Adash = *HL;
      *DE++ = Adash;
      *HL++ = C;

      C = *HL;
      RRD;
      Adash = *HL;
      *DE++ = Adash;
      *HL++ = C;

      C = *HL;
      RRD;
      Adash = *HL;
      *DE++ = Adash;
      *HL++ = C;

      C = *HL;
      RRD;
      Adash = *HL;
      *DE++ = Adash;
      *HL++ = C;

      C = *HL;
      RRD;
      Adash = *HL;
      *DE++ = Adash;
      *HL++ = C;
    }
    while (--B);

    C = *HL;
    RRD;
    Adash = *HL;
    *DE++ = Adash;
    *HL++ = C;

    C = *HL;
    RRD;
    Adash = *HL;
    *DE++ = Adash;
    *HL++ = C;

    C = *HL;
    RRD;
    Adash = *HL;
    *DE++ = Adash;
    *HL++ = C;

    A = *HL;
    HL++;
  }
  while (--Bdash);

  SP = saved_sp;
  return;
}

#endif

