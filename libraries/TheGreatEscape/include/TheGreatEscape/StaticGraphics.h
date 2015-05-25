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

extern const statictileline_t static_graphic_defs[18];

/* ----------------------------------------------------------------------- */

void plot_statics_and_menu_text(tgestate_t *state);

/* ----------------------------------------------------------------------- */

#endif /* STATIC_GRAPHICS_H */
