/**
 * InteriorTiles.h
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

// vim: ts=8 sts=2 sw=2 et
