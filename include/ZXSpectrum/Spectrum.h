/* Spectrum.h
 *
 * Interface to a logical ZX Spectrum.
 *
 * Copyright (c) David Thomas, 2013-2018. <dave@davespace.co.uk>
 */

#ifndef ZXSPECTRUM_H
#define ZXSPECTRUM_H

#include "C99/Types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Constants */

#define SCREEN_WIDTH                    (256)
#define SCREEN_HEIGHT                   (192)

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
  port_KEYBOARD_SPACESYMSHFTMNB = 0x7FFE  /* 01111111 */
};

/**
 * Masks for port $FE.
 */
enum
{
  port_MASK_BORDER  = 7 << 0,
  port_MASK_MIC     = 1 << 3,
  port_MASK_EAR     = 1 << 4
};

/* Memory map */

#define ROM_LENGTH                      0x4000
#define SCREEN_BITMAP_LENGTH            (SCREEN_WIDTH / 8 * SCREEN_HEIGHT)
#define SCREEN_ATTRIBUTES_LENGTH        (SCREEN_WIDTH / 8 * SCREEN_HEIGHT / 8)
#define SCREEN_LENGTH                   (SCREEN_BITMAP_LENGTH + SCREEN_ATTRIBUTES_LENGTH)

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
 * Bounding box.
 */
typedef struct zxbox
{
  int x0, y0, x1, y1;
}
zxbox_t;

/**
 * Screen pixels and attributes.
 */
typedef struct zxscreen
{
  int         width, height;
  uint8_t     pixels[SCREEN_BITMAP_LENGTH];
  attribute_t attributes[SCREEN_ATTRIBUTES_LENGTH];
}
zxscreen_t;

/**
 * The current state of the machine.
 *
 * FIXME: Ought to make this structure private and expose .screen via an
 *        accessor function.
 */
struct zxspectrum
{
  /**
   * The game calls this to simulate an IN instruction.
   */
  uint8_t (*in)(zxspectrum_t *state, uint16_t address);

  /**
   * The game calls this to simulate an OUT instruction.
   */
  void (*out)(zxspectrum_t *state, uint16_t address, uint8_t byte);

  /**
   * The game calls this when screen memory has changed.
   *
   * \param[in] dirty Dirty region.
   */
  void (*draw)(zxspectrum_t *state, const zxbox_t *dirty);

  /**
   * The game calls this at the start of a timed segment.
   */
  void (*stamp)(zxspectrum_t *state);

  /**
   * The game calls this when a timed segment ends, to sleep if required.
   *
   * \param[in] duration Sleep duration in T-states.
   *
   * \return Non-zero if the thread should terminate, zero otherwise.
   */
  int (*sleep)(zxspectrum_t *state, int duration);

  zxscreen_t screen;
};

/**
 * A configuration and handler specifier for app (host environment) code.
 */
typedef struct zxconfig
{
  /** Screen dimensions. */
  int width, height;

  /** An opaque pointer passed into app callbacks. */
  void *opaque;

  /** App callback called when screen updates are ready. */
  void (*draw)(const zxbox_t *dirty, void *opaque);

  /** App callback called at the start of a timed segment. */
  void (*stamp)(void *opaque);

  /** App callback called when a timed segment ends, to sleep if required. */
  int (*sleep)(int duration, void *opaque);

  /** App callback called to test a key. */
  int (*key)(uint16_t port, void *opaque);

  /** App callback called to set the border colour. */
  void (*border)(int colour, void *opaque);

  /** App callback called to sound the speaker. */
  void (*speaker)(int on_off, void *opaque);
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

/**
 * Lock the screen then return a converted screen buffer.
 *
 * Call this at the last moment when you're ready to use the screen pixels.
 *
 * \param[in] state ZXSpectrum state.
 *
 * \return Pixels.
 */
uint32_t *zxspectrum_claim_screen(zxspectrum_t *state);

/**
 * Unlock the screen.
 *
 * \param[in] state ZXSpectrum state.
 */
void zxspectrum_release_screen(zxspectrum_t *state);

#ifdef __cplusplus
}
#endif

#endif /* ZXSPECTRUM_H */

// vim: ts=8 sts=2 sw=2 et
