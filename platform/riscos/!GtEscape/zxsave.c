/* --------------------------------------------------------------------------
 *    Name: zxsave.c
 * Purpose: Viewer save dialogue handler
 * ----------------------------------------------------------------------- */

#include <stddef.h>

#include "oslib/types.h"
#include "oslib/osfile.h"

#include "appengine/dialogues/save.h"
#include "appengine/wimp/dialogue.h"

#include "globals.h"
#include "menunames.h"
#include "zxgame.h"

#include "zxsave.h"

/* ----------------------------------------------------------------------- */

dialogue_t *zxgamesave_dlg;

static bits save_type; /* 1 for game, else screenshot */

/* ----------------------------------------------------------------------- */

static void zxgamesave_dlg_fillout(dialogue_t *d, void *opaque)
{
  zxgame_t   *zxgame;
  const char *file_name;

  NOT_USED(opaque);

  zxgame = GLOBALS.current_zxgame;
  if (zxgame == NULL)
    return;

  if (save_type != osfile_TYPE_SPRITE)
    file_name = "Escape";
  else
    file_name = "Screenshot";

  save_set_file_name(d, file_name);
  save_set_file_type(d, save_type);
}

/* Called on 'Save' button clicks, but not on drag saves. */
static void zxgamesave_dlg_handler(dialogue_t *d, const char *file_name)
{
  zxgame_t *zxgame;

  NOT_USED(d);

  zxgame = GLOBALS.current_zxgame;
  if (zxgame == NULL)
    return;

    // fixme - error handling

  if (save_type != osfile_TYPE_SPRITE)
    zxgame_save_game(zxgame, file_name);
  else
    zxgame_save_screenshot(zxgame, file_name);
}

/* ----------------------------------------------------------------------- */

static int zxgamesave_menu_warning(wimp_message *message, void *handle)
{
  wimp_message_menu_warning *menu_warning;

  NOT_USED(handle);

  menu_warning = (wimp_message_menu_warning *) &message->data;

  if (menu_warning->selection.items[0] == ZXGAME_SAVE)
  {
    switch (menu_warning->selection.items[1])
    {
      case 0:
        save_type = APPFILETYPE;
        return event_HANDLED;

      case 1:
        save_type = osfile_TYPE_SPRITE;
        return event_HANDLED;
    }
  }

  return event_NOT_HANDLED;
}

/* ----------------------------------------------------------------------- */

void zxgamesave_show_game(void)
{
  save_type = APPFILETYPE;
  dialogue_show(zxgamesave_dlg);
}

void zxgamesave_show_screenshot(void)
{
  save_type = osfile_TYPE_SPRITE;
  dialogue_show(zxgamesave_dlg);
}

/* ----------------------------------------------------------------------- */

error zxgamesave_dlg_init(void)
{
  dialogue_t *save;

  save = save_create();
  if (save == NULL)
    return error_OOM;

  dialogue_set_fillout_handler(save, zxgamesave_dlg_fillout, NULL);
  dialogue_set_menu_warning_handler(save, zxgamesave_menu_warning);
  save_set_save_handler(save, zxgamesave_dlg_handler);

  zxgamesave_dlg = save;

  return error_OK;
}

void zxgamesave_dlg_fin(void)
{
  save_destroy(zxgamesave_dlg);
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et
