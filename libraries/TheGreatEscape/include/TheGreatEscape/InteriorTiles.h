#ifndef INTERIOR_TILES_H
#define INTERIOR_TILES_H

#include "TheGreatEscape/Tiles.h"

/**
 * Identifiers of tiles used to draw interior objects.
 */
enum interior_object_tile
{
  interiorobjecttile_MAX = 194,
  interiorobjecttile_ESCAPE = 255
};

/**
 * A tile used to draw interior objects.
 */
typedef enum interior_object_tile objecttile_t;

extern const tile_t interior_tiles[interiorobjecttile_MAX];

#endif /* INTERIOR_TILES_H */

