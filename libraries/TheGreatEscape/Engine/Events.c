/**
 * Events.c
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
#include <string.h>

#include "TheGreatEscape/Main.h"
#include "TheGreatEscape/Messages.h"
#include "TheGreatEscape/RoomDefs.h"
#include "TheGreatEscape/State.h"

#include "TheGreatEscape/Events.h"

/* ----------------------------------------------------------------------- */

/**
 * $A1A0: Dispatch timed events.
 *
 * \param[in] state Pointer to game state.
 */
void dispatch_timed_event(tgestate_t *state)
{
  /**
   * $A173: Timed events.
   */
  static const timedevent_t timed_events[15] =
  {
    {   0, &event_another_day_dawns    },
    {   8, &event_wake_up              },
    {  12, &event_new_red_cross_parcel },
    {  16, &event_go_to_roll_call      },
    {  20, &event_roll_call            },
    {  21, &event_go_to_breakfast_time },
    {  36, &event_end_of_breakfast     },
    {  46, &event_go_to_exercise_time  },
    {  64, &event_exercise_time        },
    {  74, &event_go_to_roll_call      },
    {  78, &event_roll_call            },
    {  79, &event_go_to_time_for_bed   },
    {  98, &event_time_for_bed         },
    { 100, &event_night_time           },
    { 130, &event_search_light         },
  };

  eventtime_t        *pcounter; /* was HL */
  eventtime_t         time;     /* was A */
  const timedevent_t *event;    /* was HL */
  uint8_t             iters;    /* was B */

  assert(state != NULL);

  /* Increment the clock, wrapping at 140. */
  pcounter = &state->clock;
  time = *pcounter + 1;
  if (time == 140) // hoist 140 out to time_MAX or somesuch
    time = 0;
  *pcounter = time;

  /* Dispatch the event for that time. */
  event = &timed_events[0];
  iters = NELEMS(timed_events);
  do
  {
    if (time == event->time)
      goto found; // in future rewrite to avoid the goto
    event++;
  }
  while (--iters);

  return;

found:
  event->handler(state);
}

void event_night_time(tgestate_t *state)
{
  assert(state != NULL);

  if (state->hero_in_bed == 0)
  {
    static const route_t t = { routeindex_44_HUT2_RIGHT_TO_LEFT, 1 }; /* was BC */
    set_hero_route(state, &t);
  }
  set_day_or_night(state, 255);
}

void event_another_day_dawns(tgestate_t *state)
{
  assert(state != NULL);

  queue_message(state, message_ANOTHER_DAY_DAWNS);
  decrease_morale(state, 25);
  set_day_or_night(state, 0x00);
}

/**
 * $A1DE: Tail end of above two routines.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] day_night Day or night flag. (was A)
 */
void set_day_or_night(tgestate_t *state, uint8_t day_night)
{
  attribute_t attrs;

  assert(state != NULL);
  assert(day_night == 0 || day_night == 255);

  state->day_or_night = day_night; // night=255, day=0
  attrs = choose_game_window_attributes(state);
  set_game_window_attributes(state, attrs);
}

void event_wake_up(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message(state, message_TIME_TO_WAKE_UP);
  wake_up(state);
}

void event_go_to_roll_call(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message(state, message_ROLL_CALL);
  go_to_roll_call(state);
}

void event_go_to_breakfast_time(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message(state, message_BREAKFAST_TIME);
  set_route_go_to_breakfast(state);
}

void event_end_of_breakfast(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  end_of_breakfast(state);
}

void event_go_to_exercise_time(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message(state, message_EXERCISE_TIME);

  /* Unlock the gates. */
  state->locked_doors[0] = 0; /* Index into doors + clear locked flag. */
  state->locked_doors[1] = 1;

  set_route_go_to_yard(state);
}

void event_exercise_time(tgestate_t *state) // end of exercise time?
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  set_route_go_to_yard_reversed(state);
}

