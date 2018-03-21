/**
 * Music.c
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

#include "TheGreatEscape/Music.h"

/* The menu screen music is a medley of:
 * - The Great Escape theme [Elmer Bernstein, 1963]
 * - Pack Up Your Troubles in Your Old Kit Bag (and Smile, Smile, Smile) [1915]
 * - It's a Long Way to Tipperary [1912]
 */

/* Define symbols for all semitones which the music routine can produce. */

/* Octave 2 */
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

/* Octave 3 */
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

/* Octave 4 */
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

/* Octave 5 */
#define C5   41
#define CS5  42
#define D5   43
#define DS5  44
#define E5   45
#define F5   46
#define FS5  47
#define G5   48
#define GS5  49

/* Octave 6 and above can be produced but the music data doesn't use them,
 * so we don't define symbols for them here. */

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
 * $FA48: The frequency to use to produce the given semitone.
 *
 * Covers full octaves from C2 to B6 and C7 only.
 *
 * Used by frequency_for_semitone only.
 */
static const uint16_t semitone_to_frequency[] =
{
  0xFEFE, /* delay */
  0x0000, /* unused */
  0x0000, /* unused */
  0x0000, /* unused */
  0x0000, /* unused */

  /* Octave 2 */
  0x0218, /* C2  - C2 */
  0x01FA, /* CS2 - C2# */
  0x01DE, /* D2  - D2 */
  0x01C3, /* DS2 - D2# -- the lowest note used by the game music */
  0x01A9, /* E2  - E2 */
  0x0192, /* F2  - F2 */
  0x017B, /* FS2 - F2# */
  0x0166, /* G2  - G2 */
  0x0152, /* GS2 - G2# */
  0x013F, /* A2  - A2 */
  0x012D, /* AS2 - A2# */
  0x011C, /* B2  - B2 */

  /* Octave 3 */
  0x010C, /* C3  - C3 */
  0x00FD, /* CS3 - C3# */
  0x00EF, /* D3  - D3 */
  0x00E1, /* DS3 - D3# */
  0x00D5, /* E3  - E3 */
  0x00C9, /* F3  - F3 */
  0x00BE, /* FS3 - F3# */
  0x00B3, /* G3  - G3 */
  0x00A9, /* GS3 - G3# */
  0x009F, /* A3  - A3 */
  0x0096, /* AS3 - A3# */
  0x008E, /* B3  - B3 */

  /* Octave 4 */
  0x0086, /* C4  - C4 -- middle C */
  0x007E, /* CS4 - C4# */
  0x0077, /* D4  - D4 */
  0x0071, /* DS4 - D4# */
  0x006A, /* E4  - E4 */
  0x0064, /* F4  - F4 */
  0x005F, /* FS4 - F4# */
  0x0059, /* G4  - G4 */
  0x0054, /* GS4 - G4# */
  0x0050, /* A4  - A4 */
  0x004B, /* AS4 - A4# */
  0x0047, /* B4  - B4 */

  /* Octave 5 */
  0x0043, /* C5  - C5 */
  0x003F, /* CS5 - C5# -- the highest note used by the game music */
  0x003C, /* D5  - D5 */
  0x0038, /* DS5 - D5# */
  0x0035, /* E5  - E5 */
  0x0032, /* F5  - F5 */
  0x002F, /* FS5 - F5# */
  0x002D, /* G5  - G5 */
  0x002A, /* GS5 - G5# */
  0x0028, /* A5  - A5 */
  0x0026, /* AS5 - A5# */
  0x0023, /* B5  - B5 */

  /* Octave 6 */
  0x0021, /* C6  - C6 */
  0x0020, /* CS6 - C6# */
  0x001E, /* D6  - D6 */
  0x001C, /* DS6 - D6# */
  0x001B, /* E6  - E6 */
  0x0019, /* F6  - F6 */
  0x0018, /* FS6 - F6# */
  0x0016, /* G6  - G6 */
  0x0015, /* GS6 - G6# */
  0x0014, /* A6  - A6 */
  0x0013, /* AS6 - A6# */
  0x0012, /* B6  - B6 */

  /* Octave 7 */
  0x0011, /* C7  - C7 */

  /* Conv: It's unclear from the original game data where the table is
   *       supposed to end. It's not important in practice since the highest
   *       note used by the game is C5#, but for reference I've deleted all
   *       of the trailing zero entries. */
};

/**
 * $F52C: Return the frequency to play at to generate the given semitone.
 *
 * The frequency value returned is the number of iterations that the music
 * routine should count for before flipping its output bit.
 *
 * \param[in]  semitone Semitone index (never larger than 42 in practice,
 *                      but the table is larger). (was A)
 * \param[out] beep     Beeper bit (always reset to zero). (was L)
 *
 * \returns Frequency. (was DE)
 */
uint16_t frequency_for_semitone(uint8_t semitone, uint8_t *beep)
{
  // FUTURE: Embed the semitone_to_frequency table in here.
  
  uint16_t frequency; /* was BC */
  
  // Middle C on the piano keyboard is C4 at 261.625565Hz (C4 is the scientific name).
  // A4 is 220Hz (scientific)
  // I'm measuring the output from this code here as:
  // C3 generates ~140Hz (probably ought to be C4 @ 131Hz)
  // C4 generates ~275Hz (probably ought to be C5 @ 262Hz)
  
#if 0
  frequency = semitone_to_frequency[semitone];
  // Original loads endian swapped... no idea why.
  frequency = ((frequency & 0xFF00) >> 8) | ((frequency & 0x00FF) << 8);
  
  // Increment the value by 0x0101.
  // But why not just bake it into the table? Is it a frequency adjustment factor?
  
#define INC_LO(x) \
do { x = (x & 0xFF00) | (((x & 0x00FF) + 0x0001) & 0x00FF); } while (0)
#define INC_HI(x) \
do { x = (x & 0x00FF) | (((x & 0xFF00) + 0x0100) & 0xFF00); } while (0)
  
  INC_LO(frequency);
  INC_HI(frequency);
  if ((frequency & 0xFF00) == 0) // if 'hi' part rolled over
    INC_LO(frequency);
  
#undef INC_HI
#undef INC_LO
#else
  // Conv: Alternative which avoids some of the big endian swapping nonsense.
  frequency = semitone_to_frequency[semitone];
  
  /* This value pre-increments the loop counters used in the music routine so
   * that they count down correctly. It could have been baked into the
   * semitone_to_frequency table. */
  frequency += 0x0101;
  
  frequency = ((frequency & 0xFF00) >> 8) | ((frequency & 0x00FF) << 8);
#endif
  
  *beep = 0; /* Zero the beeper bit. */
  
  return frequency;
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et

