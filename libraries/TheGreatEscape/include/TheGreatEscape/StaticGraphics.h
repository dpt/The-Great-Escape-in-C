/**
 * StaticGraphics.h
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

#ifndef STATIC_GRAPHICS_H
#define STATIC_GRAPHICS_H

#include <stdint.h>

#include "TheGreatEscape/State.h"
#include "TheGreatEscape/Types.h"

/* ----------------------------------------------------------------------- */

/**
 * Draw vertically.
 */
#define statictileline_HORIZONTAL (0 << 7)
#define statictileline_VERTICAL   (1 << 7)
#define statictileline_MASK       (1 << 7)

/**
 * Defines a screen location, orientation and tiles pointer, for drawing
 * static tiles.
 */
typedef struct statictileline
{
  uint16_t       screenloc;        /**< screen offset */
  uint8_t        flags_and_length; /**< flags are statictileline_VERTICAL */
  const uint8_t *tiles;
}
statictileline_t;

/* ----------------------------------------------------------------------- */

void plot_statics_and_menu_text(tgestate_t *state);

/* ----------------------------------------------------------------------- */

#endif /* STATIC_GRAPHICS_H */

// vim: ts=8 sts=2 sw=2 et
