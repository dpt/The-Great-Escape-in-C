/* speccy.h
 *
 * Interface to a logical ZX Spectrum.
 *
 * Copyright (c) David Thomas, 2013. <dave@davespace.co.uk>
 */

#ifndef SPECCY_H
#define SPECCY_H

#include <stdint.h>

enum
{
  attribute_BLUE_OVER_BLACK = 1,
  attribute_RED_OVER_BLACK = 2,
  attribute_PURPLE_OVER_BLACK = 3,
  attribute_GREEN_OVER_BLACK = 4,
  attribute_CYAN_OVER_BLACK = 5,
  attribute_YELLOW_OVER_BLACK = 6,
  attribute_WHITE_OVER_BLACK = 7,
  attribute_BRIGHT_BLUE_OVER_BLACK = 65,
  attribute_BRIGHT_RED_OVER_BLACK = 66,
  attribute_BRIGHT_PURPLE_OVER_BLACK = 67,
  attribute_BRIGHT_GREEN_OVER_BLACK = 68,
  attribute_BRIGHT_CYAN_OVER_BLACK = 69,
  attribute_BRIGHT_YELLOW_OVER_BLACK = 70,
  attribute_BRIGHT_WHITE_OVER_BLACK = 71
};

#define screen_length                   (256 / 8 * 192)
#define screen_attributes_length        (256 / 8 * 192 / 8)

#define rom_start_address               ((uint16_t) 0x0000)
#define rom_end_address                 ((uint16_t) 0x3FFF)
#define screen_start_address            ((uint16_t) 0x4000)
#define screen_attributes_start_address ((uint16_t) 0x5800)

/**
 * The current state of the machine.
 */
typedef struct speccystate speccystate_t;

/**
 * The current state of the machine.
 */
struct speccystate
{
  /**
   * IN
   */
  uint8_t (*in)(speccystate_t *state, uint16_t address);

  /**
   * OUT
   */
  void (*out)(speccystate_t *state, uint16_t address, uint8_t byte);

  /**
   * Call the implementer when something has changed.
   *
   * Is this a good idea?
   */
  void (*kick)(speccystate_t *state);


  uint8_t screen[screen_length];
  uint8_t attributes[screen_attributes_length];
  uint8_t border; // implement this through the 'OUT' method?
};

#endif /* SPECCY_H */

