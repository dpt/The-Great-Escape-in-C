/* --------------------------------------------------------------------------
 *    Name: zxgames.h
 * Purpose: Handling all ZX games
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#ifndef ZXGAMES_H
#define ZXGAMES_H

#include "zxgame.h"

void zxgame_add(zxgame_t *zxgame);
void zxgame_remove(zxgame_t *zxgame);

typedef int (zxgame_map_callback)(zxgame_t *, void *opaque);

/* Call the specified function for every zxgame. */
void zxgame_map(zxgame_map_callback *fn, void *opaque);

void zxgame_update_all(zxgame_update_flags flags);

#endif /* ZXGAMES_H */
