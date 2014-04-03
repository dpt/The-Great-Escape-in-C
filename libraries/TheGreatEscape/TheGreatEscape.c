/**
 * TheGreatEscape.c
 *
 * Reimplementation of the ZX Spectrum 48K game "The Great Escape" created by
 * Denton Designs and published in 1986 by Ocean Software.
 *
 * This reimplementation copyright (c) David Thomas, 2012-2014.
 * <dave@davespace.co.uk>.
 */

/* ----------------------------------------------------------------------- */

/* TODO
 *
 * - Fix coordinate order in pos structures: (y,x) -> (x,y).
 * - Need function to initialise state tables (e.g. last byte of
 *   message_queue).
 * - Fix bad naming: Rename *struct types to *def.
 * - Replace uint8_t counters etc. with int where possible. [later]
 * - Some enums might be wider than expected (int vs uint8_t).
 */

/* GLOSSARY
 *
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

/**
 * NEVER_RETURNS is placed after calls which are not expected to return (ones
 * which ultimately invoke squash_stack_goto_main).
 */
#define NEVER_RETURNS assert("Unexpected return." == NULL); return

/* ----------------------------------------------------------------------- */

// FORWARD REFERENCES
//
// (in original file order)

#define INLINE inline

/* $6000 onwards */

static
void transition(tgestate_t      *state,
                vischar_t       *vischar,
                const tinypos_t *pos);
static
void enter_room(tgestate_t *state);
static
void squash_stack_goto_main(tgestate_t *state);

static
void set_player_sprite_for_room(tgestate_t *state);

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
void drop_item_A(tgestate_t *state, item_t item);
static
void drop_item_A_exterior_tail(itemstruct_t *HL);
static
void drop_item_A_interior_tail(itemstruct_t *HL);

static INLINE
itemstruct_t *item_to_itemstruct(tgestate_t *state, item_t item);

static
void draw_all_items(tgestate_t *state);
static
void draw_item(tgestate_t *state, item_t index, uint8_t *dst);

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
void keyscan_cancel(tgestate_t *state);

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
int in_numbered_area(tgestate_t *state, uint8_t index, const tinypos_t *pos);

/* $A000 onwards */

static
void wave_morale_flag(tgestate_t *state);

static
void set_morale_flag_screen_attributes(tgestate_t *state, attribute_t attrs);

static
uint8_t *get_prev_scanline(tgestate_t *state, uint8_t *addr);

static
void interior_delay_loop(void);

static
void ring_bell(tgestate_t *state);
static
void plot_ringer(tgestate_t *state, const uint8_t *DE);

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
void set_day_or_night(tgestate_t *state, uint8_t A);
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
void common_A26E(tgestate_t *state, uint8_t A, uint8_t C);

extern const character_t tenlong[10];

static
void wake_up(tgestate_t *state);
static
void breakfast_time(tgestate_t *state);

static
void set_player_target_location(tgestate_t *state, location_t location);

static
void go_to_time_for_bed(tgestate_t *state);

static
void set_location_A35F(tgestate_t *state, uint8_t *counter, uint8_t C);
static
void set_location_A373(tgestate_t *state, uint8_t *counter, uint8_t C);
static
void sub_A38C(tgestate_t *state, character_t index, uint8_t Adash, uint8_t C);
static
void sub_A3BB(tgestate_t *state, vischar_t *HL);

static INLINE
void store_location(uint8_t Adash, uint8_t C, location_t *location);

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
              uint8_t            character_index);

static
void character_sits(tgestate_t *state, character_t character, uint8_t *formerHL);
static
void character_sleeps(tgestate_t *state, character_t character, uint8_t *formerHL);
static
void character_sit_sleep_common(tgestate_t *state, room_t room, uint8_t *HL);
static
void select_room_and_plot(tgestate_t *state);

static
void player_sits(tgestate_t *state);
static
void player_sleeps(tgestate_t *state);
static
void player_sit_sleep_common(tgestate_t *state, uint8_t *HL);

static
void set_location_0x0E00(tgestate_t *state);
static
void set_location_0x8E04(tgestate_t *state);
static
void set_location_0x1000(tgestate_t *state);

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
                                 uint8_t            character_index);

static
void go_to_roll_call(tgestate_t *state);

static
void screen_reset(tgestate_t *state);

static
void escaped(tgestate_t *state);

static
uint8_t keyscan_all(tgestate_t *state);

static INLINE
escapeitem_t have_required_items(const item_t *pitem, escapeitem_t previous);
static INLINE
escapeitem_t item_to_bitmask(item_t item);

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
void zoombox_1(tgestate_t *state);
static
void zoombox_draw(tgestate_t *state);
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
void sub_AF8F(tgestate_t *state, vischar_t *vischar);

static
void sub_AFDF(tgestate_t *state, vischar_t *vischar);

/* $B000 onwards */

static
void use_bribe(tgestate_t *state, vischar_t *vischar);

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
void reset_B2FC(tgestate_t *state);

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
static
void action_red_key(tgestate_t *state);
static
void action_yellow_key(tgestate_t *state);
static
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
void searchlight_sub(tgestate_t *state, vischar_t *vischar);

static
void searchlight(tgestate_t *state);

static
uint8_t sub_B89C(tgestate_t *state, vischar_t **pvischar);

static
void mask_stuff(tgestate_t *state);

static
uint16_t scale_val(tgestate_t *state, uint8_t A, uint8_t E);

static
void mask_against_tile(uint8_t index, uint8_t *dst);

static
int vischar_visible(tgestate_t *state,
                    vischar_t  *vischar,
                    uint16_t   *pBC,
                    uint16_t   *pDE);

static
void called_from_main_loop_3(tgestate_t *state);

static
const tile_t *select_tile_set(tgestate_t *state, uint8_t H, uint8_t L);

/* $C000 onwards */

static
void called_from_main_loop_7(tgestate_t *state);

static
void called_from_main_loop_6(tgestate_t *state);

static
int spawn_characters(tgestate_t *state, characterstruct_t *HL);

static
void reset_visible_character(tgestate_t *state, vischar_t *vischar);

static
uint8_t sub_C651(tgestate_t *state, location_t *HL);

static
void move_characters(tgestate_t *state);

static
int increment_DE_by_diff(tgestate_t *state,
                         uint8_t     max,
                         int         B,
                         uint8_t    *HL,
                         uint8_t    *DE);

static
characterstruct_t *get_character_struct(tgestate_t *state, character_t index);

static
void character_event(tgestate_t *state, location_t *loc);

static
charevnt_handler_t charevnt_handler_4_zero_morale_1;
static
charevnt_handler_t charevnt_handler_6;
static
charevnt_handler_t charevnt_handler_10_player_released_from_solitary;
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
charevnt_handler_t charevnt_handler_9_player_sits;
static
charevnt_handler_t charevnt_handler_8_player_sleeps;

static
void follow_suspicious_player(tgestate_t *state);

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
uint8_t move_character_y(tgestate_t *state,
                         vischar_t  *vischar,
                         int         log2scale);

static
uint8_t move_character_x(tgestate_t *state,
                         vischar_t  *vischar,
                         int         log2scale);

static
void bribes_solitary_food(tgestate_t *state, vischar_t *vischar);

static
void sub_CB23(tgestate_t *state, vischar_t *vischar, location_t *HL);
static
void sub_CB2D(tgestate_t *state, vischar_t *vischar, location_t *HL);
static
void sub_CB61(tgestate_t *state, vischar_t *vischar, location_t *HL, uint8_t A);

static INLINE
uint16_t multiply_by_1(uint8_t A);

static INLINE
const uint8_t *element_A_of_table_7738(uint8_t A);

static
uint8_t fetch_C41A(tgestate_t *state);

static
void solitary(tgestate_t *state);

static
void guards_follow_suspicious_player(tgestate_t *state, vischar_t *vischar);

static
void guards_persue_prisoners(tgestate_t *state);

static
void is_item_discoverable(tgestate_t *state);

static
int is_item_discoverable_interior(tgestate_t *state,
                                 room_t      room,
                                 item_t     *pitem);

static
void item_discovered(tgestate_t *state, item_t C);

extern const default_item_location_t default_item_locations[item__LIMIT];

extern const character_meta_data_t character_meta_data[4];

/* $D000 onwards */

static
void mark_nearby_items(tgestate_t *state);

static
uint8_t sub_DBEB(tgestate_t *state, uint16_t BCdash, itemstruct_t **pIY);

static
void setup_item_plotting(tgestate_t *state, vischar_t *vischar, uint8_t A);

static
uint8_t sub_DD02(tgestate_t *state);

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
void masked_sprite_plotter_16_wide_left(tgestate_t *state, uint8_t A);
static
void masked_sprite_plotter_16_wide_right(tgestate_t *state, uint8_t A);

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
void setup_sprite_plotting(tgestate_t *state, vischar_t *vischar);

static
void scale_pos(const pos_t *in, tinypos_t *out);
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
void select_input_device(tgestate_t *state);

static
void wipe_game_window(tgestate_t *state);

static
void choose_keys(tgestate_t *state);

static
void set_menu_item_attributes(tgestate_t *state,
                              item_t      item,
                              attribute_t attrs);

static
uint8_t input_device_select_keyscan(tgestate_t *state);

static
void plot_statics_and_menu_text(tgestate_t *state);

static
void plot_static_tiles_horizontal(tgestate_t    *state,
                                  uint8_t       *DE,
                                  const uint8_t *HL);
static
void plot_static_tiles_vertical(tgestate_t      *state,
                                uint8_t       *DE,
                                const uint8_t *HL);
static
void plot_static_tiles(tgestate_t    *state,
                       uint8_t       *DE,
                       const uint8_t *HL,
                       int            orientation);

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
 * The player or a non-player character changes room.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character (was IY).
 * \param[in] pos     Pointer to pos (was HL).
 *
 * \return Nothing (non-player), or doesn't return (player).
 */
void transition(tgestate_t      *state,
                vischar_t       *vischar,
                const tinypos_t *pos)
{
  room_t room_index; /* was A */

  if (vischar->room == room_0_OUTDOORS)
  {
    /* Outdoors */

    /* Set position on Y axis, X axis and vertical offset (dividing by 4). */

    /* This was unrolled (compared to the original) to avoid having to access
     * the structure as an array. */
    vischar->mi.pos.y  = multiply_by_4(pos->y);
    vischar->mi.pos.x  = multiply_by_4(pos->x);
    vischar->mi.pos.vo = multiply_by_4(pos->vo);
  }
  else
  {
    /* Indoors */

    /* Set position on Y axis, X axis and vertical offset (copying). */

    /* This was unrolled (compared to the original) to avoid having to access
     * the structure as an array. */
    vischar->mi.pos.y  = pos->y;
    vischar->mi.pos.x  = pos->x;
    vischar->mi.pos.vo = pos->vo;
  }

  if (vischar != &state->vischars[0])
  {
    /* Not the player */

    reset_visible_character(state, vischar); // was exit via
  }
  else
  {
    /* Player only */

    vischar->flags &= ~vischar_BYTE1_BIT7;
    state->room_index = room_index = state->vischars[0].room;
    if (room_index == room_0_OUTDOORS)
    {
      /* Outdoors */

      vischar->b0D = vischar_BYTE13_BIT7; // likely a character direction
      vischar->b0E &= 3;   // likely a sprite direction
      reset_B2FC(state);
      squash_stack_goto_main(state); // exit
    }
    else
    {
      /* Indoors */

      enter_room(state); // (was fallthrough to following)
    }
    NEVER_RETURNS;
  }
}

/**
 * $68F4: Enter room.
 *
 * The player enters a room.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Doesn't return.
 */
void enter_room(tgestate_t *state)
{
  state->plot_game_window_x = 0;
  setup_room(state);
  plot_interior_tiles(state);
  set_player_sprite_for_room(state);
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
  longjmp(state->jmpbuf_main, 1);
  NEVER_RETURNS;
}

/* ----------------------------------------------------------------------- */

/**
 * $6920: Set appropriate player sprite for room.
 *
 * Called when changing rooms.
 *
 * \param[in] state Pointer to game state.
 */
