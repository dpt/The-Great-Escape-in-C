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

void set_hero_route(tgestate_t *state, const route_t *route);
void set_hero_route_force(tgestate_t *state, const route_t *route);

void character_bed_state(tgestate_t *state, route_t *route);
void character_bed_vischar(tgestate_t *state, route_t *route);

void character_sits(tgestate_t *state, uint8_t routeindex, route_t *route);
void character_sleeps(tgestate_t *state, uint8_t routeindex, route_t *route);

void hero_sits(tgestate_t *state);
void hero_sleeps(tgestate_t *state);

void charevnt_breakfast_state(tgestate_t *state, route_t *route);
void charevnt_breakfast_vischar(tgestate_t *state, route_t *route);

#endif /* EVENTS_H */

// vim: ts=8 sts=2 sw=2 et
