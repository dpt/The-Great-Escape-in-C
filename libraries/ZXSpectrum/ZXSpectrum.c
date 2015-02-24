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

#include "ZXSpectrum/ZXSpectrum.h"

#include "ZXScreen.h"

typedef struct ZXSpectrumPrivate
{
  ZXSpectrum_t        pub;
  ZXSpectrum_config_t config;
  
  unsigned int       *screen; /* Converted screen */
}
ZXSpectrumPrivate_t;

static uint8_t zx_in(ZXSpectrum_t *state, uint16_t address)
{
  ZXSpectrumPrivate_t *prv = (ZXSpectrumPrivate_t *) state;

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

static void zx_out(ZXSpectrum_t *state, uint16_t address, uint8_t byte)
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

static void zx_kick(ZXSpectrum_t *state)
{
  ZXSpectrumPrivate_t *prv = (ZXSpectrumPrivate_t *) state;
  
  ZXScreen_convert(prv->pub.screen, prv->screen);

  prv->config.draw(prv->screen, prv->config.opaque);
}

static void zx_sleep(ZXSpectrum_t *state, int duration)
{
  ZXSpectrumPrivate_t *prv = (ZXSpectrumPrivate_t *) state;

  prv->config.sleep(duration, prv->config.opaque);
}

ZXSpectrum_t *ZXSpectrum_create(const ZXSpectrum_config_t *config)
{
  ZXSpectrumPrivate_t *prv;
  
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
  
  ZXScreen_initialise();

  return &prv->pub;
}

void ZXSpectrum_destroy(ZXSpectrum_t *doomed)
{
  if (doomed == NULL)
    return;
  
  free(doomed->screen);
  free(doomed);
}
