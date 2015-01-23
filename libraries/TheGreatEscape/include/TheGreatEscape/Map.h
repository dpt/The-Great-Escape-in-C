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

#endif /* MAP_H */
