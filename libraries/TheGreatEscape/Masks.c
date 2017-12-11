#include <stdint.h>

#include "TheGreatEscape/Masks.h"
#include "TheGreatEscape/Types.h"

/* Mask encoding: First byte is the width.
 *
 * A top-bit-set byte indicates a repetition, the count of
 * which is in the bottom seven bits. The subsequent byte is the value to
 * repeat.
 *
 * The repetition count is +1, so <0x83> <0x01> expands to <0x01> <0x01>.
 */

/* Spacers for laying out tables. */
#define _____ /* 0x00 - transparent tile */
#define XXXXX /* 0x01 - opaque tile */

/* $E55F */
static const uint8_t exterior_mask_0[] =
{
  0x2A,
  0xA0, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x07, 0x08, 0x09, 0x01, 0x0A, 0xA2, 0x00, _____ _____
  _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x85, 0x01, XXXXX XXXXX XXXXX 0x0B, 0x9F, 0x00, _____
  _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x88, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x0C, 0x9C, 0x00,
  _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x8A, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x0D, 0x0E, 0x99,
  0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x8D, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x0F, 0x10,
  0x96, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x90, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x11,
  0x94, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x92, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x92, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x94, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x90, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x96, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x8E, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x8C, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x9A, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x8A, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x9C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x88, 0x00, _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x9E, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x18,
  0x86, 0x00, _____ _____ _____ _____ 0x05, 0x06, 0x04, 0xA1, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x84, 0x00, _____ _____ 0x05, 0x06, 0x04, 0xA3, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x00, 0x00, 0x05, 0x06, 0x04, 0xA5, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x05, 0x03, 0x04, 0xA7, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0xA9, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0xA9, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0xA9, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0xA9, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0xA9, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0xA9, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0xA9, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0xA9, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0xA9, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
};

/* $E5FF */
static const uint8_t exterior_mask_1[] =
{
  0x12,
  0x02, 0x91, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x91, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x91, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x91, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x91, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x91, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x91, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x91, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x91, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x91, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
};

/* $E61E */
static const uint8_t exterior_mask_2[] =
{
  0x10,
  0x13, 0x14, 0x15, 0x8D, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
  0x16, 0x17, 0x18, 0x17, 0x15, 0x8B, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____
  0x19, 0x1A, 0x1B, 0x17, 0x18, 0x17, 0x15, 0x89, 0x00, _____ _____ _____ _____ _____ _____ _____
  0x19, 0x1A, 0x1C, 0x1A, 0x1B, 0x17, 0x18, 0x17, 0x15, 0x87, 0x00, _____ _____ _____ _____ _____
  0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1B, 0x17, 0x13, 0x14, 0x15, 0x85, 0x00, _____ _____ _____
  0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x16, 0x17, 0x18, 0x17, 0x15, 0x83, _____ _____
  0x00, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1B, 0x17, 0x18, 0x17, 0x15,
  0x00, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1A, 0x1B, 0x17, 0x18,
  0x17, 0x00, 0x20, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1B,
  0x17, 0x83, 0x00, _____ 0x20, 0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C,
  0x1D, 0x85, 0x00, _____ _____ _____ 0x20, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C,
  0x1D, 0x87, 0x00, _____ _____ _____ _____ _____ 0x1F, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C,
  0x1D, 0x89, 0x00, _____ _____ _____ _____ _____ _____ _____ 0x20, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C,
  0x1D, 0x8B, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x20, 0x1C, 0x1A, 0x1C,
  0x1D, 0x8D, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x20, 0x1C,
  0x1D, 0x8F, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x1F
};

