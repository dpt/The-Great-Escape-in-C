/**
 * Sprites.h
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

#ifndef SPRITES_H
#define SPRITES_H

/* ----------------------------------------------------------------------- */

#include "C99/Types.h"

/* ----------------------------------------------------------------------- */

/**
 * A spritedef defines a bitmap + mask pair;
 */
typedef struct spritedef
{
  uint8_t        width;  /**< width in bytes + 1 */
  uint8_t        height; /**< height in rows */
  const uint8_t *bitmap;
  const uint8_t *mask;
}
spritedef_t;

/**
 * Identifiers of the sprites in the sprites[] array.
 */
enum
{
  sprite_STOVE,
  sprite_CRATE,
  sprite_PRISONER_FACING_AWAY_1,
  sprite_PRISONER_FACING_AWAY_2,
  sprite_PRISONER_FACING_AWAY_3,
  sprite_PRISONER_FACING_AWAY_4,
  sprite_PRISONER_FACING_TOWARDS_1,
  sprite_PRISONER_FACING_TOWARDS_2,
  sprite_PRISONER_FACING_TOWARDS_3,
  sprite_PRISONER_FACING_TOWARDS_4,
  sprite_CRAWL_FACING_TOWARDS_1,
  sprite_CRAWL_FACING_TOWARDS_2,
  sprite_CRAWL_FACING_AWAY_1,
  sprite_CRAWL_FACING_AWAY_2,
  sprite_DOG_FACING_AWAY_1,
  sprite_DOG_FACING_AWAY_2,
  sprite_DOG_FACING_AWAY_3,
  sprite_DOG_FACING_AWAY_4,
  sprite_DOG_FACING_TOWARDS_1,
  sprite_DOG_FACING_TOWARDS_2,
  sprite_DOG_FACING_TOWARDS_3,
  sprite_DOG_FACING_TOWARDS_4,
  sprite_GUARD_FACING_AWAY_1,
  sprite_GUARD_FACING_AWAY_2,
  sprite_GUARD_FACING_AWAY_3,
  sprite_GUARD_FACING_AWAY_4,
  sprite_GUARD_FACING_TOWARDS_1,
  sprite_GUARD_FACING_TOWARDS_2,
  sprite_GUARD_FACING_TOWARDS_3,
  sprite_GUARD_FACING_TOWARDS_4,
  sprite_COMMANDANT_FACING_AWAY_1,
  sprite_COMMANDANT_FACING_AWAY_2,
  sprite_COMMANDANT_FACING_AWAY_3,
  sprite_COMMANDANT_FACING_AWAY_4,
  sprite_COMMANDANT_FACING_TOWARDS_1,
  sprite_COMMANDANT_FACING_TOWARDS_2,
  sprite_COMMANDANT_FACING_TOWARDS_3,
  sprite_COMMANDANT_FACING_TOWARDS_4,
  sprite__LIMIT, /* == 38 */

  sprite_FLAG_FLIP = 1 << 7 /**< Left/right flip flag. */
};

extern const spritedef_t sprites[sprite__LIMIT];

/* ----------------------------------------------------------------------- */

#endif /* SPRITES_H */

// vim: ts=8 sts=2 sw=2 et
