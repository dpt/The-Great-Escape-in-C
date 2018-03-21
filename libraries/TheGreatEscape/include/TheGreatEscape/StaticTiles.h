/**
 * StaticTiles.h
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

#ifndef STATIC_TILES_H
#define STATIC_TILES_H

#include <stdint.h>

#include "ZXSpectrum/Spectrum.h"

#include "TheGreatEscape/Tiles.h"

struct static_tile
{
  tile_t      data;
  attribute_t attr;
};

typedef struct static_tile static_tile_t;

extern const static_tile_t static_tiles[];

#endif /* STATIC_TILES_H */

