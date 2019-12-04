/**
 * Main.h
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

#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>

#include "TheGreatEscape/TheGreatEscape.h"

#include "TheGreatEscape/Asserts.h"
#include "TheGreatEscape/Doors.h"
#include "TheGreatEscape/InteriorObjects.h"
#include "TheGreatEscape/Routes.h"
#include "TheGreatEscape/StaticGraphics.h"
#include "TheGreatEscape/Types.h"

/* ----------------------------------------------------------------------- */

// FORWARD REFERENCES
//
// (in original file order)

/* $6000 onwards */

void transition(tgestate_t *state, const mappos8_t *mappos);
void enter_room(tgestate_t *state);
INLINE void squash_stack_goto_main(tgestate_t *state);

void set_hero_sprite_for_room(tgestate_t *state);

void setup_movable_items(tgestate_t *state);
void setup_movable_item(tgestate_t          *state,
                        const movableitem_t *movableitem,
                        character_t          character);

void reset_nonplayer_visible_characters(tgestate_t *state);

void setup_doors(tgestate_t *state);

const door_t *get_door(doorindex_t index);

void wipe_visible_tiles(tgestate_t *state);

void setup_room(tgestate_t *state);

void expand_object(tgestate_t *state, object_t index, uint8_t *output);

void plot_interior_tiles(tgestate_t *state);

/* $7000 onwards */

extern const door_t doors[door_MAX * 2];

void process_player_input_fire(tgestate_t *state, input_t input);
void use_item_B(tgestate_t *state);
void use_item_A(tgestate_t *state);
void use_item_common(tgestate_t *state, item_t item);

void pick_up_item(tgestate_t *state);
void drop_item(tgestate_t *state);
void drop_item_tail(tgestate_t *state, item_t item);
void calc_exterior_item_isopos(itemstruct_t *itemstr);
void calc_interior_item_isopos(itemstruct_t *itemstr);

void draw_all_items(tgestate_t *state);
void draw_item(tgestate_t *state, item_t item, size_t dstoff);

itemstruct_t *find_nearby_item(tgestate_t *state);

void plot_bitmap(tgestate_t    *state,
                 const uint8_t *src,
                 uint8_t       *dst,
                 uint8_t        width,
                 uint8_t        height);

void screen_wipe(tgestate_t *state,
                 uint8_t    *dst,
                 uint8_t     width,
                 uint8_t     height);

uint8_t *get_next_scanline(tgestate_t *state, uint8_t *slp);

/* $8000 onwards */

/* $9000 onwards */

void main_loop(tgestate_t *state);

void check_morale(tgestate_t *state);

void keyscan_break(tgestate_t *state);

void process_player_input(tgestate_t *state);

void picking_lock(tgestate_t *state);

void cutting_wire(tgestate_t *state);

void in_permitted_area(tgestate_t *state);
int in_permitted_area_end_bit(tgestate_t *state, uint8_t room_and_flags);
int within_camp_bounds(uint8_t index, const mappos8_t *mappos);

/* $A000 onwards */

void wave_morale_flag(tgestate_t *state);

void set_morale_flag_screen_attributes(tgestate_t *state, attribute_t attrs);

uint8_t *get_prev_scanline(tgestate_t *state, uint8_t *addr);

void interior_delay_loop(tgestate_t *state);

void ring_bell(tgestate_t *state);
void plot_ringer(tgestate_t *state, const uint8_t *src);

void increase_morale(tgestate_t *state, uint8_t delta);
void decrease_morale(tgestate_t *state, uint8_t delta);
void increase_morale_by_10_score_by_50(tgestate_t *state);
void increase_morale_by_5_score_by_5(tgestate_t *state);

void increase_score(tgestate_t *state, uint8_t delta);

void plot_score(tgestate_t *state);

void play_speaker(tgestate_t *state, sound_t sound);

void set_game_window_attributes(tgestate_t *state, attribute_t attrs);

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

void screen_reset(tgestate_t *state);

void escaped(tgestate_t *state);

uint8_t keyscan_all(tgestate_t *state);

INLINE escapeitem_t item_to_escapeitem(item_t item);

const screenlocstring_t *screenlocstring_plot(tgestate_t              *state,
                                              const screenlocstring_t *slstring);

void get_supertiles(tgestate_t *state);

void plot_bottommost_tiles(tgestate_t *state);
void plot_topmost_tiles(tgestate_t *state);
void plot_horizontal_tiles_common(tgestate_t             *state,
                                  tileindex_t            *vistiles,
                                  const supertileindex_t *maptiles,
                                  uint8_t                 y,
                                  uint8_t                *window);

void plot_all_tiles(tgestate_t *state);

void plot_rightmost_tiles(tgestate_t *state);
void plot_leftmost_tiles(tgestate_t *state);
void plot_vertical_tiles_common(tgestate_t             *state,
                                tileindex_t            *vistiles,
                                const supertileindex_t *maptiles,
                                uint8_t                 x,
                                uint8_t                *window);

