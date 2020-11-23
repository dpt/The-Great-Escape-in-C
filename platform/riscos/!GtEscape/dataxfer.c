/* --------------------------------------------------------------------------
 *    Name: dataxfer.h
 * Purpose: Data transfer
 * ----------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fortify/fortify.h"

#include "oslib/types.h"
#include "oslib/osfile.h"
#include "oslib/wimp.h"

#include "appengine/types.h"
#include "appengine/dialogues/save.h"
#include "appengine/wimp/event.h"

#include "globals.h"
#include "zxgame.h"

#include "dataxfer.h"

/* ----------------------------------------------------------------------- */

static event_message_handler message_data_save,
                             message_data_save_ack,
                             message_data_load,
                             message_data_load_ack,
                             message_data_open;

/* ----------------------------------------------------------------------- */

static void register_event_handlers(int reg)
{
  static const event_message_handler_spec message_handlers[] =
  {
    { message_DATA_SAVE,     message_data_save     },
    { message_DATA_SAVE_ACK, message_data_save_ack },
    { message_DATA_LOAD,     message_data_load     },
    { message_DATA_LOAD_ACK, message_data_load_ack },
    { message_DATA_OPEN,     message_data_open     }
  };

  event_register_message_group(reg,
                               message_handlers,
                               NELEMS(message_handlers),
                               event_ANY_WINDOW,
                               event_ANY_ICON,
                               NULL);
}

error dataxfer_init(void)
{
  register_event_handlers(1);

  return error_OK;
}

void dataxfer_fin(void)
{
  register_event_handlers(0);
}

/* ----------------------------------------------------------------------- */

static int message_data_save(wimp_message *message, void *handle)
{
  return event_HANDLED;
}

static int message_data_save_ack(wimp_message *message, void *handle)
{
  zxgame_t *zxgame;

  NOT_USED(handle);

  zxgame = GLOBALS.current_zxgame;
  if (zxgame == NULL)
    return event_NOT_HANDLED;

  switch (message->data.data_xfer.file_type)
  {
    case APPFILETYPE:
    if (zxgame_save_game(zxgame, message->data.data_xfer.file_name))
      return event_HANDLED; /* attempted, but failed */
    break;

    case osfile_TYPE_SPRITE:
    if (zxgame_save_screenshot(zxgame, message->data.data_xfer.file_name))
      return event_HANDLED; /* attempted, but failed */
  }

  message->your_ref = message->my_ref;
  message->action   = message_DATA_LOAD;
  wimp_send_message(wimp_USER_MESSAGE_RECORDED, message, message->sender);

  if (save_should_close_menu())
    wimp_create_menu(wimp_CLOSE_MENU, 0, 0);

  return event_HANDLED;
}

/* ----------------------------------------------------------------------- */

static int message_data_load(wimp_message *message, void *handle)
{
  error     err;
  zxgame_t *zxgame;

  NOT_USED(handle);

  if (message->data.data_xfer.file_type != APPFILETYPE)
    return event_NOT_HANDLED;

  /* acknowledge - even if we fail, we still tried to load it */
  message->your_ref = message->my_ref;
  message->action   = message_DATA_LOAD_ACK;
  wimp_send_message(wimp_USER_MESSAGE, message, message->sender);

  err = zxgame_create(&zxgame, message->data.data_xfer.file_name);
  if (err)
  {
    error_report(err);
    return event_HANDLED;
  }

  zxgame_open(zxgame);

  return event_HANDLED;
}

static int message_data_load_ack(wimp_message *message, void *handle)
{
  NOT_USED(message);
  NOT_USED(handle);

  return event_HANDLED;
}

/* ----------------------------------------------------------------------- */

static int message_data_open(wimp_message *message, void *handle)
{
  error     err;
  zxgame_t *zxgame;

  NOT_USED(handle);

  if (message->data.data_xfer.file_type != APPFILETYPE)
    return event_NOT_HANDLED;

  /* acknowledge - even if we fail, we still tried to load it */
  message->your_ref = message->my_ref;
  message->action   = message_DATA_LOAD_ACK;
  wimp_send_message(wimp_USER_MESSAGE, message, message->sender);

  err = zxgame_create(&zxgame, message->data.data_xfer.file_name);
  if (err)
  {
    error_report(err);
    return event_HANDLED;
  }

  zxgame_open(zxgame);

  return event_HANDLED;
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et
