/**
 * Events.h
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

#ifndef EVENTS_H
#define EVENTS_H

#include "TheGreatEscape/State.h"

void dispatch_timed_event(tgestate_t *state);

timedevent_handler_t event_night_time;
timedevent_handler_t event_another_day_dawns;
void set_day_or_night(tgestate_t *state, uint8_t day_night);
timedevent_handler_t event_wake_up;
timedevent_handler_t event_go_to_roll_call;
timedevent_handler_t event_go_to_breakfast_time;
timedevent_handler_t event_end_of_breakfast;
timedevent_handler_t event_go_to_exercise_time;
timedevent_handler_t event_exercise_time;
timedevent_handler_t event_go_to_time_for_bed;
timedevent_handler_t event_new_red_cross_parcel;
timedevent_handler_t event_time_for_bed;
timedevent_handler_t event_search_light;
void set_guards_route(tgestate_t *state, route_t route);

void wake_up(tgestate_t *state);
void end_of_breakfast(tgestate_t *state);

void set_hero_route(tgestate_t *state, const route_t *route);
void set_hero_route_force(tgestate_t *state, const route_t *route);

void go_to_time_for_bed(tgestate_t *state);

void set_prisoners_and_guards_route(tgestate_t *state, route_t *route);
void set_prisoners_and_guards_route_B(tgestate_t *state, route_t *route);
void set_character_route(tgestate_t *state,
                         character_t character,
                         route_t     route);
void set_route(tgestate_t *state, vischar_t *vischar);

void character_bed_state(tgestate_t *state,
                         route_t    *route);
void character_bed_vischar(tgestate_t *state,
                           route_t    *route);
void character_bed_common(tgestate_t *state,
                          character_t character,
                          route_t    *route);

void character_sits(tgestate_t *state,
                    uint8_t     routeindex,
                    route_t    *route);
void character_sleeps(tgestate_t *state,
                      uint8_t     routeindex,
                      route_t   *route);
void character_sit_sleep_common(tgestate_t *state,
                                room_t      room,
                                route_t    *route);
void setup_room_and_plot(tgestate_t *state);

void hero_sits(tgestate_t *state);
void hero_sleeps(tgestate_t *state);
void hero_sit_sleep_common(tgestate_t *state, uint8_t *pflag);

void set_route_go_to_yard(tgestate_t *state);
void set_route_go_to_yard_reversed(tgestate_t *state);
void set_route_go_to_breakfast(tgestate_t *state);

void charevnt_breakfast_state(tgestate_t *state,
                              route_t    *route);
void charevnt_breakfast_vischar(tgestate_t *state,
                                route_t    *route);
void charevnt_breakfast_common(tgestate_t  *state,
                               character_t  character,
                               route_t     *route);

void go_to_roll_call(tgestate_t *state);

#endif /* EVENTS_H */

// vim: ts=8 sts=2 sw=2 et
