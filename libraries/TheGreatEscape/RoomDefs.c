#include "TheGreatEscape/InteriorObjects.h"
#include "TheGreatEscape/Rooms.h"

#include "TheGreatEscape/RoomDefs.h"

/* Room defintions are not const as they are updated when objects are altered. */

/**
 * $6BAD: Room and tunnel definitions.
 */
const roomdef_t *rooms_and_tunnels[room__LIMIT] =
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

uint8_t roomdef_1_hut1_right[] =
{
  0,
  3, // number of boundaries
  54, 68, 23, 34, // boundary
  54, 68, 39, 50, // boundary
  54, 68, 55, 68, // boundary
  4, // number of TBD bytes
  0, 1, 3, 10, // TBD
  10, // nobjects
  interiorobject_ROOM_OUTLINE_2,              1,  4,
  interiorobject_WIDE_WINDOW,                 8,  0,
  interiorobject_WIDE_WINDOW,                 2,  3,
  interiorobject_OCCUPIED_BED,               10,  5,
  interiorobject_OCCUPIED_BED,                6,  7,
  interiorobject_DOOR_FRAME_15,              15,  8,
  interiorobject_WARDROBE_WITH_KNOCKERS,     18,  5,
  interiorobject_WARDROBE_WITH_KNOCKERS,     20,  6,
  interiorobject_EMPTY_BED,                   2,  9,
  interiorobject_DOOR_FRAME_16,               7, 10,
};

uint8_t roomdef_2_hut2_left[] =
{
  1,
  2, // number of boundaries
  48, 64, 43, 56, // boundary
  24, 38, 26, 40, // boundary
  2, // number of TBD bytes
  13, 8, // TBD
  8, // nobjects
  interiorobject_ROOM_OUTLINE_27,             3,  6,
  interiorobject_WIDE_WINDOW,                 6,  2,
  interiorobject_DOOR_FRAME_40,              16,  5,
  interiorobject_STOVE_PIPE,                  4,  5,
  interiorobject_OCCUPIED_BED,                8,  7, // player's bed
  interiorobject_DOOR_FRAME_16,               7,  9,
  interiorobject_TABLE_2,                    11, 12,
  interiorobject_SMALL_TUNNEL_ENTRANCE,       5,  9,
};

uint8_t roomdef_3_hut2_right[] =
{
  0,
  3, // number of boundaries
  54, 68, 23, 34, // boundary
  54, 68, 39, 50, // boundary
  54, 68, 55, 68, // boundary
  4, // number of TBD bytes
  0, 1, 3, 10, // TBD
  10, // nobjects
  interiorobject_ROOM_OUTLINE_2,              1,  4,
  interiorobject_WIDE_WINDOW,                 8,  0,
  interiorobject_WIDE_WINDOW,                 2,  3,
  interiorobject_OCCUPIED_BED,               10,  5, // bed_C
  interiorobject_OCCUPIED_BED,                6,  7, // bed_B
  interiorobject_OCCUPIED_BED,                2,  9, // bed_A
  interiorobject_CHEST_OF_DRAWERS,           16,  5,
  interiorobject_DOOR_FRAME_15,              15,  8,
  interiorobject_SHORT_WARDROBE,             18,  5,
  interiorobject_DOOR_FRAME_16,               7, 10,
};

uint8_t roomdef_4_hut3_left[] =
{
  1,
  2, // number of boundaries
  24, 40, 24, 42, // boundary
  48, 64, 43, 56, // boundary
  3, // number of TBD bytes
  18, 20, 8, // TBD
  9, // nobjects
  interiorobject_ROOM_OUTLINE_27,             3,  6,
  interiorobject_DOOR_FRAME_40,              16,  5,
  interiorobject_WIDE_WINDOW,                 6,  2,
  interiorobject_STOVE_PIPE,                  4,  5,
  interiorobject_EMPTY_BED,                   8,  7,
  interiorobject_DOOR_FRAME_16,               7,  9,
  interiorobject_CHAIR_POINTING_BOTTOM_RIGHT, 11, 11,
  interiorobject_CHAIR_POINTING_BOTTOM_LEFT, 13, 10,
  interiorobject_STUFF_31,                   14, 14,
};