void set_player_sprite_for_room(tgestate_t *state)
{
  vischar_t *player;

  player = &state->vischars[0];
  player->b0D = vischar_BYTE13_BIT7; // likely a character direction

  /* When in tunnel rooms this forces the player sprite to 'prisoner' and sets
   * the crawl flag appropriately. */
  if (state->room_index >= room_29_SECOND_TUNNEL_START)
  {
    player->b0E |= vischar_BYTE14_CRAWL;
    player->mi.spriteset = &sprites[sprite_PRISONER_FACING_TOP_LEFT_4];
  }
  else
  {
    player->b0E &= ~vischar_BYTE14_CRAWL;
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

  reset_nonplayer_visible_characters(state);

  /* Changed from the original: Updated to use a switch statement rather than
   * if-else and gotos. */

  switch (state->room_index)
  {
    case room_2_HUT2LEFT:
      movableitem = &state->movable_items[movable_item_STOVE1];
      character   = character_26;
      break;

    case room_4_HUT3LEFT:
      movableitem = &state->movable_items[movable_item_STOVE2];
      character   = character_27;
      break;

    case room_9_CRATE:
      movableitem = &state->movable_items[movable_item_CRATE];
      character   = character_28;
      break;

    default:
      assert("Unexpected room" == NULL);
  }

  setup_movable_item(state, movableitem, character);

  called_from_main_loop_7(state);
  mark_nearby_items(state);
  called_from_main_loop_9(state);
  move_map(state);
  searchlight(state);
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
  /* $69A0 */
  static const uint8_t movable_item_reset_data[14] =
  {
    0x00,             /* .flags */
    0x00, 0x00,       /* .target */
    0x00, 0x00, 0x00, /* .p04 */
    0x00,             /* .b07 */
    0xF2, 0xCD,       /* .w08 */
    0x76, 0xCF,       /* .w0A */
    0x00,             /* .b0C */
    0x00,             /* .b0D */
    0x00              /* .b0E */
  };

  vischar_t *vischar1;

  /* The movable item takes the place of the first non-player visible
   * character. */
  vischar1 = &state->vischars[1];

  vischar1->character = character;
  vischar1->mi        = *movableitem;
  memcpy(&vischar1->flags, &movable_item_reset_data[0], sizeof(movable_item_reset_data));
  vischar1->room      = state->room_index;
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
    if ((pdoorpos->room_and_flags & 0xFC) == iters) // this 0xFC mask needs a name (0x3F << 2)
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
 * \return Pointer to doorpos.
 */
const doorpos_t *get_door_position(doorindex_t index)
{
  const doorpos_t *dp;

  dp = &door_positions[(index * 2) & 0xFF];
  if (index & (1 << 7))
    dp++;

  return dp;
}

/* ----------------------------------------------------------------------- */

/**
 * $6A27: Wipe visible tiles.
 *
 * \param[in] state Pointer to game state.
 */
void wipe_visible_tiles(tgestate_t *state)
{
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
   * $EA7C: Indoor masking data.
   *
   * Conv: Constant final byte added into this data.
   */
  static const eightbyte_t stru_EA7C[47] =
  {
    { 0x1B, 0x7B, 0x7F, 0xF1, 0xF3, 0x36, 0x28, 0x20 },
    { 0x1B, 0x77, 0x7B, 0xF3, 0xF5, 0x36, 0x18, 0x20 },
    { 0x1B, 0x7C, 0x80, 0xF1, 0xF3, 0x32, 0x2A, 0x20 },
    { 0x19, 0x83, 0x86, 0xF2, 0xF7, 0x18, 0x24, 0x20 },
    { 0x19, 0x81, 0x84, 0xF4, 0xF9, 0x18, 0x1A, 0x20 },
    { 0x19, 0x81, 0x84, 0xF3, 0xF8, 0x1C, 0x17, 0x20 },
    { 0x19, 0x83, 0x86, 0xF4, 0xF8, 0x16, 0x20, 0x20 },
    { 0x18, 0x7D, 0x80, 0xF4, 0xF9, 0x18, 0x1A, 0x20 },
    { 0x18, 0x7B, 0x7E, 0xF3, 0xF8, 0x22, 0x1A, 0x20 },
    { 0x18, 0x79, 0x7C, 0xF4, 0xF9, 0x22, 0x10, 0x20 },
    { 0x18, 0x7B, 0x7E, 0xF4, 0xF9, 0x1C, 0x17, 0x20 },
    { 0x18, 0x79, 0x7C, 0xF1, 0xF6, 0x2C, 0x1E, 0x20 },
    { 0x18, 0x7D, 0x80, 0xF2, 0xF7, 0x24, 0x22, 0x20 },
    { 0x1D, 0x7F, 0x82, 0xF6, 0xF7, 0x1C, 0x1E, 0x20 },
    { 0x1D, 0x82, 0x85, 0xF2, 0xF3, 0x23, 0x30, 0x20 },
    { 0x1D, 0x86, 0x89, 0xF2, 0xF3, 0x1C, 0x37, 0x20 },
    { 0x1D, 0x86, 0x89, 0xF4, 0xF5, 0x18, 0x30, 0x20 },
    { 0x1D, 0x80, 0x83, 0xF1, 0xF2, 0x28, 0x30, 0x20 },
    { 0x1C, 0x81, 0x82, 0xF4, 0xF6, 0x1C, 0x20, 0x20 },
    { 0x1C, 0x83, 0x84, 0xF4, 0xF6, 0x1C, 0x2E, 0x20 },
    { 0x1A, 0x7E, 0x80, 0xF5, 0xF7, 0x1C, 0x20, 0x20 },
    { 0x12, 0x7A, 0x7B, 0xF2, 0xF3, 0x3A, 0x28, 0x20 },
    { 0x12, 0x7A, 0x7B, 0xEF, 0xF0, 0x45, 0x35, 0x20 },
    { 0x17, 0x80, 0x85, 0xF4, 0xF6, 0x1C, 0x24, 0x20 },
    { 0x14, 0x80, 0x84, 0xF3, 0xF5, 0x26, 0x28, 0x20 },
    { 0x15, 0x84, 0x85, 0xF6, 0xF7, 0x1A, 0x1E, 0x20 },
    { 0x15, 0x7E, 0x7F, 0xF3, 0xF4, 0x2E, 0x26, 0x20 },
    { 0x16, 0x7C, 0x85, 0xEF, 0xF3, 0x32, 0x22, 0x20 },
    { 0x16, 0x79, 0x82, 0xF0, 0xF4, 0x34, 0x1A, 0x20 },
    { 0x16, 0x7D, 0x86, 0xF2, 0xF6, 0x24, 0x1A, 0x20 },
    { 0x10, 0x76, 0x78, 0xF5, 0xF7, 0x36, 0x0A, 0x20 },
    { 0x10, 0x7A, 0x7C, 0xF3, 0xF5, 0x36, 0x0A, 0x20 },
    { 0x10, 0x7E, 0x80, 0xF1, 0xF3, 0x36, 0x0A, 0x20 },
    { 0x10, 0x82, 0x84, 0xEF, 0xF1, 0x36, 0x0A, 0x20 },
    { 0x10, 0x86, 0x88, 0xED, 0xEF, 0x36, 0x0A, 0x20 },
    { 0x10, 0x8A, 0x8C, 0xEB, 0xED, 0x36, 0x0A, 0x20 },
    { 0x11, 0x73, 0x75, 0xEB, 0xED, 0x0A, 0x30, 0x20 },
    { 0x11, 0x77, 0x79, 0xED, 0xEF, 0x0A, 0x30, 0x20 },
    { 0x11, 0x7B, 0x7D, 0xEF, 0xF1, 0x0A, 0x30, 0x20 },
    { 0x11, 0x7F, 0x81, 0xF1, 0xF3, 0x0A, 0x30, 0x20 },
    { 0x11, 0x83, 0x85, 0xF3, 0xF5, 0x0A, 0x30, 0x20 },
    { 0x11, 0x87, 0x89, 0xF5, 0xF7, 0x0A, 0x30, 0x20 },
    { 0x10, 0x84, 0x86, 0xF4, 0xF7, 0x0A, 0x30, 0x20 },
    { 0x11, 0x87, 0x89, 0xED, 0xEF, 0x0A, 0x30, 0x20 },
    { 0x11, 0x7B, 0x7D, 0xF3, 0xF5, 0x0A, 0x0A, 0x20 },
    { 0x11, 0x79, 0x7B, 0xF4, 0xF6, 0x0A, 0x0A, 0x20 },
    { 0x0F, 0x88, 0x8C, 0xF5, 0xF8, 0x0A, 0x0A, 0x20 },
  };

  const roomdef_t *proomdef; /* was HL */
  bounds_t        *pbounds;  /* was DE */
  eightbyte_t     *peb;      /* was DE */
  uint8_t          count;    /* was A */
  uint8_t          iters;    /* was B */

  wipe_visible_tiles(state);

  proomdef = rooms_and_tunnels[state->room_index - 1];

  setup_doors(state);

  state->roomdef_bounds_index = *proomdef++;

  /* Copy boundaries into state. */
  state->roomdef_object_bounds_count = count = *proomdef; /* count of boundaries */
  pbounds = &state->roomdef_object_bounds[0]; /* Conv: Moved */
  if (count == 0) /* no boundaries */
  {
    proomdef++; /* skip to TBD */
  }
  else
  {
    proomdef++; /* Conv: Don't re-copy already copied count byte. */
    memcpy(pbounds, proomdef, count * 4);
    proomdef += count * 4; /* skip to TBD */
  }

  /* Copy eightbyte structs into state->indoor_mask_data. */
  state->indoor_mask_data_count = iters = *proomdef++; /* count of TBDs */
  peb = &state->indoor_mask_data[0]; /* Conv: Moved */
  while (iters--)
  {
    /* Conv: Structures changed from 7 to 8 bytes wide. */
    memcpy(peb, &stru_EA7C[*proomdef++], 8);
    peb += 8;
  }

  /* Plot all objects (as tiles). */
  iters = *proomdef++; /* Count of objects */
  while (iters--)
  {
    expand_object(state,
                  proomdef[0], /* object index */
                  &state->tile_buf[0] + proomdef[2] * state->columns + proomdef[1]); /* HL[2] = row, HL[1] = column */
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
  int                columns;
  const tgeobject_t *obj;
  int                width, height;
  int                saved_width;
  const uint8_t     *data;
  int                byte;
  int                val;

  columns     = state->columns;

  obj         = interior_object_defs[index];

  width       = obj->width;
  height      = obj->height;

  saved_width = width;

  data        = &obj->data[0];

  do
  {
    do
    {
expand:
      byte = *data;
      if (byte == interiorobjecttile_ESCAPE)
      {
        byte = *++data;
        if (byte != interiorobjecttile_ESCAPE)
        {
          if (byte >= 128)
            goto run;
          if (byte >= 64)
            goto range;
        }
      }

      if (byte)
        *output = byte;
      data++;
      output++;
    }
    while (--width);

    width = saved_width;
    output += columns - width;
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
      if (--height == 0)
        return;

      output += columns - saved_width; /* move to next row */
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
      if (--height == 0)
        return;

      output += columns - saved_width; /* move to next row */
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
  int                rows;
  int                columns;
  uint8_t           *window_buf;
  const tileindex_t *tiles_buf;
  int                rowcounter;
  int                columncounter;
  int                bytes;
  int                stride;

  rows       = state->rows;
  columns    = state->columns;

  window_buf = state->window_buf;
  tiles_buf  = state->tile_buf;

  rowcounter = rows;
  do
  {
    columncounter = columns;
    do
    {
      const tilerow_t *tile_data;

      tile_data = &interior_tiles[*tiles_buf++].row[0];

      bytes  = 8;
      stride = columns;
      do
      {
        *window_buf = *tile_data++;
        window_buf += stride;
      }
      while (--bytes);

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
  { BYTE0(room_0_OUTDOORS,           1), { 0xB2, 0x8A,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0xB2, 0x8E,  6 } },
  { BYTE0(room_0_OUTDOORS,           1), { 0xB2, 0x7A,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0xB2, 0x7E,  6 } },
  { BYTE0(room_34,                   0), { 0x8A, 0xB3,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x10, 0x34, 12 } },
  { BYTE0(room_48,                   0), { 0xCC, 0x79,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x10, 0x34, 12 } },
  { BYTE0(room_28_HUT1LEFT,          1), { 0xD9, 0xA3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x2A, 0x1C, 24 } },
  { BYTE0(room_1_HUT1RIGHT,          0), { 0xD4, 0xBD,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x1E, 0x2E, 24 } },
  { BYTE0(room_2_HUT2LEFT,           1), { 0xC1, 0xA3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x2A, 0x1C, 24 } },
  { BYTE0(room_3_HUT2RIGHT,          0), { 0xBC, 0xBD,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x20, 0x2E, 24 } },
  { BYTE0(room_4_HUT3LEFT,           1), { 0xA9, 0xA3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x2A, 0x1C, 24 } },
  { BYTE0(room_5_HUT3RIGHT,          0), { 0xA4, 0xBD,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x20, 0x2E, 24 } },
  { BYTE0(room_21_CORRIDOR,          0), { 0xFC, 0xCA,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x1C, 0x24, 24 } },
  { BYTE0(room_20_REDCROSS,          0), { 0xFC, 0xDA,  6 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x1A, 0x22, 24 } },
  { BYTE0(room_15_UNIFORM,           1), { 0xF7, 0xE3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x26, 0x19, 24 } },
  { BYTE0(room_13_CORRIDOR,          1), { 0xDF, 0xE3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x2A, 0x1C, 24 } },
  { BYTE0(room_8_CORRIDOR,           1), { 0x97, 0xD3,  6 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x2A, 0x15, 24 } },
  { BYTE0(room_6,                    1), { 0x00, 0x00,  0 } },
  { BYTE0(room_0_OUTDOORS,           3), { 0x22, 0x22, 24 } },
  { BYTE0(room_1_HUT1RIGHT,          1), { 0x2C, 0x34, 24 } },
  { BYTE0(room_28_HUT1LEFT,          3), { 0x26, 0x1A, 24 } },
  { BYTE0(room_3_HUT2RIGHT,          1), { 0x24, 0x36, 24 } },
  { BYTE0(room_2_HUT2LEFT,           3), { 0x26, 0x1A, 24 } },
  { BYTE0(room_5_HUT3RIGHT,          1), { 0x24, 0x36, 24 } },
  { BYTE0(room_4_HUT3LEFT,           3), { 0x26, 0x1A, 24 } },
  { BYTE0(room_23_BREAKFAST,         1), { 0x28, 0x42, 24 } },
  { BYTE0(room_25_BREAKFAST,         3), { 0x26, 0x18, 24 } },
  { BYTE0(room_23_BREAKFAST,         0), { 0x3E, 0x24, 24 } },
  { BYTE0(room_21_CORRIDOR,          2), { 0x20, 0x2E, 24 } },
  { BYTE0(room_19_FOOD,              1), { 0x22, 0x42, 24 } },
  { BYTE0(room_23_BREAKFAST,         3), { 0x22, 0x1C, 24 } },
  { BYTE0(room_18_RADIO,             1), { 0x24, 0x36, 24 } },
  { BYTE0(room_19_FOOD,              3), { 0x38, 0x22, 24 } },
  { BYTE0(room_21_CORRIDOR,          1), { 0x2C, 0x36, 24 } },
  { BYTE0(room_22_REDKEY,            3), { 0x22, 0x1C, 24 } },
  { BYTE0(room_22_REDKEY,            1), { 0x2C, 0x36, 24 } },
  { BYTE0(room_23_SOLITARY,          3), { 0x2A, 0x26, 24 } },
  { BYTE0(room_12_CORRIDOR,          1), { 0x42, 0x3A, 24 } },
  { BYTE0(room_18_RADIO,             3), { 0x22, 0x1C, 24 } },
  { BYTE0(room_17_CORRIDOR,          0), { 0x3C, 0x24, 24 } },
  { BYTE0(room_7_CORRIDOR,           2), { 0x1C, 0x22, 24 } },
  { BYTE0(room_15_UNIFORM,           0), { 0x40, 0x28, 24 } },
  { BYTE0(room_14_TORCH,             2), { 0x1E, 0x28, 24 } },
  { BYTE0(room_16_CORRIDOR,          1), { 0x22, 0x42, 24 } },
  { BYTE0(room_14_TORCH,             3), { 0x22, 0x1C, 24 } },
  { BYTE0(room_16_CORRIDOR,          0), { 0x3E, 0x2E, 24 } },
  { BYTE0(room_13_CORRIDOR,          2), { 0x1A, 0x22, 24 } },
  { BYTE0(room_0_OUTDOORS,           0), { 0x44, 0x30, 24 } },
  { BYTE0(room_0_OUTDOORS,           2), { 0x20, 0x30, 24 } },
  { BYTE0(room_13_CORRIDOR,          0), { 0x4A, 0x28, 24 } },
  { BYTE0(room_11_PAPERS,            2), { 0x1A, 0x22, 24 } },
  { BYTE0(room_7_CORRIDOR,           0), { 0x40, 0x24, 24 } },
  { BYTE0(room_16_CORRIDOR,          2), { 0x1A, 0x22, 24 } },
  { BYTE0(room_10_LOCKPICK,          0), { 0x36, 0x35, 24 } },
  { BYTE0(room_8_CORRIDOR,           2), { 0x17, 0x26, 24 } },
  { BYTE0(room_9_CRATE,              0), { 0x36, 0x1C, 24 } },
  { BYTE0(room_8_CORRIDOR,           2), { 0x1A, 0x22, 24 } },
  { BYTE0(room_12_CORRIDOR,          0), { 0x3E, 0x24, 24 } },
  { BYTE0(room_17_CORRIDOR,          2), { 0x1A, 0x22, 24 } },
  { BYTE0(room_29_SECOND_TUNNEL_START, 1), { 0x36, 0x36, 24 } },
  { BYTE0(room_9_CRATE,              3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_52,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_30,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_30,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_31,                   2), { 0x38, 0x26, 12 } },
  { BYTE0(room_30,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_36,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_31,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_32,                   2), { 0x0A, 0x34, 12 } },
  { BYTE0(room_32,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_33,                   3), { 0x20, 0x34, 12 } },
  { BYTE0(room_33,                   1), { 0x40, 0x34, 12 } },
  { BYTE0(room_35,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_35,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_34,                   2), { 0x0A, 0x34, 12 } },
  { BYTE0(room_36,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_35,                   2), { 0x38, 0x1C, 12 } },
  { BYTE0(room_37,                   0), { 0x3E, 0x22, 24 } }, /* Tunnel entrance */
  { BYTE0(room_2_HUT2LEFT,           2), { 0x10, 0x34, 12 } },
  { BYTE0(room_38,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_37,                   2), { 0x10, 0x34, 12 } },
  { BYTE0(room_39,                   1), { 0x40, 0x34, 12 } },
  { BYTE0(room_38,                   3), { 0x20, 0x34, 12 } },
  { BYTE0(room_40,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_38,                   2), { 0x38, 0x54, 12 } },
  { BYTE0(room_40,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_41,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_41,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_42,                   2), { 0x38, 0x26, 12 } },
  { BYTE0(room_41,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_45,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_45,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_44,                   2), { 0x38, 0x1C, 12 } },
  { BYTE0(room_43,                   1), { 0x20, 0x34, 12 } },
  { BYTE0(room_44,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_42,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_43,                   3), { 0x20, 0x34, 12 } },
  { BYTE0(room_46,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_39,                   2), { 0x38, 0x1C, 12 } },
  { BYTE0(room_47,                   1), { 0x38, 0x62, 12 } },
  { BYTE0(room_46,                   3), { 0x20, 0x34, 12 } },
  { BYTE0(room_50_BLOCKED_TUNNEL,    0), { 0x64, 0x34, 12 } },
  { BYTE0(room_47,                   2), { 0x38, 0x56, 12 } },
  { BYTE0(room_50_BLOCKED_TUNNEL,    1), { 0x38, 0x62, 12 } },
  { BYTE0(room_49,                   3), { 0x38, 0x0A, 12 } },
  { BYTE0(room_49,                   0), { 0x64, 0x34, 12 } },
  { BYTE0(room_48,                   2), { 0x38, 0x1C, 12 } },
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
  use_item_common(state, state->items_held[1]);
}

/**
 * $7AF5: Use item 'A'.
 *
 * \param[in] state Pointer to game state.
 */
void use_item_A(tgestate_t *state)
{
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

  if (item == item_NONE)
    return;

  memcpy(&state->saved_pos, &state->vischars[0].mi.pos, sizeof(pos_t));

  // ToDo: Check, is a jump to NULL avoided by some mechanism?

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
  itemstruct_t *item;
  item_t       *DE;
  item_t        A;
  attribute_t   attrs;

  if (state->items_held[0] != item_NONE &&
      state->items_held[1] != item_NONE)
    return; /* no spare slots */

  item = find_nearby_item(state);
  if (item == NULL)
    return; /* not found */

  /* Locate the empty item slot. */
  DE = &state->items_held[0];
  A = *DE;
  if (A != item_NONE)
    DE++;
  *DE = item->item & 0x1F; // what is the meaning of this mask? (== itemstruct_ITEM_MASK + (1<<4))

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

  if ((item->item & itemstruct_ITEM_FLAG_HELD) == 0)
  {
    /* Have picked up an item not previously held. */
    item->item |= itemstruct_ITEM_FLAG_HELD;
    increase_morale_by_5_score_by_5(state);
  }

  item->room   = 0;
  item->target = 0;

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

  drop_item_A(state, item);
}

/**
 * $7BB5: Drop item. Part "A".
 *
 * \param[in] state Pointer to game state.
 * \param[in] item  Item.                  (was A)
 */
void drop_item_A(tgestate_t *state, item_t item)
{
  itemstruct_t *is;     /* was HL */
  room_t        room;   /* was A */
  tinypos_t    *outpos; /* was DE */
  pos_t        *inpos;  /* was HL */

  is = item_to_itemstruct(state, item);
  room = state->room_index;
  is->room = room; /* Set object's room index. */
  if (room == room_0_OUTDOORS)
  {
    /* Outdoors. */

    outpos = &is->pos;
    inpos  = &state->vischars[0].mi.pos;

    scale_pos(inpos, outpos);
    outpos->vo = 0;

    drop_item_A_exterior_tail(is);
  }
  else
  {
    /* Indoors. */

    outpos = &is->pos;
    inpos  = &state->vischars[0].mi.pos;

    outpos->y  = inpos->y;
    outpos->x  = inpos->x;
    outpos->vo = 5;

    drop_item_A_interior_tail(is);
  }
}

/**
 * $7BD0: Assign target member for dropped exterior objects.
 *
 * Moved out to provide entry point.
 *
 * Called from drop_item_A, item_discovered.
 *
 * \param[in] HL Pointer to item struct.
 */
void drop_item_A_exterior_tail(itemstruct_t *HL)
{
  uint8_t *t = (uint8_t *) &HL->target;

  t[0] = (0x40 + HL->pos.x - HL->pos.y) * 2;
  t[1] = 0 - HL->pos.y - HL->pos.x - HL->pos.vo;
}

/**
 * $7BF2: Assign target member for dropped interior objects.
 *
 * Moved out to provide entry point.
 *
 * Called from drop_item_A, item_discovered.
 *
 * \param[in] HL Pointer to item struct.
 */
void drop_item_A_interior_tail(itemstruct_t *HL)
{
  uint8_t *t = (uint8_t *) &HL->target;

  // This was a call to divide_by_8_with_rounding, but that expects
  // separate hi and lo arguments, which is not worth the effort of
  // mimicing the original code.
  //
  // This needs to go somewhere more general.
#define divround(x) (((x) + 4) >> 3)

  t[0] = divround((0x200 + HL->pos.x - HL->pos.y) * 2);
  t[1] = divround(0x800 - HL->pos.y - HL->pos.x - HL->pos.vo);

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
  return &state->item_structs[item];
}

/* ----------------------------------------------------------------------- */

/**
 * $7C33: Draw all items.
 *
 * \param[in] state Pointer to game state.
 */
void draw_all_items(tgestate_t *state)
{
  draw_item(state, state->items_held[0], &state->speccy->screen[0x5087 - SCREEN_START_ADDRESS]);
  draw_item(state, state->items_held[1], &state->speccy->screen[0x508A - SCREEN_START_ADDRESS]);
}

/**
 * $7C46: Draw an item.
 *
 * \param[in] state Pointer to game state.
 * \param[in] index Item index.     (was A)
 * \param[in] dst   Screen address. (was HL)
 */
void draw_item(tgestate_t *state, item_t index, uint8_t *dst)
{
  attribute_t     A;
  attribute_t    *HLattrs; // was HL
  const sprite_t *HLitem;  // was HL

  /* Wipe item. */
  screen_wipe(state, 2, 16, dst);

  if (index == item_NONE)
    return;

  /* Set screen attributes. */
  // This will need to be made a lot more general.
  // This 0xFF00 ANDing is wrong. Needs to be a subtraction.
  HLattrs = ((intptr_t) dst & ~0xFF00) + &state->speccy->attributes[0x5A00 - SCREEN_ATTRIBUTES_START_ADDRESS];
  A = state->item_attributes[index];
  HLattrs[0] = A;
  HLattrs[1] = A;

  // move to next attribute row
  HLattrs += state->width;
  HLattrs[0] = A;
  HLattrs[1] = A;

  /* Plot bitmap. */
  HLitem = &item_definitions[index];
  plot_bitmap(state, HLitem->width, HLitem->height, HLitem->data, dst);
}

/* ----------------------------------------------------------------------- */

/**
 * $7C82: Returns an item within range of the player.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Pointer to item, or NULL.
 */
itemstruct_t *find_nearby_item(tgestate_t *state)
{
  uint8_t       radius; /* was C */
  uint8_t       iters;  /* was B */
  itemstruct_t *is;     /* was HL */

  /* Select a pick up radius. */
  radius = 1; /* Outdoors. */
  if (state->room_index > 0)
    radius = 6; /* Indoors. */

  iters = item__LIMIT;
  is    = &state->item_structs[0];
  do
  {
    if (is->room & itemstruct_ROOM_FLAG_ITEM_NEARBY)
    {
      uint8_t *structcoord;
      uint8_t *playercoord;
      uint8_t  coorditers;

      /* Note that we're comparing 'y' to 'x' here. This is because I need to
       * rename all of the other variables to use the x/y sense. */
      structcoord = &is->pos.y;
      playercoord = &state->player_map_position.y;
      coorditers = 2; // 2 iterations
      /* Range check. */
      do
      {
        uint8_t A;

        A = *playercoord++; /* get player map position */
        if (A - radius >= *structcoord || A + radius < *structcoord)
          goto next;
        structcoord++; // y then x
      }
      while (--coorditers);

      return is;
    }

next:
    is++;
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
  do
  {
    memcpy(dst, src, width);
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
  uint8_t *const screen = &state->speccy->screen[0];
  uint16_t       HL;
  uint16_t       DE;

  HL = slp - screen;

  HL += 0x0100;
  if (HL & 0x0700)
    return screen + HL; /* line count didn't rollover */

  if ((HL & 0xFF) >= 0xE0)
    DE = 0xFF20;
  else
    DE = 0xF820;

  HL += DE; /* needs to be a 16-bit add! */

  return screen + HL;
}

/* ----------------------------------------------------------------------- */

/**
 * $7D15: Add a message to the display queue.
 *
 * Conversion note: The original code accepts BC combined as the message index.
 * However only one of the callers sets up C. We therefore ignore the second
 * argument here, treating it as zero.
 *
 * \param[in] state         Pointer to game state.
 * \param[in] message_index The message_t to display. (was B)
 */
void queue_message_for_display(tgestate_t *state,
                               message_t   message_index)
{
  uint8_t *qp; /* was HL */

  qp = state->message_queue_pointer; /* insertion point pointer */
  if (*qp == message_NONE)
    return;

  /* Are we already about to show this message? */
  if (qp[-2] == message_index && qp[-1] == 0)
    return;

  /* Add it to the queue. */
  qp[0] = message_index;
  qp[1] = 0;
  state->message_queue_pointer = qp + 2;
}

/* ----------------------------------------------------------------------- */

/**
 * $7D2F: Indirectly plot a glyph.
 *
 * \param[in] pcharacter Pointer to character to plot. (was HL)
 * \param[in] output     Where to plot.                (was DE)
 *
 * \return Pointer to next character along.
 */
uint8_t *plot_glyph(const char *pcharacter, uint8_t *output)
{
  return plot_single_glyph(*pcharacter, output);
}

/**
 * $7D30: Plot a single glyph.
 *
 * \param[in] character Character to plot. (was HL)
 * \param[in] output    Where to plot.     (was DE)
 *
 * \return Pointer to next character along.
 */
uint8_t *plot_single_glyph(int character, uint8_t *output)
{
  const tile_t    *glyph; /* was HL */
  const tilerow_t *row;   /* was HL */
  int              iters; /* was B */

  glyph = &bitmap_font[ascii_to_font[character]];
  row   = &glyph->row[0];
  iters = 8; // 8 iterations
  do
  {
    *output = *row++;
    output += 256; // next row
  }
  while (--iters);

  return ++output; // return the next character position
}

/* ----------------------------------------------------------------------- */

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

  if (state->message_display_counter > 0)
  {
    state->message_display_counter--;
    return;
  }

  index = state->message_display_index;
  if (index == 128) // hoist out this flag
  {
    next_message(state);
  }
  else if (index > 128)
  {
    wipe_message(state);
  }
  else
  {
    pmsgchr = state->current_message_character;
    pscr    = &state->speccy->screen[screen_text_start_address + index];
    (void) plot_glyph(pmsgchr, pscr);

    state->message_display_index = (intptr_t) pscr & 31; // FIXME: Dodgy binary op on pointer

    if (*++pmsgchr == '\0') /* end of string (0xFF in original game) */
    {
      /* Leave the message for 31 turns, then wipe it. */
      state->message_display_counter = 31;
      state->message_display_index |= 1 << 7;
    }
    else
    {
      state->current_message_character = pmsgchr;
    }
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $7D87: Incrementally wipe away any on-screen game message.
 *
 * Called while message_display_index > 128.
 *
 * \param[in] state Pointer to game state.
 */
void wipe_message(tgestate_t *state)
{
  int      index; /* was A */
  uint8_t *scr;   /* was DE */

  index = state->message_display_index;
  state->message_display_index = --index;
  scr = &state->speccy->screen[screen_text_start_address + index];

  /* Plot a SPACE character. */
  (void) plot_single_glyph(' ', scr);
}

/* ----------------------------------------------------------------------- */

/**
 * $7D99: Change to displaying the next queued game message.
 *
 * Called when message_display_index == 128.
 *
 * \param[in] state Pointer to game state.
 */
void next_message(tgestate_t *state)
{
  /**
   * $7DCD: Table of game messages.
   *
   * These are 0xFF terminated in the original game.
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

  qp = &state->message_queue[0];
  if (state->message_queue_pointer == qp)
    return;

  /* The message ID is stored in the buffer itself. */
  message = messages_table[*qp];

  state->current_message_character = message;
  memmove(state->message_queue, state->message_queue + 2, 16);
  state->message_queue_pointer -= 2;
  state->message_display_index = 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $9D7B: Main game loop.
 */
void main_loop(tgestate_t *state)
{
  setjmp(state->jmpbuf_main);

  for (;;)
  {
    check_morale(state);
    keyscan_cancel(state);
    message_display(state);
    process_player_input(state);
    in_permitted_area(state);
    called_from_main_loop_3(state);
    move_characters(state);
    follow_suspicious_player(state);
    called_from_main_loop_6(state);
    called_from_main_loop_7(state);
    mark_nearby_items(state);
    ring_bell(state);
    called_from_main_loop_9(state);
    move_map(state);
    message_display(state);
    ring_bell(state);
    searchlight(state);
    plot_game_window(state);
    ring_bell(state);
    if (state->day_or_night)
      nighttime(state);
    if (state->room_index)
      interior_delay_loop();
    wave_morale_flag(state);
    if ((state->game_counter & 63) == 0)
      dispatch_timed_event(state);

    // FIXME: Infinite loop. Yield here.
    // eg. state->speccy->yield(state->speccy);
  }
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
  if (state->morale >= 2)
    return;

  queue_message_for_display(state, message_MORALE_IS_ZERO);

  /* Inhibit user input. */
  state->morale_2 = 0xFF;

  /* Immediately assume automatic control of player. */
  state->automatic_player_counter = 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $9DE5: Check for 'game cancel' keypress.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Returns only if game not canceled.
 */
void keyscan_cancel(tgestate_t *state)
{
  /* Is space pressed? */
  if (state->speccy->in(state->speccy, port_KEYBOARD_SPACESYMSHFTMNB) & 0x01)
    return; /* not pressed */

  /* Is shift pressed? */
  if (state->speccy->in(state->speccy, port_KEYBOARD_SHIFTZXCV)       & 0x01)
    return; /* not pressed */

  screen_reset(state);
  if (user_confirm(state) == 0)
    reset_game(state);

  if (state->room_index == 0)
  {
    reset_B2FC(state);
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

  /* Is morale remaining? */
  if (state->morale_1 || state->morale_2)
    return; /* none remains */

  if (state->vischars[0].flags & (vischar_BYTE1_PICKING_LOCK | vischar_BYTE1_CUTTING_WIRE))
  {
    /* Picking a lock, or cutting through a wire fence. */

    state->automatic_player_counter = 31; /* hold off on automatic control */

    if (state->vischars[0].flags == vischar_BYTE1_PICKING_LOCK)
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

    state->automatic_player_counter--; /* no user input: count down */
    input = input_NONE;
  }
  else
  {
    /* Input received */

    state->automatic_player_counter = 31; /* wait 31 turns until automatic control */

    if (state->player_in_bed || state->player_in_breakfast)
    {
      if (state->player_in_bed == 0)
      {
        /* Player was at breakfast. */
        state->vischars[0].target   = 0x002B;
        state->vischars[0].mi.pos.y = 0x34;
        state->vischars[0].mi.pos.x = 0x3E;
        roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_EMPTY_BENCH;
        state->player_in_breakfast = 0;
      }
      else
      {
        /* Player was in bed. */
        state->vischars[0].target    = 0x012C;
        state->vischars[0].p04.y     = 0x2E;
        state->vischars[0].p04.x     = 0x2E;
        state->vischars[0].mi.pos.y  = 0x2E;
        state->vischars[0].mi.pos.x  = 0x2E;
        state->vischars[0].mi.pos.vo = 24;
        roomdef_2_hut2_left[roomdef_2_BED] = interiorobject_EMPTY_BED;
        state->player_in_bed = 0;
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
 * Locks the player out until lock is picked.
 *
 * \param[in] state Pointer to game state.
 */
void picking_a_lock(tgestate_t *state)
{
  if (state->player_locked_out_until != state->game_counter)
    return;

  *state->ptr_to_door_being_lockpicked &= ~gates_and_doors_LOCKED; /* unlock */
  queue_message_for_display(state, message_IT_IS_OPEN);

  state->vischars[0].flags &= ~(vischar_BYTE1_PICKING_LOCK | vischar_BYTE1_CUTTING_WIRE);
}

/* ----------------------------------------------------------------------- */

/**
 * $9EB2: Snipping wire.
 *
 * Locks the player out until wire is snipped.
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

  uint8_t delta; /* was A */

  delta = state->player_locked_out_until - state->game_counter;
  if (delta)
  {
    if (delta < 4)
      state->vischars[0].b0D = table_9EE0[state->vischars[0].b0E & 3]; // new direction?
  }
  else
  {
    state->vischars[0].b0E = delta & 3; // set crawl flag?
    state->vischars[0].b0D = vischar_BYTE13_BIT7;
    state->vischars[0].mi.pos.vo = 24; /* set vertical offset */

    /* The original code jumps into the tail end of picking_a_lock above to do
     * this. */
    state->vischars[0].flags &= ~(vischar_BYTE1_PICKING_LOCK | vischar_BYTE1_CUTTING_WIRE);
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
   * $9EF9: ...
   */
  /* 0xFF terminated */
  static const uint8_t byte_9EF9[] = { 0x82, 0x82, 0xFF                         };
  static const uint8_t byte_9EFC[] = { 0x83, 0x01, 0x01, 0x01, 0xFF             };
  static const uint8_t byte_9F01[] = { 0x01, 0x01, 0x01, 0x00, 0x02, 0x02, 0xFF };
  static const uint8_t byte_9F08[] = { 0x01, 0x01, 0x95, 0x97, 0x99, 0xFF       };
  static const uint8_t byte_9F0E[] = { 0x83, 0x82, 0xFF                         };
  static const uint8_t byte_9F11[] = { 0x99, 0xFF                               };
  static const uint8_t byte_9F13[] = { 0x01, 0xFF                               };

  /**
   * $9EE4: ...
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
  uint8_t      red_flag;  /* was A */
  uint8_t      A, C;
  uint8_t      D, E;

  vcpos = &state->vischars[0].mi.pos;
  pos = &state->player_map_position;
  if (state->room_index == 0)
  {
    /* Outdoors. */

    scale_pos(vcpos, pos);

    if (state->vischars[0].w18 >= 0x06C8 ||
        state->vischars[0].w1A >= 0x0448)
    {
      escaped(state);
      return;
    }
  }
  else
  {
    /* Indoors. */

    pos->y  = vcpos->y;
    pos->x  = vcpos->x;
    pos->vo = vcpos->vo;
  }

  /* Red flag if picking a lock, or cutting wire. */

  if (state->vischars[0].flags & (vischar_BYTE1_PICKING_LOCK | vischar_BYTE1_CUTTING_WIRE))
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
#if 0
  A = *locptr++; // this will do a word read, not byte as expected
  C = *locptr;
  if (A & vischar_BYTE2_BIT7)
    C++;
  if (A == 0xFF)
  {
    A = ((*locptr * 0xF8) == 8) ? 1 : 2;
    if (in_permitted_area_end_bit(state, A) == 0)
      goto set_flag_green;
    else
      goto set_flag_red;
  }
  else
  {
    byte_to_pointer_t *tab;
    uint8_t            B;

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
    if (in_permitted_area_end_bit(state, tab->pointer[BC & 0xFF]) == 0) // why does this mask?
      goto set_flag_green;

    if (state->vischars[0].target & vischar_BYTE2_BIT7) // low byte only
      HL++;
    BC = 0;
    for (;;)
    {
      // PUSH BC
      HL += BC;
      A = *HL;
      if (A == 0xFF) // end of list
        goto pop_and_set_flag_red;
      // POP HL, BC // was interleaved
      if (in_permitted_area_end_bit(state, A) == 0)
        goto set_target_then_set_flag_green;
      BC++;
    }

set_target_then_set_flag_green:
    // oddness here needs checking out. only 'B' is loaded from the target, but
    // set_player_target_location needs BC
    set_player_target_location(state, state->vischars[0].target);
    goto set_flag_green;

pop_and_set_flag_red:
    // POP BC, POP HL
    goto set_flag_red;
  }
#endif

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
 * \param[in] room_and_flags Room index + flag in bit 7?
 *
 * \return 0 if in permitted? area, 1 otherwise.
 */
int in_permitted_area_end_bit(tgestate_t *state, uint8_t room_and_flags)
{
  room_t *proom; /* was HL */

  // FUTURE: Just dereference room_index once.

  proom = &state->room_index;

  if (room_and_flags & (1 << 7))
    return *proom == (room_and_flags & 0x7F); // return with flags

  if (*proom)
    return 0; /* Indoors. */

  return in_numbered_area(state, 0, &state->player_map_position);
}

/**
 * $A01A: Is the specified position within the numbered area?
 *
 * I suspect this detects objects which lie within in the main body of the camp,
 * but not outside its boundaries.
 *
 * \param[in] state Pointer to game state.
 * \param[in] index Index (0..2) into bounds[] table. (was A)
 * \param[in] pos   Pointer to position.              (was HL)
 *
 * \return 0 if in permitted? area, 1 otherwise.
 */
int in_numbered_area(tgestate_t *state, uint8_t index, const tinypos_t *pos)
{
  /**
   * $9F15: Pairs of low-high bounds.
   */
  static const bounds_t bounds[3] =
  {
    { 0x56,0x5E, 0x3D,0x48 },
    { 0x4E,0x84, 0x47,0x74 },
    { 0x4F,0x69, 0x2F,0x3F },
  };

  const bounds_t *bound; /* was HL */

  bound = &bounds[index];
  return pos->y < bound->a || pos->y >= bound->b ||
         pos->x < bound->c || pos->x >= bound->d;
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
  uint8_t *pattrs; /* was HL */
  int      iters;  /* was B */

  pattrs = &state->speccy->attributes[morale_flag_attributes_offset];
  iters = 19; /* height of flag */
  do
  {
    pattrs[0] = attrs;
    pattrs[1] = attrs;
    pattrs += state->width; // stride (== attributes width)
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
 * Delay loop called only when the player is indoors.
 */
void interior_delay_loop(void)
{
  // FIXME: Long loop. Yield here.
  // eg. state->speccy->yield(state->speccy);

  volatile int BC = 0xFFF;
  while (--BC)
    ;
}

/* ----------------------------------------------------------------------- */

/* Offset. */
#define screenoffset_BELL_RINGER 0x118E

/**
 * $A09E: Ring bell.
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

  pbell = &state->bell;
  bell = *pbell;
  if (bell == bell_STOP)
    return; /* not ringing */

  if (bell != bell_RING_PERPETUAL)
  {
    /* Decrement the ring counter. */
    *pbell = --bell;
    if (bell == 0)
    {
      *pbell = bell_STOP; /* counter hit zero - stop ringing */
      return;
    }
  }

  /* Fetch visible state of bell. */
  bell = state->speccy->screen[screenoffset_BELL_RINGER];
  if (bell != 0x3F) /* pixel value is 0x3F if on */
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

  decreased_morale = state->morale - delta;
  if (decreased_morale < morale_MIN)
    decreased_morale = morale_MIN;

  /* This goto's into the tail end of increase_morale in the original code. */
  state->morale = decreased_morale;
}

/**
 * $A0E9: Increase morale by 10, score by 50.
 *
 * \param[in] state Pointer to game state.
 */
void increase_morale_by_10_score_by_50(tgestate_t *state)
{
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

  /* Increment the score digit-wise until delta is zero. */

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

  digits = &state->score_digits[0];
  screen = &state->speccy->screen[score_address];
  iters = NELEMS(state->score_digits);
  do
    (void) plot_glyph(digits++, screen++);
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

  iters = sound >> 8;
  delay = sound & 0xFF;

  speakerbit = 16; /* Set speaker bit. */
  do
  {
    state->speccy->out(state->speccy, port_BORDER, speakerbit); /* Play. */

    /* Conv: Removed self-modified counter. */
    subcount = delay;
    while (subcount--)
      ;

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
  uint8_t      rowcount;   /* was C */
  uint16_t     stride;     /* was DE */

  attributes = &state->speccy->attributes[0x0047];
  rowcount = state->rows;
  stride = state->width - (state->columns - 1); // eg. 9
  do
  {
    uint8_t iters;

    iters = state->columns - 1; // eg. 23 // CHECK. is state->columns is storing 23 or 24?
    do
      *attributes++ = attrs;
    while (--iters);
    attrs += stride;
  }
  while (--rowcount);
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

  uint8_t            *pcounter; /* was HL */
  uint8_t             time;     /* was A */
  const timedevent_t *event;    /* was HL */
  uint8_t             iters;    /* was B */

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
  if (state->player_in_bed == 0)
    set_player_target_location(state, location_2C01);
  set_day_or_night(state, 0xFF);
}

void event_another_day_dawns(tgestate_t *state)
{
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

  state->day_or_night = day_night; // night=0xFF, day=0x00
  attrs = choose_game_window_attributes(state);
  set_game_window_attributes(state, attrs);
}

void event_wake_up(tgestate_t *state)
{
  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_TIME_TO_WAKE_UP);
  wake_up(state);
}

void event_go_to_roll_call(tgestate_t *state)
{
  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_ROLL_CALL);
  go_to_roll_call(state);
}

void event_go_to_breakfast_time(tgestate_t *state)
{
  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_BREAKFAST_TIME);
  set_location_0x1000(state);
}

void event_breakfast_time(tgestate_t *state)
{
  state->bell = bell_RING_40_TIMES;
  breakfast_time(state);
}

void event_go_to_exercise_time(tgestate_t *state)
{
  state->bell = bell_RING_40_TIMES;
  queue_message_for_display(state, message_EXERCISE_TIME);

  /* Unlock the gates. */
  state->gates_and_doors[0] = 0x00;
  state->gates_and_doors[1] = 0x01;

  set_location_0x0E00(state);
}

void event_exercise_time(tgestate_t *state)
{
  state->bell = bell_RING_40_TIMES;
  set_location_0x8E04(state);
}

void event_go_to_time_for_bed(tgestate_t *state)
{
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

  /* Don't deliver a new red cross parcel while the previous one still
   * exists. */
  if ((state->item_structs[item_RED_CROSS_PARCEL].room & itemstruct_ROOM_MASK) != itemstruct_ROOM_MASK)
    return;

  /* Select the contents of the next parcel; the first item from the list which
   * does not already exist. */
  item = &red_cross_parcel_contents_list[0];
  iters = NELEMS(red_cross_parcel_contents_list);
  do
  {
    itemstruct_t *itemstruct; /* was HL */

    itemstruct = item_to_itemstruct(state, *item);
    if ((itemstruct->room & itemstruct_ROOM_MASK) == itemstruct_ROOM_MASK)
      goto found; // later, remove goto

    item++;
  }
  while (--iters);

  return;

found:
  state->red_cross_parcel_current_contents = *item;
  memcpy(&state->item_structs[item_RED_CROSS_PARCEL].room, &red_cross_parcel_reset_data.room, 6);
  queue_message_for_display(state, message_RED_CROSS_PARCEL);
}

void event_time_for_bed(tgestate_t *state)
{
  uint8_t A, C;

  A = 0xA6;
  C = 3;
  common_A26E(state, A, C);
}

void event_search_light(tgestate_t *state)
{
  uint8_t A, C;

  A = 0x26;
  C = 0;
  common_A26E(state, A, C);
}

/**
 * Common end of event_time_for_bed and event_search_light.
 */
void common_A26E(tgestate_t *state, uint8_t A, uint8_t C)
{
  uint8_t Adash;
  uint8_t iters; /* was B */

  Adash = 12; // counter
  iters = 4; // 4 iterations
  do
  {
    sub_A38C(state, Adash, A, C);
    Adash++;
    A++;
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A27F: (unknown). An array ten bytes long.
 *
 * Likely a list of character indexes.
 *
 * Used by set_location_A35F and set_location_A373.
 */
const character_t tenlong[10] =
{
  character_12,
  character_13,
  character_20,
  character_21,
  character_22,
  character_14,
  character_15,
  character_23,
  character_24,
  character_25
};

/* ----------------------------------------------------------------------- */

/**
 * $A289: Wake up.
 *
 * \param[in] state Pointer to game state.
 */
void wake_up(tgestate_t *state)
{
  characterstruct_t *charstr; /* was HL */
  uint8_t            iters;   /* was B */
  uint8_t *const    *bedpp;   // was ?
  uint8_t            Adash;
  uint8_t            C;

  if (state->player_in_bed)
  {
    state->vischars[0].mi.pos.y = 46;
    state->vischars[0].mi.pos.x = 46;
  }

  state->player_in_bed = 0;
  set_player_target_location(state, location_2A00);

  charstr = &state->character_structs[20];
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

  Adash = 5; // incremented by set_location_A373
  C = 0;
  set_location_A373(state, &Adash, C);

  /* Update all the bed objects to be empty. */
  // PROBLEM: this writes to a possibly bshared structure, ought to be moved into the state somehow
  bedpp = &beds[0];
  iters = beds_LENGTH;
  do
    **bedpp++ = interiorobject_EMPTY_BED;
  while (--iters);

  /* Update the player's bed object to be empty. */
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
  characterstruct_t *charstr; /* was HL */
  uint8_t            iters;   /* was B */
  uint8_t            Adash;
  uint8_t            C;

  if (state->player_in_breakfast)
  {
    state->vischars[0].mi.pos.y = 52;
    state->vischars[0].mi.pos.x = 62;
  }

  state->player_in_breakfast = 0;
  set_player_target_location(state, location_9003);

  charstr = &state->character_structs[20];
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

  Adash = 144;
  C     = 3; // B already 0
  set_location_A373(state, &Adash, C);

  /* Update all the benches to be empty. */
  // PROBLEM: again. writing to shared state.
  roomdef_23_breakfast[roomdef_23_BENCH_A] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_B] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_C] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_D] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_E] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_F] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_EMPTY_BENCH;

  if (state->room_index == 0 || state->room_index >= room_29_SECOND_TUNNEL_START)
    return;

  setup_room(state);
  plot_interior_tiles(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A33F: Set player target location.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] location Location.              (was BC)
 */
void set_player_target_location(tgestate_t *state, location_t location)
{
  vischar_t *vischar; /* was HL */

  if (state->morale_1)
    return;

  vischar = &state->vischars[0];

  vischar->character &= ~vischar_BYTE1_BIT6;
  vischar->target = (location >> 8) | ((location & 0xFF) << 8); // Big endian store.
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
  uint8_t Adash;
  uint8_t C;

  set_player_target_location(state, location_8502);
  Adash = 133;
  C     = 2;
  set_location_A373(state, &Adash, C);
}

/* ----------------------------------------------------------------------- */

/**
 * $A35F: Called by go_to_roll_call.
 *
 * \param[in]     state    Pointer to game state.
 * \param[in,out] pcounter Counter incremented.   (was A')
 */
void set_location_A35F(tgestate_t *state, uint8_t *pcounter, uint8_t C)
{
  uint8_t            counter; /* new var */
  const character_t *pchars;  /* was HL */
  uint8_t            iters;   /* was B */

  counter = *pcounter; // additional: keep a local copy of counter

  pchars = &tenlong[0];
  iters = 10;
  do
  {
    sub_A38C(state, *pchars, counter, C);
    counter++;
    pchars++;
  }
  while (--iters);

  *pcounter = counter;
}

/* ----------------------------------------------------------------------- */

/**
 * $A373: Called by the set_location routines.
 *
 * \param[in]     state    Pointer to game state.
 * \param[in,out] pcounter Counter incremented.   (was A')
 */
void set_location_A373(tgestate_t *state, uint8_t *pcounter, uint8_t C)
{
  uint8_t            counter; /* new var */
  const character_t *pchars;  /* was HL */
  uint8_t            iters;   /* was B */

  counter = *pcounter; // additional: keep a local copy of counter

  pchars = &tenlong[0];
  iters = 10;
  do
  {
    sub_A38C(state, *pchars, counter, C);
    if (iters == 6)
      counter++; // 6 is which character?
    pchars++;
  }
  while (--iters);

  *pcounter = counter;
}

/* ----------------------------------------------------------------------- */

/**
 * $A38C: sub_A38C.
 *
 * Finds a charstruct, or a vischar, and stores a location in its target.
 *
 * \param[in] state Pointer to game state.
 * \param[in] index Character index.       (was A)
 * \param[in] Adash Location low byte.
 * \param[in] C     Location high byte.
 */
void sub_A38C(tgestate_t *state, character_t index, uint8_t Adash, uint8_t C)
{
  characterstruct_t *charstr; /* was HL */
  vischar_t         *vischar; /* was HL */
  uint8_t            iters;   /* was B */

  charstr = get_character_struct(state, index);
  if ((charstr->character & characterstruct_BYTE0_BIT6) != 0)
  {
    index = charstr->character & characterstruct_BYTE0_MASK; // put in separate var
    iters = vischars_LENGTH - 1;
    vischar = &state->vischars[1]; /* iterate over non-player characters */
    do
    {
      if (index == charstr->character)
        goto store_to_vischar;
      vischar++;
    }
    while (--iters);

    goto exit;
  }

  // store_to_charstruct
  store_location(Adash, C, &charstr->target);

exit:
  return;

store_to_vischar:
  vischar->flags &= ~vischar_BYTE1_BIT6;
  store_location(Adash, C, &vischar->target);

  sub_A3BB(state, vischar); // 2nd arg a guess for now -- check // fallthrough
}

/**
 * $A3BB: sub_A3BB.
 *
 * Called by sub_A38C, set_player_target_location.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was HL) (e.g. $8003 in original)
 */
void sub_A3BB(tgestate_t *state, vischar_t *vischar)
{
  uint8_t    A;   /* was A */
  tinypos_t *pos; /* was DE */

  state->byte_A13E = 0;

  // sampled HL = $8003 $8043 $8023 $8063 $8083 $80A3

  A = sub_C651(state, &vischar->target);

  pos = &vischar->p04;
  pos->y = vischar->target & 0xFF;
  pos->x = vischar->target >> 8;

  if (A == 255)
    sub_CB23(state, vischar, &vischar->target);
  else if (A == 128)
    vischar->flags |= vischar_BYTE1_BIT6; // sample = HL = $8001
}

/* ----------------------------------------------------------------------- */

/**
 * $A3ED: Store banked A then C at HL.
 *
 * Used by sub_A38C only.
 *
 * \param[in]  Adash    Location low byte.
 * \param[in]  C        Location high byte.
 * \param[out] location Pointer to vischar->target, or characterstruct->target. (was HL)
 */
void store_location(uint8_t Adash, uint8_t C, location_t *location)
{
  *location = Adash | (C << 8);
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
  uint8_t character;

  character = vischar->character;
  if (character == 0)
    set_player_target_location(state, location_2C00);
  else
    sub_A404(state, charstr, character);
}

/**
 * $A404: Common end of above two routines.
 *
 * \param[in] state           Pointer to game state.
 * \param[in] charstr         Pointer to character struct. (was HL)
 * \param[in] character_index Character index.             (was A)
 */
void sub_A404(tgestate_t        *state,
              characterstruct_t *charstr,
              uint8_t            character_index)
{
  charstr->room = room_NONE;

  if (character_index > 19)
  {
    character_index -= 13;
  }
  else
  {
    uint8_t old_character_index;

    old_character_index = character_index;
    character_index = 13;
    if (old_character_index & (1 << 0))
    {
      charstr->room = room_1_HUT1RIGHT;
      character_index |= 0x80;
    }
  }

  charstr->character = character_index;
}

/* ----------------------------------------------------------------------- */

/**
 * $A420: Character sits.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] character Character index.       (was A)
 * \param[in] formerHL  (unknown)
 */
void character_sits(tgestate_t *state, character_t character, uint8_t *formerHL)
{
  uint8_t  index; /* was ? */
  uint8_t *bench; /* was HL */
  room_t   room;  /* was C */

  index = character - 18;
  /* First three characters. */
  bench = &roomdef_25_breakfast[roomdef_25_BENCH_D];
  if (index >= 3) // character_21
  {
    /* Second three characters. */
    bench = &roomdef_23_breakfast[roomdef_23_BENCH_A];
    index -= 3;
  }

  /* Poke object. */
  bench += index * 3;
  *bench = interiorobject_PRISONER_SAT_DOWN_MID_TABLE;

  room = room_25_BREAKFAST;
  if (character >= character_21)
    room = room_23_BREAKFAST;

  character_sit_sleep_common(state, room, formerHL);
}

/**
 * $A444: Character sleeps.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] character Character index.       (was A)
 * \param[in] formerHL  (unknown)
 */
void character_sleeps(tgestate_t  *state,
                      character_t  character,
                      uint8_t     *formerHL)
{
  room_t room; /* was C */

  /* Poke object. */
  *beds[character - 7] = interiorobject_OCCUPIED_BED;

  if (character < character_10)
    room = room_3_HUT2RIGHT;
  else
    room = room_5_HUT3RIGHT;

  character_sit_sleep_common(state, room, formerHL);
}

/**
 * $A462: Common end of character sits/sleeps.
 *
 * \param[in] state Pointer to game state.
 * \param[in] room  Character index.            (was C)
 * \param[in] HL    Likely a target location_t. (was HL)
 */
void character_sit_sleep_common(tgestate_t *state, room_t room, uint8_t *HL)
{
  // FIXME
  // This is baffling. It's receiving either a character struct OR a pointer to a vischar target.
  // The vischar case is weird certainly.

  // sampled HL = 0x76BB
  // 0x76BB -> character_structs[24].room

  // sampled HL = 0x8022
  // 0x8022 -> vischar[1].target (low byte)

  // EX DE,HL
  /* The character is made to disappear. */
  *HL = room_NONE;  // $8022, $76B8, $76BF, $76A3
  // EX AF,AF'

  if (state->room_index != room)
  {
    /* Sit/sleep in a room presently not visible. */

    HL -= 4; // sampled HL = $761D,76a6,76bb,76c2,769f .. would mean this is characterstruct->pos.vo = 0xFF;
    *HL = 255;
  }
  else
  {
    /* Room is visible - force a refresh. */

    // sampled HL = 76a6,769f,76b4,76bb,76c2,76ad (sleeping)
    // sampled HL = 8022,76b4,76bb (eating)

    // 0x8022 + 26 = 0x803c (*next* character's room index)
    // 0x76bb + 26 = 0x76d5 (item_struct[1] (shovel) target high byte)

    HL += 26;
    *HL = 255;

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
  setup_room(state);
  plot_interior_tiles(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A47F: Player sits.
 *
 * \param[in] state Pointer to game state.
 */
void player_sits(tgestate_t *state)
{
  roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_PRISONER_SAT_DOWN_END_TABLE;
  player_sit_sleep_common(state, &state->player_in_breakfast);
}

/**
 * $A489: Player sleeps.
 *
 * \param[in] state Pointer to game state.
 */
void player_sleeps(tgestate_t *state)
{
  roomdef_2_hut2_left[roomdef_2_BED] = interiorobject_OCCUPIED_BED;
  player_sit_sleep_common(state, &state->player_in_bed);
}

/**
 * $A498: Common end of player sits/sleeps.
 *
 * \param[in] state Pointer to game state.
 * \param[in] pflag Pointer to player_in_breakfast or player_in_bed. (was HL)
 */
void player_sit_sleep_common(tgestate_t *state, uint8_t *pflag)
{
  /* Set player_in_breakfast or player_in_bed flag. */
  *pflag = 0xFF;

  /* Reset only the bottom byte of target location. */
  state->vischars[0].target &= 0xFF00;

  /* Set player position (y,x) to zero. */
  state->vischars[0].mi.pos.y = 0;
  state->vischars[0].mi.pos.x = 0;

  reset_position(state, &state->vischars[0]);

  select_room_and_plot(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A4A9: Set location 0x0E00.
 *
 * \param[in] state Pointer to game state.
 */
void set_location_0x0E00(tgestate_t *state)
{
  uint8_t Adash;
  uint8_t C;

  set_player_target_location(state, location_0E00);
  Adash = 0x0E;
  C = 0;
  set_location_A373(state, &Adash, C);
}

/**
 * $A4B7: Set location 0x8E04.
 *
 * \param[in] state Pointer to game state.
 */
void set_location_0x8E04(tgestate_t *state)
{
  uint8_t Adash;
  uint8_t C;

  set_player_target_location(state, location_8E04);
  Adash = 0x10;
  C = 0;
  set_location_A373(state, &Adash, C);
}

/**
 * $A4C5: Set location 0x1000.
 *
 * \param[in] state Pointer to game state.
 */
void set_location_0x1000(tgestate_t *state)
{
  uint8_t Adash;
  uint8_t C;

  set_player_target_location(state, location_1000);
  Adash = 0x10;
  C = 0;
  set_location_A373(state, &Adash, C);
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
  uint8_t character;

  character = vischar->character;
  if (character == 0)
    set_player_target_location(state, location_2B00);
  else
    byte_A13E_anotherone_common(state, charstr, character);
}

/**
 * $A4E4: Common end of above two routines.
 *
 * \param[in] state           Pointer to game state.
 * \param[in] charstr         Pointer to character struct. (was HL)
 * \param[in] character_index Character index.             (was A)
 */
void byte_A13E_anotherone_common(tgestate_t        *state,
                                 characterstruct_t *charstr,
                                 uint8_t            character_index)
{
  charstr->room = room_NONE;

  if (character_index > 19)
  {
    character_index -= 2;
  }
  else
  {
    uint8_t old_character_index;

    old_character_index = character_index;
    character_index = 24;
    if (old_character_index & (1 << 0))
      character_index++;
  }

  charstr->character = character_index;
}

/* ----------------------------------------------------------------------- */

/**
 * $A4FD: Go to roll call.
 *
 * \param[in] state Pointer to game state.
 */
void go_to_roll_call(tgestate_t *state)
{
  uint8_t Adash;
  uint8_t C;

  Adash = 26;
  C = 0; // This must be getting set for some reason.
  set_location_A35F(state, &Adash, C);
  set_player_target_location(state, location_2D00);
}

/* ----------------------------------------------------------------------- */

/**
 * $A50B: Screen reset.
 *
 * \param[in] state Pointer to game state.
 */
void screen_reset(tgestate_t *state)
{
  wipe_visible_tiles(state);
  plot_interior_tiles(state);
  zoombox(state);
  plot_game_window(state);
  set_game_window_attributes(state, attribute_WHITE_OVER_BLACK);
}

/* ----------------------------------------------------------------------- */

/**
 * $A51C: Player has escaped.
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
    { 0x006E,  9, "WELL DONE" },
    { 0x00AA, 16, "YOU HAVE ESCAPED" },
    { 0x00CC, 13, "FROM THE CAMP" },
    { 0x0809, 18, "AND WILL CROSS THE" },
    { 0x0829, 19, "BORDER SUCCESSFULLY" },
    { 0x0809, 19, "BUT WERE RECAPTURED" },
    { 0x082A, 17, "AND SHOT AS A SPY" },
    { 0x0829, 18, "TOTALLY UNPREPARED" },
    { 0x082C, 12, "TOTALLY LOST" },
    { 0x0828, 21, "DUE TO LACK OF PAPERS" },
    { 0x100D, 13, "PRESS ANY KEY" },
  };

  const screenlocstring_t *slstring;         /* was HL */
  escapeitem_t             escapeitem_flags; /* was C */
  const item_t            *pitem;            /* was HL */
  uint8_t                  keys;             /* was A */

  screen_reset(state);

  /* Print standard prefix messages. */
  slstring = &messages[0];
  slstring = screenlocstring_plot(state, slstring); /* WELL DONE */
  slstring = screenlocstring_plot(state, slstring); /* YOU HAVE ESCAPED */
  (void) screenlocstring_plot(state, slstring);     /* FROM THE CAMP */

  /* Print item-tailored messages. */
  escapeitem_flags = 0;
  pitem = &state->items_held[0];
  escapeitem_flags = have_required_items(pitem++, escapeitem_flags);
  escapeitem_flags = have_required_items(pitem,   escapeitem_flags);
  if (escapeitem_flags == (escapeitem_COMPASS | escapeitem_PURSE))
  {
    slstring = &messages[3];
    slstring = screenlocstring_plot(state, slstring); /* AND WILL CROSS THE */
    (void) screenlocstring_plot(state, slstring);     /* BORDER SUCCESSFULLY */
    escapeitem_flags = 0xFF; /* success - reset game */
  }
  else if (escapeitem_flags != (escapeitem_COMPASS | escapeitem_PAPERS))
  {
    slstring = &messages[5]; /* BUT WERE RECAPTURED */
    slstring = screenlocstring_plot(state, slstring);
    /* no uniform => AND SHOT AS A SPY */
    if (escapeitem_flags < escapeitem_UNIFORM)
    {
      /* no objects => TOTALLY UNPREPARED */
      slstring = &messages[7];
      if (escapeitem_flags)
      {
        /* no compass => TOTALLY LOST */
        slstring = &messages[8];
        if (escapeitem_flags & escapeitem_COMPASS)
        {
          /* no papers => DUE TO LACK OF PAPERS */
          slstring = &messages[9];
        }
      }
    }
    (void) screenlocstring_plot(state, slstring);
  }

  slstring = &messages[10]; /* PRESS ANY KEY */
  (void) screenlocstring_plot(state, slstring);

  /* Wait for a keypress. */
  do
    keys = keyscan_all(state);
  while (keys != 0);   /* down press */
  do
    keys = keyscan_all(state);
  while (keys == 0);   /* up press */

  /* Reset the game, or send the player to solitary. */
  if (escapeitem_flags == 0xFF || escapeitem_flags >= escapeitem_UNIFORM)
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

  /* Scan all keyboard ports (0xFEFE .. 0x7FFE). */
  port = port_KEYBOARD_SHIFTZXCV;
  do
  {
    keys = state->speccy->in(state->speccy, port);
    keys = ~keys & 0x1F; /* Invert bits and mask off key bits. */
    if (keys)
      return keys; /* Key(s) pressed. */

    /* Rotate the top byte of the port number to get the next port. */
    // RLC B
    carry = (port >> 15) != 0;
    port = ((port << 1) & 0xFF00) | (carry << 8) | (port & 0x00FF);
  }
  while (carry);

  return 0; /* Nothing pressed. */
}

/* ----------------------------------------------------------------------- */

/**
 * $A59C: Return bitmask indicating the presence of required items.
 *
 * \param[in] pitem    Pointer to item.   (was HL)
 * \param[in] previous Previous bitfield. (was C)
 *
 * \return Sum of bitmasks. (was C)
 */
escapeitem_t have_required_items(const item_t *pitem, escapeitem_t previous)
{
  return item_to_bitmask(*pitem) | previous;
}

/**
 * $A5A3: Return a bitmask indicating the presence of required items.
 *
 * \param[in] item Item. (was A)
 *
 * \return COMPASS, PAPERS, PURSE, UNIFORM => 1, 2, 4, 8. (was A)
 */
escapeitem_t item_to_bitmask(item_t item)
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

  /* Get vertical offset. */
  v = state->map_position[1] & 0xFC; /* = 0, 4, 8, 12, ... */

  /* Multiply A by 13.5. (v is a multiple of 4, so this goes 0, 54, 108, 162, ...) */
  tiles = &map[0] - MAPX + (v + (v >> 1)) * 9; // Subtract MAPX so it skips the first row.

  /* Add horizontal offset. */
  tiles += state->map_position[0] >> 2;

  /* Populate map_buf with 7x5 array of supertile refs. */
  iters = MAPBUFY;
  buf = &state->map_buf[0];
  do
  {
    memcpy(buf, tiles, MAPBUFX);
    buf   += MAPBUFX;
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

  offset = (pos & 3) * 4;
  pos_1 = (state->map_position[0] & 3) + offset;

  /* Initial edge. */

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

  iters2 = 5; /* 5 iterations */
  do
  {
    tiles = &supertiles[*maptiles].tiles[0] + offset; // self modified by $A82A

    iters = 4; /* 4 iterations */
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
 * Called by pick_up_item and reset_B2FC.
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

  offset = pos & 3; // self modify (local)
  pos_1 = (state->map_position[1] & 3) * 4 + offset;

  /* Initial edge. */

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

  iters2 = 3; // 3 iterations
  do
  {
    tiles = &supertiles[*maptiles].tiles[0] + offset; // self modified by $A8F6

    iters = 4; // 4 iterations
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
  return plot_tile(state, tile_index, psupertileindex, scr) + (state->columns * 8 - 1); // -1 compensates the +1 in plot_tile
}

/* ----------------------------------------------------------------------- */

/**
 * $A9AD: Plot a tile then increment 'scr' by 1.
 *
 * (Leaf)
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
  // Original code has a copy of map_position in HL on entry. In this version
  // we read it from source.
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

  uint16_t  DE;
  uint8_t   C;
  uint8_t   A;
  movefn_t *HL;

  static movefn_t *const map_move[] =
  {
    &map_move_up_left,
    &map_move_up_right,
    &map_move_down_right,
    &map_move_down_left,
  };

  if (state->room_index)
    return; /* outdoors only */

  if (state->vischars[0].b07 & vischar_BYTE7_BIT6)
    return; // unknown

  DE = state->vischars[0].w0A;
  C  = state->vischars[0].b0C;
  DE += 3;
  //A = *DE; // So DE (and w0A) must be a pointer.
  if (A == 255)
    return;
  if (C & (1 << 7))
    A ^= 2;

  HL = map_move[A];
  // PUSH HL // pushes map_move routine to stack
  // PUSH AF

#if 0
  BC = 0x7C00;
  if (A >= 2)
    B = 0x00;
  else if (A != 1 && A != 2)
    C = 0xC0;
  HL = map_position;
  if (L == C || H == B)
  {
    // POP AF
    // POP HL
    return;
  }
  // POP AF

  HL = &state->used_by_move_map; // screen offset of some sort?
  if (A >= 2)
    A = *HL + 1;
  else
    A = *HL - 1;
  A &= 3;
  *HL = A;
  // EX DE,HL
  HL = 0x0000;
//  ; I /think/ this is equivalent:
//  ; HL = 0x0000;
//  ; if (A != 0) {
//    ;     HL = 0x0060;
//    ;     if (A != 2) {
//      ;         HL = 0xFF30;
//      ;         if (A != 1) {
//        ;             HL = 0xFF90;
//        ;         }
//      ;     }
//    ; }
  if (A == 0) goto ab2a;
  L = 0x60;
  if (A == 2) goto ab2a;
  HL = 0xFF30;
  if (A == 1) goto ab2a;
  L = 0x90;

ab2a:
  state->plot_game_window_x = HL;
  HL = map_position;
#endif

  // pops and calls map_move_* routine pushed at $AAE0
  HL(state, &DE);
}

// called when player moves down-right (map is shifted up-left)
void map_move_up_left(tgestate_t *state, uint8_t *DE)
{
  uint8_t A;

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
 * \return Chosen attributes.
 */
attribute_t choose_game_window_attributes(tgestate_t *state)
{
  attribute_t attr;

  if (state->room_index < room_29_SECOND_TUNNEL_START)
  {
    /* Player is outside or in a room. */

    if (state->day_or_night == 0) /* day */
      attr = attribute_WHITE_OVER_BLACK;
    else if (state->room_index == 0)
      attr = attribute_BRIGHT_BLUE_OVER_BLACK;
    else
      attr = attribute_CYAN_OVER_BLACK;
  }
  else
  {
    /* Player is in a tunnel. */

    if (state->items_held[0] == item_TORCH ||
        state->items_held[1] == item_TORCH)
    {
      attr = attribute_RED_OVER_BLACK;
    }
    else
    {
      /* No torch - draw nothing. */

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
  attribute_t  attrs;
  uint8_t     *HL;
  uint8_t      A;

  state->zoombox_x = 12;
  state->zoombox_y = 8;

  attrs = choose_game_window_attributes(state);

  state->speccy->attributes[ 9 * state->width + 18] = attrs;
  state->speccy->attributes[ 9 * state->width + 19] = attrs;
  state->speccy->attributes[10 * state->width + 18] = attrs;
  state->speccy->attributes[10 * state->width + 19] = attrs;

  state->zoombox_horizontal_count = 0;
  state->zoombox_vertical_count   = 0;

  do
  {
    HL = &state->zoombox_x;
    A = *HL;
    if (A != 1)
    {
      (*HL)--;
      A--;
      HL[1]++;
    }

    HL++; // &state->zoombox_horizontal_count;
    A += *HL;
    if (A < 22)
      (*HL)++;

    HL++; // &state->zoombox_y;
    A = *HL;
    if (A != 1)
    {
      (*HL)--;
      A--;
      HL[1]++;
    }

    HL++; // &state->zoombox_vertical_count;
    A += *HL;
    if (A < 15)
      (*HL)++;

    zoombox_1(state);
    zoombox_draw(state);
  }
  while (state->zoombox_vertical_count + state->zoombox_horizontal_count < 35);
}

void zoombox_1(tgestate_t *state)
{
  uint8_t B_Outer;

#if 0
  // is this doing: DE = y / 2, HL = y / 4 ?
  // scaling to 3/4?

  H = state->zoombox_y;
  A = 0;
  // SRL H  // carry = H & 1; H >>= 1;
  // RRA    // carry_out = A & 1; A = (A >> 1) | (carry << 7); carry = carry_out;
  E = A;
  D = H;
  // SRL H  // as above
  // RRA
  L = A;
  HL += DE + zoombox_x;
  HL += screen_buffer_start_address + 1;

  DE = state->game_window_start_offsets[zoombox_y * 8]; // ie. * 16
  DE += zoombox_x;

  A = state->zoombox_horizontal_count;
  self_AC55 = A; // self modify
  self_AC4D = -A + 24; // self modify

  B_Outer = state->zoombox_vertical_count; /* iterations */
  do
  {
    // PUSH DE
    A = 8; // 8 iterations
    do
    {
      BC = state->zoombox_horizontal_count;
      memcpy(DE, HL, BC); DE += BC; HL += BC; // LDIR
      HL += self_AC4D; // self modified
      E -= self_AC55; // self modified
      D++;
    }
    while (--A);
    // POP DE

    BC = 32;
    if (E >= 224)
      B = 7;
    DE += BC;
  }
  while (--B_Outer);
#endif
}

void zoombox_draw(tgestate_t *state)
{
  uint16_t HL;

  HL = state->game_window_start_offsets[(state->zoombox_y - 1) * 8]; // ie. * 16

#if 0
  /* Top left */
  HL += state->zoombox_x - 1;
  zoombox_draw_tile(state, zoombox_tile_TL, HL);
  HL++;

  /* Horizontal */
  B = state->zoombox_horizontal_count; /* iterations */
  do
  {
    zoombox_draw_tile(state, zoombox_tile_HZ, HL);
    HL++;
  }
  while (--B);

  /* Top right */
  zoombox_draw_tile(state, zoombox_tile_TR, HL);
  DE = 32;
  if (L >= 224)
    D = 0x07;
  HL += DE;

  /* Vertical */
  B = state->zoombox_vertical_count; /* iterations */
  do
  {
    zoombox_draw_tile(state, zoombox_tile_VT, HL);
    DE = state->width;
    if (L >= 224)
      D = 0x07;
    HL += DE;
  }
  while (--B);

  /* Bottom right */
  zoombox_draw_tile(state, zoombox_tile_BR, HL);
  HL--;

  /* Horizontal */
  B = state->zoombox_horizontal_count; /* iterations */
  do
  {
    zoombox_draw_tile(state, zoombox_tile_HZ, HL);
    HL--;
  }
  while (--B);

  /* Bottom left */
  zoombox_draw_tile(state, zoombox_tile_BL, HL);
  DE = 0xFFE0;
  if (L < 32)
    DE = 0xF8E0;
  HL += DE;

  /* Vertical */
  B = state->zoombox_vertical_count; /* iterations */
  do
  {
    zoombox_draw_tile(state, zoombox_tile_VT, HL);
    DE = 0xFFE0;
    if (L < 32)
      DE = 0xF8E0;
    HL += DE;
  }
  while (--B);
#endif
}

/**
 * $ACFC: zoombox_draw_tile.
 *
 * \param[in] state Pointer to game state.
 * \param[in] index Tile index into zoombox_tiles (was A).
 * \param[in] addr  Screen address (was HL).
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

  uint8_t         *DE;
  const tilerow_t *row;
  uint8_t          B;
  ptrdiff_t        off;
  attribute_t     *attrp;

  DE = addr; // was EX DE,HL
  row = &zoombox_tiles[index].row[0];
  B = 8; // 8 iterations
  do
  {
    *DE = *row++;
    DE += 256;
  }
  while (--B);

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
  uint8_t D;
  uint8_t E;

  E = *HL++;
  D = *HL++;
#if 0
  (*HL)--;
  if (Z)
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
    HL++;
    C = *HL++;
    B = *HL;
    HL -= 2;
    BC += A * 2;
    A = *BC;
    if (A == 0xFF)
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
    if (*HL & (1 << 7)) A ^= 2;
    if (A < 2) D -= 2;
    D++;
    if (A != 0 && A != 3)
      E += 2;
    else
      E -= 2;
    HL -= 3;
    *HL-- = D;
    *HL = E;
  }
#endif
}

/**
 * $ADBD: Turns white screen elements light blue and tracks the player with a
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

  const uint8_t *HL;
  uint8_t        D;
  uint8_t        E;
  uint8_t        H;
  uint8_t        L;
  uint8_t        B;

  if (state->searchlight_state == searchlight_STATE_OFF)
    goto not_tracking;

  if (state->room_index)
  {
    /* Player is indoors. */

    /* If the player goes indoors the searchlight loses track. */
    state->searchlight_state = searchlight_STATE_OFF;
    return;
  }

  /* Player is outdoors. */
  if (state->searchlight_state == searchlight_STATE_1F)
  {
    E = state->map_position[0] + 4;
    D = state->map_position[1];
    L = state->searchlight_coords[0]; // fused load split
    H = state->searchlight_coords[1];

    /* If the highlight doesn't need to move, quit. */
    if (L == E && H == D)
      return;

    /* Move searchlight left/right to focus on player. */
    if (L < E)
      L++;
    else
      L--;

    /* Move searchlight up/down to focus on player. */
    if (H != D)
    {
      if (H < D)
        H++;
      else
        H--;
    }

    state->searchlight_coords[0] = L; // fused store split
    state->searchlight_coords[1] = H;
  }

#if 0
  DE = state->map_position;
  HL = $AE77; // &searchlight_coords + 1 byte; // compensating for HL--; jumped into
  B = 1; // 1 iteration
  // PUSH BC
  // PUSH HL
  goto ae3f;
#endif

not_tracking:

  HL = &spotlight_movement_data_maybe[0];
  B = 3; // 3 iterations
  do
  {
#if 0
    // PUSH BC
    // PUSH HL
    searchlight_AD59(state, HL);
    // POP HL
    // PUSH HL
    searchlight_caught(state, HL);
    // POP HL
    // PUSH HL
    DE = state->map_position;
    if (E + 23 < *HL || *HL + 16 < E)
      goto next;
    HL++;
    if (D + 16 < *HL || *HL + 16 < D)
      goto next;

ae3f:
    A = 0;

    HL--;
    B = 0x00;
    if (*HL - E < 0)
    {
      B = 0xFF;
      A = ~A;
    }

    C = Adash;
    Adash = *++HL;
    H = 0x00;
    Adash -= D;
    if (Adash < 0)
      H = 0xFF;
    L = Adash;
    // HL must be row, BC must be column
    HL = HL * state->width + BC + &state->speccy->attributes[0x46]; // address of top-left game window attribute
    // EX DE,HL

    state->searchlight_related = A;
    searchlight_plot(state);

next:
    // POP HL
    // POP BC
#endif
    HL += 7;
  }
  while (--B);
}

/**
 * $AE78: Suspect this is when the player is caught in the spotlight.
 *
 * \param[in] state Pointer to game state.
 * \param[in] HL    Pointer to spotlight_movement_data_maybe.
 */
void searchlight_caught(tgestate_t *state, uint8_t *HL)
{
  uint8_t D, E;

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

  if (state->searchlight_state == searchlight_STATE_1F)
    return;

  state->searchlight_state = searchlight_STATE_1F;

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

  const uint8_t *DEdash;
  uint8_t        Cdash;
  uint8_t        A;
  attribute_t   *HL;

  DEdash = &searchlight_shape[0];
  Cdash = 16; /* iterations */
  do
  {
    A = state->searchlight_related;
    HL = &state->speccy->attributes[0x0240]; // HL = 0x5A40; // screen attribute address (column 0 + bottom of game screen)
#if 0
    if (A != 0 && (E & 31) >= 22)
      L = 32;
    // SBC HL,DE
    // RET C  // if (HL < DE) return; // what about carry?
    // PUSH DE
    HL = 0x5840; // screen attribute address (column 0 + top of game screen)
    if (A != 0 && (E & 31) >= 7)
      L = 32;
    // SBC HL,DE
    // JR C,aef0  // if (HL < DE) goto aef0;

    DEdash += 2;

    goto nextrow;

aef0:
    // EX DE,HL

    Bdash = 2; /* iterations */
    do
    {
      A = *DEdash

      DE = 0x071E;
      C = A;
      B = 8; /* iterations */
      do
      {
        A = state->searchlight_related;
        if (A != 0) ...
          A = L; // interleaved
        ... goto af0c;
        if ((A & 31) >= 22)
          goto af1b;
        goto af18;

af0c:
        if ((A & 31) < E)
          goto af18;

        do
          DEdash++;
        while (--Bdash);

        goto nextrow;

af18:
        if (A < D)
        {
af1b:
          // was RL C
          C <<= 1;
        }
        else
        {
          // was RL C
          carry = (C & (1 << 7)) != 0;
          C <<= 1;
          if (carry)
            *HL = attribute_YELLOW_OVER_BLACK;
          else
            *HL = attribute_BRIGHT_BLUE_OVER_BLACK;
        }
        HL++;
      }
      while (--B);

      DEdash++;
    }
    while (--Bdash);

nextrow:
    // POP HL
    HL += state->width;
    // EX DE,HL
#endif
  }
  while (--Cdash);
}

/* ----------------------------------------------------------------------- */

/**
 * $AF8F: (unknown)
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 */
void sub_AF8F(tgestate_t *state, vischar_t *vischar)
{
  uint8_t    stashed_A;
  vischar_t *HL;
  uint8_t    A; // might need to be an arg

  // EX AF, AF'
  //stashed_A = A; // actually Adash...
  vischar->b07 |= vischar_BYTE7_BIT6 | vischar_BYTE7_BIT7;  // wild guess: clamp character in position?
  HL = vischar;

  if (vischar == &state->vischars[0] && state->automatic_player_counter > 0)
    door_handling(state, vischar);

  if (vischar > &state->vischars[0] || ((state->vischars[0].flags & (vischar_BYTE1_PICKING_LOCK | vischar_BYTE1_CUTTING_WIRE)) != vischar_BYTE1_CUTTING_WIRE))
  {
    bounds_check(state, vischar);
    return;
  }

  /* Cutting wire only from here onwards? */
  A = vischar->character;
  if (A < character_26)
  {
    sub_AFDF(state, vischar);
    //if (!Z)
    //  return;
  }
  //; else object only from here on ?
  vischar->b07 &= ~vischar_BYTE7_BIT6;
  vischar->mi.pos = state->saved_pos;
  vischar->mi.b17 = stashed_A; // possibly a flip left/right flag
  A = 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $AFDF: (unknown)
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void sub_AFDF(tgestate_t *state, vischar_t *vischar)
{
  vischar_t *HL;
  uint8_t    B;

  HL = &state->vischars[0];
  // was HL = $8001; // &vischar[0].byte1;
  B = 8; // 8 iterations
  do
  {
    if (HL->flags & vischar_BYTE1_BIT7)
      goto next; // $8001, $8021, ...

#if 0
    // PUSH BC
    // PUSH HL

    HL += 0x0E; // $800F etc. - y axis position

    // $AFEF --------

    C = *HL++;
    B = *HL;
    // EX DE, HL
    HL = state->saved_pos.y;
    BC += 4;
    if (HL != BC)
    {
      if (HL > BC)
        goto pop_next;
      BC -= 8; // ie -4 over original
      HL = saved_Y;
      if (HL < BC)
        goto pop_next;
    }
    // EX DE, HL
    HL++;

    // $B016 --------

    C = *HL++;
    B = *HL;
    // EX DE, HL
    HL = state->saved_pos.x;
    BC += 4;
    if (HL != BC)
    {
      if (HL > BC)
        goto pop_next;
      BC -= 8;
      HL = saved_X;
      if (HL < BC)
        goto pop_next;
    }
    // EX DE, HL
    HL++;

    // $B03D --------

    C = *HL;
    A = state->saved_pos.vo - C;
    if (A < 0)
      A = -A;
    if (A >= 24)
      goto pop_next;

    // --------

    A = vischar->flags & 0x0F; // sampled IY=$8020, $8040, $8060, $8000 // but this is *not* vischar_BYTE1_MASK, which is 0x1F
    if (A == 1)
    {
      // POP HL
      // PUSH HL // sampled HL=$8021, $8041, $8061, $80A1
      HL--;
      if ((HL & 0xFF) == 0)   // ie. $8000
      {
        if (bribed_character == vischar->character)
        {
          use_bribe(state, vischar);
        }
        else
        {
          // POP HL
          // POP BC
          HL = vischar + 1;
          solitary(state);
          return; // exit via
        }
      }
    }
    // POP HL
    HL--;
    A = *HL; // sampled HL = $80C0, $8040, $8000, $8020, $8060 // vischar_BYTE0
    if (A >= 26)
    {
      // PUSH HL

      HL += 17;

      B = 7;
      C = 35;
      tmpA = A;
      A = vischar->b0E; // interleaved
      if (tmpA == 28)
      {
        L -= 2;
        C = 54;
        A ^= 1;
      }
      if (A == 0)
      {
        A = *HL;
        if (A != C)
        {
          if (A >= C)
            (*HL) -= 2;  // decrement by -2 then execute +1 anyway to avoid branch
          (*HL)++;
        }
      }
      else if (A == 1)
      {
        A = C + B;
        if (A != *HL)
          (*HL)++;
      }
      else if (A == 2)
      {
        A = C - B;
        *HL = A;
      }
      else
      {
        A = C - B;
        if (A != *HL)
          (*HL)--;
      }
      // POP HL
      // POP BC
    }
    HL += 13;
    A = *HL & vischar_BYTE13_MASK; // sampled HL = $806D, $804D, $802D, $808D, $800D
    if (A)
    {
      HL++;
      A = *HL ^ 2;
      if (A != vischar->b0E)
      {
        vischar->b0D = vischar_BYTE13_BIT7;

b0d0:
        vischar->b07 = (vischar->b07 & vischar_BYTE7_MASK_HI) | 5; // preserve flags and set 5? // sampled IY = $8000, $80E0
        if (!Z)
          return; /* odd */
      }
    }

    BC = vischar->b0E; // sampled IY = $8000, $8040, $80E0
    vischar->b0D = four_bytes_B0F8[BC];
    if ((C & 1) == 0)
      vischar->b07 &= ~vischar_BYTE7_BIT5;
    else
      vischar->b07 |= vischar_BYTE7_BIT5;
    goto b0d0;

    //B $B0F8, 4 four_bytes_B0F8
    //D $B0F8( < - sub_AFDF)

pop_next:
    // POP HL
    // POP BC
#endif

next:
    HL++;
  }
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $B107: Use a bribe.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void use_bribe(tgestate_t *state, vischar_t *vischar)
{
  item_t    *DE;
  uint8_t    B;
  vischar_t *vc;

  increase_morale_by_10_score_by_50(state);

  vischar->flags = 0;

  sub_CB23(state, vischar, &vischar->target);

  DE = &state->items_held[0];
  if (*DE != item_BRIBE && *++DE != item_BRIBE)
    return; /* have no bribes */

  /* We have a bribe. */
  *DE = item_NONE;

  state->item_structs[item_BRIBE].room = (room_t) itemstruct_ROOM_MASK;

  draw_all_items(state);

  B = vischars_LENGTH - 1;
  vc = &state->vischars[1]; /* iterate over non-player characters */
  do
  {
    if (vc->character < character_20) // a character index. but why 20?
      vc->flags = vischar_BYTE1_BIT2; // character has taken bribe?
    vc++;
  }
  while (--B);

  queue_message_for_display(state, message_HE_TAKES_THE_BRIBE);
  queue_message_for_display(state, message_AND_ACTS_AS_DECOY);
}

/* ----------------------------------------------------------------------- */

/**
 * $B14C: Check the character is inside of bounds.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 */
int bounds_check(tgestate_t *state, vischar_t *vischar)
{
  uint8_t       B;
  const wall_t *wall;

  if (state->room_index)
    return interior_bounds_check(state, vischar);

  B = NELEMS(walls); // count of walls + fences
  wall = &walls[0];
  do
  {
    uint16_t min0, max0, min1, max1, min2, max2;

    min0 = wall->a * 8;
    max0 = wall->b * 8;
    min1 = wall->c * 8;
    max1 = wall->d * 8;
    min2 = wall->e * 8;
    max2 = wall->f * 8;

    if (state->saved_pos.y  >= min0 + 2 &&
        state->saved_pos.y  <  max0 + 4 &&
        state->saved_pos.x  >= min1     &&
        state->saved_pos.x  <  max1 + 4 &&
        state->saved_pos.vo >= min2     &&
        state->saved_pos.vo <  max2 + 2)
    {
      vischar->b07 ^= vischar_BYTE7_BIT5;
      return 1; // NZ
    }

    wall++;
  }
  while (--B);

  return 0; // Z
}

/* ----------------------------------------------------------------------- */

/**
* $B1C7: BC becomes A * 8.
*
* (Leaf).
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
  int      mask;
  int      cur;
  uint8_t *door;
  int      iters;

  mask  = 0xFF & ~door_FLAG_LOCKED;
  cur   = state->current_door & mask;
  door  = &state->gates_and_doors[0];
  iters = NELEMS(state->gates_and_doors);
  do
  {
    if ((*door & mask) == cur)
    {
      if ((*door & door_FLAG_LOCKED) == 0)
        return 0; // open

      queue_message_for_display(state, message_THE_DOOR_IS_LOCKED);
      return 1; // locked
    }
    door++;
  }
  while (--iters);

  return 0; // open
}

/* ----------------------------------------------------------------------- */

/**
 * $B1F5: Door handling.
 *
 * \param[in] state Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 *
 * \return Nothing. // suspect A is returned
 */
void door_handling(tgestate_t *state, vischar_t *vischar)
{
  room_t           A;
  const doorpos_t *HL;

  if (state->room_index)
  {
    door_handling_interior(state, vischar);
    return;
  }

  HL = &door_positions[0];
#if 0
  E = vischar->b0E;
  if (E >= 2)
    HL = &door_position[1];

  B = 16; /* iterations */
  do
  {
    if ((*HL & 3) == E)
    {
      // PUSH BC
      // PUSH HL
      // PUSH DE
      door_in_range(state, HL);
      // POP DE
      // POP HL
      // POP BC
      if (!C)
        goto found;
    }
    HL++;
  }
  while (--B);
  A &= B; // set Z (B is zero)
  return;

found:
  state->current_door = 16 - B;
  // EXX
  if (is_door_open(state))
    return; // door was locked - return nonzero?
  // EXX
  vischar->room = (*HL >> 2) & 0x3F; // sampled HL = $792E (in door_positions[])
  if ((*HL & 3) >= 2)
  {
    HL += 5;
    transition(state, vischar, HL); // seems to goto reset then jump back to main (icky)
  }
  else
  {
    HL -= 3;
    transition(state, vischar, HL);
  }
  // likely NEVER_RETURNS
#endif
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
 * \return 1 if in range, 0 if not.
 */
int door_in_range(tgestate_t *state, const doorpos_t *doorpos)
{
  uint16_t y; /* was BC */
  uint16_t x; /* was BC */

  y = multiply_by_4(doorpos->pos.y);
  if (state->saved_pos.y < y - 3 || state->saved_pos.y >= y + 3)
    return 0;

  x = multiply_by_4(doorpos->pos.x);
  if (state->saved_pos.x < x - 3 || state->saved_pos.x >= x + 3)
    return 0;

  return 1;
}

/* ----------------------------------------------------------------------- */

/**
 * $B295: BC becomes A * 4.
 *
 * (Leaf).
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
 * (Leaf).
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
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

  const bounds_t *roombounds;  /* was BC */
  uint16_t       *saved_coord; /* was HL */
  uint8_t        *objbounds;   /* was HL */
  uint8_t         nbounds;     /* was B */

  roombounds = &roomdef_bounds[state->roomdef_bounds_index];
  saved_coord = &state->saved_pos.y;
  if (roombounds->a < *saved_coord || roombounds->b + 4 >= *saved_coord)
    goto stop;

  saved_coord++; /* &state->saved_pos.x; */
  if (roombounds->c - 4 < *saved_coord || roombounds->d >= *saved_coord)
    goto stop;

  objbounds = &state->roomdef_object_bounds[0];
  for (nbounds = *objbounds++; nbounds != 0; nbounds--)
  {
    uint8_t  *innerHL;
    uint16_t *DE;
    uint8_t   innerB;

    innerHL = objbounds;
    DE      = &state->saved_pos.y;
    innerB  = 2; /* 2 iterations */
    do
    {
      uint8_t A;

      A = *DE;
      if (A < innerHL[0] || A >= innerHL[1])
        goto next;

      DE++; /* &state->saved_pos.x; */
      innerHL += 2; /* increment de-interleaved from original */
    }
    while (--innerB);

stop:
    /* Found. */
    vischar->b07 ^= vischar_BYTE7_BIT5;
    return 1; /* return NZ */

next:
    /* Next iteration. */
    objbounds += 4;
  }

  /* Not found. */
  return 0; /* return Z */
}

/* ----------------------------------------------------------------------- */

/**
 * $B2FC: reset_B2FC.
 *
 * Suspect this causes a transition to outdoors.
 *
 * \param[in] state Pointer to game state.
 */
void reset_B2FC(tgestate_t *state)
{
  reset_position(state, &state->vischars[0]); /* reset player */

  /* I'm avoiding a divide_by_8 call here. If it turns out to be self
   * modified it'll need restoring. */
  state->map_position[0] = (state->vischars[0].w18 >> 3) - 11;
  state->map_position[1] = (state->vischars[1].w1A >> 3) - 6;

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
  uint8_t         *HL;
  uint8_t          A;
  tinypos_t       *location;
  const doorpos_t *HLdoor;
  uint8_t          Cdash;
  uint8_t          Bdash;
  uint8_t          B;
  uint8_t         *DEdash;

  HL = &state->door_related[0];
  for (;;)
  {
    A = *HL;
    if (A == 255)
      return;

    state->current_door = A;

    HLdoor = get_door_position(A);
    A = HLdoor->room_and_flags;

    Cdash = A; // later masked
    Bdash = A & 3;
    if ((vischar->b0D & 3) != Bdash)
      goto next;

    HLdoor++; // advance by a byte ?

    // registers renamed here
    DEdash = &state->saved_pos.y; // reusing saved_pos as 8-bit here? a case for saved_pos being a union of tinypos and pos types?
    Bdash = 2; // 2 iterations
    do
    {
      //A = *HLdoor;
      if (((A - 3) >= *DEdash) || (A + 3) < *DEdash)
        goto next; // -3 .. +2
      DEdash += 2;
      HLdoor++; // advance by a byte ?
    }
    while (--Bdash);

    HLdoor++; // advance by a byte ?
    if (is_door_open(state) == 0)
      return; // door was closed

    vischar->room = (Cdash >> 2) & 0x3F;
    HLdoor++; // advance by a byte ?
    if (state->current_door & (1 << 7)) // unsure if this flag is gates_and_doors_LOCKED
      HLdoor -= 8;

    transition(state, vischar, HLdoor/*location*/);

    return; // exit via // with banked registers...

next:
    HL++;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B387: Player has tried to open the red cross parcel.
 *
 * \param[in] state Pointer to game state.
 */
void action_red_cross_parcel(tgestate_t *state)
{
  item_t *HL;

  state->item_structs[item_RED_CROSS_PARCEL].room = room_NONE & itemstruct_ROOM_MASK;

  HL = &state->items_held[0];
  if (*HL != item_RED_CROSS_PARCEL)
    HL++; /* One or the other must be a red cross parcel item, so advance. */
  *HL = item_NONE; /* Parcel opened. */

  draw_all_items(state);

  drop_item_A(state, state->red_cross_parcel_current_contents);

  queue_message_for_display(state, message_YOU_OPEN_THE_BOX);
  increase_morale_by_10_score_by_50(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B3A8: Player has tried to bribe a prisoner.
 *
 * This searches visible characters only.
 *
 * \param[in] state Pointer to game state.
 */
void action_bribe(tgestate_t *state)
{
  vischar_t *vc;        /* was HL */
  uint8_t    iters;     /* was B */
  uint8_t    character; /* was A */
  
  vc    = &state->vischars[1]; /* Walk visible characters. */
  iters = vischars_LENGTH - 1;
  do
  {
    character = vc->character;
    if (character != character_NONE && character >= character_20)
      goto found;
    vc++;
  }
  while (--iters);

  return;

found:
  state->bribed_character = character;
  vc->flags = vischar_BYTE1_PERSUE;
}

/* ----------------------------------------------------------------------- */

/**
 * $B3C4: Use poison.
 *
 * \param[in] state Pointer to game state.
 */
void action_poison(tgestate_t *state)
{
  if (state->items_held[0] != item_FOOD &&
      state->items_held[1] != item_FOOD)
    return; /* return - no poison */

  if (state->item_structs[item_FOOD].item & itemstruct_ITEM_FLAG_POISONED)
    return;

  state->item_structs[item_FOOD].item |= itemstruct_ITEM_FLAG_POISONED;

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

  sprite = &sprites[sprite_GUARD_FACING_TOP_LEFT_4];

  if (state->vischars[0].mi.spriteset == sprite)
    return; /* already in uniform */

  if (state->room_index >= room_29_SECOND_TUNNEL_START)
    return; /* can't don uniform when in a tunnel */

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
  if (state->room_index != room_50_BLOCKED_TUNNEL)
    return;

  if (roomdef_50_blocked_tunnel[2] == 255)
    return; /* blockage already cleared */

  roomdef_50_blocked_tunnel[2] = 255; /* release boundary */
  roomdef_50_blocked_tunnel[roomdef_50_BLOCKAGE] = interiorobject_TUNNEL_0; /* remove blockage graphic */

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

  wall = &walls[12]; /* == .d; - start of fences */
  pos = &state->player_map_position;
  iters = 4;
  do
  {
    uint8_t A; /* was A */

    A = pos->x;
    if (A < wall->d && A >= wall->c) // reverse
    {
      A = pos->y;
      if (A == wall->b)
        goto set_to_4;
      if (A - 1 == wall->b)
        goto set_to_6;
    }

    wall++;
  }
  while (--iters);

  wall = &walls[12 + 4]; // .a
  pos = &state->player_map_position;
  iters = 3;
  do
  {
    uint8_t A; /* was A */

    A = pos->y;
    if (A >= wall->a && A < wall->b)
    {
      A = pos->x;
      if (A == wall->c)
        goto set_to_5;
      if (A - 1 == wall->c)
        goto set_to_7;
    }

    wall++;
  }
  while (--iters);

  return;

set_to_4: // crawl TL
  flag = 4;
  goto action_wiresnips_tail;
  
set_to_5: // crawl TR
  flag = 5;
  goto action_wiresnips_tail;
  
set_to_6: // crawl BR
  flag = 6;
  goto action_wiresnips_tail;
  
set_to_7: // crawl BL
  flag = 7;

action_wiresnips_tail:
  state->vischars[0].b0E          = flag; // walk/crawl flag
  state->vischars[0].b0D          = vischar_BYTE13_BIT7;
  state->vischars[0].flags        = vischar_BYTE1_CUTTING_WIRE;
  state->vischars[0].mi.pos.vo    = 12; /* set vertical offset */
  state->vischars[0].mi.spriteset = &sprites[sprite_PRISONER_FACING_TOP_LEFT_4];
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

  HL = open_door(state);
  if (HL == NULL)
    return; /* Wrong door? */

  state->ptr_to_door_being_lockpicked = HL;
  state->player_locked_out_until = state->game_counter + 255;
  state->vischars[0].flags = vischar_BYTE1_PICKING_LOCK;
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
  uint8_t   A;
  message_t B = 0; // initialisation is temporary

  HL = open_door(state);
  if (HL == NULL)
    return; /* Wrong door? */

  A = *HL & ~gates_and_doors_LOCKED; // mask off locked flag
  if (A != room)
  {
    B = message_INCORRECT_KEY;
  }
  else
  {
    *HL &= ~gates_and_doors_LOCKED; // clear the locked flag
    increase_morale_by_10_score_by_50(state);
    B = message_IT_IS_OPEN;
  }

  queue_message_for_display(state, B);
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
  uint8_t         *gate;   /* was HL */
  uint8_t          iters;  /* was B */
  const doorpos_t *HLdash; /* was HL' */
  uint8_t          C;      /* was C */
  uint8_t         *DE;     /* was DE */
  uint8_t          Bdash;  /* was B' */
  uint8_t         *DEdash; /* was DE' */

  if (state->room_index)
  {
    /* Outdoors. */

    gate = &state->gates_and_doors[0]; // gates_flags
    iters = 5; /* iterations */ // gates_and_doors ranges must overlap
    do
    {
      uint8_t A;

      A = *gate & ~gates_and_doors_LOCKED;

      HLdash = get_door_position(A);
      if ((door_in_range(state, HLdash + 0) == 0) ||
          (door_in_range(state, HLdash + 1) == 0))
        return NULL; /* Conv: goto removed */

      gate++;
    }
    while (--iters);

    return NULL;
  }
  else
  {
    /* Indoors. */

    gate = &state->gates_and_doors[2]; // door_flags
    iters = 8; /* iterations */
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
    HLdash = get_door_position(*DE);
    /* Range check pattern (-2..+3). */
    DEdash = &state->saved_pos.y; // note: 16-bit values holding 8-bit values
    Bdash = 2; /* iterations */
    do
    {
      if (*DEdash <= HLdash->pos.y - 3 || *DEdash > HLdash->pos.y + 3)
        goto next;
      DEdash += 2; // -> saved_pos.x
      HLdash++;
    }
    while (--Bdash);

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
  { 0x6A, 0x6E, 0x52, 0x62, 0x00, 0x0B },
  { 0x5E, 0x62, 0x52, 0x62, 0x00, 0x0B },
  { 0x52, 0x56, 0x52, 0x62, 0x00, 0x0B },
  { 0x3E, 0x5A, 0x6A, 0x80, 0x00, 0x30 },
  { 0x34, 0x80, 0x72, 0x80, 0x00, 0x30 },
  { 0x7E, 0x98, 0x5E, 0x80, 0x00, 0x30 },
  { 0x82, 0x98, 0x5A, 0x80, 0x00, 0x30 },
  { 0x86, 0x8C, 0x46, 0x80, 0x00, 0x0A },
  { 0x82, 0x86, 0x46, 0x4A, 0x00, 0x12 },
  { 0x6E, 0x82, 0x46, 0x47, 0x00, 0x0A },
  { 0x6D, 0x6F, 0x45, 0x49, 0x00, 0x12 },
  { 0x67, 0x69, 0x45, 0x49, 0x00, 0x12 },
  { 0x46, 0x46, 0x46, 0x6A, 0x00, 0x08 }, // 12
  { 0x3E, 0x3E, 0x3E, 0x6A, 0x00, 0x08 },
  { 0x4E, 0x4E, 0x2E, 0x3E, 0x00, 0x08 },
  { 0x68, 0x68, 0x2E, 0x45, 0x00, 0x08 },
  { 0x3E, 0x68, 0x3E, 0x3E, 0x00, 0x08 },
  { 0x4E, 0x68, 0x2E, 0x2E, 0x00, 0x08 },
  { 0x46, 0x67, 0x46, 0x46, 0x00, 0x08 },
  { 0x68, 0x6A, 0x38, 0x3A, 0x00, 0x08 },
  { 0x4E, 0x50, 0x2E, 0x30, 0x00, 0x08 },
  { 0x46, 0x48, 0x46, 0x48, 0x00, 0x08 },
  { 0x46, 0x48, 0x5E, 0x60, 0x00, 0x08 },
  { 0x69, 0x6D, 0x46, 0x49, 0x00, 0x08 },
};

/* ----------------------------------------------------------------------- */

/**
 * $B5CE: called_from_main_loop_9
 *
 * \param[in] state Pointer to game state.
 */
void called_from_main_loop_9(tgestate_t *state)
{
  /**
   * $CDAA: (unknown)
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

  uint8_t    B;
  vischar_t *vischar; /* was IY */

  B = vischars_LENGTH; /* iterations */
  vischar = &state->vischars[0];
  do
  {
    if (vischar->flags == vischar_BYTE1_EMPTY_SLOT)
      goto next;

    // PUSH BC
    vischar->flags |= vischar_BYTE1_BIT7;

    if (vischar->b0D & vischar_BYTE13_BIT7)
      goto byte13bit7set;

#if 0
    H = vischar->b0B;
    L = vischar->w0A; // byte or word load?
    A = vischar->b0C;
    if (!even_parity(A))
    {
      A &= vischar_BYTE12_MASK;
      if (A == 0)
        goto snozzle;

      HL += (A + 1) * 4 - 1;
      A = *HL++;
      // EX AF, AF'

resume1:
      // EX DE,HL
      L = vischar->b0F; // Y axis
      H = vischar->b10;
      A = *DE; // sampled DE = $CF9A, $CF9E, $CFBE, $CFC2, $CFB2, $CFB6, $CFA6, $CFAA (character_related_data)
      C = A;
      A &= 0x80;
      if (A)
        A = 0xFF;
      B = A;
      HL -= BC;
      state->saved_pos.y = HL;
      DE++;

      L = vischar->b11; // X axis
      H = vischar->b12;
      A = *DE;
      C = A;
      A &= 0x80;
      if (A)
        A = 0xFF;
      B = A;
      HL -= BC;
      state->saved_pos.x = HL;
      DE++;

      L = vischar->b13; // vertical offset
      H = vischar->b14;
      A = *DE;
      C = A;
      A &= 0x80;
      if (A)
        A = 0xFF;
      B = A;
      HL -= BC;
      state->saved_pos.vo = HL;

      sub_AF8F(state, vischar);
      if (!Z)
        goto pop_next;

      vischar->b0C--;
    }
    else
    {
      if (A == *HL)
        goto snozzle;
      HL += (A + 1) * 4;

resume2:
      // EX DE,HL
      A = *DE;
      L = A;
      A &= 0x80;
      if (A)
        A = 0xFF;
      H = A;
      C = vischar->b0F; // Y axis
      B = vischar->b10;
      HL += BC;
      saved_Y = HL;
      DE++;

      A = *DE;
      L = A;
      A &= 0x80;
      if (A)
        A = 0xFF;
      H = A;
      C = vischar->b11; // X axis
      B = vischar->b12;
      HL += BC;
      saved_X = HL;
      DE++;

      A = *DE;
      L = A;
      A &= 0x80;
      if (A)
        A = 0xFF;
      H = A;
      C = vischar->b13; // vertical offset
      B = vischar->b14;
      HL += BC;
      saved_VO = HL;

      DE++;
      A = *DE;
      // EX AF,AF'

      sub_AF8F(state, vischar);
      if (!Z)
        goto pop_next;

      vischar->b0C++;
    }
    HL = vischar;
reset_position:
    reset_position_end(state, vischar);
#endif

pop_next:
    // POP BC
    if (vischar->flags != vischar_BYTE1_EMPTY_SLOT)
      vischar->flags &= ~vischar_BYTE1_BIT7; // $8001

next:
    vischar++;
  }
  while (--B);

byte13bit7set:
  vischar->b0D &= ~vischar_BYTE13_BIT7; // sampled IY = $8020, $80A0, $8060, $80E0, $8080,

#if 0
snozzle:
  A = byte_CDAA[vischar->b0E][vischar->b0D];
  C = A;
  L = vischar->w08;
  H = vischar->b09;
  HL += A * 2;
  E = *HL++;
  vischar->w0A = E;
  D = *HL;
  vischar->b0B = D;
  if ((C & (1 << 7)) == 0)
  {
    vischar->b0C = 0;
    DE += 2;
    vischar->b0E = *DE;
    DE += 2;
    // EX DE,HL
    goto resume2;
  }
  else
  {
    A = *DE;
    C = A;
    vischar->b0C = A | 0x80;
    vischar->b0E = *++DE;
    DE += 3;
    // PUSH DE
    // EX DE,HL
    HL += C * 4 - 1;
    A = *HL;
    // EX AF,AF'
    // POP HL
    goto resume1;
  }
#endif
}

/* ----------------------------------------------------------------------- */

/**
 * $B71B: Reset position.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 */
void reset_position(tgestate_t *state, vischar_t *vischar)
{
  /* Save a copy of the vischar's position + offset. */
  memcpy(&state->saved_pos, &vischar->mi.pos, 6);

  reset_position_end(state, vischar);
}

/**
 * $B729: Reset position (end bit).
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 */
void reset_position_end(tgestate_t *state, vischar_t *vischar)
{
  vischar->w18 = (state->saved_pos.x + 0x0200 - state->saved_pos.y) * 2;
  vischar->w1A = 0x0800 - state->saved_pos.y - state->saved_pos.vo - state->saved_pos.x;
}

/* ----------------------------------------------------------------------- */

/**
 * $B75A: Reset the game.
 *
 * \param[in] state Pointer to game state.
 */
void reset_game(tgestate_t *state)
{
  int    B;
  item_t C;

  /* Discover all items. */
  B = item__LIMIT;
  C = 0;
  do
    item_discovered(state, C++);
  while (--B);

  /* Reset message queue. */
  state->message_queue_pointer = &state->message_queue[2];
  reset_map_and_characters(state);
  state->vischars[0].flags = 0;

  /* Reset score. */
  memset(&state->score_digits[0], 0, 10);

  /* Reset morale. */
  state->morale = morale_MAX;
  plot_score(state);

  /* Reset items. */
  state->items_held[0] = item_NONE;
  state->items_held[1] = item_NONE;
  draw_all_items(state);

  /* Reset sprite. */
  state->vischars[0].mi.spriteset = &sprites[sprite_PRISONER_FACING_TOP_LEFT_4];

  /* Put player to bed. */
  state->room_index = room_2_HUT2LEFT;
  player_sleeps(state);

  enter_room(state); // returns by goto main_loop
  NEVER_RETURNS;
}

/* ----------------------------------------------------------------------- */

/**
 * $B79B: Resets all visible characters, clock, day_or_night flag, general
 * flags, collapsed tunnel objects, locks the gates, resets all beds, clears the
 * mess halls and resets characters.
 *
 * \param[in] state Pointer to game state.
 */
void reset_map_and_characters(tgestate_t *state)
{
  typedef struct character_reset_partial
  {
    room_t  room;
    uint8_t y, x; // partial of a tinypos_t
  }
  character_reset_partial_t;

  /**
   * $B819: Partial character struct for reset
   */
  static const character_reset_partial_t character_reset_data[10] =
  {
    { room_3_HUT2RIGHT, 40, 60 }, /* for character 12 */
    { room_3_HUT2RIGHT, 36, 48 }, /* for character 13 */
    { room_5_HUT3RIGHT, 40, 60 }, /* for character 14 */
    { room_5_HUT3RIGHT, 36, 34 }, /* for character 15 */
    { room_NONE,        52, 60 }, /* for character 20 */
    { room_NONE,        52, 44 }, /* for character 21 */
    { room_NONE,        52, 28 }, /* for character 22 */
    { room_NONE,        52, 60 }, /* for character 23 */
    { room_NONE,        52, 44 }, /* for character 24 */
    { room_NONE,        52, 28 }, /* for character 25 */
  };

  uint8_t                          B;
  vischar_t                       *HL;
  uint8_t                         *HLgate;
  uint8_t *const                  *HLbed;
  characterstruct_t               *DE;
  uint8_t                          C;
  const character_reset_partial_t *HLreset;

  B = vischars_LENGTH - 1; /* iterations */
  HL = &state->vischars[1]; /* iterate over non-player characters */
  do
    reset_visible_character(state, HL++);
  while (--B);

  state->clock = 7;
  state->day_or_night = 0;
  state->vischars[0].flags = 0;
  roomdef_50_blocked_tunnel[roomdef_50_BLOCKAGE] = interiorobject_COLLAPSED_TUNNEL;
  roomdef_50_blocked_tunnel[2] = 0x34; /* reset boundary */

  /* Lock the gates. */
  HLgate = &state->gates_and_doors[0];
  B = 9; // 9 iterations
  do
    *HLgate++ |= gates_and_doors_LOCKED;
  while (--B);

  /* Reset all beds. */
  B = beds_LENGTH; /* iterations */
  HLbed = &beds[0];
  do
    **HLbed = interiorobject_OCCUPIED_BED;
  while (--B);

  /* Clear the mess halls. */
  roomdef_23_breakfast[roomdef_23_BENCH_A] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_B] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_C] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_D] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_E] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_F] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_EMPTY_BENCH;

  /* Reset characters 12..15 and 20..25. */
  DE = &state->character_structs[character_12];
  C = NELEMS(character_reset_data); /* iterations */
  HLreset = &character_reset_data[0];
  do
  {
    DE->room   = HLreset->room;
    DE->pos.y  = HLreset->y;
    DE->pos.x  = HLreset->x;
    DE->pos.vo = 18; /* Odd: This is reset to 18 but the initial data is 24. */
    DE->target &= 0xFF00; /* clear bottom byte */
    DE++;
    HLreset++;

    if (C == 7)
      DE = &state->character_structs[20];
  }
  while (--C);
}

/* ----------------------------------------------------------------------- */

/**
 * $B83B searchlight_sub
 *
 * Looks like it's testing mask_buffer to find player?
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void searchlight_sub(tgestate_t *state, vischar_t *vischar)
{
  uint8_t     *HL;
  attribute_t  attrs;
  uint8_t      B;
  uint8_t      C;

  if (vischar > &state->vischars[0])
    return; /* skip non-player character */

  HL = &state->mask_buffer[0x31];
  B = 8; // 8 iterations
  C = 4; // i don't see this used
  do
  {
    if (*HL != 0)
      goto set_state_1f;
    HL += 4;
  }
  while (--B);

  if (--state->searchlight_state != 0xFF)
    return;

  attrs = choose_game_window_attributes(state);
  set_game_window_attributes(state, attrs);

  return;

set_state_1f:
  state->searchlight_state = searchlight_STATE_1F;
}

/* ----------------------------------------------------------------------- */

/**
 * $B866: Searchlight?
 *
 * \param[in] state Pointer to game state.
 */
void searchlight(tgestate_t *state)
{
  uint8_t    A;
  vischar_t *vischar; /* was IY */

  for (;;)
  {
    A = sub_B89C(state, &vischar); // must be returning IY too
#if 0
    if (!Z)
      return;

    if ((A & (1 << 6)) == 0) // mysteryflagconst874
    {
      setup_sprite_plotting(state, vischar);
      if (Z)
      {
        mask_stuff(state);
        if (searchlight_state != searchlight_STATE_OFF)
          searchlight_sub(state, vischar);
        if (vischar->width_bytes != 3)
          masked_sprite_plotter_24_wide(state, vischar);
        else
          masked_sprite_plotter_16_wide(state, vischar);
      }
    }
    else
    {
      setup_item_plotting(state, vischar, A); // A must be an item
      if (Z)
      {
        mask_stuff(state);
        masked_sprite_plotter_16_wide_searchlight(state);
      }
    }
#endif
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $B89C: (unknown)
 *
 * Called from searchlight().
 *
 * \param[in]  state    Pointer to game state.
 * \param[out] pvischar Pointer to receive vischar pointer.
 *
 * \return Nothing. // must return something - searchlight() tests Z flag on return and A // must return IY too. Returns an item | (1 << 6), or something else.
 */
uint8_t sub_B89C(tgestate_t *state, vischar_t **pvischar)
{
  uint16_t   BC;
  uint16_t   DE;
  uint8_t    A;
  uint16_t   DEdash;
  uint8_t    Bdash;
  vischar_t *HLdash;
  uint16_t   BCdash;

  BC = 0;
  DE = 0;
  A = 0xFF;
  // EX AF, AF'
  // EXX
  DEdash = 0;
  Bdash  = 8; /* iterations */
  HLdash = &state->vischars[0]; // was pointing to $8007
  do
  {
    if ((HLdash->b07 & vischar_BYTE7_BIT7) == 0)
      goto next;

#if 0
    // PUSH HLdash
    // PUSH BCdash

    //old HLdash += 8; // $8007 + 8 = $800F = mi.pos.y
    //old Cdash = *HLdash++;
    //old Bdash = *HLdash;
    //old BCdash += 4;
    BCdash = HLdash->mi.pos.y + 4;

    // PUSH BCdash

    // EXX

    // POP HL
    // SBC HL,BC

    // EXX

    // JR C,pop_next
    HLdash++;
    Cdash = *HLdash++;
    Bdash = *HLdash;
    BCdash += 4;
    // PUSH BCdash

    // EXX

    // POP HL
    // SBC HL,DE

    // EXX

    // JR C,pop_next
    HLdash++;
    // POP BCdash
    // PUSH BCdash
    Adash = 8 - Bdash;
    // EX AF,AF'  // unpaired
    Edash = *HLdash++;
    Ddash = *HLdash;
    // PUSH HLdash
    // EXX
    // POP HL
    HL -= 2;
    D = *HL--;
    E = *HL--;
    B = *HL--;
    C = *HL;
    HL -= 15;
    IY = HL;
    // EXX
#endif

pop_next:
    // POP BCdash
    // POP HLdash

next:
    HLdash += 32; // stride
  }
  while (--Bdash);

  // Bdash must be zero, Cdash must be 32 (stride)
  BCdash = 32; // sampling proves this is always 32
  //Adash = sub_DBEB(state, BCdash, &IY);
  // IY is returned from this, but is a itemstruct_t not a vischar
#if 0
  // EX AF, AF' // return value
  if (A & (1 << 7)) // hard to tell if this is A or Adash as above loop has an unpaired EX AF,AF' in it
    return A;

  HL = IY;
  if ((A & (1 << 6)) == 0) // mysteryflagconst874
  {
    IY->b07 &= ~vischar_BYTE7_BIT7;
  }
  else
  {
    HL->flags &= ~vischar_BYTE1_BIT6; // returning an item in this case
    //BIT 6,HL[1]  // this tests the bit we've just cleared
  }
#endif

  // *pvischar = IY;  will be needed. possibly in other paths too.

  return A;
}

/* ----------------------------------------------------------------------- */

/**
 * $B916: Suspected mask stuff.
 *
 * \param[in] state Pointer to game state.
 */
void mask_stuff(tgestate_t *state)
{
  // { byte count+flags; ... }

  /* $E55F */
  static const uint8_t outdoors_mask_0[] =
  {
    0x2A, 0xA0, 0x00, 0x05, 0x07, 0x08, 0x09, 0x01,
    0x0A, 0xA2, 0x00, 0x05, 0x06, 0x04, 0x85, 0x01,
    0x0B, 0x9F, 0x00, 0x05, 0x06, 0x04, 0x88, 0x01,
    0x0C, 0x9C, 0x00, 0x05, 0x06, 0x04, 0x8A, 0x01,
    0x0D, 0x0E, 0x99, 0x00, 0x05, 0x06, 0x04, 0x8D,
    0x01, 0x0F, 0x10, 0x96, 0x00, 0x05, 0x06, 0x04,
    0x90, 0x01, 0x11, 0x94, 0x00, 0x05, 0x06, 0x04,
    0x92, 0x01, 0x12, 0x92, 0x00, 0x05, 0x06, 0x04,
    0x94, 0x01, 0x12, 0x90, 0x00, 0x05, 0x06, 0x04,
    0x96, 0x01, 0x12, 0x8E, 0x00, 0x05, 0x06, 0x04,
    0x98, 0x01, 0x12, 0x8C, 0x00, 0x05, 0x06, 0x04,
    0x9A, 0x01, 0x12, 0x8A, 0x00, 0x05, 0x06, 0x04,
    0x9C, 0x01, 0x12, 0x88, 0x00, 0x05, 0x06, 0x04,
    0x9E, 0x01, 0x18, 0x86, 0x00, 0x05, 0x06, 0x04,
    0xA1, 0x01, 0x84, 0x00, 0x05, 0x06, 0x04, 0xA3,
    0x01, 0x00, 0x00, 0x05, 0x06, 0x04, 0xA5, 0x01,
    0x05, 0x03, 0x04, 0xA7, 0x01, 0x02, 0xA9, 0x01,
    0x02, 0xA9, 0x01, 0x02, 0xA9, 0x01, 0x02, 0xA9,
    0x01, 0x02, 0xA9, 0x01, 0x02, 0xA9, 0x01, 0x02,
    0xA9, 0x01, 0x02, 0xA9, 0x01, 0x02, 0xA9, 0x01,
  };

  /* $E5FF */
  static const uint8_t outdoors_mask_1[] =
  {
    0x12, 0x02, 0x91, 0x01, 0x02, 0x91, 0x01, 0x02,
    0x91, 0x01, 0x02, 0x91, 0x01, 0x02, 0x91, 0x01,
    0x02, 0x91, 0x01, 0x02, 0x91, 0x01, 0x02, 0x91,
    0x01, 0x02, 0x91, 0x01, 0x02, 0x91, 0x01
  };

  /* $E61E */
  static const uint8_t outdoors_mask_2[] =
  {
    0x10, 0x13, 0x14, 0x15, 0x8D, 0x00, 0x16, 0x17,
    0x18, 0x17, 0x15, 0x8B, 0x00, 0x19, 0x1A, 0x1B,
    0x17, 0x18, 0x17, 0x15, 0x89, 0x00, 0x19, 0x1A,
    0x1C, 0x1A, 0x1B, 0x17, 0x18, 0x17, 0x15, 0x87,
    0x00, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1B,
    0x17, 0x13, 0x14, 0x15, 0x85, 0x00, 0x19, 0x1A,
    0x1C, 0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x16, 0x17,
    0x18, 0x17, 0x15, 0x83, 0x00, 0x19, 0x1A, 0x1C,
    0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1B,
    0x17, 0x18, 0x17, 0x15, 0x00, 0x19, 0x1A, 0x1C,
    0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C,
    0x1A, 0x1B, 0x17, 0x18, 0x17, 0x00, 0x20, 0x1C,
    0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C,
    0x1A, 0x1C, 0x1A, 0x1B, 0x17, 0x83, 0x00, 0x20,
    0x1C, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1A,
    0x1C, 0x1A, 0x1C, 0x1D, 0x85, 0x00, 0x20, 0x1C,
    0x1D, 0x19, 0x1A, 0x1C, 0x1A, 0x1C, 0x1A, 0x1C,
    0x1D, 0x87, 0x00, 0x1F, 0x19, 0x1A, 0x1C, 0x1A,
    0x1C, 0x1A, 0x1C, 0x1D, 0x89, 0x00, 0x20, 0x1C,
    0x1A, 0x1C, 0x1A, 0x1C, 0x1D, 0x8B, 0x00, 0x20,
    0x1C, 0x1A, 0x1C, 0x1D, 0x8D, 0x00, 0x20, 0x1C,
    0x1D, 0x8F, 0x00, 0x1F
  };

  /* $E6CA */
  static const uint8_t outdoors_mask_3[] =
  {
    0x1A, 0x88, 0x00, 0x05, 0x4C, 0x90, 0x00, 0x86,
    0x00, 0x05, 0x06, 0x04, 0x32, 0x30, 0x4C, 0x8E,
    0x00, 0x84, 0x00, 0x05, 0x06, 0x04, 0x84, 0x01,
    0x32, 0x30, 0x4C, 0x8C, 0x00, 0x00, 0x00, 0x05,
    0x06, 0x04, 0x88, 0x01, 0x32, 0x30, 0x4C, 0x8A,
    0x00, 0x00, 0x06, 0x04, 0x8C, 0x01, 0x32, 0x30,
    0x4C, 0x88, 0x00, 0x02, 0x90, 0x01, 0x32, 0x30,
    0x4C, 0x86, 0x00, 0x02, 0x92, 0x01, 0x32, 0x30,
    0x4C, 0x84, 0x00, 0x02, 0x94, 0x01, 0x32, 0x30,
    0x4C, 0x00, 0x00, 0x02, 0x96, 0x01, 0x32, 0x30,
    0x00, 0x02, 0x98, 0x01, 0x12, 0x02, 0x98, 0x01,
    0x12, 0x02, 0x98, 0x01, 0x12, 0x02, 0x98, 0x01,
    0x12, 0x02, 0x98, 0x01, 0x12, 0x02, 0x98, 0x01,
    0x12, 0x02, 0x98, 0x01, 0x12, 0x02, 0x98, 0x01,
    0x12, 0x02, 0x98, 0x01, 0x12, 0x02, 0x98, 0x01,
    0x12, 0x02, 0x98, 0x01, 0x12, 0x02, 0x98, 0x01,
    0x12
  };

  /* $E74B */
  static const uint8_t outdoors_mask_4[] =
  {
    0x0D, 0x02, 0x8C, 0x01, 0x02, 0x8C, 0x01, 0x02,
    0x8C, 0x01, 0x02, 0x8C, 0x01
  };

  /* $E758 */
  static const uint8_t outdoors_mask_5[] =
  {
    0x0E, 0x02, 0x8C, 0x01, 0x12, 0x02, 0x8C, 0x01,
    0x12, 0x02, 0x8C, 0x01, 0x12, 0x02, 0x8C, 0x01,
    0x12, 0x02, 0x8C, 0x01, 0x12, 0x02, 0x8C, 0x01,
    0x12, 0x02, 0x8C, 0x01, 0x12, 0x02, 0x8C, 0x01,
    0x12, 0x02, 0x8D, 0x01, 0x02, 0x8D, 0x01
  };

  /* $E77F */
  static const uint8_t outdoors_mask_6[] =
  {
    0x08, 0x5B, 0x5A, 0x86, 0x00, 0x01, 0x01, 0x5B,
    0x5A, 0x84, 0x00, 0x84, 0x01, 0x5B, 0x5A, 0x00,
    0x00, 0x86, 0x01, 0x5B, 0x5A, 0xD8, 0x01
  };


  /* $E796 */
  static const uint8_t outdoors_mask_7[] =
  {
    0x09, 0x88, 0x01, 0x12, 0x88, 0x01, 0x12, 0x88,
    0x01, 0x12, 0x88, 0x01, 0x12, 0x88, 0x01, 0x12,
    0x88, 0x01, 0x12, 0x88, 0x01, 0x12, 0x88, 0x01,
    0x12
  };

  /* $E7AF */
  static const uint8_t outdoors_mask_8[] =
  {
    0x10, 0x8D, 0x00, 0x23, 0x24, 0x25, 0x8B, 0x00,
    0x23, 0x26, 0x27, 0x26, 0x28, 0x89, 0x00, 0x23,
    0x26, 0x27, 0x26, 0x22, 0x29, 0x2A, 0x87, 0x00,
    0x23, 0x26, 0x27, 0x26, 0x22, 0x29, 0x2B, 0x29,
    0x2A, 0x85, 0x00, 0x23, 0x24, 0x25, 0x26, 0x22,
    0x29, 0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x83, 0x00,
    0x23, 0x26, 0x27, 0x26, 0x28, 0x2F, 0x2B, 0x29,
    0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x00, 0x23, 0x26,
    0x27, 0x26, 0x22, 0x29, 0x2A, 0x2F, 0x2B, 0x29,
    0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x26, 0x27, 0x26,
    0x22, 0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x29,
    0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x26, 0x22, 0x29,
    0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x29,
    0x2B, 0x29, 0x2B, 0x31, 0x2D, 0x2F, 0x2B, 0x29,
    0x2B, 0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x29,
    0x2B, 0x31, 0x83, 0x00, 0x2F, 0x2B, 0x29, 0x2B,
    0x29, 0x2B, 0x29, 0x2A, 0x2F, 0x2B, 0x31, 0x85,
    0x00, 0x2F, 0x2B, 0x29, 0x2B, 0x29, 0x2B, 0x29,
    0x2A, 0x2E, 0x87, 0x00, 0x2F, 0x2B, 0x29, 0x2B,
    0x29, 0x2B, 0x31, 0x2D, 0x88, 0x00, 0x2F, 0x2B,
    0x29, 0x2B, 0x31, 0x8B, 0x00, 0x2F, 0x2B, 0x31,
    0x8D, 0x00, 0x2E, 0x8F, 0x00
  };

  /* $E85C */
  static const uint8_t outdoors_mask_9[] =
  {
    0x0A, 0x83, 0x00, 0x05, 0x06, 0x30, 0x4C, 0x83,
    0x00, 0x00, 0x05, 0x06, 0x04, 0x01, 0x01, 0x32,
    0x30, 0x4C, 0x00, 0x34, 0x04, 0x86, 0x01, 0x32,
    0x33, 0x83, 0x00, 0x40, 0x01, 0x01, 0x3F, 0x83,
    0x00, 0x02, 0x46, 0x47, 0x48, 0x49, 0x42, 0x41,
    0x45, 0x44, 0x12, 0x34, 0x01, 0x01, 0x46, 0x4B,
    0x43, 0x44, 0x01, 0x01, 0x33, 0x00, 0x3C, 0x3E,
    0x40, 0x01, 0x01, 0x3F, 0x37, 0x39, 0x00, 0x83,
    0x00, 0x3D, 0x3A, 0x3B, 0x38, 0x83, 0x00
  };

  /* $E8A3 */
  static const uint8_t outdoors_mask_10[] =
  {
    0x08, 0x35, 0x86, 0x01, 0x36, 0x90, 0x01, 0x88,
    0x00, 0x3C, 0x86, 0x00, 0x39, 0x3C, 0x00, 0x02,
    0x36, 0x35, 0x12, 0x00, 0x39, 0x3C, 0x00, 0x02,
    0x01, 0x01, 0x12, 0x00, 0x39, 0x3C, 0x00, 0x02,
    0x01, 0x01, 0x12, 0x00, 0x39, 0x3C, 0x00, 0x02,
    0x01, 0x01, 0x12, 0x00, 0x39, 0x3C, 0x00, 0x02,
    0x01, 0x01, 0x12, 0x00, 0x39, 0x3C, 0x00, 0x02,
    0x01, 0x01, 0x12, 0x00, 0x39, 0x3C, 0x00, 0x02,
    0x01, 0x01, 0x12, 0x00, 0x39, 0x3C, 0x00, 0x02,
    0x01, 0x01, 0x12, 0x00, 0x39
  };

  /* $E8F0 */
  static const uint8_t outdoors_mask_11[] =
  {
    0x08, 0x01, 0x4F, 0x86, 0x00, 0x01, 0x50, 0x01,
    0x4F, 0x84, 0x00, 0x01, 0x00, 0x00, 0x51, 0x01,
    0x4F, 0x00, 0x00, 0x01, 0x00, 0x00, 0x53, 0x19,
    0x50, 0x01, 0x4F, 0x01, 0x00, 0x00, 0x53, 0x19,
    0x00, 0x00, 0x52, 0x01, 0x00, 0x00, 0x53, 0x19,
    0x00, 0x00, 0x52, 0x01, 0x54, 0x00, 0x53, 0x19,
    0x00, 0x00, 0x52, 0x83, 0x00, 0x55, 0x19, 0x00,
    0x00, 0x52, 0x85, 0x00, 0x54, 0x00, 0x52
  };

  /* $E92F */
  static const uint8_t outdoors_mask_12[] =
  {
    0x02, 0x56, 0x57, 0x56, 0x57, 0x58, 0x59, 0x58,
    0x59, 0x58, 0x59, 0x58, 0x59, 0x58, 0x59, 0x58,
    0x59
  };

  /* $E940 */
  static const uint8_t outdoors_mask_13[] =
  {
    0x05, 0x00, 0x00, 0x23, 0x24, 0x25, 0x02, 0x00,
    0x27, 0x26, 0x28, 0x02, 0x00, 0x22, 0x26, 0x28,
    0x02, 0x00, 0x2B, 0x29, 0x2A, 0x02, 0x00, 0x2B,
    0x29, 0x2A, 0x02, 0x00, 0x2B, 0x29, 0x2A, 0x02,
    0x00, 0x2B, 0x29, 0x2A, 0x02, 0x00, 0x2B, 0x29,
    0x2A, 0x02, 0x00, 0x2B, 0x31, 0x00, 0x02, 0x00,
    0x83, 0x00
  };

  /* $E972 */
  static const uint8_t outdoors_mask_14[] =
  {
    0x04, 0x19, 0x83, 0x00, 0x19, 0x17, 0x15, 0x00,
    0x19, 0x17, 0x18, 0x17, 0x19, 0x1A, 0x1B, 0x17,
    0x19, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1D,
    0x19, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1D,
    0x19, 0x1A, 0x1C, 0x1D, 0x00, 0x20, 0x1C, 0x1D
  };

  /* $E99A */
  static const uint8_t outdoors_mask_15[] =
  {
    0x02, 0x04, 0x32, 0x01, 0x01
  };

  /* $E99F */
  static const uint8_t outdoors_mask_16[] =
  {
    0x09, 0x86, 0x00, 0x5D, 0x5C, 0x54, 0x84, 0x00,
    0x5D, 0x5C, 0x01, 0x01, 0x01, 0x00, 0x00, 0x5D,
    0x5C, 0x85, 0x01, 0x5D, 0x5C, 0x87, 0x01, 0x2B,
    0x88, 0x01
  };

  /* $E9B9 */
  static const uint8_t outdoors_mask_17[] =
  {
    0x05, 0x00, 0x00, 0x5D, 0x5C, 0x67, 0x5D, 0x5C,
    0x83, 0x01, 0x3C, 0x84, 0x01
  };

  /* $E9C6 */
  static const uint8_t outdoors_mask_18[] =
  {
    0x02, 0x5D, 0x68, 0x3C, 0x69
  };

  /* $E9CB */
  static const uint8_t outdoors_mask_19[] =
  {
    0x0A, 0x86, 0x00, 0x5D, 0x5C, 0x46, 0x47, 0x84,
    0x00, 0x5D, 0x5C, 0x83, 0x01, 0x39, 0x00, 0x00,
    0x5D, 0x5C, 0x86, 0x01, 0x5D, 0x5C, 0x88, 0x01,
    0x4A, 0x89, 0x01
  };

  /* $E9E6 */
  static const uint8_t outdoors_mask_20[] =
  {
    0x06, 0x5D, 0x5C, 0x01, 0x47, 0x6A, 0x00, 0x4A,
    0x84, 0x01, 0x6B, 0x00, 0x84, 0x01, 0x5F
  };

  /* $E9F5 */
  static const uint8_t outdoors_mask_21[] =
  {
    0x04, 0x05, 0x4C, 0x00, 0x00, 0x61, 0x65, 0x66,
    0x4C, 0x61, 0x12, 0x02, 0x60, 0x61, 0x12, 0x02,
    0x60, 0x61, 0x12, 0x02, 0x60, 0x61, 0x12, 0x02,
    0x60
  };

  /* $EA0E */
  static const uint8_t outdoors_mask_22[] =
  {
    0x04, 0x00, 0x00, 0x05, 0x4C, 0x05, 0x63, 0x64,
    0x60, 0x61, 0x12, 0x02, 0x60, 0x61, 0x12, 0x02,
    0x60, 0x61, 0x12, 0x02, 0x60, 0x61, 0x12, 0x02,
    0x60, 0x61, 0x12, 0x62, 0x00
  };

  /* $EA2B */
  static const uint8_t outdoors_mask_23[] =
  {
    0x03, 0x00, 0x6C, 0x00, 0x02, 0x01, 0x68, 0x02,
    0x01, 0x69
  };

  /* $EA35 */
  static const uint8_t outdoors_mask_24[] =
  {
    0x05, 0x01, 0x5E, 0x4C, 0x00, 0x00, 0x01, 0x01,
    0x32, 0x30, 0x00, 0x84, 0x01, 0x5F
  };

  /* $EA43 */
  static const uint8_t outdoors_mask_25[] =
  {
    0x02, 0x6E, 0x5A, 0x6D, 0x39, 0x3C, 0x39
  };

  /* $EA4A */
  static const uint8_t outdoors_mask_26[] =
  {
    0x04, 0x5D, 0x5C, 0x46, 0x47, 0x4A, 0x01, 0x01,
    0x39
  };

  /* $EA53 */
  static const uint8_t outdoors_mask_27[] =
  {
    0x03, 0x2C, 0x47, 0x00, 0x00, 0x61, 0x12, 0x00,
    0x61, 0x12
  };

  /* $EA5D */
  static const uint8_t outdoors_mask_28[] =
  {
    0x03, 0x00, 0x45, 0x1E, 0x02, 0x60, 0x00, 0x02,
    0x60, 0x00
  };

  /* $EA67 */
  static const uint8_t outdoors_mask_29[] =
  {
    0x05, 0x45, 0x1E, 0x2C, 0x47, 0x00, 0x2C, 0x47,
    0x45, 0x1E, 0x12, 0x00, 0x61, 0x12, 0x61, 0x12,
    0x00, 0x61, 0x5F, 0x00, 0x00
  };

  /**
   * $EBC5: Probably mask data pointers.
   */
  static const uint8_t *exterior_mask_pointers[36] =
  {
    &outdoors_mask_0[0],  /* $E55F */
    &outdoors_mask_1[0],  /* $E5FF */
    &outdoors_mask_2[0],  /* $E61E */
    &outdoors_mask_3[0],  /* $E6CA */
    &outdoors_mask_4[0],  /* $E74B */
    &outdoors_mask_5[0],  /* $E758 */
    &outdoors_mask_6[0],  /* $E77F */
    &outdoors_mask_7[0],  /* $E796 */
    &outdoors_mask_8[0],  /* $E7AF */
    &outdoors_mask_9[0],  /* $E85C */
    &outdoors_mask_10[0], /* $E8A3 */
    &outdoors_mask_11[0], /* $E8F0 */
    &outdoors_mask_13[0], /* $E940 */
    &outdoors_mask_14[0], /* $E972 */
    &outdoors_mask_12[0], /* $E92F */
    &outdoors_mask_29[0], /* $EA67 */
    &outdoors_mask_27[0], /* $EA53 */
    &outdoors_mask_28[0], /* $EA5D */
    &outdoors_mask_15[0], /* $E99A */
    &outdoors_mask_16[0], /* $E99F */
    &outdoors_mask_17[0], /* $E9B9 */
    &outdoors_mask_18[0], /* $E9C6 */
    &outdoors_mask_19[0], /* $E9CB */
    &outdoors_mask_20[0], /* $E9E6 */
    &outdoors_mask_21[0], /* $E9F5 */
    &outdoors_mask_22[0], /* $EA0E */
    &outdoors_mask_23[0], /* $EA2B */
    &outdoors_mask_24[0], /* $EA35 */
    &outdoors_mask_25[0], /* $EA43 */
    &outdoors_mask_26[0]  /* $EA4A */
  };

  /**
   * $EC01: 58 x 8-byte structs.
   *
   * { ?, y,y, x,x, y, x, vo }
   */
  static const eightbyte_t exterior_mask_data[58] =
  {
    { 0x00, 0x47, 0x70, 0x27, 0x3F, 0x6A, 0x52, 0x0C },
    { 0x00, 0x5F, 0x88, 0x33, 0x4B, 0x5E, 0x52, 0x0C },
    { 0x00, 0x77, 0xA0, 0x3F, 0x57, 0x52, 0x52, 0x0C },
    { 0x01, 0x9F, 0xB0, 0x28, 0x31, 0x3E, 0x6A, 0x3C },
    { 0x01, 0x9F, 0xB0, 0x32, 0x3B, 0x3E, 0x6A, 0x3C },
    { 0x02, 0x40, 0x4F, 0x4C, 0x5B, 0x46, 0x46, 0x08 },
    { 0x02, 0x50, 0x5F, 0x54, 0x63, 0x46, 0x46, 0x08 },
    { 0x02, 0x60, 0x6F, 0x5C, 0x6B, 0x46, 0x46, 0x08 },
    { 0x02, 0x70, 0x7F, 0x64, 0x73, 0x46, 0x46, 0x08 },
    { 0x02, 0x30, 0x3F, 0x54, 0x63, 0x3E, 0x3E, 0x08 },
    { 0x02, 0x40, 0x4F, 0x5C, 0x6B, 0x3E, 0x3E, 0x08 },
    { 0x02, 0x50, 0x5F, 0x64, 0x73, 0x3E, 0x3E, 0x08 },
    { 0x02, 0x60, 0x6F, 0x6C, 0x7B, 0x3E, 0x3E, 0x08 },
    { 0x02, 0x70, 0x7F, 0x74, 0x83, 0x3E, 0x3E, 0x08 },
    { 0x02, 0x10, 0x1F, 0x64, 0x73, 0x4A, 0x2E, 0x08 },
    { 0x02, 0x20, 0x2F, 0x6C, 0x7B, 0x4A, 0x2E, 0x08 },
    { 0x02, 0x30, 0x3F, 0x74, 0x83, 0x4A, 0x2E, 0x08 },
    { 0x03, 0x2B, 0x44, 0x33, 0x47, 0x67, 0x45, 0x12 },
    { 0x04, 0x2B, 0x37, 0x48, 0x4B, 0x6D, 0x45, 0x08 },
    { 0x05, 0x37, 0x44, 0x48, 0x51, 0x67, 0x45, 0x08 },
    { 0x06, 0x08, 0x0F, 0x2A, 0x3C, 0x6E, 0x46, 0x0A },
    { 0x06, 0x10, 0x17, 0x2E, 0x40, 0x6E, 0x46, 0x0A },
    { 0x06, 0x18, 0x1F, 0x32, 0x44, 0x6E, 0x46, 0x0A },
    { 0x06, 0x20, 0x27, 0x36, 0x48, 0x6E, 0x46, 0x0A },
    { 0x06, 0x28, 0x2F, 0x3A, 0x4C, 0x6E, 0x46, 0x0A },
    { 0x07, 0x08, 0x10, 0x1F, 0x26, 0x82, 0x46, 0x12 },
    { 0x07, 0x08, 0x10, 0x27, 0x2D, 0x82, 0x46, 0x12 },
    { 0x08, 0x80, 0x8F, 0x64, 0x73, 0x46, 0x46, 0x08 },
    { 0x08, 0x90, 0x9F, 0x5C, 0x6B, 0x46, 0x46, 0x08 },
    { 0x08, 0xA0, 0xB0, 0x54, 0x63, 0x46, 0x46, 0x08 },
    { 0x08, 0xB0, 0xBF, 0x4C, 0x5B, 0x46, 0x46, 0x08 },
    { 0x08, 0xC0, 0xCF, 0x44, 0x53, 0x46, 0x46, 0x08 },
    { 0x08, 0x80, 0x8F, 0x74, 0x83, 0x3E, 0x3E, 0x08 },
    { 0x08, 0x90, 0x9F, 0x6C, 0x7B, 0x3E, 0x3E, 0x08 },
    { 0x08, 0xA0, 0xB0, 0x64, 0x73, 0x3E, 0x3E, 0x08 },
    { 0x08, 0xB0, 0xBF, 0x5C, 0x6B, 0x3E, 0x3E, 0x08 },
    { 0x08, 0xC0, 0xCF, 0x54, 0x63, 0x3E, 0x3E, 0x08 },
    { 0x08, 0xD0, 0xDF, 0x4C, 0x5B, 0x3E, 0x3E, 0x08 },
    { 0x08, 0x40, 0x4F, 0x74, 0x83, 0x4E, 0x2E, 0x08 },
    { 0x08, 0x50, 0x5F, 0x6C, 0x7B, 0x4E, 0x2E, 0x08 },
    { 0x08, 0x10, 0x1F, 0x58, 0x67, 0x68, 0x2E, 0x08 },
    { 0x08, 0x20, 0x2F, 0x50, 0x5F, 0x68, 0x2E, 0x08 },
    { 0x08, 0x30, 0x3F, 0x48, 0x57, 0x68, 0x2E, 0x08 },
    { 0x09, 0x1B, 0x24, 0x4E, 0x55, 0x68, 0x37, 0x0F },
    { 0x0A, 0x1C, 0x23, 0x51, 0x5D, 0x68, 0x38, 0x0A },
    { 0x09, 0x3B, 0x44, 0x72, 0x79, 0x4E, 0x2D, 0x0F },
    { 0x0A, 0x3C, 0x43, 0x75, 0x81, 0x4E, 0x2E, 0x0A },
    { 0x09, 0x7B, 0x84, 0x62, 0x69, 0x46, 0x45, 0x0F },
    { 0x0A, 0x7C, 0x83, 0x65, 0x71, 0x46, 0x46, 0x0A },
    { 0x09, 0xAB, 0xB4, 0x4A, 0x51, 0x46, 0x5D, 0x0F },
    { 0x0A, 0xAC, 0xB3, 0x4D, 0x59, 0x46, 0x5E, 0x0A },
    { 0x0B, 0x58, 0x5F, 0x5A, 0x62, 0x46, 0x46, 0x08 },
    { 0x0B, 0x48, 0x4F, 0x62, 0x6A, 0x3E, 0x3E, 0x08 },
    { 0x0C, 0x0B, 0x0F, 0x60, 0x67, 0x68, 0x2E, 0x08 },
    { 0x0D, 0x0C, 0x0F, 0x61, 0x6A, 0x4E, 0x2E, 0x08 },
    { 0x0E, 0x7F, 0x80, 0x7C, 0x83, 0x3E, 0x3E, 0x08 },
    { 0x0D, 0x2C, 0x2F, 0x51, 0x5A, 0x3E, 0x3E, 0x08 },
    { 0x0D, 0x3C, 0x3F, 0x49, 0x52, 0x46, 0x46, 0x08 },
  };

  uint8_t            iters;       /* was B */
  uint8_t            A;           /* was A */
  const eightbyte_t *peightbyte;  /* was HL */
  uint8_t            C;           /* was C */
  const uint8_t     *DE;          /* was DE */
  uint16_t           HL2;         /* was HL */
  const uint8_t     *saved_DE;    /* was stacked */
  uint8_t            E;           /* was E */
  uint8_t           *HL3;         /* was HL */
  uint8_t            iters2;      /* was C */

  /* Clear the mask buffer. */
  memset(&state->mask_buffer[0], 0xFF, NELEMS(state->mask_buffer));

  if (state->room_index)
  {
    /* Indoors */

    A = state->indoor_mask_data_count; /* count byte */
    if (A == 0)
      return; /* no masks */
    
    iters = A;
    
    peightbyte = &state->indoor_mask_data[0];
    //peightbyte += 2; // skip count byte, then off by 2 bytes
  }
  else
  {
    /* Outdoors */

    iters = 59; /* iterations */ // this count doesn't match NELEMS(exterior_mask_data); which is 58
    peightbyte = &exterior_mask_data[0]; // off by - 2 bytes; original points to $EC03, table starts at $EC01 // fix by propogation
  }
  
  do
  {
    uint8_t mpr1, mpr2; /* was C/A */
    uint8_t B;
    uint8_t self_BA70;
    uint8_t self_BA72;
    uint8_t self_BA90;
    uint8_t self_BABA;
    
    // PUSH BC
    // PUSH HLeb
    
    A = state->map_position_related_1;
    if (A - 1 >= peightbyte->eb_c || A + 2 <= peightbyte->eb_b) // $EC03, $EC02
      goto pop_next;
    
    A = state->map_position_related_2;
    if (A - 1 >= peightbyte->eb_e || A + 3 <= peightbyte->eb_d) // $EC05, $EC04
      goto pop_next;
    
    if (state->tinypos_81B2.y <= peightbyte->eb_f) // $EC06
      goto pop_next;
    
    if (state->tinypos_81B2.x < peightbyte->eb_g) // $EC07
      goto pop_next;
    
    A = state->tinypos_81B2.vo;
    if (A)
      A--; // make inclusive?
    if (A >= peightbyte->eb_h) // $EC08
      goto pop_next;
    
    // eb_f/g/h could be a tinypos_t
    
    // redundant: HLeb -= 6;
    
    mpr1 = state->map_position_related_1;
    if (mpr1 >= peightbyte->eb_b) // must be $EC02
    {
      state->byte_B837 = mpr1 - peightbyte->eb_b;
      state->byte_B83A = MIN(peightbyte->eb_c - mpr1, 3) + 1;
    }
    else
    {
      uint8_t eb_b;
      
      eb_b = peightbyte->eb_b; // must be $EC02
      state->byte_B837 = 0;
      state->byte_B83A = MIN((peightbyte->eb_c - eb_b) + 1, 4 - (eb_b - mpr1));
    }
    
    mpr2 = state->map_position_related_2;
    if (mpr2 >= peightbyte->eb_d)
    {
      state->byte_B838 = mpr2 - peightbyte->eb_d;
      state->byte_B839 = MIN(peightbyte->eb_e - mpr2, 4) + 1;
    }
    else
    {
      uint8_t eb_d;
      
      eb_d = peightbyte->eb_d;
      state->byte_B838 = 0;
      state->byte_B839 = MIN((peightbyte->eb_e - eb_d) + 1, 5 - (eb_d - mpr2));
    }
    
    // In the original code, HLeb is here decremented to point at member eb_d.
    
    B = C = 0;
    if (state->byte_B838 == 0)
      C = -state->map_position_related_2 + peightbyte->eb_d;
    if (state->byte_B837 == 0)
      B = -state->map_position_related_1 + peightbyte->eb_b;
    A = peightbyte->eb_a;
    
    state->mask_buffer_pointer = &state->mask_buffer[C * 32 + B];
    
    // If I break this bit then the character gets drawn on top of *indoors* objects.
    
    DE = exterior_mask_pointers[A];
    HL2 = state->byte_B839 | (state->byte_B83A << 8); // vertical bits
    
    self_BA70 = (HL2 >> 0) & 0xFF; // self modify
    self_BA72 = (HL2 >> 8) & 0xFF; // self modify
    self_BA90 = *DE - self_BA72; // self modify // *DE looks like a count
    self_BABA = 32 - self_BA72; // self modify
    
    saved_DE = DE; // PUSH DE
    
    E = *DE; // same count again
    HL2 = scale_val(state, state->byte_B838, E); // args in regs A,E // call this HL3? // sets D to 0
    E = state->byte_B837; // horz
    HL2 += E; // was +DE, but D is zero
    
    DE = saved_DE; // POP DE
    
    HL2++; /* iterations */
    do
    {
    ba4d:
      A = *DE; // DE -> $E560 upwards (in probably_mask_data)
      //if (!even_parity(A))
      {
        // uneven number of bits set
        A &= 0x7F;
        DE++;
        HL2 -= A;
        if (HL2 < 0)
          goto ba69;
        DE++;
        if (DE)
          goto ba4d;
        A = 0;
        goto ba6c;
      }
      DE++;
    }
    while (--HL2);
    goto ba6c;
    
  ba69:
    A = -(HL2 & 0xFF);
    
  ba6c:
    HL3 = state->mask_buffer_pointer;
    // R I:C Iterations (inner loop);
    iters2 = self_BA70; // self modified
    do
    {
      uint8_t B2;
      uint8_t Adash;
      
      B2 = self_BA72; // self modified
      do
      {
        Adash = *DE;
        
        //if (!even_parity(Adash))
        {
          Adash &= 0x7F;
          
          DE++;
          A = *DE;
        }
        
        if (A != 0)
          mask_against_tile(A, HL3); // index, dst
        
        HL3++;
        
        // EX AF,AF' // unpaired?
        
        if (A != 0 && --A != 0)
          DE--;
        DE++;
      }
      while (--B2);
      
      // PUSH BC
      
      B2 = self_BA90; // self modified
      if (B2)
      {
        if (A)
          goto baa3;
        
      ba9b:
        do
        {
          A = *DE;
          //if (!even_parity(A))
          {
            A &= 0x7F;
            DE++;
            
          baa3:
            B2 -= A;
            if (B2 < 0)
              goto bab6;
            
            DE++;
            //if (!Z)
            goto ba9b;
            
            // EX AF,AF' // why not just jump instr earlier? // bank
            goto bab9;
          }
          
          DE++;
        }
        while (--B2);
        
        A = 0;
        // EX AF,AF' // why not just jump instr earlier? // bank
        
        goto bab9;
        
      bab6:
        A = -A;
        // EX AF,AF' // bank
      }
      
    bab9:
      HL3 += self_BABA; // self modified
      // EX AF,AF'  // unbank
      // POP BC
    }
    while (--iters2);
    
  pop_next:
    // POP HLeb
    // POP BC
    peightbyte++; // stride is 1 eightbyte_t
  }
  while (--iters);

  // This scales A by B then returns nothing: is it really doing this for no good reason?
  HL2 = scale_val(state, A, 8);
}

/**
 * $BACD: Given a bitmask in A, produce a widened and shifted 16-bit bitmask.
 *
 * If A is 00000001 and E is 8 => 00000000 00001000
 * If A is 10000000 and E is 8 => 00000100 00000000
 * If A is 00001111 and E is 8 => 00000000 01111000
 * If A is 01010101 and E is 8 => 00000010 10101000
 *
 * \param[in] state Pointer to game state.
 * \param[in] value Bitmask of ? // was A
 * \param[in] shift Some sort of shift (added at each iteration). Observed: 4, 8 // was E
 *
 * \return Widened value.
 */
uint16_t scale_val(tgestate_t *state, uint8_t value, uint8_t shift)
{
  uint8_t  iters;  // was B
  uint16_t result; // was HL

  iters = 8; /* iterations */
  result = 0;
  do
  {
    int carry;

    result <<= 1; // double, or shift, what's the intent here? make space
    carry = value >> 7; // shift out of high end
    value <<= 1; // was RRA
    if (carry)
      result += shift; // shift into low end
  }
  while (--iters);

  return result;
}

/* ----------------------------------------------------------------------- */

/**
 * $BADC: Masks tiles (placed in a stride-of-4 arrangement) by the specified
 * index exterior_tiles (set zero).
 *
 * (Leaf.)
 *
 * \param[in] index Mask tile index. // tileindex_t? (was A)
 * \param[in] dst   Pointer to source/destination. (was HL)
 */
void mask_against_tile(uint8_t index, uint8_t *dst)
{
  const tilerow_t *row; // tile_t *t; or tilerow?
  uint8_t          B;

  row = &exterior_tiles_0[index].row[0];
  B = 8; /* iterations */
  do
  {
    *dst &= *row++;
    dst += 4; /* stride */
  }
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $BAF7: (unknown) Looks like it's clipping sprites to the game window.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 * \param[out] pBC    Returned BC.
 * \param[out] pDE    Returned DE.
 *
 * \return 0 or 0xFF (perhaps visible / invisible).
 */
int vischar_visible(tgestate_t *state, vischar_t *vischar, uint16_t *pBC, uint16_t *pDE)
{
  uint8_t *HL;
  uint8_t  A;
  uint16_t BC;
  uint16_t DE;

  HL = &state->map_position_related_1;
  A = state->map_position[0] + 24 - *HL;
  if (A > 0)
  {
#if 0
    // CP vischar->width_bytes  // if (A < vischar->width_bytes)  (i think)
    if (carry)
    {
      BC = A;
    }
    else
    {
      A = *HL + vischar->width_bytes - state->map_position[0]; // width_bytes == width in bytes
      if (A <= 0)
        goto exit;
      // CP vischar->width_bytes  // if (A < vischar->width_bytes)  (i think)
      if (carry)
      {
        C = A;
        B = -A + vischar->width_bytes;
      }
      else
      {
        BC = vischar->width_bytes;
      }
    }

    HL = ((state->map_position >> 8) + 17) * 8;
    DE = vischar->w1A; // fused
    A &= A; // likely: clear carry
    HL -= DE;
    if (result <= 0)  // HL < DE
      goto exit;
    if (H)
      goto exit;
    A = L;
    // CP vischar->height
    if (carry)
    {
      DE = A;
    }
    else
    {
      HL = vischar->height + DE; // height == height in rows
      DE = (state->map_position >> 8) * 8;
      A &= A; // likely: clear carry
      HL -= DE;
      if (result <= 0)  // HL < DE
        goto exit;
      if (H)
        goto exit;
      A = L;
      // CP vischar->height
      if (carry)
      {
        E = A;
        D = -A + vischar->height;
      }
      else
      {
        DE = vischar->height;
      }
    }
#endif

    *pBC = BC;
    *pDE = DE;

    return 0; // return Z
  }

exit:
  return 0xFF; // return NZ
}

/* ----------------------------------------------------------------------- */

/**
 * $BB98: Called from main loop 3.
 *
 * Plots tiles.
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

  iters = vischars_LENGTH; /* iterations */
  vischar = &state->vischars[0];
  do
  {
    uint16_t BC;
    uint16_t DE;

    // PUSH BC
    if (vischar->flags == 0)
      goto next;

    state->map_position_related_2 = vischar->w1A >> 3; // divide by 8
    state->map_position_related_1 = vischar->w18 >> 3; // divide by 8

    if (vischar_visible(state, vischar, &BC, &DE) == 0xFF)
      goto next; // possibly the not visible case

    A = (((DE & 0xFF) >> 3) & 31) + 2;
    saved_A = A;
    A += state->map_position_related_2 - state->map_position[1];
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
    B = BC >> 8;
    C = BC & 0xFF;
    D = DE >> 8;
    E = DE & 0xFF;

    self_BC5F = A; // self modify // outer loop counter

    self_BC61 = C; // self modify // inner loop counter
    self_BC89 = C; // self modify

    A = 24 - C;
    self_BC8E = A; // self modify // DE stride

    A += 7 * 24;
    self_BC95 = A; // self modify // HL stride

    uint8_t *HL;
    int carry, carry_out;

    HL = &state->map_position[0];

    C = BC & 0xFF;

    if (B == 0)
      B = state->map_position_related_1 - HL[0];
    else
      B = 0; // was interleaved

    if (D == 0) // (yes, D)
      C = state->map_position_related_2 - HL[1];
    else
      C = 0; // was interleaved

    H = C;
    A = 0; // can push forward to reduce later ops
    carry = H & 1; H >>= 1; // SRL H
    carry_out = A & 1; A = (A >> 1) | (carry << 7); carry = carry_out; // RRA
    // A = carry << 7; carry = 0; // with A = 0
    E = A;

    D = H;
    carry = H & 1; H >>= 1; // SRL H
    carry_out = A & 1; A = (A >> 1) | (carry << 7); carry = carry_out; // RRA
    L = A;

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
        uint8_t Bdash;

        // PUSH HL
        A = *DE;

        // POP DEdash // visible tiles array pointer
        // PUSH HLdash
        select_tile_set(state, HLdash); // call using banked registers
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
 * \param[in] state Pointer to game state.
 * \param[in] H     ?
 * \param[in] L     ?
 */
const tile_t *select_tile_set(tgestate_t *state, uint8_t H, uint8_t L)
{
  const tile_t *tileset; /* was BC */

  if (state->room_index)
  {
    tileset = &interior_tiles[0];
  }
  else
  {
    uint8_t   pos;  /* was Adash */
    supertileindex_t tile; /* was Adash */

    /* Convert map position to an index into 7x5 supertile refs array. */
    pos = ((state->map_position[1] & 3) + L) >> 2;
    L = (pos & 0x3F) * 7;
    pos = ((state->map_position[0] & 3) + H) >> 2;
    pos = (pos & 0x3F) + L;

    tile = state->map_buf[pos]; /* 7x5 supertile refs */
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
void called_from_main_loop_7(tgestate_t *state)
{
  characterstruct_t *HL;
  uint8_t            B;
  room_t             A;
  uint8_t            H, L;
  uint8_t            D, E;
  uint8_t            C;

  /* Form a map position in DE. */
  H = state->map_position[1];
  L = state->map_position[0];
  E = (L < 8) ? 0 : L;
  D = (H < 8) ? 0 : H;

  /* Walk all character structs. */
  HL = &state->character_structs[0];
  B  = 26; // the 26 'real' characters
  do
  {
    if ((HL->character & characterstruct_BYTE0_BIT6) == 0)
    {
      A = state->room_index;
      if (A == HL->room)
      {
        if (A == room_0_OUTDOORS)
        {
          /* Outdoors. */

          C = 0 - HL->pos.y - HL->pos.x - HL->pos.vo;
          if (C <= D)
            goto skip; // check
          D += 32;
          if (D > 0xFF)
            D = 0xFF;
          if (C > D)
            goto skip; // check

          HL->pos.x += 64;
          HL->pos.y -= 64;

          C = 128;
          if (C <= E)
            goto skip; // check
          E += 40;
          if (E > 0xFF)
            E = 0xFF;
          if (C > E)
            goto skip; // check
        }

        spawn_characters(state, HL);
      }
    }

skip:
    HL++;
  }
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $C47E: Called from main loop 6.
 *
 * Run through all visible characters, resetting them.
 *
 * \param[in] state Pointer to game state.
 */
void called_from_main_loop_6(tgestate_t *state)
{
  uint8_t    D, E;
  uint8_t    B;
  vischar_t *HL;
  uint16_t   CA;
  uint8_t    C, A;

  E = MAX(state->map_position[0] - 9, 0);
  D = MAX(state->map_position[1] - 9, 0);

  B  = vischars_LENGTH - 1; /* iterations */
  HL = &state->vischars[1]; /* iterate over non-player characters */
  do
  {
    if (HL->character == character_NONE)
      goto next;

    if (state->room_index != HL->room)
      goto reset; /* character not in room */

    CA = HL->w1A;
    C  = CA >> 8;
    A  = CA & 0xFF;
    divide_by_8_with_rounding(&C, &A);
    C = A;
    if (C <= D || C > MIN(D + 34, 255))
      goto reset;

    CA = HL->w18;
    C  = CA >> 8;
    A  = CA & 0xFF;
    divide_by_8(&C, &A);
    C = A;
    if (C <= E || C > MIN(E + 42, 255))
      goto reset;

    goto next;

reset:
    reset_visible_character(state, HL);

next:
    HL++;
  }
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $C4E0: Add characters to the visible character list.
 *
 * \param[in] state Pointer to game state.
 * \param[in] HL    Pointer to character to spawn.
 *
 * \return NZ if not? spawned.
 */
int spawn_characters(tgestate_t *state, characterstruct_t *HL)
{
  static const uint8_t _cf06[] = { 0x01,0x04,0x04,0xFF,0x00,0x00,0x00,0x0A };
  static const uint8_t _cf0e[] = { 0x01,0x05,0x05,0xFF,0x00,0x00,0x00,0x8A };
  static const uint8_t _cf16[] = { 0x01,0x06,0x06,0xFF,0x00,0x00,0x00,0x88 };
  static const uint8_t _cf1e[] = { 0x01,0x07,0x07,0xFF,0x00,0x00,0x00,0x08 };
  static const uint8_t _cf26[] = { 0x04,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x01,0x02,0x00,0x00,0x02,0x02,0x00,0x00,0x03 };
  static const uint8_t _cf3a[] = { 0x04,0x01,0x01,0x03,0x00,0x02,0x00,0x80,0x00,0x02,0x00,0x81,0x00,0x02,0x00,0x82,0x00,0x02,0x00,0x83 };
  static const uint8_t _cf4e[] = { 0x04,0x02,0x02,0x00,0xFE,0x00,0x00,0x04,0xFE,0x00,0x00,0x05,0xFE,0x00,0x00,0x06,0xFE,0x00,0x00,0x07 };
  static const uint8_t _cf62[] = { 0x04,0x03,0x03,0x01,0x00,0xFE,0x00,0x84,0x00,0xFE,0x00,0x85,0x00,0xFE,0x00,0x86,0x00,0xFE,0x00,0x87 };
  static const uint8_t _cf76[] = { 0x01,0x00,0x00,0xFF,0x00,0x00,0x00,0x00 };
  static const uint8_t _cf7e[] = { 0x01,0x01,0x01,0xFF,0x00,0x00,0x00,0x80 };
  static const uint8_t _cf86[] = { 0x01,0x02,0x02,0xFF,0x00,0x00,0x00,0x04 };
  static const uint8_t _cf8e[] = { 0x01,0x03,0x03,0xFF,0x00,0x00,0x00,0x84 };
  static const uint8_t _cf96[] = { 0x02,0x00,0x01,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80 };
  static const uint8_t _cfa2[] = { 0x02,0x01,0x02,0xFF,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x04 };
  static const uint8_t _cfae[] = { 0x02,0x02,0x03,0xFF,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x84 };
  static const uint8_t _cfba[] = { 0x02,0x03,0x00,0xFF,0x00,0x00,0x00,0x84,0x00,0x00,0x00,0x00 };
  static const uint8_t _cfc6[] = { 0x02,0x04,0x04,0x02,0x02,0x00,0x00,0x0A,0x02,0x00,0x00,0x0B };
  static const uint8_t _cfd2[] = { 0x02,0x05,0x05,0x03,0x00,0x02,0x00,0x8A,0x00,0x02,0x00,0x8B };
  static const uint8_t _cfde[] = { 0x02,0x06,0x06,0x00,0xFE,0x00,0x00,0x88,0xFE,0x00,0x00,0x89 };
  static const uint8_t _cfea[] = { 0x02,0x07,0x07,0x01,0x00,0xFE,0x00,0x08,0x00,0xFE,0x00,0x09 };
  static const uint8_t _cff6[] = { 0x02,0x04,0x05,0xFF,0x00,0x00,0x00,0x0A,0x00,0x00,0x00,0x8A };
  static const uint8_t _d002[] = { 0x02,0x05,0x06,0xFF,0x00,0x00,0x00,0x8A,0x00,0x00,0x00,0x88 };
  static const uint8_t _d00e[] = { 0x02,0x06,0x07,0xFF,0x00,0x00,0x00,0x88,0x00,0x00,0x00,0x08 };
  static const uint8_t _d01a[] = { 0x02,0x07,0x04,0xFF,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x0A };

  /**
   * $CD9A: (unknown)
   */
  static const uint8_t *character_related_pointers[24] =
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

  /**
   * $CD9A: Maps characters to sprite sets (maybe).
   */
  static const character_meta_data_t character_meta_data[4] =
  {
    { &character_related_pointers[0], &sprites[sprite_COMMANDANT_FACING_TOP_LEFT_4] },
    { &character_related_pointers[0], &sprites[sprite_GUARD_FACING_TOP_LEFT_4] },
    { &character_related_pointers[0], &sprites[sprite_DOG_FACING_TOP_LEFT_1] },
    { &character_related_pointers[0], &sprites[sprite_PRISONER_FACING_TOP_LEFT_4] },
  };

  vischar_t         *pvc;
  uint8_t            B;
  characterstruct_t *DE;
  vischar_t         *vischar; /* was IY */
  pos_t             *saved_pos;
  //uint8_t            A;

  if (HL->character & characterstruct_BYTE0_BIT6)
    return 1; // NZ

  /* Find an empty slot. */
  pvc = &state->vischars[1];
  B = vischars_LENGTH - 1;
  do
  {
    if (pvc->character == vischar_BYTE0_EMPTY_SLOT)
      goto found; /* Empty slot found. */
    pvc++;
  }
  while (--B);

  return 0; // check


found:

  DE = HL;
  vischar = pvc;

  /* Scale coords dependent on which room the character is in. */
  saved_pos = &state->saved_pos;
  if (DE->room == 0)
  {
    /* Original code was not unrolled. */
    saved_pos->y  = DE->pos.y  * 8;
    saved_pos->x  = DE->pos.x  * 8;
    saved_pos->vo = DE->pos.vo * 8;
  }
  else
  {
    saved_pos->y  = DE->pos.y;
    saved_pos->x  = DE->pos.x;
    saved_pos->vo = DE->pos.vo;
  }

  sub_AFDF(state, vischar);
#if 0
  if (Z)
    bounds_check(state);

  // POP DE
  // POP HL (pvc)

  if (NZ)
    return;

  A = DE->flags | characterstruct_BYTE0_BIT6;
  DE->flags = A;

  A &= characterstruct_BYTE0_MASK;
  HL->character = A;
  HL->flags     = 0;

  // PUSH DE

  DE = &character_meta_data[0]; // commandant
  if (A == 0) goto selected;
  DE = &character_meta_data[1]; // guard
  if (A < 16) goto selected;
  DE = &character_meta_data[2]; // dog
  if (A < 20) goto selected;
  DE = &character_meta_data[3]; // prisoner

selected:
  EX DE, HL

  DE += 7;
  *DE++ = *HL++;
  *DE++ = *HL++;
  DE += 11;
  *DE++ = *HL++;
  *DE++ = *HL++;
  DE -= 8;
  memcpy(DE, &saved_Y, 6);
  // POP HL
  HL += 5;
  DE += 7;

  A = state->room_index;
  *DE = A;
  if (A)
  {
    play_speaker(sound_CHARACTER_ENTERS_2);
    play_speaker(sound_CHARACTER_ENTERS_1);
  }

  DE -= 26;
  *DE++ = *HL++;
  *DE++ = *HL++;
  HL -= 2;

c592:
  if (*HL == 0)
  {
    DE += 3;
  }
  else
  {
    state->byte_A13E = 0;
    // PUSH DE
    sub_C651(state, HL);
    if (A == 255)
    {
      // POP HL
      HL -= 2; // point to ->target?
      // PUSH HL
      sub_CB2D(state, vischar, HL);
      // POP HL
      DE = HL + 2;
      goto c592;
    }
    else if (A == 128)
    {
      vischar->flags |= vischar_BYTE1_BIT6;
    }
    // POP DE
    memcpy(DE, HL, 3);
  }
  *DE = 0;
  DE -= 7;
  // EX DE,HL
  // PUSH HL
  reset_position(state, vischar);
  // POP HL
  character_behaviour(state, vischar);
  return; // exit via

#endif

  return 1;
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
  character_t        character; /* (was A) */
  pos_t             *pos;       /* (was DE) */
  characterstruct_t *DE;        /* (was DE) */
  room_t             room;

  character = vischar->character;
  if (character == character_NONE)
    return;

  if (character >= character_26)
  {
    /* A stove/crate character. */

    vischar->character = character_NONE;
    vischar->flags     = 0xFF; /* flags */
    vischar->b07       = 0;    /* more flags */

    /* Save the old position. */
    if (character == character_26)
      pos = &state->movable_items[movable_item_STOVE1].pos;
    else if (character == character_27)
      pos = &state->movable_items[movable_item_STOVE2].pos;
    else
      pos = &state->movable_items[movable_item_CRATE].pos;
    memcpy(pos, &vischar->mi.pos, sizeof(*pos));
  }
  else
  {
    pos_t     *vispos_in;
    tinypos_t *charpos_out;

    /* A non-object character. */

    DE = get_character_struct(state, character);
    DE->character &= ~characterstruct_BYTE0_BIT6;

    room = vischar->room;
    DE->room = room;

    vischar->b07 = 0; /* more flags */

    /* Save the old position. */

    vispos_in = &vischar->mi.pos;
    charpos_out = &DE->pos; // byte-wide form of pos struct

    // set y,x,vo
    if (room == room_0_OUTDOORS)
    {
      /* Outdoors. */
      scale_pos(vispos_in, charpos_out);
    }
    else
    {
      /* Indoors. */
      /* Unrolled from original code. */
      charpos_out->y  = vispos_in->y;
      charpos_out->x  = vispos_in->x;
      charpos_out->vo = vispos_in->vo;
    }

    //character = vischar->character; // BUT we already have this from earlier
    vischar->character = character_NONE;
    vischar->flags     = 0xFF;

    /* Guard dogs? */
    if (character >= character_16 && character < character_20)
    {
      vischar->target = 0x00FF; // TODO: Double check endianness.
      if (character >= character_18) /* 18 and 19 */
        vischar->target = 0x18FF; // TODO: Double check.
    }

    // set target (suspected)
    DE->target = vischar->target;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $C651: (unknown)
 *
 * \param[in] state Pointer to game state.
 * \param[in] HL    Pointer to characterstruct + 5 [or others]. (target field(s))
 *
 * \return 0/128/255. // FIXME: HL must be returned too
 */
uint8_t sub_C651(tgestate_t *state, location_t *HL)
{
  /**
   * $783A: (unknown) Looks like an array of locations...
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
    0x3D3D,
    0x673D,
    0x4C74,
    0x2A2C,
    0x486A,
    0x486E,
    0x6851,
    0x3C34,
    0x2C34,
    0x1C34,
    0x6B77,
    0x6E7A,
    0x1C34,
    0x3C28,
    0x2224,
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
    0x3634,
    0x2E34,
    0x2434,
    0x3E34,
    0x3820,
    0x1834,
    0x2E2A,
    0x2222,
    0x6E78,
    0x6E76,
    0x6E74,
    0x6D79,
    0x6D77,
    0x6D75,
  };

  uint8_t A;

  A = *HL & 0x00FF; // read low byte only
  if (A == 0xFF)
  {
    uint8_t A1, A2;

    // In original code, *HL is used as a temporary.

    A1 = ((*HL & 0xFF00) >> 8) & characterstruct_BYTE6_MASK_HI; // read high byte only
    A2 = fetch_C41A(state) & characterstruct_BYTE6_MASK_LO;
    *HL = (*HL & 0x00FF) | ((A1 | A2) << 8); // write high byte only
  }
  else
  {
#if 0
    // PUSH HL
    C = *++HL; // byte6 / t2  C = (*HL & 0xFF00) >> 8;
    DE = element_A_of_table_7738(A);
    H = 0;
    A = C;
    if (A == 0xFF) // if high byte is 0xFF
      H--; // H = 0 - 1 => 0xFF
    L = A;
    HL += DE;
    // EX DE,HL
    A = *DE;
    // POP HL // was interleaved
    if (A == 0xFF)
      goto return_255;

    if ((A & 0x7F) < 40)
    {
      A = *DE;
      if (*HL & (1 << 7))
        A ^= 0x80; // 762C, 8002, 7672, 7679, 7680, 76A3, 76AA, 76B1, 76B8, 76BF, ... looks quite general
      transition(state, IY, HL /*LOCATION*/); // IY has to come from somewhere!
      HL++;
      A = 0x80;
      return;
    }
    A = *DE - 40;
#endif
  }

  // sample A=$38,2D,02,06,1E,20,21,3C,23,2B,3A,0B,2D,04,03,1C,1B,21,3C,...
  HL = &word_783A[A];

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
  characterstruct_t *HL;
  uint8_t            C;

  state->byte_A13E = 0xFF;

  state->character_index = (state->character_index + 1) % character_26; // 26 = highest + 1 character

  HL = get_character_struct(state, state->character_index);
  if (HL->character & characterstruct_BYTE0_BIT6)
    return;

#if 0
  // PUSH HL
  A = *++HL; // characterstruct byte1 == room
  if (A != room_0_OUTDOORS)
  {
    if (is_item_discoverable_interior(state, A, &C) == 0)
      item_discovered(state, C);
  }
  // POP HL
  HL += 2; // point at characterstruct y,x coords
  // PUSH HL
  HL += 3; // point at characterstruct byte2
  A = *HL;
  if (A == 0)
  {
    // POP HL
    return;
  }

  A = sub_C651(state, HL);
  if (A == 0xFF)
  {
    A = character_index;
    if (A != character_0)
    {
      // Not a player character.
      if (A >= character_12_prisoner)
        goto char_ge_12;
      // Characters 1..11.

back:
      *HL++ ^= 0x80;
      if (A & 7)(*HL) -= 2;
      (*HL)++; // weird // i.e -1 or +1
      // POP HL
      return;
    }

    // Player character.
char_is_zero:
    A = *HL & characterstruct_BYTE5_MASK; // fetching a character index? // sampled = HL = $7617 (characterstruct + 5) // location
    if (A != 36)
      goto back;

char_ge_12:
    // POP DE
    goto character_event; // exit via

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
      increment_DE_by_diff(A, B, HL, DE);
      DE++;
      HL++;
      increment_DE_by_diff(A, B, HL, DE);
      // POP HL
      if (B != 2)
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
        B = 3;
        do
          *DE++ = *HL++ >> 1;
        while (--B);
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
      increment_DE_by_diff(state, Adash, B, HL, DE);
      HL++;
      DE++;
      increment_DE_by_diff(state, Adash, B, HL, DE);
      DE++;
      if (B != 2)
        return;
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
  }
#endif
}

/* ----------------------------------------------------------------------- */

/**
 * $C79A: (unknown)
 *
 * Leaf.
 *
 * \param[in] state Pointer to game state.
 * \param[in] max   A maximum value. (was A')
 * \param[in] B     Incremented if no movement.
 * \param[in] HL    Pointer to second value.
 * \param[in] DE    Pointer to first value.
 *
 * \return B
 */
int increment_DE_by_diff(tgestate_t *state,
                         uint8_t     max,
                         int         B,
                         uint8_t    *HL,
                         uint8_t    *DE)
{
  int8_t delta; /* was A */

  delta = *DE - *HL;
  if (delta == 0)
  {
    B++;
  }
  else if (delta < 0)
  {
    delta = -delta;
    if (delta >= max)
      delta = max;
    *DE += delta;
  }
  else
  {
    if (delta >= max)
      delta = max;
    *DE -= delta;
  }

  return B;
}

/* ----------------------------------------------------------------------- */

/**
 * $C7B9: Get character struct.
 *
 * \param[in] index Character index.
 *
 * \return Pointer to characterstruct.
 */
characterstruct_t *get_character_struct(tgestate_t *state, character_t index)
{
  return &state->character_structs[index];
}

/* ----------------------------------------------------------------------- */

/**
 * $C7C6: Character event.
 *
 * \param[in] state Pointer to game state.
 * \param[in] loc   Pointer to location (was HL). [ NEEDS TO BE CHARSTRUCT+5 *OR* VISCHAR+2   // likely a location_t and not a characterstruct_t ]
 */
void character_event(tgestate_t *state, location_t *loc)
{
  /* I strongly suspect that I've misinterpreted this code and gotten it all
   * wrong. */

#if 0

  /* $C7F9 */
  static const charactereventmap_t eventmap[] =
  {
    { character_6  | 0xA0,  0 },
    { character_7  | 0xA0,  0 },
    { character_8  | 0xA0,  1 },
    { character_9  | 0xA0,  1 },
    { character_5  | 0x00,  0 },
    { character_6  | 0x00,  1 },
    { character_5  | 0x80,  3 },
    { character_6  | 0x80,  3 },
    { character_14 | 0x00,  2 },
    { character_15 | 0x00,  2 },
    { character_14 | 0x80,  0 },
    { character_15 | 0x80,  1 },
    { character_16 | 0x00,  5 },
    { character_17 | 0x00,  5 },
    { character_16 | 0x80,  0 },
    { character_17 | 0x80,  1 },
    { character_0  | 0xA0,  0 },
    { character_1  | 0xA0,  1 },
    { character_10 | 0x20,  7 },
    { character_12 | 0x20,  8 }, /* sleeps */
    { character_11 | 0x20,  9 }, /* sits */
    { character_4  | 0xA0,  6 },
    { character_4  | 0x20, 10 }, /* released from solitary */
    { character_5  | 0x20,  4 }  /* morale related */
  };

  /* $C829 */
  static charevnt_handler_t *const handlers[] =
  {
    &charevnt_handler_1,
    &charevnt_handler_2,
    &charevnt_handler_3_check_var_A13E,
    &charevnt_handler_4_zero_morale_1,
    &charevnt_handler_5_check_var_A13E_anotherone,
    &charevnt_handler_6,
    &charevnt_handler_7,
    &charevnt_handler_8_player_sleeps,
    &charevnt_handler_9_player_sits,
    &charevnt_handler_10_player_released_from_solitary
  };

  uint8_t                    A;
  const charactereventmap_t *HL;
  uint8_t                    B;

  A = charstr->character;
  if (A >= character_7  && A <= character_12)
  {
    character_sleeps(state, A, charstr);
    return;
  }
  if (A >= character_18 && A <= character_22)
  {
    character_sits(state, A, charstr);
    return;
  }

  HL = &eventmap[0];
  B = NELEMS(eventmap);
  do
  {
    if (A == HL->character_and_flag)
    {
      handlers[HL->handler].handler(state, &charstr->character);
      return;
    }
    HL++;
  }
  while (--B);

  charstr->character = 0; // no action;
#endif
}

/**
 * $C83F:
 */
void charevnt_handler_4_zero_morale_1(tgestate_t  *state,
                                      character_t *charptr,
                                      vischar_t   *vischar)
{
  state->morale_1 = 0;
  charevnt_handler_0(state, charptr, vischar);
}

/**
 * $C845:
 */
void charevnt_handler_6(tgestate_t  *state,
                        character_t *charptr,
                        vischar_t   *vischar)
{
  // POP charptr // (popped) sampled charptr = $80C2 (x2), $8042  // likely target location
  *charptr++ = 0x03;
  *charptr   = 0x15;
}

/**
 * $C84C:
 */
void charevnt_handler_10_player_released_from_solitary(tgestate_t  *state,
                                                       character_t *charptr,
                                                       vischar_t   *vischar)
{
  // POP charptr
  *charptr++ = 0xA4;
  *charptr   = 0x03;
  state->automatic_player_counter = 0; // force automatic control
  set_player_target_location(state, 0x2500); // original jump was $A344, but have moved it
}

/**
 * $C85C:
 */
void charevnt_handler_1(tgestate_t  *state,
                        character_t *charptr,
                        vischar_t   *vischar)
{
  uint8_t C;

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

  C = 0x08; // 0xFF08 // sampled HL=$8022,$8042,$8002,$8062
  localexit(state, charptr, C);
}

void localexit(tgestate_t *state, character_t *charptr, uint8_t C)
{
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
  // POP charptr
  *charptr++ = 0x05;
  *charptr   = 0x00;
}

/**
 * $C889:
 */
void charevnt_handler_9_player_sits(tgestate_t  *state,
                                    character_t *charptr,
                                    vischar_t   *vischar)
{
  // POP HL
  player_sits(state);
}

/**
 * $C88D:
 */
void charevnt_handler_8_player_sleeps(tgestate_t  *state,
                                      character_t *charptr,
                                      vischar_t   *vischar)
{
  // POP HL
  player_sleeps(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $C892: Causes characters to follow the player if they're being suspicious.
 * Poisoned food handling.
 *
 * \param[in] state Pointer to game state.
 */
void follow_suspicious_player(tgestate_t *state)
{
  vischar_t *IY;
  uint8_t    B;

  state->byte_A13E = 0;

  if (state->bell)
    guards_persue_prisoners(state);

  if (state->food_discovered_counter != 0 &&
      --state->food_discovered_counter == 0)
  {
    /* De-poison the food. */
    state->item_structs[item_FOOD].item &= ~itemstruct_ITEM_FLAG_POISONED;
    item_discovered(state, item_FOOD);
  }

  IY = &state->vischars[1];
  B = vischars_LENGTH - 1;
  do
  {
    if (IY->flags != 0)
    {
      uint8_t A;

      A = IY->character & vischar_BYTE0_MASK; // character index
      if (A < 20)
      {
        is_item_discoverable(state);

        if (state->red_flag || state->automatic_player_counter > 0)
          guards_follow_suspicious_player(state, IY);

        if (A > 15)
        {
          // 16,17,18,19 // could these be the dogs?
          if (state->item_structs[item_FOOD].room & itemstruct_ROOM_FLAG_ITEM_NEARBY)
            IY->flags = vischar_BYTE1_PERSUE | vischar_BYTE1_BIT1; // dead dog?
        }
      }

      character_behaviour(state, IY);
    }
    IY++;
  }
  while (--B);

  if (!state->red_flag && (state->morale_1 || state->automatic_player_counter == 0))
    character_behaviour(state, &state->vischars[0]);
}

/* ----------------------------------------------------------------------- */

/**
 * $C918: Character behaviour stuff.
 *
 * \param[in] state Pointer to game state.
 * \param[in] IY    Pointer to visible character.
 */
void character_behaviour(tgestate_t *state, vischar_t *IY)
{
  uint8_t    A;
  uint8_t    B;
  vischar_t *HL;
  //uint16_t  *DEdash;
  //tinypos_t *HLdash;
  uint8_t    Cdash;
  uint8_t    log2scale;

  B = A = IY->b07; /* more flags */
  if (A & vischar_BYTE7_MASK)
  {
    IY->b07 = --B;
    return;
  }

  HL = IY; // no need for separate HL and IY now other code reduced
  A = HL->flags;
  if (A != 0)
  {
    if (A == vischar_BYTE1_PERSUE) // == 1
    {
a_1:
      HL->p04.y = state->player_map_position.y;
      HL->p04.x = state->player_map_position.x;
      goto jump_c9c0;
    }
    else if (A == vischar_BYTE1_BIT1) // == 2
    {
      if (state->automatic_player_counter)
        goto a_1;

      HL->flags = 0;
      sub_CB23(state, HL, &HL->target);
      return;
    }
    else if (A == (vischar_BYTE1_PERSUE + vischar_BYTE1_BIT1)) // == 3
    {
      if (state->item_structs[item_FOOD].room & itemstruct_ROOM_FLAG_ITEM_NEARBY)
      {
        HL->p04.y = state->item_structs[item_FOOD].pos.y;
        HL->p04.x = state->item_structs[item_FOOD].pos.x;
        goto jump_c9c0;
      }
      else
      {
        HL->flags  = 0;
        HL->target = 0x00FF;
        sub_CB23(state, HL, &HL->target);
        return;
      }
    }
    else if (A == vischar_BYTE1_BIT2) // == 4 // gone mad / bribe flag
    {
      character_t  Acharacter;
      vischar_t   *HLfoundvischar;

      Acharacter = state->bribed_character;
      if (Acharacter != character_NONE)
      {
        B = vischars_LENGTH - 1;
        HLfoundvischar = &state->vischars[1]; // iterate over non-player characters
        do
        {
          if (HLfoundvischar->character == Acharacter) /* Bug? This doesn't mask off HL->character. */
            goto found_bribed;
          HLfoundvischar++;
        }
        while (--B);
      }

      /* Bribed character was not visible. */
      HL->flags = 0;
      sub_CB23(state, HL, &HL->target);
      return;

found_bribed:
      {
        pos_t     *HLpos;
        tinypos_t *DEpos;

        HLpos = &HLfoundvischar->mi.pos;
        DEpos = &HL->p04;
        if (state->room_index > 0)
        {
          /* Indoors */
          scale_pos(HLpos, DEpos);
        }
        else
        {
          /* Outdoors */
          DEpos->y = HLpos->y;
          DEpos->x = HLpos->x;
        }
        goto jump_c9c0;
      }

    }
  }

  A = HL->target & 0x00FF; // byte load at $8002 $8022 $8042 $8062 $8082 $80A2
  if (A == 0)
  {
    character_behaviour_end_1(state, IY, A);
    return; // exit via
  }

jump_c9c0:
  Cdash = A = HL->flags; // sampled = $8081, 80a1, 80e1, 8001, 8021, ...

#if ORIGINAL
  /* Original code self modifies move_character_y/x routines. */
  if (state->room_index)
    HLdash = &multiply_by_1;
  else if (Cdash & vischar_BYTE1_BIT6)
    HLdash = &multiply_by_4;
  else
    HLdash = &multiply_by_8;

  self_CA13 = HLdash; // self-modify move_character_y:$CA13
  self_CA4B = HLdash; // self-modify move_character_x:$CA4B
#else
  /* Replacement code passes down a log2scale factor. */
  if (state->room_index)
    log2scale = 1;
  else if (Cdash & vischar_BYTE1_BIT6)
    log2scale = 4;
  else
    log2scale = 8;
#endif

  if (IY->b07 & vischar_BYTE7_BIT5)
  {
    character_behaviour_end_2(state, IY, A, log2scale);
    return;
  }

  if (move_character_y(state, IY, log2scale) == 0 &&
      move_character_x(state, IY, log2scale) == 0)
  {
    // something like: character couldn't move?

    bribes_solitary_food(state, IY);
    return;
  }

  character_behaviour_end_1(state, IY, A); /* was fallthrough */
}

/**
 * $C9F5: Unknown end part.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 * \param[in] A       Flags (of another character...)
 */
void character_behaviour_end_1(tgestate_t *state, vischar_t *vischar, uint8_t A)
{
  if (A != vischar->b0D)
    vischar->b0D = A | vischar_BYTE13_BIT7;
}

/**
 * $C9FF: Unknown end part.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] vischar   Pointer to visible character. (was IY)
 * \param[in] A         Flags (of another character...)
 * \param[in] log2scale Log2Scale factor (replaces self-modifying code in original).
 */
void character_behaviour_end_2(tgestate_t *state, vischar_t *vischar, uint8_t A, int log2scale)
{
  // is x,y the right order? or y,x?
  if (move_character_x(state, vischar, log2scale) == 0 &&
      move_character_y(state, vischar, log2scale) == 0)
  {
    character_behaviour_end_1(state, vischar, A); // likely: couldn't move, so .. do something
    return;
  }

  bribes_solitary_food(state, vischar);
}

/* ----------------------------------------------------------------------- */

/**
 * $CA11: Move character on the Y axis.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] vischar   Pointer to visible character.
 * \param[in] log2scale Log2Scale factor (replaces self-modifying code in
 *                      original).
 *
 * \return 8/4/0 .. meaning?
 */
uint8_t move_character_y(tgestate_t *state,
                         vischar_t  *vischar,
                         int         log2scale)
{
  uint16_t DE;
  uint8_t  D;
  uint8_t  E;

  /* I'm assuming (until proven otherwise) that HL and IY point into the same
   * vischar on entry. */

  DE = vischar->mi.pos.y - (vischar->p04.y << log2scale);
  if (DE)
  {
    D = (DE & 0xFF00) >> 8;
    E = (DE & 0x00FF) >> 0;

    if (DE > 0) /* +ve */
    {
      if (D != 0   || E >= 3)
      {
        return 8;
      }
    }
    else /* -ve */
    {
      if (D != 255 || E < 254)
      {
        return 4;
      }
    }
  }

  vischar->b07 |= vischar_BYTE7_BIT5;
  return 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $CA49: Move character on the X axis.
 *
 * Nearly identical to move_character_y.
 *
 * \param[in] state     Pointer to game state.
 * \param[in] vischar   Pointer to visible character.
 * \param[in] log2scale Log2Scale factor (replaces self-modifying code in
 *                      original).
 *
 * \return 5/7/0 .. meaning?
 */
uint8_t move_character_x(tgestate_t *state,
                         vischar_t  *vischar,
                         int         log2scale)
{
  uint16_t DE;
  uint8_t  D;
  uint8_t  E;

  /* I'm assuming (until proven otherwise) that HL and IY point into the same
   * vischar on entry. */

  DE = vischar->mi.pos.x - (vischar->p04.x << log2scale);
  if (DE)
  {
    D = (DE & 0xFF00) >> 8;
    E = (DE & 0x00FF) >> 0;

    if (DE > 0) /* +ve */
    {
      if (D != 0   || E >= 3)
      {
        return 5;
      }
    }
    else /* -ve */
    {
      if (D != 255 || E < 254)
      {
        return 7;
      }
    }
  }

  vischar->b07 &= ~vischar_BYTE7_BIT5;
  return 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $CA81: Bribes, solitary, food, 'character enters' sound.
 *
 * \param[in] state Pointer to game state.
 * \param[in] IY    Pointer to visible character.
 */
void bribes_solitary_food(tgestate_t *state, vischar_t *IY)
{
  uint8_t A;
  uint8_t C;

  // In the original code HL is IY + 4 on entry.
  // In this version we replace HL references with IY ones.

  A = IY->flags;
  C = A; // copy flags
  A &= vischar_BYTE1_MASK;
  if (A)
  {
    if (A == vischar_BYTE1_PERSUE)
    {
      if (IY->character == state->bribed_character)
      {
        use_bribe(state, IY);
        return; // exit via
      }
      else
      {
        solitary(state);
        return; // exit via
      }
    }
    else if (A == vischar_BYTE1_BIT1 || A == vischar_BYTE1_BIT2)
    {
      return;
    }

    if ((state->item_structs[item_FOOD].item & itemstruct_ITEM_FLAG_POISONED) == 0)
      A = 32; /* food is not poisoned */
    else
      A = 255; /* food is poisoned */
    state->food_discovered_counter = A;

    //HL -= 2; // points to IY->target
    //*HL = 0;
    IY->target &= ~0xFFu; // clear low byte

    character_behaviour_end_1(state, IY, A); // character_behaviour:$C9F5;
    return;
  }

#if 0
  if (C & vischar_BYTE1_BIT6)
  {
    //orig:C = *--HL; // 80a3, 8083, 8063, 8003 // likely target location
    //orig:A = *--HL; // 80a2 etc

    C = IY->target >> 8;
    A = IY->target & 0xFF;

    A = element_A_of_table_7738(A)[C];
    if ((IY->target & 0xFF) & vischar_BYTE2_BIT7) // orig:(*HL & vischar_BYTE2_BIT7)
      A ^= 0x80;

    // PUSH AF
    A = *HL++; // $8002, ...
    if (A & vischar_BYTE2_BIT7)
      (*HL) -= 2; // $8003, ... // likely target location
    (*HL)++; // likely target location
    // POP AF

    HL = get_door_position(A); // door related
    IY->room = (*HL >> 2) & 0x3F; // HL=$790E,$7962,$795E => door position thingy // 0x3F is door_positions[0] room mask shifted right 2
    A = *HL & doorposition_BYTE0_MASK_LO; // door position thingy, lowest two bits -- index?
    if (A < 2)
      HL += 5;
    else
      HL -= 3; // delta of 8 - related to door stride stuff?
    // PUSH HL
    HL = IY;
    if ((HL & 0x00FF) == 0) // replace with (HL == &state->vischars[0])
    {
      // player's vischar only
      HL->flags &= ~vischar_BYTE1_BIT6;
      sub_CB23(state, HL, &HL->target);
    }
    // POP HL
    transition(state, IY, HL = LOCATION);
    play_speaker(state, sound_CHARACTER_ENTERS_1);
    return;
  }

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

  sub_CB23(state, IY, HL); // was fallthrough
#endif
}

/**
 * $CB23: sub_CB23
 *
 * \param[in] state Pointer to game state.
 * \param[in] IY    Pointer to visible character.
 * \param[in] HL    seems to be an arg. (target loc ptr)
 */
void sub_CB23(tgestate_t *state, vischar_t *IY, location_t *HL) // needs vischar
{
  uint8_t A;

  // PUSH HL
  A = sub_C651(state, HL);
  if (A != 0xFF)
    sub_CB61(state, IY, HL, A);
  else
    // POP HL
    sub_CB2D(state, IY, HL); // was fallthrough
}

/**
 * $CB2D: sub_CB2D
 *
 * \param[in] state Pointer to game state.
 * \param[in] IY    Pointer to visible character.
 */
void sub_CB2D(tgestate_t *state, vischar_t *IY, location_t *HL)
{
  uint8_t A;

  if (HL != &state->vischars[0].target) // was (L != 0x02)
  {
    // if not player's vischar
    A = IY->character & vischar_BYTE0_MASK;
    if (A == 0) // character 0
    {
      A = *HL & vischar_BYTE2_MASK;
      if (A == 36)
        goto cb46; // character index
      A = 0; // defeat the next 'if' statement
    }
    if (A == 12) // character 2
      goto cb50;
  }

cb46:
  // arrive here if (character == 0 && (location low byte & 7F) == 36)
  // PUSH HL
  character_event(state, HL /* loc */);
  // POP HL
  A = *HL;
  if (A == 0)
    return; // A returned?

  sub_CB23(state, IY, HL); // re-enter
  return; // exit via

cb50:
  *HL = *HL ^ 0x80;
  HL++;
  if (A & (1 << 7)) // which flag is this?
    (*HL) -= 2;
  (*HL)++;
  HL--;

  A = 0; // returned?
}

// $CB5F,2 Unreferenced bytes.

/**
 * $CB61: sub_CB61
 *
 * \param[in] state Pointer to game state.
 * \param[in] IY    Pointer to visible character.
 * \param[in] A     ...
 */
void sub_CB61(tgestate_t *state, vischar_t *IY, location_t *HL, uint8_t A)
{
  if (A == 128)
    IY->flags |= vischar_BYTE1_BIT6;
#if 0
  // POP DE // old HL stored in sub_CB23?
  memcpy(DE + 2, HL, 2); // replace with straight assignment
#endif
  A = 128; // returned?
}

/* ----------------------------------------------------------------------- */

/**
 * $CB75: BC becomes A.
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
    &data_77D8[0], /* dupe */
    &data_77DA[0], /* dupe */
    &data_77DC[0], /* dupe */
    &data_77DE[0],
    &data_77E1[0],
    &data_77E1[0], /* dupe */
    &data_77E7[0],
    &data_77EC[0],
    &data_77F1[0],
    &data_77F3[0],
    &data_77F5[0],
    &data_77F1[0], /* dupe */
    &data_77F3[0], /* dupe */
    &data_77F5[0], /* dupe */
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
 * $CB85: Return whatever word_C41A indexes ANDed with 0x0F.
 *
 * (Leaf.) Only ever used by sub_C651.
 *
 * \return Whatever it is.
 */
uint8_t fetch_C41A(tgestate_t *state)
{
  uint8_t *HL;
  uint8_t  A;

  HL = state->word_C41A + 1;
  A = *HL & 0x0F;
  state->word_C41A = HL;

  return A;
}

/* ----------------------------------------------------------------------- */

/**
 * $CB98: Player has been sent to solitary.
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
  static const uint8_t solitary_player_reset_data[6] =
  {
    0x00, 0x74, 0x64, 0x03, 0x24, 0x00
  };

  item_t       *HLpitem;
  item_t        C;
  uint8_t       B;
  vischar_t    *IY;
  itemstruct_t *HLpitemstruct;

  /* Silence bell. */
  state->bell = bell_STOP;

  /* Seize player's held items. */
  HLpitem = &state->items_held[0];
  C = *HLpitem;
  *HLpitem = item_NONE;
  item_discovered(state, C);

  HLpitem = &state->items_held[1];
  C = *HLpitem;
  *HLpitem = item_NONE;
  item_discovered(state, C);

  draw_all_items(state);

  /* Discover all items. */
  B = item__LIMIT;
  HLpitemstruct = &state->item_structs[0];
  do
  {
    if ((HLpitemstruct->room & itemstruct_ROOM_MASK) == room_0_OUTDOORS)
    {
      item_t     A;
      tinypos_t *HLtinypos;
      uint8_t    Adash;

      A         = HLpitemstruct->item;
      HLtinypos = &HLpitemstruct->pos;
      Adash = 0; /* iterate 0,1,2 */
      do
      {
        if (in_numbered_area(state, Adash, HLtinypos) == 0)
          goto discovered;
      }
      while (++Adash != 3);

      goto next;

discovered:
      item_discovered(state, A);
    }

next:
    HLpitemstruct++;
  }
  while (--B);

  /* Move character to solitary. */
  state->vischars[0].room = room_23_SOLITARY;
  state->current_door = 20;

  decrease_morale(state, 35);

  reset_map_and_characters(state);

  memcpy(&state->character_structs[0].room, &solitary_player_reset_data, 6);

  queue_message_for_display(state, message_YOU_ARE_IN_SOLITARY);
  queue_message_for_display(state, message_WAIT_FOR_RELEASE);
  queue_message_for_display(state, message_ANOTHER_DAY_DAWNS);

  state->morale_1 = 0xFF; /* inhibit user input */
  state->automatic_player_counter = 0; /* immediately take automatic control of player */
  state->vischars[0].mi.spriteset = &sprites[sprite_PRISONER_FACING_TOP_LEFT_4]; // $8015 = sprite_prisoner_tl_4;
  IY = &state->vischars[0];
  IY->b0E = 3; // character faces bottom left
  state->vischars[0].target &= 0xFF00; // ($8002) = 0; // target location - why is this storing a byte and not a word?
  transition(state, IY, &solitary_pos);

  // will this ever return?
}

/* ----------------------------------------------------------------------- */

/**
 * $CC37: Guards follow suspicious player.
 *
 * \param[in] state Pointer to game state.
 * \param[in] IY    Pointer to visible character.
 */
void guards_follow_suspicious_player(tgestate_t *state, vischar_t *IY)
{
  vischar_t   *HL;
  character_t  A;
  tinypos_t   *DE;
  pos_t       *HLpos;

  HL = IY;
  A = HL->character;

  /* Don't follow non-players dressed as guards. */
  if (A != character_0 &&
      state->vischars[0].mi.spriteset == &sprites[sprite_GUARD_FACING_TOP_LEFT_4])
    return;

  if (HL->flags == vischar_BYTE1_BIT2) /* 'Gone mad' (bribe) flag */
    return;

  HLpos = &HL->mi.pos; /* was HL += 15; */
  DE = &state->tinypos_81B2;
  if (state->room_index == 0)
  {
    scale_pos(HLpos, DE);
#if 0
    HL = &player_map_position.y;
    DE = &state->tinypos_81B2.y;
    A = IY->b0E;
    carry = A & 1; A >>= 1;
    C = A;
    if (!carry)
    {
      HL++;
      DE++;
      // range check
      A = *DE - 1;
      if (A >= *HL || A + 2 < *HL) // *DE - 1 .. *DE + 1
        return;
      HL--;
      DE--;
      A = *DE;
      // CP *HL  // TRICKY!
      // BIT 0,C // if ((C & (1<<0)) == 0) carry = !carry;
      // RET C   // This is odd: CCF then RET C? // will need to fall into 'else' clause
    }
    else
    {
      // range check
      A = *DE - 1;
      if (A >= *HL || A + 2 < *HL) // *DE - 1 .. *DE + 1
        return;
      HL++;
      DE++;
      A = *DE;
      // CP *HL  // TRICKY!
      // BIT 0,C // if ((C & (1<<0)) == 0) carry = !carry;
      // RET C
    }
  }

  if (!state->red_flag)
  {
    A = IY->pos.vo; // sampled IY=$8020 // saw this breakpoint hit when outdoors
    if (A < 32) // vertical offset
      IY->flags = vischar_BYTE1_BIT1;
    return;
#endif
  }
  state->bell = bell_RING_PERPETUAL;
  guards_persue_prisoners(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $CCAB: Guards persue prisoners.
 *
 * Searches for a visible character and something else, sets a flag.
 *
 * \param[in] state Pointer to game state.
 */
void guards_persue_prisoners(tgestate_t *state)
{
  vischar_t *HL;
  uint8_t    B;

  HL = &state->vischars[1];
  B  = vischars_LENGTH - 1;
  do
  {
    if (HL->character < character_20 && HL->mi.pos.vo < 32)
      HL->flags = vischar_BYTE1_PERSUE;
    HL++;
  }
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $CCCD: Is item discoverable?
 *
 * Searches item_structs for items dropped nearby. If items are found the
 * guards are made to persue the player.
 *
 * Green key and food items are ignored.
 *
 * \param[in] state Pointer to game state.
 */
void is_item_discoverable(tgestate_t *state)
{
  room_t        A;
  itemstruct_t *HL;
  uint8_t       B;
  item_t        item;

  A = state->room_index;
  if (A != room_0_OUTDOORS)
  {
    if (is_item_discoverable_interior(state, A, NULL) == 0) // check the NULL - is the returned item needed?
      guards_persue_prisoners(state);
    return;
  }
  else
  {
    HL = &state->item_structs[0];
    B  = item__LIMIT;
    do
    {
      if (HL->room & itemstruct_ROOM_FLAG_ITEM_NEARBY)
        goto nearby;

next:
      HL++;
    }
    while (--B);
    return;

nearby:
    item = HL->item & itemstruct_ITEM_MASK;
    if (item == item_GREEN_KEY || item == item_FOOD) /* ignore these items */
      goto next;

    guards_persue_prisoners(state);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $CCFB: Is item discoverable indoors?
 *
 * \param[in]  state Pointer to game state.
 * \param[in]  room  Room index. (was A)
 * \param[out] pitem Pointer to item (if found). (Can be NULL)
 *
 * \return 0 if found, 1 if not found.
 */
int is_item_discoverable_interior(tgestate_t *state,
                                  room_t      room,
                                  item_t     *pitem)
{
  const itemstruct_t *HL;
  uint8_t             B;
  uint8_t             A;

  HL = &state->item_structs[0];
  B  = item__LIMIT;
  do
  {
    /* Is the item in the specified room? */
    if ((HL->room & itemstruct_ROOM_MASK) == room &&
        /* Has the item been moved to a different room? */
        /* Note that room_and_flags doesn't get its flags masked off. Does it
         * need & 0x3F ? */
        default_item_locations[HL->item & itemstruct_ITEM_MASK].room_and_flags != room)
    {
      A = HL->item & itemstruct_ITEM_MASK;

      /* Ignore the red cross parcel. */
      if (A != item_RED_CROSS_PARCEL)
      {
        if (pitem) /* Added to support return of pitem. */
          *pitem = A;

        return 0; /* found */
      }
    }

    HL++;
  }
  while (--B);

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
  const default_item_location_t *default_item_location;
  room_t                         room; /* was A */
  itemstruct_t                  *itemstruct;

  if (item == item_NONE)
    return;

  item &= itemstruct_ITEM_MASK;

  queue_message_for_display(state, message_ITEM_DISCOVERED);
  decrease_morale(state, 5);

  default_item_location = &default_item_locations[item];
  room = default_item_location->room_and_flags;

  itemstruct = item_to_itemstruct(state, item);
  itemstruct->item &= ~itemstruct_ITEM_FLAG_HELD;

  itemstruct->room  = default_item_location->room_and_flags;
  itemstruct->pos.y = default_item_location->y;
  itemstruct->pos.x = default_item_location->x;

  if (room == room_0_OUTDOORS)
  {
    itemstruct->pos.vo = 0; /* The original code assigned room here, as it's zero. */
    drop_item_A_exterior_tail(itemstruct);
  }
  else
  {
    itemstruct->pos.vo = 5;
    drop_item_A_interior_tail(itemstruct);
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

/**
 * $DB9E: Mark nearby items.
 *
 * Iterates over item structs. Tests to see if items are within range
 * (-1..22,0..15) of the map position.
 *
 * This is similar to is_item_discoverable_interior in that it iterates over
 * item_structs.
 *
 * \param[in] state Pointer to game state.
 */
void mark_nearby_items(tgestate_t *state)
{
  room_t        C;
  uint8_t       D, E;
  uint8_t       B;
  itemstruct_t *HL;

  C = state->room_index;
  if (C == room_NONE)
    C = room_0_OUTDOORS;

  E = state->map_position[0];
  D = state->map_position[1];

  B  = item__LIMIT;
  HL = &state->item_structs[0];
  do
  {
    uint8_t t1, t2;

    t1 = (HL->target & 0x00FF) >> 0;
    t2 = (HL->target & 0xFF00) >> 8;

    if ((HL->room & itemstruct_ROOM_MASK) == C &&
        (t1 > E - 2 && t1 < E + 23) &&
        (t2 > D - 1 && t2 < D + 16))
      HL->room |= itemstruct_ROOM_FLAG_BIT6 | itemstruct_ROOM_FLAG_ITEM_NEARBY; /* set */
    else
      HL->room &= ~(itemstruct_ROOM_FLAG_BIT6 | itemstruct_ROOM_FLAG_ITEM_NEARBY); /* reset */

    HL++;
  }
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $DBEB: Iterates over all item_structs looking for nearby items.
 *
 * \param[in]  state  Pointer to game state.
 * \param[in]  BCdash ? Compared to X and Y.  // sampled BCdash = $26, $3E, $00, $26, $34, $22, $32
 * \param[out] pIY    Returned pointer to item struct.
 *
 * \return Adash.
 */
uint8_t sub_DBEB(tgestate_t *state, uint16_t BCdash, itemstruct_t **pIY)
{
  uint8_t       B;
  itemstruct_t *itemstructs; /* was HL */
  uint8_t       Adash;

  B = 16; /* iterations */
  itemstructs = &state->item_structs[0];
  do
  {
    const enum itemstruct_flags FLAGS = itemstruct_ROOM_FLAG_BIT6 | itemstruct_ROOM_FLAG_ITEM_NEARBY;

    if ((itemstructs->room & FLAGS) == FLAGS)
    {
      itemstruct_t *HL;

      HL = itemstructs; // in original points to &HL->room;
      // original calls out to multiply by 8, HLdash is temp
      if ((HL->pos.y * 8 > BCdash) && (HL->pos.x * 8 > BCdash))
      {
        uint8_t  *p; /* was HLdash */
        uint16_t  DEdash;
        uint16_t  BCdash; // shadows input BCdash -- does this match the original?

        p = &HL->pos.x;
        DEdash = *p-- * 8; // x position
        BCdash = *p-- * 8; // y position .. which BCdash is this writing to? the outer one or a local copy?
        *pIY = (itemstruct_t *) (p - 1); // sampled IY = $771C,7715 (pointing into item_structs)

        // original code has unpaired A register exchange here. if the routine loops then it's unclear which output register is used
        Adash = (16 - B) | (1 << 6); // iteration count + flag?
      }
    }
    itemstructs++;
  }
  while (--B);

  return Adash;
}

/* ----------------------------------------------------------------------- */

/**
 * $DC41: Looks like it sets up item plotting.
 *
 * \param[in] state Pointer to game state.
 * \param[in] IY    Pointer to visible character.
 * \param[in] A     ? suspect an item
 */
void setup_item_plotting(tgestate_t *state, vischar_t *IY, uint8_t A)
{
  location_t *HL;
  tinypos_t  *DE;
  uint16_t    BC;

  A &= 0x3F; // some item max mask (which ought to be 0x1F BUT outer routine is also setting 1<<6)
  state->possibly_holds_an_item = A; // This is written to but never read from. (A memory breakpoint set in FUSE confirmed this).

  // copy target+tinypos to state->tinypos+map_position_related_1/2?

  memcpy(&state->tinypos_81B2, &IY->target, 5);
  HL = &IY->target + 5;
  DE = &state->tinypos_81B2 + 5;
  BC = 0;

#if 0
  // EX DE, HL
  *HL = B; // B always zero here?
  HL = &item_definitions[A].height;
  A = *HL;
  state->item_height = A; // = item_definitions[A].height;
  memcpy(&bitmap_pointer, ++HL, 4); // copy bitmap and mask pointers
  sub_DD02(state);
  if (!Z)
    return;

  // PUSH BC
  // PUSH DE

  state->self_E2C2 = E; // self modify

  if (B == 0)
  {
    A = 0x77; // LD (HL),A
    Adash = C;
  }
  else
  {
    A = 0; // NOP
    Adash = 3 - C;
  }

  Cdash = Adash;

  // set the addresses in the jump table to NOP or LD (HL),A
  HLdash = &masked_sprite_plotter_16_enables[0];
  Bdash = 3; /* iterations */
  do
  {
    DEdash = wordat(HLdash); HLdash += 2; *DEdash = A;
    DEdash = wordat(HLdash); HLdash += 2; *DEdash = A;
    if (--Cdash == 0)
      A ^= 0x77; // toggle between LD and NOP
  }
  while (--Bdash);

  A = D;
  A &= A; // test D
  DE = 0;
  if (Z)
  {
    HL = &state->map_position[1];
    A = state->map_position_related_2 - *HL;
    HL = A * 192;
    // EX DE, HL
  }

  A = state->map_position_related_1 - state->map_position[0];
  if (HL < 0)
    H = 0xFF;
  state->screen_pointer = HL + DE + &state->window_buf[0]; // screen buffer start address
  HL = &state->mask_buffer[0];

  // pop the clipped height, i think
  // POP DE
  // PUSH DE

  L += D * 4;
  state->foreground_mask_pointer = HL;
  // POP DE
  // PUSH DE
  A = D;
  if (A)
  {
    D = A;
    A = 0;
    E = 3; // unusual (or self modified and i've not spotted the write instruction)
    E--;
    do
      A += E;
    while (--D);
  }

  E = A;
  state->bitmap_pointer += DE;
  state->mask_pointer   += DE;
  // POP BC
  // POP DE
  A = 0;
#endif
}

/* ----------------------------------------------------------------------- */

/**
 * $DD02: (unknown)
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0/1
 */
uint8_t sub_DD02(tgestate_t *state)
{
  uint8_t *HL;
  uint16_t DE;
  uint8_t  A;

  HL = &state->map_position_related_1;
  DE = state->map_position;

#if 0
  A = ((DE & 0xFF) >> 0) + 24 - HL[0];
  if (A <= 0)
    goto return_1;

  if (A < 3)
  {
    BC = A;
  }
  else
  {
    A = HL[0] + 3 - E;
    if (A <= 0)
      goto return_1;

    if (A < 3)
    {
      C = A;
      B = 3 - C;
    }
    else
    {
      BC = 3;
    }
  }

  A = ((DE & 0xFF) >> 8) + 17 - HL[1]; // map_position_related_2
  if (A <= 0)
    goto return_1;

  if (A < 2)
  {
    DE = 8;
  }
  else
  {
    A = HL[1] + 2 - D;
    if (A <= 0)
      goto return_1;

    if (A < 2)
    {
      E = item_height - 8;
      D = 8;
    }
    else
    {
      DE = item_height;
    }
  }

return_0:
  return 0;

return_1:
#endif
  return 1;
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
 * $E0E0:
 *
 * Used by setup_item_plotting and setup_sprite_plotting. [setup_item_plotting: items are always 16 wide].
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

#define SLA(r) do { carry = (r) >> 7; (r) <<= 1; } while (0)
#define RL(r)  do { carry_out = (r) >> 7; (r) = ((r) << 1) | (carry); carry = carry_out; } while (0)
#define SRL(r) do { carry = (r) & 1; (r) >>= 1; } while (0)
#define RR(r)  do { carry_out = (r) & 1; (r) = ((r) >> 1) | (carry << 7); carry = carry_out; } while (0)

#define MASK(bm,mask) ((~*foremaskptr | (mask)) & *screenptr) | ((bm) & *foremaskptr)

/**
 * $E102: Sprite plotter. Used for characters and objects.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void masked_sprite_plotter_24_wide(tgestate_t *state, vischar_t *vischar)
{
  uint8_t        A;
  uint8_t        iters; /* was B */
  const uint8_t *maskptr;
  const uint8_t *bitmapptr;
  const uint8_t *foremaskptr;
  uint8_t       *screenptr;

  if ((A = ((vischar->w18 & 0xFF) & 7)) < 4)
  {
    uint8_t self_E161, self_E143;

    /* Shift right? */

    A = (~A & 3) * 8; // jump table offset (on input, A is 0..3)

    self_E161 = A; // self-modify: jump into mask rotate
    self_E143 = A; // self-modify: jump into bitmap rotate

    maskptr   = state->mask_pointer;
    bitmapptr = state->bitmap_pointer;

    iters = state->self_E121; /* iterations */ // height?
    do
    {
      uint8_t bm0, bm1, bm2, bm3;         // was B, C, E, D
      uint8_t mask0, mask1, mask2, mask3; // was B', C', E', D'
      int     carry, carry_out;

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

      A = MASK(bm0, mask0);
      foremaskptr++;
      if (state->enable_E188)
        *screenptr++ = A;

      A = MASK(bm0, mask1);
      foremaskptr++;
      if (state->enable_E199)
        *screenptr++ = A;

      A = MASK(bm0, mask2);
      foremaskptr++;
      if (state->enable_E1AA)
        *screenptr++ = A;

      A = MASK(bm0, mask3);
      foremaskptr++;
      state->foreground_mask_pointer = foremaskptr;
      if (state->enable_E1BF)
        *screenptr = A;

      screenptr += 21; // stride (24 - 3)
      state->screen_pointer = screenptr;
    }
    while (--iters);
  }
  else
  {
    uint8_t self_E22A, self_E204;

    /* Shift left? */

    A -= 4; // (on input, A is 4..7)
    A = (A << 3) | (A >> 5); // was 3 x RLCA

    self_E22A = A; // self-modify: jump into mask rotate
    self_E204 = A; // self-modify: jump into bitmap rotate

    maskptr   = state->mask_pointer;
    bitmapptr = state->bitmap_pointer;

    iters = state->self_E1E2; /* iterations */ // height?
    do
    {
      /* Note the different variable order to the case above. */
      uint8_t bm0, bm1, bm2, bm3;         // was E, C, B, D
      uint8_t mask0, mask1, mask2, mask3; // was E', C', B', D'
      int     carry, carry_out;

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

      A = MASK(bm3, mask3);
      foremaskptr++;
      if (state->enable_E259)
        *screenptr = A;
      screenptr++;

      A = MASK(bm2, mask2);
      foremaskptr++;
      if (state->enable_E26A)
        *screenptr = A;
      screenptr++;

      A = MASK(bm1, mask1);
      foremaskptr++;
      if (state->enable_E27B)
        *screenptr = A;
      screenptr++;

      A = MASK(bm0, mask0);
      foremaskptr++;
      state->foreground_mask_pointer = foremaskptr;
      if (state->enable_E290)
        *screenptr = A;
      screenptr++;

      screenptr += 21; // stride (24 - 3)
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
  masked_sprite_plotter_16_wide_left(state, 0);
}

/**
 * $E2A2: Sprite plotter. Used for characters and objects.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 */
void masked_sprite_plotter_16_wide(tgestate_t *state, vischar_t *vischar)
{
  uint8_t A;

  if ((A = ((vischar->w18 & 0xFF) & 7)) < 4)
    masked_sprite_plotter_16_wide_right(state, A);
  else
    masked_sprite_plotter_16_wide_left(state, A); /* i.e. fallthrough */
}

/**
 * $E2AC: Sprite plotter. Shifts left/right (unsure).
 *
 * \param[in] state Pointer to game state.
 * \param[in] A     Offset?
 */
void masked_sprite_plotter_16_wide_left(tgestate_t *state, uint8_t A)
{
  uint8_t        iters; /* was B */
  const uint8_t *maskptr;
  const uint8_t *bitmapptr;
  const uint8_t *foremaskptr;
  uint8_t       *screenptr;
  uint8_t        self_E2DC, self_E2F4;

  A = (~A & 3) * 6; // jump table offset (on input, A is 0..3 => 3..0) // 6 = length of asm chunk

  self_E2DC = A; // self-modify: jump into mask rotate
  self_E2F4 = A; // self-modify: jump into bitmap rotate

  maskptr   = state->mask_pointer;
  bitmapptr = state->bitmap_pointer;

  iters = state->self_E2C2; /* iterations */ // height? // self modified by $E49D (setup_sprite_plotting)
  do
  {
    uint8_t bm0, bm1, bm2;       // was D, E, C
    uint8_t mask0, mask1, mask2; // was D', E', C'
    int     carry, carry_out;

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

    A = MASK(bm0, mask0);
    foremaskptr++;
    if (state->enable_E319)
      *screenptr = A;
    screenptr++;

    A = MASK(bm1, mask1);
    foremaskptr++;
    if (state->enable_E32A)
      *screenptr = A;
    screenptr++;

    A = MASK(bm2, mask2);
    foremaskptr += 2;
    state->foreground_mask_pointer = foremaskptr;
    if (state->enable_E340)
      *screenptr = A;

    screenptr += 22; // stride (24 - 2)
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
 * \param[in] A     Offset?
 */
void masked_sprite_plotter_16_wide_right(tgestate_t *state, uint8_t A)
{
  uint8_t        iters; /* was B */
  const uint8_t *maskptr;
  const uint8_t *bitmapptr;
  const uint8_t *foremaskptr;
  uint8_t       *screenptr;
  uint8_t        self_E39A, self_E37D;

  A = (A - 4) * 6; // jump table offset (on input, A is 4..7 => 0..3) // 6 = length of asm chunk

  self_E39A = A; // self-modify: jump into bitmap rotate
  self_E37D = A; // self-modify: jump into mask rotate

  maskptr   = state->mask_pointer;
  bitmapptr = state->bitmap_pointer;

  iters = state->self_E363; /* iterations */ // height? // self modified
  do
  {
    uint8_t bm0, bm1, bm2;       // was E, D, C
    uint8_t mask0, mask1, mask2; // was E', D', C'
    int     carry, carry_out;

    /* Load bitmap bytes into D,E. */
    bm1 = *bitmapptr++;
    bm0 = *bitmapptr++;

    /* Load mask bytes into D',E'. */
    mask1 = *maskptr++;
    mask0 = *maskptr++;

    if (state->flip_sprite & (1<<7)) // ToDo: Hoist out this constant flag.
      flip_16_masked_pixels(state, &mask1, &mask0, &bm1, &bm0);

    foremaskptr = state->foreground_mask_pointer;

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

    screenptr = state->screen_pointer; // moved relative to the 24 version

    A = MASK(bm2, mask2);
    foremaskptr++;
    if (state->enable_E3C5)
      *screenptr = A;
    screenptr++;

    A = MASK(bm1, mask1);
    foremaskptr++;
    if (state->enable_E3D6)
      *screenptr = A;
    screenptr++;

    A = MASK(bm0, mask0);
    foremaskptr += 2;
    state->foreground_mask_pointer = foremaskptr;
    if (state->enable_E3EC)
      *screenptr = A;

    screenptr += 22; // stride (24 - 2)
    state->screen_pointer = screenptr;
  }
  while (--iters);
}


/**
 * $E3FA: Reverses the 24 pixels in E,B,C and E',B',C'.
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

  // Conv: Much simplified over the original code.

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
 * $E420: setup_sprite_plotting
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character. (was IY)
 *
 * HL is passed in too (but disassembly says it's always the same as IY).
 */
void setup_sprite_plotting(tgestate_t *state, vischar_t *vischar)
{
  /**
   * $E0EC: Locations which are poked between NOPs and LD (HL),A.
   */
  static const size_t masked_sprite_plotter_24_enables[5 * 2] =
  {
    offsetof(tgestate_t, enable_E188),
    offsetof(tgestate_t, enable_E259),
    offsetof(tgestate_t, enable_E199),
    offsetof(tgestate_t, enable_E26A),
    offsetof(tgestate_t, enable_E1AA),
    offsetof(tgestate_t, enable_E27B),
    offsetof(tgestate_t, enable_E1BF),
    offsetof(tgestate_t, enable_E290),

    // suspect these are unused
    //masked_sprite_plotter_16_wide
    //masked_sprite_plotter_24_wide
  };

  pos_t          *HLpos;
  tinypos_t      *DEtinypos;
  const sprite_t *BCsprites;
  uint8_t         A;
  const sprite_t *DEsprites;
  uint16_t        BCclipped;
  uint16_t        DEclipped;
  const size_t   *HLenables;
  uint8_t         self_E4C0;
  uint8_t         Adash;
  uint8_t         Cdash;
  uint8_t         Bdash;
  uint8_t         E;
  uint16_t        HL;
  uint8_t        *HLmaskbuf;
  uint16_t        DEsub;

  HLpos = &vischar->mi.pos;
  DEtinypos = &state->tinypos_81B2;

  if (state->room_index)
  {
    /* Indoors. */

    DEtinypos->y  = HLpos->y;
    DEtinypos->x  = HLpos->x;
    DEtinypos->vo = HLpos->vo;
  }
  else
  {
    /* Outdoors. */

    // unrolled over original, removed divide-by-8 calls
    DEtinypos->y  = (HLpos->y + 4) >> 3; /* with rounding */
    DEtinypos->x  = (HLpos->x    ) >> 3;
    DEtinypos->vo = (HLpos->vo   ) >> 3;
  }

  BCsprites = vischar->mi.spriteset;

  state->flip_sprite = A = vischar->mi.b17; // set left/right flip flag / sprite offset

  // HL now points after b17
  // DE now points to state->map_position_related_1

  // unrolled over original
  state->map_position_related_1 = vischar->w18 >> 3;
  state->map_position_related_2 = vischar->w1A >> 3;

  DEsprites = &BCsprites[A]; // spriteset pointer // A takes what values?

  vischar->width_bytes = DEsprites->width;  // width in bytes
  vischar->height = DEsprites->height; // height in rows

  state->bitmap_pointer = DEsprites->data;
  state->mask_pointer   = DEsprites->mask;

  A = vischar_visible(state, vischar, &BCclipped, &DEclipped);
  if (A)
    return;

  // PUSH BCclipped
  // PUSH DEclipped

  E = DEclipped & 0xFF; // must be no of visible rows?

  if (vischar->width_bytes == 3) // 3 => 16 wide, 4 => 24 wide
  {
    state->self_E2C2 = E; // self-modify
    state->self_E363 = E; // self-modify

    A = 3;
    HLenables = &masked_sprite_plotter_16_enables[0];
  }
  else
  {
    state->self_E121 = E; // self-modify
    state->self_E1E2 = E; // self-modify

    A = 4;
    HLenables = &masked_sprite_plotter_24_enables[0];
  }

  // PUSH HL

  self_E4C0 = A; // self-modify
  E = A;
  A = (BCclipped & 0xFF00) >> 8;
  if (A == 0)
  {
    A     = 0x77; // LD (HL),A
    Adash = BCclipped & 0xFF;
  }
  else
  {
    A     = 0x00; // NOP
    Adash = E - (BCclipped & 0xFF);
  }

  // POP HLdash // entry point

  // set the addresses in the jump table to NOP or LD (HL),A
  Cdash = Adash; // must be no of columns?
  Bdash = self_E4C0; /* iterations */ // 3 or 4
  do
  {
    *(((uint8_t *) state) + *HLenables++) = A;
    *(((uint8_t *) state) + *HLenables++) = A;
    if (--Cdash == 0)
      A ^= 0x77; // toggle between LD and NOP
  }
  while (--Bdash);

  if ((DEclipped >> 8) == 0)
    DEsub = (vischar->w1A - (state->map_position[1] * 8)) * 24;
  else
    DEsub = 0; // Conv: reordered

  HL = state->map_position_related_1 - state->map_position[0]; // signed subtract + extend to 16-bit
  state->screen_pointer = HL + DEsub + &state->window_buf[0]; // screen buffer start address

  HLmaskbuf = &state->mask_buffer[0];

  // POP DE  // pop DEclipped
  // PUSH DE

  HLmaskbuf += (DEclipped >> 8) * 4 + (vischar->w1A & 7) * 4; // i *think* its DEclipped
  state->foreground_mask_pointer = HLmaskbuf;

  // POP DE  // pop DEclipped

  A = DEclipped >> 8;
  if (A)
  {
    // Conv: Replaced loop with multiply.
    A *= vischar->width_bytes - 1;
    DEclipped &= 0x00FF; // D = 0
  }

  DEclipped = (DEclipped & 0xFF00) | A; // check all this guff over again
  state->bitmap_pointer += DEclipped;
  state->mask_pointer   += DEclipped;

  // POP BCclipped  // why save it anyway? if it's just getting popped. unless it's being returned.
}

/* ----------------------------------------------------------------------- */

/**
 * $E542: Scale down a pos_t and assign result to a tinypos_t.
 *
 * Divides the three input 16-bit words by 8, with rounding to nearest,
 * storing the result as bytes.
 *
 * \param[in]  in  Input pos_t.
 * \param[out] out Output tinypos_t.
 */
void scale_pos(const pos_t *in, tinypos_t *out)
{
  const uint8_t *HL;
  uint8_t       *DE;
  uint8_t        B;

  /* Use knowledge of the structure layout to cast the pointers to array[3]
   * of their respective types. */
  HL = (const uint8_t *) &in->y;
  DE = (uint8_t *) &out->y;

  B = 3;
  do
  {
    uint8_t A, C;

    A = *HL++;
    C = *HL++;
    divide_by_8_with_rounding(&C, &A);
    *DE++ = A;
  }
  while (--B);
}

/**
 * $E550: Divide AC by 8, with rounding.
 *
 * \param[in,out] A Pointer to low.
 * \param[in,out] C Pointer to high.
 */
void divide_by_8_with_rounding(uint8_t *A, uint8_t *C)
{
  int t;

  t = *A + 4;
  if (t >= 0x100)
  {
    *A = (uint8_t) t; /* Store modulo 256. */
    (*C)++;
  }

  divide_by_8(A, C);
}

/**
 * $E555: Divide AC by 8.
 *
 * \param[in,out] A Pointer to low.
 * \param[in,out] C Pointer to high.
 */
void divide_by_8(uint8_t *A, uint8_t *C)
{
  *A = (*A >> 3) | (*C << 5);
  *C >>= 3;
}

/* ----------------------------------------------------------------------- */

/**
 * $EED3: Plot the game screen.
 *
 * \param[in] state Pointer to game state.
 */
void plot_game_window(tgestate_t *state)
{
  uint8_t *const  screen = &state->speccy->screen[0];
  uint8_t         A;
  uint8_t        *HL;
  const uint16_t *SP;
  uint8_t        *DE;
  uint8_t         Bdash;
  uint8_t         B;
  uint8_t         C;
  uint8_t         tmp;

  A = (state->plot_game_window_x & 0xFF00) >> 8;
  if (A == 0)
  {
    HL = &state->window_buf[1] + (state->plot_game_window_x & 0xFF);
    SP = &state->game_window_start_offsets[0];
    A  = 128; /* iterations */
    do
    {
      DE = screen + *SP++;
      *DE++ = *HL++; /* unrolled: 23 copies */
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      *DE++ = *HL++;
      HL++; // skip 24th
    }
    while (--A);
  }
  else
  {
    HL = &state->window_buf[1] + (state->plot_game_window_x & 0xFF);
    A  = *HL++;
    SP = &state->game_window_start_offsets[0];
    Bdash = 128; /* iterations */
    do
    {
      DE = screen + *SP++;

/* RRD is equivalent to: */
#define RRD(A, HL, tmp)          \
  tmp = A & 0x0F;                \
  A = (*HL & 0x0F) | (A & 0xF0); \
  *HL = (*HL >> 4) | (tmp << 4);

      /* The original code was unrolled into 4 groups of 5 ops, then a final 3. */
      B = 4 * 5 + 3; /* iterations */
      do
      {
        C = *HL; // safe copy of source data
        RRD(A, HL, tmp);
        A = *HL; // rotated result
        *DE++ = A;
        *HL++ = C; // restore safe copy

        // A simplified form would be:
        // next_A = *HL;
        // *HL++ = (*HL >> 4) | (Adash << 4);
        // Adash = next_A;
      }
      while (--B);

      A = *HL++;
    }
    while (--Bdash);
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
  uint16_t   DE;
  uint8_t   *HL;
  uint8_t    B;
  uint8_t    A;
  vischar_t *vischar;

  /* Is the player within the roll call area bounds? */
  // UNROLLED
  /* Range checking. X in (0x72..0x7C) and Y in (0x6A..0x72). */
  DE = map_ROLL_CALL_X; // note the confused coords business
  HL = &state->player_map_position.y;
  A = *HL++;
  if (A < ((DE >> 8) & 0xFF) || A >= ((DE >> 0) & 0xFF))
    goto not_at_roll_call;

  DE = map_ROLL_CALL_Y;
  A = *HL++; // -> state->player_map_position.x
  if (A < ((DE >> 8) & 0xFF) || A >= ((DE >> 0) & 0xFF))
    goto not_at_roll_call;

  /* All characters turn forward. */
  vischar = &state->vischars[0];
  B = vischars_LENGTH;
  do
  {
    vischar->b0D = vischar_BYTE13_BIT7; // movement
    vischar->b0E = 0x03; // direction (3 => face bottom left)
    vischar++;
  }
  while (--B);

  return;

not_at_roll_call:
  state->bell = bell_RING_PERPETUAL;
  queue_message_for_display(state, message_MISSED_ROLL_CALL);
  guards_persue_prisoners(state); // exit via
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

  uint16_t  DE;
  uint8_t  *HL;
  uint8_t   A;

  /* Is the player within the main gate bounds? */
  // UNROLLED
  /* Range checking. X in (0x69..0x6D) and Y in (0x49..0x4B). */
  DE = map_MAIN_GATE_X; // note the confused coords business
  HL = &state->player_map_position.y;
  A = *HL++;
  if (A < ((DE >> 8) & 0xFF) || A >= ((DE >> 0) & 0xFF))
    return;

  DE = map_MAIN_GATE_Y;
  A = *HL++; // -> state->player_map_position.x
  if (A < ((DE >> 8) & 0xFF) || A >= ((DE >> 0) & 0xFF))
    return;

  /* Using the papers at the main gate when not in uniform causes the player
   * to be sent to solitary */
  if (state->vischars[0].mi.spriteset != &sprites[sprite_GUARD_FACING_TOP_LEFT_4])
  {
    solitary(state);
    return;
  }

  increase_morale_by_10_score_by_50(state);
  state->vischars[0].room = room_0_OUTDOORS;

  /* Transition to outside the main gate. */
  transition(state, &state->vischars[0], &outside_main_gate);
  NEVER_RETURNS;
}

/* ----------------------------------------------------------------------- */

/**
 * $EFFC: Waits for the user to press Y or N.
 *
 * \param[in] state Pointer to game state.
 *
 * \return 1 if 'Y' pressed, 0 if 'N' pressed.
 */
int user_confirm(tgestate_t *state)
{
  /** $F014 */
  static const screenlocstring_t screenlocstring_confirm_y_or_n =
  {
    0x100B, 15, "CONFIRM. Y OR N"
  };

  uint8_t keymask; /* was A */

  screenlocstring_plot(state, &screenlocstring_confirm_y_or_n);

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

    // FIXME: Infinite loop. Yield here.
    // eg. state->speccy->yield(state->speccy);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F163: Main.
 *
 * \param[in] state Pointer to game state.
 */
void tge_main(tgestate_t *state)
{
  /**
   * $F1C9: Initial state of a visible character.
   *
   * The original game stores just 23 bytes of the structure, we store a
   * whole structure here.
   */
  static const vischar_t vischar_initial =
  {
    0x00,                 // character
    0x00,                 // flags
    0x012C,               // target
    { 0x2E, 0x2E, 0x18 }, // p04
    0x00,                 // more flags
    0xCDF2,               // w08
    0xCF76,               // w0A
    0x00,                 // b0C
    0x00,                 // b0D
    0x00,                 // b0E
    { { 0x0000, 0x0000, 0x0018 }, &sprites[sprite_PRISONER_FACING_TOP_LEFT_4], 0 },
    0x0000,               // w18
    0x0000,               // w1A
    room_0_OUTDOORS,      // room
    0x00,                 // b1D
    0x00,                 // width_bytes
    0x00,                 // height
  };

  uint8_t   *HL;
  uint8_t    A;
  uint8_t    C;
  uint8_t    B;
  int        carry;
  vischar_t *HLvc;

  wipe_full_screen_and_attributes(state);
  set_morale_flag_screen_attributes(state, attribute_BRIGHT_GREEN_OVER_BLACK);
  /* The original code seems to pass in 0x44, not zero, as it uses a register
   * left over from a previous call to set_morale_flag_screen_attributes(). */
  set_menu_item_attributes(state, 0, attribute_YELLOW_OVER_BLACK);
  plot_statics_and_menu_text(state);

  plot_score(state);

  menu_screen(state);

  /* Construct a table of 256 bit-reversed bytes at 0x7F00. */
  HL = &state->reversed[0];
  do
  {
    /* This was 'A = L;' in original code as alignment was guaranteed. */
    A = HL - &state->reversed[0]; /* Get a counter 0..255. */

    /* Reverse the counter byte. */
    C = 0;
    B = 8; /* iterations */
    do
    {
      carry = A & 1;
      A >>= 1;
      C = (C << 1) | carry;
    }
    while (--B);

    *HL++ = C;
  }
  while (HL != &state->reversed[256]); // was ((HL & 0xFF) != 0);

  /* Initialise all visible characters. */
  HLvc = &state->vischars[0];
  B = vischars_LENGTH; /* iterations */
  do
  {
    memcpy(HLvc, &vischar_initial, sizeof(vischar_initial));
    HLvc++;
  }
  while (--B);

  /* Write 0xFF 0xFF over all non-visible characters. */
  B = vischars_LENGTH - 1; /* iterations */
  HLvc = &state->vischars[1];
  do
  {
    HLvc->character = character_NONE;
    HLvc->flags     = 0xFF;
    HLvc++;
  }
  while (--B);

  /* In the original code this wiped all state from $8100 up until the start
   * of tiles ($8218). We'll assume for now that tgestate_t is calloc'd and
   * so wiped by default. */

  reset_game(state);
  enter_room(state); // returns by goto main_loop
  NEVER_RETURNS;
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
    { 0x080D, 11, "1 KEYBOARD"              },
    { 0x084D, 10, "2 KEMPSTON"              },
    { 0x088D, 10, "3 SINCLAIR"              },
    { 0x08CD,  8, "4 PROTEK"                },
    { 0x1007, 23, "BREAK OR CAPS AND SPACE" },
    { 0x102C, 12, "FOR NEW GAME"            },
  };

  const statictileline_t  *stline;
  int                      B;
  uint8_t                 *DE;
  const screenlocstring_t *slstring;

  /* Plot statics and menu text. */
  stline = &static_graphic_defs[0];
  B      = NELEMS(static_graphic_defs);
  do
  {
    DE = &state->speccy->screen[stline->screenloc]; /* Fetch screen address offset. */
    if (stline->flags_and_length & statictileline_VERTICAL)
      plot_static_tiles_vertical(state, DE, stline->tiles);
    else
      plot_static_tiles_horizontal(state, DE, stline->tiles);
    stline++;
  }
  while (--B);

  /* Plot menu text. */
  B        = NELEMS(key_choice_screenlocstrings);
  slstring = &key_choice_screenlocstrings[0];
  do
    slstring = screenlocstring_plot(state, slstring);
  while (--B);
}

/**
 * $F206: Plot static tiles horizontally.
 *
 * \param[in] state Pointer to game state.
 * \param[in] out   Pointer to screen address.
 * \param[in] tiles Pointer to [count, tile indices, ...].
 */
void plot_static_tiles_horizontal(tgestate_t     *state,
                                  uint8_t        *out,
                                  const uint8_t *tiles)
{
  plot_static_tiles(state, out, tiles, 0);
}

/**
 * $F209: Plot static tiles vertically.
 *
 * \param[in] state Pointer to game state.
 * \param[in] out   Pointer to screen address.
 * \param[in] tiles Pointer to [count, tile indices, ...].
 */
void plot_static_tiles_vertical(tgestate_t     *state,
                                uint8_t        *out,
                                const uint8_t *tiles)
{
  plot_static_tiles(state, out, tiles, 255);
}

/**
 * $F20B: Plot static tiles.
 *
 * \param[in] state       Pointer to game state.
 * \param[in] out         Pointer to screen address.
 * \param[in] tiles       Pointer to [count, tile indices, ...].
 * \param[in] orientation Orientation: 0/255 = horizontal/vertical.
 */
void plot_static_tiles(tgestate_t     *state,
                       uint8_t        *out,
                       const uint8_t *tiles,
                       int            orientation)
{
  int tiles_remaining;

  tiles_remaining = *tiles++ & 0x7F; /* Loop iterations (flag is ?). */
  do
  {
    int                  tile_index;
    const static_tile_t *static_tile;
    const tilerow_t     *tile_data;
    int                  iters;
    ptrdiff_t            soffset;
    int                  aoffset;
    uint8_t             *out_attr;

    tile_index = *tiles++;

    static_tile = &static_tiles[tile_index]; // elements: 9 bytes each

    /* Plot a tile. */
    tile_data = &static_tile->data.row[0];
    iters = 8; /* 8 iterations */
    do
    {
      *out = *tile_data++;
      out += 256; // next row
    }
    while (--iters);
    out -= 256;

    /* Calculate screen attribute address of tile. */
    // ((out - screen_base) / 0x800) * 0x100 + attr_base
    soffset = &state->speccy->screen[0] - out;
    aoffset = soffset & 0xFF;
    if (soffset >= 0x0800) aoffset += 256;
    if (soffset >= 0x1000) aoffset += 256;
    out_attr = &state->speccy->attributes[aoffset];

    *out_attr = static_tile->attr; // copy attribute byte

    if (orientation == 0) /* Horizontal. */
      out -= 7 * 256;
    else /* Vertical. */
      out = (uint8_t *) get_next_scanline(state, out); // must cast away constness
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
  memset(&state->speccy->screen, 0, SCREEN_LENGTH);
  memset(&state->speccy->attributes, attribute_WHITE_OVER_BLACK, SCREEN_ATTRIBUTES_LENGTH);

  state->speccy->out(state->speccy, port_BORDER, 0); /* set border to black */
}

/* ----------------------------------------------------------------------- */

/**
 * $F271: Menu screen key handling. (Routine could do with a rename).
 *
 * \param[in] state Pointer to game state.
 */
void select_input_device(tgestate_t *state)
{
  uint8_t A;

  A = input_device_select_keyscan(state);
  if (A == 0xFF)
    return; /* nothing happened */

  if (A)
  {
    /* 1..4 pressed. -> 0..3 */
    A = state->chosen_input_device;
    set_menu_item_attributes(state, A - 1, attribute_WHITE_OVER_BLACK);
    set_menu_item_attributes(state, A,     attribute_BRIGHT_YELLOW_OVER_BLACK);
  }
  else
  {
    /* Zero pressed to start game. */

    /* At this point the original game copies the required input routine to
     * $F075. */

    if (state->chosen_input_device == inputdevice_KEYBOARD)
      choose_keys(state); /* Keyboard was selected. */
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
  uint8_t *const screen = &state->speccy->screen[0];

  const uint16_t *sp;
  uint8_t         A;

  sp = &state->game_window_start_offsets[0]; /* points to offsets */
  A = 128; /* rows */
  do
    memset(screen + *sp++, 0, 23);
  while (--A);
}

/* ----------------------------------------------------------------------- */

/**
 * $F350: Choose keys.
 *
 * \param[in] state Pointer to game state.
 */
void choose_keys(tgestate_t *state)
{
  /** $F2AD: Prompts. */
  static const screenlocstring_t choose_key_prompts[6] =
  {
    { 0x006D, 11, "CHOOSE KEYS" },
    { 0x00CD,  5, "LEFT." },
    { 0x080D,  6, "RIGHT." },
    { 0x084D,  3, "UP." },
    { 0x088D,  5, "DOWN." },
    { 0x08CD,  5, "FIRE." },
  };

  /** $F2E1: Keyscan high bytes. */
  static const uint8_t keyboard_port_hi_bytes[10] =
  {
    /* The first byte here is unused AFAICT. */
    /* Final zero byte is a terminator. */
    0x24, 0xF7, 0xEF, 0xFB, 0xDF, 0xFD, 0xBF, 0xFE, 0x7F, 0x00,
  };

  /** $F2EB: Special key name strings. */
  static const char modifier_key_names[] = "\x05" "ENTER"
                                           "\x04" "CAPS"   /* CAPS SHIFT */
                                           "\x06" "SYMBOL" /* SYMBOL SHIFT */
                                           "\x05" "SPACE";

  /** $F303: Table mapping key codes to glyph indices. */
  /* Each table entry is a character (in original code: a glyph index) OR if
   * its top bit is set then bottom seven bits are an index into
   * modifier_key_names. */
#define O(n) ((n) | (1 << 7))
  static const unsigned char keycode_to_glyph[8 * 5] =
  {
    '1',   '2',   '3',   '4',   '5', /* table_12345 */
    '0',   '9',   '8',   '7',   '6', /* table_09876 */
    'Q',   'W',   'E',   'R',   'T', /* table_QWERT */
    'P',   'O',   'I',   'U',   'Y', /* table_POIUY */
    'A',   'S',   'D',   'F',   'G', /* table_ASDFG */
    O(0),  'L',   'K',   'J',   'H', /* table_ENTERLKJH */
    O(6),  'Z',   'X',   'C',   'V', /* table_SHIFTZXCV */
    O(18), O(11), 'M',   'N',   'B', /* table_SPACESYMSHFTMNB */
  };
#undef O

  /** $F32B: Screen offsets where to plot key names.
   * In the original code these are absolute addresses. */
  static const uint16_t key_name_screen_offsets[5] =
  {
    0x00D5,
    0x0815,
    0x0855,
    0x0895,
    0x08D5,
  };

  for (;;)
  {
    uint8_t                  B;
    const screenlocstring_t *HL;

    wipe_game_window(state);
    set_game_window_attributes(state, attribute_WHITE_OVER_BLACK);

    /* Draw key choice prompt strings */
    B = NELEMS(choose_key_prompts); /* iterations */
    HL = &choose_key_prompts[0];
    do
    {
      uint8_t    *DE;
      uint8_t     Binner;
      const char *HLstring;

      DE = &state->speccy->screen[HL->screenloc];
      Binner = HL->length; /* iterations */
      HLstring = HL->string;
      do
      {
        // A = *HLstring; /* Present in original code but this is redundant when calling plot_glyph(). */
        plot_glyph(HLstring, DE);
        HLstring++;
      }
      while (--Binner);
    }
    while (--B);

    /* Wipe keydefs */
    memset(&state->keydefs.defs[0], 0, 10);

    {
      const uint16_t *HLoffset;

      B = 5; /* iterations */ /* L/R/U/D/F */
      HLoffset = &key_name_screen_offsets[0];
      do
      {
        uint16_t self_F3E9;
        uint8_t  A;

        self_F3E9 = *HLoffset++; // self modify screen addr
        A = 0xFF;

for_loop:
        for (;;)
        {
          const uint8_t *HLkeybytes;
          uint8_t        D;
          int            carry;

          HLkeybytes = &keyboard_port_hi_bytes[0]; // point to unused byte
          D = 0xFF;
#if 1
do_loop:
          {
            uint8_t   Adash;
            uint8_t   C;
            uint8_t   E;
            keydef_t *HLkeydef;

            HLkeybytes++;
            D++;
            Adash = *HLkeybytes;
            if (Adash == 0) /* Hit end of keyboard_port_hi_bytes. */
              goto for_loop;

            B = Adash; // checked later
            C = port_BORDER;
            Adash = ~state->speccy->in(state->speccy, (B << 8) | C);
            E = Adash;
            C = 1 << 5;
f3a0:
            carry = C & 1; C >>= 1; // was SRL C
            if (carry)
              goto do_loop; // masking loop ran out, move to next key

            Adash = C & E;
            if (Adash == 0)
              goto f3a0;

            if (A)
              goto for_loop;

            A = D;

            // looks like a loop checking for an already defined key

            HLkeydef = &state->keydefs.defs[-1]; // is this pointing to the byte before?
f3b1:
            HLkeydef++;

            Adash = HLkeydef->port;
            if (Adash == 0) // empty slot?
              goto assign_keydef;

            if (Adash != B || HLkeydef->mask != C)
              goto f3b1;
          }
#endif
        } // for_loop

assign_keydef:
        B=B; // no-op for now, delete me later
#if 0
        *HL++ = B;
        *HL = C;

        // A = row
        HL = &keycode_to_glyph[A * 5] - 1; /* off by one to compensate for preincrement */
        // skip entries until C carries out
        do
        {
          HL++;
          carry_out = C & 1; C = (C >> 1) | (carry << 7); carry = carry_out; // RR C;
        }
        while (!carry);

        B = 1; // length == 1
        A = *HL; // plot the byte at HL, string length is 1
        if (A & (1 << 7))
        {
          // else top bit was set => it's a modifier key
          HL = &modifier_key_names[A & 0x7F];
          B = *HL++; // read length
        }

        /* Plot. */
        DE = self_F3E9; // self modified // screen offset
        do
        {
          A = *HL;
          plot_glyph(HL, /*screenaddr + */ DE);
          HL++;
        }
        while (--B);
#endif
      }
      while (--B);
    }

    {
      uint16_t BC;

      /* Delay loop */
      BC = 0xFFFF;
      while (--BC)
        ;
    }

    /* Wait for user's input */
    if (user_confirm(state))
      return; // confirmed
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F408: Set menu item attributes
 *
 * \param[in] state Pointer to game state.
 * \param[in] item  Item index.
 * \param[in] attrs Screen attributes.
 */
void set_menu_item_attributes(tgestate_t *state,
                              item_t      item,
                              attribute_t attrs)
{
  attribute_t *pattr;

  pattr = &state->speccy->attributes[(0x590D - SCREEN_ATTRIBUTES_START_ADDRESS)];

  /* Skip to the item's row */
  pattr += item * 2 * state->width; /* two rows */

  /* Draw */
  memset(pattr, attrs, 10);
}

/* ----------------------------------------------------------------------- */

/**
 * $F41C: Input device select keyscan.
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0/1/2/3/4 = keypress, or 255 = no keypress
 */
uint8_t input_device_select_keyscan(tgestate_t *state)
{
  uint8_t E;
  uint8_t A;
  uint8_t B;

  E = 0;
  A = ~state->speccy->in(state->speccy, port_KEYBOARD_12345) & 0xF;
  if (A)
  {
    B = 4; /* iterations */
    do
    {
      /* Reordered from original to avoid carry flag test */
      E++;
      if (A & 1)
        break;
      A >>= 1;
    }
    while (--B);

    return E;
  }
  else
  {
    if ((state->speccy->in(state->speccy, port_KEYBOARD_09876) & 1) == 0)
      return E; /* always zero */

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
  uint16_t BC;
  uint16_t BCdash;
  uint16_t DE;
  uint16_t DEdash;
  uint8_t  A;
  uint8_t  H;
  uint16_t HL;
  uint16_t HLdash;
  uint8_t  L;
  uint8_t  Ldash;

  for (;;)
  {
    select_input_device(state);

    wave_morale_flag(state);

    /* Play music */
    HL = state->music_channel0_index + 1;
    for (;;)
    {
      state->music_channel0_index = HL;
      A = music_channel0_data[HL];
      if (A != 0xFF) /* end marker */
        break;
      HL = 0;
    }
    DE = get_tuning(A); // might need to be DE = BC = get_tuning
    L = 0;

    HLdash = state->music_channel1_index + 1;
    for (;;)
    {
      state->music_channel1_index = HLdash;
      A = music_channel1_data[HLdash];
      if (A != 0xFF) /* end marker */
        break;
      HLdash = 0;
    }
    DEdash = get_tuning(A); // using banked registers
    Ldash = 0;

//    A = Bdash; // result from get_tuning? likely equivalent A = (DEdash & 0xFF) >> 8;
    if (A == 0xFF)
    {
      BCdash = BC;
      DEdash = BCdash;
    }

    A = 24; /* overall tune speed (lower => faster) */
    do
    {
      H = 255; /* iterations */
      do
      {
        // big-endian counting down?
        //if (--B == 0 && --C == 0)
        {
          L ^= 16;
          state->speccy->out(state->speccy, port_BORDER, L);
          BC = DE;
        }
        //if (--Bdash == 0 && --Cdash == 0)
        {
          Ldash ^= 16;
          state->speccy->out(state->speccy, port_BORDER, Ldash);
          BCdash = DEdash;
        }
      }
      while (--H);
    }
    while (--A);
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

  BC = music_tuning_table[A];

  // Might be able to increment by 0x0101 rather than incrementing the bytes
  // separately, but unsure currently of exact nature of increment.

  BC = ((BC + 0x0001) & 0x00FF) | ((BC + 0x0100) & 0xFF00); // C++, B++;
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
  const keydef_t *def;
  uint8_t         inputs;
  uint16_t        port;
  uint8_t         key_pressed;

  def = &state->keydefs.defs[0]; /* list of (port, mask) */

  inputs = input_NONE; /* == 0 */

  /* left or right */
  port = (def->port << 8) | 0xFE;
  key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
  def++;
  if (key_pressed)
  {
    def++; /* skip right keydef */
    inputs += input_LEFT;
  }
  else
  {
    /* right */
    port = (def->port << 8) | 0xFE;
    key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
    def++;
    if (key_pressed)
      inputs += input_RIGHT;
  }

  /* up or down */
  port = (def->port << 8) | 0xFE;
  key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
  def++;
  if (key_pressed)
  {
    def++; /* skip down keydef */
    inputs += input_UP;
  }
  else
  {
    /* down */
    port = (def->port << 8) | 0xFE;
    key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
    def++;
    if (key_pressed)
      inputs += input_DOWN;
  }

  /* fire */
  port = (def->port << 8) | 0xFE;
  key_pressed = ~state->speccy->in(state->speccy, port) & def->mask;
  if (key_pressed)
    inputs += input_FIRE;

  return inputs;
}

/**
 * $FE47: Protek joystick input routine.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
input_t inputroutine_protek(tgestate_t *state)
{
  uint8_t  A;
  uint8_t  E;
  uint16_t BC;
  uint8_t  D;

  /* horizontal */
  A = ~state->speccy->in(state->speccy, port_KEYBOARD_12345);
  E = input_LEFT;
  BC = port_KEYBOARD_09876;
  if ((A & (1 << 4)) == 0)
  {
    A = ~state->speccy->in(state->speccy, BC);
    E = input_RIGHT;
    if ((A & (1 << 2)) == 0)
      E = 0;
  }

  /* vertical */
  D = ~state->speccy->in(state->speccy, BC);
  A = input_UP;
  if ((D & (1 << 3)) == 0)
  {
    A = input_DOWN;
    if ((D & (1 << 4)) == 0)
      A = input_NONE;
  }

  /* fire */
  E += A;
  A = input_FIRE;
  if (D & (1 << 0))
    A = 0; // no vt;

  return A + E; // combine axis
}

/**
 * $FE7E: Kempston joystick input routine.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
input_t inputroutine_kempston(tgestate_t *state)
{
  return 0; // NYI
}

/**
 * $FEA3: Fuller joystick input routine.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
input_t inputroutine_fuller(tgestate_t *state)
{
  return 0; // NYI
}

/**
 * $FECD: Sinclair joystick input routine.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Inputs.
 */
input_t inputroutine_sinclair(tgestate_t *state)
{
  return 0; // NYI
}

/* ----------------------------------------------------------------------- */

// ADDITIONAL FUNCTIONS
//

/**
 * The original game has a routine which copies an inputroutine_* routine
 * into place and then the game calls that. Instead we use a selector routine
 * here.
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

  assert(state->chosen_input_device < inputdevice__LIMIT);

  return inputroutines[state->chosen_input_device](state);
}
