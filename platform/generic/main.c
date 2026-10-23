/* main.c
 *
 * Headless front-end for The Great Escape.
 *
 * Runs the game as fast as possible for a number of frames with no display or
 * sound output. See ZXGame's HeadlessHost.c for the options.
 *
 * (c) David Thomas, 2017-2026.
 */

#include "TheGreatEscape/GameOps.h"

int main(int argc, char *argv[])
{
  return zxgame_headless_main(argc, argv, &tge_game_ops);
}
