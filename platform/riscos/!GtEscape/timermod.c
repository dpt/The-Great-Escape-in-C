/* --------------------------------------------------------------------------
 *    Name: timermod.c
 * Purpose: TimerMod interface
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#include "swis.h"

#include "timermod.h"

#define Timer_Value 0x490C2

void read_timer(timer_t *t)
{
  unsigned int s, us;

  (void) _swi(Timer_Value, _OUTR(0,1), &s, &us);
  //fprintf(stderr, "read_timer @ %d : %d\n", s, us);

  t->s  = s;
  t->us = us;
}

float diff_timer(const timer_t *left, const timer_t *right)
{
  float d;

  d  = (left->s  - right->s);
  d += (left->us - right->us) / 1000000.0f; // usec -> sec

  return d;
}
