/**
 * RoomDefs.c
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

#include "TheGreatEscape/InteriorObjects.h"
#include "TheGreatEscape/Rooms.h"
#include "TheGreatEscape/State.h"
#include "TheGreatEscape/Types.h"

#include "TheGreatEscape/RoomDefs.h"

/* ----------------------------------------------------------------------- */

typedef uint8_t roomdef_t;

/* ----------------------------------------------------------------------- */

static const uint8_t roomdef_1_hut1_right[] =
{
  0, /* room dimensions index */
  3, /* number of boundaries */
  54, 68, 23, 34, /* boundary */
  54, 68, 39, 50, /* boundary */
  54, 68, 55, 68, /* boundary */
  4, /* number of mask bytes */
  0, 1, 3, 10, /* mask */
  10, /* nobjects */
  interiorobject_ROOM_OUTLINE_22x12_A,                   1,  4,
  interiorobject_WIDE_WINDOW_FACING_SE,                  8,  0,
  interiorobject_WIDE_WINDOW_FACING_SE,                  2,  3,
  interiorobject_OCCUPIED_BED,                          10,  5,
  interiorobject_OCCUPIED_BED,                           6,  7,
  interiorobject_DOOR_FRAME_SE,                         15,  8,
  interiorobject_ORNATE_WARDROBE_FACING_SW,             18,  5,
  interiorobject_ORNATE_WARDROBE_FACING_SW,             20,  6,
  interiorobject_EMPTY_BED_FACING_SE,                    2,  9,
  interiorobject_DOOR_FRAME_SW,                          7, 10,
};

static const uint8_t roomdef_2_hut2_left[] =
{
  1, /* room dimensions index */
  2, /* number of boundaries */
  48, 64, 43, 56, /* boundary */ // bed boundary
  24, 38, 26, 40, /* boundary */ // table boundary
  2, /* number of mask bytes */
  13, 8, /* mask */
  8, /* nobjects */
  interiorobject_ROOM_OUTLINE_18x10_A,                   3,  6,
  interiorobject_WIDE_WINDOW_FACING_SE,                  6,  2,
  interiorobject_DOOR_FRAME_NE,                         16,  5,
  interiorobject_STOVE_PIPE,                             4,  5,
  interiorobject_OCCUPIED_BED,                           8,  7, /* Hero's bed. */
  interiorobject_DOOR_FRAME_SW,                          7,  9,
  interiorobject_TABLE,                                 11, 12,
  interiorobject_SMALL_TUNNEL_ENTRANCE,                  5,  9,
};

static const uint8_t roomdef_3_hut2_right[] =
{
  0, /* room dimensions index */
  3, /* number of boundaries */
  54, 68, 23, 34, /* boundary */
  54, 68, 39, 50, /* boundary */
  54, 68, 55, 68, /* boundary */
  4, /* number of mask bytes */
  0, 1, 3, 10, /* mask */
  10, /* nobjects */
  interiorobject_ROOM_OUTLINE_22x12_A,                   1,  4,
  interiorobject_WIDE_WINDOW_FACING_SE,                  8,  0,
  interiorobject_WIDE_WINDOW_FACING_SE,                  2,  3,
  interiorobject_OCCUPIED_BED,                          10,  5,
  interiorobject_OCCUPIED_BED,                           6,  7,
  interiorobject_OCCUPIED_BED,                           2,  9,
  interiorobject_CHEST_OF_DRAWERS_FACING_SW,            16,  5,
  interiorobject_DOOR_FRAME_SE,                         15,  8,
  interiorobject_SHORT_WARDROBE_FACING_SW,              18,  5,
  interiorobject_DOOR_FRAME_SW,                          7, 10,
};

