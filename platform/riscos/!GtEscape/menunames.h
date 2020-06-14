/* --------------------------------------------------------------------------
 *    Name: menunames.h
 * Purpose: Identifiers for menu numbers
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#ifndef MENUNAMES_H
#define MENUNAMES_H

enum
{
  ICONBAR_INFO          = 0,
  ICONBAR_HELP,
  ICONBAR_INSTRUCTIONS,
  ICONBAR_QUIT,

  ZXGAME_VIEW           = 0,
  ZXGAME_SAVE,
  ZXGAME_SOUND,
  ZXGAME_SPEED,

  VIEW_FIXED            = 0,
  VIEW_SCALED,
  VIEW_FULL_SCREEN,
  VIEW_MONOCHROME,

  FIXED_SELECTED        = 0,
  FIXED_SCALE_VIEW,
  FIXED_BIG_WINDOW,

  SCALED_SELECTED       = 0,
  SCALED_SNAP,

  SAVE_GAME             = 0,
  SAVE_SCREENSHOT,

  SPEED_100PC           = 0,
  SPEED_MAXIMUM,
  SPEED_FASTER,
  SPEED_SLOWER,
  SPEED_PAUSE,

  SOUND_ENABLED         = 0
};

#endif /* MENUNAMES_H */

// vim: ts=8 sts=2 sw=2 et
