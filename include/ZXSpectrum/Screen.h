/* Screen.h
 *
 * ZX Spectrum screen decoding.
 *
 * Copyright (c) David Thomas, 2013-2018. <dave@davespace.co.uk>
 */

#ifndef ZXSPECTRUM_SCREEN_H
#define ZXSPECTRUM_SCREEN_H

#include "ZXSpectrum/Spectrum.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Convert the given ZX Spectrum format screen into 0x00BBGGRR pixel format
 * (or 0x00RRGGBB on Windows).
 *
 * \param[in] screen ZX Spectrum screen data.
 * \param[in] output Output screen pixels.
 * \param[in] dirty  Dirty rectangle in cartesian space - (0,0) is bottom left.
 */
void zxscreen_convert(const void    *screen,
                      unsigned int  *output,
                      const zxbox_t *dirty);

// 4bpp variant
void zxscreen_convert16(const void    *vscr,
                        unsigned int  *poutput,
                        const zxbox_t *dirty);

#ifdef __cplusplus
}
#endif

#endif /* ZXSPECTRUM_SCREEN_H */

// vim: ts=8 sts=2 sw=2 et