uint8_t roomdef_5_hut3_right[] =
{
  0,
  3, // number of boundaries
  54, 68, 23, 34, // boundary
  54, 68, 39, 50, // boundary
  54, 68, 55, 68, // boundary
  4, // number of TBD bytes
  0, 1, 3, 10, // TBD
  10, // nobjects
  interiorobject_ROOM_OUTLINE_2,              1,  4,
  interiorobject_WIDE_WINDOW,                 8,  0,
  interiorobject_WIDE_WINDOW,                 2,  3,
  interiorobject_OCCUPIED_BED,               10,  5, // bed_D
  interiorobject_OCCUPIED_BED,                6,  7, // bed_E
  interiorobject_OCCUPIED_BED,                2,  9, // bed_F
  interiorobject_DOOR_FRAME_15,              15,  8,
  interiorobject_CHEST_OF_DRAWERS,           16,  5,
  interiorobject_CHEST_OF_DRAWERS,           20,  7,
  interiorobject_DOOR_FRAME_16,               7, 10,
};

uint8_t roomdef_8_corridor[] =
{
  2,
  0, // number of boundaries
  1, // number of TBD bytes
  9, // TBD
  5, // nobjects
  interiorobject_ROOM_OUTLINE_46,             3,  6,
  interiorobject_DOOR_FRAME_38,              10,  3,
  interiorobject_DOOR_FRAME_38,               4,  6,
  interiorobject_DOOR_FRAME_16,               5, 10,
  interiorobject_SHORT_WARDROBE,             18,  6,
};

uint8_t roomdef_9_crate[] =
{
  1,
  1, // number of boundaries
  58, 64, 28, 42, // boundary
  2, // number of TBD bytes
  4, 21, // TBD
  10, // nobjects
  interiorobject_ROOM_OUTLINE_27,             3,  6,
  interiorobject_SMALL_WINDOW,                6,  3,
  interiorobject_SMALL_SHELF,                 9,  4,
  interiorobject_DOOR_FRAME_36,              12,  6,
  interiorobject_DOOR_FRAME_15,              13, 10,
  interiorobject_TALL_WARDROBE,              16,  6,
  interiorobject_SHORT_WARDROBE,             18,  8,
  interiorobject_CUPBOARD,                    3,  6,
  interiorobject_SMALL_CRATE,                 6,  8,
  interiorobject_SMALL_CRATE,                 4,  9,
};

uint8_t roomdef_10_lockpick[] =
{
  4,
  2, // number of boundaries
  69, 75, 32, 54, // boundary
  36, 47, 48, 60, // boundary
  3, // number of TBD bytes
  6, 14, 22, // TBD
  14, // nobjects
  interiorobject_ROOM_OUTLINE_47,             1,  4,
  interiorobject_DOOR_FRAME_15,              15, 10,
  interiorobject_SMALL_WINDOW,                4,  1,
  interiorobject_KEY_RACK,                    2,  3,
  interiorobject_KEY_RACK,                    7,  2,
  interiorobject_TALL_WARDROBE,              10,  2,
  interiorobject_CUPBOARD_42,                13,  3,
  interiorobject_CUPBOARD_42,                15,  4,
  interiorobject_CUPBOARD_42,                17,  5,
  interiorobject_TABLE_2,                    14,  8,
  interiorobject_CHEST_OF_DRAWERS,           18,  8,
  interiorobject_CHEST_OF_DRAWERS,           20,  9,
  interiorobject_SMALL_CRATE,                 6,  5,
  interiorobject_TABLE_2,                     2,  6,
};

uint8_t roomdef_11_papers[] =
{
  4,
  1, // number of boundaries
  27, 44, 36, 48, // boundary
  1, // number of TBD bytes
  23, // TBD
  9, // nobjects
  interiorobject_ROOM_OUTLINE_47,             1,  4,
  interiorobject_SMALL_SHELF,                 6,  3,
  interiorobject_TALL_WARDROBE,              12,  3,
  interiorobject_DRAWERS_50,                 10,  3,
  interiorobject_SHORT_WARDROBE,             14,  5,
  interiorobject_DOOR_FRAME_38,               2,  2,
  interiorobject_DRAWERS_50,                 18,  7,
  interiorobject_DRAWERS_50,                 20,  8,
  interiorobject_DESK,                       12, 10,
};

