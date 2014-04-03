#ifndef SUPERTILES_H
#define SUPERTILES_H

#include <stdint.h>

#include "TheGreatEscape/Tiles.h"

/**
 * A supertile is a 4x4 array of tile indices.
 */
typedef struct supertile supertile_t;

struct supertile
{
  tileindex_t tiles[4 * 4];
};

/**
 * A supertileindex is an index into supertiles[].
 */
typedef uint8_t supertileindex_t;

enum
{
  supertileindex__LIMIT = 0xDA
};

/**
 * 'supertiles' defines all of the supertiles.
 */
extern const supertile_t supertiles[supertileindex__LIMIT];

#endif /* SUPERTILES_H */

