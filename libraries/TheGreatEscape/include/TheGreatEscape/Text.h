/**
 * Text.h
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

#ifndef TEXT_H
#define TEXT_H

#include "TheGreatEscape/State.h"

uint8_t *plot_glyph(tgestate_t *state,
                    const char *pcharacter,
                    uint8_t    *output);
uint8_t *plot_single_glyph(tgestate_t *state,
                           int         character,
                           uint8_t    *output);

#endif /* TEXT_H */
