#ifndef STATIC_GRAPHICS_H
#define STATIC_GRAPHICS_H

#include <stdint.h>

#include "TheGreatEscape/Types.h"

/**
 * Draw vertically.
 */
#define statictileline_VERTICAL (1 << 7)

/**
 * Defines a screen location, orientation and tiles pointer, for drawing
 * static tiles.
 */
typedef struct statictileline
{
  uint16_t       screenloc;        /* screen offset */
  uint8_t        flags_and_length; /* flags include statictileline_VERTICAL */
  const uint8_t *tiles;
}
statictileline_t;

extern const statictileline_t static_graphic_defs[18];

#endif /* STATIC_GRAPHICS_H */