static const uint8_t roomdef_4_hut3_left[] =
{
  1, /* room dimensions index */
  2, /* number of boundaries */
  24, 40, 24, 42, /* boundary */
  48, 64, 43, 56, /* boundary */
  3, /* number of mask bytes */
  18, 20, 8, /* mask */
  9, /* nobjects */
  interiorobject_ROOM_OUTLINE_18x10_A,                   3,  6,
  interiorobject_DOOR_FRAME_NE,                         16,  5,
  interiorobject_WIDE_WINDOW_FACING_SE,                  6,  2,
  interiorobject_STOVE_PIPE,                             4,  5,
  interiorobject_EMPTY_BED_FACING_SE,                    8,  7,
  interiorobject_DOOR_FRAME_SW,                          7,  9,
  interiorobject_CHAIR_FACING_SE,                       11, 11,
  interiorobject_CHAIR_FACING_SW,                       13, 10,
  interiorobject_PAPERS_ON_FLOOR,                       14, 14,
};

static const uint8_t roomdef_5_hut3_right[] =
{
  0, /* room dimensions index */
  3, /* number of boundaries */
  54, 68, 23, 34, /* boundary */
  54, 68, 39, 50, /* boundary */
  54, 68, 55, 68, /* boundary */
  4, /* number of mask bytes */
  0, 1, 3, 10, /* mask */
  10, /* nobjects */
  interiorobject_ROOM_OUTLINE_22x12_A,                   1,  4,
  interiorobject_WIDE_WINDOW_FACING_SE,                  8,  0,
  interiorobject_WIDE_WINDOW_FACING_SE,                  2,  3,
  interiorobject_OCCUPIED_BED,                          10,  5,
  interiorobject_OCCUPIED_BED,                           6,  7,
  interiorobject_OCCUPIED_BED,                           2,  9,
  interiorobject_DOOR_FRAME_SE,                         15,  8,
  interiorobject_CHEST_OF_DRAWERS_FACING_SW,            16,  5,
  interiorobject_CHEST_OF_DRAWERS_FACING_SW,            20,  7,
  interiorobject_DOOR_FRAME_SW,                          7, 10,
};

static const uint8_t roomdef_8_corridor[] =
{
  2, /* room dimensions index */
  0, /* number of boundaries */
  1, /* number of mask bytes */
  9, /* mask */
  5, /* nobjects */
  interiorobject_ROOM_OUTLINE_18x10_B,                   3,  6,
  interiorobject_DOOR_FRAME_NW,                         10,  3,
  interiorobject_DOOR_FRAME_NW,                          4,  6,
  interiorobject_DOOR_FRAME_SW,                          5, 10,
  interiorobject_SHORT_WARDROBE_FACING_SW,              18,  6,
};

static const uint8_t roomdef_9_crate[] =
{
  1, /* room dimensions index */
  1, /* number of boundaries */
  58, 64, 28, 42, /* boundary */
  2, /* number of mask bytes */
  4, 21, /* mask */
  10, /* nobjects */
  interiorobject_ROOM_OUTLINE_18x10_A,                   3,  6,
  interiorobject_SMALL_WINDOW_WITH_BARS_FACING_SE,       6,  3,
  interiorobject_SMALL_SHELF_FACING_SE,                  9,  4,
  interiorobject_TINY_DOOR_FRAME_NE,                    12,  6,
  interiorobject_DOOR_FRAME_SE,                         13, 10,
  interiorobject_TALL_WARDROBE_FACING_SW,               16,  6,
  interiorobject_SHORT_WARDROBE_FACING_SW,              18,  8,
  interiorobject_CUPBOARD_FACING_SE,                     3,  6,
  interiorobject_SMALL_CRATE,                            6,  8,
  interiorobject_SMALL_CRATE,                            4,  9,
};

