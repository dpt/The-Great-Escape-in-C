/**
 * Font.c
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

#include "TheGreatEscape/Pixels.h"
#include "TheGreatEscape/Tiles.h"

#include "TheGreatEscape/Font.h"

/**
 * $A69E: Bitmap font definition.
 */
const tile_t bitmap_font[38] =
{
  { /* Used for both zero and letter 'O' characters. */
    { ________,
      _XXXXX__,
      XXXXXXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXXXXXX_,
      _XXXXX__
    }
  },
  {
    { ________,
      ___XXXX_,
      __XXXXX_,
      _XX_XXX_,
      ____XXX_,
      ____XXX_,
      ____XXX_,
      ____XXX_
    }
  },
  {
    { ________,
      _XXXXX__,
      XXXXXXX_,
      XX__XXX_,
      ___XXX__,
      _XXX____,
      XXXXXXX_,
      XXXXXXX_
    }
  },
  {
    { ________,
      XXXXXX__,
      XXXXXXX_,
      ____XXX_,
      __XXXX__,
      ____XXX_,
      XXXXXXX_,
      XXXXXX__
    }
  },
  {
    { ________,
      ____XXX_,
      ___XXXX_,
      __XXXXX_,
      _XX_XXX_,
      XXXXXXX_,
      ____XXX_,
      ____XXX_
    }
  },
  {
    { ________,
      XXXXXX__,
      XX______,
      XXXXXX__,
      _XXXXXX_,
      ____XXX_,
      XXXXXXX_,
      XXXXXX__
    }
  },
  {
    { ________,
      __XXX___,
      _XX_____,
      XXXXXX__,
      XXXXXXX_,
      XX___XX_,
      XXXXXXX_,
      _XXXXX__
    }
  },
  {
    { ________,
      XXXXXXX_,
      ____XXX_,
      ____XXX_,
      ___XXX__,
      ___XXX__,
      __XXX___,
      __XXX___
    }
  },
  {
    { ________,
      _XXXXX__,
      XXX_XXX_,
      XXX_XXX_,
      _XXXXX__,
      XXX_XXX_,
      XXX_XXX_,
      _XXXXX__
    }
  },
  {
    { ________,
      _XXXXX__,
      XXXXXXX_,
      XX___XX_,
      XXXXXXX_,
      _XXXXXX_,
      ____XX__,
      __XXX___
    }
  },
  {
    { ________,
      __XXX___,
      _XXXXX__,
      _XXXXX__,
      XXX_XXX_,
      XXX_XXX_,
      XXXXXXX_,
      XXX_XXX_
    }
  },
  {
    { ________,
      XXXXXX__,
      XXX_XXX_,
      XXX_XXX_,
      XXXXXX__,
      XXX_XXX_,
      XXX_XXX_,
      XXXXXX__
    }
  },
  {
    { ________,
      ___XXXX_,
      _XXXXXX_,
      XXXXXXX_,
      XXXX____,
      XXXXXXX_,
      _XXXXXX_,
      ___XXXX_
    }
  },
  {
    { ________,
      XXXX____,
      XXXXXX__,
      XXX_XXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXXXXX__,
      XXXX____
    }
  },
  {
    { ________,
      XXXXXXX_,
      XXXXXXX_,
      XXX_____,
      XXXXXXX_,
      XXX_____,
      XXXXXXX_,
      XXXXXXX_
    }
  },
  {
    { ________,
      XXXXXXX_,
      XXXXXXX_,
      XXX_____,
      XXXXXX__,
      XXX_____,
      XXX_____,
      XXX_____
    }
  },
  {
    { ________,
      ___XXXX_,
      _XXXXXX_,
      XXXX____,
      XXX_XXX_,
      XXXX__X_,
      _XXXXXX_,
      ___XXXX_
    }
  },
  {
    { ________,
      XXX_XXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXXXXXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXX_XXX_
    }
  },
  {
    { ________,
      __XXX___,
      __XXX___,
      __XXX___,
      __XXX___,
      __XXX___,
      __XXX___,
      __XXX___
    }
  },
  {
    { ________,
      XXXXXXX_,
      __XXX___,
      __XXX___,
      __XXX___,
      __XXX___,
      XXXXX___,
      XXXX____
    }
  },
  {
    { ________,
      XXX_XXX_,
      XXX_XXX_,
      XXXXXX__,
      XXXXX___,
      XXXXXX__,
      XXX_XXX_,
      XXX_XXX_
    }
  },
  {
    { ________,
      XXX_____,
      XXX_____,
      XXX_____,
      XXX_____,
      XXX_____,
      XXXXXXX_,
      XXXXXXX_
    }
  },
  {
    { ________,
      _XX_XX__,
      XXXXXXX_,
      XXXXXXX_,
      XX_X_XX_,
      XX_X_XX_,
      XX___XX_,
      XX___XX_
    }
  },
  {
    { ________,
      XXX__XX_,
      XXXX_XX_,
      XXXXXXX_,
      XXXXXXX_,
      XXX_XXX_,
      XXX__XX_,
      XXX__XX_
    }
  },
  {
    { ________,
      XXXXXX__,
      XXX_XXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXXXXX__,
      XXX_____,
      XXX_____
    }
  },
  {
    { ________,
      _XXXXX__,
      XXXXXXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXXXXX__,
      _XXXXXX_
    }
  },
  {
    { ________,
      XXXXXX__,
      XXX_XXX_,
      XXX_XXX_,
      XXXXXX__,
      XXXXX___,
      XXX_XX__,
      XXX_XXX_
    }
  },
  {
    { ________,
      _XXXXXX_,
      XXXXXXX_,
      XXXX____,
      _XXXXX__,
      ___XXXX_,
      XXXXXXX_,
      XXXXXX__
    }
  },
  {
    { ________,
      XXXXXXX_,
      XXXXXXX_,
      __XXX___,
      __XXX___,
      __XXX___,
      __XXX___,
      __XXX___
    }
  },
  {
    { ________,
      XXX_XXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXXXXXX_,
      _XXXXX__
    }
  },
  {
    { ________,
      XXX_XXX_,
      XXX_XXX_,
      XXX_XXX_,
      XXX_XXX_,
      _XX_XX__,
      _XXXXX__,
      __XXX___
    }
  },
  {
    { ________,
      XX___XX_,
      XX___XX_,
      XX___XX_,
      XX_X_XX_,
      XXXXXXX_,
      XXX_XXX_,
      XX___XX_
    }
  },
  {
    { ________,
      XX___XX_,
      XXX_XXX_,
      _XXXXX__,
      __XXX___,
      _XXXXX__,
      XXX_XXX_,
      XX___XX_
    }
  },
  {
    { ________,
      XX___XX_,
      XXX_XXX_,
      _XXXXX__,
      __XXX___,
      __XXX___,
      __XXX___,
      __XXX___
    }
  },
  {
    { ________,
      XXXXXXX_,
      XXXXXXX_,
      ____XXX_,
      __XXX___,
      XXX_____,
      XXXXXXX_,
      XXXXXXX_
    }
  },
  {
    { ________,
      ________,
      ________,
      ________,
      ________,
      ________,
      ________,
      ________
    }
  },
  {
    { ________,
      ________,
      ________,
      ________,
      ________,
      ________,
      __XX____,
      __XX____
    }
  },
  { /* Conv: Added to represent unknown glyphs. */
    {
      _X_X_X_X,
      X_X_X_X_,
      _X_X_X_X,
      X_X_X_X_,
      _X_X_X_X,
      X_X_X_X_,
      _X_X_X_X,
      X_X_X_X_
    }
  },
};

/**
 * A table which maps from ASCII into the in-game glyph encoding.
 */
const unsigned char ascii_to_font[256] =
{
#define _ 37 // UNKNOWN
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
 35,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _, 36,  _,
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  _,  _,  _,  _,  _,  _,
  _, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,  0,
 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
#undef _
};

// vim: ts=8 sts=2 sw=2 et