/* $E6CA */
static const uint8_t exterior_mask_3[] =
{
  0x1A,
  0x88, 0x00, _____ _____ _____ _____ _____ _____ 0x05, 0x4C, 0x90, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
  0x86, 0x00, _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x32, 0x30, 0x4C, 0x8E, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
  0x84, 0x00, _____ _____ 0x05, 0x06, 0x04, 0x84, 0x01, XXXXX XXXXX 0x32, 0x30, 0x4C, 0x8C, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
  0x00, 0x00, 0x05, 0x06, 0x04, 0x88, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x32, 0x30, 0x4C, 0x8A, 0x00, _____ _____ _____ _____ _____ _____ _____ _____
  0x00, 0x06, 0x04, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x32, 0x30, 0x4C, 0x88, 0x00, _____ _____ _____ _____ _____ _____
  0x02, 0x90, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x32, 0x30, 0x4C, 0x86, 0x00, 0x02, _____ _____ _____
  0x92, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x32, 0x30, 0x4C, 0x84, 0x00, _____ _____
  0x02, 0x94, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x32, 0x30, 0x4C, 0x00, 0x00,
  0x02, 0x96, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x32, 0x30, 0x00,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x98, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12
};

/* $E74B */
static const uint8_t exterior_mask_4[] =
{
  0x0D,
  0x02, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x8C, 0x01  XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
};

/* $E758 */
static const uint8_t exterior_mask_5[] =
{
  0x0E,
  0x02, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x8C, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x02, 0x8D, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x02, 0x8D, 0x01  XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
};

/* $E77F */
static const uint8_t exterior_mask_6[] =
{
  0x08,
  0x5B, 0x5A, 0x86, 0x00, _____ _____ _____ _____
  0x01, 0x01, 0x5B, 0x5A, 0x84, 0x00, _____ _____
  0x84, 0x01, XXXXX XXXXX 0x5B, 0x5A, 0x00, 0x00,
  0x86, 0x01, XXXXX XXXXX XXXXX XXXXX 0x5B, 0x5A,
  0xD8, 0x01  XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
};

/* $E796 */
static const uint8_t exterior_mask_7[] =
{
  0x09,
  0x88, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x88, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x88, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x88, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x88, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x88, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x88, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12,
  0x88, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX 0x12
};

/* $E7AF */
static const uint8_t exterior_mask_8[] =
{
  0x10,
  0x8D, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x23, 0x24, 0x25,
  0x8B, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x23, 0x26, 0x27, 0x26, 0x28,
  0x89, 0x00, _____ _____ _____ _____ _____ _____ _____ 0x23, 0x26, 0x27, 0x26, 0x22, 0x29, 0x2A,
  0x87, 0x00, _____ _____ _____ _____ _____ 0x23, 0x26, 0x27, 0x26, 0x22, 0x29, 0x2B, 0x29, 0x2A,
  0x85, 0x00, _____ _____ _____ 0x23, 0x24, 0x25, 0x26, 0x22, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A,
  0x83, 0x00, 0x23, 0x26, 0x27, 0x26, 0x28, 0x2F, 0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x00,
  0x23, 0x26, 0x27, 0x26, 0x22, 0x29, 0x2A, 0x2F, 0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x26,
  0x27, 0x26, 0x22, 0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x26,
  0x22, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x31, 0x2D, 0x2F,
  0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x29, 0x2B, 0x31, 0x83, 0x00, _____ 0x2F,
  0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x31, 0x85, 0x00, _____ _____ _____ 0x2F,
  0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x2E, 0x87, 0x00, _____ _____ _____ _____ _____ 0x2F,
  0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x31, 0x2D, 0x88, 0x00, _____ _____ _____ _____ _____ _____ 0x2F,
  0x2B, 0x29, 0x2B, 0x31, 0x8B, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x2F,
  0x2B, 0x31, 0x8D, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
  0x2E, 0x8F, 0x00  _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
};

