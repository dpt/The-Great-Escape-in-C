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

#include "ZXSpectrum/Macros.h"

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

/* Set minimums smaller than maximums to invalidate. */
static void zxbox_invalidate(zxbox_t *b)
{
  b->x0 = INT_MAX;
  b->y0 = INT_MAX;
  b->x1 = INT_MIN;
  b->y1 = INT_MIN;
}

/* Return true if box is valid. */
static int zxbox_is_valid(zxbox_t *b)
{
  return (b->x0 < b->x1) && (b->y0 < b->y1);
}

/* Set largest possible valid box. */
static void zxbox_maximise(zxbox_t *b)
{
  b->x0 = INT_MIN;
  b->y0 = INT_MIN;
  b->x1 = INT_MAX;
  b->y1 = INT_MAX;
}

/* Return true if box is largest possible. */
static int zxbox_is_maximised(const zxbox_t *b)
{
  return b->x0 == INT_MIN &&
         b->y0 == INT_MIN &&
         b->x1 == INT_MAX &&
         b->y1 == INT_MAX;
}

/* Return true if box can hold (width,height) at (0,0). */
static int zxbox_exceeds(const zxbox_t *b, int width, int height)
{
  return (b->x0 <= 0)     && (b->y0 <= 0)      &&
         (b->x1 >= width) && (b->y1 >= height);
}

/* Return the union in 'c' of boxes 'a' and 'b'. */
static void zxbox_union(const zxbox_t *a, const zxbox_t *b, zxbox_t *c)
{
  c->x0 = MIN(a->x0, b->x0);
  c->y0 = MIN(a->y0, b->y0);
  c->x1 = MAX(a->x1, b->x1);
  c->y1 = MAX(a->y1, b->y1);
}

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

/* The game is telling us that the screen it draws to has been modified.
 *
 * Only the game (thread) writes to pub.screen. This entry point is called
 * when the game modifies it and wants us to know that. We're not obligated
 * to cause a screen/window refresh immediately, so may maintain an overall
 * dirty rectangle coalesced from multiple updates, but doing it in a timely
 * manner will improve the game's latency.
 *
 * We try to avoid copying the whole screen buffer on every update, where
 * practical, for speed. We do a partial copy then update our dirty rectangle
 * ready for zxspectrum_claim_screen.
 *
 * It's tempting to not copy the pub.screen here but instead wait for a
 * redraw message from the OS and do it then. However, the duration of this
 * call is the only window we have for legal access to pub.screen, so we have
 * to quickly copy out into our own buffer from where zxspectrum_claim_screen
 * can have unimpeded access.
 */
static void zx_draw(zxspectrum_t *state, const zxbox_t *dirty)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  mutex_lock(prv->lock);

  /* If no dirty rectangle was specified then assume the full screen. */
  if (dirty == NULL || zxbox_exceeds(dirty, SCREEN_WIDTH, SCREEN_HEIGHT))
  {
    /* Entire screen has been modified - copy it all. */
    memcpy(&prv->screen_copy, &prv->pub.screen, sizeof(prv->screen_copy));

    /* Maximise the overall dirty box that zxspectrum_claim_screen() will
     * use. */
    zxbox_maximise(&prv->dirty);
  }
  else
  {
    /* Copy pub.screen's dirty region into the screen copy. */

    zxbox_t box;
    int     width;
    int     height;
    int     linear_y;

    /* Clamp the dirty rectangle to the screen dimensions. */
    box.x0 = CLAMP(dirty->x0, 0, 255);
    box.y0 = CLAMP(dirty->y0, 0, 191);
    box.x1 = CLAMP(dirty->x1, 1, 256);
    box.y1 = CLAMP(dirty->y1, 1, 192);

    /* Divide down the x coordinates to get byte-sized quantities. */
    box.x0 = (box.x0    ) >> 3; /* divide to 0..31 rounding down */
    box.x1 = (box.x1 + 7) >> 3; /* divide to 0..31 rounding up */

    width = box.x1 - box.x0;

    /* Convert y coordinates into screen space - (0,0) is top left. */
    height = box.y1 - box.y0;
    box.y0 = 192 - box.y1;
    box.y1 = box.y0 + height;

    for (linear_y = box.y0; linear_y < box.y1; linear_y++)
    {
      /* Transpose fields using XOR */
      unsigned int tmp = (linear_y ^ (linear_y >> 3)) & 7;
      int          y   = linear_y ^ (tmp | (tmp << 3));

      memcpy(&prv->screen_copy.pixels[y * 32 + box.x0],
             &prv->pub.screen.pixels[y * 32 + box.x0],
             width);
    }

    /* Divide down the y coordinates to get attribute-sized quantities. */
    box.y0 = (box.y0    ) >> 3;
    box.y1 = (box.y1 + 7) >> 3;

    for (linear_y = box.y0; linear_y < box.y1; linear_y++)
    {
      memcpy(&prv->screen_copy.attributes[linear_y * 32 + box.x0],
             &prv->pub.screen.attributes[linear_y * 32 + box.x0],
             width);
    }

    /* Union the overall dirty box that zxspectrum_claim_screen() will use
     * with the outstanding one. */
    zxbox_union(&prv->dirty, dirty, &prv->dirty);
  }

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

  prv->pub.in            = zx_in;
  prv->pub.out           = zx_out;
  prv->pub.draw          = zx_draw;
  prv->pub.stamp         = zx_stamp;
  prv->pub.sleep         = zx_sleep;
  prv->pub.screen.width  = config->width;
  prv->pub.screen.height = config->height;

  prv->config = *config;

  mutex_init(prv->lock);

  zxbox_invalidate(&prv->dirty);

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
  if (zxbox_is_valid(&prv->dirty))
  {
    zxscreen_convert(prv->screen_copy.pixels, prv->converted, &prv->dirty);

    /* Invalidate the dirty region once complete */
    zxbox_invalidate(&prv->dirty);
  }

  return prv->converted;
}

void zxspectrum_release_screen(zxspectrum_t *state)
{
  zxspectrum_private_t *prv = (zxspectrum_private_t *) state;

  mutex_unlock(prv->lock);
}

// vim: ts=8 sts=2 sw=2 et
