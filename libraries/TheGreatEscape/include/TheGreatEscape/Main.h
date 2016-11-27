#ifndef THEGREATESCAPE_H
#define THEGREATESCAPE_H

#include "TheGreatEscape/TheGreatEscape.h"

#include "TheGreatEscape/Doors.h"
#include "TheGreatEscape/InteriorObjects.h"
#include "TheGreatEscape/StaticGraphics.h"
#include "TheGreatEscape/Types.h"

/* ----------------------------------------------------------------------- */

// these belong somewhere else

// state

#define ASSERT_SCREEN_PTR_VALID(p)                    \
do {                                                  \
  assert((p) >= &state->speccy->screen[0]);           \
  assert((p) < &state->speccy->screen[SCREEN_LENGTH]); \
} while (0)

#define ASSERT_SCREEN_ATTRIBUTES_PTR_VALID(p)         \
do {                                                  \
  assert((p) >= &state->speccy->attributes[0]);       \
  assert((p) < &state->speccy->attributes[SCREEN_ATTRIBUTES_LENGTH]); \
} while (0)

#define ASSERT_MASK_BUF_PTR_VALID(p)                  \
do {                                                  \
  assert((p) >= &state->mask_buffer[0]);              \
  assert((p) < &state->mask_buffer[MASK_BUFFER_LENGTH]); \
} while (0)

#define ASSERT_TILE_BUF_PTR_VALID(p)                  \
do {                                                  \
  assert((p) >= state->tile_buf);                     \
  assert((p) < state->tile_buf + (state->columns * state->rows)); \
} while (0)

#define ASSERT_WINDOW_BUF_PTR_VALID(p)                \
do {                                                  \
  assert((p) >= state->window_buf);                   \
  assert((p) < state->window_buf + (state->columns * state->rows * 8)); \
} while (0)

#define ASSERT_MAP_BUF_PTR_VALID(p)                   \
do {                                                  \
  assert((p) >= state->map_buf);                      \
  assert((p) < state->map_buf + (state->st_columns * state->st_rows)); \
} while (0)

#define ASSERT_VISCHAR_VALID(p)                       \
do {                                                  \
  assert((p) >= &state->vischars[0]);                 \
  assert((p) < &state->vischars[vischars_LENGTH]);    \
} while (0)

// non-state

#define ASSERT_CHARACTER_VALID(c)                             \
do {                                                          \
  assert(c >= 0 && c < character__LIMIT);                     \
} while (0)

#define ASSERT_ROOM_VALID(r)                                  \
do {                                                          \
  assert(r >= 0 && r < room__LIMIT);                          \
} while (0)

#define ASSERT_ITEM_VALID(i)                                  \
do {                                                          \
  assert(i >= 0 && i < item__LIMIT);                          \
} while (0)

#define ASSERT_INTERIOR_TILES_VALID(p)                                \
do {                                                                  \
  assert(p >= &interior_tiles[0].row[0]);                             \
  assert(p <= &interior_tiles[interiorobjecttile__LIMIT - 1].row[0]); \
} while (0)

#define ASSERT_DOORS_VALID(p)                                 \
do {                                                          \
  assert(p >= &state->doors[0]);                              \
  assert(p < &state->doors[4]);                               \
} while (0)

#define ASSERT_SUPERTILE_PTR_VALID(p)                            \
do {                                                             \
  assert(p >= &supertiles[0].tiles[0]);                          \
  assert(p <= &supertiles[supertileindex__LIMIT - 1].tiles[15]); \
} while (0)

#define ASSERT_MAP_PTR_VALID(p)                               \
do {                                                          \
  assert(p >= &map[0]);                                       \
  assert(p < &map[MAPX * MAPY]);                              \
} while (0)

// These limits were determined by checking the original game and cover the main map only. They'll need adjusting.
#define ASSERT_MAP_POSITION_VALID(p)                          \
do {                                                          \
  assert(p.x >= 0x00);                                        \
  assert(p.x < 0xBC);                                         \
  assert(p.y >= 0x06);                                        \
  assert(p.y < 0x78);                                         \
} while (0)

/* ----------------------------------------------------------------------- */

// FORWARD REFERENCES
//
// (in original file order)

#define INLINE __inline

/* $6000 onwards */

void transition(tgestate_t      *state,
                const tinypos_t *pos);