static const uint8_t roomdef_10_lockpick[] =
{
  4, /* room dimensions index */
  2, /* number of boundaries */
  69, 75, 32, 54, /* boundary */
  36, 47, 48, 60, /* boundary */
  3, /* number of mask bytes */
  6, 14, 22, /* mask */
  14, /* nobjects */
  interiorobject_ROOM_OUTLINE_22x12_B,                   1,  4,
  interiorobject_DOOR_FRAME_SE,                         15, 10,
  interiorobject_SMALL_WINDOW_WITH_BARS_FACING_SE,       4,  1,
  interiorobject_KEY_RACK_FACING_SE,                     2,  3,
  interiorobject_KEY_RACK_FACING_SE,                     7,  2,
  interiorobject_TALL_WARDROBE_FACING_SW,               10,  2,
  interiorobject_CUPBOARD_FACING_SW,                    13,  3,
  interiorobject_CUPBOARD_FACING_SW,                    15,  4,
  interiorobject_CUPBOARD_FACING_SW,                    17,  5,
  interiorobject_TABLE,                                 14,  8,
  interiorobject_CHEST_OF_DRAWERS_FACING_SW,            18,  8,
  interiorobject_CHEST_OF_DRAWERS_FACING_SW,            20,  9,
  interiorobject_SMALL_CRATE,                            6,  5,
  interiorobject_TABLE,                                  2,  6,
};

static const uint8_t roomdef_11_papers[] =
{
  4, /* room dimensions index */
  1, /* number of boundaries */
  27, 44, 36, 48, /* boundary */
  1, /* number of mask bytes */
  23, /* mask */
  9, /* nobjects */
  interiorobject_ROOM_OUTLINE_22x12_B,                   1,  4,
  interiorobject_SMALL_SHELF_FACING_SE,                  6,  3,
  interiorobject_TALL_WARDROBE_FACING_SW,               12,  3,
  interiorobject_TALL_DRAWERS_FACING_SW,                10,  3,
  interiorobject_SHORT_WARDROBE_FACING_SW,              14,  5,
  interiorobject_DOOR_FRAME_NW,                          2,  2,
  interiorobject_TALL_DRAWERS_FACING_SW,                18,  7,
  interiorobject_TALL_DRAWERS_FACING_SW,                20,  8,
  interiorobject_DESK_FACING_SW,                        12, 10,
};

static const uint8_t roomdef_12_corridor[] =
{
  1, /* room dimensions index */
  0, /* number of boundaries */
  2, /* number of mask bytes */
  4, 7, /* mask */
  4, /* nobjects */
  interiorobject_ROOM_OUTLINE_18x10_A,                   3,  6,
  interiorobject_SMALL_WINDOW_WITH_BARS_FACING_SE,       6,  3,
  interiorobject_DOOR_FRAME_SW,                          9, 10,
  interiorobject_DOOR_FRAME_SE,                         13, 10,
};

static const uint8_t roomdef_13_corridor[] =
{
  1, /* room dimensions index */
  0, /* number of boundaries */
  2, /* number of mask bytes */
  4, 8, /* mask */
  6, /* nobjects */
  interiorobject_ROOM_OUTLINE_18x10_A,                   3,  6,
  interiorobject_DOOR_FRAME_NW,                          6,  3,
  interiorobject_DOOR_FRAME_SW,                          7,  9,
  interiorobject_DOOR_FRAME_SE,                         13, 10,
  interiorobject_TALL_DRAWERS_FACING_SW,                12,  5,
  interiorobject_CHEST_OF_DRAWERS_FACING_SW,            14,  7,
};

static const uint8_t roomdef_14_torch[] =
{
  0, /* room dimensions index */
  3, /* number of boundaries */
  54, 68, 22, 32, /* boundary */
  62, 68, 48, 58, /* boundary */
  54, 68, 54, 68, /* boundary */
  1, /* number of mask bytes */
  1, /* mask */
  9, /* nobjects */
  interiorobject_ROOM_OUTLINE_22x12_A,                   1,  4,
  interiorobject_DOOR_FRAME_NW,                          4,  3,
  interiorobject_TINY_DRAWERS_FACING_SE,                 8,  5,
  interiorobject_EMPTY_BED_FACING_SE,                   10,  5,
  interiorobject_CHEST_OF_DRAWERS_FACING_SW,            16,  5,
  interiorobject_SHORT_WARDROBE_FACING_SW,              18,  5,
  interiorobject_DOOR_FRAME_NE,                         20,  4,
  interiorobject_SMALL_SHELF_FACING_SE,                  2,  7,
  interiorobject_EMPTY_BED_FACING_SE,                    2,  9,
};

