#ifndef SUPERTILES_H
#define SUPERTILES_H

#include "TheGreatEscape/Tiles.h"

/**
 * A supertile is a 4x4 group of tile indices.
 */
typedef struct supertile supertile_t;

struct supertile
{
  tileindex_t tiles[4 * 4];
};

typedef uint8_t supertileindex_t;

enum
{
  map_supertiles__LIMIT = 0xDA
};

extern const supertile_t map_supertiles[map_supertiles__LIMIT];

#endif /* SUPERTILES_H */

