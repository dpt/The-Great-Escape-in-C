/**
 * Save.c
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

#ifdef TGE_SAVES

/* ----------------------------------------------------------------------- */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "TheGreatEscape/State.h"
#include "TheGreatEscape/Main.h" /* for animations[] */
#include "TheGreatEscape/Messages.h" /* for messages_table[] */

#include "zerotape.h"

/* ----------------------------------------------------------------------- */

/* Identifiers for custom loaders and savers */

#define CUSTOM_ID_VISCHAR_ANIM    (0)
#define CUSTOM_ID_MESSAGES_CURCHR (1)
#define CUSTOM_ID__LIMIT          (2)

/* ----------------------------------------------------------------------- */

/* Identifiers of dynamic locations */

static const char messages_queue_id[] = "m";
static const char window_buf_id[] = "w";
static const char screen_id[] = "s";
static const char lockeddoors_id[] = "l";

#define NREGIONS (4)

/* ----------------------------------------------------------------------- */

static const ztfield_t meta_mappos8_fields[] =
{
  ZTUCHAR(u, mappos8_t),
  ZTUCHAR(v, mappos8_t),
  ZTUCHAR(w, mappos8_t)
};
static const ztstruct_t meta_mappos8 =
{
  NELEMS(meta_mappos8_fields), &meta_mappos8_fields[0]
};

static const ztfield_t meta_mappos16_fields[] =
{
  ZTUSHORT(u, mappos16_t),
  ZTUSHORT(v, mappos16_t),
  ZTUSHORT(w, mappos16_t)
};
static const ztstruct_t meta_mappos16 =
{
  NELEMS(meta_mappos16_fields), &meta_mappos16_fields[0]
};

static const ztfield_t meta_pos8_fields[] =
{
  ZTUCHAR(x, pos8_t),
  ZTUCHAR(y, pos8_t)
};
static const ztstruct_t meta_pos8 =
{
  NELEMS(meta_pos8_fields), &meta_pos8_fields[0]
};

static const ztfield_t meta_pos16_fields[] =
{
  ZTUSHORT(x, pos16_t),
  ZTUSHORT(y, pos16_t)
};
static const ztstruct_t meta_pos16 =
{
  NELEMS(meta_pos16_fields), &meta_pos16_fields[0]
};

/* ----------------------------------------------------------------------- */

static const ztfield_t meta_route_fields[] =
{
  ZTUCHAR(index, route_t),
  ZTUCHAR(step, route_t),
};
static const ztstruct_t meta_route =
{
  NELEMS(meta_route_fields), &meta_route_fields[0]
};

/* ----------------------------------------------------------------------- */

static const ztarray_t meta_spritedefs_array =
{
  &sprites[0], sizeof(sprites), NELEMS(sprites)
};

/* ----------------------------------------------------------------------- */

static const ztfield_t meta_movableitem_fields[] =
{
  ZTSTRUCT(mappos, movableitem_t, mappos16_t, &meta_mappos16),
  ZTARRAYIDX_STATIC(sprite, movableitem_t, const spritedef_t *, &meta_spritedefs_array),
  ZTUCHAR(sprite_index, movableitem_t)
};
static const ztstruct_t meta_movableitem =
{
  NELEMS(meta_movableitem_fields), &meta_movableitem_fields[0]
};

static const ztfield_t meta_characterstruct_fields[] =
{
  ZTUCHAR(character_and_flags, characterstruct_t),
  ZTUCHAR(room, characterstruct_t),
  ZTSTRUCT(mappos, characterstruct_t, mappos8_t, &meta_mappos8),
  ZTSTRUCT(route, characterstruct_t, route_t, &meta_route),
};
static const ztstruct_t meta_characterstruct =
{
  NELEMS(meta_characterstruct_fields), &meta_characterstruct_fields[0]
};

