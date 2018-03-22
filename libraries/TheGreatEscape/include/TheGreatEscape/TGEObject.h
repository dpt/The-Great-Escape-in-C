/**
 * TGEObject.h
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
 * The recreated version is copyright (c) 2012-2018 David Thomas
 */

#ifndef TGE_OBJECT_H
#define TGE_OBJECT_H

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

#endif /* TGE_OBJECT_H */

// vim: ts=8 sts=2 sw=2 et
