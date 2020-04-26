/**
 * Music.h
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

#ifndef MUSIC_H
#define MUSIC_H

/* ----------------------------------------------------------------------- */

#include "C99/Types.h"

/* ----------------------------------------------------------------------- */

extern const uint8_t music_channel0_data[80 * 8 + 1];
extern const uint8_t music_channel1_data[80 * 8 + 1];

uint16_t frequency_for_semitone(uint8_t semitone, uint8_t *beep);

/* ----------------------------------------------------------------------- */

#endif /* MUSIC_H */

// vim: ts=8 sts=2 sw=2 et