static const ztfield_t meta_itemstruct_fields[] =
{
  ZTUCHAR(item_and_flags, itemstruct_t),
  ZTUCHAR(room_and_flags, itemstruct_t),
  ZTSTRUCT(mappos, itemstruct_t, mappos8_t, &meta_mappos8),
  ZTSTRUCT(isopos, itemstruct_t, pos8_t, &meta_pos8)
};
static const ztstruct_t meta_itemstruct =
{
  NELEMS(meta_itemstruct_fields), &meta_itemstruct_fields[0]
};

static ztresult_t messages_curptr_saver(const void *pvoidval,
                                        char       *buf,
                                        size_t      bufsz)
{
  const char *p;
  int         i;
  const char *start = NULL;

  /* The message pointer points into messages_table[].
   * Turn it into "[row, column]". */

  p = *((const char **) pvoidval);
  if (p == NULL)
  {
    sprintf(buf, "[]");
    return ztresult_OK;
  }

  for (i = 0; i < message__LIMIT; i++)
  {
    const char *end;

    start = messages_table[i];
    end = start + strlen(start);
    if (p >= start && p < end)
      break;
  }
  if (i == message__LIMIT)
    return ztresult_BAD_POINTER;

  sprintf(buf, "[%d, %d]", i, (int)(p - start));

  return ztresult_OK;
}

static ztresult_t messages_curptr_loader(const ztast_expr_t *arrayexpr,
                                         void               *pvoidval,
                                         char              **syntax_error)
{
  const char             **p;
  int                      index;
  const ztast_arrayelem_t *elem;
  int                      elems[2] = { 0, 0 };

  p = (const char **) pvoidval;

  if (arrayexpr->type != ZTEXPR_ARRAY)
  {
    *syntax_error = "array type required (custom)"; /* ztsyntx_NEED_ARRAY */
    return ztresult_SYNTAX_ERROR;
  }
  if (arrayexpr->data.array->elems == NULL)
  {
    *p = NULL;
    return ztresult_OK;
  }

  index = -1;
  for (elem = arrayexpr->data.array->elems; elem; elem = elem->next)
  {
    ztast_expr_t  *expr;
    ztast_value_t *value;
    unsigned int   integer;

    /* Elements without a specified position will have an index of -1 */
    index = (elem->index >= 0) ? elem->index : index + 1;

    expr = elem->expr;
    if (expr->type != ZTEXPR_VALUE)
    {
      *syntax_error = "value type required (custom)"; /* ztsyntx_NEED_VALUE */
      return ztresult_SYNTAX_ERROR;
    }
    value = expr->data.value;
    if (value->type != ZTVAL_INTEGER)
    {
      *syntax_error = "non-integer in array (custom)"; /* ztsyntx_UNEXPECTED_TYPE */
      return ztresult_SYNTAX_ERROR;
    }
    integer = value->data.integer;
    if ((index == 0 && (integer < 0 || integer >= message__LIMIT)) ||
        (index == 1 && (integer < 0 || integer >= strlen(messages_table[elems[0]]))) ||
        (index >= 2))
    {
      *syntax_error = "value out of range (custom)"; /* ztsyntx_VALUE_RANGE */
      return ztresult_SYNTAX_ERROR;
    }

    elems[index] = integer;
  }

  *p = messages_table[elems[0]] + elems[1];

  return ztresult_OK;
}

static const ztfield_t meta_messages_fields[] =
{
  ZTUCHARARRAY(queue, messages_t, 12),
  ZTUCHAR(display_delay, messages_t),
  ZTUCHAR(display_index, messages_t),
  ZTARRAYIDX(queue_pointer, messages_t, uint8_t *, messages_queue_id), /* relative to 'queue' */
  ZTCUSTOM(current_character, messages_t, CUSTOM_ID_MESSAGES_CURCHR)
};
static const ztstruct_t meta_messages =
{
  NELEMS(meta_messages_fields), &meta_messages_fields[0]
};

