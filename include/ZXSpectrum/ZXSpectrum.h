/* ZXSpectrum.h
 *
 * Interface to a logical ZX Spectrum.
 *
 * Copyright (c) David Thomas, 2013. <dave@davespace.co.uk>
 */

#ifndef ZXSPECTRUM_H
#define ZXSPECTRUM_H

#include <stdint.h>

/**
 * Identifiers of screen attributes.
 */
enum
{
  attribute_BLUE_OVER_BLACK           = 1,
  attribute_RED_OVER_BLACK            = 2,
  attribute_PURPLE_OVER_BLACK         = 3,
  attribute_GREEN_OVER_BLACK          = 4,
  attribute_CYAN_OVER_BLACK           = 5,
  attribute_YELLOW_OVER_BLACK         = 6,
  attribute_WHITE_OVER_BLACK          = 7,
  attribute_BRIGHT_BLUE_OVER_BLACK    = 65,
  attribute_BRIGHT_RED_OVER_BLACK     = 66,
  attribute_BRIGHT_PURPLE_OVER_BLACK  = 67,
  attribute_BRIGHT_GREEN_OVER_BLACK   = 68,
  attribute_BRIGHT_CYAN_OVER_BLACK    = 69,
  attribute_BRIGHT_YELLOW_OVER_BLACK  = 70,
  attribute_BRIGHT_WHITE_OVER_BLACK   = 71
};

/**
 * A screen attribute.
 */
typedef uint8_t attribute_t;

/**
 * Identifiers of port numbers.
 */
enum
{
  port_BORDER                   = 0x00FE, // border, ear, mic

  port_KEYBOARD_SHIFTZXCV       = 0xFEFE, // 11111110
  port_KEYBOARD_ASDFG           = 0xFDFE, // 11111101
  port_KEYBOARD_QWERT           = 0xFBFE, // 11111011
  port_KEYBOARD_12345           = 0xF7FE, // 11110111
  port_KEYBOARD_09876           = 0xEFFE, // 11101111
  port_KEYBOARD_POIUY           = 0xDFFE, // 11011111
  port_KEYBOARD_ENTERLKJH       = 0xBFFE, // 10111111
  port_KEYBOARD_SPACESYMSHFTMNB = 0x7FFE, // 01111111
};

/* Memory map */

#define ROM_LENGTH                      0x4000
#define SCREEN_LENGTH                   (256 / 8 * 192)
#define SCREEN_ATTRIBUTES_LENGTH        (256 / 8 * 192 / 8)

#define ROM_START_ADDRESS               ((uint16_t) 0x0000)
#define ROM_END_ADDRESS                 ((uint16_t) 0x3FFF)
#define SCREEN_START_ADDRESS            ((uint16_t) 0x4000)
#define SCREEN_END_ADDRESS              ((uint16_t) 0x57FF)
#define SCREEN_ATTRIBUTES_START_ADDRESS ((uint16_t) 0x5800)
#define SCREEN_ATTRIBUTES_END_ADDRESS   ((uint16_t) 0x5AFF)

/**
 * The current state of the machine.
 */
typedef struct ZXSpectrum ZXSpectrum_t;

/**
 * The current state of the machine.
 */
struct ZXSpectrum
{
  /**
   * IN
   */
  uint8_t (*in)(ZXSpectrum_t *state, uint16_t address);

  /**
   * OUT
   */
  void (*out)(ZXSpectrum_t *state, uint16_t address, uint8_t byte);


  uint8_t     screen[SCREEN_LENGTH];
  attribute_t attributes[SCREEN_ATTRIBUTES_LENGTH];
  
  /**
   * Call the implementer when screen or attributes have changed.
   *
   * Is this a good idea?
   */
  void (*kick)(ZXSpectrum_t *state);
};

/**
 * Create a logical ZX Spectrum.
 *
 * \return New ZXSpectrum.
 */
ZXSpectrum_t *ZXSpectrum_create(void);
 
/**
 * Destroy a logical ZX Spectrum.
 *
 * \param[in] doomed Doomed ZXSpectrum.
 */
void ZXSpectrum_destroy(ZXSpectrum_t *doomed);

#endif /* ZXSPECTRUM_H */

