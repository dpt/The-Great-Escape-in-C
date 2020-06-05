/* --------------------------------------------------------------------------
 *    Name: poll.h
 * Purpose: Poll loop
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#ifndef POLL_H
#define POLL_H

void poll_set_target(os_t target);
int poll(void);

#endif /* POLL_H */

// vim: ts=8 sts=2 sw=2 et