static const uint8_t roomdef_15_uniform[] =
{
  0, /* room dimensions index */
  4, /* number of boundaries */
  54, 68, 22, 32, /* boundary */
  54, 68, 54, 68, /* boundary */
  62, 68, 40, 58, /* boundary */
  30, 40, 56, 67, /* boundary */
  4, /* number of mask bytes */
  1, 5, 10, 15, /* mask */
  10, /* nobjects */
  interiorobject_ROOM_OUTLINE_22x12_A,                   1,  4,
  interiorobject_SHORT_WARDROBE_FACING_SW,              16,  4,
  interiorobject_EMPTY_BED_FACING_SE,                   10,  5,
  interiorobject_TINY_DRAWERS_FACING_SE,                 8,  5,
  interiorobject_TINY_DRAWERS_FACING_SE,                 6,  6,
  interiorobject_SMALL_SHELF_FACING_SE,                  2,  7,
  interiorobject_EMPTY_BED_FACING_SE,                    2,  9,
  interiorobject_DOOR_FRAME_SW,                          7, 10,
  interiorobject_DOOR_FRAME_SE,                         13,  9,
  interiorobject_TABLE,                                 18,  8,
};

static const uint8_t roomdef_16_corridor[] =
{
  1, /* room dimensions index */
  0, /* number of boundaries */
  2, /* number of mask bytes */
  4, 7, /* mask */
  4, /* nobjects */
  interiorobject_ROOM_OUTLINE_18x10_A,                   3,  6,
  interiorobject_DOOR_FRAME_NW,                          4,  4,
  interiorobject_DOOR_FRAME_SW,                          9, 10,
  interiorobject_DOOR_FRAME_SE,                         13, 10,
};

static const uint8_t roomdef_7_corridor[] =
{
  1, /* room dimensions index */
  0, /* number of boundaries */
  1, /* number of mask bytes */
  4, /* mask */
  4, /* nobjects */
  interiorobject_ROOM_OUTLINE_18x10_A,                   3,  6,
  interiorobject_DOOR_FRAME_NW,                          4,  4,
  interiorobject_DOOR_FRAME_SE,                         13, 10,
  interiorobject_TALL_WARDROBE_FACING_SW,               12,  4,
};

static const uint8_t roomdef_18_radio[] =
{
  4, /* room dimensions index */
  3, /* number of boundaries */
  38, 56, 48, 60, /* boundary */
  38, 46, 39, 60, /* boundary */
  22, 32, 48, 60, /* boundary */
  5, /* number of mask bytes */
  11, 17, 16, 24, 25, /* mask */
  10, /* nobjects */
  interiorobject_ROOM_OUTLINE_22x12_B,                   1,  4,
  interiorobject_CUPBOARD_FACING_SE,                     1,  4,
  interiorobject_SMALL_WINDOW_WITH_BARS_FACING_SE,       4,  1,
  interiorobject_SMALL_SHELF_FACING_SE,                  7,  2,
  interiorobject_DOOR_FRAME_NE,                         10,  1,
  interiorobject_TABLE,                                 12,  7,
  interiorobject_MESS_BENCH_SHORT,                      12,  9,
  interiorobject_TABLE,                                 18, 10,
  interiorobject_TINY_TABLE,                            16, 12,
  interiorobject_DOOR_FRAME_SW,                          5,  7,
};