/* $E85C */
static const uint8_t exterior_mask_9[] =
{
  0x0A,
  0x83, 0x00, _____ 0x05, 0x06, 0x30, 0x4C, 0x83, 0x00, _____
  0x00, 0x05, 0x06, 0x04, 0x01, 0x01, 0x32, 0x30, 0x4C, 0x00,
  0x34, 0x04, 0x86, 0x01, XXXXX XXXXX XXXXX XXXXX 0x32, 0x33,
  0x83, 0x00, _____ 0x40, 0x01, 0x01, 0x3F, 0x83, 0x00, _____
  0x02, 0x46, 0x47, 0x48, 0x49, 0x42, 0x41, 0x45, 0x44, 0x12,
  0x34, 0x01, 0x01, 0x46, 0x4B, 0x43, 0x44, 0x01, 0x01, 0x33,
  0x00, 0x3C, 0x3E, 0x40, 0x01, 0x01, 0x3F, 0x37, 0x39, 0x00,
  0x83, 0x00, _____ 0x3D, 0x3A, 0x3B, 0x38, 0x83, 0x00  _____
};

/* $E8A3 */
static const uint8_t exterior_mask_10[] =
{
  0x08,
  0x35, 0x86, 0x01, XXXXX XXXXX XXXXX XXXXX 0x36,
  0x90, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x88, 0x00, _____ _____ _____ _____ _____ _____
  0x3C, 0x86, 0x00, _____ _____ _____ _____ 0x39,
  0x3C, 0x00, 0x02, 0x36, 0x35, 0x12, 0x00, 0x39,
  0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
  0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
  0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
  0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
  0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
  0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
  0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39
};

/* $E8F0 */
static const uint8_t exterior_mask_11[] =
{
  0x08,
  0x01, 0x4F, 0x86, 0x00, _____ _____ _____ _____
  0x01, 0x50, 0x01, 0x4F, 0x84, 0x00, _____ _____
  0x01, 0x00, 0x00, 0x51, 0x01, 0x4F, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x53, 0x19, 0x50, 0x01, 0x4F,
  0x01, 0x00, 0x00, 0x53, 0x19, 0x00, 0x00, 0x52,
  0x01, 0x00, 0x00, 0x53, 0x19, 0x00, 0x00, 0x52,
  0x01, 0x54, 0x00, 0x53, 0x19, 0x00, 0x00, 0x52,
  0x83, 0x00, _____ 0x55, 0x19, 0x00, 0x00, 0x52,
  0x85, 0x00, _____ _____ _____ 0x54, 0x00, 0x52
};

/* $E92F */
static const uint8_t exterior_mask_12[] =
{
  0x02,
  0x56, 0x57,
  0x56, 0x57,
  0x58, 0x59,
  0x58, 0x59,
  0x58, 0x59,
  0x58, 0x59,
  0x58, 0x59,
  0x58, 0x59
};

/* $E940 */
static const uint8_t exterior_mask_13[] =
{
  0x05,
  0x00, 0x00, 0x23, 0x24, 0x25,
  0x02, 0x00, 0x27, 0x26, 0x28,
  0x02, 0x00, 0x22, 0x26, 0x28,
  0x02, 0x00, 0x2B, 0x29, 0x2A,
  0x02, 0x00, 0x2B, 0x29, 0x2A,
  0x02, 0x00, 0x2B, 0x29, 0x2A,
  0x02, 0x00, 0x2B, 0x29, 0x2A,
  0x02, 0x00, 0x2B, 0x29, 0x2A,
  0x02, 0x00, 0x2B, 0x31, 0x00,
  0x02, 0x00, 0x83, 0x00  _____
};

/* $E972 */
static const uint8_t exterior_mask_14[] =
{
  0x04,
  0x19, 0x83, 0x00, _____
  0x19, 0x17, 0x15, 0x00,
  0x19, 0x17, 0x18, 0x17,
  0x19, 0x1A, 0x1B, 0x17,
  0x19, 0x1A, 0x1C, 0x1D,
  0x19, 0x1A, 0x1C, 0x1D,
  0x19, 0x1A, 0x1C, 0x1D,
  0x19, 0x1A, 0x1C, 0x1D,
  0x19, 0x1A, 0x1C, 0x1D,
  0x00, 0x20, 0x1C, 0x1D
};

/* $E99A */
static const uint8_t interior_mask_15[] =
{
  0x02,
  0x04, 0x32,
  0x01, 0x01
};

