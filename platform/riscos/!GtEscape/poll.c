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

#include "poll.h"

/* A target time will be set when polling when inside the context of the
 * game, but otherwise we will use a default target time. This will control
 * how fast the outer loop runs. Since the game loop is rate limited this
 * shouldn't matter.
 *
 * BUT it does, because the speed of the loop is what we're limiting, but not
 * its rate.
 */

static os_t target = INT_MAX;

void poll_set_target(os_t time)
{
  if (time < target)
    target = time;
}

int poll(void)
{
  wimp_block poll;

  if (target == INT_MAX)
    target = os_read_monotonic_time() + 2; /* 50Hz */

  fprintf(stderr, "poll: target=%d\n", target);

  event_set_earliest(target);
  (void) event_poll(&poll);

  target = INT_MAX;

  return 0;
}

// vim: ts=8 sts=2 sw=2 et
