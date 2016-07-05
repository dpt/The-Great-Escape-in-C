//
//  speckey.h
//  TheGreatEscape
//
//  Created by David Thomas on 04/07/2016.
//  Copyright © 2016 David Thomas. All rights reserved.
//

#ifndef SPECKEY_H
#define SPECKEY_H

/**
 * A code for every Spectrum key.
 */
typedef enum speckey
{
  speckey_SPACE,
  speckey_SYMBOL_SHIFT,
  speckey_M,
  speckey_N,
  speckey_B,

  speckey_ENTER,
  speckey_L,
  speckey_K,
  speckey_J,
  speckey_H,

  speckey_P,
  speckey_O,
  speckey_I,
  speckey_U,
  speckey_Y,

  speckey_0,
  speckey_9,
  speckey_8,
  speckey_7,
  speckey_6,

  speckey_1,
  speckey_2,
  speckey_3,
  speckey_4,
  speckey_5,

  speckey_Q,
  speckey_W,
  speckey_E,
  speckey_R,
  speckey_T,

  speckey_A,
  speckey_S,
  speckey_D,
  speckey_F,
  speckey_G,

  speckey_CAPS_SHIFT,
  speckey_Z,
  speckey_X,
  speckey_C,
  speckey_V,

  speckey__LIMIT,

  speckey_UNKNOWN = -1
}
speckey_t;

/**
 * A bitfield large enough to hold all 40 Spectrum keys using one bit each.
 */
typedef unsigned long long speckeyfield_t;

/**
 * Mark or unmark the given key.
 */
speckeyfield_t assign_speckey(speckeyfield_t keystate,
                              speckey_t      index,
                              bool           on_off);

/**
 * Extract the current key state for the specified port and return it.
 */
int port_to_keyfield(uint16_t port, speckeyfield_t keystate);

/**
 * Mark the given character c as a held down key.
 */
speckeyfield_t set_speckey(speckeyfield_t keystate, int c);

/**
 * Mark the given character c as a released key.
 */
speckeyfield_t clear_speckey(speckeyfield_t keystate, int c);

#endif /* SPECKEY_H */
