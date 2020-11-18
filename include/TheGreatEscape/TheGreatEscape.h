/**
 * TheGreatEscape.h
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
 * The recreated version is copyright (c) 2012-2020 David Thomas
 */

#ifndef THE_GREAT_ESCAPE_H
#define THE_GREAT_ESCAPE_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef TGE_API
#  ifdef _WIN32
#    if defined(TGE_BUILD_SHARED) /* build dll */
#      define TGE_API __declspec(dllexport)
#    elif !defined(TGE_BUILD_STATIC) /* use dll */
#      define TGE_API __declspec(dllimport)
#    else /* static library */
#      define TGE_API
#    endif
#  else
#    if defined(__GNUC__) && __GNUC__ >= 4
#      define TGE_API __attribute__((visibility("default")))
#    else
#      define TGE_API
#    endif
#  endif
#endif


#include "ZXSpectrum/Spectrum.h"


/* Exports go here... */

/**
 * Holds the current state of the game.
 */
typedef struct tgestate tgestate_t;

/**
 * Create a game instance.
 */
TGE_API tgestate_t *tge_create(zxspectrum_t *speccy);

/**
 * Destroy a game instance.
 */
TGE_API void tge_destroy(tgestate_t *state);

/**
 * Prepare the game screen.
 */
TGE_API void tge_setup(tgestate_t *state);

/**
 * Run the game menu.
 *
 * Call this repeatedly until it returns > 0.
 *
 * \return > 0 when it's time to continue on to tge_setup2.
 */
TGE_API int tge_menu(tgestate_t *state);

/**
 * Prepare the game proper.
 */
TGE_API void tge_setup2(tgestate_t *state);

/**
 * Invoke the game instance.
 *
 * Call this repeatedly.
 */
TGE_API void tge_main(tgestate_t *state);

#ifdef TGE_SAVES

/**
 * Save the game state to 'filename'.
 */
TGE_API int tge_save(tgestate_t *state, const char *filename);

/**
 * Load the game state from 'filename'.
 */
TGE_API int tge_load(tgestate_t *state, const char *filename);

#endif /* TGE_SAVES */

#ifdef __cplusplus
}
#endif

#endif /* THE_GREAT_ESCAPE_H */

// vim: ts=8 sts=2 sw=2 et