static const uint8_t roomdef_19_food[] =
{
  1, /* room dimensions index */
  1, /* number of boundaries */
  52, 64, 47, 56, /* boundary */
  1, /* number of mask bytes */
  7, /* mask */
  11, /* nobjects */
  interiorobject_ROOM_OUTLINE_18x10_A,                   3,  6,
  interiorobject_SMALL_WINDOW_WITH_BARS_FACING_SE,       6,  3,
  interiorobject_CUPBOARD_FACING_SE,                     9,  3,
  interiorobject_CUPBOARD_FACING_SW,                    12,  3,
  interiorobject_CUPBOARD_FACING_SW,                    14,  4,
  interiorobject_TABLE,                                  9,  6,
  interiorobject_SMALL_SHELF_FACING_SE,                  3,  5,
  interiorobject_SINK_FACING_SE,                         3,  7,
  interiorobject_CHEST_OF_DRAWERS_FACING_SW,            14,  7,
  interiorobject_DOOR_FRAME_NE,                         16,  5,
  interiorobject_DOOR_FRAME_SW,                          9, 10,
};

static const uint8_t roomdef_20_redcross[] =
{
  1, /* room dimensions index */
  2, /* number of boundaries */
  58, 64, 26, 42, /* boundary */
  50, 64, 46, 54, /* boundary */
  2, /* number of mask bytes */
  21, 4, /* mask */
  11, /* nobjects */
  interiorobject_ROOM_OUTLINE_18x10_A,                   3,  6,
  interiorobject_DOOR_FRAME_SE,                         13, 10,
  interiorobject_SMALL_SHELF_FACING_SE,                  9,  4,
  interiorobject_CUPBOARD_FACING_SE,                     3,  6,
  interiorobject_SMALL_CRATE,                            6,  8,
  interiorobject_SMALL_CRATE,                            4,  9,
  interiorobject_TABLE,                                  9,  6,
  interiorobject_TALL_WARDROBE_FACING_SW,               14,  5,
  interiorobject_TALL_WARDROBE_FACING_SW,               16,  6,
  interiorobject_ORNATE_WARDROBE_FACING_SW,             18,  8,
  interiorobject_TINY_TABLE,                            11,  8,
};

static const uint8_t roomdef_22_red_key[] =
{
  3, /* room dimensions index */
  2, /* number of boundaries */
  54, 64, 46, 56, /* boundary */
  58, 64, 36, 44, /* boundary */
  2, /* number of mask bytes */
  12, 21, /* mask */
  7, /* nobjects */
  interiorobject_ROOM_OUTLINE_15x8,                      5,  6,
  interiorobject_NOTICEBOARD_FACING_SE,                  4,  4,
  interiorobject_SMALL_SHELF_FACING_SE,                  9,  4,
  interiorobject_SMALL_CRATE,                            6,  8,
  interiorobject_DOOR_FRAME_SW,                          9,  8,
  interiorobject_TABLE,                                  9,  6,
  interiorobject_DOOR_FRAME_NE,                         14,  4,
};

static const uint8_t roomdef_23_breakfast[] =
{
  0, /* room dimensions index */
  1, /* number of boundaries */
  54, 68, 34, 68, /* boundary */
  2, /* number of mask bytes */
  10, 3, /* mask */
  12, /* nobjects */
  interiorobject_ROOM_OUTLINE_22x12_A,                   1,  4,
  interiorobject_SMALL_WINDOW_WITH_BARS_FACING_SE,       8,  0,
  interiorobject_SMALL_WINDOW_WITH_BARS_FACING_SE,       2,  3,
  interiorobject_DOOR_FRAME_SW,                          7, 10,
  interiorobject_MESS_TABLE,                             5,  4,
  interiorobject_CUPBOARD_FACING_SW,                    18,  4,
  interiorobject_DOOR_FRAME_NE,                         20,  4,
  interiorobject_DOOR_FRAME_SE,                         15,  8,
  interiorobject_MESS_BENCH,                             7,  6,
  interiorobject_EMPTY_BENCH,                           12,  5,
  interiorobject_EMPTY_BENCH,                           10,  6,
  interiorobject_EMPTY_BENCH,                            8,  7,
};

