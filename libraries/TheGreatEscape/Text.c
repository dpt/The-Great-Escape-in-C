#include <assert.h>

#include "TheGreatEscape/Font.h"
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
uint8_t *plot_glyph(const char *pcharacter, uint8_t *output)
{
  assert(pcharacter != NULL);
  assert(output     != NULL);

  return plot_single_glyph(*pcharacter, output);
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
uint8_t *plot_single_glyph(int character, uint8_t *output)
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

  /* Return the position of the next character. */
  return ++saved_output;
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et
