/**
 * Main.c
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

/* TODO
 *
 * - Some enums might be wider than expected (int vs uint8_t).
 */

/* LATER
 *
 * - Move screen memory to a linear format (again, required for
 *   variably-sized screen).
 * - Replace uint8_t counters, and other types which are smaller than int,
 *   with int where possible.
 * - Merge loop elements into the body of the code.
 *
 * Other items are marked "FUTURE:".
 */

/* GLOSSARY
 *
 * - "Conv:"
 *   -- Code which has required adjustment or has otherwise been changed during
 *      the conversion from Z80 into C.
 * - "was <register>/<address>"
 *   -- Records which register, registers or address the original code used to
 *      hold this variable.
 * - "was fallthrough"
 *   -- In the original code there was no call here but instead the code
 *      continued through to the adjacent function.
 * - "was tail call"
 *   -- In the original Z80 a call marked with this would have branched directly
 *      to its target to exit.
 */

/* ----------------------------------------------------------------------- */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "C99/Types.h"

#include "TheGreatEscape/TheGreatEscape.h"

#include "TheGreatEscape/Asserts.h"
#include "TheGreatEscape/Debug.h"
#include "TheGreatEscape/Events.h"
#include "TheGreatEscape/ExteriorTiles.h"
#include "TheGreatEscape/Input.h"
#include "TheGreatEscape/InteriorObjectDefs.h"
#include "TheGreatEscape/InteriorTiles.h"
#include "TheGreatEscape/ItemBitmaps.h"
#include "TheGreatEscape/Map.h"
#include "TheGreatEscape/Masks.h"
#include "TheGreatEscape/Menu.h"
#include "TheGreatEscape/Messages.h"
#include "TheGreatEscape/Pixels.h"
#include "TheGreatEscape/RoomDefs.h"
#include "TheGreatEscape/Screen.h"
#include "TheGreatEscape/SpriteBitmaps.h"
#include "TheGreatEscape/State.h"
#include "TheGreatEscape/StaticGraphics.h"
#include "TheGreatEscape/Text.h"
#include "TheGreatEscape/TGEObject.h"
#include "TheGreatEscape/Zoombox.h"

#include "TheGreatEscape/Main.h"

/* ----------------------------------------------------------------------- */

/* Divide x by 8, with rounding to nearest.
 *
 * This replaces calls to divide_by_8_with_rounding in the original code.
 */
#define divround(x) (((x) + 4) >> 3)

/* ----------------------------------------------------------------------- */

/**
 * $68A2: Transition.
 *
 * The current character (in state->IY) changes room.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] mappos Pointer to pos. (was HL)
 *
 * \remarks Exits using longjmp in the hero case.
 */
void transition(tgestate_t *state, const mappos8_t *mappos)
{
  vischar_t *vischar;    /* was IY */
  room_t     room_index; /* was A */

  assert(state  != NULL);
  assert(mappos != NULL);

  vischar = state->IY;
  ASSERT_VISCHAR_VALID(vischar);

  if (vischar->room == room_0_OUTDOORS)
  {
    /* Outdoors */

    /* Set position on X axis, Y axis and height (multiplying by 4). */

    /* Conv: This was unrolled (compared to the original) to avoid having to
     * access the structure as an array. */
    vischar->mi.mappos.u = mappos->u * 4;
    vischar->mi.mappos.v = mappos->v * 4;
    vischar->mi.mappos.w = mappos->w * 4;
  }
  else
  {
    /* Indoors */

    /* Set position on X axis, Y axis and height (copying). */

    /* Conv: This was unrolled (compared to the original) to avoid having to
     * access the structure as an array. */
    vischar->mi.mappos.u = mappos->u;
    vischar->mi.mappos.v = mappos->v;
    vischar->mi.mappos.w = mappos->w;
  }

  if (vischar != &state->vischars[0])
  {
    /* Not the hero. */

    reset_visible_character(state, vischar); /* was tail call */
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
      vischar->direction &= vischar_DIRECTION_MASK; /* clear crawl flag */
      reset_outdoors(state);
      squash_stack_goto_main(state); /* was tail call */
    }
    else
    {
      /* Indoors */

      enter_room(state); /* was fallthrough */
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
  // FUTURE: replace with call to setup_room_and_plot
  setup_room(state);
  plot_interior_tiles(state);
  state->map_position.x = 116;
  state->map_position.y = 234;
  set_hero_sprite_for_room(state);
  calc_vischar_isopos_from_vischar(state, &state->vischars[0]);
  setup_movable_items(state);
  zoombox(state);
  increase_score(state, 1);

  squash_stack_goto_main(state); /* was fallthrough */
  NEVER_RETURNS;
}

/**
 * $691A: Squash stack, then goto main.
 *
 * In the original code this squashed the stack then jumped to the start of
 * main_loop(). In this version it uses longjmp() to exit from tge_main() to
 * permit cooperation with the host environment.
 *
 * \param[in] state Pointer to game state.
 *
 * \remarks Exits using longjmp().
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
   * directly anywhere else so it must be an offset into animindices[].
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
  animate(state);
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
  ASSERT_CHARACTER_VALID(character);

  /* The movable item uses the first non-player visible character slot. */
  vischar1 = &state->vischars[1];

  vischar1->character         = character;
  vischar1->mi                = *movableitem;

  /* Conv: Reset data inlined. */

  vischar1->flags             = 0;
  vischar1->route.index       = routeindex_0_HALT;
  vischar1->route.step        = 0;
  vischar1->target.u          = 0;
  vischar1->target.v          = 0;
  vischar1->target.w          = 0;
  vischar1->counter_and_flags = 0;
  vischar1->animbase          = &animations[0];
  vischar1->anim              = animations[8]; /* -> anim_wait_tl animation */
  vischar1->animindex         = 0;
  vischar1->input             = 0;
  vischar1->direction         = direction_TOP_LEFT; /* == 0 */

  vischar1->room              = state->room_index;
  calc_vischar_isopos_from_vischar(state, vischar1);
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

  /* Wipe state->interior_doors[] with interiordoor_NONE.
   * Alternative: memset(&state->interior_doors[0], interiordoor_NONE, 4); */
  pdoorindex = &state->interior_doors[3];
  iters = 4;
  do
    *pdoorindex-- = interiordoor_NONE;
  while (--iters);

  pdoorindex++; /* Reset to first byte of interior_doors[]. */
  ASSERT_DOORS_VALID(pdoorindex);

  /* Ensure that no flag bits remain. */
  assert(state->room_index < room__LIMIT);

  room       = state->room_index << 2; /* Shift left to match comparison in loop. */
  door_index = 0;
  door       = &doors[0];
  door_iters = NELEMS(doors); /* Iterate over every door in doors[]. */
  do
  {
    /* Save the door index (and current reverse flag) whenever we match the
     * current room. Rooms are always rectangular so should have no more than
     * four doors present but that is unchecked by this code. */
    if ((door->room_and_direction & ~door_FLAGS_MASK_DIRECTION) == room)
      *pdoorindex++ = door_index ^ door_REVERSE;

    /* On every iteration toggle the reverse flag. */
    door_index ^= door_REVERSE;

    /* Increment door_index once every two iterations. */
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
 * \param[in] index Index of door + lock flag in bit 7. (was A)
 *
 * \return Pointer to door_t. (was HL)
 */
const door_t *get_door(doorindex_t index)
{
  const door_t *door; /* was HL */

  assert((index & ~door_REVERSE) < door_MAX);

  /* Conv: Mask before multiplication to avoid overflow. */
  door = &doors[(index & ~door_REVERSE) * 2];
  if (index & door_REVERSE)
    door++;

  return door;
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

  memset(state->tile_buf, 0, state->tile_buf_size); /* was 24 * 17 */
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
    { 16, { 132, 134, 244, 246 }, { 10, 48, 32 } }, /* BUG FIX: bound.y1 was 1 too high. */
    { 17, { 135, 137, 237, 239 }, { 10, 48, 32 } },
    { 17, { 123, 125, 243, 245 }, { 10, 10, 32 } },
    { 17, { 121, 123, 244, 246 }, { 10, 10, 32 } },
    { 15, { 136, 140, 245, 248 }, { 10, 10, 32 } },
  };

  /* Conv: Direct access to the room definition data has been removed and
   * replaced with calls to get_room_byte. */

  //const roomdef_t *proomdef; /* was HL */
  int              room_index;  /* Conv: Replaces direct access */
  int              offset;      /* Conv: Replaces direct access */
  bounds_t        *pbounds;  /* was DE */
  mask_t          *pmask;    /* was DE */
  uint8_t          count;    /* was A */
  uint8_t          iters;    /* was B */

  assert(state != NULL);

  wipe_visible_tiles(state);

  assert(state->room_index >= 0);
  assert(state->room_index < room__LIMIT);
  room_index = state->room_index; /* local cached copy */
  offset     = 0;

  setup_doors(state);

  state->roomdef_dimensions_index = get_roomdef(state, room_index, offset++);

  /* Copy boundaries into state. */
  state->roomdef_object_bounds_count = count = get_roomdef(state, room_index, offset); /* count of boundaries */
  assert(count <= MAX_ROOMDEF_OBJECT_BOUNDS);
  pbounds = &state->roomdef_object_bounds[0]; /* Conv: Moved */
  if (count == 0) /* no boundaries */
  {
    offset++; /* skip to interior mask */
  }
  else
  {
    uint8_t     *p; /* so we can access bounds as bytes */
    unsigned int c; /* */

    offset++; /* Conv: Don't re-copy already copied count byte. */
    p = &pbounds->x0;
    for (c = 0; c < count * sizeof(bounds_t); c++)
      *p++ = get_roomdef(state, room_index, offset++);
    /* arrive at interior mask */
  }

  /* Copy interior mask into state->interior_mask_data. */
  state->interior_mask_data_count = iters = get_roomdef(state, room_index, offset++); /* count of interior masks */
  assert(iters <= MAX_INTERIOR_MASK_REFS);
  pmask = &state->interior_mask_data[0]; /* Conv: Moved */
  while (iters--)
  {
    uint8_t index;

    index = get_roomdef(state, room_index, offset++);
    /* Conv: Structures were changed from 7 to 8 bytes wide. */
    memcpy(pmask, &interior_mask_data_source[index], sizeof(*pmask));
    pmask++;
  }

  /* Plot all objects (as tiles). */
  iters = get_roomdef(state, room_index, offset++); /* Count of objects */
  while (iters--)
  {
    uint8_t object_index;
    uint8_t column;
    uint8_t row;

    object_index = get_roomdef(state, room_index, offset++);
    column       = get_roomdef(state, room_index, offset++);
    row          = get_roomdef(state, room_index, offset++);

    expand_object(state, object_index, &state->tile_buf[row * state->columns + column]);
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
 *   <255> <64..79> <t>:   emit tiles <t> <t+1> <t+2> .. up to 15 times
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
            goto repetition;
          if (byte == 64)
            goto range;
          assert(0);
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


repetition:
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

      // val = *data; // needless reload in C version (required in original)

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
        ASSERT_WINDOW_BUF_PTR_VALID(window_buf2, 0);
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
const roomdef_address_t beds[beds_LENGTH] =
{
  /* Conv: The original game uses pointers here. */
  { room_3_HUT2RIGHT, roomdef_3_BED_A },
  { room_3_HUT2RIGHT, roomdef_3_BED_B },
  { room_3_HUT2RIGHT, roomdef_3_BED_C },
  { room_5_HUT3RIGHT, roomdef_5_BED_D },
  { room_5_HUT3RIGHT, roomdef_5_BED_E },
  { room_5_HUT3RIGHT, roomdef_5_BED_F },
};

/* ----------------------------------------------------------------------- */

/**
 * $78D6: Door positions.
 *
 * Used by setup_doors, get_door, door_handling and target_reached.
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
  // outdoor targets are divided by 4

  // 0 - gate - initially locked
  { ROOMDIR(room_0_OUTDOORS,              TR), { 178, 138,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), { 178, 142,  6 } },
  // 1 - gate - initially locked
  { ROOMDIR(room_0_OUTDOORS,              TR), { 178, 122,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), { 178, 126,  6 } },
  // 2
  { ROOMDIR(room_34,                      TL), { 138, 179,  6 } },  // tunnels 2 (end)
  { ROOMDIR(room_0_OUTDOORS,              BR), {  16,  52, 12 } },
  // 3
  { ROOMDIR(room_48,                      TL), { 204, 121,  6 } },  // tunnels 1 (end)
  { ROOMDIR(room_0_OUTDOORS,              BR), {  16,  52, 12 } },
  // 4
  { ROOMDIR(room_28_HUT1LEFT,             TR), { 217, 163,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BL), {  42,  28, 24 } },
  // 5
  { ROOMDIR(room_1_HUT1RIGHT,             TL), { 212, 189,  6 } },
  { ROOMDIR(room_0_OUTDOORS,              BR), {  30,  46, 24 } },
  // 6 - home room - door to outside
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
  // 17 - home room - top right door in HUT2LEFT
  { ROOMDIR(room_3_HUT2RIGHT,             TR), {  36,  54, 24 } },
  { ROOMDIR(room_2_HUT2LEFT,              BL), {  38,  26, 24 } },
  // 18
  { ROOMDIR(room_5_HUT3RIGHT,             TR), {  36,  54, 24 } },
  { ROOMDIR(room_4_HUT3LEFT,              BL), {  38,  26, 24 } },
  // 19
  { ROOMDIR(room_23_MESS_HALL,            TR), {  40,  66, 24 } },
  { ROOMDIR(room_25_MESS_HALL,            BL), {  38,  24, 24 } },
  // 20 -
  { ROOMDIR(room_23_MESS_HALL,            TL), {  62,  36, 24 } },
  { ROOMDIR(room_21_CORRIDOR,             BR), {  32,  46, 24 } },
  // 21
  { ROOMDIR(room_19_FOOD,                 TR), {  34,  66, 24 } },
  { ROOMDIR(room_23_MESS_HALL,            BL), {  34,  28, 24 } },
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

  memcpy(&state->saved_mappos,
         &state->vischars[0].mi.mappos,
          sizeof(mappos16_t));

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
    // FUTURE: replace with call to setup_room_and_plot
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
  itemstr->isopos.x       = 0;
  itemstr->isopos.y       = 0;

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
  mappos8_t    *outpos;  /* was DE */
  mappos16_t   *inpos;   /* was HL */

  assert(state != NULL);
  ASSERT_ITEM_VALID(item);

  itemstr = &state->item_structs[item];
  room = state->room_index;
  itemstr->room_and_flags = room; /* Set object's room index. */
  if (room == room_0_OUTDOORS)
  {
    /* Outdoors. */

    // TODO: Hoist common code.
    outpos = &itemstr->mappos;
    inpos  = &state->vischars[0].mi.mappos;

    scale_mappos_down(inpos, outpos);
    outpos->w = 0;

    calc_exterior_item_isopos(itemstr);
  }
  else
  {
    /* $7BE4: Drop item, interior part. */

    outpos = &itemstr->mappos;
    inpos  = &state->vischars[0].mi.mappos;

    outpos->u = inpos->u; // note: narrowing
    outpos->v = inpos->v; // note: narrowing
    outpos->w = 5;

    calc_interior_item_isopos(itemstr);
  }
}

/**
 * $7BD0: Calculate isometric screen position for exterior item.
 *
 * Called by drop_item_tail and item_discovered.
 *
 * \param[in] itemstr Pointer to item struct. (was HL)
 */
void calc_exterior_item_isopos(itemstruct_t *itemstr)
{
  pos8_t *isopos;

  assert(itemstr != NULL);

  isopos = &itemstr->isopos;

  isopos->x = (0x40 - itemstr->mappos.u + itemstr->mappos.v) * 2;
  isopos->y = 0x100 - itemstr->mappos.u - itemstr->mappos.v - itemstr->mappos.w; /* Conv: 0x100 is 0 in the original. */
}

/**
 * $7BF2: Calculate isometric screen position for interior item.
 *
 * Called by drop_item_tail and item_discovered.
 *
 * \param[in] itemstr Pointer to item struct. (was HL)
 */
void calc_interior_item_isopos(itemstruct_t *itemstr)
{
  pos8_t *isopos;

  assert(itemstr != NULL);

  isopos = &itemstr->isopos;

  isopos->x = divround((0x200 - itemstr->mappos.u + itemstr->mappos.v) * 2);
  isopos->y = divround(0x800 - itemstr->mappos.u - itemstr->mappos.v - itemstr->mappos.w);
}

/* ----------------------------------------------------------------------- */

/* Conv: $7C26/item_to_itemstruct was inlined. */

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
  uint8_t *const screen = &state->speccy->screen.pixels[0];

  attribute_t       *attrs;  /* was HL */
  attribute_t        attr;   /* was A */
  const spritedef_t *sprite; /* was HL */

  /* Wipe item. */
  screen_wipe(state, screen + dstoff, 2, 16);

  if (item == item_NONE)
    return;

  ASSERT_ITEM_VALID(item);

  /* Set screen attributes. */
  attrs = (dstoff & 0xFF) + &state->speccy->screen.attributes[0x5A00 - SCREEN_ATTRIBUTES_START_ADDRESS];
  attr = state->item_attributes[item];
  attrs[0] = attr;
  attrs[1] = attr;

  /* Move to next attribute row. */
  attrs += state->width;
  attrs[0] = attr;
  attrs[1] = attr;

  /* Plot the item bitmap. */
  sprite = &item_definitions[item];
  plot_bitmap(state, sprite->bitmap, screen + dstoff, sprite->width, sprite->height);
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

      // FUTURE: Candidate for loop unrolling.
      structcoord = &itemstr->mappos.u;
      herocoord   = &state->hero_mappos.u;
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
 * \param[in]  state  Pointer to game state.
 * \param[in]  src    Source address.       (was DE)
 * \param[out] dst    Destination address.  (was HL)
 * \param[in]  width  Width, in bytes.      (was B)
 * \param[in]  height Height, in scanlines. (was C)
 */
void plot_bitmap(tgestate_t    *state,
                 const uint8_t *src,
                 uint8_t       *dst,
                 uint8_t        width,
                 uint8_t        height)
{
  uint8_t  h;
  uint8_t *curr_dst;

  assert(state != NULL);
  assert(src   != NULL);
  assert(dst   != NULL);
  assert(width  > 0);
  assert(height > 0);

  h        = height;
  curr_dst = dst;
  do
  {
    memcpy(curr_dst, src, width);
    src += width;
    curr_dst = get_next_scanline(state, curr_dst);
  }
  while (--h);

  /* Conv: Invalidation added over the original game. */
  invalidate_bitmap(state, dst, width * 8, height);
}

/* ----------------------------------------------------------------------- */

/**
 * $7CD4: Wipe the screen.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] dst    Destination address.  (was HL)
 * \param[in] width  Width, in bytes.      (was B)
 * \param[in] height Height, in scanlines. (was C)
 */
void screen_wipe(tgestate_t *state,
                 uint8_t    *dst,
                 uint8_t     width,
                 uint8_t     height)
{
  uint8_t  h;
  uint8_t *curr_dst;

  assert(state != NULL);
  assert(dst   != NULL);
  assert(width  > 0);
  assert(height > 0);

  h        = height;
  curr_dst = dst;
  do
  {
    memset(curr_dst, 0, width);
    curr_dst = get_next_scanline(state, curr_dst);
  }
  while (--h);

  /* Conv: Invalidation added over the original game. */
  invalidate_bitmap(state, dst, width * 8, height);
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
  uint8_t *const screen = &state->speccy->screen.pixels[0]; /* added */
  uint16_t       offset; /* was HL */
  uint16_t       delta;  /* was DE */

  assert(state != NULL);
  assert(slp   != NULL);

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

  state->speccy->stamp(state->speccy);

  check_morale(state);
  keyscan_break(state);
  message_display(state);
  process_player_input(state);
  in_permitted_area(state);
  restore_tiles(state);
  move_a_character(state);
  automatics(state);
  purge_invisible_characters(state);
  spawn_characters(state);
  mark_nearby_items(state);
  ring_bell(state);
  animate(state);
  move_map(state);
  message_display(state); /* second */
  ring_bell(state); /* second */
  plot_sprites(state);
  plot_game_window(state);
  ring_bell(state); /* third */
  if (state->day_or_night != 0)
    nighttime(state);
  /* Conv: Removed interior_delay_loop call here. */
  wave_morale_flag(state);
  if ((state->game_counter & 63) == 0)
    dispatch_timed_event(state);

  /* Conv: Timing: To calculate the magic timing interval value here the
   * original game was run in FUSE and its main_loop breakpointed. For both
   * indoor and outdoor scenes the T-states and current frame number were
   * captured, then the average T-state figure for the game was calculated.
   *
   * The original game is not dependent on accurate timing: it is much slower
   * in outdoor scenes and especially when multiple characters are on the
   * screen simultaneously. */
  (void) state->speccy->sleep(state->speccy, 367731);
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

  queue_message(state, message_MORALE_IS_ZERO);

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
        state->vischars[0].route.index = routeindex_43_7833;
        state->vischars[0].route.step  = 0;
        state->vischars[0].mi.mappos.u = 52;
        state->vischars[0].mi.mappos.v = 62;
        set_roomdef(state,
                    room_25_MESS_HALL,
                    roomdef_25_BENCH_G,
                    interiorobject_EMPTY_BENCH);
        state->hero_in_breakfast = 0;
      }
      else
      {
        /* Hero was in bed. */
        state->vischars[0].route.index = routeindex_44_HUT2_RIGHT_TO_LEFT;
        state->vischars[0].route.step  = 1;
        state->vischars[0].target.u    = 46;
        state->vischars[0].target.v    = 46;
        state->vischars[0].mi.mappos.u = 46;
        state->vischars[0].mi.mappos.v = 46;
        state->vischars[0].mi.mappos.w = 24;
        set_roomdef(state,
                    room_2_HUT2LEFT,
                    roomdef_2_BED,
                    interiorobject_EMPTY_BED_FACING_SE);
        state->hero_in_bed = 0;
      }

      // FUTURE: replace with call to setup_room_and_plot
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
  queue_message(state, message_IT_IS_OPEN);

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

  vischar_t   *hero = &state->vischars[0]; /* new, for conciseness */
  unsigned int delta; /* was A */

  assert(state != NULL);

  delta = state->player_locked_out_until - state->game_counter;
  if (delta)
  {
    if (delta < 4)
      hero->input = new_inputs[hero->direction & vischar_DIRECTION_MASK];
  }
  else
  {
    /* Countdown reached: Break through the fence. */
    /* BUG FIX: The original game neglects to fetch hero->direction here and
     * uses 'delta' instead as the direction. delta is always zero here, so
     * 'hero->direction' gets set to zero too. Thus the hero will always face
     * top-left (direction_TOP_LEFT == 0) after breaking through a fence.
     * We fix that here. */
    hero->direction = hero->direction & vischar_DIRECTION_MASK;
    hero->input = input_KICK;
    hero->mi.mappos.w = 24;

    /* Conv: The original code jumps into the tail end of picking_lock()
     * above to do this. */
    hero->flags &= ~(vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE);
  }
}

/* ----------------------------------------------------------------------- */

#define permitted_route_ROOM (1 << 7) /* Encodes a room index. */

/**
 * $9F21: Check the hero's map position and colour the flag accordingly.
 *
 * Also detect escapes.
 *
 * \param[in] state Pointer to game state.
 */
void in_permitted_area(tgestate_t *state)
{
  /**
   * $9EF9: Seven variable-length arrays which encode a list of valid rooms
   * (if top bit set) or permitted areas (0, 1 or 2) for a given route and
   * step within the route. Terminated with 255.
   *
   * Note that while routes encode transitions _between_ rooms this table
   * encodes rooms or areas, so each list here will be one entry longer than
   * the corresponding route.
   */
#define R permitted_route_ROOM /* shorthand */
  static const uint8_t permitted_route42[] = { R| 2, R|2,                      255 }; /* for route_hut2_left_to_right */
  static const uint8_t permitted_route5[]  = { R| 3,   1,    1,    1,          255 }; /* for route_exit_hut2 */
  static const uint8_t permitted_route14[] = {    1,   1,    1,    0,    2, 2, 255 }; /* for route_go_to_yard */
  static const uint8_t permitted_route16[] = {    1,   1, R|21, R|23, R|25,    255 }; /* for route_breakfast_room_25 (breakfasty) */
  static const uint8_t permitted_route44[] = { R| 3, R|2,                      255 }; /* for route_hut2_right_to_left */
  static const uint8_t permitted_route43[] = { R|25,                           255 }; /* for route_7833 */
  static const uint8_t permitted_route45[] = {    1,                           255 }; /* for route_hero_roll_call */
#undef R

  /**
   * $9EE4: Maps route indices to pointers to the above arrays.
   *
   * Suspect that this means "if you're in <this> route you can be in <these> places."
   */
  static const route_to_permitted_t route_to_permitted[7] =
  {
    { routeindex_42_HUT2_LEFT_TO_RIGHT, &permitted_route42[0] },
    { routeindex_5_EXIT_HUT2,           &permitted_route5[0]  },
    { routeindex_14_GO_TO_YARD,         &permitted_route14[0] },
    { routeindex_16_BREAKFAST_25,       &permitted_route16[0] },
    { routeindex_44_HUT2_RIGHT_TO_LEFT, &permitted_route44[0] },
    { routeindex_43_7833,               &permitted_route43[0] },
    { routeindex_45_HERO_ROLL_CALL,     &permitted_route45[0] },
  };

  mappos16_t  *hero_vischar_mappos; /* was HL */
  mappos8_t   *hero_state_mappos;   /* was DE */
  attribute_t  attr;                /* was A */
  uint8_t      routeindex;          /* was A */
  route_t      route;               /* was CA */
  uint16_t     i;                   /* was BC */
  uint8_t      red_flag;            /* was A */

  assert(state != NULL);

  hero_vischar_mappos = &state->vischars[0].mi.mappos;
  hero_state_mappos   = &state->hero_mappos;
  if (state->room_index == room_0_OUTDOORS)
  {
    /* Outdoors. */

    scale_mappos_down(hero_vischar_mappos, hero_state_mappos);

    /* (217 * 8, 137 * 8) */
    if (state->vischars[0].isopos.x >= MAP_WIDTH  * 8 ||
        state->vischars[0].isopos.y >= MAP_HEIGHT * 8)
    {
      escaped(state);
      return;
    }
  }
  else
  {
    /* Indoors. */

    hero_state_mappos->u = hero_vischar_mappos->u; // note: narrowing
    hero_state_mappos->v = hero_vischar_mappos->v; // note: narrowing
    hero_state_mappos->w = hero_vischar_mappos->w; // note: narrowing
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

  // puzzling - look at the /next/ step?
  route = state->vischars[0].route;
  ASSERT_ROUTE_VALID(route);
  if (route.index & ROUTEINDEX_REVERSE_FLAG)
    route.step++;

  if (route.index == routeindex_255_WANDER)
  {
    /* Hero is wandering */

    uint8_t area; /* was A */

    /* Note that this does NOT using route.step which is possibly modified. */
    /* 1 => Hut area, 2 => Yard area */
    area = ((state->vischars[0].route.step & ~7) == 8) ? 1 : 2;
    if (!in_permitted_area_end_bit(state, area))
      goto set_flag_red;
    else
      goto set_flag_green; /* This happens naturally anyway */
  }
  else
  {
    /* Hero is en route */

    const route_to_permitted_t *tab;       /* was HL */
    uint8_t                     iters;     /* was B */
    const uint8_t              *permitted; /* was HL */

    routeindex = route.index & ~ROUTEINDEX_REVERSE_FLAG;
    tab = &route_to_permitted[0];
    iters = NELEMS(route_to_permitted);
    do
    {
      if (routeindex == tab->routeindex)
        goto found;
      tab++;
    }
    while (--iters);

    /* If the route is not found in route_to_permitted assume a green flag */
    goto set_flag_green;

found:
    /* Route index was found in route_to_permitted[] */

    permitted = tab->permitted;
    // PUSH permitted(DE)
    // POP HL
    // HL = permitted + route.step(C)
    // PUSH permitted(DE)
    if (!in_permitted_area_end_bit(state, permitted[route.step]))
    // POP HL = permitted
    {
      // FUTURE: The route index is already in 'route.index'.
      if (state->vischars[0].route.index & ROUTEINDEX_REVERSE_FLAG)
        permitted++; // puzzling - see above

      i = 0;
      for (;;)
      {
        uint8_t room_or_area; /* was A */

        room_or_area = permitted[i];
        if (room_or_area == 255) /* end of list */
          goto set_flag_red; /* Conv: Jump adjusted */
        if (in_permitted_area_end_bit(state, room_or_area))
          break; /* green */
        i++;
      }

      {
        route_t route2; /* was BC */

        assert(i < 256);

        route2.index = state->vischars[0].route.index;
        route2.step  = i;
        set_hero_route(state, &route2);
      }
    }
  }

  /* Green flag code path. */
set_flag_green:
  red_flag = 0;
  attr = attribute_BRIGHT_GREEN_OVER_BLACK;

flag_select:
  /* Conv: Original game tests the attributes directly here, instead we test
   * the red_flag. */
  if (state->red_flag == red_flag)
    return; /* flag is already the requested state */

  state->red_flag = red_flag;
  // was: if (attr == state->speccy->screen.attributes[morale_flag_attributes_offset])
  // was:   return; /* flag is already the correct colour */

  if (attr == attribute_BRIGHT_GREEN_OVER_BLACK)
    state->bell = bell_STOP;

  set_morale_flag_screen_attributes(state, attr);
  return;

  /* Red flag code path. */
set_flag_red:
  /* Conv: Original game tests the attributes directly here, instead we test
   * the red_flag. */
  // was: if (state->speccy->screen.attributes[morale_flag_attributes_offset] == attribute_BRIGHT_RED_OVER_BLACK)
  if (state->red_flag == 255)
    return; /* flag is already red */

  attr = attribute_BRIGHT_RED_OVER_BLACK;

  state->vischars[0].input = 0;
  red_flag = 255;
  goto flag_select;
}

/**
 * $A007: In permitted area (end bit).
 *
 * Checks that the hero is in the specified room or camp bounds.
 *
 * \param[in] state          Pointer to game state.
 * \param[in] room_and_flags If bit 7 is set then bits 0..6 contain a room index. Otherwise it's an area index as passed into within_camp_bounds. (was A)
 *
 * \return true if in permitted area.
 */
int in_permitted_area_end_bit(tgestate_t *state, uint8_t room_and_flags)
{
  room_t room; /* was HL */

  assert(state != NULL);

  room = state->room_index; /* Conv: Dereferenced up-front once. */

  if (room_and_flags & permitted_route_ROOM)
  {
    /* Hero should be in the specified room. */
    ASSERT_ROOM_VALID(room_and_flags & ~permitted_route_ROOM);
    return room == (room_and_flags & ~permitted_route_ROOM);
  }
  else if (room == room_0_OUTDOORS) // is outside
  {
    /* Hero is outdoors - check bounds. */
    return within_camp_bounds(room_and_flags, &state->hero_mappos);
  }
  else
  {
    /* Hero should be outside but is in room - not permitted. */
    return 0;
  }
}

#undef permitted_route_ROOM

/**
 * $A01A: For outdoor areas is the specified position within the bounds of the area?
 *
 * \param[in] area   Index (0..2) into permitted_bounds[] table. (was A)
 * \param[in] mappos Pointer to position. (was HL)
 *
 * \return true if in permitted area.
 */
int within_camp_bounds(uint8_t          area, // ought to be an enum
                       const mappos8_t *mappos)
{
  /**
   * $9F15: Bounds of the three main exterior areas.
   */
  static const bounds_t permitted_bounds[3] =
  {
    { 86, 94, 61, 72 }, /* Corridor to yard */
    { 78,132, 71,116 }, /* Hut area */
    { 79,105, 47, 63 }, /* Yard area */
  };

  const bounds_t *bounds; /* was HL */

  assert(area < NELEMS(permitted_bounds));
  assert(mappos != NULL);

  bounds = &permitted_bounds[area];
  return mappos->u >= bounds->x0 && mappos->u < bounds->x1 &&
         mappos->v >= bounds->y0 && mappos->v < bounds->y1;
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
  uint8_t       *scanline;          /* was HL */
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
      scanline = get_next_scanline(state, state->moraleflag_screen_address);
    }
    else
    {
      /* Increasing morale. */
      (*pdisplayed_morale)++;
      scanline = get_prev_scanline(state, state->moraleflag_screen_address);
    }
    state->moraleflag_screen_address = scanline;
  }

  flag_bitmap = &flag_down[0];
  if (*pgame_counter & 2)
    flag_bitmap = &flag_up[0];
  plot_bitmap(state,
              flag_bitmap,
              state->moraleflag_screen_address,
              3, 25);
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

  pattrs = &state->speccy->screen.attributes[morale_flag_attributes_offset];
  iters = 19; /* Height of flag. */
  do
  {
    pattrs[0] = attrs;
    pattrs[1] = attrs;
    pattrs[2] = attrs;
    pattrs += state->width;
  }
  while (--iters);

  /* Conv: Invalidation added over the original game. */
  invalidate_attrs(state,
                   &state->speccy->screen.attributes[morale_flag_attributes_offset],
                   3 * 8, 19 * 8);
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
  uint8_t *const screen = &state->speccy->screen.pixels[0];
  ptrdiff_t      raddr = addr - screen;

  assert(state != NULL);
  assert(addr  != NULL);

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
  bell = state->speccy->screen.pixels[screenoffset_BELL_RINGER];
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
              src,
              &state->speccy->screen.pixels[screenoffset_BELL_RINGER],
              1, 12 /* dimensions: 8 x 12 */);
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
  screen = &state->speccy->screen.pixels[score_address];
  iters = NELEMS(state->score_digits);
  do
  {
    char digit = '0' + *digits; /* Conv: Pass as ASCII. */

    screen = plot_glyph(state, &digit, screen);
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

  speakerbit = port_MASK_EAR; /* Initial speaker bit. */
  do
  {
    state->speccy->out(state->speccy, port_BORDER_EAR_MIC, speakerbit); /* Play. */

    /* Conv: Timing: The original game uses an empty delay loop here to
     * maintain the currently output speaker bit. Instead of that, since our
     * sound code has no knowledge of time, we just output the speaker bit
     * for the delay period. */
    {
      int i;

      for (i = 0; i < delay; i++)
        state->speccy->out(state->speccy, port_BORDER_EAR_MIC, speakerbit);;
    }

    speakerbit ^= port_MASK_EAR; /* Toggle speaker bit. */
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

  attributes = &state->speccy->screen.attributes[0x0047];
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

  /* Conv: Invalidation added over the original game. */
  invalidate_attrs(state,
                   &state->speccy->screen.attributes[0x0047],
                   state->columns * 8,
                   (state->rows - 1) * 8);
}

/* ----------------------------------------------------------------------- */

/**
 * $A50B: Reset the screen.
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
  uint8_t                  keys;      /* was A */

  assert(state != NULL);

  screen_reset(state);

  /* Print standard prefix messages. */
  message = &messages[0];
  message = screenlocstring_plot(state, message); /* WELL DONE */
  message = screenlocstring_plot(state, message); /* YOU HAVE ESCAPED */
  (void) screenlocstring_plot(state, message);    /* FROM THE CAMP */

  /* Form escape items bitfield. */
  itemflags = item_to_escapeitem(state->items_held[0]) |
              item_to_escapeitem(state->items_held[1]);

  /*         Items                     Message
   *    Unf Pur Ppr Cmp                -------
   *    ---+---+---+---
   *  0  .   .   .   .  "BUT WERE RECAPTURED TOTALLY UNPREPARED" (lose)
   *  1  .   .   .   X  "BUT WERE RECAPTURED DUE TO LACK OF PAPERS" (lose)
   *  2  .   .   X   .  "BUT WERE RECAPTURED TOTALLY LOST" (lose)
   *  3  .   .   X   X  (the game offers no extra message) (win)
   *  4  .   X   .   .  "BUT WERE RECAPTURED TOTALLY LOST" (lose)
   *  5  .   X   .   X  "AND WILL CROSS THE BORDER SUCCESSFULLY" (win)
   *  6  .   X   X   .  "BUT WERE RECAPTURED TOTALLY LOST" (lose)
   *  8  X   .   .   .  "BUT WERE RECAPTURED AND SHOT AS A SPY" (lose)
   *  9  X   .   .   X  "BUT WERE RECAPTURED AND SHOT AS A SPY" (lose)
   * 10  X   .   X   .  "BUT WERE RECAPTURED AND SHOT AS A SPY" (lose)
   * 12  X   X   .   .  "BUT WERE RECAPTURED AND SHOT AS A SPY" (lose)
   */

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

    /* Have uniform => AND SHOT AS A SPY. */

    if (itemflags < escapeitem_UNIFORM)
    {
      /* No items => TOTALLY UNPREPARED. */
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

  /* Debounce: First wait for any already-held key to be released. */
  do
    keys = keyscan_all(state);
  while (keys != 0); /* Down press */
  /* Then wait for any key to be pressed. */
  do
    keys = keyscan_all(state);
  while (keys == 0); /* Up press */

  /* Reset the game, or send the hero to solitary. */
  if (itemflags == 0xFF || itemflags >= escapeitem_UNIFORM)
    reset_game(state); /* was tail call */
  else
    solitary(state); /* was tail call */
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
      goto exit; /* Key(s) pressed */

    /* Rotate the top byte of the port number to get the next port. */
    // Conv: Was RLC B
    carry = (port >> 15) != 0;
    port  = ((port << 1) & 0xFF00) | (carry << 8) | (port & 0x00FF);
  }
  while (carry);

  keys = 0; /* No keys pressed */

exit:

  /* Conv: Timing: The original game keyscans as fast as it can. We can't
   * have that so instead we introduce a short delay and handle game thread
   * termination. */
  gamedelay(state, 3500000 / 50); /* 50/sec */

  return keys;
}

