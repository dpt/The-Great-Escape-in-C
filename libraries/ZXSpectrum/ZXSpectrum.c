#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ZXSpectrum/ZXSpectrum.h"

typedef struct ZXSpectrumPrivate
{
  ZXSpectrum_t pub;
  
  int          some_private_variable;
}
ZXSpectrumPrivate_t;

static uint8_t zx_in(ZXSpectrum_t *state, uint16_t address)
{
  ZXSpectrumPrivate_t *prv = (ZXSpectrumPrivate_t *) state;
  
  // call out to host keyboard routine
  // map results to keycode
  // return keycode
  
  switch (address)
  {
    case port_KEYBOARD_SHIFTZXCV:
    case port_KEYBOARD_ASDFG:
    case port_KEYBOARD_QWERT:
    case port_KEYBOARD_12345:
    case port_KEYBOARD_09876:
    case port_KEYBOARD_POIUY:
    case port_KEYBOARD_ENTERLKJH:
    case port_KEYBOARD_SPACESYMSHFTMNB:
      return 0x1F;
      
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
}

ZXSpectrum_t *ZXSpectrum_create(void)
{
  ZXSpectrumPrivate_t *zx;
  
  zx = malloc(sizeof(*zx));
  if (zx == NULL)
    return NULL;

  zx->pub.in   = zx_in;
  zx->pub.out  = zx_out;
  zx->pub.kick = zx_kick;
  
  zx->some_private_variable = 42; // scratch
  
  return &zx->pub;
}

void ZXSpectrum_destroy(ZXSpectrum_t *zx)
{
  free(zx);
}
