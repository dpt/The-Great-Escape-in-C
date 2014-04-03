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

/** Dimensions of the map buffer in state. */
enum
{
  MAPBUFX = 7,
  MAPBUFY = 5,
};

#endif /* MAP_H */
