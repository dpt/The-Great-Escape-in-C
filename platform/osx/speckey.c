//
//  speckey.c
//  TheGreatEscape
//
//  Created by David Thomas on 04/07/2016.
//  Copyright © 2016 David Thomas. All rights reserved.
//

#include <ctype.h>
#include <stdbool.h>

#include "speckey.h"

speckeyfield_t assign_speckey(speckeyfield_t keystate,
                              speckey_t      index,
                              bool           on_off)
{
  return (keystate & (~1ULL << index)) | ((unsigned long long) on_off << index);
}

int port_to_keyfield(uint16_t port, speckeyfield_t keystate)
{
  int clz = __builtin_clz(~port << 16);
  return (~keystate >> clz * 5) & 0x1F;
}

/**
 * Convert the given character to a speckey_t.
 */
static speckey_t char_to_speckey(int c)
{
  switch (toupper(c))
  {
    case ' ':
      return speckey_SPACE;
//  case XXX:
//    return speckey_SYMBOL_SHIFT;
    case 'M':
      return speckey_M;
    case 'N':
      return speckey_N;
    case 'B':
      return speckey_B;

    case '\r':
    case '\n':
      return speckey_ENTER;
    case 'L':
      return speckey_L;
    case 'K':
      return speckey_K;
    case 'J':
      return speckey_J;
    case 'H':
      return speckey_H;

    case 'P':
      return speckey_P;
    case 'O':
      return speckey_O;
    case 'I':
      return speckey_I;
    case 'U':
      return speckey_U;
    case 'Y':
      return speckey_Y;

    case '0':
      return speckey_0;
    case '9':
      return speckey_9;
    case '8':
      return speckey_8;
    case '7':
      return speckey_7;
    case '6':
      return speckey_6;

    case '1':
      return speckey_1;
    case '2':
      return speckey_2;
    case '3':
      return speckey_3;
    case '4':
      return speckey_4;
    case '5':
      return speckey_5;

    case 'Q':
      return speckey_Q;
    case 'W':
      return speckey_W;
    case 'E':
      return speckey_E;
    case 'R':
      return speckey_R;
    case 'T':
      return speckey_T;

    case 'A':
      return speckey_A;
    case 'S':
      return speckey_S;
    case 'D':
      return speckey_D;
    case 'F':
      return speckey_F;
    case 'G':
      return speckey_G;

//  case XXX:
//    return speckey_CAPS_SHIFT;
    case 'Z':
      return speckey_Z;
    case 'X':
      return speckey_X;
    case 'C':
      return speckey_C;
    case 'V':
      return speckey_V;

    default:
      return speckey_UNKNOWN;
  }
}

speckeyfield_t set_speckey(speckeyfield_t keystate, int c)
{
  speckey_t sk = char_to_speckey(c);
  if (sk != speckey_UNKNOWN)
    keystate |= 1ULL << sk;
  return keystate;
}

speckeyfield_t clear_speckey(speckeyfield_t keystate, int c)
{
  speckey_t sk = char_to_speckey(c);
  if (sk != speckey_UNKNOWN)
    keystate &= ~(1ULL << sk);
  return keystate;
}
