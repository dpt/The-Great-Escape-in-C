/* Screen.c
 *
 * ZX Spectrum screen decoding.
 *
 * Copyright (c) David Thomas, 2013-2020. <dave@davespace.co.uk>
 */

#include <assert.h>
#include <stdlib.h>

#include "ZXSpectrum/Macros.h"

#include "ZXSpectrum/Screen.h"

/* Define to highlight dirty rectangles when they're drawn. */
//#define SHOW_DIRTY_RECTS

/* Ideally, hoist this test out. */
#if defined(_WIN32) || defined(TGE_SDL)
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

/* BRIGHT 0 */
#define BKd NORMAL(BK_)
#define RDd NORMAL(RD_)
#define GRd NORMAL(GR_)
#define YLd NORMAL(YL_)
#define BLd NORMAL(BL_)
#define MGd NORMAL(MG_)
#define CYd NORMAL(CY_)
#define WHd NORMAL(WH_)
/* BRIGHT 1 */
#define BKb BRIGHT(BK_)
#define RDb BRIGHT(RD_)
#define GRb BRIGHT(GR_)
#define YLb BRIGHT(YL_)
#define BLb BRIGHT(BL_)
#define MGb BRIGHT(MG_)
#define CYb BRIGHT(CY_)
#define WHb BRIGHT(WH_)

/* Given a set of tokens: A, B, C, which have permutations:
 *   (AA, AB, AC, BA, BB, BC, CA, CB, CC)
 * They can be written out contiguously as:
 *   AAABACBABBBCCACBCC
 * We can remove and index the redundant overlaps:
 *   AABACBBCACC
 * (Note that 'AC' occurs twice in the sequence).
 * Producing a mapping:
 *   (0, 1, 3, 2, 5, 6, 7, 4, 9)
 * This is a De Bruijn sequence.
 *
 * The following palette is laid out in a De Bruijn sequence in a vain attempt
 * to improve performance.
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

  /* The zeroth bright entry is shared. */
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

#define WRITE8PIX(shift)                            \
do {                                                \
  pal = &palette[offsets[(attrs >> shift) & 0x7F]]; \
  *poutput++ = pal[(input >> (shift + 7)) & 1];     \
  *poutput++ = pal[(input >> (shift + 6)) & 1];     \
  *poutput++ = pal[(input >> (shift + 5)) & 1];     \
  *poutput++ = pal[(input >> (shift + 4)) & 1];     \
  *poutput++ = pal[(input >> (shift + 3)) & 1];     \
  *poutput++ = pal[(input >> (shift + 2)) & 1];     \
  *poutput++ = pal[(input >> (shift + 1)) & 1];     \
  *poutput++ = pal[(input >> (shift + 0)) & 1];     \
} while (0)

/* For reference:
 *
 * Spectrum screen memory has the arrangement:
 * 0b010BBLLLRRRCCCCC (B = band, L = line, R = row, C = column)
 *
 * Attribute bytes have the format:
 * 0bLRBBBFFF (L = flash, R = bright, B = paper (background), F = ink (foreground))
 */

