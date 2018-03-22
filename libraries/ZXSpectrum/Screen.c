/* Screen.c
 *
 * ZX Spectrum screen decoding.
 *
 * Copyright (c) David Thomas, 2013-2018. <dave@davespace.co.uk>
 */

#include <stdlib.h>

#include "ZXSpectrum/Screen.h"

/* Define to highlight dirty rectangles when they're drawn. */
// #define SHOW_DIRTY_RECTS

#if defined(_WIN32) || defined(TGE_SDL) // ick
#define RGB
#endif

#ifdef RGB
/* 0x00RRGGBB */
#define BK_ 0x00000000
#define RD_ 0x00010000
#define GR_ 0x00000100
#define YL_ 0x00010100
#define BL_ 0x00000001
#define MG_ 0x00010001
#define CY_ 0x00000101
#define WH_ 0x00010101
#else
/* 0x00BBGGRR */
#define BK_ 0x00000000
#define RD_ 0x00000001
#define GR_ 0x00000100
#define YL_ 0x00000101
#define BL_ 0x00010000
#define MG_ 0x00010001
#define CY_ 0x00010100
#define WH_ 0x00010101
#endif

#define NORMAL(C) (C * 0xCD)
#define BRIGHT(C) (C * 0xFF)

// BRIGHT 0
#define BKd NORMAL(BK_)
#define RDd NORMAL(RD_)
#define GRd NORMAL(GR_)
#define YLd NORMAL(YL_)
#define BLd NORMAL(BL_)
#define MGd NORMAL(MG_)
#define CYd NORMAL(CY_)
#define WHd NORMAL(WH_)
// BRIGHT 1
#define BKb BRIGHT(BK_)
#define RDb BRIGHT(RD_)
#define GRb BRIGHT(GR_)
#define YLb BRIGHT(YL_)
#define BLb BRIGHT(BL_)
#define MGb BRIGHT(MG_)
#define CYb BRIGHT(CY_)
#define WHb BRIGHT(WH_)

/* Given a set of tokens: A, B, C
 *
 * Which have permutations:
 * (AA, AB, AC,
 *  BA, BB, BC,
 *  CA, CB, CC)
 *
 * They can be written out contiguously:
 * AAABACBABBBCCACBCC
 *
 * We can remove and index the redundant overlaps:
 * AABACBBCACC
 *
 * And produce a mapping:
 * (0, 1, 3,
 *  2, 5, 6,
 *  7, 4, 9)
 *
 * This is a De Bruijn sequence.
 *
 * Note that 'AC' occurs twice in the reduced output...
 *
 * The following palette is also a De Bruijn sequence.
 */
static const unsigned int palette[66 + 65] =
{
  BKd, BKd, BLd, BKd, RDd, BKd, MGd, BKd,
  GRd, BKd, CYd, BKd, YLd, BKd, WHd, BLd,
  BLd, RDd, BLd, MGd, BLd, GRd, BLd, CYd,
  BLd, YLd, BLd, WHd, RDd, RDd, MGd, RDd,
  GRd, RDd, CYd, RDd, YLd, RDd, WHd, MGd,
  MGd, GRd, MGd, CYd, MGd, YLd, MGd, WHd,
  GRd, GRd, CYd, GRd, YLd, GRd, WHd, CYd,
  CYd, YLd, CYd, WHd, YLd, YLd, WHd, BKd,
  WHd, WHd,

  /* Zeroth bright entry is shared. */
       BKb, BLb, BKb, RDb, BKb, MGb, BKb,
  GRb, BKb, CYb, BKb, YLb, BKb, WHb, BLb,
  BLb, RDb, BLb, MGb, BLb, GRb, BLb, CYb,
  BLb, YLb, BLb, WHb, RDb, RDb, MGb, RDb,
  GRb, RDb, CYb, RDb, YLb, RDb, WHb, MGb,
  MGb, GRb, MGb, CYb, MGb, YLb, MGb, WHb,
  GRb, GRb, CYb, GRb, YLb, GRb, WHb, CYb,
  CYb, YLb, CYb, WHb, YLb, YLb, WHb, BKb,
  WHb, WHb,
};

static const unsigned char offsets[66 + 65] =
{
    0,   1,   3,   5,   7,   9,  11,  13,
    2,  15,  16,  18,  20,  22,  24,  26,
    4,  17,  28,  29,  31,  33,  35,  37,
    6,  19,  30,  39,  40,  42,  44,  46,
    8,  21,  32,  41,  48,  49,  51,  53,
   10,  23,  34,  43,  50,  55,  56,  58,
   12,  25,  36,  45,  52,  57,  60,  61,
   62,  14,  27,  38,  47,  54,  59,  64,
    0,  66,  68,  70,  72,  74,  76,  78,
   67,  80,  81,  83,  85,  87,  89,  91,
   69,  82,  93,  94,  96,  98, 100, 102,
   71,  84,  95, 104, 105, 107, 109, 111,
   73,  86,  97, 106, 113, 114, 116, 118,
   75,  88,  99, 108, 115, 120, 121, 123,
   77,  90, 101, 110, 117, 122, 125, 126,
  127,  79,  92, 103, 112, 119, 124, 129,
};

