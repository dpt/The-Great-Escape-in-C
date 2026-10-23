/* --------------------------------------------------------------------------
 *    Name: main.c
 * Purpose: The Great Escape RISC OS front-end
 * ----------------------------------------------------------------------- */

#include "TheGreatEscape/GameOps.h"

static const zxgame_riscos_app_t app =
{
  "GtEscape",
  0x1E7, /* officially allocated */
  "gtescap",
  "Escape"
};

int main(int argc, char *argv[])
{
  return zxgame_riscos_main(argc, argv, &tge_game_ops, &app);
}
