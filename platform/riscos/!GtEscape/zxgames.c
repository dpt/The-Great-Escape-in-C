/* --------------------------------------------------------------------------
 *    Name: zxgames.c
 * Purpose: Handling all ZX games
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#include <stdlib.h>

#include "fortify/fortify.h"

#include "appengine/datastruct/list.h"

#include "zxgame.h"
#include "zxgames.h"

/* ----------------------------------------------------------------------- */

/* Games are stored in a linked list. */
static list_t list_anchor = { NULL };

/* ----------------------------------------------------------------------- */

void zxgame_map(zxgame_map_callback *fn, void *opaque)
{
  /* Note that the callback signatures are identical, so we can cast. */
  list_walk(&list_anchor, (list_walk_callback *) fn, opaque);
}

/* ----------------------------------------------------------------------- */

static int update_all_callback(zxgame_t *zxgame, void *opaque)
{
  zxgame_update(zxgame, (unsigned int) opaque);
  return 0;
}

void zxgame_update_all(zxgame_update_flags flags)
{
  zxgame_map(update_all_callback, (void *) flags);
}

/* ----------------------------------------------------------------------- */

void zxgame_add(zxgame_t *zxgame)
{
  list_add_to_head(&list_anchor, (list_t *) zxgame); // icky
}

void zxgame_remove(zxgame_t *zxgame)
{
  list_remove(&list_anchor, (list_t *) zxgame);
}

/* ----------------------------------------------------------------------- */
