/**
 * Text.c
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

#include <assert.h>

#include "TheGreatEscape/Font.h"
#include "TheGreatEscape/Main.h"
#include "TheGreatEscape/State.h"
#include "TheGreatEscape/Text.h"

/* ----------------------------------------------------------------------- */

/**
 * $7D2F: Plot a single glyph (indirectly).
 *
 * \param[in] pcharacter Pointer to character to plot. (was HL)
 * \param[in] output     Where to plot.                (was DE)
 *
 * \return Pointer to next character along.
 */
uint8_t *plot_glyph(tgestate_t *state,
                    const char *pcharacter,
                    uint8_t    *output)
{
  assert(pcharacter != NULL);
  assert(output     != NULL);

  return plot_single_glyph(state, *pcharacter, output);
}

/**
 * $7D30: Plot a single glyph.
 *
 * Conv: Characters are specified in ASCII.
 *
 * \param[in] character Character to plot. (was HL)
 * \param[in] output    Where to plot.     (was DE)
 *
 * \return Pointer to next character along. (was DE)
 */
uint8_t *plot_single_glyph(tgestate_t *state, int character, uint8_t *output)
{
  const tile_t    *glyph;        /* was HL */
  const tilerow_t *row;          /* was HL */
  int              iters;        /* was B */
  uint8_t         *saved_output; /* was stacked */

  assert(character < 256);
  assert(output != NULL);

  glyph        = &bitmap_font[ascii_to_font[character]];
  row          = &glyph->row[0];
  saved_output = output;

  iters = 8;
  do
  {
    *output = *row++;
    output += 256; /* Advance to next row. */
  }
  while (--iters);

  /* Conv: Invalidation added over the original game. */
  invalidate_bitmap(state, saved_output, 8, 8);

  /* Return the position of the next character. */
  return ++saved_output;
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et
