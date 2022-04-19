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

/* ----------------------------------------------------------------------- */

#include <stddef.h>

#include "C99/Types.h"

#include "TheGreatEscape/State.h"

#include "TheGreatEscape/Screen.h"

/* ----------------------------------------------------------------------- */

/**
 * $EDD3: Game screen start addresses.
 *
 * Absolute addresses in the original code. These are now offsets.
 */
const uint16_t game_window_start_offsets[128] =
{
  0x0047,
  0x0147,
  0x0247,
  0x0347,
  0x0447,
  0x0547,
  0x0647,
  0x0747,
  0x0067,
  0x0167,
  0x0267,
  0x0367,
  0x0467,
  0x0567,
  0x0667,
  0x0767,
  0x0087,
  0x0187,
  0x0287,
  0x0387,
  0x0487,
  0x0587,
  0x0687,
  0x0787,
  0x00A7,
  0x01A7,
  0x02A7,
  0x03A7,
  0x04A7,
  0x05A7,
  0x06A7,
  0x07A7,
  0x00C7,
  0x01C7,
  0x02C7,
  0x03C7,
  0x04C7,
  0x05C7,
  0x06C7,
  0x07C7,
  0x00E7,
  0x01E7,
  0x02E7,
  0x03E7,
  0x04E7,
  0x05E7,
  0x06E7,
  0x07E7,
  0x0807,
  0x0907,
  0x0A07,
  0x0B07,
  0x0C07,
  0x0D07,
  0x0E07,
  0x0F07,
  0x0827,
  0x0927,
  0x0A27,
  0x0B27,
  0x0C27,
  0x0D27,
  0x0E27,
  0x0F27,
  0x0847,
  0x0947,
  0x0A47,
  0x0B47,
  0x0C47,
  0x0D47,
  0x0E47,
  0x0F47,
  0x0867,
  0x0967,
  0x0A67,
  0x0B67,
  0x0C67,
  0x0D67,
  0x0E67,
  0x0F67,
  0x0887,
  0x0987,
  0x0A87,
  0x0B87,
  0x0C87,
  0x0D87,
  0x0E87,
  0x0F87,
  0x08A7,
  0x09A7,
  0x0AA7,
  0x0BA7,
  0x0CA7,
  0x0DA7,
  0x0EA7,
  0x0FA7,
  0x08C7,
  0x09C7,
  0x0AC7,
  0x0BC7,
  0x0CC7,
  0x0DC7,
  0x0EC7,
  0x0FC7,
  0x08E7,
  0x09E7,
  0x0AE7,
  0x0BE7,
  0x0CE7,
  0x0DE7,
  0x0EE7,
  0x0FE7,
  0x1007,
  0x1107,
  0x1207,
  0x1307,
  0x1407,
  0x1507,
  0x1607,
  0x1707,
  0x1027,
  0x1127,
  0x1227,
  0x1327,
  0x1427,
  0x1527,
  0x1627,
  0x1727,
};

/* ----------------------------------------------------------------------- */

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

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et
