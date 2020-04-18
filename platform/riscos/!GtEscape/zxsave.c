/* --------------------------------------------------------------------------
 *    Name: zxsave.c
 * Purpose: Viewer save dialogue handler
 * ----------------------------------------------------------------------- */

#include <stddef.h>

#include "oslib/types.h"

#include "appengine/dialogues/save.h"
#include "appengine/wimp/dialogue.h"

#include "globals.h"
#include "zxgame.h"

#include "zxsave.h"

/* ----------------------------------------------------------------------- */

dialogue_t *zxgamesave_dlg;

/* ----------------------------------------------------------------------- */

static void zxgamesave_dlg_fillout(dialogue_t *d, void *opaque)
{
  zxgame_t *zxgame;

  NOT_USED(opaque);

  zxgame = GLOBALS.current_zxgame;
  if (zxgame == NULL)
    return;

  save_set_file_name(d, "SaveGame");
  save_set_file_type(d, 0x18E);
}

/* Called on 'Save' button clicks, but not on drag saves. */
static void zxgamesave_dlg_handler(dialogue_t *d, const char *file_name)
{
  zxgame_t *zxgame;

  NOT_USED(d);

  zxgame = GLOBALS.current_zxgame;
  if (zxgame == NULL)
    return;

  //zxgame_save(zxgame, file_name);
}

/* ----------------------------------------------------------------------- */

error zxgamesave_dlg_init(void)
{
  dialogue_t *save;

  save = save_create();
  if (save == NULL)
    return error_OOM;

  dialogue_set_fillout_handler(save, zxgamesave_dlg_fillout, NULL);
  save_set_save_handler(save, zxgamesave_dlg_handler);

  zxgamesave_dlg = save;

  return error_OK;
}

void zxgamesave_dlg_fin(void)
{
  save_destroy(zxgamesave_dlg);
}
