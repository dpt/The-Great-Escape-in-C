#ifndef MAP_H
#define MAP_H

#include <stdint.h>

enum
{
  MAPX = 54,
  MAPY = 34
};

typedef uint8_t maptile_t;

extern const maptile_t map_tiles[MAPX *MAPY];

#endif /* MAP_H */

