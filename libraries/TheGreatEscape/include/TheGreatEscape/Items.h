/**
 * Items.h
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

#ifndef ITEMS_H
#define ITEMS_H

#include <stdint.h>

/**
 * Identifiers of game items.
 */
enum item
{
  item_WIRESNIPS,
  item_SHOVEL,
  item_LOCKPICK,
  item_PAPERS,
  item_TORCH,
  item_BRIBE,
  item_UNIFORM,
  item_FOOD,
  item_POISON,
  item_RED_KEY,
  item_YELLOW_KEY,
  item_GREEN_KEY,
  item_RED_CROSS_PARCEL,
  item_RADIO,
  item_PURSE,
  item_COMPASS,
  item__LIMIT,
  item_NONE = 255
};

/**
 * A game item.
 */
typedef uint8_t item_t;

#endif /* ITEMS_H */

// vim: ts=8 sts=2 sw=2 et