#define WRITE8PIX(shift) \
do { \
  pal = &palette[offsets[(attrs >> shift) & 0x7F]]; \
  *poutput++ = pal[(input >> (shift + 7)) & 1]; \
  *poutput++ = pal[(input >> (shift + 6)) & 1]; \
  *poutput++ = pal[(input >> (shift + 5)) & 1]; \
  *poutput++ = pal[(input >> (shift + 4)) & 1]; \
  *poutput++ = pal[(input >> (shift + 3)) & 1]; \
  *poutput++ = pal[(input >> (shift + 2)) & 1]; \
  *poutput++ = pal[(input >> (shift + 1)) & 1]; \
  *poutput++ = pal[(input >> (shift + 0)) & 1]; \
} while (0)

#define CLAMP(val, min, max) \
  ((val < min) ? min : (val > max) ? max : val)

/* For reference:
 *
 * Spectrum screen memory has the arrangement:
 * 0b010BBLLLRRRCCCCC (B = band, L = line, R = row, C = column)
 *
 * Attribute bytes have the format:
 * 0bLRBBBFFF (L = flash, R = bright, B = background, F = foreground)
 */

void zxscreen_convert(const void    *vscr,
                      unsigned int  *poutput,
                      const zxbox_t *dirty)
{
  static const zxbox_t fullscreen = { 0, 0, 256, 192 };

  zxbox_t              box;
  int                  height;
  const unsigned int  *pattrs;
  int                  width;
  int                  x,linear_y;
  const unsigned int  *pinput;
  unsigned int         input;
  unsigned int         attrs;
  const unsigned int  *pal;

#ifdef SHOW_DIRTY_RECTS
  static int foo;
  foo = 0x20202020 - foo;
#endif

  if (dirty)
  {
    /* If a dirty rectangle was given clamp it to the screen dimensions. */
    box.x0 = CLAMP(dirty->x0, 0, 255);
    box.y0 = CLAMP(dirty->y0, 0, 191);
    box.x1 = CLAMP(dirty->x1, 1, 256);
    box.y1 = CLAMP(dirty->y1, 1, 192);
  }
  else
  {
    /* If no dirty rectangle was specified then assume the full screen. */
    box = fullscreen;
  }

  /* The inner loop processes 32 pixels at a time so we convert x coordinates
   * into chunks four attributes wide while rounding up and down as
   * required. */
  box.x0 = (box.x0     ) / 32; /* divide to 0..7 rounding down */
  box.x1 = (box.x1 + 31) / 32; /* divide to 0..7 rounding up */

  /* Convert y coordinates into screen space - (0,0) is top left. */
  height = box.y1 - box.y0;
  box.y0 = 192 - box.y1;
  box.y1 = box.y0 + height;

  pattrs = (const unsigned int *) vscr
         + (SCREEN_BITMAP_LENGTH
         + box.y0 / 8 * 32  /* 8 scanlines/row, 32 attrs/row */
         + box.x0 * 4) / 4; /* 4 bytes/chunk, 4 bytes/word */

  poutput += box.y0 * 256 /* 256 pixels/row (256 words) for output */
           + box.x0 * 32; /* 32 pixels/chunk (32 words) */

  width = box.x1 - box.x0; /* hoisted out of loop */

  for (linear_y = box.y0; linear_y < box.y1; linear_y++)
  {
    /* Transpose fields using XOR */
    unsigned int tmp = (linear_y ^ (linear_y >> 3)) & 7;
    int          y   = linear_y ^ (tmp | (tmp << 3));

    pinput = (const unsigned int *) vscr
           + (y     * 32           /* 32 bytes/row */
           + box.x0 * 32 / 8) / 4; /* 32 bytes/row, 8 pixels/byte, 4 bytes/word */
    for (x = width; x > 0; x--) /* x is unused in the loop body */
    {
      input = *pinput++;
      attrs = *pattrs++;
#ifdef SHOW_DIRTY_RECTS
      attrs ^= foo; /* force colour attrs to show redrawn areas */
#endif

      WRITE8PIX(0);
      WRITE8PIX(8);
      WRITE8PIX(16);
      WRITE8PIX(24);
    }

    /* Skip to the start of the next row. */
    pattrs  += 8   - width;
    poutput += 256 - width * 32;

    /* Rewind pattrs except at the end of an attribute row. */
    if ((linear_y & 7) != 7)
      pattrs -= 8;
  }
}

// vim: ts=8 sts=2 sw=2 et