/* $E99F */
static const uint8_t interior_mask_16[] =
{
  0x09,
  0x86, 0x00, _____ _____ _____ _____ 0x5D, 0x5C, 0x54,
  0x84, 0x00, _____ _____ 0x5D, 0x5C, 0x01, 0x01, 0x01,
  0x00, 0x00, 0x5D, 0x5C, 0x85, 0x01, XXXXX XXXXX XXXXX
  0x5D, 0x5C, 0x87, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX
  0x2B, 0x88, 0x01  XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
};

/* $E9B9 */
static const uint8_t interior_mask_17[] =
{
  0x05,
  0x00, 0x00, 0x5D, 0x5C, 0x67,
  0x5D, 0x5C, 0x83, 0x01, XXXXX
  0x3C, 0x84, 0x01  XXXXX XXXXX
};

/* $E9C6 */
static const uint8_t interior_mask_18[] =
{
  0x02,
  0x5D, 0x68,
  0x3C, 0x69
};

/* $E9CB */
static const uint8_t interior_mask_19[] =
{
  0x0A,
  0x86, 0x00, _____ _____ _____ _____ 0x5D, 0x5C, 0x46, 0x47,
  0x84, 0x00, _____ _____ 0x5D, 0x5C, 0x83, 0x01, XXXXX 0x39,
  0x00, 0x00, 0x5D, 0x5C, 0x86, 0x01, XXXXX XXXXX XXXXX XXXXX
  0x5D, 0x5C, 0x88, 0x01, XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
  0x4A, 0x89, 0x01  XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
};

/* $E9E6 */
static const uint8_t interior_mask_20[] =
{
  0x06,
  0x5D, 0x5C, 0x01, 0x47, 0x6A, 0x00,
  0x4A, 0x84, 0x01, XXXXX XXXXX 0x6B,
  0x00, 0x84, 0x01, XXXXX XXXXX 0x5F
};

/* $E9F5 */
static const uint8_t interior_mask_21[] =
{
  0x04,
  0x05, 0x4C, 0x00, 0x00,
  0x61, 0x65, 0x66, 0x4C,
  0x61, 0x12, 0x02, 0x60,
  0x61, 0x12, 0x02, 0x60,
  0x61, 0x12, 0x02, 0x60,
  0x61, 0x12, 0x02, 0x60
};

/* $EA0E */
static const uint8_t interior_mask_22[] =
{
  0x04,
  0x00, 0x00, 0x05, 0x4C,
  0x05, 0x63, 0x64, 0x60,
  0x61, 0x12, 0x02, 0x60,
  0x61, 0x12, 0x02, 0x60,
  0x61, 0x12, 0x02, 0x60,
  0x61, 0x12, 0x02, 0x60,
  0x61, 0x12, 0x62, 0x00
};

/* $EA2B */
static const uint8_t interior_mask_23[] =
{
  0x03,
  0x00, 0x6C, 0x00,
  0x02, 0x01, 0x68,
  0x02, 0x01, 0x69
};

/* $EA35 */
static const uint8_t interior_mask_24[] =
{
  0x05,
  0x01, 0x5E, 0x4C, 0x00, 0x00,
  0x01, 0x01, 0x32, 0x30, 0x00,
  0x84, 0x01, XXXXX XXXXX 0x5F
};

/* $EA43 */
static const uint8_t interior_mask_25[] =
{
  0x02,
  0x6E, 0x5A,
  0x6D, 0x39,
  0x3C, 0x39
};

/* $EA4A */
static const uint8_t interior_mask_26[] =
{
  0x04,
  0x5D, 0x5C, 0x46, 0x47,
  0x4A, 0x01, 0x01, 0x39
};

/* $EA53 */
static const uint8_t interior_mask_27[] =
{
  0x03,
  0x2C, 0x47, 0x00,
  0x00, 0x61, 0x12,
  0x00, 0x61, 0x12
};

/* $EA5D */
static const uint8_t interior_mask_28[] =
{
  0x03,
  0x00, 0x45, 0x1E,
  0x02, 0x60, 0x00,
  0x02, 0x60, 0x00
};

