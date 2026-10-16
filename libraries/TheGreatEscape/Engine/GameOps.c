/* GameOps.c
 *
 * Adapts The Great Escape's setup/menu/main entry points to the ZXGame
 * front-end interface.
 *
 * Copyright (c) David Thomas, 2017-2026. <dave@davespace.co.uk>
 */

#include <stdlib.h>

#include "ZXSpectrum/Keyboard.h"
#include "ZXSpectrum/Spectrum.h"

#include "TheGreatEscape/GameOps.h"
#include "TheGreatEscape/TheGreatEscape.h"

/* ----------------------------------------------------------------------- */

typedef enum phase
{
  phase_setup,
  phase_menu,
  phase_main
}
phase_t;

typedef struct tge_game
{
  zxspectrum_t *zx;
  tgestate_t   *state;
  phase_t       phase;
}
tge_game_t;

/* ----------------------------------------------------------------------- */

static void *ops_create(zxspectrum_t *zx)
{
  tge_game_t *game;

  game = malloc(sizeof(*game));
  if (game == NULL)
    return NULL;

  game->zx    = zx;
  game->state = tge_create(zx);
  game->phase = phase_setup;

  if (game->state == NULL)
  {
    free(game);
    return NULL;
  }

  return game;
}

static void ops_destroy(void *opaque)
{
  tge_game_t *game = opaque;

  tge_destroy(game->state);
  free(game);
}

/* Starts the game proper, optionally restoring a saved game over the top. */
static void start_main(tge_game_t *game, const zxgame_launch_t *launch)
{
  tge_setup2(game->state);

#ifdef TGE_SAVES
  if (launch != NULL && launch->load_file != NULL)
  {
    char *error = NULL;

    if (tge_load(game->state, launch->load_file, &error) != 0)
      tge_disposeoferror(error);
  }
#endif

  game->phase = phase_main;
}

static int ops_step(void *opaque, const zxgame_launch_t *launch)
{
  tge_game_t *game = opaque;

  switch (game->phase)
  {
  case phase_setup:
    tge_setup(game->state);
    game->phase = phase_menu;
    break;

  case phase_menu:
    if (launch != NULL && launch->load_file != NULL)
      start_main(game, launch); /* bypass the menu */
    else if (tge_menu(game->state) > 0)
      start_main(game, launch);
    break;

  case phase_main:
    tge_main(game->state);
    zxspectrum_reset_stamps(game->zx);
    break;
  }

  return zxspectrum_quit_requested(game->zx);
}

static void ops_run(void *opaque, const zxgame_launch_t *launch)
{
  while (!ops_step(opaque, launch))
    ;
}

#ifdef TGE_SAVES

static int ops_save(void *opaque, const char *filename)
{
  tge_game_t *game = opaque;

  return tge_save(game->state, filename);
}

static int ops_load(void *opaque, const char *filename, char **error)
{
  tge_game_t *game = opaque;

  return tge_load(game->state, filename, error);
}

#endif

static int ops_in_menu(void *opaque)
{
  tge_game_t *game = opaque;

  return game->phase != phase_main;
}

/* Scripted key presses for unattended runs: '2' selects the Kempston
 * joystick at the menu, then '0' starts the game. */
static int ops_scripted_key(uint16_t port, unsigned poll_count)
{
  if (port == port_KEMPSTON_JOYSTICK)
    return 0; /* active high */

  if (poll_count < 3 && port == port_KEYBOARD_12345)
    return 0x1F ^ 2;

  if (poll_count < 6 && port == port_KEYBOARD_09876)
    return 0x1F ^ 1;

  return 0x1F;
}

/* ----------------------------------------------------------------------- */

const zxgame_ops_t tge_game_ops =
{
  "The Great Escape",
  ZXGAME_MODEL_48K,
  ZXGAME_MODEL_48K,
  ops_create,
  ops_destroy,
  ops_run,
  NULL, /* stop */
  ops_step,
#ifdef TGE_SAVES
  ops_save,
  ops_load,
  tge_disposeoferror,
  "tge",
#else
  NULL,
  NULL,
  NULL,
  NULL,
#endif
  ops_scripted_key,
  NULL, /* osd_glyph */
  NULL, /* toggle_debug */
  NULL, /* debug_buffer */
  ops_in_menu
};

/* vim: ts=8 sts=2 sw=2 et */