static ztresult_t vischar_anim_saver(const void *pvoidval,
                                     char       *buf,
                                     size_t      bufsz)
{
  const anim_t *anim;
  int           i;

  anim = *((const anim_t **) pvoidval);
  if (anim == NULL)
  {
    sprintf(buf, "nil");
    return ztresult_OK;
  }

  for (i = 0; i < animations__LIMIT; i++)
    if (animations[i] == anim)
      break;
  if (i == animations__LIMIT)
    return ztresult_BAD_POINTER;

  sprintf(buf, "%d", i);

  return ztresult_OK;
}

static ztresult_t vischar_anim_loader(const ztast_expr_t *expr,
                                      void               *pvoidval,
                                      char              **syntax_error)
{
  const anim_t **anim;
  int            index;

  if (expr->type != ZTEXPR_VALUE)
  {
    *syntax_error = "value type required (custom)"; /* ztsyntx_NEED_VALUE */
    return ztresult_SYNTAX_ERROR;
  }

  anim = (const anim_t **) pvoidval;

  switch (expr->data.value->type)
  {
  case ZTVAL_INTEGER:
    index = expr->data.value->data.integer;
    if (index < 0 || index >= animations__LIMIT)
    {
      *syntax_error = "value out of range (custom)"; /* ztsyntx_VALUE_RANGE */
      return ztresult_SYNTAX_ERROR;
    }

    *anim = animations[index];
    break;

  case ZTVAL_NIL:
    *anim = NULL;
    break;

  default:
    *syntax_error = "integer or nil type required (custom)"; /* ztsyntx_NEED_INTEGER */
    return ztresult_SYNTAX_ERROR;
  }
  *syntax_error = NULL;

  return ztresult_OK;
}

static const ztfield_t meta_vischar_fields[] =
{
  ZTUCHAR(character, vischar_t),
  ZTUCHAR(flags, vischar_t),
  ZTUSHORT(route, vischar_t),
  ZTSTRUCT(target, vischar_t, mappos8_t, &meta_mappos8),
  ZTUCHAR(counter_and_flags, vischar_t),
  /* don't store "animbase" */
  ZTCUSTOM(anim, vischar_t, CUSTOM_ID_VISCHAR_ANIM),
  ZTUCHAR(animindex, vischar_t),
  ZTUCHAR(input, vischar_t),
  ZTUCHAR(direction, vischar_t),
  ZTSTRUCT(mi, vischar_t, movableitem_t, &meta_movableitem),
  ZTSTRUCT(isopos, vischar_t, pos16_t, &meta_pos16),
  ZTUCHAR(room, vischar_t),
  ZTUCHAR(unused, vischar_t),
  ZTUCHAR(width_bytes, vischar_t),
  ZTUCHAR(height, vischar_t)
};
static const ztstruct_t meta_vischar =
{
  NELEMS(meta_vischar_fields), &meta_vischar_fields[0]
};

static const ztfield_t meta_bounds_fields[] =
{
  ZTUCHAR(x0, bounds_t),
  ZTUCHAR(x1, bounds_t),
  ZTUCHAR(y0, bounds_t),
  ZTUCHAR(y1, bounds_t)
};
static const ztstruct_t meta_bounds =
{
  NELEMS(meta_bounds_fields), &meta_bounds_fields[0]
};

static const ztfield_t meta_mask_fields[] =
{
  ZTUCHAR(index, mask_t),
  ZTSTRUCT(bounds, mask_t, bounds_t, &meta_bounds),
  ZTSTRUCT(mappos, mask_t, mappos8_t, &meta_mappos8)
};
static const ztstruct_t meta_mask =
{
  NELEMS(meta_mask_fields), &meta_mask_fields[0]
};

static const ztfield_t meta_zoombox_fields[] =
{
  ZTUCHAR(x0, bounds_t),
  ZTUCHAR(x1, bounds_t),
  ZTUCHAR(y0, bounds_t),
  ZTUCHAR(y1, bounds_t)
};
static const ztstruct_t meta_zoombox =
{
  NELEMS(meta_zoombox_fields), &meta_zoombox_fields[0]
};

