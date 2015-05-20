#ifndef SPRITES_H
#define SPRITES_H

#include "TheGreatEscape/SpriteBitmaps.h"

/**
 * A sprite (a bitmap + mask pair);
 */
typedef struct sprite
{
  uint8_t        width; /* in bytes + 1 */
  uint8_t        height;
  const uint8_t *bitmap;
  const uint8_t *mask;
}
sprite_t;

enum
{
  sprite_STOVE,
  sprite_CRATE,
  sprite_PRISONER_FACING_AWAY_4,
  sprite_PRISONER_FACING_AWAY_3,
  sprite_PRISONER_FACING_AWAY_2,
  sprite_PRISONER_FACING_AWAY_1,
  sprite_PRISONER_FACING_TOWARDS_1,
  sprite_PRISONER_FACING_TOWARDS_2,
  sprite_PRISONER_FACING_TOWARDS_3,
  sprite_PRISONER_FACING_TOWARDS_4,
  sprite_CRAWL_FACING_TOWARDS_2,
  sprite_CRAWL_FACING_TOWARDS_1,
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
  sprite_GUARD_FACING_AWAY_4,
  sprite_GUARD_FACING_AWAY_3,
  sprite_GUARD_FACING_AWAY_2,
  sprite_GUARD_FACING_AWAY_1,
  sprite_GUARD_FACING_TOWARDS_1,
  sprite_GUARD_FACING_TOWARDS_2,
  sprite_GUARD_FACING_TOWARDS_3,
  sprite_GUARD_FACING_TOWARDS_4,
  sprite_COMMANDANT_FACING_AWAY_4,
  sprite_COMMANDANT_FACING_AWAY_3,
  sprite_COMMANDANT_FACING_AWAY_2,
  sprite_COMMANDANT_FACING_AWAY_1,
  sprite_COMMANDANT_FACING_TOWARDS_1,
  sprite_COMMANDANT_FACING_TOWARDS_2,
  sprite_COMMANDANT_FACING_TOWARDS_3,
  sprite_COMMANDANT_FACING_TOWARDS_4,
  sprite__LIMIT
};

extern const sprite_t sprites[sprite__LIMIT];

#endif /* SPRITES_H */

