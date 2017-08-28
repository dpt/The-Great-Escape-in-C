/* Spectrum.h
 *
 * Interface to a logical ZX Spectrum.
 *
 * Copyright (c) David Thomas, 2013-2017. <dave@davespace.co.uk>
 */

#ifndef ZXSPECTRUM_H
#define ZXSPECTRUM_H

#ifdef __cplusplus
extern "C"
{
#endif

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
  port_KEMPSTON_JOYSTICK        = 0x001F, /* 000FUDLR / active bits high */

  port_BORDER_EAR_MIC           = 0x00FE, /* Border, Ear, Mic */

  port_KEYBOARD_SHIFTZXCV       = 0xFEFE, /* 11111110 */
  port_KEYBOARD_ASDFG           = 0xFDFE, /* 11111101 */
  port_KEYBOARD_QWERT           = 0xFBFE, /* 11111011 */
  port_KEYBOARD_12345           = 0xF7FE, /* 11110111 */
  port_KEYBOARD_09876           = 0xEFFE, /* 11101111 */
  port_KEYBOARD_POIUY           = 0xDFFE, /* 11011111 */
  port_KEYBOARD_ENTERLKJH       = 0xBFFE, /* 10111111 */
  port_KEYBOARD_SPACESYMSHFTMNB = 0x7FFE, /* 01111111 */
};

/**
 * Masks for port $FE.
 */
enum
{
  port_MASK_BORDER  = 7 << 0,
  port_MASK_MIC     = 1 << 3,
  port_MASK_EAR     = 1 << 4,
};

/* Memory map */

#define ROM_LENGTH                      0x4000
#define SCREEN_BITMAP_LENGTH            (256 / 8 * 192)
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
typedef struct zxspectrum zxspectrum_t;

/**
 * Indicates what type of operation is in progress when doing sleep
 * (e.g. waiting for a menu, playing a sound).
 */
typedef enum
{
  sleeptype_MENU,     // main menu loop
  sleeptype_SOUND,    // inbetween sound pulses
  sleeptype_KEYSCAN,  // waiting for a keypress
  sleeptype_DELAY,    // too vague
  sleeptype_LOOP      // main game loop
}
sleeptype_t;

/**
 * Bounding box.
 */
typedef struct zxbox
{
  int x0, y0, x1, y1;
}
zxbox_t;

/**
 * The current state of the machine.
 */
struct zxspectrum
{
  /**
   * IN
   */
  uint8_t (*in)(zxspectrum_t *state, uint16_t address);

  /**
   * OUT
   */
  void (*out)(zxspectrum_t *state, uint16_t address, uint8_t byte);

  /**
   * Call the implementer when screen or attributes have changed.
   */
  void (*draw)(zxspectrum_t *state, const zxbox_t *dirty);

  /**
   * Call the implementer when we need to sleep.
   */
  void (*sleep)(zxspectrum_t *state, sleeptype_t type, int duration); // duration units are what?


  uint8_t     screen[SCREEN_BITMAP_LENGTH];
  // if a gap appears here then ZXScreen will break!
  attribute_t attributes[SCREEN_ATTRIBUTES_LENGTH];
};

/**
 * The configuration of the machine.
 */
typedef struct zxconfig
{
  /** An opaque pointer passed into callbacks. */
  void *opaque;

  /** Called when there's a new frame to draw. */
  void (*draw)(unsigned int *pixels, const zxbox_t *dirty, void *opaque);

  /** Called when there's nothing to do. */
  void (*sleep)(int duration, sleeptype_t sleeptype, void *opaque);

  /** Called when a key is tested. */
  int (*key)(uint16_t port, void *opaque);
}
zxconfig_t;

/**
 * Create a logical ZX Spectrum.
 *
 * \return New ZXSpectrum.
 */
zxspectrum_t *zxspectrum_create(const zxconfig_t *config);

/**
 * Destroy a logical ZX Spectrum.
 *
 * \param[in] doomed Doomed ZXSpectrum.
 */
void zxspectrum_destroy(zxspectrum_t *doomed);

#ifdef __cplusplus
}
#endif

#endif /* ZXSPECTRUM_H */

