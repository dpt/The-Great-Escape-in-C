/* --------------------------------------------------------------------------
 *    Name: globals.h
 * Purpose: Global variables
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#ifndef GLOBALS_H
#define GLOBALS_H

#include "oslib/wimp.h"

#include "appengine/wimp/dialogue.h"

#include "zxgame.h"

enum
{
  Flag_Quit = 1 << 0
};

typedef unsigned int Flags;

extern struct TheGreatEscapeGlobals
{
  Flags            flags;
  wimp_t           task_handle;
  wimp_version_no  wimp_version;

  wimp_w           zxgame_w; // template game window
  wimp_menu       *zxgame_m;

  zxgame_t        *current_zxgame;
}
GLOBALS;

#endif /* GLOBALS_H */

// vim: ts=8 sts=2 sw=2 et
