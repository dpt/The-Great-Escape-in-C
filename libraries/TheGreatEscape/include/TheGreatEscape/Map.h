/**
 * Map.h
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

#ifndef MAP_H
#define MAP_H

#include "TheGreatEscape/SuperTiles.h"

/** Dimensions of the map. */
enum
{
  MAPX = 54,
  MAPY = 34,
};

extern const supertileindex_t map[MAPX * MAPY];

/* Used in in_permitted_area to detect when hero is off the side of the map
 * and has escaped. */
/* These equate to (-136, 1088) in map coordinates. */
#define MAP_WIDTH  217
#define MAP_HEIGHT 137

#endif /* MAP_H */
