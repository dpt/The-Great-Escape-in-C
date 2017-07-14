#ifndef MAP_H
#define MAP_H

#include <stdint.h>

#include "TheGreatEscape/SuperTiles.h"

/** Dimensions of the map. */
enum
{
  MAPX = 54,
  MAPY = 34,
};

extern const supertileindex_t map[MAPX * MAPY];

/* Used in in_permitted_area to detect when hero is off the side of the map
 * and has escaped. */
/* These equate to (-136, 1088) in map coordinates. */
#define MAP_WIDTH  217
#define MAP_HEIGHT 137

#endif /* MAP_H */
