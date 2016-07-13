/* ZXScreen.h
 *
 * ZX Spectrum screen decoding.
 *
 * Copyright (c) David Thomas, 2013-2016. <dave@davespace.co.uk>
 */

#ifndef ZXSPECTRUM_SCREEN_H
#define ZXSPECTRUM_SCREEN_H

/**
 * Build conversion tables.
 *
 * Can be called multiple times.
 */
void zxscreen_initialise(void);

/**
 * Convert the given screen into 0x00BBGGRR pixel format.
 *
 * \param[in] screen ZX Spectrum screen data.
 * \param[in] output Output screen pixels.
 */
void zxscreen_convert(const void *screen, unsigned int *output);

#endif /* ZXSPECTRUM_SCREEN_H */
