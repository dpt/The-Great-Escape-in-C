/* --------------------------------------------------------------------------
 *    Name: poll.c
 * Purpose: Poll loop
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#include <stdio.h>
#include <limits.h>

#include "oslib/os.h"
#include "oslib/wimp.h"

#include "appengine/types.h"
#include "appengine/wimp/event.h"

#include "globals.h"

#include "poll.h"

/* A target time will be set when polling when inside the context of the
 * game, but otherwise we will use a default target time. */

static struct
{
  int  target_set;
  os_t target;
}
LOCALS;

void poll_set_target(os_t new_target)
{
  if (LOCALS.target_set && new_target >= LOCALS.target)
    return; /* retain existing target which is sooner */

  LOCALS.target_set = 1;
  LOCALS.target     = new_target;
}

int poll(void)
{
  wimp_block poll;

  //fprintf(stderr, "poll: target=%d\n", LOCALS.target);

  event_set_earliest(LOCALS.target);
  do
  {
    (void) event_poll(&poll);

    if ((GLOBALS.flags & Flag_Quit) != 0)
      break;
  }
  while (LOCALS.target_set && os_read_monotonic_time() < LOCALS.target);

  LOCALS.target_set = 0;
  LOCALS.target     = 0;

  return 0;
}

// vim: ts=8 sts=2 sw=2 et