static const ztfield_t meta_searchlight_movement_fields[] =
{
  ZTSTRUCT(xy, searchlight_movement_t, pos8_t, &meta_pos8),
  ZTUCHAR(counter, searchlight_movement_t),
  ZTUCHAR(direction, searchlight_movement_t),
  ZTUCHAR(index, searchlight_movement_t),
  /*  don't store 'ptr' (it's constant) */
};
static const ztstruct_t meta_searchlight_movement =
{
  NELEMS(meta_searchlight_movement_fields), &meta_searchlight_movement_fields[0]
};

static const ztfield_t meta_searchlight_fields[] =
{
  ZTSTRUCTARRAY(states, searchlight_t, searchlight_movement_t, 3, &meta_searchlight_movement),
  ZTSTRUCT(caught_coord, searchlight_t, pos8_t, &meta_pos8)
};
static const ztstruct_t meta_searchlight =
{
  NELEMS(meta_searchlight_fields), &meta_searchlight_fields[0]
};

static const ztfield_t meta_spriteplotter_fields[] =
{
  ZTUCHAR(height_24_right,   spriteplotter_t),
  ZTUCHAR(height_24_left,    spriteplotter_t),
  ZTUCHAR(height_16_left,    spriteplotter_t),
  ZTUCHAR(height_16_right,   spriteplotter_t),
  ZTUCHAR(enable_24_right_1, spriteplotter_t),
  ZTUCHAR(enable_24_right_2, spriteplotter_t),
  ZTUCHAR(enable_24_right_3, spriteplotter_t),
  ZTUCHAR(enable_24_right_4, spriteplotter_t),
  ZTUCHAR(enable_24_left_1,  spriteplotter_t),
  ZTUCHAR(enable_24_left_2,  spriteplotter_t),
  ZTUCHAR(enable_24_left_3,  spriteplotter_t),
  ZTUCHAR(enable_24_left_4,  spriteplotter_t),
  ZTUCHAR(enable_16_left_1,  spriteplotter_t),
  ZTUCHAR(enable_16_left_2,  spriteplotter_t),
  ZTUCHAR(enable_16_left_3,  spriteplotter_t),
  ZTUCHAR(enable_16_right_1, spriteplotter_t),
  ZTUCHAR(enable_16_right_2, spriteplotter_t),
  ZTUCHAR(enable_16_right_3, spriteplotter_t)
};
static const ztstruct_t meta_spriteplotter =
{
  NELEMS(meta_spriteplotter_fields), &meta_spriteplotter_fields[0]
};

static const ztfield_t meta_keydefs_fields[] =
{
  ZTUSHORTARRAY(defs, keydefs_t, 5)
};
static const ztstruct_t meta_keydefs =
{
  NELEMS(meta_keydefs_fields), &meta_keydefs_fields[0]
};

/* ----------------------------------------------------------------------- */

