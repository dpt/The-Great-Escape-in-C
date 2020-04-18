/* --------------------------------------------------------------------------
 *    Name: game.c
 * Purpose: RISC OS front-end for The Great Escape
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#include "kernel.h"
#include "swis.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oslib/types.h"
#include "oslib/os.h"
#include "oslib/osspriteop.h"
#include "oslib/wimp.h"
#include "oslib/wimpspriteop.h"

#include "appengine/types.h"
#include "appengine/app/choices.h"
#include "appengine/base/bsearch.h"
#include "appengine/base/messages.h"
#include "appengine/base/os.h"
#include "appengine/base/oserror.h"
#include "appengine/base/strings.h"
#include "appengine/dialogues/scale.h"
#include "appengine/gadgets/iconbar.h"
#include "appengine/geom/box.h"
#include "appengine/vdu/screen.h"
#include "appengine/vdu/sprite.h"
#include "appengine/wimp/event.h"
#include "appengine/wimp/icon.h"
#include "appengine/wimp/menu.h"
#include "appengine/wimp/window.h"

#include "globals.h"
#include "menunames.h"
#include "zxgame.h"
#include "zxgames.h"
#include "iconbar.h"
#include "poll.h"

/* ----------------------------------------------------------------------- */

static event_message_handler message_quit,
                             message_palette_change,
                             message_mode_change,
                             message_save_desktop,
                             message_window_info;

/* ----------------------------------------------------------------------- */

static void register_event_handlers(int reg)
{
  static const event_message_handler_spec message_handlers[] =
  {
    { message_QUIT,           message_quit           },
    { message_PALETTE_CHANGE, message_palette_change },
    { message_MODE_CHANGE,    message_mode_change    },
    { message_SAVE_DESKTOP,   message_save_desktop   },
    { message_WINDOW_INFO,    message_window_info    },
  };

  event_register_message_group(reg,
                               message_handlers,
                               NELEMS(message_handlers),
                               event_ANY_WINDOW,
                               event_ANY_ICON,
                               NULL);
}

/* ----------------------------------------------------------------------- */

int main(void)
{
  static const wimp_MESSAGE_LIST(2) messages =
  {{
    message_MENUS_DELETED, /* We register all other messages as they are
                              required using Wimp_AddMessages, but without
                              this here, ColourPicker blows things up. */
    message_QUIT
  }};

  /* ColourTrans 1.64 is the RISC OS 3.6 version,
   * needed for wide translation table support. */
  if (xos_cli("RMEnsure ColourTrans 1.64") == NULL)
    GLOBALS.flags |= Flag_HaveWideColourTrans;

  if (xos_cli("RMEnsure SharedSoundBuffer 0.07") == NULL)
    GLOBALS.flags |= Flag_HaveSharedSoundBuffer;

  open_messages(APPNAME "Res:Messages");

  GLOBALS.task_handle = wimp_initialise(wimp_VERSION_RO3,
                                        message0("task"), // task name
           (const wimp_message_list *) &messages,
                                       &GLOBALS.wimp_version);

  /* Event handling */
  event_initialise();

  cache_mode_vars();

  /* Sprites */
  window_load_sprites(APPNAME "Res:Sprites");

  /* Window creation and event registration */
  templates_open(APPNAME "Res:Templates");

  // choices init goes here

  /* Initialise subsystems */
  zxgame_init();
  tge_icon_bar_init();

  templates_close();

  register_event_handlers(1);

  while ((GLOBALS.flags & Flag_Quit) == 0)
    poll();

  register_event_handlers(0);

  /* Finalise subsystems */
  tge_icon_bar_fin();
  zxgame_fin();

  event_finalise();

  wimp_close_down(GLOBALS.task_handle);

  close_messages();

  exit(EXIT_SUCCESS);
}

/* ----------------------------------------------------------------------- */

static int message_quit(wimp_message *message, void *handle)
{
  NOT_USED(message);
  NOT_USED(handle);

  GLOBALS.flags |= Flag_Quit;

  return event_HANDLED;
}

static int message_palette_change(wimp_message *message, void *handle)
{
  NOT_USED(message);
  NOT_USED(handle);

  zxgame_update_all(zxgame_UPDATE_COLOURS | zxgame_UPDATE_REDRAW);

  return event_PASS_ON;
}

static int kick_update_event_null_reason_code(wimp_event_no event_no,
                                              wimp_block   *block,
                                              void         *handle)
{
  NOT_USED(event_no);
  NOT_USED(block);
  NOT_USED(handle);

  zxgame_update_all(zxgame_UPDATE_EXTENT | zxgame_UPDATE_REDRAW);

  event_deregister_wimp_handler(wimp_NULL_REASON_CODE,
                                event_ANY_WINDOW,
                                event_ANY_ICON,
                                kick_update_event_null_reason_code,
                                NULL);

  return event_HANDLED;
}

static int message_mode_change(wimp_message *message, void *handle)
{
  NOT_USED(message);
  NOT_USED(handle);

  cache_mode_vars();

  zxgame_update_all(zxgame_UPDATE_COLOURS | zxgame_UPDATE_SCALING);

  // update all game windows

//  if (GLOBALS.choices.viewer.size == 1)
//  {
//    /* Since we can't change the extent of windows on a mode change, we need
//     * to register a callback to perform the work later on. If there was a
//     * way to test if the screen resolution had actually changed then I could
//     * avoid this. */
//
//    event_register_wimp_handler(wimp_NULL_REASON_CODE,
//                                event_ANY_WINDOW,
//                                event_ANY_ICON,
//                                kick_update_event_null_reason_code,
//                                NULL);
//  }

  return event_PASS_ON;
}

static int message_save_desktop(wimp_message *message, void *handle)
{
  NOT_USED(message);
  NOT_USED(handle);

  // TODO

  return event_HANDLED;
}

static int message_window_info(wimp_message *message, void *handle)
{
  wimp_message_window_info *window_info;

  NOT_USED(handle);

  message->size = sizeof(wimp_full_message_window_info);
  message->your_ref = message->my_ref;

  window_info = (wimp_message_window_info *) &message->data;
  strcpy(window_info->sprite_name, "gtescap"); // can't use the full 8 :-(
  strcpy(window_info->title, "The Great Escape");

  wimp_send_message(wimp_USER_MESSAGE, message, message->sender);

  return event_HANDLED;
}

// vim: ts=8 sts=2 sw=2 et
