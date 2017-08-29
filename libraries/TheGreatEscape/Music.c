#include "TheGreatEscape/Music.h"

/* The menu screen music is a medley of:
 * - The Great Escape theme [Elmer Bernstein, 1963]
 * - Pack Up Your Troubles in Your Old Kit Bag (and Smile, Smile, Smile) [1915]
 * - It's a Long Way to Tipperary [1912]
 */

/* All semitones which the music routine can produce. */
#define C2    5
#define CS2   6
#define D2    7
#define DS2   8
#define E2    9
#define F2   10
#define FS2  11
#define G2   12
#define GS2  13
#define A2   14
#define AS2  15
#define B2   16
//
#define C3   17
#define CS3  18
#define D3   19
#define DS3  20
#define E3   21
#define F3   22
#define FS3  23
#define G3   24
#define GS3  25
#define A3   26
#define AS3  27
#define B3   28
//
#define C4   29 /* middle C */
#define CS4  30
#define D4   31
#define DS4  32
#define E4   33
#define F4   34
#define FS4  35
#define G4   36
#define GS4  37
#define A4   38
#define AS4  39
#define B4   40
//
#define C5   41
#define CS5  42
#define D5   43
#define DS5  44
#define E5   45
#define F5   46
#define FS5  47
#define G5   48
#define GS5  49
//
#define END 255

/**
 * $F546: High music channel notes (semitones).
 *
 * The notes for the high part sorted by popularity:
 *  36x AS3
 *  26x DS4
 *  19x F4
 *  19x C4
 *  18x GS4
 *  13x G4
 *  11x C5
 *  11x AS4
 *  10x D4
 *  10x GS3
 *  10x G3
 *   8x F3
 *   5x A3
 *   3x DS3
 *   3x D3
 *   3x CS3 (lowest note)
 *   2x CS5 (highest note)
 *   2x E3
 *   1x E4
 *   1x A4
 */