static const ztfield_t meta_tgestate_fields[] =
{
  ZTUCHAR(room_index, tgestate_t),
  ZTUCHAR(current_door, tgestate_t),
  ZTSTRUCTARRAY(movable_items, tgestate_t, movableitem_t, movable_item__LIMIT, &meta_movableitem),
  ZTSTRUCTARRAY(character_structs, tgestate_t, characterstruct_t, character_structs__LIMIT, &meta_characterstruct),
  ZTSTRUCTARRAY(item_structs, tgestate_t, itemstruct_t, item__LIMIT, &meta_itemstruct),
  ZTSTRUCT(messages, tgestate_t, messages_t, &meta_messages),
  /* don't store "reversed" array (generated at startup) */
  ZTSTRUCTARRAY(vischars, tgestate_t, vischar_t, vischars_LENGTH, &meta_vischar),
  ZTUCHARARRAY2D(mask_buffer, tgestate_t, MASK_BUFFER_LENGTH, MASK_BUFFER_ROWBYTES),
  ZTARRAYIDX(window_buf_pointer, tgestate_t, uint8_t *, window_buf_id),
  ZTSTRUCT(saved_mappos, tgestate_t, mappos16_t, &meta_pos16), /* a union: we'll always save in pos16 form */
  /* don't store bitmap_pointer (setup in item/vischar_setup_plotting) */
  /* don't store mask_pointer */
  /* don't store foreground_mask_pointer */
  /* don't store mappos_stash */
  ZTSTRUCT(isopos, tgestate_t, pos8_t, &meta_pos8),
  ZTUCHAR(sprite_index, tgestate_t),
  ZTSTRUCT(hero_mappos, tgestate_t, mappos8_t, &meta_mappos8),
  ZTSTRUCT(map_position, tgestate_t, pos8_t, &meta_pos8),
  ZTUCHAR(searchlight_state, tgestate_t),
  ZTUCHAR(roomdef_dimensions_index, tgestate_t),
  ZTUCHAR(roomdef_object_bounds_count, tgestate_t),
  ZTSTRUCTARRAY(roomdef_object_bounds, tgestate_t, bounds_t, MAX_ROOMDEF_OBJECT_BOUNDS, &meta_bounds),
  ZTUCHARARRAY(interior_doors, tgestate_t, 4),
  ZTUCHAR(interior_mask_data_count, tgestate_t),
  ZTSTRUCTARRAY(interior_mask_data, tgestate_t, mask_t, MAX_INTERIOR_MASK_REFS, &meta_mask),
  ZTUCHAR(item_height, tgestate_t),
  ZTUCHARARRAY(items_held, tgestate_t, 2),
  ZTUCHAR(character_index, tgestate_t),
  ZTUCHAR(game_counter, tgestate_t),
  ZTUCHAR(bell, tgestate_t),
  ZTUCHARARRAY(score_digits, tgestate_t, 5),
  ZTUCHAR(hero_in_breakfast, tgestate_t),
  ZTUCHAR(red_flag, tgestate_t),
  ZTUCHAR(automatic_player_counter, tgestate_t),
  ZTUCHAR(in_solitary, tgestate_t),
  ZTUCHAR(morale_exhausted, tgestate_t),
  ZTUCHAR(morale, tgestate_t),
  ZTUCHAR(clock, tgestate_t),
  ZTUCHAR(entered_move_a_character, tgestate_t),
  ZTUCHAR(hero_in_bed, tgestate_t),
  ZTUCHAR(displayed_morale, tgestate_t),
  ZTARRAYIDX(moraleflag_screen_address, tgestate_t, uint8_t *, screen_id),
  ZTARRAYIDX(ptr_to_door_being_lockpicked, tgestate_t, doorindex_t *, lockeddoors_id),
  ZTUCHAR(player_locked_out_until, tgestate_t),
  ZTUCHAR(day_or_night, tgestate_t),
  ZTUCHAR(red_cross_parcel_current_contents, tgestate_t),
  ZTUCHAR(move_map_y, tgestate_t),
  ZTSTRUCT(game_window_offset, tgestate_t, pos8_t, &meta_pos8),
  ZTSTRUCT(zoombox, tgestate_t, zoombox_t, &meta_zoombox),
  ZTUCHAR(game_window_attribute, tgestate_t),
  ZTSTRUCT(searchlight, tgestate_t, searchlight_t, &meta_searchlight),
  ZTUCHAR(bribed_character, tgestate_t),
  ZTUCHAR(prng_index, tgestate_t),
  ZTUCHAR(food_discovered_counter, tgestate_t),
  ZTUCHARARRAY(item_attributes, tgestate_t, item__LIMIT),
  ZTSTRUCT(spriteplotter, tgestate_t, spriteplotter_t, &meta_spriteplotter),
  ZTUCHARARRAY(locked_doors, tgestate_t, LOCKED_DOORS_LENGTH),
  ZTSTRUCT(keydefs, tgestate_t, keydefs_t, &meta_keydefs),
  ZTUCHARARRAY2DPTR(tile_buf, tgestate_t, TILE_BUF_LENGTH, 24),
  ZTUCHAR(chosen_input_device, tgestate_t),
  ZTUSHORT(music_channel0_index, tgestate_t),
  ZTUSHORT(music_channel1_index, tgestate_t),
  ZTUCHARARRAY2DPTR(window_buf, tgestate_t, WINDOW_BUF_LENGTH, 24),
  ZTUCHARARRAY2DPTR(map_buf, tgestate_t, MAP_BUF_LENGTH, 7)
};
static const ztstruct_t meta_tgestate =
{
  NELEMS(meta_tgestate_fields), &meta_tgestate_fields[0]
};

