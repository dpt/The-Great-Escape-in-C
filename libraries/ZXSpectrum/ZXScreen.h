/* ZXScreen.h
 *
 * ZX Spectrum screen decoding.
 *
 * Copyright (c) David Thomas, 2013-2015. <dave@davespace.co.uk>
 */

#ifndef ZXSPECTRUM_ZXSCREEN_H
#define ZXSPECTRUM_ZXSCREEN_H

/**
 * Initialise ZXScreen.
 *
 * Can be called multiple times.
 */
void ZXScreen_initialise(void);

/**
 * Convert the given screen into 0x00BBGGRR pixel format.
 *
 * \param[in] screen ZX Spectrum screen data.
 * \param[in] output Output screen pixels.
 */
void ZXScreen_convert(const void *screen, unsigned int *output);

#endif /* ZXSPECTRUM_ZXSCREEN_H */
