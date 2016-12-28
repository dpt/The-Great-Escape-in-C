#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "TheGreatEscape/State.h"

#include "TheGreatEscape/Messages.h"
#include "TheGreatEscape/TheGreatEscape.h"

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
    { character_0_COMMANDANT,   room_11_PAPERS,   {  46,  46, 24 }, { 0x03, 0x00 } },
    { character_1_GUARD_1,      room_0_OUTDOORS,  { 102,  68,  3 }, { 0x01, 0x00 } },
    { character_2_GUARD_2,      room_0_OUTDOORS,  {  68, 104,  3 }, { 0x01, 0x02 } },
    { character_3_GUARD_3,      room_16_CORRIDOR, {  46,  46, 24 }, { 0x03, 0x13 } },
    { character_4_GUARD_4,      room_0_OUTDOORS,  {  61, 103,  3 }, { 0x02, 0x04 } },
    { character_5_GUARD_5,      room_0_OUTDOORS,  { 106,  56, 13 }, { 0x00, 0x00 } },
    { character_6_GUARD_6,      room_0_OUTDOORS,  {  72,  94, 13 }, { 0x00, 0x00 } },
    { character_7_GUARD_7,      room_0_OUTDOORS,  {  72,  70, 13 }, { 0x00, 0x00 } },
    { character_8_GUARD_8,      room_0_OUTDOORS,  {  80,  46, 13 }, { 0x00, 0x00 } },
    { character_9_GUARD_9,      room_0_OUTDOORS,  { 108,  71, 21 }, { 0x04, 0x00 } },
    { character_10_GUARD_10,    room_0_OUTDOORS,  {  92,  52,  3 }, { 0xFF, 0x38 } },
    { character_11_GUARD_11,    room_0_OUTDOORS,  { 109,  69,  3 }, { 0x00, 0x00 } },
    { character_12_GUARD_12,    room_3_HUT2RIGHT, {  40,  60, 24 }, { 0x00, 0x08 } },
    { character_13_GUARD_13,    room_2_HUT2LEFT,  {  36,  48, 24 }, { 0x00, 0x08 } },
    { character_14_GUARD_14,    room_5_HUT3RIGHT, {  40,  60, 24 }, { 0x00, 0x10 } },
    { character_15_GUARD_15,    room_5_HUT3RIGHT, {  36,  34, 24 }, { 0x00, 0x10 } },
    { character_16_GUARD_DOG_1, room_0_OUTDOORS,  {  68,  84,  1 }, { 0xFF, 0x00 } },
    { character_17_GUARD_DOG_2, room_0_OUTDOORS,  {  68, 104,  1 }, { 0xFF, 0x00 } },
    { character_18_GUARD_DOG_3, room_0_OUTDOORS,  { 102,  68,  1 }, { 0xFF, 0x18 } },
    { character_19_GUARD_DOG_4, room_0_OUTDOORS,  {  88,  68,  1 }, { 0xFF, 0x18 } },
    { character_20_PRISONER_1,  room_NONE,        {  52,  60, 24 }, { 0x00, 0x08 } }, // wake_up, breakfast_time
    { character_21_PRISONER_2,  room_NONE,        {  52,  44, 24 }, { 0x00, 0x08 } }, // wake_up, breakfast_time
    { character_22_PRISONER_3,  room_NONE,        {  52,  28, 24 }, { 0x00, 0x08 } }, // wake_up, breakfast_time
    { character_23_PRISONER_4,  room_NONE,        {  52,  60, 24 }, { 0x00, 0x10 } }, // wake_up, breakfast_time
    { character_24_PRISONER_5,  room_NONE,        {  52,  44, 24 }, { 0x00, 0x10 } }, // wake_up, breakfast_time
    { character_25_PRISONER_6,  room_NONE,        {  52,  28, 24 }, { 0x00, 0x10 } }, // wake_up, breakfast_time
  };

  /**
   * $76C8: Default item structs.
   */
  static const itemstruct_t item_structs[item__LIMIT] =
  {
    { item_WIRESNIPS,        room_NONE,        { 64, 32,  2 }, { 0x78, 0xF4 } },
    { item_SHOVEL,           room_9_CRATE,     { 62, 48,  0 }, { 0x7C, 0xF2 } },
    { item_LOCKPICK,         room_10_LOCKPICK, { 73, 36, 16 }, { 0x77, 0xF0 } },
    { item_PAPERS,           room_11_PAPERS,   { 42, 58,  4 }, { 0x84, 0xF3 } },
    { item_TORCH,            room_14_TORCH,    { 34, 24,  2 }, { 0x7A, 0xF6 } },
    { item_BRIBE,            room_NONE,        { 36, 44,  4 }, { 0x7E, 0xF4 } },
    { item_UNIFORM,          room_15_UNIFORM,  { 44, 65, 16 }, { 0x87, 0xF1 } },
    { item_FOOD,             room_19_FOOD,     { 64, 48, 16 }, { 0x7E, 0xF0 } },
    { item_POISON,           room_1_HUT1RIGHT, { 66, 52,  4 }, { 0x7C, 0xF1 } },
    { item_RED_KEY,          room_22_REDKEY,   { 60, 42,  0 }, { 0x7B, 0xF2 } },
    { item_YELLOW_KEY,       room_11_PAPERS,   { 28, 34,  0 }, { 0x81, 0xF8 } },
    { item_GREEN_KEY,        room_0_OUTDOORS,  { 74, 72,  0 }, { 0x7A, 0x6E } },
    { item_RED_CROSS_PARCEL, room_NONE,        { 28, 50, 12 }, { 0x85, 0xF6 } },
    { item_RADIO,            room_18_RADIO,    { 36, 58,  8 }, { 0x85, 0xF4 } },
    { item_PURSE,            room_NONE,        { 36, 44,  4 }, { 0x7E, 0xF4 } },
    { item_COMPASS,          room_NONE,        { 52, 28,  4 }, { 0x7E, 0xF4 } },
  };

  /**
   * $783A: Default map locations.
   */
  static const xy_t locations[78] =
  {
    { 0x45, 0x68 }, // 0
    { 0x44, 0x54 },
    { 0x44, 0x46 },
    { 0x40, 0x66 },
    { 0x40, 0x40 },
    { 0x44, 0x44 },
    { 0x40, 0x40 },
    { 0x44, 0x40 },
    { 0x68, 0x70 },
    { 0x60, 0x70 },

    { 0x6A, 0x66 }, // 10
    { 0x5D, 0x68 },
    { 0x7C, 0x65 },
    { 0x7C, 0x70 },
    { 0x74, 0x68 },
    { 0x70, 0x64 },
    { 0x78, 0x60 },
    { 0x80, 0x58 },
    { 0x70, 0x60 },
    { 0x74, 0x54 },

    { 0x7C, 0x64 }, // 20
    { 0x7C, 0x70 },
    { 0x74, 0x68 },
    { 0x70, 0x64 },
    { 0x66, 0x44 },
    { 0x66, 0x40 },
    { 0x60, 0x40 },
    { 0x5C, 0x44 },
    { 0x56, 0x44 },
    { 0x54, 0x40 },

    { 0x4A, 0x44 }, // 30
    { 0x4A, 0x40 },
    { 0x66, 0x44 },
    { 0x44, 0x44 },
    { 0x44, 0x68 },
    { 0x6B, 0x45 },
    { 0x6B, 0x2D },
    { 0x4D, 0x2D },
    { 0x4D, 0x3D },
    { 0x3D, 0x3D }, // outside main map?

    { 0x3D, 0x67 }, // outside main map? // 40
    { 0x74, 0x4C },
    { 0x2C, 0x2A }, // outside main map?
    { 0x6A, 0x48 },
    { 0x6E, 0x48 },
    { 0x51, 0x68 },
    { 0x34, 0x3C }, // outside main map?
    { 0x34, 0x2C }, // outside main map?
    { 0x34, 0x1C },
    { 0x77, 0x6B },

    { 0x7A, 0x6E }, // 50
    { 0x34, 0x1C }, // outside main map?
    { 0x28, 0x3C }, // outside main map?
    { 0x24, 0x22 }, // outside main map?
    { 0x50, 0x4C },
    { 0x59, 0x4C },
    { 0x59, 0x3C },
    { 0x64, 0x3D },
    { 0x5C, 0x36 },
    { 0x54, 0x32 },

    { 0x66, 0x30 }, // 60
    { 0x60, 0x38 },
    { 0x4F, 0x3B },
    { 0x67, 0x2F },
    { 0x34, 0x36 }, // outside main map?
    { 0x34, 0x2E }, // outside main map?
    { 0x34, 0x24 }, // outside main map?
    { 0x34, 0x3E }, // outside main map?
    { 0x20, 0x38 }, // outside main map?
    { 0x34, 0x18 }, // outside main map?

    { 0x2A, 0x2E }, // outside main map? // 70
    { 0x22, 0x22 }, // outside main map?
    { 0x78, 0x6E }, // roll call
    { 0x76, 0x6E }, // roll call
    { 0x74, 0x6E }, // roll call
    { 0x79, 0x6D }, // roll call
    { 0x77, 0x6D }, // roll call
    { 0x75, 0x6D }, // roll call
  };

  /* $AD3E: Searchlight movement pattern for L-shaped gap? */
  static const uint8_t movement_0[] =
  {
    0x20, direction_BOTTOM_RIGHT,
    0x20, direction_TOP_RIGHT,
    0xFF
  };

  /* $AD43: Searchlight movement pattern for main compound. */
  static const uint8_t movement_1[] =
  {
    0x18, direction_TOP_RIGHT,
    0x0C, direction_TOP_LEFT,
    0x18, direction_BOTTOM_LEFT,
    0x0C, direction_TOP_LEFT,
    0x20, direction_TOP_RIGHT,
    0x14, direction_TOP_LEFT,
    0x20, direction_BOTTOM_LEFT,
    0x2C, direction_BOTTOM_RIGHT,
    0xFF,
  };

  /* $AD54: Searchlight movement pattern for ? */
  static const uint8_t movement_2[] =
  {
    0x2C, direction_BOTTOM_RIGHT,
    0x2A, direction_TOP_RIGHT,
    0xFF,
  };

  /**
   * $AD29: Default searchlight movement data.
   */
  static const searchlight_movement_t searchlight_states[3] =
  {
    { { 0x24, 0x52 }, 0x2C, direction_BOTTOM_RIGHT, 0, &movement_2[0] },
    { { 0x78, 0x52 }, 0x18, direction_TOP_RIGHT,    0, &movement_1[0] },
    { { 0x3C, 0x4C }, 0x20, direction_BOTTOM_RIGHT, 0, &movement_0[0] },
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

  /**
   * $EDD3: Game screen start addresses.
   *
   * Absolute addresses in the original code. These are now offsets.
   */
  static const uint16_t game_window_start_offsets[128] =
  {
    0x0047,
    0x0147,
    0x0247,
    0x0347,
    0x0447,
    0x0547,
    0x0647,
    0x0747,
    0x0067,
    0x0167,
    0x0267,
    0x0367,
    0x0467,
    0x0567,
    0x0667,
    0x0767,
    0x0087,
    0x0187,
    0x0287,
    0x0387,
    0x0487,
    0x0587,
    0x0687,
    0x0787,
    0x00A7,
    0x01A7,
    0x02A7,
    0x03A7,
    0x04A7,
    0x05A7,
    0x06A7,
    0x07A7,
    0x00C7,
    0x01C7,
    0x02C7,
    0x03C7,
    0x04C7,
    0x05C7,
    0x06C7,
    0x07C7,
    0x00E7,
    0x01E7,
    0x02E7,
    0x03E7,
    0x04E7,
    0x05E7,
    0x06E7,
    0x07E7,
    0x0807,
    0x0907,
    0x0A07,
    0x0B07,
    0x0C07,
    0x0D07,
    0x0E07,
    0x0F07,
    0x0827,
    0x0927,
    0x0A27,
    0x0B27,
    0x0C27,
    0x0D27,
    0x0E27,
    0x0F27,
    0x0847,
    0x0947,
    0x0A47,
    0x0B47,
    0x0C47,
    0x0D47,
    0x0E47,
    0x0F47,
    0x0867,
    0x0967,
    0x0A67,
    0x0B67,
    0x0C67,
    0x0D67,
    0x0E67,
    0x0F67,
    0x0887,
    0x0987,
    0x0A87,
    0x0B87,
    0x0C87,
    0x0D87,
    0x0E87,
    0x0F87,
    0x08A7,
    0x09A7,
    0x0AA7,
    0x0BA7,
    0x0CA7,
    0x0DA7,
    0x0EA7,
    0x0FA7,
    0x08C7,
    0x09C7,
    0x0AC7,
    0x0BC7,
    0x0CC7,
    0x0DC7,
    0x0EC7,
    0x0FC7,
    0x08E7,
    0x09E7,
    0x0AE7,
    0x0BE7,
    0x0CE7,
    0x0DE7,
    0x0EE7,
    0x0FE7,
    0x1007,
    0x1107,
    0x1207,
    0x1307,
    0x1407,
    0x1507,
    0x1607,
    0x1707,
    0x1027,
    0x1127,
    0x1227,
    0x1327,
    0x1427,
    0x1527,
    0x1627,
    0x1727,
  };

  /* Initialise in structure order. */

  // Future: Table drive this copying.

  /* $69AE */
  memcpy(state->movable_items, movable_items, sizeof(movable_items));

  /* $7612 */
  memcpy(state->character_structs,
         character_structs,
         sizeof(character_structs));

  /* $76C8 */
  memcpy(state->item_structs, item_structs, sizeof(item_structs));

  /* $783A */
  memcpy(state->locations, locations, sizeof(locations));

  /* $7CFC */
  memset(&state->messages.queue[0], 0, message_queue_LENGTH);
  state->messages.queue[0] = message_QUEUE_END;
  state->messages.queue[1] = message_QUEUE_END;
  state->messages.queue[message_queue_LENGTH - 1] = message_QUEUE_END;
  state->messages.display_index = 1 << 7; // message_NEXT
  state->messages.queue_pointer = &state->messages.queue[2];

  /* $A13C */
  state->morale = morale_MAX;

  /* $A141 */
  state->moraleflag_screen_address =
      &state->speccy->screen[0x5002 - SCREEN_START_ADDRESS];

  /** $A263 */
  state->red_cross_parcel_current_contents = item_NONE;

  /* $AD29 */
  memcpy(state->searchlight.states,
         searchlight_states,
         sizeof(searchlight_states));

  /* $DD69 */
  memcpy(state->item_attributes,
         item_attributes,
         sizeof(item_attributes));

  /* $EDD3 */
  // Future: recalculate these
  memcpy(state->game_window_start_offsets,
         game_window_start_offsets,
         sizeof(game_window_start_offsets));

  // temporary
  memset(state->tile_buf,   0x55,  state->columns * state->rows);
  memset(state->window_buf, 0x55, (state->columns * state->rows * 8));
  memset(state->map_buf,    0x55,  state->st_columns * state->st_rows);
}