void enter_room(tgestate_t *state);
INLINE void squash_stack_goto_main(tgestate_t *state);

void set_hero_sprite_for_room(tgestate_t *state);

void setup_movable_items(tgestate_t *state);
void setup_movable_item(tgestate_t          *state,
                        const movableitem_t *movableitem,
                        character_t          character);

void reset_nonplayer_visible_characters(tgestate_t *state);

void setup_doors(tgestate_t *state);

const doorpos_t *get_door_position(doorindex_t door);

void wipe_visible_tiles(tgestate_t *state);

void setup_room(tgestate_t *state);

void expand_object(tgestate_t *state, object_t index, uint8_t *output);

void plot_interior_tiles(tgestate_t *state);

extern uint8_t *const beds[beds_LENGTH];

/* $7000 onwards */

extern /*const*/ doorpos_t door_positions[door_MAX * 2];

void process_player_input_fire(tgestate_t *state, input_t input);
void use_item_B(tgestate_t *state);
void use_item_A(tgestate_t *state);
void use_item_common(tgestate_t *state, item_t item);

void pick_up_item(tgestate_t *state);
void drop_item(tgestate_t *state);
void drop_item_tail(tgestate_t *state, item_t item);
void calc_exterior_item_screenpos(itemstruct_t *itemstr);
void calc_interior_item_screenpos(itemstruct_t *itemstr);

INLINE itemstruct_t *item_to_itemstruct(tgestate_t *state, item_t item);

void draw_all_items(tgestate_t *state);
void draw_item(tgestate_t *state, item_t item, size_t dstoff);

itemstruct_t *find_nearby_item(tgestate_t *state);

void plot_bitmap(tgestate_t    *state,
                 uint8_t        width,
                 uint8_t        height,
                 const uint8_t *src,
                 uint8_t       *dst);

void screen_wipe(tgestate_t *state,
                 uint8_t     width,
                 uint8_t     height,
                 uint8_t    *dst);

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
int within_camp_bounds(uint8_t          index,
                       const tinypos_t *pos);

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
void set_guards_target(tgestate_t *state, xy_t target);

extern const character_t prisoners_and_guards[10];

void wake_up(tgestate_t *state);
void end_of_breakfast(tgestate_t *state);

void set_hero_target(tgestate_t *state, xy_t target);

void go_to_time_for_bed(tgestate_t *state);

void set_prisoners_and_guards_target(tgestate_t *state, xy_t *target);
void set_prisoners_and_guards_target_B(tgestate_t *state, xy_t *target);
void set_character_target(tgestate_t *state,
                          character_t character,
                          xy_t        target);
void set_target(tgestate_t *state, vischar_t *vischar);

INLINE void store_target(xy_t target, xy_t *ptarget);

void byte_A13E_is_nonzero(tgestate_t *state,
                          xy_t       *target);
void byte_A13E_is_zero(tgestate_t *state,
                       xy_t       *target);
void byte_A13E_common(tgestate_t *state,
                      character_t character,
                      xy_t       *target);

void character_sits(tgestate_t *state,
                    uint8_t     A,
                    xy_t       *target);
void character_sleeps(tgestate_t *state,
                      uint8_t     A,
                      xy_t       *target);
void character_sit_sleep_common(tgestate_t *state,
                                room_t      room,
                                xy_t       *target);
void select_room_and_plot(tgestate_t *state);

void hero_sits(tgestate_t *state);
void hero_sleeps(tgestate_t *state);
void hero_sit_sleep_common(tgestate_t *state, uint8_t *HL);

void set_target_0x0E00(tgestate_t *state);
void set_target_0x8E04(tgestate_t *state);
void set_target_0x1000(tgestate_t *state);

void byte_A13E_is_nonzero_anotherone(tgestate_t *state,
                                     xy_t       *target);
void byte_A13E_is_zero_anotherone(tgestate_t *state,
                                  xy_t       *target);
void byte_A13E_anotherone_common(tgestate_t  *state,
                                 character_t  character,
                                 xy_t        *target);

void go_to_roll_call(tgestate_t *state);

void screen_reset(tgestate_t *state);

void escaped(tgestate_t *state);

uint8_t keyscan_all(tgestate_t *state);

INLINE escapeitem_t join_item_to_escapeitem(const item_t *pitem,
                                     escapeitem_t  previous);
