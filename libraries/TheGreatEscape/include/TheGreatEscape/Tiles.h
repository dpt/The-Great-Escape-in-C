/**
 * Tiles.h
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
 * The recreated version is copyright (c) 2012-2019 David Thomas
 */

#ifndef TILES_H
#define TILES_H

/* ----------------------------------------------------------------------- */

#include "C99/Types.h"

/* ----------------------------------------------------------------------- */

/**
 * A tile index.
 */
typedef uint8_t tileindex_t;

/**
 * An 8-pixel wide row within a tile.
 */
typedef uint8_t tilerow_t;

/**
 * An 8-by-8 tile.
 */
typedef struct
{
  tilerow_t row[8];
}
tile_t;

/* ----------------------------------------------------------------------- */

#endif /* TILES_H */

// vim: ts=8 sts=2 sw=2 et