const uint8_t music_channel0_data[] =
{
  /* Start of The Great Escape theme. */
   D3,   0, DS3,   0,   0,  E3,  F3,   0, AS3,   0,   0,   0,   0,   0,   0,
    0,  F3,   0,  D4,   0,   0,  C4, AS3,   0,  G3,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,  C4,   0,   0,   0,  C4,   0,   0,
  AS3,  A3,   0, AS3,   0,  A3,   0,  G3,   0,  F3,   0,  D3,   0,   0,   0,
    0,   0,   0,   0,  D3,   0, DS3,   0,   0,  E3,  F3,   0, AS3,   0,   0,
    0,   0,   0,   0,   0,  F3,   0,  D4,   0,   0,  C4, AS3,   0,  G3,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  C4,   0,   0,
    0,  C4,   0,   0, AS3,  A3,   0,  F3,   0,   0,   0,  C4,   0, AS3,   0,
    0,   0,  D4,   0,   0,  C4, AS3,   0, GS3,   0,  G3,   0,  F3,   0,

  /* Start of "Pack Up Your Troubles in Your Old Kit Bag". */
  AS3,   0,   0,   0, AS3,   0,  C4,   0, AS3,   0, GS3,   0,  G3,   0, GS3,
    0, AS3,   0,   0,   0,  G4,   0,   0,   0,  G4,   0,   0,   0,  F4,   0,
    0,   0, DS4,   0,   0,   0,   0,   0,   0,   0,  C4,   0,   0,   0,   0,
    0,   0,   0, AS3,   0,   0,   0, AS3,   0,   0,  C4, AS3,   0, GS3,   0,
   G3,   0,  F3,   0, AS3,   0,   0,   0, AS3,   0,  C4,   0, AS3,   0, GS3,
    0,  G3,   0, GS3,   0, AS3,   0,   0,   0,  G4,   0,   0,   0, DS4,   0,
   D4,   0, DS4,   0,  E4,   0,  F4,   0,   0,   0,  C4,   0,   0,   0,  D4,
    0,   0,   0, DS4,   0,   0,   0,  F4,   0,   0,   0,  F4,   0,   0, DS4,
   D4,   0, AS3,   0,  C4,   0,  D4,   0, DS4,   0,   0,   0,   0,   0,  F4,
    0,  G4,   0,   0,   0, DS4,   0,   0,   0,  D4,   0, DS4,   0,  F4,   0,
  AS3,  C4,  D4,   0, DS4,   0,  F4,   0,  G4,   0, GS4,   0,   0,   0,  F4,
    0,   0,   0,  G4,   0,   0,   0, DS4,   0,   0,   0,  F4,   0,   0,   0,
  AS3,   0,   0,  C4, AS3,   0, GS3,   0,  G3,   0, GS3,   0, AS3,   0,   0,
    0, AS3,   0,  C4,   0, AS3,   0, GS3,   0,  G3,   0, GS3,   0, AS3,   0,
    0,   0, AS4,   0,   0,   0, AS4,   0,   0,   0, GS4,   0,   0,   0,  G4,
    0, AS3, AS3,  A3,   0, AS3,   0,  F4,   0, AS3, AS3,  A3,   0, AS3,   0,
  DS4,   0, AS3,   0,  G3,   0, AS3,   0, DS3,   0,  C4,   0, CS3,   0,

  /* Start of "It's a Long Way to Tipperary". */
   D4,   0, DS4,   0,   0,   0, DS4,   0,   0,   0,   0,   0, DS4,   0,  F4,
    0,  G4,   0, GS4,   0,   0,   0,  C5,   0,   0,   0,   0,   0,   0,   0,
   C5,   0, AS4,   0, GS4,   0,   0,   0,  F4,   0,   0,   0,   0,   0,   0,
    0, GS4,   0,   0,   0, DS4,   0,   0,   0, DS4,   0,   0,  F4, DS4,   0,
  CS3,   0,  C4,   0, CS3,   0, DS4,   0,   0,   0, DS4,   0,   0,   0, DS4,
    0, DS4,   0,  F4,   0,  G4,   0, GS4,   0,   0,   0,  C5,   0,   0,   0,
    0,   0,  G4,   0, GS4,   0,  A4,   0, AS4,   0,   0,   0,  F4,   0,   0,
    0,  G4,   0,   0,   0, GS4,   0,   0,   0, AS4,   0,   0,   0, AS4,   0,
    0,  C5, AS4,   0, GS4,   0,  G4,   0,  F4,   0, DS4,   0,   0,   0, DS4,
    0,   0,   0, DS4,   0, DS4,   0,  F4,   0,  G4,   0, GS4,   0,   0,   0,
   C5,   0,   0,   0,   0,   0, GS4,   0, AS4,   0,  C5,   0, CS5,   0,   0,
    0,  F4,   0,   0,   0, GS4,   0,   0,   0, CS5,   0,   0,   0,  C5,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0, GS4,   0, AS4,   0,  C5,
    0,   0,   0,  C5,   0,   0,   0,  C5,   0, GS4,   0, AS4,   0, GS4,   0,
   F4,   0,   0,   0,   0,   0,   0,   0, DS4,   0,   0,   0, GS4,   0,   0,
    0,  C5,   0,   0,   0, GS4,   0,   0,   0,   0,   0,   0,   0, AS4,   0,
    0,   0, GS4,   0,   0,   0,   0,   0,   0,   0,   0,   0,

  END
};

/**
 * $F7C7: Low music channel notes (semitones).
 *
 * The notes for the low part sorted by popularity:
 *  49x C3
 *  41x G3
 *  39x AS2
 *  32x DS3 (highest note)
 *  30x D3
 *  28x GS2
 *  21x F3
 *  19x DS2 (lowest note)
 *  18x F2
 *  12x A3
 *   7x CS3
 *   7x G2
 *   6x E3
 *   3x A2
 *   2x GS3
 */
