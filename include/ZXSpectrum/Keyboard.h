/* Keyboard.h
 *
 * ZX Spectrum keyboard handling.
 *
 * Copyright (c) David Thomas, 2016-2020. <dave@davespace.co.uk>
 */

#ifndef ZXSPECTRUM_KEYBOARD_H
#define ZXSPECTRUM_KEYBOARD_H

#include "C99/Types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * One code for each of the 40 Spectrum keys.
 */
typedef enum zxkey
{
  zxkey_SPACE,
  zxkey_SYMBOL_SHIFT,
  zxkey_M,
  zxkey_N,
  zxkey_B,

  zxkey_ENTER,
  zxkey_L,
  zxkey_K,
  zxkey_J,
  zxkey_H,

  zxkey_P,
  zxkey_O,
  zxkey_I,
  zxkey_U,
  zxkey_Y,

  zxkey_0,
  zxkey_9,
  zxkey_8,
  zxkey_7,
  zxkey_6,

  zxkey_1,
  zxkey_2,
  zxkey_3,
  zxkey_4,
  zxkey_5,

  zxkey_Q,
  zxkey_W,
  zxkey_E,
  zxkey_R,
  zxkey_T,

  zxkey_A,
  zxkey_S,
  zxkey_D,
  zxkey_F,
  zxkey_G,

  zxkey_CAPS_SHIFT,
  zxkey_Z,
  zxkey_X,
  zxkey_C,
  zxkey_V,

  zxkey__LIMIT,

  zxkey_UNKNOWN = -1
}
zxkey_t;

/**
 * A structure to hold all 40 Spectrum keys using one bit each.
 */
typedef struct
{
  unsigned int bits[2];
}
zxkeyset_t;

/**
 * Clear the given keyset.
 */
void zxkeyset_clear(zxkeyset_t *keystate);

/**
 * Mark or unmark the given key.
 */
void zxkeyset_assign(zxkeyset_t *keystate, zxkey_t index, int on_off);

/**
 * Extract the current key state for the specified port and return it.
 */
int zxkeyset_for_port(uint16_t port, const zxkeyset_t *keystate);

/**
 * Mark the given character c as a held down key.
 */
void zxkeyset_setchar(zxkeyset_t *keystate, int c);

/**
 * Mark the given character c as a released key.
 */
void zxkeyset_clearchar(zxkeyset_t *keystate, int c);

#ifdef __cplusplus
}
#endif

#endif /* ZXSPECTRUM_KEYBOARD_H */

// vim: ts=8 sts=2 sw=2 et
