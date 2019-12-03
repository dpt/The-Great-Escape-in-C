/**
 * Debug.c
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

#include <assert.h>

#include "TheGreatEscape/Debug.h"
#include "TheGreatEscape/State.h"

void check_map_buf(tgestate_t *state)
{
  for (int i = 0; i < state->st_columns * state->st_rows; i++)
  {
    assert(state->map_buf[i] < supertileindex__LIMIT);
  }
}
