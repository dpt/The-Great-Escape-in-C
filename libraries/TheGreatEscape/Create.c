#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "TheGreatEscape/State.h"

#include "TheGreatEscape/TheGreatEscape.h"

/**
 * Initialise the state.
 */
static void tge_initialise(tgestate_t *state)
{
  /**
   * $69AE: Movable items.
   */
  static const movableitem_t movable_items[movable_item__LIMIT] =
  {
    { { 62, 35, 16, }, &sprites[sprite_STOVE], 0 },
    { { 55, 54, 14, }, &sprites[sprite_CRATE], 0 },
    { { 62, 35, 16, }, &sprites[sprite_STOVE], 0 },
  };

  /**
   * $7612: Character structures.
   */
  static const characterstruct_t character_structs[26] =
  {
    { character_0,  room_11_PAPERS,   {  46,  46, 24 }, 0x0003 },
    { character_1,  room_0_OUTDOORS,  { 102,  68,  3 }, 0x0001 },
    { character_2,  room_0_OUTDOORS,  {  68, 104,  3 }, 0x0201 },
    { character_3,  room_16_CORRIDOR, {  46,  46, 24 }, 0x1303 },
    { character_4,  room_0_OUTDOORS,  {  61, 103,  3 }, 0x0402 },
    { character_5,  room_0_OUTDOORS,  { 106,  56, 13 }, 0x0000 },
    { character_6,  room_0_OUTDOORS,  {  72,  94, 13 }, 0x0000 },
    { character_7,  room_0_OUTDOORS,  {  72,  70, 13 }, 0x0000 },
    { character_8,  room_0_OUTDOORS,  {  80,  46, 13 }, 0x0000 },
    { character_9,  room_0_OUTDOORS,  { 108,  71, 21 }, 0x0004 },
    { character_10, room_0_OUTDOORS,  {  92,  52,  3 }, 0x38FF },
    { character_11, room_0_OUTDOORS,  { 109,  69,  3 }, 0x0000 },
    { character_12, room_3_HUT2RIGHT, {  40,  60, 24 }, 0x0800 },
    { character_13, room_2_HUT2LEFT,  {  36,  48, 24 }, 0x0800 },
    { character_14, room_5_HUT3RIGHT, {  40,  60, 24 }, 0x1000 },
    { character_15, room_5_HUT3RIGHT, {  36,  34, 24 }, 0x1000 },
    { character_16, room_0_OUTDOORS,  {  68,  84,  1 }, 0x00FF },
    { character_17, room_0_OUTDOORS,  {  68, 104,  1 }, 0x00FF },
    { character_18, room_0_OUTDOORS,  { 102,  68,  1 }, 0x18FF },
    { character_19, room_0_OUTDOORS,  {  88,  68,  1 }, 0x18FF },

    { character_20, room_NONE,        {  52,  60, 24 }, 0x0800 }, // wake_up, breakfast_time
    { character_21, room_NONE,        {  52,  44, 24 }, 0x0800 }, // wake_up, breakfast_time
    { character_22, room_NONE,        {  52,  28, 24 }, 0x0800 }, // wake_up, breakfast_time

    { character_23, room_NONE,        {  52,  60, 24 }, 0x1000 }, // wake_up, breakfast_time
    { character_24, room_NONE,        {  52,  44, 24 }, 0x1000 }, // wake_up, breakfast_time
    { character_25, room_NONE,        {  52,  28, 24 }, 0x1000 }, // wake_up, breakfast_time
  };

  /**
   * $76C8: Item structs.
   */
  static const itemstruct_t item_structs[item__LIMIT] =
  {
    { item_WIRESNIPS,        room_NONE,        { 64, 32,  2 }, 0xF478 },
    { item_SHOVEL,           room_9_CRATE,     { 62, 48,  0 }, 0xF27C },
    { item_LOCKPICK,         room_10_LOCKPICK, { 73, 36, 16 }, 0xF077 },
    { item_PAPERS,           room_11_PAPERS,   { 42, 58,  4 }, 0xF384 },
    { item_TORCH,            room_14_TORCH,    { 34, 24,  2 }, 0xF67A },
    { item_BRIBE,            room_NONE,        { 36, 44,  4 }, 0xF47E },
    { item_UNIFORM,          room_15_UNIFORM,  { 44, 65, 16 }, 0xF187 },
    { item_FOOD,             room_19_FOOD,     { 64, 48, 16 }, 0xF07E },
    { item_POISON,           room_1_HUT1RIGHT, { 66, 52,  4 }, 0xF17C },
    { item_RED_KEY,          room_22_REDKEY,   { 60, 42,  0 }, 0xF27B },
    { item_YELLOW_KEY,       room_11_PAPERS,   { 28, 34,  0 }, 0xF881 },
    { item_GREEN_KEY,        room_0_OUTDOORS,  { 74, 72,  0 }, 0x6E7A },
    { item_RED_CROSS_PARCEL, room_NONE,        { 28, 50, 12 }, 0xF685 },
    { item_RADIO,            room_18_RADIO,    { 36, 58,  8 }, 0xF485 },
    { item_PURSE,            room_NONE,        { 36, 44,  4 }, 0xF47E },
    { item_COMPASS,          room_NONE,        { 52, 28,  4 }, 0xF47E },
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

  memcpy(state->movable_items, movable_items, sizeof(movable_items));

  memcpy(state->character_structs,
         character_structs,
         sizeof(character_structs));

  memcpy(state->item_structs, item_structs, sizeof(item_structs));

  state->message_queue[message_queue_LENGTH - 1] = message_NONE;

  state->moraleflag_screen_address =
      &state->speccy->screen[0x5002 - SCREEN_START_ADDRESS];

  // future: recalculate these
  memcpy(state->game_window_start_offsets,
         game_window_start_offsets,
         sizeof(game_window_start_offsets));
}

tgestate_t *tge_create(ZXSpectrum_t *speccy, tgeconfig_t *config)
{
  tgestate_t  *state                     = NULL;
  uint16_t    *game_window_start_offsets = NULL;
  tileindex_t *tile_buf                  = NULL;
  uint8_t     *window_buf                = NULL;
  supertileindex_t   *map_buf                   = NULL;

  assert(config);

  if (!config)
    goto failure;
  
  /* Allocate state structure. */

  state = malloc(sizeof(*state));
  if (state == NULL)
    goto failure;
  
  /* Configure. */
  
  state->width      = config->width;
  state->height     = config->height;
  
  /* This is the size of the game window only. */
  state->columns    = 23;
  state->rows       = 16;
  
  state->tb_columns = state->columns + 1;
  state->tb_rows    = state->rows    + 1; // perhaps not
  
  /* This is held separately, rather than computed from columns and rows, as
   * it's wider than I expected it to be. */
  state->st_columns = 7;
  state->st_rows    = 5;
  
  /* Allocate buffers. */
  
  game_window_start_offsets = malloc((size_t) (state->rows * 8) * sizeof(*game_window_start_offsets));
  tile_buf                  = malloc((size_t) (state->tb_columns * state->tb_rows));
  window_buf                = malloc((size_t) (state->columns * state->rows * 8));
  map_buf                   = malloc((size_t) (state->st_columns * state->st_rows));

  if (game_window_start_offsets == NULL ||
      tile_buf                  == NULL ||
      window_buf                == NULL ||
      map_buf                   == NULL)
    goto failure;
  
  state->game_window_start_offsets = game_window_start_offsets;
  state->tile_buf                  = tile_buf;
  state->window_buf                = window_buf;
  state->map_buf                   = map_buf;

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

void tge_destroy(tgestate_t *state)
{
  if (state == NULL)
    return;

  free(state->game_window_start_offsets);
  free(state->map_buf);
  free(state->window_buf);
  free(state->tile_buf);

  free(state);
}
