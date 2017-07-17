/* Screen.h
 *
 * ZX Spectrum screen decoding.
 *
 * Copyright (c) David Thomas, 2013-2016. <dave@davespace.co.uk>
 */

#ifndef ZXSPECTRUM_SCREEN_H
#define ZXSPECTRUM_SCREEN_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Build conversion tables.
 *
 * Can be called multiple times.
 */
void zxscreen_initialise(void);

/**
 * Convert the given screen into 0x00BBGGRR pixel format (or 0x00RRGGBB on Windows).
 *
 * \param[in] screen ZX Spectrum screen data.
 * \param[in] output Output screen pixels.
 */
void zxscreen_convert(const void *screen, unsigned int *output);

#ifdef __cplusplus
}
#endif

#endif /* ZXSPECTRUM_SCREEN_H */
