/**
 * Input.c
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

#include "TheGreatEscape/Input.h"
#include "TheGreatEscape/State.h"
#include "TheGreatEscape/Utils.h"

/* ----------------------------------------------------------------------- */

/**
 * $FE00: Keyboard input routine.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
static input_t inputroutine_keyboard(tgestate_t *state)
{
  const keydef_t *def;         /* was HL */
  input_t         inputs;      /* was E */
  uint16_t        port;        /* was BC */
  uint8_t         key_pressed; /* was A */

  assert(state != NULL);

  def = &state->keydefs.defs[0]; /* A list of (port high byte, key mask) */

  /* Left or right? */
  port = (def->port << 8) | 0xFE;
  key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
  def++;
  if (key_pressed)
  {
    def++; /* Skip right keydef */
    inputs = input_LEFT;
  }
  else
  {
    /* Right */
    port = (def->port << 8) | 0xFE;
    key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
    def++;
    if (key_pressed)
      inputs = input_RIGHT;
    else
      inputs = input_NONE; /* == 0 */
  }

  /* Up or down? */
  port = (def->port << 8) | 0xFE;
  key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
  def++;
  if (key_pressed)
  {
    def++; /* Skip down keydef */
    inputs += input_UP;
  }
  else
  {
    /* Down */
    port = (def->port << 8) | 0xFE;
    key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
    def++;
    if (key_pressed)
      inputs += input_DOWN;
  }

  /* Fire? */
  port = (def->port << 8) | 0xFE;
  key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
  if (key_pressed)
    inputs += input_FIRE;

  return inputs;
}

/**
 * $FE47: Protek (cursor) joystick input routine.
 *
 * Up/Down/Left/Right/Fire = keys 7/6/5/8/0.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
static input_t inputroutine_protek(tgestate_t *state)
{
  uint8_t  keybits_left;       /* was A */
  input_t  left_right;         /* was E */
  uint16_t port;               /* was BC */
  uint8_t  keybits_others;     /* was D */
  input_t  up_down;            /* was A */
  input_t  left_right_up_down; /* was E */
  input_t  fire;               /* was A */

  assert(state != NULL);

  /* Horizontal */
  keybits_left = ~state->speccy->in(state->speccy, port_KEYBOARD_12345);
  left_right = input_LEFT;
  port = port_KEYBOARD_09876;
  if ((keybits_left & (1 << 4)) == 0)
  {
    keybits_left = ~state->speccy->in(state->speccy, port);
    left_right = input_RIGHT;
    if ((keybits_left & (1 << 2)) == 0)
      left_right = 0;
  }

  /* Vertical */
  keybits_others = ~state->speccy->in(state->speccy, port);
  up_down = input_UP;
  if ((keybits_others & (1 << 3)) == 0)
  {
    up_down = input_DOWN;
    if ((keybits_others & (1 << 4)) == 0)
      up_down = input_NONE;
  }

  left_right_up_down = left_right + up_down; /* Combine axis */

  /* Fire */
  fire = input_FIRE;
  if (keybits_others & (1 << 0))
    fire = 0;

  return fire + left_right_up_down; /* Combine axis */
}

/**
 * $FE7E: Kempston joystick input routine.
 *
 * Reading port 0x1F yields 000FUDLR active high.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
static input_t inputroutine_kempston(tgestate_t *state)
{
  uint8_t keybits;    /* was A */
  input_t left_right; /* was B */
  input_t up_down;    /* was C */
  int     carry = 0;  /* was flags */
  input_t fire;       /* was A */

  assert(state != NULL);

  keybits = state->speccy->in(state->speccy, 0x001F);

  left_right = up_down = 0;

  RR(keybits);
  if (carry)
    left_right = input_RIGHT;

  RR(keybits);
  if (carry)
    left_right = input_LEFT;

  RR(keybits);
  if (carry)
    up_down = input_DOWN;

  RR(keybits);
  if (carry)
    up_down = input_UP;

  RR(keybits);
  fire = input_FIRE;
  if (!carry)
    fire = 0;

  return fire + left_right + up_down; /* Combine axis */
}

#if 0

/**
 * $FEA3: Fuller joystick input routine.
 *
 * Unused, but present, as in the original game.
 *
 * Reading port 0x7F yields F---RLDU active low.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
static input_t inputroutine_fuller(tgestate_t *state)
{
  uint8_t keybits;    /* was A */
  input_t left_right; /* was B */
  input_t up_down;    /* was C */
  int     carry = 0;  /* was flags */
  input_t fire;       /* was A */

  assert(state != NULL);

  keybits = state->speccy->in(state->speccy, 0x007F);

  left_right = up_down = 0;

  if (keybits & (1 << 4))
    keybits = ~keybits;

  RR(keybits);
  if (carry)
    up_down = input_UP;

  RR(keybits);
  if (carry)
    up_down = input_DOWN;

  RR(keybits);
  if (carry)
    left_right = input_LEFT;

  RR(keybits);
  if (carry)
    left_right = input_RIGHT;

  if (keybits & (1 << 3))
    fire = input_FIRE;
  else
    fire = 0;

  return fire + left_right + up_down; /* Combine axis */
}

#endif

/**
 * $FECD: Sinclair joystick input routine.
 *
 * Up/Down/Left/Right/Fire = keys 9/8/6/7/0.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
static input_t inputroutine_sinclair(tgestate_t *state)
{
  uint8_t keybits;    /* was A */
  input_t left_right; /* was B */
  input_t up_down;    /* was C */
  int     carry = 0;  /* was flags */
  input_t fire;       /* was A */

  assert(state != NULL);

  keybits = ~state->speccy->in(state->speccy, port_KEYBOARD_09876); /* xxx67890 */

  left_right = up_down = 0;

  RRC(keybits); /* 0xxx6789 */
  RRC(keybits); /* 90xxx678 */
  if (carry)
    up_down = input_UP;

  RRC(keybits); /* 890xxx67 */
  if (carry)
    up_down = input_DOWN;

  RRC(keybits); /* 7890xxx6 */
  if (carry)
    left_right = input_RIGHT;

  RRC(keybits); /* 67890xxx */
  if (carry)
    left_right = input_LEFT;

  keybits &= 1 << 3; /* Set A to 8 or 0 */
  if (keybits)
    fire = input_FIRE;
  else
    fire = 0;

  return fire + left_right + up_down; /* Combine axis */
}

/* ----------------------------------------------------------------------- */

// ADDITIONAL FUNCTIONS
//

/**
 * Conv: The original game has a routine which copies an inputroutine_*
 * routine to a fixed location so that process_player_input() can call it
 * directly. We can't do that portably, so we add a selector routine here.
 */
input_t input_routine(tgestate_t *state)
{
  /**
   * $F43D: Available input routines.
   */
  static const inputroutine_t inputroutines[inputdevice__LIMIT] =
  {
    &inputroutine_keyboard,
    &inputroutine_kempston,
    &inputroutine_sinclair,
    &inputroutine_protek,
    /* &inputroutine_fuller -- Present in the game but never used */
  };

  assert(state != NULL);

  assert(state->chosen_input_device < inputdevice__LIMIT);

  return inputroutines[state->chosen_input_device](state);
}

// vim: ts=8 sts=2 sw=2 et
