#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>

/**
 * Identifiers for character sprites.
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

/**
 * Holds a sprite index.
 */
typedef uint8_t spriteindex_t;

/**
 * A spritedef defines a bitmap + mask pair;
 */
typedef struct spritedef
{
  uint8_t        width; /* in bytes + 1 */
  uint8_t        height;
  const uint8_t *bitmap;
  const uint8_t *mask;
}
spritedef_t;

extern const spritedef_t sprites[sprite__LIMIT];

#endif /* SPRITES_H */

