/**
 * Interface of The Great Escape in C.
 *
 * Copyright (c) David Thomas, 2013-2015.
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
#    if __GNUC__ >= 4
#      define TGE_API __attribute__((visibility("default")))
#    else
#      define TGE_API
#    endif
#  endif
#endif


#include "ZXSpectrum/Spectrum.h"


/* Exports go here... */

/**
 */
typedef struct tgeconfig
{
  int width, height;
}
tgeconfig_t;

/**
 * Holds the current state of the game.
 */
typedef struct tgestate tgestate_t;

/**
 * Create a game instance.
 */
TGE_API tgestate_t *tge_create(zxspectrum_t      *speccy,
                               const tgeconfig_t *config);

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
 * Call this repeatedly until it returns > 0.
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


#ifdef __cplusplus
}
#endif

#endif /* THE_GREAT_ESCAPE_H */

