/**
 * TheGreatEscape.c
 *
 * Reimplementation of the ZX Spectrum 48K game "The Great Escape" created by
 * Denton Designs and published in 1986 by Ocean Software.
 *
 * This reimplementation copyright (c) David Thomas, 2012-2015.
 * <dave@davespace.co.uk>.
 */

/* ----------------------------------------------------------------------- */

/* TODO
 *
 * - Some enums might be wider than expected (int vs uint8_t).
 * - Target member -- this could be a screen location, or an index.
 * - Decode table_7738.
 * - Decode foreground masks.
 */

/* LATER
 *
 * - Hoist out constant sizes to variables in the state structure (required
 *   for variably-sized screens).
 * - Move screen memory to a linear format (again, required for
 *   variably-sized screen).
 * - Replace uint8_t counters, and other types which are smaller than int,
 *   with int where possible.
 */

/* GLOSSARY
 *
 * - "Conv:"
 *   -- Code which has required adjustment.
 * - A call marked "exit via"
 *   -- The original code branched directly to this routine.
 */

/* ----------------------------------------------------------------------- */

#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ZXSpectrum/ZXSpectrum.h"

#include "TheGreatEscape/ExteriorTiles.h"
#include "TheGreatEscape/Font.h"
#include "TheGreatEscape/InteriorObjectDefs.h"
#include "TheGreatEscape/InteriorObjects.h"
#include "TheGreatEscape/InteriorTiles.h"
#include "TheGreatEscape/ItemBitmaps.h"
#include "TheGreatEscape/Items.h"
#include "TheGreatEscape/Map.h"
#include "TheGreatEscape/Music.h"
#include "TheGreatEscape/RoomDefs.h"
#include "TheGreatEscape/Rooms.h"
#include "TheGreatEscape/Sprites.h"
#include "TheGreatEscape/State.h"
#include "TheGreatEscape/StaticGraphics.h"
#include "TheGreatEscape/StaticTiles.h"
#include "TheGreatEscape/SuperTiles.h"
#include "TheGreatEscape/TGEObject.h"
#include "TheGreatEscape/Tiles.h"
#include "TheGreatEscape/Utils.h"

#include "TheGreatEscape/TheGreatEscape.h"

/* ----------------------------------------------------------------------- */

/* Z80 instruction simulator macros. */

/**
 * Shift left arithmetic.
 */
#define SLA(r)      \
do {                \
  carry = (r) >> 7; \
  (r) <<= 1;        \
} while (0)

/**
 * Shift right logical.
 */
#define SRL(r)      \
do {                \
  carry = (r) & 1;  \
  (r) >>= 1;        \
} while (0)

/**
 * Rotate left.
 */
#define RL(r)                 \
do {                          \
  int carry_out;              \
                              \
  carry_out = (r) >> 7;       \
  (r) = ((r) << 1) | (carry); \
  carry = carry_out;          \
} while (0)

/**
 * Rotate right through carry.
 */
#define RR(r)                       \
do {                                \
  int carry_out;                    \
                                    \
  carry_out = (r) & 1;              \
  (r) = ((r) >> 1) | (carry << 7);  \
  carry = carry_out;                \
} while (0)

/**
 * Rotate right.
 */
#define RRC(r)                      \
do {                                \
  carry = (r) & 1;                  \
  (r) = ((r) >> 1) | (carry << 7);  \
} while (0)

/**
 * Swap two variables (replacing EX/EXX).
 */
#define SWAP(T,a,b) \
do {                \
  T tmp = a;        \
  a = b;            \
  b = tmp;          \
} while (0)

/* ----------------------------------------------------------------------- */

/**
 * The NEVER_RETURNS macro is placed after calls which are not expected to
 * return (calls which ultimately invoke squash_stack_goto_main()).
 */
#define NEVER_RETURNS assert("Unexpected return." == NULL); return

#define UNFINISHED assert("Unfinished conversion here!" == NULL)

/* ----------------------------------------------------------------------- */

#define ASSERT_SCREEN_PTR_VALID(p)                    \
do {                                                  \
  assert(p >= &state->speccy->screen[0]);             \
  assert(p < &state->speccy->screen[SCREEN_LENGTH]);  \
} while (0)

#define ASSERT_SCREEN_ATTRIBUTES_PTR_VALID(p)         \
do {                                                  \
  assert(p >= &state->speccy->attributes[0]);         \
  assert(p < &state->speccy->attributes[SCREEN_ATTRIBUTES_LENGTH]); \
} while (0)

#define ASSERT_MASK_BUF_PTR_VALID(p)                  \
do {                                                  \
  assert(p >= &state->mask_buffer[0]);                \
  assert(p < &state->mask_buffer[5 * MASK_BUFFER_WIDTH]); \
} while (0)

#define ASSERT_TILE_BUF_PTR_VALID(p)                  \
do {                                                  \
  assert(p >= state->tile_buf);                       \
  assert(p < state->tile_buf + (state->tb_columns * state->tb_rows)); \
} while (0)

#define ASSERT_WINDOW_BUF_PTR_VALID(p)                \
do {                                                  \
  assert(p >= state->window_buf);                     \
  assert(p < state->window_buf + ((state->columns + 1) * state->rows * 8)); \
} while (0)

#define ASSERT_MAP_BUF_PTR_VALID(p)                   \
do {                                                  \
  assert(p >= state->map_buf);                        \
  assert(p < state->map_buf + (state->st_columns * state->st_rows)); \
} while (0)

#define ASSERT_VISCHAR_VALID(p)                       \
do {                                                  \
  assert(p >= &state->vischars[0]);                   \
  assert(p < &state->vischars[vischars_LENGTH]);      \
} while (0)

/* ----------------------------------------------------------------------- */

// FORWARD REFERENCES
//
// (in original file order)

#define INLINE __inline

/* $6000 onwards */

static
void transition(tgestate_t      *state,
                const tinypos_t *pos);
static
void enter_room(tgestate_t *state);
static INLINE
void squash_stack_goto_main(tgestate_t *state);

static
void set_hero_sprite_for_room(tgestate_t *state);

static
void setup_movable_items(tgestate_t *state);
static
void setup_movable_item(tgestate_t          *state,
                        const movableitem_t *movableitem,
                        character_t          character);

static
void reset_nonplayer_visible_characters(tgestate_t *state);

static
void setup_doors(tgestate_t *state);

static
const doorpos_t *get_door_position(doorindex_t index);

static
void wipe_visible_tiles(tgestate_t *state);

static
void setup_room(tgestate_t *state);

static
void expand_object(tgestate_t *state, object_t index, uint8_t *output);

static
void plot_interior_tiles(tgestate_t *state);

extern uint8_t *const beds[beds_LENGTH];

/* $7000 onwards */

extern const doorpos_t door_positions[124];

static
void process_player_input_fire(tgestate_t *state, input_t input);
static
void use_item_B(tgestate_t *state);
static
void use_item_A(tgestate_t *state);
static
void use_item_common(tgestate_t *state, item_t item);

static
void pick_up_item(tgestate_t *state);
static
void drop_item(tgestate_t *state);
static
void drop_item_tail(tgestate_t *state, item_t item);
static
void drop_item_tail_exterior(itemstruct_t *HL);
static
void drop_item_tail_interior(itemstruct_t *HL);

static INLINE
itemstruct_t *item_to_itemstruct(tgestate_t *state, item_t item);

static
void draw_all_items(tgestate_t *state);
static
void draw_item(tgestate_t *state, item_t index, size_t dstoff);

static
itemstruct_t *find_nearby_item(tgestate_t *state);

static
void plot_bitmap(tgestate_t    *state,
                 uint8_t        width,
                 uint8_t        height,
                 const uint8_t *src,
                 uint8_t       *dst);

static
void screen_wipe(tgestate_t *state,
                 uint8_t     width,
                 uint8_t     height,
                 uint8_t    *dst);

static INLINE
uint8_t *get_next_scanline(tgestate_t *state, uint8_t *slp);

static
void queue_message_for_display(tgestate_t *state,
                               message_t   message_index);

static INLINE
uint8_t *plot_glyph(const char *pcharacter, uint8_t *output);
static
uint8_t *plot_single_glyph(int character, uint8_t *output);

static
void message_display(tgestate_t *state);
static
void wipe_message(tgestate_t *state);
static
void next_message(tgestate_t *state);

/* $8000 onwards */

/* $9000 onwards */

static
void main_loop(tgestate_t *state);

static
void check_morale(tgestate_t *state);

static
void keyscan_break(tgestate_t *state);

static
void process_player_input(tgestate_t *state);

static
void picking_a_lock(tgestate_t *state);

static
void snipping_wire(tgestate_t *state);

static
void in_permitted_area(tgestate_t *state);
static
int in_permitted_area_end_bit(tgestate_t *state, uint8_t room_and_flags);
static
int within_camp_bounds(uint8_t          index,
                       const tinypos_t *pos);

/* $A000 onwards */

static
void wave_morale_flag(tgestate_t *state);

static
void set_morale_flag_screen_attributes(tgestate_t *state, attribute_t attrs);

static
uint8_t *get_prev_scanline(tgestate_t *state, uint8_t *addr);

static
void interior_delay_loop(tgestate_t *state);

static
void ring_bell(tgestate_t *state);
static
void plot_ringer(tgestate_t *state, const uint8_t *src);

static
void increase_morale(tgestate_t *state, uint8_t delta);
static
void decrease_morale(tgestate_t *state, uint8_t delta);
static
void increase_morale_by_10_score_by_50(tgestate_t *state);
static
void increase_morale_by_5_score_by_5(tgestate_t *state);

static
void increase_score(tgestate_t *state, uint8_t delta);

static
void plot_score(tgestate_t *state);

static
void play_speaker(tgestate_t *state, sound_t sound);

static
void set_game_window_attributes(tgestate_t *state, attribute_t attrs);

static
void dispatch_timed_event(tgestate_t *state);

static
timedevent_handler_t event_night_time;
static
timedevent_handler_t event_another_day_dawns;
static
void set_day_or_night(tgestate_t *state, uint8_t day_night);
static
timedevent_handler_t event_wake_up;
static
timedevent_handler_t event_go_to_roll_call;
static
timedevent_handler_t event_go_to_breakfast_time;
static
timedevent_handler_t event_breakfast_time;
static
timedevent_handler_t event_go_to_exercise_time;
static
timedevent_handler_t event_exercise_time;
static
timedevent_handler_t event_go_to_time_for_bed;
static
timedevent_handler_t event_new_red_cross_parcel;
static
timedevent_handler_t event_time_for_bed;
static
timedevent_handler_t event_search_light;
static
void set_guards_location(tgestate_t *state, location_t location);

extern const character_t prisoners_and_guards[10];

static
void wake_up(tgestate_t *state);
static
void breakfast_time(tgestate_t *state);

static
void set_hero_target_location(tgestate_t *state, location_t location);

static
void go_to_time_for_bed(tgestate_t *state);

static
void set_prisoners_and_guards_location(tgestate_t *state,
                                 uint8_t    *p_loc_low,
                                 uint8_t     loc_high);
static
void set_prisoners_and_guards_location_B(tgestate_t *state,
                                   uint8_t    *p_loc_low,
                                   uint8_t     loc_high);
static
void set_character_location(tgestate_t  *state,
                            character_t  index,
                            location_t   location);
static
void sub_A3BB(tgestate_t *state, vischar_t *vischar);

static INLINE
void store_location(location_t location, location_t *plocation);

static
void byte_A13E_is_nonzero(tgestate_t        *state,
                          characterstruct_t *charstr);
static
void byte_A13E_is_zero(tgestate_t        *state,
                       characterstruct_t *charstr,
                       vischar_t         *vischar);
static
void sub_A404(tgestate_t        *state,
              characterstruct_t *charstr,
              character_t        character);

static
void character_sits(tgestate_t  *state,
                    character_t  character,
                    location_t  *location);
static
void character_sleeps(tgestate_t  *state,
                      character_t  character,
                      location_t  *location);
static
void character_sit_sleep_common(tgestate_t *state,
                                room_t      room,
                                location_t *location);
static
void select_room_and_plot(tgestate_t *state);

static
void hero_sits(tgestate_t *state);
static
void hero_sleeps(tgestate_t *state);
static
void hero_sit_sleep_common(tgestate_t *state, uint8_t *HL);

static
void set_location_0x000E(tgestate_t *state);
static
void set_location_0x048E(tgestate_t *state);
static
void set_location_0x0010(tgestate_t *state);

static
void byte_A13E_is_nonzero_anotherone(tgestate_t        *state,
                                     vischar_t         *vischar,
                                     characterstruct_t *charstr);
static
void byte_A13E_is_zero_anotherone(tgestate_t        *state,
                                  vischar_t         *vischar,
                                  characterstruct_t *charstr);
static
void byte_A13E_anotherone_common(tgestate_t        *state,
                                 characterstruct_t *charstr,
                                 character_t        character_index);

static
void go_to_roll_call(tgestate_t *state);

static
void screen_reset(tgestate_t *state);

static
void escaped(tgestate_t *state);

static
uint8_t keyscan_all(tgestate_t *state);

static INLINE
escapeitem_t join_item_to_escapeitem(const item_t *pitem, escapeitem_t previous);
static INLINE
escapeitem_t item_to_escapeitem(item_t item);

static
const screenlocstring_t *screenlocstring_plot(tgestate_t              *state,
                                              const screenlocstring_t *slstring);

static
void get_supertiles(tgestate_t *state);

static
void supertile_plot_horizontal_1(tgestate_t *state);
static
void supertile_plot_horizontal_2(tgestate_t *state);
static
void supertile_plot_horizontal_common(tgestate_t       *state,
                                      tileindex_t      *vistiles,
                                      supertileindex_t *maptiles,
                                      uint8_t           pos,
                                      uint8_t          *window);

static
void supertile_plot_all(tgestate_t *state);

static
void supertile_plot_vertical_1(tgestate_t *state);
static
void supertile_plot_vertical_2(tgestate_t *state);
static
void supertile_plot_vertical_common(tgestate_t       *state,
                                    tileindex_t      *vistiles,
                                    supertileindex_t *maptiles,
                                    uint8_t           pos,
                                    uint8_t          *window);

static
uint8_t *plot_tile_then_advance(tgestate_t             *state,
                                tileindex_t             tile_index,
                                const supertileindex_t *psupertileindex,
                                uint8_t                *scr);

static
uint8_t *plot_tile(tgestate_t             *state,
                   tileindex_t             tile_index,
                   const supertileindex_t *psupertileindex,
                   uint8_t                *scr);

static
void map_shunt_horizontal_1(tgestate_t *state);
static
void map_shunt_horizontal_2(tgestate_t *state);
static
void map_shunt_diagonal_1_2(tgestate_t *state);
static
void map_shunt_vertical_1(tgestate_t *state);
static
void map_shunt_vertical_2(tgestate_t *state);
static
void map_shunt_diagonal_2_1(tgestate_t *state);

static
void move_map(tgestate_t *state);

static
void map_move_up_left(tgestate_t *state, uint8_t *DE);
static
void map_move_up_right(tgestate_t *state, uint8_t *DE);
static
void map_move_down_right(tgestate_t *state, uint8_t *DE);
static
void map_move_down_left(tgestate_t *state, uint8_t *DE);

static
attribute_t choose_game_window_attributes(tgestate_t *state);

static
void zoombox(tgestate_t *state);
static
void zoombox_fill(tgestate_t *state);
static
void zoombox_draw_border(tgestate_t *state);
static
void zoombox_draw_tile(tgestate_t *state, uint8_t index, uint8_t *addr);

static
void searchlight_AD59(tgestate_t *state, uint8_t *HL);
static
void nighttime(tgestate_t *state);
static
void searchlight_caught(tgestate_t *state, uint8_t *HL);
static
void searchlight_plot(tgestate_t *state);

static
int touch(tgestate_t *state, vischar_t *vischar, uint8_t Adash);

static
int collision(tgestate_t *state, vischar_t *vischar);

/* $B000 onwards */

static
void accept_bribe(tgestate_t *state, vischar_t *vischar);

static
int bounds_check(tgestate_t *state, vischar_t *vischar);

static INLINE
uint16_t multiply_by_8(uint8_t A);

static
int is_door_open(tgestate_t *state);

static
void door_handling(tgestate_t *state, vischar_t *vischar);

static
int door_in_range(tgestate_t *state, const doorpos_t *doorpos);

static INLINE
uint16_t multiply_by_4(uint8_t A);

static
int interior_bounds_check(tgestate_t *state, vischar_t *vischar);

static
void reset_outdoors(tgestate_t *state);

static
void door_handling_interior(tgestate_t *state, vischar_t *vischar);

static
void action_red_cross_parcel(tgestate_t *state);
static
void action_bribe(tgestate_t *state);
static
void action_poison(tgestate_t *state);
static
void action_uniform(tgestate_t *state);
static
void action_shovel(tgestate_t *state);
static
void action_lockpick(tgestate_t *state);
static INLINE
void action_red_key(tgestate_t *state);
static INLINE
void action_yellow_key(tgestate_t *state);
static INLINE
void action_green_key(tgestate_t *state);
static
void action_key(tgestate_t *state, room_t room);

static
void *open_door(tgestate_t *state);

static
void action_wiresnips(tgestate_t *state);

extern const wall_t walls[24];

static
void called_from_main_loop_9(tgestate_t *state);

static
void reset_position(tgestate_t *state, vischar_t *vischar);
static
void reset_position_end(tgestate_t *state, vischar_t *vischar);

static
void reset_game(tgestate_t *state);
static
void reset_map_and_characters(tgestate_t *state);

static
void searchlight_mask_test(tgestate_t *state, vischar_t *vischar);

static
void locate_vischar_or_itemstruct_then_plot(tgestate_t *state);

static
int locate_vischar_or_itemstruct(tgestate_t    *state,
                                 uint8_t       *pindex,
                                 vischar_t    **pvischar,
                                 itemstruct_t **pitemstruct);

static
void mask_stuff(tgestate_t *state);

static
uint16_t multiply(uint8_t value, uint8_t shift);

static
void mask_against_tile(tileindex_t index, uint8_t *dst);

static
int vischar_visible(tgestate_t *state,
                    vischar_t  *vischar,
                    uint16_t   *clipped_width,
                    uint16_t   *clipped_height);

static
void called_from_main_loop_3(tgestate_t *state);

static
const tile_t *select_tile_set(tgestate_t *state,
                              uint8_t     x_shift,
                              uint8_t     y_shift);

/* $C000 onwards */

static
void spawn_characters(tgestate_t *state);

static
void purge_visible_characters(tgestate_t *state);

static
int spawn_character(tgestate_t *state, characterstruct_t *charstr);

static
void reset_visible_character(tgestate_t *state, vischar_t *vischar);

static
uint8_t sub_C651(tgestate_t *state,
                 location_t *location);

static
void move_characters(tgestate_t *state);

static
int change_by_delta(int8_t         max,
                    int            rc,
                    const uint8_t *second,
                    uint8_t       *first);

static
characterstruct_t *get_character_struct(tgestate_t  *state,
                                        character_t  index);

static
void character_event(tgestate_t *state, location_t *location);

static
charevnt_handler_t charevnt_handler_4_zeroes_morale_1;
static
charevnt_handler_t charevnt_handler_6;
static
charevnt_handler_t charevnt_handler_10_hero_released_from_solitary;
static
charevnt_handler_t charevnt_handler_1;
static
charevnt_handler_t charevnt_handler_2;
static
charevnt_handler_t charevnt_handler_0;
static
void localexit(tgestate_t *state, character_t *charptr, uint8_t C);
static
charevnt_handler_t charevnt_handler_3_check_var_A13E;
static
charevnt_handler_t charevnt_handler_5_check_var_A13E_anotherone;
static
charevnt_handler_t charevnt_handler_7;
static
charevnt_handler_t charevnt_handler_9_hero_sits;
static
charevnt_handler_t charevnt_handler_8_hero_sleeps;

static
void follow_suspicious_character(tgestate_t *state);

static
void character_behaviour(tgestate_t *state,
                         vischar_t  *vischar);
static
void character_behaviour_end_1(tgestate_t *state,
                               vischar_t  *vischar,
                               uint8_t     A);
static
void character_behaviour_end_2(tgestate_t *state,
                               vischar_t  *vischar,
                               uint8_t     A,
                               int         log2scale);

static
uint8_t move_character_x(tgestate_t *state,
                         vischar_t  *vischar,
                         int         log2scale);

static
uint8_t move_character_y(tgestate_t *state,
                         vischar_t  *vischar,
                         int         log2scale);

static
void bribes_solitary_food(tgestate_t *state, vischar_t *vischar);

static
void sub_CB23(tgestate_t *state, vischar_t *vischar, location_t *location);
static
void sub_CB2D(tgestate_t *state, vischar_t *vischar, location_t *location);
static
void sub_CB61(tgestate_t *state,
              vischar_t  *vischar,
              location_t *pushed_HL,
              location_t *location,
              uint8_t     A);

static INLINE
uint16_t multiply_by_1(uint8_t A);

static INLINE
const uint8_t *element_A_of_table_7738(uint8_t A);

static
uint8_t random_nibble(tgestate_t *state);

static
void solitary(tgestate_t *state);

static
void guards_follow_suspicious_character(tgestate_t *state, vischar_t *vischar);

static
void hostiles_persue(tgestate_t *state);

static
void is_item_discoverable(tgestate_t *state);

static
int is_item_discoverable_interior(tgestate_t *state,
                                  room_t      room,
                                  item_t     *pitem);

static
void item_discovered(tgestate_t *state, item_t item);

extern const default_item_location_t default_item_locations[item__LIMIT];

extern const character_meta_data_t character_meta_data[4];

extern const uint8_t *character_related_pointers[24];

/* $D000 onwards */

static
void mark_nearby_items(tgestate_t *state);

static
uint8_t get_greatest_itemstruct(tgestate_t    *state,
                                item_t         item_and_flag,
                                uint16_t       x,
                                uint16_t       y,
                                itemstruct_t **pitemstr);

static
uint8_t setup_item_plotting(tgestate_t   *state,
                            itemstruct_t *itemstr,
                            item_t        item);

static
uint8_t item_visible(tgestate_t *state,
                     uint16_t   *clipped_width,
                     uint16_t   *clipped_height);

extern const sprite_t item_definitions[item__LIMIT];

/* $E000 onwards */

const size_t masked_sprite_plotter_16_enables[2 * 3];

static
void masked_sprite_plotter_24_wide(tgestate_t *state, vischar_t *vischar);

static
void masked_sprite_plotter_16_wide_searchlight(tgestate_t *state);
static
void masked_sprite_plotter_16_wide(tgestate_t *state, vischar_t *vischar);
static
void masked_sprite_plotter_16_wide_left(tgestate_t *state, uint8_t x);
static
void masked_sprite_plotter_16_wide_right(tgestate_t *state, uint8_t x);

static
void flip_24_masked_pixels(tgestate_t *state,
                           uint8_t    *pE,
                           uint8_t    *pC,
                           uint8_t    *pB,
                           uint8_t    *pEdash,
                           uint8_t    *pCdash,
                           uint8_t    *pBdash);

static
void flip_16_masked_pixels(tgestate_t *state,
                           uint8_t    *pD,
                           uint8_t    *pE,
                           uint8_t    *pDdash,
                           uint8_t    *pEdash);

static
int setup_vischar_plotting(tgestate_t *state, vischar_t *vischar);

static
void pos_to_tinypos(const pos_t *in, tinypos_t *out);
static INLINE
void divide_by_8_with_rounding(uint8_t *A, uint8_t *C);
static INLINE
void divide_by_8(uint8_t *A, uint8_t *C);

static
void plot_game_window(tgestate_t *state);

static
timedevent_handler_t event_roll_call;

static
void action_papers(tgestate_t *state);

static
int user_confirm(tgestate_t *state);

/* $F000 onwards */

static
void wipe_full_screen_and_attributes(tgestate_t *state);

static
int check_menu_keys(tgestate_t *state);

static
void wipe_game_window(tgestate_t *state);

static
void choose_keys(tgestate_t *state);

static
void set_menu_item_attributes(tgestate_t *state,
                              int         index,
                              attribute_t attrs);

static
uint8_t menu_keyscan(tgestate_t *state);

static
void plot_statics_and_menu_text(tgestate_t *state);

static
void plot_static_tiles_horizontal(tgestate_t             *state,
                                  uint8_t                *out,
                                  const statictileline_t *stline);
static
void plot_static_tiles_vertical(tgestate_t             *state,
                                uint8_t                *out,
                                const statictileline_t *stline);
static
void plot_static_tiles(tgestate_t             *state,
                       uint8_t                *out,
                       const statictileline_t *stline,
                       int                     orientation);

static
void menu_screen(tgestate_t *state);

static
uint16_t get_tuning(uint8_t A);

static
input_t inputroutine_keyboard(tgestate_t *state);
static
input_t inputroutine_protek(tgestate_t *state);
static
input_t inputroutine_kempston(tgestate_t *state);
static
input_t inputroutine_fuller(tgestate_t *state);
static
input_t inputroutine_sinclair(tgestate_t *state);

static
input_t input_routine(tgestate_t *state);

/* ----------------------------------------------------------------------- */

/**
 * $68A2: Transition.
 *
 * The hero or a non-player character changes room.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] pos     Pointer to pos.               (was HL)
 *
 * \return Nothing (non-player), or doesn't return (hero).
 */
void transition(tgestate_t      *state,
                const tinypos_t *pos)
{
  vischar_t *vischar;    /* was IY */
  room_t     room_index; /* was A */

  assert(state   != NULL);
  assert(pos     != NULL);

  vischar = state->IY;
  ASSERT_VISCHAR_VALID(vischar);

  if (vischar->room == room_0_OUTDOORS)
  {
    /* Outdoors */

    /* Set position on X axis, Y axis and vertical offset (dividing by 4). */

    /* This was unrolled (compared to the original) to avoid having to access
     * the structure as an array. */
    vischar->mi.pos.x      = multiply_by_4(pos->x);
    vischar->mi.pos.y      = multiply_by_4(pos->y);
    vischar->mi.pos.height = multiply_by_4(pos->height);
  }
  else
  {
    /* Indoors */

    /* Set position on X axis, Y axis and vertical offset (copying). */

    /* This was unrolled (compared to the original) to avoid having to access
     * the structure as an array. */
    vischar->mi.pos.x      = pos->x;
    vischar->mi.pos.y      = pos->y;
    vischar->mi.pos.height = pos->height;
  }

  if (vischar != &state->vischars[0])
  {
    /* Not the hero. */

    reset_visible_character(state, vischar); // was exit via
  }
  else
  {
    /* Hero only. */

    vischar->flags &= ~vischar_FLAGS_BIT7;
    state->room_index = room_index = state->vischars[0].room;
    if (room_index == room_0_OUTDOORS)
    {
      /* Outdoors */

      vischar->b0D = vischar_BYTE13_BIT7; // likely a character direction
      vischar->b0E &= 3;  // likely a sprite direction
      reset_outdoors(state);
      squash_stack_goto_main(state); // exit
    }
    else
    {
      /* Indoors */

      enter_room(state); // was fallthrough
    }
    NEVER_RETURNS;
  }
}

/**
 * $68F4: Enter room.
 *
 * The hero enters a room.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Doesn't return.
 */
void enter_room(tgestate_t *state)
{
  assert(state != NULL);

  state->plot_game_window_x = 0;
  setup_room(state);
  plot_interior_tiles(state);
  set_hero_sprite_for_room(state);
  reset_position(state, &state->vischars[0]);
  setup_movable_items(state);
  zoombox(state);
  increase_score(state, 1);

  squash_stack_goto_main(state); // (was fallthrough to following)
  NEVER_RETURNS;
}

/**
 * $691A: Squash stack, then goto main.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Doesn't return.
 */
void squash_stack_goto_main(tgestate_t *state)
{
  assert(state != NULL);

  longjmp(state->jmpbuf_main, 1);
  NEVER_RETURNS;
}

/* ----------------------------------------------------------------------- */

/**
 * $6920: Set appropriate hero sprite for room.
 *
 * Called when changing rooms.
 *
 * \param[in] state Pointer to game state.
 */
void set_hero_sprite_for_room(tgestate_t *state)
{
  vischar_t *hero; /* was ? */

  assert(state != NULL);

  hero = &state->vischars[0];
  hero->b0D = vischar_BYTE13_BIT7; // likely a character direction

  /* When in tunnel rooms this forces the hero sprite to 'prisoner' and sets
   * the crawl flag appropriately. */
  if (state->room_index >= room_29_SECOND_TUNNEL_START)
  {
    hero->b0E |= vischar_BYTE14_CRAWL;
    hero->mi.spriteset = &sprites[sprite_PRISONER_FACING_TOP_LEFT_4];
  }
  else
  {
    hero->b0E &= ~vischar_BYTE14_CRAWL;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $6939: Setup movable items.
 *
 * \param[in] state Pointer to game state.
 */
void setup_movable_items(tgestate_t *state)
{
  const movableitem_t *movableitem;
  character_t          character;

  assert(state != NULL);

  reset_nonplayer_visible_characters(state);

  /* Conv: Use a switch statement rather than if-else and gotos. */

  switch (state->room_index)
  {
    case room_2_HUT2LEFT:
      movableitem = &state->movable_items[movable_item_STOVE1];
      character   = character_26_STOVE_1;
      setup_movable_item(state, movableitem, character);
      break;

    case room_4_HUT3LEFT:
      movableitem = &state->movable_items[movable_item_STOVE2];
      character   = character_27_STOVE_2;
      setup_movable_item(state, movableitem, character);
      break;

    case room_9_CRATE:
      movableitem = &state->movable_items[movable_item_CRATE];
      character   = character_28_CRATE;
      setup_movable_item(state, movableitem, character);
      break;

    default:
      break;
  }

  spawn_characters(state);
  mark_nearby_items(state);
  called_from_main_loop_9(state);
  move_map(state);
  locate_vischar_or_itemstruct_then_plot(state);
}

/**
 * $697D: Setup movable item.
 *
 * \param[in] state       Pointer to game state.
 * \param[in] movableitem Pointer to movable item.
 * \param[in] character   Character.
 */
void setup_movable_item(tgestate_t          *state,
                        const movableitem_t *movableitem,
                        character_t          character)
{
  vischar_t *vischar1;

  assert(state       != NULL);
  assert(movableitem != NULL);
  assert(character >= 0 && character < character__LIMIT);

  /* The movable item takes the place of the first non-player visible
   * character. */
  vischar1 = &state->vischars[1];

  vischar1->character   = character;
  vischar1->mi          = *movableitem;

  vischar1->flags       = 0;
  vischar1->target      = 0;
  vischar1->p04.x       = 0;
  vischar1->p04.y       = 0;
  vischar1->p04.height  = 0;
  vischar1->b07         = 0;
  vischar1->w08         = &character_related_pointers[0];
  vischar1->w0A         = character_related_pointers[8];
  vischar1->b0C         = 0;
  vischar1->b0D         = 0;
  vischar1->b0E         = 0;

  vischar1->room        = state->room_index;
  reset_position(state, vischar1);
}

/* ----------------------------------------------------------------------- */

/**
 * $69C9: Reset non-player visible characters.
 *
 * \param[in] state Pointer to game state.
 */
void reset_nonplayer_visible_characters(tgestate_t *state)
{
  vischar_t *vischar; /* was HL */
  uint8_t    iters;   /* was B */

  assert(state != NULL);

  vischar = &state->vischars[1];
  iters = vischars_LENGTH - 1;
  do
    reset_visible_character(state, vischar++);
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $69DC: Setup doors.
 *
 * \param[in] state Pointer to game state.
 */
void setup_doors(tgestate_t *state)
{
  uint8_t         *pdoor;    /* was DE */
  uint8_t          iters;    /* was B */
  uint8_t          flag;     /* was C */
  const doorpos_t *pdoorpos; /* was HL' */
  uint8_t          ndoorpos; /* was B' */

  assert(state != NULL);

  /* Wipe door_related[] with 0xFF. (Could alternatively use memset().) */
  pdoor = &state->door_related[3];
  iters = 4;
  do
    *pdoor-- = 0xFF;
  while (--iters);

  pdoor++; /* === DE = &door_related[0]; */
  iters = state->room_index << 2;
  flag = 0;
  pdoorpos = &door_positions[0];
  ndoorpos = NELEMS(door_positions);
  do
  {
    if ((pdoorpos->room_and_flags & doorposition_BYTE0_MASK_HI) == iters)
      *pdoor++ = flag ^ 0x80;
    flag ^= 0x80;
    if (flag >= 0)
      flag++; // increment every two stops?
    pdoorpos++;
  }
  while (--ndoorpos);
}

/* ----------------------------------------------------------------------- */

/**
 * $6A12: Get door position.
 *
 * \param[in] index Index of door + flag in bit 7.
 *
 * \return Pointer to doorpos. (was HL)
 */
const doorpos_t *get_door_position(doorindex_t index)
{
  const doorpos_t *doorpos;

  // assert(index); // FIXME devise precondition

  doorpos = &door_positions[(index * 2) & 0xFF];
  if (index & (1 << 7))
    doorpos++;

  return doorpos;
}

/* ----------------------------------------------------------------------- */

/**
 * $6A27: Wipe visible tiles.
 *
 * \param[in] state Pointer to game state.
 */
void wipe_visible_tiles(tgestate_t *state)
{
  assert(state != NULL);

  memset(state->tile_buf, 0, state->columns * (state->rows + 1)); /* was 24 * 17 */
}

/* ----------------------------------------------------------------------- */

/**
 * $6A35: Setup room.
 *
 * Using state->room_index expand out the room definition.
 *
 * \param[in] state Pointer to game state.
 */
void setup_room(tgestate_t *state)
{
  /**
   * $EA7C: Interior masking data.
   *
   * Conv: Constant final (pos.height) byte added into this data.
   */
  static const mask_t interior_mask_data[47] =
  {
    { 0x1B, { 0x7B, 0x7F, 0xF1, 0xF3 }, { 0x36, 0x28, 0x20 } },
    { 0x1B, { 0x77, 0x7B, 0xF3, 0xF5 }, { 0x36, 0x18, 0x20 } },
    { 0x1B, { 0x7C, 0x80, 0xF1, 0xF3 }, { 0x32, 0x2A, 0x20 } },
    { 0x19, { 0x83, 0x86, 0xF2, 0xF7 }, { 0x18, 0x24, 0x20 } },
    { 0x19, { 0x81, 0x84, 0xF4, 0xF9 }, { 0x18, 0x1A, 0x20 } },
    { 0x19, { 0x81, 0x84, 0xF3, 0xF8 }, { 0x1C, 0x17, 0x20 } },
    { 0x19, { 0x83, 0x86, 0xF4, 0xF8 }, { 0x16, 0x20, 0x20 } },
    { 0x18, { 0x7D, 0x80, 0xF4, 0xF9 }, { 0x18, 0x1A, 0x20 } },
    { 0x18, { 0x7B, 0x7E, 0xF3, 0xF8 }, { 0x22, 0x1A, 0x20 } },
    { 0x18, { 0x79, 0x7C, 0xF4, 0xF9 }, { 0x22, 0x10, 0x20 } },
    { 0x18, { 0x7B, 0x7E, 0xF4, 0xF9 }, { 0x1C, 0x17, 0x20 } },
    { 0x18, { 0x79, 0x7C, 0xF1, 0xF6 }, { 0x2C, 0x1E, 0x20 } },
    { 0x18, { 0x7D, 0x80, 0xF2, 0xF7 }, { 0x24, 0x22, 0x20 } },
    { 0x1D, { 0x7F, 0x82, 0xF6, 0xF7 }, { 0x1C, 0x1E, 0x20 } },
    { 0x1D, { 0x82, 0x85, 0xF2, 0xF3 }, { 0x23, 0x30, 0x20 } },
    { 0x1D, { 0x86, 0x89, 0xF2, 0xF3 }, { 0x1C, 0x37, 0x20 } },
    { 0x1D, { 0x86, 0x89, 0xF4, 0xF5 }, { 0x18, 0x30, 0x20 } },
    { 0x1D, { 0x80, 0x83, 0xF1, 0xF2 }, { 0x28, 0x30, 0x20 } },
    { 0x1C, { 0x81, 0x82, 0xF4, 0xF6 }, { 0x1C, 0x20, 0x20 } },
    { 0x1C, { 0x83, 0x84, 0xF4, 0xF6 }, { 0x1C, 0x2E, 0x20 } },
    { 0x1A, { 0x7E, 0x80, 0xF5, 0xF7 }, { 0x1C, 0x20, 0x20 } },
    { 0x12, { 0x7A, 0x7B, 0xF2, 0xF3 }, { 0x3A, 0x28, 0x20 } },
    { 0x12, { 0x7A, 0x7B, 0xEF, 0xF0 }, { 0x45, 0x35, 0x20 } },
    { 0x17, { 0x80, 0x85, 0xF4, 0xF6 }, { 0x1C, 0x24, 0x20 } },
    { 0x14, { 0x80, 0x84, 0xF3, 0xF5 }, { 0x26, 0x28, 0x20 } },
    { 0x15, { 0x84, 0x85, 0xF6, 0xF7 }, { 0x1A, 0x1E, 0x20 } },
    { 0x15, { 0x7E, 0x7F, 0xF3, 0xF4 }, { 0x2E, 0x26, 0x20 } },
    { 0x16, { 0x7C, 0x85, 0xEF, 0xF3 }, { 0x32, 0x22, 0x20 } },
    { 0x16, { 0x79, 0x82, 0xF0, 0xF4 }, { 0x34, 0x1A, 0x20 } },
    { 0x16, { 0x7D, 0x86, 0xF2, 0xF6 }, { 0x24, 0x1A, 0x20 } },
    { 0x10, { 0x76, 0x78, 0xF5, 0xF7 }, { 0x36, 0x0A, 0x20 } },
    { 0x10, { 0x7A, 0x7C, 0xF3, 0xF5 }, { 0x36, 0x0A, 0x20 } },
    { 0x10, { 0x7E, 0x80, 0xF1, 0xF3 }, { 0x36, 0x0A, 0x20 } },
    { 0x10, { 0x82, 0x84, 0xEF, 0xF1 }, { 0x36, 0x0A, 0x20 } },
    { 0x10, { 0x86, 0x88, 0xED, 0xEF }, { 0x36, 0x0A, 0x20 } },
    { 0x10, { 0x8A, 0x8C, 0xEB, 0xED }, { 0x36, 0x0A, 0x20 } },
    { 0x11, { 0x73, 0x75, 0xEB, 0xED }, { 0x0A, 0x30, 0x20 } },
    { 0x11, { 0x77, 0x79, 0xED, 0xEF }, { 0x0A, 0x30, 0x20 } },
    { 0x11, { 0x7B, 0x7D, 0xEF, 0xF1 }, { 0x0A, 0x30, 0x20 } },
    { 0x11, { 0x7F, 0x81, 0xF1, 0xF3 }, { 0x0A, 0x30, 0x20 } },
    { 0x11, { 0x83, 0x85, 0xF3, 0xF5 }, { 0x0A, 0x30, 0x20 } },
    { 0x11, { 0x87, 0x89, 0xF5, 0xF7 }, { 0x0A, 0x30, 0x20 } },
    { 0x10, { 0x84, 0x86, 0xF4, 0xF7 }, { 0x0A, 0x30, 0x20 } },
    { 0x11, { 0x87, 0x89, 0xED, 0xEF }, { 0x0A, 0x30, 0x20 } },
    { 0x11, { 0x7B, 0x7D, 0xF3, 0xF5 }, { 0x0A, 0x0A, 0x20 } },
    { 0x11, { 0x79, 0x7B, 0xF4, 0xF6 }, { 0x0A, 0x0A, 0x20 } },
    { 0x0F, { 0x88, 0x8C, 0xF5, 0xF8 }, { 0x0A, 0x0A, 0x20 } },
  };

  const roomdef_t *proomdef; /* was HL */
  bounds_t        *pbounds;  /* was DE */
  mask_t          *pmask;    /* was DE */
  uint8_t          count;    /* was A */
  uint8_t          iters;    /* was B */

  assert(state != NULL);

  wipe_visible_tiles(state);

  proomdef = rooms_and_tunnels[state->room_index - 1];

  setup_doors(state);

  state->roomdef_bounds_index = *proomdef++;

  /* Copy boundaries into state. */
  state->roomdef_object_bounds_count = count = *proomdef; /* count of boundaries */
  pbounds = &state->roomdef_object_bounds[0]; /* Conv: Moved */
  if (count == 0) /* no boundaries */
  {
    proomdef++; /* skip to interior mask */
  }
  else
  {
    proomdef++; /* Conv: Don't re-copy already copied count byte. */
    memcpy(pbounds, proomdef, count * 4);
    proomdef += count * 4; /* skip to interior mask */
  }

  /* Copy interior mask into state->interior_mask_data. */
  state->interior_mask_data_count = iters = *proomdef++; /* count of interior mask */
  assert(iters < 8);
  pmask = &state->interior_mask_data[0]; /* Conv: Moved */
  while (iters--)
  {
    /* Conv: Structures changed from 7 to 8 bytes wide. */
    memcpy(pmask, &interior_mask_data[*proomdef++], sizeof(*pmask));
    pmask++;
  }

  /* Plot all objects (as tiles). */
  iters = *proomdef++; /* Count of objects */
  while (iters--)
  {
    expand_object(state,
                  proomdef[0], /* object index */
                  &state->tile_buf[proomdef[2] * state->tb_columns + proomdef[1]]); /* HL[2] = row, HL[1] = column */
    proomdef += 3;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $6AB5: Expands RLE-encoded objects to a full set of tile references.
 *
 * Format:
 * <w> <h>: width, height
 * Repeat:
 *   <t>:                   emit tile <t>
 *   <0xFF> <64..127> <t>:  emit tiles <t> <t+1> <t+2> .. up to 63 times
 *   <0xFF> <128..254> <t>: emit tile <t> up to 126 times
 *   <0xFF> <0xFF>:         emit <0xFF>
 *
 * \param[in]  state  Pointer to game state.
 * \param[in]  index  Object index to expand.       (was A)
 * \param[out] output Screen location to expand to. (was DE)
 */
void expand_object(tgestate_t *state, object_t index, uint8_t *output)
{
  int                columns;       /* new var */
  const tgeobject_t *obj;           /* was HL */
  int                width, height; /* was B, C */
  int                self_width;    /* was $6AE7 */
  const uint8_t     *data;          /* was HL */
  int                byte;          /* was A */
  int                val;           /* was A' */

  assert(state  != NULL);
  assert(index >= 0 && index < interiorobject__LIMIT);
  assert(output != NULL);

  columns     = state->columns + 1; // Conv: Added.
  // note: columns ought to be 24 here

  obj         = interior_object_defs[index];

  width       = obj->width;
  height      = obj->height;

  self_width  = width;

  data        = &obj->data[0];

  assert(width  > 0);
  assert(height > 0);

  do
  {
    do
    {
expand:

      assert(width  > 0);
      assert(height > 0);

      byte = *data;
      if (byte == interiorobjecttile_ESCAPE)
      {
        byte = *++data;
        if (byte != interiorobjecttile_ESCAPE)
        {
          byte &= 0xF0;
          if (byte >= 128)
            goto run;
          if (byte == 64)
            goto range;
        }
      }

      if (byte)
        *output = byte;
      data++;
      output++;
    }
    while (--width);

    width = self_width;
    output += columns - width; /* move to next row */
  }
  while (--height);

  return;


run:
  byte = *data++ & 0x7F;
  val = *data;
  do
  {
    if (val > 0)
      *output = val;
    output++;

    if (--width == 0) /* ran out of width */
    {
      width = self_width;
      output += columns - width; /* move to next row */

      // val = *data; // needless reload

      if (--height == 0)
        return;
    }
  }
  while (--byte);

  data++;

  goto expand;


range:
  byte = *data++ & 0x0F;
  val = *data;
  do
  {
    *output++ = val++;

    if (--width == 0) /* ran out of width */
    {
      width = self_width;
      output += columns - self_width; /* move to next row */

      if (--height == 0)
        return;
    }
  }
  while (--byte);

  data++;

  goto expand;
}

/* ----------------------------------------------------------------------- */

/**
 * $6B42: Expand all of the tile indices in the tiles buffer to full tiles in
 * the screen buffer.
 *
 * \param[in] state Pointer to game state.
 */
void plot_interior_tiles(tgestate_t *state)
{
  int                rows;          /* added */
  int                columns;       /* added */
  uint8_t           *window_buf;    /* was HL */
  const tileindex_t *tiles_buf;     /* was DE */
  int                rowcounter;    /* was C */
  int                columncounter; /* was B */
  int                iters;         /* was B */
  int                stride;        /* was C */

  assert(state != NULL);

  rows       = state->rows;
  columns    = state->columns + 1;

  window_buf = state->window_buf;
  tiles_buf  = state->tile_buf;

  rowcounter = rows;
  do
  {
    columncounter = columns;
    do
    {
      const tilerow_t *tile_data;   /* was HL */
      uint8_t         *window_buf2; /* was DE */

      assert(tiles_buf >= &state->tile_buf[0]);
      assert(tiles_buf < &state->tile_buf[24*17]);

      tile_data = &interior_tiles[*tiles_buf++].row[0];

      assert(tile_data >= &interior_tiles[0].row[0]);
      assert(tile_data < &interior_tiles[interiorobjecttile_MAX].row[0]);

      window_buf2 = window_buf;

      iters  = 8;
      stride = columns;
      do
      {
        assert(window_buf2 >= &state->window_buf[0]);
        assert(window_buf2 < &state->window_buf[24*16*8]);

        *window_buf2 = *tile_data++;
        window_buf2 += stride;
      }
      while (--iters);

      //tiles_buf++;
      window_buf++; // move to next character position
    }
    while (--columncounter);
    window_buf += 7 * columns; // move to next row
  }
  while (--rowcounter);
}

/* ----------------------------------------------------------------------- */

/**
 * $6B79: Locations of beds.
 *
 * Used by wake_up, character_sleeps and reset_map_and_characters.
 */
uint8_t *const beds[beds_LENGTH] =
{
  &roomdef_3_hut2_right[29],
  &roomdef_3_hut2_right[32],
  &roomdef_3_hut2_right[35],
  &roomdef_5_hut3_right[29],
  &roomdef_5_hut3_right[32],
  &roomdef_5_hut3_right[35],
};

/* ----------------------------------------------------------------------- */

/**
 * $7A26: Door positions.
 *
 * Used by setup_doors, get_door_position, door_handling and
 * bribes_solitary_food.
 */
const doorpos_t door_positions[124] =
{
#define BYTE0(room, other) ((room << 2) | other)
  { BYTE0(room_0_OUTDOORS,           1), { 0xB2, 0x8A,  6 } }, // 0
  { BYTE0(room_0_OUTDOORS,           3), { 0xB2, 0x8E,  6 } },
  { BYTE0(room_0_OUTDOORS,           1), { 0xB2, 0x7A,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0xB2, 0x7E,  6 } },
  { BYTE0(room_34,                   0), { 0x8A, 0xB3,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x10, 0x34, 12 } },
  { BYTE0(room_48,                   0), { 0xCC, 0x79,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x10, 0x34, 12 } },
  { BYTE0(room_28_HUT1LEFT,          1), { 0xD9, 0xA3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x2A, 0x1C, 24 } },
  { BYTE0(room_1_HUT1RIGHT,          0), { 0xD4, 0xBD,  6 } }, // 10
  { BYTE0(room_0_OUTDOORS,           2), { 0x1E, 0x2E, 24 } },
  { BYTE0(room_2_HUT2LEFT,           1), { 0xC1, 0xA3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x2A, 0x1C, 24 } },
  { BYTE0(room_3_HUT2RIGHT,          0), { 0xBC, 0xBD,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x20, 0x2E, 24 } },
  { BYTE0(room_4_HUT3LEFT,           1), { 0xA9, 0xA3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x2A, 0x1C, 24 } },
  { BYTE0(room_5_HUT3RIGHT,          0), { 0xA4, 0xBD,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x20, 0x2E, 24 } },
  { BYTE0(room_21_CORRIDOR,          0), { 0xFC, 0xCA,  6 } }, // 20
  { BYTE0(room_0_OUTDOORS,           2), { 0x1C, 0x24, 24 } },
  { BYTE0(room_20_REDCROSS,          0), { 0xFC, 0xDA,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x1A, 0x22, 24 } },
  { BYTE0(room_15_UNIFORM,           1), { 0xF7, 0xE3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x26, 0x19, 24 } },
  { BYTE0(room_13_CORRIDOR,          1), { 0xDF, 0xE3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x2A, 0x1C, 24 } },
  { BYTE0(room_8_CORRIDOR,           1), { 0x97, 0xD3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x2A, 0x15, 24 } },
  { BYTE0(room_6,                    1), { 0x00, 0x00,  0 } }, // 30
  { BYTE0(room_0_OUTDOORS,           3), { 0x22, 0x22, 24 } },
  { BYTE0(room_1_HUT1RIGHT,          1), { 0x2C, 0x34, 24 } },
  { BYTE0(room_28_HUT1LEFT,          3), { 0x26, 0x1A, 24 } },
  { BYTE0(room_3_HUT2RIGHT,          1), { 0x24, 0x36, 24 } },
  { BYTE0(room_2_HUT2LEFT,           3), { 0x26, 0x1A, 24 } },
  { BYTE0(room_5_HUT3RIGHT,          1), { 0x24, 0x36, 24 } },
  { BYTE0(room_4_HUT3LEFT,           3), { 0x26, 0x1A, 24 } },
  { BYTE0(room_23_BREAKFAST,         1), { 0x28, 0x42, 24 } },
  { BYTE0(room_25_BREAKFAST,         3), { 0x26, 0x18, 24 } },
  { BYTE0(room_23_BREAKFAST,         0), { 0x3E, 0x24, 24 } }, // 40
  { BYTE0(room_21_CORRIDOR,          2), { 0x20, 0x2E, 24 } },
  { BYTE0(room_19_FOOD,              1), { 0x22, 0x42, 24 } },
  { BYTE0(room_23_BREAKFAST,         3), { 0x22, 0x1C, 24 } },
  { BYTE0(room_18_RADIO,             1), { 0x24, 0x36, 24 } },
  { BYTE0(room_19_FOOD,              3), { 0x38, 0x22, 24 } },
  { BYTE0(room_21_CORRIDOR,          1), { 0x2C, 0x36, 24 } },
  { BYTE0(room_22_REDKEY,            3), { 0x22, 0x1C, 24 } },
  { BYTE0(room_22_REDKEY,            1), { 0x2C, 0x36, 24 } },
  { BYTE0(room_23_SOLITARY,          3), { 0x2A, 0x26, 24 } },
  { BYTE0(room_12_CORRIDOR,          1), { 0x42, 0x3A, 24 } }, // 50
  { BYTE0(room_18_RADIO,             3), { 0x22, 0x1C, 24 } },
  { BYTE0(room_17_CORRIDOR,          0), { 0x3C, 0x24, 24 } },
  { BYTE0(room_7_CORRIDOR,           2), { 0x1C, 0x22, 24 } },
  { BYTE0(room_15_UNIFORM,           0), { 0x40, 0x28, 24 } },
  { BYTE0(room_14_TORCH,             2), { 0x1E, 0x28, 24 } },
  { BYTE0(room_16_CORRIDOR,          1), { 0x22, 0x42, 24 } },
  { BYTE0(room_14_TORCH,             3), { 0x22, 0x1C, 24 } },
  { BYTE0(room_16_CORRIDOR,          0), { 0x3E, 0x2E, 24 } },
  { BYTE0(room_13_CORRIDOR,          2), { 0x1A, 0x22, 24 } }, // 60
  { BYTE0(room_0_OUTDOORS,           0), { 0x44, 0x30, 24 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x20, 0x30, 24 } },
  { BYTE0(room_13_CORRIDOR,          0), { 0x4A, 0x28, 24 } },
  { BYTE0(room_11_PAPERS,            2), { 0x1A, 0x22, 24 } },
  { BYTE0(room_7_CORRIDOR,           0), { 0x40, 0x24, 24 } },
  { BYTE0(room_16_CORRIDOR,          2), { 0x1A, 0x22, 24 } },
  { BYTE0(room_10_LOCKPICK,          0), { 0x36, 0x35, 24 } },
  { BYTE0(room_8_CORRIDOR,           2), { 0x17, 0x26, 24 } },
  { BYTE0(room_9_CRATE,              0), { 0x36, 0x1C, 24 } },
  { BYTE0(room_8_CORRIDOR,           2), { 0x1A, 0x22, 24 } }, // 70
  { BYTE0(room_12_CORRIDOR,          0), { 0x3E, 0x24, 24 } },
  { BYTE0(room_17_CORRIDOR,          2), { 0x1A, 0x22, 24 } },
  { BYTE0(room_29_SECOND_TUNNEL_START, 1), { 0x36, 0x36, 24 } },
  { BYTE0(room_9_CRATE,              3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_52,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_30,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_30,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_31,                   2), { 0x38, 0x26, 12 } },
  { BYTE0(room_30,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_36,                   3), { 0x38, 0x0A, 12 } }, // 80
  { BYTE0(room_31,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_32,                   2), { 0x0A, 0x34, 12 } },
  { BYTE0(room_32,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_33,                   3), { 0x20, 0x34, 12 } },
  { BYTE0(room_33,                   1), { 0x40, 0x34, 12 } },
  { BYTE0(room_35,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_35,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_34,                   2), { 0x0A, 0x34, 12 } },
  { BYTE0(room_36,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_35,                   2), { 0x38, 0x1C, 12 } }, // 90
  { BYTE0(room_37,                   0), { 0x3E, 0x22, 24 } }, /* Tunnel entrance */
  { BYTE0(room_2_HUT2LEFT,           2), { 0x10, 0x34, 12 } },
  { BYTE0(room_38,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_37,                   2), { 0x10, 0x34, 12 } },
  { BYTE0(room_39,                   1), { 0x40, 0x34, 12 } },
  { BYTE0(room_38,                   3), { 0x20, 0x34, 12 } },
  { BYTE0(room_40,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_38,                   2), { 0x38, 0x54, 12 } },
  { BYTE0(room_40,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_41,                   3), { 0x38, 0x0A, 12 } }, // 100
  { BYTE0(room_41,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_42,                   2), { 0x38, 0x26, 12 } },
  { BYTE0(room_41,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_45,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_45,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_44,                   2), { 0x38, 0x1C, 12 } },
  { BYTE0(room_43,                   1), { 0x20, 0x34, 12 } },
  { BYTE0(room_44,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_42,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_43,                   3), { 0x20, 0x34, 12 } }, // 110
  { BYTE0(room_46,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_39,                   2), { 0x38, 0x1C, 12 } },
  { BYTE0(room_47,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_46,                   3), { 0x20, 0x34, 12 } },
  { BYTE0(room_50_BLOCKED_TUNNEL,    0), { 0x64, 0x34, 12 } },
  { BYTE0(room_47,                   2), { 0x38, 0x56, 12 } },
  { BYTE0(room_50_BLOCKED_TUNNEL,    1), { 0x38, 0x62, 12 } },
  { BYTE0(room_49,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_49,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_48,                   2), { 0x38, 0x1C, 12 } }, // 120
  { BYTE0(room_51,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_29_SECOND_TUNNEL_START, 3), { 0x20, 0x34, 12 } },
  { BYTE0(room_52,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_51,                   2), { 0x38, 0x54, 12 } },
#undef BYTE0
};

/* ----------------------------------------------------------------------- */

/**
 * $7AC9: Check for 'pick up', 'drop' and 'use' input.
 *
 * \param[in] state Pointer to game state.
 * \param[in] input User input event.
 */
void process_player_input_fire(tgestate_t *state, input_t input)
{
  assert(state != NULL);

  switch (input)
  {
  case input_UP_FIRE:
    pick_up_item(state);
    break;
  case input_DOWN_FIRE:
    drop_item(state);
    break;
  case input_LEFT_FIRE:
    use_item_A(state);
    break;
  case input_RIGHT_FIRE:
    use_item_B(state);
    break;
  case input_FIRE:
    break;
  default:
    assert("Non-fire input" == NULL);
    break;
  }

  return;
}

/**
 * $7AF0: Use item 'B'.
 *
 * \param[in] state Pointer to game state.
 */
void use_item_B(tgestate_t *state)
{
  assert(state != NULL);

  use_item_common(state, state->items_held[1]);
}

/**
 * $7AF5: Use item 'A'.
 *
 * \param[in] state Pointer to game state.
 */
void use_item_A(tgestate_t *state)
{
  assert(state != NULL);

  use_item_common(state, state->items_held[0]);
}

/**
 * $7AFB: Use item common.
 *
 * \param[in] state Pointer to game state.
 */
void use_item_common(tgestate_t *state, item_t item)
{
  /**
   * $7B16: Item actions jump table.
   */
  static const item_action_t item_actions_jump_table[item__LIMIT] =
  {
    action_wiresnips,
    action_shovel,
    action_lockpick,
    action_papers,
    NULL,
    action_bribe,
    action_uniform,
    NULL,
    action_poison,
    action_red_key,
    action_yellow_key,
    action_green_key,
    action_red_cross_parcel,
    NULL,
    NULL,
    NULL,
  };

  assert(state != NULL);

  if (item == item_NONE)
    return;

  assert(item >= 0 && item < item__LIMIT);

  memcpy(&state->saved_pos, &state->vischars[0].mi.pos, sizeof(pos_t));

  // TODO: Check, is a jump to NULL avoided by some mechanism?
  assert(item_actions_jump_table[item] != NULL);

  item_actions_jump_table[item](state);
}

/* ----------------------------------------------------------------------- */

/**
 * $7B36: Pick up item.
 *
 * \param[in] state Pointer to game state.
 */
void pick_up_item(tgestate_t *state)
{
  itemstruct_t *itemstr; /* was ? */
  item_t       *pitem;   /* was DE */
  item_t        item;    /* was A */
  attribute_t   attrs; 	 /* was ? */

  assert(state != NULL);

  if (state->items_held[0] != item_NONE &&
      state->items_held[1] != item_NONE)
    return; /* No spare slots. */

  itemstr = find_nearby_item(state);
  if (itemstr == NULL)
    return; /* No item nearby. */

  /* Locate the empty item slot. */
  pitem = &state->items_held[0];
  item = *pitem;
  if (item != item_NONE)
    pitem++;
  *pitem = itemstr->item_and_flags & (itemstruct_ITEM_MASK | itemstruct_ITEM_FLAG_UNKNOWN); // I don't see this unknown flag used anywhere else in the code. Is it a real flag, or evidence that the items pool was originally larger (ie. up to 32 objects rather than 16).

  if (state->room_index == room_0_OUTDOORS)
  {
    /* Outdoors. */
    supertile_plot_all(state);
  }
  else
  {
    /* Indoors. */
    setup_room(state);
    plot_interior_tiles(state);
    attrs = choose_game_window_attributes(state);
    set_game_window_attributes(state, attrs);
  }

  if ((itemstr->item_and_flags & itemstruct_ITEM_FLAG_HELD) == 0)
  {
    /* Have picked up an item not previously held. */
    itemstr->item_and_flags |= itemstruct_ITEM_FLAG_HELD;
    increase_morale_by_5_score_by_5(state);
  }

  itemstr->room_and_flags = 0;
  itemstr->target         = 0;

  draw_all_items(state);
  play_speaker(state, sound_PICK_UP_ITEM);
}

/* ----------------------------------------------------------------------- */

/**
 * $7B8B: Drop item.
 *
 * \param[in] state Pointer to game state.
 */
void drop_item(tgestate_t *state)
{
  item_t      item;    /* was A */
  item_t     *itemp;   /* was HL */
  item_t      tmpitem; /* was A */
  attribute_t attrs;   /* was A */

  assert(state != NULL);

  /* Drop the first item. */
  item = state->items_held[0];
  if (item == item_NONE)
    return;

  if (item == item_UNIFORM)
    state->vischars[0].mi.spriteset = &sprites[sprite_PRISONER_FACING_TOP_LEFT_4];

  /* Shuffle items down. */
  itemp = &state->items_held[1];
  tmpitem = *itemp;
  *itemp-- = item_NONE;
  *itemp = tmpitem;

  draw_all_items(state);
  play_speaker(state, sound_DROP_ITEM);
  attrs = choose_game_window_attributes(state);
  set_game_window_attributes(state, attrs);

  drop_item_tail(state, item);
}

/**
 * $7BB5: Drop item tail part.
 *
 * Called by drop_item and action_red_cross_parcel.
 *
 * \param[in] state Pointer to game state.
 * \param[in] item  Item.                  (was A)
 */
void drop_item_tail(tgestate_t *state, item_t item)
{
  itemstruct_t *itemstr;  /* was HL */
  room_t        room;     /* was A */
  tinypos_t    *outpos;   /* was DE */
  pos_t        *inpos;    /* was HL */

  assert(state != NULL);
  assert(item >= 0 && item < item__LIMIT);

  itemstr = item_to_itemstruct(state, item);
  room = state->room_index;
  itemstr->room_and_flags = room; /* Set object's room index. */
  if (room == room_0_OUTDOORS)
  {
    /* Outdoors. */

    // TODO: Hoist common code.
    outpos = &itemstr->pos;
    inpos  = &state->vischars[0].mi.pos;

    pos_to_tinypos(inpos, outpos);
    outpos->height = 0;

    drop_item_tail_exterior(itemstr);
  }
  else
  {
    /* Indoors. */

    outpos = &itemstr->pos;
    inpos  = &state->vischars[0].mi.pos;

    outpos->x      = inpos->x;
    outpos->y      = inpos->y;
    outpos->height = 5;

    drop_item_tail_interior(itemstr);
  }
}

/**
 * $7BD0: Assign target member for dropped exterior objects.
 *
 * Moved out to provide entry point.
 *
 * Called by drop_item_tail and item_discovered.
 *
 * \param[in] itemstr Pointer to item struct. (was HL)
 */
void drop_item_tail_exterior(itemstruct_t *itemstr)
{
  uint8_t *t;

  assert(itemstr != NULL);

  t = (uint8_t *) &itemstr->target;

  t[0] = (0x40 + itemstr->pos.y - itemstr->pos.x) * 2;
  t[1] = 0x100 - itemstr->pos.x - itemstr->pos.y - itemstr->pos.height; /* Conv: 0x100 is 0 in the original. */
}

/**
 * $7BF2: Assign target member for dropped interior objects.
 *
 * Moved out to provide entry point.
 *
 * Called by drop_item_tail and item_discovered.
 *
 * \param[in] itemstr Pointer to item struct. (was HL)
 */
void drop_item_tail_interior(itemstruct_t *itemstr)
{
  uint8_t *t;

  assert(itemstr != NULL);

  t = (uint8_t *) &itemstr->target;

  // This was a call to divide_by_8_with_rounding, but that expects
  // separate hi and lo arguments, which is not worth the effort of
  // mimicing the original code.
  //
  // This needs to go somewhere more general.
#define divround(x) (((x) + 4) >> 3)

  t[0] = divround((0x200 + itemstr->pos.y - itemstr->pos.x) * 2);
  t[1] = divround(0x800 - itemstr->pos.x - itemstr->pos.y - itemstr->pos.height);

#undef divround
}

/* ----------------------------------------------------------------------- */

/**
 * $7C26: Convert an item_t to an itemstruct pointer.
 *
 * \param[in] item Item index. (was A)
 *
 * \return Pointer to itemstruct.
 */
itemstruct_t *item_to_itemstruct(tgestate_t *state, item_t item)
{
  assert(state != NULL);
  assert(item >= 0 && item < item__LIMIT);

  return &state->item_structs[item];
}

/* ----------------------------------------------------------------------- */

/**
 * $7C33: Draw both held items.
 *
 * \param[in] state Pointer to game state.
 */
void draw_all_items(tgestate_t *state)
{
  assert(state != NULL);

  /* Conv: Converted screen pointers into offsets. */
  draw_item(state, state->items_held[0], 0x5087 - SCREEN_START_ADDRESS);
  draw_item(state, state->items_held[1], 0x508A - SCREEN_START_ADDRESS);
}

/**
 * $7C46: Draw a single held item.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] index  Item index.    (was A)
 * \param[in] dstoff Screen offset. (was HL) Assumed to be in the lower third of the display.
 */
void draw_item(tgestate_t *state, item_t index, size_t dstoff)
{
  assert(state != NULL);

  uint8_t *const screen = &state->speccy->screen[0];

  attribute_t    *attrs;  /* was HL */
  attribute_t     attr;   /* was A */
  const sprite_t *sprite; /* was HL */

  /* Wipe item. */
  screen_wipe(state, 2, 16, screen + dstoff);

  if (index == item_NONE)
    return;

  assert(index >= 0 && index < item__LIMIT);

  /* Set screen attributes. */
  attrs = (dstoff & 0xFF) + &state->speccy->attributes[0x5A00 - SCREEN_ATTRIBUTES_START_ADDRESS];
  attr = state->item_attributes[index];
  attrs[0] = attr;
  attrs[1] = attr;

  /* Move to next attribute row. */
  attrs += state->width;
  attrs[0] = attr;
  attrs[1] = attr;

  /* Plot the item bitmap. */
  sprite = &item_definitions[index];
  plot_bitmap(state, sprite->width, sprite->height, sprite->bitmap, screen + dstoff);
}

/* ----------------------------------------------------------------------- */

/**
 * $7C82: Returns an item within range of the hero.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Pointer to itemstruct, or NULL.
 */
itemstruct_t *find_nearby_item(tgestate_t *state)
{
  uint8_t       radius;  /* was C */
  uint8_t       iters;   /* was B */
  itemstruct_t *itemstr; /* was HL */

  assert(state != NULL);

  /* Select a pick up radius. */
  radius = 1; /* Outdoors. */
  if (state->room_index > room_0_OUTDOORS)
    radius = 6; /* Indoors. */

  /* For all items. */
  iters   = item__LIMIT;
  itemstr = &state->item_structs[0];
  do
  {
    if (itemstr->room_and_flags & itemstruct_ROOM_FLAG_NEARBY_7)
    {
      uint8_t *structcoord; /* was ? */
      uint8_t *herocoord;   /* was ? */
      uint8_t  coorditers;  /* was ? */

      // FIXME: Candidate for loop unrolling.
      structcoord = &itemstr->pos.x;
      herocoord   = &state->hero_map_position.x;
      coorditers = 2;
      /* Range check. */
      do
      {
        uint8_t A;

        A = *herocoord++; /* get hero map position */
        if (A - radius >= *structcoord || A + radius < *structcoord)
          goto next;
        structcoord++;
      }
      while (--coorditers);

      return itemstr;
    }

next:
    itemstr++;
  }
  while (--iters);

  return NULL;
}

/* ----------------------------------------------------------------------- */

/**
 * $7CBE: Plot a bitmap.
 *
 * This is a straight copy without a mask.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] width  Width, in bytes.     (was B)
 * \param[in] height Height.              (was C)
 * \param[in] src    Source address.      (was DE)
 * \param[in] dst    Destination address. (was HL)
 */
void plot_bitmap(tgestate_t    *state,
                 uint8_t        width,
                 uint8_t        height,
                 const uint8_t *src,
                 uint8_t       *dst)
{
  assert(state != NULL);
  assert(width  > 0);
  assert(height > 0);
  assert(src   != NULL);
  assert(dst   != NULL);

  do
  {
    memcpy(dst, src, width);
    src += width;
    dst = get_next_scanline(state, dst);
  }
  while (--height);
}

/* ----------------------------------------------------------------------- */

/**
 * $7CD4: Wipe the screen.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] width  Width, in bytes.     (was B)
 * \param[in] height Height.              (was C)
 * \param[in] dst    Destination address. (was HL)
 */
void screen_wipe(tgestate_t *state,
                 uint8_t     width,
                 uint8_t     height,
                 uint8_t    *dst)
{
  assert(state != NULL);
  assert(width  > 0);
  assert(height > 0);
  assert(dst   != NULL);

  do
  {
    memset(dst, 0, width);
    dst = get_next_scanline(state, dst);
  }
  while (--height);
}

/* ----------------------------------------------------------------------- */

/**
 * $7CE9: Given a screen address, return the same position on the next
 * scanline.
 *
 * \param[in] state Pointer to game state.
 * \param[in] slp   Scanline pointer.      (was HL)
 *
 * \return Subsequent scanline pointer.
 */
uint8_t *get_next_scanline(tgestate_t *state, uint8_t *slp)
{
  assert(state != NULL);
  assert(slp   != NULL);

  uint8_t *const screen = &state->speccy->screen[0]; /* added */
  uint16_t       offset; /* was HL */
  uint16_t       delta;  /* was DE */

  offset = slp - screen;

  offset += 0x0100;
  if (offset & 0x0700)
    return screen + offset; /* line count didn't rollover */

  if ((offset & 0xFF) >= 0xE0)
    delta = 0xFF20;
  else
    delta = 0xF820;

  offset += delta;

  return screen + offset;
}

/* ----------------------------------------------------------------------- */

/**
 * $7D15: Add a message to the display queue.
 *
 * Conversion note: The original code accepts BC combined as the message
 * index. However only one of the callers sets up C. We therefore ignore the
 * second argument here, treating it as zero.
 *
 * \param[in] state         Pointer to game state.
 * \param[in] message_index The message_t to display. (was B)
 */
void queue_message_for_display(tgestate_t *state,
                               message_t   message_index)
{
  uint8_t *qp; /* was HL */

  assert(state != NULL);
  assert(message_index >= 0 && message_index < message__LIMIT);

  qp = state->messages.queue_pointer; /* insertion point pointer */
  if (*qp == message_QUEUE_END)
    return; /* Queue full. */

  /* Is this message pending? */
  if (qp[-2] == message_index && qp[-1] == 0)
    return; /* Yes - skip adding it. */

  /* Add it to the queue. */
  qp[0] = message_index;
  qp[1] = 0;
  state->messages.queue_pointer = qp + 2;
}

/* ----------------------------------------------------------------------- */

/**
 * $7D2F: Plot a single glyph (indirectly).
 *
 * \param[in] pcharacter Pointer to character to plot. (was HL)
 * \param[in] output     Where to plot.                (was DE)
 *
 * \return Pointer to next character along.
 */
uint8_t *plot_glyph(const char *pcharacter, uint8_t *output)
{
  assert(pcharacter != NULL);
  assert(output     != NULL);

  return plot_single_glyph(*pcharacter, output);
}

/**
 * $7D30: Plot a single glyph.
 *
 * Conv: Characters are specified in ASCII.
 *
 * \param[in] character Character to plot. (was HL)
 * \param[in] output    Where to plot.     (was DE)
 *
 * \return Pointer to next character along. (was DE)
 */
uint8_t *plot_single_glyph(int character, uint8_t *output)
{
  const tile_t    *glyph;        /* was HL */
  const tilerow_t *row;          /* was HL */
  int              iters;        /* was B */
  uint8_t         *saved_output; /* was stacked */

  assert(character < 256);
  assert(output != NULL);

  glyph        = &bitmap_font[ascii_to_font[character]];
  row          = &glyph->row[0];
  saved_output = output;

  iters = 8;
  do
  {
    *output = *row++;
    output += 256; /* Advance to next row. */
  }
  while (--iters);

  /* Return the position of the next character. */
  return ++saved_output;
}

/* ----------------------------------------------------------------------- */

#define message_NEXT (1 << 7)

/**
 * $7D48: Incrementally display queued game messages.
 *
 * \param[in] state Pointer to game state.
 */
void message_display(tgestate_t *state)
{
  uint8_t     index;   /* was A */
  const char *pmsgchr; /* was HL */
  uint8_t    *pscr;    /* was DE */

  assert(state != NULL);

  if (state->messages.display_delay > 0)
  {
    state->messages.display_delay--;
    return;
  }

  index = state->messages.display_index;
  if (index == message_NEXT)
  {
    next_message(state);
  }
  else if (index > message_NEXT)
  {
    wipe_message(state);
  }
  else
  {
    pmsgchr = state->messages.current_character;
    pscr    = &state->speccy->screen[screen_text_start_address + index];
    (void) plot_glyph(pmsgchr, pscr);

    state->messages.display_index = index + 1; // Conv: Original used (pscr & 31). CHECK

    if (*++pmsgchr == '\0') /* end of string (0xFF in original game) */
    {
      /* Leave the message for 31 turns, then wipe it. */
      state->messages.display_delay = 31;
      state->messages.display_index |= message_NEXT;
    }
    else
    {
      state->messages.current_character = pmsgchr;
    }
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $7D87: Incrementally wipe away any on-screen game message.
 *
 * Called while messages.display_index > 128.
 *
 * \param[in] state Pointer to game state.
 */
void wipe_message(tgestate_t *state)
{
  int      index; /* was A */
  uint8_t *scr;   /* was DE */

  assert(state != NULL);

  index = state->messages.display_index;

  state->messages.display_index = --index;

  /* Conv: Must remove message_NEXT from index to keep screen address sane. */
  index -= message_NEXT;
  assert(index < 128);

  scr = &state->speccy->screen[screen_text_start_address + index];

  /* Plot a SPACE character. */
  (void) plot_single_glyph(' ', scr);
}

/* ----------------------------------------------------------------------- */

/**
 * $7D99: Change to displaying the next queued game message.
 *
 * Called when messages.display_index == 128.
 *
 * \param[in] state Pointer to game state.
 */
void next_message(tgestate_t *state)
{
  /**
   * $7DCD: Table of game messages.
   *
   * Conv: These are 0xFF terminated in the original game.
   */
  static const char *messages_table[message__LIMIT] =
  {
    "MISSED ROLL CALL",
    "TIME TO WAKE UP",
    "BREAKFAST TIME",
    "EXERCISE TIME",
    "TIME FOR BED",
    "THE DOOR IS LOCKED",
    "IT IS OPEN",
    "INCORRECT KEY",
    "ROLL CALL",
    "RED CROSS PARCEL",
    "PICKING THE LOCK",
    "CUTTING THE WIRE",
    "YOU OPEN THE BOX",
    "YOU ARE IN SOLITARY",
    "WAIT FOR RELEASE",
    "MORALE IS ZERO",
    "ITEM DISCOVERED",

    "HE TAKES THE BRIBE", /* $F026 */
    "AND ACTS AS DECOY",  /* $F039 */
    "ANOTHER DAY DAWNS"   /* $F04B */
  };

  uint8_t    *qp;      /* was DE */
  const char *message; /* was HL */

  assert(state != NULL);

  qp = &state->messages.queue[0];
  if (state->messages.queue_pointer == qp)
    return;

  message = messages_table[*qp];

  state->messages.current_character = message;
  memmove(state->messages.queue, state->messages.queue + 2, 16);
  state->messages.queue_pointer -= 2;
  state->messages.display_index = 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $9D7B: Main game loop.
 */
void main_loop(tgestate_t *state)
{
  assert(state != NULL);

  check_morale(state);
  keyscan_break(state);
  message_display(state);
  process_player_input(state);
  in_permitted_area(state);
  called_from_main_loop_3(state);
  move_characters(state);
  follow_suspicious_character(state);
  purge_visible_characters(state);
  spawn_characters(state);
  mark_nearby_items(state);
  ring_bell(state);
  called_from_main_loop_9(state);
  move_map(state);
  message_display(state);
  ring_bell(state);
  locate_vischar_or_itemstruct_then_plot(state);
  plot_game_window(state);
  ring_bell(state);
  if (state->day_or_night != 0)
    nighttime(state);
  if (state->room_index > room_0_OUTDOORS)
    interior_delay_loop(state);
  wave_morale_flag(state);
  if ((state->game_counter & 63) == 0)
    dispatch_timed_event(state);

//  // safety kick
//  state->speccy->kick(state->speccy);
}

/* ----------------------------------------------------------------------- */

/**
 * $9DCF: Check morale level, report if (near) zero and inhibit player
 * control.
 *
 * \param[in] state Pointer to game state.
 */
void check_morale(tgestate_t *state)
{
  assert(state != NULL);

  if (state->morale >= 2)
    return;

  queue_message_for_display(state, message_MORALE_IS_ZERO);

  /* Inhibit user input. */
  state->morale_2 = 0xFF;

  /* Immediately assume automatic control of hero. */
  state->automatic_player_counter = 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $9DE5: Check for break keypress.
 *
 * If pressed, then clear the screen and confirm with the player that they
 * want to reset the game. Reset it if requested.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Returns only if game not canceled.
 */
void keyscan_break(tgestate_t *state)
{
  assert(state != NULL);

  /* Is shift-space (break) pressed? */
  if ((state->speccy->in(state->speccy, port_KEYBOARD_SPACESYMSHFTMNB) & 0x01) != 0 ||
      (state->speccy->in(state->speccy, port_KEYBOARD_SHIFTZXCV)       & 0x01) != 0)
    return; /* not pressed */

  screen_reset(state);
  if (user_confirm(state) == 0)
  {
    reset_game(state);
    NEVER_RETURNS;
  }

  if (state->room_index == room_0_OUTDOORS)
  {
    reset_outdoors(state);
  }
  else
  {
    enter_room(state); /* doesn't return (jumps to main_loop) */
    NEVER_RETURNS;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $9E07: Process player input.
 *
 * \param[in] state Pointer to game state.
 */
void process_player_input(tgestate_t *state)
{
  input_t input; /* was A */

  assert(state != NULL);

  /* Is morale remaining? */
  if (state->morale_1 || state->morale_2)
    return; /* None remains. */

  if (state->vischars[0].flags & (vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE))
  {
    /* Picking a lock, or cutting through a wire fence. */

    /* Hold off on automatic control. */
    state->automatic_player_counter = 31;

    if (state->vischars[0].flags == vischar_FLAGS_PICKING_LOCK)
      picking_a_lock(state);
    else
      snipping_wire(state);

    return;
  }

  input = input_routine(state);
  if (input == input_NONE)
  {
    /* No input */

    if (state->automatic_player_counter == 0)
      return;

    /* No player input: count down. */
    state->automatic_player_counter--;
    input = input_NONE;
  }
  else
  {
    /* Input received */

    /* Wait 31 turns until automatic control. */
    state->automatic_player_counter = 31;

    if (state->hero_in_bed || state->hero_in_breakfast)
    {
      if (state->hero_in_bed == 0)
      {
        /* Hero was at breakfast. */
        state->vischars[0].target         = 0x002B;
        state->vischars[0].mi.pos.x       = 0x34;
        state->vischars[0].mi.pos.y       = 0x3E;
        roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_EMPTY_BENCH;
        state->hero_in_breakfast = 0;
      }
      else
      {
        /* Hero was in bed. */
        state->vischars[0].target         = 0x012C;
        state->vischars[0].p04.x          = 0x2E;
        state->vischars[0].p04.y          = 0x2E;
        state->vischars[0].mi.pos.x       = 0x2E;
        state->vischars[0].mi.pos.y       = 0x2E;
        state->vischars[0].mi.pos.height  = 24;
        roomdef_2_hut2_left[roomdef_2_BED] = interiorobject_EMPTY_BED;
        state->hero_in_bed = 0;
      }

      setup_room(state);
      plot_interior_tiles(state);
    }

    if (input >= input_FIRE)
    {
      process_player_input_fire(state, input);
      input = vischar_BYTE13_BIT7;
    }
  }

  if (state->vischars[0].b0D != input)
    state->vischars[0].b0D = input | vischar_BYTE13_BIT7;
}

/* ----------------------------------------------------------------------- */

/**
 * $9E98: Picking a lock.
 *
 * Locks the player out until the lock is picked.
 *
 * \param[in] state Pointer to game state.
 */
void picking_a_lock(tgestate_t *state)
{
  assert(state != NULL);

  if (state->player_locked_out_until != state->game_counter)
    return;

  *state->ptr_to_door_being_lockpicked &= ~gates_and_doors_LOCKED; /* unlock */
  queue_message_for_display(state, message_IT_IS_OPEN);

  state->vischars[0].flags &= ~(vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE);
}

/* ----------------------------------------------------------------------- */

/**
 * $9EB2: Snipping wire.
 *
 * Locks the player out until the wire is snipped.
 *
 * \param[in] state Pointer to game state.
 */
void snipping_wire(tgestate_t *state)
{
  /**
   * $9EE0: Change of direction table used when wire is snipped?
   */
  static const uint8_t table_9EE0[4] =
  {
    vischar_BYTE13_BIT7 | 4,
    vischar_BYTE13_BIT7 | 7,
    vischar_BYTE13_BIT7 | 8,
    vischar_BYTE13_BIT7 | 5
  };

  int delta; /* was A */

  assert(state != NULL);

  delta = state->player_locked_out_until - state->game_counter;
  if (delta)
  {
    if (delta < 4)
      state->vischars[0].b0D = table_9EE0[state->vischars[0].b0E & 3]; // new direction?
  }
  else
  {
    state->vischars[0].b0E = delta & 3; // set direction?
    state->vischars[0].b0D = vischar_BYTE13_BIT7;
    state->vischars[0].mi.pos.height = 24;

    /* Conv: The original code jumps into the tail end of picking_a_lock()
     * above to do this. */
    state->vischars[0].flags &= ~(vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $9F21: In permitted area.
 *
 * \param[in] state Pointer to game state.
 */
void in_permitted_area(tgestate_t *state)
{
  /**
   * $9EF9: Variable-length arrays, 0xFF terminated.
   */
  static const uint8_t byte_9EF9[] = { 0x82, 0x82, 0xFF                         };
  static const uint8_t byte_9EFC[] = { 0x83, 0x01, 0x01, 0x01, 0xFF             };
  static const uint8_t byte_9F01[] = { 0x01, 0x01, 0x01, 0x00, 0x02, 0x02, 0xFF };
  static const uint8_t byte_9F08[] = { 0x01, 0x01, 0x95, 0x97, 0x99, 0xFF       };
  static const uint8_t byte_9F0E[] = { 0x83, 0x82, 0xFF                         };
  static const uint8_t byte_9F11[] = { 0x99, 0xFF                               };
  static const uint8_t byte_9F13[] = { 0x01, 0xFF                               };

  /**
   * $9EE4: Maps bytes to pointers to the above arrays.
   */
  static const byte_to_pointer_t byte_to_pointer[7] =
  {
    { 42, &byte_9EF9[0] },
    {  5, &byte_9EFC[0] },
    { 14, &byte_9F01[0] },
    { 16, &byte_9F08[0] },
    { 44, &byte_9F0E[0] },
    { 43, &byte_9F11[0] },
    { 45, &byte_9F13[0] },
  };

  pos_t       *vcpos;     /* was HL */
  tinypos_t   *pos;       /* was DE */
  attribute_t  attr;      /* was A */
  location_t  *locptr;    /* was HL */
  uint8_t      A;
  location_t   CA;
  uint16_t     BC;
  uint8_t      red_flag;  /* was A */

  assert(state != NULL);

  vcpos = &state->vischars[0].mi.pos;
  pos = &state->hero_map_position;
  if (state->room_index == room_0_OUTDOORS)
  {
    /* Outdoors. */

    pos_to_tinypos(vcpos, pos);

    /* (217 * 8, 137 * 8) */
    if (state->vischars[0].scrx >= 0x06C8 ||
        state->vischars[0].scry >= 0x0448)
    {
      escaped(state);
      return;
    }
  }
  else
  {
    /* Indoors. */

    pos->x      = vcpos->x;
    pos->y      = vcpos->y;
    pos->height = vcpos->height;
  }

  /* Red flag if picking a lock, or cutting wire. */

  if (state->vischars[0].flags & (vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE))
    goto set_flag_red;

  /* At night, home room is the only safe place. */

  if (state->clock >= 100) /* night time */
  {
    if (state->room_index == room_2_HUT2LEFT)
      goto set_flag_green;
    else
      goto set_flag_red;
  }

  if (state->morale_1)
    goto set_flag_green;

  locptr = &state->vischars[0].target;
  CA = *locptr;
  if ((CA & 0xFF) & vischar_BYTE2_BIT7)
    CA += 1 << 8; // was 'C++'

  if ((CA & 0xFF) == 0xFF)
  {
    //uint8_t A;

    A = ((*locptr & 0xF8) == 8) ? 1 : 2; // *locptr is a byte read in original
    if (in_permitted_area_end_bit(state, A) == 0)
      goto set_flag_green;
    else
      goto set_flag_red;
  }
  else
  {
    const byte_to_pointer_t *tab;   /* was HL */
    uint8_t                  iters; /* was B */
    const uint8_t           *HL;

    A = CA & 0xFF; // added to coax A back from CA

    A &= ~vischar_BYTE2_BIT7;
    tab = &byte_to_pointer[0]; // table mapping bytes to offsets
    iters = NELEMS(byte_to_pointer);
    do
    {
      if (A == tab->byte)
        goto found;
      tab++;
    }
    while (--iters);
    goto set_flag_green;

found:
    HL = tab->pointer;
    HL += CA >> 8; // original code used B=0
    if (in_permitted_area_end_bit(state, *HL) == 0)
      goto set_flag_green;

    if (state->vischars[0].target & vischar_BYTE2_BIT7) // low byte only
      HL++;

    BC = 0;
    for (;;)
    {
      const uint8_t *HL2;

      HL2 = HL; // was PUSH HL
      HL2 += BC;
      A = *HL2;
      if (A == 0xFF) /* end of list */
        goto pop_and_set_flag_red;
      if (in_permitted_area_end_bit(state, A) == 0)
        goto set_target_then_set_flag_green;
      BC++;
    }

set_target_then_set_flag_green:
    // oddness here needs checking out. only 'B' is loaded from the target, but
    // set_hero_target_location needs BC
    // BC is reset to zero above, so C must be the loop counter
    set_hero_target_location(state, ((state->vischars[0].target << 8) & 0xFF) | (BC & 0xFF));
    goto set_flag_green;

pop_and_set_flag_red:
    goto set_flag_red;
  }

set_flag_green:
  red_flag = 0;
  attr = attribute_BRIGHT_GREEN_OVER_BLACK;

flag_select:
  state->red_flag = red_flag;
  if (attr == state->speccy->attributes[morale_flag_attributes_offset])
    return; /* flag is already the correct colour */

  if (attr == attribute_BRIGHT_GREEN_OVER_BLACK)
    state->bell = bell_STOP;

  set_morale_flag_screen_attributes(state, attr);
  return;

set_flag_red:
  attr = attribute_BRIGHT_RED_OVER_BLACK;
  if (state->speccy->attributes[morale_flag_attributes_offset] == attr)
    return; /* flag is already red */

  state->vischars[0].b0D = 0;
  red_flag = 255;
  goto flag_select;
}

/**
 * $A007: In permitted area (end bit).
 *
 * \param[in] state          Pointer to game state.
 * \param[in] room_and_flags Room index + flag in bit 7
 *
 * \return 0 if the position is within the area bounds, 1 otherwise.
 */
int in_permitted_area_end_bit(tgestate_t *state, uint8_t room_and_flags)
{
  room_t *proom; /* was HL */

  assert(state != NULL);
  // assert(room_and_flags); // FIXME devise precondition

  // FUTURE: Just dereference proom once.

  proom = &state->room_index;

  if (room_and_flags & (1 << 7)) // flag is?
    return *proom == (room_and_flags & 0x7F); /* Return room only. */

  if (*proom != room_0_OUTDOORS)
    return 0; /* Indoors. */

  return within_camp_bounds(0 /* area: corridor to yard */,
                            &state->hero_map_position);
}

/**
 * $A01A: Is the specified position within the bounds of the indexed area?
 *
 * \param[in] area Index (0..2) into permitted_bounds[] table. (was A)
 * \param[in] pos  Pointer to position. (was HL)
 *
 * \return 0 if the position is within the area bounds, 1 otherwise.
 */
int within_camp_bounds(uint8_t          area, // ought to be an enum
                       const tinypos_t *pos)
{
  /**
   * $9F15: Boundings of the three main exterior areas.
   */
  static const bounds_t permitted_bounds[3] =
  {
    { 0x56,0x5E, 0x3D,0x48 }, /* corridor to yard */
    { 0x4E,0x84, 0x47,0x74 }, /* hut area */
    { 0x4F,0x69, 0x2F,0x3F }, /* yard area */
  };

  const bounds_t *bounds; /* was HL */

  assert(area < NELEMS(permitted_bounds));
  assert(pos != NULL);

  bounds = &permitted_bounds[area];
  return pos->x < bounds->x0 || pos->x >= bounds->x1 ||
         pos->y < bounds->y0 || pos->y >= bounds->y1;
}

/* ----------------------------------------------------------------------- */

/**
 * $A035: Wave the morale flag.
 *
 * \param[in] state Pointer to game state.
 */
void wave_morale_flag(tgestate_t *state)
{
  uint8_t       *pgame_counter;     /* was HL */
  uint8_t        morale;            /* was A */
  uint8_t       *pdisplayed_morale; /* was HL */
  const uint8_t *flag_bitmap;       /* was DE */

  assert(state != NULL);

  pgame_counter = &state->game_counter;
  (*pgame_counter)++;

  /* Wave the flag on every other turn */
  if (*pgame_counter & 1)
    return;

  morale = state->morale;
  pdisplayed_morale = &state->displayed_morale;
  if (morale != *pdisplayed_morale)
  {
    if (morale < *pdisplayed_morale)
    {
      /* Decreasing morale */
      (*pdisplayed_morale)--;
      pdisplayed_morale = get_next_scanline(state, state->moraleflag_screen_address);
    }
    else
    {
      /* Increasing morale */
      (*pdisplayed_morale)++;
      pdisplayed_morale = get_prev_scanline(state, state->moraleflag_screen_address);
    }
    state->moraleflag_screen_address = pdisplayed_morale;
  }

  flag_bitmap = flag_down;
  if (*pgame_counter & 2)
    flag_bitmap = flag_up;
  plot_bitmap(state, 3, 25, flag_bitmap, state->moraleflag_screen_address);
}

/* ----------------------------------------------------------------------- */

/**
 * $A071: Set the screen attributes of the morale flag.
 *
 * \param[in] state Pointer to game state.
 * \param[in] attrs Screen attributes.
 */
void set_morale_flag_screen_attributes(tgestate_t *state, attribute_t attrs)
{
  attribute_t *pattrs; /* was HL */
  int          iters;  /* was B */

  assert(state != NULL);
  assert(attrs >= attribute_BLUE_OVER_BLACK && attrs <= attribute_BRIGHT_WHITE_OVER_BLACK);

  pattrs = &state->speccy->attributes[morale_flag_attributes_offset];
  iters = 19; /* Height of flag. */
  do
  {
    pattrs[0] = attrs;
    pattrs[1] = attrs;
    pattrs[2] = attrs;
    pattrs += state->width;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A082: Get previous scanline.
 *
 * Given a screen address, returns the same position on the previous
 * scanline.
 *
 * \param[in] state Pointer to game state.
 * \param[in] addr  Original screen address.
 *
 * \return Updated screen address.
 */
uint8_t *get_prev_scanline(tgestate_t *state, uint8_t *addr)
{
  assert(state != NULL);
  assert(addr  != NULL);

  uint8_t *const screen = &state->speccy->screen[0];
  uint16_t       raddr = addr - screen;

  if ((raddr & 0x0700) != 0)
  {
    // NNN bits
    // step back one scanline
    raddr -= 256;
  }
  else
  {
    if ((raddr & 0x00FF) < 32)
      raddr -= 32;
    else
      // complicated
      raddr += 0x06E0;
  }

  return screen + raddr;
}

/* ----------------------------------------------------------------------- */

/**
 * $A095: Delay loop.
 *
 * Delay loop called only when the hero is indoors.
 */
void interior_delay_loop(tgestate_t *state)
{
  assert(state != NULL);

  state->speccy->sleep(state->speccy, 0xFFF);

//  volatile int BC = 0xFFF;
//  while (--BC)
//    ;
}

/* ----------------------------------------------------------------------- */

/* Offset. */
#define screenoffset_BELL_RINGER 0x118E

/**
 * $A09E: Ring the alarm bell.
 *
 * \param[in] state Pointer to game state.
 */
void ring_bell(tgestate_t *state)
{
  /**
   * $A147: Bell ringer bitmaps.
   */
  static const uint8_t bell_ringer_bitmap_off[] =
  {
    0xE7, 0xE7, 0x83, 0x83, 0x43, 0x41, 0x20, 0x10, 0x08, 0x04, 0x02, 0x02
  };
  static const uint8_t bell_ringer_bitmap_on[] =
  {
    0x3F, 0x3F, 0x27, 0x13, 0x13, 0x09, 0x08, 0x04, 0x04, 0x02, 0x02, 0x01
  };

  bellring_t *pbell; /* was HL */
  bellring_t  bell;  /* was A */

  assert(state != NULL);

  pbell = &state->bell;
  bell = *pbell;
  if (bell == bell_STOP)
    return; /* Not ringing */

  if (bell != bell_RING_PERPETUAL)
  {
    /* Decrement the ring counter. */
    *pbell = --bell;
    if (bell == 0)
    {
      /* Counter hit zero - stop ringing */
      *pbell = bell_STOP;
      return;
    }
  }

  /* Fetch visible state of bell. */
  bell = state->speccy->screen[screenoffset_BELL_RINGER];
  if (bell != 0x3F) /* Pixel value is 0x3F if on */
  {
    /* Plot ringer "on". */
    plot_ringer(state, bell_ringer_bitmap_on);
    play_speaker(state, sound_BELL_RINGER);
  }
  else
  {
    /* Plot ringer "off". */
    plot_ringer(state, bell_ringer_bitmap_off);
  }
}

/**
 * $A0C9: Plot ringer.
 *
 * \param[in] state Pointer to game state.
 * \param[in] src   Source bitmap.         (was HL)
 */
void plot_ringer(tgestate_t *state, const uint8_t *src)
{
  assert(state != NULL);
  assert(src   != NULL);

  plot_bitmap(state,
              1, 12, /* dimensions: 8 x 12 */
              src,
              &state->speccy->screen[screenoffset_BELL_RINGER]);
}

/* ----------------------------------------------------------------------- */

/**
 * $A0D2: Increase morale.
 *
 * \param[in] state Pointer to game state.
 * \param[in] delta Amount to increase morale by. (was B)
 */
void increase_morale(tgestate_t *state, uint8_t delta)
{
  uint8_t increased_morale;

  assert(state != NULL);
  assert(delta > 0);

  increased_morale = state->morale + delta;
  if (increased_morale >= morale_MAX)
    increased_morale = morale_MAX;

  state->morale = increased_morale;
}

/**
 * $A0E0: Decrease morale.
 *
 * \param[in] state Pointer to game state.
 * \param[in] delta Amount to decrease morale by. (was B)
 */
void decrease_morale(tgestate_t *state, uint8_t delta)
{
  uint8_t decreased_morale;

  assert(state != NULL);
  assert(delta > 0);

  decreased_morale = state->morale - delta;
  if (decreased_morale < morale_MIN)
    decreased_morale = morale_MIN;

  /* Conv: This jumps into the tail end of increase_morale in the original
   * code. */
  state->morale = decreased_morale;
}

/**
 * $A0E9: Increase morale by 10, score by 50.
 *
 * \param[in] state Pointer to game state.
 */
void increase_morale_by_10_score_by_50(tgestate_t *state)
{
  assert(state != NULL);

  increase_morale(state, 10);
  increase_score(state, 50);
}

/**
 * $A0F2: Increase morale by 5, score by 5.
 *
 * \param[in] state Pointer to game state.
 */
void increase_morale_by_5_score_by_5(tgestate_t *state)
{
  assert(state != NULL);

  increase_morale(state, 5);
  increase_score(state, 5);
}

/* ----------------------------------------------------------------------- */

/**
 * $A0F9: Increase score.
 *
 * \param[in] state Pointer to game state.
 * \param[in] delta Amount to increase score by. (was B)
 */
void increase_score(tgestate_t *state, uint8_t delta)
{
  char *pdigit; /* was HL */

  assert(state != NULL);
  assert(delta > 0);

  /* Increment the score digit-wise until delta is zero. */

  assert(state->score_digits[0] <= 9);
  assert(state->score_digits[1] <= 9);
  assert(state->score_digits[2] <= 9);
  assert(state->score_digits[3] <= 9);
  assert(state->score_digits[4] <= 9);

  pdigit = &state->score_digits[4];
  do
  {
    char *tmp; /* was HL */

    tmp = pdigit;
    for (;;)
    {
      (*pdigit)++;
      if (*pdigit < 10)
        break;

      *pdigit-- = 0;
    }
    pdigit = tmp;
  }
  while (--delta);

  plot_score(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A10B: Plot score.
 *
 * \param[in] state Pointer to game state.
 */
void plot_score(tgestate_t *state)
{
  char    *digits; /* was HL */
  uint8_t *screen; /* was DE */
  uint8_t  iters;  /* was B */

  assert(state != NULL);

  digits = &state->score_digits[0];
  screen = &state->speccy->screen[score_address];
  iters = NELEMS(state->score_digits);
  do
  {
    char digit = '0' + *digits; /* Conv: Pass as ASCII. */

    screen = plot_glyph(&digit, screen);
    digits++;
    screen++; /* Additionally to plot_glyph, so screen += 2 each iter. */
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A11D: Play the speaker.
 *
 * \param[in] state Pointer to game state.
 * \param[in] sound Number of iterations to play for (hi). Delay inbetween each iteration (lo). (was BC)
 */
void play_speaker(tgestate_t *state, sound_t sound)
{
  uint8_t iters;      /* was B */
  uint8_t delay;      /* was A */
  uint8_t speakerbit; /* was A */
  uint8_t subcount;   /* was C */

  assert(state != NULL);
  // assert(sound); somehow

  iters = sound >> 8;
  delay = sound & 0xFF;

  speakerbit = 16; /* Set speaker bit. */
  do
  {
    state->speccy->out(state->speccy, port_BORDER, speakerbit); /* Play. */

    /* Conv: Removed self-modified counter. */
    state->speccy->sleep(state->speccy, delay);
//    subcount = delay;
//    while (subcount--)
//      ;

    speakerbit ^= 16; /* Toggle speaker bit. */
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A15F: Set game screen attributes.
 *
 * \param[in] state Pointer to game state.
 * \param[in] attrs Screen attributes.     (was A)
 */
void set_game_window_attributes(tgestate_t *state, attribute_t attrs)
{
  attribute_t *attributes; /* was HL */
  uint8_t      rows;       /* was C */
  uint16_t     stride;     /* was DE */

  assert(state != NULL);
  assert(attrs >= attribute_BLUE_OVER_BLACK && attrs <= attribute_BRIGHT_WHITE_OVER_BLACK);

  attributes = &state->speccy->attributes[0x0047];
  rows   = state->rows; /* e.g. 23 */
  stride = state->width - state->columns; /* e.g. 9 */
  do
  {
    uint8_t iters;

    iters = state->columns; /* e.g. 23 */
    do
      *attributes++ = attrs;
    while (--iters);
    attributes += stride;
  }
  while (--rows);
}

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
    {  36, &event_breakfast_time       },
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
    set_hero_target_location(state, location_012C);
  set_day_or_night(state, 0xFF);
}

void event_another_day_dawns(tgestate_t *state)
{
  assert(state != NULL);

  queue_message_for_display(state, message_ANOTHER_DAY_DAWNS);
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
  assert(day_night == 0x00 || day_night == 0xFF);

  state->day_or_night = day_night; // night=0xFF, day=0x00
  attrs = choose_game_window_attributes(state);
  set_game_window_attributes(state, attrs);
}

void event_wake_up(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_TIME_TO_WAKE_UP);
  wake_up(state);
}

void event_go_to_roll_call(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_ROLL_CALL);
  go_to_roll_call(state);
}

void event_go_to_breakfast_time(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_BREAKFAST_TIME);
  set_location_0x0010(state);
}

void event_breakfast_time(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  breakfast_time(state);
}

void event_go_to_exercise_time(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_EXERCISE_TIME);

  /* Unlock the gates. */
  state->gates_and_doors[0] = 0x00;
  state->gates_and_doors[1] = 0x01;

  set_location_0x000E(state);
}

void event_exercise_time(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;
  set_location_0x048E(state);
}

void event_go_to_time_for_bed(tgestate_t *state)
{
  assert(state != NULL);

  state->bell = bell_RING_40_TIMES;

  /* Lock the gates. */
  state->gates_and_doors[0] = 0x80;
  state->gates_and_doors[1] = 0x81;

  queue_message_for_display(state, message_TIME_FOR_BED);
  go_to_time_for_bed(state);
}

void event_new_red_cross_parcel(tgestate_t *state)
{
  static const itemstruct_t red_cross_parcel_reset_data =
  {
    0x00, /* never used */
    room_20_REDCROSS,
    { 44, 44, 12 },
    0xF480
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

    itemstruct = item_to_itemstruct(state, *item);
    if ((itemstruct->room_and_flags & itemstruct_ROOM_MASK) == itemstruct_ROOM_NONE)
      goto found; /* Future: Remove goto. */

    item++;
  }
  while (--iters);

  return;

found:
  state->red_cross_parcel_current_contents = *item;
  memcpy(&state->item_structs[item_RED_CROSS_PARCEL].room_and_flags,
         &red_cross_parcel_reset_data.room_and_flags,
         6);
  queue_message_for_display(state, message_RED_CROSS_PARCEL);
}

void event_time_for_bed(tgestate_t *state)
{
  assert(state != NULL);

  set_guards_location(state, location_03A6);
}

void event_search_light(tgestate_t *state)
{
  assert(state != NULL);

  set_guards_location(state, location_0026);
}

/**
 * Common end of event_time_for_bed and event_search_light.
 * Sets the location of guards 12..15 (the guards from prisoners_and_guards).
 *
 * \param[in] state    Pointer to game state.
 * \param[in] location Location.              (was A lo + C hi)
 */
void set_guards_location(tgestate_t *state, location_t location)
{
  character_t index; /* was A' */
  uint8_t     iters; /* was B */

  assert(state    != NULL);
  assert(location != NULL);

  index = character_12_GUARD_12;
  iters = 4;
  do
  {
    set_character_location(state, index, location);
    index++;
    location++; /* Conv: Incrementing the full location_t not just the low byte. */
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A27F: List of non-player characters: six prisoners and four guards.
 *
 * Used by set_prisoners_and_guards_location and set_prisoners_and_guards_location_B.
 */
const character_t prisoners_and_guards[10] =
{
  character_12_GUARD_12,    /* Guard      */
  character_13_GUARD_13,    /* Guard      */
  character_20_PRISONER_1,  /* Prisoner   */
  character_21_PRISONER_2,  /* Prisoner   */
  character_22_PRISONER_3,  /* Prisoner   */
  character_14_GUARD_14,    /* Guard      */
  character_15_GUARD_15,    /* Guard      */
  character_23_PRISONER_4,  /* Prisoner   */
  character_24_PRISONER_5,  /* Prisoner   */
  character_25_PRISONER_6   /* Prisoner   */
};

/* ----------------------------------------------------------------------- */

/**
 * $A289: Wake up.
 *
 * \param[in] state Pointer to game state.
 */
void wake_up(tgestate_t *state)
{
  characterstruct_t *charstr;  /* was HL */
  uint8_t            iters;    /* was B */
  uint8_t *const    *bedpp;    // was ?
  uint8_t            loc_low;  /* was A' */
  uint8_t            loc_high; /* was C */

  assert(state != NULL);

  if (state->hero_in_bed)
  {
    /* Hero gets out of bed. */
    state->vischars[0].mi.pos.x = 46;
    state->vischars[0].mi.pos.y = 46;
  }

  state->hero_in_bed = 0;
  set_hero_target_location(state, location_002A);

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

  loc_low  = location_0005 & 0xFF; // incremented by set_prisoners_and_guards_location_B (but it's unclear why)
  loc_high = location_0005 >> 8;
  set_prisoners_and_guards_location_B(state, &loc_low, loc_high);

  /* Update all the bed objects to be empty. */
  // FIXME: This writes to a possibly shared structure, ought to be moved into the state somehow.
  bedpp = &beds[0];
  iters = beds_LENGTH;
  do
    **bedpp++ = interiorobject_EMPTY_BED;
  while (--iters);

  /* Update the hero's bed object to be empty. */
  roomdef_2_hut2_left[roomdef_2_BED] = interiorobject_EMPTY_BED;
  if (state->room_index == room_0_OUTDOORS || state->room_index >= room_6)
    return;

  setup_room(state);
  plot_interior_tiles(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A2E2: Breakfast time.
 *
 * \param[in] state Pointer to game state.
 */
void breakfast_time(tgestate_t *state)
{
  characterstruct_t *charstr;  /* was HL */
  uint8_t            iters;    /* was B */
  uint8_t            loc_low;  /* was A' */
  uint8_t            loc_high; /* was C */

  assert(state != NULL);

  if (state->hero_in_breakfast)
  {
    state->vischars[0].mi.pos.x = 52;
    state->vischars[0].mi.pos.y = 62;
  }

  state->hero_in_breakfast = 0;
  set_hero_target_location(state, location_0390);

  charstr = &state->character_structs[character_20_PRISONER_1];
  iters = 3;
  do
  {
    charstr->room = room_25_BREAKFAST;
    charstr++;
  }
  while (--iters);
  iters = 3;
  do
  {
    charstr->room = room_23_BREAKFAST;
    charstr++;
  }
  while (--iters);

  loc_low  = location_0390 & 0xFF;
  loc_high = location_0390 >> 8;
  set_prisoners_and_guards_location_B(state, &loc_low, loc_high);

  /* Update all the benches to be empty. */
  // FIXME: Writing to shared state.
  roomdef_23_breakfast[roomdef_23_BENCH_A] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_B] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_C] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_D] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_E] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_F] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_EMPTY_BENCH;

  if (state->room_index == room_0_OUTDOORS ||
      state->room_index >= room_29_SECOND_TUNNEL_START)
    return;

  setup_room(state);
  plot_interior_tiles(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A33F: Set the hero's target location.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] location Location.              (was BC)
 */
void set_hero_target_location(tgestate_t *state, location_t location)
{
  vischar_t *vischar; /* was HL */

  assert(state != NULL);
  // assert(location);

  if (state->morale_1)
    return;

  vischar = &state->vischars[0];

  vischar->character &= ~vischar_FLAGS_BIT6;
  vischar->target = location;
  sub_A3BB(state, vischar);
}

/* ----------------------------------------------------------------------- */

/**
 * $A351: Go to time for bed.
 *
 * \param[in] state Pointer to game state.
 */
void go_to_time_for_bed(tgestate_t *state)
{
  uint8_t loc_low;  /* was A' */
  uint8_t loc_high; /* was C */

  assert(state != NULL);

  set_hero_target_location(state, location_0285);
  loc_low  = location_0285 & 0xFF;
  loc_high = location_0285 >> 8;
  set_prisoners_and_guards_location_B(state, &loc_low, loc_high);
}

/* ----------------------------------------------------------------------- */

/**
 * $A35F: Set the location of the prisoners_and_guards.
 *
 * Set a different location for each character.
 *
 * Called by go_to_roll_call.
 *
 * This increments loc_low and returns it, but it's unclear why it does that.
 * None of the calls seem to make use of it.
 *
 * \param[in]     state     Pointer to game state.
 * \param[in,out] p_loc_low Pointer to location low byte. (was A')
 * \param[in]     loc_high  Location high byte.           (was C)
 */
void set_prisoners_and_guards_location(tgestate_t *state,
                                       uint8_t    *p_loc_low,
                                       uint8_t     loc_high)
{
  uint8_t            loc_low; /* new var */
  const character_t *pchars;  /* was HL */
  uint8_t            iters;   /* was B */

  assert(state     != NULL);
  assert(p_loc_low != NULL);
  // assert(loc_high);

  loc_low = *p_loc_low; // Conv: Keep a local copy of counter.

  pchars = &prisoners_and_guards[0];
  iters = NELEMS(prisoners_and_guards);
  do
  {
    set_character_location(state, *pchars, loc_low | (loc_high << 8));
    loc_low++;
    pchars++;
  }
  while (--iters);

  *p_loc_low = loc_low;
}

/* ----------------------------------------------------------------------- */

/**
 * $A373: Set the location of the prisoners_and_guards.
 *
 * Set the same location for each half of the group.
 *
 * Called by the set_location routines.
 *
 * \param[in]     state     Pointer to game state.
 * \param[in,out] p_loc_low Pointer to location low byte. (was A')
 * \param[in]     loc_high  Location high byte.           (was C)
 */
void set_prisoners_and_guards_location_B(tgestate_t *state,
                                         uint8_t    *p_loc_low,
                                         uint8_t     loc_high)
{
  uint8_t            loc_low; /* new var */
  const character_t *pchars;  /* was HL */
  uint8_t            iters;   /* was B */

  assert(state     != NULL);
  assert(p_loc_low != NULL);
  // assert(loc_high);

  loc_low = *p_loc_low; // Conv: Keep a local copy of counter.

  pchars = &prisoners_and_guards[0];
  iters = NELEMS(prisoners_and_guards);
  do
  {
    set_character_location(state, *pchars, loc_low | (loc_high << 8));

    /* When this is 6, the character being processed is
     * character_22_PRISONER_3 and the next is character_14_GUARD_14, the
     * start of the second half of the list. */
    if (iters == 6)
      loc_low++;

    pchars++;
  }
  while (--iters);

  *p_loc_low = loc_low;
}

/* ----------------------------------------------------------------------- */

/**
 * $A38C: Set location of a character.
 *
 * Finds a charstruct, or a vischar, and stores a location in its target.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] index    Character index.       (was A)
 * \param[in] location Location.              (was A' lo + C hi)
 */
void set_character_location(tgestate_t  *state,
                            character_t  index,
                            location_t   location)
{
  characterstruct_t *charstr; /* was HL */
  vischar_t         *vischar; /* was HL */
  uint8_t            iters;   /* was B */

  assert(state != NULL);
  assert(index >= 0 && index < character__LIMIT);
  // assert(location);

  charstr = get_character_struct(state, index);
  if ((charstr->character & characterstruct_FLAG_DISABLED) != 0)
  {
    index = charstr->character & characterstruct_BYTE0_MASK;
    iters = vischars_LENGTH - 1;
    vischar = &state->vischars[1]; /* iterate over non-player characters */
    do
    {
      if (index == charstr->character)
        goto store_to_vischar; // an already visible character?
      vischar++;
    }
    while (--iters);

    goto exit;
  }

  // store_to_charstruct:
  store_location(location, &charstr->target);

exit:
  return;

store_to_vischar:
  vischar->flags &= ~vischar_FLAGS_BIT6;
  store_location(location, &vischar->target);

  sub_A3BB(state, vischar); // 2nd arg a guess for now -- check // fallthrough
}

/**
 * $A3BB: sub_A3BB.
 *
 * Called by set_character_location, set_hero_target_location.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was HL) (e.g. $8003 in original)
 */
void sub_A3BB(tgestate_t *state, vischar_t *vischar)
{
  uint8_t    A;   /* was A */
  tinypos_t *pos; /* was DE */

  assert(state   != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  state->byte_A13E = 0;

  // sampled HL = $8003 $8043 $8023 $8063 $8083 $80A3

  A = sub_C651(state, &vischar->target);

  pos = &vischar->p04;
  pos->x = vischar->target & 0xFF;
  pos->y = vischar->target >> 8;

  if (A == 255)
  {
    state->IY = vischar; // TODO: Work out all the other places which assign IY.
    sub_CB23(state, vischar, &vischar->target);
  }
  else if (A == 128)
  {
    vischar->flags |= vischar_FLAGS_BIT6;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $A3ED: Store a location_t at the specified address.
 *
 * Used by set_character_location only.
 *
 * \param[in]  location  Location. (was A' lo + C hi)
 * \param[out] plocation Pointer to vischar->target, or characterstruct->target. (was HL)
 */
void store_location(location_t location, location_t *plocation)
{
  // assert(location);
  assert(plocation != NULL);

  *plocation = location;
}

/* ----------------------------------------------------------------------- */

/**
 * $A3F3: byte_A13E is non-zero.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] charstr Pointer to character struct. (was HL)
 */
void byte_A13E_is_nonzero(tgestate_t        *state,
                          characterstruct_t *charstr)
{
  assert(state   != NULL);
  assert(charstr != NULL);

  sub_A404(state, charstr, state->character_index);
}

/**
 * $A3F8: byte_A13E is zero.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] charstr Pointer to character struct.  (was HL)
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void byte_A13E_is_zero(tgestate_t        *state,
                       characterstruct_t *charstr,
                       vischar_t         *vischar)
{
  character_t character;

  assert(state   != NULL);
  assert(charstr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  character = vischar->character;
  if (character == character_0_COMMANDANT)
    set_hero_target_location(state, location_002C);
  else
    sub_A404(state, charstr, character);
}

/**
 * $A404: Common end of above two routines.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] charstr   Pointer to character struct. (was HL)
 * \param[in] character Character index.             (was A)
 */
void sub_A404(tgestate_t        *state,
              characterstruct_t *charstr,
              character_t        character)
{
  assert(state   != NULL);
  assert(charstr != NULL);
  assert(character >= 0 && character < character__LIMIT);

  charstr->room = room_NONE;

  if (character >= character_20_PRISONER_1)
  {
    character -= 13; // character_20_PRISONER_1 -> character_7_GUARD_7 etc.
  }
  else
  {
    character_t old_character;

    old_character = character;
    character = character_13_GUARD_13;
    if (old_character & (1 << 0))
    {
      charstr->room = room_1_HUT1RIGHT;
      character |= characterstruct_BYTE0_BIT7;
    }
  }

  charstr->character = character;
}

/* ----------------------------------------------------------------------- */

/**
 * $A420: Character sits.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] character Character index.       (was A)
 * \param[in] location  (unknown)
 */
void character_sits(tgestate_t  *state,
                    character_t  character,
                    location_t  *location)
{
  uint8_t  index; /* was A */
  uint8_t *bench; /* was HL */
  room_t   room;  /* was C */

  assert(state    != NULL);
  assert(character >= 0 && character < character__LIMIT);
  assert(location != NULL);

  index = character - 18;
  /* First three characters. */
  bench = &roomdef_25_breakfast[roomdef_25_BENCH_D];
  if (index >= 3) /* character_21_PRISONER_2 */
  {
    /* Second three characters. */
    bench = &roomdef_23_breakfast[roomdef_23_BENCH_A];
    index -= 3;
  }

  /* Poke object. */
  bench[index * 3] = interiorobject_PRISONER_SAT_DOWN_MID_TABLE;

  room = room_25_BREAKFAST;
  if (character >= character_21_PRISONER_2)
    room = room_23_BREAKFAST;

  character_sit_sleep_common(state, room, location);
}

/**
 * $A444: Character sleeps.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] character Character index.       (was A)
 * \param[in] location  (unknown)
 */
void character_sleeps(tgestate_t  *state,
                      character_t  character,
                      location_t  *location)
{
  room_t room; /* was C */

  assert(state    != NULL);
  assert(character >= 0 && character < character__LIMIT);
  assert(location != NULL);

  /* Poke object. */
  *beds[character - 7] = interiorobject_OCCUPIED_BED;

  if (character < character_10_GUARD_10)
    room = room_3_HUT2RIGHT;
  else
    room = room_5_HUT3RIGHT;

  character_sit_sleep_common(state, room, location);
}

/**
 * $A462: Common end of character sits/sleeps.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] room     Room index.                 (was C)
 * \param[in] location Likely a target location_t. (was DE/HL)
 */
void character_sit_sleep_common(tgestate_t *state,
                                room_t      room,
                                location_t *location)
{
  /* This receives a pointer to a location structure which is within either a
   * characterstruct or a vischar. */

  assert(state    != NULL);
  assert(room >= 0 && room < room__LIMIT);
  assert(location != NULL);

  // sampled HL =
  // breakfast $8022 + + $76B8 $76BF
  // night     $76A3 $76B8 $76C6 $76B1 $76BF $76AA
  // (not always in the same order)
  //
  // $8022 -> vischar[1]->target
  // others -> character_structs->target

  *location |= 255; /* Conv: Was a byte write. */

  if (state->room_index != room)
  {
    /* Sit/sleep in a room presently not visible. */

    characterstruct_t *cs;

    /* Retrieve the parent structure pointer. */
    cs = structof(location, characterstruct_t, target);
    cs->room = room_NONE;
  }
  else
  {
    /* Room is visible - force a refresh. */

    vischar_t *vc;

    /* This is only ever hit when location is in vischar @ $8020. */

    /* Retrieve the parent structure pointer. */
    vc = structof(location, vischar_t, target);
    vc->room = room_NONE;

    select_room_and_plot(state);
  }
}

/**
 * $A479: Select room and plot.
 *
 * \param[in] state Pointer to game state.
 */
void select_room_and_plot(tgestate_t *state)
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

  roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_PRISONER_SAT_DOWN_END_TABLE;
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

  roomdef_2_hut2_left[roomdef_2_BED] = interiorobject_OCCUPIED_BED;
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
  *pflag = 0xFF;

  /* Reset only the bottom byte of target location. */
  state->vischars[0].target &= 0xFF00;

  /* Set hero position (x,y) to zero. */
  state->vischars[0].mi.pos.x = 0;
  state->vischars[0].mi.pos.y = 0;

  reset_position(state, &state->vischars[0]);

  select_room_and_plot(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A4A9: Set location 0x000E.
 *
 * \param[in] state Pointer to game state.
 */
void set_location_0x000E(tgestate_t *state)
{
  uint8_t loc_low;  /* was A' */
  uint8_t loc_high; /* was C */

  assert(state != NULL);

  set_hero_target_location(state, location_000E);
  loc_low  = location_000E & 0xFF;
  loc_high = location_000E >> 8;
  set_prisoners_and_guards_location_B(state, &loc_low, loc_high);
}

/**
 * $A4B7: Set location 0x048E.
 *
 * \param[in] state Pointer to game state.
 */
void set_location_0x048E(tgestate_t *state)
{
  uint8_t loc_low;  /* was A' */
  uint8_t loc_high; /* was C */

  assert(state != NULL);

  set_hero_target_location(state, location_048E);
  loc_low  = location_0010 & 0xFF;
  loc_high = location_0010 >> 8;
  set_prisoners_and_guards_location_B(state, &loc_low, loc_high);
}

/**
 * $A4C5: Set location 0x0010.
 *
 * \param[in] state Pointer to game state.
 */
void set_location_0x0010(tgestate_t *state)
{
  uint8_t loc_low;  /* was A' */
  uint8_t loc_high; /* was C */

  assert(state != NULL);

  set_hero_target_location(state, location_0010);
  loc_low  = location_0010 & 0xFF;
  loc_high = location_0010 >> 8;
  set_prisoners_and_guards_location_B(state, &loc_low, loc_high);
}

/* ----------------------------------------------------------------------- */

/**
 * $A4D3: byte_A13E is non-zero (another one).
 *
 * Very similar to the routine at $A3F3.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] charstr Pointer to character struct.  (was HL)
 */
void byte_A13E_is_nonzero_anotherone(tgestate_t        *state,
                                     vischar_t         *vischar,
                                     characterstruct_t *charstr)
{
  assert(state   != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(charstr != NULL);

  byte_A13E_anotherone_common(state, charstr, state->character_index);
}

/**
 * $A4D8: byte_A13E is zero (another one).
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] charstr Pointer to character struct.  (was HL)
 */
void byte_A13E_is_zero_anotherone(tgestate_t        *state,
                                  vischar_t         *vischar,
                                  characterstruct_t *charstr)
{
  character_t character;

  assert(state   != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(charstr != NULL);

  character = vischar->character;
  if (character == character_0_COMMANDANT)
    set_hero_target_location(state, location_002B);
  else
    byte_A13E_anotherone_common(state, charstr, character);
}

/**
 * $A4E4: Common end of above two routines.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] charstr   Pointer to character struct. (was HL)
 * \param[in] character Character index.             (was A)
 */
void byte_A13E_anotherone_common(tgestate_t        *state,
                                 characterstruct_t *charstr,
                                 character_t        character)
{
  assert(state   != NULL);
  assert(charstr != NULL);
  assert(character >= 0 && character < character__LIMIT);

  charstr->room = room_NONE;

  if (character >= character_20_PRISONER_1)
  {
    character -= 2;
  }
  else
  {
    character_t old_character;

    old_character = character;
    character = character_24_PRISONER_5;
    if (old_character & (1 << 0))
      character++; // gets hit during breakfast
  }

  charstr->character = character;
}

/* ----------------------------------------------------------------------- */

/**
 * $A4FD: Go to roll call.
 *
 * \param[in] state Pointer to game state.
 */
void go_to_roll_call(tgestate_t *state)
{
  uint8_t loc_low;  /* was A' */
  uint8_t loc_high; /* was C */

  assert(state != NULL);

  loc_low  = location_001A & 0xFF;
  loc_high = location_001A >> 8;
  set_prisoners_and_guards_location(state, &loc_low, loc_high);
  set_hero_target_location(state, location_002D);
}

/* ----------------------------------------------------------------------- */

/**
 * $A50B: Screen reset.
 *
 * \param[in] state Pointer to game state.
 */
void screen_reset(tgestate_t *state)
{
  assert(state != NULL);

  wipe_visible_tiles(state);
  plot_interior_tiles(state);
  zoombox(state);
  plot_game_window(state);
  set_game_window_attributes(state, attribute_WHITE_OVER_BLACK);
}

/* ----------------------------------------------------------------------- */

/**
 * $A51C: Hero has escaped.
 *
 * Print 'well done' message then test to see if the correct objects were
 * used in the escape attempt.
 *
 * \param[in] state Pointer to game state.
 */
void escaped(tgestate_t *state)
{
  /**
   * $A5CE: Escape messages.
   */
  static const screenlocstring_t messages[11] =
  {
    { 0x006E,  9, "WELL DONE"             },
    { 0x00AA, 16, "YOU HAVE ESCAPED"      },
    { 0x00CC, 13, "FROM THE CAMP"         },
    { 0x0809, 18, "AND WILL CROSS THE"    },
    { 0x0829, 19, "BORDER SUCCESSFULLY"   },
    { 0x0809, 19, "BUT WERE RECAPTURED"   },
    { 0x082A, 17, "AND SHOT AS A SPY"     },
    { 0x0829, 18, "TOTALLY UNPREPARED"    },
    { 0x082C, 12, "TOTALLY LOST"          },
    { 0x0828, 21, "DUE TO LACK OF PAPERS" },
    { 0x100D, 13, "PRESS ANY KEY"         },
  };

  const screenlocstring_t *message;   /* was HL */
  escapeitem_t             itemflags; /* was C */
  const item_t            *pitem;     /* was HL */
  uint8_t                  keys;      /* was A */

  assert(state != NULL);

  screen_reset(state);

  /* Print standard prefix messages. */
  message = &messages[0];
  message = screenlocstring_plot(state, message); /* WELL DONE */
  message = screenlocstring_plot(state, message); /* YOU HAVE ESCAPED */
  (void) screenlocstring_plot(state, message);    /* FROM THE CAMP */

  /* Form escape items bitfield. */
  itemflags = 0;
  pitem = &state->items_held[0];
  itemflags = join_item_to_escapeitem(pitem++, itemflags);
  itemflags = join_item_to_escapeitem(pitem,   itemflags);

  /* Print item-tailored messages. */
  if (itemflags == (escapeitem_COMPASS | escapeitem_PURSE))
  {
    message = &messages[3];
    message = screenlocstring_plot(state, message); /* AND WILL CROSS THE */
    (void) screenlocstring_plot(state, message);    /* BORDER SUCCESSFULLY */

    itemflags = 0xFF; /* success - reset game */
  }
  else if (itemflags != (escapeitem_COMPASS | escapeitem_PAPERS))
  {
    message = &messages[5]; /* BUT WERE RECAPTURED */
    message = screenlocstring_plot(state, message);

    /* No uniform => AND SHOT AS A SPY. */
    if (itemflags < escapeitem_UNIFORM)
    {
      /* No objects => TOTALLY UNPREPARED. */
      message = &messages[7];
      if (itemflags)
      {
        /* No compass => TOTALLY LOST. */
        message = &messages[8];
        if (itemflags & escapeitem_COMPASS)
        {
          /* No papers => DUE TO LACK OF PAPERS. */
          message = &messages[9];
        }
      }
    }
    (void) screenlocstring_plot(state, message);
  }

  message = &messages[10]; /* PRESS ANY KEY */
  (void) screenlocstring_plot(state, message);

  /* Wait for a keypress. */
  do
    keys = keyscan_all(state);
  while (keys != 0); /* Down press */
  do
    keys = keyscan_all(state);
  while (keys == 0); /* Up press */

  /* Reset the game, or send the hero to solitary. */
  if (itemflags == 0xFF || itemflags >= escapeitem_UNIFORM)
    reset_game(state); // exit via
  else
    solitary(state); // exit via
}

/* ----------------------------------------------------------------------- */

/**
 * $A58C: Key scan of all ports.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Key code. (was A)
 */
uint8_t keyscan_all(tgestate_t *state)
{
  uint16_t port;  /* was BC */
  uint8_t  keys;  /* was A */
  int      carry;

  assert(state != NULL);

  /* Scan all keyboard ports (0xFEFE .. 0x7FFE). */
  port = port_KEYBOARD_SHIFTZXCV;
  do
  {
    keys = state->speccy->in(state->speccy, port);

    /* Invert bits and mask off key bits. */
    keys = ~keys & 0x1F;
    if (keys)
      return keys; /* Key(s) pressed */

    /* Rotate the top byte of the port number to get the next port. */
    // Conv: Was RLC B
    carry = (port >> 15) != 0;
    port  = ((port << 1) & 0xFF00) | (carry << 8) | (port & 0x00FF);
  }
  while (carry);

  return 0; /* No keys pressed */
}

/* ----------------------------------------------------------------------- */

/**
 * $A59C: Call item_to_escapeitem then merge result with a previous escapeitem.
 *
 * \param[in] pitem    Pointer to item.              (was HL)
 * \param[in] previous Previous escapeitem bitfield. (was C)
 *
 * \return Merged escapeitem bitfield. (was C)
 */
escapeitem_t join_item_to_escapeitem(const item_t *pitem, escapeitem_t previous)
{
  assert(pitem != NULL);
  // assert(previous);

  return item_to_escapeitem(*pitem) | previous;
}

/**
 * $A5A3: Return a bitmask indicating the presence of items required for escape.
 *
 * \param[in] item Item. (was A)
 *
 * \return COMPASS, PAPERS, PURSE, UNIFORM => 1, 2, 4, 8. (was A)
 */
escapeitem_t item_to_escapeitem(item_t item)
{
  switch (item)
  {
  case item_COMPASS:
    return escapeitem_COMPASS;
  case item_PAPERS:
    return escapeitem_PAPERS;
  case item_PURSE:
    return escapeitem_PURSE;
  case item_UNIFORM:
    return escapeitem_UNIFORM;
  default:
    return 0;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $A5BF: Plot a screenlocstring.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] slstring Pointer to screenlocstring. (was HL)
 *
 * \return Pointer to next screenlocstring. (was HL)
 */
const screenlocstring_t *screenlocstring_plot(tgestate_t              *state,
                                              const screenlocstring_t *slstring)
{
  uint8_t    *screen; /* was DE */
  int         length; /* was B */
  const char *string; /* was HL */

  assert(state    != NULL);
  assert(slstring != NULL);

  screen = &state->speccy->screen[slstring->screenloc];
  length = slstring->length;
  string = slstring->string;
  do
    screen = plot_glyph(string++, screen);
  while (--length);

  return slstring + 1;
}

/* ----------------------------------------------------------------------- */

/**
 * $A7C9: Get supertiles.
 *
 * Uses state->map_position to copy supertile indices from map into
 * the buffer at state->map_buf.
 *
 * \param[in] state Pointer to game state.
 */
void get_supertiles(tgestate_t *state)
{
  uint8_t                 v;     /* was A */
  const supertileindex_t *tiles; /* was HL */
  uint8_t                 iters; /* was A */
  uint8_t                *buf;   /* was DE */

  assert(state != NULL);

  /* Get vertical offset. */
  v = state->map_position[1] & ~3; /* = 0, 4, 8, 12, ... */

  /* Multiply A by 13.5. (v is a multiple of 4, so this goes 0, 54, 108, 162, ...) */
  tiles = &map[0] - MAPX + (v + (v >> 1)) * 9; // Subtract MAPX so it skips the first row.

  /* Add horizontal offset. */
  tiles += state->map_position[0] >> 2;

  /* Populate map_buf with 7x5 array of supertile refs. */
  iters = state->st_rows;
  buf = &state->map_buf[0];
  do
  {
    memcpy(buf, tiles, state->st_columns);
    buf   += state->st_columns;
    tiles += MAPX;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A80A: supertile_plot_horizontal_1
 *
 * \param[in] state Pointer to game state.
 */
void supertile_plot_horizontal_1(tgestate_t *state)
{
  tileindex_t      *vistiles; /* was DE */
  supertileindex_t *maptiles; /* was HLdash */
  uint8_t           pos;      /* was A */
  uint8_t          *window;   /* was DEdash */

  assert(state != NULL);

  vistiles = &state->tile_buf[24 * 16];       // $F278 = visible tiles array + 24 * 16
  maptiles = &state->map_buf[28];             // $FF74
  pos      = state->map_position[1];          // map_position hi
  window   = &state->window_buf[24 * 16 * 8]; // $FE90

  supertile_plot_horizontal_common(state, vistiles, maptiles, pos, window);
}

/**
 * $A819: supertile_plot_horizontal_2
 *
 * \param[in] state Pointer to game state.
 */
void supertile_plot_horizontal_2(tgestate_t *state)
{
  tileindex_t      *vistiles; /* was DE */
  supertileindex_t *maptiles; /* was HLdash */
  uint8_t           pos;      /* was A */
  uint8_t          *window;   /* was DEdash */

  assert(state != NULL);

  vistiles = &state->tile_buf[0];    // $F0F8 = visible tiles array + 0
  maptiles = &state->map_buf[8];     // $FF58
  pos      = state->map_position[1]; // map_position hi
  window   = &state->window_buf[0];  // $F290

  supertile_plot_horizontal_common(state, vistiles, maptiles, pos, window);
}

/**
 * $A826: Plotting supertiles.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] vistiles Pointer to visible tiles array.         (was DE)
 * \param[in] maptiles Pointer to 7x5 supertile refs.          (was HL')
 * \param[in] pos      Map position high byte.                 (was A)
 * \param[in] window   Pointer to screen buffer start address. (was DE')
 */
void supertile_plot_horizontal_common(tgestate_t       *state,
                                      tileindex_t      *vistiles,
                                      supertileindex_t *maptiles,
                                      uint8_t           pos,
                                      uint8_t          *window)
{
  // Conv: self_A86A removed. Can be replaced with pos_copy.

  uint8_t            offset; /* was Cdash */
  const tileindex_t *tiles;  /* was HL */
  uint8_t            A;      /* was A */
  uint8_t            iters;  /* was B */
  uint8_t            iters2; /* was B' */
  uint8_t            pos_1;  // new

  assert(state    != NULL);
  assert(vistiles != NULL);
  assert(maptiles != NULL);
  // assert(pos);
  assert(window   != NULL);

  offset = (pos & 3) * 4;
  pos_1 = (state->map_position[0] & 3) + offset;

  /* Initial edge. */

  assert(*maptiles < supertileindex__LIMIT);
  tiles = &supertiles[*maptiles].tiles[0] + pos_1;
  A = tiles - &supertiles[0].tiles[0]; // Conv: Original code could simply use L.

  // 0,1,2,3 => 4,3,2,1
  A = -A & 3;
  if (A == 0)
    A = 4;

  iters = A; /* 1..4 iterations */
  do
  {
    tileindex_t t; /* was A */

    // Conv: Fused accesses and increments.
    t = *vistiles++ = *tiles++; // A = tile index
    plot_tile(state, t, maptiles, window);
  }
  while (--iters);

  maptiles++;

  /* Middle loop. */

  iters2 = 5;
  do
  {
    assert(*maptiles < supertileindex__LIMIT);
    tiles = &supertiles[*maptiles].tiles[0] + offset; // self modified by $A82A

    iters = 4;
    do
    {
      tileindex_t t; /* was A */

      t = *vistiles++ = *tiles++; // A = tile index
      plot_tile(state, t, maptiles, window);
    }
    while (--iters);

    maptiles++;
  }
  while (--iters2);

  /* Trailing edge. */

  //A = pos_copy; // an apparently unused assignment

  assert(*maptiles < supertileindex__LIMIT);
  tiles = &supertiles[*maptiles].tiles[0] + offset; // read of self modified instruction
  // Conv: A was A'.
  A = state->map_position[0] & 3; // map_position lo (repeats earlier work)
  if (A == 0)
    return;

  iters = A;
  do
  {
    tileindex_t t; /* was Adash */

    t = *vistiles++ = *tiles++; // Adash = tile index
    plot_tile(state, t, maptiles, window);
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A8A2: Plot all tiles.
 *
 * Called by pick_up_item and reset_outdoors.
 *
 * \param[in] state Pointer to game state.
 */
void supertile_plot_all(tgestate_t *state)
{
  tileindex_t      *vistiles; /* was DE */
  supertileindex_t *maptiles; /* was HL' */
  uint8_t          *window;   /* was DE' */
  uint8_t           pos;      /* was A */
  uint8_t           iters;    /* was B' */

  assert(state != NULL);

  vistiles = &state->tile_buf[0];    /* visible tiles array */
  maptiles = &state->map_buf[0];     /* 7x5 supertile refs */
  window   = &state->window_buf[0];  /* screen buffer start address */
  pos      = state->map_position[0]; /* map_position lo */

  iters = state->columns; /* Conv: was 24 */
  do
  {
    uint8_t newpos; /* was C' */

    supertile_plot_vertical_common(state, vistiles, maptiles, pos, window);
    vistiles++;

    newpos = ++pos;
    if ((pos & 3) == 0)
      maptiles++;
    pos = newpos;
    window++;
  }
  while (--iters);
}

/**
 * $A8CF: supertile_plot_vertical_1.
 *
 * \param[in] state Pointer to game state.
 */
void supertile_plot_vertical_1(tgestate_t *state)
{
  tileindex_t      *vistiles; /* was DE */
  supertileindex_t *maptiles; /* was HLdash */
  uint8_t          *window;   /* was DEdash */
  uint8_t           pos;      /* was A */

  assert(state != NULL);

  vistiles = &state->tile_buf[23];   /* visible tiles array */
  maptiles = &state->map_buf[6];     /* 7x5 supertile refs */
  window   = &state->window_buf[23]; /* screen buffer start address */
  pos      = state->map_position[0]; /* map_position lo */

  pos &= 3;
  if (pos == 0)
    maptiles--;
  pos = state->map_position[0] - 1; /* map_position lo */

  supertile_plot_vertical_common(state, vistiles, maptiles, pos, window);
}

/**
 * $A8E7: supertile_plot_vertical_2.
 *
 * \param[in] state Pointer to game state.
 */
void supertile_plot_vertical_2(tgestate_t *state)
{
  tileindex_t      *vistiles; /* was DE */
  supertileindex_t *maptiles; /* was HLdash */
  uint8_t          *window;   /* was DEdash */
  uint8_t           pos;      /* was A */

  assert(state != NULL);

  vistiles = &state->tile_buf[0];    /* visible tiles array */
  maptiles = &state->map_buf[0];     /* 7x5 supertile refs */
  window   = &state->window_buf[0];  /* screen buffer start address */
  pos      = state->map_position[0]; /* map_position lo */

  supertile_plot_vertical_common(state, vistiles, maptiles, pos, window);
}

/**
 * $A8F4: Plotting supertiles (second variant).
 *
 * \param[in] state    Pointer to game state.
 * \param[in] vistiles Pointer to visible tiles array.         (was DE)
 * \param[in] maptiles Pointer to 7x5 supertile refs.          (was HL')
 * \param[in] pos      Map position low byte.                  (was A)
 * \param[in] window   Pointer to screen buffer start address. (was DE')
 */
void supertile_plot_vertical_common(tgestate_t       *state,
                                    tileindex_t      *vistiles,
                                    supertileindex_t *maptiles,
                                    uint8_t           pos,
                                    uint8_t          *window)
{
  // Conv: self_A94D removed.

  uint8_t            offset; /* was Cdash */
  uint8_t            pos_1;  // new
  const tileindex_t *tiles;  /* was HL */
  uint8_t            A;

  uint8_t            iters2; /* was B' */
  uint8_t            iters;  /* was A */

  assert(state    != NULL);
  assert(vistiles != NULL);
  assert(maptiles != NULL);
  // assert(pos);
  assert(window   != NULL);

  offset = pos & 3; // self modify (local)
  pos_1 = (state->map_position[1] & 3) * 4 + offset;

  /* Initial edge. */

  assert(*maptiles < supertileindex__LIMIT);
  tiles = &supertiles[*maptiles].tiles[0] + pos_1;

  // 0,1,2,3 => 4,3,2,1
  A = -((pos_1 >> 2) & 3) & 3; // iterations
  if (A == 0)
    A = 4; // 1..4

  iters = A;
  do
  {
    tileindex_t t; /* was A */

    t = *vistiles = *tiles; // A = tile index
    plot_tile_then_advance(state, t, tiles, window);
    vistiles += 4; // stride
    tiles += state->columns;
  }
  while (--iters);

  maptiles += 7;

  /* Middle loop. */

  iters2 = 3;
  do
  {
    assert(*maptiles < supertileindex__LIMIT);
    tiles = &supertiles[*maptiles].tiles[0] + offset; // self modified by $A8F6

    iters = 4;
    do
    {
      tileindex_t t; /* was A */

      t = *vistiles = *tiles; // A = tile index
      plot_tile_then_advance(state, t, tiles, window);
      tiles += state->columns;
      vistiles += 4; // stride
    }
    while (--iters);

    maptiles += 7;
  }
  while (--iters2);

  /* Trailing edge. */

  assert(*maptiles < supertileindex__LIMIT);
  tiles = &supertiles[*maptiles].tiles[0] + offset; // read self modified instruction
  iters = (state->map_position[1] & 3) + 1;
  do
  {
    tileindex_t t; /* was A */

    t = *vistiles = *tiles;
    plot_tile_then_advance(state, t, tiles, window);
    vistiles += 4; // stride
    tiles += state->columns;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A9A0: Call plot_tile then advance 'scr' by a row.
 *
 * \param[in] state      Pointer to game state.
 * \param[in] tile_index Tile index. (was A)
 * \param[in] psupertileindex Pointer to supertile index (used to select the
 *                       correct exterior tile set). (was HL')
 * \param[in] scr        Address of output buffer start address. (was DE')
 *
 * \return Next output address. (was DE')
 */
uint8_t *plot_tile_then_advance(tgestate_t             *state,
                                tileindex_t             tile_index,
                                const supertileindex_t *psupertileindex,
                                uint8_t                *scr)
{
  assert(state != NULL);

  return plot_tile(state, tile_index, psupertileindex, scr) + (state->columns * 8 - 1); // -1 compensates the +1 in plot_tile
}

/* ----------------------------------------------------------------------- */

/**
 * $A9AD: Plot a tile then increment 'scr' by 1.
 *
 * Leaf.
 *
 * \param[in] state      Pointer to game state.
 * \param[in] tile_index Tile index. (was A)
 * \param[in] psupertileindex Pointer to supertile index (used to select the
                         correct exterior tile set). (was HL')
 * \param[in] scr        Output buffer start address. (was DE')
 *
 * \return Next output address. (was DE')
 */
uint8_t *plot_tile(tgestate_t             *state,
                   tileindex_t             tile_index,
                   const supertileindex_t *psupertileindex,
                   uint8_t                *scr)
{
  supertileindex_t  supertileindex; /* was A' */
  const tile_t     *tileset;        /* was BC' */
  const tilerow_t  *src;            /* was DE' */
  uint8_t          *dst;            /* was HL' */
  uint8_t           iters;          /* was A */

  assert(state           != NULL);
  assert(tile_index < 220); // ideally the constant should be elsewhere
  assert(psupertileindex != NULL);
  assert(scr             != NULL);

  supertileindex = *psupertileindex; /* get supertile index */
  if (supertileindex < 45)
    tileset = &exterior_tiles_1[0];
  else if (supertileindex < 139 || supertileindex >= 204)
    tileset = &exterior_tiles_2[0];
  else
    tileset = &exterior_tiles_3[0];

  src = &tileset[tile_index].row[0];
  dst = scr;
  iters = 8;
  do
  {
    *dst = *src++;
    dst += state->columns; /* stride */
  }
  while (--iters);

  return scr + 1;
}

/* ----------------------------------------------------------------------- */

/**
 * $A9E4: map_shunt_horizontal_1.
 *
 * \param[in] state Pointer to game state.
 */
void map_shunt_horizontal_1(tgestate_t *state)
{
  assert(state != NULL);

  state->map_position[0]++;

  get_supertiles(state);

  memmove(&state->tile_buf[0], &state->tile_buf[1], visible_tiles_length - 1);
  memmove(&state->window_buf[0], &state->window_buf[1], screen_buffer_length - 1);

  supertile_plot_vertical_1(state);
}

/**
 * $AA05: map_shunt_horizontal_2.
 *
 * \param[in] state Pointer to game state.
 */
void map_shunt_horizontal_2(tgestate_t *state)
{
  assert(state != NULL);

  state->map_position[0]--;

  get_supertiles(state);

  memmove(&state->tile_buf[1], &state->tile_buf[0], visible_tiles_length - 1);
  memmove(&state->window_buf[1], &state->window_buf[0], screen_buffer_length);

  supertile_plot_vertical_2(state);
}

/**
 * $AA26: map_shunt_diagonal_1_2.
 *
 * \param[in] state Pointer to game state.
 */
void map_shunt_diagonal_1_2(tgestate_t *state)
{
  assert(state != NULL);

  // Conv: The original code has a copy of map_position in HL on entry. In
  // this version we read it from source.
  state->map_position[0]--;
  state->map_position[1]++;

  get_supertiles(state);

  memmove(&state->tile_buf[1], &state->tile_buf[24], visible_tiles_length - 24);
  memmove(&state->window_buf[1], &state->window_buf[24 * 8], screen_buffer_length - 24 * 8);

  supertile_plot_horizontal_1(state);
  supertile_plot_vertical_2(state);
}

/**
 * $AA4B: map_shunt_vertical_1.
 *
 * \param[in] state Pointer to game state.
 */
void map_shunt_vertical_1(tgestate_t *state)
{
  assert(state != NULL);

  state->map_position[1]++;

  get_supertiles(state);

  memmove(&state->tile_buf[0], &state->tile_buf[24], visible_tiles_length - 24);
  memmove(&state->window_buf[0], &state->window_buf[24 * 8], screen_buffer_length - 24 * 8);

  supertile_plot_horizontal_1(state);
}

/**
 * $AA6C: map_shunt_vertical_2.
 *
 * \param[in] state Pointer to game state.
 */
void map_shunt_vertical_2(tgestate_t *state)
{
  assert(state != NULL);

  state->map_position[1]--;

  get_supertiles(state);

  memmove(&state->tile_buf[24], &state->tile_buf[0], visible_tiles_length - 24);
  memmove(&state->window_buf[24 * 8], &state->window_buf[0], screen_buffer_length - 24 * 8);

  supertile_plot_horizontal_2(state);
}

/**
 * $AA8D: map_shunt_diagonal_2_1.
 *
 * \param[in] state Pointer to game state.
 */
void map_shunt_diagonal_2_1(tgestate_t *state)
{
  assert(state != NULL);

  state->map_position[0]++;
  state->map_position[1]--;

  get_supertiles(state);

  memmove(&state->tile_buf[24], &state->tile_buf[1], visible_tiles_length - 24 - 1);
  memmove(&state->window_buf[24 * 8], &state->window_buf[1], screen_buffer_length - 24 * 8);

  supertile_plot_horizontal_2(state);
  supertile_plot_vertical_1(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $AAB2: Move map.
 *
 * Moves the map when the character walks.
 *
 * \param[in] state Pointer to game state.
 */
void move_map(tgestate_t *state)
{
  typedef void (movefn_t)(tgestate_t *, uint8_t *);

  static movefn_t *const map_move[] =
  {
    &map_move_up_left,
    &map_move_up_right,
    &map_move_down_right,
    &map_move_down_left,
  };

  const uint8_t *char_related;  /* was DE */
  uint8_t        C;
  uint8_t        A;
  movefn_t      *pmovefn;       /* was HL */
  uint8_t        B;
  uint8_t       *HL;
  uint8_t       *DE;
  uint16_t       game_window_x; /* was HL */
  // uint16_t       HLpos;         /* was HL */

  assert(state != NULL);

  if (state->room_index > room_0_OUTDOORS)
    return; /* Can't move the map when indoors. */

  if (state->vischars[0].b07 & vischar_BYTE7_BIT6)
    return; // unknown

  char_related = state->vischars[0].w0A;
  C = state->vischars[0].b0C;
  A = char_related[3]; // FF, 0, 1, 2, or 3
  if (A == 0xFF)
    return;
  if (C & vischar_BYTE12_BIT7)
    A ^= 2;

  pmovefn = map_move[A];
  // PUSH HL // pushes map_move routine to stack
  // PUSH AF

  B = 0x7C;
  C = 0x00;
  if (A >= 2) // 2 or 3
    B = 0x00;
  if (A != 1 && A != 2) // 0 or 3
    C = 0xC0;

  // so...
  // if A == 0: B,C = 0x7C,C0
  // if A == 1: B,C = 0x7C,00
  // if A == 2: B,C = 0x00,00
  // if A == 3: B,C = 0x00,C0

  // This looks like it ought to be an AND but it's definitely an OR in the game.
  if (state->map_position[0] == C || state->map_position[1] == B)
    return; // don't move

  // POP AF

  HL = &state->move_map_index; // a screen offset of some sort?
  if (A >= 2) // 2 or 3
    A = *HL + 1;
  else // 0 or 1
    A = *HL - 1;
  A &= 3;
  *HL = A;

  DE = HL; // was EX DE,HL

  game_window_x = 0x0000;
  if (A != 0)
  {
    game_window_x = 0x0060;
    if (A != 2)
    {
      game_window_x = 0xFF30; // this must be signed
      if (A != 1)
      {
        game_window_x = 0xFF90;
      }
    }
  }

  state->plot_game_window_x = game_window_x;
  // HLpos = state->map_position; // passing map_position in HL omitted in this version

  // pops and calls map_move_* routine pushed at $AAE0
  pmovefn(state, DE);
}

// called when player moves down-right (map is shifted up-left)
void map_move_up_left(tgestate_t *state, uint8_t *DE)
{
  uint8_t A;

  assert(state != NULL);
  assert(DE != NULL);

  A = *DE;
  if (A == 0)
    map_shunt_vertical_1(state);
  else if (A & 1)
    map_shunt_horizontal_1(state);
}

// called when player moves down-left (map is shifted up-right)
void map_move_up_right(tgestate_t *state, uint8_t *DE)
{
  uint8_t A;

  assert(state != NULL);
  assert(DE != NULL);

  A = *DE;
  if (A == 0)
    map_shunt_diagonal_1_2(state);
  else if (A == 2)
    map_shunt_horizontal_2(state);
}

// called when player moves up-left (map is shifted down-right)
void map_move_down_right(tgestate_t *state, uint8_t *DE)
{
  uint8_t A;

  assert(state != NULL);
  assert(DE != NULL);

  A = *DE;
  if (A == 3)
    map_shunt_vertical_2(state);
  else if ((A & 1) == 0) // CHECK
    map_shunt_horizontal_2(state);
}

// called when player moves up-right (map is shifted down-left)
void map_move_down_left(tgestate_t *state, uint8_t *DE)
{
  uint8_t A;

  assert(state != NULL);
  assert(DE != NULL);

  A = *DE;
  if (A == 1)
    map_shunt_horizontal_1(state);
  else if (A == 3)
    map_shunt_diagonal_2_1(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $AB6B: Choose game window attributes.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Chosen attributes. (was A)
 */
attribute_t choose_game_window_attributes(tgestate_t *state)
{
  attribute_t attr;

  assert(state != NULL);

  if (state->room_index < room_29_SECOND_TUNNEL_START)
  {
    /* The hero is outside, or in a room, but not in a tunnel. */

    if (state->day_or_night == 0) /* Outside or room, daytime. */
      attr = attribute_WHITE_OVER_BLACK;
    else if (state->room_index == room_0_OUTDOORS) /* Outside, nighttime. */
      attr = attribute_BRIGHT_BLUE_OVER_BLACK;
    else /* Room, nighttime. */
      attr = attribute_CYAN_OVER_BLACK;
  }
  else
  {
    /* The hero is in a tunnel. */

    if (state->items_held[0] == item_TORCH ||
        state->items_held[1] == item_TORCH)
    {
      /* The hero holds a torch - draw the room. */

      attr = attribute_RED_OVER_BLACK;
    }
    else
    {
      /* The hero holds no torch - draw nothing. */

      wipe_visible_tiles(state);
      plot_interior_tiles(state);
      attr = attribute_BLUE_OVER_BLACK;
    }
  }

  state->game_window_attribute = attr;

  return attr;
}

/* ----------------------------------------------------------------------- */

/**
 * $ABA0: Zoombox.
 *
 * \param[in] state Pointer to game state.
 */
void zoombox(tgestate_t *state)
{
  attribute_t attrs; /* was A */
  uint8_t    *pvar;  /* was HL */
  uint8_t     var;   /* was A */

  assert(state != NULL);

  state->zoombox.x = 12;
  state->zoombox.y = 8;

  attrs = choose_game_window_attributes(state);

  state->speccy->attributes[ 9 * state->width + 18] = attrs;
  state->speccy->attributes[ 9 * state->width + 19] = attrs;
  state->speccy->attributes[10 * state->width + 18] = attrs;
  state->speccy->attributes[10 * state->width + 19] = attrs;

  state->zoombox.width  = 0;
  state->zoombox.height = 0;

  do
  {
    pvar = &state->zoombox.x;
    var = *pvar;
    if (var != 1)
    {
      (*pvar)--;
      var--;
      pvar[1]++;
    }

    pvar++; // &state->width;
    var += *pvar;
    if (var < 22)
      (*pvar)++;

    pvar++; // &state->zoombox.y;
    var = *pvar;
    if (var != 1)
    {
      (*pvar)--;
      var--;
      pvar[1]++;
    }

    pvar++; // &state->height;
    var += *pvar;
    if (var < 15)
      (*pvar)++;

    zoombox_fill(state);
    zoombox_draw_border(state);

    state->speccy->kick(state->speccy);
  }
  while (state->zoombox.height + state->zoombox.width < 35);
}

/**
 * $ABF9: Zoombox. partial copy of window buffer contents?
 *
 * \param[in] state Pointer to game state.
 */
void zoombox_fill(tgestate_t *state)
{
  assert(state != NULL);

  uint8_t *const screen_base = &state->speccy->screen[0]; // Conv: Added

  uint8_t   iters;      /* was B */
  uint8_t   hz_count;   /* was A */
  uint8_t   iters2;     /* was A */
  uint8_t   hz_count1;  /* was $AC55 */
  uint8_t   src_skip;   /* was $AC4D */
  uint16_t  hz_count2;  /* was BC */
  uint8_t  *dst;        /* was DE */
  uint16_t  offset;     /* was HL */
  uint8_t  *src;        /* was HL */
  uint16_t  dst_stride; /* was BC */
  uint8_t  *prev_dst;   /* was stack */

  /* Conv: Simplified calculation to use a single multiply. */
  offset = state->zoombox.y * state->tb_columns * 8 + state->zoombox.x;
  src = &state->window_buf[offset + 1];
  ASSERT_WINDOW_BUF_PTR_VALID(src);
  dst = screen_base + state->game_window_start_offsets[state->zoombox.y * 8] + state->zoombox.x; // Conv: Screen base was hoisted from table.
  ASSERT_SCREEN_PTR_VALID(dst);

  hz_count  = state->zoombox.width;
  hz_count1 = hz_count;
  src_skip  = state->tb_columns - hz_count;

  iters = state->zoombox.height; /* iterations */
  do
  {
    prev_dst = dst;

    iters2 = 8; // one row
    do
    {
      hz_count2 = state->zoombox.width; /* TODO: This duplicates the read above. */
      memcpy(dst, src, hz_count2);

      // these computations might take the values out of range on the final iteration (but never be used)
      dst += hz_count2; // this is LDIR post-increment. it can be removed along with the line below.
      src += hz_count2 + src_skip; // move to next source line
      dst -= hz_count1; // was E -= self_AC55; // undo LDIR postinc
      dst += state->width * 8; // was D++; // move to next scanline

      if (iters2 > 1 || iters > 1)
        ASSERT_SCREEN_PTR_VALID(dst);
    }
    while (--iters2);

    dst = prev_dst;

    dst_stride = state->width; // scanline-to-scanline stride (32)
    if (((dst - screen_base) & 0xFF) >= 224) // 224+32 == 256, so do skip to next band // TODO: 224 should be (256 - state->width)
      dst_stride += 7 << 8; // was B = 7

    dst += dst_stride;
  }
  while (--iters);
}

/**
 * $AC6F: zoombox_draw_border
 *
 * \param[in] state Pointer to game state.
 */
void zoombox_draw_border(tgestate_t *state)
{
  assert(state != NULL);

  uint8_t *const screen_base = &state->speccy->screen[0]; // Conv: Added var.

  uint8_t *addr;  /* was HL */
  uint8_t  iters; /* was B */
  int      delta; /* was DE */

  addr = screen_base + state->game_window_start_offsets[(state->zoombox.y - 1) * 8]; // Conv: Screen base hoisted from table.
  ASSERT_SCREEN_PTR_VALID(addr);

  /* Top left */
  addr += state->zoombox.x - 1;
  zoombox_draw_tile(state, zoombox_tile_TL, addr++);

  /* Horizontal, moving right */
  iters = state->zoombox.width;
  do
    zoombox_draw_tile(state, zoombox_tile_HZ, addr++);
  while (--iters);

  /* Top right */
  zoombox_draw_tile(state, zoombox_tile_TR, addr);
  delta = state->width; // move to next character row
  if (((addr - screen_base) & 0xFF) >= 224) // was: if (L >= 224) // if adding 32 would overflow
    delta += 0x0700; // was D=7
  addr += delta;

  /* Vertical, moving down */
  iters = state->zoombox.height;
  do
  {
    zoombox_draw_tile(state, zoombox_tile_VT, addr);
    delta = state->width; // move to next character row
    if (((addr - screen_base) & 0xFF) >= 224) // was: if (L >= 224) // if adding 32 would overflow
      delta += 0x0700; // was D=7
    addr += delta;
  }
  while (--iters);

  /* Bottom right */
  zoombox_draw_tile(state, zoombox_tile_BR, addr--);

  /* Horizontal, moving left */
  iters = state->zoombox.width;
  do
    zoombox_draw_tile(state, zoombox_tile_HZ, addr--);
  while (--iters);

  /* Bottom left */
  zoombox_draw_tile(state, zoombox_tile_BL, addr);
  delta = -state->width; // move to next character row
  if (((addr - screen_base) & 0xFF) < 32) // was: if (L < 32) // if subtracting 32 would underflow
    delta -= 0x0700;
  addr += delta;

  /* Vertical, moving up */
  iters = state->zoombox.height;
  do
  {
    zoombox_draw_tile(state, zoombox_tile_VT, addr);
    delta = -state->width; // move to next character row
    if (((addr - screen_base) & 0xFF) < 32) // was: if (L < 32) // if subtracting 32 would underflow
      delta -= 0x0700;
    addr += delta;
  }
  while (--iters);
}

/**
 * $ACFC: Draw a zoombox border tile.
 *
 * \param[in] state Pointer to game state.
 * \param[in] index Tile index into zoombox_tiles. (was A)
 * \param[in] addr  Screen address.                (was HL)
 */
void zoombox_draw_tile(tgestate_t *state, uint8_t index, uint8_t *addr)
{
  /**
   * $AF5E: Zoombox tiles.
   */
  static const tile_t zoombox_tiles[6] =
  {
    { { 0x00, 0x00, 0x00, 0x03, 0x04, 0x08, 0x08, 0x08 } }, /* tl */
    { { 0x00, 0x20, 0x18, 0xF4, 0x2F, 0x18, 0x04, 0x00 } }, /* hz */
    { { 0x00, 0x00, 0x00, 0x00, 0xE0, 0x10, 0x08, 0x08 } }, /* tr */
    { { 0x08, 0x08, 0x1A, 0x2C, 0x34, 0x58, 0x10, 0x10 } }, /* vt */
    { { 0x10, 0x10, 0x10, 0x20, 0xC0, 0x00, 0x00, 0x00 } }, /* br */
    { { 0x10, 0x10, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00 } }, /* bl */
  };

  uint8_t         *DE;    /* was DE */
  const tilerow_t *row;   /* was ? */
  uint8_t          iters; /* was B */
  ptrdiff_t        off;   /* was ? */
  attribute_t     *attrp; /* was ? */

  assert(state != NULL);
  assert(index < NELEMS(zoombox_tiles));
  ASSERT_SCREEN_PTR_VALID(addr);

  DE = addr; // was EX DE,HL
  row = &zoombox_tiles[index].row[0];
  iters = 8;
  do
  {
    *DE = *row++;
    DE += 256;
  }
  while (--iters);

  /* Original game can munge bytes directly here, assuming screen addresses,
   * but I can't... */

  DE -= 256; /* Compensate for overshoot */
  off = DE - &state->speccy->screen[0];

  attrp = &state->speccy->attributes[off & 0xFF]; // to within a third

  // can i ((off >> 11) << 8) ?
  if (off >= 0x0800)
  {
    attrp += 256;
    if (off >= 0x1000)
      attrp += 256;
  }

  *attrp = state->game_window_attribute;
}

/* ----------------------------------------------------------------------- */

/**
 * $AD59: (unknown)
 *
 * \param[in] state Pointer to game state.
 * \param[in] HL    Pointer to spotlight_movement_data_maybe.
 */
void searchlight_AD59(tgestate_t *state, uint8_t *HL)
{
  uint8_t   D;  /* was D */
  uint8_t   E;  /* was E */
  uint8_t   A;  /* was A */
  uint8_t   B;  /* was B */
  uint8_t   C;  /* was C */
  uint16_t *BC; /* was BC */

  assert(state != NULL);
  assert(HL != NULL);

  E = *HL++;
  D = *HL++;
  (*HL)--;
  if (*HL == 0)
  {
    HL += 2;
    A = *HL; // sampled HL = $AD3B, $AD34, $AD2D
    if (A & (1 << 7))
    {
      A &= 0x7F;
      if (A == 0)
      {
        *HL &= ~(1 << 7);
      }
      else
      {
        (*HL)--;
        A--;
      }
    }
    else
    {
      A++;
      *HL = A;
    }
    BC = HL[1] | (HL[2] << 8);
    // sampled BC $AD49, AD47, AD40, AD54, AD45, AD43, AD42, AD43, AD56, AD3E, AD45, AD47, AD58, AD3E
    A = BC[A * 2];
    if (A == 0xFF) // end of list?
    {
      (*HL)--;
      *HL |= 1 << 7;
      BC -= 2;
      A = *BC;
    }
    HL -= 2;
    *HL++ = *BC++;
    *HL = *BC;
  }
  else
  {
    HL++;
    A = *HL++;
    if (*HL & (1 << 7))
      A ^= 2;
    if (A < 2)
      D -= 2;
    D++;
    if (A != 0 && A != 3)
      E += 2;
    else
      E -= 2;
    HL -= 3;
    *HL-- = D;
    *HL = E;
  }
}

/**
 * $ADBD: Turns white screen elements light blue and tracks the hero with a
 * searchlight.
 *
 * \param[in] state Pointer to game state.
 */
void nighttime(tgestate_t *state)
{
  /**
   * $AD29: Likely: spotlight movement data.
   */
  static const uint8_t spotlight_movement_data_maybe[] =
  {
    0x24, 0x52, 0x2C, 0x02, 0x00, 0x54, 0xAD,
    0x78, 0x52, 0x18, 0x01, 0x00, 0x43, 0xAD,
    0x3C, 0x4C, 0x20, 0x02, 0x00, 0x3E, 0xAD,
    0x20, 0x02, 0x20, 0x01, 0xFF,

    0x18, 0x01, 0x0C, 0x00, 0x18, 0x03, 0x0C,
    0x00, 0x20, 0x01, 0x14, 0x00, 0x20, 0x03,
    0x2C, 0x02, 0xFF,

    0x2C, 0x02, 0x2A, 0x01, 0xFF,
  };

  uint8_t  D;
  uint8_t  E;
  uint8_t  H;
  uint8_t  L;
  uint8_t  iters; /* was B */
  uint8_t  A;
  uint8_t  Adash;
  uint8_t  C;
  uint16_t HLrow;
  uint16_t BCcolumn;

  assert(state != NULL);

  if (state->searchlight_state == searchlight_STATE_OFF)
    goto not_tracking;

  if (state->room_index > room_0_OUTDOORS)
  {
    /* If the hero goes indoors then the searchlight loses track. */
    state->searchlight_state = searchlight_STATE_OFF;
    return;
  }

  /* The hero is outdoors. */

  /* If the searchlight previously caught the hero, then track him. */

  if (state->searchlight_state == searchlight_STATE_CAUGHT)
  {
    E = state->map_position[0] + 4;
    D = state->map_position[1];
    L = state->searchlight_coords[0]; /* Conv: Fused load split apart. */
    H = state->searchlight_coords[1];

    /* If the highlight doesn't need to move, quit. */
    if (L == E && H == D)
      return;

    /* Move searchlight left/right to focus on the hero. */
    // Bug? Possibly missing L != E test surrounding the block.
    if (L < E)
      L++;
    else
      L--;

    /* Move searchlight up/down to focus on the hero. */
    if (H != D)
    {
      if (H < D)
        H++;
      else
        H--;
    }

    state->searchlight_coords[0] = L; // Conv: Fused store split apart.
    state->searchlight_coords[1] = H;
  }

  uint8_t *HL;

  E = state->map_position[0];
  D = state->map_position[1];
  HL = &state->searchlight_coords[1]; // offset of 1 compensates for the HL-- ahead
  iters = 1; // 1 iteration
  // PUSH BC
  // PUSH HL
  goto ae3f;

not_tracking:

  HL = &spotlight_movement_data_maybe[0];
  iters = 3; // 3 iterations
  do
  {
    // PUSH BC

    // PUSH HL
    searchlight_AD59(state, HL);
    // POP HL

    // PUSH HL
    searchlight_caught(state, HL);
    // POP HL

    // PUSH HL
    E = state->map_position[0];
    D = state->map_position[1];
    // Original: if (E + 23 < HL[0] || HL[0] + 16 < E)
    if (E < HL[0] - 23 || E >= HL[0] + 16) // E-22 .. E+15
      goto next;
    // Original: if (D + 16 < HL[1] || HL[1] + 16 < D)
    if (D < HL[1] - 16 || D >= HL[1] + 16) // D-16 .. D+15
      goto next;
    HL++;

ae3f:
    A = 0;
    //Adash = A;
    HL--;
    iters = 0x00;
    Adash = *HL;
    if (Adash < E)
    {
      iters = 0xFF;
      A = ~A; // this looks like it's always ~0
    }

    C = A;
    A = *++HL;
    H = 0x00;
    A -= D;
    if (A < 0)
      H = 0xFF;
    L = A;
    // HL must be row, BC must be column
    HLrow = (H << 8) | L;
    BCcolumn = (iters << 8) | C;
    HL = HLrow * state->width + BCcolumn + &state->speccy->attributes[0x46]; // address of top-left game window attribute
    // EX DE,HL

    state->searchlight_related = A;
    searchlight_plot(state);

next:
    // POP HL

    // POP BC
    HL += 7;
  }
  while (--iters);
}

/**
 * $AE78: Is the hero is caught in the spotlight?
 *
 * \param[in] state Pointer to game state.
 * \param[in] HL    Pointer to spotlight_movement_data_maybe.
 */
void searchlight_caught(tgestate_t *state, uint8_t *HL)
{
  uint8_t D, E;

  assert(state != NULL);
  assert(HL != NULL);

  D = state->map_position[1];
  E = state->map_position[0];

  if ((HL[0] + 5 >= E + 12 || HL[0] + 10 < E + 10) ||
      (HL[1] + 5 >= D + 10 || HL[1] + 12 < D +  6))
    return;
  // We could rearrange the above like this:
  //if ((HL[0] >= E + 7 || HL[0] < E    ) ||
  //    (HL[1] >= D + 5 || HL[1] < D - 6))
  //  return;
  // But that needs checking for edge cases.

  if (state->searchlight_state == searchlight_STATE_CAUGHT)
    return; /* already caught */ // seems odd to not do this test before the earlier one

  state->searchlight_state = searchlight_STATE_CAUGHT;

  state->searchlight_coords[0] = HL[1];
  state->searchlight_coords[1] = HL[0];

  state->bell = bell_RING_PERPETUAL;

  decrease_morale(state, 10); // exit via
}

/**
 * $AEB8: Searchlight plotter.
 *
 * \param[in] state Pointer to game state.
 */
void searchlight_plot(tgestate_t *state)
{
  /**
   * $AF3E: Searchlight shape
   */
  static const uint8_t searchlight_shape[2 * 16] =
  {
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x01, 0x80,
    0x07, 0xE0,
    0x0F, 0xF0,
    0x0F, 0xF0,
    0x1F, 0xF8,
    0x1F, 0xF8,
    0x0F, 0xF0,
    0x0F, 0xF0,
    0x07, 0xE0,
    0x01, 0x80,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
  };

  assert(state != NULL);

  attribute_t *const attrs = &state->speccy->attributes[0];
  ASSERT_SCREEN_ATTRIBUTES_PTR_VALID(attrs);

  const uint8_t *shape;   /* was DE */
  uint8_t        iters;   /* was C */
  uint8_t        A;       /* was A */
  attribute_t   *pattrs;  /* was HL */

  uint8_t         D, E;
  uint8_t         L;
  uint8_t         iters2; /* was B' */
  uint8_t         iters3, C;
  int             carry;

  shape = &searchlight_shape[0];
  iters = 16; /* height */
  do
  {
    A = state->searchlight_related;
    pattrs = &attrs[0x240]; // was HL = 0x5A40; // screen attribute address (column 0 + bottom of game screen)

    // Conv: Was E & 31 => 4th to last row, or later
//    if (A != 0 && (shape & 31) >= 22)
//      pattrs = (pattrs & ~0xFF) | 32; // Conv: was L = 32; // FIXME. Needs pointer subtraction.

    // SBC HL,DE
    // RET C  // if (HL < DE) return; // what about carry?
    if (pattrs < shape) // does this even make sense?
      return;

    // PUSH DE
    pattrs = &attrs[0x20]; // screen attribute address (column 0 + top of game screen)
    if (A != 0 && (E & 31) >= 7)
      L = 32;

    // SBC HL,DE
    // JR C,aef0  // if (HL < DE) goto aef0;
    if (pattrs >= shape)
    {
      shape += 2;
      goto nextrow;
    }

    // EX DE,HL

    iters2 = 2; /* iterations */
    do
    {
      C = *shape; // Conv: Original uses A as temporary.

      D = 7; // left edge?
      E = 30; // right edge?
      iters3 = 8; /* iterations */
      do
      {
        uint8_t oldA;

        oldA = state->searchlight_related;
        A = L; // Conv: was interleaved
        if (oldA != 0)
          goto af0c;
        if ((A & 31) >= 22)
          goto af1b;
        goto af18;

af0c:
        if ((A & 31) < E)
          goto af18;

        do
          shape++;
        while (--iters2);

        goto nextrow;

af18:
        if (A < D)
        {
af1b:
          RL(C);
        }
        else
        {
          RL(C);
          if (carry)
            *pattrs = attribute_YELLOW_OVER_BLACK;
          else
            *pattrs = attribute_BRIGHT_BLUE_OVER_BLACK;
        }
        pattrs++;
      }
      while (--iters3);

      shape++;
    }
    while (--iters2);

nextrow:
    // POP HL
    pattrs += state->width;
    // EX DE,HL
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $AF8F: Stuff touching.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] Adash   Flip flag + sprite offset.
 *
 * \return 0/1 => within/outwith bounds
 */
int touch(tgestate_t *state, vischar_t *vischar, uint8_t Adash)
{
  uint8_t stashed_A; /* was $81AA */ // FUTURE: Can be removed.

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  // assert(Adash);

  stashed_A = Adash;
  vischar->b07 |= vischar_BYTE7_BIT6 | vischar_BYTE7_BIT7;  // wild guess: clamp character in position?

  // Conv: Removed little register shuffle to get (vischar & 0xFF) into A.

  /* If hero is player controlled, then check for door transitions. */
  if (vischar == &state->vischars[0] && state->automatic_player_counter > 0)
    door_handling(state, vischar);

  /* If non-player character or hero is not cutting the fence. */
  if (vischar > &state->vischars[0] || ((state->vischars[0].flags & (vischar_FLAGS_PICKING_LOCK | vischar_FLAGS_CUTTING_WIRE)) != vischar_FLAGS_CUTTING_WIRE))
  {
    if (bounds_check(state, vischar)) // if within bounds?
      return 1; // NZ
  }

  if (vischar->character <= character_25_PRISONER_6) /* character temp was in Adash */
    if (collision(state, vischar))
      return 1; // NZ

  vischar->b07 &= ~vischar_BYTE7_BIT6;
  vischar->mi.pos = state->saved_pos;
  vischar->mi.flip_sprite = stashed_A; // left/right flip flag / sprite offset

  // A = 0; likely just to set flags
  return 0; // Z
}

/* ----------------------------------------------------------------------- */

/**
 * $AFDF: (unknown) Definitely something to do with moving objects around. Collisions?
 *
 * \param[in] state   Pointer to game state.
 * \param[in] input_vischar Pointer to visible character. (was IY)
 */
int collision(tgestate_t *state, vischar_t *input_vischar)
{
  /** $B0F8: */
  static const uint8_t four_bytes[] = { 0x85, 0x84, 0x87, 0x88 };

  vischar_t  *vischar; /* was HL */
  uint8_t     iters;   /* was B */
  uint16_t    BC;      /* was BC */
  uint16_t    x;       /* was HL */
  uint16_t    y;       /* was HL */
  int8_t      delta;   /* was A */
  uint8_t     A;
  character_t character; /* was A */
  uint8_t     B;
  uint8_t     C;
  character_t tmpA;

  assert(state != NULL);
  assert(input_vischar != NULL);

  vischar = &state->vischars[0];
  iters = vischars_LENGTH - 1;
  do
  {
    if (vischar->flags & vischar_FLAGS_BIT7)
      goto next; // $8001, $8021, ...

    // PUSH BC, HL here

    // Check: Check this all over...

    BC = vischar->mi.pos.x + 4;
    x = state->saved_pos.x;
    if (x != BC)
      if (x > BC || state->saved_pos.x < BC - 8)
        goto pop_next;

    BC = vischar->mi.pos.y + 4;
    y = state->saved_pos.y;
    if (y != BC)
      if (y > BC || state->saved_pos.y < BC - 8)
        goto pop_next;

    delta = state->saved_pos.height - vischar->mi.pos.height; // Note: Signed result.
    if (delta < 0)
      delta = -delta; // absolute value
    if (delta >= 24)
      goto pop_next;

    /* If specified vischar character has a bribe pending... */
    // Odd: 0x0F is *not* vischar_FLAGS_MASK, which is 0x3F
    if ((input_vischar->flags & 0x0F) == vischar_FLAGS_BRIBE_PENDING) // sampled IY=$8020, $8040, $8060, $8000
    {
      /* and current vischar is not the hero... */
      if (vischar != &state->vischars[0])
      {
        if (state->bribed_character == input_vischar->character)
        {
          accept_bribe(state, input_vischar);
        }
        else
        {
          /* A hostile catches the hero! */

          // POP HL
          // POP BC
          vischar = input_vischar + 1; // 1 byte == 2 bytes into struct => vischar->target
          // but .. that's not needed
          solitary(state); // is this supposed to take a target?
          NEVER_RETURNS 0;
        }
      }
    }

    // POP HL // $8021 etc.

    character = vischar->character; // sampled HL = $80C0, $8040, $8000, $8020, $8060 // vischar_BYTE0
    if (character >= character_26_STOVE_1)
    {
      // PUSH HL
      uint16_t *coord; /* was HL */

      coord = &vischar->mi.pos.y;

      B = 7; // min y
      C = 35; // max y
      tmpA = character;
      A = input_vischar->b0E; // interleaved
      if (tmpA == character_28_CRATE)
      {
        // crate must move on x axis only
        coord--; // -> HL->mi.pos.x
        C = 54; // max x
        A ^= 1;
      }
      // test direction?
      if (A == 0)
      {
        A = *coord;
        if (A != C)
        {
          if (A >= C)
            (*coord) -= 2;  // decrement by -2 then execute +1 anyway to avoid branch
          (*coord)++;
        }
      }
      else if (A == 1)
      {
        A = C + B;
        if (A != *coord)
          (*coord)++;
      }
      else if (A == 2)
      {
        A = C - B;
        *coord = A;
      }
      else
      {
        A = C - B;
        if (A != *coord)
          (*coord)--;
      }
      // POP HL
      // POP BC
    }


    uint8_t b0D;
    uint8_t b0E;

    b0D = vischar->b0D & vischar_BYTE13_MASK; // sampled HL = $806D, $804D, $802D, $808D, $800D
    if (b0D)
    {
      b0E = vischar->b0E ^ 2;
      if (b0E != input_vischar->b0E)
      {
        input_vischar->b0E = vischar_BYTE13_BIT7;

b0d0:
        input_vischar->b07 = (input_vischar->b07 & vischar_BYTE7_MASK_HI) | 5; // preserve flags and set 5? // sampled IY = $8000, $80E0
        // Weird code in the original game which ORs 5 then does a conditional return dependent on Z clear.
        //if (!Z)
          return 1; /* odd -- returning with Z not set? */
      }
    }

    BC = input_vischar->b0E;
    input_vischar->b0D = four_bytes[BC]; // sampled IY = $8000, $8040, $80E0
    if ((BC & 1) == 0)
      input_vischar->b07 &= ~vischar_BYTE7_BIT5;
    else
      input_vischar->b07 |= vischar_BYTE7_BIT5;
    goto b0d0;

pop_next:
    // POP HL
    // POP BC
next:
    vischar++;
  }
  while (--iters);

  return 0; // return with Z set?
}

/* ----------------------------------------------------------------------- */

/**
 * $B107: Character accepts the bribe.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void accept_bribe(tgestate_t *state, vischar_t *vischar)
{
  item_t    *item;     /* was DE */
  uint8_t    iters;    /* was B */
  vischar_t *vischar2; /* was HL */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  increase_morale_by_10_score_by_50(state);

  vischar->flags = 0;

  sub_CB23(state, vischar, &vischar->target);

  item = &state->items_held[0];
  if (*item != item_BRIBE && *++item != item_BRIBE)
    return; /* Have no bribes. */

  /* We have a bribe, take it away. */
  *item = item_NONE;

  state->item_structs[item_BRIBE].room_and_flags = (room_t) itemstruct_ROOM_NONE;

  draw_all_items(state);

  /* Iterate over non-player characters. */
  iters    = vischars_LENGTH - 1;
  vischar2 = &state->vischars[1];
  do
  {
    if (vischar2->character <= character_19_GUARD_DOG_4) /* Hostile characters only. */
      vischar2->flags = vischar_FLAGS_BIT2; // look for bribed character?
    vischar2++;
  }
  while (--iters);

  queue_message_for_display(state, message_HE_TAKES_THE_BRIBE);
  queue_message_for_display(state, message_AND_ACTS_AS_DECOY);
}

/* ----------------------------------------------------------------------- */

/**
 * $B14C: Check the character is inside of bounds.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 *
 * \return 0 => outwith bounds, 1 => within
 */
int bounds_check(tgestate_t *state, vischar_t *vischar)
{
  uint8_t       iters; /* was B */
  const wall_t *wall;  /* was DE */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  if (state->room_index > room_0_OUTDOORS)
    return interior_bounds_check(state, vischar);

  iters = NELEMS(walls); // count of walls + fences
  wall  = &walls[0];
  do
  {
    uint16_t minx, maxx, miny, maxy, minheight, maxheight;

    minx      = wall->minx      * 8;
    maxx      = wall->maxx      * 8;
    miny      = wall->miny      * 8;
    maxy      = wall->maxy      * 8;
    minheight = wall->minheight * 8;
    maxheight = wall->maxheight * 8;

    if (state->saved_pos.x      >= minx + 2   &&
        state->saved_pos.x      <  maxx + 4   &&
        state->saved_pos.y      >= miny       &&
        state->saved_pos.y      <  maxy + 4   &&
        state->saved_pos.height >= minheight  &&
        state->saved_pos.height <  maxheight + 2)
    {
      vischar->b07 ^= vischar_BYTE7_BIT5;
      return 1; // NZ
    }

    wall++;
  }
  while (--iters);

  return 0; // Z
}

/* ----------------------------------------------------------------------- */

/**
* $B1C7: BC becomes A * 8.
*
* Leaf.
*
* \param[in] 'A' to be multiplied and widened.
*
* \return 'A' * 8 widened to a uint16_t.
*/
uint16_t multiply_by_8(uint8_t A)
{
  return A * 8;
}

/* ----------------------------------------------------------------------- */

#define door_FLAG_LOCKED (1 << 7)

/**
* $B1D4: Locate current door and queue message if it's locked.
*
* \param[in] state Pointer to game state.
*
* \return 0 => open, 1 => locked.
*/
int is_door_open(tgestate_t *state)
{
  int      mask;  /* new */
  int      cur;   /* was C */
  uint8_t *door;  /* was HL */
  int      iters; /* was B */

  assert(state != NULL);

  mask  = 0xFF & ~door_FLAG_LOCKED;
  cur   = state->current_door & mask;
  door  = &state->gates_and_doors[0];
  iters = NELEMS(state->gates_and_doors);
  do
  {
    if ((*door & mask) == cur)
    {
      if ((*door & door_FLAG_LOCKED) == 0)
        return 0; /* open */

      queue_message_for_display(state, message_THE_DOOR_IS_LOCKED);
      return 1; /* locked */
    }
    door++;
  }
  while (--iters);

  return 0; /* open */
}

/* ----------------------------------------------------------------------- */

/**
 * $B1F5: Door handling.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 *
 * \return Nothing. // suspect A is returned
 */
void door_handling(tgestate_t *state, vischar_t *vischar)
{
  const doorpos_t *doorpos; /* was HL */
  uint8_t          b0E;     /* was E */
  uint8_t          iters;   /* was B */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  if (state->room_index > room_0_OUTDOORS)
  {
    door_handling_interior(state, vischar);
    return;
  }

  doorpos = &door_positions[0];
  b0E = vischar->b0E;
  if (b0E >= 2)
    doorpos = &door_positions[1];

  iters = 16;
  do
  {
    if ((doorpos->room_and_flags & doorposition_BYTE0_MASK_LO) == b0E)
      if (door_in_range(state, doorpos))
        goto found;
    doorpos++;
  }
  while (--iters);
  // A &= B; // seems to exist to set Z, but routine doesn't return stuff
  return;

found:
  state->current_door = 16 - iters;

  if (is_door_open(state))
    return; // door was locked - return nonzero?

  vischar->room = (doorpos->room_and_flags & doorposition_BYTE0_MASK_HI) >> 2; // sampled HL = $792E (in door_positions[])
  if ((doorpos->room_and_flags & doorposition_BYTE0_MASK_LO) >= 2)
  {
    // point at the next door's pos
    transition(state, &doorpos[1].pos);
  }
  else
  {
    // point at the previous door's pos
    transition(state, &doorpos[-1].pos);
  }
  NEVER_RETURNS; // highly likely if this is only tiggered by the hero
}

/* ----------------------------------------------------------------------- */

/**
 * $B252: Door in range.
 *
 * (saved_Y,saved_X) within (-2,+2) of HL[1..] scaled << 2
 *
 * \param[in] state   Pointer to game state.
 * \param[in] doorpos Pointer to doorpos_t. (was HL)
 *
 * \return 1 if in range, 0 if not. (was carry flag)
 */
int door_in_range(tgestate_t *state, const doorpos_t *doorpos)
{
  uint16_t x; /* was BC */
  uint16_t y; /* was BC */

  assert(state != NULL);
  assert(doorpos != NULL);

  x = multiply_by_4(doorpos->pos.x);
  if (state->saved_pos.x < x - 3 || state->saved_pos.x >= x + 3)
    return 0;

  y = multiply_by_4(doorpos->pos.y);
  if (state->saved_pos.y < y - 3 || state->saved_pos.y >= y + 3)
    return 0;

  return 1;
}

/* ----------------------------------------------------------------------- */

/**
 * $B295: BC becomes A * 4.
 *
 * Leaf.
 *
 * \param[in] 'A' to be multiplied and widened.
 *
 * \return 'A' * 4 widened to a uint16_t.
 */
uint16_t multiply_by_4(uint8_t A)
{
  return A * 4;
}

/* ----------------------------------------------------------------------- */

/**
 * $B29F: Check the character is inside of bounds, when indoors.
 *
 * Leaf.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 *
 * \return 1 if a boundary was hit, 0 otherwise.
 */
int interior_bounds_check(tgestate_t *state, vischar_t *vischar)
{
  /**
   * $6B85: Room dimensions.
   */
  static const bounds_t roomdef_bounds[10] =
  {
    { 0x42,0x1A, 0x46,0x16 },
    { 0x3E,0x16, 0x3A,0x1A },
    { 0x36,0x1E, 0x42,0x12 },
    { 0x3E,0x1E, 0x3A,0x22 },
    { 0x4A,0x12, 0x3E,0x1E },
    { 0x38,0x32, 0x64,0x0A },
    { 0x68,0x06, 0x38,0x32 },
    { 0x38,0x32, 0x64,0x1A },
    { 0x68,0x1C, 0x38,0x32 },
    { 0x38,0x32, 0x58,0x0A },
  };

  const bounds_t *room_bounds;   /* was BC */
  pos_t          *saved_pos;     /* was HL */
  bounds_t       *object_bounds; /* was HL */
  uint8_t         nbounds;       /* was B */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  room_bounds = &roomdef_bounds[state->roomdef_bounds_index];
  saved_pos = &state->saved_pos;
  /* Conv: Merged conditions. */
  if (room_bounds->x0     < saved_pos->x || room_bounds->x1 + 4 >= saved_pos->x ||
      room_bounds->y0 - 4 < saved_pos->y || room_bounds->y1     >= saved_pos->y)
    goto stop;

  object_bounds = &state->roomdef_object_bounds[0]; /* Conv: Moved around. */
  for (nbounds = state->roomdef_object_bounds_count; nbounds > 0; nbounds--)
  {
    pos_t   *pos;  /* was DE */
    uint8_t  x, y; /* was A, A */

    /* Conv: HL dropped. */
    pos = &state->saved_pos;

    /* Conv: Two-iteration loop unrolled. */
    x = pos->x; // narrowing // saved_pos must be 8-bit
    if (x < object_bounds->x0 || x >= object_bounds->x1)
      goto next;
    y = pos->y;
    if (y < object_bounds->y0 || y >= object_bounds->y1)
      goto next;

stop:
    /* Found. */
    vischar->b07 ^= vischar_BYTE7_BIT5;
    return 1; /* return NZ */

next:
    /* Next iteration. */
    object_bounds++;
  }

  /* Not found. */
  return 0; /* return Z */
}

/* ----------------------------------------------------------------------- */

/**
 * $B2FC: Reset the hero's position, redraw the scene, then zoombox it onto
 * the screen.
 *
 * \param[in] state Pointer to game state.
 */
void reset_outdoors(tgestate_t *state)
{
  assert(state != NULL);

  /* Reset hero. */
  reset_position(state, &state->vischars[0]);

  /* Centre the screen on the hero. */
  /* Conv: Removed divide_by_8 calls here. */
  state->map_position[0] = (state->vischars[0].scrx >> 3) - 11; // 11 would be screen width minus half of character width?
  state->map_position[1] = (state->vischars[1].scry >> 3) - 6;  // 6 would be screen height minus half of character height?

  state->room_index = room_NONE;
  get_supertiles(state);
  supertile_plot_all(state);
  setup_movable_items(state);
  zoombox(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B32D: Door handling (indoors).
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 */
void door_handling_interior(tgestate_t *state, vischar_t *vischar)
{
  uint8_t         *pdoor_related;   /* was HL */
  uint8_t          current_door;    /* was A */
  uint8_t          room_and_flags;  /* was A */
  const doorpos_t *door;            /* was HL' */
  const tinypos_t *tinypos;         /* was HL' */
  pos_t           *pos;             /* was DE' */
  uint8_t          coord;           /* was A */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  pdoor_related = &state->door_related[0];
  for (;;)
  {
    current_door = *pdoor_related;
    if (current_door == 255)
      return;

    state->current_door = current_door;

    door = get_door_position(current_door);
    room_and_flags = door->room_and_flags;

    // Conv: Cdash removed
    if ((vischar->b0D & 3) != (room_and_flags & 3)) // used B' // could be a check for facing the same direction as the door?
      goto next;

    tinypos = &door->pos;
    // registers renamed here
    pos = &state->saved_pos; // reusing saved_pos as 8-bit here? a case for saved_pos being a union of tinypos and pos types?

    // Conv: Unrolled.
    coord = tinypos->x;
    if (((coord - 3) >= pos->x) || (coord + 3) < pos->x)
      goto next; // -3 .. +2
    coord = tinypos->y;
    if (((coord - 3) >= pos->y) || (coord + 3) < pos->y)
      goto next; // -3 .. +2

    door++; // Conv: Original just incremented HL'.

    if (is_door_open(state) == 0)
      return; /* the door was closed */

    vischar->room = (room_and_flags & doorposition_BYTE0_MASK_HI) >> 2;

    tinypos = &door->pos;
    if (state->current_door & (1 << 7))
      tinypos = &door[-2].pos;

    transition(state, tinypos);

    return; // exit via // with banked registers...

next:
    pdoor_related++;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B387: The hero has tried to open the red cross parcel.
 *
 * \param[in] state Pointer to game state.
 */
void action_red_cross_parcel(tgestate_t *state)
{
  item_t *item; /* was HL */

  assert(state != NULL);

  state->item_structs[item_RED_CROSS_PARCEL].room_and_flags = room_NONE & itemstruct_ROOM_MASK;

  item = &state->items_held[0];
  if (*item != item_RED_CROSS_PARCEL)
    item++; /* One or the other must be a red cross parcel item, so advance. */
  *item = item_NONE; /* Parcel opened. */

  draw_all_items(state);

  drop_item_tail(state, state->red_cross_parcel_current_contents);

  queue_message_for_display(state, message_YOU_OPEN_THE_BOX);
  increase_morale_by_10_score_by_50(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B3A8: The hero tries to bribe a prisoner.
 *
 * This searches visible friendly characters only and returns the first found.
 *
 * \param[in] state Pointer to game state.
 */
void action_bribe(tgestate_t *state)
{
  vischar_t *vischar;   /* was HL */
  uint8_t    iters;     /* was B */
  uint8_t    character; /* was A */

  assert(state != NULL);

  /* Walk non-player visible characters. */
  vischar = &state->vischars[1];
  iters = vischars_LENGTH - 1;
  do
  {
    character = vischar->character;
    if (character != character_NONE && character >= character_20_PRISONER_1)
      goto found;
    vischar++;
  }
  while (--iters);

  return;

found:
  state->bribed_character = character;
  vischar->flags = vischar_FLAGS_BRIBE_PENDING;
}

/* ----------------------------------------------------------------------- */

/**
 * $B3C4: Use poison.
 *
 * \param[in] state Pointer to game state.
 */
void action_poison(tgestate_t *state)
{
  assert(state != NULL);

  if (state->items_held[0] != item_FOOD &&
      state->items_held[1] != item_FOOD)
    return; /* Have no poison. */

  if (state->item_structs[item_FOOD].item_and_flags & itemstruct_ITEM_FLAG_POISONED)
    return; /* Food is already poisoned. */

  state->item_structs[item_FOOD].item_and_flags |= itemstruct_ITEM_FLAG_POISONED;

  state->item_attributes[item_FOOD] = attribute_BRIGHT_PURPLE_OVER_BLACK;

  draw_all_items(state);

  increase_morale_by_10_score_by_50(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B3E1: Use uniform.
 *
 * \param[in] state Pointer to game state.
 */
void action_uniform(tgestate_t *state)
{
  const sprite_t *sprite; /* was HL */

  assert(state != NULL);

  sprite = &sprites[sprite_GUARD_FACING_TOP_LEFT_4];

  if (state->vischars[0].mi.spriteset == sprite)
    return; /* Already in uniform. */

  if (state->room_index >= room_29_SECOND_TUNNEL_START)
    return; /* Can't don uniform when in a tunnel. */

  state->vischars[0].mi.spriteset = sprite;

  increase_morale_by_10_score_by_50(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B3F6: Use shovel.
 *
 * \param[in] state Pointer to game state.
 */
void action_shovel(tgestate_t *state)
{
  assert(state != NULL);

  if (state->room_index != room_50_BLOCKED_TUNNEL)
    return; /* Shovel only works in the blocked tunnel room. */

  if (roomdef_50_blocked_tunnel[2] == 255)
    return; /* Blockage is already cleared. */

  /* Release boundary. */
  roomdef_50_blocked_tunnel[2] = 255;
  /* Remove blockage graphic. */
  roomdef_50_blocked_tunnel[roomdef_50_BLOCKAGE] = interiorobject_TUNNEL_0;

  setup_room(state);
  choose_game_window_attributes(state);
  plot_interior_tiles(state);
  increase_morale_by_10_score_by_50(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B416: Use wiresnips.
 *
 * \param[in] state Pointer to game state.
 */
void action_wiresnips(tgestate_t *state)
{
  const wall_t    *wall;  /* was HL */
  const tinypos_t *pos;   /* was DE */
  uint8_t          iters; /* was B */
  uint8_t          flag;  /* was A */

  assert(state != NULL);

  wall = &walls[12]; /* == .d; - start of fences */
  pos = &state->hero_map_position;
  iters = 4;
  do
  {
    uint8_t coord; /* was A */

    // check: this is using x then y which is the wrong order, isn't it?
    coord = pos->y;
    if (coord >= wall->miny && coord < wall->maxy) /* Conv: Reversed test order. */
    {
      coord = pos->x;
      if (coord == wall->maxx)
        goto set_to_4;
      if (coord - 1 == wall->maxx)
        goto set_to_6;
    }

    wall++;
  }
  while (--iters);

  wall = &walls[12 + 4]; // .a
  pos = &state->hero_map_position;
  iters = 3;
  do
  {
    uint8_t coord; /* was A */

    coord = pos->x;
    if (coord >= wall->minx && coord < wall->maxx)
    {
      coord = pos->y;
      if (coord == wall->miny)
        goto set_to_5;
      if (coord - 1 == wall->miny)
        goto set_to_7;
    }

    wall++;
  }
  while (--iters);

  return;

set_to_4: /* Crawl TL */
  flag = 4;
  goto action_wiresnips_tail;

set_to_5: /* Crawl TR. */
  flag = 5;
  goto action_wiresnips_tail;

set_to_6: /* Crawl BR. */
  flag = 6;
  goto action_wiresnips_tail;

set_to_7: /* Crawl BL. */
  flag = 7;

action_wiresnips_tail:
  state->vischars[0].b0E            = flag; // walk/crawl flag
  state->vischars[0].b0D            = vischar_BYTE13_BIT7;
  state->vischars[0].flags          = vischar_FLAGS_CUTTING_WIRE;
  state->vischars[0].mi.pos.height  = 12;
  state->vischars[0].mi.spriteset   = &sprites[sprite_PRISONER_FACING_TOP_LEFT_4];
  state->player_locked_out_until = state->game_counter + 96;
  queue_message_for_display(state, message_CUTTING_THE_WIRE);
}

/* ----------------------------------------------------------------------- */

/**
 * $B495: Use lockpick.
 *
 * \param[in] state Pointer to game state.
 */
void action_lockpick(tgestate_t *state)
{
  void *HL; // FIXME: we don't yet know what type open_door() returns

  assert(state != NULL);

  HL = open_door(state);
  if (HL == NULL)
    return; /* Wrong door? */

  state->ptr_to_door_being_lockpicked = HL;
  state->player_locked_out_until = state->game_counter + 255;
  state->vischars[0].flags = vischar_FLAGS_PICKING_LOCK;
  queue_message_for_display(state, message_PICKING_THE_LOCK);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4AE: Use red key.
 *
 * \param[in] state Pointer to game state.
 */
void action_red_key(tgestate_t *state)
{
  assert(state != NULL);

  action_key(state, room_22_REDKEY);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4B2: Use yellow key.
 *
 * \param[in] state Pointer to game state.
 */
void action_yellow_key(tgestate_t *state)
{
  assert(state != NULL);

 action_key(state, room_13_CORRIDOR);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4B6: Use green key.
 *
 * \param[in] state Pointer to game state.
 */
void action_green_key(tgestate_t *state)
{
  assert(state != NULL);

  action_key(state, room_14_TORCH);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4B8: Use key (common).
 *
 * \param[in] state Pointer to game state.
 * \param[in] room  Room number. (was A)
 */
void action_key(tgestate_t *state, room_t room)
{
  uint8_t  *HL; // FIXME: we don't yet know what type open_door() returns
  uint8_t   flags;   /* was A */
  message_t message; /* was B */

  assert(state != NULL);
  assert(room >= 0 && room < room__LIMIT);

  HL = open_door(state);
  if (HL == NULL)
    return; /* Wrong door? */

  flags = *HL & ~gates_and_doors_LOCKED; // mask off locked flag
  if (flags != room)
  {
    message = message_INCORRECT_KEY;
  }
  else
  {
    *HL &= ~gates_and_doors_LOCKED; // clear the locked flag
    increase_morale_by_10_score_by_50(state);
    message = message_IT_IS_OPEN;
  }

  queue_message_for_display(state, message);
  (void) open_door(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4D0: Open door.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Pointer to HL? FIXME: Determine its type.
 */
void *open_door(tgestate_t *state)
{
  uint8_t         *gate;    /* was HL */
  uint8_t          iters;   /* was B */
  const doorpos_t *doorpos; /* was HL' */
  uint8_t          C;       /* was C */
  uint8_t         *DE;      /* was DE */
  pos_t           *pos;     /* was DE' */

  assert(state != NULL);

  if (state->room_index > room_0_OUTDOORS)
  {
    /* Outdoors. */

    gate = &state->gates_and_doors[0]; // gate flags
    iters = 5; // gate and door ranges must overlap
    do
    {
      doorpos = get_door_position(*gate & ~gates_and_doors_LOCKED); // Conv: A used as temporary.
      if (door_in_range(state, doorpos + 0) == 0 ||
          door_in_range(state, doorpos + 1) == 0)
        return NULL; /* Conv: goto removed. */

      gate++;
    }
    while (--iters);

    return NULL;
  }
  else
  {
    /* Indoors. */

    gate = &state->gates_and_doors[2]; // door flags
    iters = 8;
    do
    {
      C = *gate & ~gates_and_doors_LOCKED;

      /* Search door_related for C. */
      DE = &state->door_related[0];
      for (;;)
      {
        uint8_t A;

        A = *DE;
        if (A != 0xFF)
        {
          if ((A & gates_and_doors_MASK) == C)
            goto found;
          DE++;
        }
      }
next:
      gate++;
    }
    while (--iters);

    return NULL; // temporary, should do something else return 1;

found:
    doorpos = get_door_position(*DE);
    /* Range check pattern (-2..+3). */
    pos = &state->saved_pos; // note: 16-bit values holding 8-bit values
    // Conv: Unrolled.
    if (pos->x <= doorpos->pos.x - 3 || pos->x > doorpos->pos.x + 3 ||
        pos->y <= doorpos->pos.y - 3 || pos->y > doorpos->pos.y + 3)
      goto next;

    return NULL;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B53E: Walls & $B586: Fences
 *
 * Used by bounds_check and action_wiresnips.
 */
const wall_t walls[24] =
{
  { 0x6A, 0x6E, 0x52, 0x62, 0, 11 },
  { 0x5E, 0x62, 0x52, 0x62, 0, 11 },
  { 0x52, 0x56, 0x52, 0x62, 0, 11 },
  { 0x3E, 0x5A, 0x6A, 0x80, 0, 48 }, // 48 is high... main gate?
  { 0x34, 0x80, 0x72, 0x80, 0, 48 },
  { 0x7E, 0x98, 0x5E, 0x80, 0, 48 },
  { 0x82, 0x98, 0x5A, 0x80, 0, 48 },
  { 0x86, 0x8C, 0x46, 0x80, 0, 10 },
  { 0x82, 0x86, 0x46, 0x4A, 0, 18 },
  { 0x6E, 0x82, 0x46, 0x47, 0, 10 },
  { 0x6D, 0x6F, 0x45, 0x49, 0, 18 },
  { 0x67, 0x69, 0x45, 0x49, 0, 18 },
  { 0x46, 0x46, 0x46, 0x6A, 0,  8 }, // 12th
  { 0x3E, 0x3E, 0x3E, 0x6A, 0,  8 },
  { 0x4E, 0x4E, 0x2E, 0x3E, 0,  8 },
  { 0x68, 0x68, 0x2E, 0x45, 0,  8 },
  { 0x3E, 0x68, 0x3E, 0x3E, 0,  8 },
  { 0x4E, 0x68, 0x2E, 0x2E, 0,  8 },
  { 0x46, 0x67, 0x46, 0x46, 0,  8 },
  { 0x68, 0x6A, 0x38, 0x3A, 0,  8 },
  { 0x4E, 0x50, 0x2E, 0x30, 0,  8 },
  { 0x46, 0x48, 0x46, 0x48, 0,  8 },
  { 0x46, 0x48, 0x5E, 0x60, 0,  8 },
  { 0x69, 0x6D, 0x46, 0x49, 0,  8 },
};

/* ----------------------------------------------------------------------- */

/**
 * $B5CE: called_from_main_loop_9
 *
 * - movement
 * - crash if disabled, crash if iters reduced from 8
 *
 * \param[in] state Pointer to game state.
 */
void called_from_main_loop_9(tgestate_t *state)
{
  /**
   * $CDAA: (unknown)  likely: sprite offsets + flip flags
   */
  static const uint8_t byte_CDAA[8][9] =
  {
    { 0x08,0x00,0x04,0x87,0x00,0x87,0x04,0x04,0x04 },
    { 0x09,0x84,0x05,0x05,0x84,0x05,0x01,0x01,0x05 },
    { 0x0A,0x85,0x02,0x06,0x85,0x06,0x85,0x85,0x02 },
    { 0x0B,0x07,0x86,0x03,0x07,0x03,0x07,0x07,0x86 },
    { 0x14,0x0C,0x8C,0x93,0x0C,0x93,0x10,0x10,0x8C },
    { 0x15,0x90,0x11,0x8D,0x90,0x95,0x0D,0x0D,0x11 },
    { 0x16,0x8E,0x0E,0x12,0x8E,0x0E,0x91,0x91,0x0E },
    { 0x17,0x13,0x92,0x0F,0x13,0x0F,0x8F,0x8F,0x92 },
  };

  uint8_t        iters;   /* was B */
  vischar_t     *vischar; /* was IY */
  const uint8_t *HL;      /* was HL */
  uint8_t        A;       /* was A */
  uint8_t        C;       /* was C */
  const uint8_t *DE;      /* was HL */
  uint16_t       BC;      /* was BC */
  uint8_t        Adash;   /* was A' */ // flip flag
  uint16_t       HL2;     /* was HL */

  assert(state != NULL);

  iters   = vischars_LENGTH;
  vischar = &state->vischars[0];
  state->IY = &state->vischars[0];
  do
  {
    if (vischar->flags == vischar_FLAGS_EMPTY_SLOT)
      goto next;

    vischar->flags |= vischar_FLAGS_BIT7; // mark the slot as used?

    if (vischar->b0D & vischar_BYTE13_BIT7) // 'kick' flag?
      goto byte13bit7set;

    HL = vischar->w0A; // character_related_pointer
    A = vischar->b0C;
    if (A >= 128) // signed byte < 0
    {
      A &= vischar_BYTE12_MASK;
      if (A == 0)
        goto snozzle;

      HL += (A + 1) * 4 - 1; /* 4..512 + 1 */
      A = *HL++;

      SWAP(uint8_t, A, Adash);

// Conv: Simplified sign extend sequence to this. Too far?
#define SXT_8_16(P) ((uint16_t) (*(int8_t *) (P)))

resume1:
      SWAP(const uint8_t *, DE, HL);

      // sampled DE = $CF9A, $CF9E, $CFBE, $CFC2, $CFB2, $CFB6, $CFA6, $CFAA (character_related_data)

      BC = SXT_8_16(DE);
      state->saved_pos.x = vischar->mi.pos.x - BC;
      DE++;

      BC = SXT_8_16(DE);
      state->saved_pos.y = vischar->mi.pos.y - BC;
      DE++;

      BC = SXT_8_16(DE);
      state->saved_pos.height = vischar->mi.pos.height - BC;

      if (touch(state, vischar, Adash))
        goto pop_next;

      vischar->b0C--;
    }
    else
    {
      if (A == *HL)
        goto snozzle;

      HL += (A + 1) * 4;

resume2:
      SWAP(const uint8_t *, DE, HL);

      HL2 = SXT_8_16(DE);
      state->saved_pos.x = HL2 + vischar->mi.pos.x;
      DE++;

      HL2 = SXT_8_16(DE);
      state->saved_pos.y = HL2 + vischar->mi.pos.y;
      DE++;

      HL2 = SXT_8_16(DE);
      state->saved_pos.height = HL2 + vischar->mi.pos.height;
      DE++;

      A = *DE; // unsure

      SWAP(uint8_t, A, Adash);

      if (touch(state, vischar, Adash))
        goto pop_next;

      vischar->b0C++;
    }

    // HL = vischar;

    reset_position_end(state, vischar);

pop_next:
    if (vischar->flags != vischar_FLAGS_EMPTY_SLOT)
      vischar->flags &= ~vischar_FLAGS_BIT7; // $8001

next:
    vischar++;
  }
  while (--iters);

  return;


byte13bit7set:
  vischar->b0D &= ~vischar_BYTE13_BIT7; // sampled IY = $8020, $80A0, $8060, $80E0, $8080,

snozzle:
  C = byte_CDAA[vischar->b0E][vischar->b0D];
  DE = vischar->w08[C];
  vischar->w0A = DE;
  if ((C & (1 << 7)) == 0)
  {
    vischar->b0C = 0;
    DE += 2;
    vischar->b0E = *DE;
    DE += 2;
    SWAP(const uint8_t *, DE, HL);
    goto resume2;
  }
  else
  {
    const uint8_t *stacked;

    C = *DE;
    vischar->b0C = C | 0x80;
    vischar->b0E = *++DE;
    DE += 3;
    stacked = DE;
    SWAP(const uint8_t *, DE, HL);
    HL += C * 4 - 1;
    A = *HL;
    SWAP(uint8_t, A, Adash);
    HL = stacked;
    goto resume1;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B71B: Reset position.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was HL)
 */
void reset_position(tgestate_t *state, vischar_t *vischar)
{
  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  /* Save a copy of the vischar's position + offset. */
  memcpy(&state->saved_pos, &vischar->mi.pos, sizeof(pos_t));

  reset_position_end(state, vischar);
}

/**
 * $B729: Reset position (end bit).
 *
 * This looks very much like it computes a screen position.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was HL)
 */
void reset_position_end(tgestate_t *state, vischar_t *vischar)
{
  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  vischar->scrx = (state->saved_pos.y + 0x0200 - state->saved_pos.x) * 2;
  vischar->scry = 0x0800 - state->saved_pos.x - state->saved_pos.height - state->saved_pos.y;
}

/* ----------------------------------------------------------------------- */

/**
 * $B75A: Reset the game.
 *
 * \param[in] state Pointer to game state.
 */
void reset_game(tgestate_t *state)
{
  int    iters; /* was B */
  item_t item;  /* was C */

  assert(state != NULL);

  /* Cause discovery of all items. */
  iters = item__LIMIT;
  item = 0;
  do
    item_discovered(state, item++);
  while (--iters);

  /* Reset message queue. */
  state->messages.queue_pointer = &state->messages.queue[2];
  reset_map_and_characters(state);
  state->vischars[0].flags = 0;

  /* Reset score. */
  memset(&state->score_digits[0], 0, 5);

  /* Reset morale. */
  state->morale = morale_MAX;
  plot_score(state);

  /* Reset items. */
  state->items_held[0] = item_NONE;
  state->items_held[1] = item_NONE;
  draw_all_items(state);

  /* Reset the hero's sprite. */
  state->vischars[0].mi.spriteset = &sprites[sprite_PRISONER_FACING_TOP_LEFT_4];

  /* Put the hero to bed. */
  state->room_index = room_2_HUT2LEFT;
  hero_sleeps(state);

  enter_room(state); // returns by goto main_loop
  NEVER_RETURNS;
}

/* ----------------------------------------------------------------------- */

/**
 * $B79B: Resets all visible characters, clock, day_or_night flag, general
 * flags, collapsed tunnel objects, locks the gates, resets all beds, clears
 * the mess halls and resets characters.
 *
 * \param[in] state Pointer to game state.
 */
void reset_map_and_characters(tgestate_t *state)
{
  typedef struct character_reset_partial
  {
    room_t  room;
    uint8_t x, y; // partial of a tinypos_t
  }
  character_reset_partial_t;

  /**
   * $B819: Partial character struct for reset
   */
  static const character_reset_partial_t character_reset_data[10] =
  {
    { room_3_HUT2RIGHT, 40, 60 }, /* for character 12 Guard */
    { room_3_HUT2RIGHT, 36, 48 }, /* for character 13 Guard */
    { room_5_HUT3RIGHT, 40, 60 }, /* for character 14 Guard */
    { room_5_HUT3RIGHT, 36, 34 }, /* for character 15 Guard */
    { room_NONE,        52, 60 }, /* for character 20 Prisoner */
    { room_NONE,        52, 44 }, /* for character 21 Prisoner */
    { room_NONE,        52, 28 }, /* for character 22 Prisoner */
    { room_NONE,        52, 60 }, /* for character 23 Prisoner */
    { room_NONE,        52, 44 }, /* for character 24 Prisoner */
    { room_NONE,        52, 28 }, /* for character 25 Prisoner */
  };

  uint8_t                          iters;   /* was B */
  vischar_t                       *vischar; /* was HL */
  uint8_t                         *gate;    /* was HL */
  uint8_t *const                  *bed;     /* was HL */
  characterstruct_t               *charstr; /* was DE */
  uint8_t                          iters2;  /* was C */
  const character_reset_partial_t *reset;   /* was HL */

  assert(state != NULL);

  iters = vischars_LENGTH - 1;
  vischar = &state->vischars[1]; /* iterate over non-player characters */
  do
    reset_visible_character(state, vischar++);
  while (--iters);

  state->clock = 7;
  state->day_or_night = 0;
  state->vischars[0].flags = 0;
  roomdef_50_blocked_tunnel[roomdef_50_BLOCKAGE] = interiorobject_COLLAPSED_TUNNEL;
  roomdef_50_blocked_tunnel[2] = 0x34; /* Reset boundary. */

  /* Lock the gates. */
  gate = &state->gates_and_doors[0];
  iters = 9;
  do
    *gate++ |= gates_and_doors_LOCKED;
  while (--iters);

  /* Reset all beds. */
  iters = beds_LENGTH;
  bed = &beds[0];
  do
    **bed = interiorobject_OCCUPIED_BED;
  while (--iters);

  /* Clear the mess halls. */
  roomdef_23_breakfast[roomdef_23_BENCH_A] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_B] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_C] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_D] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_E] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_F] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_EMPTY_BENCH;

  /* Reset characters 12..15 (guards) and 20..25 (prisoners). */
  charstr = &state->character_structs[character_12_GUARD_12];
  iters2 = NELEMS(character_reset_data);
  reset = &character_reset_data[0];
  do
  {
    charstr->room       = reset->room;
    charstr->pos.x      = reset->x;
    charstr->pos.y      = reset->y;
    charstr->pos.height = 18; /* Bug/Odd: This is reset to 18 but the initial data is 24. */
    charstr->target &= 0xFF00; /* clear bottom byte */
    charstr++;
    reset++;

    if (iters2 == 7) /* when 7 remain */
      charstr = &state->character_structs[character_20_PRISONER_1];
  }
  while (--iters2);
}

/* ----------------------------------------------------------------------- */

/**
 * $B83B searchlight_mask_test
 *
 * Looks like it's testing mask_buffer to find the hero.
 * Perhaps this lets the hero hide in masked areas.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void searchlight_mask_test(tgestate_t *state, vischar_t *vischar)
{
  uint8_t     *buf;   /* was HL */
  attribute_t  attrs; /* was A */
  uint8_t      iters; /* was B */
  //uint8_t      C;     /* was C */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  if (vischar > &state->vischars[0])
    return; /* Skip non-hero characters. */

  buf = &state->mask_buffer[0x31]; // 0x31 = 32 + 17 ... but unsure if 32 is the right rowsize // could be 12*4+1
  iters = 8;
  // C = 4; /* Conv/Bug: Original code does a fused load of BC, but doesn't seem to use C here. Probably a leftover stride constant. */
  do
  {
    if (*buf != 0)
      goto still_in_spotlight;
    buf += 4; // stride is 4?
  }
  while (--iters);

  /* Otherwise the hero has escaped the spotlight, so decrement the counter. */
  if (--state->searchlight_state == searchlight_STATE_OFF)
  {
    attrs = choose_game_window_attributes(state);
    set_game_window_attributes(state, attrs);
  }

  return;

still_in_spotlight:
  state->searchlight_state = searchlight_STATE_CAUGHT;
}

/* ----------------------------------------------------------------------- */

/**
 * $B866: Seems to locate things to plot, then invoke masked sprite plotter
 * on those things.
 *
 * \param[in] state Pointer to game state.
 */
void locate_vischar_or_itemstruct_then_plot(tgestate_t *state)
{
  uint8_t       index;      /* was A */
  vischar_t    *vischar;    /* was IY */
  itemstruct_t *itemstruct; /* was IY */
  int           found;      /* was Z */

  assert(state != NULL);

  for (;;)
  {
    /* This can return a vischar OR an itemstruct, but not both. */
    found = locate_vischar_or_itemstruct(state, &index, &vischar, &itemstruct);
    if (!found)
      return;

#ifndef NDEBUG
    if (vischar)
      ASSERT_VISCHAR_VALID(vischar);
    else
      assert(itemstruct != NULL);
#endif

    if ((index & (1 << 6)) == 0) // mysteryflagconst874 'item found' flag
    {
      found = setup_vischar_plotting(state, vischar);
      if (found)
      {
        mask_stuff(state);
        if (state->searchlight_state != searchlight_STATE_OFF)
          searchlight_mask_test(state, vischar);
        if (vischar->width_bytes != 3)
          masked_sprite_plotter_24_wide(state, vischar);
        else
          masked_sprite_plotter_16_wide(state, vischar);
      }
    }
    else
    {
      found = setup_item_plotting(state, itemstruct, index);
      if (found)
      {
        mask_stuff(state);
        masked_sprite_plotter_16_wide_searchlight(state);
      }
    }
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B89C: Locates a vischar or item to plot.
 *
 * Only called from searchlight().
 *
 * \param[in]  state       Pointer to game state.
 * \param[out] pindex      Pointer to receive ? (was A) //  Returns an item | (1 << 6), or something else. // If vischar, returns vischars_LENGTH - iters; if itemstruct, returns (item__LIMIT - iters) | (1 << 6)
 * \param[out] pvischar    Pointer to receive vischar pointer. (was IY)
 * \param[out] pitemstruct Pointer to receive itemstruct pointer. (was IY)
 *
 * \return State of Z flag: 1 if Z, 0 if NZ. // unsure which is found/notfound right now
 */
int locate_vischar_or_itemstruct(tgestate_t    *state,
                                 uint8_t       *pindex,
                                 vischar_t    **pvischar,
                                 itemstruct_t **pitemstruct)
{
  uint16_t      x;                /* was BC */
  uint16_t      y;                /* was DE */
  item_t        item_and_flag;    /* was A' */
  uint16_t      DEdash;           /* was DEdash */
  uint8_t       iters;            /* was Bdash */
  vischar_t    *vischar;          /* was HLdash */
  uint16_t      BCdash;           /* was BCdash */
  vischar_t    *found_vischar;    /* was IY */
  itemstruct_t *found_itemstruct; /* was IY */

  assert(state       != NULL);
  assert(pindex      != NULL);
  assert(pvischar    != NULL);
  assert(pitemstruct != NULL);

  *pvischar    = NULL;
  *pitemstruct = NULL;

  x             = 0;    // prev-x
  y             = 0;    // prev-y
  item_and_flag = 0xFF; // 'nothing found' marker // item
  DEdash        = 0;

  iters   = vischars_LENGTH; /* iterations */
  vischar = &state->vischars[0]; /* Conv: Original points to $8007. */
  do
  {
    if ((vischar->b07 & vischar_BYTE7_BIT7) == 0)
      goto next;

    if ((vischar->mi.pos.x + 4 < x) ||
        (vischar->mi.pos.y + 4 < y))
      goto next;

    BCdash = iters; // assigned but not ever used again?
    item_and_flag = vischars_LENGTH - iters; // item
    DEdash = vischar->mi.pos.height;

    y = vischar->mi.pos.y; // y,x order is correct
    x = vischar->mi.pos.x;
    found_vischar = vischar; // needs looking at again

    state->IY = vischar; // TODO: Work out all the other places which assign IY.

next:
    vischar++;
  }
  while (--iters);

  // IY is returned from this, but is an itemstruct_t not a vischar
  // Adash only ever has a flag ORed in, nothing is cleared
  item_and_flag = get_greatest_itemstruct(state,
                                          item_and_flag,
                                          x,
                                          y,
                                          &found_itemstruct);
  *pitemstruct = found_itemstruct;
  *pindex = item_and_flag;
  /* If Adash's top bit remains set from initialisation, then no vischar was
   * found. (It's not affected by get_greatest_itemstruct which only ORs). */
  if (item_and_flag & (1 << 7))
    return 0; // NZ => not found

  if ((item_and_flag & (1 << 6)) == 0) // mysteryflagconst874 'item found' flag?
  {
    found_vischar->b07 &= ~vischar_BYTE7_BIT7;

    *pvischar = found_vischar; // Conv: Additional code.

    return 1; // Z => found
  }
  else
  {
    int Z;

    found_itemstruct->room_and_flags &= ~itemstruct_ROOM_FLAG_NEARBY_6;
    Z = (found_itemstruct->room_and_flags & (1 << 6)) == 0; // was BIT 6,HL[1] // this tests the bit we've just cleared

    *pitemstruct = found_itemstruct; // Conv: Additional code.

    return Z;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B916: Suspected mask stuff.
 *
 * \param[in] state Pointer to game state.
 */
void mask_stuff(tgestate_t *state)
{
  /* Mask encoding: A top-bit-set byte indicates a repetition, the count of
   * which is in the bottom seven bits. The subsequent byte is the value to
   * repeat. */

#define _____ /* A spacer for laying out tables. */

  /* $E55F */
  static const uint8_t exterior_mask_0[] =
  {
    0x2A,
    0xA0, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x07, 0x08, 0x09, 0x01, 0x0A, 0xA2, 0x00, _____ _____
    _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x85, 0x01, _____ _____ _____ 0x0B, 0x9F, 0x00, _____
    _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x88, 0x01, _____ _____ _____ _____ _____ _____ 0x0C, 0x9C, 0x00,
    _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x8A, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ 0x0D, 0x0E, 0x99,
    0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x8D, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x0F, 0x10,
    0x96, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x90, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x11,
    0x94, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x92, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x92, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x94, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x90, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x96, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x8E, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x8C, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x9A, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x8A, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x9C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x88, 0x00, _____ _____ _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x9E, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x18,
    0x86, 0x00, _____ _____ _____ _____ 0x05, 0x06, 0x04, 0xA1, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x84, 0x00, _____ _____ 0x05, 0x06, 0x04, 0xA3, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x00, 0x00, 0x05, 0x06, 0x04, 0xA5, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x05, 0x03, 0x04, 0xA7, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0xA9, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0xA9, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0xA9, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0xA9, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0xA9, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0xA9, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0xA9, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0xA9, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0xA9, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
  };

  /* $E5FF */
  static const uint8_t exterior_mask_1[] =
  {
    0x12,
    0x02, 0x91, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x91, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x91, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x91, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x91, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x91, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x91, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x91, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x91, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x91, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
  };

  /* $E61E */
  static const uint8_t exterior_mask_2[] =
  {
    0x10,
    0x13, 0x14, 0x15, 0x8D, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x16, 0x17, 0x18, 0x17, 0x15, 0x8B, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x19, 0x1A, 0x1B, 0x17, 0x18, 0x17, 0x15, 0x89, 0x00, _____ _____ _____ _____ _____ _____ _____
    0x19, 0x1A, 0x1C, 0x1A, 0x1B, 0x17, 0x18, 0x17, 0x15, 0x87, 0x00, _____ _____ _____ _____ _____
    0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1B, 0x17, 0x13, 0x14, 0x15, 0x85, 0x00, _____ _____ _____
    0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x16, 0x17, 0x18, 0x17, 0x15, 0x83, _____ _____
    0x00, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1B, 0x17, 0x18, 0x17, 0x15,
    0x00, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1A, 0x1B, 0x17, 0x18,
    0x17, 0x00, 0x20, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1B,
    0x17, 0x83, 0x00, _____ 0x20, 0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C,
    0x1D, 0x85, 0x00, _____ _____ _____ 0x20, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C,
    0x1D, 0x87, 0x00, _____ _____ _____ _____ _____ 0x1F, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C,
    0x1D, 0x89, 0x00, _____ _____ _____ _____ _____ _____ _____ 0x20, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C,
    0x1D, 0x8B, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x20, 0x1C, 0x1A, 0x1C,
    0x1D, 0x8D, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x20, 0x1C,
    0x1D, 0x8F, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x1F
  };

  /* $E6CA */
  static const uint8_t exterior_mask_3[] =
  {
    0x1A,
    0x88, 0x00, _____ _____ _____ _____ _____ _____ 0x05, 0x4C, 0x90, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x86, 0x00, _____ _____ _____ _____ 0x05, 0x06, 0x04, 0x32, 0x30, 0x4C, 0x8E, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x84, 0x00, _____ _____ 0x05, 0x06, 0x04, 0x84, 0x01, _____ _____ 0x32, 0x30, 0x4C, 0x8C, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x00, 0x00, 0x05, 0x06, 0x04, 0x88, 0x01, _____ _____ _____ _____ _____ _____ 0x32, 0x30, 0x4C, 0x8A, 0x00, _____ _____ _____ _____ _____ _____ _____ _____
    0x00, 0x06, 0x04, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x32, 0x30, 0x4C, 0x88, 0x00, _____ _____ _____ _____ _____ _____
    0x02, 0x90, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x32, 0x30, 0x4C, 0x86, 0x00, 0x02, _____ _____ _____
    0x92, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x32, 0x30, 0x4C, 0x84, 0x00, _____ _____
    0x02, 0x94, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x32, 0x30, 0x4C, 0x00, 0x00,
    0x02, 0x96, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x32, 0x30, 0x00,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x98, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12
  };

  /* $E74B */
  static const uint8_t exterior_mask_4[] =
  {
    0x0D,
    0x02, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x8C, 0x01  _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
  };

  /* $E758 */
  static const uint8_t exterior_mask_5[] =
  {
    0x0E,
    0x02, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x8C, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x12,
    0x02, 0x8D, 0x01, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x02, 0x8D, 0x01  _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
  };

  /* $E77F */
  static const uint8_t exterior_mask_6[] =
  {
    0x08,
    0x5B, 0x5A, 0x86, 0x00, _____ _____ _____ _____ 
    0x01, 0x01, 0x5B, 0x5A, 0x84, 0x00, _____ _____
    0x84, 0x01, _____ _____ 0x5B, 0x5A, 0x00, 0x00,
    0x86, 0x01, _____ _____ _____ _____ 0x5B, 0x5A,
    0xD8, 0x01  _____ _____ _____ _____ _____ _____
  };

  /* $E796 */
  static const uint8_t exterior_mask_7[] =
  {
    0x09,
    0x88, 0x01, _____ _____ _____ _____ _____ _____ 0x12,
    0x88, 0x01, _____ _____ _____ _____ _____ _____ 0x12,
    0x88, 0x01, _____ _____ _____ _____ _____ _____ 0x12,
    0x88, 0x01, _____ _____ _____ _____ _____ _____ 0x12,
    0x88, 0x01, _____ _____ _____ _____ _____ _____ 0x12,
    0x88, 0x01, _____ _____ _____ _____ _____ _____ 0x12,
    0x88, 0x01, _____ _____ _____ _____ _____ _____ 0x12,
    0x88, 0x01, _____ _____ _____ _____ _____ _____ 0x12
  };

  /* $E7AF */
  static const uint8_t exterior_mask_8[] =
  {
    0x10,
    0x8D, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x23, 0x24, 0x25,
    0x8B, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x23, 0x26, 0x27, 0x26, 0x28,
    0x89, 0x00, _____ _____ _____ _____ _____ _____ _____ 0x23, 0x26, 0x27, 0x26, 0x22, 0x29, 0x2A,
    0x87, 0x00, _____ _____ _____ _____ _____ 0x23, 0x26, 0x27, 0x26, 0x22, 0x29, 0x2B, 0x29, 0x2A,
    0x85, 0x00, _____ _____ _____ 0x23, 0x24, 0x25, 0x26, 0x22, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A,
    0x83, 0x00, 0x23, 0x26, 0x27, 0x26, 0x28, 0x2F, 0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x00,
    0x23, 0x26, 0x27, 0x26, 0x22, 0x29, 0x2A, 0x2F, 0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x26,
    0x27, 0x26, 0x22, 0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x26,
    0x22, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x31, 0x2D, 0x2F,
    0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x29, 0x2B, 0x31, 0x83, 0x00, _____ 0x2F,
    0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x31, 0x85, 0x00, _____ _____ _____ 0x2F,
    0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x2E, 0x87, 0x00, _____ _____ _____ _____ _____ 0x2F,
    0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x31, 0x2D, 0x88, 0x00, _____ _____ _____ _____ _____ _____ 0x2F,
    0x2B, 0x29, 0x2B, 0x31, 0x8B, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ 0x2F,
    0x2B, 0x31, 0x8D, 0x00, _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
    0x2E, 0x8F, 0x00  _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____ _____
  };

  /* $E85C */
  static const uint8_t exterior_mask_9[] =
  {
    0x0A,
    0x83, 0x00, _____ 0x05, 0x06, 0x30, 0x4C, 0x83, 0x00, _____
    0x00, 0x05, 0x06, 0x04, 0x01, 0x01, 0x32, 0x30, 0x4C, 0x00,
    0x34, 0x04, 0x86, 0x01, _____ _____ _____ _____ 0x32, 0x33,
    0x83, 0x00, _____ 0x40, 0x01, 0x01, 0x3F, 0x83, 0x00, _____
    0x02, 0x46, 0x47, 0x48, 0x49, 0x42, 0x41, 0x45, 0x44, 0x12,
    0x34, 0x01, 0x01, 0x46, 0x4B, 0x43, 0x44, 0x01, 0x01, 0x33,
    0x00, 0x3C, 0x3E, 0x40, 0x01, 0x01, 0x3F, 0x37, 0x39, 0x00,
    0x83, 0x00, _____ 0x3D, 0x3A, 0x3B, 0x38, 0x83, 0x00  _____
  };

  /* $E8A3 */
  static const uint8_t exterior_mask_10[] =
  {
    0x08,
    0x35, 0x86, 0x01, _____ _____ _____ _____ 0x36,
    0x90, 0x01, _____ _____ _____ _____ _____ _____
    _____ _____ _____ _____ _____ _____ _____ _____
    0x88, 0x00, _____ _____ _____ _____ _____ _____
    0x3C, 0x86, 0x00, _____ _____ _____ _____ 0x39,
    0x3C, 0x00, 0x02, 0x36, 0x35, 0x12, 0x00, 0x39,
    0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
    0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
    0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
    0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
    0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
    0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39,
    0x3C, 0x00, 0x02, 0x01, 0x01, 0x12, 0x00, 0x39
  };

  /* $E8F0 */
  static const uint8_t exterior_mask_11[] =
  {
    0x08,
    0x01, 0x4F, 0x86, 0x00, _____ _____ _____ _____
    0x01, 0x50, 0x01, 0x4F, 0x84, 0x00, _____ _____
    0x01, 0x00, 0x00, 0x51, 0x01, 0x4F, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x53, 0x19, 0x50, 0x01, 0x4F,
    0x01, 0x00, 0x00, 0x53, 0x19, 0x00, 0x00, 0x52,
    0x01, 0x00, 0x00, 0x53, 0x19, 0x00, 0x00, 0x52,
    0x01, 0x54, 0x00, 0x53, 0x19, 0x00, 0x00, 0x52,
    0x83, 0x00, _____ 0x55, 0x19, 0x00, 0x00, 0x52,
    0x85, 0x00, _____ _____ _____ 0x54, 0x00, 0x52
  };

  /* $E92F */
  static const uint8_t exterior_mask_12[] =
  {
    0x02,
    0x56, 0x57,
    0x56, 0x57,
    0x58, 0x59,
    0x58, 0x59,
    0x58, 0x59,
    0x58, 0x59,
    0x58, 0x59,
    0x58, 0x59
  };

  /* $E940 */
  static const uint8_t exterior_mask_13[] =
  {
    0x05,
    0x00, 0x00, 0x23, 0x24, 0x25,
    0x02, 0x00, 0x27, 0x26, 0x28,
    0x02, 0x00, 0x22, 0x26, 0x28,
    0x02, 0x00, 0x2B, 0x29, 0x2A,
    0x02, 0x00, 0x2B, 0x29, 0x2A,
    0x02, 0x00, 0x2B, 0x29, 0x2A,
    0x02, 0x00, 0x2B, 0x29, 0x2A,
    0x02, 0x00, 0x2B, 0x29, 0x2A,
    0x02, 0x00, 0x2B, 0x31, 0x00,
    0x02, 0x00, 0x83, 0x00  _____
  };

  /* $E972 */
  static const uint8_t exterior_mask_14[] =
  {
    0x04,
    0x19, 0x83, 0x00, _____
    0x19, 0x17, 0x15, 0x00,
    0x19, 0x17, 0x18, 0x17,
    0x19, 0x1A, 0x1B, 0x17,
    0x19, 0x1A, 0x1C, 0x1D,
    0x19, 0x1A, 0x1C, 0x1D,
    0x19, 0x1A, 0x1C, 0x1D,
    0x19, 0x1A, 0x1C, 0x1D,
    0x19, 0x1A, 0x1C, 0x1D,
    0x00, 0x20, 0x1C, 0x1D
  };

  /* $E99A */
  static const uint8_t interior_mask_15[] =
  {
    0x02,
    0x04, 0x32,
    0x01, 0x01
  };

  /* $E99F */
  static const uint8_t interior_mask_16[] =
  {
    0x09,
    0x86, 0x00, _____ _____ _____ _____ 0x5D, 0x5C, 0x54,
    0x84, 0x00, _____ _____ 0x5D, 0x5C, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x5D, 0x5C, 0x85, 0x01, _____ _____ _____ 
    0x5D, 0x5C, 0x87, 0x01, _____ _____ _____ _____ _____
    0x2B, 0x88, 0x01  _____ _____ _____ _____ _____ _____
  };

  /* $E9B9 */
  static const uint8_t interior_mask_17[] =
  {
    0x05,
    0x00, 0x00, 0x5D, 0x5C, 0x67,
    0x5D, 0x5C, 0x83, 0x01, _____
    0x3C, 0x84, 0x01  _____ _____
  };

  /* $E9C6 */
  static const uint8_t interior_mask_18[] =
  {
    0x02,
    0x5D, 0x68,
    0x3C, 0x69
  };

  /* $E9CB */
  static const uint8_t interior_mask_19[] =
  {
    0x0A,
    0x86, 0x00, _____ _____ _____ _____ 0x5D, 0x5C, 0x46, 0x47,
    0x84, 0x00, _____ _____ 0x5D, 0x5C, 0x83, 0x01, _____ 0x39,
    0x00, 0x00, 0x5D, 0x5C, 0x86, 0x01, _____ _____ _____ _____
    0x5D, 0x5C, 0x88, 0x01, _____ _____ _____ _____ _____ _____
    0x4A, 0x89, 0x01  _____ _____ _____ _____ _____ _____ _____
  };

  /* $E9E6 */
  static const uint8_t interior_mask_20[] =
  {
    0x06,
    0x5D, 0x5C, 0x01, 0x47, 0x6A, 0x00,
    0x4A, 0x84, 0x01, _____ _____ 0x6B,
    0x00, 0x84, 0x01, _____ _____ 0x5F
  };

  /* $E9F5 */
  static const uint8_t interior_mask_21[] =
  {
    0x04,
    0x05, 0x4C, 0x00, 0x00,
    0x61, 0x65, 0x66, 0x4C,
    0x61, 0x12, 0x02, 0x60,
    0x61, 0x12, 0x02, 0x60,
    0x61, 0x12, 0x02, 0x60,
    0x61, 0x12, 0x02, 0x60
  };

  /* $EA0E */
  static const uint8_t interior_mask_22[] =
  {
    0x04,
    0x00, 0x00, 0x05, 0x4C,
    0x05, 0x63, 0x64, 0x60,
    0x61, 0x12, 0x02, 0x60,
    0x61, 0x12, 0x02, 0x60,
    0x61, 0x12, 0x02, 0x60,
    0x61, 0x12, 0x02, 0x60,
    0x61, 0x12, 0x62, 0x00
  };

  /* $EA2B */
  static const uint8_t interior_mask_23[] =
  {
    0x03,
    0x00, 0x6C, 0x00,
    0x02, 0x01, 0x68,
    0x02, 0x01, 0x69
  };

  /* $EA35 */
  static const uint8_t interior_mask_24[] =
  {
    0x05,
    0x01, 0x5E, 0x4C, 0x00, 0x00,
    0x01, 0x01, 0x32, 0x30, 0x00,
    0x84, 0x01, _____ _____ 0x5F
  };

  /* $EA43 */
  static const uint8_t interior_mask_25[] =
  {
    0x02,
    0x6E, 0x5A,
    0x6D, 0x39,
    0x3C, 0x39
  };

  /* $EA4A */
  static const uint8_t interior_mask_26[] =
  {
    0x04,
    0x5D, 0x5C, 0x46, 0x47,
    0x4A, 0x01, 0x01, 0x39
  };

  /* $EA53 */
  static const uint8_t interior_mask_27[] =
  {
    0x03,
    0x2C, 0x47, 0x00,
    0x00, 0x61, 0x12,
    0x00, 0x61, 0x12
  };

  /* $EA5D */
  static const uint8_t interior_mask_28[] =
  {
    0x03,
    0x00, 0x45, 0x1E,
    0x02, 0x60, 0x00,
    0x02, 0x60, 0x00
  };

  /* $EA67 */
  static const uint8_t interior_mask_29[] =
  {
    0x05,
    0x45, 0x1E, 0x2C, 0x47, 0x00,
    0x2C, 0x47, 0x45, 0x1E, 0x12,
    0x00, 0x61, 0x12, 0x61, 0x12,
    0x00, 0x61, 0x5F, 0x00, 0x00
  };

  /**
   * $EBC5: Pointers to run-length encoded mask data.
   *
   * The first half is outdoor masks, the second is indoor masks.
   */
  static const uint8_t *mask_pointers[30] =
  {
    &exterior_mask_0[0],  /* $E55F */
    &exterior_mask_1[0],  /* $E5FF */
    &exterior_mask_2[0],  /* $E61E */
    &exterior_mask_3[0],  /* $E6CA */
    &exterior_mask_4[0],  /* $E74B */
    &exterior_mask_5[0],  /* $E758 */
    &exterior_mask_6[0],  /* $E77F */
    &exterior_mask_7[0],  /* $E796 */
    &exterior_mask_8[0],  /* $E7AF */
    &exterior_mask_9[0],  /* $E85C */
    &exterior_mask_10[0], /* $E8A3 */
    &exterior_mask_11[0], /* $E8F0 */
    &exterior_mask_13[0], /* $E940 */
    &exterior_mask_14[0], /* $E972 */
    &exterior_mask_12[0], /* $E92F */

    &interior_mask_29[0], /* $EA67 */
    &interior_mask_27[0], /* $EA53 */
    &interior_mask_28[0], /* $EA5D */
    &interior_mask_15[0], /* $E99A */
    &interior_mask_16[0], /* $E99F */
    &interior_mask_17[0], /* $E9B9 */
    &interior_mask_18[0], /* $E9C6 */
    &interior_mask_19[0], /* $E9CB */
    &interior_mask_20[0], /* $E9E6 */
    &interior_mask_21[0], /* $E9F5 */
    &interior_mask_22[0], /* $EA0E */
    &interior_mask_23[0], /* $EA2B */
    &interior_mask_24[0], /* $EA35 */
    &interior_mask_25[0], /* $EA43 */
    &interior_mask_26[0]  /* $EA4A */
  };

  /**
   * $EC01: mask_t structs for the exterior scene.
   */
  static const mask_t exterior_mask_data[58] =
  {
    { 0x00, { 0x47, 0x70, 0x27, 0x3F }, { 0x6A, 0x52, 0x0C } },
    { 0x00, { 0x5F, 0x88, 0x33, 0x4B }, { 0x5E, 0x52, 0x0C } },
    { 0x00, { 0x77, 0xA0, 0x3F, 0x57 }, { 0x52, 0x52, 0x0C } },
    { 0x01, { 0x9F, 0xB0, 0x28, 0x31 }, { 0x3E, 0x6A, 0x3C } },
    { 0x01, { 0x9F, 0xB0, 0x32, 0x3B }, { 0x3E, 0x6A, 0x3C } },
    { 0x02, { 0x40, 0x4F, 0x4C, 0x5B }, { 0x46, 0x46, 0x08 } },
    { 0x02, { 0x50, 0x5F, 0x54, 0x63 }, { 0x46, 0x46, 0x08 } },
    { 0x02, { 0x60, 0x6F, 0x5C, 0x6B }, { 0x46, 0x46, 0x08 } },
    { 0x02, { 0x70, 0x7F, 0x64, 0x73 }, { 0x46, 0x46, 0x08 } },
    { 0x02, { 0x30, 0x3F, 0x54, 0x63 }, { 0x3E, 0x3E, 0x08 } },
    { 0x02, { 0x40, 0x4F, 0x5C, 0x6B }, { 0x3E, 0x3E, 0x08 } },
    { 0x02, { 0x50, 0x5F, 0x64, 0x73 }, { 0x3E, 0x3E, 0x08 } },
    { 0x02, { 0x60, 0x6F, 0x6C, 0x7B }, { 0x3E, 0x3E, 0x08 } },
    { 0x02, { 0x70, 0x7F, 0x74, 0x83 }, { 0x3E, 0x3E, 0x08 } },
    { 0x02, { 0x10, 0x1F, 0x64, 0x73 }, { 0x4A, 0x2E, 0x08 } },
    { 0x02, { 0x20, 0x2F, 0x6C, 0x7B }, { 0x4A, 0x2E, 0x08 } },
    { 0x02, { 0x30, 0x3F, 0x74, 0x83 }, { 0x4A, 0x2E, 0x08 } },
    { 0x03, { 0x2B, 0x44, 0x33, 0x47 }, { 0x67, 0x45, 0x12 } },
    { 0x04, { 0x2B, 0x37, 0x48, 0x4B }, { 0x6D, 0x45, 0x08 } },
    { 0x05, { 0x37, 0x44, 0x48, 0x51 }, { 0x67, 0x45, 0x08 } },
    { 0x06, { 0x08, 0x0F, 0x2A, 0x3C }, { 0x6E, 0x46, 0x0A } },
    { 0x06, { 0x10, 0x17, 0x2E, 0x40 }, { 0x6E, 0x46, 0x0A } },
    { 0x06, { 0x18, 0x1F, 0x32, 0x44 }, { 0x6E, 0x46, 0x0A } },
    { 0x06, { 0x20, 0x27, 0x36, 0x48 }, { 0x6E, 0x46, 0x0A } },
    { 0x06, { 0x28, 0x2F, 0x3A, 0x4C }, { 0x6E, 0x46, 0x0A } },
    { 0x07, { 0x08, 0x10, 0x1F, 0x26 }, { 0x82, 0x46, 0x12 } },
    { 0x07, { 0x08, 0x10, 0x27, 0x2D }, { 0x82, 0x46, 0x12 } },
    { 0x08, { 0x80, 0x8F, 0x64, 0x73 }, { 0x46, 0x46, 0x08 } },
    { 0x08, { 0x90, 0x9F, 0x5C, 0x6B }, { 0x46, 0x46, 0x08 } },
    { 0x08, { 0xA0, 0xB0, 0x54, 0x63 }, { 0x46, 0x46, 0x08 } },
    { 0x08, { 0xB0, 0xBF, 0x4C, 0x5B }, { 0x46, 0x46, 0x08 } },
    { 0x08, { 0xC0, 0xCF, 0x44, 0x53 }, { 0x46, 0x46, 0x08 } },
    { 0x08, { 0x80, 0x8F, 0x74, 0x83 }, { 0x3E, 0x3E, 0x08 } },
    { 0x08, { 0x90, 0x9F, 0x6C, 0x7B }, { 0x3E, 0x3E, 0x08 } },
    { 0x08, { 0xA0, 0xB0, 0x64, 0x73 }, { 0x3E, 0x3E, 0x08 } },
    { 0x08, { 0xB0, 0xBF, 0x5C, 0x6B }, { 0x3E, 0x3E, 0x08 } },
    { 0x08, { 0xC0, 0xCF, 0x54, 0x63 }, { 0x3E, 0x3E, 0x08 } },
    { 0x08, { 0xD0, 0xDF, 0x4C, 0x5B }, { 0x3E, 0x3E, 0x08 } },
    { 0x08, { 0x40, 0x4F, 0x74, 0x83 }, { 0x4E, 0x2E, 0x08 } },
    { 0x08, { 0x50, 0x5F, 0x6C, 0x7B }, { 0x4E, 0x2E, 0x08 } },
    { 0x08, { 0x10, 0x1F, 0x58, 0x67 }, { 0x68, 0x2E, 0x08 } },
    { 0x08, { 0x20, 0x2F, 0x50, 0x5F }, { 0x68, 0x2E, 0x08 } },
    { 0x08, { 0x30, 0x3F, 0x48, 0x57 }, { 0x68, 0x2E, 0x08 } },
    { 0x09, { 0x1B, 0x24, 0x4E, 0x55 }, { 0x68, 0x37, 0x0F } },
    { 0x0A, { 0x1C, 0x23, 0x51, 0x5D }, { 0x68, 0x38, 0x0A } },
    { 0x09, { 0x3B, 0x44, 0x72, 0x79 }, { 0x4E, 0x2D, 0x0F } },
    { 0x0A, { 0x3C, 0x43, 0x75, 0x81 }, { 0x4E, 0x2E, 0x0A } },
    { 0x09, { 0x7B, 0x84, 0x62, 0x69 }, { 0x46, 0x45, 0x0F } },
    { 0x0A, { 0x7C, 0x83, 0x65, 0x71 }, { 0x46, 0x46, 0x0A } },
    { 0x09, { 0xAB, 0xB4, 0x4A, 0x51 }, { 0x46, 0x5D, 0x0F } },
    { 0x0A, { 0xAC, 0xB3, 0x4D, 0x59 }, { 0x46, 0x5E, 0x0A } },
    { 0x0B, { 0x58, 0x5F, 0x5A, 0x62 }, { 0x46, 0x46, 0x08 } },
    { 0x0B, { 0x48, 0x4F, 0x62, 0x6A }, { 0x3E, 0x3E, 0x08 } },
    { 0x0C, { 0x0B, 0x0F, 0x60, 0x67 }, { 0x68, 0x2E, 0x08 } },
    { 0x0D, { 0x0C, 0x0F, 0x61, 0x6A }, { 0x4E, 0x2E, 0x08 } },
    { 0x0E, { 0x7F, 0x80, 0x7C, 0x83 }, { 0x3E, 0x3E, 0x08 } },
    { 0x0D, { 0x2C, 0x2F, 0x51, 0x5A }, { 0x3E, 0x3E, 0x08 } },
    { 0x0D, { 0x3C, 0x3F, 0x49, 0x52 }, { 0x46, 0x46, 0x08 } },
  };

  uint8_t       iters;  /* was B */
  const mask_t *pmask;  /* was HL */

  assert(state != NULL);

  /* Clear the mask buffer. */
  memset(&state->mask_buffer[0], 0xFF, NELEMS(state->mask_buffer));

  if (state->room_index > room_0_OUTDOORS)
  {
    /* Indoors */

    /* Conv: Merged use of A and B. */
    iters = state->interior_mask_data_count; /* count byte */
    if (iters == 0)
      return; /* no masks */

    pmask = &state->interior_mask_data[0];
    //peightbyte += 2; // skip count byte, then off by 2 bytes
  }
  else
  {
    /* Outdoors */

    iters = 59; // Bug? This count doesn't match NELEMS(exterior_mask_data); which is 58.
    pmask = &exterior_mask_data[0]; // off by - 2 bytes; original points to $EC03, table starts at $EC01 // fix by propagation
  }

  uint8_t            A;           /* was A */
  const uint8_t     *DE;          /* was DE */
  uint16_t           HL;          /* was HL */
  uint16_t           count_of_something; /* was HL */

  /* mask against all */
  do
  {
    uint8_t x, y, height; /* was A/A/A */
    uint8_t mpr1, mpr2;   /* was C/A */

    // moved from state

    /** $B837: (unknown) - Used by mask_stuff. */
    uint8_t         byte_B837;
    /** $B838: (unknown) - Used by mask_stuff. */
    uint8_t         byte_B838;
    /** $B839: (unknown) - Used by mask_stuff. */
    uint8_t         byte_B839;
    /** $B83A: (unknown) - Used by mask_stuff. */
    uint8_t         byte_B83A;

    uint8_t self_BA70;
    uint8_t self_BA72;
    uint8_t self_BA90;
    uint8_t skip; /* was $BABA */


    // PUSH BC
    // PUSH HLeb

    x = state->map_position_related_x;
    if (x - 1 >= pmask->bounds.x1 || x + 2 <= pmask->bounds.x0) // $EC03, $EC02
      goto pop_next;

    y = state->map_position_related_y;
    if (y - 1 >= pmask->bounds.y1 || y + 3 <= pmask->bounds.y0) // $EC05, $EC04
      goto pop_next;

    if (state->tinypos_81B2.x <= pmask->pos.x) // $EC06
      goto pop_next;

    if (state->tinypos_81B2.y < pmask->pos.y) // $EC07
      goto pop_next;

    height = state->tinypos_81B2.height;
    if (height)
      height--; // make inclusive?
    if (height >= pmask->pos.height) // $EC08
      goto pop_next;

    // redundant: HLeb -= 6;

    mpr1 = state->map_position_related_x;
    if (mpr1 >= pmask->bounds.x0) // must be $EC02
    {
      byte_B837 = mpr1 - pmask->bounds.x0;
      byte_B83A = MIN(pmask->bounds.x1 - mpr1, 3) + 1;
    }
    else
    {
      uint8_t x0;

      x0 = pmask->bounds.x0; // must be $EC02
      byte_B837 = 0;
      byte_B83A = MIN((pmask->bounds.x1 - x0) + 1, 4 - (x0 - mpr1));
    }

    mpr2 = state->map_position_related_y;
    if (mpr2 >= pmask->bounds.y0)
    {
      byte_B838 = mpr2 - pmask->bounds.y0;
      byte_B839 = MIN(pmask->bounds.y1 - mpr2, 4) + 1;
    }
    else
    {
      uint8_t y0;

      y0 = pmask->bounds.y0;
      byte_B838 = 0;
      byte_B839 = MIN((pmask->bounds.y1 - y0) + 1, 5 - (y0 - mpr2));
    }

    // In the original code, HLeb is here decremented to point at member y0.

    {
      uint8_t x, y; /* was B, C */ // x,y are the correct way around here i think!
      uint8_t index; /* was A */

      x = y = 0;
      if (byte_B838 == 0)
        y = -state->map_position_related_y + pmask->bounds.y0;
      if (byte_B837 == 0)
        x = -state->map_position_related_x + pmask->bounds.x0;

      index = pmask->index;
      assert(index < NELEMS(mask_pointers));

      state->mask_buffer_pointer = &state->mask_buffer[y * MASK_BUFFER_WIDTH + x];

      DE = mask_pointers[index];
      HL = byte_B839 | (byte_B83A << 8); // vertical bits // split apart again immediately below

      self_BA70 = (HL >> 0) & 0xFF; // self modify
      self_BA72 = (HL >> 8) & 0xFF; // self modify
      self_BA90 = *DE - self_BA72; // self modify // *DE looks like a count
      skip = MASK_BUFFER_WIDTH - self_BA72; // self modify
    }

    count_of_something = scale_val(state, byte_B838, *DE); // DE is the same count as fetched above // sets D to 0
    count_of_something += byte_B837; // horz // was +DE, but D is zero
    count_of_something++; /* iterations */
    do
    {
    ba4d:
      A = *DE; // DE -> $E560 upwards (in outdoors_mask_data)
      if (A >= 128) // signed byte < 0
      {
        A &= 0x7F;
        DE++;
        count_of_something -= A;
        if (count_of_something < 0)
          goto ba69;
        DE++;
        if (DE)
          goto ba4d;
        A = 0;
        goto ba6c;
      }
      DE++;
    }
    while (--count_of_something);
    goto ba6c;

  ba69:
    A = -(count_of_something & 0xFF);

  ba6c:
    {
      uint8_t *maskbufptr; /* was HL */
      uint8_t  iters2;     /* was C */

      maskbufptr = state->mask_buffer_pointer;
      // R I:C Iterations (inner loop);
      iters2 = self_BA70; // self modified
      do
      {
        uint8_t iters3; /* was B */
        uint8_t Adash; /* was A' */

        iters3 = self_BA72; // self modified
        do
        {
          Adash = *DE;

          if (Adash >= 128) // signed byte < 0
          {
            Adash &= 0x7F;

            DE++;
            A = *DE;
          }

          if (A != 0)
            mask_against_tile(A, maskbufptr); // tile index, dst

          maskbufptr++;

          // EX AF,AF' // unpaired?

          if (A != 0 && --A != 0)
            DE--;
          DE++;
        }
        while (--iters3);

        // PUSH BC

        iters3 = self_BA90; // self modified
        if (iters3)
        {
          if (A)
            goto baa3;

        ba9b:
          do
          {
            A = *DE;
            if (A >= 128) // signed byte < 0
            {
              A &= 0x7F;
              DE++;

            baa3:
              iters3 -= A;
              if (iters3 < 0)
                goto bab6;

              DE++;
              //if (!Z)
              goto ba9b;

              // EX AF,AF' // why not just jump instr earlier? // bank
              goto bab9;
            }

            DE++;
          }
          while (--iters3);

          A = 0;
          // EX AF,AF' // why not just jump instr earlier? // bank

          goto bab9;

        bab6:
          A = -A;
          // EX AF,AF' // bank
        }

      bab9:
        maskbufptr += skip; // self modified
        // EX AF,AF'  // unbank
        // POP BC
      }
      while (--iters2);
    }

  pop_next:
    // POP HLeb
    // POP BC
    pmask++; // stride is 1 mask_t
  }
  while (--iters);

  // This scales A by B then returns nothing: is it really doing this for no good reason?
  count_of_something = scale_val(state, A, 8);
}

/**
 * $BACD: Multiply the two input values returning a widened result.
 *
 * Leaf.
 *
 * If value is 00000001 and shift is 8 => 00000000 00001000
 * If value is 10000000 and shift is 8 => 00000100 00000000
 * If value is 00001111 and shift is 8 => 00000000 01111000
 * If value is 01010101 and shift is 8 => 00000010 10101000
 *
 * \param[in] left  Left hand value. (was A)
 * \param[in] right Right hand value. (was E)
 *
 * \return Widened value. (was HL)
 */
uint16_t multiply(uint8_t left, uint8_t right)
{
  uint8_t  iters;  /* was B */
  uint16_t result; /* was HL */

  /* sampled val,shift = $1A,$4 / $42,$8 / $1A,$2A */

  iters  = 8;
  result = 0;
  do
  {
    int carry;

    result <<= 1;
    carry = left >> 7; /* shift out of high end */
    left <<= 1; /* was RRA */
    if (carry)
      result += right; /* shift into low end */
  }
  while (--iters);

  assert(result == left * right);

  return result;
}

/* ----------------------------------------------------------------------- */

/**
 * $BADC: Masks tiles (placed in a stride-of-4 arrangement) by the specified
 * index exterior_tiles (set zero).
 *
 * Leaf.
 *
 * \param[in] index Mask tile index. // a tileindex_t? (was A)
 * \param[in] dst   Pointer to source/destination. (was HL)
 */
void mask_against_tile(tileindex_t index, uint8_t *dst)
{
  const tilerow_t *row; // tile_t *t; or tilerow? /* was HL' */
  uint8_t          iters; /* was B */

  assert(index < 200); // total guess for now
  assert(dst != NULL);

  row = &exterior_tiles_0[index].row[0];
  iters = 8;
  do
  {
    *dst &= *row++;
    dst += 4; /* supertile stride */
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $BAF7: Clipping vischars to the game window.
 *
 * Counterpart to \ref item_visible.
 *
 * \param[in]  state          Pointer to game state.
 * \param[in]  vischar        Pointer to visible character.       (was IY)
 * \param[out] clipped_width  Pointer to returned clipped width.  (was BC)
 * \param[out] clipped_height Pointer to returned ciipped height. (was DE)
 *
 * \return 0 => visible, 0xFF => invisible. (was A)
 */
int vischar_visible(tgestate_t *state,
                    vischar_t  *vischar,
                    uint16_t   *clipped_width,
                    uint16_t   *clipped_height)
{
  const uint8_t *mpr1;   /* was HL */
  uint8_t        A;      /* was A */
  uint16_t       width;  /* was BC */
  uint16_t       height; /* was DE */
  uint16_t       foo1;   /* was HL */
  uint16_t       foo2;   /* was HL */

  assert(state          != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(clipped_width  != NULL);
  assert(clipped_height != NULL);

  /* Width part */

  mpr1 = &state->map_position_related_x;
  A = state->map_position[0] + state->tb_columns - *mpr1;
  if (A > 0)
  {
    if (A < vischar->width_bytes)
    {
      width = A;
    }
    else
    {
      // FIXME. Signed calc will break here.
      A = *mpr1 + vischar->width_bytes - state->map_position[0]; // width_bytes == width in bytes
      if (A <= 0)
        goto invisible;

      if (A < vischar->width_bytes)
        width = ((-A + vischar->width_bytes) << 8) | A; // to grok // packed offset + width?
      else
        width = vischar->width_bytes;
    }

    /* Height part */

    foo1 = (state->map_position[1] + state->tb_rows) * 8; // CHECK if tb_rows is always 17. if so, good.
    height = vischar->scry; // fused
    if (foo1 < height)
      goto invisible;
    if ((foo1 >> 8) != 0)
      goto invisible;
    A = foo1 & 0xFF;
    if (A < vischar->height)
    {
      height = A;
    }
    else
    {
      foo2 = vischar->height + height; // height == height in rows
      height = state->map_position[1] * 8;
      if (foo2 < height)
        goto invisible;
      if ((foo2 >> 8) != 0)
        goto invisible;
      A = foo2 & 0xFF;
      if (A < vischar->height)
        height = ((vischar->height - A) << 8) | A;
      else
        height = vischar->height;
    }

    *clipped_width  = width;
    *clipped_height = height;

    return 0; // return Z (vischar is visible)
  }

invisible:
  return 0xFF; // return NZ (vischar is not visible)
}

/* ----------------------------------------------------------------------- */

/**
 * $BB98: Called from main loop 3.
 *
 * Plots tiles. Checks that characters are visible.
 *
 * \param[in] state Pointer to game state.
 */
void called_from_main_loop_3(tgestate_t *state)
{
  uint8_t    iters;   /* was B */
  vischar_t *vischar; /* was IY */
  uint8_t    A;
  uint8_t    saved_A;
  uint8_t    self_BC5F;
  uint8_t    self_BC61;
  uint8_t    self_BC89;
  uint8_t    self_BC8E;
  uint8_t    self_BC95;
  uint8_t    B, C;
  uint8_t    D, E;
  uint8_t    H, L;

  assert(state != NULL);

  iters   = vischars_LENGTH;
  vischar = &state->vischars[0];
  state->IY = &state->vischars[0];
  do
  {
    uint16_t clipped_width;  /* was BC */
    uint16_t clipped_height; /* was DE */

    // PUSH BC // i.e. iters

    if (vischar->flags == 0)
      goto next; /* Likely: No character. */

    state->map_position_related_y = vischar->scry >> 3; // divide by 8
    state->map_position_related_x = vischar->scrx >> 3; // divide by 8

    if (vischar_visible(state, vischar, &clipped_width, &clipped_height) == 0xFF)
      goto next; /* invisible */

    A = (((clipped_height & 0xFF) >> 3) & 31) + 2;
    saved_A = A;
    A += state->map_position_related_y - state->map_position[1];
    if (A >= 0)
    {
      A -= 17;
      if (A > 0)
      {
        E = A;
        A = saved_A - E;
        if (A < E) // if carry
          goto next;
        if (A != E)
          goto bbf8;
        goto next;
      }
    }
    A = saved_A;

  bbf8:
    if (A > 5)
      A = 5;

    // ADDED / CONV
    B = clipped_width >> 8;
    C = clipped_width & 0xFF;
    D = clipped_height >> 8;
    E = clipped_height & 0xFF;

    self_BC5F = A; // self modify // outer loop counter

    self_BC61 = C; // self modify // inner loop counter
    self_BC89 = C; // self modify

    A = 24 - C;
    self_BC8E = A; // self modify // DE stride

    A += 7 * 24;
    self_BC95 = A; // self modify // HL stride

    uint8_t *HL;
    int carry;

    HL = &state->map_position[0];

    C = clipped_width & 0xFF;

    if (B == 0)
      B = state->map_position_related_x - HL[0];
    else
      B = 0; // was interleaved

    if (D == 0) // (yes, D)
      C = state->map_position_related_y - HL[1];
    else
      C = 0; // was interleaved

    H = C;
    A = 0; // can push forward to reduce later ops
    carry = H & 1; H >>= 1; // SRL H
    RR(A); // was RRA, but same effect
    // A = carry << 7; carry = 0; // when A = 0
    E = A;

    D = H;
    carry = H & 1; H >>= 1; // SRL H
    RR(A); // was RRA, but same effect
    L = A;

    UNFINISHED;
#if 0
    DE = (D << 8) | E;
    HL = (H << 8) | L;

    HL += DE + B + &state->window_buf[0]; // screen buffer start address
    // EX DE, HL
    // PUSH BC

    // POP HLdash

    // at this point C => row, B => column

    HL = C * 24 + B + state->tile_buf; // visible tiles array // was $F0F8
    // EX DE, HL
    C = self_BC5F; /* iterations */ // self modified
    do
    {
      B = self_BC61; /* iterations */ // self modified
      do
      {
        uint8_t Bdash; /* was B' */

        // PUSH HL
        A = *DE;

        // POP DEdash // visible tiles array pointer
        // PUSH HLdash
        select_tile_set(state, Hdash, Ldash); // call using banked registers
        HLdash = A * 8 + BCdash;
        Bdash = 8; /* iterations */
        do
        {
          *DEdash = *HLdash++;
          DEdash += state->tb_columns; // 24
        }
        while (--Bdash);
        // POP HLdash
        Hdash++; // ie. HLdash += 256

        DE++;
        HL++;
      }
      while (--B);

      Hdash -= self_BC89; // self modified
      Ldash++;

      DE += self_BC8E; // self modified
      HL += self_BC95; // self modified
    }
    while (--C);
#endif

next:
    // POP BC
    vischar++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $BCAA: Turn a map ref into a tile set pointer?
 *
 * Called from called_from_main_loop_3.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] x_shift Unknown. (was H)
 * \param[in] y_shift Unknown. (was L)
 */
const tile_t *select_tile_set(tgestate_t *state,
                              uint8_t     x_shift,
                              uint8_t     y_shift)
{
  const tile_t *tileset; /* was BC */

  assert(state != NULL);
  // assert(x_shift);
  // assert(y_shift);

  if (state->room_index != room_0_OUTDOORS)
  {
    tileset = &interior_tiles[0];
  }
  else
  {
    uint8_t          temp; /* was L */
    uint8_t          pos;  /* was Adash */
    supertileindex_t tile; /* was Adash */

    /* Convert map position to an index into 7x5 supertile refs array. */
    temp = ((((state->map_position[1] & 3) + y_shift) >> 2) & 0x3F) * state->st_columns; // vertical // Conv: constant columns 7 made variable
    pos  = ((((state->map_position[0] & 3) + x_shift) >> 2) & 0x3F) + temp; // horizontal + vertical

    tile = state->map_buf[pos]; /* (7x5) supertile refs */
    tileset = &exterior_tiles_1[0];
    if (tile >= 45)
    {
      tileset = &exterior_tiles_2[0];
      if (tile >= 139 && tile < 204)
        tileset = &exterior_tiles_3[0];
    }
  }
  return tileset;
}

/* ----------------------------------------------------------------------- */

/**
 * $C41C: Called from main loop 7.
 *
 * Seems to move characters around, or perhaps just spawn them.
 *
 * \param[in] state Pointer to game state.
 */
void spawn_characters(tgestate_t *state)
{
  uint8_t            H, L;
  uint8_t            D, E;
  characterstruct_t *charstr; /* was HL */
  uint8_t            iters;   /* was B */
  room_t             room;    /* was A */
  uint8_t            C;

  assert(state != NULL);

  /* Form a map position in DE. */
  H = state->map_position[1];
  L = state->map_position[0];
  E = (L < 8) ? 0 : L;
  D = (H < 8) ? 0 : H;

  /* Walk all character structs. */
  charstr = &state->character_structs[0];
  iters   = character_structs__LIMIT; // the 26 'real' characters
  do
  {
    if ((charstr->character & characterstruct_FLAG_DISABLED) == 0)
    {
      room = state->room_index;
      if (room == charstr->room)
      {
        if (room == room_0_OUTDOORS)
        {
          /* Outdoors. */

          C = 0 - charstr->pos.x - charstr->pos.y - charstr->pos.height;
          if (C <= D)
            goto skip; // check
          D = MIN(D + 32, 0xFF);
          if (C > D)
            goto skip; // check

          charstr->pos.y += 64;
          charstr->pos.x -= 64; // right way around?

          C = 128;
          if (C <= E)
            goto skip; // check
          E = MIN(E + 40, 0xFF);
          if (C > E)
            goto skip; // check
        }

        spawn_character(state, charstr);
      }
    }

skip:
    charstr++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $C47E: Run through all visible characters, resetting them if they're
 * off-screen.
 *
 * Called from main loop.
 *
 * \param[in] state Pointer to game state.
 */
void purge_visible_characters(tgestate_t *state)
{
  enum { EDGE = 9 };

  uint8_t    D, E;    /* was D, E */
  uint8_t    iters;   /* was B */
  vischar_t *vischar; /* was HL */
  uint8_t    C, A;    /* was C, A */

  assert(state != NULL);

  E = MAX(state->map_position[0] - EDGE, 0);
  D = MAX(state->map_position[1] - EDGE, 0);

  iters   = vischars_LENGTH - 1;
  vischar = &state->vischars[1]; /* iterate over non-player characters */
  do
  {
    if (vischar->character == character_NONE)
      goto next;

    if (state->room_index != vischar->room)
      goto reset; /* this character is not in the current room */

    C = vischar->scry >> 8;
    A = vischar->scry & 0xFF;
    divide_by_8_with_rounding(&C, &A);
    if (A <= D || A > MIN(D + EDGE + 25, 255)) // Conv: C -> A
      goto reset;

    C = vischar->scrx >> 8;
    A = vischar->scrx & 0xFF;
    divide_by_8(&C, &A);
    if (A <= E || A > MIN(E + EDGE + 33, 255)) // Conv: C -> A
      goto reset;

    goto next;

reset:
    reset_visible_character(state, vischar);

next:
    vischar++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $C4E0: Add a character to the visible character list.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] charstr Pointer to character to spawn. (was HL)
 *
 * \return NZ if not? spawned.
 */
int spawn_character(tgestate_t *state, characterstruct_t *charstr)
{
  /**
   * $CD9A: Maps characters to sprite sets (maybe).
   */
  static const character_meta_data_t character_meta_data[4] =
  {
    { &character_related_pointers[0], &sprites[sprite_COMMANDANT_FACING_TOP_LEFT_4] },
    { &character_related_pointers[0], &sprites[sprite_GUARD_FACING_TOP_LEFT_4]      },
    { &character_related_pointers[0], &sprites[sprite_DOG_FACING_TOP_LEFT_1]        },
    { &character_related_pointers[0], &sprites[sprite_PRISONER_FACING_TOP_LEFT_4]   },
  };

  vischar_t                   *vischar;   /* was HL/IY */
  uint8_t                      iters;     /* was B */
  characterstruct_t           *charstr2;  /* was DE */
  pos_t                       *saved_pos; /* was HL */
  character_t                  character; /* was A */
  const character_meta_data_t *metadata;  /* was DE */
  int                          Z;
  room_t                       room;      /* was A */
  uint8_t                      A;

  assert(state   != NULL);
  assert(charstr != NULL);

  if (charstr->character & characterstruct_FLAG_DISABLED) /* character disabled */
    return 1; // NZ

  /* Find an empty slot. */
  vischar = &state->vischars[1];
  iters = vischars_LENGTH - 1;
  do
  {
    if (vischar->character == vischar_CHARACTER_EMPTY_SLOT)
      goto found_empty_slot;
    vischar++;
  }
  while (--iters);

  return 0; // check


found_empty_slot:

  // POP DE  (DE = charstr)
  // PUSH HL (vischar)
  // POP IY  (IY_vischar = HL_vischar)
  state->IY = vischar; // TODO: Work out all the other places which assign IY.
  // PUSH HL (vischar)
  // PUSH DE (charstr)

  charstr2 = charstr;

  /* Scale coords dependent on which room the character is in. */
  saved_pos = &state->saved_pos;
  if (charstr2->room == 0)
  {
    /* Conv: Unrolled. */
    saved_pos->x      = charstr2->pos.x      * 8;
    saved_pos->y      = charstr2->pos.y      * 8;
    saved_pos->height = charstr2->pos.height * 8;
  }
  else
  {
    /* Conv: Unrolled. */
    saved_pos->x      = charstr2->pos.x;
    saved_pos->y      = charstr2->pos.y;
    saved_pos->height = charstr2->pos.height;
  }

  Z = collision(state, vischar);
  if (Z == 0)
    Z = bounds_check(state, vischar);

  // POP DE (charstr2)
  // POP HL (vischar)

  if (Z == 1) // if collision or bounds_check is nonzero, then return
    return 0; // check

  character = charstr2->character | characterstruct_FLAG_DISABLED; /* Disable character */
  charstr2->character = character;

  character &= characterstruct_BYTE0_MASK;
  vischar->character = character;
  vischar->flags     = 0;

  // PUSH DE (charstr2)

  metadata = &character_meta_data[0]; /* Commandant */
  if (character != 0)
  {
    metadata = &character_meta_data[1]; /* Guard */
    if (character >= 16)
    {
      metadata = &character_meta_data[2]; /* Dog */
      if (character >= 20)
      {
        metadata = &character_meta_data[3]; /* Prisoner */
      }
    }
  }

  // EX DE, HL (DE = vischar, HL = charstr)

  vischar->w08          = metadata->data;
  vischar->mi.spriteset = metadata->spriteset;
  memcpy(&vischar->mi.pos, &state->saved_pos, sizeof(pos_t));

  // POP HL

  //HL += 5; // -> charstr->target
  //DE += 7; // -> vischar->room

  room = state->room_index;
  vischar->room = room;
  if (room > room_0_OUTDOORS)
  {
    play_speaker(state, sound_CHARACTER_ENTERS_2);
    play_speaker(state, sound_CHARACTER_ENTERS_1);
  }

  //DE -= 26; // -> vischar->target
  //*DE++ = *HL++;
  //*DE++ = *HL++;
  vischar->target = charstr->target;
  //HL -= 2; // -> charstr->target

c592:
  if ((charstr->target & 0xFF) == 0)
  {
    // DE += 3; // -> vischar->b07
  }
  else
  {
    state->byte_A13E = 0;
    // PUSH DE // -> vischar->p04
    A = sub_C651(state, &charstr->target); // is A being set by sub_C651?
    if (A == 255)
    {
      // POP HL // HL = DE
      // HL -= 2; // -> vischar->target
      // PUSH HL
      sub_CB2D(state, vischar, &vischar->target);
      // POP HL
      // DE = HL + 2; // -> vischar->p04
      goto c592;
    }
    else if (A == 128)
    {
      vischar->flags |= vischar_FLAGS_BIT6;
    }
    // POP DE // -> vischar->p04
    // sampled HL is $7913 (a door_positions thing)
    // FIXME: HL must be returned from sub_C651
    memcpy(&vischar->p04, NULL/*HL*/, 3); // pointers incremented in original?
  }
  vischar->b07 = 0;
  // DE -= 7;
  // EX DE,HL
  // PUSH HL
  reset_position(state, vischar);
  // POP HL
  character_behaviour(state, vischar);
  return 1; // exit via
}

/* ----------------------------------------------------------------------- */

/**
 * $C5D3: Reset visible character
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 */
void reset_visible_character(tgestate_t *state, vischar_t *vischar)
{
  character_t        character; /* was A */
  pos_t             *pos;       /* was DE */
  characterstruct_t *charstr;   /* was DE */
  room_t             room;      /* was A */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  character = vischar->character;
  if (character == character_NONE)
    return;

  if (character >= character_26_STOVE_1)
  {
    /* A stove or crate character. */

    vischar->character = character_NONE;
    vischar->flags     = 0xFF; /* flags */
    vischar->b07       = 0;    /* more flags */

    /* Save the old position. */
    if (character == character_26_STOVE_1)
      pos = &state->movable_items[movable_item_STOVE1].pos;
    else if (character == character_27_STOVE_2)
      pos = &state->movable_items[movable_item_STOVE2].pos;
    else
      pos = &state->movable_items[movable_item_CRATE].pos;
    memcpy(pos, &vischar->mi.pos, sizeof(*pos));
  }
  else
  {
    pos_t     *vispos_in;   /* was HL */
    tinypos_t *charpos_out; /* was DE */

    /* A non-object character. */

    charstr = get_character_struct(state, character);
    charstr->character &= ~characterstruct_FLAG_DISABLED; /* Enable character */

    room = vischar->room;
    charstr->room = room;

    vischar->b07 = 0; /* more flags */

    /* Save the old position. */

    vispos_in   = &vischar->mi.pos;
    charpos_out = &charstr->pos;

    if (room == room_0_OUTDOORS)
    {
      /* Outdoors. */
      pos_to_tinypos(vispos_in, charpos_out);
    }
    else
    {
      /* Indoors. */
      /* Conv: Unrolled from original code. */
      charpos_out->x      = vispos_in->x;
      charpos_out->y      = vispos_in->y;
      charpos_out->height = vispos_in->height;
    }

    // character = vischar->character; /* Done in original code, but we already have this from earlier. */
    vischar->character = character_NONE;
    vischar->flags     = 0xFF;

    /* Guard dogs only. */
    if (character >= character_16_GUARD_DOG_1 && character <= character_19_GUARD_DOG_4)
    {
      vischar->target = 0x00FF;
      if (character >= character_18_GUARD_DOG_3) /* Characters 18 and 19 */
        vischar->target = 0x18FF;
    }

    charstr->target = vischar->target;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $C651: (unknown)
 *
 * \param[in] state    Pointer to game state.
 * \param[in] location Pointer to characterstruct + 5 [or others]. (target field(s)) (was HL)
 *
 * \return 0/128/255. // FIXME: HL must be returned too
 */
uint8_t sub_C651(tgestate_t *state, location_t *location)
{
  /**
   * $783A: Map locations. (0xYYXX)
   */
  static const uint16_t word_783A[78] =
  {
    0x6845,
    0x5444,
    0x4644,
    0x6640,
    0x4040,
    0x4444,
    0x4040,
    0x4044,
    0x7068,
    0x7060,
    0x666A,
    0x685D,
    0x657C,
    0x707C,
    0x6874,
    0x6470,
    0x6078,
    0x5880,
    0x6070,
    0x5474,
    0x647C,
    0x707C,
    0x6874,
    0x6470,
    0x4466,
    0x4066,
    0x4060,
    0x445C,
    0x4456,
    0x4054,
    0x444A,
    0x404A,
    0x4466,
    0x4444,
    0x6844,
    0x456B,
    0x2D6B,
    0x2D4D,
    0x3D4D,
    0x3D3D, // outside main map?
    0x673D, // outside main map?
    0x4C74,
    0x2A2C, // outside main map?
    0x486A,
    0x486E,
    0x6851,
    0x3C34, // outside main map?
    0x2C34, // outside main map?
    0x1C34,
    0x6B77,
    0x6E7A,
    0x1C34, // outside main map?
    0x3C28, // outside main map?
    0x2224, // outside main map?
    0x4C50,
    0x4C59,
    0x3C59,
    0x3D64,
    0x365C,
    0x3254,
    0x3066,
    0x3860,
    0x3B4F,
    0x2F67,
    0x3634, // outside main map?
    0x2E34, // outside main map?
    0x2434, // outside main map?
    0x3E34, // outside main map?
    0x3820, // outside main map?
    0x1834, // outside main map?
    0x2E2A, // outside main map?
    0x2222, // outside main map?
    0x6E78, // roll call
    0x6E76, // roll call
    0x6E74, // roll call
    0x6D79, // roll call
    0x6D77, // roll call
    0x6D75, // roll call
  };

  uint8_t A;

  assert(state    != NULL);
  assert(location != NULL);

  A = *location & 0xFF; // read low byte only
  if (A == 0xFF)
  {
    uint8_t A1, A2;

    // In original code, *HL is used as a temporary.

    A1 = ((*location & 0xFF00) >> 8) & characterstruct_BYTE6_MASK_HI; // read high byte only
    A2 = random_nibble(state) & 7;
    *location = (*location & 0x00FF) | ((A1 | A2) << 8); // write high byte only
  }
  else
  {
    uint8_t        C;
    const uint8_t *DE;
    uint16_t       HL2; // likely needs to be signed
    const uint8_t *HL3;

    // PUSH HL
    C = (*location & 0xFF00) >> 8;
    DE = element_A_of_table_7738(A);

    HL2 = 0; // was HL = 0;
    A = C;
    if (A == 0xFF) // if high byte is 0xFF
      HL2 = 0xFF00; // was H--; // H = 0 - 1 => 0xFF
    HL2 |= A; // was L = A;
    HL3 = DE + HL2;
    // EX DE,HL
    A = *HL3; // was A = *DE
    // POP HL // was interleaved
    if (A == 0xFF)
      goto return_255;

    if ((A & 0x7F) < 40)
    {
      A = *DE;
      if (*location & (1 << 7))
        A ^= 0x80; // 762C, 8002, 7672, 7679, 7680, 76A3, 76AA, 76B1, 76B8, 76BF, ... looks quite general
      transition(state, location);
      location++;
      return 0x80; // CHECK
    }
    A = *DE - 40;
  }

  // sample A=$38,2D,02,06,1E,20,21,3C,23,2B,3A,0B,2D,04,03,1C,1B,21,3C,...
  location = &word_783A[A];
  // need to be able to pass back HL (location)

  return 0;

return_255:
  return 255;
}

/* ----------------------------------------------------------------------- */

/**
 * $C6A0: Moves characters around.
 *
 * \param[in] state Pointer to game state.
 */
void move_characters(tgestate_t *state)
{
  characterstruct_t *charstr; /* was HL */
  item_t             item;    /* was C */
  room_t             room;    /* was A */
  uint8_t            A2;      /* was A */
  character_t        A3;      /* was A */

  assert(state != NULL);

  state->byte_A13E = 0xFF;

  /* Conv: Test replaced with modulus. */
  state->character_index = (state->character_index + 1) % character_26_STOVE_1; // 26 = highest + 1 character

  charstr = get_character_struct(state, state->character_index);
  if (charstr->character & characterstruct_FLAG_DISABLED)
    return; /* Disabled character. */

  // PUSH HL
  room = charstr->room;
  if (room != room_0_OUTDOORS)
    if (is_item_discoverable_interior(state, room, &item) == 0)
      item_discovered(state, item);
  // POP HL
  //HL += 2; // point at characterstruct x,y coords
  // PUSH HL
  //HL += 3; // point at characterstruct target
  A2 = charstr->target & 0xFF;
  if (A2 == 0)
  {
    // POP HL
    return;
  }

  A2 = sub_C651(state, &charstr->target); // vischar needs to come from somewhere, but we don't know where!
  if (A2 == 0xFF)
  {
    A3 = state->character_index;
    if (A3 != character_0_COMMANDANT)
    {
      // Not the commandant.

      if (A3 >= character_12_GUARD_12)
        goto char_ge_12;

      // Characters 1..11.
      UNFINISHED;
#if 0
back:
      *HL++ ^= 0x80;
      if (A & 7)
        (*HL) -= 2;
      (*HL)++; // weird // i.e -1 or +1
      // POP HL
#endif
      return;
    }

    // Hero character.
//char_is_zero:
    UNFINISHED;
#if 0
    A = *HL & characterstruct_BYTE5_MASK; // fetching a character index? // sampled = HL = $7617 (characterstruct + 5) // location
    if (A != 36)
      goto back;
#endif

char_ge_12:
    // POP DE
    character_event(state, NULL/*HL*/);
    return; // exit via
  }
  else // CHECK THIS IS RIGHT
  {
    UNFINISHED;
#if 0
    if (A == 0x80)
    {
      // POP DE
      A = DE[-1];
      // PUSH HL
      if (A == 0)
      {
        // PUSH DE
        DE = &saved_Y;
        // UNROLLED
        *DE++ = *HL++ >> 1;
        *DE++ = *HL++ >> 1;
        HL = &saved_Y;
        // POP DE
      }
      if (DE[-1] == 0)
        A = 2;
      else
        A = 6;
      // EX AF, AF'
      B = 0;
      change_by_delta(A, B, HL, DE); // max, rc, second, first
      DE++;
      HL++;
      change_by_delta(A, B, HL, DE);
      // POP HL
      if (B != 2) // if B == 2 then change_by_delta didn't move
        return; // managed to move
      DE -= 2;
      HL--;
      *DE = (*HL & doorposition_BYTE0_MASK_HI) >> 2; // mask

      // Stuff reading from door_positions.
      if ((*HL & doorposition_BYTE0_MASK_LO) < 2)
        // sampled HL = 78fa,794a,78da,791e,78e2,790e,796a,790e,791e,7962,791a
        HL += 5;
      else
        HL -= 3;

      A = *DE++;
      if (A)
      {
        *DE++ = *HL++;
        *DE++ = *HL++;
        *DE++ = *HL++;
        DE--;
      }
      else
      {
        /* Conv: Unrolled. */
        *DE++ = *HL++ >> 1;
        *DE++ = *HL++ >> 1;
        *DE++ = *HL++ >> 1;
        DE--;
      }
    }
    else
    {
      // POP DE
      tmpA = DE[-1];
      A = 2;
      if (tmpA)
        A = 6;
      // EX AF,AF'
      B = 0;
      change_by_delta(Adash, B, HL, DE);
      HL++;
      DE++;
      change_by_delta(Adash, B, HL, DE);
      DE++;
      if (B != 2)
        return; // managed to move
    }
    DE++;
    // EX DE, HL
    A = *HL; // address? 761e 7625 768e 7695 7656 7695 7680 // => character struct entry + 5
    if (A == 0xFF)
      return;
    if ((A & (1 << 7)) != 0) ...
      HL++;       // interleaved
    ... goto exit;
    (*HL)++;
    return;

exit:
    (*HL)--;
#endif
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $C79A: Increments 'first' by ('first' - 'second'). Used by move_characters().
 *
 * Leaf.
 *
 * \param[in] max    A maximum value. (was A')  usually 2 or 6
 * \param[in] rc     Return code. Incremented if delta is zero. (was B)
 * \param[in] second Pointer to second value. (was HL)
 * \param[in] first  Pointer to first value. (was DE)
 *
 * \return rc as passed in, or rc incremented if delta was zero. (was B)
 */
int change_by_delta(int8_t         max,
                    int            rc,
                    const uint8_t *second,
                    uint8_t       *first)
{
  int delta; /* was A */

  // assert(max);
  assert(rc < 2); /* Should not be called with otherwise. */
  assert(second != NULL);
  assert(first  != NULL);

  delta = *first - *second;
  if (delta == 0)
  {
    rc++;
  }
  else if (delta < 0)
  {
    delta = -delta;
    if (delta >= max)
      delta = max;
    *first += delta;
  }
  else
  {
    if (delta >= max)
      delta = max;
    *first -= delta;
  }

  return rc;
}

/* ----------------------------------------------------------------------- */

/**
 * $C7B9: Get character struct.
 *
 * \param[in] state Pointer to game state.
 * \param[in] index Character index. (was A)
 *
 * \return Pointer to characterstruct. (was HL)
 */
characterstruct_t *get_character_struct(tgestate_t *state, character_t index)
{
  assert(state != NULL);
  assert(index >= 0 && index < character__LIMIT);

  return &state->character_structs[index];
}

/* ----------------------------------------------------------------------- */

/**
 * $C7C6: Character event.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] location Pointer to location (was HL). [ NEEDS TO BE CHARSTRUCT+5 *OR* VISCHAR+2   // likely a location_t and not a characterstruct_t ]
 */
void character_event(tgestate_t *state, location_t *location)
{
  /* $C7F9 */
  static const charactereventmap_t eventmap[] =
  {
    { 0xA6,  0 },
    { 0xA7,  0 },
    { 0xA8,  1 },
    { 0xA9,  1 },
    { 0x05,  0 },
    { 0x06,  1 },
    { 0x85,  3 },
    { 0x86,  3 },
    { 0x0E,  2 },
    { 0x0F,  2 },
    { 0x8E,  0 },
    { 0x8F,  1 },
    { 0x10,  5 },
    { 0x11,  5 },
    { 0x90,  0 },
    { 0x91,  1 },
    { 0xA0,  0 },
    { 0xA1,  1 },
    { 0x2A,  7 },
    { 0x2C,  8 }, /* sleeps */
    { 0x2B,  9 }, /* sits */
    { 0xA4,  6 },
    { 0x24, 10 }, /* released from solitary .. walking out? */
    { 0x25,  4 }  /* morale related .. re-enables player control after solitary released? */
  };

  /* $C829 */
  static charevnt_handler_t *const handlers[] =
  {
    &charevnt_handler_1,
    &charevnt_handler_2,
    &charevnt_handler_3_check_var_A13E,
    &charevnt_handler_4_zeroes_morale_1,
    &charevnt_handler_5_check_var_A13E_anotherone,
    &charevnt_handler_6,
    &charevnt_handler_7,
    &charevnt_handler_8_hero_sleeps,
    &charevnt_handler_9_hero_sits,
    &charevnt_handler_10_hero_released_from_solitary
  };

  uint8_t                    A;         /* was A */
  const charactereventmap_t *peventmap;   /* was HL */
  uint8_t                    iters;       /* was B */

  assert(state    != NULL);
  assert(location != NULL);

  A = *location & 0xFF;
  if (A >= 7 && A <= 12)
  {
    character_sleeps(state, A, location);
    return;
  }
  if (A >= 18 && A <= 22)
  {
    character_sits(state, A, location);
    return;
  }

  peventmap = &eventmap[0];
  iters = NELEMS(eventmap);
  do
  {
    if (A == peventmap->something)
    {
      handlers[peventmap->handler](state, 0,0); // FIXME expecting charptr, vischar
      return;
    }
    peventmap++;
  }
  while (--iters);

  *location = 0; /* Conv: Sets both bytes, original code sets first byte only. */
}

/**
 * $C83F:
 */
void charevnt_handler_4_zeroes_morale_1(tgestate_t  *state,
                                        character_t *charptr,
                                        vischar_t   *vischar)
{
  assert(state   != NULL);
  assert(charptr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  state->morale_1 = 0; /* Enable player control */
  charevnt_handler_0(state, charptr, vischar);
}

/**
 * $C845:
 */
void charevnt_handler_6(tgestate_t  *state,
                        character_t *charptr,
                        vischar_t   *vischar)
{
  assert(state   != NULL);
  assert(charptr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  // POP charptr // (popped) sampled charptr = $80C2 (x2), $8042  // likely target location
  *charptr++ = 0x03;
  *charptr   = 0x15;
}

/**
 * $C84C:
 */
void charevnt_handler_10_hero_released_from_solitary(tgestate_t  *state,
                                                     character_t *charptr,
                                                     vischar_t   *vischar)
{
  assert(state   != NULL);
  assert(charptr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  // POP charptr
  *charptr++ = 0xA4;
  *charptr   = 0x03;
  state->automatic_player_counter = 0; // force automatic control
  set_hero_target_location(state, 0x2500); // original jump was $A344, but have moved it
}

/**
 * $C85C:
 */
void charevnt_handler_1(tgestate_t  *state,
                        character_t *charptr,
                        vischar_t   *vischar)
{
  uint8_t C;

  assert(state   != NULL);
  assert(charptr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  C = 0x10; // 0xFF10
  localexit(state, charptr, C);
}

/**
 * $C860:
 */
void charevnt_handler_2(tgestate_t  *state,
                        character_t *charptr,
                        vischar_t   *vischar)
{
  uint8_t C;

  assert(state   != NULL);
  assert(charptr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  C = 0x38; // 0xFF38
  localexit(state, charptr, C);
}

/**
 * $C864:
 */
void charevnt_handler_0(tgestate_t  *state,
                        character_t *charptr,
                        vischar_t   *vischar)
{
  uint8_t C;

  assert(state   != NULL);
  assert(charptr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  C = 0x08; // 0xFF08 // sampled HL=$8022,$8042,$8002,$8062
  localexit(state, charptr, C);
}

void localexit(tgestate_t *state, character_t *charptr, uint8_t C)
{
  assert(state   != NULL);
  assert(charptr != NULL);
  // assert(C);

  // POP charptr
  *charptr++ = 0xFF;
  *charptr   = C;
}

/**
 * $C86C:
 */
void charevnt_handler_3_check_var_A13E(tgestate_t  *state,
                                       character_t *charptr,
                                       vischar_t   *vischar)
{
  assert(state   != NULL);
  assert(charptr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  // POP HL
  if (state->byte_A13E == 0)
    byte_A13E_is_zero(state, charptr, vischar);
  else
    byte_A13E_is_nonzero(state, charptr);
}

/**
 * $C877:
 */
void charevnt_handler_5_check_var_A13E_anotherone(tgestate_t   *state,
                                                  character_t *charptr,
                                                  vischar_t    *vischar)
{
  assert(state   != NULL);
  assert(charptr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  // POP HL
  if (state->byte_A13E == 0)
    byte_A13E_is_zero_anotherone(state, charptr, vischar);
  else
    byte_A13E_is_nonzero_anotherone(state, charptr, vischar);
}

/**
 * $C882:
 */
void charevnt_handler_7(tgestate_t  *state,
                        character_t *charptr,
                        vischar_t   *vischar)
{
  assert(state   != NULL);
  assert(charptr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  // POP charptr
  *charptr++ = 0x05;
  *charptr   = 0x00;
}

/**
 * $C889:
 */
void charevnt_handler_9_hero_sits(tgestate_t  *state,
                                  character_t *charptr,
                                  vischar_t   *vischar)
{
  assert(state   != NULL);
  assert(charptr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  // POP HL
  hero_sits(state);
}

/**
 * $C88D:
 */
void charevnt_handler_8_hero_sleeps(tgestate_t  *state,
                                    character_t *charptr,
                                    vischar_t   *vischar)
{
  assert(state   != NULL);
  assert(charptr != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  // POP HL
  hero_sleeps(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $C892: Causes characters to follow the hero if he's being suspicious.
 * Also: Food item discovery.
 * Also: Automatic hero behaviour.
 *
 * \param[in] state Pointer to game state.
 */
void follow_suspicious_character(tgestate_t *state)
{
  vischar_t *vischar; /* was IY */
  uint8_t    iters;   /* was B */

  assert(state != NULL);

  /* (I've still no idea what this flag means). */
  state->byte_A13E = 0;

  /* If the bell is ringing, hostiles persue. */
  if (state->bell)
    hostiles_persue(state);

  /* If food was dropped then count down until it is discovered. */
  if (state->food_discovered_counter != 0 &&
      --state->food_discovered_counter == 0)
  {
    /* De-poison the food. */
    state->item_structs[item_FOOD].item_and_flags &= ~itemstruct_ITEM_FLAG_POISONED;
    item_discovered(state, item_FOOD);
  }

  /* Make supporting characters react. */
  vischar = &state->vischars[1];
  state->IY = &state->vischars[0];
  iters   = vischars_LENGTH - 1;
  do
  {
    if (vischar->flags != 0)
    {
      character_t character; /* was A */

      character = vischar->character & vischar_CHARACTER_MASK;
      if (character <= character_19_GUARD_DOG_4) /* Hostile characters only. */
      {
        /* Characters 0..19. */

        is_item_discoverable(state);

        if (state->red_flag || state->automatic_player_counter > 0)
          guards_follow_suspicious_character(state, vischar);

        if (character >= character_16_GUARD_DOG_1)
        {
          /* Guard dogs 1..4 (characters 16..19). */

          /* Is the food nearby? */
          if (state->item_structs[item_FOOD].room_and_flags & itemstruct_ROOM_FLAG_NEARBY_7)
            vischar->flags = vischar_FLAGS_BRIBE_PENDING | vischar_FLAGS_BIT1; // dead dog? or just baited?
        }
      }

      character_behaviour(state, vischar);
    }
    vischar++;
  }
  while (--iters);

  /* Inhibit hero automatic behaviour when the flag is red, or otherwise
   * inhibited. */
  if (!state->red_flag &&
      (state->morale_1 || state->automatic_player_counter == 0))
  {
    state->IY = &state->vischars[0];
    character_behaviour(state, &state->vischars[0]);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $C918: Character behaviour stuff.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void character_behaviour(tgestate_t *state, vischar_t *vischar)
{
  uint8_t    flags;    /* was A */
  uint8_t    b07;      /* was B */
  uint8_t    iters;    /* was B */
  vischar_t *vischar2; /* was HL */
  uint8_t    A;        /* was A */
  uint8_t    Cdash;
  uint8_t    log2scale;

  assert(state   != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  b07 = vischar->b07; /* more flags */ // Conv: Use of A dropped.
  if (b07 & vischar_BYTE7_MASK) // if bottom nibble set...
  {
    vischar->b07 = --b07; // ...decrement it. A counter of some sort?
    return;
  }

  vischar2 = vischar; // no need for separate HL and IY now other code reduced
  flags = vischar2->flags;
  if (flags != 0)
  {
    if (flags == vischar_FLAGS_BRIBE_PENDING) // == 1
    {
a_1:
      vischar2->p04.x = state->hero_map_position.x;
      vischar2->p04.y = state->hero_map_position.y;
      goto jump_c9c0;
    }
    else if (flags == vischar_FLAGS_BIT1) // == 2
    {
      if (state->automatic_player_counter > 0)
        goto a_1;

      vischar2->flags = 0;
      sub_CB23(state, vischar2, &vischar2->target);
      return;
    }
    else if (flags == (vischar_FLAGS_BRIBE_PENDING + vischar_FLAGS_BIT1)) // == 3
    {
      if (state->item_structs[item_FOOD].room_and_flags & itemstruct_ROOM_FLAG_NEARBY_7)
      {
        // Moves dog toward poisoned food?
        vischar2->p04.x = state->item_structs[item_FOOD].pos.x;
        vischar2->p04.y = state->item_structs[item_FOOD].pos.y;
        goto jump_c9c0;
      }
      else
      {
        vischar2->flags  = 0;
        vischar2->target = 0x00FF;
        sub_CB23(state, vischar2, &vischar2->target);
        return;
      }
    }
    else if (flags == vischar_FLAGS_BIT2) // gone mad / bribe flag
    {
      character_t  bribed_character; /* was A */
      vischar_t   *found;            /* was HL */

      bribed_character = state->bribed_character;
      if (bribed_character != character_NONE)
      {
        /* Iterate over non-player characters. */
        iters = vischars_LENGTH - 1;
        found = &state->vischars[1];
        do
        {
          if (found->character == bribed_character) /* Bug? This doesn't mask off HL->character. */
            goto found_bribed;
          found++;
        }
        while (--iters);
      }

      /* Bribed character was not visible. */
      vischar2->flags = 0;
      sub_CB23(state, vischar2, &vischar2->target);
      return;

found_bribed:
      {
        pos_t     *pos;     /* was HL */
        tinypos_t *tinypos; /* was DE */

        pos     = &found->mi.pos;
        tinypos = &vischar2->p04;
        if (state->room_index > room_0_OUTDOORS)
        {
          /* Indoors */
          pos_to_tinypos(pos, tinypos);
        }
        else
        {
          /* Outdoors */
          tinypos->x = pos->x;
          tinypos->y = pos->y;
        }
        goto jump_c9c0;
      }

    }
  }

  A = vischar2->target & 0x00FF;
  if (A == 0)
  {
    character_behaviour_end_1(state, vischar, A);
    return; // exit via
  }

jump_c9c0:
  Cdash = A = vischar2->flags; // sampled = $8081, 80a1, 80e1, 8001, 8021, ...

#if ORIGINAL
  /* Original code self modifies move_character_x/y routines. */
  if (state->room_index > room_0_OUTDOORS)
    HLdash = &multiply_by_1;
  else if (Cdash & vischar_FLAGS_BIT6)
    HLdash = &multiply_by_4;
  else
    HLdash = &multiply_by_8;

  self_CA13 = HLdash; // self-modify move_character_x:$CA13
  self_CA4B = HLdash; // self-modify move_character_y:$CA4B
#else
  /* Replacement code passes down a log2scale factor. */
  if (state->room_index > room_0_OUTDOORS)
    log2scale = 1;
  else if (Cdash & vischar_FLAGS_BIT6)
    log2scale = 4;
  else
    log2scale = 8;
#endif

  if (vischar->b07 & vischar_BYTE7_BIT5)
    character_behaviour_end_2(state, vischar, A, log2scale);
  else if (move_character_x(state, vischar, log2scale) == 0 &&
           move_character_y(state, vischar, log2scale) == 0)
    // character couldn't move?
    bribes_solitary_food(state, vischar);
  else
    character_behaviour_end_1(state, vischar, A); /* was fallthrough */
}

/**
 * $C9F5: Unknown end part.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.   (was IY)
 * \param[in] flags   Flags (of another character...) (was A)
 */
void character_behaviour_end_1(tgestate_t *state,
                               vischar_t  *vischar,
                               uint8_t     flags)
{
  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  // assert(flags);

  if (flags != vischar->b0D)
    vischar->b0D = flags | vischar_BYTE13_BIT7;
}

/**
 * $C9FF: Unknown end part.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] vischar   Pointer to visible character.   (was IY)
 * \param[in] flags     Flags (of another character...) (was A)
 * \param[in] log2scale Log2Scale factor (replaces self-modifying code in original).
 */
void character_behaviour_end_2(tgestate_t *state,
                               vischar_t  *vischar,
                               uint8_t     flags,
                               int         log2scale)
{
  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  // assert(flags);
  // assert(log2scale);

  /* Note: Unusual y,x order but it matches the original code. */
  if (move_character_y(state, vischar, log2scale) == 0 &&
      move_character_x(state, vischar, log2scale) == 0)
  {
    character_behaviour_end_1(state, vischar, flags); // likely: couldn't move, so .. do something
    return;
  }

  bribes_solitary_food(state, vischar);
}

/* ----------------------------------------------------------------------- */

/**
 * $CA11: Move character on the X axis.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] vischar   Pointer to visible character. (was IY)
 * \param[in] log2scale Log2Scale factor (replaces self-modifying code in
 *                      original).
 *
 * \return 8/4/0 .. meaning?
 */
uint8_t move_character_x(tgestate_t *state,
                         vischar_t  *vischar,
                         int         log2scale)
{
  uint16_t delta; /* was DE */
  uint8_t  D;
  uint8_t  E;

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  // assert(log2scale);

  /* I'm assuming (until proven otherwise) that HL and IY point into the same
   * vischar on entry. */

  delta = vischar->mi.pos.x - (vischar->p04.x << log2scale);
  if (delta)
  {
    /* Conv: Split. */
    D = delta >> 8;
    E = delta & 0xFF;

    if (delta > 0) /* +ve */
    {
      if (D != 0   || E >= 3)
        return 8;
    }
    else /* -ve */
    {
      if (D != 255 || E < 254)
        return 4;
    }
  }

  vischar->b07 |= vischar_BYTE7_BIT5;
  return 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $CA49: Move character on the Y axis.
 *
 * Nearly identical to move_character_x.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] vischar   Pointer to visible character. (was IY)
 * \param[in] log2scale Log2Scale factor (replaces self-modifying code in
 *                      original).
 *
 * \return 5/7/0 .. meaning?
 */
uint8_t move_character_y(tgestate_t *state,
                         vischar_t  *vischar,
                         int         log2scale)
{
  uint16_t delta; /* was DE */
  uint8_t  D;
  uint8_t  E;

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  // assert(log2scale);

  /* I'm assuming (until proven otherwise) that HL and IY point into the same
   * vischar on entry. */

  delta = vischar->mi.pos.y - (vischar->p04.y << log2scale);
  if (delta)
  {
    /* Conv: Split. */
    D = delta >> 8;
    E = delta & 0xFF;

    if (delta > 0) /* +ve */
    {
      if (D != 0   || E >= 3)
        return 5;
    }
    else /* -ve */
    {
      if (D != 255 || E < 254)
        return 7;
    }
  }

  vischar->b07 &= ~vischar_BYTE7_BIT5;
  return 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $CA81: Bribes, solitary, food, 'character enters' sound.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void bribes_solitary_food(tgestate_t *state, vischar_t *vischar)
{
  uint8_t flags;  /* was A */
  uint8_t C;      /* was C */
  uint8_t counter;      /* was A */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  // In the original code HL is IY + 4 on entry.
  // In this version we replace HL references with IY ones.

  flags = vischar->flags;
  C = flags; // copy flags
  flags &= vischar_FLAGS_MASK;
  if (flags)
  {
    if (flags == vischar_FLAGS_BRIBE_PENDING)
    {
      if (vischar->character == state->bribed_character)
        accept_bribe(state, vischar);
      else
        solitary(state); // failed to bribe?
      return; // exit via // factored out
    }
    else if (flags == vischar_FLAGS_BIT1 || flags == vischar_FLAGS_BIT2)
    {
      return;
    }

    if ((state->item_structs[item_FOOD].item_and_flags & itemstruct_ITEM_FLAG_POISONED) == 0)
      counter = 32; /* food is not poisoned */
    else
      counter = 255; /* food is poisoned */
    state->food_discovered_counter = counter;

    vischar->target &= ~0xFFu; // clear low byte

    character_behaviour_end_1(state, vischar, 0); // character_behaviour:$C9F5;
    return;
  }

  if (C & vischar_FLAGS_BIT6)
  {
    UNFINISHED;
#if 0
    //orig:C = *--HL; // 80a3, 8083, 8063, 8003 // likely target location
    //orig:A = *--HL; // 80a2 etc

    C = vischar->target >> 8;
    A = vischar->target & 0xFF;

    A = element_A_of_table_7738(A)[C];
    if ((vischar->target & 0xFF) & vischar_BYTE2_BIT7) // orig:(*HL & vischar_BYTE2_BIT7)
      A ^= 0x80;

    // PUSH AF
    Astacked = *HL++; // $8002, ...
    if (Astacked & vischar_BYTE2_BIT7)
      (*HL) -= 2; // $8003, ... // likely target location
    (*HL)++; // likely target location
    // POP AF

    HL = get_door_position(A); // door related
    vischar->room = (*HL >> 2) & 0x3F; // HL=$790E,$7962,$795E => door position thingy // 0x3F is door_positions[0] room mask shifted right 2
    A = *HL & doorposition_BYTE0_MASK_LO; // door position thingy, lowest two bits -- index?
    if (A < 2)
      HL += 5;
    else
      HL -= 3; // delta of 8 - related to door stride stuff?
    // PUSH HL
    HL = vischar;
    if ((HL & 0x00FF) == 0) // replace with (HL == &state->vischars[0])
    {
      // Hero's vischar only
      HL->flags &= ~vischar_FLAGS_BIT6;
      sub_CB23(state, HL, &HL->target);
    }
    // POP HL
    transition(state, vischar, HL = LOCATION);
#endif
    play_speaker(state, sound_CHARACTER_ENTERS_1);
    return;
  }

  UNFINISHED;
#if 0
  HL -= 2;
  A = *HL; // $8002 etc. // likely target location
  if (A != 0xFF)
  {
    HL++;
    if (A & vischar_BYTE2_BIT7)
    {
      (*HL) -= 2;
    } // $8003 etc.
    else
    {
      (*HL)++;
      HL--;
    }
  }

  sub_CB23(state, vischar, HL); // was fallthrough
#endif
}

/**
 * $CB23: sub_CB23
 *
 * \param[in] state    Pointer to game state.
 * \param[in] vischar  Pointer to visible character. (was IY)
 * \param[in] location seems to be an arg. (target loc ptr). (was HL)
 */
void sub_CB23(tgestate_t *state, vischar_t *vischar, location_t *location)
{
  uint8_t A;

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(location);

  A = sub_C651(state, location);
  if (A != 0xFF)
    sub_CB61(state, vischar, location, location, A); // double HL pass is an adjustment for push-pop
  else
    sub_CB2D(state, vischar, location); // was fallthrough
}

/**
 * $CB2D: sub_CB2D
 *
 * \param[in] state    Pointer to game state.
 * \param[in] vischar  Pointer to visible character. (was IY)
 * \param[in] location seems to be an arg. (target loc ptr). (was HL)
 */
void sub_CB2D(tgestate_t *state, vischar_t *vischar, location_t *location)
{
  character_t character; /* was A */
  uint8_t     A;         /* was A */

  assert(state    != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(location != NULL);

  /* If not the hero's vischar ... */
  if (location != &state->vischars[0].target) /* was (L != 0x02) */
  {
    character = vischar->character & vischar_CHARACTER_MASK;
    if (character == character_0_COMMANDANT)
    {
      A = *location & vischar_BYTE2_MASK;
      if (A == 0x24)
        goto cb46; // character index
      character = 0; // defeat the next 'if' statement // can probably go
    }
    if (character == character_12_GUARD_12)
      goto cb50;
  }

cb46:
  // arrive here if (character == 0 && (location low byte & 7F) == 36)
  // PUSH HL
  character_event(state, location);
  // POP HL
  A = *location;
  if (A == 0)
    return; // A returned?

  sub_CB23(state, vischar, location); // re-enter
  return; // exit via

cb50:
  *location = *location ^ 0x80;
  location++;
  if (A & (1 << 7)) // which flag is this?
    (*location) -= 2;
  (*location)++;
  location--;

  A = 0; // returned?
}

// $CB5F,2 Unreferenced bytes.

/**
 * $CB61: sub_CB61
 *
 * Only ever called by sub_CB23.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] vischar  Pointer to visible character. (was IY)
 * \param[in] location Pointer to location.          (was HL)
 * \param[in] A        flags of some sort
 */
void sub_CB61(tgestate_t *state,
              vischar_t  *vischar,
              location_t *pushed_HL,
              location_t *location,
              uint8_t     A)
{
  assert(state     != NULL);
  ASSERT_VISCHAR_VALID(vischar);
  assert(pushed_HL != NULL);
  assert(location  != NULL);
  // assert(A);

  if (A == 128)
    vischar->flags |= vischar_FLAGS_BIT6;

  memcpy(pushed_HL + 2, location, 2); // replace with straight assignment

  A = 128; // returned?
}

/* ----------------------------------------------------------------------- */

/**
 * $CB75: Widen A to BC.
 *
 * \param[in] A 'A' to be widened.
 *
 * \return 'A' widened to a uint16_t.
 */
uint16_t multiply_by_1(uint8_t A)
{
  return A;
}

/* ----------------------------------------------------------------------- */

/**
 * $CB79: Return the A'th element of table_7738.
 *
 * \param[in] A Index.
 *
 * \return Pointer to ?
 */
const uint8_t *element_A_of_table_7738(uint8_t A)
{
  static const uint8_t data_7795[] = { 0x48, 0x49, 0x4A, 0xFF };
  static const uint8_t data_7799[] = { 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0xFF };
  static const uint8_t data_77A0[] = { 0x56, 0x1F, 0x1D, 0x20, 0x1A, 0x23, 0x99, 0x96, 0x95, 0x94, 0x97, 0x52, 0x17, 0x8A, 0x0B, 0x8B, 0x0C, 0x9B, 0x1C, 0x9D, 0x8D, 0x33, 0x5F, 0x80, 0x81, 0x64, 0x01, 0x00, 0x04, 0x10, 0x85, 0x33, 0x07, 0x91, 0x86, 0x08, 0x12, 0x89, 0x55, 0x0E, 0x22, 0xA2, 0x21, 0xA1, 0xFF };
  static const uint8_t data_77CD[] = { 0x53, 0x54, 0xFF };
  static const uint8_t data_77D0[] = { 0x87, 0x33, 0x34, 0xFF };
  static const uint8_t data_77D4[] = { 0x89, 0x55, 0x36, 0xFF };
  static const uint8_t data_77D8[] = { 0x56, 0xFF };
  static const uint8_t data_77DA[] = { 0x57, 0xFF };
  static const uint8_t data_77DC[] = { 0x58, 0xFF };
  static const uint8_t data_77DE[] = { 0x5C, 0x5D, 0xFF };
  static const uint8_t data_77E1[] = { 0x33, 0x5F, 0x80, 0x81, 0x60, 0xFF };
  static const uint8_t data_77E7[] = { 0x34, 0x0A, 0x14, 0x93, 0xFF };
  static const uint8_t data_77EC[] = { 0x38, 0x34, 0x0A, 0x14, 0xFF };
  static const uint8_t data_77F1[] = { 0x68, 0xFF };
  static const uint8_t data_77F3[] = { 0x69, 0xFF };
  static const uint8_t data_77F5[] = { 0x6A, 0xFF };
  static const uint8_t data_77F7[] = { 0x6C, 0xFF };
  static const uint8_t data_77F9[] = { 0x6D, 0xFF };
  static const uint8_t data_77FB[] = { 0x31, 0xFF };
  static const uint8_t data_77FD[] = { 0x33, 0xFF };
  static const uint8_t data_77FF[] = { 0x39, 0xFF };
  static const uint8_t data_7801[] = { 0x59, 0xFF };
  static const uint8_t data_7803[] = { 0x70, 0xFF };
  static const uint8_t data_7805[] = { 0x71, 0xFF };
  static const uint8_t data_7807[] = { 0x72, 0xFF };
  static const uint8_t data_7809[] = { 0x73, 0xFF };
  static const uint8_t data_780B[] = { 0x74, 0xFF };
  static const uint8_t data_780D[] = { 0x75, 0xFF };
  static const uint8_t data_780F[] = { 0x36, 0x0A, 0x97, 0x98, 0x52, 0xFF };
  static const uint8_t data_7815[] = { 0x18, 0x17, 0x8A, 0x36, 0xFF };
  static const uint8_t data_781A[] = { 0x34, 0x33, 0x07, 0x5C, 0xFF };
  static const uint8_t data_781F[] = { 0x34, 0x33, 0x07, 0x91, 0x5D, 0xFF };
  static const uint8_t data_7825[] = { 0x34, 0x33, 0x55, 0x09, 0x5C, 0xFF };
  static const uint8_t data_782B[] = { 0x34, 0x33, 0x55, 0x09, 0x5D, 0xFF };
  static const uint8_t data_7831[] = { 0x11, 0xFF };
  static const uint8_t data_7833[] = { 0x6B, 0xFF };
  static const uint8_t data_7835[] = { 0x91, 0x6E, 0xFF };
  static const uint8_t data_7838[] = { 0x5A, 0xFF };

  /**
   * $7738: (unknown)
   */
  static const uint8_t *table_7738[46] =
  {
    NULL, /* was zero */
    &data_7795[0],
    &data_7799[0],
    &data_77A0[0],
    &data_77CD[0],
    &data_77D0[0],
    &data_77D4[0],
    &data_77D8[0],
    &data_77DA[0],
    &data_77DC[0],
    &data_77D8[0], /* dupes index 7 */
    &data_77DA[0], /* dupes index 8 */
    &data_77DC[0], /* dupes index 9 */
    &data_77DE[0],
    &data_77E1[0],
    &data_77E1[0], /* dupes index 14 */
    &data_77E7[0],
    &data_77EC[0],
    &data_77F1[0],
    &data_77F3[0],
    &data_77F5[0],
    &data_77F1[0], /* dupes index 18 */
    &data_77F3[0], /* dupes index 19 */
    &data_77F5[0], /* dupes index 20 */
    &data_77F7[0],
    &data_77F9[0],
    &data_77FB[0],
    &data_77FD[0],
    &data_7803[0],
    &data_7805[0],
    &data_7807[0],
    &data_77FF[0],
    &data_7801[0],
    &data_7809[0],
    &data_780B[0],
    &data_780D[0],
    &data_780F[0],
    &data_7815[0],
    &data_781A[0],
    &data_781F[0],
    &data_7825[0],
    &data_782B[0],
    &data_7831[0],
    &data_7833[0],
    &data_7835[0],
    &data_7838[0]
  };

  return table_7738[A];
}

/* ----------------------------------------------------------------------- */

/**
 * $CB85: Pseudo-random number generator.
 *
 * Leaf. Only ever used by sub_C651.
 *
 * \return Pseudo-random number from 0..15.
 */
uint8_t random_nibble(tgestate_t *state)
{
#if IMPRACTICAL_VERSION
  // Impractical code which mimics the original game.

  uint8_t *HL;
  uint8_t  A;

  // word_C41A starts as $9000

  HL = state->word_C41A + 1; // ought to wrap at 256
  A = *HL & 0x0F;
  state->word_C41A = HL;

  return A;
#else
  // Conv: The original code treats $C41A as a pointer. This points to bytes
  // at $9000..$90FF in the exterior tiles data. This routine fetches a byte
  // from this location and returns it to the caller as a nibble, giving a
  // simple random number source.
  //
  // Poking around in our own innards is likely to be too fragile for the
  // portable C version, but we still want to emulate the game precisely, so
  // we capture all possible values from the original game into the following
  // array and use it as our random data source.

  static const uint32_t packed_nibbles[] =
  {
    0x00000000,
    0x00CBF302,
    0x00C30000,
    0x00000000,
    0x3C0800C3,
    0xC0000000,
    0x00CFD3CF,
    0xDFFFF7FF,
    0xFFDFFFBF,
    0xFDFC3FFF,
    0xFF37C000,
    0xCC003C00,
    0xB4444B80,
    0x34026666,
    0x66643C00,
    0x66666426,
    0x66643FC0,
    0x66642664,
    0xF5310000,
    0x3DDDDDBB,
    0x26666666,
    0x200003FC,
    0x34BC2666,
    0xC82C3426,
    0x3FC26666,
    0x3CFFF3CF,
    0x3DDDDDBB,
    0x43C2DFFB,
    0x3FC3C3F3,
    0xC3730003,
    0xC0477643,
    0x2C34002C,
  };

  int prng_index;
  int row, column;

  assert(state != NULL);

  prng_index = ++state->prng_index; /* Wraps around at 256 */
  row    = prng_index >> 3;
  column = prng_index & 7;

  assert(row < NELEMS(packed_nibbles));

  return (packed_nibbles[row] >> (column * 4)) & 0x0F;
}
#endif

/* ----------------------------------------------------------------------- */

/**
 * $CB98: The hero has been sent to solitary.
 *
 * \param[in] state Pointer to game state.
 */
void solitary(tgestate_t *state)
{
  /**
   * $7AC6
   */
  static const tinypos_t solitary_pos =
  {
    0x3A, 0x2A, 0x18
  };

  /**
   * $CC31: Partial character struct.
   */
  static const uint8_t solitary_hero_reset_data[6] =
  {
    0x00, 0x74, 0x64, 0x03, 0x24, 0x00
  };

  item_t       *pitem;       /* was HL */
  item_t        item;        /* was C */
  uint8_t       iters;       /* was B */
  vischar_t    *vischar;     /* was IY */
  itemstruct_t *pitemstruct; /* was HL */

  assert(state != NULL);

  /* Silence the bell. */
  state->bell = bell_STOP;

  /* Seize the hero's held items. */
  pitem = &state->items_held[0];
  item = *pitem;
  *pitem = item_NONE;
  item_discovered(state, item);

  pitem = &state->items_held[1];
  item = *pitem;
  *pitem = item_NONE;
  item_discovered(state, item);

  draw_all_items(state);

  /* Discover all items. */
  iters = item__LIMIT;
  pitemstruct = &state->item_structs[0];
  do
  {
    /* Is the item outdoors? */
    if ((pitemstruct->room_and_flags & itemstruct_ROOM_MASK) == room_0_OUTDOORS)
    {
      item_t     item_and_flags; /* was A */
      tinypos_t *tinypos;        /* was HL */
      uint8_t    area;           /* was A' */

      item_and_flags = pitemstruct->item_and_flags;
      tinypos = &pitemstruct->pos;
      area = 0; /* iterate 0,1,2 */
      do
        /* If the item is within the camp bounds then it will be discovered.
         */
        if (within_camp_bounds(area, tinypos) == 0)
          goto discovered;
      while (++area != 3);

      goto next;

discovered:
      item_discovered(state, item_and_flags);
    }

next:
    pitemstruct++;
  }
  while (--iters);

  /* Move character to solitary. */
  state->vischars[0].room = room_23_SOLITARY;
  state->current_door = 20;

  decrease_morale(state, 35);

  reset_map_and_characters(state);

  memcpy(&state->character_structs[0].room, &solitary_hero_reset_data, 6);

  queue_message_for_display(state, message_YOU_ARE_IN_SOLITARY);
  queue_message_for_display(state, message_WAIT_FOR_RELEASE);
  queue_message_for_display(state, message_ANOTHER_DAY_DAWNS);

  state->morale_1 = 0xFF; /* inhibit user input */
  state->automatic_player_counter = 0; /* immediately take automatic control of hero */
  state->vischars[0].mi.spriteset = &sprites[sprite_PRISONER_FACING_TOP_LEFT_4]; // $8015 = sprite_prisoner_tl_4;
  vischar = &state->vischars[0];
  state->IY = &state->vischars[0];
  vischar->b0E = 3; // character faces bottom left
  state->vischars[0].target &= 0xFF00; // ($8002) = 0; // target location - why is this storing a byte and not a word?
  transition(state, &solitary_pos);

  NEVER_RETURNS;
}

/* ----------------------------------------------------------------------- */

/**
 * $CC37: Guards follow suspicious character.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character (only ever a supporting character). (was IY / HL)
 */
void guards_follow_suspicious_character(tgestate_t *state,
                                        vischar_t  *vischar)
{
  character_t  character; /* was A */
  tinypos_t   *tinypos;   /* was DE */
  pos_t       *pos;       /* was HL */

  assert(state   != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  /* Conv: Copy of vischar in HL factored out. */

  character = vischar->character;

  /* Wearing the uniform stops anyone but the commandant from following the
   * hero. */
  if (character != character_0_COMMANDANT &&
      state->vischars[0].mi.spriteset == &sprites[sprite_GUARD_FACING_TOP_LEFT_4])
    return;

  // Which is the case here?
  // - Don't follow mad people, or
  // - Don't follow the hero when bribe has been used
  if (vischar->flags == vischar_FLAGS_BIT2) /* 'Gone mad' (bribe) flag */
    return;

  pos = &vischar->mi.pos; /* was HL += 15 */
  tinypos = &state->tinypos_81B2;
  if (state->room_index == room_0_OUTDOORS)
  {
    tinypos_t *hero_map_pos; /* was HL */
    int        dir;        /* Conv */
    uint8_t    b0E;          /* was A / C */

    pos_to_tinypos(pos, tinypos); // tinypos_81B2 becomes vischar pos

    hero_map_pos = &state->hero_map_position;
    /* Conv: Dupe tinypos ptr factored out. */

    b0E = vischar->b0E; // direction thing
    /* Conv: Avoided shifting b0E here. Propagated shift into later ops. */

    if ((b0E & 1) == 0)
    {
      // range check (uses A as temporary)
      if (tinypos->y - 1 >= hero_map_pos->y || tinypos->y + 1 < hero_map_pos->y)
        return;

      dir = tinypos->x < hero_map_pos->x;
      if ((b0E & 2) == 0)
        dir = !dir;
      if (dir)
        return;
    }

    // range check (uses A as temporary)
    if (tinypos->x - 1 >= hero_map_pos->x || tinypos->x + 1 < hero_map_pos->x)
      return;

    dir = tinypos->height < hero_map_pos->height;
    if ((b0E & 2) == 0)
      dir = !dir;
    if (dir)
      return;
  }

  if (!state->red_flag)
  {
    if (vischar->mi.pos.height < 32) /* uses A as temporary */
      vischar->flags = vischar_FLAGS_BIT1; // dog+food flag?
  }
  else
  {
    state->bell = bell_RING_PERPETUAL;
    hostiles_persue(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CCAB: Guards persue prisoners.
 *
 * For all visible, hostile characters, at height < 32, set the bribed/persue
 * flag.
 *
 * \param[in] state Pointer to game state.
 */
void hostiles_persue(tgestate_t *state)
{
  vischar_t *vischar; /* was HL */
  uint8_t    iters;   /* was B */

  assert(state != NULL);

  vischar = &state->vischars[1];
  iters   = vischars_LENGTH - 1;
  do
  {
    if (vischar->character <= character_19_GUARD_DOG_4 &&
        vischar->mi.pos.height < 32) /* Excludes the guards high up in towers. */
      vischar->flags = vischar_FLAGS_BRIBE_PENDING;
    vischar++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $CCCD: Is item discoverable?
 *
 * Searches item_structs for items dropped nearby. If items are found the
 * hostiles are made to persue the hero.
 *
 * Green key and food items are ignored.
 *
 * \param[in] state Pointer to game state.
 */
void is_item_discoverable(tgestate_t *state)
{
  room_t              room;       /* was A */
  const itemstruct_t *itemstruct; /* was HL */
  uint8_t             iters;      /* was B */
  item_t              item;       /* was A */

  assert(state != NULL);

  room = state->room_index;
  if (room != room_0_OUTDOORS)
  {
    if (is_item_discoverable_interior(state, room, NULL) == 0) /* Item found */
      hostiles_persue(state);
  }
  else
  {
    itemstruct = &state->item_structs[0];
    iters      = item__LIMIT;
    do
    {
      if (itemstruct->room_and_flags & itemstruct_ROOM_FLAG_NEARBY_7)
        goto nearby;

next:
      itemstruct++;
    }
    while (--iters);
    return;

nearby:
    item = itemstruct->item_and_flags & itemstruct_ITEM_MASK;
    if (item == item_GREEN_KEY || item == item_FOOD) /* Ignore these items */
      goto next;

    /* Suspected bug in original game appears here: itemstruct pointer is
     * decremented to access item_and_flags but is not re-adjusted
     * afterwards. */

    hostiles_persue(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CCFB: Is an item discoverable indoors?
 *
 * \param[in]  state Pointer to game state.
 * \param[in]  room  Room index. (was A)
 * \param[out] pitem Pointer to item (if found). (Can be NULL). (was C)
 *
 * \return 0 if found, 1 if not found. (Conv: Was Z flag).
 */
int is_item_discoverable_interior(tgestate_t *state,
                                  room_t      room,
                                  item_t     *pitem)
{
  const itemstruct_t *itemstr; /* was HL */
  uint8_t             iters;   /* was B */
  item_t              item;    /* was A */

  assert(state != NULL);
  assert(room >= 0 && room < room__LIMIT);
  assert(pitem != NULL);

  itemstr = &state->item_structs[0];
  iters   = item__LIMIT;
  do
  {
    /* Is the item in the specified room? */
    if ((itemstr->room_and_flags & itemstruct_ROOM_MASK) == room &&
        /* Has the item been moved to a different room? */
        /* Bug? Note that room_and_flags doesn't get its flags masked off.
         * Does it need & 0x3F ? */
        default_item_locations[itemstr->item_and_flags & itemstruct_ITEM_MASK].room_and_flags != room)
    {
      item = itemstr->item_and_flags & itemstruct_ITEM_MASK;

      /* Ignore the red cross parcel. */
      if (item != item_RED_CROSS_PARCEL)
      {
        if (pitem) /* Conv: Added to support return of pitem. */
          *pitem = item;

        return 0; /* found */
      }
    }

    itemstr++;
  }
  while (--iters);

  return 1; /* not found */
}

/* ----------------------------------------------------------------------- */

/**
 * $CD31: Item discovered.
 *
 * \param[in] state Pointer to game state.
 * \param[in] item  Item. (was C)
 */
void item_discovered(tgestate_t *state, item_t item)
{
  const default_item_location_t *default_item_location; /* was HL/DE */
  room_t                         room;                  /* was A */
  itemstruct_t                  *itemstruct;            /* was HL/DE */

  assert(state != NULL);

  if (item == item_NONE)
    return;

  assert(item >= 0 && item < item__LIMIT);

  item &= itemstruct_ITEM_MASK;

  queue_message_for_display(state, message_ITEM_DISCOVERED);
  decrease_morale(state, 5);

  default_item_location = &default_item_locations[item];
  room = default_item_location->room_and_flags;

  /* Bug: 'item' is not masked, so goes out of range. */
  itemstruct = item_to_itemstruct(state, item);
  itemstruct->item_and_flags &= ~itemstruct_ITEM_FLAG_HELD;

  itemstruct->room_and_flags = default_item_location->room_and_flags;
  itemstruct->pos.x = default_item_location->x;
  itemstruct->pos.y = default_item_location->y;

  if (room == room_0_OUTDOORS) /* Only gets hit for the green key. */
  {
    /* Conv: The original code just assigned A/'room' here, as it's already zero. */
    itemstruct->pos.height = 0;
    drop_item_tail_exterior(itemstruct);
  }
  else
  {
    itemstruct->pos.height = 5;
    drop_item_tail_interior(itemstruct);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CD6A: Default item locations.
 */

#define ITEM_ROOM(item_no, flags) ((item_no & 0x3F) | (flags << 6))

const default_item_location_t default_item_locations[item__LIMIT] =
{
  { ITEM_ROOM(room_NONE,        3), 0x40, 0x20 }, /* item_WIRESNIPS        */
  { ITEM_ROOM(room_9_CRATE,     0), 0x3E, 0x30 }, /* item_SHOVEL           */
  { ITEM_ROOM(room_10_LOCKPICK, 0), 0x49, 0x24 }, /* item_LOCKPICK         */
  { ITEM_ROOM(room_11_PAPERS,   0), 0x2A, 0x3A }, /* item_PAPERS           */
  { ITEM_ROOM(room_14_TORCH,    0), 0x32, 0x18 }, /* item_TORCH            */
  { ITEM_ROOM(room_NONE,        0), 0x24, 0x2C }, /* item_BRIBE            */
  { ITEM_ROOM(room_15_UNIFORM,  0), 0x2C, 0x41 }, /* item_UNIFORM          */
  { ITEM_ROOM(room_19_FOOD,     0), 0x40, 0x30 }, /* item_FOOD             */
  { ITEM_ROOM(room_1_HUT1RIGHT, 0), 0x42, 0x34 }, /* item_POISON           */
  { ITEM_ROOM(room_22_REDKEY,   0), 0x3C, 0x2A }, /* item_RED_KEY          */
  { ITEM_ROOM(room_11_PAPERS,   0), 0x1C, 0x22 }, /* item_YELLOW_KEY       */
  { ITEM_ROOM(room_0_OUTDOORS,  0), 0x4A, 0x48 }, /* item_GREEN_KEY        */
  { ITEM_ROOM(room_NONE,        0), 0x1C, 0x32 }, /* item_RED_CROSS_PARCEL */
  { ITEM_ROOM(room_18_RADIO,    0), 0x24, 0x3A }, /* item_RADIO            */
  { ITEM_ROOM(room_NONE,        0), 0x1E, 0x22 }, /* item_PURSE            */
  { ITEM_ROOM(room_NONE,        0), 0x34, 0x1C }, /* item_COMPASS          */
};

#undef ITEM_ROOM

/* ----------------------------------------------------------------------- */

/* Conv: Moved to precede parent array. */

static const uint8_t _cf06[] = { 0x01,0x04,0x04,0xFF, 0x00,0x00,0x00,0x0A };
static const uint8_t _cf0e[] = { 0x01,0x05,0x05,0xFF, 0x00,0x00,0x00,0x8A };
static const uint8_t _cf16[] = { 0x01,0x06,0x06,0xFF, 0x00,0x00,0x00,0x88 };
static const uint8_t _cf1e[] = { 0x01,0x07,0x07,0xFF, 0x00,0x00,0x00,0x08 };
static const uint8_t _cf26[] = { 0x04,0x00,0x00,0x02, 0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x01,0x02,0x00,0x00,0x02,0x02,0x00,0x00,0x03 };
static const uint8_t _cf3a[] = { 0x04,0x01,0x01,0x03, 0x00,0x02,0x00,0x80,0x00,0x02,0x00,0x81,0x00,0x02,0x00,0x82,0x00,0x02,0x00,0x83 };
static const uint8_t _cf4e[] = { 0x04,0x02,0x02,0x00, 0xFE,0x00,0x00,0x04,0xFE,0x00,0x00,0x05,0xFE,0x00,0x00,0x06,0xFE,0x00,0x00,0x07 };
static const uint8_t _cf62[] = { 0x04,0x03,0x03,0x01, 0x00,0xFE,0x00,0x84,0x00,0xFE,0x00,0x85,0x00,0xFE,0x00,0x86,0x00,0xFE,0x00,0x87 };
static const uint8_t _cf76[] = { 0x01,0x00,0x00,0xFF, 0x00,0x00,0x00,0x00 };
static const uint8_t _cf7e[] = { 0x01,0x01,0x01,0xFF, 0x00,0x00,0x00,0x80 };
static const uint8_t _cf86[] = { 0x01,0x02,0x02,0xFF, 0x00,0x00,0x00,0x04 };
static const uint8_t _cf8e[] = { 0x01,0x03,0x03,0xFF, 0x00,0x00,0x00,0x84 };
static const uint8_t _cf96[] = { 0x02,0x00,0x01,0xFF, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80 };
static const uint8_t _cfa2[] = { 0x02,0x01,0x02,0xFF, 0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x04 };
static const uint8_t _cfae[] = { 0x02,0x02,0x03,0xFF, 0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x84 };
static const uint8_t _cfba[] = { 0x02,0x03,0x00,0xFF, 0x00,0x00,0x00,0x84,0x00,0x00,0x00,0x00 };
static const uint8_t _cfc6[] = { 0x02,0x04,0x04,0x02, 0x02,0x00,0x00,0x0A,0x02,0x00,0x00,0x0B };
static const uint8_t _cfd2[] = { 0x02,0x05,0x05,0x03, 0x00,0x02,0x00,0x8A,0x00,0x02,0x00,0x8B };
static const uint8_t _cfde[] = { 0x02,0x06,0x06,0x00, 0xFE,0x00,0x00,0x88,0xFE,0x00,0x00,0x89 };
static const uint8_t _cfea[] = { 0x02,0x07,0x07,0x01, 0x00,0xFE,0x00,0x08,0x00,0xFE,0x00,0x09 };
static const uint8_t _cff6[] = { 0x02,0x04,0x05,0xFF, 0x00,0x00,0x00,0x0A,0x00,0x00,0x00,0x8A };
static const uint8_t _d002[] = { 0x02,0x05,0x06,0xFF, 0x00,0x00,0x00,0x8A,0x00,0x00,0x00,0x88 };
static const uint8_t _d00e[] = { 0x02,0x06,0x07,0xFF, 0x00,0x00,0x00,0x88,0x00,0x00,0x00,0x08 };
static const uint8_t _d01a[] = { 0x02,0x07,0x04,0xFF, 0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x0A };

/**
 * $CDF2: (unknown)
 */
const uint8_t *character_related_pointers[24] =
{
  _cf26,
  _cf3a,
  _cf4e,
  _cf62,
  _cf96,
  _cfa2,
  _cfae,
  _cfba,
  _cf76,
  _cf7e,
  _cf86,
  _cf8e,
  _cfc6,
  _cfd2,
  _cfde,
  _cfea,
  _cff6,
  _d002,
  _d00e,
  _d01a,
  _cf06,
  _cf0e,
  _cf16,
  _cf1e,
};

/* ----------------------------------------------------------------------- */

/**
 * $DB9E: Mark nearby items.
 *
 * Iterates over item structs, testing to see if each item is within range
 * (-1..22, 0..15) of the map position.
 *
 * This is similar to is_item_discoverable_interior in that it iterates over
 * all item_structs.
 *
 * \param[in] state Pointer to game state.
 */
void mark_nearby_items(tgestate_t *state)
{
  room_t        room;         /* was C */
  uint8_t       map_y, map_x; /* was D, E */
  uint8_t       iters;        /* was B */
  itemstruct_t *itemstruct;   /* was HL */

  assert(state != NULL);

  room = state->room_index;
  if (room == room_NONE)
    room = room_0_OUTDOORS;

  map_x = state->map_position[0];
  map_y = state->map_position[1];

  iters      = item__LIMIT;
  itemstruct = &state->item_structs[0];
  do
  {
    uint8_t t1, t2;

    t1 = (itemstruct->target & 0x00FF) >> 0;
    t2 = (itemstruct->target & 0xFF00) >> 8;

    /* Conv: Ranges adjusted. */
    if ((itemstruct->room_and_flags & itemstruct_ROOM_MASK) == room &&
        (t1 >= map_x - 1 && t1 <= map_x + 25 - 1) &&
        (t2 >= map_y     && t2 <= map_y + 17    ))
      itemstruct->room_and_flags |= itemstruct_ROOM_FLAG_NEARBY_6 | itemstruct_ROOM_FLAG_NEARBY_7; /* set */
    else
      itemstruct->room_and_flags &= ~(itemstruct_ROOM_FLAG_NEARBY_6 | itemstruct_ROOM_FLAG_NEARBY_7); /* reset */

    itemstruct++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $DBEB: Iterates over all item_structs looking for nearby items.
 *
 * Returns the furthest/highest/nearest item?
 *
 * Leaf.
 *
 * \param[in]  state    Pointer to game state.
 * \param[in]  x        X pos? Compared to X. (was BC') // sampled BCdash = $26, $3E, $00, $26, $34, $22, $32
 * \param[in]  y        Y pos? Compared to Y. (was DE')
 * \param[out] pitemstr Returned pointer to item struct. (was IY)
 *
 * \return item+flag. (was Adash)
 */
uint8_t get_greatest_itemstruct(tgestate_t    *state,
                                item_t         item_and_flag,
                                uint16_t       x,
                                uint16_t       y,
                                itemstruct_t **pitemstr)
{
  uint8_t             iters;   /* was B */
  const itemstruct_t *itemstr; /* was HL */

  assert(state    != NULL);
  // assert(item_and_flag);
  // assert(x);
  // assert(y);
  assert(pitemstr != NULL);

  *pitemstr = NULL; /* Conv: Added safety initialisation. */

  iters   = item__LIMIT;
  itemstr = &state->item_structs[0]; /* Conv: Original pointed to itemstruct->room_and_flags. */
  do
  {
    const enum itemstruct_flags FLAGS = itemstruct_ROOM_FLAG_NEARBY_6 |
                                        itemstruct_ROOM_FLAG_NEARBY_7;

    if ((itemstr->room_and_flags & FLAGS) == FLAGS)
    {
      // Conv: Original calls out to multiply by 8, HLdash is temp.
      if (itemstr->pos.x * 8 > x &&
          itemstr->pos.y * 8 > y)
      {
        const tinypos_t *pos; /* was HLdash */

        pos = &itemstr->pos; /* Conv: Was direct pointer, now points to member. */
        /* Get these for the next loop iteration. */
        y = pos->y * 8; // y
        x = pos->x * 8; // x
        *pitemstr = (itemstruct_t *) itemstr;

//        state->IY = itemstr; // TODO: Work out all the other places which assign IY.

        /* The original code has an unpaired A register exchange here. If the
         * loop continues then it's unclear which output register is used. */
        /* It seems that A' is the output register, irresspective. */
        item_and_flag = (item__LIMIT - iters) | (1 << 6); // iteration count + 'item found' flag? mysteryflagconst874
      }
    }
    itemstr++;
  }
  while (--iters);

  return item_and_flag;
}

/* ----------------------------------------------------------------------- */

/**
 * $DC41: Looks like it sets up item plotting.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] itemstr Pointer to itemstruct. (was IY)
 * \param[in] item    Item. (was A)
 *
 * \return Z
 */
uint8_t setup_item_plotting(tgestate_t   *state,
                            itemstruct_t *itemstr,
                            item_t        item)
{
//  location_t   *HL; /* was HL */
//  tinypos_t    *DE; /* was DE */
//  uint16_t      BC; /* was BC */
  uint16_t      clipped_width;  /* was BC */
  uint16_t      clipped_height; /* was DE */
  uint8_t       instr;          /* was A */
  uint8_t       A;              /* was A */
  uint8_t       offset_tmp;     /* was A' */
  uint8_t       iters;          /* was B' */
  uint8_t       offset;         /* was C' */
//  uint16_t      DEdash; /* was DE' */
  const size_t *enables;        /* was HL' */
  int16_t       x, y;           // was HL, DE // signed needed for X calc
  uint8_t      *maskbuf;        /* was HL */
  uint16_t      skip;           /* was DE */

  assert(state   != NULL);
  assert(itemstr != NULL); // will need ASSERT_ITEMSTRUCT_VALID
  assert(item >= 0 && item < item__LIMIT);

  /* 0x3F looks like it ought to be 0x1F (item__LIMIT - 1). Potential bug: The use of A later on does not re-clamp it to 0x1F. */
  item &= 0x3F; // mask off mysteryflagconst874

  /* Bug: The original game writes to this location but it's never
   * subsequently read from (a memory breakpoint set in FUSE confirmed this).
   */
  /* $8213 = A; */

  state->tinypos_81B2           = itemstr->pos;
  // map_position_related_x/y could well be a location_t
  state->map_position_related_x = itemstr->target & 0xFF;
  state->map_position_related_y = itemstr->target >> 8;
// after LDIR we have:
//  HL = &IY->pos.x + 5;
//  DE = &state->tinypos_81B2 + 5;
//  BC = 0;
//
//  // EX DE, HL
//
//  *DE = 0; // was B, which is always zero here
  state->flip_sprite    = 0; /* Items are never drawn flipped. */

  state->item_height    = item_definitions[item].height;
  state->bitmap_pointer = item_definitions[item].bitmap;
  state->mask_pointer   = item_definitions[item].mask;
  if (item_visible(state, &clipped_width, &clipped_height) != 0)
    return 1; /* invisible */ // NZ

  // PUSH clipped_width
  // PUSH clipped_height

  state->self_E2C2 = clipped_height & 0xFF; // self modify masked_sprite_plotter_16_wide_left

  if ((clipped_width >> 8) == 0)
  {
    instr = 0x77; /* opcode of 'LD (HL),A' */
    offset_tmp = clipped_width & 0xFF;
  }
  else
  {
    instr = 0; /* opcode of 'LD (HL),A' */
    offset_tmp = 3 - (clipped_width & 0xFF);
  }

  offset = offset_tmp; /* Future: Could assign directly to offset instead. */

  /* Set the addresses in the jump table to NOP or LD (HL),A */
  enables = &masked_sprite_plotter_16_enables[0];
  iters = 3; /* iterations */
  do
  {
    *(((uint8_t *) state) + *enables++) = instr;
    *(((uint8_t *) state) + *enables++) = instr;
    if (--offset == 0)
      instr ^= 0x77; /* Toggle between LD (HL),A and NOP. */
  }
  while (--iters);

  y = 0; /* Conv: Moved. */
  if ((clipped_height >> 8) == 0)
    y = (state->map_position_related_y - state->map_position[1]) * 192; // temp: HL

  x = state->map_position_related_x - state->map_position[0]; // X axis

  state->screen_pointer = &state->window_buf[x + y]; // screen buffer start address
  ASSERT_SCREEN_PTR_VALID(state->screen_pointer);

  maskbuf = &state->mask_buffer[0];

  // POP DE  // get clipped_height
  // PUSH DE

  state->foreground_mask_pointer = &maskbuf[(clipped_height >> 8) * 4];
  ASSERT_MASK_BUF_PTR_VALID(state->foreground_mask_pointer);

  // POP DE  // this pop/push seems to be needless - DE already has the value
  // PUSH DE

  A = clipped_height >> 8; // A = D // sign?
  if (A)
  {
    /* The original game has a generic multiply loop here which is setup to
     * only ever multiply by two. A duplicate of the equivalent portion in
     * setup_vischar_plotting. In this version instead we'll just multiply by
     * two.
     *
     * {
     *   uint8_t D, E;
     *
     *   D = A;
     *   A = 0;
     *   E = 3; // width_bytes
     *   E--;   // width_bytes - 1
     *   do
     *     A += E;
     *   while (--D);
     * }
     */

    A *= 2;
    // clipped_height &= 0x00FF; // D = 0
  }

  /* D ends up as zero either way. */

  skip = A;
  state->bitmap_pointer += skip;
  state->mask_pointer   += skip;

  // POP BC
  // POP DE

  return 0; // Z
}

/* ----------------------------------------------------------------------- */

/**
 * $DD02: Clipping items to the game window.
 *
 * Counterpart to \ref vischar_visible.
 *
 * Called by setup_item_plotting only.
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0 => visible, 1 => invisible (was A)
 */
uint8_t item_visible(tgestate_t *state,
                     uint16_t   *clipped_width,
                     uint16_t   *clipped_height)
{
  const uint8_t *px;            /* was HL */
  uint16_t       map_position;  /* was DE */
  uint8_t        xoff;          /* was A */
  uint8_t        yoff;          /* was A */
  uint16_t       width;         /* was BC */
  uint16_t       height;        /* was DE */

  assert(state          != NULL);
  assert(clipped_width  != NULL);
  assert(clipped_height != NULL);

  /* Width part */

  // Seems to be doing (map_position_x + state->tb_columns <= px[0])
  px = &state->map_position_related_x;
  map_position = state->map_position[0] | (state->map_position[1] << 8); // Conv: Was a fused load

  xoff = (map_position & 0xFF) + state->tb_columns - px[0];
  if (xoff > 0)
  {
#define WIDTH_BYTES 3
    if (xoff < WIDTH_BYTES)
    {
      width = xoff;
    }
    else
    {
      xoff = px[0] + WIDTH_BYTES - (map_position & 0xFF);
      if (xoff <= 0)
        goto invisible; // off the left hand side?

      if (xoff < WIDTH_BYTES)
        width = ((WIDTH_BYTES - xoff) << 8) | xoff;
      else
        width = WIDTH_BYTES;
    }

    /* Height part */

    yoff = (map_position >> 8) + state->tb_rows - px[1]; // map_position_related_y
    if (yoff > 0)
    {
#define HEIGHT 2
      if (yoff < HEIGHT)
      {
        height = 8;
      }
      else
      {
        yoff = px[1] + HEIGHT - (map_position >> 8);
        if (yoff <= 0)
          goto invisible;

        if (yoff < HEIGHT)
          height = (8 << 8) | (state->item_height - 8);
        else
          height = state->item_height;
      }

      *clipped_width  = width;
      *clipped_height = height;

      return 0; /* item is visible */
    }
  }

invisible:
  return 1; /* item is not visible */
}

/* ----------------------------------------------------------------------- */

/**
 * $DD7D: Item definitions.
 *
 * Used by draw_item and setup_item_plotting.
 */
const sprite_t item_definitions[item__LIMIT] =
{
  { 2, 11, bitmap_wiresnips, mask_wiresnips },
  { 2, 13, bitmap_shovel,    mask_shovelkey },
  { 2, 16, bitmap_lockpick,  mask_lockpick  },
  { 2, 15, bitmap_papers,    mask_papers    },
  { 2, 12, bitmap_torch,     mask_torch     },
  { 2, 13, bitmap_bribe,     mask_bribe     },
  { 2, 16, bitmap_uniform,   mask_uniform   },
  { 2, 16, bitmap_food,      mask_food      },
  { 2, 16, bitmap_poison,    mask_poison    },
  { 2, 13, bitmap_key,       mask_shovelkey },
  { 2, 13, bitmap_key,       mask_shovelkey },
  { 2, 13, bitmap_key,       mask_shovelkey },
  { 2, 16, bitmap_parcel,    mask_parcel    },
  { 2, 16, bitmap_radio,     mask_radio     },
  { 2, 12, bitmap_purse,     mask_purse     },
  { 2, 12, bitmap_compass,   mask_compass   },
};

/* ----------------------------------------------------------------------- */

/**
 * $E0E0: Addresses (offsets) of simulated instruction locations.
 *
 * Used by setup_item_plotting and setup_vischar_plotting. [setup_item_plotting: items are always 16 wide].
 */
const size_t masked_sprite_plotter_16_enables[2 * 3] =
{
  offsetof(tgestate_t, enable_E319),
  offsetof(tgestate_t, enable_E3C5),
  offsetof(tgestate_t, enable_E32A),
  offsetof(tgestate_t, enable_E3D6),
  offsetof(tgestate_t, enable_E340),
  offsetof(tgestate_t, enable_E3EC),
};

/* ----------------------------------------------------------------------- */

#define MASK(bm,mask) ((~*foremaskptr | (mask)) & *screenptr) | ((bm) & *foremaskptr)

/**
 * $E102: Sprite plotter. Used for characters and objects.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void masked_sprite_plotter_24_wide(tgestate_t *state, vischar_t *vischar)
{
  uint8_t        x;           /* was A */
  uint8_t        iters;       /* was B */
  const uint8_t *maskptr;     /* was ? */
  const uint8_t *bitmapptr;   /* was ? */
  const uint8_t *foremaskptr; /* was ? */
  uint8_t       *screenptr;   /* was ? */

  assert(state   != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  if ((x = (vischar->scrx & 7)) < 4)
  {
    uint8_t self_E161, self_E143;

    /* Shift right? */

    x = (~x & 3) * 8; // jump table offset (on input, A is 0..3)

    self_E161 = x; // self-modify: jump into mask rotate
    self_E143 = x; // self-modify: jump into bitmap rotate

    maskptr   = state->mask_pointer;
    bitmapptr = state->bitmap_pointer;

    assert(maskptr   != NULL);
    assert(bitmapptr != NULL);

    iters = state->self_E121; // clipped_height & 0xFF
    do
    {
      uint8_t bm0, bm1, bm2, bm3;         // was B, C, E, D
      uint8_t mask0, mask1, mask2, mask3; // was B', C', E', D'
      int     carry;

      /* Load bitmap bytes into B,C,E. */
      bm0 = *bitmapptr++;
      bm1 = *bitmapptr++;
      bm2 = *bitmapptr++;

      /* Load mask bytes into B',C',E'. */
      mask0 = *maskptr++;
      mask1 = *maskptr++;
      mask2 = *maskptr++;

      if (state->flip_sprite & (1<<7)) // ToDo: Hoist out this constant flag.
        flip_24_masked_pixels(state, &mask2, &mask1, &mask0, &bm2, &bm1, &bm0);

      foremaskptr = state->foreground_mask_pointer;
      screenptr   = state->screen_pointer; // moved compared to the other routines

      ASSERT_MASK_BUF_PTR_VALID(foremaskptr);
      ASSERT_SCREEN_PTR_VALID(screenptr);

      /* Shift bitmap. */

      bm3 = 0;
      // Conv: Replaced self-modified goto with if-else chain.
      if (self_E143 >= 0)
      {
        SRL(bm0);
        RR(bm1);
        RR(bm2);
        RR(bm3);
      }
      if (self_E143 >= 8)
      {
        SRL(bm0);
        RR(bm1);
        RR(bm2);
        RR(bm3);
      }
      if (self_E143 >= 16)
      {
        SRL(bm0);
        RR(bm1);
        RR(bm2);
        RR(bm3);
      }

      /* Shift mask. */

      mask3 = 0xFF;
      carry = 1;
      // Conv: Replaced self-modified goto with if-else chain.
      if (self_E161 >= 0)
      {
        RR(mask0);
        RR(mask1);
        RR(mask2);
        RR(mask3);
      }
      if (self_E161 >= 0)
      {
        RR(mask0);
        RR(mask1);
        RR(mask2);
        RR(mask3);
      }
      if (self_E161 >= 0)
      {
        RR(mask0);
        RR(mask1);
        RR(mask2);
        RR(mask3);
      }

      /* Plot, using foreground mask. */

      x = MASK(bm0, mask0);
      foremaskptr++;
      if (state->enable_E188)
        *screenptr++ = x;

      x = MASK(bm0, mask1);
      foremaskptr++;
      if (state->enable_E199)
        *screenptr++ = x;

      x = MASK(bm0, mask2);
      foremaskptr++;
      if (state->enable_E1AA)
        *screenptr++ = x;

      x = MASK(bm0, mask3);
      foremaskptr++;
      state->foreground_mask_pointer = foremaskptr;
      if (state->enable_E1BF)
        *screenptr = x;

      screenptr += state->tb_columns - 3;
      state->screen_pointer = screenptr;
    }
    while (--iters);
  }
  else
  {
    uint8_t self_E22A, self_E204;

    /* Shift left? */

    x -= 4; // (on input, A is 4..7)
    x = (x << 3) | (x >> 5); // was 3 x RLCA

    self_E22A = x; // self-modify: jump into mask rotate
    self_E204 = x; // self-modify: jump into bitmap rotate

    maskptr   = state->mask_pointer;
    bitmapptr = state->bitmap_pointer;

    assert(maskptr   != NULL);
    assert(bitmapptr != NULL);

    iters = state->self_E1E2; // clipped_height & 0xFF
    do
    {
      /* Note the different variable order to the case above. */
      uint8_t bm0, bm1, bm2, bm3;         // was E, C, B, D
      uint8_t mask0, mask1, mask2, mask3; // was E', C', B', D'
      int     carry;

      /* Load bitmap bytes into B,C,E. */
      bm2 = *bitmapptr++;
      bm1 = *bitmapptr++;
      bm0 = *bitmapptr++;

      /* Load mask bytes into B',C',E'. */
      mask2 = *maskptr++;
      mask1 = *maskptr++;
      mask0 = *maskptr++;

      if (state->flip_sprite & (1<<7)) // ToDo: Hoist out this constant flag.
        flip_24_masked_pixels(state, &mask0, &mask1, &mask2, &bm0, &bm1, &bm2);

      foremaskptr = state->foreground_mask_pointer;
      screenptr   = state->screen_pointer;

      ASSERT_MASK_BUF_PTR_VALID(foremaskptr);
      ASSERT_SCREEN_PTR_VALID(screenptr);

      /* Shift bitmap. */

      bm3 = 0;
      // Conv: Replaced self-modified goto with if-else chain.
      if (self_E204 >= 0)
      {
        SLA(bm0);
        RL(bm1);
        RL(bm2);
        RL(bm3);
      }
      if (self_E204 >= 8)
      {
        SLA(bm0);
        RL(bm1);
        RL(bm2);
        RL(bm3);
      }
      if (self_E204 >= 16)
      {
        SLA(bm0);
        RL(bm1);
        RL(bm2);
        RL(bm3);
      }
      if (self_E204 >= 24)
      {
        SLA(bm0);
        RL(bm1);
        RL(bm2);
        RL(bm3);
      }

      /* Shift mask. */

      mask3 = 0xFF;
      carry = 1;
      // Conv: Replaced self-modified goto with if-else chain.
      if (self_E22A >= 0)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }
      if (self_E22A >= 8)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }
      if (self_E22A >= 16)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }
      if (self_E22A >= 24)
      {
        RL(mask0);
        RL(mask1);
        RL(mask2);
        RL(mask3);
      }

      /* Plot, using foreground mask. */

      x = MASK(bm3, mask3);
      foremaskptr++;
      if (state->enable_E259)
        *screenptr = x;
      screenptr++;

      x = MASK(bm2, mask2);
      foremaskptr++;
      if (state->enable_E26A)
        *screenptr = x;
      screenptr++;

      x = MASK(bm1, mask1);
      foremaskptr++;
      if (state->enable_E27B)
        *screenptr = x;
      screenptr++;

      x = MASK(bm0, mask0);
      foremaskptr++;
      state->foreground_mask_pointer = foremaskptr;
      if (state->enable_E290)
        *screenptr = x;
      screenptr++;

      screenptr += state->tb_columns - 3;
      state->screen_pointer = screenptr;
    }
    while (--iters);
  }
}

/**
 * $E29F: Entry point for masked_sprite_plotter_16_wide_left which assumes A == 0. (+ no vischar passed).
 *
 * \param[in] state Pointer to game state.
 */
void masked_sprite_plotter_16_wide_searchlight(tgestate_t *state)
{
  masked_sprite_plotter_16_wide_left(state, 0 /* x */);
}

/**
 * $E2A2: Sprite plotter. Used for characters and objects.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void masked_sprite_plotter_16_wide(tgestate_t *state, vischar_t *vischar)
{
  uint8_t x;

  ASSERT_VISCHAR_VALID(vischar);

  if ((x = (vischar->scrx & 7)) < 4)
    masked_sprite_plotter_16_wide_right(state, x);
  else
    masked_sprite_plotter_16_wide_left(state, x); /* i.e. fallthrough */
}

/**
 * $E2AC: Sprite plotter. Shifts left/right (unsure).
 *
 * \param[in] state Pointer to game state.
 * \param[in] x     X offset. (was A)
 */
void masked_sprite_plotter_16_wide_left(tgestate_t *state, uint8_t x)
{
  uint8_t        iters;       /* was B */
  const uint8_t *maskptr;     /* was ? */
  const uint8_t *bitmapptr;   /* was ? */
  const uint8_t *foremaskptr; /* was ? */
  uint8_t       *screenptr;   /* was ? */
  uint8_t        self_E2DC, self_E2F4;

  assert(state != NULL);
  // assert(x);

  x = (~x & 3) * 6; // jump table offset (on input, A is 0..3 => 3..0) // 6 = length of asm chunk

  self_E2DC = x; // self-modify: jump into mask rotate
  self_E2F4 = x; // self-modify: jump into bitmap rotate

  maskptr   = state->mask_pointer;
  bitmapptr = state->bitmap_pointer;

  assert(maskptr   != NULL);
  assert(bitmapptr != NULL);

  iters = state->self_E2C2; // (clipped height & 0xFF) // self modified by $E49D (setup_vischar_plotting)
  do
  {
    uint8_t bm0, bm1, bm2;       // was D, E, C
    uint8_t mask0, mask1, mask2; // was D', E', C'
    int     carry;

    /* Load bitmap bytes into D,E. */
    bm0 = *bitmapptr++;
    bm1 = *bitmapptr++;

    /* Load mask bytes into D',E'. */
    mask0 = *maskptr++;
    mask1 = *maskptr++;

    if (state->flip_sprite & (1<<7)) // ToDo: Hoist out this constant flag.
      flip_16_masked_pixels(state, &mask0, &mask1, &bm0, &bm1);

    // I'm assuming foremaskptr to be a foreground mask pointer based on it being
    // incremented by four each step, like a supertile wide thing.
    foremaskptr = state->foreground_mask_pointer;
    ASSERT_MASK_BUF_PTR_VALID(foremaskptr);

    // 24 version does bitmap rotates then mask rotates.
    // This is the opposite way around to save a bank switch?

    /* Shift mask. */

    mask2 = 0xFF; // all bits set => mask OFF (that would match the observed stored mask format)
    carry = 1; // mask OFF
    // Conv: Replaced self-modified goto with if-else chain.
    if (self_E2DC >= 0)
    {
      RR(mask0);
      RR(mask1);
      RR(mask2);
    }
    if (self_E2DC >= 6)
    {
      RR(mask0);
      RR(mask1);
      RR(mask2);
    }
    if (self_E2DC >= 12)
    {
      RR(mask0);
      RR(mask1);
      RR(mask2);
    }

    /* Shift bitmap. */

    bm2 = 0; // all bits clear => pixels OFF
    //carry = 0; in original code but never read in practice
    // Conv: Replaced self-modified goto with if-else chain.
    if (self_E2F4 >= 0)
    {
      SRL(bm0);
      RR(bm1);
      RR(bm2);
    }
    if (self_E2F4 >= 6)
    {
      SRL(bm0);
      RR(bm1);
      RR(bm2);
    }
    if (self_E2F4 >= 12)
    {
      SRL(bm0);
      RR(bm1);
      RR(bm2);
    }

    /* Plot, using foreground mask. */

    screenptr = state->screen_pointer; // moved relative to the 24 version
    ASSERT_SCREEN_PTR_VALID(screenptr);

    x = MASK(bm0, mask0);
    foremaskptr++;
    if (state->enable_E319)
      *screenptr = x;
    screenptr++;

    x = MASK(bm1, mask1);
    foremaskptr++;
    if (state->enable_E32A)
      *screenptr = x;
    screenptr++;

    x = MASK(bm2, mask2);
    foremaskptr += 2;
    state->foreground_mask_pointer = foremaskptr;
    if (state->enable_E340)
      *screenptr = x;

    screenptr += state->tb_columns - 2;
    state->screen_pointer = screenptr;
  }
  while (--iters);
}

/**
 * $E34E: Sprite plotter. Shifts left/right (unsure). Used for characters and objects. Counterpart of above routine.
 *
 * Only called by masked_sprite_plotter_16_wide.
 *
 * \param[in] state Pointer to game state.
 * \param[in] x     X offset. (was A)
 */
void masked_sprite_plotter_16_wide_right(tgestate_t *state, uint8_t x)
{
  uint8_t        iters;       /* was B */
  const uint8_t *maskptr;     /* was ? */
  const uint8_t *bitmapptr;   /* was ? */
  const uint8_t *foremaskptr; /* was ? */
  uint8_t       *screenptr;   /* was ? */
  uint8_t        self_E39A, self_E37D;

  assert(state != NULL);
  // assert(x);

  x = (x - 4) * 6; // jump table offset (on input, 'x' is 4..7 => 0..3) // 6 = length of asm chunk

  self_E39A = x; // self-modify: jump into bitmap rotate
  self_E37D = x; // self-modify: jump into mask rotate

  maskptr   = state->mask_pointer;
  bitmapptr = state->bitmap_pointer;

  assert(maskptr   != NULL);
  assert(bitmapptr != NULL);

  iters = state->self_E363; // (clipped height & 0xFF) // self modified by $E49D (setup_vischar_plotting)
  do
  {
    /* Note the different variable order to the 'left' case above. */
    uint8_t bm0, bm1, bm2;       // was E, D, C
    uint8_t mask0, mask1, mask2; // was E', D', C'
    int     carry;

    /* Load bitmap bytes into D,E. */
    bm1 = *bitmapptr++;
    bm0 = *bitmapptr++;

    /* Load mask bytes into D',E'. */
    mask1 = *maskptr++;
    mask0 = *maskptr++;

    if (state->flip_sprite & (1<<7)) // ToDo: Hoist out this constant flag.
      flip_16_masked_pixels(state, &mask1, &mask0, &bm1, &bm0);

    foremaskptr = state->foreground_mask_pointer;
    ASSERT_MASK_BUF_PTR_VALID(foremaskptr);

    /* Shift mask. */

    mask2 = 0xFF; // all bits set => mask OFF (that would match the observed stored mask format)
    carry = 1; // mask OFF
    // Conv: Replaced self-modified goto with if-else chain.
    if (self_E39A >= 0)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }
    if (self_E39A >= 6)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }
    if (self_E39A >= 12)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }
    if (self_E39A >= 18)
    {
      RL(mask0);
      RL(mask1);
      RL(mask2);
    }

    /* Shift bitmap. */

    bm2 = 0; // all bits clear => pixels OFF
    // no carry reset in this variant?
    if (self_E37D >= 0)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }
    if (self_E37D >= 6)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }
    if (self_E37D >= 12)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }
    if (self_E37D >= 18)
    {
      SLA(bm0);
      RL(bm1);
      RL(bm2);
    }

    /* Plot, using foreground mask. */

    screenptr = state->screen_pointer; // this line is moved relative to the 24 version
    ASSERT_SCREEN_PTR_VALID(screenptr);

    x = MASK(bm2, mask2);
    foremaskptr++;
    if (state->enable_E3C5)
      *screenptr = x;
    screenptr++;

    x = MASK(bm1, mask1);
    foremaskptr++;
    if (state->enable_E3D6)
      *screenptr = x;
    screenptr++;

    x = MASK(bm0, mask0);
    foremaskptr += 2;
    state->foreground_mask_pointer = foremaskptr;
    if (state->enable_E3EC)
      *screenptr = x;

    screenptr += 22; // stride (24 - 2)
    state->screen_pointer = screenptr;
  }
  while (--iters);
}

/**
 * $E3FA: Reverses the 24 pixels in E,C,B and E',C',B'.
 *
 * \param[in,out] state  Pointer to game state.
 * \param[in,out] pE     Pointer to pixels/mask.
 * \param[in,out] pC     Pointer to pixels/mask.
 * \param[in,out] pB     Pointer to pixels/mask.
 * \param[in,out] pEdash Pointer to pixels/mask.
 * \param[in,out] pCdash Pointer to pixels/mask.
 * \param[in,out] pBdash Pointer to pixels/mask.
 */
void flip_24_masked_pixels(tgestate_t *state,
                           uint8_t    *pE,
                           uint8_t    *pC,
                           uint8_t    *pB,
                           uint8_t    *pEdash,
                           uint8_t    *pCdash,
                           uint8_t    *pBdash)
{
  const uint8_t *HL;
  uint8_t        E, C, B;

  assert(state  != NULL);
  assert(pE     != NULL);
  assert(pC     != NULL);
  assert(pB     != NULL);
  assert(pEdash != NULL);
  assert(pCdash != NULL);
  assert(pBdash != NULL);

  /* Conv: Routine was much simplified over the original code. */

  HL = &state->reversed[0];

  B = HL[*pE];
  E = HL[*pB];
  C = HL[*pC];

  *pB = B;
  *pE = E;
  *pC = C;

  B = HL[*pEdash];
  E = HL[*pBdash];
  C = HL[*pCdash];

  *pBdash = B;
  *pEdash = E;
  *pCdash = C;
}

/**
 * $E40F: Reverses the 16 pixels in D,E and D',E'.
 *
 * \param[in,out] state  Pointer to game state.
 * \param[in,out] pD     Pointer to pixels/mask.
 * \param[in,out] pE     Pointer to pixels/mask.
 * \param[in,out] pDdash Pointer to pixels/mask.
 * \param[in,out] pEdash Pointer to pixels/mask.
 */
void flip_16_masked_pixels(tgestate_t *state,
                           uint8_t    *pD,
                           uint8_t    *pE,
                           uint8_t    *pDdash,
                           uint8_t    *pEdash)
{
  const uint8_t *HL;
  uint8_t        D, E;

  assert(state  != NULL);
  assert(pD     != NULL);
  assert(pE     != NULL);
  assert(pDdash != NULL);
  assert(pEdash != NULL);

  HL = &state->reversed[0];

  D = HL[*pE];
  E = HL[*pD];

  *pE = E;
  *pD = D;

  D = HL[*pEdash];
  E = HL[*pDdash];

  *pEdash = E;
  *pDdash = D;
}

/**
 * $E420: setup_vischar_plotting
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 *
 * \return 0 => vischar is invisible, 1 => vischar is visible (was Z?)
 *
 * HL is passed in too (but disassembly says it's always the same as IY).
 */
int setup_vischar_plotting(tgestate_t *state, vischar_t *vischar)
{
  /**
   * $E0EC: Addresses of self-modified locations which in the original game
   * are changed between NOPs and LD (HL),A.
   */
  static const size_t masked_sprite_plotter_24_enables[4 * 2] =
  {
    offsetof(tgestate_t, enable_E188),
    offsetof(tgestate_t, enable_E259),
    offsetof(tgestate_t, enable_E199),
    offsetof(tgestate_t, enable_E26A),
    offsetof(tgestate_t, enable_E1AA),
    offsetof(tgestate_t, enable_E27B),
    offsetof(tgestate_t, enable_E1BF),
    offsetof(tgestate_t, enable_E290),

    /* These two addresses are present here in the original game but are
     * unreferenced. */
    // masked_sprite_plotter_16_wide
    // masked_sprite_plotter_24_wide
  };

  pos_t          *pos;            /* was HL */
  tinypos_t      *tinypos;        /* was DE */
  const sprite_t *sprite;         /* was BC */
  uint8_t         flip_sprite;    /* was A */
  const sprite_t *sprite2;        /* was DE */
  uint16_t        clipped_width;  /* was BC */
  uint16_t        clipped_height; /* was DE */
  const size_t   *enables;        /* was HL */
  uint8_t         self_E4C0;      /* was $E4C0 */
  uint8_t         offset;         /* was A' */
  uint8_t         Cdash;          /* was C' */
  uint8_t         iters;          /* was B' */
  uint8_t         E;              /* was E */
  uint8_t         instr;          /* was A */
  uint8_t         A;              /* was A */
  uint16_t        x;              /* was HL */
  uint8_t        *maskbuf;        /* was HL */
  uint16_t        y;              /* was DE */
  uint16_t        skip;           /* was DE */

  assert(state != NULL);
  ASSERT_VISCHAR_VALID(vischar);

  pos     = &vischar->mi.pos;
  tinypos = &state->tinypos_81B2;

  if (state->room_index > room_0_OUTDOORS)
  {
    /* Indoors. */

    tinypos->x      = pos->x;
    tinypos->y      = pos->y;
    tinypos->height = pos->height;
  }
  else
  {
    /* Outdoors. */

    /* Conv: Unrolled, removed divide-by-8 calls. */
    tinypos->x      = (pos->x + 4 ) >> 3; /* with rounding */
    tinypos->y      = (pos->y     ) >> 3; /* without rounding */
    tinypos->height = (pos->height) >> 3;
  }

  sprite = vischar->mi.spriteset;

  state->flip_sprite = flip_sprite = vischar->mi.flip_sprite; // set left/right flip flag / sprite offset

  // HL now points after flip_sprite
  // DE now points to state->map_position_related_x

  // unrolled over original
  state->map_position_related_x = vischar->scrx >> 3;
  state->map_position_related_y = vischar->scry >> 3;

  // A is (1<<7) mask OR sprite offset
  // original game uses ADD A,A to double A and in doing so discards top bit
  sprite2 = &sprite[flip_sprite & 0x7F]; // spriteset pointer // A takes what values?

  vischar->width_bytes = sprite2->width;  // width in bytes
  vischar->height      = sprite2->height; // height in rows

  state->bitmap_pointer = sprite2->bitmap;
  state->mask_pointer   = sprite2->mask;

  if (vischar_visible(state, vischar, &clipped_width, &clipped_height) != 0) // used A as temporary
    return 0; /* invisible */

  // PUSH clipped_width
  // PUSH clipped_height

  E = clipped_height & 0xFF; // must be no of visible rows?

  if (vischar->width_bytes == 3) // 3 => 16 wide, 4 => 24 wide
  {
    state->self_E2C2 = E; // self modify masked_sprite_plotter_16_wide_left
    state->self_E363 = E; // self-modify masked_sprite_plotter_16_wide_right

    A = 3;
    enables = &masked_sprite_plotter_16_enables[0];
  }
  else
  {
    state->self_E121 = E; // self-modify masked_sprite_plotter_24_wide (shift right case)
    state->self_E1E2 = E; // self-modify masked_sprite_plotter_24_wide (shift left case)

    A = 4;
    enables = &masked_sprite_plotter_24_enables[0];
  }

  // PUSH HL

  self_E4C0 = A; // self-modify
  E = A;
  A = (clipped_width & 0xFF00) >> 8;
  if (A == 0)
  {
    instr = 0x77; /* opcode of 'LD (HL),A' */
    offset = clipped_width & 0xFF;
  }
  else
  {
    instr = 0x00; /* opcode of 'LD (HL),A' */
    offset = E - (clipped_width & 0xFF);
  }

  // POP HLdash // entry point

  /* Set the addresses in the jump table to NOP or LD (HL),A */
  Cdash = offset; // must be no of columns?
  iters = self_E4C0; /* 3 or 4 iterations */
  do
  {
    *(((uint8_t *) state) + *enables++) = instr;
    *(((uint8_t *) state) + *enables++) = instr;
    if (--offset == 0)
      instr ^= 0x77; /* Toggle between LD and NOP. */
  }
  while (--iters);

  y = 0; /* Conv: Moved. */
  if ((clipped_height >> 8) == 0)
    y = (vischar->scry - (state->map_position[1] * 8)) * 24;

  x = state->map_position_related_x - state->map_position[0]; // signed subtract + extend to 16-bit

  state->screen_pointer = &state->window_buf[x + y]; // screen buffer start address
  ASSERT_SCREEN_PTR_VALID(state->screen_pointer);

  maskbuf = &state->mask_buffer[0];

  // POP DE  // get clipped_height
  // PUSH DE

  maskbuf += (clipped_height >> 8) * 4 + (vischar->scry & 7) * 4; // i *think* its clipped_height -- check
  ASSERT_MASK_BUF_PTR_VALID(maskbuf);
  state->foreground_mask_pointer = maskbuf;

  // POP DE  // pop clipped_height

  A = clipped_height >> 8; // sign?
  if (A)
  {
    /* Conv: The original game has a generic multiply loop here. In this
     * version instead we'll just multiply. */

    A *= vischar->width_bytes - 1;
    // clipped_height &= 0x00FF; // D = 0
  }

  /* D ends up as zero either way. */

  skip = A;
  state->bitmap_pointer += skip;
  state->mask_pointer   += skip;

  // POP clipped_width  // why save it anyway? if it's just getting popped. unless it's being returned.

  return 1; /* visible */ // Conv: Added.
}

/* ----------------------------------------------------------------------- */

/**
 * $E542: Scale down a pos_t and assign result to a tinypos_t.
 *
 * Divides the three input 16-bit words by 8, with rounding to nearest,
 * storing the result as bytes.
 *
 * \param[in]  in  Input pos_t.      (was HL)
 * \param[out] out Output tinypos_t. (was DE)
 */
void pos_to_tinypos(const pos_t *in, tinypos_t *out)
{
  const uint8_t *pcoordin;  /* was HL */
  uint8_t       *pcoordout; /* was DE */
  uint8_t        iters;     /* was B */

  assert(in  != NULL);
  assert(out != NULL);

  /* Use knowledge of the structure layout to cast the pointers to array[3]
   * of their respective types. */
  pcoordin = (const uint8_t *) &in->x;
  pcoordout = (uint8_t *) &out->x;

  iters = 3;
  do
  {
    uint8_t low, high; /* was A, C */

    low = *pcoordin++;
    high = *pcoordin++;
    divide_by_8_with_rounding(&high, &low);
    *pcoordout++ = low;
  }
  while (--iters);
}

/**
 * $E550: Divide AC by 8, with rounding.
 *
 * \param[in,out] plow Pointer to low. (was A)
 * \param[in,out] phigh Pointer to high. (was C)
 */
void divide_by_8_with_rounding(uint8_t *plow, uint8_t *phigh)
{
  int t; /* Conv: Added. */

  assert(plow  != NULL);
  assert(phigh != NULL);

  t = *plow + 4; /* Like adding 0.5. */
  *plow = (uint8_t) t; /* Store modulo 256. */
  if (t >= 0x100)
    (*phigh)++;

  divide_by_8(plow, phigh);
}

/**
 * $E555: Divide AC by 8.
 *
 * \param[in,out] plow Pointer to low. (was A)
 * \param[in,out] phigh Pointer to high. (was C)
 */
void divide_by_8(uint8_t *plow, uint8_t *phigh)
{
  assert(plow  != NULL);
  assert(phigh != NULL);

  *plow = (*plow >> 3) | (*phigh << 5);
  *phigh >>= 3;
}

/* ----------------------------------------------------------------------- */

/**
 * $EED3: Plot the game screen.
 *
 * \param[in] state Pointer to game state.
 */
void plot_game_window(tgestate_t *state)
{
  assert(state != NULL);

  uint8_t *const  screen = &state->speccy->screen[0];

  uint8_t         x_hi;      /* was A */
  uint8_t        *src;       /* was HL */
  const uint16_t *offsets;   /* was SP */
  uint8_t         y_iters_A; /* was A */
  uint8_t        *dst;       /* was DE */
  uint8_t         data;      /* was A */
  uint8_t         y_iters_B; /* was B' */
  uint8_t         iters;     /* was B */
  uint8_t         copy;      /* was C */
  uint8_t         tmp;       /* added for RRD macro */

  x_hi = (state->plot_game_window_x & 0xFF00) >> 8;
  if (x_hi == 0)
  {
    src = &state->window_buf[1] + (state->plot_game_window_x & 0xFF);
    ASSERT_WINDOW_BUF_PTR_VALID(src);
    offsets = &state->game_window_start_offsets[0];
    y_iters_A = 128; /* iterations */
    do
    {
      dst = screen + *offsets++;
      ASSERT_SCREEN_PTR_VALID(dst);

      *dst++ = *src++; /* unrolled: 23 copies */
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      src++; // skip 24th
    }
    while (--y_iters_A);
  }
  else
  {
    src = &state->window_buf[1] + (state->plot_game_window_x & 0xFF);
    ASSERT_WINDOW_BUF_PTR_VALID(src);
    data = *src++;
    offsets = &state->game_window_start_offsets[0];
    y_iters_B = 128; /* iterations */
    do
    {
      dst = screen + *offsets++;
      ASSERT_SCREEN_PTR_VALID(dst);

/* RRD is equivalent to: */
#define RRD(A, HL, tmp)          \
  tmp = A & 0x0F;                \
  A = (*HL & 0x0F) | (A & 0xF0); \
  *HL = (*HL >> 4) | (tmp << 4);

      /* Conv: Unrolling removed compared to original code which did 4 groups of 5 ops, then a final 3. */
      iters = 4 * 5 + 3; /* iterations */
      do
      {
        copy = *src; // safe copy of source data
        RRD(data, src, tmp);
        data = *src; // rotated result
        *dst++ = data;
        *src++ = copy; // restore safe copy

        // A simplified form would be:
        // next_A = *HL;
        // *HL++ = (*HL >> 4) | (Adash << 4);
        // Adash = next_A;
      }
      while (--iters);

      data = *src++;
    }
    while (--y_iters_B);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $EF9A: Event: roll call.
 *
 * \param[in] state Pointer to game state.
 */
void event_roll_call(tgestate_t *state)
{
  uint16_t   range;   /* was DE */
  uint8_t   *pcoord;  /* was HL */
  uint8_t    iters;   /* was B */
  uint8_t    coord;   /* was A */
  vischar_t *vischar; /* was HL */

  assert(state != NULL);

  /* Is the hero within the roll call area bounds? */
  /* Conv: Unrolled. */
  /* Range checking. X in (0x72..0x7C) and Y in (0x6A..0x72). */
  range = map_ROLL_CALL_X; // note the confused coords business
  pcoord = &state->hero_map_position.x;
  coord = *pcoord++;
  if (coord < ((range >> 8) & 0xFF) || coord >= ((range >> 0) & 0xFF))
    goto not_at_roll_call;

  range = map_ROLL_CALL_Y;
  coord = *pcoord++; // -> state->hero_map_position.y
  if (coord < ((range >> 8) & 0xFF) || coord >= ((range >> 0) & 0xFF))
    goto not_at_roll_call;

  /* All characters turn forward. */
  vischar = &state->vischars[0];
  iters = vischars_LENGTH;
  do
  {
    vischar->b0D = vischar_BYTE13_BIT7; // kick movement
    vischar->b0E = 0x03; // direction (3 => face bottom left)
    vischar++;
  }
  while (--iters);

  return;

not_at_roll_call:
  state->bell = bell_RING_PERPETUAL;
  queue_message_for_display(state, message_MISSED_ROLL_CALL);
  hostiles_persue(state); // exit via
}

/* ----------------------------------------------------------------------- */

/**
 * $EFCB: Use papers.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Nothing. (May not return at all).
 */
void action_papers(tgestate_t *state)
{
  /**
   * $EFF9: Position outside the main gate.
   */
  static const tinypos_t outside_main_gate = { 0xD6, 0x8A, 0x06 };

  uint16_t  range;  /* was DE */
  uint8_t  *pcoord; /* was HL */
  uint8_t   coord;  /* was A */

  assert(state != NULL);

  /* Is the hero within the main gate bounds? */
  // UNROLLED
  /* Range checking. X in (0x69..0x6D) and Y in (0x49..0x4B). */
  range = map_MAIN_GATE_X; // note the confused coords business
  pcoord = &state->hero_map_position.x;
  coord = *pcoord++;
  if (coord < ((range >> 8) & 0xFF) || coord >= ((range >> 0) & 0xFF))
    return;

  range = map_MAIN_GATE_Y;
  coord = *pcoord++; // -> state->hero_map_position.y
  if (coord < ((range >> 8) & 0xFF) || coord >= ((range >> 0) & 0xFF))
    return;

  /* Using the papers at the main gate when not in uniform causes the hero
   * to be sent to solitary */
  if (state->vischars[0].mi.spriteset != &sprites[sprite_GUARD_FACING_TOP_LEFT_4])
  {
    solitary(state);
    return;
  }

  increase_morale_by_10_score_by_50(state);
  state->vischars[0].room = room_0_OUTDOORS;

  /* Transition to outside the main gate. */
  state->IY = &state->vischars[0];
  transition(state, &outside_main_gate);
  NEVER_RETURNS;
}

/* ----------------------------------------------------------------------- */

/**
 * $EFFC: Waits for the user to press Y or N.
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0 if 'Y' pressed, 1 if 'N' pressed.
 */
int user_confirm(tgestate_t *state)
{
  /** $F014 */
  static const screenlocstring_t screenlocstring_confirm_y_or_n =
  {
    0x100B, 15, "CONFIRM. Y OR N"
  };

  uint8_t keymask; /* was A */

  assert(state != NULL);

  screenlocstring_plot(state, &screenlocstring_confirm_y_or_n);

  state->speccy->kick(state->speccy);

  /* Keyscan. */
  for (;;)
  {
    keymask = state->speccy->in(state->speccy, port_KEYBOARD_POIUY);
    if ((keymask & (1 << 4)) == 0)
      return 0; /* is 'Y' pressed? return Z */

    keymask = state->speccy->in(state->speccy, port_KEYBOARD_SPACESYMSHFTMNB);
    keymask = ~keymask;
    if ((keymask & (1 << 3)) != 0)
      return 1; /* is 'N' pressed? return NZ */

    state->speccy->sleep(state->speccy, 0xFFFF);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F163: Setup the game.
 *
 * \param[in] state Pointer to game state.
 */
void tge_setup(tgestate_t *state)
{
  /**
   * $F1C9: Initial state of a visible character.
   *
   * The original game stores just 23 bytes of the structure, we store a
   * whole structure here.
   *
   * This can't be a _static_ const structure as it contains references which
   * are not compile time constant.
   */
  const vischar_t vischar_initial =
  {
    0x00,                 // character
    0x00,                 // flags
    0x012C,               // target
    { 0x2E, 0x2E, 0x18 }, // p04
    0x00,                 // more flags
    &character_related_pointers[0], // w08
    character_related_pointers[8],
    0x00,                 // b0C
    0x00,                 // b0D
    0x00,                 // b0E
    { { 0x0000, 0x0000, 0x0018 }, &sprites[sprite_PRISONER_FACING_TOP_LEFT_4], 0 },
    0x0000,               // scrx
    0x0000,               // scry
    room_0_OUTDOORS,      // room
    0x00,                 // unused
    0x00,                 // width_bytes
    0x00,                 // height
  };

  uint8_t   *reversed; /* was HL */
  uint8_t    counter;  /* was A */
  uint8_t    byte;     /* was C */
  uint8_t    iters;    /* was B */
  int        carry;
  vischar_t *vischar;  /* was HL */

  assert(state != NULL);

  wipe_full_screen_and_attributes(state);
  set_morale_flag_screen_attributes(state, attribute_BRIGHT_GREEN_OVER_BLACK);
  /* The original code seems to pass in 0x44, not zero, as it uses a register
   * left over from a previous call to set_morale_flag_screen_attributes(). */
  set_menu_item_attributes(state, 0, attribute_BRIGHT_YELLOW_OVER_BLACK);
  plot_statics_and_menu_text(state);

  plot_score(state);

  menu_screen(state);


  /* Construct a table of 256 bit-reversed bytes at 0x7F00. */
  reversed = &state->reversed[0];
  do
  {
    /* This was 'A = L;' in original code as alignment was guaranteed. */
    counter = reversed - &state->reversed[0]; /* Get a counter 0..255. */

    /* Reverse the counter byte. */
    byte = 0;
    iters = 8;
    do
    {
      carry = counter & 1;
      counter >>= 1;
      byte = (byte << 1) | carry;
    }
    while (--iters);

    *reversed++ = byte;
  }
  while (reversed != &state->reversed[256]); // was ((HL & 0xFF) != 0);

  /* Initialise all visible characters. */
  vischar = &state->vischars[0];
  iters = vischars_LENGTH;
  do
  {
    memcpy(vischar, &vischar_initial, sizeof(vischar_initial));
    vischar++;
  }
  while (--iters);

  /* Write 0xFF 0xFF over all non-visible characters. */
  iters = vischars_LENGTH - 1;
  vischar = &state->vischars[1];
  do
  {
    vischar->character = character_NONE;
    vischar->flags     = 0xFF;
    vischar++;
  }
  while (--iters);


  // Conv: Set up a jmpbuf so the game will return here.
  if (setjmp(state->jmpbuf_main) == 0)
  {
    /* In the original code this wiped all state from $8100 up until the
     * start of tiles ($8218). We'll assume for now that tgestate_t is
     * calloc'd and so zeroed by default. */

    reset_game(state);

    // i reckon this never gets this far, as reset_game jumps to enter_room itself
    //enter_room(state); // returns by goto main_loop
    NEVER_RETURNS;
  }
}

void tge_main(tgestate_t *state)
{
  // Conv: Need to get main loop to setjmp so we call it from here.
  if (setjmp(state->jmpbuf_main) == 0)
  {
    main_loop(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F1E0: Plot statics and menu text.
 *
 * \param[in] state Pointer to game state.
 */
void plot_statics_and_menu_text(tgestate_t *state)
{
  /**
   * $F446: key_choice_screenlocstrings.
   */
  static const screenlocstring_t key_choice_screenlocstrings[] =
  {
    { 0x008E,  8, "CONTROLS"                },
    { 0x00CD,  8, "0 SELECT"                },
    { 0x080D, 10, "1 KEYBOARD"              },
    { 0x084D, 10, "2 KEMPSTON"              },
    { 0x088D, 10, "3 SINCLAIR"              },
    { 0x08CD,  8, "4 PROTEK"                },
    { 0x1007, 23, "BREAK OR CAPS AND SPACE" },
    { 0x102C, 12, "FOR NEW GAME"            },
  };

  const statictileline_t  *stline;    /* was HL */
  int                      iters;     /* was B */
  uint8_t                 *screenptr; /* was DE */
  const screenlocstring_t *slstring;  /* was HL */

  assert(state != NULL);

  /* Plot statics and menu text. */
  stline = &static_graphic_defs[0];
  iters  = NELEMS(static_graphic_defs);
  do
  {
    screenptr = &state->speccy->screen[stline->screenloc]; /* Fetch screen address offset. */
    ASSERT_SCREEN_PTR_VALID(screenptr);

    if (stline->flags_and_length & statictileline_VERTICAL)
      plot_static_tiles_vertical(state, screenptr, stline);
    else
      plot_static_tiles_horizontal(state, screenptr, stline);
    stline++;
  }
  while (--iters);

  /* Plot menu text. */
  iters    = NELEMS(key_choice_screenlocstrings);
  slstring = &key_choice_screenlocstrings[0];
  do
    slstring = screenlocstring_plot(state, slstring);
  while (--iters);
}

/**
 * $F206: Plot static tiles horizontally.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] out    Pointer to screen address. (was DE)
 * \param[in] stline Pointer to [count, tile indices, ...]. (was HL)
 */
void plot_static_tiles_horizontal(tgestate_t             *state,
                                  uint8_t                *out,
                                  const statictileline_t *stline)
{
  plot_static_tiles(state, out, stline, 0);
}

/**
 * $F209: Plot static tiles vertically.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] out    Pointer to screen address. (was DE)
 * \param[in] stline Pointer to [count, tile indices, ...]. (was HL)
 */
void plot_static_tiles_vertical(tgestate_t             *state,
                                uint8_t                *out,
                                const statictileline_t *stline)
{
  plot_static_tiles(state, out, stline, 255);
}

/**
 * $F20B: Plot static tiles.
 *
 * \param[in] state       Pointer to game state.
 * \param[in] out         Pointer to screen address. (was DE)
 * \param[in] stline      Pointer to [count, tile indices, ...]. (was HL)
 * \param[in] orientation Orientation: 0/255 = horizontal/vertical. (was A)
 */
void plot_static_tiles(tgestate_t             *state,
                       uint8_t                *out,
                       const statictileline_t *stline,
                       int                     orientation)
{
  int            tiles_remaining; /* was A/B */
  const uint8_t *tiles;           /* was HL */

  assert(state  != NULL);
  ASSERT_SCREEN_PTR_VALID(out);
  assert(stline != NULL);
  assert(orientation == 0x00 || orientation == 0xFF);

  tiles = stline->tiles;

  tiles_remaining = stline->flags_and_length & ~statictileline_MASK;
  do
  {
    int                  tile_index;  /* was A */
    const static_tile_t *static_tile; /* was HL' */
    const tilerow_t     *tile_data;   /* was HL' */
    int                  iters;       /* was B' */
    ptrdiff_t            soffset;     /* was A */
    int                  aoffset;

    tile_index = *tiles++;

    static_tile = &static_tiles[tile_index]; // elements: 9 bytes each

    /* Plot a tile. */
    tile_data = &static_tile->data.row[0];
    iters = 8;
    do
    {
      *out = *tile_data++;
      out += 256; /* move to next screen row */
    }
    while (--iters);
    out -= 256;

    /* Calculate screen attribute address of tile. */
    // ((out - screen_base) / 0x800) * 0x100 + attr_base
    soffset = out - &state->speccy->screen[0];
    aoffset = soffset & 0xFF;
    if (soffset >= 0x0800) aoffset += 256;
    if (soffset >= 0x1000) aoffset += 256;
    state->speccy->attributes[aoffset] = static_tile->attr; /* Copy attribute byte. */

    if (orientation == 0) /* Horizontal */
      out = out - 7 * 256 + 1;
    else /* Vertical */
      out = (uint8_t *) get_next_scanline(state, out); // must cast away constness

    ASSERT_SCREEN_PTR_VALID(out);
  }
  while (--tiles_remaining);
}

/* ----------------------------------------------------------------------- */

/**
 * $F257: Clear the screen and attributes. Set the border to black.
 *
 * \param[in] state Pointer to game state.
 */
void wipe_full_screen_and_attributes(tgestate_t *state)
{
  assert(state != NULL);

  memset(&state->speccy->screen, 0, SCREEN_LENGTH);
  memset(&state->speccy->attributes, attribute_WHITE_OVER_BLACK, SCREEN_ATTRIBUTES_LENGTH);

  state->speccy->out(state->speccy, port_BORDER, 0); /* set border to black */
}

/* ----------------------------------------------------------------------- */

/**
 * $F271: Menu screen key handling.
 *
 * Scan for a keypress which starts the game or selects an input device. If
 * an input device is chosen, update the menu highlight to match and record
 * which input device was chosen.
 *
 * If the game is started and keyboard input device is selected then call
 * choose_keys().
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0 => no keypress, 1 => keypress handled, -1 => start the game
 */
int check_menu_keys(tgestate_t *state)
{
  uint8_t keycode; /* was A */

  assert(state != NULL);

  keycode = menu_keyscan(state);
  if (keycode == 0xFF)
    return 0; /* no keypress */

  if (keycode)
  {
    /* 1..4 -> 0..3 */
    keycode--;

    /* Clear old selection. */
    set_menu_item_attributes(state,
                             state->chosen_input_device,
                             attribute_WHITE_OVER_BLACK);

    /* Highlight new selection. */
    state->chosen_input_device = keycode;
    set_menu_item_attributes(state,
                             keycode,
                             attribute_BRIGHT_YELLOW_OVER_BLACK);

    return 1;
  }
  else
  {
    /* Zero pressed to start game. */

    /* Conv: At this point the original game copies the selected input
     * routine to $F075. */

    if (state->chosen_input_device == inputdevice_KEYBOARD)
      choose_keys(state); /* Keyboard was selected. */

    return -1; /* Start the game */
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F335: Wipe the game screen.
 *
 * \param[in] state Pointer to game state.
 */
void wipe_game_window(tgestate_t *state)
{
  assert(state != NULL);

  uint8_t *const  screen = &state->speccy->screen[0];
  const uint16_t *poffsets; /* was SP */
  uint8_t         iters;    /* was A */

  poffsets = &state->game_window_start_offsets[0]; /* points to offsets */
  iters = state->rows * 8;
  do
  {
    uint8_t *const p = screen + *poffsets++;

    ASSERT_SCREEN_PTR_VALID(p);
    memset(p, 0, state->columns);
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $F350: Choose keys.
 *
 * \param[in] state Pointer to game state.
 */
void choose_keys(tgestate_t *state)
{
  /** $F2AD: Table of prompt strings. */
  static const screenlocstring_t choose_key_prompts[6] =
  {
    { 0x006D, 11, "CHOOSE KEYS" },
    { 0x00CD,  5, "LEFT." },
    { 0x080D,  6, "RIGHT." },
    { 0x084D,  3, "UP." },
    { 0x088D,  5, "DOWN." },
    { 0x08CD,  5, "FIRE." },
  };

  /** $F2E1: Table of keyscan high bytes. */
  static const uint8_t keyboard_port_hi_bytes[10] =
  {
    /* The first byte is unused. */
    /* The final zero byte is a terminator. */
    0x24, 0xF7, 0xEF, 0xFB, 0xDF, 0xFD, 0xBF, 0xFE, 0x7F, 0x00,
  };

  /** $F2EB: Table of special key name strings, prefixed by their length. */
  static const char modifier_key_names[] = "\x05" "ENTER"
                                           "\x04" "CAPS"   /* CAPS SHIFT */
                                           "\x06" "SYMBOL" /* SYMBOL SHIFT */
                                           "\x05" "SPACE";

  /* Macro to encode modifier_key_names in below keycode_to_glyph table. */
#define O(n) ((n) | (1 << 7))

  /** $F303: Table mapping key codes to glyph indices. */
  /* Each table entry is a character (in original code: a glyph index) OR if
   * its top bit is set then bottom seven bits are an index into
   * modifier_key_names. */
  static const unsigned char keycode_to_glyph[8][5] =
  {
    { '1',   '2',   '3',   '4',   '5', }, /* table_12345 */
    { '0',   '9',   '8',   '7',   '6', }, /* table_09876 */
    { 'Q',   'W',   'E',   'R',   'T', }, /* table_QWERT */
    { 'P',   'O',   'I',   'U',   'Y', }, /* table_POIUY */
    { 'A',   'S',   'D',   'F',   'G', }, /* table_ASDFG */
    { O(0),  'L',   'K',   'J',   'H', }, /* table_ENTERLKJH */
    { O(6),  'Z',   'X',   'C',   'V', }, /* table_SHIFTZXCV */
    { O(18), O(11), 'M',   'N',   'B', }, /* table_SPACESYMSHFTMNB */
  };

#undef O

  /** $F32B: Screen offsets where to plot key names.
   * Conv: In the original code these are absolute addresses. */
  static const uint16_t key_name_screen_offsets[5] =
  {
    0x00D5,
    0x0815,
    0x0855,
    0x0895,
    0x08D5,
  };

  assert(state != NULL);

  uint8_t *const screen = &state->speccy->screen[0]; /* Conv: Added */

  /* Loop while the user does not confirm. */
  for (;;)
  {
    uint8_t                  prompt_iters; /* was B */
    const screenlocstring_t *prompt;       /* was HL */

    /* Clear the game window. */
    wipe_game_window(state);
    set_game_window_attributes(state, attribute_WHITE_OVER_BLACK);

    /* Draw key choice prompt strings. */
    prompt_iters = NELEMS(choose_key_prompts);
    prompt = &choose_key_prompts[0];
    do
    {
      uint8_t    *screenptr; /* was DE */
      uint8_t     iters;     /* was B */
      const char *string;    /* was HL */

      screenptr = &state->speccy->screen[prompt->screenloc];
      iters  = prompt->length;
      string = prompt->string;
      do
      {
        // A = *HLstring; /* Conv: Present in original code but this is redundant when calling plot_glyph(). */
        ASSERT_SCREEN_PTR_VALID(screenptr);
        screenptr = plot_glyph(string, screenptr);
        string++;
      }
      while (--iters);
      prompt++; /* Conv: Original has all data contiguous, but we need this in addition. */
    }
    while (--prompt_iters);


    state->speccy->kick(state->speccy);


    /* Wipe keydefs. */
    memset(&state->keydefs.defs[0], 0, 5 * 2);

    uint8_t   Adash = 0;  /* was A; */ // initialised to zero
    {
      const uint16_t *poffset; /* was HL */

      prompt_iters = NELEMS(key_name_screen_offsets); /* L/R/U/D/F => 5 */
      poffset = &key_name_screen_offsets[0];
      do
      {
        uint16_t  self_F3E9;  /* was $F3E9 */
        uint8_t   A; /* was A */ // seems to be a row count

        // this block moved up for scope
        keydef_t *keydef;  /* was HL */
        uint8_t   port;    /* was B */
        uint8_t   mask;    /* was C */

        self_F3E9 = *poffset++; // self modify screen addr
        A = 0xFF;

        const uint8_t *hi_bytes;    /* was HL */
        uint8_t        index;       /* was D */
        int            carry;
//        uint8_t        hi_byte;     /* was A' */
//        uint8_t        storedport;  /* was A' */
        uint8_t        keyflags;    /* was E */

for_loop:
        for (;;)
        {
          state->speccy->sleep(state->speccy, 10000); // 10000 is arbitrary for the moment

          SWAP(uint8_t, A, Adash);

          hi_bytes = &keyboard_port_hi_bytes[0]; /* Byte 0 is unused. */
          index = 0xFF;
try_next_port:
          hi_bytes++;
          index++;
          A = *hi_bytes;
          if (A == 0) /* Hit end of keyboard_port_hi_bytes. */
            goto for_loop;

          port = A; // saved so it can be stored later
          A = ~state->speccy->in(state->speccy, (port << 8) | 0xFE);
          keyflags = A;
          mask = 1 << 5;
key_loop:
          SRL(mask);
          if (carry)
            goto try_next_port; /* Masking loop ran out. Move to the next keyboard port. */

          A = mask;
          A = A & keyflags;
          if (A == 0) // temps: A'
            goto key_loop; /* Key was not pressed. Move to next key. */

          SWAP(uint8_t, A, Adash);

          if (A)
            goto for_loop;

          /* Draw the pressed key. */

          A = index;

          SWAP(uint8_t, A, Adash);

          /* Check for an already defined key. */
          keydef = &state->keydefs.defs[0] - 1;
          do
          {
            keydef++;

            A = keydef->port;
            if (A == 0) /* If an empty slot, assign */
              goto assign_keydef;
          }
          while (A != port || keydef->mask != mask);
        } // for_loop

assign_keydef:
        keydef->port = port;
        keydef->mask = mask;

        {
          const unsigned char *pglyph;   /* was HL */
          const char          *pkeyname; /* was HL */
          int                  carry = 0;

          SWAP(uint8_t, A, Adash);

          pglyph = &keycode_to_glyph[A][0] - 1; /* Off by one to compensate for pre-increment */
          /* Skip entries until 'mask' carries out. */
          do
          {
            pglyph++;
            RR(mask);
          }
          while (!carry);

          uint8_t length;          /* was B */
          uint8_t glyph_and_flags; /* was A */

          // plot the byte at HL, string length is 1
          length = 1;
          glyph_and_flags = *pglyph;
          pkeyname = (const char *) pglyph; // Conv: Added. For type reasons.

          if (glyph_and_flags & (1 << 7))
          {
            /* If the top bit was set then it's a modifier key. */
            pkeyname = &modifier_key_names[glyph_and_flags & ~(1 << 7)];
            length = *pkeyname++;
          }

          unsigned char *screenoff; /* was DE */

          /* Plot. */
          screenoff = screen + self_F3E9; // self modified // screen offset
          do
          {
            // glyph_and_flags = *pkeyname; // Conv: dead code? similar to other instances of calls to plot_glyph
            ASSERT_SCREEN_PTR_VALID(screenoff);
            screenoff = plot_glyph(pkeyname, screenoff);
            pkeyname++;
          }
          while (--length);

          state->speccy->kick(state->speccy);
        }
      }
      while (--prompt_iters);
    }

    {
//      uint16_t count; /* was BC */
//
//      /* Delay loop */
//      count = 0xFFFF;
//      while (--count)
//        ;

      state->speccy->sleep(state->speccy, 0xFFFF);
    }

    /* Wait for user's input */
    if (user_confirm(state) == 0) /* Confirmed - return */
      return; /* Start the game */
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F408: Set the screen attributes of the specified menu item.
 *
 * \param[in] state Pointer to game state.
 * \param[in] index Item index. (was A)
 * \param[in] attrs Screen attributes. (was E)
 */
void set_menu_item_attributes(tgestate_t *state,
                              int         index,
                              attribute_t attrs)
{
  attribute_t *pattr;

  assert(state != NULL);
  assert(index < 4); // a guess
  assert(attrs <= attribute_BRIGHT_WHITE_OVER_BLACK);

  pattr = &state->speccy->attributes[(0x590D - SCREEN_ATTRIBUTES_START_ADDRESS)];

  /* Skip to the item's row */
  pattr += index * 2 * state->width; /* two rows */

  /* Draw */
  ASSERT_SCREEN_ATTRIBUTES_PTR_VALID(pattr);
  memset(pattr, attrs, 10);
}

/* ----------------------------------------------------------------------- */

/**
 * $F41C: Scan for keys to select an input device.
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0/1/2/3/4 = keypress, or 255 = no keypress
 */
uint8_t menu_keyscan(tgestate_t *state)
{
  uint8_t count;   /* was E */
  uint8_t keymask; /* was A */
  uint8_t iters;   /* was B */

  assert(state != NULL);

  count = 0;
  keymask = ~state->speccy->in(state->speccy, port_KEYBOARD_12345) & 0xF; /* 1..4 keys only */
  if (keymask)
  {
    iters = 4;
    do
    {
      /* Conv: Reordered from original to avoid carry flag test */
      count++;
      if (keymask & 1)
        break;
      keymask >>= 1;
    }
    while (--iters);

    return count;
  }
  else
  {
    if ((state->speccy->in(state->speccy, port_KEYBOARD_09876) & 1) == 0) /* 0 key only */
      return count; /* always zero */

    return 0xFF; /* no keypress */
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F4B7: Runs the menu screen.
 *
 * Waits for user to select an input device, waves the morale flag and plays
 * the title tune.
 *
 * \param[in] state Pointer to game state.
 */
void menu_screen(tgestate_t *state)
{
  uint16_t BC;              /* was BC */
  uint16_t BCdash;          /* was BC' */
  uint16_t DE;              /* was DE */
  uint16_t DEdash;          /* was DE' */
  uint8_t  datum;           /* was A */
  uint8_t  overall_delay;   /* was A */
  uint8_t  iters;           /* was H */
  uint16_t channel0_index;  /* was HL */
  uint16_t channel1_index;  /* was HL' */
  uint8_t  L;               /* was L */
  uint8_t  Ldash;           /* was L' */
  uint8_t  B;               /* was B */
  uint8_t  C;               /* was C */
  uint8_t  Bdash;           /* was B' */
  uint8_t  Cdash;           /* was C' */

  assert(state != NULL);

  for (;;)
  {
    /* Conv: Routine changed to return values. */
    if (check_menu_keys(state) < 0)
      return; /* Start the game */

    wave_morale_flag(state);

    /* Play music */
    channel0_index = state->music_channel0_index + 1;
    /* Loop until the end marker is encountered. */
    for (;;)
    {
      state->music_channel0_index = channel0_index;
      datum = music_channel0_data[channel0_index];
      if (datum != 0xFF) /* end marker */
        break;
      channel0_index = 0;
    }
    DE = BC = get_tuning(datum);
    channel0_index &= 0xFF00; // was L = 0;

    channel1_index = state->music_channel1_index + 1;
    /* Loop until the end marker is encountered. */
    for (;;)
    {
      state->music_channel1_index = channel1_index;
      datum = music_channel1_data[channel1_index];
      if (datum != 0xFF) /* end marker */
        break;
      channel1_index = 0;
    }
    DEdash = BCdash = get_tuning(datum);
    channel1_index &= 0xFF00; // was Ldash = 0;

    if ((BCdash >> 8) == 0xFF) // (BCdash >> 8) was Bdash;
    {
      BCdash = BC;
      DEdash = BCdash;
    }

    overall_delay = 24; /* overall tune speed (a delay: lower values are faster) */
    do
    {
      iters = 255;
      do
      {
        // B,C are a pair of counters?

        B = BC >> 8;
        C = BC & 0xFF;
        if (--B == 0 && --C == 0)
        {
          channel0_index ^= 16;
          state->speccy->out(state->speccy, port_BORDER, channel0_index & 0xFF);
          BC = DE;
        }
        // otherwise needs B & C writing back

        Bdash = BCdash >> 8;
        Cdash = BCdash & 0xFF;
        if (--Bdash == 0 && --Cdash == 0)
        {
          channel1_index ^= 16;
          state->speccy->out(state->speccy, port_BORDER, channel1_index & 0xFF);
          BCdash = DEdash;
        }
        // otherwise needs B' & C' writing back
      }
      while (--iters);
    }
    while (--overall_delay);


    state->speccy->kick(state->speccy);
    state->speccy->sleep(state->speccy, 87500);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F52C: Get tuning.
 *
 * \param[in] A Index.
 *
 * \returns (was DE)
 */
uint16_t get_tuning(uint8_t A)
{
  uint16_t BC;

  BC = music_tuning_table[A]; // FIXME: Original loads endian swapped...

  // Might be able to increment by 0x0101 rather than incrementing the bytes
  // separately, but unsure currently of exact nature of calculation.

  BC = ((BC + 0x0001) & 0x00FF); // B++
  BC = ((BC + 0x0100) & 0xFF00); // C++
  if ((BC & 0x00FF) == 0)
    BC++;

  // L = 0; unsure what this is doing

  return BC;
}

/* ----------------------------------------------------------------------- */

/**
 * $FE00: inputroutine_keyboard.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
input_t inputroutine_keyboard(tgestate_t *state)
{
  const keydef_t *def;         /* was HL */
  input_t         inputs;      /* was E */
  uint16_t        port;        /* was BC */
  uint8_t         key_pressed; /* was A */

  assert(state != NULL);

  def = &state->keydefs.defs[0]; /* A list of (port, mask) */

  /* Left or right? */
  port = (def->port << 8) | 0xFE;
  key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
  def++;
  if (key_pressed)
  {
    def++; /* Skip right keydef */
    inputs = input_LEFT;
  }
  else
  {
    /* Right */
    port = (def->port << 8) | 0xFE;
    key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
    def++;
    if (key_pressed)
      inputs = input_RIGHT;
    else
      inputs = input_NONE; /* == 0 */
  }

  /* Up or down? */
  port = (def->port << 8) | 0xFE;
  key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
  def++;
  if (key_pressed)
  {
    def++; /* Skip down keydef */
    inputs += input_UP;
  }
  else
  {
    /* Down */
    port = (def->port << 8) | 0xFE;
    key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
    def++;
    if (key_pressed)
      inputs += input_DOWN;
  }

  /* Fire? */
  port = (def->port << 8) | 0xFE;
  key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
  if (key_pressed)
    inputs += input_FIRE;

  return inputs;
}

/**
 * $FE47: Protek/cursor joystick input routine.
 *
 * Up/Down/Left/Right/Fire = keys 7/6/5/8/0.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
input_t inputroutine_protek(tgestate_t *state)
{
  uint8_t  keybits_left;       /* was A */
  input_t  left_right;         /* was E */
  uint16_t port;               /* was BC */
  uint8_t  keybits_others;     /* was D */
  input_t  up_down;            /* was A */
  input_t  left_right_up_down; /* was E */
  input_t  fire;               /* was A */

  assert(state != NULL);

  /* Horizontal */
  keybits_left = ~state->speccy->in(state->speccy, port_KEYBOARD_12345);
  left_right = input_LEFT;
  port = port_KEYBOARD_09876;
  if ((keybits_left & (1 << 4)) == 0)
  {
    keybits_left = ~state->speccy->in(state->speccy, port);
    left_right = input_RIGHT;
    if ((keybits_left & (1 << 2)) == 0)
      left_right = 0;
  }

  /* Vertical */
  keybits_others = ~state->speccy->in(state->speccy, port);
  up_down = input_UP;
  if ((keybits_others & (1 << 3)) == 0)
  {
    up_down = input_DOWN;
    if ((keybits_others & (1 << 4)) == 0)
      up_down = input_NONE;
  }

  left_right_up_down = left_right + up_down; /* Combine axis */

  /* Fire */
  fire = input_FIRE;
  if (keybits_others & (1 << 0))
    fire = 0;

  return fire + left_right_up_down; /* Combine axis */
}

/**
 * $FE7E: Kempston joystick input routine.
 *
 * Reading port 0x1F yields 000FUDLR active high.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
input_t inputroutine_kempston(tgestate_t *state)
{
  uint8_t keybits;    /* was A */
  input_t left_right; /* was B */
  input_t up_down;    /* was C */
  int     carry = 0;
  input_t fire;       /* was A */

  assert(state != NULL);

  keybits = state->speccy->in(state->speccy, 0x001F);

  left_right = up_down = 0;

  RR(keybits);
  if (carry)
    left_right = input_RIGHT;

  RR(keybits);
  if (carry)
    left_right = input_LEFT;

  RR(keybits);
  if (carry)
    up_down = input_DOWN;

  RR(keybits);
  if (carry)
    up_down = input_UP;

  RR(keybits);
  fire = input_FIRE;
  if (!carry)
    fire = 0;

  return fire + left_right + up_down; /* Combine axis */
}

/**
 * $FEA3: Fuller joystick input routine.
 *
 * Unused, but present, as in the original game.
 *
 * Reading port 0x7F yields F---RLDU active low.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
input_t inputroutine_fuller(tgestate_t *state)
{
  uint8_t keybits;    /* was A */
  input_t left_right; /* was B */
  input_t up_down;    /* was C */
  int     carry = 0;
  input_t fire;       /* was A */

  assert(state != NULL);

  keybits = state->speccy->in(state->speccy, 0x007F);

  left_right = up_down = 0;

  if (keybits & (1<<4))
    keybits = ~keybits;

  RR(keybits);
  if (carry)
    up_down = input_UP;

  RR(keybits);
  if (carry)
    up_down = input_DOWN;

  RR(keybits);
  if (carry)
    left_right = input_LEFT;

  RR(keybits);
  if (carry)
    left_right = input_RIGHT;

  if (keybits & (1<<3))
    fire = input_FIRE;
  else
    fire = 0;

  return fire + left_right + up_down; /* Combine axis */
}

/**
 * $FECD: Sinclair joystick input routine.
 *
 * Up/Down/Left/Right/Fire = keys 9/8/6/7/0.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
input_t inputroutine_sinclair(tgestate_t *state)
{
  uint8_t keybits;    /* was A */
  input_t left_right; /* was B */
  input_t up_down;    /* was C */
  int     carry = 0;
  input_t fire;       /* was A */

  assert(state != NULL);

  keybits = ~state->speccy->in(state->speccy, port_KEYBOARD_09876); /* xxx67890 */

  left_right = up_down = 0;

  RRC(keybits); /* 0xxx6789 */
  RRC(keybits); /* 90xxx678 */
  if (carry)
    up_down = input_UP;

  RRC(keybits); /* 890xxx67 */
  if (carry)
    up_down = input_DOWN;

  RRC(keybits); /* 7890xxx6 */
  if (carry)
    left_right = input_RIGHT;

  RRC(keybits); /* 67890xxx */
  if (carry)
    left_right = input_LEFT;

  keybits &= 1 << 3; /* Set A to 8 or 0 */
  if (keybits == 0)
    fire = input_FIRE;
  else
    fire = 0;

  return fire + left_right + up_down; /* Combine axis */
}

/* ----------------------------------------------------------------------- */

// ADDITIONAL FUNCTIONS
//

/**
 * The original game has a routine which copies an inputroutine_* routine
 * to a fixed location so that process_player_input() can call directly.
 * We can't do that portably, so we add a selector routine here.
 */
input_t input_routine(tgestate_t *state)
{
  /**
   * $F43D: Available input routines.
   */
  static const inputroutine_t inputroutines[inputdevice__LIMIT] =
  {
    &inputroutine_keyboard,
    &inputroutine_kempston,
    &inputroutine_sinclair,
    &inputroutine_protek,
  };

  assert(state != NULL);

  assert(state->chosen_input_device < inputdevice__LIMIT);

  return inputroutines[state->chosen_input_device](state);
}

// vim: ts=8 sts=2 sw=2 et