INLINE escapeitem_t item_to_escapeitem(item_t item);

const screenlocstring_t *screenlocstring_plot(tgestate_t              *state,
                                              const screenlocstring_t *slstring);

void get_supertiles(tgestate_t *state);

void plot_bottommost_tiles(tgestate_t *state);
void plot_topmost_tiles(tgestate_t *state);
void plot_horizontal_tiles_common(tgestate_t       *state,
                                  tileindex_t      *vistiles,
                                  supertileindex_t *maptiles,
                                  uint8_t           pos,
                                  uint8_t          *window);

void plot_all_tiles(tgestate_t *state);

void plot_rightmost_tiles(tgestate_t *state);
void plot_leftmost_tiles(tgestate_t *state);
void plot_vertical_tiles_common(tgestate_t       *state,
                                tileindex_t      *vistiles,
                                supertileindex_t *maptiles,
                                uint8_t           x,
                                uint8_t          *window);

uint8_t *plot_tile_then_advance(tgestate_t             *state,
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
void searchlight_plot(tgestate_t *state, attribute_t *DE);

int touch(tgestate_t *state, vischar_t *vischar, spriteindex_t sprite_index);

int collision(tgestate_t *state);

/* $B000 onwards */

void accept_bribe(tgestate_t *state);

int bounds_check(tgestate_t *state, vischar_t *vischar);

INLINE uint16_t multiply_by_8(uint8_t A);

int is_door_locked(tgestate_t *state);

void door_handling(tgestate_t *state, vischar_t *vischar);

int door_in_range(tgestate_t *state, const doorpos_t *doorpos);

INLINE uint16_t multiply_by_4(uint8_t A);

int interior_bounds_check(tgestate_t *state, vischar_t *vischar);

void reset_outdoors(tgestate_t *state);

void door_handling_interior(tgestate_t *state, vischar_t *vischar);

void action_red_cross_parcel(tgestate_t *state);
void action_bribe(tgestate_t *state);
void action_poison(tgestate_t *state);
void action_uniform(tgestate_t *state);
void action_shovel(tgestate_t *state);
void action_lockpick(tgestate_t *state);
INLINE void action_red_key(tgestate_t *state);
INLINE void action_yellow_key(tgestate_t *state);
INLINE void action_green_key(tgestate_t *state);
void action_key(tgestate_t *state, room_t room_of_key);

doorindex_t *open_door(tgestate_t *state);

void action_wiresnips(tgestate_t *state);

extern const wall_t walls[24];

void called_from_main_loop_9(tgestate_t *state);

void calc_vischar_screenpos_from_mi_pos(tgestate_t *state, vischar_t *vischar);
void calc_vischar_screenpos(tgestate_t *state, vischar_t *vischar);

void reset_game(tgestate_t *state);
void reset_map_and_characters(tgestate_t *state);

void searchlight_mask_test(tgestate_t *state, vischar_t *vischar);

void locate_vischar_or_itemstruct_then_plot(tgestate_t *state);

int locate_vischar_or_itemstruct(tgestate_t    *state,
                                 uint8_t       *pindex,
                                 vischar_t    **pvischar,
                                 itemstruct_t **pitemstruct);

void render_mask_buffer(tgestate_t *state);

uint16_t multiply(uint8_t left, uint8_t right);

void mask_against_tile(tileindex_t index, tilerow_t *dst);

int vischar_visible(tgestate_t      *state,
                    const vischar_t *vischar,
                    uint16_t        *clipped_width,
                    uint16_t        *clipped_height);

void restore_tiles(tgestate_t *state);

const tile_t *select_tile_set(tgestate_t *state,
                              uint8_t     x_shift,
                              uint8_t     y_shift);

/* $C000 onwards */

void spawn_characters(tgestate_t *state);

void purge_visible_characters(tgestate_t *state);

int spawn_character(tgestate_t *state, characterstruct_t *charstr);

void reset_visible_character(tgestate_t *state, vischar_t *vischar);

uint8_t get_next_target(tgestate_t *state,
                        xy_t       *target,
                        xy_t      **target_out);

void move_characters(tgestate_t *state);

int change_by_delta(int8_t         max,
                    int            rc,
                    const uint8_t *second,
                    uint8_t       *first);

characterstruct_t *get_character_struct(tgestate_t *state,
                                        character_t character);

void character_event(tgestate_t *state, xy_t *target);

charevnt_handler_t charevnt_handler_4_solitary_ends;
charevnt_handler_t charevnt_handler_6;
charevnt_handler_t charevnt_handler_10_hero_released_from_solitary;
charevnt_handler_t charevnt_handler_1;
charevnt_handler_t charevnt_handler_2;
charevnt_handler_t charevnt_handler_0;
void set_target_ffxx(tgestate_t *state, xy_t *target, uint8_t y);
charevnt_handler_t charevnt_handler_3_check_var_A13E;
charevnt_handler_t charevnt_handler_5_check_var_A13E_anotherone;
charevnt_handler_t charevnt_handler_7;
charevnt_handler_t charevnt_handler_9_hero_sits;
charevnt_handler_t charevnt_handler_8_hero_sleeps;

void follow_suspicious_character(tgestate_t *state);

void character_behaviour(tgestate_t *state,
                         vischar_t  *vischar);
void character_behaviour_set_input(tgestate_t *state,
                                   vischar_t  *vischar,
                                   uint8_t     new_input);
void character_behaviour_impeded(tgestate_t *state,
                                 vischar_t  *vischar,
                                 uint8_t     new_input,
                                 int         scale);

uint8_t move_character_x(tgestate_t *state,
                         vischar_t  *vischar,
                         int         scale);

uint8_t move_character_y(tgestate_t *state,
                         vischar_t  *vischar,
                         int         scale);

void bribes_solitary_food(tgestate_t *state, vischar_t *vischar);

void get_next_target_and_handle_it(tgestate_t *state,
                                   vischar_t  *vischar,
                                   xy_t       *target);
void ran_out_of_list(tgestate_t *state, vischar_t *vischar, xy_t *target);
void handle_target(tgestate_t *state,
                   vischar_t  *vischar,
                   xy_t       *pushed_HL,
                   const xy_t *new_target,
                   uint8_t     A);

INLINE uint16_t multiply_by_1(uint8_t A);

INLINE const uint8_t *element_A_of_table_7738(uint8_t A);

uint8_t random_nibble(tgestate_t *state);

void solitary(tgestate_t *state);

void guards_follow_suspicious_character(tgestate_t *state,
                                        vischar_t  *vischar);

void hostiles_persue(tgestate_t *state);

void is_item_discoverable(tgestate_t *state);

int is_item_discoverable_interior(tgestate_t *state,
                                  room_t      room,
                                  item_t     *pitem);

void item_discovered(tgestate_t *state, item_t item);

extern const default_item_location_t default_item_locations[item__LIMIT];

extern const character_class_data_t character_class_data[4];

extern const uint8_t *animations[24];

/* $D000 onwards */

void mark_nearby_items(tgestate_t *state);

uint8_t get_greatest_itemstruct(tgestate_t    *state,
                                item_t         item_and_flag,
                                uint16_t       x,
                                uint16_t       y,
                                itemstruct_t **pitemstr);

uint8_t setup_item_plotting(tgestate_t   *state,
                            itemstruct_t *itemstr,
                            item_t        item);

uint8_t item_visible(tgestate_t *state,
                     uint16_t   *clipped_width,
                     uint16_t   *clipped_height);

extern const spritedef_t item_definitions[item__LIMIT];

/* $E000 onwards */

extern const size_t masked_sprite_plotter_16_enables[2 * 3];

void masked_sprite_plotter_24_wide_vischar(tgestate_t *state, vischar_t *vischar);

void masked_sprite_plotter_16_wide_item(tgestate_t *state);
void masked_sprite_plotter_16_wide_vischar(tgestate_t *state, vischar_t *vischar);
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

void pos_to_tinypos(const pos_t *in, tinypos_t *out);
INLINE void divide_by_8_with_rounding(uint8_t *A, uint8_t *C);
INLINE void divide_by_8(uint8_t *A, uint8_t *C);

void plot_game_window(tgestate_t *state);

timedevent_handler_t event_roll_call;

void action_papers(tgestate_t *state);

int user_confirm(tgestate_t *state);

/* $F000 onwards */

void wipe_full_screen_and_attributes(tgestate_t *state);

#endif /* THEGREATESCAPE_H */
