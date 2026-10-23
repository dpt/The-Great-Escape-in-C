/* sdl3main.c
 *
 * SDL3 front-end for The Great Escape.
 *
 * (c) David Thomas, 2017-2026.
 */

#include "TheGreatEscape/GameOps.h"

int main(int argc, char *argv[])
{
  return zxgame_sdl3_main(argc, argv, &tge_game_ops);
}
