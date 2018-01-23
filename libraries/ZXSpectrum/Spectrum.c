/* Spectrum.c
 *
 * Interface to a logical ZX Spectrum.
 *
 * Copyright (c) David Thomas, 2013-2018. <dave@davespace.co.uk>
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
  unsigned int        prev_border;
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
  case port_KEMPSTON_JOYSTICK:
    return prv->config.key(address, prv->config.opaque);

  default:
    assert("zx_in not implemented for that port" == NULL);
    return 0x00;
  }
}

static void zx_out(zxspectrum_t *state, uint16_t address, uint8_t byte)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  switch (address)
  {
  case port_BORDER_EAR_MIC:
    {
      unsigned int border;
      unsigned int ear;

      border = byte & port_MASK_BORDER;
      ear    = byte & port_MASK_EAR;

      if (border != prv->prev_border)
      {
        prv->config.border(border, prv->config.opaque);
        prv->prev_border = border;
      }

      if (prv->config.speaker)
        prv->config.speaker(ear != 0, prv->config.opaque);
    }
    break;

  default:
    assert("zx_out not implemented for that port" == NULL);
    break;
  }
}

static void zx_draw(zxspectrum_t *state, const zxbox_t *dirty)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  zxscreen_convert(prv->pub.screen.pixels, prv->screen, dirty);

  prv->config.draw(prv->screen, dirty, prv->config.opaque);
}

static void zx_stamp(zxspectrum_t *state)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  prv->config.stamp(prv->config.opaque);
}

static void zx_sleep(zxspectrum_t *state,
                     int           duration)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  prv->config.sleep(duration, prv->config.opaque);
}

zxspectrum_t *zxspectrum_create(const zxconfig_t *config)
{
  zxspectrum_private_t *prv;

  prv = malloc(sizeof(*prv));
  if (prv == NULL)
    return NULL;

  prv->pub.in    = zx_in;
  prv->pub.out   = zx_out;
  prv->pub.draw  = zx_draw;
  prv->pub.stamp = zx_stamp;
  prv->pub.sleep = zx_sleep;

  prv->config = *config;

  /* Converted screen */

  prv->screen = malloc(256 * 192 * sizeof(*prv->screen));
  if (prv->screen == NULL)
  {
    free(prv);
    return NULL;
  }

  prv->prev_border = ~0;

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
