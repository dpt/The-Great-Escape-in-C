/* --------------------------------------------------------------------------
 *    Name: zxscale.c
 * Purpose: ZX game scale dialogue handler
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#include <assert.h>
#include <stddef.h>

#include "oslib/os.h"

#include "appengine/dialogues/scale.h"
#include "appengine/wimp/dialogue.h"

#include "globals.h"
//#include "zxgame.h"

#include "zxscale.h"

/* ----------------------------------------------------------------------- */

dialogue_t *zxgamescale_dlg;

/* ----------------------------------------------------------------------- */

static void zxgamescale_dlg_fillout(dialogue_t *d, void *opaque)
{
  zxgame_t *zxgame;

  NOT_USED(opaque);

  assert(opaque == NULL); /* handler is installed globally */

  zxgame = GLOBALS.current_zxgame;
  if (zxgame == NULL)
    return;

  scale_set(d, zxgame_get_scale(zxgame));
}

static void zxgamescale_dlg_handler(dialogue_t *d,
                                    scale_type  type,
                                    int         scale)
{
  zxgame_t *zxgame;

  NOT_USED(d);

  zxgame = GLOBALS.current_zxgame;
  if (zxgame == NULL)
    return;

  switch (type)
  {
  case scale_TYPE_VALUE:
    zxgame_set_scale(zxgame, scale);
    break;

  case scale_TYPE_FIT_TO_SCREEN:
    break;

  case scale_TYPE_FIT_TO_WINDOW:
    break;
  }
}

/* ----------------------------------------------------------------------- */

result_t zxgamescale_dlg_init(void)
{
  dialogue_t *scale;

  scale = scale_create();
  if (scale == NULL)
    return result_OOM;

  dialogue_set_fillout_handler(scale, zxgamescale_dlg_fillout, NULL);
  scale_set_range(scale, 10, 800);
  scale_set_steppings(scale, 10, 5);
  scale_set_scale_handler(scale, zxgamescale_dlg_handler);

  zxgamescale_dlg = scale;

  return result_OK;
}

void zxgamescale_dlg_fin(void)
{
  scale_destroy(zxgamescale_dlg);
}