INLINE uint8_t *plot_tile_then_advance(tgestate_t             *state,
                                       tileindex_t             tile_index,
                                       const supertileindex_t *psupertileindex,
                                       uint8_t                *scr);

uint8_t *plot_tile(tgestate_t             *state,
                   tileindex_t             tile_index,
                   const supertileindex_t *psupertileindex,
                   uint8_t                *scr);

void shunt_map_left(tgestate_t *state);
void shunt_map_right(tgestate_t *state);
void shunt_map_up_right(tgestate_t *state);
void shunt_map_up(tgestate_t *state);
void shunt_map_down(tgestate_t *state);
void shunt_map_down_left(tgestate_t *state);

void move_map(tgestate_t *state);

void move_map_up_left(tgestate_t *state, uint8_t *pmove_map_y);
void move_map_up_right(tgestate_t *state, uint8_t *pmove_map_y);
void move_map_down_right(tgestate_t *state, uint8_t *pmove_map_y);
void move_map_down_left(tgestate_t *state, uint8_t *pmove_map_y);

attribute_t choose_game_window_attributes(tgestate_t *state);

void zoombox(tgestate_t *state);
void zoombox_fill(tgestate_t *state);
void zoombox_draw_border(tgestate_t *state);
void zoombox_draw_tile(tgestate_t     *state,
                       zoombox_tile_t  index,
                       uint8_t        *addr);

void nighttime(tgestate_t *state);
void searchlight_movement(searchlight_movement_t *slstate);
void searchlight_caught(tgestate_t                *state,
                        const searchlight_movement_t *slstate);
void searchlight_plot(tgestate_t  *state,
                      attribute_t *attrs,
                      int          clip_left);

int touch(tgestate_t *state, vischar_t *vischar, spriteindex_t sprite_index);

int collision(tgestate_t *state);

/* $B000 onwards */

void accept_bribe(tgestate_t *state);

int bounds_check(tgestate_t *state, vischar_t *vischar);

int is_door_locked(tgestate_t *state);

void door_handling(tgestate_t *state, vischar_t *vischar);

int door_in_range(tgestate_t *state, const door_t *door);

int interior_bounds_check(tgestate_t *state, vischar_t *vischar);

void reset_outdoors(tgestate_t *state);

void door_handling_interior(tgestate_t *state, vischar_t *vischar);

void action_red_cross_parcel(tgestate_t *state);
void action_bribe(tgestate_t *state);
void action_poison(tgestate_t *state);
void action_uniform(tgestate_t *state);
void action_shovel(tgestate_t *state);
void action_lockpick(tgestate_t *state);
void action_red_key(tgestate_t *state);
void action_yellow_key(tgestate_t *state);
void action_green_key(tgestate_t *state);
void action_key(tgestate_t *state, room_t room_of_key);

doorindex_t *get_nearest_door(tgestate_t *state);

void action_wiresnips(tgestate_t *state);

extern const wall_t walls[24];

void animate(tgestate_t *state);

void calc_vischar_isopos_from_vischar(tgestate_t *state, vischar_t *vischar);
void calc_vischar_isopos_from_state(tgestate_t *state, vischar_t *vischar);

void reset_game(tgestate_t *state);
void reset_map_and_characters(tgestate_t *state);

void searchlight_mask_test(tgestate_t *state, vischar_t *vischar);

void plot_sprites(tgestate_t *state);

int get_next_drawable(tgestate_t    *state,
                      uint8_t       *pindex,
                      vischar_t    **pvischar,
                      itemstruct_t **pitemstruct);

void render_mask_buffer(tgestate_t *state);

uint16_t multiply(uint8_t left, uint8_t right);

void mask_against_tile(tileindex_t index, tilerow_t *dst);

int vischar_visible(tgestate_t      *state,
                    const vischar_t *vischar,
                    uint8_t         *left_skip,
                    uint8_t         *clipped_width,
                    uint8_t         *top_skip,
                    uint8_t         *clipped_height);

void restore_tiles(tgestate_t *state);

const tile_t *select_tile_set(tgestate_t *state,
                              uint8_t     x_shift,
                              uint8_t     y_shift);

/* $C000 onwards */

void spawn_characters(tgestate_t *state);

void purge_invisible_characters(tgestate_t *state);

void spawn_character(tgestate_t *state, characterstruct_t *charstr);

void reset_visible_character(tgestate_t *state, vischar_t *vischar);

#define get_target_LOCATION   0
#define get_target_DOOR       128
#define get_target_ROUTE_ENDS 255

uint8_t get_target(tgestate_t       *state,
                   route_t          *route,
                   const mappos8_t **doormappos,
                   const pos8_t    **location);

