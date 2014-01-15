#ifndef STATICTILES_H
#define STATICTILES_H

#include <stdint.h>

#include "ZXSpectrum/ZXSpectrum.h"

#include "TheGreatEscape/Tiles.h"

struct static_tile
{
  tile_t      data;
  attribute_t attr;
};

typedef struct static_tile static_tile_t;

extern const static_tile_t static_tiles[75];

#endif /* STATICTILES_H */