static const uint8_t roomdef_24_solitary[] =
{
  3, /* room dimensions index */
  1, /* number of boundaries */
  48, 54, 38, 46, /* boundary */
  1, /* number of mask bytes */
  26, /* mask */
  3, /* nobjects */
  interiorobject_ROOM_OUTLINE_15x8,                      5,  6,
  interiorobject_DOOR_FRAME_NE,                         14,  4,
  interiorobject_TINY_TABLE,                            10,  9,
};

static const uint8_t roomdef_25_breakfast[] =
{
  0, /* room dimensions index */
  1, /* number of boundaries */
  54, 68, 34, 68, /* boundary */
  0, /* number of mask bytes */
  11, /* nobjects */
  interiorobject_ROOM_OUTLINE_22x12_A,                   1,  4,
  interiorobject_SMALL_WINDOW_WITH_BARS_FACING_SE,       8,  0,
  interiorobject_CUPBOARD_FACING_SE,                     5,  3,
  interiorobject_SMALL_WINDOW_WITH_BARS_FACING_SE,       2,  3,
  interiorobject_DOOR_FRAME_NE,                         18,  3,
  interiorobject_MESS_TABLE,                             5,  4,
  interiorobject_MESS_BENCH,                             7,  6,
  interiorobject_EMPTY_BENCH,                           12,  5,
  interiorobject_EMPTY_BENCH,                           10,  6,
  interiorobject_EMPTY_BENCH,                            8,  7,
  interiorobject_EMPTY_BENCH,                           14,  4,
};

static const uint8_t roomdef_28_hut1_left[] =
{
  1, /* room dimensions index */
  2, /* number of boundaries */
  28, 40, 28, 52, /* boundary */
  48, 63, 44, 56, /* boundary */
  3, /* number of mask bytes */
  8, 13, 19, /* mask */
  8, /* nobjects */
  interiorobject_ROOM_OUTLINE_18x10_A,                   3,  6,
  interiorobject_WIDE_WINDOW_FACING_SE,                  6,  2,
  interiorobject_DOOR_FRAME_NE,                         14,  4,
  interiorobject_CUPBOARD_FACING_SE,                     3,  6,
  interiorobject_OCCUPIED_BED,                           8,  7,
  interiorobject_DOOR_FRAME_SW,                          7,  9,
  interiorobject_CHAIR_FACING_SW,                       15, 10,
  interiorobject_TABLE,                                 11, 12,
};

static const uint8_t roomdef_29_second_tunnel_start[] =
{
  5, /* room dimensions index */
  0, /* number of boundaries */
  6, /* number of mask bytes */
  30, 31, 32, 33, 34, 35, /* mask */
  6, /* nobjects */
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 20,  0,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 16,  2,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 12,  4,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                  8,  6,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                  4,  8,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                  0, 10,
};

static const uint8_t roomdef_31[] =
{
  6, /* room dimensions index */
  0, /* number of boundaries */
  6, /* number of mask bytes */
  36, 37, 38, 39, 40, 41, /* mask */
  6, /* nobjects */
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  0,  0,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  4,  2,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  8,  4,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                 12,  6,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                 16,  8,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                 20, 10,
};

static const uint8_t roomdef_36[] =
{
  7, /* room dimensions index */
  0, /* number of boundaries */
  6, /* number of mask bytes */
  31, 32, 33, 34, 35, 45, /* mask */
  5, /* nobjects */
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 20,  0,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 16,  2,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 12,  4,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                  8,  6,
  interiorobject_TUNNEL_CORNER_NE_SE,                    4,  8,
};

static const uint8_t roomdef_32[] =
{
  8, /* room dimensions index */
  0, /* number of boundaries */
  6, /* number of mask bytes */
  36, 37, 38, 39, 40, 42, /* mask */
  5, /* nobjects */
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  0,  0,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  4,  2,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  8,  4,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                 12,  6,
  interiorobject_TUNNEL_CORNER_NW_SW,                   16,  8,
};

