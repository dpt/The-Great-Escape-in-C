/* Kempston.h
 *
 * ZX Spectrum Kempston joystick handling.
 *
 * Copyright (c) David Thomas, 2017. <dave@davespace.co.uk>
 */

#ifndef ZXSPECTRUM_KEMPSTON_H
#define ZXSPECTRUM_KEMPSTON_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum zxjoystick
{
  zxjoystick_UP,
  zxjoystick_DOWN,
  zxjoystick_LEFT,
  zxjoystick_RIGHT,
  zxjoystick_FIRE,

  zxjoystick_UNKNOWN = -1
}
zxjoystick_t;

typedef unsigned int zxkempston_t;

void zxkempston_assign(zxkempston_t *kempston,
                       zxjoystick_t  index,
                       int           on_off);

#ifdef __cplusplus
}
#endif

#endif /* ZXSPECTRUM_KEMPSTON_H */

// vim: ts=8 sts=2 sw=2 et
