/* Spectrum.c
 *
 * Interface to a logical ZX Spectrum.
 *
 * Copyright (c) David Thomas, 2013-2018. <dave@davespace.co.uk>
 */

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ZXSpectrum/Screen.h"

#include "ZXSpectrum/Spectrum.h"

/* ----------------------------------------------------------------------- */

#ifdef _WIN32

#include <windows.h>

#define mutex_t          CRITICAL_SECTION
#define mutex_init(M)    InitializeCriticalSection(&M)
#define mutex_destroy(M) DeleteCriticalSection(&M)
#define mutex_lock(M)    EnterCriticalSection(&M)
#define mutex_unlock(M)  LeaveCriticalSection(&M)

#else

#include <pthread.h>

#define mutex_t          pthread_mutex_t
#define mutex_init(M)    pthread_mutex_init(&M, NULL)
#define mutex_lock(M)    pthread_mutex_lock(&M)
#define mutex_unlock(M)  pthread_mutex_unlock(&M)
#define mutex_destroy(M) pthread_mutex_destroy(&M)

#endif

/* ----------------------------------------------------------------------- */

/**
 * Return the minimum of (a,b).
 */
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

/**
 * Return the maximum of (a,b).
 */
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

/* ----------------------------------------------------------------------- */

typedef struct zxspectrum_private
{
  zxspectrum_t    pub;
  zxconfig_t      config;

  unsigned int    prev_border;

  mutex_t         lock;
  zxbox_t         dirty;
  zxscreen_t      screen_copy;
  uint32_t        converted[SCREEN_WIDTH * SCREEN_HEIGHT];
}
zxspectrum_private_t;

/* ----------------------------------------------------------------------- */

/* Callbacks called on game thread */

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
        if (prv->config.border)
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

/* The game is telling us that the screen has been modified.
 *
 * - only the game (thread) writes to pub.screen
 * - this entry point is called after the game modifies the screen and wants us to know about it
 * - we copy the whole screen and update the dirty region ready for zxspectrum_claim_screen
 */
static void zx_draw(zxspectrum_t *state, const zxbox_t *dirty)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  mutex_lock(prv->lock);

  if (dirty == NULL)
  {
    /* Entire screen has been modified */
    prv->dirty.x0 = 0;
    prv->dirty.y0 = 0;
    prv->dirty.x1 = SCREEN_WIDTH;
    prv->dirty.y1 = SCREEN_HEIGHT;
  }
  else
  {
    /* Part of the screen has been modified. Union the new dirty region with
     * the outstanding one. */
    prv->dirty.x0 = MIN(prv->dirty.x0, dirty->x0);
    prv->dirty.y0 = MIN(prv->dirty.y0, dirty->y0);
    prv->dirty.x1 = MAX(prv->dirty.x1, dirty->x1);
    prv->dirty.y1 = MAX(prv->dirty.y1, dirty->y1);
  }

  /* Copy the dirty region into the screen copy */
  // TODO: This copies the full screen every time. Use the dirty region to
  // copy less here (but that's complicated).
  memcpy(&prv->screen_copy, &prv->pub.screen.pixels, SCREEN_LENGTH);

  mutex_unlock(prv->lock);

  prv->config.draw(dirty, prv->config.opaque);
}

static void zx_stamp(zxspectrum_t *state)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  prv->config.stamp(prv->config.opaque);
}

static void zx_sleep(zxspectrum_t *state, int duration)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  prv->config.sleep(duration, prv->config.opaque);
}

/* ----------------------------------------------------------------------- */

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

  mutex_init(prv->lock);

  prv->dirty.x0 = INT_MAX;
  prv->dirty.y0 = INT_MAX;
  prv->dirty.x1 = INT_MIN;
  prv->dirty.y1 = INT_MIN;

  prv->prev_border = ~0;

  return &prv->pub;
}

void zxspectrum_destroy(zxspectrum_t *doomed)
{
  if (doomed == NULL)
    return;

  zxspectrum_private_t *prv = (zxspectrum_private_t *) doomed;

  mutex_destroy(prv->lock);

  free(prv);
}

uint32_t *zxspectrum_claim_screen(zxspectrum_t *state)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  mutex_lock(prv->lock);

  /* Check for any changes */
  if (prv->dirty.x0 != INT_MAX || prv->dirty.y0 != INT_MAX ||
      prv->dirty.x1 != INT_MIN || prv->dirty.y1 != INT_MIN)
  {
    zxscreen_convert(prv->screen_copy.pixels, prv->converted, &prv->dirty);

    /* Invalidate the dirty region once complete */
    prv->dirty.x0 = prv->dirty.y0 = INT_MAX;
    prv->dirty.x1 = prv->dirty.y1 = INT_MIN;
  }

  return prv->converted;
}

void zxspectrum_release_screen(zxspectrum_t *state)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  mutex_unlock(prv->lock);
}

// vim: ts=8 sts=2 sw=2 et