static const uint8_t roomdef_34[] =
{
  6, /* room dimensions index */
  0, /* number of boundaries */
  6, /* number of mask bytes */
  36, 37, 38, 39, 40, 46, /* mask */
  6, /* nobjects */
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  0,  0,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  4,  2,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  8,  4,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                 12,  6,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                 16,  8,
  interiorobject_TUNNEL_ENTRANCE,                       20, 10,
};

static const uint8_t roomdef_35[] =
{
  6, /* room dimensions index */
  0, /* number of boundaries */
  6, /* number of mask bytes */
  36, 37, 38, 39, 40, 41, /* mask */
  6, /* nobjects */
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  0,  0,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  4,  2,
  interiorobject_TUNNEL_T_JOIN_NW_SE,                    8,  4,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                 12,  6,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                 16,  8,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                 20, 10,
};

static const uint8_t roomdef_30[] =
{
  5, /* room dimensions index */
  0, /* number of boundaries */
  7, /* number of mask bytes */
  30, 31, 32, 33, 34, 35, 44, /* mask */
  6, /* nobjects */
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 20,  0,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 16,  2,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 12,  4,
  interiorobject_TUNNEL_T_JOIN_SW_NE,                    8,  6,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                  4,  8,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                  0, 10,
};

static const uint8_t roomdef_40[] =
{
  9, /* room dimensions index */
  0, /* number of boundaries */
  6, /* number of mask bytes */
  30, 31, 32, 33, 34, 43, /* mask */
  6, /* nobjects */
  interiorobject_TUNNEL_CORNER_SW_SE,                   20,  0,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 16,  2,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 12,  4,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                  8,  6,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                  4,  8,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                  0, 10,
};

static const uint8_t roomdef_44[] =
{
  8, /* room dimensions index */
  0, /* number of boundaries */
  5, /* number of mask bytes */
  36, 37, 38, 39, 40, /* mask */
  5, /* nobjects */
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  0,  0,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  4,  2,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                  8,  4,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,                 12,  6,
  interiorobject_TUNNEL_CORNER_NW_NE,                   16,  8,
};

static const uint8_t roomdef_50_blocked_tunnel[] =
{
  5, /* room dimensions index */
  1, /* number of boundaries */
  52, 58, 32, 54, /* boundary */
  6, /* number of mask bytes */
  30, 31, 32, 33, 34, 43, /* mask */
  6, /* nobjects */
  interiorobject_TUNNEL_CORNER_SW_SE,                   20,  0,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 16,  2,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                 12,  4,
  interiorobject_COLLAPSED_TUNNEL_SW_NE,                 8,  6, /* collapsed_tunnel_obj */
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                  4,  8,
  interiorobject_STRAIGHT_TUNNEL_SW_NE,                  0, 10,
};

/**
 * $6BAD: Room and tunnel definitions.
 */
static const roomdef_t *const rooms_and_tunnels[room__LIMIT] =
{
  /* Array of pointers to rooms (starting with room 1). */
  &roomdef_1_hut1_right[0],
  &roomdef_2_hut2_left[0],
  &roomdef_3_hut2_right[0],
  &roomdef_4_hut3_left[0],
  &roomdef_5_hut3_right[0],
  &roomdef_8_corridor[0], /* unused */
  &roomdef_7_corridor[0],
  &roomdef_8_corridor[0],
  &roomdef_9_crate[0],
  &roomdef_10_lockpick[0],
  &roomdef_11_papers[0],
  &roomdef_12_corridor[0],
  &roomdef_13_corridor[0],
  &roomdef_14_torch[0],
  &roomdef_15_uniform[0],
  &roomdef_16_corridor[0],
  &roomdef_7_corridor[0],
  &roomdef_18_radio[0],
  &roomdef_19_food[0],
  &roomdef_20_redcross[0],
  &roomdef_16_corridor[0],
  &roomdef_22_red_key[0],
  &roomdef_23_breakfast[0],
  &roomdef_24_solitary[0],
  &roomdef_25_breakfast[0],
  &roomdef_28_hut1_left[0], /* unused */
  &roomdef_28_hut1_left[0], /* unused */
  &roomdef_28_hut1_left[0],

  /* Array of pointers to tunnels. */
  &roomdef_29_second_tunnel_start[0],
  &roomdef_30[0],
  &roomdef_31[0],
  &roomdef_32[0],
  &roomdef_29_second_tunnel_start[0],
  &roomdef_34[0],
  &roomdef_35[0],
  &roomdef_36[0],
  &roomdef_34[0],
  &roomdef_35[0],
  &roomdef_32[0],
  &roomdef_40[0],
  &roomdef_30[0],
  &roomdef_32[0],
  &roomdef_29_second_tunnel_start[0],
  &roomdef_44[0],
  &roomdef_36[0],
  &roomdef_36[0],
  &roomdef_32[0],
  &roomdef_34[0],
  &roomdef_36[0],
  &roomdef_50_blocked_tunnel[0],
  &roomdef_32[0],
  &roomdef_40[0],
};