/* ----------------------------------------------------------------------- */

/* Conv: $A59C/join_item_to_escapeitem was inlined. */

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

  screen = &state->speccy->screen.pixels[slstring->screenloc];
  length = slstring->length;
  string = slstring->string;
  do
    screen = plot_glyph(state, string++, screen);
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

  /* Multiply 'v' by (MAPX / 4) (= 13.5 = 1.5 * 9).
   * 'v' is a multiple of 4, so this goes 0, 54, 108, 162, ...)
   * MAPX is subtracted to skip the first row. */
  tiles = &map[0] - MAPX + (v + (v >> 1)) * 9;

  /* Add horizontal offset. */
  tiles += state->map_position.x >> 2;

  iters = state->st_rows;
  /* Conv: Avoid reading outside the map bounds. */
  if ((tiles + ((state->st_rows - 1) * MAPX) + state->st_columns) > &map[MAPX * MAPY])
    iters--;

  /* Populate map_buf with 7x5 array of supertile refs. */
  buf = &state->map_buf[0];
  do
  {
    ASSERT_MAP_PTR_VALID(tiles);
    memcpy(buf, tiles, state->st_columns);
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
  maptiles = &state->map_buf[7 * 4];          // $FF74
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
  ASSERT_WINDOW_BUF_PTR_VALID(window, 0);

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
    return; /* can skip trailing edge */

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
  ASSERT_WINDOW_BUF_PTR_VALID(window, 0);

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
    ASSERT_WINDOW_BUF_PTR_VALID(window, 0);

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
      ASSERT_WINDOW_BUF_PTR_VALID(window, 0);

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
    ASSERT_WINDOW_BUF_PTR_VALID(window, 0);

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

  return plot_tile(state, tile_index, maptiles, scr) + state->window_buf_stride - 1; // -1 compensates the +1 in plot_tile // was 191
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
  ASSERT_WINDOW_BUF_PTR_VALID(scr, 0);

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
    ASSERT_WINDOW_BUF_PTR_VALID(dst, 0);
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
  // Conv: Original code uses LDDR
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
  const anim_t *anim;               /* was DE */
  uint8_t       animindex;          /* was C */
  direction_t   map_direction;      /* was A */
  uint8_t       move_map_y;         /* was A */
  uint8_t       x,y;                /* was C,B */
  pos8_t        game_window_offset; /* was HL */

  assert(state != NULL);

  if (state->room_index > room_0_OUTDOORS)
    return; /* Don't move the map when indoors. */

  if (state->vischars[0].counter_and_flags & vischar_BYTE7_DONT_MOVE_MAP)
    return; /* Don't move the map when touch() saw contact. */

  anim      = state->vischars[0].anim;
  animindex = state->vischars[0].animindex;
  map_direction = anim->map_direction;
  if (map_direction == 255)
    return; /* Don't move the map if the current animation inhibits it. */

  if (animindex & vischar_ANIMINDEX_REVERSE)
    map_direction ^= 2; /* Exchange up and down directions when reversed. */

  /* Clamp the map position to (0..192,0..124). */

  if (/* DISABLES CODE */ (0))
  {
    /* Equivalent to: */
    switch (map_direction)
    {
      case direction_TOP_LEFT:     x = 192; y = 124; break;
      case direction_TOP_RIGHT:    x =   0; y = 124; break;
      case direction_BOTTOM_RIGHT: x =   0; y =   0; break;
      case direction_BOTTOM_LEFT:  x = 192; y =   0; break;
    }
  }
  else
  {
    y = 124;
    x = 0;
    /* direction_BOTTOM_* - bottom of the map clamp */
    if (map_direction >= direction_BOTTOM_RIGHT)
      y = 0;
    /* direction_*_LEFT - left of the map clamp */
    if (map_direction != direction_TOP_RIGHT &&
        map_direction != direction_BOTTOM_RIGHT)
      x = 192;
  }

  if (state->map_position.x == x || state->map_position.y == y)
    return; /* Don't move. */

  if (map_direction <= direction_TOP_RIGHT)
    /* Either direction_TOP_* */
    state->move_map_y++;
  else
    /* Either direction_BOTTOM_* */
    state->move_map_y--;
  state->move_map_y &= 3;

  move_map_y = state->move_map_y;
  assert(move_map_y <= 3);

  /* Set the game_window_offset. This is used by plot_game_window (which masks
   * off the top and bottom separately). */
  /* Conv: Simplified. */
  switch (move_map_y)
  {
    case 0: game_window_offset.x = 0x00; game_window_offset.y = 0x00; break;
    case 1: game_window_offset.x = 0x30; game_window_offset.y = 0xFF; break;
    case 2: game_window_offset.x = 0x60; game_window_offset.y = 0x00; break;
    case 3: game_window_offset.x = 0x90; game_window_offset.y = 0xFF; break;
  }

  state->game_window_offset = game_window_offset;

  /* These cases check the move_map_y counter to decide when to shunt up/down,
   * and left/right. */
  /* Conv: Inlined. */
  switch (map_direction)
  {
    case direction_TOP_LEFT:
      /* Used when player moves down-right (map is shifted up-left).
       * Pattern: 0..3 => up/left/none/left. */
      if (move_map_y == 0)
        shunt_map_up(state);
      else if (move_map_y & 1)
        shunt_map_left(state);
      break;

    case direction_TOP_RIGHT:
      /* Used when player moves down-left (map is shifted up-right).
       * Pattern: 0..3 => up-right/none/right/none. */
      if (move_map_y == 0)
        shunt_map_up_right(state);
      else if (move_map_y == 2)
        shunt_map_right(state);
      break;

    case direction_BOTTOM_RIGHT:
      /* Used when player moves up-left (map is shifted down-right).
       * Pattern: 0..3 => right/none/right/down. */
      if (move_map_y == 3)
        shunt_map_down(state);
      else if ((move_map_y & 1) == 0)
        shunt_map_right(state);
      break;

    case direction_BOTTOM_LEFT:
      /* Used when player moves up-right (map is shifted down-left).
       * Pattern: 0..3 => none/left/none/down-left. */
      if (move_map_y == 1)
        shunt_map_left(state);
      else if (move_map_y == 3)
        shunt_map_down_left(state);
      break;
  }
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
 * $AD59: Decides searchlight movement.
 *
 * Leaf.
 *
 * \param[in] slstate Pointer to a searchlight movement data. (was HL)
 */
void searchlight_movement(searchlight_movement_t *slstate)
{
#define REVERSE (1 << 7)

  uint8_t        x,y;       /* was E,D */
  uint8_t        index;     /* was A */
  const uint8_t *ptr;       /* was BC */
  direction_t    direction; /* was A */

  assert(slstate != NULL);

  x = slstate->xy.x;
  y = slstate->xy.y;
  if (--slstate->counter == 0)
  {
    /* End of previous sweep: work out the next. */

    index = slstate->index; // sampled HL = $AD3B, $AD34, $AD2D
    if (index & REVERSE) /* reverse direction bit set */
    {
      index &= ~REVERSE;
      if (index == 0)
      {
        slstate->index &= ~REVERSE; /* clear direction bit when index hits zero */
      }
      else
      {
        slstate->index--; /* count down */
        index--; /* just a copy sans direction bit */
        assert(index == (slstate->index ^ REVERSE));
      }
    }
    else
    {
      slstate->index = ++index; /* count up */
    }
    assert(index >= 0);
    assert(index <= 8); // movement_1 is the longest
    ptr = slstate->ptr + index * 2;
    if (*ptr == 255) /* end of list? */
    {
      slstate->index--; /* overshot? count down counter byte */
      slstate->index |= REVERSE; /* go negative */
      ptr -= 2;
      // A = *ptr; /* BUG: This is a flaw in original code: A is fetched but never used again. */
    }
    /* Copy counter + direction_t. */
    slstate->counter   = ptr[0];
    slstate->direction = ptr[1];
  }
  else
  {
    direction = slstate->direction;
    if (slstate->index & REVERSE) /* test sign */
      direction ^= 2; /* toggle direction */

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

    slstate->xy.x = x; // Conv: Reversed store order.
    slstate->xy.y = y;
  }

#undef REVERSE
}

/**
 * $ADBD: Turns white screen elements light blue and tracks the hero with a
 * searchlight.
 *
 * \param[in] state Pointer to game state.
 */
void nighttime(tgestate_t *state)
{
  uint8_t                 map_y;     /* was D */
  uint8_t                 map_x;     /* was E */
  uint8_t                 caught_y;  /* was H */
  uint8_t                 caught_x;  /* was L */
  int                     iters;     /* was B */
  attribute_t            *attrs;     /* was BC */
  searchlight_movement_t *slstate;   /* was HL */
  uint8_t                 clip_left; /* was A */
  int16_t                 column;    /* was BC */
  int16_t                 row;       /* was HL */

  assert(state != NULL);

  if (state->searchlight_state != searchlight_STATE_SEARCHING)
  {
    /* Caught. */

    if (state->room_index > room_0_OUTDOORS)
    {
      /* If the hero goes indoors then the searchlight loses track. */
      state->searchlight_state = searchlight_STATE_SEARCHING;
      return;
    }

    /* Otherwise the hero is outdoors. */

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

  /* When not tracking the hero all three searchlights are cycled through. */

  slstate = &state->searchlight.states[0];
  iters = 3; // 3 iterations == three searchlights
  do
  {
    searchlight_movement(slstate);
    searchlight_caught(state, slstate);

    map_x = state->map_position.x; // in E
    map_y = state->map_position.y; // in D

    /* Would the searchlight intersect with the game window? */
    // Conv: Reordered into the expected series
    // first check is if the searchlight image is off the left hand side
    // second check is if the searchlight image is off the right hand side
    // etc.
    if (slstate->xy.x + 16 < map_x || slstate->xy.x >= map_x + state->columns ||
        slstate->xy.y + 16 < map_y || slstate->xy.y >= map_y + state->rows)
      goto next;

middle_bit:
    clip_left = 0; // flag // the light is clipped to the left side of the window

    // HL points to slstate->xy OR -> state->searchlight.caught_coord depending on how it's entered

    column = slstate->xy.x - map_x;
    if (column < 0) // partly off the left hand side of the window
      clip_left = ~clip_left; // toggle flag 0 -> 255

    row = slstate->xy.y - map_y;

    // FUTURE: To avoid undefined behaviour (row & column _can_ be out of
    // bounds) pass the row and column into searchlight_plot.

    attrs = &state->speccy->screen.attributes[0x46 + row * state->width + column]; // 0x46 = address of top-left game window attribute

    // Conv: clip_left turned from state variable into function parameter.
    searchlight_plot(state, attrs, clip_left); // DE turned into HL from EX above

next:
    // POP HL
    // POP BC
    slstate++;
  }
  while (--iters);
}

/**
 * $AE78: Is the hero is caught in the searchlight?
 *
 * \param[in] state   Pointer to game state.
 * \param[in] slstate Pointer to per-searchlight state. (was HL)
 */
void searchlight_caught(tgestate_t                   *state,
                        const searchlight_movement_t *slstate)
{
  uint8_t mappos_y, mappos_x; /* was D,E */
  uint8_t y, x;

  assert(state   != NULL);
  assert(slstate != NULL);

  /* Note that this code can get hit even when the hero is in bed... */

  mappos_y = state->map_position.y;
  mappos_x = state->map_position.x;
  x        = slstate->xy.x;
  y        = slstate->xy.y;

  if (x + 5 >= mappos_x + 12 || x + 10 <  mappos_x + 10 ||
      y + 5 >= mappos_y + 10 || y + 12 <= mappos_y + 6)
    return;

  /* It seems odd to not do this (cheaper) test sooner. */
  if (state->searchlight_state == searchlight_STATE_CAUGHT)
    return; /* already caught */

  state->searchlight_state = searchlight_STATE_CAUGHT;

  state->searchlight.caught_coord.y = slstate->xy.y;
  state->searchlight.caught_coord.x = slstate->xy.x;

  state->bell = bell_RING_PERPETUAL;

  decrease_morale(state, 10); /* was tail call */
}

/**
 * $AEB8: Searchlight plotter.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] attrs     Pointer to screen attributes (can be out of bounds!) (was DE)
 * \param[in] clip_left Searchlight is to the left of the main window. (was $AE75)
 */
void searchlight_plot(tgestate_t  *state,
                      attribute_t *attrs,
                      int          clip_left)
{
  /**
   * $AF3E: Searchlight circle shape.
   */
  static const uint8_t searchlight_shape[2 * 16] =
  {
    ________,________,
    ________,________,
    ________,________,
    _______X,X_______,
    _____XXX,XXX_____,
    ____XXXX,XXXX____,
    ____XXXX,XXXX____,
    ___XXXXX,XXXXX___,
    ___XXXXX,XXXXX___,
    ____XXXX,XXXX____,
    ____XXXX,XXXX____,
    _____XXX,XXX_____,
    _______X,X_______,
    ________,________,
    ________,________,
    ________,________
  };

  /* We can't ASSERT_SCREEN_ATTRIBUTES_PTR_VALID(attrs) here as expected as
   * the attrs pointer can be out of bounds... */

  attribute_t *const attrs_base = &state->speccy->screen.attributes[0];
  ASSERT_SCREEN_ATTRIBUTES_PTR_VALID(attrs_base);

{
  const uint8_t *shape;       /* was DE' */
  uint8_t        iters;       /* was C' */
  ptrdiff_t      x;           /* was A */
  attribute_t   *max_y_attrs; /* was HL */
  attribute_t   *saved_attrs; /* was stack */
  attribute_t   *min_y_attrs; /* was HL */
  uint8_t        iters2;      /* was B' */
  int            carry = 0;   /* added */

  /* Screen coords are x = 0..31, y = 0..23.
   * Game window occupies attribute rows 2..17 and columns 7..29 (inclusive).
   * Attribute row 18 is the row beyond the bottom edge of the game window.
   */

  // Conv: clip_left code was made a parameter, code handling it is hoisted.

  shape = &searchlight_shape[0];
  iters = 16; /* height */
  do
  {
    x = (attrs - attrs_base) % state->width; // Conv: was '& 31'. hoisted.

    // Finish if we're beyond the maximum y

    max_y_attrs = &attrs_base[18 * state->width]; // was HL = 0x5A40; // screen attribute address (column 0 + bottom of game screen)
    if (clip_left)
    {
      // BUG: This chunk seems to do nothing useful but allow the bottom
      // border to be overwritten. FUTURE: Remove.
      // Conv: was: if ((E & 31) >= 22) L = 32;
      if (x >= 22) // 22 => 8 attrs away from the edge maybe?
        max_y_attrs = &attrs_base[19 * state->width]; // row 19, column 0
    }
    if (attrs >= max_y_attrs) // gone off the end of the game screen?
      /* Conv: Original code just returned, instead goto exit so I can force
       * a screen refresh down there. */
      goto exit;

    saved_attrs = attrs; // PUSH DE

    // Clip/skip rows until we're in bounds

    min_y_attrs = &attrs_base[2 * state->width]; // screen attribute address (row 2, column 0)
    if (clip_left)
    {
      // BUG: Again, this chunk seems to do nothing useful but allow the
      // bottom border to be overwritten. FUTURE: Remove.
      // Conv: was: if (E & 31) >= 7) L = 32;
      if (x >= 7)
        min_y_attrs = &attrs_base[1 * state->width]; // row 1, column 0
    }
    if (attrs < min_y_attrs)
    {
      shape += 2;
      goto next_row;
    }

    iters2 = 2; /* iterations - rowbytes */
    do
    {
      uint8_t iters3; /* was B */
      uint8_t pixels; /* was C */

      pixels = *shape; // Conv: Original uses A as temporary.

      iters3 = 8; /* iterations - bits per byte */
      do
      {
        x = (attrs - attrs_base) % state->width;  // Conv: was '& 31'. was interleaved; '& 31's hoisted from below

        /* clip right hand edge */

        // i don't presently see any evidence that this chunk affects anything in the game, bcbw
        // does this stop us sliding into the next scanline of data?
        if (clip_left)
        {
          if (x >= 22)
            goto dont_plot; // this doesn't skip the whole remainder of the row unlike the case below
        }
        else
        {
          if (x >= 30) // Conv: Constant 30 was in E
          {
            shape += iters2; /* Conv: Was a loop. */
            goto next_row; // skip whole row
          }
        }

        /* clip left hand edge */

        if (x < 7) // Conv: Constant 7 was in D
        {
          /* Don't render leftwards of the game window */
dont_plot:
          RL(pixels); // no plot
        }
        else
        {
          // plot
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
  {
    /* FUTURE: Make the dirty rectangle more accurate. */
    static const zxbox_t dirty = { 7 * 8, 2 * 8, 29 * 8, 17 * 8 };
    state->speccy->draw(state->speccy, &dirty);
  }
}
}

/* ----------------------------------------------------------------------- */

/**
 * $AF8F: Test for characters meeting obstacles like doors and map bounds.
 *
 * Also assigns saved_mappos to specified vischar's pos and sets the
 * sprite_index.
 *
 * \param[in] state        Pointer to game state.
 * \param[in] vischar      Pointer to visible character. (was IY)
 * \param[in] sprite_index Flip flag + sprite offset. (was A')
 *
 * \return 0/1 => within/outwith bounds
 */
int touch(tgestate_t *state, vischar_t *vischar, spriteindex_t sprite_index)
{
  /* Conv: Removed $81AA stashed sprite index. */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert((sprite_index & ~sprite_FLAG_FLIP) < sprite__LIMIT);

  vischar->counter_and_flags |= vischar_BYTE7_DONT_MOVE_MAP | vischar_DRAWABLE;

  /* Conv: Removed little register shuffle to get (vischar & 0xFF) into A. */

  /* If hero is player controlled then check for door transitions. */
  if (vischar == &state->vischars[0] && state->automatic_player_counter > 0)
    door_handling(state, vischar);

  /* If a non-player character or hero when he's not cutting the fence. */
  if (vischar > &state->vischars[0] || ((state->vischars[0].flags & (vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE)) != vischar_FLAGS_CUTTING_WIRE))
    if (bounds_check(state, vischar))
      return 1; /* Outwith wall bounds. */

  /* Check "real" characters for collisions. */
  if (vischar->character <= character_25_PRISONER_6) /* character temp was in A' */
    if (collision(state))
      // there was a collision
      return 1;

  /* At this point we handle non-colliding characters and items only. */

  vischar->counter_and_flags &= ~vischar_BYTE7_DONT_MOVE_MAP; // clear
  vischar->mi.mappos          = state->saved_mappos.pos16;
  vischar->mi.sprite_index    = sprite_index; // left/right flip flag / sprite offset

  // A = 0; likely just to set flags
  return 0; // Z
}

/* ----------------------------------------------------------------------- */

/**
 * $AFDF: Handle collisions between vischars, including items being pushed
 *        around.
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0 if no collision, 1 if collision.
 */
int collision(tgestate_t *state)
{
  vischar_t  *vischar;        /* was HL */
  uint8_t     iters;          /* was B */
  uint16_t    u;              /* was BC */
  uint16_t    v;              /* was BC */
  uint16_t    saved_u;        /* was HL */
  uint16_t    saved_v;        /* was HL */
  int8_t      delta;          /* was A */
  uint8_t     coord;          /* was A */
  character_t character;      /* was A */
  uint8_t     range;          /* was B */
  uint8_t     centre;         /* was C */
  uint8_t     input;          /* was A */
  uint8_t     direction;      /* was A */
  uint16_t    new_direction;  /* was BC */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(state->IY);

  /* Iterate over characters being collided with (e.g. stove). */
  vischar = &state->vischars[0];
  iters = vischars_LENGTH;
  do
  {
    if (vischar->flags & vischar_FLAGS_NO_COLLIDE)
      goto next;

    /* Test for contact between the current vischar and saved_mappos (which is
     * set on entry either from a vischar or a characterstruct).
     *
     * For contact to occur the vischar being tested must be positioned
     * within (-4..+4) on the x and y axis and within (-23..23) on the height
     * axis.
     */

    u = vischar->mi.mappos.u; /* Conv: Moved +4 forward. */
    saved_u = state->saved_mappos.pos16.u;
    if (saved_u != u + 4) /* redundant check */
      if (saved_u > u + 4 || saved_u < u - 4) /* Conv: Removed redundant reload. */
        goto next; /* no x collision */

    v = vischar->mi.mappos.v; /* Conv: Moved +4 forward. */
    saved_v = state->saved_mappos.pos16.v;
    if (saved_v != v + 4) /* redundant check */
      if (saved_v > v + 4 || saved_v < v - 4) /* Conv: Removed redundant reload. */
        goto next; /* no y collision */

    /* Ensure that the heights are within 24 of each other. This will stop
     * the character colliding with any guards high up in watchtowers.
     * Note: Signed result. */
    delta = state->saved_mappos.pos16.w - vischar->mi.mappos.w;
    if (delta < 0)
      delta = -delta; /* absolute value */
    if (delta >= 24)
      goto next;

    /* Check for pursuit. */


    /* If vischar IY is pursuing... */
    if ((state->IY->flags & vischar_FLAGS_PURSUIT_MASK) == vischar_PURSUIT_PURSUE)
    {
      /* ...and the current vischar is the hero... */
      if (vischar == &state->vischars[0])
      {
        if (state->IY->character == state->bribed_character)
        {
          /* Vischar IY is a bribed character pursuing the hero.
           * When the pursuer catches the hero the bribe will be accepted. */
          accept_bribe(state);
        }
        else
        {
          /* Vischar IY is a hostile who's caught the hero! */
          /* Conv: Removed "HL = IY + 1" code which has no effect. */
          solitary(state);
          NEVER_RETURNS 0;
        }
      }
    }

    /*
     * Check for collisions with items.
     */

    /* If it's an item... */
    character = vischar->character;
    if (character >= character_26_STOVE_1)
    {
      uint16_t *pcoord; /* was HL */

      /* Movable items: Stove 1, Stove 2 or Crate */

      pcoord = &vischar->mi.mappos.v;

      /* By default, setup for the stove.
       * It can move on the Y axis only (bottom left to top right). */
      range = 7; /* permitted range from centre point */
      centre = 35; /* centre point (object will move from 29 to 42) */
      direction = state->IY->direction; /* was interleaved */
      if (character == character_28_CRATE)
      {
        /* Setup for the crate.
         * It can move on the X axis only (top left to bottom right). */
        pcoord--; /* point at vischar->mi.pos.x */
        centre = 54; /* centre point (object will move from 47 to 61) */
        direction ^= 1; /* swap axis: left<=>right */
      }

      switch (direction)
      {
        case direction_TOP_LEFT:
          /* The player is pushing the movable item from its front, so centre
           * it. */
          coord = *pcoord; /* Conv: Original code loads a byte here. */
          if (coord != centre)
          {
            /* Conv: This was the [-2]+1 pattern. */
            if (coord > centre)
              (*pcoord)--;
            else
              (*pcoord)++;
          }
          break;

        case direction_TOP_RIGHT:
          if (*pcoord != (centre + range))
            (*pcoord)++;
          break;

        case direction_BOTTOM_RIGHT:
          /* Note: This case happens in the game. */
          *pcoord = centre - range;
          break;

        case direction_BOTTOM_LEFT:
          if (*pcoord != (centre - range))
            (*pcoord)--;
          break;

        default:
          assert("Should not happen" == NULL);
          break;
      }
    }

    /*
     * Check for collisions with characters.
     */

    input = vischar->input & ~input_KICK;
    if (input)
    {
      assert(input < 9); // TODO: Add an input_XXX symbol for '9'

      /* Swap direction top <=> bottom, then compare. */
      if ((vischar->direction ^ 2) != state->IY->direction)
      {
        /* The characters collided while facing away from each other. */

        state->IY->input = input_KICK; /* force an update */

collided_set_delay:
        /* Delay calling character_behaviour() for five turns. This delay
         * controls how long it takes before a blocked character will try
         * another direction. */
        state->IY->counter_and_flags = (state->IY->counter_and_flags & ~vischar_BYTE7_COUNTER_MASK) | 5;
        return 1; /* collided */
      }
    }

    /* Pick a new direction for the vischar at state->IY. */
    {
      /** $B0F8: Inputs which move the character in the next anticlockwise
       *         direction. */
      static const uint8_t new_inputs[4] =
      {
        input_DOWN + input_LEFT  + input_KICK, /* if facing TL, move D+L */
        input_UP   + input_LEFT  + input_KICK, /* if facing TR, move U+L */
        input_UP   + input_RIGHT + input_KICK, /* if facing BR, move U+R */
        input_DOWN + input_RIGHT + input_KICK  /* if facing BL, move D+R */
      };

      /* BUG FIX: Mask 'direction' so that we don't access out-of-bounds in
       * new_inputs[] if we collide when crawling. */
      new_direction = state->IY->direction & vischar_DIRECTION_MASK;
      assert(new_direction < 4);
      state->IY->input = new_inputs[new_direction];
      assert((state->IY->input & ~input_KICK) < 9);
      if ((new_direction & 1) == 0) /* if was facing TL or BR */
        state->IY->counter_and_flags &= ~vischar_BYTE7_V_DOMINANT;
      else
      state->IY->counter_and_flags |= vischar_BYTE7_V_DOMINANT;
    }

    goto collided_set_delay;

next:
    vischar++;
  }
  while (--iters);

  return 0; /* no collision */
}

/* ----------------------------------------------------------------------- */

/**
 * $B107: A friendly character is taking a bribe from the hero.
 *
 * \param[in] state Pointer to game state.
 */
void accept_bribe(tgestate_t *state)
{
  route_t    *route;  /* was HL */
  item_t    *item;    /* was DE */
  uint8_t    iters;   /* was B */
  vischar_t *vischar; /* was HL */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(state->IY);

  increase_morale_by_10_score_by_50(state);

  /* Clear the vischar_PURSUIT_PURSUE flag. */
  state->IY->flags = 0;

  route = &state->IY->route;
  get_target_assign_pos(state, state->IY, route);

  /* Return early if we have no bribes. */
  item = &state->items_held[0];
  if (*item != item_BRIBE && *++item != item_BRIBE)
    return;

  /* Remove the bribe item. */
  *item = item_NONE;
  state->item_structs[item_BRIBE].room_and_flags = (room_t) itemstruct_ROOM_NONE;
  draw_all_items(state);

  /* Set the vischar_PURSUIT_SAW_BRIBE flag on all visible hostiles. */
  iters   = vischars_LENGTH - 1;
  vischar = &state->vischars[1];
  do
  {
    if (vischar->character <= character_19_GUARD_DOG_4)
      vischar->flags = vischar_PURSUIT_SAW_BRIBE;
    vischar++;
  }
  while (--iters);

  queue_message(state, message_HE_TAKES_THE_BRIBE);
  queue_message(state, message_AND_ACTS_AS_DECOY);
}

/* ----------------------------------------------------------------------- */

/**
 * $B14C: Affirms that the specified character is touching/inside of
 * wall/fence bounds.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 *
 * \return 1 => pos hits bounds, 0 => pos doesn't hit bounds
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

    if (state->saved_mappos.pos16.u >= minx + 2   &&
        state->saved_mappos.pos16.u <  maxx + 4   &&
        state->saved_mappos.pos16.v >= miny       &&
        state->saved_mappos.pos16.v <  maxy + 4   &&
        state->saved_mappos.pos16.w >= minheight  &&
        state->saved_mappos.pos16.w <  maxheight + 2)
    {
      vischar->counter_and_flags ^= vischar_BYTE7_V_DOMINANT;
      return 1; // NZ => outwith wall bounds
    }

    wall++;
  }
  while (--iters);

  return 0; // Z => within wall bounds
}

/* ----------------------------------------------------------------------- */

/* Conv: $B1C7/multiply_by_8 was removed. */

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

  cur   = state->current_door & ~door_REVERSE;
  door  = &state->locked_doors[0];
  iters = NELEMS(state->locked_doors);
  do
  {
    if ((*door & ~door_LOCKED) == cur)
    {
      if ((*door & door_LOCKED) == 0)
        return 0; /* Door is open. */

      queue_message(state, message_THE_DOOR_IS_LOCKED);
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
    door_handling_interior(state, vischar); /* was tail call */
    return;
  }

  /* Select a start position in doors[] based on the direction the hero is
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
    if ((door_pos->room_and_direction & door_FLAGS_MASK_DIRECTION) == direction)
      if (door_in_range(state, door_pos) == 0)
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

  vischar->room = (door_pos->room_and_direction & ~door_FLAGS_MASK_DIRECTION) >> 2; // sampled HL = $792E (in doors[])
  ASSERT_ROOM_VALID(vischar->room);
  if ((door_pos->room_and_direction & door_FLAGS_MASK_DIRECTION) < direction_BOTTOM_RIGHT) /* TL or TR */
    /* Point to the next door's pos */
    transition(state, &door_pos[1].mappos);
  else
    /* Point to the previous door's pos */
    transition(state, &door_pos[-1].mappos);

  NEVER_RETURNS; // highly likely if this is only tiggered by the hero
}

/* ----------------------------------------------------------------------- */

/**
 * $B252: Test whether an exterior door is in range.
 *
 * A door is in range if (saved_X,saved_Y) is within (-3,+2) of its position
 * (once scaled).
 *
 * \param[in] state Pointer to game state.
 * \param[in] door  Pointer to door_t. (was HL)
 *
 * \return Zero if door is in range, non-zero otherwise. (was carry flag)
 */
int door_in_range(tgestate_t *state, const door_t *door)
{
  const int halfdist = 3;

  uint16_t u, v; /* was BC, BC */

  assert(state != NULL);
  assert(door  != NULL);

  u = door->mappos.u * 4;
  if (state->saved_mappos.pos16.u < u - halfdist || state->saved_mappos.pos16.u >= u + halfdist)
    return 1;

  v = door->mappos.v * 4;
  if (state->saved_mappos.pos16.v < v - halfdist || state->saved_mappos.pos16.v >= v + halfdist)
    return 1;

  return 0;
}

/* ----------------------------------------------------------------------- */

/* Conv: $B295/multiply_by_4 was inlined. */

/* ----------------------------------------------------------------------- */

/**
 * $B29F: Check the character is inside of bounds, when indoors.
 *
 * Leaf.
 *
 * Position to be checked is passed in in state->saved_mappos.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 *
 * \return 1 => pos hits bounds, 0 => pos doesn't hit bounds
 */
int interior_bounds_check(tgestate_t *state, vischar_t *vischar)
{
  /* Like a bounds_t but the elements are in a wacky order. */
  // FUTURE: Re-order the structure members into a bounds_t.
  typedef struct wackybounds
  {
    uint8_t x1, x0, y1, y0;
  }
  wackybounds_t;

  /**
   * $6B85: Room dimensions.
   */
  static const wackybounds_t roomdef_dimensions[10] =
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

  const wackybounds_t *room_bounds;   /* was BC */
  const mappos16_t    *saved_mappos;  /* was HL */
  const bounds_t      *object_bounds; /* was HL */
  uint8_t              nbounds;       /* was B */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  room_bounds = &roomdef_dimensions[state->roomdef_dimensions_index];
  saved_mappos = &state->saved_mappos.pos16;
  /* Conv: Merged conditions, flipped compares and reordered. */
  // inclusive/exclusive bounds look strange here
  if (saved_mappos->u <= room_bounds->x0 + 4 || saved_mappos->u > room_bounds->x1 ||
      saved_mappos->v <= room_bounds->y0     || saved_mappos->v > room_bounds->y1 - 4)
  {
    goto hit_bounds;
  }

  object_bounds = &state->roomdef_object_bounds[0]; /* Conv: Moved around. */
  for (nbounds = state->roomdef_object_bounds_count; nbounds > 0; nbounds--)
  {
    mappos16_t *mappos; /* was DE */
    uint8_t     u, v;   /* was A, A */

    /* Conv: HL dropped. */
    mappos = &state->saved_mappos.pos16;

    /* Conv: Two-iteration loop unrolled. */
    u = mappos->u; // note: narrowing // saved_mappos must be 8-bit
    if (u < object_bounds->x0 || u >= object_bounds->x1)
      goto next;
    v = mappos->v; // note: narrowing
    if (v < object_bounds->y0 || v >= object_bounds->y1)
      goto next;

hit_bounds:
    /* Toggle movement direction preference. */
    vischar->counter_and_flags ^= vischar_BYTE7_V_DOMINANT;
    return 1; /* return NZ */

next:
    /* Next iteration. */
    object_bounds++;
  }

  /* Encountered no bounds. */
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
  calc_vischar_isopos_from_vischar(state, &state->vischars[0]);

  /* Centre the map position on the hero. */
  /* Conv: Removed divide_by_8 calls here. */
  state->map_position.x = (state->vischars[0].isopos.x >> 3) - 11; // 11 would be screen width minus half of character width?
  state->map_position.y = (state->vischars[0].isopos.y >> 3) - 6;  // 6 would be screen height minus half of character height?
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
  const mappos8_t *doormappos;     /* was HL' */
  mappos16_t      *mappos;         /* was DE' */
  uint8_t          u;              /* was A */
  uint8_t          v;              /* was A */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  for (pdoors = &state->interior_doors[0]; ; pdoors++)
  {
    current_door = *pdoors;
    if (current_door == interiordoor_NONE)
      return; /* Reached end of list. */

    state->current_door = current_door;

    door = get_door(current_door);
    room_and_flags = door->room_and_direction;

    /* Does the character face the same direction as the door? */
    if ((vischar->direction & vischar_DIRECTION_MASK) != (room_and_flags & door_FLAGS_MASK_DIRECTION))
      continue;

    /* Skip any door which is more than three units away. */
    doormappos = &door->mappos;
    mappos = &state->saved_mappos.pos16; // reusing saved_mappos as 8-bit here? a case for saved_mappos being a union of pos8 and pos types?
    u = doormappos->u;
    if (u - 3 >= mappos->u || u + 3 < mappos->u) // -3 .. +2
      continue;
    v = doormappos->v;
    if (v - 3 >= mappos->v || v + 3 < mappos->v) // -3 .. +2
      continue;

    if (is_door_locked(state))
      return; /* The door was locked. */

    vischar->room = room_and_flags >> 2;

    doormappos = &door[1].mappos;
    if (state->current_door & door_REVERSE)
      doormappos = &door[-1].mappos;

    transition(state, doormappos); /* was tail call */
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

  queue_message(state, message_YOU_OPEN_THE_BOX);
  increase_morale_by_10_score_by_50(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B3A8: The hero tries to bribe a prisoner.
 *
 * This searches visible friendly characters only, returning the first one
 * found. It then makes that character pursue the hero to accept the bribe.
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
  vischar->flags = vischar_PURSUIT_PURSUE; /* Bribed character pursues hero. */
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

  if (get_roomdef(state, room_50_BLOCKED_TUNNEL, roomdef_50_BOUNDARY) == 255)
    return; /* Blockage is already cleared. */

  /* Release boundary. */
  set_roomdef(state,
              room_50_BLOCKED_TUNNEL,
              roomdef_50_BOUNDARY,
              255);
  /* Remove blockage graphic. */
  set_roomdef(state,
              room_50_BLOCKED_TUNNEL,
              roomdef_50_BLOCKAGE,
              interiorobject_STRAIGHT_TUNNEL_SW_NE);

  // FUTURE: setup_room_and_plot could replace the setup_room and plot_interior_tiles calls
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
  const wall_t    *wall;   /* was HL */
  const mappos8_t *mappos; /* was DE */
  uint8_t          iters;  /* was B */
  uint8_t          flag;   /* was A */

  assert(state != NULL);

  wall = &walls[12]; /* start of (vertical) fences */
  mappos = &state->hero_mappos;
  iters = 4;
  do
  {
    uint8_t coord; /* was A */

    // check: this is using x then y which is the wrong order, isn't it?
    coord = mappos->v;
    if (coord >= wall->miny && coord < wall->maxy) /* Conv: Reversed test order. */
    {
      coord = mappos->u;
      if (coord == wall->maxx)
        goto snips_crawl_tl;
      if (coord - 1 == wall->maxx)
        goto snips_crawl_br;
    }

    wall++;
  }
  while (--iters);

  wall = &walls[12 + 4]; /* start of (horizontal) fences */
  mappos = &state->hero_mappos;
  iters = 3;
  do
  {
    uint8_t coord; /* was A */

    coord = mappos->u;
    if (coord >= wall->minx && coord < wall->maxx)
    {
      coord = mappos->v;
      if (coord == wall->miny)
        goto snips_crawl_tr;
      if (coord - 1 == wall->miny)
        goto snips_crawl_bl;
    }

    wall++;
  }
  while (--iters);

  return;

snips_crawl_tl: /* Crawl TL */
  flag = direction_TOP_LEFT | vischar_DIRECTION_CRAWL;
  goto action_wiresnips_tail;

snips_crawl_tr: /* Crawl TR */
  flag = direction_TOP_RIGHT | vischar_DIRECTION_CRAWL;
  goto action_wiresnips_tail;

snips_crawl_br: /* Crawl BR */
  flag = direction_BOTTOM_RIGHT | vischar_DIRECTION_CRAWL;
  goto action_wiresnips_tail;

snips_crawl_bl: /* Crawl BL */
  flag = direction_BOTTOM_LEFT | vischar_DIRECTION_CRAWL;

action_wiresnips_tail:
  state->vischars[0].direction   = flag; // dir + walk/crawl flag
  state->vischars[0].input       = input_KICK;
  state->vischars[0].flags       = vischar_FLAGS_CUTTING_WIRE;
  state->vischars[0].mi.mappos.w = 12; // crawling height
  state->vischars[0].mi.sprite   = &sprites[sprite_PRISONER_FACING_AWAY_1];
  state->player_locked_out_until = state->game_counter + 96;
  queue_message(state, message_CUTTING_THE_WIRE);
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
  queue_message(state, message_PICKING_THE_LOCK);
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

  queue_message(state, message);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4D0: Return the door in range of the hero.
 *
 * The hero's position is expected in state->saved_mappos.
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
  mappos16_t   *mappos;               /* was DE' */
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
    iters = 8; // BUG: iters should be 7
    do
    {
      locked_door_index = *locked_doors & ~door_LOCKED;

      /* Search interior_doors[] for door_index. */
      for (interior_doors = &state->interior_doors[0]; ; interior_doors++)
      {
        interior_door_index = *interior_doors;
        if (interior_door_index == interiordoor_NONE)
          break; /* end of list */

        if ((interior_door_index & ~door_REVERSE) == locked_door_index) // this must be door_REVERSE as it came from interior_doors[]
          goto found;

        /* There's no termination condition here where you might expect a
         * test to see if we've run out of doors, but since every room has
         * at least one door it *must* find one. */
      }

next_locked_door:
      locked_doors++;
    }
    while (--iters);

    return NULL; /* Not found */


    // FUTURE: Move this into the body of the loop.
found:
    door = get_door(*interior_doors);
    /* Range check pattern (-2..+3). */
    mappos = &state->saved_mappos.pos16; // note: 16-bit values holding 8-bit values
    // Conv: Unrolled.
    if (mappos->u <= door->mappos.u - 3 || mappos->u > door->mappos.u + 3 ||
        mappos->v <= door->mappos.v - 3 || mappos->v > door->mappos.v + 3)
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

  /* vertical fences */
  {  70,  70,  70, 106, 0,  8 }, // 12: fence - right of main camp
  {  62,  62,  62, 106, 0,  8 }, // fence - rightmost fence
  {  78,  78,  46,  62, 0,  8 }, // fence - rightmost of yard
  { 104, 104,  46,  69, 0,  8 }, // fence - leftmost of yard

  /* horizontal fences */
  {  62, 104,  62,  62, 0,  8 }, // 16: fence - top of yard
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
 * $B5CE: Animates all visible characters.
 *
 * \param[in] state Pointer to game state.
 */
void animate(tgestate_t *state)
{
  /* The most common animations use the forward case, but the game will also
   * play some animations backwards when required. For example when crawling
   * in straight tunnels the hero cannot turn around. Moving in the opposite
   * direction doesn't turn the character around but instead plays the same
   * crawl animation but in reverse mode.
   */

  /**
   * $CDAA: Maps a character's direction and user input to an animation index
   * and reverse flag.
   *
   * The horizontal axis is input (U/D/L/R = Up/Down/Left/Right)
   * and the vertical axis is direction (TL/TR/BR/BL + crawl flag).
   *
   * For example:
   * if (direction,input) is (TL, None) then index is 8 which is anim_wait_tl.
   * if (direction,input) is (TR + Crawl, D+L) then index is 21 which is anim_crawlwait_tr.
   *
   * Modulo the reverse flag the highest index in the table is 23 which is
   * the max index in animations[] (== animations__LIMIT - 1).
   */
  static const uint8_t animindices[8 /*direction*/][9 /*input*/] =
  {
#define F (0 << 7) /* Top bit set: animation plays forwards. */
#define R (1 << 7) /* Top bit clear: animation plays in reverse. */
    /* None,   U,    D,    L,  U+L,  D+L,    R,  U+R,  D+R */
    {  8|F,  0|F,  4|F,  7|R,  0|F,  7|R,  4|F,  4|F,  4|F }, /* TL */
    {  9|F,  4|R,  5|F,  5|F,  4|R,  5|F,  1|F,  1|F,  5|F }, /* TR */
    { 10|F,  5|R,  2|F,  6|F,  5|R,  6|F,  5|R,  5|R,  2|F }, /* BR */
    { 11|F,  7|F,  6|R,  3|F,  7|F,  3|F,  7|F,  7|F,  6|R }, /* BL */
    { 20|F, 12|F, 12|R, 19|R, 12|F, 19|R, 16|F, 16|F, 12|R }, /* TL + Crawl */
    { 21|F, 16|R, 17|F, 13|R, 16|R, 21|R, 13|F, 13|F, 17|F }, /* TR + Crawl */
    { 22|F, 14|R, 14|F, 18|F, 14|R, 14|F, 17|R, 17|R, 14|F }, /* BR + Crawl */
    { 23|F, 19|F, 18|R, 15|F, 19|F, 15|F, 15|R, 15|R, 18|R }, /* BL + Crawl */
  };

  uint8_t            iters;         /* was B */
  const anim_t      *animA;         /* was HL */
  const animframe_t *frameA;        /* was HL */
  uint8_t            animindex;     /* was A */
  spriteindex_t      spriteindex;   /* was A */
  uint8_t            newanimindex;  /* was C */
  uint8_t            length;        /* was C */
  const anim_t      *animB;         /* was DE */
  const animframe_t *frameB;        /* was DE */
  spriteindex_t      spriteindex2;  /* was A' */
  vischar_t         *vischar;       /* a cache of IY (Conv: added) */

  assert(state != NULL);

  frameA = frameB = NULL; // Conv: init
  spriteindex2 = 0; // Conv: initialise

  /* Animate all vischars. */
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

    animA = vischar->anim;
    animindex = vischar->animindex;
    if (animindex & vischar_ANIMINDEX_REVERSE)
    {
      /* Reached end of animation? */
      /* BUG FIX: Checks for 0x7F, not 0. */
      animindex &= ~vischar_ANIMINDEX_REVERSE;
      if (animindex == 0x7F) /* i.e. -1 */
        goto init;

      assert(animindex < animA->nframes);

      frameA = &animA->frames[animindex];
      spriteindex = frameA->spriteindex;

      SWAP(uint8_t, spriteindex, spriteindex2);

backwards:
      SWAP(const animframe_t *, frameB, frameA);

      state->saved_mappos.pos16.u = vischar->mi.mappos.u - frameB->dx;
      state->saved_mappos.pos16.v = vischar->mi.mappos.v - frameB->dy;
      state->saved_mappos.pos16.w = vischar->mi.mappos.w - frameB->dh;

      if (touch(state, vischar, spriteindex2))
        goto pop_next; /* don't animate if collided */

      /* Conv: Preserve reverse flag. */
      vischar->animindex = (vischar->animindex - 1) | vischar_ANIMINDEX_REVERSE;
    }
    else
    {
      /* Reached end of animation? */
      if (animindex == animA->nframes)
        goto init;

      assert(animindex < animA->nframes);

      frameA = &animA->frames[animindex];

forwards:
      SWAP(const animframe_t *, frameB, frameA);

      state->saved_mappos.pos16.u = vischar->mi.mappos.u + frameB->dx;
      state->saved_mappos.pos16.v = vischar->mi.mappos.v + frameB->dy;
      state->saved_mappos.pos16.w = vischar->mi.mappos.w + frameB->dh;
      spriteindex = frameB->spriteindex;

      SWAP(uint8_t, spriteindex, spriteindex2);

      if (touch(state, vischar, spriteindex2))
        goto pop_next; /* don't animate if collided */

      vischar->animindex++;
    }

    calc_vischar_isopos_from_state(state, vischar);

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
  /* fall through */
init:
  assert(vischar->input < 9);
  assert(vischar->direction < 8);
  newanimindex = animindices[vischar->direction][vischar->input];
  /* Conv: Original game uses ADD A,A to double A and in doing so discards
   * the top flag bit. We mask it off instead. */
  assert((newanimindex & ~R) < 24);
  animB = vischar->animbase[newanimindex & ~R]; /* animbase is always &animations[0] */
  vischar->anim = animB;
  if ((newanimindex & R) == 0)
  {
    vischar->animindex = 0;
    vischar->direction = animB->to;
    frameB = &animB->frames[0]; // theoretically a no-op
    SWAP(const animframe_t *, frameB, frameA);
    goto forwards;
  }
  else
  {
    const animframe_t *stacked;

    length = animB->nframes;
    /* BUG FIX: (length - 1) used here, not length. */
    vischar->animindex = (length - 1) | vischar_ANIMINDEX_REVERSE;
    vischar->direction = animB->from;
    /* BUG FIX: Final frame used here, not first. */
    frameB = &animB->frames[length - 1];
    stacked = frameB;
    SWAP(const animframe_t *, frameB, frameA);
    spriteindex = animB->frames[length - 1].spriteindex;
    SWAP(spriteindex_t, spriteindex, spriteindex2);
    assert(frameA == stacked);
    frameA = stacked;
    goto backwards;
  }

#undef R
#undef F
}

/* ----------------------------------------------------------------------- */

/**
 * $B71B: Calculate screen position for the specified vischar from mi.pos.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was HL)
 */
void calc_vischar_isopos_from_vischar(tgestate_t *state, vischar_t *vischar)
{
  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  /* Save a copy of the vischar's position + offset. */
  state->saved_mappos.pos16 = vischar->mi.mappos;

  calc_vischar_isopos_from_state(state, vischar);
}

/**
 * $B729: Calculate screen position for the specified vischar from
 * state->saved_mappos.
 *
 * Result has three bits of fixed point accuracy.
 *
 * \see drop_item_tail_interior.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was HL)
 */
void calc_vischar_isopos_from_state(tgestate_t *state, vischar_t *vischar)
{
  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  /* Conv: Reordered. */
  vischar->isopos.x = (0x200 - state->saved_mappos.pos16.u + state->saved_mappos.pos16.v) * 2;
  vischar->isopos.y = 0x800 - state->saved_mappos.pos16.u - state->saved_mappos.pos16.v - state->saved_mappos.pos16.w;

  // Measured: (-1664..3696, -1648..2024)
  // Rounding up:
  assert((int16_t) vischar->isopos.x >= -3000);
  assert((int16_t) vischar->isopos.x <   7000);
  assert((int16_t) vischar->isopos.y >= -3000);
  assert((int16_t) vischar->isopos.y <   4000);
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

  /* BUG FIX: Reset bribed character index. */
  state->bribed_character = character_NONE;

  /* BUG FIX: Reset position of stoves and crate. */
  state->movable_items[0].mappos.u = 62;
  state->movable_items[0].mappos.v = 35;
  state->movable_items[0].mappos.w = 16;
  state->movable_items[1].mappos.u = 55;
  state->movable_items[1].mappos.v = 54;
  state->movable_items[1].mappos.w = 14;
  state->movable_items[2].mappos.u = 62;
  state->movable_items[2].mappos.v = 35;
  state->movable_items[2].mappos.w = 16;

  enter_room(state); // returns by goto main_loop
  NEVER_RETURNS;
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
    room_t      room;
    mappos8uv_t mappos;
  }
  character_reset_partial_t;

  /**
   * $B819: Partial character structs for reset.
   */
  static const character_reset_partial_t character_reset_data[10] =
  {
    { room_3_HUT2RIGHT, { 40, 60 } }, /* for character 12 Guard */
    { room_3_HUT2RIGHT, { 36, 48 } }, /* for character 13 Guard */
    { room_5_HUT3RIGHT, { 40, 60 } }, /* for character 14 Guard */
    { room_5_HUT3RIGHT, { 36, 34 } }, /* for character 15 Guard */
    { room_NONE,        { 52, 60 } }, /* for character 20 Prisoner */
    { room_NONE,        { 52, 44 } }, /* for character 21 Prisoner */
    { room_NONE,        { 52, 28 } }, /* for character 22 Prisoner */
    { room_NONE,        { 52, 60 } }, /* for character 23 Prisoner */
    { room_NONE,        { 52, 44 } }, /* for character 24 Prisoner */
    { room_NONE,        { 52, 28 } }, /* for character 25 Prisoner */
  };

  uint8_t                          iters;   /* was B */
  vischar_t                       *vischar; /* was HL */
  uint8_t                         *gate;    /* was HL */
  const roomdef_address_t         *bed;     /* was HL */
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
  set_roomdef(state,
              room_50_BLOCKED_TUNNEL,
              roomdef_50_BLOCKAGE,
              interiorobject_COLLAPSED_TUNNEL_SW_NE);
  set_roomdef(state,
              room_50_BLOCKED_TUNNEL,
              roomdef_50_BOUNDARY,
              52); /* Reset boundary. */

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
  {
    set_roomdef(state,
                bed->room_index,
                bed->offset,
                interiorobject_OCCUPIED_BED);
    bed++;
  }
  while (--iters);

  /* Clear the mess halls. */
  set_roomdef(state, room_23_MESS_HALL, roomdef_23_BENCH_A, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_23_MESS_HALL, roomdef_23_BENCH_B, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_23_MESS_HALL, roomdef_23_BENCH_C, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_25_MESS_HALL, roomdef_25_BENCH_D, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_25_MESS_HALL, roomdef_25_BENCH_E, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_25_MESS_HALL, roomdef_25_BENCH_F, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_25_MESS_HALL, roomdef_25_BENCH_G, interiorobject_EMPTY_BENCH);

  /* Reset characters 12..15 (guards) and 20..25 (prisoners). */
  charstr = &state->character_structs[character_12_GUARD_12];
  iters2 = NELEMS(character_reset_data);
  reset = &character_reset_data[0];
  do
  {
    charstr->room        = reset->room;
    charstr->mappos.u    = reset->mappos.u;
    charstr->mappos.v    = reset->mappos.v;
    charstr->mappos.w    = 18; /* BUG: This is reset to 18 but the initial data is 24. (hex/dec mixup?) */
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
 * $B83B: Check the mask buffer to see if the hero is hiding behind something.
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
  /* BUG: Original code does a fused load of BC, but doesn't use C after.
   * Probably a leftover stride constant for MASK_BUFFER_WIDTHBYTES. */
  // C = 4;
  do
  {
    if (*buf != 0)
      goto still_in_searchlight;
    buf += MASK_BUFFER_WIDTHBYTES;
  }
  while (--iters);

  /* Otherwise the hero has escaped the searchlight, so decrement the
   * counter. */
  if (--state->searchlight_state == searchlight_STATE_SEARCHING)
  {
    attrs = choose_game_window_attributes(state);
    set_game_window_attributes(state, attrs);
  }

  return;

still_in_searchlight:
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
  int           visible;    /* was Z */

  assert(state != NULL);

  for (;;)
  {
    /* This can return a vischar OR an itemstruct, but not both. */
    found = get_next_drawable(state, &index, &vischar, &itemstruct);
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
      visible = setup_vischar_plotting(state, vischar);
      if (visible)
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
      visible = setup_item_plotting(state, itemstruct, index);
      if (visible)
      {
        render_mask_buffer(state);
        masked_sprite_plotter_16_wide_item(state);
      }
    }
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B89C: Find the next vischar or itemstruct to draw.
 *
 * \param[in]  state       Pointer to game state.
 * \param[out] pindex      Returns (vischars_LENGTH - iters) if vischar,
 *                         or ((item__LIMIT - iters) | (1 << 6)) if itemstruct. (was A)
 * \param[out] pvischar    Pointer to receive vischar pointer. (was IY)
 * \param[out] pitemstruct Pointer to receive itemstruct pointer. (was IY)
 *
 * \return Non-zero if if a valid vischar or item was returned.
 */
int get_next_drawable(tgestate_t    *state,
                      uint8_t       *pindex,
                      vischar_t    **pvischar,
                      itemstruct_t **pitemstruct)
{
  uint16_t      prev_u;           /* was BC */
  uint16_t      prev_v;           /* was DE */
  item_t        item_and_flag;    /* was A' */
  uint8_t       iters;            /* was B' */
  vischar_t    *vischar;          /* was HL' */
  vischar_t    *found_vischar;    /* was IY */
  itemstruct_t *found_itemstruct; /* was IY */

  assert(state       != NULL);
  assert(pindex      != NULL);
  assert(pvischar    != NULL);
  assert(pitemstruct != NULL);

  /* Conv: Quiet MS Visual Studio warning */
  found_vischar = NULL;

  /* Conv: Added */
  *pvischar    = NULL;
  *pitemstruct = NULL;

  prev_u        = 0; /* previous x */
  prev_v        = 0; /* previous y */
  item_and_flag = item_NONE; /* item_NONE used as a 'nothing found' marker */

  /* Conv: The original code maintains a previous height variable (in DE'), but
   * it's ultimately never used, so has been removed here. */

  /* Find the rearmost vischar that is flagged for drawing. */

  iters   = vischars_LENGTH; /* iterations */
  vischar = &state->vischars[0]; /* Conv: Original code points at vischar[0].counter_and_flags */
  do
  {
    /* Select a vischar if it's behind the point (prev_x - 4, prev_y - 4). */
    if ((vischar->counter_and_flags & vischar_DRAWABLE) &&
        (vischar->mi.mappos.u >= prev_u - 4) &&
        (vischar->mi.mappos.v >= prev_v - 4))
    {
      item_and_flag = vischars_LENGTH - iters; /* Vischar index (never usefully used). */

      /* Note: The (v,u) order here matches the original code */
      prev_v = vischar->mi.mappos.v;
      prev_u = vischar->mi.mappos.u;
      state->IY = found_vischar = vischar;
    }
    vischar++;
  }
  while (--iters);

  /* Is there an item behind the selected vischar? */
  item_and_flag = get_next_drawable_itemstruct(state,
                                               item_and_flag,
                                               prev_u, prev_v,
                                              &found_itemstruct);

  *pindex = item_and_flag; /* Conv: Added */
  /* If the topmost bit of item_and_flag remains set from initialisation,
   * then no vischar was found. item_and_flag is preserved by the call to
   * get_greatest_itemstruct. */
  if (item_and_flag & (1 << 7))
  {
    return 0; /* not found */
  }
  else if ((item_and_flag & item_FOUND) == 0)
  {
    /* Vischar found */

    found_vischar->counter_and_flags &= ~vischar_DRAWABLE;

    *pvischar = found_vischar; /* Conv: Added */

    return 1; /* found */
  }
  else
  {
    /* Item found */

    int Z;

    found_itemstruct->room_and_flags &= ~itemstruct_ROOM_FLAG_NEARBY_6;
    Z = (found_itemstruct->room_and_flags & itemstruct_ROOM_FLAG_NEARBY_6) == 0; // was BIT 6,HL[1] // Odd: This tests the bit we've just cleared.

    *pitemstruct = found_itemstruct; /* Conv: Added */

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

    iters = NELEMS(exterior_mask_data); /* BUG FIX: Was 59 (should be 58). */
    pmask = &exterior_mask_data[0]; /* Conv: Original game points at pmask->bounds.x1. Fixed by propagation. */
  }

{
  uint8_t        A;                   /* was A */
  const uint8_t *mask_pointer;        /* was DE */
  uint16_t       mask_skip;           /* was HL */
  uint8_t       *mask_buffer_pointer; /* was $81A0 */

  /* Fill the mask buffer with the union of all matching masks. */
  do
  {
    uint8_t isopos_x, isopos_y, height; /* was A/C, A, A */
    uint8_t mask_left_skip;             /* was $B837 */
    uint8_t mask_top_skip;              /* was $B838 */
    uint8_t mask_run_height;            /* was $B839 */
    uint8_t mask_run_width;             /* was $B83A */
    uint8_t mask_row_skip;              /* was $BA90 - self modified */
    uint8_t buf_row_skip;               /* was $BABA - self modified */

    /* Skip any masks which don't overlap the character.
     *
     * pmask->bounds is a visual position on the map image (isometric map space)
     * pmask->pos is a map position (map space)
     * so we can cull masks if not on-screen and we can cull masks if behind play
     */

    isopos_x = state->isopos.x;
    isopos_y = state->isopos.y; /* Conv: Reordered */
    if (isopos_x - 1 >= pmask->bounds.x1 || isopos_x + 3 < pmask->bounds.x0 ||
        isopos_y - 1 >= pmask->bounds.y1 || isopos_y + 4 < pmask->bounds.y0)
      goto pop_next;

    /* CHECK: What's in state->mappos_stash at this point? It's set by
     * setup_vischar_plotting, setup_item_plotting. */

    if (state->mappos_stash.u <= pmask->mappos.u ||
        state->mappos_stash.v <  pmask->mappos.v)
      goto pop_next;

    height = state->mappos_stash.w;
    if (height)
      height--; /* make inclusive */
    if (height >= pmask->mappos.w)
      goto pop_next;

    /* The mask is valid: now work out clipping offsets, widths and heights. */

    /* Conv: Removed dupe of earlier load of isopos_x. */
    if (isopos_x >= pmask->bounds.x0)
    {
      mask_left_skip  = isopos_x - pmask->bounds.x0; /* left edge skip for mask */
      mask_run_width  = MIN(pmask->bounds.x1 - isopos_x, 3) + 1;
    }
    else
    {
      /* Conv: Removed pmask->bounds.x0 cached in x0 (was B). */
      mask_left_skip  = 0; /* no left edge skip */
      mask_run_width  = MIN((pmask->bounds.x1 - pmask->bounds.x0) + 1, 4 - (pmask->bounds.x0 - isopos_x));
    }

    /* Conv: Removed dupe of earlier load of isopos_y. */
    if (isopos_y >= pmask->bounds.y0)
    {
      mask_top_skip   = isopos_y - pmask->bounds.y0; /* top edge skip for mask */
      mask_run_height = MIN(pmask->bounds.y1 - isopos_y, 4) + 1;
    }
    else
    {
      /* Conv: Removed pmask->bounds.y0 cached in x0 (was B). */
      mask_top_skip   = 0; /* no top edge skip */
      mask_run_height = MIN((pmask->bounds.y1 - pmask->bounds.y0) + 1, 5 - (pmask->bounds.y0 - isopos_y));
    }

    /* Note: In the original code, HL is here decremented to point at member
     *       y0. */

    /* Calculate the initial mask buffer pointer. */
    {
      uint8_t buf_top_skip, buf_left_skip; /* was C, B */
      uint8_t index;                       /* was A */

      /* When the mask has a top or left hand gap, calculate that. */
      buf_top_skip = buf_left_skip = 0;
      if (mask_top_skip == 0)
        buf_top_skip  = pmask->bounds.y0 - state->isopos.y; /* FUTURE: Could avoid this reload of isopos.y. */
      if (mask_left_skip == 0)
        buf_left_skip = pmask->bounds.x0 - state->isopos.x;

      index = pmask->index;
      assert(index < NELEMS(mask_pointers));

      mask_buffer_pointer = &state->mask_buffer[buf_top_skip * MASK_BUFFER_ROWBYTES + buf_left_skip];

      mask_pointer = mask_pointers[index];

      /* Conv: Self modify of $BA70 removed (height loop). */
      /* Conv: Self modify of $BA72 removed (width loop). */
      /* mask_pointer[0] is the mask's width so this must calculate the skip
       * from the end of one row's used data to the start of the next */
      mask_row_skip = mask_pointer[0]      - mask_run_width; /* Conv: was self modified */
      buf_row_skip  = MASK_BUFFER_ROWBYTES - mask_run_width; /* Conv: was self modified */
    }

    /* Skip the initial clipped mask bytes. The masks are RLE compressed so
     * we can't jump directly to the first byte we need. */

    /* First, calculate the total number of mask tiles to skip.  */
    mask_skip = mask_top_skip * mask_pointer[0] + mask_left_skip + 1;

    /* Next, loop until we've passed through that number of output bytes. */
    do
    {
more_to_skip:
      A = *mask_pointer++; /* read a count byte or tile index */
      if (A & MASK_RUN_FLAG)
      {
        A &= ~MASK_RUN_FLAG;
        mask_skip -= A;
        if ((int16_t) mask_skip < 0) /* went too far? */
          goto skip_went_negative;
        mask_pointer++; /* skip mask index */
        if (mask_skip > 0) /* still more to skip? */
        {
          goto more_to_skip;
        }
        else /* mask_skip must be zero */
        {
          A = 0; /* zero counter */
          goto start_drawing;
        }
      }
    }
    while (--mask_skip);
    A = mask_skip; /* ie. A = 0 */
    goto start_drawing;


skip_went_negative:
    A = -(mask_skip & 0xFF); /* calculate skip counter */

start_drawing:
    {
      uint8_t *maskbufptr;   /* was HL */
      uint8_t  y_count;      /* was C */
      uint8_t  x_count;      /* was B */
      uint8_t  right_skip;   /* was B */
      uint8_t  Adash = 0xCC; /* Conv: initialise with safety value */

      maskbufptr = mask_buffer_pointer;
      // R I:C Iterations (inner loop)
      y_count = mask_run_height; /* Conv: was self modified */
      do
      {
        x_count = mask_run_width; /* Conv: was self modified */
        do
        {
          /* IN PROGRESS: The original code has some mismatched EX AF,AF
           * operations here which are hard to grok. AFAICT it's ensuring
           * that A is always the tile and that A' is the counter. */

          SWAP(uint8_t, A, Adash); /* bank the repeat length */

          A = *mask_pointer; /* read a count byte or tile index */
          if (A & MASK_RUN_FLAG)
          {
            A &= ~MASK_RUN_FLAG;
            SWAP(uint8_t, A, Adash); /* bank the repeat count */
            mask_pointer++;
            A = *mask_pointer; /* read the next byte (a tile) */
          }

          if (A != 0) /* shortcut tile 0 which is blank */
            mask_against_tile(A, maskbufptr);
          maskbufptr++;

          SWAP(uint8_t, A, Adash); /* unbank the repeat count/length */

          /* Advance the mask pointer when the counter reaches zero. */
          if (A == 0 || --A == 0) /* Conv: Written inverted vs. the original version. */
            mask_pointer++;
        }
        while (--x_count);

        /* Conv: Don't run the trailing code when we're on the final line
         * since that results in out-of-bounds memory reads. */
        if (y_count == 1)
          goto pop_next;

        // trailing skip

        right_skip = mask_row_skip; /* Conv: Was self modified. */ // == mask width - clipped width (amount to skip on the right hand side + left of next row)
        if (right_skip)
        {
          if (A)
            goto trailskip_dive_in; // must be continuing with a nonzero counter
          do
          {
trailskip_more_to_skip:
            A = *mask_pointer++; /* read a count byte or tile index */
            if (A & MASK_RUN_FLAG)
            {
              // it's a run

              A &= ~MASK_RUN_FLAG;
trailskip_dive_in:
              right_skip = A = right_skip - A; // CHECK THIS OVER // decrement skip
              if ((int8_t) right_skip < 0) // skip went negative
                goto trailskip_went_negative;
              mask_pointer++; // don't care about the mask index since we're skipping
              if (right_skip > 0)
                goto trailskip_more_to_skip;
              else
                goto next_line;
            }
          }
          while (--right_skip);

          A = 0;
          goto next_line;

trailskip_went_negative:
          A = -A;
        }
next_line:
        maskbufptr += buf_row_skip; /* Conv: Was self modified. */
      }
      while (--y_count);
    }

pop_next:
    pmask++;
  }
  while (--iters);
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
 * $BAF7: Clips the given vischar's dimensions against the game window.
 *
 * If the vischar spans a boundary a packed field is returned in the
 * respective clipped width/height. The low byte holds the width/height to
 * draw and the high byte holds the number of columns/rows to skip.
 *
 * Counterpart to \ref item_visible.
 *
 * \param[in]  state          Pointer to game state.
 * \param[in]  vischar        Pointer to visible character.       (was IY)
 * \param[out] left_skip      Pointer to returned left edge skip. (was B)
 * \param[out] clipped_width  Pointer to returned clipped width.  (was C)
 * \param[out] top_skip       Pointer to returned top edge skip.  (was D)
 * \param[out] clipped_height Pointer to returned ciipped height. (was E)
 *
 * \return 255 if the vischar is outside the game window, or zero if visible. (was A)
 */
int vischar_visible(tgestate_t      *state,
                    const vischar_t *vischar,
                    uint8_t         *left_skip,
                    uint8_t         *clipped_width,
                    uint8_t         *top_skip,
                    uint8_t         *clipped_height)
{
  uint8_t  window_right_edge;   /* was A  */
  int8_t   available_right;     /* was A  */
  uint8_t  new_left;            /* was B  */
  uint8_t  new_width;           /* was C  */
  int8_t   vischar_right_edge;  /* was A  */
  int8_t   available_left;      /* was A  */
  uint16_t window_bottom_edge;  /* was HL */
  uint16_t available_bottom;    /* was HL */
  uint8_t  new_top;             /* was D  */
  uint8_t  new_height;          /* was E  */
  uint16_t vischar_bottom_edge; /* was HL */
  uint16_t available_top;       /* was HL */

  assert(state          != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(left_skip      != NULL);
  assert(clipped_width  != NULL);
  assert(top_skip       != NULL);
  assert(clipped_height != NULL);

  /* Conv: Added to guarantee initialisation of return values. */
  *left_skip      = 255;
  *clipped_width  = 255;
  *top_skip       = 255;
  *clipped_height = 255;

  /* To determine visibility and sort out clipping there are five cases to
   * consider per axis:
   * (A) vischar is completely off the left/top of screen
   * (B) vischar is clipped on its left/top
   * (C) vischar is entirely visible
   * (D) vischar is clipped on its right/bottom
   * (E) vischar is completely off the right/bottom of screen
   */

  /*
   * Handle horizontal cases.
   */

  /* Calculate the right edge of the window in map space. */
  /* Conv: Columns was constant 24; replaced with state var. */
  window_right_edge = state->map_position.x + state->columns;

  /* Subtracting vischar's x gives the space available between vischar's
   * left edge and the right edge of the window (in bytes).
   * Note that available_right is signed to enable the subsequent test. */
  // state->isopos is isopos / 8.
  available_right = window_right_edge - state->isopos.x;
  if (available_right <= 0)
    /* Case (E): Vischar is beyond the window's right edge. */
    goto not_visible;

  /* Check for vischar partway off-screen. */
  if (available_right < vischar->width_bytes)
  {
    /* Case (D): Vischar's right edge is off-screen so truncate its width. */
    new_left  = 0;
    new_width = available_right;
  }
  else
  {
    /* Calculate the right edge of the vischar. */
    vischar_right_edge = state->isopos.x + vischar->width_bytes;

    /* Subtracting the map position's x gives the space available between
     * vischar's right edge and the left edge of the window (in bytes).
     * Note that available_left is signed to allow the subsequent test. */
    available_left = vischar_right_edge - state->map_position.x;
    if (available_left <= 0)
      /* Case (A): Vischar's right edge is beyond the window's left edge. */
      goto not_visible;

    /* Check for vischar partway off-screen. */
    if (available_left < vischar->width_bytes)
    {
      /* Case (B): Left edge is off-screen and right edge is on-screen. */
      new_left  = vischar->width_bytes - available_left;
      new_width = available_left;
    }
    else
    {
      /* Case (C): No clipping. */
      new_left  = 0;
      new_width = vischar->width_bytes;
    }
  }

  /*
   * Handle vertical cases.
   */

  /* Note: this uses vischar->isopos not state->isopos as above.
   * state->isopos.x/y is vischar->isopos.x/y >> 3. */

  /* Calculate the bottom edge of the window in map space. */
  /* Conv: Rows was constant 17; replaced with state var. */
  window_bottom_edge = state->map_position.y + state->rows;

  /* Subtracting the vischar's y gives the space available between window's
   * bottom edge and the vischar's top. */
  available_bottom = window_bottom_edge * 8 - vischar->isopos.y;
  // FUTURE: The second test here can be gotten rid of if available_bottom is
  //         made signed.
  if (available_bottom <= 0 || available_bottom >= 256)
    /* Case (E): Vischar is beyond the window's bottom edge. */
    goto not_visible;

  /* Check for vischar partway off-screen. */
  if (available_bottom < vischar->height)
  {
    /* Case (D): Vischar's bottom edge is off-screen, so truncate height. */
    new_top    = 0;
    new_height = available_bottom;
  }
  else
  {
    /* Calculate the bottom edge of the vischar. */
    vischar_bottom_edge = vischar->isopos.y + vischar->height;

    /* Subtracting the map position's y (scaled) gives the space available
     * between vischar's bottom edge and the top edge of the window (in
     * rows). */
    available_top = vischar_bottom_edge - state->map_position.y * 8;
    if (available_top <= 0 || available_top >= 256)
      /* Case (A): Vischar's bottom edge is beyond the window's top edge. */
      goto not_visible;

    /* Check for vischar partway off-screen. */
    if (available_top < vischar->height)
    {
      /* Case (B): Top edge is off-screen and bottom edge is on-screen. */
      new_top    = vischar->height - available_top;
      new_height = available_top;
    }
    else
    {
      /* Case (C): No clipping. */
      new_top    = 0;
      new_height = vischar->height;
    }
  }

  *left_skip      = new_left;
  *clipped_width  = new_width;
  *top_skip       = new_top;
  *clipped_height = new_height;

  return 0; /* Visible */


not_visible:
  return 255;
}

/* ----------------------------------------------------------------------- */

/**
 * $BB98: Paint any tiles occupied by visible characters with tiles from
 * tile_buf.
 *
 * \param[in] state Pointer to game state.
 */
void restore_tiles(tgestate_t *state)
{
  uint8_t            iters;                         /* was B */
  const vischar_t   *vischar;                       /* new local copy */
  uint8_t            height;                        /* was A/$BC5F */
  int8_t             bottom;                        /* was A */
  uint8_t            width;                         /* was $BC61 */
  uint8_t            tilebuf_skip;                  /* was $BC8E */
  uint8_t            windowbuf_skip;                /* was $BC95 */
  uint8_t            width_counter, height_counter; /* was B,C */
  uint8_t            left_skip;                     /* was B */
  uint8_t            clipped_width;                 /* was C */
  uint8_t            top_skip;                      /* was D */
  uint8_t            clipped_height;                /* was E */
  const pos8_t      *map_position;                  /* was HL */
  uint8_t           *windowbuf;                     /* was HL */
  uint8_t           *windowbuf2;                    /* was DE */
  uint8_t            x, y;                          /* was H', L' */
  const tileindex_t *tilebuf;                       /* was HL/DE */
  tileindex_t        tile;                          /* was A */
  const tile_t      *tileset;                       /* was BC */
  uint8_t            tile_counter;                  /* was B' */
  const tilerow_t   *tilerow;                       /* was HL' */

  assert(state != NULL);

  iters     = vischars_LENGTH;
  state->IY = &state->vischars[0];
  do
  {
    vischar = state->IY; /* Conv: Added local copy of IY. */

    if (vischar->flags == vischar_FLAGS_EMPTY_SLOT)
      goto next;

    /* Get the visible character's position in screen space. */
    state->isopos.y = vischar->isopos.y >> 3; /* divide by 8 (16-to-8) */
    state->isopos.x = vischar->isopos.x >> 3; /* divide by 8 (16-to-8) */

    if (vischar_visible(state,
                        vischar,
                       &left_skip,
                       &clipped_width,
                       &top_skip,
                       &clipped_height))
      goto next; /* not visible */

    /* Compute scaled clipped height. */
    /* Conv: Rotate and mask turned into right shift. */
    height = (clipped_height >> 3) + 2; /* TODO: Explain the +2 fudge factor. */

    /* Note: It seems that the following sequence (from here to clamp_height)
     * duplicates the work done by vischar_visible. I can't see any benefit to
     * it. */

    /* Compute bottom = height + isopos_y - map_position_y. This is the
     * distance of the (clipped) bottom edge of the vischar from the top of the
     * window. */
    bottom = height + state->isopos.y - state->map_position.y;
    if (bottom >= 0)
    {
      /* Bottom edge is on-screen, or off the bottom of the screen. */

      bottom -= state->rows; /* i.e. 17 */
      if (bottom > 0)
      {
        /* Bottom edge is now definitely visible. */

        int8_t visible_height; /* was A */

        visible_height = height - bottom;
        if (visible_height < 0)
        {
          goto next; /* not visible */
        }
        else if (visible_height != 0)
        {
          height = visible_height;
          goto clamp_height;
        }
        else
        {
          /* Note: Could merge this case into the earlier one. */
          assert(visible_height == 0);
          goto next; /* not visible */
        }
      }
    }

clamp_height:
    /* Clamp the height to a maximum of five. */
    if (height > 5)
      height = 5;

    /* Conv: Self modifying code replaced. */

    width          = clipped_width;
    tilebuf_skip   = state->columns - width;
    windowbuf_skip = tilebuf_skip + 7 * state->columns;

    map_position   = &state->map_position;

    /* Work out x,y offsets into the tile buffer. */

    if (left_skip == 0)
      x = state->isopos.x - map_position->x;
    else
      x = 0; /* was interleaved */

    if (top_skip == 0)
      y = state->isopos.y - map_position->y;
    else
      y = 0; /* was interleaved */

    /* Calculate the offset into the window buffer. */

    windowbuf = &state->window_buf[y * state->window_buf_stride + x];
    ASSERT_WINDOW_BUF_PTR_VALID(windowbuf, 0);

    /* Calculate the offset into the tile buffer. */

    tilebuf = &state->tile_buf[x + y * state->columns];
    ASSERT_TILE_BUF_PTR_VALID(tilebuf);

    height_counter = height; /* in rows */
    do
    {
      width_counter = width; /* in columns */
      do
      {
        ASSERT_TILE_BUF_PTR_VALID(tilebuf);

        tile = *tilebuf;
        windowbuf2 = windowbuf;

        tileset = select_tile_set(state, x, y);

        ASSERT_WINDOW_BUF_PTR_VALID(windowbuf2, 0);
        ASSERT_WINDOW_BUF_PTR_VALID(windowbuf2 + 7 * state->columns, 0);

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
          ASSERT_WINDOW_BUF_PTR_VALID(windowbuf, 0);
      }
      while (--width_counter);

      /* Reset x offset. Advance to next row. */
      x -= width;
      y++;
      tilebuf   += tilebuf_skip;
      if (height_counter > 1)
        ASSERT_TILE_BUF_PTR_VALID(tilebuf);
      windowbuf += windowbuf_skip;
      if (height_counter > 1)
        ASSERT_WINDOW_BUF_PTR_VALID(windowbuf, 0);
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
  /* Size, in UDGs, of a buffer zone around the visible screen in which
   * visible characters will spawn. */
  enum { GRACE = 8 };

  uint8_t            map_y, map_x;                  /* was H, L */
  uint8_t            map_y_clamped, map_x_clamped;  /* was D, E */
  characterstruct_t *charstr;                       /* was HL */
  uint8_t            iters;                         /* was B */
  room_t             room;                          /* was A */
  uint8_t            y, x;                          /* was C, A */

  assert(state != NULL);

  /* Form a clamped map position in (map_x, map_y). */
  map_x = state->map_position.x;
  map_y = state->map_position.y;
  map_x_clamped = (map_x < GRACE) ? 0 : map_x - GRACE;
  map_y_clamped = (map_y < GRACE) ? 0 : map_y - GRACE;

  /* Walk all character structs. */
  charstr = &state->character_structs[0];
  iters   = character_structs__LIMIT;
  do
  {
    if ((charstr->character_and_flags & characterstruct_FLAG_ON_SCREEN) == 0)
    {
      /* Is this character in the current room? */
      if ((room = state->room_index) == charstr->room)
      {
        if (room == room_0_OUTDOORS)
        {
          /* Handle outdoors. */

          /* Do screen Y calculation.
           * The 0x100 here is represented as zero in the original game. */
          y = 0x100 - charstr->mappos.u - charstr->mappos.v - charstr->mappos.w;
          if (y <= map_y_clamped || // <= might need to be <
              y > MIN(map_y_clamped + GRACE + 16 + GRACE, 0xFF))
            goto skip;

          /* Do screen X calculation. */
          x = (0x40 - charstr->mappos.u + charstr->mappos.v) * 2;
          if (x <= map_x_clamped || // <= might need to be <
              x > MIN(map_x_clamped + GRACE + 24 + GRACE, 0xFF))
            goto skip;
        }

        spawn_character(state, charstr);
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
   * visible characters will persist. (Compare to the spawning size of
   * 8). */
  enum { GRACE = 9 };

  uint8_t    miny, minx;  /* was D, E */
  uint8_t    iters;       /* was B */
  vischar_t *vischar;     /* was HL */
  uint8_t    y;           /* was C */
  uint8_t    x;           /* was C */

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

    /* Reset this character if it's not in the current room. */
    if (state->room_index != vischar->room)
      goto reset;

    // FUTURE
    //
    // /* Calculate clamped upper bound. */
    // maxx = MIN(minx + EDGE + state->columns + EDGE, 255);
    // maxy = MIN(miny + EDGE + (state->rows - 1) + EDGE, 255);
    //
    // t = (vischar->isopos.y + 4) / 8 / 256; // round
    // if (t <= miny || t > maxy)
    //   goto reset;
    // t = (vischar->isopos.x) / 8 / 256;
    // if (t <= minx || t > maxx)
    //   goto reset;

    /* Conv: Replaced screen dimension constants with state->columns/rows. */

    /* Handle Y part */
    y = divround(vischar->isopos.y);
    /* The original code uses 16 instead of 17 for 'rows' so match that. */
    if (y <= miny || y > MIN(miny + GRACE + (state->rows - 1) + GRACE, 255))
      goto reset;

    /* Handle X part */
    x = vischar->isopos.x / 8; /* round down */
    if (x <= minx || x > MIN(minx + GRACE + state->columns + GRACE, 255))
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
 */
void spawn_character(tgestate_t *state, characterstruct_t *charstr)
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

  vischar_t                    *vischar;      /* was HL/IY */
  uint8_t                       iters;        /* was B */
  characterstruct_t            *charstr2;     /* was DE */
  mappos16_t                   *saved_mappos; /* was HL */
  character_t                   character;    /* was A */
  const character_class_data_t *metadata;     /* was DE */
  int                           Z;            /* flag */
  room_t                        room;         /* was A */
  uint8_t                       target_type;  /* was A */
  const mappos8_t              *doormappos;   /* was HL */
  const pos8_t                 *location;     /* was HL */
  route_t                      *route;        /* was HL */

  assert(state   != NULL);
  assert(charstr != NULL);

  /* Skip spawning if the character is already on-screen. */
  if (charstr->character_and_flags & characterstruct_FLAG_ON_SCREEN)
    return; /* already spawned */

  /* Find an empty slot in the visible character list. */
  vischar = &state->vischars[1];
  iters = vischars_LENGTH - 1;
  do
  {
    if (vischar->character == character_NONE)
      goto found_empty_slot;
    vischar++;
  }
  while (--iters);

  return; /* no spare slot */


found_empty_slot:
  state->IY = vischar;
  charstr2 = charstr; /* Conv: Original points at charstr.room */

  /* Scale coords dependent on which room the character is in. */
  saved_mappos = &state->saved_mappos.pos16;
  if (charstr2->room == room_0_OUTDOORS)
  {
    /* Conv: Unrolled. */
    saved_mappos->u = charstr2->mappos.u * 8;
    saved_mappos->v = charstr2->mappos.v * 8;
    saved_mappos->w = charstr2->mappos.w * 8;
  }
  else
  {
    /* Conv: Unrolled. */
    saved_mappos->u = charstr2->mappos.u;
    saved_mappos->v = charstr2->mappos.v;
    saved_mappos->w = charstr2->mappos.w;
  }

  Z = collision(state);
  if (Z == 0) /* no collision */
    Z = bounds_check(state, vischar);
  if (Z == 1)
    return; /* a character or bounds collision occurred */

  /* Transfer character struct to vischar. */
  character = charstr->character_and_flags | characterstruct_FLAG_ON_SCREEN;
  charstr->character_and_flags = character;

  character &= characterstruct_CHARACTER_MASK;
  vischar->character = character;
  vischar->flags     = 0;

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

  vischar->animbase  = metadata->animbase;
  vischar->mi.sprite = metadata->sprite;
  memcpy(&vischar->mi.mappos, &state->saved_mappos, sizeof(mappos16_t));

  room = state->room_index;
  vischar->room = room;
  if (room > room_0_OUTDOORS)
  {
    play_speaker(state, sound_CHARACTER_ENTERS_2);
    play_speaker(state, sound_CHARACTER_ENTERS_1);
  }

  vischar->route = charstr->route;

  /* Conv: Removed DE handling here and below which isn't used in this port. */
  route = &charstr->route;
again:
  if (route->index != routeindex_0_HALT) /* if not stood still */
  {
    state->entered_move_a_character = 0;
    target_type = get_target(state, route, &doormappos, &location);
    if (target_type == get_target_ROUTE_ENDS)
    {
      route_ended(state, vischar, &vischar->route);

      /* To match the original code this should now jump to 'again' with 'route'
       * pointing to vischar->route, not charstr->route as used on the first
       * round. */
      route = &vischar->route;

      /* Jump back and try again. */
      goto again;
    }
    else if (target_type == get_target_DOOR)
    {
      vischar->flags |= vischar_FLAGS_TARGET_IS_DOOR;
    }
    if (target_type == get_target_DOOR)
    {
      vischar->target = *doormappos;
    }
    else
    {
      vischar->target.u = location->x;
      vischar->target.v = location->y;
      /* Note: We're not assigning .height here. */
    }
  }

  vischar->counter_and_flags = 0;
  calc_vischar_isopos_from_vischar(state, vischar);
  character_behaviour(state, vischar); /* was tail call */
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
  mappos16_t        *mappos;    /* was DE */
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
      mappos = &state->movable_items[movable_item_STOVE1].mappos;
    else if (character == character_27_STOVE_2)
      mappos = &state->movable_items[movable_item_STOVE2].mappos;
    else
      mappos = &state->movable_items[movable_item_CRATE].mappos;

    /* The DOS version of the game has a difference here. Instead of
     * memcpy'ing the current vischar's position into the movable_items's
     * position, it only copies the first two bytes. The code is setup for a
     * copy of six bytes (cx is set to 3) but the 'movsw' ought to be a 'rep
     * movsw' for it to work. It fixes the bug where stoves get left in place
     * after a restarted game, but almost looks like an accident.
     */
    memcpy(mappos, &vischar->mi.mappos, sizeof(*mappos));
  }
  else
  {
    mappos16_t *vispos_in;   /* was HL */
    mappos8_t  *charpos_out; /* was DE */

    /* A non-object character. */

    charstr = &state->character_structs[character];

    /* Re-enable the character. */
    charstr->character_and_flags &= ~characterstruct_FLAG_ON_SCREEN;

    room = vischar->room;
    charstr->room = room;

    vischar->counter_and_flags = 0; /* more flags */

    /* Save the old position. */

    vispos_in   = &vischar->mi.mappos;
    charpos_out = &charstr->mappos;

    if (room == room_0_OUTDOORS)
    {
      /* Outdoors. */
      scale_mappos_down(vispos_in, charpos_out);
    }
    else
    {
      /* Indoors. */
      /* Conv: Unrolled from original code. */
      charpos_out->u = vispos_in->u; // note: narrowing
      charpos_out->v = vispos_in->v; // note: narrowing
      charpos_out->w = vispos_in->w; // note: narrowing
    }

    // character = vischar->character; /* Done in original code, but we already have this from earlier. */
    vischar->character = character_NONE;
    vischar->flags     = vischar_FLAGS_EMPTY_SLOT;

    /* Guard dogs only. */
    if (character >= character_16_GUARD_DOG_1 &&
        character <= character_19_GUARD_DOG_4)
    {
      /* Choose random locations in the fenced off area (right side). */
      vischar->route.index = routeindex_255_WANDER; /* 0..7 */
      vischar->route.step  = 0;
      if (character >= character_18_GUARD_DOG_3) /* Characters 18 and 19 */
      {
        /* Choose random locations in the fenced off area (bottom side). */
        vischar->route.index = routeindex_255_WANDER; /* 24..31 */
        vischar->route.step  = 24;
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
 * Coordinates are returned as a mappos8_t when moving to a door or a pos8_t
 * when moving to a numbered location.
 *
 * If the route specifies 'wander' then one of eight random locations is
 * chosen starting from route.step.
 *
 * \param[in]  state      Pointer to game state.
 * \param[in]  route      Pointer to route.                            (was HL)
 * \param[out] doormappos Receives doorpos when moving to a door.      (was HL)
 * \param[out] location   Receives location when moving to a location. (was HL)
 *
 * \retval get_target_ROUTE_ENDS The route has ended.
 * \retval get_target_DOOR       The next target is a door.
 * \retval get_target_LOCATION   The next target is a location.
 */
uint8_t get_target(tgestate_t       *state,
                   route_t          *route,
                   const mappos8_t **doormappos,
                   const pos8_t    **location)
{
  /**
   * $783A: Table of map locations used in routes.
   */
  static const pos8_t locations[78] =
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

    // charevnt_wander_top uses 8..15
    { 104, 112 }, // 8
    {  96, 112 }, // 9 used by route_guard_12_roll_call
    { 106, 102 }, // 10
    {  93, 104 }, // 11 used by route_commandant, route_exit_hut2, route_77E1, route_guard_13_roll_call, route_guard_13_bed, route_guard_14_bed, route_guard_15_bed
    { 124, 101 }, // 12 used by route_exit_hut2, route_77E7, route_77EC, route_guard_13_bed, route_guard_14_bed, route_guard_15_bed
    { 124, 112 }, // 13
    { 116, 104 }, // 14 used by route_exit_hut3, route_go_to_solitary, route_hero_leave_solitary
    { 112, 100 }, // 15

    // charevnt_wander_left uses 16..23
    { 120,  96 }, // 16 used by route_77EC
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

    { 102,  68 }, // 32 used by route_7795
    {  68,  68 }, // 33 used by route_7795
    {  68, 104 }, // 34 used by route_7795

    { 107,  69 }, // 35 used by route_7799
    { 107,  45 }, // 36 used by route_7799
    {  77,  45 }, // 37 used by route_7799
    {  77,  61 }, // 38 used by route_7799
    {  61,  61 }, // 39 used by route_7799
    {  61, 103 }, // 40 used by route_7799

    { 116,  76 }, // 41
    {  44,  42 }, // 42 used by route_commandant, route_go_to_solitary
    { 106,  72 }, // 43 used by route_77CD
    { 110,  72 }, // 44 used by route_77CD
    {  81, 104 }, // 45 used by route_commandant, route_exit_hut3, route_guard_14_bed, route_guard_15_bed

    {  52,  60 }, // 46 used by route_commandant, route_prisoner_sleeps_1
    {  52,  44 }, // 47 used by route_prisoner_sleeps_2
    {  52,  28 }, // 48 used by route_prisoner_sleeps_3
    { 119, 107 }, // 49 used by route_guard_15_roll_call
    { 122, 110 }, // 50 used by route_hero_roll_call
    {  52,  28 }, // 51
    {  40,  60 }, // 52 used by route_77DE, route_guard_14_bed
    {  36,  34 }, // 53 used by route_77DE, route_guard_13_bed, route_guard_15_bed
    {  80,  76 }, // 54
    {  89,  76 }, // 55 used by route_commandant, route_77E1

    // charevnt_wander_yard uses 56..63
    {  89,  60 }, // 56 used by route_77E1
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
    {  52,  62 }, // 67 used by route_7833
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

  *doormappos  = NULL; /* Conv: Added. */
  *location = NULL; /* Conv: Added. */

#ifdef DEBUG_ROUTES
  printf("get_target(route.index=%d, route.step=%d)\n", route->index, route->step);
#endif

  routeindex = route->index;
  if (routeindex == routeindex_255_WANDER)
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

    /* Control can arrive here with routeindex set to zero. This happens when
     * the hero stands up during breakfast, is pursued by guards, then when
     * left to idle sits down and the pursuing guards resume their original
     * positions. get_route() will return in that case a NULL routebytes
     * pointer. This is not a conversion issue but happens even in the
     * original game where it starts fetching from $0001. In the Spectrum ROM
     * location $0001 holds XOR A ($AF).
     */

    routebytes = get_route(routeindex);

    if (step == 255)
    {
      /* Conv: Original code was relying on being able to fetch the preceding
       * route's final byte (255) in all cases. That's not going to work with
       * the way routes are defined in the portable version of the game so
       * instead just set routebyte to 255 here. */
      routebyte = routebyte_END;
    }
    else
    {
      /* Conv: If routebytes is NULL then mimic the ROM fetch bug. */
      if (routebytes == NULL)
        routebyte = 0xAF; /* location 7 + door_REVERSE (nonsensical) */
      else
        routebyte = routebytes[step]; // HL was temporary
    }

    if (routebyte == routebyte_END)
      /* Route byte == 255: End of route */
      return get_target_ROUTE_ENDS; /* Conv: Was a goto to a return. */

    routebyte &= ~door_REVERSE;
    if (routebyte < 40) // (0..39 || 128..167)
    {
      /* Route byte < 40: A door */
      routebyte = routebytes[step];
      if (route->index & ROUTEINDEX_REVERSE_FLAG)
        routebyte ^= door_REVERSE;
      door = get_door(routebyte);
      *doormappos = &door->mappos;
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
 * $C6A0: Move one (off-screen) character around at a time.
 *
 * \param[in] state Pointer to game state.
 */
void move_a_character(tgestate_t *state)
{
  character_t        character;   /* was A */
  characterstruct_t *charstr;     /* was HL/DE */
  room_t             room;        /* was A */
  item_t             item;        /* was C */
  uint8_t            target_type; /* was A */
  route_t           *route;       /* was HL */
  const mappos8_t   *doormappos;  /* was HL */
  const pos8_t      *location;    /* was HL */
  uint8_t            routeindex;  /* was A */
  uint8_t            max;         /* was A' */
  uint8_t            arrived;     /* was B */
  door_t            *door;        /* was HL */
  mappos8_t         *charstr_pos; /* was DE */

  assert(state != NULL);

  /* Set the 'character index is valid' flag. */
  /* This makes future character events use state->character_index. */
  state->entered_move_a_character = 255;

  /* Move to the next character, wrapping around after character 26. */
  character = state->character_index + 1;
  if (character == character_26_STOVE_1) /* 26 = (highest + 1) character */
    character = character_0_COMMANDANT;
  state->character_index = character;

  /* Get its chararacter struct and exit if the character is on-screen. */
  charstr = &state->character_structs[character];
  if (charstr->character_and_flags & characterstruct_FLAG_ON_SCREEN)
    return;

  /* Are any items to be found in the same room as the character? */
  room = charstr->room;
  ASSERT_ROOM_VALID(room);
  if (room != room_0_OUTDOORS)
    /* Note: This discovers just one item at a time. */
    if (is_item_discoverable_interior(state, room, &item) == 0)
      item_discovered(state, item);

  /* If the character is standing still, return now. */
  if (charstr->route.index == routeindex_0_HALT)
    return;

  target_type = get_target(state,
                          &charstr->route,
                          &doormappos,
                          &location);
  if (target_type == get_target_ROUTE_ENDS)
  {
    /* When the route ends, reverse the route. */

    route = &charstr->route;

    character = state->character_index;
    if (character != character_0_COMMANDANT)
    {
      /* Not the commandant. */

      if (character >= character_12_GUARD_12)
        goto trigger_event;

      /* Characters 1..11. */
reverse_route:
      route->index = routeindex = route->index ^ ROUTEINDEX_REVERSE_FLAG;

      /* Conv: Was [-2]+1 pattern. Adjusted to be clearer. */
      if (routeindex & ROUTEINDEX_REVERSE_FLAG)
        route->step--;
      else
        route->step++;
    }
    else
    {
      /* Commandant only. */

      routeindex = route->index & ~ROUTEINDEX_REVERSE_FLAG;
      if (routeindex != routeindex_36_GO_TO_SOLITARY)
        goto reverse_route;

trigger_event:
      /* We arrive here if the character index is character_12_GUARD_12, or
       * higher, or if it's the commandant on route 36 ("go to solitary"). */

      character_event(state, route); /* was tail call */
    }
  }
  else
  {
    if (target_type == get_target_DOOR)
    {
      /* Handle the target-is-a-door case. */

      const mappos8_t *doormappos2; /* was HL */

      room = charstr->room;
      if (room == room_0_OUTDOORS)
      {
        /* The original code stores to saved_mappos as if it's a pair of bytes.
         * This is different from most, if not all, of the other locations in
         * the code which treat it as a pair of 16-bit words.
         * Given that I assume it's just being used here as scratch space. */

        /* Conv: Unrolled. */
        state->saved_mappos.pos8.u = doormappos->u >> 1;
        state->saved_mappos.pos8.v = doormappos->v >> 1;
        doormappos2 = &state->saved_mappos.pos8;
      }
      else
      {
        /* Conv: Assignment made explicit. */
        doormappos2 = doormappos;
      }

      if (charstr->room == room_0_OUTDOORS) /* FUTURE: Remove reload of 'room'. */
        max = 2;
      else
        max = 6;

      charstr_pos = &charstr->mappos;

      /* args: max move, rc, second val, first val */
      arrived = move_towards(max,       0, &doormappos2->u, &charstr_pos->u);
      arrived = move_towards(max, arrived, &doormappos2->v, &charstr_pos->v);
      if (arrived != 2)
        return; /* not arrived */

      /* If we reach here the character has arrived at their destination.
       *
       * Our current target is a door, so change to the door's target room. */

      /* Turn doormappos (assigned in get_target) back into a door_t. */
      door = structof(doormappos, door_t, mappos);
      assert(door >= &doors[0]);
      assert(door < &doors[door_MAX * 2]);

      charstr->room = (door->room_and_direction & ~door_FLAGS_MASK_DIRECTION) >> 2;

      /* Determine the destination door. */
      if ((door->room_and_direction & door_FLAGS_MASK_DIRECTION) < 2)
        /* The door is facing top left or top right. */
        doormappos = &door[1].mappos; /* point at the next half of door pair */
      else
        /* The door is facing bottom left or bottom right. */
        doormappos = &door[-1].mappos; /* point at the previous half of door pair */

      /* Copy the door's position into the charstr. */
      room = charstr->room;
      charstr_pos = &charstr->mappos;
      if (room != room_0_OUTDOORS)
      {
        /* Indoors. Copy the door's position into the charstr. */

        *charstr_pos = *doormappos;
      }
      else
      {
        /* Outdoors. Copy the door's position into the charstr, dividing by
         * two. */

        /* Conv: Unrolled. */
        charstr_pos->u = doormappos->u >> 1;
        charstr_pos->v = doormappos->v >> 1;
        charstr_pos->w = doormappos->w >> 1;
      }
    }
    else
    {
      /* Normal move case. */

      /* Establish a maximum for passing into move_towards. */
      /* Conv: Reordered. */
      if (charstr->room == room_0_OUTDOORS)
        max = 2;
      else
        max = 6;

      arrived = move_towards(max,       0, &location->x, &charstr->mappos.u);
      arrived = move_towards(max, arrived, &location->y, &charstr->mappos.v);
      if (arrived != 2)
        return; /* not arrived */
    }

    /* If we reach here the character has arrived at their destination. */
    routeindex = charstr->route.index;
    if (routeindex == routeindex_255_WANDER)
      return;

    if ((routeindex & ROUTEINDEX_REVERSE_FLAG) == 0)
      charstr->route.step++;
    else
      charstr->route.step--;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $C79A: Moves the first value toward the second.
 *
 * Used only by move_a_character().
 *
 * Leaf.
 *
 * \param[in] max    Some maximum value, usually 2 or 6. (was A')
 * \param[in] rc     Return code. Incremented if delta is zero. (was B)
 * \param[in] second Pointer to second value. (was HL)
 * \param[in] first  Pointer to first value. (was DE)
 *
 * \return rc as passed in, or rc incremented if delta was zero. (was B)
 */
int move_towards(int8_t         max,
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

/* Conv: $C7B9/get_character_struct was inlined. */

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
#define REVERSE ROUTEINDEX_REVERSE_FLAG

  /* $C7F9 */
  static const route2event_t eventmap[24] =
  {
    { routeindex_38_GUARD_12_BED | REVERSE,         0 }, /* wander between locations 8..15 */
    { routeindex_39_GUARD_13_BED | REVERSE,         0 }, /* wander between locations 8..15 */
    { routeindex_40_GUARD_14_BED | REVERSE,         1 }, /* wander between locations 16..23 */
    { routeindex_41_GUARD_15_BED | REVERSE,         1 }, /* wander between locations 16..23 */

    { routeindex_5_EXIT_HUT2,                       0 }, /* wander between locations 8..15 */
    { routeindex_6_EXIT_HUT3,                       1 }, /* wander between locations 16..23 */
    { routeindex_5_EXIT_HUT2 | REVERSE,             3 },
    { routeindex_6_EXIT_HUT3 | REVERSE,             3 },

    { routeindex_14_GO_TO_YARD,                     2 }, /* wander between locations 56..63 */
    { routeindex_15_GO_TO_YARD,                     2 }, /* wander between locations 56..63 */
    { routeindex_14_GO_TO_YARD | REVERSE,           0 }, /* wander between locations 8..15 */
    { routeindex_15_GO_TO_YARD | REVERSE,           1 }, /* wander between locations 16..23 */

    { routeindex_16_BREAKFAST_25,                   5 },
    { routeindex_17_BREAKFAST_23,                   5 },
    { routeindex_16_BREAKFAST_25 | REVERSE,         0 }, /* wander between locations 8..15 */
    { routeindex_17_BREAKFAST_23 | REVERSE,         1 }, /* wander between locations 16..23 */

    { routeindex_32_GUARD_15_ROLL_CALL | REVERSE,   0 }, /* wander between locations 8..15 */
    { routeindex_33_PRISONER_4_ROLL_CALL | REVERSE, 1 }, /* wander between locations 16..23 */
    { routeindex_42_HUT2_LEFT_TO_RIGHT,             7 },
    { routeindex_44_HUT2_RIGHT_TO_LEFT,             8 }, /* hero sleeps */
    { routeindex_43_7833,                           9 }, /* hero sits */
    { routeindex_36_GO_TO_SOLITARY | REVERSE,       6 },
    { routeindex_36_GO_TO_SOLITARY,                10 }, /* hero released from solitary */
    { routeindex_37_HERO_LEAVE_SOLITARY,            4 }  /* solitary ends */
  };

  /* $C829 */
  static charevnt_handler_t *const handlers[] =
  {
    &charevnt_wander_top,
    &charevnt_wander_left,
    &charevnt_wander_yard,
    &charevnt_bed,
    &charevnt_solitary_ends,
    &charevnt_breakfast,
    &charevnt_commandant_to_yard,
    &charevnt_exit_hut2,
    &charevnt_hero_sleeps,
    &charevnt_hero_sits,
    &charevnt_hero_release
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
  if (routeindex >= routeindex_7_PRISONER_SLEEPS_1 &&
      routeindex <= routeindex_12_PRISONER_SLEEPS_3)
  {
    character_sleeps(state, routeindex, route);
    return;
  }
  /* BUG FIX: Prisoner assigned route 23 now sits. */
  if (routeindex >= routeindex_18_PRISONER_SITS_1 &&
      routeindex <= routeindex_23_PRISONER_SITS_3)
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

  route->index = routeindex_0_HALT; /* Stand still. */
}

/**
 * $C83F: Ends solitary.
 */
void charevnt_solitary_ends(tgestate_t *state, route_t *route)
{
  state->in_solitary = 0; /* Enable player control */
  charevnt_wander_top(state, route); // ($FF,$08)
}

/**
 * $C845: Sets route to { 3, 21 }.
 */
void charevnt_commandant_to_yard(tgestate_t *state, route_t *route)
{
  // this is something like: commandant walks to yard

  route->index = routeindex_3_COMMANDANT;
  route->step  = 21;
}

/**
 * $C84C: Sets route to route 36 in reverse. Sets hero to route 37.
 */
void charevnt_hero_release(tgestate_t *state, route_t *route)
{
  /* Reverse the commandant's route. */
  route->index = routeindex_36_GO_TO_SOLITARY | ROUTEINDEX_REVERSE_FLAG;
  route->step  = 3;

  state->automatic_player_counter = 0; /* Force automatic control */

  {
    static const route_t route_37 = { routeindex_37_HERO_LEAVE_SOLITARY, 0 }; /* was BC */
    set_hero_route_force(state, &route_37);
  }
}

/**
 * $C85C: Sets route to wander around locations 16..23.
 */
void charevnt_wander_left(tgestate_t *state, route_t *route)
{
  // When .x is set to $FF it says to pick a random location from the
  // locations[] array from .y to .y+7.

  // Set route to wander between random locations in the left part of the map:
  //    _________________               |
  //   |*******          |              |
  //   |*******          |______________|
  //   |*******                     :   :
  //   |*******HUT   HUT   HUT      :   :
  //  _|*******ONE   TWO   TRE     X:   :
  // | ********HUT   HUT   HUT      :   :
  // | ********ONE   TWO   TRE      :   :
  //  |                            X:   :
  // _|_______GATE_|~~~~~~~DD~~~~~~~'   :
  //              :                     :
  //              :~~~~~~~~DD~~~~~~~~~~~'
  //             X:             :
  //              :             :
  //              :            X:
  //              '~~~~~~~~~~~~~'

  route->index = routeindex_255_WANDER;
  route->step  = 16; /* 16..23 */
}

/**
 * $C860: Sets route to wander around locations 56..63.
 */
void charevnt_wander_yard(tgestate_t *state, route_t *route)
{
  // Set route to wander between random locations in the exercise yard:
  //    _________________               |
  //   |                 |              |
  //   |                 |______________|
  //   |                            :   :
  //   |       HUT   HUT   HUT      :   :
  //  _|       ONE   TWO   TRE     X:   :
  // |         HUT   HUT   HUT      :   :
  // |         ONE   TWO   TRE      :   :
  //  |                            X:   :
  // _|_______GATE_|~~~~~~~DD~~~~~~~'   :
  //              :                     :
  //              :~~~~~~~~DD~~~~~~~~~~~'
  //             X:*************:
  //              :*************:
  //              :************X:
  //              '~~~~~~~~~~~~~'

  route->index = routeindex_255_WANDER;
  route->step  = 56; /* 56..63 */
}

/**
 * $C864: Sets route to wander around locations 8..15.
 */
void charevnt_wander_top(tgestate_t *state, route_t *route)
{
  // Set route to wander between random locations in the top part of the map:
  //    _________________               |
  //   |*****************|              |
  //   |*****************|______________|
  //   |*****************           :   :
  //   |       HUT   HUT   HUT      :   :
  //  _|       ONE   TWO   TRE     X:   :
  // |         HUT   HUT   HUT      :   :
  // |         ONE   TWO   TRE      :   :
  //  |                            X:   :
  // _|_______GATE_|~~~~~~~DD~~~~~~~'   :
  //              :                     :
  //              :~~~~~~~~DD~~~~~~~~~~~'
  //             X:             :
  //              :             :
  //              :            X:
  //              '~~~~~~~~~~~~~'

  route->index = routeindex_255_WANDER;
  route->step  = 8; /* 8..15 */
}

// Conv: Inlined set_route_ffxx() in the above functions.

/**
 * $C86C:
 */
void charevnt_bed(tgestate_t *state, route_t *route)
{
  if (state->entered_move_a_character == 0)
    character_bed_vischar(state, route);
  else
    character_bed_state(state, route);
}

/**
 * $C877:
 */
void charevnt_breakfast(tgestate_t *state, route_t *route)
{
  if (state->entered_move_a_character == 0)
    charevnt_breakfast_vischar(state, route);
  else
    charevnt_breakfast_state(state, route);
}

/**
 * $C882: Sets route to ($05,$00).
 */
void charevnt_exit_hut2(tgestate_t *state, route_t *route)
{
  route->index = routeindex_5_EXIT_HUT2;
  route->step  = 0;
}

/**
 * $C889: Hero sits.
 */
void charevnt_hero_sits(tgestate_t *state, route_t *route)
{
  hero_sits(state);
}

/**
 * $C88D: Hero sleeps.
 */
void charevnt_hero_sleeps(tgestate_t *state, route_t *route)
{
  hero_sleeps(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $C892: Drives automatic behaviour for NPCs and the hero when the player
 * idles.
 *
 * Causes characters to follow the hero if he's being suspicious.
 * Also: Food item discovery.
 * Also: Automatic hero behaviour.
 *
 * \param[in] state Pointer to game state.
 */
void automatics(tgestate_t *state)
{
  uint8_t iters; /* was B */

  assert(state != NULL);

  /* (I've still no idea what this flag means). */
  state->entered_move_a_character = 0;

  /* If the bell is ringing, hostiles pursue. */
  if (state->bell == bell_RING_PERPETUAL)
    hostiles_pursue(state);

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

      character = vischar->character; /* Conv: Removed redundant mask op. */
      assert(character != character_NONE);
      if (character <= character_19_GUARD_DOG_4) /* Hostile characters only. */
      {
        /* Characters 0..19. */

        is_item_discoverable(state);

        if (state->red_flag || state->automatic_player_counter > 0)
          guards_follow_suspicious_character(state, vischar);

        /* If there is (poisoned) food nearby then put the guard dogs
         * (characters 16..19) into vischar_PURSUIT_DOG_FOOD pursue mode. */
        if (character >= character_16_GUARD_DOG_1 &&
            state->item_structs[item_FOOD].room_and_flags & itemstruct_ROOM_FLAG_NEARBY_7)
            vischar->flags = vischar_PURSUIT_DOG_FOOD;
      }

      character_behaviour(state, vischar);
    }

    state->IY++;
  }
  while (--iters);

  /* Engage automatic behaviour for the hero unless the flag is red, the
   * hero is in solitary or an input event was recently received. */
  if (state->red_flag)
    return;
  if (state->in_solitary || state->automatic_player_counter == 0)
  {
    state->IY = &state->vischars[0];
    character_behaviour(state, state->IY);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $C918: Character behaviour.
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
  uint8_t    vischar2flags;     /* was C' */
  uint8_t    scale;             /* Conv: added */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  /* Proceed into the character behaviour handling only when this delay field
   * hits zero. This stops characters navigating around obstacles too
   * quickly. */
  counter_and_flags = vischar->counter_and_flags; // Conv: Use of A dropped.
  if (counter_and_flags & vischar_BYTE7_COUNTER_MASK)
  {
    vischar->counter_and_flags = --counter_and_flags;
    return;
  }

  vischar2 = vischar; // no need for separate HL and IY now other code reduced
  flags = vischar2->flags;
  if (flags != 0)
  {
    /* Mode 1: Hero is chased by hostiles and sent to solitary if caught. */
    if (flags == vischar_PURSUIT_PURSUE)
    {
pursue_hero:
      /* Pursue the hero. */
      vischar2->target.u = state->hero_mappos.u;
      vischar2->target.v = state->hero_mappos.v;
      goto move;
    }
    /* Mode 2: Hero is chased by hostiles if under player control. */
    else if (flags == vischar_PURSUIT_HASSLE)
    {
      if (state->automatic_player_counter > 0)
      {
        /* The hero is under player control: pursue. */
        goto pursue_hero;
      }
      else
      {
        /* The hero is under automatic control: hostiles lose interest and
         * resume following their original route. */
        vischar2->flags = 0; /* clear vischar_PURSUIT_HASSLE */
        get_target_assign_pos(state, vischar2, &vischar2->route);
        return;
      }
    }
    /* Mode 3: The food item is near a guard dog. */
    else if (flags == vischar_PURSUIT_DOG_FOOD)
    {
      if (state->item_structs[item_FOOD].room_and_flags & itemstruct_ROOM_FLAG_NEARBY_7)
      {
        /* Set dog target to poisoned food. */
        vischar2->target.u = state->item_structs[item_FOOD].mappos.u;
        vischar2->target.v = state->item_structs[item_FOOD].mappos.v;
        goto move;
      }
      else
      {
        /* Food is no longer nearby: Choose random locations in the fenced-
         * off area (right side). */
        vischar2->flags = 0; /* clear vischar_PURSUIT_DOG_FOOD */
        vischar2->route.index = routeindex_255_WANDER;
        vischar2->route.step  = 0; /* 0..7 */
        get_target_assign_pos(state, vischar2, &vischar2->route);
        return;
      }
    }
    /* Mode 4: Hostile character witnessed a bribe being given (in accept_bribe). */
    else if (flags == vischar_PURSUIT_SAW_BRIBE)
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
          if (found->character == bribed_character)
            goto bribed_visible;
          found++;
        }
        while (--iters);
      }

      /* Bribed character was not visible: hostiles lose interest and resume
       * following their original route. */
      vischar2->flags = 0; /* clear vischar_PURSUIT_SAW_BRIBE */
      get_target_assign_pos(state, vischar2, &vischar2->route);
      return;

bribed_visible:
      /* Found the bribed character in vischars: hostiles target him. */
      {
        mappos16_t *mappos; /* was HL */
        mappos8_t  *target; /* was DE */

        mappos = &found->mi.mappos;
        target = &vischar2->target;
        if (state->room_index == room_0_OUTDOORS)
        {
          /* Outdoors */
          scale_mappos_down(mappos, target);
        }
        else
        {
          /* Indoors */
          target->u = mappos->u; // note: narrowing
          target->v = mappos->v; // note: narrowing
        }
        goto move;
      }
    }
  }

  if (vischar2->route.index == routeindex_0_HALT) /* Stand still */
  {
    character_behaviour_set_input(state, vischar, 0 /* new_input */);
    return; /* was tail call */
  }

move:
  vischar2flags = vischar2->flags; // sampled = $8081, 80a1, 80e1, 8001, 8021, ...

  /* Conv: The original code here self modifies the vischar_move_u/v routines.
   *       The replacement code passes in a scale factor. */
  if (state->room_index > room_0_OUTDOORS)
    scale = 1; /* Indoors. */
  else if (vischar2flags & vischar_FLAGS_TARGET_IS_DOOR)
    scale = 4; /* Outdoors + door thing. */
  else
    scale = 8; /* Outdoors. */

  /* If the vischar_BYTE7_V_DOMINANT flag is set then we try moving on the v
   * axis then the u axis, rather than u then v. This is the code which makes
   * characters alternate left/right when navigating. */
  if (vischar->counter_and_flags & vischar_BYTE7_V_DOMINANT)
  {
    /* V dominant case */

    /* Conv: Inlined the chunk from $C9FF here. */

    input_t input; /* was A */

    input = vischar_move_v(state, vischar, scale);
    if (input)
    {
      character_behaviour_set_input(state, vischar, input);
    }
    else
    {
      input = vischar_move_u(state, vischar, scale);
      if (input)
        character_behaviour_set_input(state, vischar, input);
      else
        target_reached(state, vischar); /* was tail call */
    }
  }
  else
  {
    /* U dominant case */

    input_t input; /* was A */

    input = vischar_move_u(state, vischar, scale);
    if (input)
    {
      character_behaviour_set_input(state, vischar, input);
    }
    else
    {
      input = vischar_move_v(state, vischar, scale);
      if (input)
        character_behaviour_set_input(state, vischar, input);
      else
        target_reached(state, vischar); /* was tail call */
    }
  }
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

/* ----------------------------------------------------------------------- */

/**
 * $CA11: Return the input_t which moves us closer to our U target.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] scale   Scale factor. (replaces self-modifying code in original)
 *
 * \return 8 => u >  pos.u
 *         4 => u <  pos.u
 *         0 => u == pos.u
 */
input_t vischar_move_u(tgestate_t *state,
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

  // which one's the target value?
  // one will be "where i am" one will be "where i need to be"
  // suspect that mi.pos.x is "where i am"
  // and that pos.x is "where i need to be"

  delta = (int16_t) vischar->mi.mappos.u - (int16_t) vischar->target.u * scale;
  /* Conv: Rewritten to simplify. Split D,E checking boiled down to -3/+3. */
  if (delta >= 3)
    return input_RIGHT + input_DOWN;
  else if (delta <= -3)
    return input_LEFT + input_UP;
  else
  {
    vischar->counter_and_flags |= vischar_BYTE7_V_DOMINANT;
    return input_NONE;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CA49: Return the input_t which moves us closer to our V target.
 *
 * Nearly identical to vischar_move_x.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] scale   Scale factor. (replaces self-modifying code in original)
 *
 * \return 5 => v >  pos.v
 *         7 => v <  pos.v
 *         0 => v == pos.v
 */
input_t vischar_move_v(tgestate_t *state,
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

  delta = (int16_t) vischar->mi.mappos.v - (int16_t) vischar->target.v * scale;
  /* Conv: Rewritten to simplify. Split D,E checking boiled down to -3/+3. */
  if (delta >= 3)
    return input_LEFT + input_DOWN;
  else if (delta <= -3)
    return input_RIGHT + input_UP;
  else
  {
    vischar->counter_and_flags &= ~vischar_BYTE7_V_DOMINANT;
    return input_NONE;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CA81: Called when a character reaches its target.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void target_reached(tgestate_t *state, vischar_t *vischar)
{
  uint8_t          flags_all;               /* was C */

  uint8_t          flags_lower6;            /* was A */
  uint8_t          food_discovered_counter; /* was A */
  uint8_t          step;                    /* was C */
  uint8_t          route;                   /* was A */
  doorindex_t      doorindex;               /* was A */
  const door_t    *door;                    /* was HL */
  const mappos8_t *mappos;                  /* was HL */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  /* Conv: In the original code HL is IY + 4 on entry.
   *       In this version we replace HL references with IY ones.
   */

  flags_all = vischar->flags;

  /* Check for a pursuit mode. */
  flags_lower6 = flags_all & vischar_FLAGS_MASK;
  if (flags_lower6)
  {
    /* We're in one of the pursuit modes - find out which one. */
    if (flags_lower6 == vischar_PURSUIT_PURSUE)
    {
      /* Handle 'pursue' mode. */
      if (vischar->character == state->bribed_character)
        accept_bribe(state); /* was tail call */
      else
        /* The pursuing character caught its target. */ // this bit needs work
        solitary(state); /* was tail call */
    }
    else if (flags_lower6 == vischar_PURSUIT_HASSLE ||
             flags_lower6 == vischar_PURSUIT_SAW_BRIBE)
    {
      /* Handle 'hassle' or 'saw a bribe' modes (no action required). */
    }
    else
    {
      /* Otherwise we're in vischar_PURSUIT_DOG_FOOD mode. */

      /* automatics() only permits dogs to enter this mode. */
      assert(flags_lower6 == vischar_PURSUIT_DOG_FOOD);

      /* Decide how long remains until the food is discovered.
       * Use 32 if the food is poisoned, 255 otherwise. */
      if ((state->item_structs[item_FOOD].item_and_flags & itemstruct_ITEM_FLAG_POISONED) == 0)
        food_discovered_counter = 32; /* food is not poisoned */
      else
        food_discovered_counter = 255; /* food is poisoned */
      state->food_discovered_counter = food_discovered_counter;

      /* This dog has been poisoned, so make it halt. */
      vischar->route.index = routeindex_0_HALT;

      character_behaviour_set_input(state, vischar, 0 /* new_input */);
    }

    return;
  }

  if (flags_all & vischar_FLAGS_TARGET_IS_DOOR)
  {
    /* Handle the door - this results in the character entering. */

    step  = vischar->route.step;
    route = vischar->route.index;

    doorindex = get_route(route)[step];
    if (route & ROUTEINDEX_REVERSE_FLAG) /* Conv: Avoid needless reload */
      doorindex ^= door_REVERSE;

    /* Conv: This is the [-2]+1 pattern which works out -1/+1. */
    if (route & ROUTEINDEX_REVERSE_FLAG) /* Conv: Avoid needless reload */
      vischar->route.step--;
    else
      vischar->route.step++;

    /* Get the door structure for the door index and start processing it. */
    door = get_door(doorindex);
    vischar->room = (door->room_and_direction & ~door_FLAGS_MASK_DIRECTION) >> 2;

    /* In which direction is the door facing?
     *
     * Each door in the doors array is a pair of two "half doors" where each
     * half represents one side of the doorway. We test the direction of the
     * half door we find ourselves pointing at and use it to find the
     * counterpart door's position. */
    if ((door->room_and_direction & door_FLAGS_MASK_DIRECTION) <= direction_TOP_RIGHT)
      mappos = &door[1].mappos; /* TOP_LEFT or TOP_RIGHT */
    else
      mappos = &door[-1].mappos; /* BOTTOM_RIGHT or BOTTOM_LEFT */

    if (vischar == &state->vischars[0])
    {
      /* Hero's vischar only. */
      vischar->flags &= ~vischar_FLAGS_TARGET_IS_DOOR;
      get_target_assign_pos(state, vischar, &vischar->route);
    }

    transition(state, mappos);

    play_speaker(state, sound_CHARACTER_ENTERS_1);
    return;
  }

  route = vischar->route.index;
  if (route != routeindex_255_WANDER)
  {
    if (route & ROUTEINDEX_REVERSE_FLAG)
      vischar->route.step--;
    else
      vischar->route.step++;
  }

  get_target_assign_pos(state, vischar, &vischar->route); /* was fallthrough */
}

/**
 * $CB23: Calls get_target() then puts coords in vischar->target and set flags.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] route   Pointer to vischar->route.    (was HL)
 */
void get_target_assign_pos(tgestate_t *state,
                           vischar_t  *vischar,
                           route_t    *route)
{
  uint8_t          get_target_result; /* was A */
  const mappos8_t *doormappos;        /* was HL */
  const pos8_t    *location;          /* was HL */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  get_target_result = get_target(state, route, &doormappos, &location);
  if (get_target_result != get_target_ROUTE_ENDS)
  {
    /* "Didn't hit end of list" case. */
    /* Conv: Inlined $CB61/handle_target here since we're the only user. */

    if (get_target_result == get_target_DOOR)
      vischar->flags |= vischar_FLAGS_TARGET_IS_DOOR;

    /* Conv: In the original game HL returns a doorpos or a location to
     * transfer. This version must cope with get_target() returning results in
     * different vars. */
    if (get_target_result == get_target_DOOR)
    {
      vischar->target.u = doormappos->u;
      vischar->target.v = doormappos->v;
    }
    else
    {
      vischar->target.u = location->x;
      vischar->target.v = location->y;
    }
  }
  else
  {
    route_ended(state, vischar, route); /* was fallthrough */
  }
}

/**
 * $CB2D: Called by spawn_character and get_target_assign_pos when
 * get_target has run out of route.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] route   Pointer to vischar->route.    (was HL)
 */
void route_ended(tgestate_t *state, vischar_t *vischar, route_t *route)
{
  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

#ifdef DEBUG_ROUTES
  if (vischar == &state->vischars[0])
    printf("(hero) route_ended\n");
#endif

  /* If not the hero's vischar ... */
  if (route != &state->vischars[0].route) /* Conv: was (L != 2) */
  {
    character_t character; /* was A */

    character = vischar->character; /* Conv: Removed redundant mask op. */
    assert(character != character_NONE);

    /* Call character_event at the end of commandant route 36. */
    if (character == character_0_COMMANDANT)
      if ((route->index & ~ROUTEINDEX_REVERSE_FLAG) == routeindex_36_GO_TO_SOLITARY)
        goto do_character_event;

    /* Reverse the route for guards 1..11. */
    // Guards 1..11 have fixed roles so either stand still or march back and forth along their route.
    if (character <= character_11_GUARD_11)
      goto reverse_route;
  }

do_character_event:
  /* We arrive here if:
   * - vischar is the hero, or
   * - character is character_0_COMMANDANT and route index is 36, or
   * - character is >= character_12_GUARD_12
   */

  character_event(state, route);
  if (route->index != routeindex_0_HALT) /* standing still */
    get_target_assign_pos(state, vischar, route); // re-enter
  return;

reverse_route:
  /* We arrive here if:
   * - vischar is not the hero, and
   *   - character is character_0_COMMANDANT and route index is not 36, or
   *   - character is character_1_GUARD_1 .. character_11_GUARD_11
   */

  route->index ^= ROUTEINDEX_REVERSE_FLAG; /* toggle route direction */
  // Conv: Removed [-2]+1 pattern.
  if (route->index & ROUTEINDEX_REVERSE_FLAG)
    route->step--;
  else
    route->step++;
}

// $CB5F,2 Unreferenced bytes. [Absent in DOS version]

/* ----------------------------------------------------------------------- */

/* Conv: $CB75/multiply_by_1 was inlined. */

/* ----------------------------------------------------------------------- */

/**
 * $CB79: Return a route.
 *
 * Used by the routines at get_target and target_reached.
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

const uint8_t *get_route(routeindex_t index)
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
    DOOR(21 | door_REVERSE),  // room_19_FOOD     -> room_23_MESS_HALL
    DOOR(20 | door_REVERSE),  // room_23_MESS_HALL-> room_21_CORRIDOR
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
    LOCATION(11),             // charevnt_commandant_to_yard jumps to this offset
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
  static const uint8_t route_go_to_yard[] =
  {
    LOCATION(11),
    LOCATION(55),
    DOOR(0 | door_REVERSE),   // move through gates
    DOOR(1 | door_REVERSE),   // move through gates to yard
    LOCATION(56),
    routebyte_END
  };
  static const uint8_t route_breakfast_room_25[] =
  {
    LOCATION(12),             // 93,104
    DOOR(10),                 // room_0_OUTDOORS   -> room_21_CORRIDOR
    DOOR(20),                 // room_21_CORRIDOR  -> room_23_MESS_HALL
    DOOR(19 | door_REVERSE),  // room_23_MESS_HALL -> room_25_MESS_HALL
    routebyte_END
  };
  static const uint8_t route_breakfast_room_23[] =
  {
    LOCATION(16),
    LOCATION(12),
    DOOR(10),                 // room_0_OUTDOORS  -> room_21_CORRIDOR
    DOOR(20),                 // room_21_CORRIDOR -> room_23_MESS_HALL
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
    LOCATION(67),             // 52,62
    routebyte_END
  };
  static const uint8_t route_hut2_right_to_left[] =
  {
    DOOR(17 | door_REVERSE),  // room_3_HUT2RIGHT -> room_2_HUT2LEFT
    LOCATION(70),             // 42,46 // go to bed?
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
  static const uint8_t *routes[routeindex__LIMIT] =
  {
    NULL, /* was zero */            //  0: route 0 => stand still (as opposed to route 255 => wander around randomly)  hero in solitary

    &route_7795[0],                 //  1: L-shaped route in the fenced area
    &route_7799[0],                 //  2: guard's route around the front perimeter wall
    &route_commandant[0],           //  3: the commandant's route - the longest of all the routes
    &route_77CD[0],                 //  4: guard's route marching over the front gate

    &route_exit_hut2[0],            //  5: character_1x_GUARD_12/13, character_2x_PRISONER_1/2/3 by wake_up & go_to_time_for_bed, go_to_time_for_bed (for hero),
    &route_exit_hut3[0],            //  6: character_1x_GUARD_14/15, character_2x_PRISONER_4/5/6 by wake_up & go_to_time_for_bed

    // character_sleeps routes
    &route_prisoner_sleeps_1[0],    //  7: prisoner 1 by character_bed_common
    &route_prisoner_sleeps_2[0],    //  8: prisoner 2 by character_bed_common
    &route_prisoner_sleeps_3[0],    //  9: prisoner 3 by character_bed_common
    &route_prisoner_sleeps_1[0],    // 10: prisoner 4 by character_bed_common /* dupes index 7 */
    &route_prisoner_sleeps_2[0],    // 11: prisoner 5 by character_bed_common /* dupes index 8 */
    &route_prisoner_sleeps_3[0],    // 12: prisoner 6 by character_bed_common /* dupes index 9 */

    &route_77DE[0],                 // 13: (all hostiles) by character_bed_common

    &route_go_to_yard[0],           // 14: set by set_route_go_to_yard_reversed (for hero, prisoners_and_guards-1), set_route_go_to_yard (for hero, prisoners_and_guards-1)
    &route_go_to_yard[0],           // 15: set by set_route_go_to_yard_reversed (for hero, prisoners_and_guards-2), set_route_go_to_yard (for hero, prisoners_and_guards-2)  /* dupes index 14 */

    &route_breakfast_room_25[0],    // 16: set by set_route_go_to_breakfast, end_of_breakfast (for hero, reversed), prisoners_and_guards-1 by end_of_breakfast
    &route_breakfast_room_23[0],    // 17:                                                                          prisoners_and_guards-2 by end_of_breakfast

    // character_sits routes
    &route_prisoner_sits_1[0],      // 18: character_20_PRISONER_1 by charevnt_breakfast_common  // character next to player when player seated
    &route_prisoner_sits_2[0],      // 19: character_21_PRISONER_2 by charevnt_breakfast_common
    &route_prisoner_sits_3[0],      // 20: character_22_PRISONER_3 by charevnt_breakfast_common
    &route_prisoner_sits_1[0],      // 21: character_23_PRISONER_4 by charevnt_breakfast_common  /* dupes index 18 */
    &route_prisoner_sits_2[0],      // 22: character_24_PRISONER_5 by charevnt_breakfast_common  /* dupes index 19 */
    &route_prisoner_sits_3[0],      // 23: character_25_PRISONER_6 by charevnt_breakfast_common  /* dupes index 20 */ // looks like it belongs with above chunk but not used (bug)

    &route_guardA_breakfast[0],     // 24: "even guard" by charevnt_breakfast_common
    &route_guardB_breakfast[0],     // 25: "odd guard" by charevnt_breakfast_common

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

    &route_go_to_solitary[0],       // 36: set by charevnt_hero_release (for commandant?), tested by route_ended
    &route_hero_leave_solitary[0],  // 37: set by charevnt_hero_release (for hero)

    &route_guard_12_bed[0],         // 38: guard 12 at 'time for bed', 'search light'
    &route_guard_13_bed[0],         // 39: guard 13 at 'time for bed', 'search light'
    &route_guard_14_bed[0],         // 40: guard 14 at 'time for bed', 'search light'
    &route_guard_15_bed[0],         // 41: guard 15 at 'time for bed', 'search light'

    &route_hut2_left_to_right[0],   // 42:

    &route_7833[0],                 // 43: set by charevnt_breakfast_vischar, process_player_input (was at breakfast case)
    &route_hut2_right_to_left[0],   // 44: set by process_player_input (was in bed case), set by character_bed_vischar (for the hero)

    &route_hero_roll_call[0],       // 45: (hero) by go_to_roll_call

    // original game has an FF byte here
  };

  /* Conv: The index may have its reverse flag set so mask it off. The
   * original game gets this for free when scaling the index. */
  index &= ~ROUTEINDEX_REVERSE_FLAG;

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
  static const mappos8_t solitary_pos =
  {
    58, 42, 24
  };

  /**
   * $CC31: Partial character struct.
   */
  static const characterstruct_t commandant_reset_data =
  {
    0,               /* character_and_flags */
    room_0_OUTDOORS, /* room */
    { 116, 100, 3 }, /* pos */
    { 36, 0 }        /* route */
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
      mappos8_t *itempos;        /* was HL */
      uint8_t    area;           /* was A' */

      item_and_flags = pitemstruct->item_and_flags;
      itempos = &pitemstruct->mappos;
      area = 0; /* iterate 0,1,2 */
      do
        /* If the item is within the camp bounds then it will be discovered.
         */
        if (within_camp_bounds(area, itempos))
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
  memcpy(&state->character_structs[character_0_COMMANDANT].room,
         &commandant_reset_data.room,
         sizeof(characterstruct_t) - offsetof(characterstruct_t, room));

  /* Queue solitary messages. */
  queue_message(state, message_YOU_ARE_IN_SOLITARY);
  queue_message(state, message_WAIT_FOR_RELEASE);
  queue_message(state, message_ANOTHER_DAY_DAWNS);

  state->in_solitary = 255; /* inhibit user input */
  state->automatic_player_counter = 0; /* immediately take automatic control of hero */
  state->vischars[0].mi.sprite = &sprites[sprite_PRISONER_FACING_AWAY_1];
  vischar = &state->vischars[0];
  state->IY = &state->vischars[0];
  vischar->direction = direction_BOTTOM_LEFT;
  state->vischars[0].route.index = routeindex_0_HALT; /* Stand still. */ // stores a byte, not a word
  transition(state, &solitary_pos);

  NEVER_RETURNS;
}

/* ----------------------------------------------------------------------- */

/**
 * $CC37: Hostiles follow the hero.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character (hostile only). (was IY / HL)
 */
void guards_follow_suspicious_character(tgestate_t *state,
                                        vischar_t  *vischar)
{
  character_t  character;    /* was A */
  mappos8_t   *mappos_stash; /* was DE */
  mappos16_t  *mappos;       /* was HL */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  /* Conv: Copy of vischar in HL factored out. */

  character = vischar->character;

  /* Wearing the uniform stops anyone but the commandant from pursuing the
   * hero. */
  if (character != character_0_COMMANDANT &&
      state->vischars[0].mi.sprite == &sprites[sprite_GUARD_FACING_AWAY_1])
    return;

  /* If this (hostile) character saw the bribe being used then ignore the
   * hero. */
  if (vischar->flags == vischar_PURSUIT_SAW_BRIBE)
    return;

  /* Do line of sight checking when outdoors. */

  mappos = &vischar->mi.mappos; /* was HL += 15 */
  mappos_stash = &state->mappos_stash; // TODO: Find out if this use of mappos_stash ever intersects with the use of mappos_stash for the mask rendering.
  if (state->room_index == room_0_OUTDOORS)
  {
    mappos8_t *hero_mappos; /* was HL */
    int        dir;         /* Conv: was carry */
    uint8_t    direction;   /* was A / C */

    scale_mappos_down(mappos, mappos_stash); // stash_pos = vischar.mi.pos

    hero_mappos = &state->hero_mappos;
    /* Conv: Dupe pos pointer factored out. */

    direction = vischar->direction; /* This character's direction. */
    /* Conv: Avoided shifting 'direction' here. Propagated shift into later ops. */

    if ((direction & 1) == 0)
    {
      /* TL or BR */

      /* Does the hero approximately match our Y coordinate? */
      if (mappos_stash->v - 1 >= hero_mappos->v ||
          mappos_stash->v + 1 <  hero_mappos->v)
        return;

      /* Are we facing the hero? */
      dir = mappos_stash->u < hero_mappos->u;
      if ((direction & 2) == 0)
        dir = !dir;
      if (dir)
        return;
    }
    else
    {
      /* TR or BL */

      /* Does the hero approximately match our X coordinate? */
      if (mappos_stash->u - 1 >= hero_mappos->u ||
          mappos_stash->u + 1 <  hero_mappos->u)
        return;

      /* Are we facing the hero? */
      dir = mappos_stash->v < hero_mappos->v;
      if ((direction & 2) == 0)
        dir = !dir;
      if (dir)
        return;
    }
  }

  if (!state->red_flag)
  {
    /* Hostiles *not* in guard towers hassle the hero. */
    if (vischar->mi.mappos.w < 32) /* uses A as temporary */
      vischar->flags = vischar_PURSUIT_HASSLE;
  }
  else
  {
    state->bell = bell_RING_PERPETUAL;
    hostiles_pursue(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CCAB: Hostiles pursue prisoners.
 *
 * For all visible, hostile characters, at height < 32, set the pursue flag.
 *
 * \param[in] state Pointer to game state.
 */
void hostiles_pursue(tgestate_t *state)
{
  vischar_t *vischar; /* was HL */
  uint8_t    iters;   /* was B */

  assert(state != NULL);

  vischar = &state->vischars[1];
  iters   = vischars_LENGTH - 1;
  do
  {
    if (vischar->character <= character_19_GUARD_DOG_4 &&
        vischar->mi.mappos.w < 32) /* Excludes the guards high up in towers. */
      vischar->flags = vischar_PURSUIT_PURSUE;
    vischar++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $CCCD: Is item discoverable?
 *
 * Searches item_structs for items dropped nearby. If items are found the
 * hostiles are made to pursue the hero.
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
    /* Interior. */

    if (is_item_discoverable_interior(state, room, NULL) == 0)
      /* Item was found. */
      hostiles_pursue(state);
  }
  else
  {
    /* Exterior. */

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
    /* FUTURE: Merge into loop. */
    item = itemstruct->item_and_flags & itemstruct_ITEM_MASK;

    /* The green key and food items are ignored. */
    if (item == item_GREEN_KEY || item == item_FOOD)
      goto next;

    /* BUG FIX: In the original game the itemstruct pointer is decremented to
     * access item_and_flags but is not re-adjusted afterwards. */

    hostiles_pursue(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CCFB: Is an item discoverable indoors?
 *
 * A discoverable item is one which has been moved away from its default
 * room, and one that isn't the red cross parcel.
 *
 * \param[in]  state Pointer to game state.
 * \param[in]  room  Room index to check against. (was A)
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
        /* BUG: room_and_flags doesn't get its flag bits masked off.
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
 * $CD31: An item is discovered.
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

  /* BUG FIX: 'item' was not masked in the original code. */
  item &= itemstruct_ITEM_MASK;

  ASSERT_ITEM_VALID(item);

  queue_message(state, message_ITEM_DISCOVERED);
  decrease_morale(state, 5);

  default_item_location = &default_item_locations[item];
  room = default_item_location->room_and_flags;

  itemstruct = &state->item_structs[item];
  itemstruct->item_and_flags &= ~itemstruct_ITEM_FLAG_HELD;
  itemstruct->room_and_flags = default_item_location->room_and_flags;
  itemstruct->mappos.u = default_item_location->mappos.u;
  itemstruct->mappos.v = default_item_location->mappos.v;

  if (room == room_0_OUTDOORS)
  {
    /* Conv: The original code assigned 'room' here since it's already zero. */
    itemstruct->mappos.w = 0;
    calc_exterior_item_isopos(itemstruct);
  }
  else
  {
    itemstruct->mappos.w = 5;
    calc_interior_item_isopos(itemstruct);
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

/* $CF06: Animations
 *
 * These anim_t's are written here as arrays of uint8_t to allow the variable
 * length final member to be initialised.
 *
 * Conv: These were formerly located at the end of the sprite definitions but
 * moved here so they precede the parent array animations[].
 */

/* Define shorthands for directions and crawl flag. */
#define TL direction_TOP_LEFT
#define TR direction_TOP_RIGHT
#define BR direction_BOTTOM_RIGHT
#define BL direction_BOTTOM_LEFT
#define CR vischar_DIRECTION_CRAWL
#define NO 255

/* Define a shorthand for the flip flag. */
#define F sprite_FLAG_FLIP
#define _ 0

/* "Do nothing" animations for crawling */
static const uint8_t anim_crawlwait_tl[] = { 1, TL|CR,TL|CR, NO,  0, 0, 0, _|10 };
static const uint8_t anim_crawlwait_tr[] = { 1, TR|CR,TR|CR, NO,  0, 0, 0, F|10 };
static const uint8_t anim_crawlwait_br[] = { 1, BR|CR,BR|CR, NO,  0, 0, 0, F| 8 };
static const uint8_t anim_crawlwait_bl[] = { 1, BL|CR,BL|CR, NO,  0, 0, 0, _| 8 };

/* Walking animations */
static const uint8_t anim_walk_tl[]      = { 4, TL,TL,       BR,  2, 0, 0, _| 0,
                                                                  2, 0, 0, _| 1,
                                                                  2, 0, 0, _| 2,
                                                                  2, 0, 0, _| 3 };
static const uint8_t anim_walk_tr[]      = { 4, TR,TR,       BL,  0, 2, 0, F| 0,
                                                                  0, 2, 0, F| 1,
                                                                  0, 2, 0, F| 2,
                                                                  0, 2, 0, F| 3 };
static const uint8_t anim_walk_br[]      = { 4, BR,BR,       TL, -2, 0, 0, _| 4,
                                                                 -2, 0, 0, _| 5,
                                                                 -2, 0, 0, _| 6,
                                                                 -2, 0, 0, _| 7 };
static const uint8_t anim_walk_bl[]      = { 4, BL,BL,       TR,  0,-2, 0, F| 4,
                                                                  0,-2, 0, F| 5,
                                                                  0,-2, 0, F| 6,
                                                                  0,-2, 0, F| 7 };

/* "Do nothing" animations */
static const uint8_t anim_wait_tl[]      = { 1, TL,TL,       NO,  0, 0, 0, _| 0 };
static const uint8_t anim_wait_tr[]      = { 1, TR,TR,       NO,  0, 0, 0, F| 0 };
static const uint8_t anim_wait_br[]      = { 1, BR,BR,       NO,  0, 0, 0, _| 4 };
static const uint8_t anim_wait_bl[]      = { 1, BL,BL,       NO,  0, 0, 0, F| 4 };

/* Turning animations */
static const uint8_t anim_turn_tl[]      = { 2, TL,TR,       NO,  0, 0, 0, _| 0,
                                                                  0, 0, 0, F| 0 };
static const uint8_t anim_turn_tr[]      = { 2, TR,BR,       NO,  0, 0, 0, F| 0,
                                                                  0, 0, 0, _| 4 };
static const uint8_t anim_turn_br[]      = { 2, BR,BL,       NO,  0, 0, 0, _| 4,
                                                                  0, 0, 0, F| 4 };
static const uint8_t anim_turn_bl[]      = { 2, BL,TL,       NO,  0, 0, 0, F| 4,
                                                                  0, 0, 0, _| 0 };

/* Crawling animations */
static const uint8_t anim_crawl_tl[]     = { 2, TL|CR,TL|CR, BR,  2, 0, 0, _|10,
                                                                  2, 0, 0, _|11 };
static const uint8_t anim_crawl_tr[]     = { 2, TR|CR,TR|CR, BL,  0, 2, 0, F|10,
                                                                  0, 2, 0, F|11 };
static const uint8_t anim_crawl_br[]     = { 2, BR|CR,BR|CR, TL, -2, 0, 0, F| 8,
                                                                 -2, 0, 0, F| 9 };
static const uint8_t anim_crawl_bl[]     = { 2, BL|CR,BL|CR, TR,  0,-2, 0, _| 8,
                                                                  0,-2, 0, _| 9 };

/* Turning while crawling animations */
static const uint8_t anim_crawlturn_tl[] = { 2, TL|CR,TR|CR, NO,  0, 0, 0, _|10,
                                                                  0, 0, 0, F|10 };
static const uint8_t anim_crawlturn_tr[] = { 2, TR|CR,BR|CR, NO,  0, 0, 0, F|10,
                                                                  0, 0, 0, F| 8 };
static const uint8_t anim_crawlturn_br[] = { 2, BR|CR,BL|CR, NO,  0, 0, 0, F| 8,
                                                                  0, 0, 0, _| 8 };
static const uint8_t anim_crawlturn_bl[] = { 2, BL|CR,TL|CR, NO,  0, 0, 0, _| 8,
                                                                  0, 0, 0, _|10 };

#undef _
#undef F

#undef NO
#undef CR
#undef BL
#undef BR
#undef TR
#undef TL

/**
 * $CDF2: Array of pointers to animations.
 */
const anim_t *animations[animations__LIMIT] =
{
  // Comments here record which animation is selected by animindices[].
  // [facing direction:user input]
  //

  // Normal animations, move the map
  (const anim_t *) anim_walk_tl, //  0 [TL:Up,    TL:Up+Left]
  (const anim_t *) anim_walk_tr, //  1 [TR:Right, TR:Up+Right]
  (const anim_t *) anim_walk_br, //  2 [BR:Down,  BR:Down+Right]
  (const anim_t *) anim_walk_bl, //  3 [BL:Left,  BL:Down+Left]

  // Normal + turning
  (const anim_t *) anim_turn_tl, //  4 [TL:Down, TL:Right,     TL:Up+Right,  TL:Down+Right, TR:Up,    TR:Up+Left]
  (const anim_t *) anim_turn_tr, //  5 [TR:Down, TR:Left,      TR:Down+Left, TR:Down+Right, BR:Up,    BR:Up+Left, BR:Right, BR:Up+Right]
  (const anim_t *) anim_turn_br, //  6 [BR:Left, BR:Down+Left, BL:Down,      BL:Down+Right]
  (const anim_t *) anim_turn_bl, //  7 [TL:Left, TL:Down+Left, BL:Up,        BL:Up+Left,    BL:Right, BL:Up+Right]

  // Standing still
  (const anim_t *) anim_wait_tl, //  8 [TL:None]
  (const anim_t *) anim_wait_tr, //  9 [TR:None]
  (const anim_t *) anim_wait_br, // 10 [BR:None]
  (const anim_t *) anim_wait_bl, // 11 [BL:None]

  // Note that when crawling the hero won't spin around in the tunnel. He'll
  // retain his orientation until specficially spun around using the controls
  // opposite to the direction of the tunnel.

  // Crawling (map movement is enabled which is odd... we never move the map when crawling)
  (const anim_t *) anim_crawl_tl, // 12 [TL+crawl:Up,   TL+crawl:Down,      TL+crawl:Up+Left, TL+crawl:Down+Right]
  (const anim_t *) anim_crawl_tr, // 13 [TR+crawl:Left, TR+crawl:Right,     TR+crawl:Up+Right]
  (const anim_t *) anim_crawl_br, // 14 [BR+crawl:Up,   BR+crawl:Down,      BR+crawl:Up+Left, BR+crawl:Down+Left, BR+crawl:Down+Right]
  (const anim_t *) anim_crawl_bl, // 15 [BL+crawl:Left, BL+crawl:Down+Left, BL+crawl:Right,   BL+crawl:Up+Right]

  // Crawling + turning
  (const anim_t *) anim_crawlturn_tl, // 16 [TL+crawl:Right, TL+crawl:Up+Right,   TR+crawl:Up,    TR+crawl:Up+Left]   // turn to face TR
  (const anim_t *) anim_crawlturn_tr, // 17 [TR+crawl:Down,  TR+crawl:Down+Right, BR+crawl:Right, BR+crawl:Up+Right]  // turn to face BR
  (const anim_t *) anim_crawlturn_br, // 18 [BR+crawl:Left,  BL+crawl:Down,       BL+crawl:Down+Right]                // turn to face BL
  (const anim_t *) anim_crawlturn_bl, // 19 [TL+crawl:Left,  TL+crawl:Down+Left,  BL+crawl:Up,    BL+crawl:Up+Left]   // turn to face TL

  // Crawling still
  (const anim_t *) anim_crawlwait_tl, // 20 [TL+crawl:None]
  (const anim_t *) anim_crawlwait_tr, // 21 [TR+crawl:None, TR+crawl:Down+Left] // looks like it ought to be 18, not 21
  (const anim_t *) anim_crawlwait_br, // 22 [BR+crawl:None]
  (const anim_t *) anim_crawlwait_bl, // 23 [BL+crawl:None]
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
  pos8_t        map_xy;     /* was D, E */
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
    const pos8_t isopos = itemstruct->isopos; /* new */

    if ((itemstruct->room_and_flags & itemstruct_ROOM_MASK) == room &&
        (map_xy.x - 2 <= isopos.x && map_xy.x + (state->columns - 1) >= isopos.x) &&
        (map_xy.y - 1 <= isopos.y && map_xy.y + (state->rows    - 1) >= isopos.y))
      itemstruct->room_and_flags |= itemstruct_ROOM_FLAG_NEARBY_6 | itemstruct_ROOM_FLAG_NEARBY_7; /* set */
    else
      itemstruct->room_and_flags &= ~(itemstruct_ROOM_FLAG_NEARBY_6 | itemstruct_ROOM_FLAG_NEARBY_7); /* reset */

    itemstruct++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $DBEB: Find the next item to draw that is furthest behind (u,v).
 *
 * Leaf.
 *
 * \param[in]  state         Pointer to game state.
 * \param[in]  item_and_flag Initial item_and_flag, passed through if no itemstruct is found. (was A')
 * \param[in]  u             U pos? Compared to U. (was BC')
 * \param[in]  v             V pos? Compared to V. (was DE')
 * \param[out] pitemstr      Returned pointer to item struct. (was IY)
 *
 * \return item+flag. (was A')
 */
uint8_t get_next_drawable_itemstruct(tgestate_t    *state,
                                     item_t         item_and_flag,
                                     uint16_t       u,
                                     uint16_t       v,
                                     itemstruct_t **pitemstr)
{
  uint8_t             iters;   /* was B */
  const itemstruct_t *itemstr; /* was HL */

  assert(state    != NULL);
  assert(pitemstr != NULL);

  *pitemstr = NULL; /* Conv: Added safety initialisation. */

  /* Find the rearmost itemstruct that is flagged for drawing. */

  iters   = item__LIMIT;
  itemstr = &state->item_structs[0]; /* Conv: Original pointed to itemstruct->room_and_flags. */
  do
  {
    /* Select an item if it's behind the point (u,v). */
    const enum itemstruct_room_and_flags FLAGS = itemstruct_ROOM_FLAG_NEARBY_6 |
                                                 itemstruct_ROOM_FLAG_NEARBY_7;
    /* Conv: Original calls out to multiply by 8, HL' is temp. */
    if ((itemstr->room_and_flags & FLAGS) == FLAGS &&
        (itemstr->mappos.u * 8 > u) &&
        (itemstr->mappos.v * 8 > v))
    {
      const mappos8_t *mappos; /* was HL' */

      mappos = &itemstr->mappos; /* Conv: Was direct pointer, now points to member. */
      /* Calculate (u,v) for the next iteration. */
      v = mappos->v * 8;
      u = mappos->u * 8;
      *pitemstr = (itemstruct_t *) itemstr;

      /* The original code has an unpaired A register exchange here. If the
       * loop continues then it's unclear which output register is used. */
      /* It seems that A' is the output register, irrespective. */
      item_and_flag = (item__LIMIT - iters) | item_FOUND; /* item index + 'item found' flag */
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
//  pos8_t       *HL; /* was HL */
//  mappos_t     *DE; /* was DE */
//  uint16_t      BC; /* was BC */
  uint8_t       left_skip;      /* was B */
  uint8_t       clipped_width;  /* was C */
  uint8_t       top_skip;       /* was D */
  uint8_t       clipped_height; /* was E */
  uint8_t       instr;          /* was A */
  uint8_t       A;              /* was A */
  uint8_t       offset_tmp;     /* was A' */
  uint8_t       iters;          /* was B' */
  uint8_t       offset;         /* was C' */
//  uint16_t      DEdash; /* was DE' */
  const size_t *enables;        /* was HL' */
  int16_t       x, y;           /* was HL, DE */ /* signed needed for X calc */
  uint8_t      *maskbuf;        /* was HL */
  uint16_t      skip;           /* was DE */

  assert(state   != NULL);
  ASSERT_ITEMSTRUCT_VALID(itemstr);

  /* 0x3F looks like it ought to be 0x1F (item__LIMIT - 1). */
  /* Potential BUG: The use of A later on does not re-clamp it to 0x1F. */
  item &= 0x3F; // mask off item_FOUND
  ASSERT_ITEM_VALID(item);

  /* BUG: The original game writes to this location but it's never
   * subsequently read from.
   */
  /* $8213 = A; */

  state->mappos_stash = itemstr->mappos;
  state->isopos       = itemstr->isopos;
// after LDIR we have:
//  HL = &IY->pos.x + 5;
//  DE = &state->mappos_stash + 5;
//  BC = 0;
//
//  // EX DE, HL
//
//  *DE = 0; // was B, which is always zero here
  state->sprite_index   = 0; /* Items are never drawn flipped. */

  state->item_height    = item_definitions[item].height;
  state->bitmap_pointer = item_definitions[item].bitmap;
  state->mask_pointer   = item_definitions[item].mask;
  if (item_visible(state,
                  &left_skip,
                  &clipped_width,
                  &top_skip,
                  &clipped_height))
    return 0; /* invisible */

  // PUSH left_skip + clipped_width
  // PUSH top_skip + clipped_height

  state->self_E2C2 = clipped_height; // self modify masked_sprite_plotter_16_wide_left

  if (left_skip == 0)
  {
    instr = 119; /* opcode of 'LD (HL),A' */
    offset_tmp = clipped_width;
  }
  else
  {
    instr = 0; /* opcode of 'NOP' */
    offset_tmp = 3 - clipped_width;
  }

  offset = offset_tmp; /* FUTURE: Could assign directly to offset instead. */

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

  /* Calculate Y plotting offset.
   * The full calculation can be avoided if there are rows to skip since
   * the sprite is always aligned to the top of the screen in that case. */
  y = 0; /* Conv: Moved. */
  if (top_skip == 0) /* no rows to skip */
    y = (state->isopos.y - state->map_position.y) * state->window_buf_stride;

  // (state->isopos.y - state->map_position.y) never seems to exceed $10 in the original game, but does for me
  // state->isopos.y seems to match, but state->map_position.y is 2 bigger

  /* Calculate X plotting offset. */

  // Conv: Assigns to signed var.
  x = state->isopos.x - state->map_position.x;

  state->window_buf_pointer = &state->window_buf[x + y]; // window buffer start address
  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer, 3);
  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer + (clipped_height - 1) * state->columns + clipped_width - 1, 3);

  maskbuf = &state->mask_buffer[0];

  // POP DE  // get top_skip + clipped_height
  // PUSH DE

  state->foreground_mask_pointer = &maskbuf[top_skip * 4];
  ASSERT_MASK_BUF_PTR_VALID(state->foreground_mask_pointer);

  // POP DE  // this pop/push seems to be needless - DE already has the value
  // PUSH DE

  A = top_skip; // A = D // sign?
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
    // top_skip = 0
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
 * $DD02: Clips the given item's dimensions against the game window.
 *
 * Returns 1 if the item is outside the game window, or zero if visible.
 *
 * Counterpart to \ref vischar_visible.
 *
 * Called by setup_item_plotting only.
 *
 * \param[in]  state          Pointer to game state.
 * \param[out] left_skip      Pointer to returned left edge skip. (was B)
 * \param[out] clipped_width  Pointer to returned clipped width.  (was C)
 * \param[out] top_skip       Pointer to returned top edge skip.  (was D)
 * \param[out] clipped_height Pointer to returned ciipped height. (was E)
 *
 * \return 0 => visible, 1 => invisible. (was A)
 */
uint8_t item_visible(tgestate_t *state,
                     uint8_t    *left_skip,
                     uint8_t    *clipped_width,
                     uint8_t    *top_skip,
                     uint8_t    *clipped_height)
{
#define WIDTH_BYTES 3 /* in bytes - items are always 16 pixels wide */
#define HEIGHT      2 /* in UDGs - items are ~ 16 pixels high */

  const pos8_t *pisopos;            /* was HL */
  pos8_t        map_position;       /* was DE */
  uint8_t       window_right_edge;  /* was A  */
  int8_t        available_right;    /* was A  */
  uint8_t       new_left;           /* was B  */
  uint8_t       new_width;          /* was C  */
  uint8_t       item_right_edge;    /* was A  */
  int8_t        available_left;     /* was A  */
  uint8_t       window_bottom_edge; /* was A  */
  int8_t        available_bottom;   /* was A  */
  uint8_t       new_top;            /* was D  */
  uint8_t       new_height;         /* was E  */
  uint8_t       item_bottom_edge;   /* was A  */
  uint8_t       available_top;      /* was A  */

  assert(state          != NULL);
  assert(clipped_width  != NULL);
  assert(clipped_height != NULL);

  /* Conv: Added to guarantee initialisation of return values. */
  *left_skip      = 255;
  *clipped_width  = 255;
  *top_skip       = 255;
  *clipped_height = 255;

  /* To determine visibility and sort out clipping there are five cases to
   * consider per axis:
   * (A) item is completely off left/top of screen
   * (B) item is clipped/truncated on its left/top
   * (C) item is entirely visible
   * (D) item is clipped/truncated on its right/bottom
   * (E) item is completely off right/bottom of screen
   */

  /*
   * Handle horizontal intersections.
   */

  pisopos = &state->isopos;
  map_position = state->map_position;

  /* Conv: Columns was constant 24; replaced with state var. */
  window_right_edge = map_position.x + state->columns;

  /* Subtracting item's x gives the space available between item's left edge
   * and the right edge of the window (in bytes).
   * Note that available_right is signed to enable the subsequent test. */
  available_right = window_right_edge - pisopos->x;
  if (available_right <= 0)
    /* Case (E): Item is beyond the window's right edge. */
    goto invisible;

  /* Check for item partway off-screen. */
  if (available_right < WIDTH_BYTES)
  {
    /* Case (D): Item's right edge is off-screen, so truncate width. */
    new_left  = 0;
    new_width = available_right;
  }
  else
  {
    /* Calculate the right edge of the item. */
    item_right_edge = pisopos->x + WIDTH_BYTES;

    /* Subtracting the map position's x gives the space available between
     * item's right edge and the left edge of the window (in bytes).
     * Note that available_left is signed to allow the subsequent test. */
    available_left = item_right_edge - map_position.x;
    if (available_left <= 0)
      /* Case (A): Item's right edge is beyond the window's left edge. */
      goto invisible;

    /* Check for item partway off-screen. */
    if (available_left < WIDTH_BYTES)
    {
      /* Case (B): Left edge is off-screen and right edge is on-screen.
       * Pack the lefthand skip into the low byte and the clipped width into
       * the high byte. */
      new_left  = WIDTH_BYTES - available_left;
      new_width = available_left;
    }
    else
    {
      /* Case (C): No clipping. */
      new_left  = 0;
      new_width = WIDTH_BYTES;
    }
  }

  /*
   * Handle vertical intersections.
   */

  /* Note: The companion code in vischar_visible uses 16-bit quantities
   * in its equivalent of the following. */

  /* Calculate the bottom edge of the window in map space. */
  /* Conv: Rows was constant 17; replaced with state var. */
  window_bottom_edge = map_position.y + state->rows;

  /* Subtracting the item's y gives the space available between window's
   * bottom edge and the item's top. */
  available_bottom = window_bottom_edge - pisopos->y;
  if (available_bottom <= 0)
    /* Case (E): Item is beyond the window's bottom edge. */
    goto invisible;

  /* Check for item partway off-screen. */
  if (available_bottom < HEIGHT)
  {
    /* Case (D): Item's bottom edge is off-screen, so truncate height. */
    new_top    = 0;
    new_height = 8;
  }
  else
  {
    /* Calculate the bottom edge of the item. */
    item_bottom_edge = pisopos->y + HEIGHT;

    /* Subtracting the map position's y (scaled) gives the space available
     * between item's bottom edge and the top edge of the window (in UDGs).
     */
    available_top = item_bottom_edge - map_position.y;
    if (available_top <= 0)
      goto invisible;

    /* Check for item partway off-screen. */
    if (available_top < HEIGHT)
    {
      /* Case (B): Top edge is off-screen and bottom edge is on-screen.
       * Pack the top skip into the low byte and the clipped height into the
       * high byte. */
      new_top    = 8;
      new_height = state->item_height - 8;
    }
    else
    {
      /* Case (C): No clipping. */
      new_top    = 0;
      new_height = state->item_height;
    }
  }

  *left_skip      = new_left;
  *clipped_width  = new_width;
  *top_skip       = new_top;
  *clipped_height = new_height;

  return 0; /* Visible */


invisible:
  return 1;
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
  offsetof(tgestate_t, enable_16_left_1),
  offsetof(tgestate_t, enable_16_right_1),
  offsetof(tgestate_t, enable_16_left_2),
  offsetof(tgestate_t, enable_16_right_2),
  offsetof(tgestate_t, enable_16_left_3),
  offsetof(tgestate_t, enable_16_right_3),
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
void masked_sprite_plotter_24_wide_vischar(tgestate_t *state,
                                           vischar_t  *vischar)
{
  uint8_t        x;           /* was A */
  uint8_t        iters;       /* was B */
  const uint8_t *maskptr;     /* was ? */
  const uint8_t *bitmapptr;   /* was ? */
  const uint8_t *foremaskptr; /* was ? */
  uint8_t       *screenptr;   /* was ? */

  assert(state   != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  if ((x = (vischar->isopos.x & 7)) < 4)
  {
    /* Shift right? */

    x = ~x & 3; // jump table offset (on input, A is 0..3)

    maskptr   = state->mask_pointer;
    bitmapptr = state->bitmap_pointer;

    assert(maskptr   != NULL);
    assert(bitmapptr != NULL);

    iters = state->self_E121; // clipped_height & 0xFF
    assert(iters <= MASK_BUFFER_HEIGHT * 8);
    do
    {
      uint8_t bm0, bm1, bm2, bm3;         /* was B, C, E, D */
      uint8_t mask0, mask1, mask2, mask3; /* was B', C', E', D' */
      int     carry = 0;                  /* was flags */

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
      ASSERT_WINDOW_BUF_PTR_VALID(screenptr, 3);

      /* Shift bitmap. */

      bm3 = 0;
      // Conv: Replaced self-modified goto with if-else chain.
      if (x <= 0)
      {
        SRL(bm0);
        RR(bm1);
        RR(bm2);
        RR(bm3);
      }
      if (x <= 1)
      {
        SRL(bm0);
        RR(bm1);
        RR(bm2);
        RR(bm3);
      }
      if (x <= 2)
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
      if (x <= 0)
      {
        RR(mask0);
        RR(mask1);
        RR(mask2);
        RR(mask3);
      }
      if (x <= 1)
      {
        RR(mask0);
        RR(mask1);
        RR(mask2);
        RR(mask3);
      }
      if (x <= 2)
      {
        RR(mask0);
        RR(mask1);
        RR(mask2);
        RR(mask3);
      }

      /* Plot, using foreground mask. */

      // Conv: We form 'mask' inside of the enable_* condition. The original
      // game got away with accessing out-of-bounds memory but we can't.

      if (state->enable_24_right_1)
        *screenptr = MASK(bm0, mask0);
      foremaskptr++;
      screenptr++;

      if (state->enable_24_right_2)
        *screenptr = MASK(bm1, mask1);
      foremaskptr++;
      screenptr++;

      if (state->enable_24_right_3)
        *screenptr = MASK(bm2, mask2);
      foremaskptr++;
      screenptr++;

      if (state->enable_24_right_4)
        *screenptr = MASK(bm3, mask3);
      foremaskptr++;
      state->foreground_mask_pointer = foremaskptr;

      screenptr += state->columns - 3; /* was 21 */
      if (iters > 1)
        ASSERT_WINDOW_BUF_PTR_VALID(screenptr, 3);
      state->window_buf_pointer = screenptr;
    }
    while (--iters);
  }
  else
  {
    /* Shift left? */

    x -= 4; // (on input, A is 4..7 -> 0..3)

    maskptr   = state->mask_pointer;
    bitmapptr = state->bitmap_pointer;

    assert(maskptr   != NULL);
    assert(bitmapptr != NULL);

    iters = state->self_E1E2; // clipped_height & 0xFF
    assert(iters <= MASK_BUFFER_HEIGHT * 8);
    do
    {
      /* Note the different variable order to the case above. */
      uint8_t bm0, bm1, bm2, bm3;         /* was E, C, B, D */
      uint8_t mask0, mask1, mask2, mask3; /* was E', C', B', D' */
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
      ASSERT_WINDOW_BUF_PTR_VALID(screenptr, 3);

      /* Shift bitmap. */

      bm3 = 0;
      // Conv: Replaced self-modified goto with if-else chain.
      if (x <= 0)
      {
        SLA(bm0);
        RL(bm1);
        RL(bm2);
        RL(bm3);
      }
      if (x <= 1)
      {
        SLA(bm0);
        RL(bm1);
        RL(bm2);
        RL(bm3);
      }
      if (x <= 2)
      {
        SLA(bm0);
        RL(bm1);
        RL(bm2);
        RL(bm3);
      }
      if (x <= 3)
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
      if (x <= 0)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }
      if (x <= 1)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }
      if (x <= 2)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }
      if (x <= 3)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }

      /* Plot, using foreground mask. */

      // Conv: We form 'mask' inside of the enable_* condition. The original
      // game got away with accessing out-of-bounds memory but we can't.

      if (state->enable_24_left_1)
        *screenptr = MASK(bm3, mask3);
      foremaskptr++;
      screenptr++;

      if (state->enable_24_left_2)
        *screenptr = MASK(bm2, mask2);
      foremaskptr++;
      screenptr++;

      if (state->enable_24_left_3)
        *screenptr = MASK(bm1, mask1);
      foremaskptr++;
      screenptr++;

      if (state->enable_24_left_4)
        *screenptr = MASK(bm0, mask0);
      foremaskptr++;
      state->foreground_mask_pointer = foremaskptr;

      screenptr += state->columns - 3; /* was 21 */
      if (iters > 1)
        ASSERT_WINDOW_BUF_PTR_VALID(screenptr, 3);
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
void masked_sprite_plotter_16_wide_vischar(tgestate_t *state,
                                           vischar_t  *vischar)
{
  uint8_t x; /* was A */

  ASSERT_VISCHAR_VALID(vischar);

  x = vischar->isopos.x & 7;
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

  assert(state != NULL);
  // assert(x);

  ASSERT_MASK_BUF_PTR_VALID(state->foreground_mask_pointer);

  x = (~x & 3); // jump table offset (on input, A is 0..3 => 3..0)

  maskptr   = state->mask_pointer;
  bitmapptr = state->bitmap_pointer;

  assert(maskptr   != NULL);
  assert(bitmapptr != NULL);

  iters = state->self_E2C2; // (clipped height & 0xFF) // self modified by $E49D (setup_vischar_plotting)
  assert(iters <= MASK_BUFFER_HEIGHT * 8);

  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer, 2);
  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer + (iters - 1) * state->columns + 2 - 1, 2);
  do
  {
    uint8_t bm0, bm1, bm2;       /* was D, E, C */
    uint8_t mask0, mask1, mask2; /* was D', E', C' */
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
    if (x <= 0)
    {
      RR(mask0);
      RR(mask1);
      RR(mask2);
    }
    if (x <= 1)
    {
      RR(mask0);
      RR(mask1);
      RR(mask2);
    }
    if (x <= 2)
    {
      RR(mask0);
      RR(mask1);
      RR(mask2);
    }

    /* Shift bitmap. */

    bm2 = 0; // all bits clear => pixels OFF
    //carry = 0; in original code but never read in practice
    // Conv: Replaced self-modified goto with if-else chain.
    if (x <= 0)
    {
      SRL(bm0);
      RR(bm1);
      RR(bm2);
    }
    if (x <= 1)
    {
      SRL(bm0);
      RR(bm1);
      RR(bm2);
    }
    if (x <= 2)
    {
      SRL(bm0);
      RR(bm1);
      RR(bm2);
    }

    /* Plot, using foreground mask. */

    screenptr = state->window_buf_pointer; // this line is moved relative to the 24 version
    ASSERT_WINDOW_BUF_PTR_VALID(screenptr, 2);
    ASSERT_MASK_BUF_PTR_VALID(foremaskptr);

    // Conv: We form 'mask' inside of the enable_* condition. The original
    // game got away with accessing out-of-bounds memory but we can't.

    if (state->enable_16_left_1)
      *screenptr = MASK(bm0, mask0);
    foremaskptr++;
    screenptr++;

    if (state->enable_16_left_2)
      *screenptr = MASK(bm1, mask1);
    foremaskptr++;
    screenptr++;

    if (state->enable_16_left_3)
      *screenptr = MASK(bm2, mask2);
    foremaskptr += 2;
    state->foreground_mask_pointer = foremaskptr;

    screenptr += state->columns - 2; /* was 22 */
    if (iters > 1)
      ASSERT_WINDOW_BUF_PTR_VALID(screenptr, 2);
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

  assert(state != NULL);
  // assert(x);

  ASSERT_MASK_BUF_PTR_VALID(state->foreground_mask_pointer);

  x = (x - 4); // jump table offset (on input, 'x' is 4..7 => 0..3) // 6 = length of asm chunk

  maskptr   = state->mask_pointer;
  bitmapptr = state->bitmap_pointer;

  assert(maskptr   != NULL);
  assert(bitmapptr != NULL);

  iters = state->self_E363; // (clipped height & 0xFF) // self modified by $E49D (setup_vischar_plotting)
  assert(iters <= MASK_BUFFER_HEIGHT * 8);

  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer, 2);
  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer + (iters - 1) * state->columns + 2 - 1, 2);
  do
  {
    /* Note the different variable order to the 'left' case above. */
    uint8_t bm0, bm1, bm2;       /* was E, D, C */
    uint8_t mask0, mask1, mask2; /* was E', D', C' */
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
    if (x <= 0)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }
    if (x <= 1)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }
    if (x <= 2)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }
    if (x <= 3)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }

    /* Shift bitmap. */

    bm2 = 0; // all bits clear => pixels OFF
    // no carry reset in this variant?
    if (x <= 0)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }
    if (x <= 1)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }
    if (x <= 2)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }
    if (x <= 3)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }

    /* Plot, using foreground mask. */

    screenptr = state->window_buf_pointer; // this line is moved relative to the 24 version
    ASSERT_WINDOW_BUF_PTR_VALID(screenptr, 2);
    ASSERT_MASK_BUF_PTR_VALID(foremaskptr);

    // Conv: We form 'mask' inside of the enable_* condition. The original
    // game got away with accessing out-of-bounds memory but we can't.

    if (state->enable_16_right_1)
      *screenptr = MASK(bm2, mask2);
    foremaskptr++;
    screenptr++;

    if (state->enable_16_right_2)
      *screenptr = MASK(bm1, mask1);
    foremaskptr++;
    screenptr++;

    if (state->enable_16_right_3)
      *screenptr = MASK(bm0, mask0);
    foremaskptr += 2;
    state->foreground_mask_pointer = foremaskptr;

    screenptr += state->columns - 2; /* was 22 */
    if (iters > 1)
      ASSERT_WINDOW_BUF_PTR_VALID(screenptr, 2);
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
    offsetof(tgestate_t, enable_24_right_1),
    offsetof(tgestate_t, enable_24_left_1),
    offsetof(tgestate_t, enable_24_right_2),
    offsetof(tgestate_t, enable_24_left_2),
    offsetof(tgestate_t, enable_24_right_3),
    offsetof(tgestate_t, enable_24_left_3),
    offsetof(tgestate_t, enable_24_right_4),
    offsetof(tgestate_t, enable_24_left_4),

    /* These two addresses are present here in the original game but are
     * unreferenced. */
    // masked_sprite_plotter_16_wide_vischar
    // masked_sprite_plotter_24_wide_vischar
  };

  mappos16_t        *vischar_pos;    /* was HL */
  mappos8_t         *mappos_stash;   /* was DE */
  const spritedef_t *sprite;         /* was BC */
  spriteindex_t      sprite_index;   /* was A */
  const spritedef_t *sprite2;        /* was DE */
  uint8_t            left_skip;      /* was B */
  uint8_t            clipped_width;  /* was C */
  uint8_t            top_skip;       /* was D */
  uint8_t            clipped_height; /* was E */
  const size_t      *enables;        /* was HL */
  uint8_t            enable_count;   /* was $E4C0 */
  uint8_t            counter;        /* was A' */
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

  vischar_pos  = &vischar->mi.mappos;
  mappos_stash = &state->mappos_stash;

  if (state->room_index > room_0_OUTDOORS)
  {
    /* Indoors. */
    mappos_stash->u = vischar_pos->u; // note: narrowing
    mappos_stash->v = vischar_pos->v; // note: narrowing
    mappos_stash->w = vischar_pos->w; // note: narrowing
  }
  else
  {
    /* Outdoors. */
    /* Conv: Unrolled, removed divide-by-8 calls. */
    mappos_stash->u = (vischar_pos->u + 4) >> 3; /* with rounding */
    mappos_stash->v = (vischar_pos->v    ) >> 3; /* without rounding */
    mappos_stash->w = (vischar_pos->w    ) >> 3;
  }

  sprite = vischar->mi.sprite;

  // banked in A'
  state->sprite_index = sprite_index = vischar->mi.sprite_index; // set left/right flip flag / sprite offset

  // HL now points after sprite_index, at isopos
  // DE now points to state->isopos.x

  // unrolled versus original
  state->isopos.x = vischar->isopos.x >> 3;
  state->isopos.y = vischar->isopos.y >> 3;
  // 'isopos' here is now a scaled-down projected map coordinate

  // A is (1<<7) mask OR sprite offset
  // original game uses ADD A,A to double A and in doing so discards top bit, here we mask it off explicitly
  sprite2 = &sprite[sprite_index & ~sprite_FLAG_FLIP]; // sprite pointer

  vischar->width_bytes = sprite2->width;  /* width in bytes + 1 */
  vischar->height      = sprite2->height; /* height in rows */

  state->bitmap_pointer = sprite2->bitmap;
  state->mask_pointer   = sprite2->mask;

  if (vischar_visible(state,
                      vischar,
                     &left_skip,
                     &clipped_width,
                     &top_skip,
                     &clipped_height)) // used A as temporary
    return 0; /* invisible */

  // PUSH clipped_width
  // PUSH clipped_height

  E = clipped_height; // must be no of visible rows? // Conv: added

  assert(clipped_width < 100);
  assert(E < 100);

  if (vischar->width_bytes == 3) // 3 => 16 wide, 4 => 24 wide
  {
    state->self_E2C2 = E; // self modify masked_sprite_plotter_16_wide_left
    state->self_E363 = E; // self-modify masked_sprite_plotter_16_wide_right

    enable_count = 3;
    enables = &masked_sprite_plotter_16_enables[0];
  }
  else
  {
    state->self_E121 = E; // self-modify masked_sprite_plotter_24_wide_vischar (shift right case)
    state->self_E1E2 = E; // self-modify masked_sprite_plotter_24_wide_vischar (shift left case)

    enable_count = 4;
    enables = &masked_sprite_plotter_24_enables[0];
  }

  // PUSH HL_enables

  // enable_count = A; // self-modify
  // E = A
  // A = B
  if (left_skip == 0) // no lefthand skip: start with 'on'
  {
    instr = 119; /* opcode of 'LD (HL),A' */
    counter = clipped_width; // process this many bytes before clipping
  }
  else // lefthand skip present: start with 'off'
  {
    instr = 0; /* opcode of 'NOP' */
    counter = enable_count - clipped_width; // clip until this many bytes have been processed
  }

  // EXX
  // POP HL_enables

  /* Set the addresses in the jump table to NOP or LD (HL),A. */
  //Cdash = offset; // must be no of columns?
  iters = enable_count; /* 3 or 4 iterations */
  do
  {
    *(((uint8_t *) state) + *enables++) = instr;
    *(((uint8_t *) state) + *enables++) = instr;
    if (--counter == 0)
      instr ^= 119; /* Toggle between LD and NOP. */
  }
  while (--iters);

  // EXX

  assert(vischar->isopos.y / 8 - state->map_position.y < state->rows);
  assert(vischar->isopos.y / 8 - state->map_position.y + clipped_height / 8 <= state->rows);

  /* Calculate Y plotting offset.
   * The full calculation can be avoided if there are rows to skip since
   * the sprite is always aligned to the top of the screen in that case. */
  y = 0; /* Conv: Moved. */
  if (top_skip == 0) /* no rows to skip */
    y = (vischar->isopos.y - state->map_position.y * 8) * state->columns;

  assert(y / 8 / state->columns < state->rows);

  /* Calculate X plotting offset. */
  x = state->isopos.x - state->map_position.x; // signed subtract + extend to 16-bit

  assert(x >= -3 && x <= state->columns - 1); // found empirically

  state->window_buf_pointer = &state->window_buf[x + y];

  /* Note that it _is_ valid for window_buf_pointer to point outside of
   * window_buf. It can point beyond the end by up to the width of the sprite
   * (vischar->width_bytes - 1) [char->width_bytes is (vischar width in
   * bytes + 1)] and that's fine since the clipping enable flags ensure that
   * those locations are never written to.
   */

  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer, vischar->width_bytes - 1);
  ASSERT_WINDOW_BUF_PTR_VALID(state->window_buf_pointer + (clipped_height - 1) * state->columns + clipped_width - 1, vischar->width_bytes - 1);

  maskbuf = &state->mask_buffer[0];

  // POP DE_clipped_height
  // PUSH DE

  assert((top_skip * 4 + (vischar->isopos.y & 7) * 4) < 255);

  maskbuf += top_skip * 4 + (vischar->isopos.y & 7) * 4;
  ASSERT_MASK_BUF_PTR_VALID(maskbuf);
  state->foreground_mask_pointer = maskbuf;

  // POP DE_clipped_height

  A = top_skip; // offset/skip
  if (A)
  {
    /* Conv: The original game has a generic multiply loop here. In this
     * version instead we'll just multiply. */

    A *= vischar->width_bytes - 1;
    // top_skip = 0
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
 * $E542: Scale down a mappos16_t position, assigning the result to a mappos8_t.
 *
 * Divides the three input 16-bit words by 8, with rounding to nearest,
 * storing the result as bytes.
 *
 * \param[in]  in  Input mappos16_t. (was HL)
 * \param[out] out Output mappos8_t. (was DE)
 */
void scale_mappos_down(const mappos16_t *in, mappos8_t *out)
{
  const uint16_t *pcoordin;  /* was HL */
  uint8_t        *pcoordout; /* was DE */
  uint8_t         iters;     /* was B */

  assert(in  != NULL);
  assert(out != NULL);

  /* Use knowledge of the structure layout to cast the pointers to array[3]
   * of their respective types. */
  pcoordin  = (const uint16_t *) &in->u;
  pcoordout = (uint8_t *) &out->u;

  iters = 3;
  do
    *pcoordout++ = divround(*pcoordin++);
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $EED3: Plot the game screen.
 *
 * \param[in] state Pointer to game state.
 */
void plot_game_window(tgestate_t *state)
{
  uint8_t *const  screen = &state->speccy->screen.pixels[0];

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
    ASSERT_WINDOW_BUF_PTR_VALID(src, 0);
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
    ASSERT_WINDOW_BUF_PTR_VALID(src, 0);
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

  {
    static const zxbox_t dirty = { 7*8, 6*8, 30*8, 22*8 };
    state->speccy->draw(state->speccy, &dirty);
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
  uint16_t   range;   /* was DE */ // pos8_t?
  uint8_t   *pcoord;  /* was HL */
  uint8_t    iters;   /* was B */
  uint8_t    coord;   /* was A */
  vischar_t *vischar; /* was HL */

  assert(state != NULL);

  /* Is the hero within the roll call area bounds? */
  /* Conv: Unrolled. */
  /* Range checking. X in (114..124) and Y in (106..114). */
  range = map_ROLL_CALL_X;
  pcoord = &state->hero_mappos.u;
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
  queue_message(state, message_MISSED_ROLL_CALL);
  hostiles_pursue(state); /* was tail call */
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
  static const mappos8_t outside_main_gate = { 214, 138, 6 };

  uint16_t  range;  /* was DE */
  uint8_t  *pcoord; /* was HL */
  uint8_t   coord;  /* was A */

  assert(state != NULL);

  /* Is the hero within the main gate bounds? */
  // UNROLLED
  /* Range checking. X in (105..109) and Y in (73..75). */
  range = map_MAIN_GATE_X; // note the confused coords business
  pcoord = &state->hero_mappos.u;
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
  int     flags;   /* Conv: added */

  assert(state != NULL);

  screenlocstring_plot(state, &screenlocstring_confirm_y_or_n);

  /* Keyscan. */
  for (;;)
  {
    keymask = state->speccy->in(state->speccy, port_KEYBOARD_POIUY);
    if ((keymask & (1 << 4)) == 0)
    {
      flags = 0; /* is 'Y' pressed? return Z */
      goto exit;
    }

    keymask = state->speccy->in(state->speccy, port_KEYBOARD_SPACESYMSHFTMNB);
    keymask = ~keymask;
    if ((keymask & (1 << 3)) != 0)
    {
      flags = 1; /* is 'N' pressed? return NZ */
      goto exit;
    }

    /* Conv: Timing: The original game keyscans as fast as it can. We can't
     * have that so instead we introduce a short delay and handle game thread
     * termination. */
    gamedelay(state, 3500000 / 50); /* 50/sec */
  }

exit:
  return flags;
}

/* ----------------------------------------------------------------------- */

/**
 * $F163: Setup the game screen.
 *
 * \param[in] state Pointer to game state.
 */
TGE_API void tge_setup(tgestate_t *state)
{
  assert(state != NULL);

  wipe_full_screen_and_attributes(state);
  set_morale_flag_screen_attributes(state, attribute_BRIGHT_GREEN_OVER_BLACK);
  /* Conv: The original code passes in 68, not zero, as it uses a register
   * left over from a previous call to set_morale_flag_screen_attributes(). */
  set_menu_item_attributes(state, 0, attribute_BRIGHT_YELLOW_OVER_BLACK);
  plot_statics_and_menu_text(state);

  plot_score(state);
}

/**
 * $F17A: Run the main menu until the game is ready to start.
 *
 * \param[in] state Pointer to game state.
 *
 * \return +ve when the game should begin,
 *         -ve when the game thread should terminate,
 *         zero otherwise.
 */
TGE_API int tge_menu(tgestate_t *state)
{
  /* Note: The game will sit in this function while the menu screen runs. */
  return menu_screen(state);
}

/**
 * $F17D: Setup the game proper.
 *
 * \param[in] state Pointer to game state.
 */
TGE_API void tge_setup2(tgestate_t *state)
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
    (const anim_t *) anim_wait_tl,   //animations[8],        // anim
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
  // FUTURE: Fold this to:
  // for (vischar = &state->vischars[0]; vischar < &state->vischars[vischars_LENGTH]; vischar++)
  //   *vischar = vischar_initial;
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

    // reset_game, which in turn calls enter_room which exits via longjmp to after this if statement block

    // i reckon this never gets this far, as reset_game jumps to enter_room itself
    //enter_room(state); // returns by goto main_loop
    NEVER_RETURNS;
  }
  // we arrive here once the game is initialised and the initial bedroom scene is drawn and zoomboxed onto the screen
}

/**
 * Entry point for the main game loop (in addition to original game code).
 *
 * This sets up a jump buffer so that various point in the game code can long
 * jump to the exit.
 *
 * \param[in] state Pointer to game state.
 */
TGE_API void tge_main(tgestate_t *state)
{
  if (setjmp(state->jmpbuf_main) == 0)
    /* On entry we run the main loop. */
    main_loop(state);
  else
    /* If something wanted to exit quickly we arrive here. */
    ;
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

  memset(&state->speccy->screen.pixels,
         0,
         SCREEN_BITMAP_LENGTH);
  memset(&state->speccy->screen.attributes,
         attribute_WHITE_OVER_BLACK,
         SCREEN_ATTRIBUTES_LENGTH);

  /* Set the screen border to black. */
  state->speccy->out(state->speccy, port_BORDER_EAR_MIC, 0);

  /* Redraw the whole screen. */
  state->speccy->draw(state->speccy, NULL); // Conv: Added
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et

