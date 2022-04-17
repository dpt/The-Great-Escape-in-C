/* Keyboard.c
 *
 * ZX Spectrum keyboard handling.
 *
 * Copyright (c) David Thomas, 2016-2020. <dave@davespace.co.uk>
 */

#include <assert.h>
#include <ctype.h>

#ifdef _WIN32
#include <intrin.h>
#include <windows.h>
#endif

#include "C99/Types.h"

#include "ZXSpectrum/Keyboard.h"

void zxkeyset_clear(zxkeyset_t *keystate)
{
  assert(keystate);

  keystate->bits[0] = keystate->bits[1] = 0;
}

void zxkeyset_assign(zxkeyset_t *keystate, zxkey_t index, int on_off)
{
  unsigned int *pbits;

  assert(keystate);
  assert(index >= 0 && index < zxkey__LIMIT);
  assert(on_off == 0 || on_off == 1);

  // store 20 bits in each word
  pbits = &keystate->bits[index / 20];
  index %= 20;

  *pbits &= ~(1U << index);
  *pbits |= on_off << index;
}

static __inline uint32_t my_clz(uint32_t value)
{
#if defined(_MSC_VER)
  DWORD leading_zero = 0;

  if (_BitScanReverse(&leading_zero, value))
    return 31 - leading_zero;
  else
    return 32;
#elif defined(__GNUC__)
  return value ? __builtin_clz(value) : 32;
#else
  int c;

  if (value == 0)
    return 32;

  c = 0;
  if ((value & 0xFFFF0000U) == 0) { c += 16; value <<= 16; }
  if ((value & 0xFF000000U) == 0) { c +=  8; value <<=  8; }
  if ((value & 0xF0000000U) == 0) { c +=  4; value <<=  4; }
  if ((value & 0xC0000000U) == 0) { c +=  2; value <<=  2; }
  if ((value & 0x80000000U) == 0) { c +=  1; value <<=  1; }

  return c;
#endif
}

int zxkeyset_for_port(uint16_t port, const zxkeyset_t *keystate)
{
  int nzeroes;

  nzeroes = my_clz((uint32_t) ~port << 16);
  return (~keystate->bits[nzeroes >> 2] >> ((nzeroes & 3) * 5)) & 0x1F;
}

/**
 * Convert the given character to a zxkey_t.
 */
static zxkey_t char_to_key(int c)
{
  switch (toupper(c))
  {
  case ' ':
    return zxkey_SPACE;
  case '/':
    return zxkey_SYMBOL_SHIFT;
  case 'M':
    return zxkey_M;
  case 'N':
    return zxkey_N;
  case 'B':
    return zxkey_B;

  case '\r':
  case '\n':
    return zxkey_ENTER;
  case 'L':
    return zxkey_L;
  case 'K':
    return zxkey_K;
  case 'J':
    return zxkey_J;
  case 'H':
    return zxkey_H;

  case 'P':
    return zxkey_P;
  case 'O':
    return zxkey_O;
  case 'I':
    return zxkey_I;
  case 'U':
    return zxkey_U;
  case 'Y':
    return zxkey_Y;

  case '0':
    return zxkey_0;
  case '9':
    return zxkey_9;
  case '8':
    return zxkey_8;
  case '7':
    return zxkey_7;
  case '6':
    return zxkey_6;

  case '1':
    return zxkey_1;
  case '2':
    return zxkey_2;
  case '3':
    return zxkey_3;
  case '4':
    return zxkey_4;
  case '5':
    return zxkey_5;

  case 'Q':
    return zxkey_Q;
  case 'W':
    return zxkey_W;
  case 'E':
    return zxkey_E;
  case 'R':
    return zxkey_R;
  case 'T':
    return zxkey_T;

  case 'A':
    return zxkey_A;
  case 'S':
    return zxkey_S;
  case 'D':
    return zxkey_D;
  case 'F':
    return zxkey_F;
  case 'G':
    return zxkey_G;

  case '\\':
    return zxkey_CAPS_SHIFT;
  case 'Z':
    return zxkey_Z;
  case 'X':
    return zxkey_X;
  case 'C':
    return zxkey_C;
  case 'V':
    return zxkey_V;

  default:
    return zxkey_UNKNOWN;
  }
}

void zxkeyset_setchar(zxkeyset_t *keystate, int c)
{
  zxkey_t index = char_to_key(c);
  if (index != zxkey_UNKNOWN)
    zxkeyset_assign(keystate, index, 1);
}

void zxkeyset_clearchar(zxkeyset_t *keystate, int c)
{
  zxkey_t index = char_to_key(c);
  if (index != zxkey_UNKNOWN)
    zxkeyset_assign(keystate, index, 0);
}

// vim: ts=8 sts=2 sw=2 et