const uint8_t music_channel1_data[] =
{
  /* Start of The Great Escape theme. */
   F2,   0,  G2,   0,   0,  A2, AS2,   0,  D3,   0,  F2,   0,  D3,   0, AS2,
    0,  D3,   0,  F2,   0,  D3,   0, DS3,   0,  G3,   0, AS2,   0,  G3,   0,
  DS3,   0,  G3,   0, DS3,   0,  E3,   0,  F3,   0,  A3,   0,  C3,   0,  A3,
    0,  F3,   0,  A3,   0,  C3,   0,  A3,   0, AS2,   0,  D3,   0,   0,   0,
    0,   0,   0,   0,  F2,   0,  G2,   0,   0,  A2, AS2,   0,  D3,   0,  F2,
    0,  D3,   0, AS2,   0,  D3,   0,  F2,   0,  D3,   0, DS3,   0,  G3,   0,
  AS2,   0,  G3,   0, DS3,   0,  G3,   0, DS3,   0,  E3,   0,  F3,   0,  A3,
    0,  C3,   0,  A3,   0,  F3,   0,  A3,   0,  C3,   0,  A3,   0, AS2,   0,
   D3,   0,  F2,   0,  D3,   0, AS2,   0,  D3,   0,  F2,   0,  D3,   0,

  /* Start of "Pack Up Your Troubles in Your Old Kit Bag". */
  DS3,   0,  G3,   0, AS2,   0,  G3,   0, DS3,   0,  G3,   0, AS2,   0,  G3,
    0, DS3,   0,  G3,   0, AS2,   0,  G3,   0, DS3,   0, DS3,   0,  F3,   0,
   G3,   0, GS2,   0,  C3,   0, DS2,   0,  C3,   0, GS2,   0,  C3,   0, DS2,
    0,  C3,   0, DS3,   0,  G3,   0, AS2,   0,  G3,   0, DS3,   0, AS2,   0,
   C3,   0,  D3,   0, DS3,   0,  G3,   0, AS2,   0,  G3,   0, DS3,   0,  G3,
    0, AS2,   0,  G3,   0, DS3,   0,  G3,   0, AS2,   0,  G3,   0, DS3,   0,
  AS2,   0, DS3,   0,  E3,   0,  F3,   0,  A3,   0,  C3,   0,  A3,   0,  F3,
    0,  A3,   0,  C3,   0,  A3,   0, AS2,   0,  D3,   0,  F2,   0,  D3,   0,
  AS2,   0,  D3,   0,  F2,   0,  D3,   0, DS3,   0,  G3,   0, AS2,   0,  G3,
    0, DS3,   0,  G3,   0, AS2,   0, DS3,   0,  D3,   0,  F3,   0, AS2,   0,
   D3,   0,  F2,   0,  D3,   0,  F3,   0,  G3,   0, GS3,   0, DS3,   0,  C3,
    0, GS3,   0,  G3,   0, DS3,   0, AS2,   0,  G3,   0,  D3,   0,  F3,   0,
  AS2,   0,  D3,   0,  F2,   0,  D3,   0, AS2,   0,  D3,   0, DS3,   0,  G3,
    0, AS2,   0,  G3,   0, DS3,   0,  G3,   0, AS2,   0,  G3,   0, DS3,   0,
   G3,   0, AS2,   0,  G3,   0, DS3,   0,  G3,   0, AS2,   0,  G3,   0, DS3,
    0,  G3,   0, AS2,   0,  G3,   0,  D3,   0,  F3,   0, AS2,   0,  F3,   0,
  DS3,   0,  G3,   0, AS2,   0,  G3,   0, DS3,   0,   0,   0,   0,   0,

  /* Start of "It's a Long Way to Tipperary". */
    0,   0, GS2,   0,  C3,   0, DS2,   0,  C3,   0, GS2,   0,  C3,   0, DS2,
    0,  C3,   0, GS2,   0,  C3,   0, DS2,   0,  C3,   0, GS2,   0,  C3,   0,
  AS2,   0,  C3,   0, CS3,   0,  F3,   0, GS2,   0,  F3,   0, CS3,   0,  F3,
    0, GS2,   0,  F3,   0, GS2,   0,  C3,   0, DS2,   0,  C3,   0, GS2,   0,
  DS2,   0,  F2,   0,  G2,   0, GS2,   0,  C3,   0, DS2,   0,  C3,   0, GS2,
    0,  C3,   0, DS2,   0,  C3,   0, GS2,   0,  C3,   0, DS2,   0,  C3,   0,
  GS2,   0,  C3,   0, GS2,   0,  A2,   0, AS2,   0,  D3,   0,  F2,   0,  D3,
    0, AS2,   0,  D3,   0,  F2,   0,  D3,   0, DS3,   0,  G3,   0, AS2,   0,
   G3,   0, DS3,   0, DS2,   0,  F2,   0,  G2,   0, GS2,   0,  C3,   0, DS2,
    0,  C3,   0, GS2,   0,  C3,   0,  F2,   0,  C3,   0, GS2,   0,  C3,   0,
  DS2,   0,  C3,   0, GS2,   0, GS2,   0, AS2,   0,  C3,   0, CS3,   0,  F3,
    0, GS2,   0,  F3,   0, CS3,   0,  F3,   0, CS3,   0, CS3,   0,  C3,   0,
   E3,   0,  G2,   0,  E3,   0,  C3,   0,  E3,   0,  C3,   0,  G2,   0, GS2,
    0,  C3,   0, DS2,   0,  C3,   0, GS2,   0,  C3,   0, DS2,   0,  C3,   0,
  CS3,   0,  F3,   0, GS2,   0,  F3,   0, GS2,   0,  C3,   0, DS2,   0,  C3,
    0, GS2,   0,  C3,   0, DS2,   0,  C3,   0, AS2,   0, DS2,   0,  F2,   0,
   G2,   0, GS2,   0,  C3,   0, DS2,   0,  C3,   0, GS2,   0,

  END
};