/* ----------------------------------------------------------------------- */

/* Conv: The following functions replace the original game's direct access to
 * room definitions. The original game directly overwrites room definition
 * data to make characters sit down and sleep, but that causes multiple
 * concurrent instances of the same game to trample on each others' state.
 * The handful of bytes which are modified during the game are replaced with
 * shadow bytes held in the game's state.
 */

/* Return the index of the shadow byte for (room_index, offset). */
static INLINE int get_roomdef_shadow(int room_index, int offset)
{
  assert(room_index >= 0);
  assert(room_index < room__LIMIT);
  assert(offset < 256);

  switch (room_index)
  {
    case room_2_HUT2LEFT:
      switch (offset)
      {
        case roomdef_2_BED:
          return 0;
      }
      break;

    case room_3_HUT2RIGHT:
      switch (offset)
      {
        case roomdef_3_BED_A:
          return 1;
        case roomdef_3_BED_B:
          return 2;
        case roomdef_3_BED_C:
          return 3;
      }
      break;

    case room_5_HUT3RIGHT:
      switch (offset)
      {
        case roomdef_5_BED_D:
          return 4;
        case roomdef_5_BED_E:
          return 5;
        case roomdef_5_BED_F:
          return 6;
      }
      break;

    case room_23_MESS_HALL:
      switch (offset)
      {
        case roomdef_23_BENCH_A:
          return 7;
        case roomdef_23_BENCH_B:
          return 8;
        case roomdef_23_BENCH_C:
          return 9;
      }
      break;

    case room_25_MESS_HALL:
      switch (offset)
      {
        case roomdef_25_BENCH_D:
          return 10;
        case roomdef_25_BENCH_E:
          return 11;
        case roomdef_25_BENCH_F:
          return 12;
        case roomdef_25_BENCH_G:
          return 13;
      }
      break;

    case room_50_BLOCKED_TUNNEL:
      switch (offset)
      {
        case roomdef_50_BOUNDARY:
          return 14;
        case roomdef_50_BLOCKAGE:
          return 15;
      }
      break;
  }

  return -1;
}

/* Return the room definition byte for (room_index, offset). */
int get_roomdef(tgestate_t *state, room_t room_index, int offset)
{
  int index;

  index = get_roomdef_shadow(room_index, offset);
  if (index >= 0)
    /* Get the shadow byte where present. */
    return state->roomdef_shadow_bytes[index];
  else
    /* Fetch the real byte otherwise. */
    return rooms_and_tunnels[room_index - 1][offset]; /* array starts at 1 */
}

/* Set the room definition byte for (room_index, offset). */
void set_roomdef(tgestate_t *state,
                 room_t      room_index,
                 int         offset,
                 int         new_byte)
{
  int index;

  index = get_roomdef_shadow(room_index, offset);
  /* Allow setting of replacement bytes only. */
  if (index >= 0)
    state->roomdef_shadow_bytes[index] = new_byte;
  else
    assert("Unknown roomdef byte" == NULL);
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et