/* $EA67 */
static const uint8_t interior_mask_29[] =
{
  0x05,
  0x45, 0x1E, 0x2C, 0x47, 0x00,
  0x2C, 0x47, 0x45, 0x1E, 0x12,
  0x00, 0x61, 0x12, 0x61, 0x12,
  0x00, 0x61, 0x5F, 0x00, 0x00
};

/**
 * $EBC5: Pointers to run-length encoded mask data.
 *
 * The first half is outdoor masks, the second is indoor masks.
 */
const uint8_t *mask_pointers[30] =
{
  &exterior_mask_0[0],  /* $E55F */
  &exterior_mask_1[0],  /* $E5FF */
  &exterior_mask_2[0],  /* $E61E */
  &exterior_mask_3[0],  /* $E6CA */
  &exterior_mask_4[0],  /* $E74B */
  &exterior_mask_5[0],  /* $E758 */
  &exterior_mask_6[0],  /* $E77F */
  &exterior_mask_7[0],  /* $E796 */
  &exterior_mask_8[0],  /* $E7AF */
  &exterior_mask_9[0],  /* $E85C */
  &exterior_mask_10[0], /* $E8A3 */
  &exterior_mask_11[0], /* $E8F0 */
  &exterior_mask_13[0], /* $E940 */
  &exterior_mask_14[0], /* $E972 */
  &exterior_mask_12[0], /* $E92F */

  &interior_mask_29[0], /* $EA67 */
  &interior_mask_27[0], /* $EA53 */
  &interior_mask_28[0], /* $EA5D */
  &interior_mask_15[0], /* $E99A */
  &interior_mask_16[0], /* $E99F */
  &interior_mask_17[0], /* $E9B9 */
  &interior_mask_18[0], /* $E9C6 */
  &interior_mask_19[0], /* $E9CB */
  &interior_mask_20[0], /* $E9E6 */
  &interior_mask_21[0], /* $E9F5 */
  &interior_mask_22[0], /* $EA0E */
  &interior_mask_23[0], /* $EA2B */
  &interior_mask_24[0], /* $EA35 */
  &interior_mask_25[0], /* $EA43 */
  &interior_mask_26[0]  /* $EA4A */
};

/**
 * $EC01: mask_t structs for the exterior scene.
 */
