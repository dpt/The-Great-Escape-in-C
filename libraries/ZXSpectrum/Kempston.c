/* Kempston.c
 *
 * ZX Spectrum Kempston joystick handling.
 *
 * Copyright (c) David Thomas, 2017-2020. <dave@davespace.co.uk>
 */

#include <assert.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "ZXSpectrum/Kempston.h"

void zxkempston_assign(zxkempston_t *kempston,
                       zxjoystick_t  index,
                       int           on_off)
{
  int i;

  on_off = !!on_off; /* ensure boolity */

  switch (index)
  {
    case zxjoystick_RIGHT: i = 0; break;
    case zxjoystick_LEFT:  i = 1; break;
    case zxjoystick_DOWN:  i = 2; break;
    case zxjoystick_UP:    i = 3; break;
    case zxjoystick_FIRE:  i = 4; break;
    default: return;
  }

  *kempston = (*kempston & ~(1 << i)) | on_off << i;
}

// vim: ts=8 sts=2 sw=2 et
