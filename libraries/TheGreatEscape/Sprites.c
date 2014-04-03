#include <stdint.h>

#include "TheGreatEscape/SpriteBitmaps.h"

#include "TheGreatEscape/Sprites.h"

/**
 * $CE22: Sprites: objects which can move.
 *
 * This include STOVE, CRATE, PRISONER, CRAWL, DOG, GUARD and COMMANDANT.
 */
const sprite_t sprites[sprite__LIMIT] =
{
  { 3, 22, bitmap_stove,                            mask_stove                         },
  { 4, 24, bitmap_crate,                            mask_crate                         },
  { 3, 27, bitmap_prisoner_facing_top_left_4,       mask_various_facing_top_left_4     },
  { 3, 28, bitmap_prisoner_facing_top_left_3,       mask_various_facing_top_left_3     },
  { 3, 28, bitmap_prisoner_facing_top_left_2,       mask_various_facing_top_left_2     },
  { 3, 28, bitmap_prisoner_facing_top_left_1,       mask_various_facing_top_left_1     },
  { 3, 27, bitmap_prisoner_facing_bottom_right_1,   mask_various_facing_bottom_right_1 },
  { 3, 29, bitmap_prisoner_facing_bottom_right_2,   mask_various_facing_bottom_right_2 },
  { 3, 28, bitmap_prisoner_facing_bottom_right_3,   mask_various_facing_bottom_right_3 },
  { 3, 28, bitmap_prisoner_facing_bottom_right_4,   mask_various_facing_bottom_right_4 },
  { 4, 16, bitmap_crawl_facing_bottom_left_2,       mask_crawl_facing_bottom_left      },
  { 4, 15, bitmap_crawl_facing_bottom_left_1,       mask_crawl_facing_bottom_left      },
  { 4, 16, bitmap_crawl_facing_top_left_1,          mask_crawl_facing_bottom_left      },
  { 4, 16, bitmap_crawl_facing_top_left_2,          mask_crawl_facing_bottom_left      },
  { 4, 16, bitmap_dog_facing_top_left_1,            mask_dog_facing_top_left           },
  { 4, 16, bitmap_dog_facing_top_left_2,            mask_dog_facing_top_left           },
  { 4, 15, bitmap_dog_facing_top_left_3,            mask_dog_facing_top_left           },
  { 4, 15, bitmap_dog_facing_top_left_4,            mask_dog_facing_top_left           },
  { 4, 14, bitmap_dog_facing_bottom_right_1,        mask_dog_facing_bottom_right       },
  { 4, 15, bitmap_dog_facing_bottom_right_2,        mask_dog_facing_bottom_right       },
  { 4, 15, bitmap_dog_facing_bottom_right_3,        mask_dog_facing_bottom_right       },
  { 4, 14, bitmap_dog_facing_bottom_right_4,        mask_dog_facing_bottom_right       },
  { 3, 27, bitmap_guard_facing_top_left_4,          mask_various_facing_top_left_4     },
  { 3, 29, bitmap_guard_facing_top_left_3,          mask_various_facing_top_left_3     },
  { 3, 27, bitmap_guard_facing_top_left_2,          mask_various_facing_top_left_2     },
  { 3, 27, bitmap_guard_facing_top_left_1,          mask_various_facing_top_left_1     },
  { 3, 29, bitmap_guard_facing_bottom_right_1,      mask_various_facing_bottom_right_1 },
  { 3, 29, bitmap_guard_facing_bottom_right_2,      mask_various_facing_bottom_right_2 },
  { 3, 28, bitmap_guard_facing_bottom_right_3,      mask_various_facing_bottom_right_3 },
  { 3, 28, bitmap_guard_facing_bottom_right_4,      mask_various_facing_bottom_right_4 },
  { 3, 28, bitmap_commandant_facing_top_left_4,     mask_various_facing_top_left_4     },
  { 3, 30, bitmap_commandant_facing_top_left_3,     mask_various_facing_top_left_3     },
  { 3, 29, bitmap_commandant_facing_top_left_2,     mask_various_facing_top_left_2     },
  { 3, 29, bitmap_commandant_facing_top_left_1,     mask_various_facing_top_left_1     },
  { 3, 27, bitmap_commandant_facing_bottom_right_1, mask_various_facing_bottom_right_1 },
  { 3, 28, bitmap_commandant_facing_bottom_right_2, mask_various_facing_bottom_right_2 },
  { 3, 27, bitmap_commandant_facing_bottom_right_3, mask_various_facing_bottom_right_3 },
  { 3, 28, bitmap_commandant_facing_bottom_right_4, mask_various_facing_bottom_right_4 },
};