void event_go_to_time_for_bed(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;

  /* Lock the gates. */
  state->locked_doors[0] = 0 | door_LOCKED; /* Index into doors + set locked flag. */
  state->locked_doors[1] = 1 | door_LOCKED;

  queue_message(state, message_TIME_FOR_BED);
  go_to_time_for_bed(state);
}

void event_new_red_cross_parcel(tgestate_t *state)
{
  static const itemstruct_t red_cross_parcel_reset_data =
  {
    0, /* never used */
    room_20_REDCROSS,
    { 44, 44, 12 },
    { 128, 244 }
  };

  static const item_t red_cross_parcel_contents_list[4] =
  {
    item_PURSE,
    item_WIRESNIPS,
    item_BRIBE,
    item_COMPASS,
  };

  const item_t *item;  /* was DE */
  uint8_t       iters; /* was B */

  assert(state != NULL);

  /* Don't deliver a new red cross parcel while the previous one still
   * exists. */
  if ((state->item_structs[item_RED_CROSS_PARCEL].room_and_flags & itemstruct_ROOM_MASK) != itemstruct_ROOM_NONE)
    return;

  /* Select the contents of the next parcel; choosing the first item from the
   * list which does not already exist. */
  item = &red_cross_parcel_contents_list[0];
  iters = NELEMS(red_cross_parcel_contents_list);
  do
  {
    itemstruct_t *itemstruct; /* was HL */

    itemstruct = &state->item_structs[*item];
    if ((itemstruct->room_and_flags & itemstruct_ROOM_MASK) == itemstruct_ROOM_NONE)
      goto found; /* FUTURE: Remove goto. */

    item++;
  }
  while (--iters);

  return;

found:
  state->red_cross_parcel_current_contents = *item;
  memcpy(&state->item_structs[item_RED_CROSS_PARCEL].room_and_flags,
         &red_cross_parcel_reset_data.room_and_flags,
         6);
  queue_message(state, message_RED_CROSS_PARCEL);
}

void event_time_for_bed(tgestate_t *state)
{
  assert(state != NULL);

  // Reverse route of below.

  static const route_t t = { routeindex_38_GUARD_12_BED | ROUTEINDEX_REVERSE_FLAG, 3 }; /* was C,A */
  set_guards_route(state, t);
}

void event_search_light(tgestate_t *state)
{
  assert(state != NULL);

  static const route_t t = { routeindex_38_GUARD_12_BED, 0 }; /* was C,A */
  set_guards_route(state, t);
}

/**
 * Common end of event_time_for_bed and event_search_light.
 * Sets the location of guards 12..15 (the guards from prisoners_and_guards).
 *
 * \param[in] state Pointer to game state.
 * \param[in] route Route.                 (was C,A (hi,lo))
 */
