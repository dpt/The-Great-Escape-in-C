/**
 * SuperTiles.h
 *
 * This file is part of "The Great Escape in C".
 *
 * This project recreates the 48K ZX Spectrum version of the prison escape
 * game "The Great Escape" in portable C code. It is free software provided
 * without warranty in the interests of education and software preservation.
 *
 * "The Great Escape" was created by Denton Designs and published in 1986 by
 * Ocean Software Limited.
 *
 * The original game is copyright (c) 1986 Ocean Software Ltd.
 * The original game design is copyright (c) 1986 Denton Designs Ltd.
 * The recreated version is copyright (c) 2012-2018 David Thomas
 */

#ifndef SUPER_TILES_H
#define SUPER_TILES_H

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

#endif /* SUPER_TILES_H */

