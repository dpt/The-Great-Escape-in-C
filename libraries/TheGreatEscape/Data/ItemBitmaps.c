/**
 * ItemBitmaps.c
 *
 * This file is part of "The Great Escape in C".
 *
 * This project recreates the 48K ZX Spectrum version of the prison escape
 * game "The Great Escape" in portable C code. It is free software provided
 * without warranty in the interests of education and software preservation.
 *
 * "The Great Escape" was created by Denton Designs and published in 1986 by
 * Ocean Software Limited.
 *
 * The original game is copyright (c) 1986 Ocean Software Ltd.
 * The original game design is copyright (c) 1986 Denton Designs Ltd.
 * The recreated version is copyright (c) 2012-2019 David Thomas
 */

/**
 * $DDDD: Item bitmaps and masks.
 *
 * All are 16 pixels wide. Variable height.
 */

#include "TheGreatEscape/Pixels.h"

#include "TheGreatEscape/ItemBitmaps.h"

const uint8_t bitmap_shovel[] =
{
  ________,________,
  ________,______X_,
  ________,_____X_X,
  ________,____XXX_,
  ________,__XX____,
  ________,XX______,
  __XX__XX,________,
  _XX_XX__,________,
  XXX__XXX,________,
  XXXXXX__,________,
  ________,________,
  ________,________,
  ________,________,
};

const uint8_t bitmap_key[] =
{
  ________,________,
  ________,________,
  ________,________,
  ________,___XX___,
  ________,_XX__X__,
  ________,___XXX__,
  ________,_XXX____,
  ___XX__X,XX______,
  __X__XXX,________,
  __XX__X_,________,
  ___XX__X,________,
  _____XXX,________,
  ________,________,
};

const uint8_t bitmap_lockpick[] =
{
  _______X,X_______,
  ________,XX______,
  ______XX,_XXX____,
  ____XX__,_XX_____,
  __XXX___,_X______,
  XXX_____,________,
  XX______,________,
  ______XX,___XX___,
  ____XX__,XXXX____,
  __XX____,XX______,
  __X___XX,_____XXX,
  __X_XX__,____X___,
  __XX____,__XXX___,
  ________,XXX__XX_,
  ______XX,XX___X__,
  ______XX,________,
};

const uint8_t bitmap_compass[] =
{
  ________,________,
  _____XXX,XXX_____,
  ___XX___,___XX___,
  __X__X__,__X__X__,
  _X_____X,______X_,
  _X_____X,______X_,
  __X__X__,X_X__X__,
  _X_XX___,X__XX_X_,
  __X__XXX,XXX__X__,
  ___XX___,___XX___,
  _____XXX,XXX_____,
  ________,________,
};

const uint8_t bitmap_purse[] =
{
  ________,________,
  _______X,X_______,
  _____XXX,_X______,
  ______XX,X_______,
  _______X,________,
  ______X_,X_______,
  _____X_X,_X______,
  ____XX_X,X_X_____,
  ____X_XX,XXX_____,
  ____XXXX,XXX_____,
  _____XXX,XX______,
  ________,________,
};

const uint8_t bitmap_papers[] =
{
  ________,________,
  ____XX__,________,
  _____XXX,________,
  _____XX_,XX______,
  ______X_,X_XX____,
  __XX__XX,_XX_XX__,
  _XX_XX__,XX_X_X__,
  _XX_X_XX,__XX_XX_,
  XX_XX_X_,XX__XXX_,
  XX_X_XX_,XXXX__XX,
  __XX_X_X,XXX_XX__,
  ____XX_X,XX_XXX__,
  ______XX,XX_X____,
  ________,X_______,
  ________,________,
};

const uint8_t bitmap_wiresnips[] =
{
  ________,________,
  ________,___XX___,
  ________,__XX_XX_,
  ________,_XX_____,
  ______XX,XXXXX_XX,
  ____XXX_,_XX_XXX_,
  __XX____,XXX_____,
  XX_____X,X_______,
  _____XX_,________,
  ___XX___,________,
  ________,________,
};

const uint8_t mask_shovelkey[] =
{
  XXXXXXXX,XXXXXX_X,
  XXXXXXXX,XXXXX___,
  XXXXXXXX,XXX_____,
  XXXXXXXX,X_______,
  XXXXXXXX,_______X,
  XX__XX__,_______X,
  X_______,______XX,
  ________,____XXXX,
  ________,__XXXXXX,
  ________,XXXXXXXX,
  ________,_XXXXXXX,
  XXX_____,_XXXXXXX,
  XXXXX___,XXXXXXXX,
};

