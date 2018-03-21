/**
 * StaticGraphics.c
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
 * The recreated version is copyright (c) 2012-2018 David Thomas
 */

/**
 * $F076: Definitions of fixed graphic elements.
 * Only used by plot_statics_and_menu_text().
 */

#include <assert.h>
#include <stdint.h>

#include "TheGreatEscape/State.h"
#include "TheGreatEscape/StaticGraphics.h"
#include "TheGreatEscape/StaticTiles.h"
#include "TheGreatEscape/Main.h"

/* ----------------------------------------------------------------------- */

static const uint8_t statictileline_flagpole[] =
{
  0x18, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x1A, 0x1A
};
static const uint8_t statictileline_game_window_left_border[] =
{
  0x02, 0x04, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x0E, 0x10
};
static const uint8_t statictileline_game_window_right_border[] =
{
  0x05, 0x07, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x11, 0x12, 0x09, 0x0B
};
static const uint8_t statictileline_game_window_top_border[] =
{
  0x13, 0x14, 0x13, 0x14, 0x13, 0x14, 0x13, 0x14, 0x13, 0x14, 0x15, 0x16, 0x17, 0x13, 0x14, 0x13, 0x14, 0x13, 0x14, 0x13, 0x14, 0x13, 0x14
};
static const uint8_t statictileline_game_window_bottom_border[] =
{
  /* Identical to above... */
  0x13, 0x14, 0x13, 0x14, 0x13, 0x14, 0x13, 0x14, 0x13, 0x14, 0x15, 0x16, 0x17, 0x13, 0x14, 0x13, 0x14, 0x13, 0x14, 0x13, 0x14, 0x13, 0x14
};
static const uint8_t statictileline_flagpole_grass[] =
{
  0x1F, 0x1B, 0x1C, 0x1D, 0x1E
};
static const uint8_t statictileline_medals_row0[] =
{
  0x20, 0x21, 0x22, 0x21, 0x23, 0x21, 0x24, 0x21, 0x22, 0x21, 0x25, 0x0B, 0x0C
};
static const uint8_t statictileline_medals_row1[] =
{
  0x26, 0x4E, 0x27, 0x4E, 0x28, 0x4E, 0x29, 0x4E, 0x27, 0x4E, 0x2A
};
static const uint8_t statictileline_medals_row2[] =
{
  0x2B, 0x2C, 0x2D, 0x2C, 0x2E, 0x2C, 0x2F, 0x2C, 0x2D, 0x2C, 0x30
};
static const uint8_t statictileline_medals_row3[] =
{
  0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B
};
static const uint8_t statictileline_medals_row4[] =
{
  0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45
};
static const uint8_t statictileline_bell_row0[] =
{
  0x46, 0x47, 0x48
};
static const uint8_t statictileline_bell_row1[] =
{
  0x49, 0x4A, 0x4B
};
static const uint8_t statictileline_bell_row2[] =
{
  0x4C, 0x4D
};
static const uint8_t statictileline_corner_tl[] =
{
  0x01, 0x03
};
static const uint8_t statictileline_corner_tr[] =
{
  0x06, 0x08
};
static const uint8_t statictileline_corner_bl[] =
{
  0x0D, 0x0F
};
static const uint8_t statictileline_corner_br[] =
{
  0x0A, 0x0C
};

#define HORIZONTAL statictileline_HORIZONTAL
#define VERTICAL   statictileline_VERTICAL

#define ENTRY(name, screenloc, orientation) \
  { screenloc, NELEMS(name) | orientation, name }

/* As with screenlocstrings, I can't end a struct member with a
 * variably-sized array. I therefore have to indirect them. */
static const statictileline_t static_graphic_defs[18] =
{
  ENTRY(statictileline_flagpole,                  0x0021, VERTICAL),
  ENTRY(statictileline_game_window_left_border,   0x0006, VERTICAL),
  ENTRY(statictileline_game_window_right_border,  0x001E, VERTICAL),
  ENTRY(statictileline_game_window_top_border,    0x0027, HORIZONTAL),
  ENTRY(statictileline_game_window_bottom_border, 0x1047, HORIZONTAL),
  ENTRY(statictileline_flagpole_grass,            0x10A0, HORIZONTAL),
  ENTRY(statictileline_medals_row0,               0x1073, HORIZONTAL),
  ENTRY(statictileline_medals_row1,               0x1093, HORIZONTAL),
  ENTRY(statictileline_medals_row2,               0x10B3, HORIZONTAL),
  ENTRY(statictileline_medals_row3,               0x10D3, HORIZONTAL),
  ENTRY(statictileline_medals_row4,               0x10F3, HORIZONTAL),
  ENTRY(statictileline_bell_row0,                 0x106E, HORIZONTAL),
  ENTRY(statictileline_bell_row1,                 0x108E, HORIZONTAL),
  ENTRY(statictileline_bell_row2,                 0x10AE, HORIZONTAL),
  ENTRY(statictileline_corner_tl,                 0x0005, VERTICAL),
  ENTRY(statictileline_corner_tr,                 0x001F, VERTICAL),
  ENTRY(statictileline_corner_bl,                 0x1045, VERTICAL),
  ENTRY(statictileline_corner_br,                 0x105F, VERTICAL),
};

#undef ENTRY

#undef HORIZONTAL
#undef VERTICAL

/* ----------------------------------------------------------------------- */

static void plot_static_tiles_horizontal(tgestate_t             *state,
                                         uint8_t                *out,
                                         const statictileline_t *stline);

