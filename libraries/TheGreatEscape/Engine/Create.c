/**
 * Create.c
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

/* ----------------------------------------------------------------------- */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "TheGreatEscape/TheGreatEscape.h"

#include "TheGreatEscape/Types.h"
#include "TheGreatEscape/InteriorObjectDefs.h"
#include "TheGreatEscape/Messages.h"
#include "TheGreatEscape/Rooms.h"
#include "TheGreatEscape/State.h"

/* ----------------------------------------------------------------------- */

/**
 * Initialise the game state.
 *
 * \param[in] state Pointer to game state.
 */
static void tge_initialise(tgestate_t *state)
{
  /**
   * $69AE: Default movable items.
   */
  static const movableitem_t movable_items[movable_item__LIMIT] =
  {
    { { 62, 35, 16, }, &sprites[sprite_STOVE], 0 },
    { { 55, 54, 14, }, &sprites[sprite_CRATE], 0 },
    { { 62, 35, 16, }, &sprites[sprite_STOVE], 0 },
  };

  /**
   * $7612: Default character structures.
   */
  static const characterstruct_t character_structs[character_structs__LIMIT] =
  {
    { character_0_COMMANDANT,   room_11_PAPERS,   {  46,  46, 24 }, {   3,  0 } }, // commandant route
    { character_1_GUARD_1,      room_0_OUTDOORS,  { 102,  68,  3 }, {   1,  0 } }, // L-shaped route in fenced area
    { character_2_GUARD_2,      room_0_OUTDOORS,  {  68, 104,  3 }, {   1,  2 } }, // L-shaped route in fenced area
    { character_3_GUARD_3,      room_16_CORRIDOR, {  46,  46, 24 }, {   3, 19 } }, // commandant's route but later on
    { character_4_GUARD_4,      room_0_OUTDOORS,  {  61, 103,  3 }, {   2,  4 } }, // guard route front perimeter wall
    { character_5_GUARD_5,      room_0_OUTDOORS,  { 106,  56, 13 }, {   0,  0 } }, // standing still in tower near front gate
    { character_6_GUARD_6,      room_0_OUTDOORS,  {  72,  94, 13 }, {   0,  0 } }, // standing still in the rightmost watchtower
    { character_7_GUARD_7,      room_0_OUTDOORS,  {  72,  70, 13 }, {   0,  0 } }, // standing still in the corner watchtower
    { character_8_GUARD_8,      room_0_OUTDOORS,  {  80,  46, 13 }, {   0,  0 } }, // standing still in the yard watchtower
    { character_9_GUARD_9,      room_0_OUTDOORS,  { 108,  71, 21 }, {   4,  0 } }, // the guard that marches back and forth above the main gate
    { character_10_GUARD_10,    room_0_OUTDOORS,  {  92,  52,  3 }, { 255, 56 } }, // wander in the exercise yard
    { character_11_GUARD_11,    room_0_OUTDOORS,  { 109,  69,  3 }, {   0,  0 } }, // standing still near the main gate
    { character_12_GUARD_12,    room_3_HUT2RIGHT, {  40,  60, 24 }, {   0,  8 } },
    { character_13_GUARD_13,    room_2_HUT2LEFT,  {  36,  48, 24 }, {   0,  8 } },
    { character_14_GUARD_14,    room_5_HUT3RIGHT, {  40,  60, 24 }, {   0, 16 } },
    { character_15_GUARD_15,    room_5_HUT3RIGHT, {  36,  34, 24 }, {   0, 16 } },
    { character_16_GUARD_DOG_1, room_0_OUTDOORS,  {  68,  84,  1 }, { 255,  0 } }, // wander in the right fenced off hand
    { character_17_GUARD_DOG_2, room_0_OUTDOORS,  {  68, 104,  1 }, { 255,  0 } }, // wander in the right fenced off hand
    { character_18_GUARD_DOG_3, room_0_OUTDOORS,  { 102,  68,  1 }, { 255, 24 } }, // wander in the bottom fenced off area
    { character_19_GUARD_DOG_4, room_0_OUTDOORS,  {  88,  68,  1 }, { 255, 24 } }, // wander in the bottom fenced off area
    { character_20_PRISONER_1,  room_NONE,        {  52,  60, 24 }, {   0,  8 } }, // wake_up, breakfast_time
    { character_21_PRISONER_2,  room_NONE,        {  52,  44, 24 }, {   0,  8 } }, // wake_up, breakfast_time
    { character_22_PRISONER_3,  room_NONE,        {  52,  28, 24 }, {   0,  8 } }, // wake_up, breakfast_time
    { character_23_PRISONER_4,  room_NONE,        {  52,  60, 24 }, {   0, 16 } }, // wake_up, breakfast_time
    { character_24_PRISONER_5,  room_NONE,        {  52,  44, 24 }, {   0, 16 } }, // wake_up, breakfast_time
    { character_25_PRISONER_6,  room_NONE,        {  52,  28, 24 }, {   0, 16 } }, // wake_up, breakfast_time
  };

  /**
   * $76C8: Default item structs.
   */
  static const itemstruct_t item_structs[item__LIMIT] =
  {
    { item_WIRESNIPS,        room_NONE,        { 64, 32,  2 }, { 120, 244 } },
    { item_SHOVEL,           room_9_CRATE,     { 62, 48,  0 }, { 124, 242 } },
    { item_LOCKPICK,         room_10_LOCKPICK, { 73, 36, 16 }, { 119, 240 } },
    { item_PAPERS,           room_11_PAPERS,   { 42, 58,  4 }, { 132, 243 } },
    { item_TORCH,            room_14_TORCH,    { 34, 24,  2 }, { 122, 246 } },
    { item_BRIBE,            room_NONE,        { 36, 44,  4 }, { 126, 244 } },
    { item_UNIFORM,          room_15_UNIFORM,  { 44, 65, 16 }, { 135, 241 } },
    { item_FOOD,             room_19_FOOD,     { 64, 48, 16 }, { 126, 240 } },
    { item_POISON,           room_1_HUT1RIGHT, { 66, 52,  4 }, { 124, 241 } },
    { item_RED_KEY,          room_22_REDKEY,   { 60, 42,  0 }, { 123, 242 } },
    { item_YELLOW_KEY,       room_11_PAPERS,   { 28, 34,  0 }, { 129, 248 } },
    { item_GREEN_KEY,        room_0_OUTDOORS,  { 74, 72,  0 }, { 122, 110 } },
    { item_RED_CROSS_PARCEL, room_NONE,        { 28, 50, 12 }, { 133, 246 } },
    { item_RADIO,            room_18_RADIO,    { 36, 58,  8 }, { 133, 244 } },
    { item_PURSE,            room_NONE,        { 36, 44,  4 }, { 126, 244 } },
    { item_COMPASS,          room_NONE,        { 52, 28,  4 }, { 126, 244 } },
  };

  /* $AD3E: Searchlight movement pattern for L-shaped gap? */
  static const uint8_t movement_0[] =
  {
     32, direction_BOTTOM_RIGHT,
     32, direction_TOP_RIGHT,
    255
  };

  /* $AD43: Searchlight movement pattern for main compound. */
  static const uint8_t movement_1[] =
  {
     24, direction_TOP_RIGHT,
     12, direction_TOP_LEFT,
     24, direction_BOTTOM_LEFT,
     12, direction_TOP_LEFT,
     32, direction_TOP_RIGHT,
     20, direction_TOP_LEFT,
     32, direction_BOTTOM_LEFT,
     44, direction_BOTTOM_RIGHT,
    255,
  };

  /* $AD54: Searchlight movement pattern for ? */
  static const uint8_t movement_2[] =
  {
     44, direction_BOTTOM_RIGHT,
     42, direction_TOP_RIGHT,
    255,
  };

  /**
   * $AD29: Default searchlight movement data.
   */
  static const searchlight_movement_t searchlight_states[3] =
  {
    { {  36, 82 }, 44, direction_BOTTOM_RIGHT, 0, &movement_2[0] },
    { { 120, 82 }, 24, direction_TOP_RIGHT,    0, &movement_1[0] },
    { {  60, 76 }, 32, direction_BOTTOM_RIGHT, 0, &movement_0[0] },
  };

  /**
   * $DD69: Default item attributes.
   */
  static const attribute_t item_attributes[item__LIMIT] =
  {
    attribute_YELLOW_OVER_BLACK,
    attribute_CYAN_OVER_BLACK,
    attribute_CYAN_OVER_BLACK,
    attribute_WHITE_OVER_BLACK,
    attribute_GREEN_OVER_BLACK,
    attribute_BRIGHT_RED_OVER_BLACK,
    attribute_GREEN_OVER_BLACK,
    attribute_WHITE_OVER_BLACK,
    attribute_PURPLE_OVER_BLACK,
    attribute_BRIGHT_RED_OVER_BLACK,
    attribute_YELLOW_OVER_BLACK,
    attribute_GREEN_OVER_BLACK,
    attribute_CYAN_OVER_BLACK,
    attribute_WHITE_OVER_BLACK,
    attribute_WHITE_OVER_BLACK,
    attribute_GREEN_OVER_BLACK,
  };

  /* $F05D: Default locked gates and doors. */
  static const doorindex_t locked_doors[] =
  {
    /* The doorindex_t indices here are those as passed to get_door. */
     0 | door_LOCKED, // outside-outside
     1 | door_LOCKED, // outside-outside
    13 | door_LOCKED, // inside-outside
    12 | door_LOCKED, // inside-outside
    14 | door_LOCKED, // inside-outside
    34 | door_LOCKED, // inside-inside
    24 | door_LOCKED, // inside-inside
    31 | door_LOCKED, // inside-inside
    22 | door_LOCKED, // inside-inside
    0, // unused afaict
    0, // unused afaict
  };

  /* Conv: Default values of mutable roomdef bytes. */
  static const uint8_t default_roomdef_shadow_bytes[] =
  {
    interiorobject_OCCUPIED_BED,

    interiorobject_OCCUPIED_BED,
    interiorobject_OCCUPIED_BED,
    interiorobject_OCCUPIED_BED,

    interiorobject_OCCUPIED_BED,
    interiorobject_OCCUPIED_BED,
    interiorobject_OCCUPIED_BED,

    interiorobject_EMPTY_BENCH,
    interiorobject_EMPTY_BENCH,
    interiorobject_EMPTY_BENCH,

    interiorobject_EMPTY_BENCH,
    interiorobject_EMPTY_BENCH,
    interiorobject_EMPTY_BENCH,
    interiorobject_EMPTY_BENCH,

    52,
    interiorobject_COLLAPSED_TUNNEL_SW_NE
  };


  /* Initialise in structure order. */

  /* $69AE */
  memcpy(state->movable_items, movable_items, sizeof(movable_items));

  /* $7612 */
  memcpy(state->character_structs,
         character_structs,
         sizeof(character_structs));

  /* $76C8 */
  memcpy(state->item_structs, item_structs, sizeof(item_structs));

  /* $7CFC */
  memset(&state->messages.queue[0], 0, message_queue_LENGTH);
  state->messages.queue[0] = message_QUEUE_END;
  state->messages.queue[1] = message_QUEUE_END;
  state->messages.queue[message_queue_LENGTH - 1] = message_QUEUE_END;
  state->messages.display_index = MESSAGE_NEXT_FLAG;
  state->messages.queue_pointer = &state->messages.queue[2];

  /* $A130 */
  state->bell = bell_STOP;

  /* $A13C */
  state->morale = morale_MAX;

  /* $A141 */
  state->moraleflag_screen_address =
    &state->speccy->screen.pixels[0x5002 - SCREEN_START_ADDRESS];

  /* $A263 */
  state->red_cross_parcel_current_contents = item_NONE;

  /* $AD29 */
  memcpy(state->searchlight.states,
         searchlight_states,
         sizeof(searchlight_states));

  /* $AF8E */
  state->bribed_character = character_NONE;

  /* $DD69 */
  memcpy(state->item_attributes,
         item_attributes,
         sizeof(item_attributes));

  /* $F05D */
  memcpy(state->locked_doors,
         locked_doors,
         sizeof(locked_doors));

#ifndef NDEBUG
  memset(state->tile_buf,   0x55, state->tile_buf_size);
  memset(state->window_buf, 0x55, state->window_buf_size);
  memset(state->map_buf,    0x55, state->map_buf_size);
#endif

  memcpy(state->roomdef_shadow_bytes,
         default_roomdef_shadow_bytes,
         sizeof(state->roomdef_shadow_bytes));
}

