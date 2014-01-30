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
 * - Move item_structs into tgestate (in fact, anything writable).
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

/* $6000 onwards */

void transition(tgestate_t      *state,
                vischar_t       *vischar,
                const tinypos_t *pos);
void enter_room(tgestate_t *state);
void squash_stack_goto_main(tgestate_t *state);

void set_player_sprite_for_room(tgestate_t *state);

void setup_movable_items(tgestate_t *state);
void setup_movable_item(tgestate_t    *state,
                        movableitem_t *movableitem,
                        character_t    character);

extern movableitem_t movable_items[movable_item__LIMIT];

void reset_nonplayer_visible_characters(tgestate_t *state);

void setup_doors(tgestate_t *state);

const doorpos_t *get_door_position(doorindex_t index);

void wipe_visible_tiles(tgestate_t *state);

void setup_room(tgestate_t *state);

void expand_object(tgestate_t *state, object_t index, uint8_t *output);

void plot_interior_tiles(tgestate_t *state);

extern uint8_t *const beds[beds_LENGTH];

extern const bounds_t roomdef_bounds[10];

/* $7000 onwards */

extern characterstruct_t character_structs[26];

extern const doorpos_t door_positions[124];

void check_for_pick_up_keypress(tgestate_t *state, input_t input);
void use_item_B(tgestate_t *state);
void use_item_A(tgestate_t *state);
void use_item_common(tgestate_t *state, item_t item);

extern const item_action_t item_actions_jump_table[item__LIMIT];

void pick_up_item(tgestate_t *state);
void drop_item(tgestate_t *state);
void drop_item_A(tgestate_t *state, item_t item);
void drop_item_A_exterior_tail(itemstruct_t *HL);
void drop_item_A_interior_tail(itemstruct_t *HL);

itemstruct_t *item_to_itemstruct(item_t item);

void draw_all_items(tgestate_t *state);
void draw_item(tgestate_t *state, item_t index, uint8_t *dst);

itemstruct_t *find_nearby_item(tgestate_t *state);

extern itemstruct_t item_structs[item__LIMIT];

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

void queue_message_for_display(tgestate_t *state,
                               message_t   message_index);

uint8_t *plot_glyph(const char *pcharacter, uint8_t *output);
uint8_t *plot_single_glyph(int A, uint8_t *output);

void message_display(tgestate_t *state);
void wipe_message(tgestate_t *state);
void next_message(tgestate_t *state);

extern const char *messages_table[message__LIMIT];

/* $8000 onwards */

/* $9000 onwards */

void main_loop(tgestate_t *state);

void check_morale(tgestate_t *state);

void keyscan_cancel(tgestate_t *state);

void process_player_input(tgestate_t *state);

void picking_a_lock(tgestate_t *state);

void snipping_wire(tgestate_t *state);

extern const uint8_t table_9EE0[4];

void in_permitted_area(tgestate_t *state);
int in_permitted_area_end_bit(tgestate_t *state, uint8_t room_and_flags);
int in_permitted_area_end_bit_2(tgestate_t *state, uint8_t A, tinypos_t *DE);

/* $A000 onwards */

void wave_morale_flag(tgestate_t *state);

void set_morale_flag_screen_attributes(tgestate_t *state, attribute_t attrs);

uint8_t *get_prev_scanline(tgestate_t *state, uint8_t *addr);

void interior_delay_loop(void);

void ring_bell(tgestate_t *state);
void plot_ringer(tgestate_t *state, const uint8_t *DE);

void increase_morale(tgestate_t *state, uint8_t delta);
void decrease_morale(tgestate_t *state, uint8_t delta);
void increase_morale_by_10_score_by_50(tgestate_t *state);
void increase_morale_by_5_score_by_5(tgestate_t *state);

void increase_score(tgestate_t *state, uint8_t delta);

void plot_score(tgestate_t *state);

void play_speaker(tgestate_t *state, sound_t sound);

extern const uint8_t bell_ringer_bitmap_off[];
extern const uint8_t bell_ringer_bitmap_on[];

void set_game_screen_attributes(tgestate_t *state, attribute_t attrs);

extern const timedevent_t timed_events[15];

void dispatch_timed_event(tgestate_t *state);

timedevent_handler_t event_night_time;
timedevent_handler_t event_another_day_dawns;
void set_day_or_night(tgestate_t *state, uint8_t A);
timedevent_handler_t event_wake_up;
timedevent_handler_t event_go_to_roll_call;
timedevent_handler_t event_go_to_breakfast_time;
timedevent_handler_t event_breakfast_time;
timedevent_handler_t event_go_to_exercise_time;
timedevent_handler_t event_exercise_time;
timedevent_handler_t event_go_to_time_for_bed;
timedevent_handler_t event_new_red_cross_parcel;
extern const uint8_t red_cross_parcel_reset_data[6];
extern const item_t red_cross_parcel_contents_list[4];
timedevent_handler_t event_time_for_bed;
timedevent_handler_t event_search_light;
void common_A26E(tgestate_t *state, uint8_t A, uint8_t C);

extern const character_t tenlong[10];

void wake_up(tgestate_t *state);
void breakfast_time(tgestate_t *state);

void set_player_target_location(tgestate_t *state, location_t location);

void go_to_time_for_bed(tgestate_t *state);

void sub_A35F(tgestate_t *state, uint8_t *counter);
void sub_A373(tgestate_t *state, uint8_t *counter);
void sub_A38C(tgestate_t *state, uint8_t A);
void sub_A3BB(tgestate_t *state);

void store_banked_A_then_C_at_HL(uint8_t Adash, uint8_t C, uint8_t *HL);

void byte_A13E_is_nonzero(tgestate_t        *state,
                          characterstruct_t *HL,
                          vischar_t         *IY);
void byte_A13E_is_zero(tgestate_t        *state,
                       characterstruct_t *HL,
                       vischar_t         *IY);
void sub_A404(tgestate_t        *state,
              characterstruct_t *HL,
              vischar_t         *IY,
              uint8_t            character_index);

void character_sits(tgestate_t *state, character_t A, uint8_t *formerHL);
void character_sleeps(tgestate_t *state, character_t A, uint8_t *formerHL);
void character_sit_sleep_common(tgestate_t *state, uint8_t C, uint8_t *HL);
void select_room_and_plot(tgestate_t *state);

void player_sits(tgestate_t *state);
void player_sleeps(tgestate_t *state);
void player_sit_sleep_common(tgestate_t *state, uint8_t *HL);

void set_location_0x0E00(tgestate_t *state);
void set_location_0x8E04(tgestate_t *state);
void set_location_0x1000(tgestate_t *state);

void byte_A13E_is_nonzero_anotherone(tgestate_t *state,
                                     characterstruct_t *HL,
                                     vischar_t         *IY);
void byte_A13E_is_zero_anotherone(tgestate_t *state,
                                  characterstruct_t *HL,
                                  vischar_t         *IY);
void byte_A13E_anotherone_common(tgestate_t        *state,
              characterstruct_t *HL,
              vischar_t         *IY,
              uint8_t            character_index);

void go_to_roll_call(tgestate_t *state);

void screen_reset(tgestate_t *state);

void escaped(tgestate_t *state);

uint8_t keyscan_all(tgestate_t *state);

escapeitem_t have_required_items(const item_t *pitem, escapeitem_t previous);
escapeitem_t item_to_bitmask(item_t item);

const screenlocstring_t *screenlocstring_plot(tgestate_t              *state,
                                              const screenlocstring_t *slstring);

extern const screenlocstring_t escape_strings[11];

void get_supertiles(tgestate_t *state);

void supertile_plot_1(tgestate_t *state);
void supertile_plot_2(tgestate_t *state);
void supertile_plot_common(tgestate_t *state,
                           uint8_t    *DE,
                           uint8_t    *HLdash,
                           uint8_t     A,
                           uint8_t    *DEdash);

void setup_exterior(tgestate_t *state);

void supertile_plot_3(tgestate_t *state);
void supertile_plot_4(tgestate_t *state);
void supertile_plot_common_A8F4(tgestate_t *state,
                                uint8_t    *DE,
                                uint8_t    *HLdash,
                                uint8_t     A,
                                uint8_t    *DEdash);

uint8_t *call_plot_tile_inc_de(tileindex_t              tile_index,
                               const supertileindex_t  *psupertile,
                               uint8_t                **scr);

uint8_t *plot_tile(tileindex_t             tile_index,
                   const supertileindex_t *psupertile,
                   uint8_t                *scr);

void map_shunt_1(tgestate_t *state);
void map_unshunt_1(tgestate_t *state);
void map_shunt_2(tgestate_t *state);
void map_unshunt_2(tgestate_t *state);
void map_shunt_3(tgestate_t *state);
void map_unshunt_3(tgestate_t *state);

void move_map(tgestate_t *state);

void map_move_1(tgestate_t *state, uint8_t *DE);
void map_move_2(tgestate_t *state, uint8_t *DE);
void map_move_3(tgestate_t *state, uint8_t *DE);
void map_move_4(tgestate_t *state, uint8_t *DE);

attribute_t choose_game_window_attributes(tgestate_t *state);

void zoombox(tgestate_t *state);
void zoombox_1(tgestate_t *state);
void zoombox_draw(tgestate_t *state);
void zoombox_draw_tile(tgestate_t *state, uint8_t index, uint8_t *addr);

void searchlight_AD59(tgestate_t *state, uint8_t *HL);
void nighttime(tgestate_t *state);
void searchlight_caught(tgestate_t *state, uint8_t *HL);
void searchlight_plot(tgestate_t *state);

const tile_t zoombox_tiles[6];

void sub_AF8F(tgestate_t *state, vischar_t *vischar);

void sub_AFDF(tgestate_t *state);

/* $B000 onwards */

void use_bribe(tgestate_t *state, vischar_t *vischar);

int bounds_check(tgestate_t *state, vischar_t *IY);

uint16_t BC_becomes_A_times_8(uint8_t A);

int is_door_open(tgestate_t *state);

void door_handling(tgestate_t *state, vischar_t *vischar);

int door_in_range(tgestate_t *state, const doorpos_t *HL);

uint16_t BC_becomes_A_times_4(uint8_t A);

int interior_bounds_check(tgestate_t *state, vischar_t *vischar);

void reset_B2FC(tgestate_t *state);

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
void action_key(tgestate_t *state, room_t room);

void *open_door(tgestate_t *state);

void action_wiresnips(tgestate_t *state);

extern const wall_t walls[24];

void called_from_main_loop_9(tgestate_t *state);

void reset_position(tgestate_t *state, vischar_t *vischar);
void reset_position_end(tgestate_t *state, vischar_t *vischar);

void reset_game(tgestate_t *state);
void reset_map_and_characters(tgestate_t *state);

void searchlight_sub(tgestate_t *state, vischar_t *IY);

void searchlight(tgestate_t *state);

uint8_t sub_B89C(tgestate_t *state);

void mask_stuff(tgestate_t *state);

uint16_t scale_val(tgestate_t *state, uint8_t A, uint8_t E);

void mask_against_tile(uint8_t index, uint8_t *dst);

int sub_BAF7(tgestate_t *state);

void called_from_main_loop_3(tgestate_t *state);

const tile_t *select_tile_set(tgestate_t *state, uint16_t HL);

/* $C000 onwards */

void called_from_main_loop_7(tgestate_t *state);

void called_from_main_loop_6(tgestate_t *state);

int spawn_characters(tgestate_t *state, characterstruct_t *HL);

void reset_visible_character(tgestate_t *state, vischar_t *vischar);

uint8_t sub_C651(tgestate_t *state, uint8_t *HL);

void move_characters(tgestate_t *state);

int increment_DE_by_diff(tgestate_t *state,
                         uint8_t     max,
                         int         B,
                         uint8_t    *HL,
                         uint8_t    *DE);

characterstruct_t *get_character_struct(character_t index);

void character_event(tgestate_t *state, location_t *loc);

charevnt_handler_t charevnt_handler_4_zero_morale_1;
charevnt_handler_t charevnt_handler_6;
charevnt_handler_t charevnt_handler_10_player_released_from_solitary;
charevnt_handler_t charevnt_handler_1;
charevnt_handler_t charevnt_handler_2;
charevnt_handler_t charevnt_handler_0;
void localexit(tgestate_t *state, character_t *charptr, uint8_t C);
charevnt_handler_t charevnt_handler_3_check_var_A13E;
charevnt_handler_t charevnt_handler_5_check_var_A13E_anotherone;
charevnt_handler_t charevnt_handler_7;
charevnt_handler_t charevnt_handler_9_player_sits;
charevnt_handler_t charevnt_handler_8_player_sleeps;

void follow_suspicious_player(tgestate_t *state);

void character_behaviour_stuff(tgestate_t *state, vischar_t *IY);
void gizzards(tgestate_t *state, vischar_t *vischar, uint8_t A);
void bit5set(tgestate_t *state, vischar_t *vischar, uint8_t A, int log2scale);

uint8_t move_character_y(tgestate_t *state,
                         vischar_t  *vischar,
                         int         log2scale);

uint8_t move_character_x(tgestate_t *state,
                         vischar_t  *vischar,
                         int         log2scale);

void bribes_solitary_food(tgestate_t *state, vischar_t *vischar);

void sub_CB23(tgestate_t *state, location_t *HL);

uint16_t BC_becomes_A(uint8_t A);

const uint8_t *element_A_of_table_7738(uint8_t A);

uint8_t get_A_indexed_by_C41A(tgestate_t *state);

void solitary(tgestate_t *state);

void guards_follow_suspicious_player(tgestate_t *state, vischar_t *IY);

void guards_persue_prisoners(tgestate_t *state);

void is_item_discoverable(tgestate_t *state);

int is_item_discoverable_interior(tgestate_t *state,
                                 room_t      room,
                                 item_t     *pitem);

void item_discovered(tgestate_t *state, item_t C);

extern const default_item_location_t default_item_locations[item__LIMIT];

extern const character_meta_data_t character_meta_data[4];

const uint8_t *character_related_pointers[24];

extern const uint8_t _cf06[];
extern const uint8_t _cf0e[];
extern const uint8_t _cf16[];
extern const uint8_t _cf1e[];
extern const uint8_t _cf26[];
extern const uint8_t _cf3a[];
extern const uint8_t _cf4e[];
extern const uint8_t _cf62[];
extern const uint8_t _cf76[];
extern const uint8_t _cf7e[];
extern const uint8_t _cf86[];
extern const uint8_t _cf8e[];
extern const uint8_t _cf96[];
extern const uint8_t _cfa2[];
extern const uint8_t _cfae[];
extern const uint8_t _cfba[];
extern const uint8_t _cfc6[];
extern const uint8_t _cfd2[];
extern const uint8_t _cfde[];
extern const uint8_t _cfea[];
extern const uint8_t _cff6[];
extern const uint8_t _d002[];
extern const uint8_t _d00e[];
extern const uint8_t _d01a[];

/* $D000 onwards */

void mark_nearby_items(tgestate_t *state);

void sub_DBEB(tgestate_t *state);

void sub_DC41(tgestate_t *state, vischar_t *IY, uint8_t A);

uint8_t sub_DD02(tgestate_t *state);

extern const sprite_t item_definitions[item__LIMIT];

/* $E000 onwards */

const void *masked_sprite_plotter_16_jump_table[6];
const void *masked_sprite_plotter_24_jump_table[10];

void masked_sprite_plotter_24_wide(tgestate_t *state, vischar_t *IY);

void masked_sprite_plotter_16_wide_case_1_searchlight(tgestate_t *state);
void masked_sprite_plotter_16_wide_case_1(tgestate_t *state);
void masked_sprite_plotter_16_wide_case_1_common(tgestate_t *state, uint8_t A);
void masked_sprite_plotter_16_wide_case_2(tgestate_t *state);

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

void setup_sprite_plotting(tgestate_t *state, vischar_t *IY);

void scale_pos(const pos_t *in, tinypos_t *out);
void divide_by_8_with_rounding(uint8_t *A, uint8_t *C);
void divide_by_8(uint8_t *A, uint8_t *C);

const sevenbyte_t stru_EA7C[47];
const uint8_t *exterior_mask_pointers[36];
const eightbyte_t exterior_mask_data[58];
const uint16_t game_screen_start_addresses[128];

void plot_game_screen(tgestate_t *state);

timedevent_handler_t event_roll_call;

void action_papers(tgestate_t *state);

int user_confirm(tgestate_t *state);

/* $F000 onwards */

void wipe_full_screen_and_attributes(tgestate_t *state);

void select_input_device(tgestate_t *state);

extern const screenlocstring_t define_key_prompts[6];

extern const uint8_t keyboard_port_hi_bytes[10];

extern const char counted_strings[];

extern const unsigned char key_tables[8 * 5];

extern const uint16_t key_name_screen_offsets[5];

void wipe_game_screen(tgestate_t *state);

void choose_keys(tgestate_t *state);

void set_menu_item_attributes(tgestate_t *state,
                              item_t      item,
                              attribute_t attrs);

uint8_t input_device_select_keyscan(tgestate_t *state);

void plot_statics_and_menu_text(tgestate_t *state);

void plot_static_tiles_horizontal(tgestate_t    *state,
                                  uint8_t       *DE,
                                  const uint8_t *HL);
void plot_static_tiles_vertical(tgestate_t      *state,
                                uint8_t       *DE,
                                const uint8_t *HL);
void plot_static_tiles(tgestate_t    *state,
                       uint8_t       *DE,
                       const uint8_t *HL,
                       int            orientation);

void menu_screen(tgestate_t *state);

uint16_t get_tuning(uint8_t A);

