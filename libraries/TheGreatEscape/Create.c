#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "TheGreatEscape/State.h"

#include "TheGreatEscape/TheGreatEscape.h"

tgestate_t *tge_create(ZXSpectrum_t *speccy, tgeconfig_t *config)
{
  tgestate_t *state;

  assert(config);

  if (config == NULL)
    return NULL;

  state = malloc(sizeof(*state));
  if (state == NULL)
    return NULL;
  
  /* Initialise additional variables */

  state->speccy = speccy;
  
  /* Initialise original game variables */
  
  state->moraleflag_screen_address = &state->speccy->screen[0x5002 - SCREEN_START_ADDRESS];

  return state;
}

void tge_destroy(tgestate_t *state)
{
  if (state == NULL)
    return;

  free(state);
}
