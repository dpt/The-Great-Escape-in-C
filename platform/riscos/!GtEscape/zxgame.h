/* --------------------------------------------------------------------------
 *    Name: zxgame.h
 * Purpose: ZX game handling
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#ifndef ZXGAME_H
#define ZXGAME_H

#include "appengine/base/errors.h"

typedef struct zxgame zxgame_t;

result_t zxgame_init(void);
void zxgame_fin(void);

result_t zxgame_create(zxgame_t **zxgame, const char *startup_game);
void zxgame_destroy(zxgame_t *zxgame);

int zxgame_get_scale(zxgame_t *zxgame);
void zxgame_set_scale(zxgame_t *zxgame, int scale);

void zxgame_open(zxgame_t *zxgame);

#ifdef TGE_SAVES
int zxgame_can_save(zxgame_t *zxgame);
result_t zxgame_load_game(zxgame_t *zxgame, const char *file_name);
result_t zxgame_save_game(zxgame_t *zxgame, const char *file_name);
result_t zxgame_save_screenshot(zxgame_t *zxgame, const char *file_name);
#endif /* TGE_SAVES */

enum
{
  zxgame_UPDATE_COLOURS = 1 << 0, /* regenerate pixtrans */
  zxgame_UPDATE_SCALING = 1 << 2, /* regenerate imgbox and factors */
  zxgame_UPDATE_EXTENT  = 1 << 4, /* ok to move windows */
  zxgame_UPDATE_REDRAW  = 1 << 8, /* redraw the whole window */

  zxgame_UPDATE_ALL     = zxgame_UPDATE_COLOURS |
                          zxgame_UPDATE_SCALING |
                          zxgame_UPDATE_EXTENT  |
                          zxgame_UPDATE_REDRAW
};

typedef unsigned int zxgame_update_flags;

void zxgame_update(zxgame_t *zxgame, zxgame_update_flags flags);

#endif /* ZXGAME_H */

// vim: ts=8 sts=2 sw=2 et