const uint8_t mask_lockpick[] =
{
  XXXXXX__,__XXXXXX,
  XXXXXX__,____XXXX,
  XXXX____,_____XXX,
  XX______,____XXXX,
  ______XX,___XXXXX,
  _____XXX,X_XXXXXX,
  ___XXX__,XXX__XXX,
  __XX____,______XX,
  XX______,_____XXX,
  X_______,____X___,
  X_______,__XX____,
  X_______,XX______,
  X_____XX,_______X,
  XX__XX__,________,
  XXXXX___,___X___X,
  XXXXX___,__XXX_XX,
};

const uint8_t mask_compass[] =
{
  XXXXX___,___XXXXX,
  XXX_____,_____XXX,
  XX______,______XX,
  X_______,_______X,
  ________,________,
  ________,________,
  X_______,_______X,
  ________,________,
  X_______,_______X,
  XX______,______XX,
  XXX_____,_____XXX,
  XXXXX___,___XXXXX,
};

const uint8_t mask_purse[] =
{
  XXXXXXX_,_XXXXXXX,
  XXXXX___,__XXXXXX,
  XXXX____,___XXXXX,
  XXXXX___,__XXXXXX,
  XXXXXX__,__XXXXXX,
  XXXXX___,__XXXXXX,
  XXXX____,___XXXXX,
  XXX_____,____XXXX,
  XXX_____,____XXXX,
  XXX_____,____XXXX,
  XXXX____,___XXXXX,
  XXXXX___,__XXXXXX,
};

const uint8_t mask_papers[] =
{
  XXXX__XX,XXXXXXXX,
  XXX_____,XXXXXXXX,
  XXXX____,__XXXXXX,
  XXXX____,____XXXX,
  XX__X___,______XX,
  X_______,_______X,
  ________,_______X,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  XX______,_______X,
  XXXX____,______XX,
  XXXXXX__,__X_XXXX,
  XXXXXXXX,_XXXXXXX,
};

const uint8_t mask_wiresnips[] =
{
  XXXXXXXX,XXX__XXX,
  XXXXXXXX,XX_____X,
  XXXXXXXX,X_______,
  XXXXXX__,________,
  XXXX____,________,
  XX______,________,
  ________,_______X,
  ____X___,___XXXXX,
  __X_____,_XXXXXXX,
  XX_____X,XXXXXXXX,
  XXX__XXX,XXXXXXXX,
};

const uint8_t bitmap_food[] =
{
  ________,__XX____,
  ________,________,
  ________,__XX____,
  ________,__XX____,
  ____XXX_,_XXXX___,
  ___XXXXX,X_XXX___,
  _____XXX,__XXX___,
  ___XX___,X_XXX___,
  ___XXXX_,__XXX___,
  ___XX__X,X__XX___,
  ___X_XXX,XXX_____,
  ___XX__X,XXXXX___,
  _____XX_,_XX_____,
  _____XXX,X__XX___,
  _______X,XXXXX___,
  ________,_XX_____,
};

const uint8_t bitmap_poison[] =
{
  ________,________,
  ________,X_______,
  ________,X_______,
  _______X,_X______,
  _______X,XX______,
  ________,X_______,
  _______X,_X______,
  ______XX,XXX_____,
  _____XX_,__XX____,
  _____XX_,X_XX____,
  _____XX_,__XX____,
  _____XX_,XXXX____,
  _____XX_,XXXX____,
  _____XXX,XXXX____,
  _____X_X,XX_X____,
  ______XX,XXX_____,
};

const uint8_t bitmap_torch[] =
{
  ________,________,
  ________,____X___,
  ________,__XXXX__,
  ______X_,XXXXXX__,
  ____XX_X,_XXX____,
  ___XXXX_,X_X_____,
  ___XXXX_,X_______,
  ___X_XX_,X_______,
  ___X_XX_,X_______,
  ___X_XX_,________,
  ____XX__,________,
  ________,________,
};

const uint8_t bitmap_uniform[] =
{
  _______X,XXX_____,
  _____XXX,XXXX____,
  ____XXXX,XXXXX___,
  ____XXXX,XXXXX___,
  ___XXXXX,XXXXXX__,
  ____XXXX,XXXX__XX,
  XXXX__XX,XX__XX__,
  __XXXX__,__XX____,
  ____XXXX,XX__XXXX,
  XXXX__XX,__XXXX__,
  __XXXX__,XXXX____,
  ____XXXX,XX__XXXX,
  XXXX__XX,__XXXX__,
  __XXXX__,XXXX____,
  ____XXXX,XX______,
  ______XX,________,
};

const uint8_t bitmap_bribe[] =
{
  ________,________,
  ________,________,
  ______XX,________,
  ____XXXX,XX______,
  __XXXXXX,__XX____,
  _X__XX__,XXXXXX__,
  XXXX__XX,XXXX__X_,
  __XXXX__,XX__XXXX,
  ____XXXX,__XXXX__,
  ______XX,XXXX____,
  ________,XX______,
  ________,________,
  ________,________,
};