input_t inputroutine_keyboard(tgestate_t *state);
input_t inputroutine_protek(tgestate_t *state);
input_t inputroutine_kempston(tgestate_t *state);
input_t inputroutine_fuller(tgestate_t *state);
input_t inputroutine_sinclair(tgestate_t *state);

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
  room_t room_index; // was A

  if (vischar->room == room_0_outdoors)
  {
    /* Outdoors */

    /* Set position on Y axis, X axis and vertical offset (dividing by 4). */

    /* This was unrolled (compared to the original) to avoid having to access
     * the structure as an array. */
    vischar->mi.pos.y  = BC_becomes_A_times_4(pos->y);
    vischar->mi.pos.x  = BC_becomes_A_times_4(pos->x);
    vischar->mi.pos.vo = BC_becomes_A_times_4(pos->vo);
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
    if (room_index == room_0_outdoors)
    {
      /* Outdoors */

      vischar->b0D = 0x80; // likely a character direction
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
  state->plot_game_screen_x = 0;
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
  player->b0D = 0x80; // likely a character direction

  /* When in tunnel rooms this forces the player sprite to 'prisoner' and sets
   * the crawl flag appropriately. */
  if (state->room_index >= room_29_secondtunnelstart)
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
  movableitem_t *movableitem;
  character_t    character;

  reset_nonplayer_visible_characters(state);

  /* Changed from the original: Updated to use a switch statement rather than
   * if-else and gotos. */

  switch (state->room_index)
  {
  case room_2_hut2left:
    movableitem = &movable_items[movable_item_STOVE1];
    character   = character_26;
    break;

  case room_4_hut3left:
    movableitem = &movable_items[movable_item_STOVE2];
    character   = character_27;
    break;

  case room_9_crate:
    movableitem = &movable_items[movable_item_CRATE];
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
void setup_movable_item(tgestate_t    *state,
                        movableitem_t *movableitem,
                        character_t    character)
{
  /* $69A0 */
  static const uint8_t movable_item_reset_data[14] =
  {
    0x00,       /* .flags */
    0x00, 0x00, /* .target */
    0x00, 0x00, /* .w04 */
    0x00,       /* .b06 */
    0x00,       /* .b07 */
    0xF2, 0xCD, /* .w08 */
    0x76, 0xCF, /* .w0A */
    0x00,       /* .b0C */
    0x00,       /* .b0D */
    0x00        /* .b0E */
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

movableitem_t movable_items[movable_item__LIMIT] =
{
  { { 62, 35, 16, }, &sprites[sprite_STOVE], 0 },
  { { 55, 54, 14, }, &sprites[sprite_CRATE], 0 },
  { { 62, 35, 16, }, &sprites[sprite_STOVE], 0 },
};

/* ----------------------------------------------------------------------- */

/**
 * $69C9: Reset non-player visible characters.
 *
 * \param[in] state Pointer to game state.
 */
void reset_nonplayer_visible_characters(tgestate_t *state)
{
  vischar_t *HL;
  uint8_t    B;

  HL = &state->vischars[1];
  B = vischars_LENGTH - 1;
  do
    reset_visible_character(state, HL++);
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $69DC: Setup doors.
 *
 * \param[in] state Pointer to game state.
 */
void setup_doors(tgestate_t *state)
{
  uint8_t         *DE;
  uint8_t          B;
  uint8_t          C;
  const doorpos_t *HLdash;
  uint8_t          Bdash;

  /* Wipe door_related[] with 0xFF. (Could alternatively use memset().) */
  DE = &state->door_related[3];
  B = 4;
  do
    *DE-- = 0xFF;
  while (--B);

  DE++; /* === DE = &door_related[0]; */
  B = state->room_index << 2;
  C = 0;
  HLdash = &door_positions[0];
  Bdash = NELEMS(door_positions);
  do
  {
    if ((HLdash->room_and_flags & 0xFC) == B) // this 0xFC mask needs a name (0x3F << 2)
      *DE++ = C ^ 0x80;
    C ^= 0x80;
    if (C >= 0)
      C++; // increment every two stops?
    HLdash++;
  }
  while (--Bdash);
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
  roomdef_t  HL;
  uint8_t   *DE;
  uint8_t    A;
  uint8_t    B;

  wipe_visible_tiles(state);

  HL = rooms_and_tunnels[state->room_index - 1];

  setup_doors(state);

  state->roomdef_bounds_index = *HL++;

  /* Copy boundaries into state. */
  DE = &state->roomdef_object_bounds[0];
  A = *DE = *HL; /* count of boundaries */
  if (A == 0) /* no boundaries */
  {
    HL++; /* skip to TBD */
  }
  else
  {
    /* copy boundaries and another byte */
    memcpy(DE, HL, A * 4 + 1); // 4 == sizeof boundaries (4 max), max of 17 bytes
    HL += A * 4 + 1;
  }

  /* Copy sevenbyte structs into state->indoor_mask_data. */
  DE = &state->indoor_mask_data[0];
  B = *DE++ = *HL++; /* count of TBDs */
  while (B--)
  {
    memcpy(DE, &stru_EA7C[*HL++], 7);
    DE += 7;
    *DE++ = 32; /* Fill in constant final member. */
  }

  /* Plot all objects (as tiles). */
  B = *HL++; /* Count of objects */
  while (B--)
  {
    expand_object(state,
                  HL[0], // object index,
                  &state->tile_buf[0] + HL[2] * state->columns + HL[1]); // HL[2] = row, HL[1] = column
    HL += 3;
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
 * \param[in]  index  Object index to expand. (was A)
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
 * $6B85: Room dimensions.
 */
const bounds_t roomdef_bounds[10] =
{
  { 0x42, 0x1A, 0x46, 0x16 },
  { 0x3E, 0x16, 0x3A, 0x1A },
  { 0x36, 0x1E, 0x42, 0x12 },
  { 0x3E, 0x1E, 0x3A, 0x22 },
  { 0x4A, 0x12, 0x3E, 0x1E },
  { 0x38, 0x32, 0x64, 0x0A },
  { 0x68, 0x06, 0x38, 0x32 },
  { 0x38, 0x32, 0x64, 0x1A },
  { 0x68, 0x1C, 0x38, 0x32 },
  { 0x38, 0x32, 0x58, 0x0A },
};

/* ----------------------------------------------------------------------- */

/**
 * $7612: Character structures.
 */
characterstruct_t character_structs[26] =
{
  { character_0,  room_11_papers,   {  46,  46, 24 }, 0x03, 0x00 },
  { character_1,  room_0_outdoors,  { 102,  68,  3 }, 0x01, 0x00 },
  { character_2,  room_0_outdoors,  {  68, 104,  3 }, 0x01, 0x02 },
  { character_3,  room_16_corridor, {  46,  46, 24 }, 0x03, 0x13 },
  { character_4,  room_0_outdoors,  {  61, 103,  3 }, 0x02, 0x04 },
  { character_5,  room_0_outdoors,  { 106,  56, 13 }, 0x00, 0x00 },
  { character_6,  room_0_outdoors,  {  72,  94, 13 }, 0x00, 0x00 },
  { character_7,  room_0_outdoors,  {  72,  70, 13 }, 0x00, 0x00 },
  { character_8,  room_0_outdoors,  {  80,  46, 13 }, 0x00, 0x00 },
  { character_9,  room_0_outdoors,  { 108,  71, 21 }, 0x04, 0x00 },
  { character_10, room_0_outdoors,  {  92,  52,  3 }, 0xFF, 0x38 },
  { character_11, room_0_outdoors,  { 109,  69,  3 }, 0x00, 0x00 },
  { character_12, room_3_hut2right, {  40,  60, 24 }, 0x00, 0x08 },
  { character_13, room_2_hut2left,  {  36,  48, 24 }, 0x00, 0x08 },
  { character_14, room_5_hut3right, {  40,  60, 24 }, 0x00, 0x10 },
  { character_15, room_5_hut3right, {  36,  34, 24 }, 0x00, 0x10 },
  { character_16, room_0_outdoors,  {  68,  84,  1 }, 0xFF, 0x00 },
  { character_17, room_0_outdoors,  {  68, 104,  1 }, 0xFF, 0x00 },
  { character_18, room_0_outdoors,  { 102,  68,  1 }, 0xFF, 0x18 },
  { character_19, room_0_outdoors,  {  88,  68,  1 }, 0xFF, 0x18 },
  
  { character_20, room_NONE,        {  52,  60, 24 }, 0x00, 0x08 }, // wake_up, breakfast_time
  { character_21, room_NONE,        {  52,  44, 24 }, 0x00, 0x08 }, // wake_up, breakfast_time
  { character_22, room_NONE,        {  52,  28, 24 }, 0x00, 0x08 }, // wake_up, breakfast_time
  
  { character_23, room_NONE,        {  52,  60, 24 }, 0x00, 0x10 }, // wake_up, breakfast_time
  { character_24, room_NONE,        {  52,  44, 24 }, 0x00, 0x10 }, // wake_up, breakfast_time
  { character_25, room_NONE,        {  52,  28, 24 }, 0x00, 0x10 }, // wake_up, breakfast_time
};

/* ----------------------------------------------------------------------- */

/**
 * $76C8: Item structs.
 */
itemstruct_t item_structs[item__LIMIT] =
{
  { item_WIRESNIPS,        room_NONE,        { 64, 32,  2 }, 0x78, 0xF4 },
  { item_SHOVEL,           room_9_crate,     { 62, 48,  0 }, 0x7C, 0xF2 },
  { item_LOCKPICK,         room_10_lockpick, { 73, 36, 16 }, 0x77, 0xF0 },
  { item_PAPERS,           room_11_papers,   { 42, 58,  4 }, 0x84, 0xF3 },
  { item_TORCH,            room_14_torch,    { 34, 24,  2 }, 0x7A, 0xF6 },
  { item_BRIBE,            room_NONE,        { 36, 44,  4 }, 0x7E, 0xF4 },
  { item_UNIFORM,          room_15_uniform,  { 44, 65, 16 }, 0x87, 0xF1 },
  { item_FOOD,             room_19_food,     { 64, 48, 16 }, 0x7E, 0xF0 },
  { item_POISON,           room_1_hut1right, { 66, 52,  4 }, 0x7C, 0xF1 },
  { item_RED_KEY,          room_22_redkey,   { 60, 42,  0 }, 0x7B, 0xF2 },
  { item_YELLOW_KEY,       room_11_papers,   { 28, 34,  0 }, 0x81, 0xF8 },
  { item_GREEN_KEY,        room_0_outdoors,  { 74, 72,  0 }, 0x7A, 0x6E },
  { item_RED_CROSS_PARCEL, room_NONE,        { 28, 50, 12 }, 0x85, 0xF6 },
  { item_RADIO,            room_18_radio,    { 36, 58,  8 }, 0x85, 0xF4 },
  { item_PURSE,            room_NONE,        { 36, 44,  4 }, 0x7E, 0xF4 },
  { item_COMPASS,          room_NONE,        { 52, 28,  4 }, 0x7E, 0xF4 },
};

/* ----------------------------------------------------------------------- */

/**
 * $783A: (unknown)
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

/* ----------------------------------------------------------------------- */

/**
 * $7A26: Door positions.
 */
const doorpos_t door_positions[124] =
{
#define BYTE0(room, other) ((room << 2) | other)
  { BYTE0(room_0_outdoors,           1), 0xB2, 0x8A,  6 },
  { BYTE0(room_0_outdoors,           3), 0xB2, 0x8E,  6 },
  { BYTE0(room_0_outdoors,           1), 0xB2, 0x7A,  6 },
  { BYTE0(room_0_outdoors,           3), 0xB2, 0x7E,  6 },
  { BYTE0(room_34,                   0), 0x8A, 0xB3,  6 },
  { BYTE0(room_0_outdoors,           2), 0x10, 0x34, 12 },
  { BYTE0(room_48,                   0), 0xCC, 0x79,  6 },
  { BYTE0(room_0_outdoors,           2), 0x10, 0x34, 12 },
  { BYTE0(room_28_hut1left,          1), 0xD9, 0xA3,  6 },
  { BYTE0(room_0_outdoors,           3), 0x2A, 0x1C, 24 },
  { BYTE0(room_1_hut1right,          0), 0xD4, 0xBD,  6 },
  { BYTE0(room_0_outdoors,           2), 0x1E, 0x2E, 24 },
  { BYTE0(room_2_hut2left,           1), 0xC1, 0xA3,  6 },
  { BYTE0(room_0_outdoors,           3), 0x2A, 0x1C, 24 },
  { BYTE0(room_3_hut2right,          0), 0xBC, 0xBD,  6 },
  { BYTE0(room_0_outdoors,           2), 0x20, 0x2E, 24 },
  { BYTE0(room_4_hut3left,           1), 0xA9, 0xA3,  6 },
  { BYTE0(room_0_outdoors,           3), 0x2A, 0x1C, 24 },
  { BYTE0(room_5_hut3right,          0), 0xA4, 0xBD,  6 },
  { BYTE0(room_0_outdoors,           2), 0x20, 0x2E, 24 },
  { BYTE0(room_21_corridor,          0), 0xFC, 0xCA,  6 },
  { BYTE0(room_0_outdoors,           2), 0x1C, 0x24, 24 },
  { BYTE0(room_20_redcross,          0), 0xFC, 0xDA,  6 },
  { BYTE0(room_0_outdoors,           2), 0x1A, 0x22, 24 },
  { BYTE0(room_15_uniform,           1), 0xF7, 0xE3,  6 },
  { BYTE0(room_0_outdoors,           3), 0x26, 0x19, 24 },
  { BYTE0(room_13_corridor,          1), 0xDF, 0xE3,  6 },
  { BYTE0(room_0_outdoors,           3), 0x2A, 0x1C, 24 },
  { BYTE0(room_8_corridor,           1), 0x97, 0xD3,  6 },
  { BYTE0(room_0_outdoors,           3), 0x2A, 0x15, 24 },
  { BYTE0(room_6,                    1), 0x00, 0x00,  0 },
  { BYTE0(room_0_outdoors,           3), 0x22, 0x22, 24 },
  { BYTE0(room_1_hut1right,          1), 0x2C, 0x34, 24 },
  { BYTE0(room_28_hut1left,          3), 0x26, 0x1A, 24 },
  { BYTE0(room_3_hut2right,          1), 0x24, 0x36, 24 },
  { BYTE0(room_2_hut2left,           3), 0x26, 0x1A, 24 },
  { BYTE0(room_5_hut3right,          1), 0x24, 0x36, 24 },
  { BYTE0(room_4_hut3left,           3), 0x26, 0x1A, 24 },
  { BYTE0(room_23_breakfast,         1), 0x28, 0x42, 24 },
  { BYTE0(room_25_breakfast,         3), 0x26, 0x18, 24 },
  { BYTE0(room_23_breakfast,         0), 0x3E, 0x24, 24 },
  { BYTE0(room_21_corridor,          2), 0x20, 0x2E, 24 },
  { BYTE0(room_19_food,              1), 0x22, 0x42, 24 },
  { BYTE0(room_23_breakfast,         3), 0x22, 0x1C, 24 },
  { BYTE0(room_18_radio,             1), 0x24, 0x36, 24 },
  { BYTE0(room_19_food,              3), 0x38, 0x22, 24 },
  { BYTE0(room_21_corridor,          1), 0x2C, 0x36, 24 },
  { BYTE0(room_22_redkey,            3), 0x22, 0x1C, 24 },
  { BYTE0(room_22_redkey,            1), 0x2C, 0x36, 24 },
  { BYTE0(room_24_solitary,          3), 0x2A, 0x26, 24 },
  { BYTE0(room_12_corridor,          1), 0x42, 0x3A, 24 },
  { BYTE0(room_18_radio,             3), 0x22, 0x1C, 24 },
  { BYTE0(room_17_corridor,          0), 0x3C, 0x24, 24 },
  { BYTE0(room_7_corridor,           2), 0x1C, 0x22, 24 },
  { BYTE0(room_15_uniform,           0), 0x40, 0x28, 24 },
  { BYTE0(room_14_torch,             2), 0x1E, 0x28, 24 },
  { BYTE0(room_16_corridor,          1), 0x22, 0x42, 24 },
  { BYTE0(room_14_torch,             3), 0x22, 0x1C, 24 },
  { BYTE0(room_16_corridor,          0), 0x3E, 0x2E, 24 },
  { BYTE0(room_13_corridor,          2), 0x1A, 0x22, 24 },
  { BYTE0(room_0_outdoors,           0), 0x44, 0x30, 24 },
  { BYTE0(room_0_outdoors,           2), 0x20, 0x30, 24 },
  { BYTE0(room_13_corridor,          0), 0x4A, 0x28, 24 },
  { BYTE0(room_11_papers,            2), 0x1A, 0x22, 24 },
  { BYTE0(room_7_corridor,           0), 0x40, 0x24, 24 },
  { BYTE0(room_16_corridor,          2), 0x1A, 0x22, 24 },
  { BYTE0(room_10_lockpick,          0), 0x36, 0x35, 24 },
  { BYTE0(room_8_corridor,           2), 0x17, 0x26, 24 },
  { BYTE0(room_9_crate,              0), 0x36, 0x1C, 24 },
  { BYTE0(room_8_corridor,           2), 0x1A, 0x22, 24 },
  { BYTE0(room_12_corridor,          0), 0x3E, 0x24, 24 },
  { BYTE0(room_17_corridor,          2), 0x1A, 0x22, 24 },
  { BYTE0(room_29_secondtunnelstart, 1), 0x36, 0x36, 24 },
  { BYTE0(room_9_crate,              3), 0x38, 0x0A, 12 },
  { BYTE0(room_52,                   1), 0x38, 0x62, 12 },
  { BYTE0(room_30,                   3), 0x38, 0x0A, 12 },
  { BYTE0(room_30,                   0), 0x64, 0x34, 12 },
  { BYTE0(room_31,                   2), 0x38, 0x26, 12 },
  { BYTE0(room_30,                   1), 0x38, 0x62, 12 },
  { BYTE0(room_36,                   3), 0x38, 0x0A, 12 },
  { BYTE0(room_31,                   0), 0x64, 0x34, 12 },
  { BYTE0(room_32,                   2), 0x0A, 0x34, 12 },
  { BYTE0(room_32,                   1), 0x38, 0x62, 12 },
  { BYTE0(room_33,                   3), 0x20, 0x34, 12 },
  { BYTE0(room_33,                   1), 0x40, 0x34, 12 },
  { BYTE0(room_35,                   3), 0x38, 0x0A, 12 },
  { BYTE0(room_35,                   0), 0x64, 0x34, 12 },
  { BYTE0(room_34,                   2), 0x0A, 0x34, 12 },
  { BYTE0(room_36,                   0), 0x64, 0x34, 12 },
  { BYTE0(room_35,                   2), 0x38, 0x1C, 12 },
  { BYTE0(room_37,                   0), 0x3E, 0x22, 24 }, /* Tunnel entrance */
  { BYTE0(room_2_hut2left,           2), 0x10, 0x34, 12 },
  { BYTE0(room_38,                   0), 0x64, 0x34, 12 },
  { BYTE0(room_37,                   2), 0x10, 0x34, 12 },
  { BYTE0(room_39,                   1), 0x40, 0x34, 12 },
  { BYTE0(room_38,                   3), 0x20, 0x34, 12 },
  { BYTE0(room_40,                   0), 0x64, 0x34, 12 },
  { BYTE0(room_38,                   2), 0x38, 0x54, 12 },
  { BYTE0(room_40,                   1), 0x38, 0x62, 12 },
  { BYTE0(room_41,                   3), 0x38, 0x0A, 12 },
  { BYTE0(room_41,                   0), 0x64, 0x34, 12 },
  { BYTE0(room_42,                   2), 0x38, 0x26, 12 },
  { BYTE0(room_41,                   1), 0x38, 0x62, 12 },
  { BYTE0(room_45,                   3), 0x38, 0x0A, 12 },
  { BYTE0(room_45,                   0), 0x64, 0x34, 12 },
  { BYTE0(room_44,                   2), 0x38, 0x1C, 12 },
  { BYTE0(room_43,                   1), 0x20, 0x34, 12 },
  { BYTE0(room_44,                   3), 0x38, 0x0A, 12 },
  { BYTE0(room_42,                   1), 0x38, 0x62, 12 },
  { BYTE0(room_43,                   3), 0x20, 0x34, 12 },
  { BYTE0(room_46,                   0), 0x64, 0x34, 12 },
  { BYTE0(room_39,                   2), 0x38, 0x1C, 12 },
  { BYTE0(room_47,                   1), 0x38, 0x62, 12 },
  { BYTE0(room_46,                   3), 0x20, 0x34, 12 },
  { BYTE0(room_50_blocked_tunnel,    0), 0x64, 0x34, 12 },
  { BYTE0(room_47,                   2), 0x38, 0x56, 12 },
  { BYTE0(room_50_blocked_tunnel,    1), 0x38, 0x62, 12 },
  { BYTE0(room_49,                   3), 0x38, 0x0A, 12 },
  { BYTE0(room_49,                   0), 0x64, 0x34, 12 },
  { BYTE0(room_48,                   2), 0x38, 0x1C, 12 },
  { BYTE0(room_51,                   1), 0x38, 0x62, 12 },
  { BYTE0(room_29_secondtunnelstart, 3), 0x20, 0x34, 12 },
  { BYTE0(room_52,                   0), 0x64, 0x34, 12 },
  { BYTE0(room_51,                   2), 0x38, 0x54, 12 },
#undef BYTE0
};

/* ----------------------------------------------------------------------- */

/**
 * $7AC9: Check for 'pick up' keypress.
 *
 * \param[in] state Pointer to game state.
 */
void check_for_pick_up_keypress(tgestate_t *state, input_t input)
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
  if (item == item_NONE)
    return;

  memcpy(&state->saved_pos, &state->vischars[0].mi.pos, sizeof(pos_t));

  item_actions_jump_table[item](state);
}

/**
 * $7B16: Item actions jump table.
 */
const item_action_t item_actions_jump_table[item__LIMIT] =
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

  if (state->room_index == room_0_outdoors)
  {
    /* Outdoors. */
    setup_exterior(state);
  }
  else
  {
    /* Indoors. */
    setup_room(state);
    plot_interior_tiles(state);
    attrs = choose_game_window_attributes(state);
    set_game_screen_attributes(state, attrs);
  }

  if ((item->item & itemstruct_ITEM_FLAG_HELD) == 0)
  {
    /* Have picked up an item not previously held. */
    item->item |= itemstruct_ITEM_FLAG_HELD;
    increase_morale_by_5_score_by_5(state);
  }

  item->room = 0;
  item->t1   = 0;
  item->t2   = 0;

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
  item_t      item;
  item_t     *itemp;
  item_t      tmp;
  attribute_t attrs;

  /* Drop the first item. */
  item = state->items_held[0];
  if (item == item_NONE)
    return;

  if (item == item_UNIFORM)
    state->vischars[0].mi.spriteset = &sprites[sprite_PRISONER_FACING_TOP_LEFT_4];

  /* Shuffle items down. */
  itemp = &state->items_held[1];
  tmp = *itemp;
  *itemp-- = item_NONE;
  *itemp = tmp;

  draw_all_items(state);
  play_speaker(state, sound_DROP_ITEM);
  attrs = choose_game_window_attributes(state);
  set_game_screen_attributes(state, attrs);

  drop_item_A(state, item);
}

/**
 * $7BB5: Drop item. Part "A".
 *
 * \param[in] state Pointer to game state.
 */
void drop_item_A(tgestate_t *state, item_t item)
{
  itemstruct_t *is;
  room_t        room;
  tinypos_t    *DE;
  pos_t        *HL;

  is = item_to_itemstruct(item);
  room = state->room_index;
  is->room = room; /* Set object's room index. */
  if (room == room_0_outdoors)
  {
    /* Outdoors. */

    DE = &is->pos;
    HL = &state->vischars[0].mi.pos;
    scale_pos(HL, DE);
    DE->vo = 0;

    drop_item_A_exterior_tail(is);
  }
  else
  {
    /* Indoors. */

    DE = &is->pos;
    HL = &state->vischars[0].mi.pos;
    DE->y = HL->y;
    DE->x = HL->x;
    DE->vo = 5;

    // This was a call to divide_by_8_with_rounding, but that expects
    // separate hi and lo arguments, which is not worth the effort of
    // mimicing the original code.

    // This needs to go somewhere more general.
#define divround(x) ((x + 4) >> 3)

    drop_item_A_interior_tail(is);
  }
}

/**
 * $7BD0: (unknown)
 *
 * Moved out to provide entry point.
 *
 * \param[in] DE Pointer to item struct.
 */
void drop_item_A_exterior_tail(itemstruct_t *HL)
{
  HL->t1 = (0x40 + HL->pos.x - HL->pos.y) * 2;
  HL->t2 = 0 - HL->pos.y - HL->pos.x - HL->pos.vo;
}

/**
 * $7BF2: (unknown)
 *
 * Moved out to provide entry point.
 *
 * \param[in] DE Pointer to item struct.
 */
void drop_item_A_interior_tail(itemstruct_t *HL)
{
  HL->t1 = divround((0x200 + HL->pos.x - HL->pos.y) * 2);
  HL->t2 = divround(0x800 - HL->pos.y - HL->pos.x - HL->pos.vo);
}

/* ----------------------------------------------------------------------- */

/**
 * $7C26: Convert an item_t to an itemstruct pointer.
 *
 * \param[in] item Item index.
 *
 * \return Pointer to itemstruct.
 */
itemstruct_t *item_to_itemstruct(item_t item)
{
  return &item_structs[item];
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
  uint8_t       radius;
  uint8_t       iters;
  itemstruct_t *is;

  /* Select a pick up radius. */
  radius = 1; /* outdoors */
  if (state->room_index > 0)
    radius = 6; /* Indoors. */

  iters = item__LIMIT;
  is    = &item_structs[0];
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
 * $7CBE: Plot a bitmap without masking.
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
  uint8_t  w;
  uint8_t *tdst;

  do
  {
    w = width;
    tdst = dst;
    do
      *tdst++ = *src++;
    while (--w);
    dst = get_next_scanline(state, dst);
  }
  while (--height);
}

/* ----------------------------------------------------------------------- */

/**
 * $7CD4: Wipe the screen.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] width  Width, in bytes.
 * \param[in] height Height.
 * \param[in] dst    Destination address.
 */
void screen_wipe(tgestate_t *state,
                 uint8_t     width,
                 uint8_t     height,
                 uint8_t    *dst)
{
  uint8_t  w;
  uint8_t *tdst;

  do
  {
    w = width;
    tdst = dst;
    do
      *tdst++ = 0;
    while (--w);
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
 * \param[in] slp   Scanline pointer.
 *
 * \return Subsequent scanline pointer.
 */
uint8_t *get_next_scanline(tgestate_t *state, uint8_t *slp)
{
  uint8_t *const screen = &state->speccy->screen[0];

  uint16_t HL;
  uint16_t DE;

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
 * Conversion note: The original code accepts BC as the message index.
 * However all-but-one of the callers only setup B, not BC. We therefore
 * ignore the second argument here, treating it as zero.
 *
 * \param[in] state         Pointer to game state.
 * \param[in] message_index message_t to display.
 */
void queue_message_for_display(tgestate_t *state,
                               message_t   message_index)
{
  uint8_t *qp;

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
 * \param[in] output     Where to plot. (was DE)
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
 * \param[in] output    Where to plot. (was DE)
 *
 * \return Pointer to next character along.
 */
uint8_t *plot_single_glyph(int character, uint8_t *output)
{
  const tile_t    *glyph;
  const tilerow_t *row;
  int              iters;

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
  uint8_t     A;
  const char *HL;
  uint8_t    *DE;

  if (state->message_display_counter > 0)
  {
    state->message_display_counter--;
    return;
  }

  A = state->message_display_index;
  if (A == 128)
  {
    next_message(state);
  }
  else if (A > 128)
  {
    wipe_message(state);
  }
  else
  {
    HL = state->current_message_character;
    DE = &state->speccy->screen[screen_text_start_address + A];
    (void) plot_glyph(HL, DE);
    state->message_display_index = (intptr_t) DE & 31; // dodgy
    A = *++HL;
    if (A == 0xFF) /* end of string */
    {
      /* Leave the message for 31 turns, then wipe it. */
      state->message_display_counter = 31;
      state->message_display_index |= 1 << 7;
    }
    else
    {
      state->current_message_character = HL;
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
  int      index;
  uint8_t *DE;

  index = state->message_display_index;
  state->message_display_index = --index;
  DE = &state->speccy->screen[screen_text_start_address + index];
  (void) plot_single_glyph(' ', DE); /* plot a SPACE character */
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
  uint8_t    *DE;
  const char *message;

  DE = &state->message_queue[0];
  if (state->message_queue_pointer == DE)
    return;

  /* The message ID is stored in the buffer itself. */
  message = messages_table[*DE];

  state->current_message_character = message;
  memmove(state->message_queue, state->message_queue + 2, 16);
  state->message_queue_pointer -= 2;
  state->message_display_index = 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $7DCD: Table of game messages.
 *
 * These are 0xFF terminated in the original game.
 */
/* Note: I intended to make this static and have a forward reference to it,
 * but you can't do that: it must be 'extern'. */
const char *messages_table[message__LIMIT] =
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
    plot_game_screen(state);
    ring_bell(state);
    if (state->day_or_night)
      nighttime(state);
    if (state->room_index)
      interior_delay_loop();
    wave_morale_flag(state);
    if ((state->game_counter & 63) == 0)
      dispatch_timed_event(state);
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
  state->morale_2 = 0xFF; // inhibit user input
  state->automatic_player_counter = 0; // immediately take automatic control of player
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
  if (state->speccy->in(state->speccy, port_KEYBOARD_SPACESYMSHFTMNB) & 0x01)
    return; /* space not pressed */

  if (state->speccy->in(state->speccy, port_KEYBOARD_SHIFTZXCV)       & 0x01)
    return; /* shift not pressed */

  screen_reset(state);
  if (user_confirm(state) == 0)
    reset_game(state);

  if (state->room_index == 0)
  {
    reset_B2FC(state);
  }
  else
  {
    enter_room(state); // doesn't return (jumps to main_loop)
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
  input_t A;

  if (state->morale_1 || state->morale_2)
    return;

  if (state->vischars[0].flags & (vischar_BYTE1_PICKING_LOCK | vischar_BYTE1_CUTTING_WIRE))
  {
    /* Picking a lock, or cutting wire fence */

    state->automatic_player_counter = 31; /* 31 turns until automatic control */

    if (state->vischars[0].flags == vischar_BYTE1_PICKING_LOCK)
      picking_a_lock(state);
    else
      snipping_wire(state);
    return;
  }

  A = input_routine(state);
  if (A == input_NONE)
  {
    /* No input */

    if (state->automatic_player_counter == 0)
      return;

    state->automatic_player_counter--; /* no user input: count down */
    A = 0;
  }
  else
  {
    /* Input received */

    state->automatic_player_counter = 31; /* wait 31 turns until automatic control */

    if (state->player_in_bed == 0)
    {
      if (state->player_in_breakfast)
        goto not_breakfast;

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
      state->vischars[0].w04       = 0x2E2E;
      state->vischars[0].mi.pos.y  = 0x2E;
      state->vischars[0].mi.pos.x  = 0x2E;
      state->vischars[0].mi.pos.vo = 24;
      roomdef_2_hut2_left[roomdef_2_BED] = interiorobject_EMPTY_BED;
      state->player_in_bed = 0;
    }

    setup_room(state);
    plot_interior_tiles(state);

not_breakfast:
    if (A >= input_FIRE)
    {
      check_for_pick_up_keypress(state, A);
      A = 0x80;
    }
  }

  if (state->vischars[0].b0D == A)
    return;

  state->vischars[0].b0D = A | 0x80;
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
  uint8_t A;

  A = state->player_locked_out_until - state->game_counter;
  if (A)
  {
    if (A < 4)
      state->vischars[0].b0D = table_9EE0[state->vischars[0].b0E & 3]; // new direction?
  }
  else
  {
    state->vischars[0].b0E = A & 3; // walk/crawl flag?
    state->vischars[0].b0D = 0x80;
    state->vischars[0].mi.pos.vo = 24; /* set vertical offset */

    /* The original code jumped to tail end of picking_a_lock to do this. */
    state->vischars[0].flags &= ~(vischar_BYTE1_PICKING_LOCK | vischar_BYTE1_CUTTING_WIRE);
  }
}

const uint8_t table_9EE0[4] =
{
  0x84, 0x87, 0x88, 0x85
};

/* ----------------------------------------------------------------------- */

/**
 * $9F21: In permitted area.
 *
 * \param[in] state Pointer to game state.
 */
void in_permitted_area(tgestate_t *state)
{
  /**
   * $9EF9 +
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
   * $????
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
  
  pos_t       *vcpos;
  tinypos_t   *pos;
  attribute_t  attr;
  uint8_t      red_flag;

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

  /* Red flag if picking a lock, or cutting wire */

  if (state->vischars[0].flags & (vischar_BYTE1_PICKING_LOCK | vischar_BYTE1_CUTTING_WIRE))
    goto set_flag_red;

  /* Green flag at night, when in room. Red otherwise. */

  if (state->clock >= 100) /* night time */
  {
    if (state->room_index == room_2_hut2left)
      goto set_flag_green;
    else
      goto set_flag_red;
  }

  if (state->morale_1)
    goto set_flag_green;

#if 0
  location_t *locptr;
  uint8_t     A, C;
  uint8_t     D, E;

  locptr = &state->vischars[0].target;
  A = *locptr++;
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
    B = NELEMS(byte_to_pointer);
    do
    {
      if (A == tab->byte)
        goto found;
      tab++;
    }
    while (--B);
    goto set_flag_green;

found:
    DE = tab->pointer;
    HL = DE;
    BC &= 0x00FF;
    HL += BC;
    A = *HL;
    HL = DE; // was interleaved
    if (in_permitted_area_end_bit(state, A) == 0)
      goto set_flag_green;

    A = state->vischars[0].target; // lo byte
    if (A & vischar_BYTE2_BIT7)
      HL++;
    BC = 0;
    for (;;)
    {
      PUSH BC
      HL += BC;
      A = *HL;
      if (A == 0xFF)
        goto pop_and_set_flag_red; // hit end of list
      POP HL, BC // was interleaved
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
  room_t        *HL;
  tinypos_t     *DE;
  uint8_t        A;
  uint8_t        B;

  HL = &state->room_index;

  if (room_and_flags & (1 << 7))
    return *HL == (room_and_flags & 0x7F); // return with flags

  if (*HL)
    return 0; /* Indoors. */

  DE = &state->player_map_position;
  
  return in_permitted_area_end_bit_2(state, 0, DE);
}

/**
 * $A01A: In permitted area (end bit 2).
 *
 * \param[in] state          Pointer to game state.
 * \param[in] A              ...
 *
 * \return 0 if in permitted? area, 1 otherwise.
 */
int in_permitted_area_end_bit_2(tgestate_t *state, uint8_t A, tinypos_t *DE)
{
  // These are probably pairs of lo-hi bounds.
  static const uint8_t byte_9F15[12] =
  {
    0x56, 0x5E, 0x3D, 0x48,
    0x4E, 0x84, 0x47, 0x74,
    0x4F, 0x69, 0x2F, 0x3F
  };
  const uint8_t *HL2;

  // another entry point: (A is index 0..2)
  HL2 = &byte_9F15[A * 4];
  return DE->y < HL2[0] || DE->y >= HL2[1] ||
         DE->x < HL2[2] || DE->x >= HL2[3];
}

/* ----------------------------------------------------------------------- */

/**
 * $A035: Wave the morale flag.
 *
 * \param[in] state Pointer to game state.
 */
void wave_morale_flag(tgestate_t *state)
{
  uint8_t       *pgame_counter;
  uint8_t        A;
  uint8_t       *HL;
  const uint8_t *flag_bitmap;

  pgame_counter = &state->game_counter;
  (*pgame_counter)++;

  /* Wave the flag on every other turn */
  if (*pgame_counter & 1)
    return;

  A = state->morale;
  HL = &state->displayed_morale;
  if (A != *HL)
  {
    if (A < *HL)
    {
      /* Decreasing morale */
      (*HL)--;
      HL = get_next_scanline(state, state->moraleflag_screen_address);
    }
    else
    {
      /* Increasing morale */
      (*HL)++;
      HL = get_prev_scanline(state, state->moraleflag_screen_address);
    }
    state->moraleflag_screen_address = HL;
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
  uint8_t *HL;
  int      B;

  HL = &state->speccy->attributes[morale_flag_attributes_offset];
  B = 19; /* height of flag */
  do
  {
    HL[0] = attrs;
    HL[1] = attrs;
    HL += state->width; // stride (== attributes width)
  }
  while (--B);
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
  volatile int BC = 0xFFF;
  while (--BC)
    ;
}

/* ----------------------------------------------------------------------- */

/* Offset. */
#define screenaddr_bell_ringer 0x118E

/**
 * $A09E: Ring bell.
 *
 * \param[in] state Pointer to game state.
 */
void ring_bell(tgestate_t *state)
{
  uint8_t *pbell;
  uint8_t  A;

  pbell = &state->bell;
  A = *pbell;
  if (A == bell_STOP)
    return; /* not ringing */

  if (A != bell_RING_PERPETUAL)
  {
    /* Decrement the ring counter. */
    *pbell = --A;
    if (A == 0)
    {
      *pbell = bell_STOP; /* counter hit zero - stop ringing */
      return;
    }
  }

  A = state->speccy->screen[screenaddr_bell_ringer]; /* fetch visible state of bell */
  if (A != 0x3F) /* pixel value is 0x3F if on */
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
 * \param[in] src   Source bitmap.
 */
void plot_ringer(tgestate_t *state, const uint8_t *src)
{
  plot_bitmap(state,
              1, 12, /* dimensions: 8 x 12 */
              src,
              &state->speccy->screen[screenaddr_bell_ringer]);
}

/* ----------------------------------------------------------------------- */

/**
 * $A0D2: Increase morale.
 *
 * \param[in] state Pointer to game state.
 * \param[in] delta Amount to increase morale by.
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
 * \param[in] delta Amount to decrease morale by.
 */
void decrease_morale(tgestate_t *state, uint8_t delta)
{
  uint8_t decreased_morale;

  decreased_morale = state->morale - delta;
  if (decreased_morale < morale_MIN)
    decreased_morale = morale_MIN;

  // this goto's into the tail end of increase_morale in the original code
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
 * \param[in] delta Amount to increase score by.
 */
void increase_score(tgestate_t *state, uint8_t delta)
{
  char *pdigit;

  pdigit = &state->score_digits[4];
  do
  {
    char *tmp;

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
  char    *pdigit;
  uint8_t *pscr;
  uint8_t  B;

  pdigit = &state->score_digits[0];
  pscr = &state->speccy->screen[score_address];
  B = NELEMS(state->score_digits);
  do
    (void) plot_glyph(pdigit++, pscr++);
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $A11D: Play the speaker.
 *
 * \param[in] state Pointer to game state.
 * \param[in] sound Number of iterations to play for (hi). Delay inbetween each iteration (lo).
 */
void play_speaker(tgestate_t *state, sound_t sound)
{
  uint8_t iters;
  uint8_t delay;
  uint8_t A;
  uint8_t C;

  iters = sound >> 8;
  delay = sound & 0xFF;

  A = 16;
  do
  {
    state->speccy->out(state->speccy, port_BORDER, A); /* Play. */
    C = delay;
    while (C--)
      ;
    A ^= 16; /* Toggle speaker bit. */
  }
  while (--iters);
}

/* ----------------------------------------------------------------------- */

/**
 * $A147: Bell ringer bitmaps.
 */
const uint8_t bell_ringer_bitmap_off[] =
{
  0xE7, 0xE7, 0x83, 0x83, 0x43, 0x41, 0x20, 0x10, 0x08, 0x04, 0x02, 0x02
};

const uint8_t bell_ringer_bitmap_on[] =
{
  0x3F, 0x3F, 0x27, 0x13, 0x13, 0x09, 0x08, 0x04, 0x04, 0x02, 0x02, 0x01
};

/* ----------------------------------------------------------------------- */

/**
 * $A15F: Set game screen attributes.
 *
 * \param[in] state Pointer to game state.
 * \param[in] attrs Screen attributes.
 */
void set_game_screen_attributes(tgestate_t *state, attribute_t attrs)
{
  memset(&state->speccy->attributes[0x047], attrs, 23 * 16);
}

/* ----------------------------------------------------------------------- */

/**
 * $A173: Timed events.
 */
const timedevent_t timed_events[15] =
{
  {   0, &event_another_day_dawns },
  {   8, &event_wake_up },
  {  12, &event_new_red_cross_parcel },
  {  16, &event_go_to_roll_call },
  {  20, &event_roll_call },
  {  21, &event_go_to_breakfast_time },
  {  36, &event_breakfast_time },
  {  46, &event_go_to_exercise_time },
  {  64, &event_exercise_time },
  {  74, &event_go_to_roll_call },
  {  78, &event_roll_call },
  {  79, &event_go_to_time_for_bed },
  {  98, &event_time_for_bed },
  { 100, &event_night_time },
  { 130, &event_search_light },
};

/* ----------------------------------------------------------------------- */

/**
 * $A1A0: Dispatch timed events.
 *
 * \param[in] state Pointer to game state.
 */
void dispatch_timed_event(tgestate_t *state)
{
  uint8_t            *pcounter;
  uint8_t             time;
  const timedevent_t *event;
  uint8_t             B;

  /* Increment the clock, wrapping at 140. */
  pcounter = &state->clock;
  time = *pcounter + 1;
  if (time == 140)
    time = 0;
  *pcounter = time;

  /* Dispatch the event for that time. */
  event = &timed_events[0];
  B  = NELEMS(timed_events);
  do
  {
    if (time == event->time)
      goto found;
    event++;
  }
  while (--B);
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

void set_day_or_night(tgestate_t *state, uint8_t A)
{
  attribute_t attrs;

  state->day_or_night = A; // night=0xFF, day=0x00
  attrs = choose_game_window_attributes(state);
  set_game_screen_attributes(state, attrs);
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
  const item_t *DE;
  uint8_t       B;
  uint8_t       A;
  itemstruct_t *HL;

  /* Don't deliver a new red cross parcel while the previous one still
   * exists. */
  if ((item_structs[item_RED_CROSS_PARCEL].room & itemstruct_ROOM_MASK) != itemstruct_ROOM_MASK)
    return;

  /* Select the next parcel contents -- the first item from the list which
   * does not exist. */
  DE = &red_cross_parcel_contents_list[0];
  B = NELEMS(red_cross_parcel_contents_list);
  do
  {
    A = *DE;
    HL = item_to_itemstruct(A);
    if ((HL->room & itemstruct_ROOM_MASK) == itemstruct_ROOM_MASK)
      goto found;
    DE++;
  }
  while (--B);

  return;

found:
  state->red_cross_parcel_current_contents = *DE;
  memcpy(&item_structs[item_RED_CROSS_PARCEL].room, red_cross_parcel_reset_data, 6);
  queue_message_for_display(state, message_RED_CROSS_PARCEL);
}

const uint8_t red_cross_parcel_reset_data[6] =
{
  item_RED_CROSS_PARCEL, 44, 44, 0x0C, 0x80, 0xF4
};

const item_t red_cross_parcel_contents_list[4] =
{
  item_PURSE,
  item_WIRESNIPS,
  item_BRIBE,
  item_COMPASS,
};

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
  uint8_t B;

  Adash = 12;
  B = 4; // 4 iterations
  do
  {
    // PUSH AF
    sub_A38C(state, Adash);
    // POP AF
    Adash++;
    A++;
  }
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $A27F: tenlong.
 *
 * Likely a list of character indexes.
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
  characterstruct_t *HL;
  uint8_t            B;
  uint8_t *const    *bedpp;
  uint8_t            Adash;

  if (state->player_in_bed)
  {
    state->vischars[0].mi.pos.y = 46;
    state->vischars[0].mi.pos.x = 46;
  }

  state->player_in_bed = 0;
  set_player_target_location(state, location_2A00);

  HL = &character_structs[20];
  B = 3;
  do
  {
    HL->room = room_3_hut2right;
    HL++;
  }
  while (--B);
  B = 3;
  do
  {
    HL->room = room_5_hut3right;
    HL++;
  }
  while (--B);

  Adash = 5; // incremented by sub_A373
  // BC = 0;
  sub_A373(state, &Adash);

  /* Update all the bed objects to be empty. */
  bedpp = &beds[0];
  B = beds_LENGTH;
  do
    **bedpp++ = interiorobject_EMPTY_BED;
  while (--B);

  /* Update the player's bed object to be empty. */
  roomdef_2_hut2_left[roomdef_2_BED] = interiorobject_EMPTY_BED;
  if (state->room_index == room_0_outdoors || state->room_index >= room_6)
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
  characterstruct_t *HL;
  uint8_t            B;
  uint8_t            Adash;

  if (state->player_in_breakfast)
  {
    state->vischars[0].mi.pos.y = 52;
    state->vischars[0].mi.pos.x = 62;
  }

  state->player_in_breakfast = 0;
  set_player_target_location(state, location_9003);

  HL = &character_structs[20];
  B = 3;
  do
  {
    HL->room = room_25_breakfast;
    HL++;
  }
  while (--B);
  B = 3;
  do
  {
    HL->room = room_23_breakfast;
    HL++;
  }
  while (--B);

  Adash = 144;
  // C = 3; // B already 0
  sub_A373(state, &Adash);

  /* Update all the benches to be empty. */
  roomdef_23_breakfast[roomdef_23_BENCH_A] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_B] = interiorobject_EMPTY_BENCH;
  roomdef_23_breakfast[roomdef_23_BENCH_C] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_D] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_E] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_F] = interiorobject_EMPTY_BENCH;
  roomdef_25_breakfast[roomdef_25_BENCH_G] = interiorobject_EMPTY_BENCH;

  if (state->room_index == 0 || state->room_index >= room_29_secondtunnelstart)
    return;

  setup_room(state);
  plot_interior_tiles(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $A33F: Set player target location.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] location Location.
 */
void set_player_target_location(tgestate_t *state, location_t location)
{
  if (state->morale_1)
    return;

  state->vischars[0].character &= ~vischar_BYTE1_BIT6;
  state->vischars[0].target = location;
  sub_A3BB(state);
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

  set_player_target_location(state, location_8502);
  Adash = 133;
  // C = 2;
  sub_A373(state, &Adash);
}

/* ----------------------------------------------------------------------- */

/**
 * $A35F: sub_A35F.
 *
 * \param[in]     state   Pointer to game state.
 * \param[in,out] counter Counter incremented.
 */
void sub_A35F(tgestate_t *state, uint8_t *counter)
{
  uint8_t            Adash;
  const character_t *HL;
  uint8_t            B;
  uint8_t            A;

  Adash = *counter; // additional: keep a local copy of counter

  HL = &tenlong[0];
  B = 10;
  do
  {
    // PUSH HL
    // PUSH BC
    A = *HL;
    sub_A38C(state, A);
    Adash++;
    // POP BC
    // POP HL
    HL++;
  }
  while (--B);

  *counter = Adash;
}

/* ----------------------------------------------------------------------- */

/**
 * $A373: Called by the set_location routines.
 *
 * \param[in] state Pointer to game state.
 */
void sub_A373(tgestate_t *state, uint8_t *counter)
{
  uint8_t            Adash;
  const character_t *HL;
  uint8_t            B;
  uint8_t            A;

  Adash = *counter; // additional: keep a local copy of counter

  HL = &tenlong[0];
  B = 10;
  do
  {
    // PUSH HL
    // PUSH BC
    A = *HL;
    sub_A38C(state, A);
    // POP BC
    if (B == 6)
      Adash++; // 6 is which character?
    // POP HL
    HL++;
  }
  while (--B);

  *counter = Adash;
}

/* ----------------------------------------------------------------------- */

/**
 * $A38C: sub_A38C.
 *
 * \param[in] state Pointer to game state.
 */
void sub_A38C(tgestate_t *state, uint8_t A)
{
  characterstruct_t *HL;
  vischar_t         *vc;
  uint8_t            B;

  HL = get_character_struct(A);
  if ((HL->character & characterstruct_BYTE0_BIT6) == 0)
    goto not_set;

  // PUSH BC

  A = HL->character & characterstruct_BYTE0_MASK;
  B = vischars_LENGTH - 1;
  vc = &state->vischars[1]; /* iterate over non-player characters */
  do
  {
    if (A == HL->character)
      goto found;
    vc++;
  }
  while (--B);

  // POP BC

  goto exit;

not_set:
  //store_banked_A_then_C_at_HL(Adash, C, &vc->w04); // supposed to be +5

exit:
  return;

found:
  // POP BC
  vc->flags &= ~vischar_BYTE1_BIT6;

  sub_A3BB(state);
}

/**
 * $A3BB: sub_A3BB.
 *
 * \param[in] HL
 */
void sub_A3BB(tgestate_t *state)
{
  state->byte_A13E = 0;
#if 0
  // PUSH BC
  // PUSH HL
  HL--;
  A = sub_C651(state, HL);
  // POP DE // DE = the HL stored at $A3C0
  DE++;
  // LDI // *DE++ = *HL++; BC--;
  // LDI // *DE++ = *HL++; BC--;
  if (A == 255)
  {
    DE -= 6;
    IY = DE;
    // EX DE,HL
    HL += 2;
    sub_CB23(state, HL);
    // POP BC // could have just ended the block here
    return;
  }
  else if (A == 128)
  {
    DE -= 5;
    // EX DE,HL
    HL->flags |= vischar_BYTE1_BIT6; // sample = HL = $8001
  }
  // POP B
#endif
}

/* ----------------------------------------------------------------------- */

/**
 * $A3ED: Store banked A then C at HL.
 *
 * \param[in]  Adash
 * \param[in]  C
 * \param[out] HL
 */
void store_banked_A_then_C_at_HL(uint8_t Adash, uint8_t C, uint8_t *HL)
{
  *HL++ = Adash;
  *HL   = C;
}

/* ----------------------------------------------------------------------- */

/**
 * $A3F3: byte_A13E is non-zero.
 *
 * \param[in] state Pointer to game state.
 */
void byte_A13E_is_nonzero(tgestate_t         *state,
                          characterstruct_t *HL,
                          vischar_t          *IY)
{
  sub_A404(state, HL, IY, state->character_index);
}

/**
 * $A3F8: byte_A13E is zero.
 *
 * \param[in] state Pointer to game state.
 */
void byte_A13E_is_zero(tgestate_t         *state,
                       characterstruct_t *HL,
                       vischar_t          *IY)
{
  uint8_t character;

  character = IY->character;
  if (character == 0)
  {
    set_player_target_location(state, location_2C00);
    return;
  }

  sub_A404(state, HL, IY, character);
}

/**
 * $A404: Common end of above two routines.
 *
 * \param[in] state Pointer to game state.
 */
void sub_A404(tgestate_t         *state,
              characterstruct_t *HL,
              vischar_t          *IY,
              uint8_t            character_index)
{
  uint8_t old_character_index;

  HL->room = room_NONE;

  if (character_index > 19)
  {
    character_index -= 13;
  }
  else
  {
    old_character_index = character_index;
    character_index = 13;
    if (old_character_index & (1 << 0))
    {
      HL->room = room_1_hut1right;
      character_index |= 0x80;
    }
  }

  HL->character = character_index;
}

/* ----------------------------------------------------------------------- */

/**
 * $A420: Character sits.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] A        Character index.
 * \param[in] formerHL (unknown)
 */
void character_sits(tgestate_t *state, character_t A, uint8_t *formerHL)
{
  uint8_t  index;
  uint8_t *HL;
  uint8_t  C;

  // EX DE,HL
  index = A - 18;
  // first three characters
  HL = &roomdef_25_breakfast[roomdef_25_BENCH_D];
  if (index >= 3) // character_21
  {
    // second three characters
    HL = &roomdef_23_breakfast[roomdef_23_BENCH_A];
    index -= 3;
  }
  /* Poke object. */
  HL += index * 3;
  *HL = interiorobject_PRISONER_SAT_DOWN_MID_TABLE;

  C = room_25_breakfast;
  if (A >= character_21)
    C = room_23_breakfast;
  character_sit_sleep_common(state, C, formerHL);
}

/**
 * $A444: Character sleeps.
 *
 * \param[in] state    Pointer to game state.
 * \param[in] A        Character index.
 * \param[in] formerHL (unknown)
 */
void character_sleeps(tgestate_t *state, character_t A, uint8_t *formerHL)
{
  uint8_t C;

  // EX DE,HL
  /* Poke object. */
  *beds[A - 7] = interiorobject_OCCUPIED_BED;
  if (A < character_10)
    C = room_3_hut2right;
  else
    C = room_5_hut3right;
  character_sit_sleep_common(state, C, formerHL);
}

/**
 * $A462: Common end of character sits/sleeps.
 *
 * \param[in] state Pointer to game state.
 * \param[in] C     Character index.
 * \param[in] HL    Likely a target location_t.
 */
void character_sit_sleep_common(tgestate_t *state, uint8_t C, uint8_t *HL)
{
  // EX DE,HL
  *HL = 0;  // $8022, $76B8, $76BF, $76A3  (can be in vischar OR characterstruct - weird) // likely a location_t
  // EX AF,AF'
  if (state->room_index != C)
  {
    HL -= 4;
    *HL = 255;
    return;
  }

  /* Force a refresh. */
  HL += 26;
  *HL = 255;

  select_room_and_plot(state);
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
 * \param[in] HL    Likely a location_t.
 */
void player_sit_sleep_common(tgestate_t *state, uint8_t *HL)
{
  *HL = 0xFF; // set in_breakfast, or in_bed
  //$8002 = 0; // reset only the bottom byte of target location? or did i misread the disassembly?
  state->vischars[0].target = 0; // but needs to set bottom byte only

  /* Set player position (y,x) to zero. */
  memset(&state->vischars[0].mi.pos, 0, 4);

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
  sub_A373(state, &Adash);
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
  sub_A373(state, &Adash);
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
  sub_A373(state, &Adash);
}

/* ----------------------------------------------------------------------- */

/**
 * $A4D3: byte_A13E is non-zero (another one).
 *
 * Very similar to the routine at $A3F3.
 *
 * \param[in] state Pointer to game state.
 */
void byte_A13E_is_nonzero_anotherone(tgestate_t        *state,
                                     characterstruct_t *HL,
                                     vischar_t         *IY)
{
  byte_A13E_anotherone_common(state, HL, IY, state->character_index);
}

/**
 * $A4D8: byte_A13E is zero (another one).
 *
 * \param[in] state Pointer to game state.
 */
void byte_A13E_is_zero_anotherone(tgestate_t        *state,
                                  characterstruct_t *HL,
                                  vischar_t         *IY)
{
  uint8_t character;

  character = IY->character;
  if (character == 0)
  {
    set_player_target_location(state, location_2B00);
    return;
  }

  byte_A13E_anotherone_common(state, HL, IY, character);
}

/**
 * $A4E4: Common end of above two routines.
 *
 * \param[in] state           Pointer to game state.
 * \param[in] HL              Pointer to character struct.
 * \param[in] IY              Pointer to vischar.
 * \param[in] character_index Character index. (was ?)
 */
void byte_A13E_anotherone_common(tgestate_t        *state,
                                 characterstruct_t *HL,
                                 vischar_t         *IY,
                                 uint8_t            character_index)
{
  uint8_t old_character_index;

  HL->room = room_NONE;

  if (character_index > 19)
  {
    character_index -= 2;
  }
  else
  {
    old_character_index = character_index;
    character_index = 24;
    if (old_character_index & (1 << 0))
      character_index++;
  }

  HL->character = character_index;
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
  C = 0;
  sub_A35F(state, &Adash);
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
  plot_game_screen(state);
  set_game_screen_attributes(state, attribute_WHITE_OVER_BLACK);
}

/* ----------------------------------------------------------------------- */

/**
 * $A51C: Player has escaped.
 *
 * \param[in] state Pointer to game state.
 */
void escaped(tgestate_t *state)
{
  const screenlocstring_t *slstring;
  escapeitem_t             escapeitem_flags;
  const item_t            *item_p;
  uint8_t                  keys;

  screen_reset(state);

  /* Print standard prefix messages. */
  slstring = &escape_strings[0];
  slstring = screenlocstring_plot(state, slstring); /* WELL DONE */
  slstring = screenlocstring_plot(state, slstring); /* YOU HAVE ESCAPED */
  (void) screenlocstring_plot(state, slstring);     /* FROM THE CAMP */

  /* Print item-tailored messages. */
  escapeitem_flags = 0;
  item_p           = &state->items_held[0];
  escapeitem_flags = have_required_items(item_p++, escapeitem_flags);
  escapeitem_flags = have_required_items(item_p,   escapeitem_flags);
  if (escapeitem_flags == escapeitem_COMPASS + escapeitem_PURSE)
  {
    slstring = &escape_strings[3];
    slstring = screenlocstring_plot(state, slstring); /* AND WILL CROSS THE */
    (void) screenlocstring_plot(state, slstring);     /* BORDER SUCCESSFULLY */
    escapeitem_flags = 0xFF; /* success - reset game */
  }
  else if (escapeitem_flags != escapeitem_COMPASS + escapeitem_PAPERS)
  {
    slstring = &escape_strings[5]; /* BUT WERE RECAPTURED */
    slstring = screenlocstring_plot(state, slstring);
    /* no uniform => AND SHOT AS A SPY */
    if (escapeitem_flags < escapeitem_UNIFORM)
    {
      /* no objects => TOTALLY UNPREPARED */
      slstring = &escape_strings[7];
      if (escapeitem_flags)
      {
        /* no compass => TOTALLY LOST */
        slstring = &escape_strings[8];
        if (escapeitem_flags & escapeitem_COMPASS)
        {
          /* no papers => DUE TO LACK OF PAPERS */
          slstring = &escape_strings[9];
        }
      }
    }
    (void) screenlocstring_plot(state, slstring);
  }

  slstring = &escape_strings[10]; /* PRESS ANY KEY */
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
 * \return Key code.
 */
uint8_t keyscan_all(tgestate_t *state)
{
  uint16_t port;
  uint8_t  keys;
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
 * \param[in] pitem    Pointer to item.
 * \param[in] previous Previous bitfield.
 *
 * \return Sum of bitmasks.
 */
escapeitem_t have_required_items(const item_t *pitem, escapeitem_t previous)
{
  return item_to_bitmask(*pitem) + previous;
}

/**
 * $A5A3: Return a bitmask indicating the presence of required items.
 *
 * \param[in] item Item.
 *
 * \return COMPASS, PAPERS, PURSE, UNIFORM => 1, 2, 4, 8.
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
 * \param[in] slstring Pointer to screenlocstring.
 *
 * \return Pointer to next screenlocstring.
 */
const screenlocstring_t *screenlocstring_plot(tgestate_t              *state,
                                              const screenlocstring_t *slstring)
{
  uint8_t    *screenaddr;
  int         length;
  const char *string;

  screenaddr = &state->speccy->screen[slstring->screenloc];
  length     = slstring->length;
  string     = slstring->string;
  do
    screenaddr = plot_glyph(string++, screenaddr);
  while (--length);

  return slstring + 1;
}

/* ----------------------------------------------------------------------- */

/**
 * $A5CE: Escape strings.
 */
const screenlocstring_t escape_strings[11] =
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

/* ----------------------------------------------------------------------- */

/**
 * $A7C9: Get supertiles.
 *
 * Uses state->map_position to copy supertile indices from map_tiles into
 * the buffer at state->maptiles_buf.
 *
 * \param[in] state Pointer to game state.
 */
void get_supertiles(tgestate_t *state)
{
  uint8_t          hi;
  const maptile_t *HL;
  uint8_t          A;
  uint8_t         *DE;

  /* Get vertical offset. */
  hi = state->map_position[1] & 0xFC; // A = 0, 4, 8, 12, ...
  /* Multiply A by 13.5. (A is a multiple of 4, so this goes 0, 54, 108, 162, ...) */
  HL = &map_tiles[0] - MAPX + (hi + (hi >> 1)) * 9; // -MAPX so it skips the first row

  /* Add horizontal offset. */
  HL += state->map_position[0] >> 2;

  /* Populate maptiles_buf with 7x5 array of supertile refs. */
  A = 5;
  DE = &state->maptiles_buf[0];
  do
  {
    memcpy(DE, HL, 7);
    DE += 7;
    HL += MAPX;
  }
  while (--A);
}

/* ----------------------------------------------------------------------- */

/**
 * $A80A: supertile_plot_1
 *
 * \param[in] state Pointer to game state.
 */
void supertile_plot_1(tgestate_t *state)
{
  tileindex_t *DE;
  maptile_t   *HLdash;
  uint8_t     *DEdash;
  uint8_t      A;

  DE     = &state->tile_buf[24 * 16]; // $F278 = visible tiles array + 24 * 16 (state->columns * state->rows)
//  HLdash = $FF74; // not sure where this is pointing
  A      = state->map_position[1]; // map_position hi
  DEdash = &state->window_buf[0]; // $FE90 = screen buf + ? // 0xF290 + 24 * 16 * 8

  supertile_plot_common(state, DE, HLdash, A, DEdash);
}

/**
 * $A819: supertile_plot_2
 *
 * \param[in] state Pointer to game state.
 */
void supertile_plot_2(tgestate_t *state)
{
  tileindex_t *DE;
  maptile_t   *HLdash;
  uint8_t     *DEdash;
  uint8_t      A;

  DE     = &state->tile_buf[0]; // $F0F8 = visible tiles array + 0
//  HLdash = $FF58; // not sure where this is pointing
  A      = state->map_position[1]; // map_position hi
  DEdash = &state->window_buf[0]; // $F290; // screen buffer start address

  supertile_plot_common(state, DE, HLdash, A, DEdash);
}

/**
 * $A826: Plotting supertiles.
 *
 * \param[in] state  Pointer to game state.
 * \param[in] DE     Pointer to visible tiles array.
 * \param[in] HLdash Pointer to 7x5 supertile refs.
 * \param[in] A      Map position low byte.
 * \param[in] DEdash Pointer to screen buffer start address.
 */
void supertile_plot_common(tgestate_t *state,
                           uint8_t    *DE,
                           uint8_t    *HLdash,
                           uint8_t     A,
                           uint8_t    *DEdash)
{
  uint8_t            self_A86A;
  uint8_t            Cdash;
  uint8_t            Adash;
  const supertile_t *HL;

  A = (A & 3) * 4;
  self_A86A = A; // self modify (local)
  Cdash = A;
  A = (state->map_position[0] & 3) + Cdash;

  Adash = *HLdash;

  HL = &map_supertiles[Adash];
#if 0
  A += L; // pointer alignment assumption
  L = A;

  // edge/initial case?

  A = -A & 3;
  if (A == 0)
    A = 4;
  B = A; /* 1..4 iterations */
  do
  {
    A = *DE = *HL; // A = tile index
    plot_tile(A /* tile_index */,
              HLdash /* psupertile */,
              DEdash /* scr */);
    HL++;
    DE++;
  }
  while (--B);
  HLdash++;

  Bdash = 5; // 5 iterations
  do
  {
    // PUSH BCdash
    HL = &super_tiles[*HLdash] + self_A86A; // self modified by $A82A
    B = 4; // 4 iterations
    do
    {
      A = *DE = *HL; // A = tile index
      plot_tile(A /* tile_index */,
                HLdash /* psupertile */,
                DEdash /* scr */);
      HL++;
      DE++;
    }
    while (--B);
    HLdash++;
    // POP BCdash
  }
  while (--Bdash);

  A = Cdash;
  // EX AF,AF' // unpaired?
  A = *HLdash;
  HL = &super_tiles[A] + self_A86A; // read of self modified instruction
  A = state->map_position[0] & 3; // map_position lo
  if (A == 0)
    return;

  B = A;
  do
  {
    A = *DE = *HL;
    plot_tile(A, HLdash, DEdash);
    HL++;
    DE++;
  }
  while (--B);
#endif
}

/* ----------------------------------------------------------------------- */

/**
 * $A8A2: Some sort of setup for outdoors.
 *
 * \param[in] state Pointer to game state.
 */
void setup_exterior(tgestate_t *state)
{
  tileindex_t *DE;
  maptile_t   *HLdash;
  uint8_t     *DEdash;
  uint8_t      A;
  uint8_t      Bdash;
  uint8_t      Cdash;

  DE     = &state->tile_buf[0];     /* visible tiles array */
  HLdash = &state->maptiles_buf[0]; /* 7x5 supertile refs */
  DEdash = &state->window_buf[0];   /* screen buffer start address */

  A = state->map_position[0]; // map_position lo

  Bdash = state->rows; // (was 24) iterations (assuming screen rows)
  do
  {
    // PUSH BCdash
    // PUSH DEdash
    // PUSH HLdash
    // PUSH AFdash

    // PUSH DE
    supertile_plot_common_A8F4(state, DE, HLdash, A, DEdash);
    // POP DE
    DE++;

    // POP AF
    // POP HLdash
    Cdash = ++A;
    if ((A & 3) == 0)
      HLdash++;
    A = Cdash;
    // POP DEdash
    DEdash++;
    // POP BCdash
  }
  while (--Bdash);
}

/* ----------------------------------------------------------------------- */

/**
 * $A8CF: supertile_plot_3.
 *
 * \param[in] state Pointer to game state.
 */
void supertile_plot_3(tgestate_t *state)
{
  tileindex_t *DE;
  maptile_t   *HLdash;
  uint8_t     *DEdash;
  uint8_t      A;

  DE     = &state->tile_buf[23];    /* visible tiles array */
  HLdash = &state->maptiles_buf[6]; /* 7x5 supertile refs */
  DEdash = &state->window_buf[23];  /* screen buffer start address */

  A = state->map_position[0] & 3; // map_position lo
  if (A == 0)
    HLdash--;

  A = state->map_position[0] - 1; // map_position lo

  supertile_plot_common_A8F4(state, DE, HLdash, A, DEdash);
}

/**
 * $A8E7: supertile_plot_4.
 *
 * Suspect this is supertile plotting.
 *
 * \param[in] state Pointer to game state.
 */
void supertile_plot_4(tgestate_t *state)
{
  tileindex_t *DE;
  maptile_t   *HLdash;
  uint8_t     *DEdash;
  uint8_t      A;

  DE     = &state->tile_buf[0];     /* visible tiles array */
  HLdash = &state->maptiles_buf[0]; /* 7x5 supertile refs */
  DEdash = &state->window_buf[0];   /* screen buffer start address */

  A = state->map_position[0]; // map_position lo

  supertile_plot_common_A8F4(state, DE, HLdash, A, DEdash);
}

/**
 * $A8F4: Plotting supertiles (second variant).
 *
 * \param[in] state  Pointer to game state.
 * \param[in] DE     Pointer to visible tiles array.
 * \param[in] HLdash Pointer to 7x5 supertile refs.
 * \param[in] A      Map position low byte.
 * \param[in] DEdash Pointer to screen buffer start address.
 */
void supertile_plot_common_A8F4(tgestate_t *state,
                                uint8_t    *DE,
                                uint8_t    *HLdash,
                                uint8_t     A,
                                uint8_t    *DEdash)
{
  uint8_t            self_A94D;
  uint8_t            Cdash;
  uint8_t            Adash;
  const supertile_t *HL;

  A &= 3;
  self_A94D = A; // self modify (local)
  Cdash = A;
  A = (state->map_position[1] & 3) * 4 + Cdash;

  Adash = *HLdash;

  HL = &map_supertiles[Adash];
#if 0
  A += L; // pointer alignment assumption
  L = A;

  // edge/initial case?

  A = -((A >> 2) & 3) & 3;
  if (A == 0)
    A = 4; // 1..4
  // EX DE,HL
  BC = 24; /* 24 iterations (screen rows?) */
  do
  {
    Atile = *DE = *HL; // A = tile index
    call_plot_tile_inc_de(Atile /* tile_index */,
                          HL /* psupertile */,
                          DE /* scr */);
    DE += 4; // stride
    HL += BC;
  }
  while (--A);
  // EX DE,HL

  HLdash += 7;
  Bdash = 3; // 3 iterations
  do
  {
    // PUSH BCdash
    A = *HLdash;

    HL = &map_supertiles[A] + self_A94D; // self modified by $A8F6
    BC = 24; // stride (outer)
    // EX DE,HL
    A = 4; // 4 iterations
    do
    {
      // PUSH AF
      A = *DE = *HL; // A = tile index
      call_plot_tile_inc_de(A /* tile_index */,
                            HL /* psupertile */,
                            DE /* scr */);
      HL += BC;
      DE += 4; // stride
      // POP AF
    }
    while (--A);
    // EX DE,HL

    HLdash += 7;
    // POP BCdash
  }
  while (--Bdash);
  A = *HLdash;

  HL = &map_supertiles[A] + self_A94D; // read self modified instruction
  A = (state->map_position[1] & 3) + 1;
  BC = 24;
  // EX DE,HL
  do
  {
    // PUSH AF
    A = *HL = *DE;
    call_plot_tile_inc_de(A /*tileindex*/, HL /*psupertile*/, DE /*scr*/);
    DE += 4; // stride
    HL += BC;
    // POP AF
  }
  while (--A);
  // EX DE,HL
#endif
}

/* ----------------------------------------------------------------------- */

/**
 * $A9A0: Call plot_tile then increment scr by a row.
 *
 * \param[in] tile_index Tile index.
 * \param[in] psupertile Pointer to supertile index (used to select the correct exterior tile set).
 * \param[in] scr        Address of output buffer start address.
 *
 * \return Next output address.
 */
uint8_t *call_plot_tile_inc_de(tileindex_t              tile_index,
                               const supertileindex_t  *psupertile,
                               uint8_t                **scr)
{
  return plot_tile(tile_index, psupertile, *scr) + (24 * 8 - 1);
}

/* ----------------------------------------------------------------------- */

/**
 * $A9AD: Plot a tile then increment 'scr'.
 *
 * (Leaf)
 *
 * \param[in] tile_index Tile index.
 * \param[in] psupertile Pointer to supertile index (used to select the correct exterior tile set).
 * \param[in] scr        Output buffer start address.
 *
 * \return Next output address.
 */
uint8_t *plot_tile(tileindex_t             tile_index,
                   const supertileindex_t *psupertile,
                   uint8_t                *scr)
{
  supertileindex_t  supertileindex;
  const tile_t     *tileset;
  const tilerow_t  *src;
  uint8_t          *dst;
  uint8_t           iters;

  supertileindex = *psupertile; /* get supertile index */
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
    dst += 24; /* stride */
  }
  while (--iters);

  return scr + 1;
}

/* ----------------------------------------------------------------------- */

/**
 * $A9E4: map_shunt_1.
 *
 * \param[in] state Pointer to game state.
 */
void map_shunt_1(tgestate_t *state)
{
  state->map_position[0]++;

  get_supertiles(state);

  memmove(&state->tile_buf[0], &state->tile_buf[1], visible_tiles_length - 1);
  memmove(&state->window_buf[0], &state->window_buf[1], screen_buffer_length - 1);

  supertile_plot_3(state);
}

/**
 * $AA05: map_unshunt_1.
 *
 * \param[in] state Pointer to game state.
 */
void map_unshunt_1(tgestate_t *state)
{
  state->map_position[0]--;

  get_supertiles(state);

  memmove(&state->tile_buf[1], &state->tile_buf[0], visible_tiles_length - 1);
  memmove(&state->window_buf[1], &state->window_buf[0], screen_buffer_length);

  supertile_plot_4(state);
}

/**
 * $AA26: map_shunt_2.
 *
 * \param[in] state Pointer to game state.
 */
void map_shunt_2(tgestate_t *state)
{
  // Original code has a copy of map_position in HL on entry. In this version
  // we read it from source.
  state->map_position[0]--;
  state->map_position[1]++;

  get_supertiles(state);

  memmove(&state->tile_buf[1], &state->tile_buf[24], visible_tiles_length - 24);
  memmove(&state->window_buf[1], &state->window_buf[24 * 8], screen_buffer_length - 24 * 8);

  supertile_plot_1(state);
  supertile_plot_4(state);
}

/**
 * $AA4B: map_unshunt_2.
 *
 * \param[in] state Pointer to game state.
 */
void map_unshunt_2(tgestate_t *state)
{
  state->map_position[1]++;

  get_supertiles(state);

  memmove(&state->tile_buf[0], &state->tile_buf[24], visible_tiles_length - 24);
  memmove(&state->window_buf[0], &state->window_buf[24 * 8], screen_buffer_length - 24 * 8);

  supertile_plot_1(state);
}

/**
 * $AA6C: map_shunt_3.
 *
 * \param[in] state Pointer to game state.
 */
void map_shunt_3(tgestate_t *state)
{
  state->map_position[1]--;

  get_supertiles(state);

  memmove(&state->tile_buf[24], &state->tile_buf[0], visible_tiles_length - 24);
  memmove(&state->window_buf[24 * 8], &state->window_buf[0], screen_buffer_length - 24 * 8);

  supertile_plot_2(state);
}

/**
 * $AA8D: map_unshunt_3.
 *
 * \param[in] state Pointer to game state.
 */
void map_unshunt_3(tgestate_t *state)
{
  state->map_position[0]++;
  state->map_position[1]--;

  get_supertiles(state);

  memmove(&state->tile_buf[24], &state->tile_buf[1], visible_tiles_length - 24 - 1);
  memmove(&state->window_buf[24 * 8], &state->window_buf[1], screen_buffer_length - 24 * 8);

  supertile_plot_2(state);
  supertile_plot_3(state);
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
  // static const move_func_ptr map_move_jump_table[] =
  // {
  //   &map_move_1,
  //   &map_move_2,
  //   &map_move_3,
  //   &map_move_4
  // };

  if (state->room_index)
    return; /* outdoors only */

  if (state->vischars[0].b07 & vischar_BYTE7_BIT6)
    return; // unknown

#if 0
  DE = state->vischars[0].w0A;
  C  = state->vischars[0].b0C;
  DE += 3;
  A = *DE; // So DE (and w0A) must be a pointer.
  if (A == 255)
    return;
  if (C & (1 << 7))
    A ^= 2;
  // PUSH AF;
  HL = &map_move_jump_table[A];
  A = *HL++; // ie. HL = (word at HL); HL += 2;
  H = *HL;
  L = A;
  // POP AF
  // PUSH HL
  // PUSH AF
  BC = 0x7C00;
  if (A >= 2)
    B = 0x00;
  else if (A != 1 && A != 2)
    C = 0xC0;
  HL = map_position;
  if (L == C || H == B)
  {
popret:
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
  plot_game_screen_x = HL;
  HL = map_position;
  // pops and calls map_move_* routine pushed at $AAE0
#endif
}

void map_move_1(tgestate_t *state, uint8_t *DE)
{
  uint8_t A;

  A = *DE;
  if (A == 0)
    map_unshunt_2(state);
  else if (A & 1)
    map_shunt_1(state);
}

void map_move_2(tgestate_t *state, uint8_t *DE)
{
  uint8_t A;

  A = *DE;
  if (A == 0)
    map_shunt_2(state);
  else if (A == 2)
    map_unshunt_1(state);
}

void map_move_3(tgestate_t *state, uint8_t *DE)
{
  uint8_t A;

  A = *DE;
  if (A == 3)
    map_shunt_3(state);
  else if ((A & 1) == 0) // CHECK
    map_unshunt_1(state);
}

void map_move_4(tgestate_t *state, uint8_t *DE)
{
  uint8_t A;

  A = *DE;
  if (A == 1)
    map_shunt_1(state);
  else if (A == 3)
    map_unshunt_3(state);
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

  if (state->room_index < room_29_secondtunnelstart)
  {
    /* Player is outside or in rooms. */

    if (state->day_or_night == 0) /* day */
      attr = attribute_WHITE_OVER_BLACK;
    else if (state->room_index == 0)
      attr = attribute_BRIGHT_BLUE_OVER_BLACK;
    else
      attr = attribute_CYAN_OVER_BLACK;
  }
  else
  {
    /* Players is in tunnels. */

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

  state->game_screen_attribute = attr;

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

  DE = state->game_screen_start_addresses[zoombox_y * 8]; // ie. * 16
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

  HL = state->game_screen_start_addresses[(state->zoombox_y - 1) * 8]; // ie. * 16

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

  DE -= 256; /* Cmpensate for overshoot */
  off = DE - &state->speccy->screen[0];

  attrp = &state->speccy->attributes[off & 0xFF]; // to within a third

  // can i ((off >> 11) << 8) ?
  if (off >= 0x0800)
  {
    attrp += 256;
    if (off >= 0x1000)
      attrp += 256;
  }

  *attrp = state->game_screen_attribute;
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
  // But it needs checking for edge cases.

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
 * $AF5E: Zoombox tiles.
 */
const tile_t zoombox_tiles[6] =
{
  { { 0x00, 0x00, 0x00, 0x03, 0x04, 0x08, 0x08, 0x08 } }, /* tl */
  { { 0x00, 0x20, 0x18, 0xF4, 0x2F, 0x18, 0x04, 0x00 } }, /* hz */
  { { 0x00, 0x00, 0x00, 0x00, 0xE0, 0x10, 0x08, 0x08 } }, /* tr */
  { { 0x08, 0x08, 0x1A, 0x2C, 0x34, 0x58, 0x10, 0x10 } }, /* vt */
  { { 0x10, 0x10, 0x10, 0x20, 0xC0, 0x00, 0x00, 0x00 } }, /* br */
  { { 0x10, 0x10, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00 } }, /* bl */
};

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
    sub_AFDF(state);
    //if (!Z)
    //  return;
  }
  //; else object only from here on ?
  vischar->b07 &= ~vischar_BYTE7_BIT6;
  vischar->mi.pos = state->saved_pos;
  vischar->mi.b17 = stashed_A;
  A = 0;
}

/* ----------------------------------------------------------------------- */

/**
 * $AFDF: (unknown)
 *
 * \param[in] state Pointer to game state.
 */
void sub_AFDF(tgestate_t *state)
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
    EX DE, HL
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
    EX DE, HL
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

    A = IY->flags & 0x0F; // sampled IY=$8020, $8040, $8060, $8000 // but this is *not* vischar_BYTE1_MASK, which is 0x1F
    if (A == 1)
    {
      // POP HL
      // PUSH HL // sampled HL=$8021, $8041, $8061, $80A1
      HL--;
      if ((HL & 0xFF) == 0)   // ie. $8000
      {
        if (bribed_character == IY->character)
        {
          use_bribe(state, IY);
        }
        else
        {
          // POP HL
          // POP BC
          HL = IY + 1;
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
      A = IY->b0E; // interleaved
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
      if (A != IY->b0E)
      {
        IY->b0D = 0x80;

b0d0:
        IY->b07 = (IY->b07 & vischar_BYTE7_MASK_HI) | 5; // preserve flags and set 5? // sampled IY = $8000, $80E0
        if (!Z)
          return; /* odd */
      }
    }

    BC = IY->b0E; // sampled IY = $8000, $8040, $80E0
    IY->b0D = four_bytes_B0F8[BC];
    if ((C & 1) == 0)
      IY->b07 &= ~vischar_BYTE7_BIT5;
    else
      IY->b07 |= vischar_BYTE7_BIT5;
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

  sub_CB23(state, &vischar->target);

  DE = &state->items_held[0];
  if (*DE != item_BRIBE && *++DE != item_BRIBE)
    return; /* have no bribes */

  /* We have a bribe. */
  *DE = item_NONE;

  item_structs[item_BRIBE].room = (room_t) itemstruct_ROOM_MASK;

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
uint16_t BC_becomes_A_times_8(uint8_t A)
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
  E = IY->b0E;
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
  IY->room = (*HL >> 2) & 0x3F; // sampled HL = $792E (in door_positions[])
  if ((*HL & 3) >= 2)
  {
    HL += 5;
    transition(state, IY, HL); // seems to goto reset then jump back to main (icky)
  }
  else
  {
    HL -= 3;
    transition(state, IY, HL);
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
 * \param[in] state Pointer to game state.
 * \param[in] HL    Pointer to doorpos_t.
 *
 * \return 1 if in range, 0 if not.
 */
int door_in_range(tgestate_t *state, const doorpos_t *HL)
{
  uint16_t BC;

  BC = BC_becomes_A_times_4(HL->y);
  if (state->saved_pos.y < BC - 3 || state->saved_pos.y >= BC + 3)
    return 0;

  BC = BC_becomes_A_times_4(HL->x);
  if (state->saved_pos.x < BC - 3 || state->saved_pos.x >= BC + 3)
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
uint16_t BC_becomes_A_times_4(uint8_t A)
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
  const bounds_t *roombounds;  /* was BC */
  uint16_t       *saved_coord; /* was HL */
  uint8_t        *HL;
  uint8_t         B;

  roombounds = &roomdef_bounds[state->roomdef_bounds_index];
  saved_coord = &state->saved_pos.y;
  if (roombounds->a < *saved_coord || roombounds->b + 4 >= *saved_coord)
    goto stop;

  saved_coord++; /* &state->saved_pos.x; */
  if (roombounds->c - 4 < *saved_coord || roombounds->d >= *saved_coord)
    goto stop;

  HL = &state->roomdef_object_bounds[0];
  for (B = *HL++; B != 0; B--)
  {
    uint8_t  *innerHL;
    uint16_t *DE;
    uint8_t   innerB;

    innerHL = HL;
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
    HL += 4;
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
  setup_exterior(state);
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

    HLdoor++; // by a byte

    // registers renamed here
    DEdash = &state->saved_pos.y; // reusing saved_pos as 8-bit here? a case for saved_pos being a union of tinypos and pos types?
    Bdash = 2; // 2 iterations
    do
    {
      //A = *HLdoor;
      if (((A - 3) >= *DEdash) || (A + 3) < *DEdash)
        goto next; // -3 .. +2
      DEdash += 2;
      HLdoor++;
    }
    while (--Bdash);

    HLdoor++;
    if (is_door_open(state) == 0)
      return; // door was closed

    vischar->room = (Cdash >> 2) & 0x3F;
    HLdoor++;
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

  item_structs[item_RED_CROSS_PARCEL].room = room_NONE & itemstruct_ROOM_MASK;

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
  vischar_t *HL;
  uint8_t    B;
  uint8_t    A;

  HL = &state->vischars[1]; /* Walk visible characters. */
  B  = vischars_LENGTH - 1;
  do
  {
    A = HL->character;
    if (A != character_NONE && A >= character_20)
      goto found;
    HL++;
  }
  while (--B);

  return;

found:
  state->bribed_character = A;
  HL->flags = vischar_BYTE1_PERSUE;
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

  if (item_structs[item_FOOD].item & itemstruct_ITEM_FLAG_POISONED)
    return;

  item_structs[item_FOOD].item |= itemstruct_ITEM_FLAG_POISONED;

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
  const sprite_t *HL;

  HL = &sprites[sprite_GUARD_FACING_TOP_LEFT_4];

  if (state->vischars[0].mi.spriteset == HL)
    return; /* already in uniform */

  if (state->room_index >= room_29_secondtunnelstart)
    return; /* can't don uniform when in a tunnel */

  state->vischars[0].mi.spriteset = HL;

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
  if (state->room_index != room_50_blocked_tunnel)
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
  const wall_t    *HL;
  const tinypos_t *DE;
  uint8_t          B;
  uint8_t          A;

  HL = &walls[12]; // .d; /* start of fences */
  DE = &state->player_map_position;
  B = 4;
  do
  {
    A = DE->x;
    if (A < HL->d && A >= HL->c) // reverse
    {
      A = DE->y;
      if (A == HL->b)
        goto set_to_4;
      if (A - 1 == HL->b)
        goto set_to_6;
    }

    HL++;
  }
  while (--B);

  HL = &walls[12 + 4]; // .a
  DE = &state->player_map_position;
  B = 3;
  do
  {
    A = DE->y;
    if (A >= HL->a && A < HL->b)
    {
      A = DE->x;
      if (A == HL->c)
        goto set_to_5;
      if (A - 1 == HL->c)
        goto set_to_7;
    }

    HL++;
  }
  while (--B);

  return;

set_to_4: // crawl TL
  A = 4;
  goto action_wiresnips_tail;
set_to_5: // crawl TR
  A = 5;
  goto action_wiresnips_tail;
set_to_6: // crawl BR
  A = 6;
  goto action_wiresnips_tail;
set_to_7: // crawl BL
  A = 7;

action_wiresnips_tail:
  state->vischars[0].b0E          = A; // walk/crawl flag
  state->vischars[0].b0D          = 0x80;
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
  action_key(state, room_22_redkey);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4B2: Use yellow key.
 *
 * \param[in] state Pointer to game state.
 */
void action_yellow_key(tgestate_t *state)
{
  action_key(state, room_13_corridor);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4B6: Use green key.
 *
 * \param[in] state Pointer to game state.
 */
void action_green_key(tgestate_t *state)
{
  action_key(state, room_14_torch);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4B8: Use key (common).
 *
 * \param[in] state Pointer to game state.
 * \param[in] room  Room number. (Original register: A)
 */
void action_key(tgestate_t *state, room_t room)
{
  void     *HL; // FIXME: we don't yet know what type open_door() returns
  //uint8_t   A;
  message_t B = 0; // initialisation is temporary

  // PUSH AF
  HL = open_door(state);
  // POP BC
  if (HL == NULL)
    return; /* Wrong door? */

#if 0
  A = *HL & ~gates_and_doors_LOCKED; // mask off locked flag
  if (A != B)
  {
    B = message_INCORRECT_KEY;
  }
  else
  {
    *HL &= ~gates_and_doors_LOCKED; // clear the locked flag
    increase_morale_by_10_score_by_50(state);
    B = message_IT_IS_OPEN;
  }
#endif

  queue_message_for_display(state, B);
  (void) open_door(state);
}

/* ----------------------------------------------------------------------- */

/**
 * $B4D0: Open door.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Pointer to HL? FIXME: Fix type.
 */
void *open_door(tgestate_t *state)
{
  uint8_t         *HL;
  uint8_t          B;
  uint8_t          A;
  const doorpos_t *HLdash;
  uint8_t          C;
  uint8_t         *DE;
  uint8_t          Bdash;
  uint8_t         *DEdash;

  if (state->room_index)
  {
    /* Outdoors. */

    HL = &state->gates_and_doors[0]; // gates_flags
    B = 5; /* iterations */ // gates_and_doors ranges must overlap
    do
    {
      A = *HL & ~gates_and_doors_LOCKED;

      HLdash = get_door_position(A);
      if ((door_in_range(state, HLdash + 0) == 0) ||
          (door_in_range(state, HLdash + 1) == 0))
        return NULL; // goto removed

      HL++;
    }
    while (--B);

    return NULL;
  }
  else
  {
    /* Indoors. */

    HL = &state->gates_and_doors[2]; // door_flags
    B = 8; /* iterations */
    do
    {
      C = *HL & ~gates_and_doors_LOCKED;

      /* Search door_related for C. */
      DE = &state->door_related[0];
      for (;;)
      {
        A = *DE;
        if (A != 0xFF)
        {
          if ((A & gates_and_doors_MASK) == C)
            goto found;
          DE++;
        }
      }
next:
      HL++;
    }
    while (--B);

    return NULL; // temporary, should do something else return 1;

found:
    A = *DE;
    HLdash = get_door_position(A);
    /* Range check pattern (-2..+3). */
    DEdash = &state->saved_pos.y; // note: 16-bit values holding 8-bit values
    Bdash = 2; /* iterations */
    do
    {
      if (*DEdash <= HLdash->y - 3 || *DEdash > HLdash->y + 3)
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
 */
const wall_t walls[] =
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
  { 0x46, 0x46, 0x46, 0x6A, 0x00, 0x08 },
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
  uint8_t    B;
  vischar_t *IY;

  B = vischars_LENGTH; /* iterations */
  IY = &state->vischars[0];
  do
  {
    if (IY->flags == vischar_BYTE1_EMPTY_SLOT)
      goto next;

    // PUSH BC
    IY->flags |= vischar_BYTE1_BIT7;

    if (IY->b0D & vischar_BYTE13_BIT7)
      goto byte13bit7set;

#if 0
    H = IY->b0B;
    L = IY->w0A;
    A = IY->b0C;
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
      L = IY->b0F; // Y axis
      H = IY->b10;
      A = *DE; // sampled DE = $CF9A, $CF9E, $CFBE, $CFC2, $CFB2, $CFB6, $CFA6, $CFAA (character_related_data)
      C = A;
      A &= 0x80;
      if (A)
        A = 0xFF;
      B = A;
      HL -= BC;
      state->saved_pos.y = HL;
      DE++;

      L = IY->b11; // X axis
      H = IY->b12;
      A = *DE;
      C = A;
      A &= 0x80;
      if (A)
        A = 0xFF;
      B = A;
      HL -= BC;
      state->saved_pos.x = HL;
      DE++;

      L = IY->b13; // vertical offset
      H = IY->b14;
      A = *DE;
      C = A;
      A &= 0x80;
      if (A)
        A = 0xFF;
      B = A;
      HL -= BC;
      state->saved_pos.vo = HL;

      sub_AF8F(state, IY);
      if (!Z)
        goto pop_next;

      IY->b0C--;
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
      C = IY->b0F; // Y axis
      B = IY->b10;
      HL += BC;
      saved_Y = HL;
      DE++;

      A = *DE;
      L = A;
      A &= 0x80;
      if (A)
        A = 0xFF;
      H = A;
      C = IY->b11; // X axis
      B = IY->b12;
      HL += BC;
      saved_X = HL;
      DE++;

      A = *DE;
      L = A;
      A &= 0x80;
      if (A)
        A = 0xFF;
      H = A;
      C = IY->b13; // vertical offset
      B = IY->b14;
      HL += BC;
      saved_VO = HL;

      DE++;
      A = *DE;
      // EX AF,AF'

      sub_AF8F(state, IY);
      if (!Z)
        goto pop_next;

      IY->b0C++;
    }
    HL = IY;
reset_position:
    reset_position_end(state, IY);
#endif

pop_next:
    // POP BC
    if (IY->flags != vischar_BYTE1_EMPTY_SLOT)
      IY->flags &= ~vischar_BYTE1_BIT7; // $8001

next:
    IY++;
  }
  while (--B);

byte13bit7set:
  IY->b0D &= ~vischar_BYTE13_BIT7; // sampled IY = $8020, $80A0, $8060, $80E0, $8080,

#if 0
snozzle:
  A = byte_CDAA[IY->b0E][IY->b0D];
  C = A;
  L = IY->w08;
  H = IY->b09;
  HL += A * 2;
  E = *HL++;
  IY->w0A = E;
  D = *HL;
  IY->b0B = D;
  if ((C & (1 << 7)) == 0)
  {
    IY->b0C = 0;
    DE += 2;
    IY->b0E = *DE;
    DE += 2;
    // EX DE,HL
    goto resume2;
  }
  else
  {
    A = *DE;
    C = A;
    IY->b0C = A | 0x80;
    IY->b0E = *++DE;
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
 * \param[in] state Pointer to game state.
 * \param[in] v     Pointer to visible character.
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
 * \param[in] state Pointer to game state.
 * \param[in] v     Pointer to visible character.
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
  state->room_index = room_2_hut2left;
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
    uint8_t y;
    uint8_t x;
  }
  character_reset_partial_t;

  /**
   * $B819: Partial character struct for reset
   */
  static const character_reset_partial_t character_reset_data[10] =
  {
    { room_3_hut2right, 40, 60 }, /* for character 12 */
    { room_3_hut2right, 36, 48 }, /* for character 13 */
    { room_5_hut3right, 40, 60 }, /* for character 14 */
    { room_5_hut3right, 36, 34 }, /* for character 15 */
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
  DE = &character_structs[character_12];
  C = NELEMS(character_reset_data); /* iterations */
  HLreset = &character_reset_data[0];
  do
  {
    DE->room   = HLreset->room;
    DE->pos.y  = HLreset->y;
    DE->pos.x  = HLreset->x;
    DE->pos.vo = 18; /* Odd: This is reset to 18 but the initial data is 24. */
    DE->t1     = 0;
    DE++;
    HLreset++;

    if (C == 7)
      DE = &character_structs[20];
  }
  while (--C);
}

/* ----------------------------------------------------------------------- */

/**
 * $B83B searchlight_sub
 *
 * Looks like it's testing mask_buffer to find player?
 *
 * \param[in] state Pointer to game state.
 */
void searchlight_sub(tgestate_t *state, vischar_t *IY)
{
  uint8_t     *HL;
  attribute_t  attrs;
  uint8_t      B;
  uint8_t      C;

  if (IY > &state->vischars[0])
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
  set_game_screen_attributes(state, attrs);

  return;

set_state_1f:
  state->searchlight_state = searchlight_STATE_1F;
}

/* ----------------------------------------------------------------------- */

/**
 * $B866: Searchlight.
 *
 * \param[in] state Pointer to game state.
 */
void searchlight(tgestate_t *state)
{
  uint8_t A;

  for (;;)
  {
    A = sub_B89C(state);
#if 0
    if (!Z)
      return;

    if ((A & (1 << 6)) == 0) // mysteryflagconst874
    {
      setup_sprite_plotting(state, IY);
      if (Z)
      {
        mask_stuff(state);
        if (searchlight_state != searchlight_STATE_OFF)
          searchlight_sub(state, IY);
        if (IY->b1E != 3)
          masked_sprite_plotter_24_wide(state, IY);
        else
          masked_sprite_plotter_16_wide_case_1(state, IY);
      }
    }
    else
    {
      sub_DC41(state, IY, A);
      if (Z)
      {
        mask_stuff(state);
        masked_sprite_plotter_16_wide_case_1_searchlight(state);
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
 * \param[in] state Pointer to game state.
 *
 * \return Nothing. // must return something - searchlight() tests Z flag on return and A // must return IY too
 */
uint8_t sub_B89C(tgestate_t *state)
{
  uint16_t   BC;
  uint16_t   DE;
  uint8_t    A;
  uint16_t   DEdash;
  uint8_t    Bdash;
  uint8_t    Cdash;
  vischar_t *HLdash;

  BC = 0;
  DE = 0;
  A = 0xFF;
  // EX AF, AF'
  // EXX
  DEdash = 0;
  Bdash = 8; /* iterations */
  Cdash = 32; // stride
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
    HLdash += Cdash;
  }
  while (--Bdash);

#if 0
  sub_DBEB(state); // IY returned from this
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
    HL->flags &= ~vischar_BYTE1_BIT6;
    //BIT 6,HL[1]  // this tests the bit we've just cleared
  }
#endif

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
  uint8_t        B;
  uint8_t        A;
  const uint8_t *HL;
  const eightbyte_t *HLeb;
  uint8_t        C;

  memset(&state->mask_buffer[0], 0xFF, NELEMS(state->mask_buffer));

  if (state->room_index)
  {
    /* Indoors */

    HL = &state->indoor_mask_data[0];
    A = *HL; // count byte
    if (A == 0)
      return;

    B = A; /* iterations */
    HL += 3;
  }
  else
  {
    /* Outdoors */

    B = 59; /* iterations */ // this count doesn't match NELEMS(exterior_mask_data); which is 58
    HLeb = &exterior_mask_data[0]; // off by - 2 bytes; original points to $EC03, table starts at $EC01 // fix by propogation
    do
    {
      // PUSH BC
      // PUSH HLeb

      A = state->map_position_related_1;
      if (A - 1 >= HLeb->eb_c || A + 2 <= HLeb->eb_b) // $EC03, $EC02
        goto pop_next;

      A = state->map_position_related_2;
      if (A - 1 >= HLeb->eb_e || A + 3 <= HLeb->eb_d) // $EC05, $EC04
        goto pop_next;

      if (state->tinypos_81B2.y <= HLeb->eb_f) // $EC06
        goto pop_next;

      if (state->tinypos_81B2.x < HLeb->eb_g) // $EC07
        goto pop_next;

      A = state->tinypos_81B2.vo;
      if (A)
        A--; // make inclusive?
      if (A >= HLeb->eb_h) // $EC08
        goto pop_next;

      // eb_f/g/h could be a tinypos_t

      // redundant: HLeb -= 6;

#if 0
      C = A = state->map_position_related_1;
      if (A >= HLeb->eb_b) // must be $EC02
      {
        state->byte_B837 = A - HLeb->eb_b;
        A = HLeb->eb_c - C; // $EC03
        if (A >= 3)
          A = 3;

        ($B83A) = A + 1; // word_B839 high byte
      }
      else
      {
        B = HLeb->eb_b; // must be $EC02
        state->byte_B837 = 0;
        C = 4 - (B - C);
        A = (HLeb->eb_c - B) + 1;
        if (A > C)
          A = C;

        ($B83A) = A; // word_B839 high byte
      }

      C = A = state->map_position_related_2;
      if (A >= HLeb->eb_d)
      {
        state->byte_B838 = A - HLeb->eb_d;
        A = HLeb->eb_e - C;
        if (A >= 4)
          A = 4;

        ($B839) = A + 1; // word_B839 low byte
      }
      else
      {
        B = HLeb->eb_d;
        state->byte_B838 = 0;
        C = 5 - (B - C);
        A = (HLeb->eb_e - B) + 1;
        if (A >= C)
          A = C;

        ($B839) = A; // word_B839 low byte
      }

      // In the original code, HLeb is here decremented to point at member eb_d.


      BC = 0;
      if (state->byte_B838 == 0)
        C = -state->map_position_related_2 + *HLeb;
      HLeb -= 2;
      if (state->byte_B837 == 0)
        B = -state->map_position_related_1 + *HLeb;
      HLeb--;
      A = *HLeb;

      Adash = C * 32 + B;
      HLeb = &state->mask_buffer[Adash];
      state->word_81A0 = HLeb;

      // If I break this bit then the character gets drawn on top of *indoors* objects.

      DE = probably_mask_data_pointers[A];
      HL = word_B839;
      self_BA70 = L; // self modify
      self_BA72 = H; // self modify
      self_BA90 = *DE - H; // self modify // *DE looks like a count
      self_BABA = 32 - H; // self modify
      // PUSH DE
      E = *DE;
      A = byte_B838;
      HL = scale_val(state, A, E);
      E = byte_B837;
      HL += DE;
      // POP DE
      HL++; /* iterations */
      do
      {
ba4d:
        A = *DE; // DE -> $E560 upwards (in probably_mask_data)
        if (!even_parity(A))
        {
          // uneven number of bits set
          A &= 0x7F;
          DE++;
          HL -= A;
          if (HL < 0)
            goto ba69;
          DE++;
          if (DE)
            goto ba4d;
          A = 0;
          goto ba6c;
        }
        DE++;
      }
      while (--HL);
      goto ba6c;

ba69:
      A = -L;

ba6c:
      HL = state->word_81A0;
      // R I:C Iterations (inner loop);
      C = self_BA70; // self modified
      do
      {
        B = self_BA72; // self modified
        do
        {
          Adash = *DE;
          Adash &= Adash;

          if (!P)
          {
            Adash &= 0x7F;

            DE++;
            A = *DE;
          }

          A &= A; // if (A) ...
          if (!Z)
            mask_against_tile(A, HL);

          L++; // -> HL++

          // EX AF,AF' // unpaired?

          if (A != 0 && --A != 0)
            DE--;
          DE++;
        }
        while (--B);
        // PUSH BC
        B = self_BA90; // self modified
        if (B)
        {
          if (A)
            goto baa3;

ba9b:
          do
          {
            A = *DE;
            if (!even_parity(A))
            {
              A &= 0x7F;
              DE++;

baa3:
              B -= A;
              if (B < 0)
                goto bab6;

              DE++;
              if (!Z)
                goto ba9b;

              // EX AF,AF' // why not just jump instr earlier? // bank
              goto bab9;
            }

            DE++;
          }
          while (--B);
          A = 0;
          // EX AF,AF' // why not just jump instr earlier? // bank
          goto bab9;

bab6:
          A = -A;
          // EX AF,AF' // bank
        }

bab9:
        HL += self_BABA; // self modified
        // EX AF,AF'  // unbank
        // POP BC
      }
      while (--C);

#endif
pop_next:
      // POP HLeb
      // POP BC
      HLeb++; // stride is eightbyte_t
    }
    while (--B);
  }

// ; fallthrough
  HL = scale_val(state, A, 8);
  return;
  // above would scale A by B then return nothing: must be doing this for a reason
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
 */
uint16_t scale_val(tgestate_t *state, uint8_t value, uint8_t shift)
{
  uint8_t  B;      // was B
  uint16_t result; // was HL

  B = 8; /* iterations */
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
  while (--B);

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
 * $BAF7: (unknown)
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0 or 0xFF.
 */
// this must return DE too
//D $BAF7 Sets the flags for return but looks like caller never uses them.
//R $BAF7 O:DE Return value ?
int sub_BAF7(tgestate_t *state)
{
  uint8_t A;

#if 0
  HL = &state->map_position_related_1;
  A = state->map_position[0] + 24;
  A -= *HL;
  if (A > 0)
  {
    // CP IY->b1E  // if (A ?? IY->b1E)
    if (carry)
    {
      BC = A;
    }
    else
    {
      A = *HL + IY->b1E;
      A -= state->map_position[0];
      if (A <= 0)
        goto exit;
      //CP IY->b1E
      if (carry)
      {
        C = A;
        B = -A + IY->b1E;
      }
      else
      {
        BC = IY->b1E;
      }
    }

    HL = ((map_position >> 8) + 17) * 8;
    DE = IY->w1A; // fused
    A &= A;
    HL -= DE;
    if (result <= 0)
      goto exit;
    if (H)
      goto exit;
    A = L;
    //CP IY->b1F
    if (carry)
    {
      DE = A;
    }
    else
    {
      HL = IY->b1F + DE;
      DE = state->map_position >> 8 * 8;
      A &= A; // likely: clear carry
      HL -= DE;
      if (result <= 0)
        goto exit;
      if (H)
        goto exit;
      A = L;
      //CP IY->b1F
      if (carry)
      {
        E = A;
        D = -A + IY->b1F;
      }
      else
      {
        DE = IY->b1F;
      }
    }

    return 0; // return Z
  }
#endif

exit:
  return 0xFF; // return NZ
}

/* ----------------------------------------------------------------------- */

/**
 * $BB98: Called from main loop 3.
 *
 * \param[in] state Pointer to game state.
 */
void called_from_main_loop_3(tgestate_t *state)
{
  uint8_t    B;
  vischar_t *IY;

  B = vischars_LENGTH; /* iterations */
  IY = &state->vischars[0];
  do
  {
    // PUSH BC
    if (IY->flags == 0)
      goto next;

    state->map_position_related_2 = IY->w1A >> 3; // divide by 8
    state->map_position_related_1 = IY->w18 >> 3; // divide by 8

    if (sub_BAF7(state) == 0xFF)
      goto next; // possibly the not found case

#if 0
    A = ((E >> 3) & 31) + 2; // E comes from?
    // PUSH AF
    A += state->map_position_related_2 - ((map_position & 0xFF00) >> 8);
    if (A >= 0)
    {
      A -= 17;
      if (A > 0)
      {
        E = A;
        // POP AF
        A -= E;
        if (carry)
          goto next;
        if (!Z) // ie. if (!carry && !zero)
          goto bbf8;
        goto next;
      }
    }
    // POP AF

bbf8:
    if (A > 5)
      A = 5;
    self_BC5F = A; // self modify
    self_BC61 = C; // self modify
    self_BC89 = C; // self modify
    A = 24 - C;
    self_BC8E = A; // self modify
    A += 7 * 24;
    self_BC95 = A; // self modify

    HL = &state->map_position;

    if (B == 0)
      B = map_position_related_1 - HL[0];
    else
      B = 0; // was interleaved

    if (D == 0) // (yes, D)
      C = map_position_related_2 - HL[1];
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
          DEdash += 24;
        }
        while (--Bdash);
        // POP HLdash
        Hdash++;

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
    IY++;
  }
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $BCAA: Turn a map ref into a tile set pointer?
 *
 * \param[in] state Pointer to game state.
 * \param[in] HL    ?
 */
const tile_t *select_tile_set(tgestate_t *state, uint16_t HL)
{
  const tile_t *BC;
  uint8_t       Adash;
  uint8_t       H, L;

  if (state->room_index)
  {
    BC = &interior_tiles[0];
    return BC;
  }
  else
  {
    // what's HL at this point? a pointer or a tiles offset or what?
    H = HL >> 8;
    L = HL & 0xFF;

    // converts map pos -> index into 7x5 supertile refs array
    Adash = ((state->map_position[1] & 3) + L) >> 2;
    L = (Adash & 0x3F) * 7;
    Adash = ((state->map_position[0] & 3) + H) >> 2;
    Adash = (Adash & 0x3F) + L;

    Adash = state->maptiles_buf[Adash]; /* 7x5 supertile refs */
    BC = &exterior_tiles_1[0];
    if (Adash >= 45)
    {
      BC = &exterior_tiles_2[0];
      if (Adash >= 139 && Adash < 204)
        BC = &exterior_tiles_3[0];
    }
    return BC;
  }
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

  /* Form a map position in DE. */
  H = state->map_position[1];
  L = state->map_position[0];
  E = (L < 8) ? 0 : L;
  D = (H < 8) ? 0 : H;

  /* Walk all character structs. */
  HL = &character_structs[0];
  B  = 26; // the 26 'real' characters
  do
  {
    if (HL->character & characterstruct_BYTE0_BIT6)
      goto skip;

    A = state->room_index;
    if (A != HL->room)
      goto skip; /* not in the visible room */
    if (A != 0)
      goto indoors;

#if 0
    /* Outdoors. */
    HL2++; // $7614
    A -= *HL2; // A always starts as zero here
    HL2++; // $7615
    A -= *HL2;
    HL2++; // $7616
    A -= *HL2;
    C = A;
    A = D;
    if (C <= A)
      goto skip; // check
    A += 32;
    if (A > 0xFF)
      A = 0xFF;
    if (C > A)
      goto skip; // check

    HL2--; // $7615
    A = 64;
    *HL2 += A;
    HL2--; // $7614
    *HL2 -= A;
    A *= 2; // A == 128
    C = A;
    A = E;
    if (C <= A)
      goto skip; // check
    A += 40;
    if (A > 0xFF)
      A = 0xFF;
    if (C > A)
      goto skip; // check
#endif

indoors:
    spawn_characters(state, HL);

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
  vischar_t         *pvc;
  uint8_t            B;
  characterstruct_t *DE;
  vischar_t         *IY;
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
  IY = pvc;

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

  sub_AFDF(state);
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
      HL -= 2;
      // PUSH HL
      // CALL $CB2D
      // POP HL
      DE = HL + 2;
      goto c592;
    }
    if (A == 128)
      IY->flags |= vischar_BYTE1_BIT6;
    // POP DE
    memcpy(DE, HL, 3);
  }
  *DE = 0;
  DE -= 7;
  // EX DE,HL
  // PUSH HL
  reset_position(state, IY);
  // POP HL
  character_behaviour_stuff(state, IY); return; // exit via

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
  characterstruct_t *DE;
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
      pos = &movable_items[movable_item_STOVE1].pos;
    else if (character == character_27)
      pos = &movable_items[movable_item_STOVE2].pos;
    else
      pos = &movable_items[movable_item_CRATE].pos;
    memcpy(pos, &vischar->mi.pos, sizeof(*pos));
  }
  else
  {
    pos_t     *vispos_in;
    tinypos_t *charpos_out;

    /* A non-object character. */

    DE = get_character_struct(character);
    DE->character &= ~characterstruct_BYTE0_BIT6;

    room = vischar->room;
    DE->room = room;

    vischar->b07 = 0; /* more flags */

    /* Save the old position. */

    vispos_in = &vischar->mi.pos;
    charpos_out = &DE->pos; // byte-wide form of pos struct

    // set y,x,vo
    if (room == room_0_outdoors)
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
    DE->t1 = vischar->target;
    DE->t2 = vischar->target >> 8;
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $C651: (unknown)
 *
 * \param[in] state Pointer to game state.
 * \param[in] HL    Pointer to characterstruct + 5. (target field(s))
 *
 * \return 0/255.
 */
uint8_t sub_C651(tgestate_t *state, uint8_t *HL)
{
  uint8_t A;
  uint8_t A1, A2;

  A = *HL;
  if (A == 0xFF)
  {
    A1 = *++HL & characterstruct_BYTE6_MASK_HI;
    A2 = get_A_indexed_by_C41A(state) & characterstruct_BYTE6_MASK_LO;
    *HL = A1 + A2;
  }
  else
  {
#if 0
    // PUSH HL
    C = *++HL; // byte6
    DE = element_A_of_table_7738(A);
    H = 0;
    A = C;
    if (A == 0xFF)
      H--; // H = 0 - 1 => 0xFF
    L = A;
    HL += DE;
    // EX DE,HL
    A = *DE;
    if (A == 0xFF)
      // POP HL // interleaved
      goto return_255;
    A &= 0x7F;
    if (A < 40)
    {
      A = *DE;
      if (*HL & (1 << 7))
        A ^= 0x80; // 762C, 8002, 7672, 7679, 7680, 76A3, 76AA, 76B1, 76B8, 76BF, ... looks quite general
      transition(state, VISCHAR, LOCATION);
      HL++;
      A = 0x80;
      return;
    }
    A = *DE - 40;
#endif
  }
  // sample A=$38,2D,02,06,1E,20,21,3C,23,2B,3A,0B,2D,04,03,1C,1B,21,3C,...
  HL = word_783A[A];

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

  HL = get_character_struct(state->character_index);
  if (HL->character & characterstruct_BYTE0_BIT6)
    return;

#if 0
  // PUSH HL
  A = *++HL; // characterstruct byte1 == room
  if (A != room_0_outdoors)
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
        B = 2; // 2 iters
        do
          *DE++ = *HL++ >> 1;
        while (--B);
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
 * \param[in] max   A maximum value. (was Adash)
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
characterstruct_t *get_character_struct(character_t index)
{
  return &character_structs[index];
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
  static const charactereventmap_t map[] =
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

  A = charstruct->character;
  if (A >= character_7  && A <= character_12)
  {
    character_sleeps(state, A, charstruct);
    return;
  }
  if (A >= character_18 && A <= character_22)
  {
    character_sits(state, A, charstruct);
    return;
  }

  HL = &map[0];
  B = NELEMS(map);
  do
  {
    if (A == HL->character_and_flag)
    {
      handlers[HL->handler].handler(state, &charstruct->character);
      return;
    }
    HL++;
  }
  while (--B);

  charstruct->character = 0; // no action;
#endif
}

/**
 * $C83F:
 */
void charevnt_handler_4_zero_morale_1(tgestate_t   *state,
                                      character_t *charptr,
                                      vischar_t    *IY)
{
  state->morale_1 = 0;
  charevnt_handler_0(state, charptr, IY);
}

/**
 * $C845:
 */
void charevnt_handler_6(tgestate_t   *state,
                        character_t *charptr,
                        vischar_t    *IY)
{
  // POP charptr // (popped) sampled charptr = $80C2 (x2), $8042  // likely target location
  *charptr++ = 0x03;
  *charptr   = 0x15;
}

/**
 * $C84C:
 */
void charevnt_handler_10_player_released_from_solitary(tgestate_t   *state,
                                                       character_t *charptr,
                                                       vischar_t    *IY)
{
  // POP charptr
  *charptr++ = 0xA4;
  *charptr   = 0x03;
  state->automatic_player_counter = 0; // force automatic control
  set_player_target_location(state, 0x2500);
}

/**
 * $C85C:
 */
void charevnt_handler_1(tgestate_t   *state,
                        character_t *charptr,
                        vischar_t    *IY)
{
  uint8_t C;

  C = 0x10; // 0xFF10
  localexit(state, charptr, C);
}

/**
 * $C860:
 */
void charevnt_handler_2(tgestate_t   *state,
                        character_t *charptr,
                        vischar_t    *IY)
{
  uint8_t C;

  C = 0x38; // 0xFF38
  localexit(state, charptr, C);
}

/**
 * $C864:
 */
void charevnt_handler_0(tgestate_t   *state,
                        character_t *charptr,
                        vischar_t    *IY)
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
void charevnt_handler_3_check_var_A13E(tgestate_t   *state,
                                       character_t *charptr,
                                       vischar_t    *IY)
{
  // POP HL
  if (state->byte_A13E == 0)
    byte_A13E_is_zero(state, charptr, IY);
  else
    byte_A13E_is_nonzero(state, charptr, IY);
}

/**
 * $C877:
 */
void charevnt_handler_5_check_var_A13E_anotherone(tgestate_t   *state,
                                                  character_t *charptr,
                                                  vischar_t    *IY)
{
  // POP HL
  if (state->byte_A13E == 0)
    byte_A13E_is_zero_anotherone(state, charptr, IY);
  else
    byte_A13E_is_nonzero_anotherone(state, charptr, IY);
}

/**
 * $C882:
 */
void charevnt_handler_7(tgestate_t   *state,
                        character_t *charptr,
                        vischar_t    *IY)
{
  // POP charptr
  *charptr++ = 0x05;
  *charptr   = 0x00;
}

/**
 * $C889:
 */
void charevnt_handler_9_player_sits(tgestate_t   *state,
                                    character_t *charptr,
                                    vischar_t    *IY)
{
  // POP HL
  player_sits(state);
}

/**
 * $C88D:
 */
void charevnt_handler_8_player_sleeps(tgestate_t   *state,
                                      character_t *charptr,
                                      vischar_t    *IY)
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
  uint8_t    A;

  state->byte_A13E = 0;

  if (state->bell)
    guards_persue_prisoners(state);

  if (state->food_discovered_counter != 0 &&
      --state->food_discovered_counter == 0)
  {
    item_structs[item_FOOD].item &= ~itemstruct_ITEM_FLAG_POISONED;
    item_discovered(state, item_FOOD);
  }

  IY = &state->vischars[1];
  B = vischars_LENGTH - 1;
  do
  {
    if (IY->flags != 0)
    {
      A = IY->character & vischar_BYTE0_MASK; // character index
      if (A < 20)
      {
        is_item_discoverable(state);
        if (state->red_flag || state->automatic_player_counter > 0)
          guards_follow_suspicious_player(state, IY);

        if (A > 15)
        {
          // 16,17,18,19 // could these be the dogs?
          if (item_structs[item_FOOD].room & itemstruct_ROOM_FLAG_ITEM_NEARBY)
            IY->flags = 3; // dead dog?
        }
      }

      character_behaviour_stuff(state, IY);
    }
    IY++;
  }
  while (--B);

  if (!state->red_flag && (state->morale_1 || state->automatic_player_counter == 0))
  {
    IY = &state->vischars[0];
    character_behaviour_stuff(state, IY);
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $C918: Character behaviour stuff.
 *
 * \param[in] state Pointer to game state.
 * \param[in] IY    Pointer to visible character.
 */
void character_behaviour_stuff(tgestate_t *state, vischar_t *IY)
{
  uint8_t    A;
  uint8_t    B;
  vischar_t *HL;

  B = A = IY->b07; /* more flags */
  if (A & vischar_BYTE7_MASK)
  {
    IY->b07 = --B;
    return;
  }

  HL = IY;
#if 0
  A = *++HL; // $8021 $8041 $8061 etc.
  if (A != 0)
  {
    if (A == 1)
    {
a_1:
      DEdash = HL + 3; // w04
      HLdash = &state->player_map_position;
      *DEdash++ = *HLdash++;
      *DEdash++ = *HLdash++;
      goto jump_c9c0;
    }
    else if (A == 2)
    {
      if (state->automatic_player_counter)
        goto a_1;

      *HL++ = 0;
      sub_CB23(state, HL);
      return;
    }
    else if (A == 3)
    {
      // PUSH HL
      // EX DE, HL
      if (item_structs[item_FOOD].room & itemstruct_ROOM_FLAG_ITEM_NEARBY)
      {
        HL++;
        DE += 3; // likely w04
        *DE++ = *HL++;
        *DE++ = *HL++;
        // POP HL
        goto jump_c9c0;
      }
      else
      {
        A = 0;
        *DE = A;
        // EX DE, HL
        *++HL = 0xFF;
        *++HL = 0;
        // POP HL
        sub_CB23(state, HL);
        return;
      }
    }
    else if (A == 4)
    {
      // PUSH HL
      A = state->bribed_character;
      if (A != character_NONE)
      {
        B = vischars_LENGTH - 1;
        HL = &state->vischars[1]; // iterate over non-player characters
        do
        {
          if (*HL == A)
            goto found_bribed;
          HL++;
        }
        while (--B);
      }
      // POP HL
      *HL++ = 0;
      sub_CB23(state, HL);
      return;

found_bribed:
      HL += 15; // in
      // POP DE;
      // PUSH DE;
      DE += 3; // out
      if (state->room_index > 0)
      {
        /* Indoors */
        scale_pos(HL, DE);
      }
      else
      {
        /* Outdoors */
        *DE++ = *HL++;
        HL++;
        *DE++ = *HL++;
      }
      // POP HL;
      goto jump_c9c0;
    }
  }
  A = HL[1];
  if (A == 0)
  {
    gizzards(state, IY, A);
    return; // check
  }

jump_c9c0:
  A = *HL;
  Cdash = A;

  // replace with:
  // log2scale = 1 / 4 / 8;
  // then pass that down

  if (state->room_index)
    HLdash = &BC_becomes_A;
  else if (Cdash & vischar_BYTE1_BIT6)
    HLdash = &BC_becomes_A_times_4;
  else
    HLdash = &BC_becomes_A_times_8;

  self_CA13 = HLdash; // self-modify move_character_y:$CA13
  self_CA4B = HLdash; // self-modify move_character_x:$CA4B

  if (IY->b07 & vischar_BYTE7_BIT5)
  {
    bit5set(state, IY, A, log2scale);
    return;
  }

  // HL += 3; // probably can go
  if (move_character_y(state, IY, log2scale) == 0 &&
      move_character_x(state, IY, log2scale) == 0)
  {
    bribes_solitary_food(state, IY);
    return;
  }

  // fallthrough into ...
  gizzards(state, IY, A);
#endif
}

/**
 * $C9F5: Unknown end part.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 * \param[in] A       ...
 */
void gizzards(tgestate_t *state, vischar_t *vischar, uint8_t A)
{
  if (A != vischar->b0D)
    vischar->b0D = A | vischar_BYTE13_BIT7;
}

/**
 * $C9FF: Unknown end part.
 *
 * \param[in] state   Pointer to game state.
 * \param[in] vischar Pointer to visible character.
 * \param[in] A       ...
 * \param[in] log2scale Log2Scale factor (replaces self-modifying code in original).
 */
void bit5set(tgestate_t *state, vischar_t *vischar, uint8_t A, int log2scale)
{
  // L += 4; this can be omitted as move_character_x uses vischar, not HL
  if (move_character_x(state, vischar, log2scale) == 0 &&
      move_character_y(state, vischar, log2scale) == 0)
  {
    gizzards(state, vischar, A); // likely: couldn't move, so .. do something
    return;
  }
  // HL--; suspect this can be omitted
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

  DE = vischar->mi.pos.y - ((vischar->w04 >> 0) << log2scale);
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

  DE = vischar->mi.pos.x - ((vischar->w04 >> 8) << log2scale);
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
#if 0
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

    if ((item_structs[item_FOOD].item & itemstruct_ITEM_FLAG_POISONED) == 0)
      A = 32; /* food is not poisoned */
    else
      A = 255; /* food is poisoned */
    state->food_discovered_counter = A;

    //HL -= 2; // points to IY->target
    //*HL = 0;
    IY->target = IY->target & 0xFF00; // clear low byte

    gizzards(state, IY, A); // character_behaviour_stuff:$C9F5;
    return;
  }

  if (C & vischar_BYTE1_BIT6)
  {
    //orig:C = *--HL; // 80a3, 8083, 8063, 8003 // likely target location
    //orig:A = *--HL; // 80a2 etc

    C = IY->target >> 8;
    A = IY->target & 0xFF;

    A = element_A_of_table_7738(A)[C];
    if ((IY->target & 0xFF) & vischar_BYTE2_BIT7) // orig:(*HL & vischar_BYTE2_BIT7)
      A ^= 0x80;
#if 0
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
      sub_CB23(state, &HL->target);
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
#endif
  }

  sub_CB23(state, HL); // was fallthrough
#endif
}

/**
 * $CB23: sub_CB23
 *
 * \param[in] state Pointer to game state.
 * \param[in] HL    seems to be an arg. (target loc ptr)
 */
// R $CB23 I:A Character index?
void sub_CB23(tgestate_t *state, location_t *HL)
{
  uint8_t A;

  // PUSH HL
  A = sub_C651(state, HL);

#if 0
  if (A == 0xFF)
  {
    // POP HL
    // This entry point is used by the routine at #R$C4E0.
    if (L != 0x02) // replace with (HL != &state->vischar[0]) ?
    {
      // if not player's vischar
      if ((IY->character & vischar_BYTE0_MASK) == 0)
      {
        A = HL->target & vischar_BYTE2_MASK;
        if (A == 36)
          goto cb46; // character index
        A = 0; // defeat the next 'if' statement
      }
      if (A == 12)
        goto cb50;
    }

cb46:
    // PUSH HL
    character_event(state, HL /* loc */);
    // POP HL
    A = *HL;
    if (A == 0)
      return;

    sub_CB23(state, HL); // re-enter
    return; // exit via

cb50:
    *HL = *HL ^ 0x80;
    HL++;
    if (A & (1 << 7)) // which flag is this?
      (*HL) -= 2;
    (*HL)++;
    HL--;
    A = 0;
    return;

    // $CB5F,2 Unreferenced bytes.
  }

  if (A == 128)
    IY->flags |= vischar_BYTE1_BIT6;
  // POP DE
  memcpy(DE + 2, HL, 2);
  A = 128;
#endif
}

/* ----------------------------------------------------------------------- */

/**
 * $CB75: BC becomes A.
 *
 * \param[in] A 'A' to be widened.
 *
 * \return 'A' widened to a uint16_t.
 */
uint16_t BC_becomes_A(uint8_t A)
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
 * (Leaf.)
 *
 * \return Whatever it is.
 */
uint8_t get_A_indexed_by_C41A(tgestate_t *state)
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
  static const tinypos_t solitary_transition_thing =
  {
    0x3A, 0x2A, 0x18
  };

  /**
   * $CC31
   */
  static const uint8_t solitary_player_reset_data[6] =
  {
    0x00, 0x74, 0x64, 0x03, 0x24, 0x00
  };

  item_t    *HL;
  item_t     C;
  uint8_t    B;
  vischar_t *IY;

  /* Silence bell. */
  state->bell = bell_STOP;

  /* Seize player's held items. */
  HL = &state->items_held[0];
  C = *HL;
  *HL = item_NONE;
  item_discovered(state, C);

  HL = &state->items_held[1];
  C = *HL;
  *HL = item_NONE;
  item_discovered(state, C);

  draw_all_items(state);

  /* Reset all items. [unsure] */
  B = item__LIMIT; // all items
#if 0
  HL = &item_structs[0].room;
  do
  {
    // PUSH BC
    // PUSH HL
    A = *HL & itemstruct_ROOM_MASK;
    if (A == room_0_outdoors)
    {
      A = *--HL;
      HL += 2; // -> .pos
      // EX DE, HL
      // EX AF, AF'
      A = 0;
      do
      {
        if (in_permitted_area_end_bit_2(state, A, DE) == 0)
          goto cbd5;
      }
      while (++A != 3);

      goto next;
      
cbd5:
      // EX AF,AF'
      C = A;
      item_discovered(state, C);
    }

next:
    // POP HL
    // POP BC
    HL += 7; // stride
  }
  while (--B);

#endif
  state->vischars[0].room = room_24_solitary;
  state->current_door = 20;
  decrease_morale(state, 35);
  reset_map_and_characters(state);
  memcpy(&character_structs[0].room, &solitary_player_reset_data, 6);
  queue_message_for_display(state, message_YOU_ARE_IN_SOLITARY);
  queue_message_for_display(state, message_WAIT_FOR_RELEASE);
  queue_message_for_display(state, message_ANOTHER_DAY_DAWNS);

  state->morale_1 = 0xFF; /* inhibit user input */
  state->automatic_player_counter = 0; /* immediately take automatic control of player */
  state->vischars[0].mi.spriteset = &sprites[sprite_PRISONER_FACING_TOP_LEFT_4]; // $8015 = sprite_prisoner_tl_4;
  IY = &state->vischars[0];
  IY->b0E = 3; // character faces bottom left
  state->vischars[0].target &= 0xFF00; // ($8002) = 0; // target location - why is this storing a byte and not a word?
  transition(state, IY, &solitary_transition_thing);
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
  if (A && state->vischars[0].mi.spriteset == &sprites[sprite_GUARD_FACING_TOP_LEFT_4])
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
  if (A != room_0_outdoors)
  {
    if (is_item_discoverable_interior(state, A, NULL) == 0) // check the NULL - is the returned item needed?
      guards_persue_prisoners(state);
    return;
  }
  else
  {
    HL = &item_structs[0];
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

  HL = &item_structs[0];
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

  itemstruct = item_to_itemstruct(item);
  itemstruct->item &= ~itemstruct_ITEM_FLAG_HELD;

  itemstruct->room  = default_item_location->room_and_flags;
  itemstruct->pos.y = default_item_location->y;
  itemstruct->pos.x = default_item_location->x;

  if (room == room_0_outdoors)
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
  { ITEM_ROOM(room_9_crate,     0), 0x3E, 0x30 }, /* item_SHOVEL           */
  { ITEM_ROOM(room_10_lockpick, 0), 0x49, 0x24 }, /* item_LOCKPICK         */
  { ITEM_ROOM(room_11_papers,   0), 0x2A, 0x3A }, /* item_PAPERS           */
  { ITEM_ROOM(room_14_torch,    0), 0x32, 0x18 }, /* item_TORCH            */
  { ITEM_ROOM(room_NONE,        0), 0x24, 0x2C }, /* item_BRIBE            */
  { ITEM_ROOM(room_15_uniform,  0), 0x2C, 0x41 }, /* item_UNIFORM          */
  { ITEM_ROOM(room_19_food,     0), 0x40, 0x30 }, /* item_FOOD             */
  { ITEM_ROOM(room_1_hut1right, 0), 0x42, 0x34 }, /* item_POISON           */
  { ITEM_ROOM(room_22_redkey,   0), 0x3C, 0x2A }, /* item_RED_KEY          */
  { ITEM_ROOM(room_11_papers,   0), 0x1C, 0x22 }, /* item_YELLOW_KEY       */
  { ITEM_ROOM(room_0_outdoors,  0), 0x4A, 0x48 }, /* item_GREEN_KEY        */
  { ITEM_ROOM(room_NONE,        0), 0x1C, 0x32 }, /* item_RED_CROSS_PARCEL */
  { ITEM_ROOM(room_18_radio,    0), 0x24, 0x3A }, /* item_RADIO            */
  { ITEM_ROOM(room_NONE,        0), 0x1E, 0x22 }, /* item_PURSE            */
  { ITEM_ROOM(room_NONE,        0), 0x34, 0x1C }, /* item_COMPASS          */
};

#undef ITEM_ROOM

/* ----------------------------------------------------------------------- */

/**
 * $CD9A: Maps characters to sprite sets (maybe).
 */
const character_meta_data_t character_meta_data[4] =
{
  { &character_related_pointers[0], &sprites[sprite_COMMANDANT_FACING_TOP_LEFT_4] },
  { &character_related_pointers[0], &sprites[sprite_GUARD_FACING_TOP_LEFT_4] },
  { &character_related_pointers[0], &sprites[sprite_DOG_FACING_TOP_LEFT_1] },
  { &character_related_pointers[0], &sprites[sprite_PRISONER_FACING_TOP_LEFT_4] },
};

/* ----------------------------------------------------------------------- */

/**
 * $CDAA: (unknown)
 */
const uint8_t byte_CDAA[8][9] =
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

/* ----------------------------------------------------------------------- */

/**
 * $CD9A: (unknown)
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

const uint8_t _cf06[] = { 0x01,0x04,0x04,0xFF,0x00,0x00,0x00,0x0A };
const uint8_t _cf0e[] = { 0x01,0x05,0x05,0xFF,0x00,0x00,0x00,0x8A };
const uint8_t _cf16[] = { 0x01,0x06,0x06,0xFF,0x00,0x00,0x00,0x88 };
const uint8_t _cf1e[] = { 0x01,0x07,0x07,0xFF,0x00,0x00,0x00,0x08 };
const uint8_t _cf26[] = { 0x04,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x01,0x02,0x00,0x00,0x02,0x02,0x00,0x00,0x03 };
const uint8_t _cf3a[] = { 0x04,0x01,0x01,0x03,0x00,0x02,0x00,0x80,0x00,0x02,0x00,0x81,0x00,0x02,0x00,0x82,0x00,0x02,0x00,0x83 };
const uint8_t _cf4e[] = { 0x04,0x02,0x02,0x00,0xFE,0x00,0x00,0x04,0xFE,0x00,0x00,0x05,0xFE,0x00,0x00,0x06,0xFE,0x00,0x00,0x07 };
const uint8_t _cf62[] = { 0x04,0x03,0x03,0x01,0x00,0xFE,0x00,0x84,0x00,0xFE,0x00,0x85,0x00,0xFE,0x00,0x86,0x00,0xFE,0x00,0x87 };
const uint8_t _cf76[] = { 0x01,0x00,0x00,0xFF,0x00,0x00,0x00,0x00 };
const uint8_t _cf7e[] = { 0x01,0x01,0x01,0xFF,0x00,0x00,0x00,0x80 };
const uint8_t _cf86[] = { 0x01,0x02,0x02,0xFF,0x00,0x00,0x00,0x04 };
const uint8_t _cf8e[] = { 0x01,0x03,0x03,0xFF,0x00,0x00,0x00,0x84 };
const uint8_t _cf96[] = { 0x02,0x00,0x01,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80 };
const uint8_t _cfa2[] = { 0x02,0x01,0x02,0xFF,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x04 };
const uint8_t _cfae[] = { 0x02,0x02,0x03,0xFF,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x84 };
const uint8_t _cfba[] = { 0x02,0x03,0x00,0xFF,0x00,0x00,0x00,0x84,0x00,0x00,0x00,0x00 };
const uint8_t _cfc6[] = { 0x02,0x04,0x04,0x02,0x02,0x00,0x00,0x0A,0x02,0x00,0x00,0x0B };
const uint8_t _cfd2[] = { 0x02,0x05,0x05,0x03,0x00,0x02,0x00,0x8A,0x00,0x02,0x00,0x8B };
const uint8_t _cfde[] = { 0x02,0x06,0x06,0x00,0xFE,0x00,0x00,0x88,0xFE,0x00,0x00,0x89 };
const uint8_t _cfea[] = { 0x02,0x07,0x07,0x01,0x00,0xFE,0x00,0x08,0x00,0xFE,0x00,0x09 };
const uint8_t _cff6[] = { 0x02,0x04,0x05,0xFF,0x00,0x00,0x00,0x0A,0x00,0x00,0x00,0x8A };
const uint8_t _d002[] = { 0x02,0x05,0x06,0xFF,0x00,0x00,0x00,0x8A,0x00,0x00,0x00,0x88 };
const uint8_t _d00e[] = { 0x02,0x06,0x07,0xFF,0x00,0x00,0x00,0x88,0x00,0x00,0x00,0x08 };
const uint8_t _d01a[] = { 0x02,0x07,0x04,0xFF,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x0A };

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
    C = room_0_outdoors;

  E = state->map_position[0];
  D = state->map_position[1];

  B  = item__LIMIT;
  HL = &item_structs[0];
  do
  {
    if ((HL->room & itemstruct_ROOM_MASK) == C &&
        HL->t1 > E - 2 && HL->t1 < E + 23 &&
        HL->t2 > D - 1 && HL->t2 < D + 16)
      HL->room |= itemstruct_ROOM_FLAG_BIT6 | itemstruct_ROOM_FLAG_ITEM_NEARBY; /* set */
    else
      HL->room &= ~(itemstruct_ROOM_FLAG_BIT6 | itemstruct_ROOM_FLAG_ITEM_NEARBY); /* reset */

    HL++;
  }
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $DBEB: (unknown)
 *
 * Iterates over all item_structs looking for nearby items.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Nothing. // seems to return via Adash and IY
 */
void sub_DBEB(tgestate_t *state)
{
  uint8_t       B;
  itemstruct_t *Outer_HL;
  itemstruct_t *HL;
  uint16_t      HLdash;
  uint16_t      BCdash;

  BCdash = 42; // initialise -- unsure where BCdash comes from in the original code

  B = 16; /* iterations */
  Outer_HL = &item_structs[0];
  do
  {
    const enum itemstruct_flags FLAGS = itemstruct_ROOM_FLAG_BIT6 | itemstruct_ROOM_FLAG_ITEM_NEARBY;

    if ((Outer_HL->room & FLAGS) == FLAGS)
    {
      HL = Outer_HL; // in original points to &HL->room;
      HLdash = HL->pos.y * 8; // was HLdash = *++HL * 8; // pos.y position // original calls out to multiply + expand

      // A &= A; // clear carry?

      if (HLdash > BCdash) // can't tell where BCdash is initialised
      {
        //HL++; // x position
        HLdash = HL->pos.x * 8;

        // not clearing carry?

        if (HLdash > BCdash) // where is BCdash initialised? or is this unbanked BC?
        {
#if 0
          HLdash = HL; // points to pos.x
          DEdash = *HLdash-- * 8; // x position
          BCdash = *HLdash-- * 8; // y position
          HLdash--; // point to item
          IY = HLdash; // IY is not banked

          // fetch iter count
          A = (16 - B) | (1 << 6);
          // EX AF, AF' // unpaired // returns the value in Adash
#endif
        }
      }
    }
    Outer_HL++;
  }
  while (--B);
}

/* ----------------------------------------------------------------------- */

/**
 * $DC41: (unknown)
 *
 * sprite plotting setup foo
 *
 * \param[in] state Pointer to game state.
 */
void sub_DC41(tgestate_t *state, vischar_t *IY, uint8_t A)
{
  location_t *HL;

  // plot some items?

  A &= 0x3F; // some item max mask (which ought to be 0x1F)
  state->possibly_holds_an_item = A; // This is written to but never read from. (A memory breakpoint set in FUSE confirmed this).
  HL = IY + 2;
#if 0
  DE = &state->tinypos_81B2;
  BC = 5;
  // LDIR
  // EX DE, HL
  *HL = B;
  HL = &item_definitions[A].height;
  A = *HL;
  state->item_height = A;
  memcpy(&bitmap_pointer, ++HL, 4); // copy bitmap and mask pointers
  sub_DD02(state);
  if (!Z)
    return;

  // PUSH BC
  // PUSH DE
  A = E;
  self_E2C2 = A; // self modify
  A = B;
  if (A == 0)
  {
    A = 0x77; // 0b01110111
    Adash = C;
  }
  else
  {
    A = 0;
    Adash = 3 - C;
  }

  Cdash = Adash;

  HLdash = &masked_sprite_plotter_16_jump_table[0];
  Bdash = 3; /* iterations */
  do
  {
    Edash = *HLdash++;
    Ddash = *HLdash;
    *DEdash = A;
    HLdash++;
    Edash = *HLdash++;
    Ddash = *HLdash++;
    *DEdash = A;
    if (--Cdash == 0)
      A |= 0x77;
  }
  while (--Bdash);

  A = D;
  A &= A;
  DE = 0;
  if (Z)
  {
    HL = &state->map_position[1];
    A = state->map_position_related_2 - *HL;
    HL = A * 192;
    // EX DE, HL
  }
  ;
  A = state->map_position_related_1;
  HL = &state->map_position[0];
  A -= *HL;
  HL = A;
  if (carry)
    H = 0xFF;
  state->word_81A2 = HL + DE + &state->window_buf[0]; // screen buffer start address
  HL = &state->mask_buffer[0];
  // POP DE
  // PUSH DE
  L += D * 4;
  ($81B0) = HL;
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
  bitmap_pointer += DE;
  mask_pointer += DE;
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
 */
const void *masked_sprite_plotter_16_jump_table[6] =
{
};

/**
 * $E0EC:
 */
const void *masked_sprite_plotter_24_jump_table[10] =
{
};

/**
 * $E102: Sprite plotter. Used for characters and objects.
 */
void masked_sprite_plotter_24_wide(tgestate_t *state, vischar_t *IY)
{
  uint8_t A;

#if 0
  if ((A = IY[24] & 7) >= 4) // IY[24] == (IY->w18 & 0xFF)
    goto unaligned;

  A = (~A & 3) * 8; // jump table offset

  self_E161 = A; // self-modify // set branch target of second jump
  self_E143 = A; // self-modify // set branch target of first jump

  maskptr   = mask_pointer; // mask pointer
  bitmapptr = bitmap_pointer; // bitmap pointer

  iters = self_E121; /* iterations */ // height? // self modified by $E49D (setup_sprite_plotting)
  do
  {
    // load B,C,E
    bm0 = *bitmapptr++; /* bitmap bytes */
    bm1 = *bitmapptr++;
    bm2 = *bitmapptr++;
    // load B',C',E'
    mask0 = *maskptr++; /* mask bytes */
    mask1 = *maskptr++;
    mask2 = *maskptr++;

    if (flip_sprite & (1<<7))
      flip_24_masked_pixels(state, E, B, C, E', B', C');

    foremaskptr = foreground_mask_pointer;
    screenptr = word_81A2; // screen ptr // moved compared to the other routines

    // Shift bitmap.

    bm3 = 0;
    goto $E144; // self-modified // jump table
    SRL bm0 // 0 // carry = bm0 & 1; bm0 >>= 1;
    RR bm1       // carry_out = bm1 & 1; bm1 = (bm1 >> 1) | (carry << 7); carry = carry_out;
    RR bm2       // carry_out = bm2 & 1; bm2 = (bm2 >> 1) | (carry << 7); carry = carry_out;
    RR bm3       // carry_out = bm3 & 1; bm3 = (bm3 >> 1) | (carry << 7); carry = carry_out;
    SRL bm0 // 1
    RR bm1
    RR bm2
    RR bm3
    SRL bm0 // 2
    RR bm1
    RR bm2
    RR bm3

    // Shift mask.

    mask3 = 0xFF;
    carry = 1;
    goto $E162; // self-modified // jump table
    RR mask0 // 0 // carry_out = mask0 & 1; mask0 = (mask0 >> 1) | (carry << 7); carry = carry_out;
    RR mask1      // carry_out = mask1 & 1; mask1 = (mask1 >> 1) | (carry << 7); carry = carry_out;
    RR mask2      // carry_out = mask2 & 1; mask2 = (mask2 >> 1) | (carry << 7); carry = carry_out;
    RR mask3      // carry_out = mask3 & 1; mask3 = (mask3 >> 1) | (carry << 7); carry = carry_out;
    RR mask0 // 1
    RR mask1
    RR mask2
    RR mask3
    RR mask0 // 2
    RR mask1
    RR mask2
    RR mask3

    // Plot, using foreground mask.

    A = ((~*foremaskptr | mask0) & *screenptr) | (bm0 & *foremaskptr);
    foremaskptr++;

    *screenptr++ = A;          // jump target 0
    A = ((~*foremaskptr | mask1) & *screenptr) | (bm1 & *foremaskptr);
    foremaskptr++;

    *screenptr++ = A;          // jump target 2
    A = ((~*foremaskptr | mask2) & *screenptr) | (bm2 & *foremaskptr);
    foremaskptr++;

    *screenptr++ = A;          // jump target 4
    A = ((~*foremaskptr | mask3) & *screenptr) | (bm3 & *foremaskptr);
    foremaskptr++;
    foreground_mask_pointer = foremaskptr;

    *screenptr = A;            // jump target 6
    screenptr += 21; // stride (24 - 3)
    word_81A2 = screenptr;
  }
  while (--iters);

  return;


unaligned:
  A -= 4;
  // RLCA
  // RLCA
  // RLCA
  self_E22A = A; // self-modify: set branch target - second jump
  self_E204 = A; // self-modify: set branch target - first jump
  HLdash = mask_pointer;
  HL = bitmap_pointer;
  B = 32; /* iterations */
  do
  {
    // PUSH BC
    B = *HL++;
    C = *HL++;
    E = *HL++;
    // PUSH HL
    // EXX
    B = *HL++;
    C = *HL++;
    E = *HL++;
    // PUSH HL
    if (flip_sprite & (1<<7))
      flip_24_masked_pixels();
    HL = foreground_mask_pointer;
    // EXX
    HL = word_81A2;
    D = 0;
    goto $E205; // self-modified to jump into ...;
    SLA E
    RL C
    RL B
    RL D
    SLA E
    RL C
    RL B
    RL D
    SLA E
    RL C
    RL B
    RL D
    SLA E
    RL C
    RL B
    RL D
    // EXX
    D = 255;
    // SCF
    goto $E22B; // self-modified to jump into ...;
    RL E
    RL C
    RL B
    RL D
    RL E
    RL C
    RL B
    RL D
    RL E
    RL C
    RL B
    RL D
    RL E
    RL C
    RL B
    RL D

    A = ~*HL | D;       // 1
    // EXX
    A &= *HL;
    EX AF,AF'
    A = D;
    // EXX
    A &= *HL;
    D = A;
    // EX AF,AF'
    A |= D;
    L++;
    // EXX

    *HL++ = A;          // jump target 2
    // EXX
    A = ~*HL | B;       // 2
    // EXX
    A &= *HL;
    // EX AF,AF'
    A = B;
    // EXX
    A &= *HL;
    B = A;
    // EX AF,AF'
    A |= B;
    L++;
    // EXX

    *HL++ = A;          // jump target 3
    // EXX
    A = ~*HL | C;       // 3
    // EXX
    A &= *HL;
    // EX AF,AF'
    A = C;
    // EXX
    A &= *HL;
    C = A;
    // EX AF,AF'
    A |= C;
    L++;
    // EXX

    *HL++ = A;          // jump target 5
    // EXX
    A = ~*HL | E;       // 4
    // EXX
    A &= *HL;
    // EX AF,AF'
    A = E;
    // EXX
    A &= *HL;
    E = A;
    // EX AF,AF'
    A |= E;
    L++;
    foreground_mask_pointer = HL;
    // POP HL
    // EXX

    *HL = A;            // jump target 7
    HL += 21;
    word_81A2 = HL;
    // POP HL
    // POP BC
  }
  while (--B);
#endif
}

/**
 * $E29F:
 */
void masked_sprite_plotter_16_wide_case_1_searchlight(tgestate_t *state)
{
  // A = 0;
  // goto $E2AC;
}

/**
 * $E2A2:
 */
void masked_sprite_plotter_16_wide_case_1(tgestate_t *state)
{
  uint8_t A;

#if 0
  // D $E2A2 Sprite plotter. Used for characters and objects.
  // D $E2A2 Looks like it plots a two byte-wide sprite with mask into a three byte-wide destination.

  if ((A = IY[24] & 7) >= 4)
    goto masked_sprite_plotter_16_wide_case_2;

  masked_sprite_plotter_16_wide_case_1_common(state, A); // ie. fallthrough
#endif
}

/**
 * $E2AC: Sprite plotter. Shifts right.
 */
void masked_sprite_plotter_16_wide_case_1_common(tgestate_t *state, uint8_t A)
{
#if 0
  A = (~A & 3) * 6; // jump table offset
  self_E2DC = A; // self-modify - first jump
  self_E2F4 = A; // self-modify - second jump
  maskptr = mask_pointer; // maskptr = HL'  // observed: $D505 (a mask)
  bitmapptr = bitmap_pointer; // bitmapptr = HL  // observed: $D256 (a bitmap)

  B = self_E121; /* iterations */ // height? // self modified by $E49D (setup_sprite_plotting)
  do
  {
    bm0 = *bitmapptr++; // D
    bm1 = *bitmapptr++; // E
    mask0 = *maskptr++; // D'
    mask1 = *maskptr++; // E'

    if (flip_sprite & (1<<7))
      flip_16_masked_pixels();

    // I'm assuming foremaskptr to be a foreground mask pointer based on it being
    // incremented by four each step, like a supertile wide thing.
    foremaskptr = foreground_mask_pointer;  // observed: $8100 (mask buffer)

    // Shift mask.

    mask2 = 0xFF; // all bits set => mask OFF (that would match the observed stored mask format)
    carry = 1; // mask OFF
    goto $E2DD; // self modified // jump table

    // RR = 9-bit rotation to the right
    RR mask0 // 0 // carry_out = mask0 & 1; mask0 = (mask0 >> 1) | (carry << 7); carry = carry_out;
    RR mask1      // carry_out = mask1 & 1; mask1 = (mask1 >> 1) | (carry << 7); carry = carry_out;
    RR mask2      // carry_out = mask2 & 1; mask2 = (mask2 >> 1) | (carry << 7); carry = carry_out;
    RR mask0 // 1
    RR mask1
    RR mask2
    RR mask0 // 2
    RR mask1
    RR mask2

    // Shift bitmap.

    bm2 = 0; // all bits clear => pixels OFF
    A &= A; // I do not grok this. Setting carry flag?
    goto $E2F5; // self modified // jump table

    SRL bm0 // 0 // carry = bm0 & 1; bm0 >>= 1;
    RR bm1       // carry_out = bm1 & 1; bm1 = (bm1 >> 1) | (carry << 7); carry = carry_out;
    RR bm2       // carry_out = bm2 & 1; bm2 = (bm2 >> 1) | (carry << 7); carry = carry_out;
    SRL bm0 // 1
    RR bm1
    RR bm2
    SRL bm0 // 2
    RR bm1
    RR bm2

    // Plot, using foreground mask.

    screenptr = word_81A2;
    A = ((~*foremaskptr | mask0) & *screenptr) | (bm0 & *foremaskptr);
    foremaskptr++;

    *screenptr++ = A; // entry point jump0
    A = ((~*foremaskptr | mask1) & *screenptr) | (bm1 & *foremaskptr);
    foremaskptr++;

    *screenptr++ = A; // entry point jump2
    A = ((~*foremaskptr | mask2) & *screenptr) | (bm2 & *foremaskptr);
    foremaskptr += 2;
    foreground_mask_pointer = foremaskptr;

    *screenptr = A; // entry point jump4

    screenptr += 22; // stride (24 - 2)
    word_81A2 = screenptr;
  }
  while (--B);
#endif
}

/**
 * $E34E: Sprite plotter. Shifts left. Used for characters and objects. Similar variant to above routine.
 */
void masked_sprite_plotter_16_wide_case_2(tgestate_t *state)
{
  uint8_t A;

#if 0
  A = (A - 4) * 6; // jump table offset
  self_E39A = A; // self-modify - first jump
  self_E37D = A; // self-modify - second jump
  maskptr = mask_pointer;
  bitmapptr = bitmap_pointer;

  B = 32; /* iterations */ // height? // self modified
  do
  {
    bm1 = *bitmapptr++; // numbering of the masks ... unsure
    bm2 = *bitmapptr++;
    mask1 = *maskptr++;
    mask2 = *maskptr++;

    if (flip_sprite & (1<<7))
      flip_16_masked_pixels();

    foremaskptr = foreground_mask_pointer;

    // Shift mask.

    mask0 = 0xFF; // all bits set => mask OFF (that would match the observed stored mask format)
    carry = 1; // mask OFF
    goto $E37E; // self modified // jump table
    // RL = 9-bit rotation to the left
    RL mask2 // 0 // carry_out = mask2 >> 7; mask2 = (mask2 << 1) | (carry << 0); carry = carry_out;
    RL mask1      // carry_out = mask1 >> 7; mask1 = (mask1 << 1) | (carry << 0); carry = carry_out;
    RL mask0      // carry_out = mask0 >> 7; mask0 = (mask0 << 1) | (carry << 0); carry = carry_out;
    RL mask2 // 1
    RL mask1
    RL mask0
    RL mask2 // 2
    RL mask1
    RL mask0
    RL mask2 // 3 // four groups of shifting in this routine, compared to three above.
    RL mask1
    RL mask0

    // Shift bitmap.

    bm0 = 0; // all bits clear => pixels OFF
    goto $E39B; // self modified // jump table
    SLA bm2 // 0 // carry = bm2 >> 7; bm2 <<= 1;
    RL bm1       // carry_out = bm1 >> 7; bm1 = (bm1 << 1) | (carry << 0); carry = carry_out;
    RL bm0       // carry_out = bm0 >> 7; bm0 = (bm0 << 1) | (carry << 0); carry = carry_out;
    SLA bm2
    RL bm1
    RL bm0
    SLA bm2
    RL bm1
    RL bm0
    SLA bm2
    RL bm1
    RL bm0

    // Plot, using foreground mask.

    screenptr = word_81A2;
    A = ((~*foremaskptr | mask0) & *screenptr) | (bm0 & *foremaskptr);
    foremaskptr++;

    *screenptr++ = A; // entry point jump1
    A = ((~*foremaskptr | mask1) & *screenptr) | (bm1 & *foremaskptr);
    foremaskptr++;

    *screenptr++ = A; // entry point jump3
    A = ((~*foremaskptr | mask2) & *screenptr) | (bm2 & *foremaskptr);
    foremaskptr += 2;
    foreground_mask_pointer = foremaskptr;

    *screenptr = A; // entry point jump5

    screenptr += 22; // stride (24 - 2)
    word_81A2 = screenptr;
  }
  while (--B);
#endif
}


/**
 * $E3FA: Takes the 24 pixels in E,B,C and reverses them bitwise.
 * Does the same for the mask pixels in E',B',C'.
 *
 * \param[in,out] state Pointer to game state.
 * \param[in,out] E     Pointer to pixels.
 * \param[in,out] C     Pointer to pixels.
 * \param[in,out] B     Pointer to pixels.
 * \param[in,out] Edash Pointer to mask.
 * \param[in,out] Cdash Pointer to mask.
 * \param[in,out] Bdash Pointer to mask.
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

  HL = &state->reversed[0];

  B = HL[*pE];
  C = HL[*pC];
  E = HL[*pB];

  *pE = E;
  *pB = B;
  *pC = C;

  B = HL[*pEdash];
  C = HL[*pCdash];
  E = HL[*pBdash];

  *pEdash = E;
  *pCdash = C;
  *pBdash = B;

#if 0 // original code
  H = 0x7F;  // HL = 0x7F00 | (DE & 0x00FF); // 0x7F00 -> table of bit reversed bytes
  L = E;
  E = B;     // DE = (DE & 0xFF00) | (BC >> 8);
  B = *HL;   // BC = (*HL << 8) | (BC & 0xFF);
  L = E;     // HL = (HL & 0xFF00) | (DE & 0xFF);
  E = *HL;   // DE = (DE & 0xFF00) | *HL;
  L = C;     // HL = (HL & 0xFF00) | (BC & 0xFF);
  C = *HL;   // BC = (BC & 0xFF00) | *HL;

  /* Roll the mask. */
  Hdash = 0x7F;
  Ldash = Edash;
  Edash = Bdash;
  Bdash = *HLdash;
  Ldash = Edash;
  Edash = *HLdash;
  Ldash = Cdash;
  Cdash = *HLdash;
#endif
}

/**
 * $E40F: Takes the 16 pixels in D,E and reverses them bitwise.
 * Does the same for the mask pixels in D',E'.
 *
 * \param[in,out] state Pointer to game state.
 * \param[in,out] D     Pointer to pixels.
 * \param[in,out] E     Pointer to pixels.
 * \param[in,out] Ddash Pointer to mask.
 * \param[in,out] Edash Pointer to mask.
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
 * \param[in] state Pointer to game state.
 * \param[in] IY    ...
 * //HL must be passed in too (disassembly says it's always the same as IY)
 */
void setup_sprite_plotting(tgestate_t *state, vischar_t *IY)
{
#if 0
  HL += 15;
  DE = &state->tinypos_81B2;
  if (state->room_index)
  {
    /* Indoors. */

    *DE++ = *HL++;
    HL++;
    *DE++ = *HL++;
    HL++;
    *DE++ = *HL++;
    HL++;
  }
  else
  {
    /* Outdoors. */

    A = *HL++;
    C = *HL;
    divide_by_8_with_rounding(C, A);
    *DE++ = A;
    HL++;
    B = 2; // 2 iterations
    do
    {
      A = *HL++;
      C = *HL;
      divide_by_8(C, A);
      *DE++ = A;
      HL++;
    }
    while (--B);
  }
  C = *HL++;
  B = *HL++;
  // PUSH BC
  state->flip_sprite = *HL++;  // set left/right flip flag

  B = 2; // 2 iterations
  do
  {
    Adash = *HL++;
    C = *HL++;
    divide_by_8(C, Adash);
    *DE++ = Adash;
  }
  while (--B);

  // POP DE
  DE += A * 6;
  L += 2;
  // EX DE,HL
  *DE++ = *HL++; // width in bytes
  *DE++ = *HL++; // height in rows
  memcpy(bitmap_pointer, HL, 4); // copy bitmap pointer and mask pointer
  sub_BAF7(state);
  if (A)
    return;
  // PUSH BC
  // PUSH DE
  A = IY[30];
  if (A == 3)
  {
    // 3 => 16 wide (4 => 24 wide)
    self_E2C2 = E; // self-modify
    self_E363 = E; // self-modify
    A = 3;
    HL = masked_sprite_plotter_16_jump_table;
  }
  else
  {
    A = E;
    self_E121 = A; // self-modify
    self_E1E2 = A; // self-modify
    A = 4;
    HL = masked_sprite_plotter_24_jump_table;
  }
  // PUSH HL
  self_E4C0 = A; // self-modify
  E = A;
  A = B;
  if (A == 0)
  {
    A = 0x77;

    Adash = C;
  }
  else
  {
    A = 0;

    Adash = E - C;
  }

  // POP HLdash
  Cdash = Adash;

  Bdash = self_E4C0; /* iterations */ // self modified by $E4A9
  do
  {
    Edash = *HLdash++;
    Ddash = *HLdash++;
    *DEdash = A;
    Edash = *HLdash++;
    Ddash = *HLdash++;
    *DEdash = A;
    Cdash--;
    if (Z)
      A ^= 0x77;
  }
  while (--Bdash);

  A = D;
  A &= A;
  DE = 0;
  if (Z)
  {
    HL = state->map_position[1] * 8;
    // EX DE,HL
    L = IY[26];
    H = IY[27];
    A &= A;
    // SBC HL,DE
    HL *= 24;
    // EX DE,HL
  }
  HL = map_position_related_1 - (map_position & 0xFF); // ie. low byte of map_position
  if (HL < 0)
    H = 0xFF;
  state->word_81A2 = HL + DE + &state->window_buf[0]; // screen buffer start address
  HL = &state->mask_buffer[0];
  // POP DE
  // PUSH DE
  L += D * 4 + (IY[26] & 7) * 4;
  foreground_mask_pointer = HL;
  // POP DE
  A = D;
  if (A)
  {
    D = A;
    A = 0;
    E = IY[30] - 1;
    do
      A += E;
    while (--D);
  }
  E = A;
  bitmap_pointer += DE;
  mask_pointer += DE;
  // POP BC
#endif
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

// { byte count+flags; ... }

/* $E55F */
const uint8_t outdoors_mask_0[] =
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
const uint8_t outdoors_mask_1[] =
{
  0x12, 0x02, 0x91, 0x01, 0x02, 0x91, 0x01, 0x02,
  0x91, 0x01, 0x02, 0x91, 0x01, 0x02, 0x91, 0x01,
  0x02, 0x91, 0x01, 0x02, 0x91, 0x01, 0x02, 0x91,
  0x01, 0x02, 0x91, 0x01, 0x02, 0x91, 0x01
};

/* $E61E */
const uint8_t outdoors_mask_2[] =
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
const uint8_t outdoors_mask_3[] =
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
const uint8_t outdoors_mask_4[] =
{
  0x0D, 0x02, 0x8C, 0x01, 0x02, 0x8C, 0x01, 0x02,
  0x8C, 0x01, 0x02, 0x8C, 0x01
};

/* $E758 */
const uint8_t outdoors_mask_5[] =
{
  0x0E, 0x02, 0x8C, 0x01, 0x12, 0x02, 0x8C, 0x01,
  0x12, 0x02, 0x8C, 0x01, 0x12, 0x02, 0x8C, 0x01,
  0x12, 0x02, 0x8C, 0x01, 0x12, 0x02, 0x8C, 0x01,
  0x12, 0x02, 0x8C, 0x01, 0x12, 0x02, 0x8C, 0x01,
  0x12, 0x02, 0x8D, 0x01, 0x02, 0x8D, 0x01
};

/* $E77F */
const uint8_t outdoors_mask_6[] =
{
  0x08, 0x5B, 0x5A, 0x86, 0x00, 0x01, 0x01, 0x5B,
  0x5A, 0x84, 0x00, 0x84, 0x01, 0x5B, 0x5A, 0x00,
  0x00, 0x86, 0x01, 0x5B, 0x5A, 0xD8, 0x01
};


/* $E796 */
const uint8_t outdoors_mask_7[] =
{
  0x09, 0x88, 0x01, 0x12, 0x88, 0x01, 0x12, 0x88,
  0x01, 0x12, 0x88, 0x01, 0x12, 0x88, 0x01, 0x12,
  0x88, 0x01, 0x12, 0x88, 0x01, 0x12, 0x88, 0x01,
  0x12
};

/* $E7AF */
const uint8_t outdoors_mask_8[] =
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
const uint8_t outdoors_mask_9[] =
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
const uint8_t outdoors_mask_10[] =
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
const uint8_t outdoors_mask_11[] =
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
const uint8_t outdoors_mask_12[] =
{
  0x02, 0x56, 0x57, 0x56, 0x57, 0x58, 0x59, 0x58,
  0x59, 0x58, 0x59, 0x58, 0x59, 0x58, 0x59, 0x58,
  0x59
};

/* $E940 */
const uint8_t outdoors_mask_13[] =
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
const uint8_t outdoors_mask_14[] =
{
  0x04, 0x19, 0x83, 0x00, 0x19, 0x17, 0x15, 0x00,
  0x19, 0x17, 0x18, 0x17, 0x19, 0x1A, 0x1B, 0x17,
  0x19, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1D,
  0x19, 0x1A, 0x1C, 0x1D, 0x19, 0x1A, 0x1C, 0x1D,
  0x19, 0x1A, 0x1C, 0x1D, 0x00, 0x20, 0x1C, 0x1D
};

/* $E99A */
const uint8_t outdoors_mask_15[] =
{
  0x02, 0x04, 0x32, 0x01, 0x01
};

/* $E99F */
const uint8_t outdoors_mask_16[] =
{
  0x09, 0x86, 0x00, 0x5D, 0x5C, 0x54, 0x84, 0x00,
  0x5D, 0x5C, 0x01, 0x01, 0x01, 0x00, 0x00, 0x5D,
  0x5C, 0x85, 0x01, 0x5D, 0x5C, 0x87, 0x01, 0x2B,
  0x88, 0x01
};

/* $E9B9 */
const uint8_t outdoors_mask_17[] =
{
  0x05, 0x00, 0x00, 0x5D, 0x5C, 0x67, 0x5D, 0x5C,
  0x83, 0x01, 0x3C, 0x84, 0x01
};

/* $E9C6 */
const uint8_t outdoors_mask_18[] =
{
  0x02, 0x5D, 0x68, 0x3C, 0x69
};

/* $E9CB */
const uint8_t outdoors_mask_19[] =
{
  0x0A, 0x86, 0x00, 0x5D, 0x5C, 0x46, 0x47, 0x84,
  0x00, 0x5D, 0x5C, 0x83, 0x01, 0x39, 0x00, 0x00,
  0x5D, 0x5C, 0x86, 0x01, 0x5D, 0x5C, 0x88, 0x01,
  0x4A, 0x89, 0x01
};

/* $E9E6 */
const uint8_t outdoors_mask_20[] =
{
  0x06, 0x5D, 0x5C, 0x01, 0x47, 0x6A, 0x00, 0x4A,
  0x84, 0x01, 0x6B, 0x00, 0x84, 0x01, 0x5F
};

/* $E9F5 */
const uint8_t outdoors_mask_21[] =
{
  0x04, 0x05, 0x4C, 0x00, 0x00, 0x61, 0x65, 0x66,
  0x4C, 0x61, 0x12, 0x02, 0x60, 0x61, 0x12, 0x02,
  0x60, 0x61, 0x12, 0x02, 0x60, 0x61, 0x12, 0x02,
  0x60
};

/* $EA0E */
const uint8_t outdoors_mask_22[] =
{
  0x04, 0x00, 0x00, 0x05, 0x4C, 0x05, 0x63, 0x64,
  0x60, 0x61, 0x12, 0x02, 0x60, 0x61, 0x12, 0x02,
  0x60, 0x61, 0x12, 0x02, 0x60, 0x61, 0x12, 0x02,
  0x60, 0x61, 0x12, 0x62, 0x00
};

/* $EA2B */
const uint8_t outdoors_mask_23[] =
{
  0x03, 0x00, 0x6C, 0x00, 0x02, 0x01, 0x68, 0x02,
  0x01, 0x69
};

/* $EA35 */
const uint8_t outdoors_mask_24[] =
{
  0x05, 0x01, 0x5E, 0x4C, 0x00, 0x00, 0x01, 0x01,
  0x32, 0x30, 0x00, 0x84, 0x01, 0x5F
};

/* $EA43 */
const uint8_t outdoors_mask_25[] =
{
  0x02, 0x6E, 0x5A, 0x6D, 0x39, 0x3C, 0x39
};

/* $EA4A */
const uint8_t outdoors_mask_26[] =
{
  0x04, 0x5D, 0x5C, 0x46, 0x47, 0x4A, 0x01, 0x01,
  0x39
};

/* $EA53 */
const uint8_t outdoors_mask_27[] =
{
  0x03, 0x2C, 0x47, 0x00, 0x00, 0x61, 0x12, 0x00,
  0x61, 0x12
};

/* $EA5D */
const uint8_t outdoors_mask_28[] =
{
  0x03, 0x00, 0x45, 0x1E, 0x02, 0x60, 0x00, 0x02,
  0x60, 0x00
};

/* $EA67 */
const uint8_t outdoors_mask_29[] =
{
  0x05, 0x45, 0x1E, 0x2C, 0x47, 0x00, 0x2C, 0x47,
  0x45, 0x1E, 0x12, 0x00, 0x61, 0x12, 0x61, 0x12,
  0x00, 0x61, 0x5F, 0x00, 0x00
};

/* ----------------------------------------------------------------------- */

/**
 * $EA7C: stru_EA7C. 47 x 7-byte structs.
 */
const sevenbyte_t stru_EA7C[47] =
{
  { 0x1B, 0x7B, 0x7F, 0xF1, 0xF3, 0x36, 0x28 },
  { 0x1B, 0x77, 0x7B, 0xF3, 0xF5, 0x36, 0x18 },
  { 0x1B, 0x7C, 0x80, 0xF1, 0xF3, 0x32, 0x2A },
  { 0x19, 0x83, 0x86, 0xF2, 0xF7, 0x18, 0x24 },
  { 0x19, 0x81, 0x84, 0xF4, 0xF9, 0x18, 0x1A },
  { 0x19, 0x81, 0x84, 0xF3, 0xF8, 0x1C, 0x17 },
  { 0x19, 0x83, 0x86, 0xF4, 0xF8, 0x16, 0x20 },
  { 0x18, 0x7D, 0x80, 0xF4, 0xF9, 0x18, 0x1A },
  { 0x18, 0x7B, 0x7E, 0xF3, 0xF8, 0x22, 0x1A },
  { 0x18, 0x79, 0x7C, 0xF4, 0xF9, 0x22, 0x10 },
  { 0x18, 0x7B, 0x7E, 0xF4, 0xF9, 0x1C, 0x17 },
  { 0x18, 0x79, 0x7C, 0xF1, 0xF6, 0x2C, 0x1E },
  { 0x18, 0x7D, 0x80, 0xF2, 0xF7, 0x24, 0x22 },
  { 0x1D, 0x7F, 0x82, 0xF6, 0xF7, 0x1C, 0x1E },
  { 0x1D, 0x82, 0x85, 0xF2, 0xF3, 0x23, 0x30 },
  { 0x1D, 0x86, 0x89, 0xF2, 0xF3, 0x1C, 0x37 },
  { 0x1D, 0x86, 0x89, 0xF4, 0xF5, 0x18, 0x30 },
  { 0x1D, 0x80, 0x83, 0xF1, 0xF2, 0x28, 0x30 },
  { 0x1C, 0x81, 0x82, 0xF4, 0xF6, 0x1C, 0x20 },
  { 0x1C, 0x83, 0x84, 0xF4, 0xF6, 0x1C, 0x2E },
  { 0x1A, 0x7E, 0x80, 0xF5, 0xF7, 0x1C, 0x20 },
  { 0x12, 0x7A, 0x7B, 0xF2, 0xF3, 0x3A, 0x28 },
  { 0x12, 0x7A, 0x7B, 0xEF, 0xF0, 0x45, 0x35 },
  { 0x17, 0x80, 0x85, 0xF4, 0xF6, 0x1C, 0x24 },
  { 0x14, 0x80, 0x84, 0xF3, 0xF5, 0x26, 0x28 },
  { 0x15, 0x84, 0x85, 0xF6, 0xF7, 0x1A, 0x1E },
  { 0x15, 0x7E, 0x7F, 0xF3, 0xF4, 0x2E, 0x26 },
  { 0x16, 0x7C, 0x85, 0xEF, 0xF3, 0x32, 0x22 },
  { 0x16, 0x79, 0x82, 0xF0, 0xF4, 0x34, 0x1A },
  { 0x16, 0x7D, 0x86, 0xF2, 0xF6, 0x24, 0x1A },
  { 0x10, 0x76, 0x78, 0xF5, 0xF7, 0x36, 0x0A },
  { 0x10, 0x7A, 0x7C, 0xF3, 0xF5, 0x36, 0x0A },
  { 0x10, 0x7E, 0x80, 0xF1, 0xF3, 0x36, 0x0A },
  { 0x10, 0x82, 0x84, 0xEF, 0xF1, 0x36, 0x0A },
  { 0x10, 0x86, 0x88, 0xED, 0xEF, 0x36, 0x0A },
  { 0x10, 0x8A, 0x8C, 0xEB, 0xED, 0x36, 0x0A },
  { 0x11, 0x73, 0x75, 0xEB, 0xED, 0x0A, 0x30 },
  { 0x11, 0x77, 0x79, 0xED, 0xEF, 0x0A, 0x30 },
  { 0x11, 0x7B, 0x7D, 0xEF, 0xF1, 0x0A, 0x30 },
  { 0x11, 0x7F, 0x81, 0xF1, 0xF3, 0x0A, 0x30 },
  { 0x11, 0x83, 0x85, 0xF3, 0xF5, 0x0A, 0x30 },
  { 0x11, 0x87, 0x89, 0xF5, 0xF7, 0x0A, 0x30 },
  { 0x10, 0x84, 0x86, 0xF4, 0xF7, 0x0A, 0x30 },
  { 0x11, 0x87, 0x89, 0xED, 0xEF, 0x0A, 0x30 },
  { 0x11, 0x7B, 0x7D, 0xF3, 0xF5, 0x0A, 0x0A },
  { 0x11, 0x79, 0x7B, 0xF4, 0xF6, 0x0A, 0x0A },
  { 0x0F, 0x88, 0x8C, 0xF5, 0xF8, 0x0A, 0x0A },
};

/* ----------------------------------------------------------------------- */

/**
 * $EBC5: Probably mask data pointers.
 */
const uint8_t *exterior_mask_pointers[36] =
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

/* ----------------------------------------------------------------------- */

/**
 * $EC01: 58 x 8-byte structs.
 *
 * { ?, y,y, x,x, ?, ?, ? }
 */
const eightbyte_t exterior_mask_data[58] =
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

/* ----------------------------------------------------------------------- */

/**
 * $EDD3: Game screen start addresses.
 *
 * Absolute addresses in the original code. These are now offsets.
 */
const uint16_t game_screen_start_addresses[128] =
{
  0x0047,
  0x0147,
  0x0247,
  0x0347,
  0x0447,
  0x0547,
  0x0647,
  0x0747,
  0x0067,
  0x0167,
  0x0267,
  0x0367,
  0x0467,
  0x0567,
  0x0667,
  0x0767,
  0x0087,
  0x0187,
  0x0287,
  0x0387,
  0x0487,
  0x0587,
  0x0687,
  0x0787,
  0x00A7,
  0x01A7,
  0x02A7,
  0x03A7,
  0x04A7,
  0x05A7,
  0x06A7,
  0x07A7,
  0x00C7,
  0x01C7,
  0x02C7,
  0x03C7,
  0x04C7,
  0x05C7,
  0x06C7,
  0x07C7,
  0x00E7,
  0x01E7,
  0x02E7,
  0x03E7,
  0x04E7,
  0x05E7,
  0x06E7,
  0x07E7,
  0x0807,
  0x0907,
  0x0A07,
  0x0B07,
  0x0C07,
  0x0D07,
  0x0E07,
  0x0F07,
  0x0827,
  0x0927,
  0x0A27,
  0x0B27,
  0x0C27,
  0x0D27,
  0x0E27,
  0x0F27,
  0x0847,
  0x0947,
  0x0A47,
  0x0B47,
  0x0C47,
  0x0D47,
  0x0E47,
  0x0F47,
  0x0867,
  0x0967,
  0x0A67,
  0x0B67,
  0x0C67,
  0x0D67,
  0x0E67,
  0x0F67,
  0x0887,
  0x0987,
  0x0A87,
  0x0B87,
  0x0C87,
  0x0D87,
  0x0E87,
  0x0F87,
  0x08A7,
  0x09A7,
  0x0AA7,
  0x0BA7,
  0x0CA7,
  0x0DA7,
  0x0EA7,
  0x0FA7,
  0x08C7,
  0x09C7,
  0x0AC7,
  0x0BC7,
  0x0CC7,
  0x0DC7,
  0x0EC7,
  0x0FC7,
  0x08E7,
  0x09E7,
  0x0AE7,
  0x0BE7,
  0x0CE7,
  0x0DE7,
  0x0EE7,
  0x0FE7,
  0x1007,
  0x1107,
  0x1207,
  0x1307,
  0x1407,
  0x1507,
  0x1607,
  0x1707,
  0x1027,
  0x1127,
  0x1227,
  0x1327,
  0x1427,
  0x1527,
  0x1627,
  0x1727,
};

/* ----------------------------------------------------------------------- */

/**
 * $EED3: Plot the game screen.
 *
 * \param[in] state Pointer to game state.
 */
void plot_game_screen(tgestate_t *state)
{
  uint8_t *const screen = &state->speccy->screen[0];
  uint8_t         A;
  uint8_t        *HL;
  const uint16_t *SP;
  uint8_t        *DE;
  uint8_t         Bdash;
  uint8_t         B;
  uint8_t         C;
  uint8_t         tmp;

  A = (state->plot_game_screen_x & 0xFF00) >> 8;
  if (A == 0)
  {
    HL = &state->window_buf[1] + (state->plot_game_screen_x & 0xFF);
    SP = &state->game_screen_start_addresses[0];
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
    HL = &state->window_buf[1] + (state->plot_game_screen_x & 0xFF);
    A  = *HL++;
    SP = &state->game_screen_start_addresses[0];
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
  /* Range checking. X in (0x72..0x7C) and Y in (0x6A..0x72). */
  DE = map_ROLL_CALL_X; // note the confused coords business
  HL = &state->player_map_position.y;
  B = 2;
  do
  {
    A = *HL++;
    if (A < ((DE >> 8) & 0xFF) || A >= ((DE >> 0) & 0xFF))
      goto not_at_roll_call;

    DE = map_ROLL_CALL_Y;
  }
  while (--B);

  /* All characters turn forward. */
  vischar = &state->vischars[0];
  B = vischars_LENGTH;
  do
  {
    vischar->b0D = 0x80; // movement
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
  static const tinypos_t word_EFF9 = { 0xD6, 0x8A, 0x06 };

  uint16_t  DE;
  uint8_t  *HL;
  uint8_t   B;
  uint8_t   A;

  /* Is the player within the main gate bounds? */
  /* Range checking. X in (0x69..0x6D) and Y in (0x49..0x4B). */
  DE = map_MAIN_GATE_X; // note the confused coords business
  HL = &state->player_map_position.y;
  B = 2;
  do
  {
    A = *HL++;
    if (A < ((DE >> 8) & 0xFF) || A >= ((DE >> 0) & 0xFF))
      return;

    DE = map_MAIN_GATE_Y;
  }
  while (--B);

  /* Using the papers at the main gate when not in uniform causes the player
   * to be sent to solitary */
  if (state->vischars[0].mi.spriteset != &sprites[sprite_GUARD_FACING_TOP_LEFT_4])
  {
    solitary(state);
    return;
  }

  increase_morale_by_10_score_by_50(state);
  state->vischars[0].room = room_0_outdoors;

  /* Transition to outside the main gate. */
  transition(state, &state->vischars[0], &word_EFF9); // DOES NOT RETURN
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

  uint16_t BC;
  uint8_t  A;

  screenlocstring_plot(state, &screenlocstring_confirm_y_or_n);

  /* Keyscan. */
  for (;;)
  {
    BC = port_KEYBOARD_POIUY;
    A = state->speccy->in(state->speccy, BC);
    if ((A & (1 << 4)) == 0)
      return 0; /* is 'Y' pressed? return Z */

    BC = port_KEYBOARD_SPACESYMSHFTMNB;
    A = state->speccy->in(state->speccy, BC);
    A = ~A;
    if ((A & (1 << 3)) != 0)
      return 1; /* is 'N' pressed? return NZ */
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
    0x00,
    0x00,
    0x012C, // target
    0x2E2E,
    0x18,
    0x00,
    0xCDF2,
    0xCF76,
    0x00,
    0x00,
    0x00,
    { { 0x0000, 0x0000, 0x0018 }, &sprites[sprite_PRISONER_FACING_TOP_LEFT_4], 0 },
    0x0000,
    0x0000,
    room_0_outdoors,
    0x00,
    0x00,
    0x00,
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
  const screenlocstring_t key_choice_screenlocstrings[] =
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

const screenlocstring_t define_key_prompts[6] =
{
  { 0x006D, 11, "CHOOSE KEYS" },
  { 0x00CD,  5, "LEFT." },
  { 0x080D,  6, "RIGHT." },
  { 0x084D,  3, "UP." },
  { 0x088D,  5, "DOWN." },
  { 0x08CD,  5, "FIRE." },
};

const uint8_t keyboard_port_hi_bytes[10] =
{
  /* The first byte here is unused AFAICT. */
  /* Final zero byte is a terminator. */
  0x24, 0xF7, 0xEF, 0xFB, 0xDF, 0xFD, 0xBF, 0xFE, 0x7F, 0x00,
};

const char counted_strings[] = "\x05" "ENTER"
                               "\x04" "CAPS"   /* CAPS SHIFT */
                               "\x06" "SYMBOL" /* SYMBOL SHIFT */
                               "\x05" "SPACE";

/* Each table entry is a character (in original code: a glyph index) OR if
 * its top bit is set then bottom seven bits are an index into
 * counted_strings. */
#define O(n) (0x80 + n)
const unsigned char key_tables[8 * 5] =
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

/* Screen offsets where to plot key names. (in original code: absolute addresses). */
const uint16_t key_name_screen_offsets[5] =
{
  0x00D5,
  0x0815,
  0x0855,
  0x0895,
  0x08D5,
};

/* ----------------------------------------------------------------------- */

/**
 * $F335: Wipe the game screen.
 *
 * \param[in] state Pointer to game state.
 */
void wipe_game_screen(tgestate_t *state)
{
  uint8_t *const screen = &state->speccy->screen[0];

  const uint16_t *sp;
  uint8_t         A;

  sp = &game_screen_start_addresses[0]; /* points to offsets */
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
  uint8_t                  B;
  const screenlocstring_t *HL;
  uint8_t                 *DE;
  uint8_t                  Binner;
  const char              *HLstring;
  uint16_t                 BC;
  const uint16_t          *HLoffset;
  const uint16_t          *HLkeybytes;
  uint8_t                 *self_F3E9;

  for (;;)
  {
    wipe_game_screen(state);
    set_game_screen_attributes(state, attribute_WHITE_OVER_BLACK);

    /* Draw key choice prompt strings */
    B = NELEMS(define_key_prompts); /* iterations */
    HL = &define_key_prompts[0];
    do
    {
      // PUSH BC
      DE = &state->speccy->screen[HL->screenloc];
      Binner = HL->length; /* iterations */
      HLstring = HL->string;
      do
      {
        // A = *HLstring; /* This is redundant when calling plot_glyph(). */
        plot_glyph(HLstring, DE);
        HLstring++;
      }
      while (--Binner);
      // POP BC
    }
    while (--B);

    /* Wipe keydefs */
    memset(&state->keydefs.defs[0], 0, 10);

    B = 5; /* iterations */ /* L/R/U/D/F */
    HLoffset = &key_name_screen_offsets[0];
    do
    {
#if 0
      // PUSH BC
      DE = *HLoffset++;
      // PUSH HLaddrs
      self_F3E9 = /*screenaddr + */ DE; // self modify screen addr
      A = 0xFF;

f38b:
      for (;;)
      {
        HLkeybytes = &keyboard_port_hi_bytes[-1];
        D = 0xFF;
        do
        {
          HL++;
          D++;
          Adash = *HLkeybytes;
          if (Adash == 0) /* Hit end of keyboard_port_hi_bytes. */
            goto f38b;

          Adash = ~state->speccy->in(state->speccy, (Adash << 8) | port_BORDER);
          E = Adash;
          C = 0x20;

f3a0:
          C >>= 1;
        }
        while (carry);  // this loop structure is not quite right
        Adash = C & E;
        if (Adash == 0)
          goto f3a0;

        if (A)
          goto f38b;
        A = D;

        HL = &state->keydefs[-1]; // is this pointing to the byte before?
f3b1:
        HL++;
        Adash = *HL;
        if (Adash == 0)
          goto f3c1;
        if (A != B) ...
          HL++; // interleaved
        ... goto f3b1;
        Adash = *HL;
        if (A != C)
          goto f3b1;
      }

f3c1:
      *HL++ = B;
      *HL = C;

      // A = row
      HL = &key_tables[A * 5] - 1; /* off by one to compensate for preincrement */
      do
      {
        HL++;
        carry_out = C & 1; C = (C >> 1) | (carry << 7); carry = carry_out; // RR C;
      }
      while (!carry);

      B = 1;
      A = *HL;
      A |= A;
      // JP P, f3e8 // P => 'sign positive'  == MSB of accumulator == ie. if (A & (1<<7)) then jump
      A &= 0x7F;
      HL = &counted_strings[0] + A;
      B = *HL++;

f3e8:
      DE = self_F3E9; // self modified // screen address
      do
      {
        // PUSH BC
        A = *HL;
        plot_glyph(HL, DE);
        HL++;
        // POP BC
      }
      while (--B);
      // POP HL;
      // POP BC;
#endif
    }
    while (--B);

    /* Delay loop */
    BC = 0xFFFF;
    while (--BC)
      ;

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