void move_a_character(tgestate_t *state);

int move_towards(int8_t         max,
                 int            rc,
                 const uint8_t *second,
                 uint8_t       *first);

void character_event(tgestate_t *state, route_t *route);

charevnt_handler_t charevnt_solitary_ends;
charevnt_handler_t charevnt_commandant_to_yard;
charevnt_handler_t charevnt_hero_release;
charevnt_handler_t charevnt_wander_left;
charevnt_handler_t charevnt_wander_yard;
charevnt_handler_t charevnt_wander_top;
charevnt_handler_t charevnt_bed;
charevnt_handler_t charevnt_breakfast;
charevnt_handler_t charevnt_exit_hut2;
charevnt_handler_t charevnt_hero_sits;
charevnt_handler_t charevnt_hero_sleeps;

void automatics(tgestate_t *state);

void character_behaviour(tgestate_t *state,
                         vischar_t  *vischar);
void character_behaviour_set_input(tgestate_t *state,
                                   vischar_t  *vischar,
                                   uint8_t     new_input);
void character_behaviour_move_y_dominant(tgestate_t *state,
                                         vischar_t  *vischar,
                                         int         scale);

input_t vischar_move_u(tgestate_t *state,
                       vischar_t  *vischar,
                       int         scale);

input_t vischar_move_v(tgestate_t *state,
                       vischar_t  *vischar,
                       int         scale);

void target_reached(tgestate_t *state, vischar_t *vischar);

void get_target_assign_pos(tgestate_t *state,
                           vischar_t  *vischar,
                           route_t    *route);
void route_ended(tgestate_t *state, vischar_t *vischar, route_t *route);

/** Byte which terminates a route. */
#define routebyte_END 255
INLINE const uint8_t *get_route(routeindex_t A);

uint8_t random_nibble(tgestate_t *state);

void solitary(tgestate_t *state);

void guards_follow_suspicious_character(tgestate_t *state,
                                        vischar_t  *vischar);

void hostiles_pursue(tgestate_t *state);

void is_item_discoverable(tgestate_t *state);

int is_item_discoverable_interior(tgestate_t *state,
                                  room_t      room,
                                  item_t     *pitem);

void item_discovered(tgestate_t *state, item_t item);

extern const default_item_location_t default_item_locations[item__LIMIT];

extern const character_class_data_t character_class_data[4];

#define animations__LIMIT 24
extern const anim_t *animations[animations__LIMIT];

/* $D000 onwards */

void mark_nearby_items(tgestate_t *state);

uint8_t get_next_drawable_itemstruct(tgestate_t    *state,
                                     item_t         item_and_flag,
                                     uint16_t       x,
                                     uint16_t       y,
                                     itemstruct_t **pitemstr);

uint8_t setup_item_plotting(tgestate_t   *state,
                            itemstruct_t *itemstr,
                            item_t        item);

uint8_t item_visible(tgestate_t *state,
                     uint8_t    *left_skip,
                     uint8_t    *clipped_width,
                     uint8_t    *top_skip,
                     uint8_t    *clipped_height);

extern const spritedef_t item_definitions[item__LIMIT];

/* $E000 onwards */

extern const size_t masked_sprite_plotter_16_enables[2 * 3];

void masked_sprite_plotter_24_wide_vischar(tgestate_t *state,
                                           vischar_t  *vischar);

void masked_sprite_plotter_16_wide_item(tgestate_t *state);
void masked_sprite_plotter_16_wide_vischar(tgestate_t *state,
                                           vischar_t  *vischar);
void masked_sprite_plotter_16_wide_left(tgestate_t *state, uint8_t x);
void masked_sprite_plotter_16_wide_right(tgestate_t *state, uint8_t x);

void flip_24_masked_pixels(tgestate_t *state,
                           uint8_t    *pE,
                           uint8_t    *pC,
                           uint8_t    *pB,
                           uint8_t    *pEdash,
                           uint8_t    *pCdash,
                           uint8_t    *pBdash);

void flip_16_masked_pixels(tgestate_t *state,
                           uint8_t    *pD,
                           uint8_t    *pE,
                           uint8_t    *pDdash,
                           uint8_t    *pEdash);

int setup_vischar_plotting(tgestate_t *state, vischar_t *vischar);

void scale_mappos_down(const mappos16_t *in, mappos8_t *out);
INLINE void divide_by_8_with_rounding(uint8_t *A, uint8_t *C);
INLINE void divide_by_8(uint8_t *A, uint8_t *C);

void plot_game_window(tgestate_t *state);

timedevent_handler_t event_roll_call;

void action_papers(tgestate_t *state);

int user_confirm(tgestate_t *state);

/* $F000 onwards */

void wipe_full_screen_and_attributes(tgestate_t *state);

#endif /* MAIN_H */

// vim: ts=8 sts=2 sw=2 et