uint8_t roomdef_12_corridor[] =
{
  1,
  0, // number of boundaries
  2, // number of TBD bytes
  4, 7, // TBD
  4, // nobjects
  interiorobject_ROOM_OUTLINE_27,             3,  6,
  interiorobject_SMALL_WINDOW,                6,  3,
  interiorobject_DOOR_FRAME_16,               9, 10,
  interiorobject_DOOR_FRAME_15,              13, 10,
};

uint8_t roomdef_13_corridor[] =
{
  1,
  0, // number of boundaries
  2, // number of TBD bytes
  4, 8, // TBD
  6, // nobjects
  interiorobject_ROOM_OUTLINE_27,             3,  6,
  interiorobject_DOOR_FRAME_38,               6,  3,
  interiorobject_DOOR_FRAME_16,               7,  9,
  interiorobject_DOOR_FRAME_15,              13, 10,
  interiorobject_DRAWERS_50,                 12,  5,
  interiorobject_CHEST_OF_DRAWERS,           14,  7,
};

uint8_t roomdef_14_torch[] =
{
  0,
  3, // number of boundaries
  54, 68, 22, 32, // boundary
  62, 68, 48, 58, // boundary
  54, 68, 54, 68, // boundary
  1, // number of TBD bytes
  1, // TBD
  9, // nobjects
  interiorobject_ROOM_OUTLINE_2,              1,  4,
  interiorobject_DOOR_FRAME_38,               4,  3,
  interiorobject_TINY_DRAWERS,                8,  5,
  interiorobject_EMPTY_BED,                  10,  5,
  interiorobject_CHEST_OF_DRAWERS,           16,  5,
  interiorobject_SHORT_WARDROBE,             18,  5,
  interiorobject_DOOR_FRAME_40,              20,  4,
  interiorobject_SMALL_SHELF,                 2,  7,
  interiorobject_EMPTY_BED,                   2,  9,
};

uint8_t roomdef_15_uniform[] =
{
  0,
  4, // number of boundaries
  54, 68, 22, 32, // boundary
  54, 68, 54, 68, // boundary
  62, 68, 40, 58, // boundary
  30, 40, 56, 67, // boundary
  4, // number of TBD bytes
  1, 5, 10, 15, // TBD
  10, // nobjects
  interiorobject_ROOM_OUTLINE_2,              1,  4,
  interiorobject_SHORT_WARDROBE,             16,  4,
  interiorobject_EMPTY_BED,                  10,  5,
  interiorobject_TINY_DRAWERS,                8,  5,
  interiorobject_TINY_DRAWERS,                6,  6,
  interiorobject_SMALL_SHELF,                 2,  7,
  interiorobject_EMPTY_BED,                   2,  9,
  interiorobject_DOOR_FRAME_16,               7, 10,
  interiorobject_DOOR_FRAME_15,              13,  9,
  interiorobject_TABLE_2,                    18,  8,
};

uint8_t roomdef_16_corridor[] =
{
  1,
  0, // number of boundaries
  2, // number of TBD bytes
  4, 7, // TBD
  4, // nobjects
  interiorobject_ROOM_OUTLINE_27,             3,  6,
  interiorobject_DOOR_FRAME_38,               4,  4,
  interiorobject_DOOR_FRAME_16,               9, 10,
  interiorobject_DOOR_FRAME_15,              13, 10,
};

uint8_t roomdef_7_corridor[] =
{
  1,
  0, // number of boundaries
  1, // number of TBD bytes
  4, // TBD
  4, // nobjects
  interiorobject_ROOM_OUTLINE_27,             3,  6,
  interiorobject_DOOR_FRAME_38,               4,  4,
  interiorobject_DOOR_FRAME_15,              13, 10,
  interiorobject_TALL_WARDROBE,              12,  4,
};