void set_guards_route(tgestate_t *state, route_t route)
{
  character_t index; /* was A' */
  uint8_t     iters; /* was B */

  assert(state != NULL);
  ASSERT_ROUTE_VALID(route);

  index = character_12_GUARD_12;
  iters = 4;
  do
  {
    set_character_route(state, index, route);
    index++;
    route.index++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A27F: List of non-player characters: six prisoners and four guards.
 *
 * Used by set_prisoners_and_guards_route and
 * set_prisoners_and_guards_route_B.
 */
static const character_t prisoners_and_guards[10] =
{
  character_12_GUARD_12,
  character_13_GUARD_13,
  character_20_PRISONER_1,
  character_21_PRISONER_2,
  character_22_PRISONER_3,
  character_14_GUARD_14,
  character_15_GUARD_15,
  character_23_PRISONER_4,
  character_24_PRISONER_5,
  character_25_PRISONER_6
};

/* ----------------------------------------------------------------------- */

/**
 * $A289: Wake up.
 *
 * \param[in] state Pointer to game state.
 */
void wake_up(tgestate_t *state)
{
  characterstruct_t       *charstr; /* was HL */
  uint8_t                  iters;   /* was B */
  const roomdef_address_t *rda;     /* Conv: new */

  assert(state != NULL);

  if (state->hero_in_bed)
  {
    /* Hero gets out of bed. */
    state->vischars[0].mi.mappos.u = 46;
    state->vischars[0].mi.mappos.v = 46;
  }

  state->hero_in_bed = 0;

  static const route_t t42 = { routeindex_42_HUT2_LEFT_TO_RIGHT, 0 }; /* was BC */
  set_hero_route(state, &t42);

  /* Position all six prisoners. */
  charstr = &state->character_structs[character_20_PRISONER_1];
  iters = 3;
  do
  {
    charstr->room = room_3_HUT2RIGHT;
    charstr++;
  }
  while (--iters);
  iters = 3;
  do
  {
    charstr->room = room_5_HUT3RIGHT;
    charstr++;
  }
  while (--iters);

  route_t t5 = { routeindex_5_EXIT_HUT2, 0 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t5);

  /* Update all the bed objects to be empty. */
  rda = &beds[0];
  iters = beds_LENGTH; /* BUG: Conv: Original code uses 7 which is wrong. */
  do
  {
    set_roomdef(state,
                rda->room_index,
                rda->offset,
                interiorobject_EMPTY_BED_FACING_SE);
    rda++;
  }
  while (--iters);

  /* Update the hero's bed object to be empty and redraw if required. */
  set_roomdef(state,
              room_2_HUT2LEFT,
              roomdef_2_BED,
              interiorobject_EMPTY_BED_FACING_SE);
  if (state->room_index != room_0_OUTDOORS && state->room_index < room_6)
  {
    /* FUTURE: Could replace with call to setup_room_and_plot. */
    setup_room(state);
    plot_interior_tiles(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $A2E2: End of breakfast time.
 *
 * \param[in] state Pointer to game state.
 */
void end_of_breakfast(tgestate_t *state)
{
  characterstruct_t *charstr;  /* was HL */
  uint8_t            iters;    /* was B */

  assert(state != NULL);

  if (state->hero_in_breakfast)
  {
    state->vischars[0].mi.mappos.u = 52;
    state->vischars[0].mi.mappos.v = 62;
    state->hero_in_breakfast = 0; /* Conv: Moved into if block. */
  }

  static const route_t t = { routeindex_16_BREAKFAST_25 | ROUTEINDEX_REVERSE_FLAG, 3 }; /* was BC */
  set_hero_route(state, &t);

  charstr = &state->character_structs[character_20_PRISONER_1];
  iters = 3;
  do
  {
    charstr->room = room_25_MESS_HALL;
    charstr++;
  }
  while (--iters);
  iters = 3;
  do
  {
    charstr->room = room_23_MESS_HALL;
    charstr++;
  }
  while (--iters);

  route_t t2 = { routeindex_16_BREAKFAST_25 | ROUTEINDEX_REVERSE_FLAG, 3 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t2);

  /* Update all the benches to be empty. */
  set_roomdef(state, room_23_MESS_HALL, roomdef_23_BENCH_A, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_23_MESS_HALL, roomdef_23_BENCH_B, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_23_MESS_HALL, roomdef_23_BENCH_C, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_25_MESS_HALL, roomdef_25_BENCH_D, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_25_MESS_HALL, roomdef_25_BENCH_E, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_25_MESS_HALL, roomdef_25_BENCH_F, interiorobject_EMPTY_BENCH);
  set_roomdef(state, room_25_MESS_HALL, roomdef_25_BENCH_G, interiorobject_EMPTY_BENCH);

  /* Redraw current room if the game is showing an affected scene. */
  if (state->room_index >= room_1_HUT1RIGHT &&
      state->room_index <= room_28_HUT1LEFT)
  {
    // FUTURE: Replace this with a call to setup_room_and_plot.
    setup_room(state);
    plot_interior_tiles(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $A33F: Set the hero's route, unless in solitary.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] route  Route.                (was BC)
 */
void set_hero_route(tgestate_t *state, const route_t *route)
{
  assert(state != NULL);
  ASSERT_ROUTE_VALID(*route);

  if (state->in_solitary)
    return; /* Ignore. */

  set_hero_route_force(state, route);
}

/**
 * $A344: Set the hero's route, even if solitary.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] route  Route.                (was BC)
 */
void set_hero_route_force(tgestate_t *state, const route_t *route)
{
  vischar_t *vischar; /* was HL */

  assert(state != NULL);
  ASSERT_ROUTE_VALID(*route);

  vischar = &state->vischars[0];

  vischar->flags &= ~vischar_FLAGS_TARGET_IS_DOOR;
  vischar->route = *route;
  set_route(state, vischar);
}

/* ----------------------------------------------------------------------- */

/**
 * $A351: Go to time for bed.
 *
 * \param[in] state Pointer to game state.
 */
void go_to_time_for_bed(tgestate_t *state)
{
  assert(state != NULL);

  static const route_t t5 = { routeindex_5_EXIT_HUT2 | ROUTEINDEX_REVERSE_FLAG, 2 }; /* was BC */
  set_hero_route(state, &t5);

  route_t t5_also = { routeindex_5_EXIT_HUT2 | ROUTEINDEX_REVERSE_FLAG, 2 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t5_also);
}

/* ----------------------------------------------------------------------- */

/**
 * $A35F: Set the route for all prisoners and guards.
 *
 * Set a different route for each character.
 *
 * Called by go_to_roll_call.
 *
 * \param[in]     state  Pointer to game state.
 * \param[in,out] proute Pointer to route. (was C,A')
 */
void set_prisoners_and_guards_route(tgestate_t *state, route_t *proute)
{
  route_t            route;  /* new var */
  const character_t *pchars; /* was HL */
  uint8_t            iters;  /* was B */

  assert(state != NULL);
  ASSERT_ROUTE_VALID(*proute);

  route = *proute; /* Conv: Keep a local copy of counter. */

  pchars = &prisoners_and_guards[0];
  iters = NELEMS(prisoners_and_guards);
  do
  {
    set_character_route(state, *pchars, route);
    route.index++;
    pchars++;
  }
  while (--iters);

  *proute = route; // FUTURE: Discard. This passes a route back but none of the callers ever use it.
}

/* ----------------------------------------------------------------------- */

/**
 * $A373: Set the route for all prisoners and guards.
 *
 * Set the same route for each half of the group.
 *
 * Called by the set_route routines.
 *
 * \param[in]     state  Pointer to game state.
 * \param[in,out] proute Pointer to route. (was C,A')
 */
void set_prisoners_and_guards_route_B(tgestate_t *state, route_t *proute)
{
  route_t            route; /* new var */
  const character_t *pchars; /* was HL */
  uint8_t            iters;  /* was B */

  assert(state != NULL);
  ASSERT_ROUTE_VALID(*proute);

  route = *proute; /* Conv: Keep a local copy of counter. */

  pchars = &prisoners_and_guards[0];
  iters = NELEMS(prisoners_and_guards);
  do
  {
    set_character_route(state, *pchars, route);

    /* When this is 6, the character being processed is
     * character_22_PRISONER_3 and the next is character_14_GUARD_14: the
     * start of the second half of the list. */
    if (iters == 6)
      route.index++;

    pchars++;
  }
  while (--iters);

  *proute = route; // FUTURE: Discard. This passes a route back but none of the callers ever use it.
}

/* ----------------------------------------------------------------------- */

/**
 * $A38C: Set the route for a character.
 *
 * Finds a charstruct, or a vischar, and stores a route.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] character Character index.       (was A)
 * \param[in] route     Route.                 (was A' lo + C hi)
 */
void set_character_route(tgestate_t *state,
                         character_t character,
                         route_t     route)
{
  characterstruct_t *charstr; /* was HL */
  vischar_t         *vischar; /* was HL */
  uint8_t            iters;   /* was B */

  assert(state != NULL);
  ASSERT_CHARACTER_VALID(character);
  ASSERT_ROUTE_VALID(route);

  charstr = &state->character_structs[character];
  if (charstr->character_and_flags & characterstruct_FLAG_ON_SCREEN)
  {
    assert(character == (charstr->character_and_flags & characterstruct_CHARACTER_MASK));

    // Why fetch this copy of character index, is it not the same as the passed-in character?
    // might just be a re-fetch when the original code clobbers register A.
    character = charstr->character_and_flags & characterstruct_CHARACTER_MASK;

    /* Search non-player characters to see if this character is already on-screen. */
    iters = vischars_LENGTH - 1;
    vischar = &state->vischars[1];
    do
    {
      if (character == vischar->character)
        goto found_on_screen; /* Character is on-screen: store to vischar. */
      vischar++;
    }
    while (--iters);

    return; /* Conv: Was goto exit; */
  }

  /* Store to characterstruct only. */
  charstr->route = route;
  return;

  // FUTURE: Move this chunk into the body of the loop above.
found_on_screen:
  vischar->flags &= ~vischar_FLAGS_TARGET_IS_DOOR;
  vischar->route = route;

  set_route(state, vischar); // was fallthrough
}

/**
 * $A3BB: set_route.
 *
 * Called by set_character_route, set_hero_route.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was HL) (e.g. $8003 in original)
 */
void set_route(tgestate_t *state, vischar_t *vischar)
{
  uint8_t          get_target_result; /* was A */
  mappos8_t       *target;            /* was DE */
  const mappos8_t *doormappos;        /* was HL */
  const pos8_t    *location;          /* was HL */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  state->entered_move_a_character = 0;

  // sampled HL = $8003 $8043 $8023 $8063 $8083 $80A3

#ifdef DEBUG_ROUTES
  printf("(%s) set_route > get_target(route=%d%s step=%d)\n",
         (vischar == &state->vischars[0]) ? "hero" : "non-hero",
         vischar->route.index & ~ROUTEINDEX_REVERSE_FLAG,
         (vischar->route.index & ROUTEINDEX_REVERSE_FLAG) ? " reversed" : "",
         vischar->route.step);
#endif

  get_target_result = get_target(state,
                                 &vischar->route,
                                 &doormappos,
                                 &location);

  /* Set the target coordinates. */
  target = &vischar->target;
  if (get_target_result == get_target_LOCATION)
  {
#ifdef DEBUG_ROUTES
    if (vischar == &state->vischars[0])
      printf("(hero) get_target returned location (%d,%d)\n",
             location->x, location->y);
#endif

    target->u = location->x;
    target->v = location->y;
  }
  else if (get_target_result == get_target_DOOR)
  {
#ifdef DEBUG_ROUTES
    if (vischar == &state->vischars[0])
      printf("(hero) get_target returned door (%d,%d)\n",
             doorpos->x, doorpos->y);
#endif

    target->u = doormappos->u;
    target->v = doormappos->v;
  }
  else
  {
#ifdef DEBUG_ROUTES
    if (vischar == &state->vischars[0])
      printf("(hero) get_target returned route ends\n");
#endif
  }

  if (get_target_result == get_target_ROUTE_ENDS)
  {
    state->IY = vischar;
    get_target_assign_pos(state, vischar, &vischar->route);
  }
  else if (get_target_result == get_target_DOOR)
  {
    vischar->flags |= vischar_FLAGS_TARGET_IS_DOOR;
  }
}

/* ----------------------------------------------------------------------- */

/* Conv: $A3ED/store_route was inlined. */

/* ----------------------------------------------------------------------- */

/**
 * $A3F3: Send a character to bed.
 *
 * (entered_move_a_character is non-zero.)
 *
 * \param[in] state Pointer to game state.
 * \param[in] route Pointer to route. (was HL)
 */
void character_bed_state(tgestate_t *state, route_t *route)
{
  assert(state != NULL);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  character_bed_common(state, state->character_index, route);
}

/**
 * $A3F8: entered_move_a_character is zero.
 *
 * Gets hit when hero enters hut at end of day.
 *
 * \param[in] state Pointer to game state.
 * \param[in] route Pointer to route. (was HL)
 */
void character_bed_vischar(tgestate_t *state, route_t *route)
{
  character_t character;
  vischar_t  *vischar;

  assert(state != NULL);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  vischar = state->IY;
  ASSERT_VISCHAR_VALID(vischar);

  character = vischar->character;
  if (character == character_0_COMMANDANT)
  {
    // Hero moves to bed in reaction to the commandant...

    static const route_t t = { routeindex_44_HUT2_RIGHT_TO_LEFT, 0 }; /* was BC */ // route_hut2_right_to_left
    set_hero_route(state, &t);
  }
  else
  {
    character_bed_common(state, character, route); /* was fallthrough */
  }
}

/**
 * $A404: Use character index to assign a "walk to bed" route to the specified character.
 *
 * Common end of above two routines.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] character Character index.  (was A)
 * \param[in] route     Pointer to route. (was HL)
 */
void character_bed_common(tgestate_t *state,
                          character_t character,
                          route_t    *route)
{
  uint8_t routeindex; /* was A */

  assert(state != NULL);
  ASSERT_CHARACTER_VALID(character);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  route->step = 0;

  if (character >= character_20_PRISONER_1)
  {
    /* All prisoners */

    // routeindex = 7..12 (route to bed) for prisoner 1..6
    routeindex = character - 13;
  }
  else
  {
    /* All hostiles */
    // which exactly?

    routeindex = 13;
    if (character & 1) /* Odd numbered characters? */
    {
      /* reverse route */
      route->step = 1;
      routeindex |= ROUTEINDEX_REVERSE_FLAG;
    }
  }

  route->index = routeindex;
}

/* ----------------------------------------------------------------------- */

/**
 * $A420: Character sits.
 *
 * This is called with x = 18,19,20,21,22.
 * - 18..20 go to room_25_MESS_HALL
 * - 21..22 go to room_23_MESS_HALL
 *
 * \param[in] state      Pointer to game state.
 * \param[in] routeindex Index of route.   (was A)
 * \param[in] route      Pointer to route. (was HL)
 */
void character_sits(tgestate_t *state,
                    uint8_t     routeindex,
                    route_t    *route)
{
  uint8_t  index;      /* was A */
  int      room_index; /* Conv: new */
  int      offset;     /* Conv: new */
  room_t   room;       /* was C */

  assert(state  != NULL);
  assert(routeindex >= routeindex_18_PRISONER_SITS_1 &&
         routeindex <= routeindex_23_PRISONER_SITS_3);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  index = routeindex - routeindex_18_PRISONER_SITS_1;
  /* First three characters. */
  room_index = room_25_MESS_HALL;
  offset     = roomdef_25_BENCH_D;
  if (index >= 3)
  {
    /* Last two characters. */
    room_index = room_23_MESS_HALL;
    offset     = roomdef_23_BENCH_A;
    index -= 3;
  }
  set_roomdef(state,
              room_index,
              offset + index * 3,
              interiorobject_PRISONER_SAT_MID_TABLE);

  if (routeindex < routeindex_21_PRISONER_SITS_1)
    room = room_25_MESS_HALL;
  else
    room = room_23_MESS_HALL;

  character_sit_sleep_common(state, room, route);
}

/**
 * $A444: Character sleeps.
 *
 * This is called with x = 7,8,9,10,11,12.
 * -  7..9  go to room_3_HUT2RIGHT
 * - 10..12 go to room_5_HUT3RIGHT
 *
 * \param[in] state      Pointer to game state.
 * \param[in] routeindex                   (was A)
 * \param[in] route      Pointer to route. (was HL)
 */
void character_sleeps(tgestate_t *state,
                      uint8_t     routeindex,
                      route_t    *route)
{
  room_t room; /* was C */

  assert(state  != NULL);
  assert(routeindex >= routeindex_7_PRISONER_SLEEPS_1 &&
         routeindex <= routeindex_12_PRISONER_SLEEPS_3);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  /* Poke object. */
  set_roomdef(state,
              beds[routeindex - 7].room_index,
              beds[routeindex - 7].offset,
              interiorobject_OCCUPIED_BED);

  if (routeindex < routeindex_10_PRISONER_SLEEPS_1)
    room = room_3_HUT2RIGHT;
  else
    room = room_5_HUT3RIGHT;

  character_sit_sleep_common(state, room, route); // was fallthrough
}

/**
 * $A462: Make characters disappear, repainting the screen if required.
 *
 * \param[in] state Pointer to game state.
 * \param[in] room  Room index.            (was C)
 * \param[in] route Route - used to determine either a vischar or a characterstruct depending on if the room specified is the current room. (was DE/HL)
 */
void character_sit_sleep_common(tgestate_t *state,
                                room_t      room,
                                route_t    *route)
{
  /* This receives a pointer to a route structure which is within either a
   * characterstruct or a vischar. */

  assert(state != NULL);
  ASSERT_ROOM_VALID(room);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  // sampled HL =
  // breakfast $8022 + + $76B8 $76BF
  // night     $76A3 $76B8 $76C6 $76B1 $76BF $76AA
  // (not always in the same order)
  //
  // $8022 -> vischar[1]->route
  // others -> character_structs->route

  route->index = routeindex_0_HALT; /* Stand still. */

  if (state->room_index != room)
  {
    /* Character is sitting or sleeping in a room presently not visible. */

    characterstruct_t *cs;

    /* Retrieve the parent structure pointer. */
    cs = structof(route, characterstruct_t, route);
    cs->room = room_NONE;
  }
  else
  {
    /* Character is visible - force a repaint. */

    vischar_t *vc;

    /* This is only ever hit when location is in vischar @ $8020. */

    /* Retrieve the parent structure pointer. */
    vc = structof(route, vischar_t, route);
    vc->room = room_NONE; // hides but doesn't disable the character?

    setup_room_and_plot(state);
  }
}

/**
 * $A479: Setup room and plot.
 *
 * \param[in] state Pointer to game state.
 */
void setup_room_and_plot(tgestate_t *state)
{
  assert(state != NULL);

  setup_room(state);
  plot_interior_tiles(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A47F: The hero sits.
 *
 * \param[in] state Pointer to game state.
 */
void hero_sits(tgestate_t *state)
{
  assert(state != NULL);

  set_roomdef(state,
              room_25_MESS_HALL,
              roomdef_25_BENCH_G,
              interiorobject_PRISONER_SAT_END_TABLE);
  hero_sit_sleep_common(state, &state->hero_in_breakfast);
}

/**
 * $A489: The hero sleeps.
 *
 * \param[in] state Pointer to game state.
 */
void hero_sleeps(tgestate_t *state)
{
  assert(state != NULL);

  set_roomdef(state,
              room_2_HUT2LEFT,
              roomdef_2_BED,
              interiorobject_OCCUPIED_BED);
  hero_sit_sleep_common(state, &state->hero_in_bed);
}

/**
 * $A498: Common end of hero_sits/sleeps.
 *
 * \param[in] state Pointer to game state.
 * \param[in] pflag Pointer to hero_in_breakfast or hero_in_bed. (was HL)
 */
void hero_sit_sleep_common(tgestate_t *state, uint8_t *pflag)
{
  assert(state != NULL);
  assert(pflag != NULL);

  /* Set hero_in_breakfast or hero_in_bed flag. */
  *pflag = 255;

  /* Reset only the route index. */
  state->vischars[0].route.index = routeindex_0_HALT; /* Stand still. */

  /* Set hero position (x,y) to zero. */
  state->vischars[0].mi.mappos.u = 0;
  state->vischars[0].mi.mappos.v = 0;

  calc_vischar_isopos_from_vischar(state, &state->vischars[0]);

  setup_room_and_plot(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A4A9: Set "go to yard" route.
 *
 * \param[in] state Pointer to game state.
 */
void set_route_go_to_yard(tgestate_t *state)
{
  assert(state != NULL);

  static const route_t t14 = { routeindex_14_GO_TO_YARD, 0 };
  set_hero_route(state, &t14);

  route_t t14_also = { routeindex_14_GO_TO_YARD, 0 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t14_also);
}

/**
 * $A4B7: Set "go to yard" route reversed.
 *
 * \param[in] state Pointer to game state.
 */
void set_route_go_to_yard_reversed(tgestate_t *state)
{
  assert(state != NULL);

  static const route_t t14 = { routeindex_14_GO_TO_YARD | ROUTEINDEX_REVERSE_FLAG, 4 };
  set_hero_route(state, &t14);

  route_t t14_also = { routeindex_14_GO_TO_YARD | ROUTEINDEX_REVERSE_FLAG, 4 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t14_also);
}

/**
 * $A4C5: Set route 16.
 *
 * \param[in] state Pointer to game state.
 */
void set_route_go_to_breakfast(tgestate_t *state)
{
  assert(state != NULL);

  static const route_t t16 = { routeindex_16_BREAKFAST_25, 0 };
  set_hero_route(state, &t16);

  route_t t16_also = { routeindex_16_BREAKFAST_25, 0 }; /* was A',C */
  set_prisoners_and_guards_route_B(state, &t16_also);
}

/* ----------------------------------------------------------------------- */

/**
 * $A4D3: entered_move_a_character is non-zero (another one).
 *
 * Very similar to the routine at $A3F3.
 *
 * \param[in] state Pointer to game state.
 * \param[in] route Pointer to character struct. (was HL)
 */
void charevnt_breakfast_state(tgestate_t *state, route_t *route)
{
  assert(state != NULL);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  charevnt_breakfast_common(state, state->character_index, route);
}

/**
 * $A4D8: entered_move_a_character is zero (another one).
 *
 * \param[in] state Pointer to game state.
 * \param[in] route Pointer to route. (was HL)
 */
void charevnt_breakfast_vischar(tgestate_t *state, route_t *route)
{
  character_t character;
  vischar_t  *vischar;

  assert(state != NULL);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  vischar = state->IY;
  ASSERT_VISCHAR_VALID(vischar);

  character = vischar->character;
  if (character == character_0_COMMANDANT)
  {
    static const route_t t = { routeindex_43_7833, 0 }; /* was BC */
    set_hero_route(state, &t);
  }
  else
  {
    charevnt_breakfast_common(state, character, route); /* was fallthrough */
  }
}

/**
 * $A4E4: Common end of above two routines.
 *
 * Start of breakfast: sets routes for prisoners and guards.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] character Character index.       (was A)
 * \param[in] route     Pointer to route.      (was HL)
 */
void charevnt_breakfast_common(tgestate_t *state,
                               character_t character,
                               route_t    *route)
{
  uint8_t routeindex; /* was A */

  assert(state != NULL);
  ASSERT_CHARACTER_VALID(character);
  assert(route != NULL);
  ASSERT_ROUTE_VALID(*route);

  route->step = 0;

  if (character >= character_20_PRISONER_1)
  {
    /* Prisoners 1..6 take routes 18..23 (routes to sitting). */
    routeindex = character - 2;
  }
  else
  {
    /* Guards take route 24 if even, or 25 if odd. */
    routeindex = 24;
    if (character & 1)
      routeindex++;
  }

  route->index = routeindex;
}

/* ----------------------------------------------------------------------- */

/**
 * $A4FD: Go to roll call.
 *
 * \param[in] state Pointer to game state.
 */
void go_to_roll_call(tgestate_t *state)
{
  assert(state != NULL);

  route_t t26 = { routeindex_26_GUARD_12_ROLL_CALL, 0 }; /* was C,A' */
  set_prisoners_and_guards_route(state, &t26);

  static const route_t t45 = { routeindex_45_HERO_ROLL_CALL, 0 };
  set_hero_route(state, &t45);
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et