/**
 * Initialise the game state.
 *
 * \param[in] speccy Pointer to logical ZX Spectrum.
 * \param[in] config Pointer to game preferences structure.
 * \return Pointer to game state.
 */
TGE_API tgestate_t *tge_create(zxspectrum_t *speccy)
{
  tgestate_t       *state      = NULL;
  size_t            tile_buf_size;
  size_t            window_buf_stride;
  size_t            window_buf_size;
  size_t            map_buf_size;
  tileindex_t      *tile_buf   = NULL;
  uint8_t          *window_buf = NULL;
  supertileindex_t *map_buf    = NULL;

  /* Allocate state structure. */

  state = calloc(1, sizeof(*state));
  if (state == NULL)
    goto failure;

  /* Configure. */

  // Until we can resize...
  assert(speccy->screen.width  == 32);
  assert(speccy->screen.height == 24);

  state->width      = speccy->screen.width;
  state->height     = speccy->screen.height;

  /* Set the dimensions of the window buffer. When painted onto the game
   * screen the window is one unit smaller in both dimensions. That is a
   * 24x17 buffer is displayed through a 23x16 window on-screen. This allows
   * for rolling/scrolling. */
  state->columns    = 24;
  state->rows       = 17;

  /* Set the dimensions of the supertile buffer. This is held separately,
   * rather than computed from columns and rows, as it's wider than I
   * expected it to be. */
  state->st_columns = 7; // try  = (state->columns + 1) / 4; /* +1 is a fudge */
  state->st_rows    = 5; // try  = (state->rows       ) / 4;

  /* Size and allocate buffers. */

  tile_buf_size     = state->columns * state->rows;
  window_buf_stride = state->columns * 8;
  /* 8 bytes of padding are appended to the end of the window buffer - the
   * same as in the original game - to allow for plotting routine overruns.
   */
  window_buf_size   = window_buf_stride * state->rows + 8;
  map_buf_size      = state->st_columns * state->st_rows;

  state->tile_buf_size     = tile_buf_size;
  state->window_buf_stride = window_buf_stride;
  state->window_buf_size   = window_buf_size;
  state->map_buf_size      = map_buf_size;

  tile_buf   = calloc(1, tile_buf_size);
  window_buf = calloc(1, window_buf_size);
  map_buf    = calloc(1, map_buf_size);

  if (tile_buf   == NULL ||
      window_buf == NULL ||
      map_buf    == NULL)
    goto failure;

  state->tile_buf   = tile_buf;
  state->window_buf = window_buf;
  state->map_buf    = map_buf;

  state->prng_index = 0;

  /* Initialise additional variables. */

  state->speccy = speccy;

  /* Initialise original game variables. */

  tge_initialise(state);

  return state;


failure:

  free(map_buf);
  free(window_buf);
  free(tile_buf);
  free(state);

  return NULL;
}

TGE_API void tge_destroy(tgestate_t *state)
{
  if (state == NULL)
    return;

  free(state->map_buf);
  free(state->window_buf);
  free(state->tile_buf);

  free(state);
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et
