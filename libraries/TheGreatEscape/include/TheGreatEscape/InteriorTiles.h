#ifndef INTERIOR_TILES_H
#define INTERIOR_TILES_H

#include "TheGreatEscape/Tiles.h"

/**
 * Identifiers of tiles used to draw interior objects.
 */
enum interiortileindex
{
  interiortile__LIMIT = 194,
  interiortile_ESCAPE = 255
};

/**
 * A tile used to draw interior objects.
 */
typedef uint8_t interiortileindex_t;

extern const tile_t interior_tiles[interiortile__LIMIT];

#endif /* INTERIOR_TILES_H */

