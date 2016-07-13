/* ZXSpectrum.c
 *
 * Interface to a logical ZX Spectrum.
 *
 * Copyright (c) David Thomas, 2013-2015. <dave@davespace.co.uk>
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ZXSpectrum/Screen.h"

#include "ZXSpectrum/Spectrum.h"

typedef struct zxspectrum_private
{
  zxspectrum_t        pub;
  zxconfig_t          config;
  
  unsigned int       *screen; /* Converted screen */
}
zxspectrum_private_t;

static uint8_t zx_in(zxspectrum_t *state, uint16_t address)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  switch (address)
  {
    case port_KEYBOARD_12345:
    case port_KEYBOARD_09876:
    case port_KEYBOARD_QWERT:
    case port_KEYBOARD_POIUY:
    case port_KEYBOARD_ASDFG:
    case port_KEYBOARD_ENTERLKJH:
    case port_KEYBOARD_SHIFTZXCV:
    case port_KEYBOARD_SPACESYMSHFTMNB:
      return prv->config.key(address, prv->config.opaque);

    case port_KEMPSTON_JOYSTICK:
      return 0x00;

    default:
      assert("zx_in not implemented for that port" == NULL);
      return 0x00;
  }
}

static void zx_out(zxspectrum_t *state, uint16_t address, uint8_t byte)
{
  switch (address)
  {
    case port_BORDER:
      break;
      
    default:
      assert("zx_out not implemented for that port" == NULL);
      break;
  }
}

static void zx_kick(zxspectrum_t *state)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;
  
  zxscreen_convert(prv->pub.screen, prv->screen);

  prv->config.draw(prv->screen, prv->config.opaque);
}

static void zx_sleep(zxspectrum_t *state,
                     sleeptype_t   sleeptype,
                     int           duration)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  prv->config.sleep(duration, sleeptype, prv->config.opaque);
}

zxspectrum_t *zxspectrum_create(const zxconfig_t *config)
{
  zxspectrum_private_t *prv;
  
  prv = malloc(sizeof(*prv));
  if (prv == NULL)
    return NULL;

  prv->pub.in    = zx_in;
  prv->pub.out   = zx_out;
  prv->pub.kick  = zx_kick;
  prv->pub.sleep = zx_sleep;

  prv->config = *config;
  
  /* Converted screen */
  
  prv->screen = malloc(256 * 192 * sizeof(*prv->screen));
  if (prv->screen == NULL)
  {
    free(prv);
    return NULL;
  }
  
  zxscreen_initialise();

  return &prv->pub;
}

void zxspectrum_destroy(zxspectrum_t *doomed)
{
  if (doomed == NULL)
    return;
  
  zxspectrum_private_t *prv = (zxspectrum_private_t *) doomed;

  free(prv->screen);
  free(prv);
}
