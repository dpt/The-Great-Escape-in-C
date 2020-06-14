/* --------------------------------------------------------------------------
 *    Name: timermod.h
 * Purpose: TimerMod interface
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#ifndef TIMERMOD_H
#define TIMERMOD_H

typedef struct timer
{
  unsigned int s, us;
}
timer_t;

void read_timer(timer_t *t);
float diff_timer(const timer_t *left, const timer_t *right);

#endif /* TIMERMOD_H */
