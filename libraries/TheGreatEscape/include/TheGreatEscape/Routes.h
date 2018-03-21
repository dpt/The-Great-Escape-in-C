/**
 * Routes.h
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
 * The recreated version is copyright (c) 2012-2018 David Thomas
 */

#ifndef ROUTES_H
#define ROUTES_H

#include <stdint.h>

typedef uint8_t routeindex_t;

/* Flag set to reverse a route. */
#define routeindexflag_REVERSED (1 << 7)

enum
{
  routeindex_0_HALT,

  routeindex_1_FENCED_AREA,           // improve this desc
  routeindex_2_GUARD_PERIMETER_WALK,
  routeindex_3_COMMANDANT,            // longest of the routes
  routeindex_4_GUARD_MARCHING_OVER_MAIN_GATE,

  routeindex_5_EXIT_HUT2,
  routeindex_6_EXIT_HUT3,

  routeindex_7_PRISONER_SLEEPS_1,
  routeindex_8_PRISONER_SLEEPS_2,
  routeindex_9_PRISONER_SLEEPS_3,
  routeindex_10_PRISONER_SLEEPS_1,    // dupe
  routeindex_11_PRISONER_SLEEPS_2,    // dupe
  routeindex_12_PRISONER_SLEEPS_3,    // dupe

  routeindex_13_77DE,                 // 13: (all hostiles) by character_bed_common

  routeindex_14_GO_TO_YARD,           // 14: set by set_route_go_to_yard_reversed (for hero, prisoners_and_guards-1), set_route_go_to_yard (for hero, prisoners_and_guards-1)
  routeindex_15_GO_TO_YARD,           // 15: set by set_route_go_to_yard_reversed (for hero, prisoners_and_guards-2), set_route_go_to_yard (for hero, prisoners_and_guards-2)  /* dupes index 14 */

  routeindex_16_BREAKFAST_25,         // 16: set by set_route_go_to_breakfast, end_of_breakfast (for hero, reversed), prisoners_and_guards-1 by end_of_breakfast
  routeindex_17_BREAKFAST_23,         // 17:                                                                          prisoners_and_guards-2 by end_of_breakfast

  routeindex_18_PRISONER_SITS_1,      // 18: character_20_PRISONER_1 by charevnt_breakfast_common  // character next to player when player seated
  routeindex_19_PRISONER_SITS_2,      // 19: character_21_PRISONER_2 by charevnt_breakfast_common
  routeindex_20_PRISONER_SITS_3,      // 20: character_22_PRISONER_3 by charevnt_breakfast_common
  routeindex_21_PRISONER_SITS_1,      // 21: character_23_PRISONER_4 by charevnt_breakfast_common  /* dupes index 18 - perhaps with good reason */
  routeindex_22_PRISONER_SITS_2,      // 22: character_24_PRISONER_5 by charevnt_breakfast_common  /* dupes index 19 */
  routeindex_23_PRISONER_SITS_3,      // 23: character_25_PRISONER_6 by charevnt_breakfast_common  /* dupes index 20 */ // looks like it belongs with above chunk but not used (bug)

  routeindex_24_GUARD_A_BREAKFAST,    // 24: "even guard" by charevnt_breakfast_common
  routeindex_25_GUARD_B_BREAKFAST,    // 25: "odd guard" by charevnt_breakfast_common

  routeindex_26_GUARD_12_ROLL_CALL,   // 26: character_12_GUARD_12   by go_to_roll_call
  routeindex_27_GUARD_13_ROLL_CALL,   // 27: character_13_GUARD_13   by go_to_roll_call
  routeindex_28_PRISONER_1_ROLL_CALL, // 28: character_20_PRISONER_1 by go_to_roll_call
  routeindex_29_PRISONER_2_ROLL_CALL, // 29: character_21_PRISONER_2 by go_to_roll_call
  routeindex_30_PRISONER_3_ROLL_CALL, // 30: character_22_PRISONER_3 by go_to_roll_call
  routeindex_31_GUARD_14_ROLL_CALL,   // 31: character_14_GUARD_14   by go_to_roll_call
  routeindex_32_GUARD_15_ROLL_CALL,   // 32: character_15_GUARD_15   by go_to_roll_call
  routeindex_33_PRISONER_4_ROLL_CALL, // 33: character_23_PRISONER_4 by go_to_roll_call
  routeindex_34_PRISONER_5_ROLL_CALL, // 34: character_24_PRISONER_5 by go_to_roll_call
  routeindex_35_PRISONER_6_ROLL_CALL, // 35: character_25_PRISONER_6 by go_to_roll_call

  routeindex_36_GO_TO_SOLITARY,       // 36: set by charevnt_hero_release (for commandant?), tested by route_ended
  routeindex_37_HERO_LEAVE_SOLITARY,  // 37: set by charevnt_hero_release (for hero)

  routeindex_38_GUARD_12_BED,         // 38: guard 12 at 'time for bed', 'search light'
  routeindex_39_GUARD_13_BED,         // 39: guard 13 at 'time for bed', 'search light'
  routeindex_40_GUARD_14_BED,         // 40: guard 14 at 'time for bed', 'search light'
  routeindex_41_GUARD_15_BED,         // 41: guard 15 at 'time for bed', 'search light'

  routeindex_42_HUT2_LEFT_TO_RIGHT,   // 42:

  routeindex_43_7833,                 // 43: set by charevnt_breakfast_vischar, process_player_input (was at breakfast case)
  routeindex_44_HUT2_RIGHT_TO_LEFT,   // 44: set by process_player_input (was in bed case), set by character_bed_vischar (for the hero)

  routeindex_45_HERO_ROLL_CALL,       // 45: (hero) by go_to_roll_call

  routeindex__LIMIT,

  routeindex_255_WANDER = 255
};

#endif /* ROUTES_H */
