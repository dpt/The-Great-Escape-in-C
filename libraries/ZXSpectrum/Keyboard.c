/* ZXKeyboard.c
 *
 * ZX Spectrum keyboard handling.
 *
 * Copyright (c) David Thomas, 2016. <dave@davespace.co.uk>
 */

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "ZXSpectrum/Keyboard.h"

void zxkeyset_assign(zxkeyset_t *keystate,
                     zxkey_t     index,
                     bool        on_off)
{
  assert(index < zxkey__LIMIT);

  *keystate = (*keystate & (~1ULL << index)) | ((unsigned long long) on_off << index);
}

static uint32_t __inline my_clz(uint32_t value)
{
#ifdef _WIN32
	DWORD leading_zero = 0;

	if (_BitScanReverse(&leading_zero, value))
		return 31 - leading_zero;
	else
		return 32;
#else
	return value ? __builtin_clz(value) : 32;
#endif
}

int zxkeyset_for_port(uint16_t port, zxkeyset_t keystate)
{
  int nzeroes;

  nzeroes = my_clz(~port << 16);
  return (~keystate >> nzeroes * 5) & 0x1F;
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
//  case XXX:
//    return zxkey_SYMBOL_SHIFT;
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

//  case XXX:
//    return zxkey_CAPS_SHIFT;
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

zxkeyset_t zxkeyset_setchar(zxkeyset_t keystate, int c)
{
  zxkey_t sk = char_to_key(c);
  if (sk != zxkey_UNKNOWN)
    keystate |= 1ULL << sk;
  return keystate;
}

zxkeyset_t zxkeyset_clearchar(zxkeyset_t keystate, int c)
{
  zxkey_t sk = char_to_key(c);
  if (sk != zxkey_UNKNOWN)
    keystate &= ~(1ULL << sk);
  return keystate;
}

