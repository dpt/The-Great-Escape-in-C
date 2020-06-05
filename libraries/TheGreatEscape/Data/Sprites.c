/**
 * Sprites.c
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

/* ----------------------------------------------------------------------- */

#include "TheGreatEscape/SpriteBitmaps.h"

#include "TheGreatEscape/Sprites.h"

/* ----------------------------------------------------------------------- */

/**
 * $CE22: Sprites: objects which can move.
 *
 * This includes STOVE, CRATE, PRISONER, CRAWL, DOG, GUARD and COMMANDANT.
 *
 * BUG FIX: The heights here have been corrected to match the actual bitmap and
 *          mask data heights. In the original game this was not the case and
 *          glitches could be seen because of it, e.g. when a guard dog is
 *          running towards the bottom right part of the next frame can be seen.
 */
const spritedef_t sprites[sprite__LIMIT] =
{
  { 3, 22, bitmap_stove,                            mask_stove                         },
  { 4, 24, bitmap_crate,                            mask_crate                         },

  { 3, 26, bitmap_prisoner_facing_top_left_1,       mask_various_facing_top_left_1     }, /* was 28 high */
  { 3, 27, bitmap_prisoner_facing_top_left_2,       mask_various_facing_top_left_2     }, /* was 28 high */
  { 3, 27, bitmap_prisoner_facing_top_left_3,       mask_various_facing_top_left_3     }, /* was 28 high */
  { 3, 27, bitmap_prisoner_facing_top_left_4,       mask_various_facing_top_left_4     }, /* was 28 high */
  { 3, 26, bitmap_prisoner_facing_bottom_right_1,   mask_various_facing_bottom_right_1 }, /* was 27 high */
  { 3, 28, bitmap_prisoner_facing_bottom_right_2,   mask_various_facing_bottom_right_2 }, /* was 29 high */
  { 3, 27, bitmap_prisoner_facing_bottom_right_3,   mask_various_facing_bottom_right_3 }, /* was 28 high */
  { 3, 27, bitmap_prisoner_facing_bottom_right_4,   mask_various_facing_bottom_right_4 }, /* was 28 high */

  { 4, 16, bitmap_crawl_facing_bottom_left_1,       mask_crawl_facing_bottom_left      },
  { 4, 15, bitmap_crawl_facing_bottom_left_2,       mask_crawl_facing_bottom_left      },
  { 4, 16, bitmap_crawl_facing_top_left_1,          mask_crawl_facing_top_left         },
  { 4, 16, bitmap_crawl_facing_top_left_2,          mask_crawl_facing_top_left         },

  { 4, 16, bitmap_dog_facing_top_left_1,            mask_dog_facing_top_left           },
  { 4, 16, bitmap_dog_facing_top_left_2,            mask_dog_facing_top_left           },
  { 4, 15, bitmap_dog_facing_top_left_3,            mask_dog_facing_top_left           },
  { 4, 15, bitmap_dog_facing_top_left_4,            mask_dog_facing_top_left           },
  { 4, 14, bitmap_dog_facing_bottom_right_1,        mask_dog_facing_bottom_right       },
  { 4, 15, bitmap_dog_facing_bottom_right_2,        mask_dog_facing_bottom_right       },
  { 4, 13, bitmap_dog_facing_bottom_right_3,        mask_dog_facing_bottom_right       }, /* was 15 high */
  { 4, 14, bitmap_dog_facing_bottom_right_4,        mask_dog_facing_bottom_right       },

  { 3, 27, bitmap_guard_facing_top_left_1,          mask_various_facing_top_left_1     },
  { 3, 29, bitmap_guard_facing_top_left_2,          mask_various_facing_top_left_2     },
  { 3, 27, bitmap_guard_facing_top_left_3,          mask_various_facing_top_left_3     },
  { 3, 27, bitmap_guard_facing_top_left_4,          mask_various_facing_top_left_4     },
  { 3, 29, bitmap_guard_facing_bottom_right_1,      mask_various_facing_bottom_right_1 },
  { 3, 29, bitmap_guard_facing_bottom_right_2,      mask_various_facing_bottom_right_2 },
  { 3, 28, bitmap_guard_facing_bottom_right_3,      mask_various_facing_bottom_right_3 },
  { 3, 28, bitmap_guard_facing_bottom_right_4,      mask_various_facing_bottom_right_4 },

  { 3, 28, bitmap_commandant_facing_top_left_1,     mask_various_facing_top_left_1     },
  { 3, 30, bitmap_commandant_facing_top_left_2,     mask_various_facing_top_left_2     },
  { 3, 29, bitmap_commandant_facing_top_left_3,     mask_various_facing_top_left_3     },
  { 3, 29, bitmap_commandant_facing_top_left_4,     mask_various_facing_top_left_4     },
  { 3, 27, bitmap_commandant_facing_bottom_right_1, mask_various_facing_bottom_right_1 },
  { 3, 28, bitmap_commandant_facing_bottom_right_2, mask_various_facing_bottom_right_2 },
  { 3, 27, bitmap_commandant_facing_bottom_right_3, mask_various_facing_bottom_right_3 },
  { 3, 28, bitmap_commandant_facing_bottom_right_4, mask_various_facing_bottom_right_4 },
};

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et