/**
 * Initialise the game state.
 *
 * \param[in] speccy Pointer to logical ZX Spectrum.
 * \param[in] config Pointer to game preferences structure.
 * \return Pointer to game state.
 */
TGE_API tgestate_t *tge_create(zxspectrum_t *speccy, const tgeconfig_t *config)
{
  tgestate_t       *state                     = NULL;
  uint16_t         *game_window_start_offsets = NULL;
  tileindex_t      *tile_buf                  = NULL;
  uint8_t          *window_buf                = NULL;
  supertileindex_t *map_buf                   = NULL;

  assert(config);

  if (!config)
    goto failure;
  
  /* Allocate state structure. */

  state = calloc(1, sizeof(*state));
  if (state == NULL)
    goto failure;
  
  /* Configure. */
  
  state->width      = config->width;
  state->height     = config->height;

  // Until we can resize...
  assert(state->width  == 32);
  assert(state->height == 24);

  /* Set the size of the window buffer. The game screen is assumed to be one smaller in both
   * dimensions. That is a 24x17 buffer is displayed through a 23x16 window on-screen. This allows
   * for rolling/scrolling. */
  state->columns    = 24;
  state->rows       = 17;

  /* This is held separately, rather than computed from columns and rows, as
   * it's wider than I expected it to be. */
  state->st_columns = 7;
  state->st_rows    = 5;
  
  /* Allocate buffers. */
  
  game_window_start_offsets = calloc(1, (size_t) ((state->rows - 1) * 8) * sizeof(*game_window_start_offsets));
  tile_buf                  = calloc(1, (size_t) (state->columns * state->rows));
  window_buf                = calloc(1, (size_t) (state->columns * state->rows * 8));
  map_buf                   = calloc(1, (size_t) (state->st_columns * state->st_rows));

  if (game_window_start_offsets == NULL ||
      tile_buf                  == NULL ||
      window_buf                == NULL ||
      map_buf                   == NULL)
    goto failure;
  
  state->game_window_start_offsets = game_window_start_offsets;
  state->tile_buf                  = tile_buf;
  state->window_buf                = window_buf;
  state->map_buf                   = map_buf;
  
  state->prng_index                = 0;

  /* Initialise additional variables. */
  
  state->speccy = speccy;
  
  /* Initialise original game variables. */
  
  tge_initialise(state);

  return state;


failure:

  free(game_window_start_offsets);
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

  free(state->game_window_start_offsets);
  free(state->map_buf);
  free(state->window_buf);
  free(state->tile_buf);

  free(state);
}

// vim: ts=8 sts=2 sw=2 et

