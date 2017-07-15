/* ZXScreen.c
 *
 * ZX Spectrum screen decoding.
 *
 * Copyright (c) David Thomas, 2013-2015. <dave@davespace.co.uk>
 */

#include <stdlib.h>

#include "ZXSpectrum/Screen.h"

// Spectrum screen memory has the arrangement:
// 0b010BBLLLRRRCCCCC (B = band, L = line, R = row, C = column)
//
// Attribute bytes have the format:
// 0bLRBBBFFF (L = flash, R = bright, B = background, F = foreground)

/* Given a screen memory address with fixed and column bits discarded,
 * returns the linear offset. */
static int rows[192];

void zxscreen_initialise(void)
{
  static int initialised = 0;

  int i;
  // int band, line, row;

  if (initialised)
    return;

  for (i = 0; i < 192; i++)
  {
    // band = (i >> 6) & 3;
    // line = (i >> 3) & 7;
    // row  = (i >> 0) & 7;
    //
    // rows[i] = (band << 6) | (row << 3) | (line >> 0);
    // rows[i] = (i & ~63) | ((i & 56) >> 3) | ((i & 7) << 3); // 8 ops

    /* Transpose fields using XOR. */
    unsigned int x = (i ^ (i >> 3)) & 7; // XOR temporary
    rows[i] = i ^ (x | (x << 3));
  }

  initialised = 1;
}

#ifdef _WIN32
#define BK_ 0x00000000
#define RD_ 0x00010000
#define GR_ 0x00000100
#define YL_ 0x00010100
#define BL_ 0x00000001
#define MG_ 0x00010001
#define CY_ 0x00000101
#define WH_ 0x00010101
#else
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

// Given a set of tokens: A, B, C
//
// Which have permutations:
// (AA, AB, AC,
//  BA, BB, BC,
//  CA, CB, CC)
//
// They can be written out contiguously:
// AAABACBABBBCCACBCC
//
// We can remove and index the redundant overlaps:
// AABACBBCACC
//
// And produce a mapping:
// (0, 1, 3,
//  2, 5, 6,
//  7, 4, 9)
//
// This is a De Bruijn sequence.
//
// Note that 'AC' occurs twice in the reduced output...
//
// The following palette is also a De Bruijn sequence.
//
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

void zxscreen_convert(const void *vscr, unsigned int *poutput)
{
  const unsigned int *pattrs;
  int                 x,y;
  const unsigned int *pinput;
  unsigned int        input;
  unsigned int        attrs;
  const unsigned int *pal;

  pattrs = (const unsigned int *) vscr + 8 * 192;
  for (y = 0; y < 192; y++)
  {
    /* Transpose fields using XOR. */
    unsigned int xt = (y ^ (y >> 3)) & 7;
    int ny = y ^ (xt | (xt << 3));

    pinput = (const unsigned int *) vscr + (ny << 3);
    for (x = 0; x < 8; x++)
    {
      input = *pinput++;
      attrs = *pattrs++;

      WRITE8PIX(0);
      WRITE8PIX(8);
      WRITE8PIX(16);
      WRITE8PIX(24);
    }
    if ((y & 7) != 7)
      pattrs -= 8;
  }
}
