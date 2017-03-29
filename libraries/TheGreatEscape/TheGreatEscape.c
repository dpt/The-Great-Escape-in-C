/**
 * TheGreatEscape.c
 *
 * Reimplementation in C of the 1986 ZX Spectrum 48K game "The Great Escape".
 *
 * "The Great Escape" created by Denton Designs, published by Ocean Software.
 *
 * This reimplementation copyright (c) David Thomas, 2012-2017.
 * <dave@davespace.co.uk>.
 */

/* ----------------------------------------------------------------------- */

/* TODO
 *
 * - Some enums might be wider than expected (int vs uint8_t).
 * - Decode foreground masks.
 * - Naming: "breakfast" room ought to be "mess hall".
 */

/* LATER
 *
 * - Hoist out constant sizes to variables in the state structure (required
 *   for variably-sized screens).
 * - Move screen memory to a linear format (again, required for
 *   variably-sized screen).
 * - Replace uint8_t counters, and other types which are smaller than int,
 *   with int where possible.
 * - Merge loop elements into the body of the code.
 */

/* GLOSSARY
 *
 * - "Conv:"
 *   -- Code which has required adjustment.
 * - A call marked "exit via"
 *   -- The original code branched directly to its target and exited via it.
 */

/* ----------------------------------------------------------------------- */

#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ZXSpectrum/Spectrum.h"

#include "TheGreatEscape/TheGreatEscape.h"

#include "TheGreatEscape/Doors.h"
#include "TheGreatEscape/ExteriorTiles.h"
#include "TheGreatEscape/Input.h"
#include "TheGreatEscape/InteriorObjectDefs.h"
#include "TheGreatEscape/InteriorObjects.h"
#include "TheGreatEscape/InteriorTiles.h"
#include "TheGreatEscape/ItemBitmaps.h"
#include "TheGreatEscape/Items.h"
#include "TheGreatEscape/Map.h"
#include "TheGreatEscape/Masks.h"
#include "TheGreatEscape/Menu.h"
#include "TheGreatEscape/Messages.h"
#include "TheGreatEscape/Music.h"
#include "TheGreatEscape/RoomDefs.h"
#include "TheGreatEscape/Rooms.h"
#include "TheGreatEscape/SpriteBitmaps.h"
#include "TheGreatEscape/Sprites.h"
#include "TheGreatEscape/State.h"
#include "TheGreatEscape/StaticGraphics.h"
#include "TheGreatEscape/SuperTiles.h"
#include "TheGreatEscape/TGEObject.h"
#include "TheGreatEscape/Text.h"
#include "TheGreatEscape/Tiles.h"
#include "TheGreatEscape/Utils.h"

#include "TheGreatEscape/Main.h"

// debug
static void check_map_buf(tgestate_t *state)
{
  for (int i = 0; i < state->st_columns * state->st_rows; i++)
  {
    assert(state->map_buf[i] < supertileindex__LIMIT);
  }
}



/* ----------------------------------------------------------------------- */

/**
 * $68A2: Transition.
 *
 * The hero or a non-player character changes room.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] pos     Pointer to pos. (was HL)
 *
 * \remarks Exits using longjmp in the hero case.
 */
void transition(tgestate_t      *state,
                const tinypos_t *pos)
{
  vischar_t *vischar;    /* was IY */
  room_t     room_index; /* was A */

  assert(state != NULL);
  assert(pos   != NULL);

  vischar = state->IY;
  ASSERT_VISCHAR_VALID(vischar);

  if (vischar->room == room_0_OUTDOORS)
  {
    /* Outdoors */

    /* Set position on X axis, Y axis and height (dividing by 4). */

    /* Conv: This was unrolled (compared to the original) to avoid having to
     * access the structure as an array. */
    vischar->mi.pos.x      = multiply_by_4(pos->x);
    vischar->mi.pos.y      = multiply_by_4(pos->y);
    vischar->mi.pos.height = multiply_by_4(pos->height);
  }
  else
  {
    /* Indoors */

    /* Set position on X axis, Y axis and height (copying). */

    /* Conv: This was unrolled (compared to the original) to avoid having to
     * access the structure as an array. */
    vischar->mi.pos.x      = pos->x;
    vischar->mi.pos.y      = pos->y;
    vischar->mi.pos.height = pos->height;
  }

  if (vischar != &state->vischars[0])
  {
    /* Not the hero. */

    reset_visible_character(state, vischar); // was exit via
  }
  else
  {
    /* Hero only. */

    vischar->flags &= ~vischar_FLAGS_NO_COLLIDE;
    state->room_index = room_index = state->vischars[0].room;
    if (room_index == room_0_OUTDOORS)
    {
      /* Outdoors */

      vischar->input = input_KICK;
      vischar->direction &= vischar_DIRECTION_MASK;
      reset_outdoors(state);
      squash_stack_goto_main(state); // exit
    }
    else
    {
      /* Indoors */

      enter_room(state); // was fallthrough
    }
    NEVER_RETURNS;
  }
}

/**
 * $68F4: Enter room.
 *
 * The hero enters a room.
 *
 * \param[in] state Pointer to game state.
 *
 * \remarks Exits using longjmp.
 */
void enter_room(tgestate_t *state)
{
  assert(state != NULL);

  state->game_window_offset.x = 0;
  state->game_window_offset.y = 0;
  setup_room(state);
  plot_interior_tiles(state);
  state->map_position.x = 116;
  state->map_position.y = 234;
  set_hero_sprite_for_room(state);
  calc_vischar_screenpos_from_mi_pos(state, &state->vischars[0]);
  setup_movable_items(state);
  zoombox(state);
  increase_score(state, 1);

  squash_stack_goto_main(state); // (was fallthrough to following)
  NEVER_RETURNS;
}

/**
 * $691A: Squash stack, then goto main.
 *
 * \param[in] state Pointer to game state.
 *
 * \remarks Exits using longjmp.
 */
void squash_stack_goto_main(tgestate_t *state)
{
  assert(state != NULL);

  longjmp(state->jmpbuf_main, 1);
  NEVER_RETURNS;
}

/* ----------------------------------------------------------------------- */

/**
 * $6920: Set appropriate hero sprite for room.
 *
 * Called when changing rooms.
 *
 * \param[in] state Pointer to game state.
 */
void set_hero_sprite_for_room(tgestate_t *state)
{
  vischar_t *hero; /* was HL */

  assert(state != NULL);

  hero = &state->vischars[0];
  hero->input = input_KICK;

  /* vischar_DIRECTION_CRAWL is set, or cleared, here but not tested
   * directly anywhere else so it must be an offset into byte_CDAA[].
   */

  /* When in tunnel rooms force the hero sprite to 'prisoner' and set the
   * crawl flag appropriately. */
  if (state->room_index >= room_29_SECOND_TUNNEL_START)
  {
    hero->direction |= vischar_DIRECTION_CRAWL;
    hero->mi.sprite = &sprites[sprite_PRISONER_FACING_AWAY_1];
  }
  else
  {
    hero->direction &= ~vischar_DIRECTION_CRAWL;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $6939: Setup movable items.
 *
 * \param[in] state Pointer to game state.
 */
void setup_movable_items(tgestate_t *state)
{
  const movableitem_t *movableitem;
  character_t          character;

  assert(state != NULL);

  reset_nonplayer_visible_characters(state);

  /* Conv: Use a switch statement rather than if-else and gotos. */

  switch (state->room_index)
  {
  case room_2_HUT2LEFT:
    movableitem = &state->movable_items[movable_item_STOVE1];
    character   = character_26_STOVE_1;
    setup_movable_item(state, movableitem, character);
    break;

  case room_4_HUT3LEFT:
    movableitem = &state->movable_items[movable_item_STOVE2];
    character   = character_27_STOVE_2;
    setup_movable_item(state, movableitem, character);
    break;

  case room_9_CRATE:
    movableitem = &state->movable_items[movable_item_CRATE];
    character   = character_28_CRATE;
    setup_movable_item(state, movableitem, character);
    break;

  default:
    break;
  }

  spawn_characters(state);
  mark_nearby_items(state);
  called_from_main_loop_9(state);
  move_map(state);
  plot_sprites(state);
}

/**
 * $697D: Setup the second vischar as a movable item.
 *
 * \param[in] state       Pointer to game state.
 * \param[in] movableitem Pointer to movable item. (was HL)
 * \param[in] character   Character. (was A)
 */
void setup_movable_item(tgestate_t          *state,
                        const movableitem_t *movableitem,
                        character_t          character)
{
  vischar_t *vischar1; /* was HL */

  assert(state       != NULL);
  assert(movableitem != NULL);
  assert(character >= 0 && character < character__LIMIT);

  /* The movable item takes the place of the first non-player visible
   * character. */
  vischar1 = &state->vischars[1];

  vischar1->character         = character;
  vischar1->mi                = *movableitem;

  /* Conv: Reset data inlined. */

  vischar1->flags             = 0;
  vischar1->route.index       = 0;
  vischar1->route.step        = 0;
  vischar1->pos.x             = 0;
  vischar1->pos.y             = 0;
  vischar1->pos.height        = 0;
  vischar1->counter_and_flags = 0;
  vischar1->animbase          = &animations[0];
  vischar1->anim              = animations[8]; /* -> anim_wait_tl animation */
  vischar1->animindex         = 0;
  vischar1->input             = 0;
  vischar1->direction         = direction_TOP_LEFT; /* == 0 */

  vischar1->room              = state->room_index;
  calc_vischar_screenpos_from_mi_pos(state, vischar1);
}

/* ----------------------------------------------------------------------- */

/**
 * $69C9: Reset all non-player visible characters.
 *
 * \param[in] state Pointer to game state.
 */
void reset_nonplayer_visible_characters(tgestate_t *state)
{
  vischar_t *vischar; /* was HL */
  uint8_t    iters;   /* was B */

  assert(state != NULL);

  vischar = &state->vischars[1];
  iters = vischars_LENGTH - 1;
  do
    reset_visible_character(state, vischar++);
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $69DC: Setup interior doors.
 *
 * Used by setup_room only.
 *
 * \param[in] state Pointer to game state.
 */
void setup_doors(tgestate_t *state)
{
  doorindex_t  *pdoorindex; /* was DE */
  uint8_t       iters;      /* was B */
  uint8_t       room;       /* was B */ // same type as door_t->room_and_flags
  doorindex_t   door_index; /* was C */
  const door_t *door;       /* was HL' */
  uint8_t       door_iters; /* was B' */

  assert(state != NULL);

  /* Wipe state->interior_doors[] with door_NONE. */
  // Alternative: memset(&state->interior_doors[0], door_NONE, 4);
  pdoorindex = &state->interior_doors[3];
  iters = 4;
  do
    *pdoorindex-- = door_NONE;
  while (--iters);

  pdoorindex++;
  ASSERT_DOORS_VALID(pdoorindex);

  /* Ensure that no flag bits remain. */
  assert(state->room_index < room__LIMIT);

  room       = state->room_index << 2; /* Shunt left to match comparison in loop. */
  door_index = 0;
  door       = &doors[0];
  door_iters = NELEMS(doors); /* Iterate over every individual door in doors[]. */
  do
  {
    /* Save any door index which matches the current room. */
    // Rooms are always square so should have no more than four doors but that is unchecked by this code.
    if ((door->room_and_flags & ~door_FLAGS_MASK_DIRECTION) == room)
      /* Current room. */
      *pdoorindex++ = door_index ^ door_REVERSE; // Store the index and the reverse flag.

    /* On each step toggle the reverse flag. */
    door_index ^= door_REVERSE;

    /* Increment door_index once every two steps through the array. */
    if (door_index < door_REVERSE) /* Conv: was JP M. */
      door_index++;

    door++;
  }
  while (--door_iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $6A12: Turn a door index into a door_t pointer.
 *
 * \param[in] door Index of door + lock flag in bit 7. (was A)
 *
 * \return Pointer to door_t. (was HL)
 */
const door_t *get_door(doorindex_t door)
{
  const door_t *pos; /* was HL */

  assert((door & ~door_REVERSE) < door_MAX);

  /* Conv: Mask before multiplication to avoid overflow. */
  pos = &doors[(door & ~door_REVERSE) * 2];
  if (door & door_REVERSE)
    pos++;

  return pos;
}

/* ----------------------------------------------------------------------- */

/**
 * $6A27: Wipe the visible tiles array.
 *
 * \param[in] state Pointer to game state.
 */
void wipe_visible_tiles(tgestate_t *state)
{
  assert(state != NULL);

  memset(state->tile_buf, 0, state->columns * state->rows); /* was 24 * 17 */
}

/* ----------------------------------------------------------------------- */

/**
 * $6A35: Setup room.
 *
 * Expand out the room definition for state->room_index.
 *
 * \param[in] state Pointer to game state.
 */
void setup_room(tgestate_t *state)
{
  /**
   * $EA7C: Interior masking data.
   *
   * Conv: Constant final (pos.height) byte added into this data.
   */
  static const mask_t interior_mask_data_source[47] =
  {
    { 27, { 123, 127, 241, 243 }, { 54, 40, 32 } },
    { 27, { 119, 123, 243, 245 }, { 54, 24, 32 } },
    { 27, { 124, 128, 241, 243 }, { 50, 42, 32 } },
    { 25, { 131, 134, 242, 247 }, { 24, 36, 32 } },
    { 25, { 129, 132, 244, 249 }, { 24, 26, 32 } },
    { 25, { 129, 132, 243, 248 }, { 28, 23, 32 } },
    { 25, { 131, 134, 244, 248 }, { 22, 32, 32 } },
    { 24, { 125, 128, 244, 249 }, { 24, 26, 32 } },
    { 24, { 123, 126, 243, 248 }, { 34, 26, 32 } },
    { 24, { 121, 124, 244, 249 }, { 34, 16, 32 } },
    { 24, { 123, 126, 244, 249 }, { 28, 23, 32 } },
    { 24, { 121, 124, 241, 246 }, { 44, 30, 32 } },
    { 24, { 125, 128, 242, 247 }, { 36, 34, 32 } },
    { 29, { 127, 130, 246, 247 }, { 28, 30, 32 } },
    { 29, { 130, 133, 242, 243 }, { 35, 48, 32 } },
    { 29, { 134, 137, 242, 243 }, { 28, 55, 32 } },
    { 29, { 134, 137, 244, 245 }, { 24, 48, 32 } },
    { 29, { 128, 131, 241, 242 }, { 40, 48, 32 } },
    { 28, { 129, 130, 244, 246 }, { 28, 32, 32 } },
    { 28, { 131, 132, 244, 246 }, { 28, 46, 32 } },
    { 26, { 126, 128, 245, 247 }, { 28, 32, 32 } },
    { 18, { 122, 123, 242, 243 }, { 58, 40, 32 } },
    { 18, { 122, 123, 239, 240 }, { 69, 53, 32 } },
    { 23, { 128, 133, 244, 246 }, { 28, 36, 32 } },
    { 20, { 128, 132, 243, 245 }, { 38, 40, 32 } },
    { 21, { 132, 133, 246, 247 }, { 26, 30, 32 } },
    { 21, { 126, 127, 243, 244 }, { 46, 38, 32 } },
    { 22, { 124, 133, 239, 243 }, { 50, 34, 32 } },
    { 22, { 121, 130, 240, 244 }, { 52, 26, 32 } },
    { 22, { 125, 134, 242, 246 }, { 36, 26, 32 } },
    { 16, { 118, 120, 245, 247 }, { 54, 10, 32 } },
    { 16, { 122, 124, 243, 245 }, { 54, 10, 32 } },
    { 16, { 126, 128, 241, 243 }, { 54, 10, 32 } },
    { 16, { 130, 132, 239, 241 }, { 54, 10, 32 } },
    { 16, { 134, 136, 237, 239 }, { 54, 10, 32 } },
    { 16, { 138, 140, 235, 237 }, { 54, 10, 32 } },
    { 17, { 115, 117, 235, 237 }, { 10, 48, 32 } },
    { 17, { 119, 121, 237, 239 }, { 10, 48, 32 } },
    { 17, { 123, 125, 239, 241 }, { 10, 48, 32 } },
    { 17, { 127, 129, 241, 243 }, { 10, 48, 32 } },
    { 17, { 131, 133, 243, 245 }, { 10, 48, 32 } },
    { 17, { 135, 137, 245, 247 }, { 10, 48, 32 } },
    { 16, { 132, 134, 244, 247 }, { 10, 48, 32 } },
    { 17, { 135, 137, 237, 239 }, { 10, 48, 32 } },
    { 17, { 123, 125, 243, 245 }, { 10, 10, 32 } },
    { 17, { 121, 123, 244, 246 }, { 10, 10, 32 } },
    { 15, { 136, 140, 245, 248 }, { 10, 10, 32 } },
  };

  const roomdef_t *proomdef; /* was HL */
  bounds_t        *pbounds;  /* was DE */
  mask_t          *pmask;    /* was DE */
  uint8_t          count;    /* was A */
  uint8_t          iters;    /* was B */

  assert(state != NULL);

  wipe_visible_tiles(state);

  assert(state->room_index >= 0);
  assert(state->room_index < room__LIMIT);
  proomdef = rooms_and_tunnels[state->room_index - 1]; /* array starts with room 1 */

  setup_doors(state);

  state->roomdef_bounds_index = *proomdef++;

  /* Copy boundaries into state. */
  state->roomdef_object_bounds_count = count = *proomdef; /* count of boundaries */
  assert(count <= 4);
  pbounds = &state->roomdef_object_bounds[0]; /* Conv: Moved */
  if (count == 0) /* no boundaries */
  {
    proomdef++; /* skip to interior mask */
  }
  else
  {
    proomdef++; /* Conv: Don't re-copy already copied count byte. */
    memcpy(pbounds, proomdef, count * sizeof(bounds_t));
    proomdef += count * sizeof(bounds_t); /* skip to interior mask */
  }

  /* Copy interior mask into state->interior_mask_data. */
  state->interior_mask_data_count = iters = *proomdef++; /* count of interior masks */
  assert(iters <= MAX_INTERIOR_MASK_REFS);
  pmask = &state->interior_mask_data[0]; /* Conv: Moved */
  while (iters--)
  {
    /* Conv: Structures were changed from 7 to 8 bytes wide. */
    memcpy(pmask, &interior_mask_data_source[*proomdef++], sizeof(*pmask));
    pmask++;
  }

  /* Plot all objects (as tiles). */
  iters = *proomdef++; /* Count of objects */
  while (iters--)
  {
    expand_object(state,
                  proomdef[0], /* object index */
                  &state->tile_buf[proomdef[2] * state->columns + proomdef[1]]); /* HL[2] = row, HL[1] = column */
    proomdef += 3;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $6AB5: Expands RLE-encoded objects to a full set of tile references.
 *
 * Format:
 * @code
 * <w> <h>: width, height
 * Repeat:
 *   <t>:                  emit tile <t>
 *   <255> <64..127> <t>:  emit tiles <t> <t+1> <t+2> .. up to 63 times
 *   <255> <128..254> <t>: emit tile <t> up to 126 times
 *   <255> <255>:          emit <255>
 * @endcode
 *
 * \param[in]  state  Pointer to game state.
 * \param[in]  index  Object index to expand. (was A)
 * \param[out] output Tile buffer location to expand to. (was DE)
 */
void expand_object(tgestate_t *state, object_t index, uint8_t *output)
{
  int                columns;       /* new var */
  const tgeobject_t *obj;           /* was HL */
  int                width, height; /* was B, C */
  int                self_width;    /* was $6AE7 */
  const uint8_t     *data;          /* was HL */
  int                byte;          /* was A */
  int                val;           /* was A' */

  assert(state  != NULL);
  assert(index >= 0 && index < interiorobject__LIMIT);
  assert(output != NULL); // assert within tilebuf?

  columns     = state->columns; // Conv: Added.

  assert(columns == 24); // did i add this for any particular reason?

  obj         = interior_object_defs[index];

  width       = obj->width;
  height      = obj->height;

  self_width  = width;

  data        = &obj->data[0];

  assert(width  > 0);
  assert(height > 0);

  do
  {
    do
    {
expand:

      assert(width  > 0);
      assert(height > 0);

      byte = *data;
      if (byte == interiortile_ESCAPE)
      {
        byte = *++data;
        if (byte != interiortile_ESCAPE)
        {
          byte &= 0xF0;
          if (byte >= 128)
            goto run;
          if (byte == 64)
            goto range;
        }
      }

      if (byte)
        *output = byte;
      data++;
      output++;
    }
    while (--width);

    width = self_width;
    output += columns - width; /* move to next row */
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

    if (--width == 0) /* ran out of width */
    {
      width = self_width;
      output += columns - width; /* move to next row */

      // val = *data; // needless reload

      if (--height == 0)
        return;
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

    if (--width == 0) /* ran out of width */
    {
      width = self_width;
      output += columns - self_width; /* move to next row */

      if (--height == 0)
        return;
    }
  }
  while (--byte);

  data++;

  goto expand;
}

/* ----------------------------------------------------------------------- */

/**
 * $6B42: Expand all of the tile indices in the tiles buffer to full tiles in
 * the screen buffer.
 *
 * \param[in] state Pointer to game state.
 */
void plot_interior_tiles(tgestate_t *state)
{
  int                        rows;          /* added */
  int                        columns;       /* added */
  uint8_t                   *window_buf;    /* was HL */
  const interiortileindex_t *tiles_buf;     /* was DE */
  int                        rowcounter;    /* was C */
  int                        columncounter; /* was B */
  int                        iters;         /* was B */
  int                        stride;        /* was C */

  assert(state != NULL);

  rows       = state->rows - 1; // 16
  columns    = state->columns; // 24

  window_buf = state->window_buf;
  tiles_buf  = state->tile_buf; // note: type is coerced

  rowcounter = rows;
  do
  {
    columncounter = columns;
    do
    {
      const tilerow_t *tile_data;   /* was HL */
      uint8_t         *window_buf2; /* was DE */

      ASSERT_TILE_BUF_PTR_VALID(tiles_buf);

      tile_data = &interior_tiles[*tiles_buf].row[0];

      window_buf2 = window_buf;

      iters  = 8;
      stride = columns;
      do
      {
        ASSERT_WINDOW_BUF_PTR_VALID(window_buf2);
        ASSERT_INTERIOR_TILES_VALID(tile_data);

        *window_buf2 = *tile_data++;
        window_buf2 += stride;
      }
      while (--iters);

      tiles_buf++;
      window_buf++; // move to next character position
    }
    while (--columncounter);
    window_buf += 7 * columns; // move to next row
  }
  while (--rowcounter);
}

/* ----------------------------------------------------------------------- */

/**
 * $6B79: Locations of beds.
 *
 * Used by wake_up, character_sleeps and reset_map_and_characters.
 */
uint8_t *const beds[beds_LENGTH] =
{
  &roomdef_3_hut2_right[29],
  &roomdef_3_hut2_right[32],
  &roomdef_3_hut2_right[35],
  &roomdef_5_hut3_right[29],
  &roomdef_5_hut3_right[32],
  &roomdef_5_hut3_right[35],
};

/* ----------------------------------------------------------------------- */

/**
 * $78D6: Door positions.
 *
 * Used by setup_doors, get_door, door_handling and bribes_solitary_food.
 */
const door_t doors[door_MAX * 2] =
{
  /* Shorthands for directions. */
#define TL direction_TOP_LEFT
#define TR direction_TOP_RIGHT
#define BR direction_BOTTOM_RIGHT
#define BL direction_BOTTOM_LEFT

  /* Shorthand for a packed room and direction byte. */
#define ROOMDIR(room, direction) (((room) << 2) | (direction))

  // room is a *destination* room index
  // direction is the direction of the door in the current room
  // pos is the position of the door in the current room

  // odd stuff:
  // rooms 28, 2, 4, 13 come out at the same place? 42, 28 -- maybe just doors in the same pos in different rooms
  // rooms 34 and 48 come out at the same place? -- ditto
  // rooms 3, 5, 23 ... 32,46
  //
  // could these be deltas rather than absolute values?

  // 0 - gate - initially locked
  { ROOMDIR(room_0_OUTDOORS,              TR), { 178, 138,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), { 178, 142,  6 } },
  // 1 - gate - initially locked
  { ROOMDIR(room_0_OUTDOORS,              TR), { 178, 122,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), { 178, 126,  6 } },
  // 2
  { ROOMDIR(room_34,                      TL), { 138, 179,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BR), {  16,  52, 12 } },
  // 3
  { ROOMDIR(room_48,                      TL), { 204, 121,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BR), {  16,  52, 12 } },
  // 4
  { ROOMDIR(room_28_HUT1LEFT,             TR), { 217, 163,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), {  42,  28, 24 } },
  // 5
  { ROOMDIR(room_1_HUT1RIGHT,             TL), { 212, 189,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BR), {  30,  46, 24 } },
  // 6
  { ROOMDIR(room_2_HUT2LEFT,              TR), { 193, 163,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), {  42,  28, 24 } },
  // 7
  { ROOMDIR(room_3_HUT2RIGHT,             TL), { 188, 189,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BR), {  32,  46, 24 } },
  // 8
  { ROOMDIR(room_4_HUT3LEFT,              TR), { 169, 163,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), {  42,  28, 24 } },
  // 9
  { ROOMDIR(room_5_HUT3RIGHT,             TL), { 164, 189,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BR), {  32,  46, 24 } },
  // 10 - current_door when in solitary
  { ROOMDIR(room_21_CORRIDOR,             TL), { 252, 202,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BR), {  28,  36, 24 } },
  // 11
  { ROOMDIR(room_20_REDCROSS,             TL), { 252, 218,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BR), {  26,  34, 24 } },
  // 12 - initially locked
  { ROOMDIR(room_15_UNIFORM,              TR), { 247, 227,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), {  38,  25, 24 } },
  // 13 - initially locked
  { ROOMDIR(room_13_CORRIDOR,             TR), { 223, 227,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), {  42,  28, 24 } },
  // 14 - initially locked
  { ROOMDIR(room_8_CORRIDOR,              TR), { 151, 211,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), {  42,  21, 24 } },
  // 15 - unused room
  { ROOMDIR(room_6,                       TR), {   0,   0,  0 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), {  34,  34, 24 } },
  // 16
  { ROOMDIR(room_1_HUT1RIGHT,             TR), {  44,  52, 24 } },
  { ROOMDIR(room_28_HUT1LEFT,             BL), {  38,  26, 24 } },
  // 17 - top right door in HUT2LEFT
  { ROOMDIR(room_3_HUT2RIGHT,             TR), {  36,  54, 24 } },
  { ROOMDIR(room_2_HUT2LEFT,              BL), {  38,  26, 24 } },
  // 18
  { ROOMDIR(room_5_HUT3RIGHT,             TR), {  36,  54, 24 } },
  { ROOMDIR(room_4_HUT3LEFT,              BL), {  38,  26, 24 } },
  // 19
  { ROOMDIR(room_23_BREAKFAST,            TR), {  40,  66, 24 } },
  { ROOMDIR(room_25_BREAKFAST,            BL), {  38,  24, 24 } },
  // 20 -
  { ROOMDIR(room_23_BREAKFAST,            TL), {  62,  36, 24 } },
  { ROOMDIR(room_21_CORRIDOR,             BR), {  32,  46, 24 } },
  // 21
  { ROOMDIR(room_19_FOOD,                 TR), {  34,  66, 24 } },
  { ROOMDIR(room_23_BREAKFAST,            BL), {  34,  28, 24 } },
  // 22 - initially locked
  { ROOMDIR(room_18_RADIO,                TR), {  36,  54, 24 } },
  { ROOMDIR(room_19_FOOD,                 BL), {  56,  34, 24 } },
  // 23
  { ROOMDIR(room_21_CORRIDOR,             TR), {  44,  54, 24 } },
  { ROOMDIR(room_22_REDKEY,               BL), {  34,  28, 24 } },
  // 24 - initially locked
  { ROOMDIR(room_22_REDKEY,               TR), {  44,  54, 24 } },
  { ROOMDIR(room_24_SOLITARY,             BL), {  42,  38, 24 } },
  // 25
  { ROOMDIR(room_12_CORRIDOR,             TR), {  66,  58, 24 } },
  { ROOMDIR(room_18_RADIO,                BL), {  34,  28, 24 } },
  // 26
  { ROOMDIR(room_17_CORRIDOR,             TL), {  60,  36, 24 } },
  { ROOMDIR(room_7_CORRIDOR,              BR), {  28,  34, 24 } },
  // 27
  { ROOMDIR(room_15_UNIFORM,              TL), {  64,  40, 24 } },
  { ROOMDIR(room_14_TORCH,                BR), {  30,  40, 24 } },
  // 28
  { ROOMDIR(room_16_CORRIDOR,             TR), {  34,  66, 24 } },
  { ROOMDIR(room_14_TORCH,                BL), {  34,  28, 24 } },
  // 29
  { ROOMDIR(room_16_CORRIDOR,             TL), {  62,  46, 24 } },
  { ROOMDIR(room_13_CORRIDOR,             BR), {  26,  34, 24 } },
  // 30 - strange outdoor-to-outdoor door definition. unused?
  { ROOMDIR(room_0_OUTDOORS,              TL), {  68,  48, 24 } },
  { ROOMDIR(room_0_OUTDOORS,              BR), {  32,  48, 24 } },
  // 31 - initially locked
  { ROOMDIR(room_13_CORRIDOR,             TL), {  74,  40, 24 } },
  { ROOMDIR(room_11_PAPERS,               BR), {  26,  34, 24 } },
  // 32
  { ROOMDIR(room_7_CORRIDOR,              TL), {  64,  36, 24 } },
  { ROOMDIR(room_16_CORRIDOR,             BR), {  26,  34, 24 } },
  // 33
  { ROOMDIR(room_10_LOCKPICK,             TL), {  54,  53, 24 } },
  { ROOMDIR(room_8_CORRIDOR,              BR), {  23,  38, 24 } },
  // 34 - initially locked
  { ROOMDIR(room_9_CRATE,                 TL), {  54,  28, 24 } },
  { ROOMDIR(room_8_CORRIDOR,              BR), {  26,  34, 24 } },
  // 35
  { ROOMDIR(room_12_CORRIDOR,             TL), {  62,  36, 24 } },
  { ROOMDIR(room_17_CORRIDOR,             BR), {  26,  34, 24 } },
  // 36
  { ROOMDIR(room_29_SECOND_TUNNEL_START,  TR), {  54,  54, 24 } },
  { ROOMDIR(room_9_CRATE,                 BL), {  56,  10, 12 } },
  // 37
  { ROOMDIR(room_52,                      TR), {  56,  98, 12 } },
  { ROOMDIR(room_30,                      BL), {  56,  10, 12 } },
  // 38
  { ROOMDIR(room_30,                      TL), { 100,  52, 12 } },
  { ROOMDIR(room_31,                      BR), {  56,  38, 12 } },
  // 39
  { ROOMDIR(room_30,                      TR), {  56,  98, 12 } },
  { ROOMDIR(room_36,                      BL), {  56,  10, 12 } },
  // 40
  { ROOMDIR(room_31,                      TL), { 100,  52, 12 } },
  { ROOMDIR(room_32,                      BR), {  10,  52, 12 } },
  // 41
  { ROOMDIR(room_32,                      TR), {  56,  98, 12 } },
  { ROOMDIR(room_33,                      BL), {  32,  52, 12 } },
  // 42
  { ROOMDIR(room_33,                      TR), {  64,  52, 12 } },
  { ROOMDIR(room_35,                      BL), {  56,  10, 12 } },
  // 43
  { ROOMDIR(room_35,                      TL), { 100,  52, 12 } },
  { ROOMDIR(room_34,                      BR), {  10,  52, 12 } },
  // 44
  { ROOMDIR(room_36,                      TL), { 100,  52, 12 } },
  { ROOMDIR(room_35,                      BR), {  56,  28, 12 } },
  // 45 - tunnel entrance
  { ROOMDIR(room_37,                      TL), {  62,  34, 24 } },
  { ROOMDIR(room_2_HUT2LEFT,              BR), {  16,  52, 12 } },
  // 46
  { ROOMDIR(room_38,                      TL), { 100,  52, 12 } },
  { ROOMDIR(room_37,                      BR), {  16,  52, 12 } },
  // 47
  { ROOMDIR(room_39,                      TR), {  64,  52, 12 } },
  { ROOMDIR(room_38,                      BL), {  32,  52, 12 } },
  // 48
  { ROOMDIR(room_40,                      TL), { 100,  52, 12 } },
  { ROOMDIR(room_38,                      BR), {  56,  84, 12 } },
  // 49
  { ROOMDIR(room_40,                      TR), {  56,  98, 12 } },
  { ROOMDIR(room_41,                      BL), {  56,  10, 12 } },
  // 50
  { ROOMDIR(room_41,                      TL), { 100,  52, 12 } },
  { ROOMDIR(room_42,                      BR), {  56,  38, 12 } },
  // 51
  { ROOMDIR(room_41,                      TR), {  56,  98, 12 } },
  { ROOMDIR(room_45,                      BL), {  56,  10, 12 } },
  // 52
  { ROOMDIR(room_45,                      TL), { 100,  52, 12 } },
  { ROOMDIR(room_44,                      BR), {  56,  28, 12 } },
  // 53
  { ROOMDIR(room_43,                      TR), {  32,  52, 12 } },
  { ROOMDIR(room_44,                      BL), {  56,  10, 12 } },
  // 54
  { ROOMDIR(room_42,                      TR), {  56,  98, 12 } },
  { ROOMDIR(room_43,                      BL), {  32,  52, 12 } },
  // 55
  { ROOMDIR(room_46,                      TL), { 100,  52, 12 } },
  { ROOMDIR(room_39,                      BR), {  56,  28, 12 } },
  // 56
  { ROOMDIR(room_47,                      TR), {  56,  98, 12 } },
  { ROOMDIR(room_46,                      BL), {  32,  52, 12 } },
  // 57
  { ROOMDIR(room_50_BLOCKED_TUNNEL,       TL), { 100,  52, 12 } },
  { ROOMDIR(room_47,                      BR), {  56,  86, 12 } },
  // 58
  { ROOMDIR(room_50_BLOCKED_TUNNEL,       TR), {  56,  98, 12 } },
  { ROOMDIR(room_49,                      BL), {  56,  10, 12 } },
  // 59
  { ROOMDIR(room_49,                      TL), { 100,  52, 12 } },
  { ROOMDIR(room_48,                      BR), {  56,  28, 12 } },
  // 60
  { ROOMDIR(room_51,                      TR), {  56,  98, 12 } },
  { ROOMDIR(room_29_SECOND_TUNNEL_START,  BL), {  32,  52, 12 } },
  // 61
  { ROOMDIR(room_52,                      TL), { 100,  52, 12 } },
  { ROOMDIR(room_51,                      BR), {  56,  84, 12 } },

#undef ROOMDIR

#undef TL
#undef TR
#undef BR
#undef BL
};

/* ----------------------------------------------------------------------- */

/**
 * $7AC9: Check for 'pick up', 'drop' and 'use' inputs.
 *
 * \param[in] state Pointer to game state.
 * \param[in] input User input event.
 */
void process_player_input_fire(tgestate_t *state, input_t input)
{
  assert(state != NULL);

  switch (input)
  {
  case input_UP_FIRE:
    pick_up_item(state);
    break;
  case input_DOWN_FIRE:
    drop_item(state);
    break;
  case input_LEFT_FIRE:
    use_item_common(state, state->items_held[0]); /* Conv: Inlined. */
    break;
  case input_RIGHT_FIRE:
    use_item_common(state, state->items_held[1]); /* Conv: Inlined. */
    break;
  case input_FIRE:
  default:
    break;
  }

  return;
}

/**
 * $7AFB: Use item common.
 *
 * \param[in] state Pointer to game state.
 */
void use_item_common(tgestate_t *state, item_t item)
{
  /**
   * $7B16: Item actions jump table.
   */
  static const item_action_t item_actions_jump_table[item__LIMIT] =
  {
    action_wiresnips,
    action_shovel,
    action_lockpick,
    action_papers,
    NULL,
    action_bribe,
    action_uniform,
    NULL,
    action_poison,
    action_red_key,
    action_yellow_key,
    action_green_key,
    action_red_cross_parcel,
    NULL,
    NULL,
    NULL,
  };

  assert(state != NULL);

  if (item == item_NONE)
    return;

  ASSERT_ITEM_VALID(item);

  memcpy(&state->saved_pos, &state->vischars[0].mi.pos, sizeof(pos_t));

  /* Conv: In the original game the action jumps to a RET for action-less
   * items. We use a NULL instead. */
  if (item_actions_jump_table[item])
    item_actions_jump_table[item](state);
}

/* ----------------------------------------------------------------------- */

/**
 * $7B36: Pick up an item.
 *
 * \param[in] state Pointer to game state.
 */
void pick_up_item(tgestate_t *state)
{
  itemstruct_t *itemstr; /* was HL */
  item_t       *pitem;   /* was DE */
  item_t        item;    /* was A */
  attribute_t   attrs;   /* was A */

  assert(state != NULL);

  if (state->items_held[0] != item_NONE &&
      state->items_held[1] != item_NONE)
    return; /* No spare slots. */

  itemstr = find_nearby_item(state);
  if (itemstr == NULL)
    return; /* No item nearby. */

  /* Locate an empty item slot. */
  pitem = &state->items_held[0];
  item = *pitem;
  if (item != item_NONE)
    pitem++;
  *pitem = itemstr->item_and_flags & (itemstruct_ITEM_MASK | itemstruct_ITEM_FLAG_UNKNOWN); // I don't see this unknown flag used anywhere else in the code. Is it a real flag, or evidence that the items pool was originally larger (ie. up to 32 objects rather than 16).

  if (state->room_index == room_0_OUTDOORS)
  {
    /* Outdoors. */
    plot_all_tiles(state);
  }
  else
  {
    /* Indoors. */
    setup_room(state);
    plot_interior_tiles(state);
    attrs = choose_game_window_attributes(state);
    set_game_window_attributes(state, attrs);
  }

  if ((itemstr->item_and_flags & itemstruct_ITEM_FLAG_HELD) == 0)
  {
    /* Have picked up an item not previously held. */
    itemstr->item_and_flags |= itemstruct_ITEM_FLAG_HELD;
    increase_morale_by_5_score_by_5(state);
  }

  itemstr->room_and_flags = 0;
  itemstr->screenpos.x    = 0;
  itemstr->screenpos.y    = 0;

  draw_all_items(state);
  play_speaker(state, sound_PICK_UP_ITEM);
}

/* ----------------------------------------------------------------------- */

/**
 * $7B8B: Drop the first item.
 *
 * \param[in] state Pointer to game state.
 */
void drop_item(tgestate_t *state)
{
  item_t      item;    /* was A */
  item_t     *itemp;   /* was HL */
  item_t      tmpitem; /* was A */
  attribute_t attrs;   /* was A */

  assert(state != NULL);

  /* Return if no items held. */
  item = state->items_held[0];
  if (item == item_NONE)
    return;

  /* When dropping the uniform reset the player sprite. */
  if (item == item_UNIFORM)
    state->vischars[0].mi.sprite = &sprites[sprite_PRISONER_FACING_AWAY_1];

  /* Shuffle items down. */
  itemp = &state->items_held[1];
  tmpitem = *itemp;
  *itemp-- = item_NONE;
  *itemp = tmpitem;

  draw_all_items(state);
  play_speaker(state, sound_DROP_ITEM);
  attrs = choose_game_window_attributes(state);
  set_game_window_attributes(state, attrs);

  drop_item_tail(state, item);
}

/**
 * $7BB5: Drop item, tail part.
 *
 * Called by drop_item and action_red_cross_parcel.
 *
 * \param[in] state Pointer to game state.
 * \param[in] item  Item. (was A)
 */
void drop_item_tail(tgestate_t *state, item_t item)
{
  itemstruct_t *itemstr; /* was HL */
  room_t        room;    /* was A */
  tinypos_t    *outpos;  /* was DE */
  pos_t        *inpos;   /* was HL */

  assert(state != NULL);
  ASSERT_ITEM_VALID(item);

  itemstr = item_to_itemstruct(state, item);
  room = state->room_index;
  itemstr->room_and_flags = room; /* Set object's room index. */
  if (room == room_0_OUTDOORS)
  {
    /* Outdoors. */

    // TODO: Hoist common code.
    outpos = &itemstr->pos;
    inpos  = &state->vischars[0].mi.pos;

    pos_to_tinypos(inpos, outpos);
    outpos->height = 0;

    calc_exterior_item_screenpos(itemstr);
  }
  else
  {
    /* $7BE4: Drop item, interior part. */

    outpos = &itemstr->pos;
    inpos  = &state->vischars[0].mi.pos;

    outpos->x      = inpos->x; // note: narrowing
    outpos->y      = inpos->y; // note: narrowing
    outpos->height = 5;

    calc_interior_item_screenpos(itemstr);
  }
}

/**
 * $7BD0: Calculate screen position for exterior item.
 *
 * Called by drop_item_tail and item_discovered.
 *
 * \param[in] itemstr Pointer to item struct. (was HL)
 */
void calc_exterior_item_screenpos(itemstruct_t *itemstr)
{
  xy_t *screenpos;

  assert(itemstr != NULL);

  screenpos = &itemstr->screenpos;

  screenpos->x = (0x40 - itemstr->pos.x + itemstr->pos.y) * 2;
  screenpos->y = 0x100 - itemstr->pos.x - itemstr->pos.y - itemstr->pos.height; /* Conv: 0x100 is 0 in the original. */
}

/**
 * $7BF2: Calculate screen position for interior item.
 *
 * Called by drop_item_tail and item_discovered.
 *
 * \param[in] itemstr Pointer to item struct. (was HL)
 */
void calc_interior_item_screenpos(itemstruct_t *itemstr)
{
  xy_t *screenpos;

  assert(itemstr != NULL);

  screenpos = &itemstr->screenpos;

  // This was a call to divide_by_8_with_rounding, but that expects
  // separate hi and lo arguments, which is not worth the effort of
  // mimicing the original code.
  //
  // This needs to go somewhere more general.
#define divround(x) (((x) + 4) >> 3)

  screenpos->x = divround((0x200 - itemstr->pos.x + itemstr->pos.y) * 2);
  screenpos->y = divround(0x800 - itemstr->pos.x - itemstr->pos.y - itemstr->pos.height);

#undef divround
}

/* ----------------------------------------------------------------------- */

/**
 * $7C26: Convert an item to an itemstruct pointer.
 *
 * \param[in] item Item index. (was A)
 *
 * \return Pointer to itemstruct.
 */
itemstruct_t *item_to_itemstruct(tgestate_t *state, item_t item)
{
  assert(state != NULL);
  ASSERT_ITEM_VALID(item);

  return &state->item_structs[item];
}

/* ----------------------------------------------------------------------- */

/**
 * $7C33: Draw both held items.
 *
 * \param[in] state Pointer to game state.
 */
void draw_all_items(tgestate_t *state)
{
  assert(state != NULL);

  /* Conv: Converted screen pointers into offsets. */
  draw_item(state, state->items_held[0], 0x5087 - SCREEN_START_ADDRESS);
  draw_item(state, state->items_held[1], 0x508A - SCREEN_START_ADDRESS);
}

/**
 * $7C46: Draw a single held item.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] item   Item index.    (was A)
 * \param[in] dstoff Screen offset. (was HL) Assumed to be in the lower third of the display.
 */
void draw_item(tgestate_t *state, item_t item, size_t dstoff)
{
  assert(state != NULL);

  uint8_t *const screen = &state->speccy->screen[0];

  attribute_t       *attrs;  /* was HL */
  attribute_t        attr;   /* was A */
  const spritedef_t *sprite; /* was HL */

  /* Wipe item. */
  screen_wipe(state, 2, 16, screen + dstoff);

  if (item == item_NONE)
    return;

  ASSERT_ITEM_VALID(item);

  /* Set screen attributes. */
  attrs = (dstoff & 0xFF) + &state->speccy->attributes[0x5A00 - SCREEN_ATTRIBUTES_START_ADDRESS];
  attr = state->item_attributes[item];
  attrs[0] = attr;
  attrs[1] = attr;

  /* Move to next attribute row. */
  attrs += state->width;
  attrs[0] = attr;
  attrs[1] = attr;

  /* Plot the item bitmap. */
  sprite = &item_definitions[item];
  plot_bitmap(state, sprite->width, sprite->height, sprite->bitmap, screen + dstoff);
}

/* ----------------------------------------------------------------------- */

/**
 * $7C82: Returns an item within range of the hero.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Pointer to itemstruct, or NULL.
 */
itemstruct_t *find_nearby_item(tgestate_t *state)
{
  uint8_t       radius;  /* was C */
  uint8_t       iters;   /* was B */
  itemstruct_t *itemstr; /* was HL */

  assert(state != NULL);

  /* Select a pick up radius. */
  radius = 1; /* Outdoors. */
  if (state->room_index > room_0_OUTDOORS)
    radius = 6; /* Indoors. */

  /* For all items. */
  iters   = item__LIMIT;
  itemstr = &state->item_structs[0];
  do
  {
    if (itemstr->room_and_flags & itemstruct_ROOM_FLAG_NEARBY_7)
    {
      uint8_t *structcoord; /* was HL */
      uint8_t *herocoord;   /* was DE */
      uint8_t  coorditers;  /* was B */

      // FIXME: Candidate for loop unrolling.
      structcoord = &itemstr->pos.x;
      herocoord   = &state->hero_map_position.x;
      coorditers = 2;
      /* Range check. */
      do
      {
        uint8_t A;

        A = *herocoord++; /* get hero map position */
        if (A - radius >= *structcoord || A + radius < *structcoord)
          goto next;
        structcoord++;
      }
      while (--coorditers);

      return itemstr;
    }

next:
    itemstr++;
  }
  while (--iters);

  return NULL;
}

/* ----------------------------------------------------------------------- */

/**
 * $7CBE: Plot a bitmap without masking.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] width  Width, in bytes.     (was B)
 * \param[in] height Height.              (was C)
 * \param[in] src    Source address.      (was DE)
 * \param[in] dst    Destination address. (was HL)
 */
void plot_bitmap(tgestate_t    *state,
                 uint8_t        width,
                 uint8_t        height,
                 const uint8_t *src,
                 uint8_t       *dst)
{
  assert(state != NULL);
  assert(width  > 0);
  assert(height > 0);
  assert(src   != NULL);
  assert(dst   != NULL);

  do
  {
    memcpy(dst, src, width);
    src += width;
    dst = get_next_scanline(state, dst);
  }
  while (--height);
}

/* ----------------------------------------------------------------------- */

/**
 * $7CD4: Wipe the screen.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] width  Width, in bytes.     (was B)
 * \param[in] height Height.              (was C)
 * \param[in] dst    Destination address. (was HL)
 */
void screen_wipe(tgestate_t *state,
                 uint8_t     width,
                 uint8_t     height,
                 uint8_t    *dst)
{
  assert(state != NULL);
  assert(width  > 0);
  assert(height > 0);
  assert(dst   != NULL);

  do
  {
    memset(dst, 0, width);
    dst = get_next_scanline(state, dst);
  }
  while (--height);
}

/* ----------------------------------------------------------------------- */

/**
 * $7CE9: Given a screen address, return the same position on the next
 * scanline.
 *
 * \param[in] state Pointer to game state.
 * \param[in] slp   Scanline pointer.      (was HL)
 *
 * \return Subsequent scanline pointer.
 */
uint8_t *get_next_scanline(tgestate_t *state, uint8_t *slp)
{
  assert(state != NULL);
  assert(slp   != NULL);

  uint8_t *const screen = &state->speccy->screen[0]; /* added */
  uint16_t       offset; /* was HL */
  uint16_t       delta;  /* was DE */

  offset = slp - screen; // note: narrowing

  assert(offset < 0x8000);

  offset += 0x0100;
  if (offset & 0x0700)
    return screen + offset; /* line count didn't rollover */

  if ((offset & 0xFF) >= 0xE0)
    delta = 0xFF20; /* -224 */
  else
    delta = 0xF820; /* -2016 */

  offset += delta;

  return screen + (int16_t) offset;
}

/* ----------------------------------------------------------------------- */

/**
 * $9D7B: Main game loop.
 */
void main_loop(tgestate_t *state)
{
  assert(state != NULL);

  check_morale(state);
  keyscan_break(state);
  message_display(state);
  process_player_input(state);
  in_permitted_area(state);
  restore_tiles(state);
  move_characters(state);
  follow_suspicious_character(state);
  purge_invisible_characters(state);
  spawn_characters(state);
  mark_nearby_items(state);
  ring_bell(state);
  called_from_main_loop_9(state);
  move_map(state);
  message_display(state); /* second */
  ring_bell(state); /* second */
  plot_sprites(state);
  plot_game_window(state);
  ring_bell(state); /* third */
  if (state->day_or_night != 0)
    nighttime(state);
  if (state->room_index > room_0_OUTDOORS)
    interior_delay_loop(state);
  wave_morale_flag(state);
  if ((state->game_counter & 63) == 0)
    dispatch_timed_event(state);

  // This is temporary: each part of the code which updates the display
  // should call the draw entry point with a correct dirty rectangle.
  //
  // Might be a better idea to save up all dirty rects then dispatch them in
  // one go each time around this loop.
  state->speccy->draw(state->speccy, NULL);

  // Conv: Added this overall slowdown factor.
  // Perhaps I should run main_loop on a timer instead of pratting around with sleep calls.
  state->speccy->sleep(state->speccy, sleeptype_DELAY, 40000);
}

/* ----------------------------------------------------------------------- */

/**
 * $9DCF: Check morale level, report if (near) zero and inhibit player
 * control.
 *
 * \param[in] state Pointer to game state.
 */
void check_morale(tgestate_t *state)
{
  assert(state != NULL);

  if (state->morale >= 2)
    return;

  queue_message_for_display(state, message_MORALE_IS_ZERO);

  /* Inhibit user input. */
  state->morale_exhausted = 255;

  /* Immediately assume automatic control of hero. */
  state->automatic_player_counter = 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $9DE5: Check for BREAK keypress.
 *
 * If pressed then clear the screen and confirm with the player that they
 * want to reset the game. Reset if requested.
 *
 * \param[in] state Pointer to game state.
 *
 * \remarks Exits using longjmp if the game is canceled, or indoors.
 */
void keyscan_break(tgestate_t *state)
{
  int space;
  int shift;

  assert(state != NULL);

  /* Is shift-space (break) pressed? */
  space = (state->speccy->in(state->speccy, port_KEYBOARD_SPACESYMSHFTMNB) & 1) == 0;
  shift = (state->speccy->in(state->speccy, port_KEYBOARD_SHIFTZXCV)       & 1) == 0;
  if (!space || !shift)
    return; /* not pressed */

  screen_reset(state);
  if (user_confirm(state) == 0)
  {
    reset_game(state);
    NEVER_RETURNS;
  }

  if (state->room_index == room_0_OUTDOORS)
  {
    reset_outdoors(state);
  }
  else
  {
    enter_room(state); /* doesn't return (jumps to main_loop) */
    NEVER_RETURNS;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $9E07: Process player input.
 *
 * \param[in] state Pointer to game state.
 */
void process_player_input(tgestate_t *state)
{
  input_t input; /* was A */

  assert(state != NULL);

  if (state->in_solitary || state->morale_exhausted)
    return; /* Don't allow input. */

  if (state->vischars[0].flags & (vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE))
  {
    /* Hero is picking a lock, or cutting through a wire fence. */

    /* Postpone automatic control for 31 turns. */
    state->automatic_player_counter = 31;

    if (state->vischars[0].flags == vischar_FLAGS_PICKING_LOCK)
      picking_lock(state);
    else
      cutting_wire(state);

    return;
  }

  input = input_routine(state);
  if (input == input_NONE)
  {
    /* No input */

    if (state->automatic_player_counter == 0)
      return;

    /* No player input: count down. */
    state->automatic_player_counter--;
    input = input_NONE;
  }
  else
  {
    /* Input received */

    /* Postpone automatic control for 31 turns. */
    state->automatic_player_counter = 31;

    if (state->hero_in_bed || state->hero_in_breakfast)
    {
//      printf("hero at breakfast %d\n", state->hero_in_breakfast);
//      printf("hero in bed %d\n", state->hero_in_bed);

      assert(state->hero_in_bed == 0 || state->hero_in_breakfast == 0);

      if (state->hero_in_bed == 0)
      {
        /* Hero was at breakfast. */
        state->vischars[0].route.index    = 43;
        state->vischars[0].route.step     = 0;
        state->vischars[0].mi.pos.x       = 52;
        state->vischars[0].mi.pos.y       = 62;
        roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_EMPTY_BENCH;
        state->hero_in_breakfast = 0;
      }
      else
      {
        /* Hero was in bed. */
        state->vischars[0].route.index    = 44;
        state->vischars[0].route.step     = 1;
        state->vischars[0].pos.x          = 46;
        state->vischars[0].pos.y          = 46;
        state->vischars[0].mi.pos.x       = 46;
        state->vischars[0].mi.pos.y       = 46;
        state->vischars[0].mi.pos.height  = 24;
        roomdef_2_hut2_left[roomdef_2_BED] = interiorobject_EMPTY_BED_FACING_SE;
        state->hero_in_bed = 0;
      }

      setup_room(state);
      plot_interior_tiles(state);
    }

    if (input >= input_FIRE)
    {
      process_player_input_fire(state, input);
      input = input_KICK;
    }
  }

  /* If input state has changed then kick a sprite update. */
  if (state->vischars[0].input != input)
    state->vischars[0].input = input | input_KICK;
}

/* ----------------------------------------------------------------------- */

/**
 * $9E98: Locks the player out until the lock is picked.
 *
 * \param[in] state Pointer to game state.
 */
void picking_lock(tgestate_t *state)
{
  assert(state != NULL);

  if (state->player_locked_out_until != state->game_counter)
    return;

  /* Countdown reached: Unlock the door. */
  *state->ptr_to_door_being_lockpicked &= ~door_LOCKED;
  queue_message_for_display(state, message_IT_IS_OPEN);

  state->vischars[0].flags &= ~(vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE);
}

/* ----------------------------------------------------------------------- */

/**
 * $9EB2: Locks the player out until the fence is cut.
 *
 * \param[in] state Pointer to game state.
 */
void cutting_wire(tgestate_t *state)
{
  /**
   * $9EE0: New inputs table.
   */
  static const uint8_t new_inputs[4] =
  {
    input_UP   + input_LEFT  + input_KICK,
    input_UP   + input_RIGHT + input_KICK,
    input_DOWN + input_RIGHT + input_KICK,
    input_DOWN + input_LEFT  + input_KICK
  };

  vischar_t *hero = &state->vischars[0]; /* new, for conciseness */
  int        delta; /* was A */

  assert(state != NULL);

  delta = state->player_locked_out_until - state->game_counter;
  if (delta)
  {
    if (delta < 4)
      hero->input = new_inputs[hero->direction & vischar_DIRECTION_MASK];
  }
  else
  {
    /* Countdown reached: Snip the wire. */
    /* Bug: 'delta' is always zero here, so 'hero->direction' is always set
     * to zero. */
    hero->direction = delta & vischar_DIRECTION_MASK;
    hero->input = input_KICK;
    hero->mi.pos.height = 24;

    /* Conv: The original code jumps into the tail end of picking_lock()
     * above to do this. */
    hero->flags &= ~(vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $9F21: In permitted area.
 *
 * \param[in] state Pointer to game state.
 */
void in_permitted_area(tgestate_t *state)
{
#define R (1<<7) // specifies a room index

  /**
   * $9EF9: Variable-length arrays, 255 terminated.
   */
  static const uint8_t byte_9EF9[] = { R| 2, R|  2,   255                         };
  static const uint8_t byte_9EFC[] = { R| 3,     1,     1,     1,   255           };
  static const uint8_t byte_9F01[] = {    1,     1,     1,     0,     2,   2, 255 };
  static const uint8_t byte_9F08[] = {    1,     1, R| 21, R| 23, R| 25, 255      }; // breakfasty?
  static const uint8_t byte_9F0E[] = { R| 3, R|  2,   255                         };
  static const uint8_t byte_9F11[] = { R|25,   255                                };
  static const uint8_t byte_9F13[] = {    1,   255                                };

#undef T

  /**
   * $9EE4: Maps route indicies to pointers to the above arrays.
   *
   * Suspect that this means "if you're in <this> route you can be in <these> places."
   */
  static const byte_to_pointer_t byte_to_pointer[7] =
  {
    { 42, &byte_9EF9[0] },
    {  5, &byte_9EFC[0] },
    { 14, &byte_9F01[0] },
    { 16, &byte_9F08[0] },
    { 44, &byte_9F0E[0] },
    { 43, &byte_9F11[0] },
    { 45, &byte_9F13[0] },
  };

  pos_t       *vcpos;      /* was HL */
  tinypos_t   *pos;        /* was DE */
  attribute_t  attr;       /* was A */
  uint8_t      routeindex; /* was A */
  route_t      route;      /* was CA */
  uint16_t     BC;         /* was BC */
  uint8_t      red_flag;   /* was A */

  assert(state != NULL);

  vcpos = &state->vischars[0].mi.pos;
  pos = &state->hero_map_position;
  if (state->room_index == room_0_OUTDOORS)
  {
    /* Outdoors. */

    pos_to_tinypos(vcpos, pos);

    /* (217 * 8, 137 * 8) */
    if (state->vischars[0].floogle.x >= 0x06C8 ||
        state->vischars[0].floogle.y >= 0x0448)
    {
      escaped(state);
      return;
    }
  }
  else
  {
    /* Indoors. */

    pos->x      = vcpos->x; // note: narrowing
    pos->y      = vcpos->y; // note: narrowing
    pos->height = vcpos->height; // note: narrowing
  }

  /* Red flag if picking a lock, or cutting wire. */

  if (state->vischars[0].flags & (vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE))
    goto set_flag_red;

  /* At night, home room is the only safe place. */

  if (state->clock >= 100) /* night time */
  {
    if (state->room_index == room_2_HUT2LEFT)
      goto set_flag_green;
    else
      goto set_flag_red;
  }

  /* If in solitary then bypass all checks. */

  if (state->in_solitary)
    goto set_flag_green;

  route = state->vischars[0].route;
  if (route.index & route_REVERSED)
    route.step++;

  if (route.index == route_WANDER)
  {
    uint8_t area; /* was A */

    // 1 => Hut area
    // 2 => Yard area
    area = ((route.step & ~7) == 8) ? 1 : 2;
    if (in_permitted_area_end_bit(state, area) == 0) /* within bounds */
      goto set_flag_green;
    else
      goto set_flag_red;
  }
  else // en route
  {
    const byte_to_pointer_t *tab;   /* was HL */
    uint8_t                  iters; /* was B */
    const uint8_t           *HL;

    // A is a route index
    routeindex = route.index & ~route_REVERSED; // added to coax A back from route
    tab = &byte_to_pointer[0]; // table mapping bytes to offsets
    iters = NELEMS(byte_to_pointer);
    do
    {
      if (routeindex == tab->byte)
        goto found;
      tab++;
    }
    while (--iters);

    // anything not found in byte_to_pointer => green flag
    goto set_flag_green;

found:
    // route.index (route index) was found in byte_to_pointer

    HL = tab->pointer;
    // loc.y = 0; // not needed
    HL += route.step; // original code used B=0
    // *HL must be a room_and_flags... unsure
    if (in_permitted_area_end_bit(state, *HL) == 0) /* within bounds */
      goto set_flag_green;

    if (state->vischars[0].route.index & route_REVERSED) // FUTURE: route index is already in 'route'
      HL++;

    BC = 0;
    for (;;)
    {
      uint8_t foo; /* was A */

      foo = HL[BC]; // likely; A is room_and_flags
      if (foo == 255) /* hit end of list */
        goto pop_and_set_flag_red;
      if (in_permitted_area_end_bit(state, foo) == 0)
        goto set_route_then_set_flag_green;
      BC++;
    }

set_route_then_set_flag_green:
    {
      route_t route2; /* was BC */

      assert(BC < 256);

      route2.index = state->vischars[0].route.index;
      route2.step  = BC; /* loop counter */
      set_hero_route(state, route2);
    }
    goto set_flag_green;

    // FUTURE remove this needless jump
pop_and_set_flag_red:
    goto set_flag_red;
  }

  /* Green flag code path. */
set_flag_green:
  red_flag = 0;
  attr = attribute_BRIGHT_GREEN_OVER_BLACK;

flag_select:
  state->red_flag = red_flag;
  if (attr == state->speccy->attributes[morale_flag_attributes_offset])
    return; /* flag is already the correct colour */

  if (attr == attribute_BRIGHT_GREEN_OVER_BLACK)
    state->bell = bell_STOP;

  set_morale_flag_screen_attributes(state, attr);
  return;

  /* Red flag code path. */
set_flag_red:
  attr = attribute_BRIGHT_RED_OVER_BLACK;
  if (state->speccy->attributes[morale_flag_attributes_offset] == attr)
    return; /* flag is already red */

  state->vischars[0].input = 0;
  red_flag = 255;
  goto flag_select;
}

/**
 * $A007: In permitted area (end bit).
 *
 * \param[in] state          Pointer to game state.
 * \param[in] room_and_flags If bit 7 is set then bits 0..6 contain a room index. Otherwise it's an area index as passed into within_camp_bounds. (was A)
 *
 * \return 0 if the position is within the area bounds, 1 otherwise.
 */
int in_permitted_area_end_bit(tgestate_t *state, uint8_t room_and_flags)
{
  room_t *proom; /* was HL */

  assert(state != NULL);

  // FUTURE: Just dereference proom once.

  proom = &state->room_index;

  if (room_and_flags & (1 << 7)) // flag means a room is specified
  {
    assert((room_and_flags & ~(1 << 7)) < room__LIMIT);
    return *proom == (room_and_flags & ~(1 << 7)); /* Return room only. */
  }

  if (*proom != room_0_OUTDOORS)
    return 0; /* Indoors. */

  return within_camp_bounds(room_and_flags, &state->hero_map_position);
}

/**
 * $A01A: Is the specified position within the bounds of the indexed area?
 *
 * \param[in] area Index (0..2) into permitted_bounds[] table. (was A)
 * \param[in] pos  Pointer to position. (was HL)
 *
 * \return 0 if the position is within the area bounds, 1 otherwise.
 */
int within_camp_bounds(uint8_t          area, // ought to be an enum
                       const tinypos_t *pos)
{
  /**
   * $9F15: Boundings of the three main exterior areas.
   */
  static const bounds_t permitted_bounds[3] =
  {
    { 86, 94, 61, 72 }, /* Corridor to yard */
    { 78,132, 71,116 }, /* Hut area */
    { 79,105, 47, 63 }, /* Yard area */
  };

  const bounds_t *bounds; /* was HL */

  assert(area < NELEMS(permitted_bounds));
  assert(pos != NULL);

  bounds = &permitted_bounds[area];
  return pos->x < bounds->x0 || pos->x >= bounds->x1 ||
         pos->y < bounds->y0 || pos->y >= bounds->y1;
}

/* ----------------------------------------------------------------------- */

/**
 * $A035: Wave the morale flag.
 *
 * \param[in] state Pointer to game state.
 */
void wave_morale_flag(tgestate_t *state)
{
  uint8_t       *pgame_counter;     /* was HL */
  uint8_t        morale;            /* was A */
  uint8_t       *pdisplayed_morale; /* was HL */
  const uint8_t *flag_bitmap;       /* was DE */

  assert(state != NULL);

  pgame_counter = &state->game_counter;
  (*pgame_counter)++;

  /* Wave the flag on every other turn. */
  if (*pgame_counter & 1)
    return;

  morale = state->morale;
  pdisplayed_morale = &state->displayed_morale;
  if (morale != *pdisplayed_morale)
  {
    if (morale < *pdisplayed_morale)
    {
      /* Decreasing morale. */
      (*pdisplayed_morale)--;
      pdisplayed_morale = get_next_scanline(state, state->moraleflag_screen_address);
    }
    else
    {
      /* Increasing morale. */
      (*pdisplayed_morale)++;
      pdisplayed_morale = get_prev_scanline(state, state->moraleflag_screen_address);
    }
    state->moraleflag_screen_address = pdisplayed_morale;
  }

  flag_bitmap = flag_down;
  if (*pgame_counter & 2)
    flag_bitmap = flag_up;
  plot_bitmap(state, 3, 25, flag_bitmap, state->moraleflag_screen_address);
}

/* ----------------------------------------------------------------------- */

/**
 * $A071: Set the screen attributes of the morale flag.
 *
 * \param[in] state Pointer to game state.
 * \param[in] attrs Screen attributes to use.
 */
void set_morale_flag_screen_attributes(tgestate_t *state, attribute_t attrs)
{
  attribute_t *pattrs; /* was HL */
  int          iters;  /* was B */

  assert(state != NULL);
  assert(attrs >= attribute_BLUE_OVER_BLACK && attrs <= attribute_BRIGHT_WHITE_OVER_BLACK);

  pattrs = &state->speccy->attributes[morale_flag_attributes_offset];
  iters = 19; /* Height of flag. */
  do
  {
    pattrs[0] = attrs;
    pattrs[1] = attrs;
    pattrs[2] = attrs;
    pattrs += state->width;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A082: Given a screen address, returns the same position on the previous
 * scanline.
 *
 * \param[in] state Pointer to game state.
 * \param[in] addr  Original screen address.
 *
 * \return Updated screen address.
 */
uint8_t *get_prev_scanline(tgestate_t *state, uint8_t *addr)
{
  assert(state != NULL);
  assert(addr  != NULL);

  uint8_t *const screen = &state->speccy->screen[0];
  ptrdiff_t      raddr = addr - screen;

  if ((raddr & 0x0700) != 0)
  {
    // NNN bits
    // Step back one scanline.
    raddr -= 256;
  }
  else
  {
    if ((raddr & 0x00FF) < 32)
      raddr -= 32;
    else
      // Complicated.
      raddr += 0x06E0;
  }

  return screen + raddr;
}

/* ----------------------------------------------------------------------- */

/**
 * $A095: Delay loop called only when the hero is indoors.
 */
void interior_delay_loop(tgestate_t *state)
{
  assert(state != NULL);

  state->speccy->sleep(state->speccy, sleeptype_DELAY, 4096 * 16);

// Conv: Was:
//  volatile int BC = 0xFFF;
//  while (--BC)
//    ;
}

/* ----------------------------------------------------------------------- */

/* Offset. */
#define screenoffset_BELL_RINGER 0x118E

/**
 * $A09E: Ring the alarm bell.
 *
 * \param[in] state Pointer to game state.
 */
void ring_bell(tgestate_t *state)
{
  /**
   * $A147: Bell ringer bitmaps.
   */
  static const uint8_t bell_ringer_bitmap_off[] =
  {
    0xE7, 0xE7, 0x83, 0x83, 0x43, 0x41, 0x20, 0x10, 0x08, 0x04, 0x02, 0x02
  };
  static const uint8_t bell_ringer_bitmap_on[] =
  {
    0x3F, 0x3F, 0x27, 0x13, 0x13, 0x09, 0x08, 0x04, 0x04, 0x02, 0x02, 0x01
  };

  bellring_t *pbell; /* was HL */
  bellring_t  bell;  /* was A */

  assert(state != NULL);

  pbell = &state->bell;
  bell = *pbell;
  if (bell == bell_STOP)
    return; /* Not ringing */

  if (bell != bell_RING_PERPETUAL)
  {
    /* Decrement the ring counter. */
    *pbell = --bell;
    if (bell == 0)
    {
      /* Counter hit zero - stop ringing. */
      *pbell = bell_STOP;
      return;
    }
  }

  /* Fetch visible state of bell. */
  bell = state->speccy->screen[screenoffset_BELL_RINGER];
  if (bell != 0x3F) /* Pixel value is 0x3F if on */
  {
    /* Plot ringer "on". */
    plot_ringer(state, bell_ringer_bitmap_on);
    play_speaker(state, sound_BELL_RINGER);
  }
  else
  {
    /* Plot ringer "off". */
    plot_ringer(state, bell_ringer_bitmap_off);
  }
}

/**
 * $A0C9: Plot ringer.
 *
 * \param[in] state Pointer to game state.
 * \param[in] src   Source bitmap. (was HL)
 */
void plot_ringer(tgestate_t *state, const uint8_t *src)
{
  assert(state != NULL);
  assert(src   != NULL);

  plot_bitmap(state,
              1, 12, /* dimensions: 8 x 12 */
              src,
              &state->speccy->screen[screenoffset_BELL_RINGER]);
}

/* ----------------------------------------------------------------------- */

/**
 * $A0D2: Increase morale.
 *
 * \param[in] state Pointer to game state.
 * \param[in] delta Amount to increase morale by. (was B)
 */
void increase_morale(tgestate_t *state, uint8_t delta)
{
  int increased_morale;

  assert(state != NULL);
  assert(delta > 0);

  increased_morale = state->morale + delta;
  if (increased_morale >= morale_MAX)
    increased_morale = morale_MAX;

  assert(increased_morale >= morale_MIN);
  assert(increased_morale <= morale_MAX);

  state->morale = increased_morale;
}

/**
 * $A0E0: Decrease morale.
 *
 * \param[in] state Pointer to game state.
 * \param[in] delta Amount to decrease morale by. (was B)
 */
void decrease_morale(tgestate_t *state, uint8_t delta)
{
  int decreased_morale;

  assert(state != NULL);
  assert(delta > 0);

  decreased_morale = state->morale - delta;
  if (decreased_morale < morale_MIN)
    decreased_morale = morale_MIN;

  assert(decreased_morale >= morale_MIN);
  assert(decreased_morale <= morale_MAX);

  /* Conv: This jumps into the tail end of increase_morale in the original
   * code. */
  state->morale = decreased_morale;
}

/**
 * $A0E9: Increase morale by 10, score by 50.
 *
 * \param[in] state Pointer to game state.
 */
void increase_morale_by_10_score_by_50(tgestate_t *state)
{
  assert(state != NULL);

  increase_morale(state, 10);
  increase_score(state, 50);
}

/**
 * $A0F2: Increase morale by 5, score by 5.
 *
 * \param[in] state Pointer to game state.
 */
void increase_morale_by_5_score_by_5(tgestate_t *state)
{
  assert(state != NULL);

  increase_morale(state, 5);
  increase_score(state, 5);
}

/* ----------------------------------------------------------------------- */

/**
 * $A0F9: Increases the score then plots it.
 *
 * \param[in] state Pointer to game state.
 * \param[in] delta Amount to increase score by. (was B)
 */
void increase_score(tgestate_t *state, uint8_t delta)
{
  char *pdigit; /* was HL */

  assert(state != NULL);
  assert(delta > 0);

  /* Increment the score digit-wise until delta is zero. */

  assert(state->score_digits[0] <= 9);
  assert(state->score_digits[1] <= 9);
  assert(state->score_digits[2] <= 9);
  assert(state->score_digits[3] <= 9);
  assert(state->score_digits[4] <= 9);

  pdigit = &state->score_digits[4];
  do
  {
    char *tmp; /* was HL */

    tmp = pdigit;
    for (;;)
    {
      (*pdigit)++;
      if (*pdigit < 10)
        break;

      *pdigit-- = 0;
    }
    pdigit = tmp;
  }
  while (--delta);

  plot_score(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A10B: Draws the current score to screen.
 *
 * \param[in] state Pointer to game state.
 */
void plot_score(tgestate_t *state)
{
  char    *digits; /* was HL */
  uint8_t *screen; /* was DE */
  uint8_t  iters;  /* was B */

  assert(state != NULL);

  digits = &state->score_digits[0];
  screen = &state->speccy->screen[score_address];
  iters = NELEMS(state->score_digits);
  do
  {
    char digit = '0' + *digits; /* Conv: Pass as ASCII. */

    screen = plot_glyph(&digit, screen);
    digits++;
    screen++; /* Additionally to plot_glyph, so screen += 2 each iter. */
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A11D: Plays a sound.
 *
 * \param[in] state Pointer to game state.
 * \param[in] sound Number of iterations to play for (hi). Delay inbetween each iteration (lo). (was BC)
 */
void play_speaker(tgestate_t *state, sound_t sound)
{
  uint8_t iters;      /* was B */
  uint8_t delay;      /* was A */
  uint8_t speakerbit; /* was A */
//  uint8_t subcount;   /* was C */

  assert(state != NULL);
  // assert(sound); somehow

  iters = sound >> 8;
  delay = sound & 0xFF;

  speakerbit = 16; /* Initial speaker bit. */
  do
  {
    state->speccy->out(state->speccy, port_BORDER, speakerbit); /* Play. */

    /* Conv: Removed self-modified counter. */
    state->speccy->sleep(state->speccy, sleeptype_SOUND, delay);

    speakerbit ^= 16; /* Toggle speaker bit. */
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A15F: Set game window attributes.
 *
 * \param[in] state Pointer to game state.
 * \param[in] attrs Screen attributes.     (was A)
 */
void set_game_window_attributes(tgestate_t *state, attribute_t attrs)
{
  attribute_t *attributes; /* was HL */
  uint8_t      rows;       /* was C */
  uint16_t     stride;     /* was DE */

  assert(state != NULL);
  assert(attrs >= attribute_BLUE_OVER_BLACK && attrs <= attribute_BRIGHT_WHITE_OVER_BLACK);

  attributes = &state->speccy->attributes[0x0047];
  rows   = state->rows - 1;
  stride = state->width - (state->columns - 1); /* e.g. 32 - 23 = 9 */
  do
  {
    uint8_t iters;

    iters = state->columns - 1; /* e.g. 23 */
    do
      *attributes++ = attrs;
    while (--iters);
    attributes += stride;
  }
  while (--rows);
}

/* ----------------------------------------------------------------------- */

/**
 * $A1A0: Dispatch timed events.
 *
 * \param[in] state Pointer to game state.
 */
void dispatch_timed_event(tgestate_t *state)
{
  /**
   * $A173: Timed events.
   */
  static const timedevent_t timed_events[15] =
  {
    {   0, &event_another_day_dawns    },
    {   8, &event_wake_up              },
    {  12, &event_new_red_cross_parcel },
    {  16, &event_go_to_roll_call      },
    {  20, &event_roll_call            },
    {  21, &event_go_to_breakfast_time },
    {  36, &event_end_of_breakfast     },
    {  46, &event_go_to_exercise_time  },
    {  64, &event_exercise_time        },
    {  74, &event_go_to_roll_call      },
    {  78, &event_roll_call            },
    {  79, &event_go_to_time_for_bed   },
    {  98, &event_time_for_bed         },
    { 100, &event_night_time           },
    { 130, &event_search_light         },
  };

  eventtime_t        *pcounter; /* was HL */
  eventtime_t         time;     /* was A */
  const timedevent_t *event;    /* was HL */
  uint8_t             iters;    /* was B */

  assert(state != NULL);

  /* Increment the clock, wrapping at 140. */
  pcounter = &state->clock;
  time = *pcounter + 1;
  if (time == 140) // hoist 140 out to time_MAX or somesuch
    time = 0;
  *pcounter = time;

  /* Dispatch the event for that time. */
  event = &timed_events[0];
  iters = NELEMS(timed_events);
  do
  {
    if (time == event->time)
      goto found; // in future rewrite to avoid the goto
    event++;
  }
  while (--iters);

  return;

found:
  event->handler(state);
}

void event_night_time(tgestate_t *state)
{
  assert(state != NULL);

  if (state->hero_in_bed == 0)
  {
    const route_t t = { 44, 1 }; /* was BC */
    set_hero_route(state, t);
  }
  set_day_or_night(state, 255);
}

void event_another_day_dawns(tgestate_t *state)
{
  assert(state != NULL);

  queue_message_for_display(state, message_ANOTHER_DAY_DAWNS);
  decrease_morale(state, 25);
  set_day_or_night(state, 0x00);
}

/**
 * $A1DE: Tail end of above two routines.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] day_night Day or night flag. (was A)
 */
void set_day_or_night(tgestate_t *state, uint8_t day_night)
{
  attribute_t attrs;

  assert(state != NULL);
  assert(day_night == 0 || day_night == 255);

  state->day_or_night = day_night; // night=255, day=0
  attrs = choose_game_window_attributes(state);
  set_game_window_attributes(state, attrs);
}

void event_wake_up(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_TIME_TO_WAKE_UP);
  wake_up(state);
}

void event_go_to_roll_call(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_ROLL_CALL);
  go_to_roll_call(state);
}

void event_go_to_breakfast_time(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_BREAKFAST_TIME);
  set_route_0x1000(state);
}

void event_end_of_breakfast(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  end_of_breakfast(state);
}

void event_go_to_exercise_time(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_EXERCISE_TIME);

  /* Unlock the gates. */
  state->locked_doors[0] = 0; /* Index into doors + clear locked flag. */
  state->locked_doors[1] = 1;

  set_route_0x0E00(state);
}

void event_exercise_time(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  set_route_0x8E04(state);
}

void event_go_to_time_for_bed(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;

  /* Lock the gates. */
  state->locked_doors[0] = 0 | door_LOCKED; /* Index into doors + set locked flag. */
  state->locked_doors[1] = 1 | door_LOCKED;

  queue_message_for_display(state, message_TIME_FOR_BED);
  go_to_time_for_bed(state);
}

void event_new_red_cross_parcel(tgestate_t *state)
{
  static const itemstruct_t red_cross_parcel_reset_data =
  {
    0, /* never used */
    room_20_REDCROSS,
    { 44, 44, 12 },
    { 128, 244 }
  };

  static const item_t red_cross_parcel_contents_list[4] =
  {
    item_PURSE,
    item_WIRESNIPS,
    item_BRIBE,
    item_COMPASS,
  };

  const item_t *item;  /* was DE */
  uint8_t       iters; /* was B */

  assert(state != NULL);

  /* Don't deliver a new red cross parcel while the previous one still
   * exists. */
  if ((state->item_structs[item_RED_CROSS_PARCEL].room_and_flags & itemstruct_ROOM_MASK) != itemstruct_ROOM_NONE)
    return;

  /* Select the contents of the next parcel; choosing the first item from the
   * list which does not already exist. */
  item = &red_cross_parcel_contents_list[0];
  iters = NELEMS(red_cross_parcel_contents_list);
  do
  {
    itemstruct_t *itemstruct; /* was HL */

    itemstruct = item_to_itemstruct(state, *item);
    if ((itemstruct->room_and_flags & itemstruct_ROOM_MASK) == itemstruct_ROOM_NONE)
      goto found; /* Future: Remove goto. */

    item++;
  }
  while (--iters);

  return;

found:
  state->red_cross_parcel_current_contents = *item;
  memcpy(&state->item_structs[item_RED_CROSS_PARCEL].room_and_flags,
         &red_cross_parcel_reset_data.room_and_flags,
         6);
  queue_message_for_display(state, message_RED_CROSS_PARCEL);
}

void event_time_for_bed(tgestate_t *state)
{
  assert(state != NULL);

  // Reverse route of below.

  const route_t t = { 38 | route_REVERSED, 3 }; /* was C,A */
  set_guards_route(state, t);
}

void event_search_light(tgestate_t *state)
{
  assert(state != NULL);

  const route_t t = { 38, 0 }; /* was C,A */
  set_guards_route(state, t);
}

/**
 * Common end of event_time_for_bed and event_search_light.
 * Sets the location of guards 12..15 (the guards from prisoners_and_guards).
 *
 * \param[in] state Pointer to game state.
 * \param[in] route Route.                 (was C,A (hi,lo))
 */
void set_guards_route(tgestate_t *state, route_t route)
{
  character_t index; /* was A' */
  uint8_t     iters; /* was B */

  assert(state != NULL);
  ASSERT_ROUTE_VALID(route);

  index = character_12_GUARD_12;
  iters = 4;
  do
  {
    set_character_route(state, index, route);
    index++;
    route.index++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A27F: List of non-player characters: six prisoners and four guards.
 *
 * Used by set_prisoners_and_guards_route and
 * set_prisoners_and_guards_route_B.
 */
const character_t prisoners_and_guards[10] =
{
  character_12_GUARD_12,
  character_13_GUARD_13,
  character_20_PRISONER_1,
  character_21_PRISONER_2,
  character_22_PRISONER_3,
  character_14_GUARD_14,
  character_15_GUARD_15,
  character_23_PRISONER_4,
  character_24_PRISONER_5,
  character_25_PRISONER_6
};

/* ----------------------------------------------------------------------- */

/**
 * $A289: Wake up.
 *
 * \param[in] state Pointer to game state.
 */
void wake_up(tgestate_t *state)
{
  characterstruct_t *charstr;  /* was HL */
  uint8_t            iters;    /* was B */
  uint8_t *const    *bedpp;    /* was HL */

  assert(state != NULL);

  if (state->hero_in_bed)
  {
    /* Hero gets out of bed. */
    state->vischars[0].mi.pos.x = 46;
    state->vischars[0].mi.pos.y = 46;
  }

  state->hero_in_bed = 0;

  const route_t t42 = { 42, 0 }; /* was BC */
  set_hero_route(state, t42);

  /* Position all six prisoners. */
  charstr = &state->character_structs[character_20_PRISONER_1];
  iters = 3;
  do
  {
    charstr->room = room_3_HUT2RIGHT;
    charstr++;
  }
  while (--iters);
  iters = 3;
  do
  {
    charstr->room = room_5_HUT3RIGHT;
    charstr++;
  }
  while (--iters);

  route_t t5 = { 5, 0 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t5);

  /* Update all the bed objects to be empty. */
  // FIXME: Writing to shared state.
  bedpp = &beds[0];
  iters = beds_LENGTH; /* Bug: Conv: Original code uses 7 which is wrong. */
  do
    **bedpp++ = interiorobject_EMPTY_BED_FACING_SE;
  while (--iters);

  /* Update the hero's bed object to be empty and redraw if required. */
  roomdef_2_hut2_left[roomdef_2_BED] = interiorobject_EMPTY_BED_FACING_SE;
  if (state->room_index != room_0_OUTDOORS && state->room_index < room_6)
  {
    setup_room(state);
    plot_interior_tiles(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $A2E2: End of breakfast time.
 *
 * \param[in] state Pointer to game state.
 */
void end_of_breakfast(tgestate_t *state)
{
  characterstruct_t *charstr;  /* was HL */
  uint8_t            iters;    /* was B */

  assert(state != NULL);

  if (state->hero_in_breakfast)
  {
    state->vischars[0].mi.pos.x = 52;
    state->vischars[0].mi.pos.y = 62;
  }

  state->hero_in_breakfast = 0;
  const route_t t = { 16 | route_REVERSED, 3 }; /* was BC */
  set_hero_route(state, t);

  charstr = &state->character_structs[character_20_PRISONER_1];
  iters = 3;
  do
  {
    charstr->room = room_25_BREAKFAST;
    charstr++;
  }
  while (--iters);
  iters = 3;
  do
  {
    charstr->room = room_23_BREAKFAST;
    charstr++;
  }
  while (--iters);

  route_t t2 = { 16 | route_REVERSED, 3 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t2);

  /* Update all the benches to be empty. */
  // FIXME: Writing to shared state.
  roomdef_23_breakfast[roomdef_23_BENCH_A] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_B] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_C] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_D] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_E] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_F] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_EMPTY_BENCH;

  if (state->room_index == room_0_OUTDOORS ||
      state->room_index >= room_29_SECOND_TUNNEL_START)
    return;

  setup_room(state);
  plot_interior_tiles(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A33F: Set the hero's route.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] route  Route.                (was BC)
 */
void set_hero_route(tgestate_t *state, route_t route)
{
  vischar_t *vischar; /* was HL */

  assert(state != NULL);
  ASSERT_ROUTE_VALID(route);

  if (state->in_solitary)
    return; /* Ignore. */

  vischar = &state->vischars[0];

  vischar->flags &= ~vischar_FLAGS_DOOR_THING;
  vischar->route = route;
  set_route(state, vischar);
}

/* ----------------------------------------------------------------------- */

/**
 * $A351: Go to time for bed.
 *
 * \param[in] state Pointer to game state.
 */
void go_to_time_for_bed(tgestate_t *state)
{
  assert(state != NULL);

  const route_t t5 = { 5 | route_REVERSED, 2 }; /* was BC */
  set_hero_route(state, t5);

  route_t t5_also = { 5 | route_REVERSED, 2 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t5_also);
}

/* ----------------------------------------------------------------------- */

/**
 * $A35F: Set the route for all prisoners and guards.
 *
 * Set a different route for each character.
 *
 * Called by go_to_roll_call.
 *
 * \param[in]     state  Pointer to game state.
 * \param[in,out] proute Pointer to route. (was C,A')
 */
void set_prisoners_and_guards_route(tgestate_t *state, route_t *proute)
{
  route_t            route;  /* new var */
  const character_t *pchars; /* was HL */
  uint8_t            iters;  /* was B */

  assert(state != NULL);
  ASSERT_ROUTE_VALID(*proute);

  route = *proute; /* Conv: Keep a local copy of counter. */

  pchars = &prisoners_and_guards[0];
  iters = NELEMS(prisoners_and_guards);
  do
  {
    set_character_route(state, *pchars, route);
    route.index++;
    pchars++;
  }
  while (--iters);

  *proute = route;
}

/* ----------------------------------------------------------------------- */

/**
 * $A373: Set the route for all prisoners and guards.
 *
 * Set the same route for each half of the group.
 *
 * Called by the set_route routines.
 *
 * \param[in]     state  Pointer to game state.
 * \param[in,out] proute Pointer to route. (was C,A')
 */
void set_prisoners_and_guards_route_B(tgestate_t *state, route_t *proute)
{
  route_t            route; /* new var */
  const character_t *pchars; /* was HL */
  uint8_t            iters;  /* was B */

  assert(state != NULL);
  ASSERT_ROUTE_VALID(*proute);

  route = *proute; /* Conv: Keep a local copy of counter. */

  pchars = &prisoners_and_guards[0];
  iters = NELEMS(prisoners_and_guards);
  do
  {
    set_character_route(state, *pchars, route);

    /* When this is 6, the character being processed is
     * character_22_PRISONER_3 and the next is character_14_GUARD_14, the
     * start of the second half of the list. */
    if (iters == 6)
      route.index++;

    pchars++;
  }
  while (--iters);

  *proute = route;
}

/* ----------------------------------------------------------------------- */

/**
 * $A38C: Set the route for a character.
 *
 * Finds a charstruct, or a vischar, and stores a route.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] character Character index.       (was A)
 * \param[in] route     Route.                 (was A' lo + C hi)
 */
void set_character_route(tgestate_t *state,
                         character_t character,
                         route_t     route)
{
  characterstruct_t *charstr; /* was HL */
  vischar_t         *vischar; /* was HL */
  uint8_t            iters;   /* was B */

  assert(state != NULL);
  ASSERT_CHARACTER_VALID(character);
  ASSERT_ROUTE_VALID(route);

  charstr = get_character_struct(state, character);
  if ((charstr->character_and_flags & characterstruct_FLAG_DISABLED) != 0)
  {
    assert(character == (charstr->character_and_flags & characterstruct_CHARACTER_MASK));

    // Why fetch this copy of character index, is it not the same as the passed-in character?
    // might just be a re-fetch when the original code clobbers register A.
    character = charstr->character_and_flags & characterstruct_CHARACTER_MASK;

    /* Search non-player characters to see if this character is already on-screen. */
    iters = vischars_LENGTH - 1;
    vischar = &state->vischars[1];
    do
    {
      if (character == vischar->character)
        goto found_on_screen; /* Character is on-screen: store to vischar. */
      vischar++;
    }
    while (--iters);

    return; /* Conv: Was goto exit; */
  }

  /* Store to characterstruct only. */
  store_route(route, &charstr->route);
  return;

  // FUTURE: Move this chunk into the body of the loop above.
found_on_screen:
  vischar->flags &= ~vischar_FLAGS_DOOR_THING;
  store_route(route, &vischar->route);

  set_route(state, vischar); // was fallthrough
}

/**
 * $A3BB: set_route.
 *
 * Called by set_character_route, set_hero_route.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was HL) (e.g. $8003 in original)
 */
void set_route(tgestate_t *state, vischar_t *vischar)
{
  uint8_t          get_target_result; /* was A */
  tinypos_t       *pos;               /* was DE */
  const tinypos_t *doorpos;           /* was HL */
  const xy_t      *location;          /* was HL */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  state->entered_move_characters = 0;

  // sampled HL = $8003 $8043 $8023 $8063 $8083 $80A3

  get_target_result = get_target(state,
                                 &vischar->route,
                                 &doorpos,
                                 &location);

  /* Stash the target coordinates in 'pos'. */
  pos = &vischar->pos;
  if (get_target_result == get_target_LOCATION)
  {
    pos->x = location->x;
    pos->y = location->y;
  }
  else if (get_target_result == get_target_DOOR)
  {
    pos->x = doorpos->x;
    pos->y = doorpos->y;
  }

  if (get_target_result == get_target_ROUTE_ENDS)
  {
    state->IY = vischar;
    (void) get_target_and_handle_it(state, vischar, &vischar->route);
  }
  else if (get_target_result == get_target_DOOR)
  {
    vischar->flags |= vischar_FLAGS_DOOR_THING;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $A3ED: Store an xy_t at the specified address.
 *
 * Used by set_character_route only.
 *
 * \param[in]  route  Route. (was A' lo + C hi)
 * \param[out] proute Pointer to vischar->route, or characterstruct->route. (was HL)
 */
void store_route(route_t route, route_t *proute)
{
  ASSERT_ROUTE_VALID(route);
  assert(proute != NULL);

  *proute = route;
}

/* ----------------------------------------------------------------------- */

/**
 * $A3F3: entered_move_characters is non-zero.
 *
 * \param[in] state Pointer to game state.
 * \param[in] route Pointer to route. (was HL)
 */
void byte_A13E_is_nonzero(tgestate_t *state,
                          route_t    *route)
{
  assert(state != NULL);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  byte_A13E_common(state, state->character_index, route);
}

/**
 * $A3F8: entered_move_characters is zero.
 *
 * Gets hit when hero enters hut at end of day.
 *
 * \param[in] state Pointer to game state.
 * \param[in] route Pointer to route. (was HL)
 */
void byte_A13E_is_zero(tgestate_t *state,
                       route_t    *route)
{
  character_t character;
  vischar_t  *vischar;

  assert(state != NULL);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  vischar = state->IY;
  ASSERT_VISCHAR_VALID(vischar);

  character = vischar->character;
  if (character == character_0_COMMANDANT)
  {
    // hero moving in reaction to the commandant... exiting solitary?

    route_t t = { 44, 0 }; /* was BC */
    set_hero_route(state, t);
  }
  else
  {
    byte_A13E_common(state, character, route); /* was fallthrough */
  }
}

/**
 * $A404: Common end of above two routines.
 *
 * Assigns route.index from character number.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] character Character index.       (was A)
 * \param[in] route     Pointer to route.      (was HL)
 */
void byte_A13E_common(tgestate_t *state,
                      character_t character,
                      route_t    *route)
{
  uint8_t routeindex; /* was A */

  assert(state    != NULL);
  ASSERT_CHARACTER_VALID(character);
  assert(route   != NULL);
  ASSERT_ROUTE_VALID(*route);

  route->step = 0;

  if (character >= character_20_PRISONER_1)
  {
    /* All prisoners */

    // routeindex = 7..12 (route to bed) for prisoner 1..6
    routeindex = character - 13;
  }
  else
  {
    /* All hostiles */

    routeindex = 13;
    if (character & 1) /* Odd numbered characters? */
    {
      /* reverse route */
      route->step = 1;
      routeindex |= route_REVERSED;
    }
  }

  route->index = routeindex;
}

/* ----------------------------------------------------------------------- */

/**
 * $A420: Character sits.
 *
 * This is called with x = 18,19,20,21,22.
 * - 18..20 go to room_25_BREAKFAST
 * - 21..22 go to room_23_BREAKFAST
 *
 * \param[in] state      Pointer to game state.
 * \param[in] routeindex                   (was A)
 * \param[in] route      Pointer to route. (was HL)
 */
void character_sits(tgestate_t *state,
                    uint8_t     routeindex,
                    route_t    *route)
{
  uint8_t  index; /* was A */
  uint8_t *bench; /* was HL */
  room_t   room;  /* was C */

  assert(state  != NULL);
  assert(routeindex >= 18 && routeindex <= 22);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  index = routeindex - 18;
  /* First three characters. */
  bench = &roomdef_25_breakfast[roomdef_25_BENCH_D];
  if (index >= 3)
  {
    /* Last two characters. */
    bench = &roomdef_23_breakfast[roomdef_23_BENCH_A];
    index -= 3;
  }

  /* Poke object. */
  bench[index * 3] = interiorobject_PRISONER_SAT_MID_TABLE;

  if (routeindex < 21)
    room = room_25_BREAKFAST;
  else
    room = room_23_BREAKFAST;

  character_sit_sleep_common(state, room, route);
}

/**
 * $A444: Character sleeps.
 *
 * This is called with x = 7,8,9,10,11,12.
 * -  7..9  go to room_3_HUT2RIGHT
 * - 10..12 go to room_5_HUT3RIGHT
 *
 * \param[in] state      Pointer to game state.
 * \param[in] routeindex                   (was A)
 * \param[in] route      Pointer to route. (was HL)
 */
void character_sleeps(tgestate_t *state,
                      uint8_t     routeindex,
                      route_t    *route)
{
  room_t room; /* was C */

  assert(state  != NULL);
  assert(routeindex >= 7 && routeindex <= 12);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  /* Poke object. */
  *beds[routeindex - 7] = interiorobject_OCCUPIED_BED;

  if (routeindex < 10)
    room = room_3_HUT2RIGHT;
  else
    room = room_5_HUT3RIGHT;

  character_sit_sleep_common(state, room, route); // was fallthrough
}

/**
 * $A462: Make characters disappear, repainting the screen if required.
 *
 * \param[in] state Pointer to game state.
 * \param[in] room  Room index.            (was C)
 * \param[in] route Route - used to determine either a vischar or a characterstruct depending on if the room specified is the current room. (was DE/HL)
 */
void character_sit_sleep_common(tgestate_t *state,
                                room_t      room,
                                route_t    *route)
{
  /* This receives a pointer to a route structure which is within either a
   * characterstruct or a vischar. */

  assert(state != NULL);
  ASSERT_ROOM_VALID(room);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  // sampled HL =
  // breakfast $8022 + + $76B8 $76BF
  // night     $76A3 $76B8 $76C6 $76B1 $76BF $76AA
  // (not always in the same order)
  //
  // $8022 -> vischar[1]->route
  // others -> character_structs->route

  route->index = route_WANDER; /* Choose random locations[]. */

  if (state->room_index != room)
  {
    /* Character is sitting or sleeping in a room presently not visible. */

    characterstruct_t *cs;

    /* Retrieve the parent structure pointer. */
    cs = structof(route, characterstruct_t, route);
    cs->room = room_NONE;
  }
  else
  {
    /* Character is visible - force a repaint. */

    vischar_t *vc;

    /* This is only ever hit when location is in vischar @ $8020. */

    /* Retrieve the parent structure pointer. */
    vc = structof(route, vischar_t, route);
    vc->room = room_NONE;

    select_room_and_plot(state);
  }
}

/**
 * $A479: Select room and plot.
 *
 * \param[in] state Pointer to game state.
 */
void select_room_and_plot(tgestate_t *state)
{
  assert(state != NULL);

  setup_room(state);
  plot_interior_tiles(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A47F: The hero sits.
 *
 * \param[in] state Pointer to game state.
 */
void hero_sits(tgestate_t *state)
{
  assert(state != NULL);

  roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_PRISONER_SAT_END_TABLE;
  hero_sit_sleep_common(state, &state->hero_in_breakfast);
}

/**
 * $A489: The hero sleeps.
 *
 * \param[in] state Pointer to game state.
 */
void hero_sleeps(tgestate_t *state)
{
  assert(state != NULL);

  roomdef_2_hut2_left[roomdef_2_BED] = interiorobject_OCCUPIED_BED;
  hero_sit_sleep_common(state, &state->hero_in_bed);
}

/**
 * $A498: Common end of hero_sits/sleeps.
 *
 * \param[in] state Pointer to game state.
 * \param[in] pflag Pointer to hero_in_breakfast or hero_in_bed. (was HL)
 */
void hero_sit_sleep_common(tgestate_t *state, uint8_t *pflag)
{
  assert(state != NULL);
  assert(pflag != NULL);

  /* Set hero_in_breakfast or hero_in_bed flag. */
  *pflag = 255;

  /* Reset only the route index. */
  state->vischars[0].route.index = 0; /* Stand still. */

  /* Set hero position (x,y) to zero. */
  state->vischars[0].mi.pos.x = 0;
  state->vischars[0].mi.pos.y = 0;

  calc_vischar_screenpos_from_mi_pos(state, &state->vischars[0]);

  select_room_and_plot(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A4A9: Set route 14.
 *
 * \param[in] state Pointer to game state.
 */
void set_route_0x0E00(tgestate_t *state)
{
  assert(state != NULL);

  const route_t t14 = { 14, 0 };
  set_hero_route(state, t14);

  route_t t14_also = { 14, 0 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t14_also);
}

/**
 * $A4B7: Set route 14 reversed.
 *
 * Reverse of the route selected above?
 *
 * \param[in] state Pointer to game state.
 */
void set_route_0x8E04(tgestate_t *state)
{
  assert(state != NULL);

  // route $E is 5 long, so offset 4 could be the final byte and the '128' flag is a reverse

  route_t t14 = { 14 | route_REVERSED, 4 };
  set_hero_route(state, t14);

  route_t t14_also = { 14 | route_REVERSED, 4 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t14_also);
}

/**
 * $A4C5: Set route 16.
 *
 * \param[in] state Pointer to game state.
 */
void set_route_0x1000(tgestate_t *state)
{
  assert(state != NULL);

  route_t t16 = { 16, 0 };
  set_hero_route(state, t16);

  route_t t16_also = { 16, 0 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t16_also);
}

/* ----------------------------------------------------------------------- */

/**
 * $A4D3: entered_move_characters is non-zero (another one).
 *
 * Very similar to the routine at $A3F3.
 *
 * \param[in] state Pointer to game state.
 * \param[in] route Pointer to character struct. (was HL)
 */
void byte_A13E_is_nonzero_anotherone(tgestate_t *state,
                                     route_t    *route)
{
  assert(state != NULL);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  byte_A13E_anotherone_common(state, state->character_index, route);
}

/**
 * $A4D8: entered_move_characters is zero (another one).
 *
 * \param[in] state Pointer to game state.
 * \param[in] route Pointer to route. (was HL)
 */
void byte_A13E_is_zero_anotherone(tgestate_t *state,
                                  route_t    *route)
{
  character_t character;
  vischar_t  *vischar;

  assert(state != NULL);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  vischar = state->IY;
  ASSERT_VISCHAR_VALID(vischar);

  character = vischar->character;
  if (character == character_0_COMMANDANT)
  {
    const route_t t = { 43, 0 }; /* was BC */
    set_hero_route(state, t);
  }
  else
  {
    byte_A13E_anotherone_common(state, character, route); /* was fallthrough */
  }
}

/**
 * $A4E4: Common end of above two routines.
 *
 * Start of breakfast: sets routes for prisoners and guards.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] character Character index.       (was A)
 * \param[in] route     Pointer to route.      (was HL)
 */
void byte_A13E_anotherone_common(tgestate_t *state,
                                 character_t character,
                                 route_t    *route)
{
  uint8_t routeindex; /* was A */

  assert(state != NULL);
  ASSERT_CHARACTER_VALID(character);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  route->step = 0;

  if (character >= character_20_PRISONER_1)
  {
    /* Prisoners 1..6 take routes 18..23 (routes to sitting). */
    routeindex = character - 2;
  }
  else
  {
    /* Guards take route 24 if even, or 25 if odd. */
    routeindex = 24;
    if (character & 1)
      routeindex++;
  }

  route->index = routeindex;
}

/* ----------------------------------------------------------------------- */

/**
 * $A4FD: Go to roll call.
 *
 * \param[in] state Pointer to game state.
 */
void go_to_roll_call(tgestate_t *state)
{
  assert(state != NULL);

  route_t t26 = { 26, 0 }; /* was C,A' */
  set_prisoners_and_guards_route(state, &t26);

  const route_t t45 = { 45, 0 };
  set_hero_route(state, t45);
}

/* ----------------------------------------------------------------------- */

/**
 * $A50B: Screen reset.
 *
 * \param[in] state Pointer to game state.
 */
void screen_reset(tgestate_t *state)
{
  assert(state != NULL);

  wipe_visible_tiles(state);
  plot_interior_tiles(state);
  zoombox(state);
  plot_game_window(state);
  set_game_window_attributes(state, attribute_WHITE_OVER_BLACK);
}

/* ----------------------------------------------------------------------- */

/**
 * $A51C: Hero has escaped.
 *
 * Print 'well done' message then test to see if the correct objects were
 * used in the escape attempt.
 *
 * \param[in] state Pointer to game state.
 */
void escaped(tgestate_t *state)
{
  /**
   * $A5CE: Escape messages.
   */
  static const screenlocstring_t messages[11] =
  {
    { 0x006E,  9, "WELL DONE"             },
    { 0x00AA, 16, "YOU HAVE ESCAPED"      },
    { 0x00CC, 13, "FROM THE CAMP"         },
    { 0x0809, 18, "AND WILL CROSS THE"    },
    { 0x0829, 19, "BORDER SUCCESSFULLY"   },
    { 0x0809, 19, "BUT WERE RECAPTURED"   },
    { 0x082A, 17, "AND SHOT AS A SPY"     },
    { 0x0829, 18, "TOTALLY UNPREPARED"    },
    { 0x082C, 12, "TOTALLY LOST"          },
    { 0x0828, 21, "DUE TO LACK OF PAPERS" },
    { 0x100D, 13, "PRESS ANY KEY"         },
  };

  const screenlocstring_t *message;   /* was HL */
  escapeitem_t             itemflags; /* was C */
  const item_t            *pitem;     /* was HL */
  uint8_t                  keys;      /* was A */

  assert(state != NULL);

  screen_reset(state);

  /* Print standard prefix messages. */
  message = &messages[0];
  message = screenlocstring_plot(state, message); /* WELL DONE */
  message = screenlocstring_plot(state, message); /* YOU HAVE ESCAPED */
  (void) screenlocstring_plot(state, message);    /* FROM THE CAMP */

  /* Form escape items bitfield. */
  itemflags = 0;
  pitem = &state->items_held[0];
  itemflags = join_item_to_escapeitem(pitem++, itemflags);
  itemflags = join_item_to_escapeitem(pitem,   itemflags);

  /* Print item-tailored messages. */
  if (itemflags == (escapeitem_COMPASS | escapeitem_PURSE))
  {
    message = &messages[3];
    message = screenlocstring_plot(state, message); /* AND WILL CROSS THE */
    (void) screenlocstring_plot(state, message);    /* BORDER SUCCESSFULLY */

    itemflags = 0xFF; /* success - reset game */
  }
  else if (itemflags != (escapeitem_COMPASS | escapeitem_PAPERS))
  {
    message = &messages[5]; /* BUT WERE RECAPTURED */
    message = screenlocstring_plot(state, message);

    /* No uniform => AND SHOT AS A SPY. */
    if (itemflags < escapeitem_UNIFORM)
    {
      /* No objects => TOTALLY UNPREPARED. */
      message = &messages[7];
      if (itemflags)
      {
        /* No compass => TOTALLY LOST. */
        message = &messages[8];
        if (itemflags & escapeitem_COMPASS)
        {
          /* No papers => DUE TO LACK OF PAPERS. */
          message = &messages[9];
        }
      }
    }
    (void) screenlocstring_plot(state, message);
  }

  message = &messages[10]; /* PRESS ANY KEY */
  (void) screenlocstring_plot(state, message);

  /* Wait for a keypress. */
  do
    keys = keyscan_all(state);
  while (keys != 0); /* Down press */
  do
    keys = keyscan_all(state);
  while (keys == 0); /* Up press */

  /* Reset the game, or send the hero to solitary. */
  if (itemflags == 0xFF || itemflags >= escapeitem_UNIFORM)
    reset_game(state); // exit via
  else
    solitary(state); // exit via
}

/* ----------------------------------------------------------------------- */

/**
 * $A58C: Key scan of all ports.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Key code. (was A)
 */
uint8_t keyscan_all(tgestate_t *state)
{
  uint16_t port;  /* was BC */
  uint8_t  keys;  /* was A */
  int      carry = 0;

  assert(state != NULL);

  /* Scan all keyboard ports (0xFEFE .. 0x7FFE). */
  port = port_KEYBOARD_SHIFTZXCV;
  do
  {
    keys = state->speccy->in(state->speccy, port);

    /* Invert bits and mask off key bits. */
    keys = ~keys & 0x1F;
    if (keys)
      return keys; /* Key(s) pressed */

    /* Rotate the top byte of the port number to get the next port. */
    // Conv: Was RLC B
    carry = (port >> 15) != 0;
    port  = ((port << 1) & 0xFF00) | (carry << 8) | (port & 0x00FF);
  }
  while (carry);

  return 0; /* No keys pressed */
}

/* ----------------------------------------------------------------------- */

/**
 * $A59C: Call item_to_escapeitem then merge result with a previous escapeitem.
 *
 * \param[in] pitem    Pointer to item.              (was HL)
 * \param[in] previous Previous escapeitem bitfield. (was C)
 *
 * \return Merged escapeitem bitfield. (was C)
 */
escapeitem_t join_item_to_escapeitem(const item_t *pitem, escapeitem_t previous)
{
  assert(pitem != NULL);
  // assert(previous);

  return item_to_escapeitem(*pitem) | previous;
}

/**
 * $A5A3: Return a bitmask indicating the presence of items required for escape.
 *
 * \param[in] item Item. (was A)
 *
 * \return COMPASS, PAPERS, PURSE, UNIFORM => 1, 2, 4, 8. (was A)
 */
escapeitem_t item_to_escapeitem(item_t item)
{
  switch (item)
  {
  case item_COMPASS:
    return escapeitem_COMPASS;
  case item_PAPERS:
    return escapeitem_PAPERS;
  case item_PURSE:
    return escapeitem_PURSE;
  case item_UNIFORM:
    return escapeitem_UNIFORM;
  default:
    return 0;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $A5BF: Plot a screenlocstring.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] slstring Pointer to screenlocstring. (was HL)
 *
 * \return Pointer to next screenlocstring. (was HL)
 */
const screenlocstring_t *screenlocstring_plot(tgestate_t              *state,
                                              const screenlocstring_t *slstring)
{
  uint8_t    *screen; /* was DE */
  int         length; /* was B */
  const char *string; /* was HL */

  assert(state    != NULL);
  assert(slstring != NULL);

  screen = &state->speccy->screen[slstring->screenloc];
  length = slstring->length;
  string = slstring->string;
  do
    screen = plot_glyph(string++, screen);
  while (--length);

  return slstring + 1;
}

/* ----------------------------------------------------------------------- */

/**
 * $A7C9: Get supertiles.
 *
 * Uses state->map_position to copy supertile indices from map into
 * the buffer at state->map_buf.
 *
 * \param[in] state Pointer to game state.
 */
void get_supertiles(tgestate_t *state)
{
  uint8_t                 v;     /* was A */
  const supertileindex_t *tiles; /* was HL */
  uint8_t                 iters; /* was A */
  uint8_t                *buf;   /* was DE */

  assert(state != NULL);

  /* Get vertical offset. */
  v = state->map_position.y & ~3; /* = 0, 4, 8, 12, ... */

  /* Multiply A by 13.5. (v is a multiple of 4, so this goes 0, 54, 108, 162, ...) */
  tiles = &map[0] - MAPX + (v + (v >> 1)) * 9; // Subtract MAPX so it skips the first row.

  /* Add horizontal offset. */
  tiles += state->map_position.x >> 2;

  /* Populate map_buf with 7x5 array of supertile refs. */
  iters = state->st_rows;
  buf = &state->map_buf[0];
  do
  {
    ASSERT_MAP_PTR_VALID(tiles);
    if (tiles >= &map[0] && (tiles + state->st_columns) < &map[MAPX * MAPY]) // conv: debugging
      memcpy(buf, tiles, state->st_columns);
    else
    {
      assert(0);
      memset(buf, 42, state->st_columns); // debug
    }

    buf   += state->st_columns;
    tiles += MAPX;
  }
  while (--iters);

  check_map_buf(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A80A: Plot the complete bottommost row of tiles.
 *
 * \param[in] state Pointer to game state.
 */
void plot_bottommost_tiles(tgestate_t *state)
{
  tileindex_t      *vistiles; /* was DE */
  supertileindex_t *maptiles; /* was HL' */
  uint8_t           y;        /* was A */
  uint8_t          *window;   /* was DE' */

  assert(state != NULL);

  vistiles = &state->tile_buf[24 * 16];       // $F278 = visible tiles array + 24 * 16
  maptiles = &state->map_buf[28];             // $FF74
  y        = state->map_position.y;           // map_position y
  window   = &state->window_buf[24 * 16 * 8]; // $FE90

  plot_horizontal_tiles_common(state, vistiles, maptiles, y, window);
}

/**
 * $A819: Plot the complete topmost row of tiles.
 *
 * \param[in] state Pointer to game state.
 */
void plot_topmost_tiles(tgestate_t *state)
{
  tileindex_t      *vistiles; /* was DE */
  supertileindex_t *maptiles; /* was HL' */
  uint8_t           y;        /* was A */
  uint8_t          *window;   /* was DE' */

  assert(state != NULL);

  vistiles = &state->tile_buf[0];   // $F0F8 = visible tiles array + 0
  maptiles = &state->map_buf[0];    // $FF58
  y        = state->map_position.y; // map_position y
  window   = &state->window_buf[0]; // $F290

  plot_horizontal_tiles_common(state, vistiles, maptiles, y, window);
}

/**
 * $A826: Plotting supertiles.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] vistiles Pointer to visible tiles array.         (was DE)
 * \param[in] maptiles Pointer to 7x5 supertile refs.          (was HL')
 * \param[in] y        Map position y.                         (was A)
 * \param[in] window   Pointer to screen buffer start address. (was DE')
 */
void plot_horizontal_tiles_common(tgestate_t             *state,
                                  tileindex_t            *vistiles,
                                  const supertileindex_t *maptiles,
                                  uint8_t                 y,
                                  uint8_t                *window)
{
  /* Conv: self_A86A removed. Can be replaced with pos_copy. */

  uint8_t            y_offset;  /* was Cdash */
  const tileindex_t *tiles;     /* was HL */
  uint8_t            A;         /* was A */
  uint8_t            iters;     /* was B */
  uint8_t            iters2;    /* was B' */
  uint8_t            offset;    // new or A' ?

  assert(state != NULL);
  ASSERT_TILE_BUF_PTR_VALID(vistiles);
  ASSERT_MAP_BUF_PTR_VALID(maptiles);
  // assert(y);
  ASSERT_WINDOW_BUF_PTR_VALID(window);

  y_offset = (y & 3) * 4;
  ASSERT_MAP_POSITION_VALID(state->map_position);
  offset = (state->map_position.x & 3) + y_offset;

  /* Initial edge. */

  assert(*maptiles < supertileindex__LIMIT);
  tiles = &supertiles[*maptiles].tiles[offset];
  ASSERT_SUPERTILE_PTR_VALID(tiles);

  A = tiles - &supertiles[0].tiles[0]; // Conv: Original code could simply use L.

  // 0,1,2,3 => 4,3,2,1
  A = -A & 3;
  if (A == 0)
    A = 4;

  iters = A; /* 1..4 iterations */
  do
  {
    tileindex_t t; /* was A */

    ASSERT_TILE_BUF_PTR_VALID(vistiles);
    ASSERT_SUPERTILE_PTR_VALID(tiles);

    // Conv: Fused accesses and increments.
    t = *vistiles++ = *tiles++; // A = tile index
    window = plot_tile(state, t, maptiles, window);
  }
  while (--iters);

  maptiles++;

  /* Middle loop. */

  iters2 = 5;
  do
  {
    ASSERT_MAP_BUF_PTR_VALID(maptiles);
    assert(*maptiles < supertileindex__LIMIT);
    tiles = &supertiles[*maptiles].tiles[y_offset]; // self modified by $A82A

    iters = 4;
    do
    {
      tileindex_t t; /* was A */

      ASSERT_TILE_BUF_PTR_VALID(vistiles);
      ASSERT_SUPERTILE_PTR_VALID(tiles);

      t = *vistiles++ = *tiles++; // A = tile index
      window = plot_tile(state, t, maptiles, window);
    }
    while (--iters);

    maptiles++;
  }
  while (--iters2);

  /* Trailing edge. */

  //A = pos_copy; // an apparently unused assignment

  ASSERT_MAP_BUF_PTR_VALID(maptiles);
  assert(*maptiles < supertileindex__LIMIT);
  tiles = &supertiles[*maptiles].tiles[y_offset]; // read of self modified instruction
  // Conv: A was A'.
  A = state->map_position.x & 3; // map_position lo (repeats earlier work)
  if (A == 0)
    return;

  iters = A;
  do
  {
    tileindex_t t; /* was Adash */

    ASSERT_TILE_BUF_PTR_VALID(vistiles);
    ASSERT_SUPERTILE_PTR_VALID(tiles);

    t = *vistiles++ = *tiles++; // Adash = tile index
    window = plot_tile(state, t, maptiles, window);
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A8A2: Plot all tiles.
 *
 * Called by pick_up_item and reset_outdoors.
 *
 * \param[in] state Pointer to game state.
 */
void plot_all_tiles(tgestate_t *state)
{
  tileindex_t      *vistiles; /* was DE */
  supertileindex_t *maptiles; /* was HL' */
  uint8_t          *window;   /* was DE' */
  uint8_t           x;        /* was A */
  uint8_t           iters;    /* was B' */

  assert(state != NULL);
  ASSERT_MAP_POSITION_VALID(state->map_position);

  vistiles = &state->tile_buf[0];   /* visible tiles array */
  maptiles = &state->map_buf[0];    /* 7x5 supertile refs */
  window   = &state->window_buf[0]; /* screen buffer start address */
  x        = state->map_position.x; /* map_position x */

  check_map_buf(state);

  iters = state->columns; /* Conv: was 24 */
  do
  {
    plot_vertical_tiles_common(state, vistiles, maptiles, x, window);
    vistiles++;
    x++;
    if ((x & 3) == 0)
      maptiles++;
    window++;
  }
  while (--iters);
}

/**
 * $A8CF: Plot the complete rightmost column of tiles.
 *
 * \param[in] state Pointer to game state.
 */
void plot_rightmost_tiles(tgestate_t *state)
{
  tileindex_t      *vistiles; /* was DE */
  supertileindex_t *maptiles; /* was HL' */
  uint8_t          *window;   /* was DE' */
  uint8_t           x;        /* was A */

  assert(state != NULL);

  // 23 -> state->columns - 1
  //  6 -> state->st_columns - 1

  vistiles = &state->tile_buf[23];   /* visible tiles array */
  maptiles = &state->map_buf[6];     /* 7x5 supertile refs */
  window   = &state->window_buf[23]; /* screen buffer start address */
  x        = state->map_position.x;  /* map_position x */

  x &= 3;
  if (x == 0)
    maptiles--;
  x = state->map_position.x - 1; /* map_position x */

  plot_vertical_tiles_common(state, vistiles, maptiles, x, window);
}

/**
 * $A8E7: Plot the complete leftmost column of tiles.
 *
 * \param[in] state Pointer to game state.
 */
void plot_leftmost_tiles(tgestate_t *state)
{
  tileindex_t      *vistiles; /* was DE */
  supertileindex_t *maptiles; /* was HL' */
  uint8_t          *window;   /* was DE' */
  uint8_t           x;        /* was A */

  assert(state != NULL);

  vistiles = &state->tile_buf[0];   /* visible tiles array */
  maptiles = &state->map_buf[0];    /* 7x5 supertile refs */
  window   = &state->window_buf[0]; /* screen buffer start address */
  x        = state->map_position.x; /* map_position x */

  plot_vertical_tiles_common(state, vistiles, maptiles, x, window);
}

/**
 * $A8F4: Plotting vertical tiles (common part).
 *
 * \param[in] state    Pointer to game state.
 * \param[in] vistiles Pointer to visible tiles array.         (was DE)
 * \param[in] maptiles Pointer to 7x5 supertile refs.          (was HL')
 * \param[in] x        Map position x.                         (was A)
 * \param[in] window   Pointer to screen buffer start address. (was DE')
 */
void plot_vertical_tiles_common(tgestate_t             *state,
                                tileindex_t            *vistiles,
                                const supertileindex_t *maptiles,
                                uint8_t                 x,
                                uint8_t                *window)
{
  /* Conv: self_A94D removed. */

  uint8_t            x_offset;  /* was $A94D */
  const tileindex_t *tiles;     /* was HL */  // pointer into supertile
  uint8_t            offset;    /* was A */
  uint8_t            iters;     /* was A */
  uint8_t            iters2;    /* was B' */ // FUTURE: merge with 'iters'

  assert(state != NULL);
  ASSERT_TILE_BUF_PTR_VALID(vistiles);
  ASSERT_MAP_BUF_PTR_VALID(maptiles);
  // assert(x);
  ASSERT_WINDOW_BUF_PTR_VALID(window);

  x_offset = x & 3; // self modify (local)

  /* Initial edge. */

  ASSERT_MAP_POSITION_VALID(state->map_position);
  offset = (state->map_position.y & 3) * 4 + x_offset;

  assert(*maptiles < supertileindex__LIMIT);
  tiles = &supertiles[*maptiles].tiles[offset];
  ASSERT_SUPERTILE_PTR_VALID(tiles);

  // 0,1,2,3 => 4,3,2,1
  iters = -((offset >> 2) & 3) & 3;
  if (iters == 0)
    iters = 4; // 1..4

  check_map_buf(state);

  do
  {
    tileindex_t t; /* was A */

    ASSERT_TILE_BUF_PTR_VALID(vistiles);
    ASSERT_SUPERTILE_PTR_VALID(tiles);
    ASSERT_MAP_BUF_PTR_VALID(maptiles);
    ASSERT_WINDOW_BUF_PTR_VALID(window);

    t = *vistiles = *tiles; // A = tile index
    window = plot_tile_then_advance(state, t, maptiles, window);
    tiles += 4; // supertile stride
    vistiles += state->columns;
  }
  while (--iters);

  maptiles += 7; // move to next row

  /* Middle loop. */

  iters2 = 3;
  do
  {
    ASSERT_MAP_BUF_PTR_VALID(maptiles);
    assert(*maptiles < supertileindex__LIMIT);
    tiles = &supertiles[*maptiles].tiles[x_offset]; // self modified by $A8F6

    iters = 4;
    do
    {
      tileindex_t t; /* was A */

      ASSERT_TILE_BUF_PTR_VALID(vistiles);
      ASSERT_SUPERTILE_PTR_VALID(tiles);
      ASSERT_MAP_BUF_PTR_VALID(maptiles);
      ASSERT_WINDOW_BUF_PTR_VALID(window);

      t = *vistiles = *tiles; // A = tile index
      window = plot_tile_then_advance(state, t, maptiles, window);
      vistiles += state->columns;
      tiles += 4; // supertile stride
    }
    while (--iters);

    maptiles += 7; // move to next row
  }
  while (--iters2);

  /* Trailing edge. */

  ASSERT_MAP_BUF_PTR_VALID(maptiles);
  assert(*maptiles < supertileindex__LIMIT);
  tiles = &supertiles[*maptiles].tiles[x_offset]; // x_offset = read of self modified instruction
  iters = (state->map_position.y & 3) + 1;
  do
  {
    tileindex_t t; /* was A */

    ASSERT_TILE_BUF_PTR_VALID(vistiles);
    ASSERT_SUPERTILE_PTR_VALID(tiles);
    ASSERT_MAP_BUF_PTR_VALID(maptiles);
    ASSERT_WINDOW_BUF_PTR_VALID(window);

    t = *vistiles = *tiles; // A = tile index
    window = plot_tile_then_advance(state, t, maptiles, window);
    tiles += 4; // supertile stride
    vistiles += state->columns;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A9A0: Call plot_tile then advance 'scr' by a row.
 *
 * \param[in] state      Pointer to game state.
 * \param[in] tile_index Tile index. (was A)
 * \param[in] maptiles   Pointer to supertile index (used to select the
 *                       correct exterior tile set). (was HL')
 * \param[in] scr        Address of output buffer start address. (was DE')
 *
 * \return Next output address. (was DE')
 */
uint8_t *plot_tile_then_advance(tgestate_t             *state,
                                tileindex_t             tile_index,
                                const supertileindex_t *maptiles,
                                uint8_t                *scr)
{
  assert(state != NULL);

  return plot_tile(state, tile_index, maptiles, scr) + state->columns * 8 - 1; // -1 compensates the +1 in plot_tile // was 191
}

/* ----------------------------------------------------------------------- */

/**
 * $A9AD: Plot a tile then increment 'scr' by 1.
 *
 * Leaf.
 *
 * \param[in] state      Pointer to game state.
 * \param[in] tile_index Tile index. (was A)
 * \param[in] maptiles   Pointer to supertile index (used to select the
                         correct exterior tile set). (was HL')
 * \param[in] scr        Output buffer start address. (was DE')
 *
 * \return Next output address. (was DE')
 */
uint8_t *plot_tile(tgestate_t             *state,
                   tileindex_t             tile_index,
                   const supertileindex_t *maptiles,
                   uint8_t                *scr)
{
  supertileindex_t  supertileindex; /* was A' */
  const tile_t     *tileset;        /* was BC' */
  const tilerow_t  *src;            /* was DE' */
  uint8_t          *dst;            /* was HL' */
  uint8_t           iters;          /* was A */

  assert(state != NULL);
  ASSERT_MAP_BUF_PTR_VALID(maptiles);
  ASSERT_WINDOW_BUF_PTR_VALID(scr);

  supertileindex = *maptiles; /* get supertile index */
  assert(supertileindex < supertileindex__LIMIT);

  // Supertiles 44 and lower         use tiles   0..249 (249 tile span)
  // Supertiles 45..138 and 204..218 use tiles 145..400 (255 tile span)
  // Supertiles 139..203             use tiles 365..570 (205 tile span)

  if (supertileindex <= 44)
  {
    assert(tile_index <= 249);
    tileset = &exterior_tiles[0];
  }
  else if (supertileindex <= 138 || supertileindex >= 204)
  {
    assert(tile_index <= 255);
    tileset = &exterior_tiles[145];
  }
  else
  {
    assert(tile_index <= 205);
    tileset = &exterior_tiles[365];
  }

  src = &tileset[tile_index].row[0];
  dst = scr;
  iters = 8;
  do
  {
    ASSERT_WINDOW_BUF_PTR_VALID(dst);
    *dst = *src++;
    dst += state->columns; /* stride */ // FUTURE: Hoist.
  }
  while (--iters);

  return scr + 1;
}

/* ----------------------------------------------------------------------- */

// Fixed constants for now.
#define tile_buf_length   (24 * 17)
#define window_buf_length (24 * 8 * 17)

/**
 * $A9E4: Shunt the map left.
 *
 * \param[in] state Pointer to game state.
 */
void shunt_map_left(tgestate_t *state)
{
  assert(state != NULL);

  state->map_position.x++;

  get_supertiles(state);

  memmove(&state->tile_buf[0], &state->tile_buf[1], tile_buf_length - 1);
  memmove(&state->window_buf[0], &state->window_buf[1], window_buf_length - 1);

  plot_rightmost_tiles(state);
}

/**
 * $AA05: Shunt the map right.
 *
 * \param[in] state Pointer to game state.
 */
void shunt_map_right(tgestate_t *state)
{
  assert(state != NULL);

  state->map_position.x--;

  get_supertiles(state);

  memmove(&state->tile_buf[1], &state->tile_buf[0], tile_buf_length - 1);
  memmove(&state->window_buf[1], &state->window_buf[0], window_buf_length - 1); // orig uses window_buf_length which can't be right

  plot_leftmost_tiles(state);
}

/**
 * $AA26: Shunt the map up-right.
 *
 * \param[in] state Pointer to game state.
 */
void shunt_map_up_right(tgestate_t *state)
{
  assert(state != NULL);

  /* Conv: The original code has a copy of map_position in HL on entry. In
   * this version we read it from source. */
  state->map_position.x--;
  state->map_position.y++;

  get_supertiles(state);

  memmove(&state->tile_buf[1], &state->tile_buf[24], tile_buf_length - 24);
  memmove(&state->window_buf[1], &state->window_buf[24 * 8], window_buf_length - 24 * 8);

  plot_bottommost_tiles(state);
  plot_leftmost_tiles(state);
}

/**
 * $AA4B: Shunt the map up.
 *
 * \param[in] state Pointer to game state.
 */
void shunt_map_up(tgestate_t *state)
{
  assert(state != NULL);

  state->map_position.y++;

  get_supertiles(state);

  memmove(&state->tile_buf[0], &state->tile_buf[24], tile_buf_length - 24);
  memmove(&state->window_buf[0], &state->window_buf[24 * 8], window_buf_length - 24 * 8);

  plot_bottommost_tiles(state);
}

/**
 * $AA6C: Shunt the map down.
 *
 * \param[in] state Pointer to game state.
 */
void shunt_map_down(tgestate_t *state)
{
  assert(state != NULL);

  state->map_position.y--;

  get_supertiles(state);

  memmove(&state->tile_buf[24], &state->tile_buf[0], tile_buf_length - 24);
  memmove(&state->window_buf[24 * 8], &state->window_buf[0], window_buf_length - 24 * 8);

  plot_topmost_tiles(state);
}

/**
 * $AA8D: Shunt the map down left.
 *
 * \param[in] state Pointer to game state.
 */
void shunt_map_down_left(tgestate_t *state)
{
  assert(state != NULL);

  state->map_position.x++;
  state->map_position.y--;

  get_supertiles(state);

  memmove(&state->tile_buf[24], &state->tile_buf[1], tile_buf_length - 24 - 1);
  memmove(&state->window_buf[24 * 8], &state->window_buf[1], window_buf_length - 24 * 8 - 1);

  plot_topmost_tiles(state);
  plot_rightmost_tiles(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $AAB2: Moves the map when the hero walks.
 *
 * \param[in] state Pointer to game state.
 */
void move_map(tgestate_t *state)
{
  typedef void (movemapfn_t)(tgestate_t *, uint8_t *);

  static movemapfn_t *const movemapfns[] =
  {
    &move_map_up_left,
    &move_map_up_right,
    &move_map_down_right,
    &move_map_down_left,
  };

  const uint8_t *anim;              /* was DE */
  uint8_t        animindex;         /* was C */
  direction_t    direction;         /* was A */
  uint8_t        move_map_y;        /* was A */
  movemapfn_t   *pmovefn;           /* was HL */
  uint8_t        x,y;               /* was C,B */
  uint8_t       *pmove_map_y;       /* was HL */
  uint8_t       *pmove_map_y_copy;  /* was DE */
  xy_t           game_window_offset;  /* was HL */
  // uint16_t       HLpos;  /* was HL */

  assert(state != NULL);

  if (state->room_index > room_0_OUTDOORS)
    return; /* Can't move the map when indoors. */

  if (state->vischars[0].counter_and_flags & vischar_BYTE7_TOUCHING)
    return; /* Can't move the map when touch() saw contact. */

  anim      = state->vischars[0].anim;
  animindex = state->vischars[0].animindex;
  direction = anim[3]; // third byte of anim_[A-X] - 255, 0, 1, 2, or 3
  if (direction == 255)
    return; /* Don't move. */

  if (animindex & vischar_ANIMINDEX_BIT7)
    direction ^= 2; /* Exchange up and down. */

  pmovefn = movemapfns[direction];
  // PUSH HL // pushes move_map routine to stack
  // PUSH AF

  /* Map clamping stuff. */
  if (/* DISABLES CODE */ (0))
  {
    // Equivalent
         if (direction == direction_TOP_LEFT)     { y = 124; x = 192; }
    else if (direction == direction_TOP_RIGHT)    { y = 124; x =   0; }
    else if (direction == direction_BOTTOM_RIGHT) { y =   0; x =   0; }
    else if (direction == direction_BOTTOM_LEFT)  { y =   0; x = 192; }
  }
  else
  {
    y = 124;
    x = 0;
    /* direction_BOTTOM_* - bottom of the map clamp */
    if (direction >= direction_BOTTOM_RIGHT)
      y = 0;
    /* direction_*_LEFT - left of the map clamp */
    if (direction != direction_TOP_RIGHT &&
        direction != direction_BOTTOM_RIGHT)
      x = 192;
  }

  /* Note: This looks like it ought to be an AND but it's definitely an OR in
   * the original game. */
  if (state->map_position.x == x || state->map_position.y == y)
    return; /* Don't move. */

  // POP AF

  // Is this a screen offset of some sort?
  // It gets passed into the move functions.
  // Is this a vertical index/offset only?
  pmove_map_y = &state->move_map_y;
  if (direction <= direction_TOP_RIGHT)
    /* direction_TOP_* */
    move_map_y = *pmove_map_y + 1;
  else
    /* direction_BOTTOM_* */
    move_map_y = *pmove_map_y - 1;
  move_map_y &= 3;
  *pmove_map_y = move_map_y;

  // FUTURE: Drop this copy.
  pmove_map_y_copy = pmove_map_y; // was EX DE,HL

  assert(move_map_y <= 3);

  // used by plot_game_window (which masks off top and bottom separately)
  if (0)
  {
    // Equivalent:
//         if (move_map_y == 0) game_window_offset = 0x0000;
//    else if (move_map_y == 1) game_window_offset = 0xFF30;
//    else if (move_map_y == 2) game_window_offset = 0x0060;
//    else if (move_map_y == 3) game_window_offset = 0xFF90;
  }
  else
  {
    game_window_offset.x = 0;
    game_window_offset.y = 0;
    if (move_map_y != 0)
    {
      game_window_offset.x = 96;
      if (move_map_y != 2)
      {
        game_window_offset.x = 48;
        game_window_offset.y = 255;
        if (move_map_y != 1)
        {
          game_window_offset.x = 144;
        }
      }
    }
  }

  state->game_window_offset = game_window_offset;
  // HLpos = state->map_position; // passing map_position in HL omitted in this version

  // pops and calls move_map_* routine pushed at $AAE0
  pmovefn(state, pmove_map_y_copy);
}

// FUTURE: Pass move_map_y directly instead of fetching it from state.
// FUTURE: And fold these functions in to the parent above. (e.g. in a switch).

/* Called when player moves down-right (map is shifted up-left). */
void move_map_up_left(tgestate_t *state, uint8_t *pmove_map_y)
{
  uint8_t move_map_y; /* was A */

  assert(state != NULL);
  assert(pmove_map_y == &state->move_map_y);
  assert(*pmove_map_y <= 3);

  // 0..3 => up/left/none/left

  move_map_y = *pmove_map_y;
  if (move_map_y == 0)
    shunt_map_up(state);
  else if (move_map_y & 1) // 1 or 3
    shunt_map_left(state);
  // else 2
}

/* Called when player moves down-left (map is shifted up-right). */
void move_map_up_right(tgestate_t *state, uint8_t *pmove_map_y)
{
  uint8_t move_map_y; /* was A */

  assert(state != NULL);
  assert(pmove_map_y == &state->move_map_y);
  assert(*pmove_map_y <= 3);

  // 0..3 => up-right/none/right/none

  move_map_y = *pmove_map_y;
  if (move_map_y == 0)
    shunt_map_up_right(state);
  else if (move_map_y == 2)
    shunt_map_right(state);
  // else 1 or 3
}

/* Called when player moves up-left (map is shifted down-right). */
void move_map_down_right(tgestate_t *state, uint8_t *pmove_map_y)
{
  uint8_t move_map_y; /* was A */

  assert(state != NULL);
  assert(pmove_map_y == &state->move_map_y);
  assert(*pmove_map_y <= 3);

  // 0..3 => right/none/right/down

  move_map_y = *pmove_map_y;
  if (move_map_y == 3)
    shunt_map_down(state);
  else if ((move_map_y & 1) == 0) // 0 or 2
    shunt_map_right(state);
  // else 1
}

/* Called when player moves up-right (map is shifted down-left). */
void move_map_down_left(tgestate_t *state, uint8_t *pmove_map_y)
{
  uint8_t move_map_y; /* was A */

  assert(state != NULL);
  assert(pmove_map_y == &state->move_map_y);
  assert(*pmove_map_y <= 3);

  // 0..3 => none/left/none/down-left

  move_map_y = *pmove_map_y;
  if (move_map_y == 1)
    shunt_map_left(state);
  else if (move_map_y == 3)
    shunt_map_down_left(state);
  // else 0 or 2
}

/* ----------------------------------------------------------------------- */

/**
 * $AB6B: Choose game window attributes.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Chosen attributes. (was A)
 */
attribute_t choose_game_window_attributes(tgestate_t *state)
{
  attribute_t attr;

  assert(state != NULL);

  if (state->room_index < room_29_SECOND_TUNNEL_START)
  {
    /* The hero is outside, or in a room, but not in a tunnel. */

    if (state->day_or_night == 0) /* Outside or room, daytime. */
      attr = attribute_WHITE_OVER_BLACK;
    else if (state->room_index == room_0_OUTDOORS) /* Outside, nighttime. */
      attr = attribute_BRIGHT_BLUE_OVER_BLACK;
    else /* Room, nighttime. */
      attr = attribute_CYAN_OVER_BLACK;
  }
  else
  {
    /* The hero is in a tunnel. */

    if (state->items_held[0] == item_TORCH ||
        state->items_held[1] == item_TORCH)
    {
      /* The hero holds a torch - draw the room. */

      attr = attribute_RED_OVER_BLACK;
    }
    else
    {
      /* The hero holds no torch - draw nothing. */

      wipe_visible_tiles(state);
      plot_interior_tiles(state);
      attr = attribute_BLUE_OVER_BLACK;
    }
  }

  state->game_window_attribute = attr;

  return attr;
}

/* ----------------------------------------------------------------------- */

/**
 * $ABA0: Zoombox.
 *
 * \param[in] state Pointer to game state.
 */
void zoombox(tgestate_t *state)
{
  attribute_t attrs; /* was A */
  uint8_t    *pvar;  /* was HL */
  uint8_t     var;   /* was A */

  assert(state != NULL);

  state->zoombox.x = 12;
  state->zoombox.y = 8;

  attrs = choose_game_window_attributes(state);

  state->speccy->attributes[ 9 * state->width + 18] = attrs;
  state->speccy->attributes[ 9 * state->width + 19] = attrs;
  state->speccy->attributes[10 * state->width + 18] = attrs;
  state->speccy->attributes[10 * state->width + 19] = attrs;

  state->zoombox.width  = 0;
  state->zoombox.height = 0;

  do
  {
    pvar = &state->zoombox.x;
    var = *pvar;
    if (var != 1)
    {
      (*pvar)--;
      var--;
      pvar[1]++;
    }

    pvar++; // &state->width;
    var += *pvar;
    if (var < 22)
      (*pvar)++;

    pvar++; // &state->zoombox.y;
    var = *pvar;
    if (var != 1)
    {
      (*pvar)--;
      var--;
      pvar[1]++;
    }

    pvar++; // &state->height;
    var += *pvar;
    if (var < 15)
      (*pvar)++;

    zoombox_fill(state);
    zoombox_draw_border(state);

    state->speccy->sleep(state->speccy, sleeptype_DELAY, 10000 /* 1/10th sec */);
    state->speccy->draw(state->speccy, NULL);
  }
  while (state->zoombox.height + state->zoombox.width < 35);
}

/**
 * $ABF9: Zoombox. partial copy of window buffer contents?
 *
 * \param[in] state Pointer to game state.
 */
void zoombox_fill(tgestate_t *state)
{
  assert(state != NULL);

  uint8_t *const screen_base = &state->speccy->screen[0]; // Conv: Added

  uint8_t   iters;      /* was B */
  uint8_t   hz_count;   /* was A */
  uint8_t   iters2;     /* was A */
  uint8_t   hz_count1;  /* was $AC55 */
  uint8_t   src_skip;   /* was $AC4D */
  uint16_t  hz_count2;  /* was BC */
  uint8_t  *dst;        /* was DE */
  uint16_t  offset;     /* was HL */
  uint8_t  *src;        /* was HL */
  uint16_t  dst_stride; /* was BC */
  uint8_t  *prev_dst;   /* was stack */

  /* Conv: Simplified calculation to use a single multiply. */
  offset = state->zoombox.y * state->columns * 8 + state->zoombox.x;
  src = &state->window_buf[offset + 1];
  ASSERT_WINDOW_BUF_PTR_VALID(src);
  dst = screen_base + state->game_window_start_offsets[state->zoombox.y * 8] + state->zoombox.x; // Conv: Screen base was hoisted from table.
  ASSERT_SCREEN_PTR_VALID(dst);

  hz_count  = state->zoombox.width;
  hz_count1 = hz_count;
  src_skip  = state->columns - hz_count; // columns was 24

  iters = state->zoombox.height; /* iterations */
  do
  {
    prev_dst = dst;

    iters2 = 8; // one row
    do
    {
      hz_count2 = state->zoombox.width; /* TODO: This duplicates the read above. */
      memcpy(dst, src, hz_count2);

      // these computations might take the values out of range on the final iteration (but never be used)
      dst += hz_count2; // this is LDIR post-increment. it can be removed along with the line below.
      src += hz_count2 + src_skip; // move to next source line
      dst -= hz_count1; // was E -= self_AC55; // undo LDIR postinc
      dst += state->width * 8; // was D++; // move to next scanline

      if (iters2 > 1 || iters > 1)
        ASSERT_SCREEN_PTR_VALID(dst);
    }
    while (--iters2);

    dst = prev_dst;

    dst_stride = state->width; // scanline-to-scanline stride (32)
    if (((dst - screen_base) & 0xFF) >= 224) // 224+32 == 256, so do skip to next band // TODO: 224 should be (256 - state->width)
      dst_stride += 7 << 8; // was B = 7

    dst += dst_stride;
  }
  while (--iters);
}

/**
 * $AC6F: Draw zoombox border.
 *
 * \param[in] state Pointer to game state.
 */
void zoombox_draw_border(tgestate_t *state)
{
  assert(state != NULL);

  uint8_t *const screen_base = &state->speccy->screen[0]; // Conv: Added var.

  uint8_t *addr;  /* was HL */
  uint8_t  iters; /* was B */
  int      delta; /* was DE */

  addr = screen_base + state->game_window_start_offsets[(state->zoombox.y - 1) * 8]; // Conv: Screen base hoisted from table.
  ASSERT_SCREEN_PTR_VALID(addr);

  /* Top left */
  addr += state->zoombox.x - 1;
  zoombox_draw_tile(state, zoombox_tile_TL, addr++);

  /* Horizontal, moving right */
  iters = state->zoombox.width;
  do
    zoombox_draw_tile(state, zoombox_tile_HZ, addr++);
  while (--iters);

  /* Top right */
  zoombox_draw_tile(state, zoombox_tile_TR, addr);
  delta = state->width; // move to next character row
  if (((addr - screen_base) & 0xFF) >= 224) // was: if (L >= 224) // if adding 32 would overflow
    delta += 0x0700; // was D=7
  addr += delta;

  /* Vertical, moving down */
  iters = state->zoombox.height;
  do
  {
    zoombox_draw_tile(state, zoombox_tile_VT, addr);
    delta = state->width; // move to next character row
    if (((addr - screen_base) & 0xFF) >= 224) // was: if (L >= 224) // if adding 32 would overflow
      delta += 0x0700; // was D=7
    addr += delta;
  }
  while (--iters);

  /* Bottom right */
  zoombox_draw_tile(state, zoombox_tile_BR, addr--);

  /* Horizontal, moving left */
  iters = state->zoombox.width;
  do
    zoombox_draw_tile(state, zoombox_tile_HZ, addr--);
  while (--iters);

  /* Bottom left */
  zoombox_draw_tile(state, zoombox_tile_BL, addr);
  delta = -state->width; // move to next character row
  if (((addr - screen_base) & 0xFF) < 32) // was: if (L < 32) // if subtracting 32 would underflow
    delta -= 0x0700;
  addr += delta;

  /* Vertical, moving up */
  iters = state->zoombox.height;
  do
  {
    zoombox_draw_tile(state, zoombox_tile_VT, addr);
    delta = -state->width; // move to next character row
    if (((addr - screen_base) & 0xFF) < 32) // was: if (L < 32) // if subtracting 32 would underflow
      delta -= 0x0700;
    addr += delta;
  }
  while (--iters);
}

/**
 * $ACFC: Draw a single zoombox border tile.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] tile    Tile to draw.   (was A)
 * \param[in] addr_in Screen address. (was HL)
 */
void zoombox_draw_tile(tgestate_t     *state,
                       zoombox_tile_t  tile,
                       uint8_t        *addr_in)
{
  /**
   * $AF5E: Zoombox tiles.
   */
  static const tile_t zoombox_tiles[zoombox_tile__LIMIT] =
  {
    { { 0x00, 0x00, 0x00, 0x03, 0x04, 0x08, 0x08, 0x08 } }, /* TL */
    { { 0x00, 0x20, 0x18, 0xF4, 0x2F, 0x18, 0x04, 0x00 } }, /* HZ */
    { { 0x00, 0x00, 0x00, 0x00, 0xE0, 0x10, 0x08, 0x08 } }, /* TR */
    { { 0x08, 0x08, 0x1A, 0x2C, 0x34, 0x58, 0x10, 0x10 } }, /* VT */
    { { 0x10, 0x10, 0x10, 0x20, 0xC0, 0x00, 0x00, 0x00 } }, /* BR */
    { { 0x10, 0x10, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00 } }, /* BL */
  };

  uint8_t         *addr;  /* was DE */
  const tilerow_t *row;   /* was HL */
  uint8_t          iters; /* was B */
  ptrdiff_t        off;   /* Conv: new */
  attribute_t     *attrs; /* was HL */

  assert(state != NULL);
  assert(tile < NELEMS(zoombox_tiles));
  ASSERT_SCREEN_PTR_VALID(addr_in);

  addr = addr_in; // was EX DE,HL
  row = &zoombox_tiles[tile].row[0];
  iters = 8;
  do
  {
    *addr = *row++;
    addr += 256;
  }
  while (--iters);

  /* Conv: The original game can munge bytes directly here, assuming screen
   * addresses, but I can't... */

  addr -= 256; /* Compensate for overshoot */
  off = addr - &state->speccy->screen[0];

  attrs = &state->speccy->attributes[off & 0xFF]; // to within a third

  // can i do ((off >> 11) << 8) ?
  if (off >= 0x0800)
  {
    attrs += 256;
    if (off >= 0x1000)
      attrs += 256;
  }

  *attrs = state->game_window_attribute;
}

/* ----------------------------------------------------------------------- */

/**
 * $AD59: Decides searchlight movement.
 *
 * Leaf.
 *
 * \param[in] slstate Pointer to a searchlight movement data. (was HL)
 */
void searchlight_movement(searchlight_movement_t *slstate)
{
  uint8_t        x,y;       /* was E,D */
  uint8_t        counter;   /* was A */
  const uint8_t *ptr;       /* was BC */
  direction_t    direction; /* was A */

  assert(slstate != NULL);

  x = slstate->xy.x;
  y = slstate->xy.y;
  if (--slstate->step == 0)
  {
    counter = slstate->counter; // sampled HL = $AD3B, $AD34, $AD2D
    if (counter & (1 << 7))
    {
      counter &= 0x7F;
      if (counter == 0)
      {
        slstate->counter &= ~(1 << 7); /* clear sign bit when magnitude hits zero */
      }
      else
      {
        slstate->counter--; /* count down */
        counter--; /* just a copy */
      }
    }
    else
    {
      slstate->counter = ++counter; /* count up */
    }
    ptr = slstate->ptr;
    if (ptr[counter * 2] == 255) /* end of list? */
    {
      slstate->counter--; /* overshot? count down counter byte */
      slstate->counter |= 1 << 7; /* go negative */
      ptr -= 2;
      // A = *ptr; /* This is a flaw in original code: A is fetched but never used again. */
    }
    /* Copy counter + direction_t. */
    slstate->step = ptr[0];
    slstate->direction = ptr[1];
  }
  else
  {
    direction = slstate->direction;
    if (slstate->counter & (1 << 7)) /* test sign */
      direction ^= 2; /* up-down direction toggle */

    /* Conv: This is the [-2]+1 pattern which works out -1/+1. */
    if (direction <= direction_TOP_RIGHT) /* direction_TOP_* */
      y--;
    else
      y++;

    if (direction != direction_TOP_LEFT &&
        direction != direction_BOTTOM_LEFT)
      x += 2; /* direction_*_RIGHT */
    else
      x -= 2; /* direction_*_LEFT */

    slstate->xy.y = y;
    slstate->xy.x = x;
  }
}

/**
 * $ADBD: Turns white screen elements light blue and tracks the hero with a
 * searchlight.
 *
 * \param[in] state Pointer to game state.
 */
void nighttime(tgestate_t *state)
{
  uint8_t                 map_y;           /* was D */
  uint8_t                 map_x;           /* was E */
  uint8_t                 H;               /* was H */
  uint8_t                 L;               /* was L */
  uint8_t                 caught_y;        /* was H */
  uint8_t                 caught_x;        /* was L */
  int                     iters;           /* was B */
  uint8_t                 y;               /* was A' */
  attribute_t            *attrs;           /* was BC */
  searchlight_movement_t *slstate;         /* was HL */
  uint8_t                 use_full_window; /* was A */
  uint8_t                 B, C;

  assert(state != NULL);

  if (state->searchlight_state != searchlight_STATE_SEARCHING)
  {
    if (state->room_index > room_0_OUTDOORS)
    {
      /* If the hero goes indoors then the searchlight loses track. */
      state->searchlight_state = searchlight_STATE_SEARCHING;
      return;
    }

    /* The hero is outdoors. */

    /* If the searchlight previously caught the hero then track him. */

    if (state->searchlight_state == searchlight_STATE_CAUGHT)
    {
      map_x    = state->map_position.x + 4;
      map_y    = state->map_position.y;
      caught_x = state->searchlight.caught_coord.x; // Conv: Reordered
      caught_y = state->searchlight.caught_coord.y;

      if (caught_x == map_x)
      {
        /* If the highlight doesn't need to move then quit. */
        if (caught_y == map_y)
          return;
      }
      else
      {
        /* Move the searchlight left/right to focus on the hero. */
        if (caught_x < map_x)
          caught_x++;
        else
          caught_x--;
      }

      /* Move searchlight up/down to focus on the hero. */
      if (caught_y != map_y)
      {
        if (caught_y < map_y)
          caught_y++;
        else
          caught_y--;
      }

      state->searchlight.caught_coord.x = caught_x; // Conv: Fused store split apart.
      state->searchlight.caught_coord.y = caught_y;
    }

    map_x = state->map_position.x;
    map_y = state->map_position.y;
    /* This casts caught_coord into a searchlight_movement_t knowing that
     * only the coord members will be accessed. */
    slstate = (searchlight_movement_t *) &state->searchlight.caught_coord;
    iters = 1; // 1 iteration
    // PUSH BC
    // PUSH HL
    goto middle_bit;
  }

  /* When not tracking the hero all three spotlights are cycled through. */

  slstate = &state->searchlight.states[0];
  iters = 3; // 3 iterations == three searchlights
  do
  {
    // PUSH BC
    // PUSH HL
    searchlight_movement(slstate);
    // POP HL
    // PUSH HL
    searchlight_caught(state, slstate);
    // POP HL
    // PUSH HL

    map_x = state->map_position.x;
    map_y = state->map_position.y;

    /* Would the searchlight intersect with the game window? */

    // if (slstate->xy.x >= map_x + 23 || slstate->xy.x < map_x - 16) equivalent?
    if (map_x + 23 < slstate->xy.x || slstate->xy.x + 16 < map_x)
      goto next;

    //if (slstate->xy.y >= map_y + 16 || slstate->xy.y < map_y - 16) equivalent?
    if (map_y + 16 < slstate->xy.y || slstate->xy.y + 16 < map_y)
      goto next;

middle_bit:
    // if searchlight.use_full_window == 0:
    //   the light is clipped to the left side of the window
    use_full_window = 0; // flag
    // EX AF,AF'

    // HL -> slstate->x OR -> state->searchlight.caught_coord.x
    B = 0;
    if (slstate->xy.x < map_x)
    {
      B = 255; // -ve
      use_full_window = ~use_full_window; // toggle flag 0 <-> 255
    }
    C = slstate->xy.x; // x
    // now BC is ready here

    // HL -> slstate->y OR -> state->searchlight.caught_coord.y
    y = slstate->xy.y;
    H = 0;
    if (y < map_y)
      H = 255; // -ve
    L = y;
    // now HL is ready here

    {
      // Conv: cast the original game vars into proper signed offsets

      int column;
      int row;

      column = (int16_t)((B << 8) | C);
      row    = (int16_t)((H << 8) | L);

      // HL must be row, BC must be column
      attrs = &state->speccy->attributes[0x46 + row * state->width + column]; // 0x46 = address of top-left game window attribute
    }

    state->searchlight.use_full_window = use_full_window;
    searchlight_plot(state, attrs); // DE turned into HL from EX above

next:
    // POP HL
    // POP BC
    slstate++;
  }
  while (--iters);
}

/**
 * $AE78: Is the hero is caught in the spotlight?
 *
 * \param[in] state   Pointer to game state.
 * \param[in] slstate Pointer to per-searchlight state. (was HL)
 */
void searchlight_caught(tgestate_t                   *state,
                        const searchlight_movement_t *slstate)
{
  uint8_t map_y, map_x; /* was D,E */

  assert(state   != NULL);
  assert(slstate != NULL);

  map_y = state->map_position.y;
  map_x = state->map_position.x;

  if ((slstate->xy.x + 5 >= map_x + 12 || slstate->xy.x + 10 < map_x + 10) ||
      (slstate->xy.y + 5 >= map_y + 10 || slstate->xy.y + 12 < map_y +  6))
    return;

  // FUTURE: We could rearrange the above like this:
  //if ((slm->x >= map_x + 7 || slm->x < map_x    ) ||
  //    (slm->y >= map_y + 5 || slm->y < map_y - 6))
  //  return;
  // But that needs checking for edge cases.

  /* It seems odd to not do this (cheaper) test sooner. */
  if (state->searchlight_state == searchlight_STATE_CAUGHT)
    return; /* already caught */

  state->searchlight_state = searchlight_STATE_CAUGHT;

  // CHECK: this x/y transpose looks dodgy
  state->searchlight.caught_coord.x = slstate->xy.y;
  state->searchlight.caught_coord.y = slstate->xy.x;

  state->bell = bell_RING_PERPETUAL;

  decrease_morale(state, 10); // exit via
}

/**
 * $AEB8: Searchlight plotter.
 *
 * \param[in] state Pointer to game state.
 * \param[in] attrs Pointer to screen attributes. (was DE)
 */
void searchlight_plot(tgestate_t *state, attribute_t *attrs)
{
  /**
   * $AF3E: Searchlight circle shape.
   */
  static const uint8_t searchlight_shape[2 * 16] =
  {
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x01, 0x80,
    0x07, 0xE0,
    0x0F, 0xF0,
    0x0F, 0xF0,
    0x1F, 0xF8,
    0x1F, 0xF8,
    0x0F, 0xF0,
    0x0F, 0xF0,
    0x07, 0xE0,
    0x01, 0x80,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
  };

  assert(state != NULL);

  attribute_t *const attrs_base = &state->speccy->attributes[0];
  ASSERT_SCREEN_ATTRIBUTES_PTR_VALID(attrs_base);

  const uint8_t *shape;       /* was DE' */
  uint8_t        iters;       /* was C' */
  uint8_t        use_full_window; /* was A */
  attribute_t   *max_y_attrs; /* was HL */
  attribute_t   *saved_attrs; /* was stack */
  attribute_t   *pattrs2;     /* was HL */
  uint8_t        iters2;      /* was B' */
  int            carry = 0;

  shape = &searchlight_shape[0];
  iters = 16; /* height */
  do
  {
    ptrdiff_t x; /* was A */

    use_full_window = state->searchlight.use_full_window; // a clipping flag maybe?

    /* Screen coords are x = 0..31, y = 0..23.
     * Game window occupies attribute rows 2..17 and columns 7..29 (inclusive).
     * Attribute row 18 is the row beyond the bottom edge of the game window. */

    x = (attrs - attrs_base) & 31; // '& 31' -> '% state->width'  Conv: hoisted

    // pattrs1 must be y limit
    max_y_attrs = &attrs_base[32 * 18]; // was HL = 0x5A40; // screen attribute address (column 0 + bottom of game screen)
    if (use_full_window)
    {
      // Conv: was: if ((E & 31) >= 22) L = 32;
      // E & 31 => 4th to last row, or later
      if (x >= 22) // 22 => 8 attrs away from the edge maybe?
        max_y_attrs = &attrs_base[32 * 19]; // colors in bottom border
    }
    if (attrs >= max_y_attrs) // gone off the end of the game screen?
      goto exit; // Conv: Original code just returned, goto exit instead so I can force a screen refresh down there.

    saved_attrs = attrs; // PUSH DE

    pattrs2 = &attrs_base[32 * 2]; // screen attribute address (row 2, column 0)
    if (use_full_window)
    {
      // Conv: was: if (E & 31) >= 7) L = 32;
      if (x >= 7)
        pattrs2 = &attrs_base[32 * 1]; // (row 1, column 0) // seems responsible for colouring in the top border
    }
    if (attrs < pattrs2)
    {
      shape += 2;
      goto next_row;
    }

    iters2 = 2; /* iterations */ // bytes per row
    do
    {
      uint8_t iters3; /* was B */
      uint8_t pixels; /* was C */

      pixels = *shape; // Conv: Original uses A as temporary.

      iters3 = 8; /* iterations */ // bits per byte
      do
      {
        x = (attrs - attrs_base) & 31; // Conv: was interleaved; '& 31's hoisted from below; '& 31' -> '% state->width' as above
        if (!use_full_window)
        {
          if (x >= 22)
            // don't render right of the game window
            goto dont_plot;
        }
        else
        {
          if (x >= 30) // Conv: Constant 30 was in E
          {
            // don't render right of the game window
            do
              shape++;
            while (--iters2);

            goto next_row;
          }
        }

        // plot
        if (x < 7) // Conv: Constant 7 was in D
        {
          // don't render left of the game window
dont_plot:
          RL(pixels); // no plot
        }
        else
        {
          RL(pixels);
          if (carry)
            *attrs = attribute_YELLOW_OVER_BLACK;
          else
            *attrs = attribute_BRIGHT_BLUE_OVER_BLACK;
        }
        attrs++;
      }
      while (--iters3);

      shape++;
    }
    while (--iters2);

next_row:
    attrs = saved_attrs + state->width;
  }
  while (--iters);

exit:
  state->speccy->draw(state->speccy, NULL);
}

/* ----------------------------------------------------------------------- */

/**
 * $AF8F: Test for characters meeting obstacles like doors and map bounds.
 *
 * Also assigns saved_pos to specified vischar's pos and sets the sprite_index.
 *
 * \param[in] state        Pointer to game state.
 * \param[in] vischar      Pointer to visible character. (was IY)
 * \param[in] sprite_index Flip flag + sprite offset. (was A')
 *
 * \return 0/1 => within/outwith bounds
 */
int touch(tgestate_t *state, vischar_t *vischar, spriteindex_t sprite_index)
{
  spriteindex_t stashed_sprite_index; /* was $81AA */ // FUTURE: Can be removed.

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert((sprite_index & ~sprite_FLAG_FLIP) < sprite__LIMIT);

  stashed_sprite_index = sprite_index;
  vischar->counter_and_flags |= vischar_BYTE7_TOUCHING | vischar_BYTE7_LOCATABLE; // wild guess: clamp character in position?

  // Conv: Removed little register shuffle to get (vischar & 0xFF) into A.

  /* If hero is player controlled then check for door transitions. */
  if (vischar == &state->vischars[0] && state->automatic_player_counter > 0)
    door_handling(state, vischar);

  /* If non-player character or hero is not cutting the fence. */
  // NPCs are not bounds checked
  // player is not bounds checked when cutting the fence
  if (vischar > &state->vischars[0] || ((state->vischars[0].flags & (vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE)) != vischar_FLAGS_CUTTING_WIRE))
  {
    if (bounds_check(state, vischar))
      return 1; /* Within wall bounds. */
  }

  if (vischar->character <= character_25_PRISONER_6) /* character temp was in Adash */
    if (collision(state))
      return 1; // NZ

  /* At this point we handle non-colliding characters and items only. */

  vischar->counter_and_flags &= ~vischar_BYTE7_TOUCHING; // clear
  vischar->mi.pos           = state->saved_pos.pos;
  vischar->mi.sprite_index  = stashed_sprite_index; // left/right flip flag / sprite offset

  // A = 0; likely just to set flags
  return 0; // Z
}

/* ----------------------------------------------------------------------- */

/**
 * $AFDF: (unknown) Definitely something to do with moving objects around. Collisions?
 *
 * \param[in] state         Pointer to game state.
 *
 * \return 0/1 => ?
 */
int collision(tgestate_t *state)
{
  vischar_t  *vischar;      /* was HL */
  uint8_t     iters;        /* was B */
  uint16_t    x;            /* was BC */
  uint16_t    y;            /* was BC */
  uint16_t    saved_x;      /* was HL */
  uint16_t    saved_y;      /* was HL */
  int8_t      delta;        /* was A */
  uint8_t     A;
  character_t character;    /* was A */
  uint8_t     B;
  uint8_t     C;
  uint8_t     input;
  uint8_t     direction;     /* was A (CHECK) */
  uint16_t    new_direction; /* was BC */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(state->IY);

  vischar = &state->vischars[0];
  iters = vischars_LENGTH - 1;
  do
  {
    if (vischar->flags & vischar_FLAGS_NO_COLLIDE) // sampled = $8001, $8021, ...
      goto next;

    /*
     * Check for contact between current vischar and saved_pos.
     * +/-4 for x/y
     */

    // PUSH BC, HL here

    // Check: Check this all over...

// This is like:
//      if (state->saved_pos.x > vischar->mi.pos.x + 4 ||
//          state->saved_pos.x < vischar->mi.pos.x - 4)
//        goto pop_next;
    x = vischar->mi.pos.x + 4;
    saved_x = state->saved_pos.pos.x;
    if (saved_x != x)
      if (saved_x > x || state->saved_pos.pos.x < x - 8) // redundant reload of var
        goto pop_next;

    y = vischar->mi.pos.y + 4;
    saved_y = state->saved_pos.pos.y;
    if (saved_y != y)
      if (saved_y > y || state->saved_pos.pos.y < y - 8) // redundant reload of var
        goto pop_next;

    // Check the heights are within 24 of each other
    delta = state->saved_pos.pos.height - vischar->mi.pos.height; // Note: Signed result.
    if (delta < 0)
      delta = -delta; // absolute value
    if (delta >= 24)
      goto pop_next;

    /* If current vischar character has a bribe pending... */
    // Odd: 0x0F is *not* vischar_FLAGS_MASK, which is 0x3F
    if ((state->IY->flags & 0x0F) == vischar_FLAGS_BRIBE_PENDING) // sampled IY=$8020, $8040, $8060, $8000
    {
      /* and current vischar is not the hero... */
      if (vischar != &state->vischars[0])
      {
        if (state->bribed_character == state->IY->character)
        {
          accept_bribe(state);
        }
        else
        {
          /* A hostile catches the hero! */

          // POP HL
          // POP BC
          vischar = state->IY + 1; // 1 byte == 2 bytes into struct => vischar->route
          // but .. that's not needed
          solitary(state); // is this supposed to take a route?
          NEVER_RETURNS 0;
        }
      }
    }

    // POP HL // $8021 etc.

    character = vischar->character; // sampled HL = $80C0, $8040, $8000, $8020, $8060 // vischar_BYTE0
    if (character >= character_26_STOVE_1)
    {
      /* Movable items: Stove 1, Stove 2 or Crate */

      // Stoves move on which axis? screenwise it's bottom-left to top-right
      // Crate moves on which axis? screenwise it's bottom-right to top-left

      // PUSH HL
      uint16_t *coord; /* was HL */

      coord = &vischar->mi.pos.y; // ok

      B = 7; // permitted range from centre point C
      C = 35; // cetre point (29 .. 35 .. 42)
      direction = state->IY->direction; // was interleaved
      if (character == character_28_CRATE)
      {
        /* Crate moves on x(?) axis only. */
        coord--; // -> HL->mi.pos.x
        C = 54; // cetre point (47 .. 54 .. 61)
        direction ^= 1; /* swap direction: left <=> right */
      }

      switch (direction)
      {
        case direction_TOP_LEFT:
          /* The player is pushing the movable item forward so centre it. */
          A = *coord; // note: narrowing // orig code loads bytes here?
          if (A != C) // hit when hero pushes to top-left (screenwise)
          {
            // Conv: Replaced[-2]+1 trick
            if (A > C)
              (*coord)--;
            else
              (*coord)++;
          }
          break;

        case direction_TOP_RIGHT: // hit when hero pushes to top-right (screenwise)
          if (*coord != (C + B))
            (*coord)++;
          break;

        case direction_BOTTOM_RIGHT: // likely never happens in game
          *coord = C - B;
          break;

        case direction_BOTTOM_LEFT: // hit when hero pushes to bottom-left (screenwise)
          if (*coord != (C - B))
            (*coord)--;
          break;

        default:
          assert("Should not happen" == NULL);
          break;
       }

      // POP HL
      // POP BC
    }

    input = vischar->input & ~input_KICK; // sampled HL = $806D, $804D, $802D, $808D, $800D
    if (input)
    {
      assert(input < 9);

      direction = vischar->direction ^ 2; /* swap direction: top <=> bottom */
      if (direction != state->IY->direction)
      {
        state->IY->input = input_KICK;

b0d0:
        // is the 5 a character_behaviour delay field?
        state->IY->counter_and_flags = (state->IY->counter_and_flags & vischar_BYTE7_MASK_HI) | 5; // preserve flags and set 5? // sampled IY = $8000, $80E0
        // Weird code in the original game which ORs 5 then does a conditional return dependent on Z clear, which it won't be.
        //if (!Z)
          return 1; /* odd -- returning with Z not set */
      }
    }

    {
      /** $B0F8: New inputs. */
      static const uint8_t new_inputs[4] =
      {
        input_DOWN + input_LEFT  + input_KICK,
        input_UP   + input_LEFT  + input_KICK,
        input_UP   + input_RIGHT + input_KICK,
        input_DOWN + input_RIGHT + input_KICK
      };

      new_direction = state->IY->direction;
      assert(new_direction < 4);
      state->IY->input = new_inputs[new_direction]; // sampled IY = $8000, $8040, $80E0
      assert((state->IY->input & ~input_KICK) < 9);
      if ((new_direction & 1) == 0) /* TL or BR */
        state->IY->counter_and_flags &= ~vischar_BYTE7_IMPEDED;
      else
        state->IY->counter_and_flags |= vischar_BYTE7_IMPEDED;
      goto b0d0;
    }

pop_next:
    // POP HL
    // POP BC
next:
    vischar++;
  }
  while (--iters);

  return 0; // return with Z set
}

/* ----------------------------------------------------------------------- */

/**
 * $B107: Character accepts the bribe.
 *
 * \param[in] state   Pointer to game state.
 */
void accept_bribe(tgestate_t *state)
{
  item_t    *item;     /* was DE */
  uint8_t    iters;    /* was B */
  vischar_t *vischar2; /* was HL */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(state->IY);

  increase_morale_by_10_score_by_50(state);

  state->IY->flags = 0;

  (void) get_target_and_handle_it(state, state->IY, &state->IY->route); /* FIXME these IYs. route should be HL? */

  item = &state->items_held[0];
  if (*item != item_BRIBE && *++item != item_BRIBE)
    return; /* Have no bribes. */

  /* We have a bribe, take it away. */
  *item = item_NONE;

  state->item_structs[item_BRIBE].room_and_flags = (room_t) itemstruct_ROOM_NONE;

  draw_all_items(state);

  /* Iterate over visible hostiles. */
  iters    = vischars_LENGTH - 1;
  vischar2 = &state->vischars[1];
  do
  {
    if (vischar2->character <= character_19_GUARD_DOG_4) /* All hostiles. */
      vischar2->flags = vischar_FLAGS_SAW_BRIBE; // hostile will look for bribed character?
    vischar2++;
  }
  while (--iters);

  queue_message_for_display(state, message_HE_TAKES_THE_BRIBE);
  queue_message_for_display(state, message_AND_ACTS_AS_DECOY);
}

/* ----------------------------------------------------------------------- */

/**
 * $B14C: Affirms that the specified character is touching/inside of
 * wall/fence bounds.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 *
 * \return 1 => within wall bounds, 0 => outwith
 */
int bounds_check(tgestate_t *state, vischar_t *vischar)
{
  uint8_t       iters; /* was B */
  const wall_t *wall;  /* was DE */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  if (state->room_index > room_0_OUTDOORS)
    return interior_bounds_check(state, vischar);

  iters = NELEMS(walls); /* All walls and fences. */
  wall  = &walls[0];
  do
  {
    uint16_t minx, maxx, miny, maxy, minheight, maxheight;

    minx      = wall->minx      * 8;
    maxx      = wall->maxx      * 8;
    miny      = wall->miny      * 8;
    maxy      = wall->maxy      * 8;
    minheight = wall->minheight * 8;
    maxheight = wall->maxheight * 8;

    if (state->saved_pos.pos.x      >= minx + 2   &&
        state->saved_pos.pos.x      <  maxx + 4   &&
        state->saved_pos.pos.y      >= miny       &&
        state->saved_pos.pos.y      <  maxy + 4   &&
        state->saved_pos.pos.height >= minheight  &&
        state->saved_pos.pos.height <  maxheight + 2)
    {
      vischar->counter_and_flags ^= vischar_BYTE7_IMPEDED;
      return 1; // NZ
    }

    wall++;
  }
  while (--iters);

  return 0; // Z
}

/* ----------------------------------------------------------------------- */

///**
// * $B1C7: Multiplies A by 8, returning the result in BC.
// *
// * Leaf.
// *
// * \param[in] A to be multiplied and widened.
// *
// * \return 'A' * 8 widened to a uint16_t. (was BC)
// */
//uint16_t multiply_by_8(uint8_t A)
//{
//  return A * 8;
//}

/* ----------------------------------------------------------------------- */

/**
 * $B1D4: Locate current door, queuing a message if it's locked.
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0 => open, 1 => locked.
 */
int is_door_locked(tgestate_t *state)
{
  doorindex_t  cur;   /* was C */
  doorindex_t *door;  /* was HL */
  uint8_t      iters; /* was B */

  assert(state != NULL);

  cur   = state->current_door & ~door_LOCKED; // probably door_REVERSE
  door  = &state->locked_doors[0];
  iters = NELEMS(state->locked_doors);
  do
  {
    if ((*door & ~door_LOCKED) == cur) // is definitely door_LOCKED, as 'door' is fetched from locked_doors[]
    {
      if ((*door & door_LOCKED) == 0)
        return 0; /* Door is open. */

      queue_message_for_display(state, message_THE_DOOR_IS_LOCKED);
      return 1; /* Door is locked. */
    }
    door++;
  }
  while (--iters);

  return 0; /* Door is open. */
}

/* ----------------------------------------------------------------------- */

/**
 * $B1F5: Door handling.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 *
 * \remarks Exits using longjmp.
 */
void door_handling(tgestate_t *state, vischar_t *vischar)
{
  const door_t *door_pos;  /* was HL */
  direction_t   direction; /* was E */
  uint8_t       iters;     /* was B */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  /* Interior doors are handled by another routine. */
  if (state->room_index > room_0_OUTDOORS)
  {
    door_handling_interior(state, vischar); // exit via
    return;
  }

  /* Select a start position in doors based on the direction the hero is
   * facing. */
  door_pos = &doors[0];
  direction = vischar->direction;
  if (direction >= direction_BOTTOM_RIGHT) /* BOTTOM_RIGHT or BOTTOM_LEFT */
    door_pos = &doors[1];

  /* The first 16 (pairs of) entries in doors[] are the only ones with
   * room_0_OUTDOORS as a destination, so only consider those. */
  iters = 16;
  do
  {
    if ((door_pos->room_and_flags & door_FLAGS_MASK_DIRECTION) == direction)
      if (door_in_range(state, door_pos))
        goto found;
    door_pos += 2;
  }
  while (--iters);

  /* Conv: Removed unused 'A &= B' op. */

  return;

found:
  state->current_door = 16 - iters;

  if (is_door_locked(state))
    return;

  vischar->room = (door_pos->room_and_flags & ~door_FLAGS_MASK_DIRECTION) >> 2; // sampled HL = $792E (in doors[])
  ASSERT_ROOM_VALID(vischar->room);
  if ((door_pos->room_and_flags & door_FLAGS_MASK_DIRECTION) < direction_BOTTOM_RIGHT) /* TL or TR */
    /* Point to the next door's pos */
    transition(state, &door_pos[1].pos);
  else
    /* Point to the previous door's pos */
    transition(state, &door_pos[-1].pos);

  NEVER_RETURNS; // highly likely if this is only tiggered by the hero
}

/* ----------------------------------------------------------------------- */

/**
 * $B252: Door in range. Called for exterior doors only.
 *
 * (saved_X,saved_Y) within (-2,+2) of HL[1..] scaled << 2
 *
 * \param[in] state Pointer to game state.
 * \param[in] door  Pointer to door_t. (was HL)
 *
 * \return 1 if in range, 0 if not. (was carry flag)
 */
int door_in_range(tgestate_t *state, const door_t *door)
{
  const int halfdist = 3;

  uint16_t x, y; /* was BC, BC */

  assert(state != NULL);
  assert(door  != NULL);

  x = multiply_by_4(door->pos.x);
  if (state->saved_pos.pos.x < x - halfdist || state->saved_pos.pos.x >= x + halfdist)
    return 0;

  y = multiply_by_4(door->pos.y);
  if (state->saved_pos.pos.y < y - halfdist || state->saved_pos.pos.y >= y + halfdist)
    return 0;

  return 1;
}

/* ----------------------------------------------------------------------- */

/**
 * $B295: Multiplies A by 4, returning the result in BC.
 *
 * Leaf.
 *
 * \param[in] A to be multiplied and widened.
 *
 * \return 'A' * 4 widened to a uint16_t.
 */
uint16_t multiply_by_4(uint8_t A)
{
  return A * 4;
}

/* ----------------------------------------------------------------------- */

/**
 * $B29F: Check the character is inside of bounds, when indoors.
 *
 * Leaf.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 *
 * \return 1 if a boundary was hit, 0 otherwise.
 */
int interior_bounds_check(tgestate_t *state, vischar_t *vischar)
{
  /**
   * $6B85: Room dimensions.
   */
  static const bounds_t roomdef_bounds[10] =
  {
    {  66, 26,  70, 22 },
    {  62, 22,  58, 26 },
    {  54, 30,  66, 18 },
    {  62, 30,  58, 34 },
    {  74, 18,  62, 30 },
    {  56, 50, 100, 10 },
    { 104,  6,  56, 50 },
    {  56, 50, 100, 26 },
    { 104, 28,  56, 50 },
    {  56, 50,  88, 10 },
  };

  const bounds_t *room_bounds;   /* was BC */
  const pos_t    *saved_pos;     /* was HL */
  const bounds_t *object_bounds; /* was HL */
  uint8_t         nbounds;       /* was B */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  room_bounds = &roomdef_bounds[state->roomdef_bounds_index];
  saved_pos = &state->saved_pos.pos;
  /* Conv: Merged conditions. */
  if (room_bounds->x0     < saved_pos->x || room_bounds->x1 + 4 >= saved_pos->x ||
      room_bounds->y0 - 4 < saved_pos->y || room_bounds->y1     >= saved_pos->y)
    goto stop;

  object_bounds = &state->roomdef_object_bounds[0]; /* Conv: Moved around. */
  for (nbounds = state->roomdef_object_bounds_count; nbounds > 0; nbounds--)
  {
    pos_t  *pos;  /* was DE */
    uint8_t x, y; /* was A, A */

    /* Conv: HL dropped. */
    pos = &state->saved_pos.pos;

    /* Conv: Two-iteration loop unrolled. */
    x = pos->x;  // note: narrowing // saved_pos must be 8-bit
    if (x < object_bounds->x0 || x >= object_bounds->x1)
      goto next;
    y = pos->y; // note: narrowing
    if (y < object_bounds->y0 || y >= object_bounds->y1)
      goto next;

stop:
    /* Found. */
    vischar->counter_and_flags ^= vischar_BYTE7_IMPEDED;
    return 1; /* return NZ */

next:
    /* Next iteration. */
    object_bounds++;
  }

  /* Not found. */
  return 0; /* return Z */
}

/* ----------------------------------------------------------------------- */

/**
 * $B2FC: Reset the hero's position, redraw the scene, then zoombox it onto
 * the screen.
 *
 * \param[in] state Pointer to game state.
 */
void reset_outdoors(tgestate_t *state)
{
  assert(state != NULL);

  /* Reset hero. */
  calc_vischar_screenpos_from_mi_pos(state, &state->vischars[0]);

  /* Centre the map position on the hero. */
  /* Conv: Removed divide_by_8 calls here. */
  state->map_position.x = (state->vischars[0].floogle.x >> 3) - 11; // 11 would be screen width minus half of character width?
  state->map_position.y = (state->vischars[0].floogle.y >> 3) - 6;  // 6 would be screen height minus half of character height?
  ASSERT_MAP_POSITION_VALID(state->map_position);

  state->room_index = room_0_OUTDOORS;
  get_supertiles(state);
  plot_all_tiles(state);
  setup_movable_items(state);
  zoombox(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B32D: Door handling (indoors).
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 */
void door_handling_interior(tgestate_t *state, vischar_t *vischar)
{
  doorindex_t     *pdoors;         /* was HL */
  doorindex_t      current_door;   /* was A */
  uint8_t          room_and_flags; /* was A */
  const door_t    *door;           /* was HL' */
  const tinypos_t *doorpos;        /* was HL' */
  pos_t           *pos;            /* was DE' */
  uint8_t          x;              /* was A */
  uint8_t          y;              /* was A */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  for (pdoors = &state->interior_doors[0]; ; pdoors++)
  {
    current_door = *pdoors;
    if (current_door == door_NONE)
      return; /* Reached end of list. */

    state->current_door = current_door;

    door = get_door(current_door);
    room_and_flags = door->room_and_flags;

    /* Does the character face the same direction as the door? */
    if ((vischar->direction & vischar_DIRECTION_MASK) != (room_and_flags & door_FLAGS_MASK_DIRECTION))
      continue;

    /* Skip any door which is more than three units away. */
    doorpos = &door->pos;
    pos = &state->saved_pos.pos; // reusing saved_pos as 8-bit here? a case for saved_pos being a union of tinypos and pos types?
    x = doorpos->x;
    if (x - 3 >= pos->x || x + 3 < pos->x) // -3 .. +2
      continue;
    y = doorpos->y;
    if (y - 3 >= pos->y || y + 3 < pos->y) // -3 .. +2
      continue;

    if (is_door_locked(state))
      return; /* The door was locked. */

    vischar->room = room_and_flags >> 2;

    doorpos = &door[1].pos;
    if (state->current_door & door_REVERSE)
      doorpos = &door[-1].pos;

    transition(state, doorpos); // exit via
    NEVER_RETURNS; // check
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B387: The hero has tried to open the red cross parcel.
 *
 * \param[in] state Pointer to game state.
 */
void action_red_cross_parcel(tgestate_t *state)
{
  item_t *item; /* was HL */

  assert(state != NULL);

  state->item_structs[item_RED_CROSS_PARCEL].room_and_flags = room_NONE & itemstruct_ROOM_MASK;

  item = &state->items_held[0];
  if (*item != item_RED_CROSS_PARCEL)
    item++; /* One or the other must be a red cross parcel item, so advance. */
  *item = item_NONE; /* Parcel opened. */

  draw_all_items(state);

  drop_item_tail(state, state->red_cross_parcel_current_contents);

  queue_message_for_display(state, message_YOU_OPEN_THE_BOX);
  increase_morale_by_10_score_by_50(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B3A8: The hero tries to bribe a prisoner.
 *
 * This searches visible friendly characters only and returns the first found.
 *
 * \param[in] state Pointer to game state.
 */
void action_bribe(tgestate_t *state)
{
  vischar_t *vischar;   /* was HL */
  uint8_t    iters;     /* was B */
  uint8_t    character; /* was A */

  assert(state != NULL);

  /* Walk non-player visible characters. */
  vischar = &state->vischars[1];
  iters = vischars_LENGTH - 1;
  do
  {
    character = vischar->character;
    if (character != character_NONE && character >= character_20_PRISONER_1)
      goto found;
    vischar++;
  }
  while (--iters);

  return;

found:
  state->bribed_character = character;
  vischar->flags = vischar_FLAGS_BRIBE_PENDING;
}

/* ----------------------------------------------------------------------- */

/**
 * $B3C4: Use poison.
 *
 * \param[in] state Pointer to game state.
 */
void action_poison(tgestate_t *state)
{
  assert(state != NULL);

  if (state->items_held[0] != item_FOOD &&
      state->items_held[1] != item_FOOD)
    return; /* Have no poison. */

  if (state->item_structs[item_FOOD].item_and_flags & itemstruct_ITEM_FLAG_POISONED)
    return; /* Food is already poisoned. */

  state->item_structs[item_FOOD].item_and_flags |= itemstruct_ITEM_FLAG_POISONED;

  state->item_attributes[item_FOOD] = attribute_BRIGHT_PURPLE_OVER_BLACK;

  draw_all_items(state);

  increase_morale_by_10_score_by_50(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B3E1: Use uniform.
 *
 * \param[in] state Pointer to game state.
 */
void action_uniform(tgestate_t *state)
{
  const spritedef_t *guard_sprite; /* was HL */

  assert(state != NULL);

  guard_sprite = &sprites[sprite_GUARD_FACING_AWAY_1];

  if (state->vischars[0].mi.sprite == guard_sprite)
    return; /* Already in uniform. */

  if (state->room_index >= room_29_SECOND_TUNNEL_START)
    return; /* Can't don uniform when in a tunnel. */

  state->vischars[0].mi.sprite = guard_sprite;

  increase_morale_by_10_score_by_50(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B3F6: Use shovel.
 *
 * \param[in] state Pointer to game state.
 */
void action_shovel(tgestate_t *state)
{
  assert(state != NULL);

  if (state->room_index != room_50_BLOCKED_TUNNEL)
    return; /* Shovel only works in the blocked tunnel room. */

  if (roomdef_50_blocked_tunnel[2] == 255)
    return; /* Blockage is already cleared. */

  /* Release boundary. */
  roomdef_50_blocked_tunnel[2] = 255;
  /* Remove blockage graphic. */
  roomdef_50_blocked_tunnel[roomdef_50_BLOCKAGE] = interiorobject_STRAIGHT_TUNNEL_SW_NE;

  setup_room(state);
  choose_game_window_attributes(state);
  plot_interior_tiles(state);
  increase_morale_by_10_score_by_50(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B416: Use wiresnips.
 *
 * \param[in] state Pointer to game state.
 */
void action_wiresnips(tgestate_t *state)
{
  const wall_t    *wall;  /* was HL */
  const tinypos_t *pos;   /* was DE */
  uint8_t          iters; /* was B */
  uint8_t          flag;  /* was A */

  assert(state != NULL);

  wall = &walls[12]; /* == .d; - start of fences */
  pos = &state->hero_map_position;
  iters = 4;
  do
  {
    uint8_t coord; /* was A */

    // check: this is using x then y which is the wrong order, isn't it?
    coord = pos->y;
    if (coord >= wall->miny && coord < wall->maxy) /* Conv: Reversed test order. */
    {
      coord = pos->x;
      if (coord == wall->maxx)
        goto set_to_4;
      if (coord - 1 == wall->maxx)
        goto set_to_6;
    }

    wall++;
  }
  while (--iters);

  wall = &walls[12 + 4]; // .a
  pos = &state->hero_map_position;
  iters = 3;
  do
  {
    uint8_t coord; /* was A */

    coord = pos->x;
    if (coord >= wall->minx && coord < wall->maxx)
    {
      coord = pos->y;
      if (coord == wall->miny)
        goto set_to_5;
      if (coord - 1 == wall->miny)
        goto set_to_7;
    }

    wall++;
  }
  while (--iters);

  return;

set_to_4: /* Crawl TL */
  flag = 4;
  goto action_wiresnips_tail;

set_to_5: /* Crawl TR. */
  flag = 5;
  goto action_wiresnips_tail;

set_to_6: /* Crawl BR. */
  flag = 6;
  goto action_wiresnips_tail;

set_to_7: /* Crawl BL. */
  flag = 7;

action_wiresnips_tail:
  state->vischars[0].direction      = flag; // walk/crawl flag
  state->vischars[0].input          = input_KICK;
  state->vischars[0].flags          = vischar_FLAGS_CUTTING_WIRE;
  state->vischars[0].mi.pos.height  = 12;
  state->vischars[0].mi.sprite      = &sprites[sprite_PRISONER_FACING_AWAY_1];
  state->player_locked_out_until = state->game_counter + 96;
  queue_message_for_display(state, message_CUTTING_THE_WIRE);
}

/* ----------------------------------------------------------------------- */

/**
 * $B495: Use lockpick.
 *
 * \param[in] state Pointer to game state.
 */
void action_lockpick(tgestate_t *state)
{
  doorindex_t *pdoor; /* was HL */

  assert(state != NULL);

  pdoor = get_nearest_door(state);
  if (pdoor == NULL)
    return; /* No door nearby. */

  state->ptr_to_door_being_lockpicked = pdoor;
  state->player_locked_out_until = state->game_counter + 255;
  state->vischars[0].flags = vischar_FLAGS_PICKING_LOCK;
  queue_message_for_display(state, message_PICKING_THE_LOCK);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4AE: Use red key.
 *
 * \param[in] state Pointer to game state.
 */
void action_red_key(tgestate_t *state)
{
  action_key(state, room_22_REDKEY);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4B2: Use yellow key.
 *
 * \param[in] state Pointer to game state.
 */
void action_yellow_key(tgestate_t *state)
{
  action_key(state, room_13_CORRIDOR);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4B6: Use green key.
 *
 * \param[in] state Pointer to game state.
 */
void action_green_key(tgestate_t *state)
{
  action_key(state, room_14_TORCH);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4B8: Use a key.
 *
 * \param[in] state       Pointer to game state.
 * \param[in] room_of_key Room number to which the key applies. (was A)
 */
void action_key(tgestate_t *state, room_t room_of_key)
{
  doorindex_t *pdoor;   /* was HL */
  message_t    message; /* was B */

  assert(state != NULL);
  ASSERT_ROOM_VALID(room_of_key);

  pdoor = get_nearest_door(state);
  if (pdoor == NULL)
    return; /* No door nearby. */

  if ((*pdoor & ~door_LOCKED) != room_of_key)
  {
    message = message_INCORRECT_KEY;
  }
  else
  {
    *pdoor &= ~door_LOCKED; /* Unlock. */
    increase_morale_by_10_score_by_50(state);
    message = message_IT_IS_OPEN;
  }

  queue_message_for_display(state, message);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4D0: Return the door in range of the hero.
 *
 * The hero's position is expected in state->saved_pos.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Pointer to door in state->locked_doors[], or NULL. (was HL)
 */
doorindex_t *get_nearest_door(tgestate_t *state)
{
  doorindex_t  *locked_doors;         /* was HL */
  uint8_t       iters;                /* was B */
  const door_t *door;                 /* was HL' */
  doorindex_t   locked_door_index;    /* was C */
  doorindex_t  *interior_doors;       /* was DE */
  pos_t        *pos;                  /* was DE' */
  doorindex_t   interior_door_index;  /* was A */

  assert(state != NULL);

  if (state->room_index == room_0_OUTDOORS)
  {
    /* Outdoors. */

    /* Locked doors 0..4 include exterior doors. */
    locked_doors = &state->locked_doors[0];
    iters = 5;
    do
    {
      door = get_door(*locked_doors & ~door_LOCKED); // Conv: A used as temporary.
      if (door_in_range(state, door + 0) == 0 ||
          door_in_range(state, door + 1) == 0)
        return locked_doors; /* Conv: goto removed. */

      locked_doors++;
    }
    while (--iters);

    return NULL; /* Not found */
  }
  else
  {
    /* Indoors. */

    /* Locked doors 2..8 include interior doors. */
    locked_doors = &state->locked_doors[2];
    iters = 8; // Bug: iters should be 7
    do
    {
      locked_door_index = *locked_doors & ~door_LOCKED;

      /* Search interior_doors[] for door_index. */
      interior_doors = &state->interior_doors[0];
each_interior_door:
      interior_door_index = *interior_doors;
      if (interior_door_index != door_NONE)
      {
        if ((interior_door_index & ~door_REVERSE) == locked_door_index) // this must be door_REVERSE as it came from interior_doors[]
          goto found;

        interior_doors++;
        /* There's no termination condition here where you might expect one,
         * but since every room has at least one door it *must* find one. */
        goto each_interior_door;
      }

next_locked_door:
      locked_doors++;
    }
    while (--iters);

    return NULL; /* Not found */


    // FUTURE: Move this into the body of the loop.
found:
    door = get_door(*interior_doors);
    /* Range check pattern (-2..+2). */
    pos = &state->saved_pos.pos; // note: 16-bit values holding 8-bit values
    // Conv: Unrolled.
    if (pos->x <= door->pos.x - 3 || pos->x > door->pos.x + 3 ||
        pos->y <= door->pos.y - 3 || pos->y > door->pos.y + 3)
      goto next_locked_door;

    return locked_doors;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B53E: Wall boundaries & $B586: Fence boundaries.
 *
 * Used by bounds_check and action_wiresnips.
 */
const wall_t walls[24] =
{
  { 106, 110,  82,  98, 0, 11 }, // hut 0 (leftmost)
  {  94,  98,  82,  98, 0, 11 }, // hut 1 (home hut)
  {  82,  86,  82,  98, 0, 11 }, // hut 2 (rightmost)
  {  62,  90, 106, 128, 0, 48 }, // main building, top right
  {  52, 128, 114, 128, 0, 48 }, // main building, topmost/right
  { 126, 152,  94, 128, 0, 48 }, // main building, top left
  { 130, 152,  90, 128, 0, 48 }, // main building, top left
  { 134, 140,  70, 128, 0, 10 }, // main building, left wall
  { 130, 134,  70,  74, 0, 18 }, // corner, bottom left
  { 110, 130,  70,  71, 0, 10 }, // front wall
  { 109, 111,  69,  73, 0, 18 }, // gate post (left)
  { 103, 105,  69,  73, 0, 18 }, // gate post (right)
  {  70,  70,  70, 106, 0,  8 }, // fence - right of main camp
  {  62,  62,  62, 106, 0,  8 }, // fence - rightmost fence
  {  78,  78,  46,  62, 0,  8 }, // fence - rightmost of yard
  { 104, 104,  46,  69, 0,  8 }, // fence - leftmost of yard
  {  62, 104,  62,  62, 0,  8 }, // fence - top of yard
  {  78, 104,  46,  46, 0,  8 }, // fence - bottom of yard
  {  70, 103,  70,  70, 0,  8 }, // fence - bottom of main camp
  { 104, 106,  56,  58, 0,  8 }, // fence - watchtower (left of yard)
  {  78,  80,  46,  48, 0,  8 }, // fence - watchtower (in yard)
  {  70,  72,  70,  72, 0,  8 }, // fence - watchtower (bottom left of main camp)
  {  70,  72,  94,  96, 0,  8 }, // fence - watchtower (top right of main camp)
  { 105, 109,  70,  73, 0,  8 }, // fence - gate
};

/* ----------------------------------------------------------------------- */

/**
 * $B5CE: called_from_main_loop_9
 *
 * work out screen positions and animations
 *
 * - movement
 * - crash if disabled, crash if iters reduced from 8
 *
 * \param[in] state Pointer to game state.
 */
void called_from_main_loop_9(tgestate_t *state)
{
#define O (0<<7)
#define I (1<<7) // could be: play anim backwards

  /**
   * $CDAA: Maps (facing direction, user input) to animation + flag.
   */
  static const uint8_t byte_CDAA[8 /*direction*/][9 /*input*/] =
  {
    // (U/D/L/R = Up/Down/Left/Right)
    // None,  U,      D,      L,      U+L,    D+L,    R,      U+R,    D+R
    { O| 8, O| 0, O| 4, I| 7, O| 0, I| 7, O| 4, O| 4, O| 4 }, // TL
    { O| 9, I| 4, O| 5, O| 5, I| 4, O| 5, O| 1, O| 1, O| 5 }, // TR
    { O|10, I| 5, O| 2, O| 6, I| 5, O| 6, I| 5, I| 5, O| 2 }, // BR
    { O|11, O| 7, I| 6, O| 3, O| 7, O| 3, O| 7, O| 7, I| 6 }, // BL
    { O|20, O|12, I|12, I|19, O|12, I|19, O|16, O|16, I|12 }, // TL + Crawl
    { O|21, I|16, O|17, I|13, I|16, I|21, O|13, O|13, O|17 }, // TR + Crawl
    { O|22, I|14, O|14, O|18, I|14, O|14, I|17, I|17, O|14 }, // BR + Crawl
    { O|23, O|19, I|18, O|15, O|19, O|15, I|15, I|15, I|18 }, // BL + Crawl
  };
  // highest index = 23 (== max index in animations)

  uint8_t        iters;     /* was B */
  const uint8_t *anim;      /* was HL */
  uint8_t        animindex; /* was A */
  uint8_t        A;         /* was A */
  uint8_t        C;         /* was C */
  const uint8_t *DE;        /* was HL */
  spriteindex_t  Adash;     /* was A' */
  vischar_t     *vischar;   /* a cache of IY (Conv: added) */

  assert(state != NULL);

  anim = DE = NULL; // Conv: init
  Adash = 0; // Conv: initialise

  iters     = vischars_LENGTH;
  state->IY = &state->vischars[0];
  do
  {
    vischar = state->IY; /* Conv: Added local copy of IY. */

    if (vischar->flags == vischar_FLAGS_EMPTY_SLOT)
      goto next;

    assert((vischar->input & ~input_KICK) < 9);

    vischar->flags |= vischar_FLAGS_NO_COLLIDE;

    if (vischar->input & input_KICK)
      goto kicked; /* force an update */

    assert(vischar->input < 9);

    anim = vischar->anim;
    animindex = vischar->animindex;
    if (animindex & vischar_ANIMINDEX_BIT7) // up/down flag
    {
      animindex &= ~vischar_ANIMINDEX_BIT7;
      if (animindex == 0)
        goto end_bit;

      anim += (animindex + 1) * 4 - 1; /* 4..512 + 1 */
      A = *anim++; // a spriteindex_t

      SWAP(uint8_t, A, Adash);

// Conv: Simplified sign extend sequence to this. Too far?
#define SXT_8_16(P) ((uint16_t) (*(int8_t *) (P)))

decrement:
      SWAP(const uint8_t *, DE, anim);

      // sampled DE = $CF9A, $CF9E, $CFBE, $CFC2, $CFB2, $CFB6, $CFA6, $CFAA (animations)

      state->saved_pos.pos.x = vischar->mi.pos.x - SXT_8_16(DE);
      DE++;
      state->saved_pos.pos.y = vischar->mi.pos.y - SXT_8_16(DE);
      DE++;
      state->saved_pos.pos.height = vischar->mi.pos.height - SXT_8_16(DE);

      if (touch(state, vischar, Adash /* sprite_index */))
        goto pop_next;

      vischar->animindex--;
    }
    else
    {
      if (animindex == *anim)
        goto end_bit;

      anim += (animindex + 1) * 4;

increment:
      SWAP(const uint8_t *, DE, anim);

      state->saved_pos.pos.x = vischar->mi.pos.x + SXT_8_16(DE);
      DE++;
      state->saved_pos.pos.y = vischar->mi.pos.y + SXT_8_16(DE);
      DE++;
      state->saved_pos.pos.height = vischar->mi.pos.height + SXT_8_16(DE);
      DE++;

      A = *DE; // a spriteindex_t

      SWAP(uint8_t, A, Adash);

      if (touch(state, vischar, Adash /* sprite_index */))
        goto pop_next;

      vischar->animindex++;
    }

    calc_vischar_screenpos(state, vischar);

pop_next:
    if (vischar->flags != vischar_FLAGS_EMPTY_SLOT)
      vischar->flags &= ~vischar_FLAGS_NO_COLLIDE; // $8001

next:
    state->IY++;
  }
  while (--iters);

  return;


kicked:
  vischar->input &= ~input_KICK;
  assert(vischar->input < 9);

end_bit:
  assert(vischar->input < 9);
  assert(vischar->direction < 8);
  C = byte_CDAA[vischar->direction][vischar->input];
  // Conv: Original game uses ADD A,A to double A and in doing so discards top flag bit. We mask it off instead.
  assert((C & ~I) < 24);
  DE = vischar->animbase[C & ~I]; // sampled animbase always $CDF2 == &animations[0]
  vischar->anim = DE;
  if ((C & I) == 0)
  {
    vischar->animindex = 0;
    DE += 2;
    vischar->direction = *DE;
    DE += 2; // point to groups of four
    SWAP(const uint8_t *, DE, anim);
    goto increment;
  }
  else
  {
    const uint8_t *stacked;

    C = *DE; // count of four-byte groups
    vischar->animindex = C | vischar_ANIMINDEX_BIT7;
    vischar->direction = *++DE;
    DE += 3; // point to groups of four
    stacked = DE;
    SWAP(const uint8_t *, DE, anim);
    anim += C * 4 - 1;
    A = *anim; // last byte in group of four, flip flag?
    SWAP(uint8_t, A, Adash);
    anim = stacked;
    goto decrement;
  }

#undef I
#undef O
}

/* ----------------------------------------------------------------------- */

/**
 * $B71B: Calculate screen position for the specified vischar from mi.pos.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was HL)
 */
void calc_vischar_screenpos_from_mi_pos(tgestate_t *state, vischar_t *vischar)
{
  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  /* Save a copy of the vischar's position + offset. */
  state->saved_pos.pos = vischar->mi.pos;

  calc_vischar_screenpos(state, vischar);
}

/**
 * $B729: Calculate screen position for the specified vischar from saved_pos.
 *
 * \see drop_item_tail_interior.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was HL)
 */
void calc_vischar_screenpos(tgestate_t *state, vischar_t *vischar)
{
  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  /* Conv: Reordered. */
  vischar->floogle.x = (0x200 - state->saved_pos.pos.x + state->saved_pos.pos.y) * 2;
  vischar->floogle.y = 0x800 - state->saved_pos.pos.x - state->saved_pos.pos.y - state->saved_pos.pos.height;

  // Measured: (-1664..3696, -1648..2024)
  // Rounding up:
  assert((int16_t) vischar->floogle.x >= -3000);
  assert((int16_t) vischar->floogle.x <   7000);
  assert((int16_t) vischar->floogle.y >= -3000);
  assert((int16_t) vischar->floogle.y <   4000);
}

/* ----------------------------------------------------------------------- */

/**
 * $B75A: Reset the game.
 *
 * \param[in] state Pointer to game state.
 */
void reset_game(tgestate_t *state)
{
  int    iters; /* was B */
  item_t item;  /* was C */

  assert(state != NULL);

  /* Cause discovery of all items. */
  iters = item__LIMIT;
  item = 0;
  do
    item_discovered(state, item++);
  while (--iters);

  /* Reset message queue. */
  state->messages.queue_pointer = &state->messages.queue[2];
  reset_map_and_characters(state);
  state->vischars[0].flags = 0;

  /* Reset score. */
  memset(&state->score_digits[0], 0, 5);

  state->hero_in_breakfast        = 0;
  state->red_flag                 = 0;
  state->automatic_player_counter = 0;
  state->in_solitary              = 0;
  state->morale_exhausted         = 0;

  /* Reset morale. */
  state->morale = morale_MAX;
  plot_score(state);

  /* Reset and redraw items. */
  state->items_held[0] = item_NONE;
  state->items_held[1] = item_NONE;
  draw_all_items(state);

  /* Reset the hero's sprite. */
  state->vischars[0].mi.sprite = &sprites[sprite_PRISONER_FACING_AWAY_1];

  /* Put the hero to bed. */
  state->room_index = room_2_HUT2LEFT;
  hero_sleeps(state);

  enter_room(state); // returns by goto main_loop
  NEVER_RETURNS;

  // Bug: item_structs ought to have each element's flags restored so that
  // itemstruct_ITEM_FLAG_HELD is reset.

  // Bug: fails to reset position of stoves and crate. (IIRC DOS version
  // resets them whenever they're spawned).
}

/* ----------------------------------------------------------------------- */

/**
 * $B79B: Resets all visible characters, clock, day_or_night flag, general
 * flags, collapsed tunnel objects, locks the gates, resets all beds, clears
 * the mess halls and resets characters.
 *
 * \param[in] state Pointer to game state.
 */
void reset_map_and_characters(tgestate_t *state)
{
  typedef struct character_reset_partial
  {
    room_t  room;
    uint8_t x, y; /* partial of a tinypos_t */
  }
  character_reset_partial_t;

  /**
   * $B819: Partial character structs for reset.
   */
  static const character_reset_partial_t character_reset_data[10] =
  {
    { room_3_HUT2RIGHT, 40, 60 }, /* for character 12 Guard */
    { room_3_HUT2RIGHT, 36, 48 }, /* for character 13 Guard */
    { room_5_HUT3RIGHT, 40, 60 }, /* for character 14 Guard */
    { room_5_HUT3RIGHT, 36, 34 }, /* for character 15 Guard */
    { room_NONE,        52, 60 }, /* for character 20 Prisoner */
    { room_NONE,        52, 44 }, /* for character 21 Prisoner */
    { room_NONE,        52, 28 }, /* for character 22 Prisoner */
    { room_NONE,        52, 60 }, /* for character 23 Prisoner */
    { room_NONE,        52, 44 }, /* for character 24 Prisoner */
    { room_NONE,        52, 28 }, /* for character 25 Prisoner */
  };

  uint8_t                          iters;   /* was B */
  vischar_t                       *vischar; /* was HL */
  uint8_t                         *gate;    /* was HL */
  uint8_t *const                  *bed;     /* was HL */
  characterstruct_t               *charstr; /* was DE */
  uint8_t                          iters2;  /* was C */
  const character_reset_partial_t *reset;   /* was HL */

  assert(state != NULL);

  iters = vischars_LENGTH - 1;
  vischar = &state->vischars[1]; /* iterate over non-player characters */
  do
    reset_visible_character(state, vischar++);
  while (--iters);

  state->clock = 7;
  state->day_or_night = 0;
  state->vischars[0].flags = 0;
  roomdef_50_blocked_tunnel[roomdef_50_BLOCKAGE] = interiorobject_COLLAPSED_TUNNEL_SW_NE;
  roomdef_50_blocked_tunnel[2] = 52; /* Reset boundary. */

  /* Lock the gates and doors. */
  gate = &state->locked_doors[0];
  iters = 9;
  do
    *gate++ |= door_LOCKED;
  while (--iters);

  /* Reset all beds. */
  iters = beds_LENGTH;
  bed = &beds[0];
  do
    **bed++ = interiorobject_OCCUPIED_BED;
  while (--iters);

  /* Clear the mess halls. */
  roomdef_23_breakfast[roomdef_23_BENCH_A] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_B] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_C] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_D] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_E] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_F] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_EMPTY_BENCH;

  /* Reset characters 12..15 (guards) and 20..25 (prisoners). */
  charstr = &state->character_structs[character_12_GUARD_12];
  iters2 = NELEMS(character_reset_data);
  reset = &character_reset_data[0];
  do
  {
    charstr->room        = reset->room;
    charstr->pos.x       = reset->x;
    charstr->pos.y       = reset->y;
    charstr->pos.height  = 18; /* Bug/Odd: This is reset to 18 but the initial data is 24. */
    charstr->route.index = 0; /* Stand still */
    charstr++;
    reset++;

    if (iters2 == 7) /* when 7 remain */
      charstr = &state->character_structs[character_20_PRISONER_1];
  }
  while (--iters2);
}

/* ----------------------------------------------------------------------- */

/**
 * $B83B: Check the mask data to see if the hero is hiding behind something.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void searchlight_mask_test(tgestate_t *state, vischar_t *vischar)
{
  uint8_t     *buf;   /* was HL */
  attribute_t  attrs; /* was A */
  uint8_t      iters; /* was B */
  //uint8_t      C;     /* was C */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  if (vischar > &state->vischars[0])
    return; /* Skip non-hero characters. */

  /* Start testing at approximately the middle of the character. */
  buf = &state->mask_buffer[32 + 16 + 1]; /* x=1, y=12 */
  iters = 8;
  /* Bug: Original code does a fused load of BC, but doesn't use C after.
   * Probably a leftover stride constant for MASK_BUFFER_WIDTHBYTES. */
  // C = 4;
  do
  {
    if (*buf != 0)
      goto still_in_spotlight;
    buf += MASK_BUFFER_WIDTHBYTES;
  }
  while (--iters);

  /* Otherwise the hero has escaped the spotlight, so decrement the counter. */
  if (--state->searchlight_state == searchlight_STATE_SEARCHING) // state went 255
  {
    attrs = choose_game_window_attributes(state);
    set_game_window_attributes(state, attrs);
  }

  return;

still_in_spotlight:
  state->searchlight_state = searchlight_STATE_CAUGHT;
}

/* ----------------------------------------------------------------------- */

#define item_FOUND (1 << 6) // set by get_greatest_itemstruct

/**
 * $B866: Plot vischars and items in order.
 *
 * \param[in] state Pointer to game state.
 */
void plot_sprites(tgestate_t *state)
{
  uint8_t       index;      /* was A */
  vischar_t    *vischar;    /* was IY */
  itemstruct_t *itemstruct; /* was IY */
  int           found;      /* was Z */

  assert(state != NULL);

  for (;;)
  {
    /* This can return a vischar OR an itemstruct, but not both. */
    found = locate_vischar_or_itemstruct(state, &index, &vischar, &itemstruct);
    if (!found)
      return;

#ifndef NDEBUG
    if (vischar)
      ASSERT_VISCHAR_VALID(vischar);
    else
      assert(itemstruct != NULL);
#endif

    if ((index & item_FOUND) == 0)
    {
      found = setup_vischar_plotting(state, vischar);
      if (found)
      {
        render_mask_buffer(state);
        if (state->searchlight_state != searchlight_STATE_SEARCHING)
          searchlight_mask_test(state, vischar);
        if (vischar->width_bytes != 3)
          masked_sprite_plotter_24_wide_vischar(state, vischar);
        else
          masked_sprite_plotter_16_wide_vischar(state, vischar);
      }
    }
    else
    {
      found = setup_item_plotting(state, itemstruct, index);
      if (found)
      {
        render_mask_buffer(state);
        masked_sprite_plotter_16_wide_item(state);
      }
    }
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B89C: Returns the next vischar or item to draw.
 *
 * \param[in]  state       Pointer to game state.
 * \param[out] pindex      If vischar, returns vischars_LENGTH - iters; if itemstruct, returns ((item__LIMIT - iters) | (1 << 6)). (was A)
 * \param[out] pvischar    Pointer to receive vischar pointer. (was IY)
 * \param[out] pitemstruct Pointer to receive itemstruct pointer. (was IY)
 *
 * \return State of Z flag: 1 if Z, 0 if NZ. // unsure which is found/notfound right now
 */
int locate_vischar_or_itemstruct(tgestate_t    *state,
                                 uint8_t       *pindex,
                                 vischar_t    **pvischar,
                                 itemstruct_t **pitemstruct)
{
  uint16_t      x;                /* was BC */
  uint16_t      y;                /* was DE */
  item_t        item_and_flag;    /* was A' */
  uint16_t      DEdash;           /* was DE' */
  uint8_t       iters;            /* was B' */
  vischar_t    *vischar;          /* was HL' */
  vischar_t    *found_vischar;    /* was IY */
  itemstruct_t *found_itemstruct; /* was IY */

  assert(state       != NULL);
  assert(pindex      != NULL);
  assert(pvischar    != NULL);
  assert(pitemstruct != NULL);

  *pvischar    = NULL;
  *pitemstruct = NULL;

  x             = 0; // prev-x
  y             = 0; // prev-y
  item_and_flag = item_NONE; // 'nothing found' marker 255
  DEdash        = 0; // is this even used?

  // Iterate over vischars for ?

  iters   = vischars_LENGTH; /* iterations */
  vischar = &state->vischars[0]; /* Conv: Original points to $8007. */
  do
  {
    if ((vischar->counter_and_flags & vischar_BYTE7_LOCATABLE) == 0)
      goto next;

    if ((vischar->mi.pos.x + 4 < x) ||
        (vischar->mi.pos.y + 4 < y))
      goto next;

    item_and_flag = vischars_LENGTH - iters; /* Item index. */ // BC' is temp.
    DEdash = vischar->mi.pos.height;

    y = vischar->mi.pos.y; /* Note: The y,x order here matches the original code. */
    x = vischar->mi.pos.x;
    state->IY = found_vischar = vischar;

next:
    vischar++;
  }
  while (--iters);

  // IY is returned from this, but is an itemstruct_t not a vischar
  // item_and_flag is passed through if no itemstruct is found.
  item_and_flag = get_greatest_itemstruct(state,
                                          item_and_flag,
                                          x,y,
                                          &found_itemstruct);
  *pindex = item_and_flag; // Conv: Additional code.
  /* If Adash's top bit remains set from initialisation, then no vischar was
   * found. (It's not affected by get_greatest_itemstruct which passes it
   * through). */
  if (item_and_flag & (1 << 7))
    return 0; // NZ => not found

  if ((item_and_flag & item_FOUND) == 0)
  {
    found_vischar->counter_and_flags &= ~vischar_BYTE7_LOCATABLE;

    *pvischar = found_vischar; // Conv: Additional code.

    return 1; // Z => found
  }
  else
  {
    int Z;

    found_itemstruct->room_and_flags &= ~itemstruct_ROOM_FLAG_NEARBY_6;
    Z = (found_itemstruct->room_and_flags & itemstruct_ROOM_FLAG_NEARBY_6) == 0; // was BIT 6,HL[1] // this tests the bit we've just cleared

    *pitemstruct = found_itemstruct; // Conv: Additional code.

    return Z; // check the sense isn't flipped
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B916: Render the mask buffer.
 *
 * \param[in] state Pointer to game state.
 */
void render_mask_buffer(tgestate_t *state)
{
  uint8_t       iters; /* was A, B */
  const mask_t *pmask; /* was HL */

  assert(state != NULL);

  /* Clear the whole mask buffer. */
  memset(&state->mask_buffer[0], 255, sizeof(state->mask_buffer));

  if (state->room_index > room_0_OUTDOORS)
  {
    /* Indoors */

    /* Conv: Merged use of A and B. */
    iters = state->interior_mask_data_count; /* count byte */
    if (iters == 0)
      return; /* no masks */

    pmask = &state->interior_mask_data[0];
  }
  else
  {
    /* Outdoors */

    iters = NELEMS(exterior_mask_data); // Bug? Was 59 (one too large).
    pmask = &exterior_mask_data[0]; // offset by +2 bytes; original points to $EC03, table starts at $EC01 // fix by propagation
  }

  uint8_t         A;                   /* was A */
  const uint8_t  *mask_pointer;        /* was DE */
  uint16_t        mask_skip;           /* was HL */
  uint8_t        *mask_buffer_pointer; /* was $81A0 */

  /* Mask against all. */
  do
  {
    uint8_t scrx, scry, height; /* was A/C, A, A */
    uint8_t clip_x;       /* was $B837 */ // x0 offset
    uint8_t clip_y;       /* was $B838 */ // y0 offset
    uint8_t clip_height;  /* was $B839 */ // y0 offset 2
    uint8_t clip_width;   /* was $B83A */ // x0 offset 2
    uint8_t self_BA90;    /* was $BA90 - self modified */
    uint8_t skip;         /* was $BABA - self modified */

    /* Skip any masks which aren't on-screen. */

    // PUSH BC, PUSH HL

    // pmask->bounds is a visual position on the map image (isometric map space)
    // pmask->pos is a map position (map space)
    // so we can cull masks if not on-screen and we can cull masks if behind player

    scrx = state->screenpos.x;
    scry = state->screenpos.y; // reordered
    if (scrx - 1 >= pmask->bounds.x1 || scrx + 3 < pmask->bounds.x0 ||
        scry - 1 >= pmask->bounds.y1 || scry + 4 < pmask->bounds.y0)  // $EC03,$EC02 $EC05,$EC04
      goto pop_next;

    // what's in state->tinypos_stash at this point?
    // it's set by setup_vischar_plotting, setup_item_plotting

    if (state->tinypos_stash.x <= pmask->pos.x ||
        state->tinypos_stash.y <  pmask->pos.y) // $EC06, $EC07
      goto pop_next;

    height = state->tinypos_stash.height;
    if (height)
      height--; // make inclusive?
    if (height >= pmask->pos.height) // $EC08
      goto pop_next;

    /* Work out clipping offsets, widths and heights. */

    // Conv: removed dupe of earlier load of scrx
    if (scrx >= pmask->bounds.x0) // must be $EC02
    {
      clip_x      = scrx - pmask->bounds.x0;
      clip_width  = MIN(pmask->bounds.x1 - scrx, 3) + 1;
    }
    else
    {
      // Conv: removed pmask->bounds.x0 cached in x0 (was B)
      clip_x      = 0;
      clip_width  = MIN((pmask->bounds.x1 - pmask->bounds.x0) + 1, 4 - (pmask->bounds.x0 - scrx));
    }

    // Conv: removed dupe of earlier load of scry
    if (scry >= pmask->bounds.y0)
    {
      clip_y      = scry - pmask->bounds.y0;
      clip_height = MIN(pmask->bounds.y1 - scry, 4) + 1;
    }
    else
    {
      // Conv: removed pmask->bounds.y0 cached in x0 (was B)
      clip_y      = 0;
      clip_height = MIN((pmask->bounds.y1 - pmask->bounds.y0) + 1, 5 - (pmask->bounds.y0 - scry));
    }

    // In the original code, HL is here decremented to point at member y0.

    {
      uint8_t x, y;  /* was B, C */ // x,y are the correct way around here i think!
      uint8_t index; /* was A */

      x = y = 0;
      // Conv: flipped order of these
      if (clip_x == 0)
        x = pmask->bounds.x0 - state->screenpos.x;
      if (clip_y == 0)
        y = pmask->bounds.y0 - state->screenpos.y;

      index = pmask->index;
      assert(index < NELEMS(mask_pointers));

      mask_buffer_pointer = &state->mask_buffer[y * MASK_BUFFER_ROWBYTES + x];

      mask_pointer = mask_pointers[index];

      self_BA90 = mask_pointer[0] - clip_width; // self modify // *mask_pointer is the mask's width
      skip = MASK_BUFFER_ROWBYTES - clip_width; // self modify
    }

    /* Calculate count of mask tiles to skip. */
    mask_skip = clip_y * mask_pointer[0] + clip_x + 1;

    /* Skip the initial clipped mask bytes. */
    do
    {
more_to_skip:
      A = *mask_pointer++; /* read count byte OR tile index */
      if (A & MASK_RUN_FLAG)
      {
        A &= ~MASK_RUN_FLAG;
        mask_skip -= A;
        if ((int16_t) mask_skip < 0) /* went too far? */
          goto skip_went_negative;
        mask_pointer++; /* skip mask index */
        if (mask_skip > 0) // still more to skip?
        {
          goto more_to_skip;
        }
        else // mask_skip must be zero
        {
          A = 0; // counter
          goto skip_went_zero;
        }
      }
    }
    while (--mask_skip);
    A = mask_skip; // ie. A = 0
    goto skip_went_zero;

skip_went_negative:
    A = -(mask_skip & 0xFF); // counter

skip_went_zero:
    {
      uint8_t *maskbufptr;  /* was HL */
      uint8_t  iters2;      /* was C */
      uint8_t  iters3;      /* was B */

      maskbufptr = mask_buffer_pointer;
      // R I:C Iterations (inner loop);
      iters2 = clip_height; // self modified
      do
      {
        iters3 = clip_width; // self modified
        do
        {
          // the original code has some mismatched EX AF,AF's stuff going on which is hard to grok
          // AFAICT it's ensuring that A is always the tile and that A' is the counter

          uint8_t Adash = 0xCC; // safety value

          SWAP(uint8_t, A, Adash); // was Adash = A; // save counter

          A = *mask_pointer; // read count byte OR tile index
          if (A & MASK_RUN_FLAG)
          {
            A &= ~MASK_RUN_FLAG;
            SWAP(uint8_t, A, Adash); // was Adash = A; // save counter
            mask_pointer++;
            A = *mask_pointer; // read next byte (tile)
          }

          if (A != 0) /* shortcut the totally blank tile 0 */
            mask_against_tile(A, maskbufptr);
          maskbufptr++;

          SWAP(uint8_t, A, Adash); // was A = Adash; // fetch counter

          /* advance the mask pointer when the counter reaches zero */
          if (A == 0 || --A == 0) /* Conv: written inverted over the original version */
            mask_pointer++;
        }
        while (--iters3);

        // trailing

        iters3 = self_BA90; // self modified // == mask width - clipped width
        if (iters3)
        {
          if (A)
            goto baa3; // must be continuing with a nonzero counter
          do
          {
ba9b:
            A = *mask_pointer++; // read count byte OR tile index
            if (A & MASK_RUN_FLAG)
            {
              A &= ~MASK_RUN_FLAG;
baa3:
              iters3 = A = iters3 - A;
              if ((int8_t) iters3 < 0)
                goto bab6;
              mask_pointer++;
              if (iters3 > 0)
                goto ba9b;
              else // iters3 must be zero
                goto bab9;
            }
          }
          while (--iters3);

          A = 0;
          goto bab9;
bab6:
          A = -A;
        }
bab9:
        maskbufptr += skip; // self modified
      }
      while (--iters2);
    }

pop_next:
    // POP HL
    // POP BC
    pmask++; // stride is 1 mask_t
  }
  while (--iters);

  /* Dump the mask buffer to the top-left of the screen. */
  if (1)
  {
    uint8_t *slp;
    int      yy;

    slp = &state->speccy->screen[(128 + 1) * state->width];
    for (yy = 0; yy < MASK_BUFFER_HEIGHT * 8; yy++)
    {
      memcpy(slp, &state->mask_buffer[yy * MASK_BUFFER_WIDTHBYTES], MASK_BUFFER_WIDTHBYTES);
      slp = get_next_scanline(state, slp);
    }
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $BADC: AND a tile in the mask buffer against the specified mask tile.
 *
 * Expects a buffer with a stride of MASK_BUFFER_WIDTHBYTES.
 *
 * Leaf.
 *
 * \param[in] index Mask tile index.                (was A)
 * \param[in] dst   Pointer to a tile to be masked. (was HL)
 */
void mask_against_tile(tileindex_t index, tilerow_t *dst)
{
  const tilerow_t *row;   /* was HL' */
  uint8_t          iters; /* was B' */

  assert(index < 111);
  assert(dst);
  // ASSERT_MASK_BUF_PTR_VALID(dst); // we don't have access to state from here to compare

  row = &mask_tiles[index].row[0];
  iters = 8;
  do
  {
    *dst &= *row++;
    dst += MASK_BUFFER_WIDTHBYTES; /* skip to next row */
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $BAF7: Clipping vischars to the game window.
 *
 * Counterpart to \ref item_visible.
 *
 * \param[in]  state          Pointer to game state.
 * \param[in]  vischar        Pointer to visible character.       (was IY)
 * \param[out] clipped_width  Pointer to returned clipped width.  (was BC) packed: (lo,hi) = (width, lefthand skip)
 * \param[out] clipped_height Pointer to returned ciipped height. (was DE) packed: (lo,hi) = (height, top skip)
 *
 * \return 0 => visible, 255 => invisible. (was A)
 */
int vischar_visible(tgestate_t      *state,
                    const vischar_t *vischar,
                    uint16_t        *clipped_width,
                    uint16_t        *clipped_height)
{
  int8_t   amount_visible_right;  /* was A */
  uint16_t new_width; /* was BC */
  uint16_t Y1;  /* was HL */
  uint16_t new_height;  /* was DE */
  uint16_t Y2;  /* was HL */

  assert(state          != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(clipped_width  != NULL);
  assert(clipped_height != NULL);

  /* Conv: Jump to exit for invisible case turned into immediate returns. */

  /* Width part. */

  /* Conv: Re-read of map_position_related removed. */
  /* Bail out if the vischar is off the right hand side of the screen. */
  // like: if (state->screenpos.x > state->map_position.x + state->columns) ...
  amount_visible_right = state->map_position.x + state->columns - state->screenpos.x; // columns was 24
  if (amount_visible_right <= 0)
    goto invisible;

  // Check for vischars partly on-screen.
  if (amount_visible_right < vischar->width_bytes)
  {
    /* Lefthand ok, but righthand clipped. */
    /* A1 must be the clipped width at this point. */
    new_width = amount_visible_right;
  }
  else
  {
    int8_t amount_visible_left; /* was A */

    /* Bail out if the vischar is off the left hand side of the screen. */
    // A2 is signed for this part.
    amount_visible_left = state->screenpos.x + vischar->width_bytes - state->map_position.x;
    if (amount_visible_left <= 0)
      goto invisible;

    /* Check for vischars partly on-screen. */
    if (amount_visible_left < vischar->width_bytes)
      /* Lefthand clipped, righthand ok. */
      new_width = ((vischar->width_bytes - amount_visible_left) << 8) | amount_visible_left; // pack left clip offset + width
    else
      /* No clipping. */
      new_width = vischar->width_bytes;
  }

  /* Height part. */

  // top?
  Y1 = (state->map_position.y + state->rows) * 8 - vischar->floogle.y;
  if (Y1 <= 0 || Y1 >= 256)
    goto invisible;

  if (Y1 < vischar->height)
  {
    new_height = Y1;
  }
  else
  {
    // bottom?
    Y2 = vischar->floogle.y + vischar->height - state->map_position.y * 8; // height == height in rows // signedness
    if (Y2 <= 0 || Y2 >= 256)
      goto invisible;

    if (Y2 < vischar->height)
      new_height = ((vischar->height - Y2) << 8) | Y2; // pack offset + height
    else
      /* No clipping. */
      new_height = vischar->height;
  }

  *clipped_width  = new_width;
  *clipped_height = new_height;

  return 0; /* Visible */


invisible:
  return 255;
}

/* ----------------------------------------------------------------------- */

/**
 * $BB98: Paints any tiles occupied by visible characters with tiles from tile_buf.
 *
 * \param[in] state Pointer to game state.
 */
void restore_tiles(tgestate_t *state)
{
  uint8_t             iters;                  /* was B */
  const vischar_t    *vischar;                /* new local copy */
  uint8_t             height;                 /* was A / $BC5F */
  int8_t              heightsigned;           /* was A */
  uint8_t             width;                  /* was $BC61 */
  uint8_t             tilebuf_stride;         /* was $BC8E */
  uint8_t             windowbuf_stride;       /* was $BC95 */
  uint8_t             width_counter, height_counter;  /* was B, C */
  uint16_t            clipped_width;          /* was BC */
  uint16_t            clipped_height;         /* was DE */
  const xy_t         *map_position;           /* was HL */
  uint8_t            *windowbuf;              /* was HL */
  uint8_t            *windowbuf2;             /* was DE */
  uint8_t             x, y;                   /* was H', L' */
  const tileindex_t  *tilebuf;                /* was HL/DE */
  tileindex_t         tile;                   /* was A */
  const tile_t       *tileset;                /* was BC */
  uint8_t             tile_counter;           /* was B' */
  const tilerow_t    *tilerow;                /* was HL' */

  assert(state != NULL);

  iters     = vischars_LENGTH;
  state->IY = &state->vischars[0];
  do
  {
    vischar = state->IY; /* Conv: Added local copy of IY. */

    if (vischar->flags == vischar_FLAGS_EMPTY_SLOT)
      goto next;

    /* Get the visible character's position in screen space. */
    state->screenpos.y = vischar->floogle.y >> 3; // divide by 8 (16-to-8)
    state->screenpos.x = vischar->floogle.x >> 3; // divide by 8 (16-to-8)

    if (vischar_visible(state, vischar, &clipped_width, &clipped_height) == 255)
      goto next; /* invisible */

    // $BBD3
    height = ((clipped_height >> 3) & 0x1F) + 2; // the masking will only be required if the top byte of clipped_height contains something

    heightsigned = height + state->screenpos.y - state->map_position.y;
    if (heightsigned >= 0)
    {
      heightsigned -= 17; // likely window_buf height
      if (heightsigned > 0)
      {
        clipped_height = heightsigned; // original code assigns low byte only - could be an issue
        heightsigned = height - clipped_height;
        if (heightsigned < 0) // if carry
        {
          goto next; // not visible?
        }
        else if (heightsigned != 0) // ie. > 0
        {
          height = heightsigned;
          goto clamp_height;
        }
        else
        {
          assert(heightsigned == 0);

          goto next; // not visible?
        }
      }
    }
    // unsure about the else case here

clamp_height: // was $BBF8
    if (height > 5) // note: this is height, not heightsigned (preceding POP removed)
      height = 5; // outer loop counter // height

    width            = clipped_width & 0xFF; // was self modify // inner loop counter // width
    tilebuf_stride   = state->columns - width; // was self modify
    windowbuf_stride = tilebuf_stride + 7 * state->columns; // was self modify // == (8 * state->columns - C)

    map_position = &state->map_position;

    /* Work out x,y offsets into the tile buffer. */

    if ((clipped_width >> 8) == 0)
      x = state->screenpos.x - map_position->x;
    else
      x = 0; // was interleaved

    if ((clipped_height >> 8) == 0)
      y = state->screenpos.y - map_position->y;
    else
      y = 0; // was interleaved

    windowbuf = &state->window_buf[y * state->columns * 8 + x];
    ASSERT_WINDOW_BUF_PTR_VALID(windowbuf);

    tilebuf = &state->tile_buf[x + y * state->columns];
    ASSERT_TILE_BUF_PTR_VALID(tilebuf);

    height_counter = height;
    do
    {
      width_counter = width;
      do
      {
        ASSERT_TILE_BUF_PTR_VALID(tilebuf);

        tile = *tilebuf;
        windowbuf2 = windowbuf;

        tileset = select_tile_set(state, x, y);

        ASSERT_WINDOW_BUF_PTR_VALID(windowbuf2);
        ASSERT_WINDOW_BUF_PTR_VALID(windowbuf2 + 7 * state->columns);

        /* Copy the tile into the window buffer. */
        tilerow = &tileset[tile].row[0];
        tile_counter = 8;
        do
        {
          *windowbuf2 = *tilerow++;
          windowbuf2 += state->columns;
        }
        while (--tile_counter);

        /* Move to next column. */
        x++;
        tilebuf++;
        if (width_counter > 1)
          ASSERT_TILE_BUF_PTR_VALID(tilebuf);
        windowbuf++;
        if (width_counter > 1)
          ASSERT_WINDOW_BUF_PTR_VALID(windowbuf);
      }
      while (--width_counter);

      /* Reset x offset. Advance to next row. */
      x -= width;
      y++;
      tilebuf   += tilebuf_stride;
      if (height_counter > 1)
        ASSERT_TILE_BUF_PTR_VALID(tilebuf);
      windowbuf += windowbuf_stride;
      if (height_counter > 1)
        ASSERT_WINDOW_BUF_PTR_VALID(windowbuf);
    }
    while (--height_counter);

next:
    state->IY++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $BCAA: Turn a map ref into a tile set pointer.
 *
 * Called from restore_tiles.
 *
 * \param[in] state Pointer to game state.
 * \param[in] x     X offset - added to map_position.x. (was H)
 * \param[in] y     Y offset - added to map_position.y. (was L)
 *
 * \return Tile set pointer. (was BC)
 */
const tile_t *select_tile_set(tgestate_t *state,
                              uint8_t     x,
                              uint8_t     y)
{
  const tile_t    *tileset;    /* was BC */
  uint8_t          row_offset; /* was L */
  uint8_t          offset;     /* was Adash */
  supertileindex_t tile;       /* was Adash */

  assert(state != NULL);

  if (state->room_index != room_0_OUTDOORS)
  {
    tileset = &interior_tiles[0];
  }
  else
  {
    /* Convert map position to an index into 7x5 supertile refs array. */
    // the '& 0x3F' should be redundant after the >> 2
    row_offset = ((((state->map_position.y & 3) + y) >> 2) & 0x3F) * state->st_columns; // vertical // Conv: constant columns 7 made variable
    offset     = ((((state->map_position.x & 3) + x) >> 2) & 0x3F) + row_offset; // combines horizontal + vertical

    tile = state->map_buf[offset]; /* (7x5) supertile refs */
    tileset = &exterior_tiles[0];
    if (tile >= 45)
    {
      tileset = &exterior_tiles[145];
      if (tile >= 139 && tile < 204)
        tileset = &exterior_tiles[145 + 220];
    }
  }
  return tileset;
}

/* ----------------------------------------------------------------------- */

/**
 * $C41C: Spawn characters.
 *
 * \param[in] state Pointer to game state.
 */
void spawn_characters(tgestate_t *state)
{
  uint8_t            map_y, map_x;                  /* was H, L */
  uint8_t            map_y_clamped, map_x_clamped;  /* was D, E */
  characterstruct_t *charstr;                       /* was HL */
  uint8_t            iters;                         /* was B */
  room_t             room;                          /* was A */
  uint8_t            y, x;                          /* was C, C */

  assert(state != NULL);

  /* Form a clamped map position in DE. */
  map_y = state->map_position.y;
  map_x = state->map_position.x;
  map_x_clamped = (map_x < 8) ? 0 : map_x;
  map_y_clamped = (map_y < 8) ? 0 : map_y;

  /* Walk all character structs. */
  charstr = &state->character_structs[0];
  iters   = character_structs__LIMIT;
  do
  {
    if ((charstr->character_and_flags & characterstruct_FLAG_DISABLED) == 0)
    {
      /* Is the character in this room? */
      if ((room = state->room_index) == charstr->room)
      {
        if (room == room_0_OUTDOORS)
        {
          /* Outdoors. */

          /* Screen Y calculation. */
          y = 0x200 - charstr->pos.x - charstr->pos.y - charstr->pos.height; // 0x200 is represented as zero in original code.
          if (y <= map_y_clamped || y > MIN(map_y_clamped + 32, 0xFF))
            goto skip; // check

          // move down and to the left
          // why move the character here?
          charstr->pos.y += 64;
          charstr->pos.x -= 64;

          /* Screen X calculation. */
          x = 0x80;
          if (x <= map_x_clamped || x > MIN(map_x_clamped + 40, 0xFF))
            goto skip; // check
        }

        spawn_character(state, charstr); // return code ignored
      }
    }

skip:
    charstr++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $C47E: Remove any off-screen non-player characters.
 *
 * Called from main loop.
 *
 * \param[in] state Pointer to game state.
 */
void purge_invisible_characters(tgestate_t *state)
{
  /* Size, in UDGs, of a buffer zone around the visible screen in which
   * visible characters will persist. */
  enum { GRACE = 9 };

  uint8_t    miny, minx;  /* was D, E */
  uint8_t    iters;       /* was B */
  vischar_t *vischar;     /* was HL */
  uint8_t    lo, hi;      /* was C, A */

  assert(state != NULL);

  /* Calculate clamped lower bound. */
  minx = MAX(state->map_position.x - GRACE, 0);
  miny = MAX(state->map_position.y - GRACE, 0);

  iters   = vischars_LENGTH - 1;
  vischar = &state->vischars[1]; /* iterate over non-player characters */
  do
  {
    /* Ignore inactive characters. */
    if (vischar->character == character_NONE)
      goto next;

    /* Reset characters not in the current room. */
    if (state->room_index != vischar->room)
      goto reset;

    // FUTURE
    //
    // /* Calculate clamped upper bound. */
    // maxx = MIN(minx + EDGE + state->columns + EDGE, 255);
    // maxy = MIN(miny + EDGE + (state->rows + 1) + EDGE, 255);
    //
    // t = (vischar->floogle.y + 4) / 8 / 256;
    // if (t <= miny || t > maxy)
    //   goto reset;
    // t = (vischar->floogle.x) / 8 / 256;
    // if (t <= minx || t > maxx)
    //   goto reset;

    // Conv: Replaced constants with state->columns/rows.

    hi = vischar->floogle.y >> 8;
    lo = vischar->floogle.y & 0xFF;
    divide_by_8_with_rounding(&lo, &hi);
    if (lo <= miny || lo > MIN(miny + GRACE + (state->rows + 1) + GRACE, 255)) // Conv: C -> A
      goto reset;

    hi = vischar->floogle.x >> 8;
    lo = vischar->floogle.x & 0xFF;
    divide_by_8(&lo, &hi);
    if (lo <= minx || lo > MIN(minx + GRACE + state->columns + GRACE, 255)) // Conv: C -> A
      goto reset;

    goto next;

reset:
    reset_visible_character(state, vischar);

next:
    vischar++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $C4E0: Add a character to the visible character list.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] charstr Pointer to character to spawn. (was HL)
 *
 * \return 1 if spawned, 0 if no empty slots
 */
int spawn_character(tgestate_t *state, characterstruct_t *charstr)
{
  /**
   * $CD9A: Data for the four classes of characters.
   */
  static const character_class_data_t character_class_data[4] =
  {
    { &animations[0], &sprites[sprite_COMMANDANT_FACING_AWAY_1] },
    { &animations[0], &sprites[sprite_GUARD_FACING_AWAY_1]      },
    { &animations[0], &sprites[sprite_DOG_FACING_AWAY_1]        },
    { &animations[0], &sprites[sprite_PRISONER_FACING_AWAY_1]   },
  };

  vischar_t                   *vischar;           /* was HL/IY */
  uint8_t                      iters;             /* was B */
  characterstruct_t           *charstr2;          /* was DE */
  pos_t                       *saved_pos;         /* was HL */
  character_t                  character;         /* was A */
  const character_class_data_t *metadata;         /* was DE */
  int                          Z;
  room_t                       room;              /* was A */
  uint8_t                      get_target_result; /* was A */
  const tinypos_t             *doorpos;           /* was HL */
  const xy_t                  *location;          /* was HL */

  assert(state   != NULL);
  assert(charstr != NULL);

  if (charstr->character_and_flags & characterstruct_FLAG_DISABLED) /* character disabled */
    return 1; // NZ

  /* Find an empty slot in the visible character list. */
  vischar = &state->vischars[1];
  iters = vischars_LENGTH - 1;
  do
  {
    if (vischar->character == vischar_CHARACTER_EMPTY_SLOT)
      goto found_empty_slot;
    vischar++;
  }
  while (--iters);

  return 0; // check


found_empty_slot:

  // POP DE  (DE = charstr)
  // PUSH HL (vischar)
  // POP IY  (IY_vischar = HL_vischar)
  state->IY = vischar;
  // PUSH HL (vischar)
  // PUSH DE (charstr)

  charstr2 = charstr;

  /* Scale coords dependent on which room the character is in. */
  saved_pos = &state->saved_pos.pos;
  if (charstr2->room == room_0_OUTDOORS)
  {
    /* Conv: Unrolled. */
    saved_pos->x      = charstr2->pos.x      * 8;
    saved_pos->y      = charstr2->pos.y      * 8;
    saved_pos->height = charstr2->pos.height * 8;
  }
  else
  {
    /* Conv: Unrolled. */
    saved_pos->x      = charstr2->pos.x;
    saved_pos->y      = charstr2->pos.y;
    saved_pos->height = charstr2->pos.height;
  }

  Z = collision(state);
  if (Z == 0)
    Z = bounds_check(state, vischar);
  // if Z == 1 then within wall bounds

  // POP DE (charstr2)
  // POP HL (vischar)

  if (Z == 1) // if collision or bounds_check is nonzero, then return
    return 0; // check

  character = charstr2->character_and_flags | characterstruct_FLAG_DISABLED; /* Disable character */
  charstr2->character_and_flags = character;

  character &= characterstruct_CHARACTER_MASK;
  vischar->character = character;
  vischar->flags     = 0;

  // PUSH DE (charstr2)

  metadata = &character_class_data[0]; /* Commandant */
  if (character != 0)
  {
    metadata = &character_class_data[1]; /* Guard */
    if (character >= 16)
    {
      metadata = &character_class_data[2]; /* Dog */
      if (character >= 20)
      {
        metadata = &character_class_data[3]; /* Prisoner */
      }
    }
  }

  // EX DE, HL (DE = vischar, HL = charstr)

  vischar->animbase  = metadata->animbase; // seems to be a constant
  vischar->mi.sprite = metadata->sprite;
  memcpy(&vischar->mi.pos, &state->saved_pos, sizeof(pos_t));

  // POP HL

  //HL += 5; // -> charstr->route
  //DE += 7; // -> vischar->room

  room = state->room_index;
  vischar->room = room;
  if (room > room_0_OUTDOORS)
  {
    play_speaker(state, sound_CHARACTER_ENTERS_2);
    play_speaker(state, sound_CHARACTER_ENTERS_1);
  }

  //DE -= 26; // -> vischar->route
  //*DE++ = *HL++;
  //*DE++ = *HL++;
  vischar->route = charstr->route;
  //HL -= 2; // -> charstr->route

again:
  if (charstr->route.index != 0) /* if not stood still */
  {
    state->entered_move_characters = 0;
    // PUSH DE // -> vischar->pos
    get_target_result = get_target(state,
                                   &charstr->route,
                                   &doorpos,
                                   &location);
    if (get_target_result == get_target_ROUTE_ENDS)
    {
      // POP HL // HL = DE
      // HL -= 2; // -> vischar->route
      // PUSH HL
      (void) route_ended(state, vischar, &vischar->route);
      // POP HL
      // DE = HL + 2; // -> vischar->pos
      goto again;
    }
    else if (get_target_result == get_target_DOOR)
    {
      vischar->flags |= vischar_FLAGS_DOOR_THING;
    }
    // POP DE // -> vischar->pos
    // sampled HL is $7913 (a doors tinypos)
    // it seems that get_target might return tinypos_t's or xy_t's... we'll go ahead and copy a tinypos_t irrespective.
    if (get_target_result == get_target_DOOR)
    {
      vischar->pos = *doorpos;
    }
    else
    {
      vischar->pos.x = location->x;
      vischar->pos.y = location->y;
    }
    //memcpy(&vischar->pos, route, sizeof(tinypos_t));
    // pointers incremented in original?
  }
  vischar->counter_and_flags = 0;
  // DE -= 7;
  // EX DE,HL
  // PUSH HL
  calc_vischar_screenpos_from_mi_pos(state, vischar);
  // POP HL
  character_behaviour(state, vischar);
  return 1; // exit via
}

/* ----------------------------------------------------------------------- */

/**
 * $C5D3: Reset a visible character (either a character or an object).
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 */
void reset_visible_character(tgestate_t *state, vischar_t *vischar)
{
  character_t        character; /* was A */
  pos_t             *pos;       /* was DE */
  characterstruct_t *charstr;   /* was DE */
  room_t             room;      /* was A */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  character = vischar->character;
  if (character == character_NONE)
    return;

  if (character >= character_26_STOVE_1)
  {
    /* A stove or crate character. */

    vischar->character         = character_NONE;
    vischar->flags             = vischar_FLAGS_EMPTY_SLOT; /* flags */
    vischar->counter_and_flags = 0;    /* more flags */

    /* Save the old position. */
    if (character == character_26_STOVE_1)
      pos = &state->movable_items[movable_item_STOVE1].pos;
    else if (character == character_27_STOVE_2)
      pos = &state->movable_items[movable_item_STOVE2].pos;
    else
      pos = &state->movable_items[movable_item_CRATE].pos;

    /* The DOS version of the game has a difference here. Instead of
     * memcpy'ing the current vischar's position into the movable_items's
     * position, it only copies the first two bytes. The code is setup for a
     * copy of six bytes (cx is set to 3) but the 'movsw' ought to be a 'rep
     * movsw' for it to work. It fixes the bug where stoves get left in place
     * after a restarted game, but almost looks like an accident.
     */
    memcpy(pos, &vischar->mi.pos, sizeof(*pos));
  }
  else
  {
    pos_t     *vispos_in;   /* was HL */
    tinypos_t *charpos_out; /* was DE */

    /* A non-object character. */

    charstr = get_character_struct(state, character);
    charstr->character_and_flags &= ~characterstruct_FLAG_DISABLED; /* Enable character */

    room = vischar->room;
    charstr->room = room;

    vischar->counter_and_flags = 0; /* more flags */

    /* Save the old position. */

    vispos_in   = &vischar->mi.pos;
    charpos_out = &charstr->pos;

    if (room == room_0_OUTDOORS)
    {
      /* Outdoors. */
      pos_to_tinypos(vispos_in, charpos_out);
    }
    else
    {
      /* Indoors. */
      /* Conv: Unrolled from original code. */
      charpos_out->x      = vispos_in->x; // note: narrowing
      charpos_out->y      = vispos_in->y; // note: narrowing
      charpos_out->height = vispos_in->height; // note: narrowing
    }

    // character = vischar->character; /* Done in original code, but we already have this from earlier. */
    vischar->character = character_NONE;
    vischar->flags     = vischar_FLAGS_EMPTY_SLOT;

    /* Guard dogs only. */
    if (character >= character_16_GUARD_DOG_1 &&
        character <= character_19_GUARD_DOG_4)
    {
      vischar->route.index = route_WANDER; /* Choose random locations[]. */
      vischar->route.step  = 0; /* 0..7 */
      if (character >= character_18_GUARD_DOG_3) /* Characters 18 and 19 */
      {
        vischar->route.index = route_WANDER; /* Choose random locations[]. */
        vischar->route.step  = 24; /* 24..31 */
      }
    }

    charstr->route = vischar->route;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $C651: Return the coordinates of the route's current target.
 *
 * Given a route_t return the coordinates the character should move to.
 * Coordinates are returned as a tinypos_t when moving to a door or an xy_t
 * when moving to a numbered location.
 *
 * If the route specifies 'wander' then one of eight random locations is
 * chosen starting from route.step.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] route     Pointer to route.                            (was HL)
 * \param[in] doorpos   Receives doorpos when moving to a door.      (was HL)
 * \param[in] location  Receives location when moving to a location. (was HL)
 *
 * \retval get_target_ROUTE_ENDS The route has ended.
 * \retval get_target_DOOR       The next target is a door.
 * \retval get_target_LOCATION   The next target is a location.
 */
uint8_t get_target(tgestate_t       *state,
                   route_t          *route,
                   const tinypos_t **doorpos,
                   const xy_t      **location)
{
  /**
   * $783A: Default map locations.
   */
  static const xy_t locations[78] =
  {
    // reset_visible_character guard dogs 1 & 2 use 0..7
    // character_behaviour uss this range also, for dog behaviour
    {  68, 104 }, // 0
    {  68,  84 }, // 1
    {  68,  70 }, // 2
    {  64, 102 }, // 3
    {  64,  64 }, // 4
    {  68,  68 }, // 5
    {  64,  64 }, // 6
    {  68,  64 }, // 7

    // charevnt_handler_0 uses 8..15
    { 104, 112 }, // 8
    {  96, 112 }, // 9 used by route_guard_12_roll_call
    { 106, 102 }, // 10
    {  93, 104 }, // 11 used by route_commandant, route_exit_hut2, data_77E1, route_guard_13_roll_call, route_guard_13_bed, route_guard_14_bed, route_guard_15_bed
    { 124, 101 }, // 12 used by route_exit_hut2, data_77E7, data_77EC, route_guard_13_bed, route_guard_14_bed, route_guard_15_bed
    { 124, 112 }, // 13
    { 116, 104 }, // 14 used by route_exit_hut3, route_go_to_solitary, route_hero_leave_solitary
    { 112, 100 }, // 15

    // charevnt_handler_1 uses 16..23
    { 120,  96 }, // 16 used by data_77EC
    { 128,  88 }, // 17 used by route_guard_14_roll_call
    { 112,  96 }, // 18
    { 116,  84 }, // 19
    { 124, 100 }, // 20
    { 124, 112 }, // 21
    { 116, 104 }, // 22
    { 112, 100 }, // 23

    // reset_visible_character guard dogs 3 & 4 use 24..31
    { 102,  68 }, // 24
    { 102,  64 }, // 25
    {  96,  64 }, // 26
    {  92,  68 }, // 27
    {  86,  68 }, // 28
    {  84,  64 }, // 29
    {  74,  68 }, // 30
    {  74,  64 }, // 31

    { 102,  68 }, // 32 used by data_7795
    {  68,  68 }, // 33 used by data_7795
    {  68, 104 }, // 34 used by data_7795

    { 107,  69 }, // 35 used by data_7799
    { 107,  45 }, // 36 used by data_7799
    {  77,  45 }, // 37 used by data_7799
    {  77,  61 }, // 38 used by data_7799
    {  61,  61 }, // 39 used by data_7799
    {  61, 103 }, // 40 used by data_7799

    { 116,  76 }, // 41
    {  44,  42 }, // 42 used by route_commandant, route_go_to_solitary
    { 106,  72 }, // 43 used by data_77CD
    { 110,  72 }, // 44 used by data_77CD
    {  81, 104 }, // 45 used by route_commandant, route_exit_hut3, route_guard_14_bed, route_guard_15_bed

    {  52,  60 }, // 46 used by route_commandant, route_prisoner_sleeps_1
    {  52,  44 }, // 47 used by route_prisoner_sleeps_2
    {  52,  28 }, // 48 used by route_prisoner_sleeps_3
    { 119, 107 }, // 49 used by route_guard_15_roll_call
    { 122, 110 }, // 50 used by route_hero_roll_call
    {  52,  28 }, // 51
    {  40,  60 }, // 52 used by data_77DE, route_guard_14_bed
    {  36,  34 }, // 53 used by data_77DE, route_guard_13_bed, route_guard_15_bed
    {  80,  76 }, // 54
    {  89,  76 }, // 55 used by route_commandant, data_77E1

    // charevnt_handler_2 uses 56..63
    {  89,  60 }, // 56 used by data_77E1
    { 100,  61 }, // 57
    {  92,  54 }, // 58
    {  84,  50 }, // 59
    { 102,  48 }, // 60 used by route_commandant
    {  96,  56 }, // 61
    {  79,  59 }, // 62
    { 103,  47 }, // 63

    {  52,  54 }, // 64 character walks into breakfast room, used by route_prisoner_sits_1
    {  52,  46 }, // 65 used by route_prisoner_sits_2
    {  52,  36 }, // 66 used by route_prisoner_sits_3
    {  52,  62 }, // 67 used by data_7833
    {  32,  56 }, // 68 used by route_guardA_breakfast
    {  52,  24 }, // 69 used by route_guardB_breakfast
    {  42,  46 }, // 70 used by route_hut2_right_to_left
    {  34,  34 }, // 71
    { 120, 110 }, // 72 roll call used by route_prisoner_1_roll_call
    { 118, 110 }, // 73 roll call used by route_prisoner_2_roll_call
    { 116, 110 }, // 74 roll call used by route_prisoner_3_roll_call
    { 121, 109 }, // 75 roll call used by route_prisoner_4_roll_call
    { 119, 109 }, // 76 roll call used by route_prisoner_5_roll_call
    { 117, 109 }, // 77 roll call used by route_prisoner_6_roll_call
  };

  uint8_t        routeindex; /* was A */
  uint8_t        index;      /* was A */
  uint8_t        step;       /* was A or C */
  const uint8_t *routebytes; /* was DE */
  uint8_t        routebyte;  /* was A */
  const door_t  *door;       /* was HL */

  assert(state     != NULL);
  assert(route     != NULL);
  ASSERT_ROUTE_VALID(*route);

  *doorpos  = NULL; /* Conv: Added. */
  *location = NULL; /* Conv: Added. */

  routeindex = route->index;
  if (routeindex == route_WANDER)
  {
    /* Wander around randomly. */
    /* Uses .step + rand(0..7) to index locations[]. */

    /* Randomise the bottom 3 bits of route->step. */
    /* Conv: In the original code *HL is used as a temporary. */
    index = route->step & ~7;
    index |= random_nibble(state) & 7; /* Conv: ADD became OR. */
    route->step = index;
  }
  else
  {
    step = route->step;

    routebytes = get_route(routeindex);
    assert(routebytes != NULL);

    if (step == 255)
      /* Conv: Original code was relying on being able to fetch the preceding
       * route's final byte (255) in all cases. That's not going to work with
       * the way routes are defined in the portable version of the game so
       * instead just set routebyte to 255. */
      routebyte = 255;
    else
      routebyte = routebytes[step]; // HL was temporary

    if (routebyte == routebyte_END) // needs a better end byte
    {
      /* Route byte == 255: End of route */
      return get_target_ROUTE_ENDS; /* Conv: Was a goto to a return. */
    }
    else if ((routebyte & ~door_REVERSE) < 40) // (0..39 || 128..167)
    {
      /* Route byte < 40: A door */
      if (route->index & route_REVERSED)
        routebyte ^= door_REVERSE;
      door = get_door(routebyte);
      *doorpos = &door->pos;
      return get_target_DOOR;
    }
    else
    {
      /* Route byte == 40..117: A location index */
      assert(routebyte >= 40);
      assert(routebyte <= 117);
      index = routebyte - 40;
    }
  }

  assert(index < NELEMS(locations));

  *location = &locations[index];

  return get_target_LOCATION;
}

/* ----------------------------------------------------------------------- */

/**
 * $C6A0: Move one character around at a time.
 *
 * \param[in] state Pointer to game state.
 */
void move_characters(tgestate_t *state)
{
  character_t        character;               /* was A */
  characterstruct_t *charstr;                 /* was HL */
  room_t             room;                    /* was A */
  item_t             item;                    /* was C */
  uint8_t            get_target_result;       /* was A */
  route_t           *HLroute;                 /* was HL */
  const tinypos_t   *HLtinypos;               /* was HL */
  const xy_t        *HLlocation;              /* was HL */
  uint8_t            routeindex;              /* was A */
  characterstruct_t *DEcharstr;               /* was DE */
  uint8_t            max;                     /* was A' */
  uint8_t            B;                       /* was B */
  door_t            *HLdoor;                  /* was HL */
  tinypos_t         *DEcharstr_tinypos;       /* was DE */

  assert(state != NULL);

  /* This makes future character events use state->character_index. */
  state->entered_move_characters = 255;

  /* Move to the next character, wrapping around after character 26. */
  character = state->character_index + 1;
  if (character == character_26_STOVE_1) /* 26 = (highest + 1) character */
    character = character_0_COMMANDANT;
  state->character_index = character;

  /* Get its chararacter struct, exiting if it's not enabled. */
  charstr = get_character_struct(state, character);
  if (charstr->character_and_flags & characterstruct_FLAG_DISABLED)
    return; /* Disabled character. */

  // PUSH HL_charstr

  /* Are any items to be found in the same room as the character? */
  room = charstr->room;
  ASSERT_ROOM_VALID(room);
  if (room != room_0_OUTDOORS)
    /* This discovers one item at a time. */
    if (is_item_discoverable_interior(state, room, &item) == 0)
      item_discovered(state, item);

  // POP HL_charstr
  // HL += 2; // point at charstr->pos
  // PUSH HL_pos
  // HL += 3; // point at charstr->route

  /* If standing still, return. */
  if (charstr->route.index == 0) /* temp was A */
    // POP HL_pos
    return;

  get_target_result = get_target(state,
                                 &charstr->route,
                                 &HLtinypos,
                                 &HLlocation);
  if (get_target_result == get_target_ROUTE_ENDS)
  {
    /* When the route ends reverse the route. */

    HLroute = &charstr->route;

    character = state->character_index;
    if (character != character_0_COMMANDANT)
    {
      /* Not the commandant. */

      if (character >= character_12_GUARD_12)
        goto trigger_event;

      /* Characters 1..11. */
reverse_route:
      HLroute->index = routeindex = HLroute->index ^ route_REVERSED;

      /* Conv: Was [-2]+1 pattern. Adjusted to be clearer. */
      if (routeindex & route_REVERSED)
        HLroute->step--;
      else
        HLroute->step++;

      // POP HL_pos
      return;
    }
    else
    {
      /* Commandant only. */

      // sampled HL = $7617 (characterstruct + 5)
      routeindex = HLroute->index & ~route_REVERSED;
      if (routeindex != 36)
        goto reverse_route;

trigger_event:
      /* We arrive here if character >= character_12_GUARD_12 or if
       * commandant on route 36. */

      // POP DE_pos
      character_event(state, HLroute);
      return; // exit via
    }
  }
  else
  {
    if (get_target_result == get_target_DOOR)
    {
      const tinypos_t *HLtinypos2; /* was HL */

      // POP DE_pos
      DEcharstr = charstr; // points at characterstruct.pos
      room = DEcharstr->room; // read one byte earlier
      // PUSH HL_route
      if (room == room_0_OUTDOORS)
      {
        // The original code stores to saved_pos as if it's a pair of bytes.
        // This is different from most, if not all, of the other locations in
        // the code which treat it as a pair of 16-bit words.
        // Given that I assume it's just being used here as scratch space.

        /* Conv: Unrolled. */
        state->saved_pos.tinypos.x = HLtinypos->x >> 1;
        state->saved_pos.tinypos.y = HLtinypos->y >> 1;
        HLtinypos2 = &state->saved_pos.tinypos;
      }
      else
      {
        /* Conv: Assignment made explicit. */
        HLtinypos2 = HLtinypos;
      }

      if (DEcharstr->room == room_0_OUTDOORS) // FUTURE: Remove reload of 'room'.
        max = 2;
      else
        max = 6;

      DEcharstr_tinypos = &DEcharstr->pos;

      B = change_by_delta(max, 0, &HLtinypos2->x, &DEcharstr_tinypos->x); // max, rc, second, first
      B = change_by_delta(max, B, &HLtinypos2->y, &DEcharstr_tinypos->y);
      if (B != 2)
        return; /* Managed to move. */

      // sampled DE at $C73B = 767c, 7675, 76ad, 7628, 76b4, 76bb, 76c2, 7613, 769f
      // => character_structs.room

      /* This unpleasant cast turns HLtinypos (assigned in get_target)
       * back into a door_t. */
      HLdoor = (door_t *)((char *) HLtinypos - 1);
      assert(HLdoor >= &doors[0]);
      assert(HLdoor < &doors[door_MAX * 2]);

      // sampled HL at $C73C = 7942, 79be, 79d6, 79a6, 7926, 79ee, 78da, 79a2, 78e2
      // => doors.room_and_flags

      DEcharstr->room = (HLdoor->room_and_flags & ~door_FLAGS_MASK_DIRECTION) >> 2;

      // Stuff reading from doors.
      if ((HLdoor->room_and_flags & door_FLAGS_MASK_DIRECTION) < 2)
        // top left or top right
        // sampled HL = 78fa,794a,78da,791e,78e2,790e,796a,790e,791e,7962,791a
        HLtinypos = &HLdoor[1].pos;
      else
        // bottom left or bottom right
        HLtinypos = &HLdoor[-1].pos;

      room = DEcharstr->room; // *DE++;
      DEcharstr_tinypos = &DEcharstr->pos;
      if (room != room_0_OUTDOORS)
      {
        *DEcharstr_tinypos = *HLtinypos;
//        DE += 3;
//        DE--; // DE points to DEtinypos->height
        // HL += 3 // HL points to next door_t?
      }
      else
      {
        /* Conv: Unrolled. */
        DEcharstr_tinypos->x      = HLtinypos->x      >> 1;
        DEcharstr_tinypos->y      = HLtinypos->y      >> 1;
        DEcharstr_tinypos->height = HLtinypos->height >> 1;
//        DE += 3;
//        DE--;
      }
    }
    else
    {
      // POP DE
      DEcharstr = charstr; // points at characterstruct.pos

      /* Conv: Reordered. */
      if (charstr->room == room_0_OUTDOORS) // was DE[-1]
        max = 2;
      else
        max = 6;

      // sampled HL at $C77A = 787a, 787e, 7888, 7890, 78aa, 783a, 786a, 7896, 787c,
      // => locations[]
      // HL comes from HLlocation

      B = change_by_delta(max, 0, &HLlocation->x, &charstr->pos.x);
      B = change_by_delta(max, B, &HLlocation->y, &charstr->pos.y);
      if (B != 2)
        return; // managed to move
    }
//    DE++;
    // EX DE, HL
    // HL -> DEcharstr->route
    routeindex = DEcharstr->route.index; // address? 761e 7625 768e 7695 7656 7695 7680 // => character struct entry + 5 // route field
    if (routeindex == route_WANDER)
      return;
    if ((routeindex & route_REVERSED) == 0)  // similar to pattern above (with the -1/+1)
      DEcharstr->route.step++;
    else
      DEcharstr->route.step--;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $C79A: Increments 'first' by ('first' - 'second').
 *
 * Used only by move_characters().
 *
 * Leaf.
 *
 * \param[in] max    A maximum value, usually 2 or 6. (was A')
 * \param[in] rc     Return code. Incremented if delta is zero. (was B)
 * \param[in] second Pointer to second value. (was HL)
 * \param[in] first  Pointer to first value. (was DE)
 *
 * \return rc as passed in, or rc incremented if delta was zero. (was B)
 */
int change_by_delta(int8_t         max,
                    int            rc,
                    const uint8_t *second,
                    uint8_t       *first)
{
  int delta; /* was A */

  assert(max == 2 || max == 6);
  assert(rc < 2); /* Should not be called with otherwise. */
  assert(second != NULL);
  assert(first  != NULL);

  delta = *first - *second;
  if (delta == 0)
  {
    rc++;
  }
  else if (delta < 0) // delta -ve => second > first
  {
    delta = -delta; /* absolute value */
    if (delta >= max)
      delta = max;
    *first += delta; // move first towards second
  }
  else // delta +ve => first > second
  {
    if (delta >= max)
      delta = max;
    *first -= delta; // move first towards second
  }

  return rc;
}

/* ----------------------------------------------------------------------- */

/**
 * $C7B9: Get character struct.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] character Character index. (was A)
 *
 * \return Pointer to character struct. (was HL)
 */
characterstruct_t *get_character_struct(tgestate_t *state,
                                        character_t character)
{
  assert(state != NULL);
  ASSERT_CHARACTER_VALID(character);

  return &state->character_structs[character];
}

/* ----------------------------------------------------------------------- */

/**
 * $C7C6: Character event.
 *
 * Sampling shows this can be a charstruct.route or a vischar.route.
 *
 * Theory: That this is triggered at the end of a route.
 *
 * \param[in] state Pointer to game state.
 * \param[in] route Pointer to a route.    (was HL).
 */
void character_event(tgestate_t *state, route_t *route)
{
  /* $C7F9 */
  static const route2event_t eventmap[24] =
  {
    { 38 | route_REVERSED,  0 }, /* target wanders locations 8..15 */
    { 39 | route_REVERSED,  0 }, /* target wanders locations 8..15 */
    { 40 | route_REVERSED,  1 }, /* target wanders locations 16..23 */
    { 41 | route_REVERSED,  1 }, /* target wanders locations 16..23 */

    {  5,                   0 }, /* target wanders locations 8..15 */
    {  6,                   1 }, /* target wanders locations 16..23 */
    {  5 | route_REVERSED,  3 },
    {  6 | route_REVERSED,  3 },

    { 14,                   2 }, /* target wanders locations 56..63 */
    { 15,                   2 }, /* target wanders locations 56..63 */
    { 14 | route_REVERSED,  0 }, /* target wanders locations 8..15 */
    { 15 | route_REVERSED,  1 }, /* target wanders locations 16..23 */

    { 16,                   5 },
    { 17,                   5 },
    { 16 | route_REVERSED,  0 }, /* target wanders locations 8..15 */
    { 17 | route_REVERSED,  1 }, /* target wanders locations 16..23 */

    { 32 | route_REVERSED,  0 }, /* target wanders locations 8..15 */
    { 33 | route_REVERSED,  1 }, /* target wanders locations 16..23 */
    { 42,                   7 },
    { 44,                   8 }, /* hero sleeps */
    { 43,                   9 }, /* hero sits */
    { 36 | route_REVERSED,  6 },
    { 36,                  10 }, /* hero released from solitary */
    { 37,                   4 }  /* solitary ends */
  };

  /* $C829 */
  static charevnt_handler_t *const handlers[] =
  {
    &charevnt_handler_0,
    &charevnt_handler_1,
    &charevnt_handler_2,
    &charevnt_handler_3_check_var_A13E,
    &charevnt_handler_4_solitary_ends,
    &charevnt_handler_5_check_var_A13E_anotherone,
    &charevnt_handler_6,
    &charevnt_handler_7,
    &charevnt_handler_8_hero_sleeps,
    &charevnt_handler_9_hero_sits,
    &charevnt_handler_10_hero_released_from_solitary
  };

  uint8_t              routeindex; /* was A */
  const route2event_t *peventmap;  /* was HL */
  uint8_t              iters;      /* was B */

  assert(state != NULL);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  // When I first started pulling the game apart I thought that these values
  // identified characters but then got confused when I found another set of
  // character constants (now called character_t's).

  routeindex = route->index;
  if (routeindex >= 7 && routeindex <= 12) // 6 indices
  {
    character_sleeps(state, routeindex, route);
    return;
  }
  if (routeindex >= 18 && routeindex <= 22) // only 5 indices - is this why one character doesn't sit for breakfast? there's an entry in routes[] which looks like it ought to match
  {
    character_sits(state, routeindex, route);
    return;
  }

  // PUSH route (HL)

  peventmap = &eventmap[0];
  iters = NELEMS(eventmap);
  do
  {
    if (routeindex == peventmap->route)
    {
      /* Conv: call_action moved here. */
      handlers[peventmap->handler](state, route);
      return;
    }
    peventmap++;
  }
  while (--iters);

  // POP route (HL)

  route->index = 0; /* Stand still. */
}

/**
 * $C83F: Ends solitary.
 */
void charevnt_handler_4_solitary_ends(tgestate_t *state,
                                      route_t    *route)
{
  state->in_solitary = 0; /* Enable player control */
  charevnt_handler_0(state, route); // ($FF,$08)
}

/**
 * $C845: Sets route to { 3, 21 }.
 */
void charevnt_handler_6(tgestate_t *state,
                        route_t    *route)
{
  // this is something like: commandant walks to yard

  route->index = 3; // route_commandant
  route->step  = 21; // offset $15
}

/**
 * $C84C: Sets route to route 36 in reverse. Sets hero to route 37.
 */
void charevnt_handler_10_hero_released_from_solitary(tgestate_t *state,
                                                     route_t    *route)
{
  // If I remove the 128 here then when the commandant arrives to pick up the
  // hero he repeatedly re-enters the room instead of turning around and
  // leaving.

  route->index = 36 | route_REVERSED;
  route->step  = 3;

  state->automatic_player_counter = 0; /* Force automatic control */

  const route_t route_37 = { 37, 0 }; /* was BC */
  set_hero_route(state, route_37); // original jump was $A344, but have moved it
}

/**
 * $C85C: Sets route to wander around locations 16..23.
 */
void charevnt_handler_1(tgestate_t *state,
                        route_t    *route)
{
  // When .x is set to $FF it says to pick a random location from the
  // locations[] array from .y to .y+7.

  route->index = route_WANDER; /* Choose random locations[]. */
  route->step  = 16; /* locations 16..23 */
}

/**
 * $C860: Sets route to wander around locations 56..63.
 */
void charevnt_handler_2(tgestate_t *state,
                        route_t    *route)
{
  route->index = route_WANDER; /* Choose random locations[]. */
  route->step  = 56; /* locations 56..63 */
}

/**
 * $C864: Sets route to wander around locations 8..15.
 *
 * \param[in] route ? (was HL)
 */
void charevnt_handler_0(tgestate_t *state,
                        route_t    *route)
{
  route->index = route_WANDER; /* Choose random locations[]. */
  route->step  = 8; /* locations 8..15 */
}

// Conv: Inlined set_route_ffxx() in the above functions.

/**
 * $C86C:
 */
void charevnt_handler_3_check_var_A13E(tgestate_t *state,
                                       route_t    *route)
{
  if (state->entered_move_characters == 0)
    byte_A13E_is_zero(state, route);
  else
    byte_A13E_is_nonzero(state, route);
}

/**
 * $C877:
 */
void charevnt_handler_5_check_var_A13E_anotherone(tgestate_t *state,
                                                  route_t    *route)
{
  if (state->entered_move_characters == 0)
    byte_A13E_is_zero_anotherone(state, route);
  else
    byte_A13E_is_nonzero_anotherone(state, route);
}

/**
 * $C882: Sets route to ($05,$00).
 */
void charevnt_handler_7(tgestate_t *state,
                        route_t    *route)
{
  route->index = 5; // route_exit_hut2
  route->step  = 0;
}

/**
 * $C889: Hero sits.
 */
void charevnt_handler_9_hero_sits(tgestate_t *state,
                                  route_t    *route)
{
  hero_sits(state);
}

/**
 * $C88D: Hero sleeps.
 */
void charevnt_handler_8_hero_sleeps(tgestate_t *state,
                                    route_t    *route)
{
  hero_sleeps(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $C892: Causes characters to follow the hero if he's being suspicious.
 * Also: Food item discovery.
 * Also: Automatic hero behaviour.
 *
 * \param[in] state Pointer to game state.
 */
void follow_suspicious_character(tgestate_t *state)
{
  uint8_t iters; /* was B */

  assert(state != NULL);

  /* (I've still no idea what this flag means). */
  state->entered_move_characters = 0;

  /* If the bell is ringing, hostiles persue. */
  if (state->bell)
    hostiles_persue(state);

  /* If food was dropped then count down until it is discovered. */
  if (state->food_discovered_counter != 0 &&
      --state->food_discovered_counter == 0)
  {
    /* De-poison the food. */
    state->item_structs[item_FOOD].item_and_flags &= ~itemstruct_ITEM_FLAG_POISONED;
    item_discovered(state, item_FOOD);
  }

  /* Make supporting characters react (if flags are set). */
  state->IY = &state->vischars[1];
  iters = vischars_LENGTH - 1;
  do
  {
    vischar_t *vischar; /* was IY/new local copy */

    vischar = state->IY; // cache

    if (vischar->flags != vischar_FLAGS_EMPTY_SLOT)
    {
      character_t character; /* was A */

      character = vischar->character & vischar_CHARACTER_MASK;
      if (character <= character_19_GUARD_DOG_4) /* Hostile characters only. */
      {
        /* Characters 0..19. */

        is_item_discoverable(state);

        if (state->red_flag || state->automatic_player_counter > 0)
          guards_follow_suspicious_character(state, vischar);

        if (character >= character_16_GUARD_DOG_1)
        {
          /* Guard dogs 1..4 (characters 16..19). */

          /* Is the food nearby? */
          if (state->item_structs[item_FOOD].room_and_flags & itemstruct_ROOM_FLAG_NEARBY_7)
            vischar->flags = vischar_FLAGS_DOG_FOOD; // guards dogs are near the food (not necesarily poisoned?)
        }
      }

      character_behaviour(state, vischar);
    }
    state->IY++;
  }
  while (--iters);

  /* Engage automatic behaviour for the hero unless the flag is red, the
   * hero is in solitary or an input event was recently received. */
  if (!state->red_flag &&
      (state->in_solitary || state->automatic_player_counter == 0))
  {
    state->IY = &state->vischars[0];
    character_behaviour(state, &state->vischars[0]);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $C918: Character behaviour stuff.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void character_behaviour(tgestate_t *state, vischar_t *vischar)
{
  uint8_t    flags;             /* was A */
  uint8_t    counter_and_flags; /* was B */
  uint8_t    iters;             /* was B */
  vischar_t *vischar2;          /* was HL */
  uint8_t    Cdash;             /* was C' */
  uint8_t    scale;             /* Conv: added */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  counter_and_flags = vischar->counter_and_flags; // Conv: Use of A dropped.
  /* If the bottom nibble is set then decrement it. */
  if (counter_and_flags & vischar_BYTE7_MASK_LO)
  {
    vischar->counter_and_flags = --counter_and_flags;
    return; // if i nop this then characters get wedged
  }

  vischar2 = vischar; // no need for separate HL and IY now other code reduced
  flags = vischar2->flags;
  if (flags != 0)
  {
    if (flags == vischar_FLAGS_BRIBE_PENDING) // == 1
    {
set_pos:
      vischar2->pos.x = state->hero_map_position.x;
      vischar2->pos.y = state->hero_map_position.y;
      goto end_bit;
    }
    else if (flags == vischar_FLAGS_PERSUE) // == 2
    {
      if (state->automatic_player_counter > 0) // under player control, not automatic
        goto set_pos;

      // hero is under automatic control

      vischar2->flags = 0;
      (void) get_target_and_handle_it(state, vischar2, &vischar2->route);
      return;
    }
    else if (flags == vischar_FLAGS_DOG_FOOD) // == 3
    {
      if (state->item_structs[item_FOOD].room_and_flags & itemstruct_ROOM_FLAG_NEARBY_7)
      {
        // Moves dog toward poisoned food?
        vischar2->pos.x = state->item_structs[item_FOOD].pos.x;
        vischar2->pos.y = state->item_structs[item_FOOD].pos.y;
        goto end_bit;
      }
      else
      {
        vischar2->flags       = 0;
        vischar2->route.index = route_WANDER; /* Choose random locations[]. */
        vischar2->route.step  = 0; /* 0..7 */
        (void) get_target_and_handle_it(state, vischar2, &vischar2->route);
        return;
      }
    }
    else if (flags == vischar_FLAGS_SAW_BRIBE) // == 4
    {
      character_t  bribed_character; /* was A */
      vischar_t   *found;            /* was HL */

      bribed_character = state->bribed_character;
      if (bribed_character != character_NONE)
      {
        /* Iterate over non-player characters. */
        iters = vischars_LENGTH - 1;
        found = &state->vischars[1];
        do
        {
          if (found->character == bribed_character) /* Bug? This doesn't mask found->character with vischar_CHARACTER_MASK. */
            goto bribed_visible;
          found++;
        }
        while (--iters);
      }

      /* Bribed character was not visible. */
      vischar2->flags = 0;
      (void) get_target_and_handle_it(state, vischar2, &vischar2->route);
      return;

bribed_visible:
      /* Found bribed character in vischars. */
      {
        pos_t     *pos; /* was HL */
        tinypos_t *tinypos; /* was DE */

        pos     = &found->mi.pos;
        tinypos = &vischar2->pos;
        if (state->room_index > room_0_OUTDOORS)
        {
          /* Indoors */
          pos_to_tinypos(pos, tinypos);
        }
        else
        {
          /* Outdoors */
          tinypos->x = pos->x; // note: narrowing
          tinypos->y = pos->y; // note: narrowing
        }
        goto end_bit;
      }

    }
  }

  if (vischar2->route.index == 0) /* Stand still */
  {
    character_behaviour_set_input(state, vischar, 0 /* new_input */);
    return; // exit via
  }

end_bit:
  Cdash = vischar2->flags; // sampled = $8081, 80a1, 80e1, 8001, 8021, ...

  /* The original code self modifies the vischar_at_pos_x/y routines. */
//  if (state->room_index > room_0_OUTDOORS)
//    HLdash = &multiply_by_1;
//  else if (Cdash & vischar_FLAGS_DOOR_THING)
//    HLdash = &multiply_by_4;
//  else
//    HLdash = &multiply_by_8;
//
//  self_CA13 = HLdash; // self-modify vischar_at_pos_x:$CA13
//  self_CA4B = HLdash; // self-modify vischar_at_pos_y:$CA4B

  /* Replacement code passes down a scale factor. */
  if (state->room_index > room_0_OUTDOORS)
    scale = 1; /* Indoors. */
  else if (Cdash & vischar_FLAGS_DOOR_THING)
    scale = 4; /* Outdoors + door thing. */
  else
    scale = 8; /* Outdoors. */

  // I'm still trying to figure ot this IMPEDED flag. If it's clear and the
  // vischar matches its stored 'pos' then we enter bribes_solitary_food(),
  // else we character_behaviour_set_input(). However, if IMPEDED is set then
  // that is reversed.

  if (vischar->counter_and_flags & vischar_BYTE7_IMPEDED) // hit a wall etc?
    character_behaviour_impeded(state, vischar, scale); // exit via
  else if (vischar_at_pos_x(state, vischar, scale) == 0 &&
           vischar_at_pos_y(state, vischar, scale) == 0)
    /* Character is at stored pos. */
    bribes_solitary_food(state, vischar); // exit via
  else
    /* The zero passed in as the third argument here (new_input) is the
     * return value of 'A' in vischar_at_pos_x/y. */
    character_behaviour_set_input(state, vischar, 0); /* was fallthrough */
}

/**
 * $C9F5: Sets an input if different from current.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] vischar   Pointer to visible character. (was IY)
 * \param[in] new_input New input flags.              (was A)
 */
void character_behaviour_set_input(tgestate_t *state,
                                   vischar_t  *vischar,
                                   uint8_t     new_input)
{
  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  // assert(new_input);

  if (new_input != vischar->input)
    vischar->input = new_input | input_KICK;
  assert((vischar->input & ~input_KICK) < 9);
}

/**
 * $C9FF: Called when a character is (was) impeded.

 similar to end of $C918 but the sense is reversed...

 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.   (was IY)
 * \param[in] scale   Scale factor (replaces self-modifying code in original).
 */
void character_behaviour_impeded(tgestate_t *state,
                                 vischar_t  *vischar,
                                 int         scale)
{
  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(scale == 1 || scale == 4 || scale == 8);

  /* Note: The y,x order here looks unusual but it matches the original code. */
  if (vischar_at_pos_y(state, vischar, scale) == 0 &&
      vischar_at_pos_x(state, vischar, scale) == 0)
    /* Character is at stored pos. */
    character_behaviour_set_input(state, vischar, 0);
  else
    bribes_solitary_food(state, vischar); // exit via
}

/* ----------------------------------------------------------------------- */

/**
 * $CA11: How close is this vischar to its 'pos.x' coord?
 *
 * Note: Callers only ever test the return code against zero.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] scale   Scale factor. (replaces self-modifying code in original)
 *
 * \return 8 => x >  pos.x
 *         4 => x <  pos.x
 *         0 => x == pos.x
 */
uint8_t vischar_at_pos_x(tgestate_t *state,
                         vischar_t  *vischar,
                         int         scale)
{
  int16_t delta; /* was DE */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(scale == 1 || scale == 4 || scale == 8);

  /* I'm assuming here (until proven otherwise) that HL and IY point to the
   * same vischar on entry. */
  assert(vischar == state->IY);

  delta = vischar->mi.pos.x - vischar->pos.x * scale;
  /* Conv: Rewritten to simplify. Split D,E checking boiled down to -3/+3. */
  if (delta >= 3)
    return 8;
  else if (delta <= -3)
    return 4;
  else
  {
    vischar->counter_and_flags |= vischar_BYTE7_IMPEDED; // doubting that this is an impeded flag
    return 0;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CA49: How close is this vischar to its 'pos.y' coord?
 *
 * Nearly identical to vischar_at_pos_x.
 *
 * Note: Callers only ever test the return code against zero.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] scale   Scale factor. (replaces self-modifying code in original)
 *
 * \return 5 => y >  pos.y
 *         7 => y <  pos.y
 *         0 => y == pos.y
 */
uint8_t vischar_at_pos_y(tgestate_t *state,
                         vischar_t  *vischar,
                         int         scale)
{
  int16_t delta; /* was DE */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(scale == 1 || scale == 4 || scale == 8);

  /* I'm assuming here (until proven otherwise) that HL and IY point to the
   * same vischar on entry. */
  assert(vischar == state->IY);

  delta = vischar->mi.pos.y - vischar->pos.y * scale;
  /* Conv: Rewritten to simplify. Split D,E checking boiled down to -3/+3. */
  if (delta >= 3)
    return 5;
  else if (delta <= -3)
    return 7;
  else
  {
    vischar->counter_and_flags &= ~vischar_BYTE7_IMPEDED;
    return 0;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CA81: Bribes, solitary, food, 'character enters' sound.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void bribes_solitary_food(tgestate_t *state, vischar_t *vischar)
{
  uint8_t          flags_lower6;            /* was A */
  uint8_t          flags_all;               /* was C */
  uint8_t          food_discovered_counter; /* was A */
  uint8_t          step;                    /* was C */
  uint8_t          route;                   /* was A */
  doorindex_t      doorindex;               /* was A */
  uint8_t          route2;                  /* was A */
  const door_t    *door;                    /* was HL */
  const tinypos_t *tinypos;                 /* was HL */
  vischar_t       *vischarHL;               /* was HL */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  // In the original code HL is IY + 4 on entry.
  // In this version we replace HL references with IY ones.

  flags_all = flags_lower6 = vischar->flags;
  flags_lower6 &= vischar_FLAGS_MASK; // lower 6 bits only
  if (flags_lower6)
  {
    if (flags_lower6 == vischar_FLAGS_BRIBE_PENDING)
    {
      if (vischar->character == state->bribed_character)
        accept_bribe(state);
      else
        solitary(state); // failed to bribe?
      return; // exit via // factored out
    }
    else if (flags_lower6 == vischar_FLAGS_PERSUE ||
             flags_lower6 == vischar_FLAGS_SAW_BRIBE)
    {
      /* Perhaps when hostiles are distracted food remains undiscovered? */
      return;
    }

    /* Decide how long until food is discovered. */
    if ((state->item_structs[item_FOOD].item_and_flags & itemstruct_ITEM_FLAG_POISONED) == 0)
      food_discovered_counter = 32; /* food is not poisoned */
    else
      food_discovered_counter = 255; /* food is poisoned */
    state->food_discovered_counter = food_discovered_counter;

    vischar->route.index = 0; /* Stand still. */

    character_behaviour_set_input(state, vischar, 0 /* new_input */); // character_behaviour:$C9F5;
    return;
  }

  if (flags_all & vischar_FLAGS_DOOR_THING)
  {
    /* Results in character entering. */

    //orig:C = *--HL; // 80a3, 8083, 8063, 8003 // likely route
    //orig:A = *--HL; // 80a2 etc

    step  = vischar->route.step; // check order
    route = vischar->route.index;

    // A must be a door index
    doorindex = get_route(route)[step]; // follow this through
    if (vischar->route.index & route_REVERSED) // needless reload
      doorindex ^= door_REVERSE; // later given to get_door, so must be a doorindex_t, so the flag must be door_REVERSE

    // PUSH AF
    route2 = vischar->route.index; // $8002, ... // needless reload
    // Conv: This is the [-2]+1 pattern which works out -1/+1.
    if (route2 & route_REVERSED)
      vischar->route.step--;
    else
      vischar->route.step++;
    // POP AF

    door = get_door(doorindex); // door related
    vischar->room = (door->room_and_flags & ~door_FLAGS_MASK_DIRECTION) >> 2; // was (*HL >> 2) & 0x3F; // sampled HL = $790E, $7962, $795E => door position

    if ((door->room_and_flags & door_FLAGS_MASK_DIRECTION) <= direction_TOP_RIGHT)
      /* TOP_LEFT or TOP_RIGHT */
      tinypos = &door[1].pos; // was HL += 5; // next door pos
    else
      /* BOTTOM_RIGHT or BOTTOM_LEFT */
      tinypos = &door[0].pos; // was HL -= 3; // current door pos

    // PUSH HL_tinypos

    vischarHL = vischar;
    if (vischarHL == &state->vischars[0]) // was if ((HL & 0x00FF) == 0)
    {
      /* Hero's vischar only. */
      vischarHL->flags &= ~vischar_FLAGS_DOOR_THING;
      (void) get_target_and_handle_it(state, vischarHL, &vischarHL->route);
    }

    // POP HL_tinypos

    transition(state, tinypos);

    play_speaker(state, sound_CHARACTER_ENTERS_1);
    return;
  }

  /* When on a route (.x != 255) advance the counter (.y) in the required direction. */
  route = vischar->route.index;
  if (route != route_WANDER)
  {
    if (route & route_REVERSED)
      vischar->route.step--;
    else
      vischar->route.step++;
  }

  (void) get_target_and_handle_it(state, vischar, &vischar->route); // was fallthrough
}

/**
 * $CB23: Calls get_target
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] route   Pointer to vischar->route.    (was HL)
 */
uint8_t get_target_and_handle_it(tgestate_t *state,
                                 vischar_t  *vischar,
                                 route_t    *route)
{
  uint8_t          get_target_result; /* was A */
  const tinypos_t *doorpos;           /* was HL */
  const xy_t      *location;          /* was HL */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  get_target_result = get_target(state,
                                 route,
                                 &doorpos,
                                 &location);
  if (get_target_result != get_target_ROUTE_ENDS)
  {
    /* Didn't hit end of list case. */
    /* Chunk from $CB61 */

    if (get_target_result == get_target_DOOR)
      vischar->flags |= vischar_FLAGS_DOOR_THING;

    // Conv: Cope with get_target returning results in different vars.
    // Original game just transfers x,y across.
    if (get_target_result == get_target_DOOR)
    {
      vischar->pos.x = doorpos->x;
      vischar->pos.y = doorpos->y;
    }
    else
    {
      vischar->pos.x = location->x;
      vischar->pos.y = location->y;
    }

    return 1 << 7;
  }
  else
  {
    return route_ended(state, vischar, route); // was fallthrough
  }
}

/**
 * $CB2D: get_target has run out of route.
 *
 * Called by spawn_character, get_target_and_handle_it.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] route   Pointer to vischar->route.    (was HL)
 */
uint8_t route_ended(tgestate_t *state,
                    vischar_t  *vischar,
                    route_t    *route)
{
  character_t character;  /* was A */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  /* If not the hero's vischar ... */
  if (route != &state->vischars[0].route) /* Conv: was (L != 2) */
  {
    character = vischar->character & vischar_CHARACTER_MASK;

    /* Call character event for every step of the commandant route 36. */
    if (character == character_0_COMMANDANT)
      if ((route->index & ~route_REVERSED) == 36)
        goto do_character_event;

    if (character <= character_11_GUARD_11)
      goto reverse_route;
  }

do_character_event:
  /* We arrive here if:
   * - vischar is the hero, or
   * - character is character_0_COMMANDANT and (route.index & 0x7F) == 36, or
   * - character is >= character_12_GUARD_12
   */

  character_event(state, route);
  if (route->index == 0) /* standing still */
    return 0;
  return get_target_and_handle_it(state, vischar, route); // re-enter

reverse_route:
  /* We arrive here if:
   * - vischar is not the hero, and
   *   - character is character_0_COMMANDANT and (route.index & 0x7F) != 36, or
   *   - character is character_1_GUARD_1 .. character_11_GUARD_11
   */

  route->index ^= route_REVERSED; /* toggle route direction */
  // Conv: Removed [-2]+1 pattern.
  if (route->index & route_REVERSED)
    route->step--;
  else
    route->step++;

  return 0;
}

// $CB5F,2 Unreferenced bytes. [Absent in DOS version]

/* ----------------------------------------------------------------------- */

///**
// * $CB75: Widen A to BC.
// *
// * \param[in] A 'A' to be widened.
// *
// * \return 'A' widened to a uint16_t.
// */
//uint16_t multiply_by_1(uint8_t A)
//{
//  return A;
//}

/* ----------------------------------------------------------------------- */

/**
 * $CB79: Return a route.
 *
 * Used by the routines at get_target and bribes_solitary_food.
 *
 * \param[in] index Route index.
 *
 * \return Pointer to route data. (was DE)
 */

// get_target tells me that if (val & ~door_LOCKED) < 40 then it's a
// door else other values are +40.
//
// How does 40 relate to door_MAX?
//
// Highest door used here: 35, lowest: 0.
// Highest not-door used here: 77, lowest: 9.

const uint8_t *get_route(uint8_t index)
{
  /** Specifies a door within a route. */
#define DOOR(d) (d)
  /** Specifies a location index (in locations[]) within a route. */
#define LOCATION(d) (d + 40)

  static const uint8_t route_7795[] =
  {
    LOCATION(32),
    LOCATION(33),
    LOCATION(34),
    routebyte_END
  };
  static const uint8_t route_7799[] =
  {
    LOCATION(35),
    LOCATION(36),
    LOCATION(37),
    LOCATION(38),
    LOCATION(39),
    LOCATION(40),
    routebyte_END
  };
  /* I believe this to be the commandant's route. It's the longest and most
   * complex of the routes. */
  static const uint8_t route_commandant[] =
  {
    LOCATION(46),
    DOOR(31),                 // room_11_PAPERS   -> room_13_CORRIDOR
    DOOR(29),                 // room_13_CORRIDOR -> room_16_CORRIDOR
    DOOR(32),                 // room_16_CORRIDOR -> room_7_CORRIDOR
    DOOR(26),                 // room_7_CORRIDOR  -> room_17_CORRIDOR
    DOOR(35),                 // room_17_CORRIDOR -> room_12_CORRIDOR
    DOOR(25 | door_REVERSE),  // room_12_CORRIDOR -> room_18_RADIO
    DOOR(22 | door_REVERSE),  // room_18_RADIO    -> room_19_FOOD
    DOOR(21 | door_REVERSE),  // room_19_FOOD     -> room_23_BREAKFAST
    DOOR(20 | door_REVERSE),  // room_23_BREAKFAST-> room_21_CORRIDOR
    DOOR(23 | door_REVERSE),  // room_21_CORRIDOR -> room_22_REDKEY
    LOCATION(42),
    DOOR(23),                 // room_22_REDKEY   -> room_21_CORRIDOR
    DOOR(10 | door_REVERSE),  // room_21_CORRIDOR -> room_0_OUTDOORS
    DOOR(11),                 // room_0_OUTDOORS  -> room_20_REDCROSS
    DOOR(11 | door_REVERSE),  // room_20_REDCROSS -> room_0_OUTDOORS
    DOOR(12),                 // room_0_OUTDOORS  -> room_15_UNIFORM
    DOOR(27 | door_REVERSE),  // room_15_UNIFORM  -> room_14_TORCH
    DOOR(28),                 // room_14_TORCH    -> room_16_CORRIDOR
    DOOR(29 | door_REVERSE),  // room_16_CORRIDOR -> room_13_CORRIDOR
    DOOR(13 | door_REVERSE),  // room_13_CORRIDOR -> room_0_OUTDOORS
    LOCATION(11),             // charevnt_handler_6 jumps to this offset
    LOCATION(55),
    DOOR( 0 | door_REVERSE),  // room_0_OUTDOORS  -> room_0_OUTDOORS (gate 0 to yard)
    DOOR( 1 | door_REVERSE),  // room_0_OUTDOORS  -> room_0_OUTDOORS (gate 1 to yard)
    LOCATION(60),
    DOOR( 1),                 // room_0_OUTDOORS  -> room_0_OUTDOORS (return through gate 1)
    DOOR( 0),                 // room_0_OUTDOORS  -> room_0_OUTDOORS (return through gate 0)
    DOOR( 4),                 // room_0_OUTDOORS  -> room_28_HUT1LEFT
    DOOR(16),                 // room_28_HUT1LEFT -> room_1_HUT1RIGHT
    DOOR( 5 | door_REVERSE),  // room_1_HUT1RIGHT -> room_0_OUTDOORS
    LOCATION(11),
    DOOR( 7),                 // room_0_OUTDOORS  -> room_3_HUT2RIGHT
    DOOR(17 | door_REVERSE),  // room_3_HUT2RIGHT -> room_2_HUT2LEFT
    DOOR( 6 | door_REVERSE),  // room_2_HUT2LEFT  -> room_0_OUTDOORS
    DOOR( 8),                 // room_0_OUTDOORS  -> room_4_HUT3LEFT
    DOOR(18),                 // room_4_HUT3LEFT  -> room_5_HUT3RIGHT
    DOOR( 9 | door_REVERSE),  // room_5_HUT3RIGHT -> room_0_OUTDOORS
    LOCATION(45),
    DOOR(14),                 // room_0_OUTDOORS  -> room_8_CORRIDOR
    DOOR(34),                 // room_8_CORRIDOR  -> room_9_CRATE
    DOOR(34 | door_REVERSE),  // room_9_CRATE     -> room_8_CORRIDOR
    DOOR(33),                 // room_8_CORRIDOR  -> room_10_LOCKPICK
    DOOR(33 | door_REVERSE),  // room_10_LOCKPICK -> room_8_CORRIDOR
    routebyte_END
  };
  static const uint8_t route_77CD[] =
  {
    LOCATION(43),
    LOCATION(44),
    routebyte_END
  };
  static const uint8_t route_exit_hut2[] =
  {
    DOOR(7 | door_REVERSE),   // room_3_HUT2RIGHT -> room_0_OUTDOORS
    LOCATION(11),
    LOCATION(12),
    routebyte_END
  };
  static const uint8_t route_exit_hut3[] =
  {
    DOOR(9 | door_REVERSE),   // room_5_HUT3RIGHT -> room_0_OUTDOORS
    LOCATION(45),
    LOCATION(14),
    routebyte_END
  };
  static const uint8_t route_prisoner_sleeps_1[] =
  {
    LOCATION(46),
    routebyte_END
  };
  static const uint8_t route_prisoner_sleeps_2[] =
  {
    LOCATION(47),
    routebyte_END
  };
  static const uint8_t route_prisoner_sleeps_3[] =
  {
    LOCATION(48),
    routebyte_END
  };
  static const uint8_t route_77DE[] =
  {
    LOCATION(52),
    LOCATION(53),
    routebyte_END
  };
  static const uint8_t route_77E1[] =
  {
    LOCATION(11),
    LOCATION(55),
    DOOR(0 | door_REVERSE),   // move through gates
    DOOR(1 | door_REVERSE),   // move through gates to yard
    LOCATION(56),
    routebyte_END
  };
  static const uint8_t route_77E7[] =
  {
    LOCATION(12),
    DOOR(10),                 // room_0_OUTDOORS   -> room_21_CORRIDOR
    DOOR(20),                 // room_21_CORRIDOR  -> room_23_BREAKFAST
    DOOR(19 | door_REVERSE),  // room_23_BREAKFAST -> room_25_BREAKFAST
    routebyte_END
  };
  static const uint8_t route_77EC[] =
  {
    LOCATION(16),
    LOCATION(12),
    DOOR(10),                 // room_0_OUTDOORS  -> room_21_CORRIDOR
    DOOR(20),                 // room_21_CORRIDOR -> room_23_BREAKFAST
    routebyte_END
  };
  static const uint8_t route_prisoner_sits_1[] =
  {
    LOCATION(64),
    routebyte_END
  };
  static const uint8_t route_prisoner_sits_2[] =
  {
    LOCATION(65),
    routebyte_END
  };
  static const uint8_t route_prisoner_sits_3[] =
  {
    LOCATION(66),
    routebyte_END
  };
  static const uint8_t route_guardA_breakfast[] =
  {
    LOCATION(68),
    routebyte_END
  };
  static const uint8_t route_guardB_breakfast[] =
  {
    LOCATION(69),
    routebyte_END
  };
  static const uint8_t route_guard_12_roll_call[] =
  {
    LOCATION(9),
    routebyte_END
  };
  static const uint8_t route_guard_13_roll_call[] =
  {
    LOCATION(11),
    routebyte_END
  };
  static const uint8_t route_guard_14_roll_call[] =
  {
    LOCATION(17),
    routebyte_END
  };
  static const uint8_t route_guard_15_roll_call[] =
  {
    LOCATION(49),
    routebyte_END
  };
  static const uint8_t route_prisoner_1_roll_call[] =
  {
    LOCATION(72),
    routebyte_END
  };
  static const uint8_t route_prisoner_2_roll_call[] =
  {
    LOCATION(73),
    routebyte_END
  };
  static const uint8_t route_prisoner_3_roll_call[] =
  {
    LOCATION(74),
    routebyte_END
  };
  static const uint8_t route_prisoner_4_roll_call[] =
  {
    LOCATION(75),
    routebyte_END
  };
  static const uint8_t route_prisoner_5_roll_call[] =
  {
    LOCATION(76),
    routebyte_END
  };
  static const uint8_t route_prisoner_6_roll_call[] =
  {
    LOCATION(77),
    routebyte_END
  };
  static const uint8_t route_go_to_solitary[] =
  {
    LOCATION(14),
    DOOR(10),                 // room_0_OUTDOORS  -> room_21_CORRIDOR
    DOOR(23 | door_REVERSE),  // room_21_CORRIDOR -> room_22_REDKEY
    DOOR(24 | door_REVERSE),  // room_22_REDKEY   -> room_24_SOLITARY
    LOCATION(42),
    routebyte_END
  };
  static const uint8_t route_hero_leave_solitary[] =
  {
    DOOR(24),                 // room_24_SOLITARY -> room_22_REDKEY
    DOOR(23),                 // room_22_REDKEY   -> room_21_CORRIDOR
    DOOR(10 | door_REVERSE),  // room_21_CORRIDOR -> room_0_OUTDOORS
    LOCATION(14),
    routebyte_END
  };
  static const uint8_t route_guard_12_bed[] =
  {
    LOCATION(12),
    LOCATION(11),
    DOOR(7),                  // room_0_OUTDOORS -> room_3_HUT2RIGHT
    LOCATION(52),
    routebyte_END
  };
  static const uint8_t route_guard_13_bed[] =
  {
    LOCATION(12),
    LOCATION(11),
    DOOR(7),                  // room_0_OUTDOORS  -> room_3_HUT2RIGHT
    DOOR(17 | door_REVERSE),  // room_3_HUT2RIGHT -> room_2_HUT2LEFT
    LOCATION(53),
    routebyte_END
  };
  static const uint8_t route_guard_14_bed[] =
  {
    LOCATION(12),
    LOCATION(11),
    LOCATION(45),
    DOOR(9),                  // room_0_OUTDOORS -> room_5_HUT3RIGHT
    LOCATION(52),
    routebyte_END
  };
  static const uint8_t route_guard_15_bed[] =
  {
    LOCATION(12),
    LOCATION(11),
    LOCATION(45),
    DOOR(9),                  // room_0_OUTDOORS -> room_5_HUT3RIGHT
    LOCATION(53),
    routebyte_END
  };
  static const uint8_t route_hut2_left_to_right[] =
  {
    DOOR(17),                 // room_2_HUT2LEFT -> room_3_HUT2RIGHT
    routebyte_END
  };
  static const uint8_t route_7833[] =
  {
    LOCATION(67),
    routebyte_END
  };
  static const uint8_t route_hut2_right_to_left[] =
  {
    DOOR(17 | door_REVERSE),  // room_3_HUT2RIGHT -> room_2_HUT2LEFT
    LOCATION(70),
    routebyte_END
  };
  static const uint8_t route_hero_roll_call[] =
  {
    LOCATION(50),
    routebyte_END
  };
#undef DOOR
#undef LOCATION

  /**
   * $7738: Table of pointers to routes.
   */
  static const uint8_t *routes[46] =
  {
    NULL, /* was zero */            //  0: route 0 => stand still (as opposed to route 255 => wander around randomly)  hero in solitary

    &route_7795[0],                 //  1:
    &route_7799[0],                 //  2:
    &route_commandant[0],           //  3: set by charevnt_handler_6 [longest of all the routes -- comandants's route perhaps?]
    &route_77CD[0],                 //  4:

    &route_exit_hut2[0],            //  5: character_1x_GUARD_12/13, character_2x_PRISONER_1/2/3 by wake_up & go_to_time_for_bed, go_to_time_for_bed (for hero),
    &route_exit_hut3[0],            //  6: character_1x_GUARD_14/15, character_2x_PRISONER_4/5/6 by wake_up & go_to_time_for_bed

    // character_sleeps routes
    &route_prisoner_sleeps_1[0],    //  7: prisoner 1 by byte_A13E_common
    &route_prisoner_sleeps_2[0],    //  8: prisoner 2 by byte_A13E_common
    &route_prisoner_sleeps_3[0],    //  9: prisoner 3 by byte_A13E_common
    &route_prisoner_sleeps_1[0],    // 10: prisoner 4 by byte_A13E_common /* dupes index 7 */
    &route_prisoner_sleeps_2[0],    // 11: prisoner 5 by byte_A13E_common /* dupes index 8 */
    &route_prisoner_sleeps_3[0],    // 12: prisoner 6 by byte_A13E_common /* dupes index 9 */

    &route_77DE[0],                 // 13: (all hostiles) by byte_A13E_common

    &route_77E1[0],                 // 14: set by set_route_0x8E04 (for hero, prisoners_and_guards-1), set_route_0x0E00 (for hero, prisoners_and_guards-1)
    &route_77E1[0],                 // 15: set by set_route_0x8E04 (for hero, prisoners_and_guards-2), set_route_0x0E00 (for hero, prisoners_and_guards-2)  /* dupes index 14 */

    &route_77E7[0],                 // 16: set by set_route_0x1000, end_of_breakfast (for hero), prisoners_and_guards-1 by end_of_breakfast
    &route_77EC[0],                 // 17:                                                       prisoners_and_guards-2 by end_of_breakfast

    // character_sits routes
    &route_prisoner_sits_1[0],      // 18: character_20_PRISONER_1 by byte_A13E_anotherone_common  // character next to player when player seated
    &route_prisoner_sits_2[0],      // 19: character_21_PRISONER_2 by byte_A13E_anotherone_common
    &route_prisoner_sits_3[0],      // 20: character_22_PRISONER_3 by byte_A13E_anotherone_common
    &route_prisoner_sits_1[0],      // 21: character_23_PRISONER_4 by byte_A13E_anotherone_common  /* dupes index 18 */
    &route_prisoner_sits_2[0],      // 22: character_24_PRISONER_5 by byte_A13E_anotherone_common  /* dupes index 19 */
    &route_prisoner_sits_3[0],      // 23: character_25_PRISONER_6 by byte_A13E_anotherone_common  /* dupes index 20 */ // looks like it belongs with above chunk but not used (bug)

    &route_guardA_breakfast[0],     // 24: "even guard" by byte_A13E_anotherone_common
    &route_guardB_breakfast[0],     // 25: "odd guard" by byte_A13E_anotherone_common

    &route_guard_12_roll_call[0],   // 26: character_12_GUARD_12   by go_to_roll_call
    &route_guard_13_roll_call[0],   // 27: character_13_GUARD_13   by go_to_roll_call
    &route_prisoner_1_roll_call[0], // 28: character_20_PRISONER_1 by go_to_roll_call
    &route_prisoner_2_roll_call[0], // 29: character_21_PRISONER_2 by go_to_roll_call
    &route_prisoner_3_roll_call[0], // 30: character_22_PRISONER_3 by go_to_roll_call
    &route_guard_14_roll_call[0],   // 31: character_14_GUARD_14   by go_to_roll_call
    &route_guard_15_roll_call[0],   // 32: character_15_GUARD_15   by go_to_roll_call
    &route_prisoner_4_roll_call[0], // 33: character_23_PRISONER_4 by go_to_roll_call
    &route_prisoner_5_roll_call[0], // 34: character_24_PRISONER_5 by go_to_roll_call
    &route_prisoner_6_roll_call[0], // 35: character_25_PRISONER_6 by go_to_roll_call

    &route_go_to_solitary[0],       // 36: set by charevnt_handler_10_hero_released_from_solitary (for commandant?), tested by route_ended
    &route_hero_leave_solitary[0],  // 37: set by charevnt_handler_10_hero_released_from_solitary (for hero)

    &route_guard_12_bed[0],         // 38: guard 12 at 'time for bed', 'search light'
    &route_guard_13_bed[0],         // 39: guard 13 at 'time for bed', 'search light'
    &route_guard_14_bed[0],         // 40: guard 14 at 'time for bed', 'search light'
    &route_guard_15_bed[0],         // 41: guard 15 at 'time for bed', 'search light'

    &route_hut2_left_to_right[0],   // 42:

    &route_7833[0],                 // 43: set by byte_A13E_is_zero_anotherone, process_player_input (was at breakfast case)
    &route_hut2_right_to_left[0],   // 44: set by process_player_input (was in bed case), set by byte_A13E_is_zero (for the hero)

    &route_hero_roll_call[0],       // 45: (hero) by go_to_roll_call

    // original game has an FF byte here
  };

  /* Conv: index may have flag bit 7 set so mask it off. */
  index &= ~route_REVERSED;

  assert(index < NELEMS(routes));

  return routes[index];
}

/* ----------------------------------------------------------------------- */

/**
 * $CB85: Pseudo-random number generator.
 *
 * Leaf. Only ever used by get_target.
 *
 * \return Pseudo-random number from 0..15.
 */
uint8_t random_nibble(tgestate_t *state)
{
#if IMPRACTICAL_VERSION
  /* Impractical code which mimics the original game.
   * This is here as an illustration only. */

  uint8_t *HL;
  uint8_t  A;

  /* prng_index is initialised to point at $9000. */

  HL = state->prng_index + 1; /* Increments L only, so wraps at 256. */
  A = *HL & 0x0F;
  state->prng_index = HL;

  return A;
#else
  /* Conv: The original code treats $C41A as a pointer. This points to bytes
   * at $9000..$90FF in the exterior tiles data. This routine fetches a byte
   * from this location and returns it to the caller as a nibble, giving a
   * simple pseudo-random number source.
   *
   * Poking around in our own innards will be too fragile for the portable C
   * version, but we still want to emulate the game precisely, so we capture
   * all 256 nibbles from the original game into the following array and use
   * it as our random data source.
   */

  static const uint32_t packed_nibbles[] =
  {
    0x00000000,
    0x00CBF302,
    0x00C30000,
    0x00000000,
    0x3C0800C3,
    0xC0000000,
    0x00CFD3CF,
    0xDFFFF7FF,
    0xFFDFFFBF,
    0xFDFC3FFF,
    0xFF37C000,
    0xCC003C00,
    0xB4444B80,
    0x34026666,
    0x66643C00,
    0x66666426,
    0x66643FC0,
    0x66642664,
    0xF5310000,
    0x3DDDDDBB,
    0x26666666,
    0x200003FC,
    0x34BC2666,
    0xC82C3426,
    0x3FC26666,
    0x3CFFF3CF,
    0x3DDDDDBB,
    0x43C2DFFB,
    0x3FC3C3F3,
    0xC3730003,
    0xC0477643,
    0x2C34002C,
  };

  int prng_index;
  int row, column;

  assert(state != NULL);

  prng_index = ++state->prng_index; /* Wraps around at 256 */
  row    = prng_index >> 3;
  column = prng_index & 7;

  assert(row < NELEMS(packed_nibbles));

  return (packed_nibbles[row] >> (column * 4)) & 0x0F;
#endif
}

/* ----------------------------------------------------------------------- */

/**
 * $CB98: Send the hero to solitary.
 *
 * \param[in] state Pointer to game state.
 */
void solitary(tgestate_t *state)
{
  /**
   * $7AC6: Solitary map position.
   */
  static const tinypos_t solitary_pos =
  {
    58, 42, 24
  };

  /**
   * $CC31: Partial character struct.
   */
  static const uint8_t solitary_commandant_reset_data[6] =
  {
    room_0_OUTDOORS,  // room
    116, 100, 3, // pos
    36, 0        // route
  };

  item_t       *pitem;       /* was HL */
  item_t        item;        /* was C */
  uint8_t       iters;       /* was B */
  vischar_t    *vischar;     /* was IY */
  itemstruct_t *pitemstruct; /* was HL */

  assert(state != NULL);

  /* Silence the bell. */
  state->bell = bell_STOP;

  /* Seize the hero's held items. */
  pitem = &state->items_held[0];
  item = *pitem;
  *pitem = item_NONE;
  item_discovered(state, item);

  pitem = &state->items_held[1];
  item = *pitem;
  *pitem = item_NONE;
  item_discovered(state, item);

  draw_all_items(state);

  /* Discover all items. */
  iters = item__LIMIT;
  pitemstruct = &state->item_structs[0];
  do
  {
    /* Is the item outdoors? */
    if ((pitemstruct->room_and_flags & itemstruct_ROOM_MASK) == room_0_OUTDOORS)
    {
      item_t     item_and_flags; /* was A */
      tinypos_t *itempos;        /* was HL */
      uint8_t    area;           /* was A' */

      item_and_flags = pitemstruct->item_and_flags;
      itempos = &pitemstruct->pos;
      area = 0; /* iterate 0,1,2 */
      do
        /* If the item is within the camp bounds then it will be discovered.
         */
        if (within_camp_bounds(area, itempos) == 0)
          goto discovered;
      while (++area != 3);

      goto next;

discovered:
      item_discovered(state, item_and_flags);
    }

next:
    pitemstruct++;
  }
  while (--iters);

  /* Move the hero to solitary. */
  state->vischars[0].room = room_24_SOLITARY;

  /* I reckon this should be 24 which is the door between room_22_REDKEY and
   * room_24_SOLITARY. */
  state->current_door = 20;

  decrease_morale(state, 35);

  reset_map_and_characters(state);

  /* Set the commandant on a path which results in the hero being released. */
  memcpy(&state->character_structs[character_0_COMMANDANT].room, &solitary_commandant_reset_data, 6);

  queue_message_for_display(state, message_YOU_ARE_IN_SOLITARY);
  queue_message_for_display(state, message_WAIT_FOR_RELEASE);
  queue_message_for_display(state, message_ANOTHER_DAY_DAWNS);

  state->in_solitary = 255; /* inhibit user input */
  state->automatic_player_counter = 0; /* immediately take automatic control of hero */
  state->vischars[0].mi.sprite = &sprites[sprite_PRISONER_FACING_AWAY_1];
  vischar = &state->vischars[0];
  state->IY = &state->vischars[0];
  vischar->direction = direction_BOTTOM_LEFT;
  state->vischars[0].route.index = 0; /* Stand still. */ // stores a byte, not a word
  transition(state, &solitary_pos);

  NEVER_RETURNS;
}

/* ----------------------------------------------------------------------- */

/**
 * $CC37: Guards follow suspicious character.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character (hostile only). (was IY / HL)
 */
void guards_follow_suspicious_character(tgestate_t *state,
                                        vischar_t  *vischar)
{
  character_t  character; /* was A */
  tinypos_t   *tinypos;   /* was DE */
  pos_t       *pos;       /* was HL */

  assert(state   != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  /* Conv: Copy of vischar in HL factored out. */

  character = vischar->character;

  /* Wearing the uniform stops anyone but the commandant from following the
   * hero. */
  if (character != character_0_COMMANDANT &&
      state->vischars[0].mi.sprite == &sprites[sprite_GUARD_FACING_AWAY_1])
    return;

  /* If this character saw the bribe being used then ignore the hero. */
  if (vischar->flags == vischar_FLAGS_SAW_BRIBE)
    return;

  pos = &vischar->mi.pos; /* was HL += 15 */
  tinypos = &state->tinypos_stash; // find out if this use of tinypos_stash ever intersects with the use of tinypos_stash for the mask rendering
  if (state->room_index == room_0_OUTDOORS)
  {
    tinypos_t *hero_map_pos;  /* was HL */
    int        dir;           /* Conv */
    uint8_t    direction;     /* was A / C */

    pos_to_tinypos(pos, tinypos); // tinypos_stash becomes vischar mi pos

    hero_map_pos = &state->hero_map_position;
    /* Conv: Dupe tinypos ptr factored out. */

    direction = vischar->direction; /* This character's direction. */
    /* Conv: Avoided shifting 'direction' here. Propagated shift into later ops. */

    /* Guess: This is orienting the hostile to face the hero. */

    if ((direction & 1) == 0) /* TL or BR */
    {
      // range check (uses A as temporary)
      if (tinypos->y - 1 >= hero_map_pos->y || tinypos->y + 1 < hero_map_pos->y)
        return;

      dir = tinypos->x < hero_map_pos->x;
      if ((direction & 2) == 0) /* TL (can't be TR) */
        dir = !dir;
      if (dir)
        return;
    }

    // range check (uses A as temporary)
    if (tinypos->x - 1 >= hero_map_pos->x || tinypos->x + 1 < hero_map_pos->x)
      return;

    dir = tinypos->height < hero_map_pos->height;
    if ((direction & 2) == 0) /* TL or TR */
      dir = !dir;
    if (dir)
      return;
  }

  if (!state->red_flag)
  {
    /* Hostiles *not* in guard towers persue the hero. */
    if (vischar->mi.pos.height < 32) /* uses A as temporary */
      vischar->flags = vischar_FLAGS_PERSUE;
  }
  else
  {
    state->bell = bell_RING_PERPETUAL;
    hostiles_persue(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CCAB: Hostiles persue prisoners.
 *
 * For all visible, hostile characters, at height < 32, set the bribed/persue
 * flag.
 *
 * \param[in] state Pointer to game state.
 */
void hostiles_persue(tgestate_t *state)
{
  vischar_t *vischar; /* was HL */
  uint8_t    iters;   /* was B */

  assert(state != NULL);

  vischar = &state->vischars[1];
  iters   = vischars_LENGTH - 1;
  do
  {
    if (vischar->character <= character_19_GUARD_DOG_4 &&
        vischar->mi.pos.height < 32) /* Excludes the guards high up in towers. */
      vischar->flags = vischar_FLAGS_BRIBE_PENDING;
    vischar++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $CCCD: Is item discoverable?
 *
 * Searches item_structs for items dropped nearby. If items are found the
 * hostiles are made to persue the hero.
 *
 * Green key and food items are ignored.
 *
 * \param[in] state Pointer to game state.
 */
void is_item_discoverable(tgestate_t *state)
{
  room_t              room;       /* was A */
  const itemstruct_t *itemstruct; /* was HL */
  uint8_t             iters;      /* was B */
  item_t              item;       /* was A */

  assert(state != NULL);

  room = state->room_index;
  if (room != room_0_OUTDOORS)
  {
    if (is_item_discoverable_interior(state, room, NULL) == 0) /* Item found */
      hostiles_persue(state);
  }
  else
  {
    itemstruct = &state->item_structs[0];
    iters      = item__LIMIT;
    do
    {
      if (itemstruct->room_and_flags & itemstruct_ROOM_FLAG_NEARBY_7)
        goto nearby;

next:
      itemstruct++;
    }
    while (--iters);
    return;

nearby:
    // FUTURE: merge into loop
    item = itemstruct->item_and_flags & itemstruct_ITEM_MASK;

    /* The green key and food items are ignored. */
    if (item == item_GREEN_KEY || item == item_FOOD)
      goto next;

    /* Suspected bug in original game appears here: itemstruct pointer is
     * decremented to access item_and_flags but is not re-adjusted
     * afterwards. */

    hostiles_persue(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CCFB: Is an item discoverable indoors?
 *
 * A discoverable item is one moved away from its default room, and one that
 * isn't the red cross parcel.
 *
 * \param[in]  state Pointer to game state.
 * \param[in]  room  Room index. (was A)
 * \param[out] pitem Pointer to item (if found). (Can be NULL). (was C)
 *
 * \return 0 if found, 1 if not found. (Conv: Was Z flag).
 */
int is_item_discoverable_interior(tgestate_t *state,
                                  room_t      room,
                                  item_t     *pitem)
{
  const itemstruct_t *itemstr; /* was HL */
  uint8_t             iters;   /* was B */
  item_t              item;    /* was A */

  assert(state != NULL);
  assert((room >= 0 && room < room__LIMIT) || (room == room_NONE));
  /* pitem may be NULL */

  itemstr = &state->item_structs[0];
  iters   = item__LIMIT;
  do
  {
    /* Is the item in the specified room? */
    if ((itemstr->room_and_flags & itemstruct_ROOM_MASK) == room &&
        /* Has the item been moved to a different room? */
        /* Bug? Note that room_and_flags doesn't get its flag bits masked off.
         * Does it need & 0x3F ? */ // FUTURE: add an assert to check this
        // wiresnips' room_and_flags is 255 so will never match 'room' (0..room_limit)
        // so the effect is ... wiresnips are always discoverable?
        default_item_locations[itemstr->item_and_flags & itemstruct_ITEM_MASK].room_and_flags != room)
    {
      item = itemstr->item_and_flags & itemstruct_ITEM_MASK;

      /* Ignore the red cross parcel. */
      if (item != item_RED_CROSS_PARCEL)
      {
        if (pitem) /* Conv: Added to support return of pitem. */
          *pitem = item;

        return 0; /* found */
      }
    }

    itemstr++;
  }
  while (--iters);

  return 1; /* not found */
}

/* ----------------------------------------------------------------------- */

/**
 * $CD31: Item discovered. / reset item
 *
 * \param[in] state Pointer to game state.
 * \param[in] item  Item. (was C)
 */
void item_discovered(tgestate_t *state, item_t item)
{
  const default_item_location_t *default_item_location; /* was HL/DE */
  room_t                         room;                  /* was A */
  itemstruct_t                  *itemstruct;            /* was HL/DE */

  assert(state != NULL);

  if (item == item_NONE)
    return;

  ASSERT_ITEM_VALID(item);

  item &= itemstruct_ITEM_MASK;

  queue_message_for_display(state, message_ITEM_DISCOVERED);
  decrease_morale(state, 5);

  default_item_location = &default_item_locations[item];
  room = default_item_location->room_and_flags;

  /* Conv: 'item' was not masked in the original code (bug fix). */
  itemstruct = item_to_itemstruct(state, item);
  itemstruct->item_and_flags &= ~itemstruct_ITEM_FLAG_HELD;
  itemstruct->room_and_flags = default_item_location->room_and_flags;
  itemstruct->pos.x = default_item_location->pos.x;
  itemstruct->pos.y = default_item_location->pos.y;

  if (room == room_0_OUTDOORS)
  {
    /* Conv: The original code just assigned A/'room' here, as it's already zero. */
    itemstruct->pos.height = 0;
    calc_exterior_item_screenpos(itemstruct);
  }
  else
  {
    itemstruct->pos.height = 5;
    calc_interior_item_screenpos(itemstruct);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CD6A: Default item locations.
 */

#define ITEM_ROOM(room_no, flags) ((room_no & 0x3F) | (flags << 6))

const default_item_location_t default_item_locations[item__LIMIT] =
{
  { ITEM_ROOM(room_NONE,        3), { 64, 32 } }, /* item_WIRESNIPS        */
  { ITEM_ROOM(room_9_CRATE,     0), { 62, 48 } }, /* item_SHOVEL           */
  { ITEM_ROOM(room_10_LOCKPICK, 0), { 73, 36 } }, /* item_LOCKPICK         */
  { ITEM_ROOM(room_11_PAPERS,   0), { 42, 58 } }, /* item_PAPERS           */
  { ITEM_ROOM(room_14_TORCH,    0), { 50, 24 } }, /* item_TORCH            */
  { ITEM_ROOM(room_NONE,        0), { 36, 44 } }, /* item_BRIBE            */
  { ITEM_ROOM(room_15_UNIFORM,  0), { 44, 65 } }, /* item_UNIFORM          */
  { ITEM_ROOM(room_19_FOOD,     0), { 64, 48 } }, /* item_FOOD             */
  { ITEM_ROOM(room_1_HUT1RIGHT, 0), { 66, 52 } }, /* item_POISON           */
  { ITEM_ROOM(room_22_REDKEY,   0), { 60, 42 } }, /* item_RED_KEY          */
  { ITEM_ROOM(room_11_PAPERS,   0), { 28, 34 } }, /* item_YELLOW_KEY       */
  { ITEM_ROOM(room_0_OUTDOORS,  0), { 74, 72 } }, /* item_GREEN_KEY        */
  { ITEM_ROOM(room_NONE,        0), { 28, 50 } }, /* item_RED_CROSS_PARCEL */
  { ITEM_ROOM(room_18_RADIO,    0), { 36, 58 } }, /* item_RADIO            */
  { ITEM_ROOM(room_NONE,        0), { 30, 34 } }, /* item_PURSE            */
  { ITEM_ROOM(room_NONE,        0), { 52, 28 } }, /* item_COMPASS          */
};

#undef ITEM_ROOM

/* ----------------------------------------------------------------------- */

/* Conv: Moved to precede parent array. */

// spriteindex's top bit is probably a flip flag.

/* Define a shorthand for the flip flag. */
#define F sprite_FLAG_FLIP
#define _ 0

/* The first byte is the number of trailing anim_t's. */

// struct animation
// {
//   uint8_t count;     // how many anims in anim[]
//   uint8_t A;         // unknown yet
//   uint8_t B;         // unknown yet
//                      // A+B differ when changing direction
//   uint8_t direction; // selects map movement routine in move_map:
//                      // 0 = player moves down-right (map is shifted up-left)
//                      // 1 = player moves down-left  (map is shifted up-right)
//                      // 2 = player moves up-left    (map is shifted down-right)
//                      // 3 = player moves up-right   (map is shifted down-left)
//                      // 255 = don't move the map
//                      // perhaps a direction_t if interpretation of this field is "which direction to move the map"
//   anim_t  anim[UNKNOWN];
//                      // each anim has x/y/z - a triple of delta values used for movement
//                      // z's are always zero as no movement changes height
// };

static const uint8_t anim_crawlwait_tl[] = { 1, 4,4,255,  0, 0, 0, _|10 };
static const uint8_t anim_crawlwait_tr[] = { 1, 5,5,255,  0, 0, 0, F|10 };
static const uint8_t anim_crawlwait_br[] = { 1, 6,6,255,  0, 0, 0, F|8  };
static const uint8_t anim_crawlwait_bl[] = { 1, 7,7,255,  0, 0, 0, _|8  };

static const uint8_t anim_walk_tl[]      = { 4, 0,0,2,    2, 0, 0, _|0,  2, 0, 0, _|1,  2, 0, 0, _|2,  2, 0, 0, _|3 };
static const uint8_t anim_walk_tr[]      = { 4, 1,1,3,    0, 2, 0, F|0,  0, 2, 0, F|1,  0, 2, 0, F|2,  0, 2, 0, F|3 };
static const uint8_t anim_walk_br[]      = { 4, 2,2,0,   -2, 0, 0, _|4, -2, 0, 0, _|5, -2, 0, 0, _|6, -2, 0, 0, _|7 };
static const uint8_t anim_walk_bl[]      = { 4, 3,3,1,    0,-2, 0, F|4,  0,-2, 0, F|5,  0,-2, 0, F|6,  0,-2, 0, F|7 };

static const uint8_t anim_wait_tl[]      = { 1, 0,0,255,  0, 0, 0, _|0  };
static const uint8_t anim_wait_tr[]      = { 1, 1,1,255,  0, 0, 0, F|0  };
static const uint8_t anim_wait_br[]      = { 1, 2,2,255,  0, 0, 0, _|4  };
static const uint8_t anim_wait_bl[]      = { 1, 3,3,255,  0, 0, 0, F|4  };

static const uint8_t anim_turn_tl[]      = { 2, 0,1,255,  0, 0, 0, _|0,  0, 0, 0, F|0  };
static const uint8_t anim_turn_tr[]      = { 2, 1,2,255,  0, 0, 0, F|0,  0, 0, 0, _|4  };
static const uint8_t anim_turn_br[]      = { 2, 2,3,255,  0, 0, 0, _|4,  0, 0, 0, F|4  };
static const uint8_t anim_turn_bl[]      = { 2, 3,0,255,  0, 0, 0, F|4,  0, 0, 0, _|0  };

static const uint8_t anim_crawl_tl[]     = { 2, 4,4,2,    2, 0, 0, _|10, 2, 0, 0, _|11 };
static const uint8_t anim_crawl_tr[]     = { 2, 5,5,3,    0, 2, 0, F|10, 0, 2, 0, F|11 };
static const uint8_t anim_crawl_br[]     = { 2, 6,6,0,   -2, 0, 0, F|8, -2, 0, 0, F|9  };
static const uint8_t anim_crawl_bl[]     = { 2, 7,7,1,    0,-2, 0, _|8,  0,-2, 0, _|9  };

static const uint8_t anim_crawlturn_tl[] = { 2, 4,5,255,  0, 0, 0, _|10, 0, 0, 0, F|10 };
static const uint8_t anim_crawlturn_tr[] = { 2, 5,6,255,  0, 0, 0, F|10, 0, 0, 0, F|8  };
static const uint8_t anim_crawlturn_br[] = { 2, 6,7,255,  0, 0, 0, F|8,  0, 0, 0, _|8  };
static const uint8_t anim_crawlturn_bl[] = { 2, 7,4,255,  0, 0, 0, _|8,  0, 0, 0, _|10 };

#undef _
#undef F

/**
 * $CDF2: Animation states.
 */
const uint8_t *animations[24] =
{
  // Comments here record which animation is selected by byte_CDAA.
  // [facing direction:user input]
  //

  // Normal animations, move the map
  anim_walk_tl, //  0 [TL:Up,    TL:Up+Left]
  anim_walk_tr, //  1 [TR:Right, TR:Up+Right]
  anim_walk_br, //  2 [BR:Down,  BR:Down+Right]
  anim_walk_bl, //  3 [BL:Left,  BL:Down+Left]

  // Normal + turning
  anim_turn_tl, //  4 [TL:Down, TL:Right,     TL:Up+Right,  TL:Down+Right, TR:Up,    TR:Up+Left]
  anim_turn_tr, //  5 [TR:Down, TR:Left,      TR:Down+Left, TR:Down+Right, BR:Up,    BR:Up+Left, BR:Right, BR:Up+Right]
  anim_turn_br, //  6 [BR:Left, BR:Down+Left, BL:Down,      BL:Down+Right]
  anim_turn_bl, //  7 [TL:Left, TL:Down+Left, BL:Up,        BL:Up+Left,    BL:Right, BL:Up+Right]

  // Standing still
  anim_wait_tl, //  8 [TL:None]
  anim_wait_tr, //  9 [TR:None]
  anim_wait_br, // 10 [BR:None]
  anim_wait_bl, // 11 [BL:None]

  // Note that when crawling the hero won't spin around in the tunnel. He'll
  // retain his orientation until specficially spun around using the controls
  // opposite to the direction of the tunnel.

  // Crawling (map movement is enabled which is odd... we never move the map when crawling)
  anim_crawl_tl, // 12 [TL+crawl:Up,   TL+crawl:Down,      TL+crawl:Up+Left, TL+crawl:Down+Right]
  anim_crawl_tr, // 13 [TR+crawl:Left, TR+crawl:Right,     TR+crawl:Up+Right]
  anim_crawl_br, // 14 [BR+crawl:Up,   BR+crawl:Down,      BR+crawl:Up+Left, BR+crawl:Down+Left, BR+crawl:Down+Right]
  anim_crawl_bl, // 15 [BL+crawl:Left, BL+crawl:Down+Left, BL+crawl:Right,   BL+crawl:Up+Right]

  // Crawling + turning
  anim_crawlturn_tl, // 16 [TL+crawl:Right, TL+crawl:Up+Right,   TR+crawl:Up,    TR+crawl:Up+Left]   // turn to face TR
  anim_crawlturn_tr, // 17 [TR+crawl:Down,  TR+crawl:Down+Right, BR+crawl:Right, BR+crawl:Up+Right]  // turn to face BR
  anim_crawlturn_br, // 18 [BR+crawl:Left,  BL+crawl:Down,       BL+crawl:Down+Right]                // turn to face BL
  anim_crawlturn_bl, // 19 [TL+crawl:Left,  TL+crawl:Down+Left,  BL+crawl:Up,    BL+crawl:Up+Left]   // turn to face TL

  // Crawling still
  anim_crawlwait_tl, // 20 [TL+crawl:None]
  anim_crawlwait_tr, // 21 [TR+crawl:None, TR+crawl:Down+Left] // looks like it ought to be 18, not 21
  anim_crawlwait_br, // 22 [BR+crawl:None]
  anim_crawlwait_bl, // 23 [BL+crawl:None]
};

/* ----------------------------------------------------------------------- */

/**
 * $DB9E: Mark nearby items.
 *
 * Iterates over item structs, testing to see if each item is within range
 * (-1..22, 0..15) of the map position.
 *
 * This is similar to is_item_discoverable_interior in that it iterates over
 * all item_structs.
 *
 * \param[in] state Pointer to game state.
 */
void mark_nearby_items(tgestate_t *state)
{
  room_t        room;       /* was C */
  xy_t          map_xy;     /* was D, E */
  uint8_t       iters;      /* was B */
  itemstruct_t *itemstruct; /* was HL */

  assert(state != NULL);

  room = state->room_index;
  if (room == room_NONE)
    room = room_0_OUTDOORS;

  map_xy = state->map_position;

  iters      = item__LIMIT;
  itemstruct = &state->item_structs[0];
  do
  {
    xy_t screenpos = itemstruct->screenpos; /* new */

    /* Conv: Ranges adjusted. */
    // todo: 25, 17 need updating to be state->tb_columns etc.
    if ((itemstruct->room_and_flags & itemstruct_ROOM_MASK) == room &&
        (screenpos.x >= map_xy.x - 1 && screenpos.x <= map_xy.x + 25 - 1) &&
        (screenpos.y >= map_xy.y     && screenpos.y <= map_xy.y + 17    ))
      itemstruct->room_and_flags |= itemstruct_ROOM_FLAG_NEARBY_6 | itemstruct_ROOM_FLAG_NEARBY_7; /* set */
    else
      itemstruct->room_and_flags &= ~(itemstruct_ROOM_FLAG_NEARBY_6 | itemstruct_ROOM_FLAG_NEARBY_7); /* reset */

    itemstruct++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $DBEB: Iterates over all item_structs looking for nearby items.
 *
 * Returns the furthest/highest/nearest/ item? Next frontmost?
 *
 * Leaf.
 *
 * \param[in]  state         Pointer to game state.
 * \param[in]  item_and_flag Initial item_and_flag, passed through if no itemstruct is found. (was A')
 * \param[in]  x             X pos? Compared to X. (was BC') // sampled BCdash = $26, $3E, $00, $26, $34, $22, $32
 * \param[in]  y             Y pos? Compared to Y. (was DE')
 * \param[out] pitemstr      Returned pointer to item struct. (was IY)
 *
 * \return item+flag. (was A')
 */
uint8_t get_greatest_itemstruct(tgestate_t    *state,
                                item_t         item_and_flag,
                                uint16_t       x,
                                uint16_t       y,
                                itemstruct_t **pitemstr)
{
  uint8_t             iters;   /* was B */
  const itemstruct_t *itemstr; /* was HL */

  assert(state    != NULL);
  // assert(item_and_flag);
  // assert(x);
  // assert(y);
  assert(pitemstr != NULL);

  *pitemstr = NULL; /* Conv: Added safety initialisation. */

  iters   = item__LIMIT;
  itemstr = &state->item_structs[0]; /* Conv: Original pointed to itemstruct->room_and_flags. */
  do
  {
    const enum itemstruct_flags FLAGS = itemstruct_ROOM_FLAG_NEARBY_6 |
                                        itemstruct_ROOM_FLAG_NEARBY_7;

    if ((itemstr->room_and_flags & FLAGS) == FLAGS)
    {
      /* Conv: Original calls out to multiply by 8, HLdash is temp. */
      if (itemstr->pos.x * 8 > x &&
          itemstr->pos.y * 8 > y)
      {
        const tinypos_t *pos; /* was HL' */

        pos = &itemstr->pos; /* Conv: Was direct pointer, now points to member. */
        /* Get x,y for the next iteration. */
        y = pos->y * 8;
        x = pos->x * 8;
        *pitemstr = (itemstruct_t *) itemstr;

        state->IY = (vischar_t *) itemstr; // FIXME: Cast is a bodge. // Odd!

        /* The original code has an unpaired A register exchange here. If the
         * loop continues then it's unclear which output register is used. */
        /* It seems that A' is the output register, irrespective. */
        item_and_flag = (item__LIMIT - iters) | item_FOUND; // iteration count + 'item found' flag
      }
    }
    itemstr++;
  }
  while (--iters);

  return item_and_flag;
}

/* ----------------------------------------------------------------------- */

/**
 * $DC41: Set up item plotting.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] itemstr Pointer to itemstruct. (was IY)
 * \param[in] item    Item. (was A)
 *
 * \return 0 => vischar is invisible, 1 => vischar is visible (was Z?)
 */
uint8_t setup_item_plotting(tgestate_t   *state,
                            itemstruct_t *itemstr,
                            item_t        item)
{
//  xy_t         *HL; /* was HL */
//  tinypos_t    *DE; /* was DE */
//  uint16_t      BC; /* was BC */
  uint16_t      clipped_width;  /* was BC */
  uint16_t      clipped_height; /* was DE */
  uint8_t       instr;          /* was A */
  uint8_t       A;              /* was A */
  uint8_t       offset_tmp;     /* was A' */
  uint8_t       iters;          /* was B' */
  uint8_t       offset;         /* was C' */
//  uint16_t      DEdash; /* was DE' */
  const size_t *enables;        /* was HL' */
  int16_t       x, y;           // was HL, DE // signed needed for X calc
  uint8_t      *maskbuf;        /* was HL */
  uint16_t      skip;           /* was DE */

  assert(state   != NULL);
  assert(itemstr != NULL); // will need ASSERT_ITEMSTRUCT_VALID

  /* 0x3F looks like it ought to be 0x1F (item__LIMIT - 1). Potential bug: The use of A later on does not re-clamp it to 0x1F. */
  item &= 0x3F; // mask off item_FOUND
  ASSERT_ITEM_VALID(item);

  /* Bug: The original game writes to this location but it's never
   * subsequently read from.
   */
  /* $8213 = A; */

  state->tinypos_stash = itemstr->pos;
  state->screenpos     = itemstr->screenpos;
// after LDIR we have:
//  HL = &IY->pos.x + 5;
//  DE = &state->tinypos_stash + 5;
//  BC = 0;
//
//  // EX DE, HL
//
//  *DE = 0; // was B, which is always zero here
  state->sprite_index   = 0; /* Items are never drawn flipped. */

  state->item_height    = item_definitions[item].height;
  state->bitmap_pointer = item_definitions[item].bitmap;
  state->mask_pointer   = item_definitions[item].mask;
  if (item_visible(state, &clipped_width, &clipped_height) != 0)
    return 0; /* invisible */

  // PUSH clipped_width
  // PUSH clipped_height

  state->self_E2C2 = clipped_height & 0xFF; // self modify masked_sprite_plotter_16_wide_left

  if ((clipped_width >> 8) == 0)
  {
    instr = 119; /* opcode of 'LD (HL),A' */
    offset_tmp = clipped_width & 0xFF;
  }
  else
  {
    instr = 0; /* opcode of 'NOP' */
    offset_tmp = 3 - (clipped_width & 0xFF);
  }

  offset = offset_tmp; /* Future: Could assign directly to offset instead. */

  /* Set the addresses in the jump table to NOP or LD (HL),A */
  enables = &masked_sprite_plotter_16_enables[0];
  iters = 3; /* iterations */
  do
  {
    *(((uint8_t *) state) + *enables++) = instr;
    *(((uint8_t *) state) + *enables++) = instr;
    if (--offset == 0)
      instr ^= 119; /* Toggle between LD (HL),A and NOP. */
  }
  while (--iters);

  y = 0; /* Conv: Moved. */
  if ((clipped_height >> 8) == 0)
    y = (state->screenpos.y - state->map_position.y) * 24 * 8; // temp: HL

  // state->screenpos.y - state->map_position.y never seems to exceed $10 in the original game, but does for me
  // state->screenpos.y seems to match, but state->map_position.y is 2 bigger

  // Conv: Assigns to signed var.
  x = state->screenpos.x - state->map_position.x; // X axis

  state->window_buf_pointer = &state->window_buf[x + y]; // window buffer start address
  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer);
  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer + clipped_height * state->columns - 1);

  maskbuf = &state->mask_buffer[0];

  // POP DE  // get clipped_height
  // PUSH DE

  state->foreground_mask_pointer = &maskbuf[(clipped_height >> 8) * 4];
  ASSERT_MASK_BUF_PTR_VALID(state->foreground_mask_pointer);

  // POP DE  // this pop/push seems to be needless - DE already has the value
  // PUSH DE

  A = clipped_height >> 8; // A = D // sign?
  if (A)
  {
    /* The original game has a generic multiply loop here which is setup to
     * only ever multiply by two. A duplicate of the equivalent portion in
     * setup_vischar_plotting. In this version instead we'll just multiply by
     * two.
     *
     * {
     *   uint8_t D, E;
     *
     *   D = A;
     *   A = 0;
     *   E = 3; // width_bytes
     *   E--;   // width_bytes - 1
     *   do
     *     A += E;
     *   while (--D);
     * }
     */

    A *= 2;
    // clipped_height &= 0x00FF; // D = 0
  }

  /* D ends up as zero either way. */

  skip = A;
  state->bitmap_pointer += skip;
  state->mask_pointer   += skip;

  // POP BC
  // POP DE

  return 1; /* visible */
}

/* ----------------------------------------------------------------------- */

/**
 * $DD02: Clipping items to the game window.
 *
 * Counterpart to \ref vischar_visible.
 *
 * Called by setup_item_plotting only.
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0 => visible, 1 => invisible (was A)
 */
uint8_t item_visible(tgestate_t *state,
                     uint16_t   *clipped_width,
                     uint16_t   *clipped_height)
{
  const uint8_t *px;            /* was HL */
  xy_t           map_position;  /* was DE */
  int8_t         amount_visible_right; /* was A */
  int8_t         Y1;            /* was A */
  uint16_t       width;         /* was BC */
  uint16_t       height;        /* was DE */

  assert(state          != NULL);
  assert(clipped_width  != NULL);
  assert(clipped_height != NULL);

  /* Width part */

  // Seems to be doing (map_position_x + state->columns <= px[0])
  px = &state->screenpos.x;
  map_position = state->map_position;

  amount_visible_right = map_position.x + state->columns - px[0]; // columns was 24
  if (amount_visible_right <= 0)
    goto invisible;

#define WIDTH_BYTES 3
  if (amount_visible_right < WIDTH_BYTES)
  {
    width = amount_visible_right;
  }
  else
  {
    int8_t amount_visible_left; /* was A */

    amount_visible_left = px[0] + WIDTH_BYTES - map_position.x;
    if (amount_visible_left <= 0)
      goto invisible; // off the left hand side?

    if (amount_visible_left < WIDTH_BYTES)
      width = ((WIDTH_BYTES - amount_visible_left) << 8) | amount_visible_left;
    else
      width = WIDTH_BYTES;
  }

  /* Height part */

  Y1 = map_position.y + state->rows - px[1]; // px[1] == map_position_related.y
  if (Y1 <= 0 || Y1 >= 256)
    goto invisible;

#define HEIGHT 2 // in UDGs
  if (Y1 < HEIGHT)
  {
    height = 8;
  }
  else
  {
    Y1 = px[1] + HEIGHT - map_position.y;
    if (Y1 <= 0)
      goto invisible;

    if (Y1 < HEIGHT)
      height = (8 << 8) | (state->item_height - 8); // looks odd
    else
      height = state->item_height;
  }

  *clipped_width  = width;
  *clipped_height = height;

  return 0; /* item is visible */


invisible:
  return 1; /* item is not visible */
}

/* ----------------------------------------------------------------------- */

/**
 * $DD7D: Item definitions.
 *
 * Used by draw_item and setup_item_plotting.
 */
const spritedef_t item_definitions[item__LIMIT] =
{
  { 2, 11, bitmap_wiresnips, mask_wiresnips },
  { 2, 13, bitmap_shovel,    mask_shovelkey },
  { 2, 16, bitmap_lockpick,  mask_lockpick  },
  { 2, 15, bitmap_papers,    mask_papers    },
  { 2, 12, bitmap_torch,     mask_torch     },
  { 2, 13, bitmap_bribe,     mask_bribe     },
  { 2, 16, bitmap_uniform,   mask_uniform   },
  { 2, 16, bitmap_food,      mask_food      },
  { 2, 16, bitmap_poison,    mask_poison    },
  { 2, 13, bitmap_key,       mask_shovelkey },
  { 2, 13, bitmap_key,       mask_shovelkey },
  { 2, 13, bitmap_key,       mask_shovelkey },
  { 2, 16, bitmap_parcel,    mask_parcel    },
  { 2, 16, bitmap_radio,     mask_radio     },
  { 2, 12, bitmap_purse,     mask_purse     },
  { 2, 12, bitmap_compass,   mask_compass   },
};

/* ----------------------------------------------------------------------- */

/**
 * $E0E0: Addresses of self-modified locations which are changed between NOPs
 * and LD (HL),A.
 *
 * Used by setup_item_plotting and setup_vischar_plotting. [setup_item_plotting: items are always 16 wide].
 */
const size_t masked_sprite_plotter_16_enables[2 * 3] =
{
  offsetof(tgestate_t, enable_E319),
  offsetof(tgestate_t, enable_E3C5),
  offsetof(tgestate_t, enable_E32A),
  offsetof(tgestate_t, enable_E3D6),
  offsetof(tgestate_t, enable_E340),
  offsetof(tgestate_t, enable_E3EC),
};

/* ----------------------------------------------------------------------- */

#define MASK(bm,mask) ((~*foremaskptr | (mask)) & *screenptr) | ((bm) & *foremaskptr)

/**
 * $E102: Sprite plotter for 24-pixel-wide sprites. Used for characters and
 * objects.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void masked_sprite_plotter_24_wide_vischar(tgestate_t *state, vischar_t *vischar)
{
  uint8_t        x;           /* was A */
  uint8_t        iters;       /* was B */
  const uint8_t *maskptr;     /* was ? */
  const uint8_t *bitmapptr;   /* was ? */
  const uint8_t *foremaskptr; /* was ? */
  uint8_t       *screenptr;   /* was ? */

  assert(state   != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  if ((x = (vischar->floogle.x & 7)) < 4)
  {
    uint8_t self_E161, self_E143;

    /* Shift right? */

    x = (~x & 3) * 8; // jump table offset (on input, A is 0..3)

    self_E161 = x; // self-modify: jump into mask rotate
    self_E143 = x; // self-modify: jump into bitmap rotate

    maskptr   = state->mask_pointer;
    bitmapptr = state->bitmap_pointer;

    assert(maskptr   != NULL);
    assert(bitmapptr != NULL);

    iters = state->self_E121; // clipped_height & 0xFF
    assert(iters <= MASK_BUFFER_HEIGHT * 8);
    do
    {
      uint8_t bm0, bm1, bm2, bm3;         // was B, C, E, D
      uint8_t mask0, mask1, mask2, mask3; // was B', C', E', D'
      int     carry = 0;

      /* Load bitmap bytes into B,C,E. */
      bm0 = *bitmapptr++;
      bm1 = *bitmapptr++;
      bm2 = *bitmapptr++;

      /* Load mask bytes into B',C',E'. */
      mask0 = *maskptr++;
      mask1 = *maskptr++;
      mask2 = *maskptr++;

      if (state->sprite_index & sprite_FLAG_FLIP)
        flip_24_masked_pixels(state, &mask2, &mask1, &mask0, &bm2, &bm1, &bm0);

      foremaskptr = state->foreground_mask_pointer;
      screenptr   = state->window_buf_pointer; // moved compared to the other routines

      ASSERT_MASK_BUF_PTR_VALID(foremaskptr);
      ASSERT_WINDOW_BUF_PTR_VALID(screenptr);

      /* Shift bitmap. */

      bm3 = 0;
      // Conv: Replaced self-modified goto with if-else chain.
      if (self_E143 <= 0)
      {
        SRL(bm0);
        RR(bm1);
        RR(bm2);
        RR(bm3);
      }
      if (self_E143 <= 8)
      {
        SRL(bm0);
        RR(bm1);
        RR(bm2);
        RR(bm3);
      }
      if (self_E143 <= 16)
      {
        SRL(bm0);
        RR(bm1);
        RR(bm2);
        RR(bm3);
      }

      /* Shift mask. */

      mask3 = 0xFF;
      carry = 1;
      // Conv: Replaced self-modified goto with if-else chain.
      if (self_E161 <= 0)
      {
        RR(mask0);
        RR(mask1);
        RR(mask2);
        RR(mask3);
      }
      if (self_E161 <= 8)
      {
        RR(mask0);
        RR(mask1);
        RR(mask2);
        RR(mask3);
      }
      if (self_E161 <= 16)
      {
        RR(mask0);
        RR(mask1);
        RR(mask2);
        RR(mask3);
      }

      /* Plot, using foreground mask. */

      x = MASK(bm0, mask0);
      foremaskptr++;
      if (state->enable_E188)
        *screenptr = x;
      screenptr++;

      x = MASK(bm1, mask1);
      foremaskptr++;
      if (state->enable_E199)
        *screenptr = x;
      screenptr++;

      x = MASK(bm2, mask2);
      foremaskptr++;
      if (state->enable_E1AA)
        *screenptr = x;
      screenptr++;

      x = MASK(bm3, mask3);
      foremaskptr++;
      ASSERT_MASK_BUF_PTR_VALID(foremaskptr);
      state->foreground_mask_pointer = foremaskptr;
      if (state->enable_E1BF)
        *screenptr = x;

      screenptr += state->columns - 3; // was 21
      ASSERT_WINDOW_BUF_PTR_VALID(screenptr);
      state->window_buf_pointer = screenptr;
    }
    while (--iters);
  }
  else
  {
    uint8_t self_E22A, self_E204;

    /* Shift left? */

    x -= 4; // (on input, A is 4..7)
    x = (x << 3) | (x >> 5); // was 3 x RLCA

    self_E22A = x; // self-modify: jump into mask rotate
    self_E204 = x; // self-modify: jump into bitmap rotate

    maskptr   = state->mask_pointer;
    bitmapptr = state->bitmap_pointer;

    assert(maskptr   != NULL);
    assert(bitmapptr != NULL);

    iters = state->self_E1E2; // clipped_height & 0xFF
    assert(iters <= MASK_BUFFER_HEIGHT * 8);
    do
    {
      /* Note the different variable order to the case above. */
      uint8_t bm0, bm1, bm2, bm3;         // was E, C, B, D
      uint8_t mask0, mask1, mask2, mask3; // was E', C', B', D'
      int     carry = 0;

      /* Load bitmap bytes into B,C,E. */
      bm2 = *bitmapptr++;
      bm1 = *bitmapptr++;
      bm0 = *bitmapptr++;

      /* Load mask bytes into B',C',E'. */
      mask2 = *maskptr++;
      mask1 = *maskptr++;
      mask0 = *maskptr++;

      if (state->sprite_index & sprite_FLAG_FLIP)
        flip_24_masked_pixels(state, &mask0, &mask1, &mask2, &bm0, &bm1, &bm2);

      foremaskptr = state->foreground_mask_pointer;
      screenptr   = state->window_buf_pointer;

      ASSERT_MASK_BUF_PTR_VALID(foremaskptr);
      ASSERT_WINDOW_BUF_PTR_VALID(screenptr);

      /* Shift bitmap. */

      bm3 = 0;
      // Conv: Replaced self-modified goto with if-else chain.
      if (self_E204 <= 0)
      {
        SLA(bm0);
        RL(bm1);
        RL(bm2);
        RL(bm3);
      }
      if (self_E204 <= 8)
      {
        SLA(bm0);
        RL(bm1);
        RL(bm2);
        RL(bm3);
      }
      if (self_E204 <= 16)
      {
        SLA(bm0);
        RL(bm1);
        RL(bm2);
        RL(bm3);
      }
      if (self_E204 <= 24)
      {
        SLA(bm0);
        RL(bm1);
        RL(bm2);
        RL(bm3);
      }

      /* Shift mask. */

      mask3 = 0xFF;
      carry = 1;
      // Conv: Replaced self-modified goto with if-else chain.
      if (self_E22A <= 0)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }
      if (self_E22A <= 8)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }
      if (self_E22A <= 16)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }
      if (self_E22A <= 24)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }

      /* Plot, using foreground mask. */

      x = MASK(bm3, mask3);
      foremaskptr++;
      if (state->enable_E259)
        *screenptr = x;
      screenptr++;

      x = MASK(bm2, mask2);
      foremaskptr++;
      if (state->enable_E26A)
        *screenptr = x;
      screenptr++;

      x = MASK(bm1, mask1);
      foremaskptr++;
      if (state->enable_E27B)
        *screenptr = x;
      screenptr++;

      x = MASK(bm0, mask0);
      foremaskptr++;
      state->foreground_mask_pointer = foremaskptr;
      if (state->enable_E290)
        *screenptr = x;

      screenptr += state->columns - 3; // was 21
      ASSERT_WINDOW_BUF_PTR_VALID(screenptr);
      state->window_buf_pointer = screenptr;
    }
    while (--iters);
  }
}

/**
 * $E29F: Sprite plotter entry point for items only.
 *
 * \param[in] state Pointer to game state.
 */
void masked_sprite_plotter_16_wide_item(tgestate_t *state)
{
  masked_sprite_plotter_16_wide_left(state, 0 /* x */);
}

/**
 * $E2A2: Sprite plotter entry point for vischars only.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void masked_sprite_plotter_16_wide_vischar(tgestate_t *state, vischar_t *vischar)
{
  uint8_t x;

  ASSERT_VISCHAR_VALID(vischar);

  x = vischar->floogle.x & 7;
  if (x < 4)
    masked_sprite_plotter_16_wide_left(state, x); /* was fallthrough */
  else
    masked_sprite_plotter_16_wide_right(state, x);
}

/**
 * $E2AC: Sprite plotter. Shifts left/right (unsure).
 *
 * \param[in] state Pointer to game state.
 * \param[in] x     X offset. (was A)
 */
void masked_sprite_plotter_16_wide_left(tgestate_t *state, uint8_t x)
{
  uint8_t        iters;       /* was B */
  const uint8_t *maskptr;     /* was ? */
  const uint8_t *bitmapptr;   /* was ? */
  const uint8_t *foremaskptr; /* was ? */
  uint8_t       *screenptr;   /* was ? */
  uint8_t        self_E2DC, self_E2F4;

  assert(state != NULL);
  // assert(x);

  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer);
  ASSERT_MASK_BUF_PTR_VALID(state->foreground_mask_pointer);

  x = (~x & 3) * 6; // jump table offset (on input, A is 0..3 => 3..0) // 6 = length of asm chunk

  self_E2DC = x; // self-modify: jump into mask rotate
  self_E2F4 = x; // self-modify: jump into bitmap rotate

  maskptr   = state->mask_pointer;
  bitmapptr = state->bitmap_pointer;

  assert(maskptr   != NULL);
  assert(bitmapptr != NULL);

  iters = state->self_E2C2; // (clipped height & 0xFF) // self modified by $E49D (setup_vischar_plotting)
  assert(iters <= MASK_BUFFER_HEIGHT * 8);
  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer + iters * state->columns - 1);
  do
  {
    uint8_t bm0, bm1, bm2;       // was D, E, C
    uint8_t mask0, mask1, mask2; // was D', E', C'
    int     carry = 0;

    /* Load bitmap bytes into D,E. */
    bm0 = *bitmapptr++;
    bm1 = *bitmapptr++;

    /* Load mask bytes into D',E'. */
    mask0 = *maskptr++;
    mask1 = *maskptr++;

    if (state->sprite_index & sprite_FLAG_FLIP)
      flip_16_masked_pixels(state, &mask0, &mask1, &bm0, &bm1);

    // I'm assuming foremaskptr to be a foreground mask pointer based on it being
    // incremented by four each step, like a supertile wide thing.
    foremaskptr = state->foreground_mask_pointer;
    ASSERT_MASK_BUF_PTR_VALID(foremaskptr);

    // 24 version does bitmap rotates then mask rotates.
    // Is this the opposite way around to save a bank switch?

    /* Shift mask. */

    mask2 = 0xFF; // all bits set => mask OFF (that would match the observed stored mask format)
    carry = 1; // mask OFF
    // Conv: Replaced self-modified goto with if-else chain.
    if (self_E2DC <= 0)
    {
      RR(mask0);
      RR(mask1);
      RR(mask2);
    }
    if (self_E2DC <= 6)
    {
      RR(mask0);
      RR(mask1);
      RR(mask2);
    }
    if (self_E2DC <= 12)
    {
      RR(mask0);
      RR(mask1);
      RR(mask2);
    }

    /* Shift bitmap. */

    bm2 = 0; // all bits clear => pixels OFF
    //carry = 0; in original code but never read in practice
    // Conv: Replaced self-modified goto with if-else chain.
    if (self_E2F4 <= 0)
    {
      SRL(bm0);
      RR(bm1);
      RR(bm2);
    }
    if (self_E2F4 <= 6)
    {
      SRL(bm0);
      RR(bm1);
      RR(bm2);
    }
    if (self_E2F4 <= 12)
    {
      SRL(bm0);
      RR(bm1);
      RR(bm2);
    }

    /* Plot, using foreground mask. */

    screenptr = state->window_buf_pointer; // moved relative to the 24 version
    ASSERT_WINDOW_BUF_PTR_VALID(screenptr);
    ASSERT_MASK_BUF_PTR_VALID(foremaskptr);

    x = MASK(bm0, mask0);
    foremaskptr++;
    if (state->enable_E319)
      *screenptr = x;
    screenptr++;

    x = MASK(bm1, mask1);
    foremaskptr++;
    if (state->enable_E32A)
      *screenptr = x;
    screenptr++;

    x = MASK(bm2, mask2);
    foremaskptr += 2;
    state->foreground_mask_pointer = foremaskptr;
    if (state->enable_E340)
      *screenptr = x;

    screenptr += state->columns - 2; // was 22
    if (iters > 1)
      ASSERT_WINDOW_BUF_PTR_VALID(screenptr);
    state->window_buf_pointer = screenptr;
  }
  while (--iters);
}

/**
 * $E34E: Sprite plotter. Shifts left/right (unsure). Used for characters and objects. Counterpart of above routine.
 *
 * Only called by masked_sprite_plotter_16_wide_vischar.
 *
 * \param[in] state Pointer to game state.
 * \param[in] x     X offset. (was A)
 */
void masked_sprite_plotter_16_wide_right(tgestate_t *state, uint8_t x)
{
  uint8_t        iters;       /* was B */
  const uint8_t *maskptr;     /* was ? */
  const uint8_t *bitmapptr;   /* was ? */
  const uint8_t *foremaskptr; /* was ? */
  uint8_t       *screenptr;   /* was ? */
  uint8_t        self_E39A, self_E37D;

  assert(state != NULL);
  // assert(x);

  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer);
  ASSERT_MASK_BUF_PTR_VALID(state->foreground_mask_pointer);

  x = (x - 4) * 6; // jump table offset (on input, 'x' is 4..7 => 0..3) // 6 = length of asm chunk

  self_E39A = x; // self-modify: jump into bitmap rotate
  self_E37D = x; // self-modify: jump into mask rotate

  maskptr   = state->mask_pointer;
  bitmapptr = state->bitmap_pointer;

  assert(maskptr   != NULL);
  assert(bitmapptr != NULL);

  iters = state->self_E363; // (clipped height & 0xFF) // self modified by $E49D (setup_vischar_plotting)
  assert(iters <= MASK_BUFFER_HEIGHT * 8);
  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer + iters * state->columns - 1);
  do
  {
    /* Note the different variable order to the 'left' case above. */
    uint8_t bm0, bm1, bm2;       // was E, D, C
    uint8_t mask0, mask1, mask2; // was E', D', C'
    int     carry = 0;

    /* Load bitmap bytes into D,E. */
    bm1 = *bitmapptr++;
    bm0 = *bitmapptr++;

    /* Load mask bytes into D',E'. */
    mask1 = *maskptr++;
    mask0 = *maskptr++;

    if (state->sprite_index & sprite_FLAG_FLIP)
      flip_16_masked_pixels(state, &mask1, &mask0, &bm1, &bm0);

    foremaskptr = state->foreground_mask_pointer;
    ASSERT_MASK_BUF_PTR_VALID(foremaskptr);

    /* Shift mask. */

    mask2 = 0xFF; // all bits set => mask OFF (that would match the observed stored mask format)
    carry = 1; // mask OFF
    // Conv: Replaced self-modified goto with if-else chain.
    if (self_E39A <= 0)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }
    if (self_E39A <= 6)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }
    if (self_E39A <= 12)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }
    if (self_E39A <= 18)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }

    /* Shift bitmap. */

    bm2 = 0; // all bits clear => pixels OFF
    // no carry reset in this variant?
    if (self_E37D <= 0)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }
    if (self_E37D <= 6)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }
    if (self_E37D <= 12)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }
    if (self_E37D <= 18)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }

    /* Plot, using foreground mask. */

    screenptr = state->window_buf_pointer; // this line is moved relative to the 24 version
    ASSERT_WINDOW_BUF_PTR_VALID(screenptr);

    x = MASK(bm2, mask2); // 'x' should probably be another variable here
    foremaskptr++;
    if (state->enable_E3C5)
      *screenptr = x;
    screenptr++;

    x = MASK(bm1, mask1);
    foremaskptr++;
    if (state->enable_E3D6)
      *screenptr = x;
    screenptr++;

    x = MASK(bm0, mask0);
    foremaskptr += 2;
    state->foreground_mask_pointer = foremaskptr;
    if (state->enable_E3EC)
      *screenptr = x;

    screenptr += state->columns - 2; // was 22
    if (iters > 1)
      ASSERT_WINDOW_BUF_PTR_VALID(screenptr);
    state->window_buf_pointer = screenptr;
  }
  while (--iters);
}

/**
 * $E3FA: Reverses the 24 pixels in E,C,B and E',C',B'.
 *
 * \param[in,out] state  Pointer to game state.
 * \param[in,out] pE     Pointer to pixels/mask.
 * \param[in,out] pC     Pointer to pixels/mask.
 * \param[in,out] pB     Pointer to pixels/mask.
 * \param[in,out] pEdash Pointer to pixels/mask.
 * \param[in,out] pCdash Pointer to pixels/mask.
 * \param[in,out] pBdash Pointer to pixels/mask.
 */
void flip_24_masked_pixels(tgestate_t *state,
                           uint8_t    *pE,
                           uint8_t    *pC,
                           uint8_t    *pB,
                           uint8_t    *pEdash,
                           uint8_t    *pCdash,
                           uint8_t    *pBdash)
{
  const uint8_t *HL;
  uint8_t        E, C, B;

  assert(state  != NULL);
  assert(pE     != NULL);
  assert(pC     != NULL);
  assert(pB     != NULL);
  assert(pEdash != NULL);
  assert(pCdash != NULL);
  assert(pBdash != NULL);

  /* Conv: Routine was much simplified over the original code. */

  HL = &state->reversed[0];

  B = HL[*pE];
  E = HL[*pB];
  C = HL[*pC];

  *pB = B;
  *pE = E;
  *pC = C;

  B = HL[*pEdash];
  E = HL[*pBdash];
  C = HL[*pCdash];

  *pBdash = B;
  *pEdash = E;
  *pCdash = C;
}

/**
 * $E40F: Reverses the 16 pixels in D,E and D',E'.
 *
 * \param[in,out] state  Pointer to game state.
 * \param[in,out] pD     Pointer to pixels/mask.
 * \param[in,out] pE     Pointer to pixels/mask.
 * \param[in,out] pDdash Pointer to pixels/mask.
 * \param[in,out] pEdash Pointer to pixels/mask.
 */
void flip_16_masked_pixels(tgestate_t *state,
                           uint8_t    *pD,
                           uint8_t    *pE,
                           uint8_t    *pDdash,
                           uint8_t    *pEdash)
{
  const uint8_t *HL;
  uint8_t        D, E;

  assert(state  != NULL);
  assert(pD     != NULL);
  assert(pE     != NULL);
  assert(pDdash != NULL);
  assert(pEdash != NULL);

  HL = &state->reversed[0];

  D = HL[*pE];
  E = HL[*pD];

  *pE = E;
  *pD = D;

  D = HL[*pEdash];
  E = HL[*pDdash];

  *pEdash = E;
  *pDdash = D;
}

/**
 * $E420: Set up vischar plotting.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 *
 * \return 0 => vischar is invisible, 1 => vischar is visible (was Z?)
 *
 * HL is passed in too (but disassembly says it's always the same as IY).
 */
int setup_vischar_plotting(tgestate_t *state, vischar_t *vischar)
{
  /**
   * $E0EC: Addresses of self-modified locations which are changed between
   * NOPs and LD (HL),A.
   */
  static const size_t masked_sprite_plotter_24_enables[4 * 2] =
  {
    offsetof(tgestate_t, enable_E188),
    offsetof(tgestate_t, enable_E259),
    offsetof(tgestate_t, enable_E199),
    offsetof(tgestate_t, enable_E26A),
    offsetof(tgestate_t, enable_E1AA),
    offsetof(tgestate_t, enable_E27B),
    offsetof(tgestate_t, enable_E1BF),
    offsetof(tgestate_t, enable_E290),

    /* These two addresses are present here in the original game but are
     * unreferenced. */
    // masked_sprite_plotter_16_wide_vischar
    // masked_sprite_plotter_24_wide_vischar
  };

  pos_t             *pos;            /* was HL */
  tinypos_t         *tinypos;        /* was DE */
  const spritedef_t *sprite;         /* was BC */
  spriteindex_t      sprite_index;   /* was A */
  const spritedef_t *sprite2;        /* was DE */
  uint16_t           clipped_width;  /* was BC */
  uint16_t           clipped_height; /* was DE */
  const size_t      *enables;        /* was HL */
  uint8_t            self_E4C0;      /* was $E4C0 */
  uint8_t            offset;         /* was A' */
  uint8_t            iters;          /* was B' */
  uint8_t            E;              /* was E */
  uint8_t            instr;          /* was A */
  uint8_t            A;              /* was A */
  int16_t            x;              /* was HL */
  uint8_t           *maskbuf;        /* was HL */
  uint16_t           y;              /* was DE */
  uint16_t           skip;           /* was DE */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  pos     = &vischar->mi.pos;
  tinypos = &state->tinypos_stash;

  if (state->room_index > room_0_OUTDOORS)
  {
    /* Indoors. */

    tinypos->x      = pos->x; // note: narrowing
    tinypos->y      = pos->y; // note: narrowing
    tinypos->height = pos->height; // note: narrowing
  }
  else
  {
    /* Outdoors. */

    /* Conv: Unrolled, removed divide-by-8 calls. */
    tinypos->x      = (pos->x + 4 ) >> 3; /* with rounding */
    tinypos->y      = (pos->y     ) >> 3; /* without rounding */
    tinypos->height = (pos->height) >> 3;
  }

  sprite = vischar->mi.sprite;

  state->sprite_index = sprite_index = vischar->mi.sprite_index; // set left/right flip flag / sprite offset

  // HL now points after sprite_index
  // DE now points to state->map_position_related.x

  // unrolled versus original
  state->screenpos.x = vischar->floogle.x >> 3;
  state->screenpos.y = vischar->floogle.y >> 3;
  // 'screenpos' here is now a scaled-down projected map coordinate

  // A is (1<<7) mask OR sprite offset
  // original game uses ADD A,A to double A and in doing so discards top bit, here we mask it off explicitly
  sprite2 = &sprite[sprite_index & ~sprite_FLAG_FLIP]; // sprite pointer

  vischar->width_bytes = sprite2->width;  /* width in bytes + 1 */
  vischar->height      = sprite2->height; /* height in rows */

  state->bitmap_pointer = sprite2->bitmap;
  state->mask_pointer   = sprite2->mask;

  if (vischar_visible(state, vischar, &clipped_width, &clipped_height) != 0) // used A as temporary
    return 0; /* invisible */

  // PUSH clipped_width
  // PUSH clipped_height

  E = clipped_height & 0xFF; // must be no of visible rows?

  if (vischar->width_bytes == 3) // 3 => 16 wide, 4 => 24 wide
  {
    state->self_E2C2 = E; // self modify masked_sprite_plotter_16_wide_left
    state->self_E363 = E; // self-modify masked_sprite_plotter_16_wide_right

    A = 3;
    enables = &masked_sprite_plotter_16_enables[0];
  }
  else
  {
    state->self_E121 = E; // self-modify masked_sprite_plotter_24_wide_vischar (shift right case)
    state->self_E1E2 = E; // self-modify masked_sprite_plotter_24_wide_vischar (shift left case)

    A = 4;
    enables = &masked_sprite_plotter_24_enables[0];
  }

  // PUSH HL

  self_E4C0 = A; // self-modify
  if ((clipped_width & 0xFF00) == 0) // no lefthand skip: start with 'on'
  {
    instr = 119; /* opcode of 'LD (HL),A' */
    offset = clipped_width & 0xFF; // process this many bytes before clipping
  }
  else // lefthand skip present: start with 'off'
  {
    instr = 0; /* opcode of 'NOP' */
    offset = A - (clipped_width & 0xFF); // clip until this many bytes have been processed
  }

  // POP HLdash // entry point

  /* Set the addresses in the jump table to NOP or LD (HL),A. */
  //unused? Cdash = offset; // must be no of columns?
  iters = self_E4C0; /* 3 or 4 iterations */
  do
  {
    *(((uint8_t *) state) + *enables++) = instr;
    *(((uint8_t *) state) + *enables++) = instr;
    if (--offset == 0)
      instr ^= 119; /* Toggle between LD and NOP. */
  }
  while (--iters);

  y = 0; /* Conv: Moved. */
  if ((clipped_height >> 8) == 0) // top skippage
    y = (vischar->floogle.y - (state->map_position.y * 8)) * 24;

  x = state->screenpos.x - state->map_position.x; // signed subtract + extend to 16-bit

  state->window_buf_pointer = &state->window_buf[x + y]; // screen buffer start address
  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer);
  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer + clipped_height * state->columns - 1);

  maskbuf = &state->mask_buffer[0];

  // POP DE  // get clipped_height
  // PUSH DE

  maskbuf += (clipped_height >> 8) * 4 + (vischar->floogle.y & 7) * 4; // i *think* its clipped_height -- check
  ASSERT_MASK_BUF_PTR_VALID(maskbuf);
  state->foreground_mask_pointer = maskbuf;

  // POP DE  // pop clipped_height

  A = clipped_height >> 8; // sign?
  if (A)
  {
    /* Conv: The original game has a generic multiply loop here. In this
     * version instead we'll just multiply. */

    A *= vischar->width_bytes - 1;
    // clipped_height &= 0x00FF; // D = 0
  }

  /* D ends up as zero either way. */

  skip = A;
  state->bitmap_pointer += skip;
  state->mask_pointer   += skip;

  // POP clipped_width  // why save it anyway? if it's just getting popped. unless it's being returned.

  return 1; /* visible */ // Conv: Added.
}

/* ----------------------------------------------------------------------- */

/**
 * $E542: Scale down a pos_t and assign result to a tinypos_t.
 *
 * Divides the three input 16-bit words by 8, with rounding to nearest,
 * storing the result as bytes.
 *
 * \param[in]  in  Input pos_t.      (was HL)
 * \param[out] out Output tinypos_t. (was DE)
 */
void pos_to_tinypos(const pos_t *in, tinypos_t *out)
{
  const uint8_t *pcoordin;  /* was HL */
  uint8_t       *pcoordout; /* was DE */
  uint8_t        iters;     /* was B */

  assert(in  != NULL);
  assert(out != NULL);

  /* Use knowledge of the structure layout to cast the pointers to array[3]
   * of their respective types. */
  pcoordin = (const uint8_t *) &in->x;
  pcoordout = (uint8_t *) &out->x;

  iters = 3;
  do
  {
    uint8_t low, high; /* was A, C */

    low = *pcoordin++;
    high = *pcoordin++;
    divide_by_8_with_rounding(&low, &high);
    *pcoordout++ = low;
  }
  while (--iters);
}

/**
 * $E550: Divide AC by 8, with rounding to nearest.
 *
 * \param[in,out] plow Pointer to low. (was A)
 * \param[in,out] phigh Pointer to high. (was C)
 */
void divide_by_8_with_rounding(uint8_t *plow, uint8_t *phigh)
{
  int t; /* Conv: Added. */

  assert(plow  != NULL);
  assert(phigh != NULL);

  t = *plow + 4; /* Like adding 0.5. */
  *plow = (uint8_t) t; /* Store modulo 256. */
  if (t >= 256)
    (*phigh)++; /* Carry. */

  divide_by_8(plow, phigh);
}

/**
 * $E555: Divide AC by 8.
 *
 * \param[in,out] plow Pointer to low. (was A)
 * \param[in,out] phigh Pointer to high. (was C)
 */
void divide_by_8(uint8_t *plow, uint8_t *phigh)
{
  assert(plow  != NULL);
  assert(phigh != NULL);

  *plow = (*plow >> 3) | (*phigh << 5);
  *phigh >>= 3;
}

/* ----------------------------------------------------------------------- */

/**
 * $EED3: Plot the game screen.
 *
 * \param[in] state Pointer to game state.
 */
void plot_game_window(tgestate_t *state)
{
  assert(state != NULL);

  uint8_t *const  screen = &state->speccy->screen[0];

  uint8_t         y;         /* was A */
  const uint8_t  *src;       /* was HL */
  const uint16_t *offsets;   /* was SP */
  uint8_t         y_iters_A; /* was A */
  uint8_t        *dst;       /* was DE */
  uint8_t         prev;      /* was A */
  uint8_t         y_iters_B; /* was B' */
  uint8_t         iters;     /* was B */
  uint8_t         tmp;       /* Conv: added for RRD macro */

  assert(state->game_window_offset.x == 0 * 24 ||
         state->game_window_offset.x == 2 * 24 ||
         state->game_window_offset.x == 4 * 24 ||
         state->game_window_offset.x == 6 * 24);

  y = state->game_window_offset.y; // might not be a Y value. seems to only ever be 0 or 255.
  assert(y == 0 || y == 255);
  if (y == 0)
  {
    src = &state->window_buf[1] + state->game_window_offset.x;
    ASSERT_WINDOW_BUF_PTR_VALID(src);
    offsets = &state->game_window_start_offsets[0];
    y_iters_A = 128; /* iterations */
    do
    {
      dst = screen + *offsets++;
      ASSERT_SCREEN_PTR_VALID(dst);

      *dst++ = *src++; /* unrolled: 23 copies */
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      src++; // skip 24th
    }
    while (--y_iters_A);
  }
  else
  {
    src = &state->window_buf[0] + state->game_window_offset.x;
    ASSERT_WINDOW_BUF_PTR_VALID(src);
    prev = *src++;
    offsets = &state->game_window_start_offsets[0];
    y_iters_B = 128; /* iterations */
    do
    {
      dst = screen + *offsets++;
      ASSERT_SCREEN_PTR_VALID(dst);

      /* Conv: Unrolling removed compared to original code which did 4 groups of 5 ops, then a final 3. */
      iters = 4 * 5 + 3; /* 23 iterations */
      do
      {
        /* Conv: Original code banks and shuffles registers to preserve stuff. This is simplified. */

        tmp = prev & 0x0F;
        prev = *src; // save pixel so we can use the bottom nibble next time around
        *dst++ = (*src++ >> 4) | (tmp << 4);
      }
      while (--iters);

      prev = *src++;
    }
    while (--y_iters_B);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $EF9A: Event: roll call.
 *
 * \param[in] state Pointer to game state.
 */
void event_roll_call(tgestate_t *state)
{
  uint16_t   range;   /* was DE */ // xy_t?
  uint8_t   *pcoord;  /* was HL */
  uint8_t    iters;   /* was B */
  uint8_t    coord;   /* was A */
  vischar_t *vischar; /* was HL */

  assert(state != NULL);

  /* Is the hero within the roll call area bounds? */
  /* Conv: Unrolled. */
  /* Range checking. X in (114..124) and Y in (106..114). */
  range = map_ROLL_CALL_X;
  pcoord = &state->hero_map_position.x;
  coord = *pcoord++;
  if (coord < ((range >> 8) & 0xFF) || coord >= ((range >> 0) & 0xFF))
    goto not_at_roll_call;

  range = map_ROLL_CALL_Y;
  coord = *pcoord++; // -> state->hero_map_position.y
  if (coord < ((range >> 8) & 0xFF) || coord >= ((range >> 0) & 0xFF))
    goto not_at_roll_call;

  /* All visible characters turn forward. */
  vischar = &state->vischars[0];
  iters = vischars_LENGTH;
  do
  {
    vischar->input = input_KICK;
    vischar->direction = direction_BOTTOM_LEFT;
    vischar++;
  }
  while (--iters);

  return;

not_at_roll_call:
  state->bell = bell_RING_PERPETUAL;
  queue_message_for_display(state, message_MISSED_ROLL_CALL);
  hostiles_persue(state); // exit via
}

/* ----------------------------------------------------------------------- */

/**
 * $EFCB: Use papers.
 *
 * \param[in] state Pointer to game state.
 *
 * \remarks Exits using longjmp if the action takes place.
 */
void action_papers(tgestate_t *state)
{
  /**
   * $EFF9: Position outside the main gate.
   */
  static const tinypos_t outside_main_gate = { 214, 138, 6 };

  uint16_t  range;  /* was DE */
  uint8_t  *pcoord; /* was HL */
  uint8_t   coord;  /* was A */

  assert(state != NULL);

  /* Is the hero within the main gate bounds? */
  // UNROLLED
  /* Range checking. X in (105..109) and Y in (73..75). */
  range = map_MAIN_GATE_X; // note the confused coords business
  pcoord = &state->hero_map_position.x;
  coord = *pcoord++;
  if (coord < ((range >> 8) & 0xFF) || coord >= ((range >> 0) & 0xFF))
    return;

  range = map_MAIN_GATE_Y;
  coord = *pcoord++; // -> state->hero_map_position.y
  if (coord < ((range >> 8) & 0xFF) || coord >= ((range >> 0) & 0xFF))
    return;

  /* Using the papers at the main gate when not in uniform causes the hero
   * to be sent to solitary */
  if (state->vischars[0].mi.sprite != &sprites[sprite_GUARD_FACING_AWAY_1])
  {
    solitary(state);
    return;
  }

  increase_morale_by_10_score_by_50(state);
  state->vischars[0].room = room_0_OUTDOORS;

  /* Transition to outside the main gate. */
  state->IY = &state->vischars[0];
  transition(state, &outside_main_gate);
  NEVER_RETURNS;
}

/* ----------------------------------------------------------------------- */

/**
 * $EFFC: Waits for the user to press Y or N.
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0 if 'Y' pressed, 1 if 'N' pressed.
 */
int user_confirm(tgestate_t *state)
{
  /** $F014 */
  static const screenlocstring_t screenlocstring_confirm_y_or_n =
  {
    0x100B, 15, "CONFIRM. Y OR N"
  };

  uint8_t keymask; /* was A */

  assert(state != NULL);

  screenlocstring_plot(state, &screenlocstring_confirm_y_or_n);

  state->speccy->draw(state->speccy, NULL);

  /* Keyscan. */
  for (;;)
  {
    keymask = state->speccy->in(state->speccy, port_KEYBOARD_POIUY);
    if ((keymask & (1 << 4)) == 0)
      return 0; /* is 'Y' pressed? return Z */

    keymask = state->speccy->in(state->speccy, port_KEYBOARD_SPACESYMSHFTMNB);
    keymask = ~keymask;
    if ((keymask & (1 << 3)) != 0)
      return 1; /* is 'N' pressed? return NZ */

    state->speccy->sleep(state->speccy, sleeptype_KEYSCAN, 0xFFFF);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F163: Setup the game.
 *
 * \param[in] state Pointer to game state.
 */
TGE_API void tge_setup(tgestate_t *state)
{
  /**
   * $F1C9: Initial state of a visible character.
   *
   * The original game stores just 23 bytes of the structure, we store a
   * whole structure here.
   *
   * This can't be a _static_ const structure as it contains references which
   * are not compile time constant.
   */
  const vischar_t vischar_initial =
  {
    0,                    // character
    0,                    // flags
    { 44, 1 },            // route
    { 46, 46, 24 },       // pos
    0,                    // counter_and_flags
    &animations[0],       // animbase
    animations[8],        // anim
    0,                    // animindex
    0,                    // input
    direction_TOP_LEFT,   // direction
    { { 0, 0, 24 }, &sprites[sprite_PRISONER_FACING_AWAY_1], 0 }, // mi
    { 0, 0 },             // scrx, scry
    room_0_OUTDOORS,      // room
    0,                    // unused
    0,                    // width_bytes
    0,                    // height
  };

  uint8_t   *reversed; /* was HL */
  uint8_t    counter;  /* was A */
  uint8_t    byte;     /* was C */
  uint8_t    iters;    /* was B */
  int        carry = 0;
  vischar_t *vischar;  /* was HL */

  assert(state != NULL);

  wipe_full_screen_and_attributes(state);
  set_morale_flag_screen_attributes(state, attribute_BRIGHT_GREEN_OVER_BLACK);
  /* The original code seems to pass in 68, not zero, as it uses a register
   * left over from a previous call to set_morale_flag_screen_attributes(). */
  set_menu_item_attributes(state, 0, attribute_BRIGHT_YELLOW_OVER_BLACK);
  plot_statics_and_menu_text(state);

  plot_score(state);

  menu_screen(state);


  /* Construct a table of 256 bit-reversed bytes in state->reversed[]. */
  reversed = &state->reversed[0];
  do
  {
    /* This was 'A = L;' in original code as alignment was guaranteed. */
    counter = reversed - &state->reversed[0]; /* Get a counter 0..255. */ // note: narrowing

    /* Reverse the counter byte. */
    byte = 0;
    iters = 8;
    do
    {
      carry = counter & 1;
      counter >>= 1;
      byte = (byte << 1) | carry;
    }
    while (--iters);

    *reversed++ = byte;
  }
  while (reversed != &state->reversed[256]); // was ((HL & 0xFF) != 0);

  /* Initialise all visible characters. */
  vischar = &state->vischars[0];
  iters = vischars_LENGTH;
  do
  {
    memcpy(vischar, &vischar_initial, sizeof(vischar_initial));
    vischar++;
  }
  while (--iters);

  /* Write (character_NONE, vischar_FLAGS_EMPTY_SLOT) over all non-visible
   * characters. */
  iters = vischars_LENGTH - 1;
  vischar = &state->vischars[1];
  do
  {
    vischar->character = character_NONE;
    vischar->flags     = vischar_FLAGS_EMPTY_SLOT;
    vischar++;
  }
  while (--iters);


  // Conv: Set up a jmpbuf so the game will return here.
  if (setjmp(state->jmpbuf_main) == 0)
  {
    /* In the original code this wiped all state from $8100 up until the
     * start of tiles ($8218). We'll assume for now that tgestate_t is
     * calloc'd and so zeroed by default. */

    reset_game(state);

    // i reckon this never gets this far, as reset_game jumps to enter_room itself
    //enter_room(state); // returns by goto main_loop
    NEVER_RETURNS;
  }
}

TGE_API void tge_main(tgestate_t *state)
{
  // Conv: Need to get main loop to setjmp so we call it from here.
  if (setjmp(state->jmpbuf_main) == 0)
  {
    main_loop(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F257: Clear the screen and attributes and set the screen border to black.
 *
 * \param[in] state Pointer to game state.
 */
void wipe_full_screen_and_attributes(tgestate_t *state)
{
  assert(state != NULL);
  assert(state->speccy != NULL);

  memset(&state->speccy->screen, 0, SCREEN_LENGTH);
  memset(&state->speccy->attributes,
         attribute_WHITE_OVER_BLACK,
         SCREEN_ATTRIBUTES_LENGTH);

  /* Set the screen border to black. */
  state->speccy->out(state->speccy, port_BORDER, 0);
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et:
