#ifndef MESSAGES_H
#define MESSAGES_H

#include "TheGreatEscape/TheGreatEscape.h"

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

/* $7D15 */
void queue_message_for_display(tgestate_t *state,
                               message_t   message_index);
/* $7D48 */
void message_display(tgestate_t *state);

#endif /* MESSAGES_H */