static void plot_static_tiles_vertical(tgestate_t             *state,
                                       uint8_t                *out,
                                       const statictileline_t *stline);

static void plot_static_tiles(tgestate_t             *state,
                              uint8_t                *out,
                              const statictileline_t *stline,
                              int                     orientation);

/* ----------------------------------------------------------------------- */

/**
 * $F1E0: Plot statics and menu text.
 *
 * \param[in] state Pointer to game state.
 */
void plot_statics_and_menu_text(tgestate_t *state)
{
  /**
   * $F446: Key choice screenlocstrings.
   */
  static const screenlocstring_t key_choice_screenlocstrings[] =
  {
    { 0x008E,  8, "CONTROLS"                },
    { 0x00CD,  8, "0 SELECT"                },
    { 0x080D, 10, "1 KEYBOARD"              },
    { 0x084D, 10, "2 KEMPSTON"              },
    { 0x088D, 10, "3 SINCLAIR"              },
    { 0x08CD,  8, "4 PROTEK"                },
    { 0x1007, 23, "BREAK OR CAPS AND SPACE" },
    { 0x102C, 12, "FOR NEW GAME"            },
  };

  const statictileline_t  *stline;    /* was HL */
  int                      iters;     /* was B */
  uint8_t                 *screenptr; /* was DE */
  const screenlocstring_t *slstring;  /* was HL */

  assert(state != NULL);

  /* Plot statics and menu text. */
  stline = &static_graphic_defs[0];
  iters  = NELEMS(static_graphic_defs);
  do
  {
    screenptr = &state->speccy->screen.pixels[stline->screenloc]; /* Fetch screen address offset. */
    ASSERT_SCREEN_PTR_VALID(screenptr);

    if (stline->flags_and_length & statictileline_VERTICAL)
      plot_static_tiles_vertical(state, screenptr, stline);
    else
      plot_static_tiles_horizontal(state, screenptr, stline);
    stline++;
  }
  while (--iters);

  /* Plot menu text. */
  iters    = NELEMS(key_choice_screenlocstrings);
  slstring = &key_choice_screenlocstrings[0];
  do
    slstring = screenlocstring_plot(state, slstring);
  while (--iters);
}

/**
 * $F206: Plot static tiles horizontally.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] out    Pointer to screen address. (was DE)
 * \param[in] stline Pointer to [count, tile indices, ...]. (was HL)
 */
static void plot_static_tiles_horizontal(tgestate_t             *state,
                                         uint8_t                *out,
                                         const statictileline_t *stline)
{
  plot_static_tiles(state, out, stline, 0);
}

/**
 * $F209: Plot static tiles vertically.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] out    Pointer to screen address. (was DE)
 * \param[in] stline Pointer to [count, tile indices, ...]. (was HL)
 */
static void plot_static_tiles_vertical(tgestate_t             *state,
                                       uint8_t                *out,
                                       const statictileline_t *stline)
{
  plot_static_tiles(state, out, stline, 255);
}

/**
 * $F20B: Plot static tiles.
 *
 * \param[in] state       Pointer to game state.
 * \param[in] out         Pointer to screen address. (was DE)
 * \param[in] stline      Pointer to [count, tile indices, ...]. (was HL)
 * \param[in] orientation Orientation: 0/255 = horizontal/vertical. (was A)
 */
static void plot_static_tiles(tgestate_t             *state,
                              uint8_t                *out,
                              const statictileline_t *stline,
                              int                     orientation)
{
  uint8_t       *saved_out = out;

  int            tiles_remaining; /* was A/B */
  const uint8_t *tiles;           /* was HL */

  assert(state  != NULL);
  ASSERT_SCREEN_PTR_VALID(out);
  assert(stline != NULL);
  assert(orientation == 0x00 || orientation == 0xFF);

  tiles = stline->tiles;

  tiles_remaining = stline->flags_and_length & ~statictileline_MASK;
  do
  {
    int                  tile_index;  /* was A */
    const static_tile_t *static_tile; /* was HL' */
    const tilerow_t     *tile_data;   /* was HL' */
    int                  iters;       /* was B' */
    ptrdiff_t            soffset;     /* was A */
    int                  aoffset;

    tile_index = *tiles++;

    static_tile = &static_tiles[tile_index]; // elements: 9 bytes each

    /* Plot a tile. */
    tile_data = &static_tile->data.row[0];
    iters = 8;
    do
    {
      *out = *tile_data++;
      out += 256; /* move to next screen row */
    }
    while (--iters);
    out -= 256;

    /* Calculate screen attribute address of tile. */
    // ((out - screen_base) / 0x800) * 0x100 + attr_base
    soffset = out - &state->speccy->screen.pixels[0];
    aoffset = soffset & 0xFF;
    if (soffset >= 0x0800) aoffset += 256;
    if (soffset >= 0x1000) aoffset += 256;
    state->speccy->screen.attributes[aoffset] = static_tile->attr; /* Copy attribute byte. */

    if (orientation == 0) /* Horizontal */
      out = out - 7 * 256 + 1;
    else /* Vertical */
      out = (uint8_t *) get_next_scanline(state, out); // must cast away constness

    ASSERT_SCREEN_PTR_VALID(out);
  }
  while (--tiles_remaining);

  /* Conv: Invalidation added over the original game. */
  tiles_remaining = stline->flags_and_length & ~statictileline_MASK;
  if (orientation == 0) /* Horizontal */
    invalidate_bitmap(state, saved_out, tiles_remaining * 8, 8);
  else
    invalidate_bitmap(state, saved_out, 8, tiles_remaining * 8);
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et

