// main.cpp
//
// Windows front-end for The Great Escape
//
// Copyright (c) David Thomas, 2016-2026. <dave@davespace.co.uk>
//

#include <windows.h>

#include "TheGreatEscape/GameOps.h"

#include "resource.h"

int WINAPI WinMain(__in     HINSTANCE hInstance,
                   __in_opt HINSTANCE hPrevInstance,
                   __in     LPSTR     lpszCmdParam,
                   __in     int       nCmdShow)
{
  return zxgame_windows_main(hInstance, nCmdShow, &tge_game_ops, IDI_ICON1);
}

// vim: ts=8 sts=2 sw=2 et