/* ----------------------------------------------------------------------- */

TGE_API int tge_save(tgestate_t *state, const char *filename)
{
  ztresult_t zt;
  ztregion_t regions[NREGIONS];
  ztsaver_t *savers[CUSTOM_ID__LIMIT];

  regions[0].id          = messages_queue_id;
  regions[0].spec.base   = &state->messages.queue[0];
  regions[0].spec.length = message_queue_LENGTH;
  regions[0].spec.nelems = message_queue_LENGTH;

  regions[1].id          = window_buf_id;
  regions[1].spec.base   = state->window_buf;
  regions[1].spec.length = state->window_buf_size;
  regions[1].spec.nelems = state->window_buf_size;

  regions[2].id          = screen_id;
  regions[2].spec.base   = state->speccy->screen.pixels;
  regions[2].spec.length = SCREEN_BITMAP_LENGTH;
  regions[2].spec.nelems = SCREEN_BITMAP_LENGTH;

  regions[3].id          = lockeddoors_id;
  regions[3].spec.base   = &state->locked_doors[0];
  regions[3].spec.length = LOCKED_DOORS_LENGTH;
  regions[3].spec.nelems = LOCKED_DOORS_LENGTH;

  savers[CUSTOM_ID_VISCHAR_ANIM]    = vischar_anim_saver;
  savers[CUSTOM_ID_MESSAGES_CURCHR] = messages_curptr_saver;

  zt = zt_save(&meta_tgestate,
               state,
               filename,
               regions,
               NREGIONS,
               savers,
               NELEMS(savers));
  if (zt != ztresult_OK)
    return 1;

  return 0;
}

TGE_API int tge_load(tgestate_t *state, const char *filename)
{
  ztresult_t  zt;
  ztregion_t  regions[NREGIONS];
  ztloader_t *loaders[CUSTOM_ID__LIMIT];

  regions[0].id          = messages_queue_id;
  regions[0].spec.base   = &state->messages.queue;
  regions[0].spec.length = message_queue_LENGTH;
  regions[0].spec.nelems = message_queue_LENGTH;

  regions[1].id          = window_buf_id;
  regions[1].spec.base   = state->window_buf;
  regions[1].spec.length = state->window_buf_size;
  regions[1].spec.nelems = state->window_buf_size;

  regions[2].id          = screen_id;
  regions[2].spec.base   = state->speccy->screen.pixels;
  regions[2].spec.length = SCREEN_BITMAP_LENGTH;
  regions[2].spec.nelems = SCREEN_BITMAP_LENGTH;

  regions[3].id          = lockeddoors_id;
  regions[3].spec.base   = &state->locked_doors[0];
  regions[3].spec.length = LOCKED_DOORS_LENGTH;
  regions[3].spec.nelems = LOCKED_DOORS_LENGTH;

  loaders[CUSTOM_ID_VISCHAR_ANIM]    = vischar_anim_loader;
  loaders[CUSTOM_ID_MESSAGES_CURCHR] = messages_curptr_loader;

  zt = zt_load(&meta_tgestate,
               state,
               filename,
               regions,
               NREGIONS,
               loaders,
               NELEMS(loaders));
  if (zt != ztresult_OK)
    return 1;

  return 0;
}

/* ----------------------------------------------------------------------- */

#endif /* TGE_SAVES */

// vim: ts=8 sts=2 sw=2 et