/**
 * $FA48: Music tuning table.
 */
static const uint16_t music_tuning_table[76] =
{
  0xFEFE,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0218,
  0x01FA,
  0x01DE,
  0x01C3,
  0x01A9,
  0x0192,
  0x017B,
  0x0166,
  0x0152,
  0x013F,
  0x012D,
  0x011C,
  0x010C,
  0x00FD,
  0x00EF,
  0x00E1,
  0x00D5,
  0x00C9,
  0x00BE,
  0x00B3,
  0x00A9,
  0x009F,
  0x0096,
  0x008E,
  0x0086,
  0x007E,
  0x0077,
  0x0071,
  0x006A,
  0x0064,
  0x005F,
  0x0059,
  0x0054,
  0x0050,
  0x004B,
  0x0047,
  0x0043,
  0x003F,
  0x003C,
  0x0038,
  0x0035,
  0x0032,
  0x002F,
  0x002D,
  0x002A,
  0x0028,
  0x0026,
  0x0023,
  0x0021,
  0x0020,
  0x001E,
  0x001C,
  0x001B,
  0x0019,
  0x0018,
  0x0016,
  0x0015,
  0x0014,
  0x0013,
  0x0012,
  0x0011,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000,
  0x0000, // likely end of table
};

/* ----------------------------------------------------------------------- */

/**
 * $F52C: Get tuning.
 *
 * \param[in] A Index (never larger than 42?).
 * \param[in] L Speaker bit, to be reset.
 *
 * \returns (was DE)
 */
uint16_t get_tuning(uint8_t A, uint8_t *L)
{
  uint16_t BC;

  BC = music_tuning_table[A]; // FIXME: Original loads endian swapped...

#define INC_LO(x) \
do { x = (x & 0xFF00) | (((x & 0x00FF) + 0x0001) & 0x00FF); } while (0)
#define INC_HI(x) \
do { x = (x & 0x00FF) | (((x & 0xFF00) + 0x0100) & 0xFF00); } while (0)

  INC_LO(BC);
  INC_HI(BC);
  if ((BC & 0xFF00) == 0)
    INC_LO(BC);

  /* Reset the speaker bit. */
  *L = 0;

#undef INC_HI
#undef INC_LO

  return BC;
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et

