/**
 * Screen.h
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

#ifndef SCREEN_H
#define SCREEN_H

/* ----------------------------------------------------------------------- */

#include "C99/Types.h"

#include "TheGreatEscape/TheGreatEscape.h"

/* ----------------------------------------------------------------------- */

/**
 * Invalidate (signal to redraw) the specified screen area where bitmap data
 * has changed.
 *
 * Conv: This helper function was added over the original game.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] addr   Pointer to screen pixels written to.
 * \param[in] width  Width in pixels of rectangle to invalidate.
 * \param[in] height Height in pixels of rectangle to invalidate.
 */
void invalidate_bitmap(tgestate_t *state,
                       uint8_t    *addr,
                       int         width,
                       int         height);

/**
 * Invalidate (signal to redraw) the specified screen area where attribute
 * data has changed.
 *
 * Conv: This helper function was added over the original game.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] addr   Pointer to screen attributes written to.
 * \param[in] width  Width in pixels of rectangle to invalidate.
 * \param[in] height Height in pixels of rectangle to invalidate.
 */
void invalidate_attrs(tgestate_t *state,
                      uint8_t    *addr,
                      int         width,
                      int         height);

/* ----------------------------------------------------------------------- */

#endif /* SCREEN_H */
