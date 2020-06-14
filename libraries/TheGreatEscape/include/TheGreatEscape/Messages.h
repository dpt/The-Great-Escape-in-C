/**
 * Messages.h
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

#ifndef MESSAGES_H
#define MESSAGES_H

/* ----------------------------------------------------------------------- */

#include "TheGreatEscape/TheGreatEscape.h"

/* ----------------------------------------------------------------------- */

/**
 * Identifiers of game messages.
 */
typedef enum message
{
  message_MISSED_ROLL_CALL,
  message_TIME_TO_WAKE_UP,
  message_BREAKFAST_TIME,
  message_EXERCISE_TIME,
  message_TIME_FOR_BED,
  message_THE_DOOR_IS_LOCKED,
  message_IT_IS_OPEN,
  message_INCORRECT_KEY,
  message_ROLL_CALL,
  message_RED_CROSS_PARCEL,
  message_PICKING_THE_LOCK,
  message_CUTTING_THE_WIRE,
  message_YOU_OPEN_THE_BOX,
  message_YOU_ARE_IN_SOLITARY,
  message_WAIT_FOR_RELEASE,
  message_MORALE_IS_ZERO,
  message_ITEM_DISCOVERED,
  message_HE_TAKES_THE_BRIBE,
  message_AND_ACTS_AS_DECOY,
  message_ANOTHER_DAY_DAWNS,
  message__LIMIT,
  message_QUEUE_END = 255
}
message_t;

/**
 * A flag set on state.messages.display_index to show the next message.
 */
#define MESSAGE_NEXT_FLAG (1 << 7)

/* $7D15 */
void queue_message(tgestate_t *state,
                   message_t   message_index);
/* $7D48 */
void message_display(tgestate_t *state);

/* ----------------------------------------------------------------------- */

#endif /* MESSAGES_H */

// vim: ts=8 sts=2 sw=2 et
