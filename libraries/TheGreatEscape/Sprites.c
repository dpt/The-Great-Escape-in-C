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
 * The recreated version is copyright (c) 2012-2018 David Thomas
 */

#include <stdint.h>

#include "TheGreatEscape/SpriteBitmaps.h"

#include "TheGreatEscape/Sprites.h"

/**
 * $CE22: Sprites: objects which can move.
 *
 * This includes STOVE, CRATE, PRISONER, CRAWL, DOG, GUARD and COMMANDANT.
 */
const spritedef_t sprites[sprite__LIMIT] =
{
  { 3, 22, bitmap_stove,                            mask_stove                         }, // dims ok
  { 4, 24, bitmap_crate,                            mask_crate                         }, // dims ok

  { 3, 26, bitmap_prisoner_facing_top_left_1,       mask_various_facing_top_left_1     }, // ! sprite is 26 high
  { 3, 27, bitmap_prisoner_facing_top_left_2,       mask_various_facing_top_left_2     }, // ! sprite is 27 high
  { 3, 27, bitmap_prisoner_facing_top_left_3,       mask_various_facing_top_left_3     }, // ! sprite is 27 high
  { 3, 27, bitmap_prisoner_facing_top_left_4,       mask_various_facing_top_left_4     }, // ! sprite is 27 high
  { 3, 26, bitmap_prisoner_facing_bottom_right_1,   mask_various_facing_bottom_right_1 }, // ! sprite is 26 high
  { 3, 28, bitmap_prisoner_facing_bottom_right_2,   mask_various_facing_bottom_right_2 }, // ! sprite is 28 high
  { 3, 27, bitmap_prisoner_facing_bottom_right_3,   mask_various_facing_bottom_right_3 }, // ! sprite is 27 high
  { 3, 27, bitmap_prisoner_facing_bottom_right_4,   mask_various_facing_bottom_right_4 }, // ! sprite is 27 high

  { 4, 16, bitmap_crawl_facing_bottom_left_1,       mask_crawl_facing_bottom_left      }, // dims ok
  { 4, 15, bitmap_crawl_facing_bottom_left_2,       mask_crawl_facing_bottom_left      }, // dims ok
  { 4, 16, bitmap_crawl_facing_top_left_1,          mask_crawl_facing_top_left         }, // dims ok
  { 4, 16, bitmap_crawl_facing_top_left_2,          mask_crawl_facing_top_left         }, // dims ok

  { 4, 16, bitmap_dog_facing_top_left_1,            mask_dog_facing_top_left           }, // dims ok
  { 4, 16, bitmap_dog_facing_top_left_2,            mask_dog_facing_top_left           }, // dims ok
  { 4, 15, bitmap_dog_facing_top_left_3,            mask_dog_facing_top_left           }, // dims ok
  { 4, 15, bitmap_dog_facing_top_left_4,            mask_dog_facing_top_left           }, // dims ok
  { 4, 14, bitmap_dog_facing_bottom_right_1,        mask_dog_facing_bottom_right       }, // dims ok
  { 4, 15, bitmap_dog_facing_bottom_right_2,        mask_dog_facing_bottom_right       }, // dims ok
  { 4, 13, bitmap_dog_facing_bottom_right_3,        mask_dog_facing_bottom_right       }, // ! sprite is 13 high
  { 4, 14, bitmap_dog_facing_bottom_right_4,        mask_dog_facing_bottom_right       }, // dims ok

  { 3, 27, bitmap_guard_facing_top_left_1,          mask_various_facing_top_left_1     }, // dims ok
  { 3, 29, bitmap_guard_facing_top_left_2,          mask_various_facing_top_left_2     }, // dims ok
  { 3, 27, bitmap_guard_facing_top_left_3,          mask_various_facing_top_left_3     }, // dims ok
  { 3, 27, bitmap_guard_facing_top_left_4,          mask_various_facing_top_left_4     }, // dims ok
  { 3, 29, bitmap_guard_facing_bottom_right_1,      mask_various_facing_bottom_right_1 }, // dims ok
  { 3, 29, bitmap_guard_facing_bottom_right_2,      mask_various_facing_bottom_right_2 }, // dims ok
  { 3, 28, bitmap_guard_facing_bottom_right_3,      mask_various_facing_bottom_right_3 }, // dims ok
  { 3, 28, bitmap_guard_facing_bottom_right_4,      mask_various_facing_bottom_right_4 }, // dims ok

  { 3, 28, bitmap_commandant_facing_top_left_1,     mask_various_facing_top_left_1     }, // dims ok
  { 3, 30, bitmap_commandant_facing_top_left_2,     mask_various_facing_top_left_2     }, // dims ok
  { 3, 29, bitmap_commandant_facing_top_left_3,     mask_various_facing_top_left_3     }, // dims ok
  { 3, 29, bitmap_commandant_facing_top_left_4,     mask_various_facing_top_left_4     }, // dims ok
  { 3, 27, bitmap_commandant_facing_bottom_right_1, mask_various_facing_bottom_right_1 }, // dims ok
  { 3, 28, bitmap_commandant_facing_bottom_right_2, mask_various_facing_bottom_right_2 }, // dims ok
  { 3, 27, bitmap_commandant_facing_bottom_right_3, mask_various_facing_bottom_right_3 }, // dims ok
  { 3, 28, bitmap_commandant_facing_bottom_right_4, mask_various_facing_bottom_right_4 }, // dims ok
};

// vim: ts=8 sts=2 sw=2 et