uint8_t roomdef_18_radio[] =
{
  4,
  3, // number of boundaries
  38, 56, 48, 60, // boundary
  38, 46, 39, 60, // boundary
  22, 32, 48, 60, // boundary
  5, // number of TBD bytes
  11, 17, 16, 24, 25, // TBD
  10, // nobjects
  interiorobject_ROOM_OUTLINE_47,             1,  4,
  interiorobject_CUPBOARD,                    1,  4,
  interiorobject_SMALL_WINDOW,                4,  1,
  interiorobject_SMALL_SHELF,                 7,  2,
  interiorobject_DOOR_FRAME_40,              10,  1,
  interiorobject_TABLE_2,                    12,  7,
  interiorobject_MESS_BENCH_SHORT,           12,  9,
  interiorobject_TABLE_2,                    18, 10,
  interiorobject_TINY_TABLE,                 16, 12,
  interiorobject_DOOR_FRAME_16,               5,  7,
};

uint8_t roomdef_19_food[] =
{
  1,
  1, // number of boundaries
  52, 64, 47, 56, // boundary
  1, // number of TBD bytes
  7, // TBD
  11, // nobjects
  interiorobject_ROOM_OUTLINE_27,             3,  6,
  interiorobject_SMALL_WINDOW,                6,  3,
  interiorobject_CUPBOARD,                    9,  3,
  interiorobject_CUPBOARD_42,                12,  3,
  interiorobject_CUPBOARD_42,                14,  4,
  interiorobject_TABLE_2,                     9,  6,
  interiorobject_SMALL_SHELF,                 3,  5,
  interiorobject_SINK,                        3,  7,
  interiorobject_CHEST_OF_DRAWERS,           14,  7,
  interiorobject_DOOR_FRAME_40,              16,  5,
  interiorobject_DOOR_FRAME_16,               9, 10,
};

uint8_t roomdef_20_redcross[] =
{
  1,
  2, // number of boundaries
  58, 64, 26, 42, // boundary
  50, 64, 46, 54, // boundary
  2, // number of TBD bytes
  21, 4, // TBD
  11, // nobjects
  interiorobject_ROOM_OUTLINE_27,             3,  6,
  interiorobject_DOOR_FRAME_15,              13, 10,
  interiorobject_SMALL_SHELF,                 9,  4,
  interiorobject_CUPBOARD,                    3,  6,
  interiorobject_SMALL_CRATE,                 6,  8,
  interiorobject_SMALL_CRATE,                 4,  9,
  interiorobject_TABLE_2,                     9,  6,
  interiorobject_TALL_WARDROBE,              14,  5,
  interiorobject_TALL_WARDROBE,              16,  6,
  interiorobject_WARDROBE_WITH_KNOCKERS,     18,  8,
  interiorobject_TINY_TABLE,                 11,  8,
};

uint8_t roomdef_22_red_key[] =
{
  3,
  2, // number of boundaries
  54, 64, 46, 56, // boundary
  58, 64, 36, 44, // boundary
  2, // number of TBD bytes
  12, 21, // TBD
  7, // nobjects
  interiorobject_ROOM_OUTLINE_41,             5,  6,
  interiorobject_NOTICEBOARD,                 4,  4,
  interiorobject_SMALL_SHELF,                 9,  4,
  interiorobject_SMALL_CRATE,                 6,  8,
  interiorobject_DOOR_FRAME_16,               9,  8,
  interiorobject_TABLE_2,                     9,  6,
  interiorobject_DOOR_FRAME_40,              14,  4,
};

uint8_t roomdef_23_breakfast[] =
{
  0,
  1, // number of boundaries
  54, 68, 34, 68, // boundary
  2, // number of TBD bytes
  10, 3, // TBD
  12, // nobjects
  interiorobject_ROOM_OUTLINE_2,              1,  4,
  interiorobject_SMALL_WINDOW,                8,  0,
  interiorobject_SMALL_WINDOW,                2,  3,
  interiorobject_DOOR_FRAME_16,               7, 10,
  interiorobject_MESS_TABLE,                  5,  4,
  interiorobject_CUPBOARD_42,                18,  4,
  interiorobject_DOOR_FRAME_40,              20,  4,
  interiorobject_DOOR_FRAME_15,              15,  8,
  interiorobject_MESS_BENCH,                  7,  6,
  interiorobject_EMPTY_BENCH,                12,  5,
  interiorobject_EMPTY_BENCH,                10,  6,
  interiorobject_EMPTY_BENCH,                 8,  7,
};