const uint8_t bitmap_radio[] =
{
  ________,___X____,
  ________,___X____,
  __XXX___,___X____,
  XX___XX_,___X____,
  __XX_XXX,X__X____,
  XX__XX__,_X_X____,
  XXXX__XX,_X_X____,
  XX__XX__,XXX_XXX_,
  X_XX_XXX,__XXX___,
  X_XX_XX_,XX___XX_,
  XX__XXXX,__XX_XX_,
  __XXXXX_,XX_X_XX_,
  ____XXXX,__XX_XX_,
  ______XX,XX_X_XX_,
  ________,XXXX_XX_,
  ________,__XX_X_X,
};

const uint8_t bitmap_parcel[] =
{
  ________,________,
  ______XX,________,
  ____XXX_,_X______,
  __XXX__X,XXXX____,
  XXX__XXX,XXX__X__,
  ___XXXXX,X__XXXXX,
  X___XXX_,_XXXXX__,
  X_XX___X,XXXX__XX,
  X_XXX___,XX__XXXX,
  X_XXX_XX,__XX_XXX,
  X_XXX_XX,_XXX__XX,
  X_XXX_XX,_XX__XXX,
  X_XXX_XX,_XXX_XXX,
  __XXX_XX,_XXXXX__,
  ____X_XX,_XXX____,
  ______XX,_X______,
};

const uint8_t mask_bribe[] =
{
  XXXXXX__,XXXXXXXX,
  XXXX____,__XXXXXX,
  XX______,____XXXX,
  X_______,______XX,
  X_______,________,
  ________,________,
  ________,________,
  ________,________,
  XX______,________,
  XXXX____,________,
  XXXXXX__,________,
  XXXXXXXX,______XX,
  XXXXXXXX,XX__XXXX,
};

const uint8_t mask_uniform[] =
{
  XXXXX___,____XXXX,
  XXXX____,_____XXX,
  XXX_____,______XX,
  XXX_____,______XX,
  XX______,________,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  ________,______XX,
  XX______,____XXXX,
  XXXX____,__XXXXXX,
};

const uint8_t mask_parcel[] =
{
  XXXXXX__,XXXXXXXX,
  XXXX____,__XXXXXX,
  XX______,____XXXX,
  ________,______XX,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  ________,________,
  XX______,______XX,
  XXXX____,____XXXX,
};

const uint8_t mask_poison[] =
{
  XXXXXXXX,_XXXXXXX,
  XXXXXXX_,__XXXXXX,
  XXXXXXX_,__XXXXXX,
  XXXXXX__,___XXXXX,
  XXXXXX__,___XXXXX,
  XXXXXXX_,__XXXXXX,
  XXXXXX__,___XXXXX,
  XXXXX___,____XXXX,
  XXXX____,_____XXX,
  XXXX____,_____XXX,
  XXXX____,_____XXX,
  XXXX____,_____XXX,
  XXXX____,_____XXX,
  XXXX____,_____XXX,
  XXXX____,_____XXX,
  XXXXX___,____XXXX,
};

const uint8_t mask_torch[] =
{
  XXXXXXXX,XXXX_XXX,
  XXXXXXXX,XX____XX,
  XXXXXX_X,_______X,
  XXXX____,_______X,
  XXX_____,______XX,
  XX______,____XXXX,
  XX______,___XXXXX,
  XX______,__XXXXXX,
  XX______,__XXXXXX,
  XX______,_XXXXXXX,
  XXX____X,XXXXXXXX,
  XXXX__XX,XXXXXXXX,
};

const uint8_t mask_radio[] =
{
  XXXXXXXX,XX___XXX,
  XX___XXX,XX___XXX,
  _______X,XX___XXX,
  ________,_X___XXX,
  ________,_____XXX,
  ________,_____XXX,
  ________,_______X,
  ________,________,
  ________,_______X,
  ________,________,
  ________,________,
  ________,________,
  XX______,________,
  XXXX____,________,
  XXXXXX__,________,
  XXXXXXXX,_______X,
};

const uint8_t mask_food[] =
{
  XXXXXXXX,X____XXX,
  XXXXXXXX,XX__XXXX,
  XXXXXXXX,X____XXX,
  XXXX___X,X____XXX,
  XXX_____,______XX,
  XX______,______XX,
  XXX_____,______XX,
  XX______,______XX,
  XX______,______XX,
  XX______,______XX,
  XX______,_____XXX,
  XX______,______XX,
  XXX_____,_____XXX,
  XXXX____,______XX,
  XXXXX___,______XX,
  XXXXXXX_,_____XXX,
};

// vim: ts=8 sts=2 sw=2 et
