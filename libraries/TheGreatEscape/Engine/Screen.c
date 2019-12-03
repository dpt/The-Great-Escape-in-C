/**
 * Screen.c
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

#include "TheGreatEscape/TheGreatEscape.h"

#include "TheGreatEscape/Screen.h"
#include "TheGreatEscape/State.h"

/* Conv: These helper functions were added over the original game. */

void invalidate_bitmap(tgestate_t *state,
                       uint8_t    *start,
                       int         width,
                       int         height)
{
  uint8_t   *base = &state->speccy->screen.pixels[0];
  ptrdiff_t  offset;
  int        x,y;
  zxbox_t    dirty;

  offset = start - base;

  /* Convert screen offset to cartesian for the interface. */
  x = (offset & 31) * 8;
  y = ((offset & 0x0700) >> 8) |
      ((offset & 0x00E0) >> 2) |
      ((offset & 0x1800) >> 5);
  y = 191 - y;    /* flip */
  y = y + 1;      /* inclusive lower bound becomes exclusive upper */
  y = y - height; /* get min-y */

  dirty.x0 = x;
  dirty.y0 = y;
  dirty.x1 = x + width;
  dirty.y1 = y + height;
  state->speccy->draw(state->speccy, &dirty);
}

void invalidate_attrs(tgestate_t *state,
                      uint8_t    *start,
                      int         width,
                      int         height)
{
  uint8_t   *base = &state->speccy->screen.pixels[SCREEN_BITMAP_LENGTH];
  ptrdiff_t  offset;
  int        x,y;
  zxbox_t    dirty;

  offset = start - base;

  /* Convert attribute offset to cartesian for the interface. */
  x = (offset & 31) * 8;
  y = (int) offset >> 5;
  y = 23 - y;     /* flip */
  y = y + 1;      /* inclusive lower bound becomes exclusive upper */
  y = y * 8;      /* scale */
  y = y - height; /* get min-y */

  dirty.x0 = x;
  dirty.y0 = y;
  dirty.x1 = x + width;
  dirty.y1 = y + height;
  state->speccy->draw(state->speccy, &dirty);
}
