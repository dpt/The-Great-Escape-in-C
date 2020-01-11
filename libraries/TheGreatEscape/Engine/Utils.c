/**
 * Utils.c
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

#include "TheGreatEscape/TheGreatEscape.h"

#include "TheGreatEscape/Main.h"
#include "TheGreatEscape/State.h"
#include "TheGreatEscape/Utils.h"

int menudelay(tgestate_t *state, int duration)
{
  state->speccy->stamp(state->speccy);
  return state->speccy->sleep(state->speccy, duration);
}

void gamedelay(tgestate_t *state, int duration)
{
  int terminate;

  state->speccy->stamp(state->speccy);
  terminate = state->speccy->sleep(state->speccy, duration);
  if (terminate)
    squash_stack_goto_main(state); /* exits from tge_main */
}