uint8_t roomdef_24_solitary[] =
{
  3,
  1, // number of boundaries
  48, 54, 38, 46, // boundary
  1, // number of TBD bytes
  26, // TBD
  3, // nobjects
  interiorobject_ROOM_OUTLINE_41,             5,  6,
  interiorobject_DOOR_FRAME_40,              14,  4,
  interiorobject_TINY_TABLE,                 10,  9,
};

uint8_t roomdef_25_breakfast[] =
{
  0,
  1, // number of boundaries
  54, 68, 34, 68, // boundary
  0, // number of TBD bytes
  11, // nobjects
  interiorobject_ROOM_OUTLINE_2,              1,  4,
  interiorobject_SMALL_WINDOW,                8,  0,
  interiorobject_CUPBOARD,                    5,  3,
  interiorobject_SMALL_WINDOW,                2,  3,
  interiorobject_DOOR_FRAME_40,              18,  3,
  interiorobject_MESS_TABLE,                  5,  4,
  interiorobject_MESS_BENCH,                  7,  6,
  interiorobject_EMPTY_BENCH,                12,  5,
  interiorobject_EMPTY_BENCH,                10,  6,
  interiorobject_EMPTY_BENCH,                 8,  7,
  interiorobject_EMPTY_BENCH,                14,  4,
};

uint8_t roomdef_28_hut1_left[] =
{
  1,
  2, // number of boundaries
  28, 40, 28, 52, // boundary
  48, 63, 44, 56, // boundary
  3, // number of TBD bytes
  8, 13, 19, // TBD
  8, // nobjects
  interiorobject_ROOM_OUTLINE_27,             3,  6,
  interiorobject_WIDE_WINDOW,                 6,  2,
  interiorobject_DOOR_FRAME_40,              14,  4,
  interiorobject_CUPBOARD,                    3,  6,
  interiorobject_OCCUPIED_BED,                8,  7,
  interiorobject_DOOR_FRAME_16,               7,  9,
  interiorobject_CHAIR_POINTING_BOTTOM_LEFT, 15, 10,
  interiorobject_TABLE_2,                    11, 12,
};

uint8_t roomdef_29_second_tunnel_start[] =
{
  5,
  0, // number of boundaries
  6, // number of TBD bytes
  30, 31, 32, 33, 34, 35, // TBD
  6, // nobjects
  interiorobject_TUNNEL_0,                   20,  0,
  interiorobject_TUNNEL_0,                   16,  2,
  interiorobject_TUNNEL_0,                   12,  4,
  interiorobject_TUNNEL_0,                    8,  6,
  interiorobject_TUNNEL_0,                    4,  8,
  interiorobject_TUNNEL_0,                    0, 10,
};

uint8_t roomdef_31[] =
{
  6,
  0, // number of boundaries
  6, // number of TBD bytes
  36, 37, 38, 39, 40, 41, // TBD
  6, // nobjects
  interiorobject_TUNNEL_3,                    0,  0,
  interiorobject_TUNNEL_3,                    4,  2,
  interiorobject_TUNNEL_3,                    8,  4,
  interiorobject_TUNNEL_3,                   12,  6,
  interiorobject_TUNNEL_3,                   16,  8,
  interiorobject_TUNNEL_3,                   20, 10,
};

uint8_t roomdef_36[] =
{
  7,
  0, // number of boundaries
  6, // number of TBD bytes
  31, 32, 33, 34, 35, 45, // TBD
  5, // nobjects
  interiorobject_TUNNEL_0,                   20,  0,
  interiorobject_TUNNEL_0,                   16,  2,
  interiorobject_TUNNEL_0,                   12,  4,
  interiorobject_TUNNEL_0,                    8,  6,
  interiorobject_TUNNEL_14,                   4,  8,
};

