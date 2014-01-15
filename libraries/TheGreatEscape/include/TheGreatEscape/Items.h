#ifndef ITEMS_H
#define ITEMS_H

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
typedef enum item item_t;

#endif /* ITEMS_H */

