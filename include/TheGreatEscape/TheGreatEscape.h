/**
 * Interface of The Great Escape in C.
 *
 * Copyright (c) David Thomas, 2013.
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
  
  
#include "ZXSpectrum/ZXSpectrum.h"


/* Exports go here... */

/**
 */
typedef struct tgeconfig
{
  int width, height; // unused as yet
}
tgeconfig_t;

/**
 * Holds the current state of the game.
 */
typedef struct tgestate tgestate_t;

/**
 * Create a game instance.
 */
TGE_API tgestate_t *tge_create(ZXSpectrum_t *speccy, tgeconfig_t *config);

/**
 * Destroy a game instance.
 */
TGE_API void tge_destroy(tgestate_t *state);

/**
 * Invoke the game instance.
 */
TGE_API void tge_main(tgestate_t *state);


#ifdef __cplusplus
}
#endif

#endif /* THE_GREAT_ESCAPE_H */