uint8_t roomdef_32[] =
{
  8,
  0, // number of boundaries
  6, // number of TBD bytes
  36, 37, 38, 39, 40, 42, // TBD
  5, // nobjects
  interiorobject_TUNNEL_3,                    0,  0,
  interiorobject_TUNNEL_3,                    4,  2,
  interiorobject_TUNNEL_3,                    8,  4,
  interiorobject_TUNNEL_3,                   12,  6,
  interiorobject_TUNNEL_17,                  16,  8,
};

uint8_t roomdef_34[] =
{
  6,
  0, // number of boundaries
  6, // number of TBD bytes
  36, 37, 38, 39, 40, 46, // TBD
  6, // nobjects
  interiorobject_TUNNEL_3,                    0,  0,
  interiorobject_TUNNEL_3,                    4,  2,
  interiorobject_TUNNEL_3,                    8,  4,
  interiorobject_TUNNEL_3,                   12,  6,
  interiorobject_TUNNEL_3,                   16,  8,
  interiorobject_TUNNEL_18,                  20, 10,
};

uint8_t roomdef_35[] =
{
  6,
  0, // number of boundaries
  6, // number of TBD bytes
  36, 37, 38, 39, 40, 41, // TBD
  6, // nobjects
  interiorobject_TUNNEL_3,                    0,  0,
  interiorobject_TUNNEL_3,                    4,  2,
  interiorobject_TUNNEL_JOIN_4,               8,  4,
  interiorobject_TUNNEL_3,                   12,  6,
  interiorobject_TUNNEL_3,                   16,  8,
  interiorobject_TUNNEL_3,                   20, 10,
};

uint8_t roomdef_30[] =
{
  5,
  0, // number of boundaries
  7, // number of TBD bytes
  30, 31, 32, 33, 34, 35, 44, // TBD
  6, // nobjects
  interiorobject_TUNNEL_0,                   20,  0,
  interiorobject_TUNNEL_0,                   16,  2,
  interiorobject_TUNNEL_0,                   12,  4,
  interiorobject_TUNNEL_CORNER_6,             8,  6,
  interiorobject_TUNNEL_0,                    4,  8,
  interiorobject_TUNNEL_0,                    0, 10,
};

uint8_t roomdef_40[] =
{
  9,
  0, // number of boundaries
  6, // number of TBD bytes
  30, 31, 32, 33, 34, 43, // TBD
  6, // nobjects
  interiorobject_TUNNEL_7,                   20,  0,
  interiorobject_TUNNEL_0,                   16,  2,
  interiorobject_TUNNEL_0,                   12,  4,
  interiorobject_TUNNEL_0,                    8,  6,
  interiorobject_TUNNEL_0,                    4,  8,
  interiorobject_TUNNEL_0,                    0, 10,
};

uint8_t roomdef_44[] =
{
  8,
  0, // number of boundaries
  5, // number of TBD bytes
  36, 37, 38, 39, 40, // TBD
  5, // nobjects
  interiorobject_TUNNEL_3,                    0,  0,
  interiorobject_TUNNEL_3,                    4,  2,
  interiorobject_TUNNEL_3,                    8,  4,
  interiorobject_TUNNEL_3,                   12,  6,
  interiorobject_TUNNEL_12,                  16,  8,
};

uint8_t roomdef_50_blocked_tunnel[] =
{
  5,
  1, // number of boundaries
  52, 58, 32, 54, // boundary
  6, // number of TBD bytes
  30, 31, 32, 33, 34, 43, // TBD
  6, // nobjects
  interiorobject_TUNNEL_7,                   20,  0,
  interiorobject_TUNNEL_0,                   16,  2,
  interiorobject_TUNNEL_0,                   12,  4,
  interiorobject_COLLAPSED_TUNNEL,            8,  6, // collapsed_tunnel_obj
  interiorobject_TUNNEL_0,                    4,  8,
  interiorobject_TUNNEL_0,                    0, 10,
};