void zxscreen_convert(const void    *vscr,
                      unsigned int  *poutput,
                      const zxbox_t *dirty)
{
  zxbox_t              box;
  int                  height;
  const unsigned int  *pattrs;
  int                  width;
  int                  x,linear_y;
  const unsigned int  *pinput;
  unsigned int         input;
  unsigned int         attrs;
  const unsigned int  *pal;

  assert(dirty);

#ifdef SHOW_DIRTY_RECTS
  static int dirtybits;
  dirtybits = 0x20202020 - dirtybits;
#endif

  /* Clamp the dirty rectangle to the screen dimensions. */
  box.x0 = CLAMP(dirty->x0, 0, 255);
  box.y0 = CLAMP(dirty->y0, 0, 191);
  box.x1 = CLAMP(dirty->x1, 1, 256);
  box.y1 = CLAMP(dirty->y1, 1, 192);

  /* The inner loop processes 32 pixels at a time, so we need to convert x
   * coordinates into chunks four attributes wide while rounding up and down as
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
      attrs ^= dirtybits; /* force colour attrs to show redrawn areas */
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



// flash  = (attr & (1 << 7)) != 0;
// bright = (attr & (1 << 6)) != 0;
// paper  = (attr & (7 << 3)) >> 3;
// ink    = (attr & (7 << 0)) >> 0;
//
// for (attr = 0..127)
//   tab[i] = ((bright * 8 + ink)   << 0) |
//            ((bright * 8 + paper) << 4);

// Extract attribute colour and fold in bright flag
#define ATTR(v,s) ((((v) & (1 << 6)) >> 3) + (((v) >> s) & 7))

// Form 8x foreground pixels
#define FG(v) (ATTR(v,0) * 0x11111111U)

// Form 8x background pixels
#define BG(v) (ATTR(v,3) * 0x11111111U)

// A pixel entry
#define E(v) BG(v), FG(v),

// Macros to form 64 pixel entries at a time
#define E4(b) E(b+0) E(b+1) E(b+2) E(b+3)
#define E16(b) E4(b+0) E4(b+4) E4(b+8) E4(b+12)
#define E64(b) E16(b+0) E16(b+16) E16(b+32) E16(b+48)

// Note: This table could be De Brujin'd down
static const unsigned int pixels8tab[128 * 2] =
{
  E64(0) E64(64)
};

// Convert an 8-bit pixel into a 32-bit mask with a 0x0 or 0xF nibble for
// each input bit (also includes a byte flip).
static const unsigned int mask8tab[256] =
{
  0x00000000,
  0xF0000000,
  0x0F000000,
  0xFF000000,
  0x00F00000,
  0xF0F00000,
  0x0FF00000,
  0xFFF00000,
  0x000F0000,
  0xF00F0000,
  0x0F0F0000,
  0xFF0F0000,
  0x00FF0000,
  0xF0FF0000,
  0x0FFF0000,
  0xFFFF0000,
  0x0000F000,
  0xF000F000,
  0x0F00F000,
  0xFF00F000,
  0x00F0F000,
  0xF0F0F000,
  0x0FF0F000,
  0xFFF0F000,
  0x000FF000,
  0xF00FF000,
  0x0F0FF000,
  0xFF0FF000,
  0x00FFF000,
  0xF0FFF000,
  0x0FFFF000,
  0xFFFFF000,
  0x00000F00,
  0xF0000F00,
  0x0F000F00,
  0xFF000F00,
  0x00F00F00,
  0xF0F00F00,
  0x0FF00F00,
  0xFFF00F00,
  0x000F0F00,
  0xF00F0F00,
  0x0F0F0F00,
  0xFF0F0F00,
  0x00FF0F00,
  0xF0FF0F00,
  0x0FFF0F00,
  0xFFFF0F00,
  0x0000FF00,
  0xF000FF00,
  0x0F00FF00,
  0xFF00FF00,
  0x00F0FF00,
  0xF0F0FF00,
  0x0FF0FF00,
  0xFFF0FF00,
  0x000FFF00,
  0xF00FFF00,
  0x0F0FFF00,
  0xFF0FFF00,
  0x00FFFF00,
  0xF0FFFF00,
  0x0FFFFF00,
  0xFFFFFF00,
  0x000000F0,
  0xF00000F0,
  0x0F0000F0,
  0xFF0000F0,
  0x00F000F0,
  0xF0F000F0,
  0x0FF000F0,
  0xFFF000F0,
  0x000F00F0,
  0xF00F00F0,
  0x0F0F00F0,
  0xFF0F00F0,
  0x00FF00F0,
  0xF0FF00F0,
  0x0FFF00F0,
  0xFFFF00F0,
  0x0000F0F0,
  0xF000F0F0,
  0x0F00F0F0,
  0xFF00F0F0,
  0x00F0F0F0,
  0xF0F0F0F0,
  0x0FF0F0F0,
  0xFFF0F0F0,
  0x000FF0F0,
  0xF00FF0F0,
  0x0F0FF0F0,
  0xFF0FF0F0,
  0x00FFF0F0,
  0xF0FFF0F0,
  0x0FFFF0F0,
  0xFFFFF0F0,
  0x00000FF0,
  0xF0000FF0,
  0x0F000FF0,
  0xFF000FF0,
  0x00F00FF0,
  0xF0F00FF0,
  0x0FF00FF0,
  0xFFF00FF0,
  0x000F0FF0,
  0xF00F0FF0,
  0x0F0F0FF0,
  0xFF0F0FF0,
  0x00FF0FF0,
  0xF0FF0FF0,
  0x0FFF0FF0,
  0xFFFF0FF0,
  0x0000FFF0,
  0xF000FFF0,
  0x0F00FFF0,
  0xFF00FFF0,
  0x00F0FFF0,
  0xF0F0FFF0,
  0x0FF0FFF0,
  0xFFF0FFF0,
  0x000FFFF0,
  0xF00FFFF0,
  0x0F0FFFF0,
  0xFF0FFFF0,
  0x00FFFFF0,
  0xF0FFFFF0,
  0x0FFFFFF0,
  0xFFFFFFF0,
  0x0000000F,
  0xF000000F,
  0x0F00000F,
  0xFF00000F,
  0x00F0000F,
  0xF0F0000F,
  0x0FF0000F,
  0xFFF0000F,
  0x000F000F,
  0xF00F000F,
  0x0F0F000F,
  0xFF0F000F,
  0x00FF000F,
  0xF0FF000F,
  0x0FFF000F,
  0xFFFF000F,
  0x0000F00F,
  0xF000F00F,
  0x0F00F00F,
  0xFF00F00F,
  0x00F0F00F,
  0xF0F0F00F,
  0x0FF0F00F,
  0xFFF0F00F,
  0x000FF00F,
  0xF00FF00F,
  0x0F0FF00F,
  0xFF0FF00F,
  0x00FFF00F,
  0xF0FFF00F,
  0x0FFFF00F,
  0xFFFFF00F,
  0x00000F0F,
  0xF0000F0F,
  0x0F000F0F,
  0xFF000F0F,
  0x00F00F0F,
  0xF0F00F0F,
  0x0FF00F0F,
  0xFFF00F0F,
  0x000F0F0F,
  0xF00F0F0F,
  0x0F0F0F0F,
  0xFF0F0F0F,
  0x00FF0F0F,
  0xF0FF0F0F,
  0x0FFF0F0F,
  0xFFFF0F0F,
  0x0000FF0F,
  0xF000FF0F,
  0x0F00FF0F,
  0xFF00FF0F,
  0x00F0FF0F,
  0xF0F0FF0F,
  0x0FF0FF0F,
  0xFFF0FF0F,
  0x000FFF0F,
  0xF00FFF0F,
  0x0F0FFF0F,
  0xFF0FFF0F,
  0x00FFFF0F,
  0xF0FFFF0F,
  0x0FFFFF0F,
  0xFFFFFF0F,
  0x000000FF,
  0xF00000FF,
  0x0F0000FF,
  0xFF0000FF,
  0x00F000FF,
  0xF0F000FF,
  0x0FF000FF,
  0xFFF000FF,
  0x000F00FF,
  0xF00F00FF,
  0x0F0F00FF,
  0xFF0F00FF,
  0x00FF00FF,
  0xF0FF00FF,
  0x0FFF00FF,
  0xFFFF00FF,
  0x0000F0FF,
  0xF000F0FF,
  0x0F00F0FF,
  0xFF00F0FF,
  0x00F0F0FF,
  0xF0F0F0FF,
  0x0FF0F0FF,
  0xFFF0F0FF,
  0x000FF0FF,
  0xF00FF0FF,
  0x0F0FF0FF,
  0xFF0FF0FF,
  0x00FFF0FF,
  0xF0FFF0FF,
  0x0FFFF0FF,
  0xFFFFF0FF,
  0x00000FFF,
  0xF0000FFF,
  0x0F000FFF,
  0xFF000FFF,
  0x00F00FFF,
  0xF0F00FFF,
  0x0FF00FFF,
  0xFFF00FFF,
  0x000F0FFF,
  0xF00F0FFF,
  0x0F0F0FFF,
  0xFF0F0FFF,
  0x00FF0FFF,
  0xF0FF0FFF,
  0x0FFF0FFF,
  0xFFFF0FFF,
  0x0000FFFF,
  0xF000FFFF,
  0x0F00FFFF,
  0xFF00FFFF,
  0x00F0FFFF,
  0xF0F0FFFF,
  0x0FF0FFFF,
  0xFFF0FFFF,
  0x000FFFFF,
  0xF00FFFFF,
  0x0F0FFFFF,
  0xFF0FFFFF,
  0x00FFFFFF,
  0xF0FFFFFF,
  0x0FFFFFFF,
  0xFFFFFFFF
};

// Form 8 4bpp pixels at a time
#define WRITE8PIX_16(shift) \
do { \
  bg = pixels8tab[((attrs >> shift) & 0x7F) * 2 + 0]; \
  fg = pixels8tab[((attrs >> shift) & 0x7F) * 2 + 1]; \
  mask = mask8tab[input]; \
  *poutput++ = (bg & ~mask) | (fg & mask); \
} while (0)

void zxscreen_convert16(const void    *vscr,
                        unsigned int  *poutput,
                        const zxbox_t *dirty)
{
  zxbox_t              box;
  int                  height;
  const unsigned char *pattrs;
  int                  width;
  int                  linear_y;
  const unsigned char *pinput;
  int                  x;
  unsigned int         input;
  unsigned int         attrs;
  unsigned int         bg;
  unsigned int         fg;
  unsigned int         mask;

  assert(dirty);

#ifdef SHOW_DIRTY_RECTS
  static int foo;
  foo = 0x20202020 - foo;
#endif

  /* Clamp the dirty rectangle to the screen dimensions. */
  box.x0 = CLAMP(dirty->x0, 0, 255);
  box.y0 = CLAMP(dirty->y0, 0, 191);
  box.x1 = CLAMP(dirty->x1, 1, 256);
  box.y1 = CLAMP(dirty->y1, 1, 192);

  /* The inner loop processes 8 pixels at a time so we convert x coordinates
   * into chunks one attribute wide while rounding up and down as required.
   */
#define CHUNK 8
  box.x0 = (box.x0            ) / CHUNK; /* divide to 0..31 rounding down */
  box.x1 = (box.x1 + CHUNK - 1) / CHUNK; /* divide to 0..31 rounding up */

  /* Convert y coordinates into screen space - (0,0) is top left. */
  height = box.y1 - box.y0;
  box.y0 = 192 - box.y1;
  box.y1 = box.y0 + height;

  pattrs = (const unsigned char *) vscr
         + SCREEN_BITMAP_LENGTH
         + box.y0 / 8 * 32
         + box.x0; /* 8 scanlines/row, 32 attrs/row */

  poutput += box.y0 * 32 /* 256 pixels/row (128 bytes / 32 words) for output */
           + box.x0; /* 8 pixels/chunk (1 word) */

  width = box.x1 - box.x0; /* hoisted out of loop */

  for (linear_y = box.y0; linear_y < box.y1; linear_y++)
  {
    /* Transpose fields using XOR */
    unsigned int tmp = (linear_y ^ (linear_y >> 3)) & 7;
    int          y   = linear_y ^ (tmp | (tmp << 3));

    pinput = (const unsigned char *) vscr
           + y * 32 /* 32 bytes/row */
           + box.x0;
    for (x = width; x > 0; x--) /* x is unused in the loop body */
    {
#ifdef SHOW_DIRTY_RECTS
      attrs ^= foo; /* force colour attrs to show redrawn areas */
#endif

      input = *pinput++;
      attrs = *pattrs++;

      WRITE8PIX_16(0);
    }

    /* Skip to the start of the next row. */
    pattrs  += 32 - width;
    poutput += (256 - width * CHUNK) / 8; // == 32 - width

    /* Rewind pattrs except at the end of an attribute row. */
    if ((linear_y & 7) != 7)
      pattrs -= 32;
  }
}

// vim: ts=8 sts=2 sw=2 et
