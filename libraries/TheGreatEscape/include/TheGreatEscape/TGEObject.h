#ifndef TGEOBJECT_H
#define TGEOBJECT_H

#include <stdint.h>

#include "TheGreatEscape/Utils.h"

/**
 * A game object.
 */
struct tgeobject
{
  uint8_t width, height;
  uint8_t data[UNKNOWN];
};

/**
 * A game object.
 */
typedef struct tgeobject tgeobject_t;

#endif /* TGEOBJECT_H */