const mask_t exterior_mask_data[58] =
{
  /* index, bounds, pos */
  {  0, {  71, 112,  39,  63 }, { 106,  82, 12 } }, // hut
  {  0, {  95, 136,  51,  75 }, {  94,  82, 12 } }, // hut
  {  0, { 119, 160,  63,  87 }, {  82,  82, 12 } }, // hut
  {  1, { 159, 176,  40,  49 }, {  62, 106, 60 } }, // square
  {  1, { 159, 176,  50,  59 }, {  62, 106, 60 } }, // square
  {  2, {  64,  79,  76,  91 }, {  70,  70,  8 } }, // fence left
  {  2, {  80,  95,  84,  99 }, {  70,  70,  8 } }, // fence left
  {  2, {  96, 111,  92, 107 }, {  70,  70,  8 } }, // fence left
  {  2, { 112, 127, 100, 115 }, {  70,  70,  8 } }, // fence left
  {  2, {  48,  63,  84,  99 }, {  62,  62,  8 } }, // fence left
  {  2, {  64,  79,  92, 107 }, {  62,  62,  8 } }, // fence left
  {  2, {  80,  95, 100, 115 }, {  62,  62,  8 } }, // fence left
  {  2, {  96, 111, 108, 123 }, {  62,  62,  8 } }, // fence left
  {  2, { 112, 127, 116, 131 }, {  62,  62,  8 } }, // fence left
  {  2, {  16,  31, 100, 115 }, {  74,  46,  8 } }, // fence left
  {  2, {  32,  47, 108, 123 }, {  74,  46,  8 } }, // fence left
  {  2, {  48,  63, 116, 131 }, {  74,  46,  8 } }, // fence left
  {  3, {  43,  68,  51,  71 }, { 103,  69, 18 } }, // main gate
  {  4, {  43,  55,  72,  75 }, { 109,  69,  8 } }, // square 2
  {  5, {  55,  68,  72,  81 }, { 103,  69,  8 } }, // square 3
  {  6, {   8,  15,  42,  60 }, { 110,  70, 10 } }, // wall
  {  6, {  16,  23,  46,  64 }, { 110,  70, 10 } }, // wall
  {  6, {  24,  31,  50,  68 }, { 110,  70, 10 } }, // wall
  {  6, {  32,  39,  54,  72 }, { 110,  70, 10 } }, // wall
  {  6, {  40,  47,  58,  76 }, { 110,  70, 10 } }, // wall
  {  7, {   8,  16,  31,  38 }, { 130,  70, 18 } }, // square 4
  {  7, {   8,  16,  39,  45 }, { 130,  70, 18 } }, // square 4
  {  8, { 128, 143, 100, 115 }, {  70,  70,  8 } }, // fence right
  {  8, { 144, 159,  92, 107 }, {  70,  70,  8 } }, // fence right
  {  8, { 160, 176,  84,  99 }, {  70,  70,  8 } }, // fence right
  {  8, { 176, 191,  76,  91 }, {  70,  70,  8 } }, // fence right
  {  8, { 192, 207,  68,  83 }, {  70,  70,  8 } }, // fence right
  {  8, { 128, 143, 116, 131 }, {  62,  62,  8 } }, // fence right
  {  8, { 144, 159, 108, 123 }, {  62,  62,  8 } }, // fence right
  {  8, { 160, 176, 100, 115 }, {  62,  62,  8 } }, // fence right
  {  8, { 176, 191,  92, 107 }, {  62,  62,  8 } }, // fence right
  {  8, { 192, 207,  84,  99 }, {  62,  62,  8 } }, // fence right
  {  8, { 208, 223,  76,  91 }, {  62,  62,  8 } }, // fence right
  {  8, {  64,  79, 116, 131 }, {  78,  46,  8 } }, // fence right
  {  8, {  80,  95, 108, 123 }, {  78,  46,  8 } }, // fence right
  {  8, {  16,  31,  88, 103 }, { 104,  46,  8 } }, // fence right
  {  8, {  32,  47,  80,  95 }, { 104,  46,  8 } }, // fence right
  {  8, {  48,  63,  72,  87 }, { 104,  46,  8 } }, // fence right
  {  9, {  27,  36,  78,  85 }, { 104,  55, 15 } }, // watchtower top
  { 10, {  28,  35,  81,  93 }, { 104,  56, 10 } }, // watchtower bottom
  {  9, {  59,  68, 114, 121 }, {  78,  45, 15 } }, // watchtower top
  { 10, {  60,  67, 117, 129 }, {  78,  46, 10 } }, // watchtower bottom
  {  9, { 123, 132,  98, 105 }, {  70,  69, 15 } }, // watchtower top
  { 10, { 124, 131, 101, 113 }, {  70,  70, 10 } }, // watchtower bottom
  {  9, { 171, 180,  74,  81 }, {  70,  93, 15 } }, // watchtower top
  { 10, { 172, 179,  77,  89 }, {  70,  94, 10 } }, // watchtower bottom
  { 11, {  88,  95,  90,  98 }, {  70,  70,  8 } }, // exercise yard gate
  { 11, {  72,  79,  98, 106 }, {  62,  62,  8 } }, // exercise yard gate
  { 12, {  11,  15,  96, 103 }, { 104,  46,  8 } }, // fence end piece 12
  { 13, {  12,  15,  97, 106 }, {  78,  46,  8 } }, // fence end piece 13
  { 14, { 127, 128, 124, 131 }, {  62,  62,  8 } }, // fence corner piece
  { 13, {  44,  47,  81,  90 }, {  62,  62,  8 } }, // fence end piece 13
  { 13, {  60,  63,  73,  82 }, {  70,  70,  8 } }, // fence end piece 13
};
