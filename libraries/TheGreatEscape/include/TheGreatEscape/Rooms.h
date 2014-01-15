#ifndef ROOMS_H
#define ROOMS_H

/* Key
 *
 *   '' => door (up/down)
 *   =  => door (left/right)
 *   in => entrance
 *   [] => exit
 *   ~  => void / ground
 *
 * Map of rooms' indices
 *
 *   +--------+--------+----+----+----+
 *   |   25   =   23   = 19 = 18 = 12 |
 *   +--------+-+-''-+-+----+    +-''-+
 *    | 24 = 22 = 21 | | 20 +    + 17 |
 *    +----+----+-''-+ +-''-+----+-''-+
 *                          = 15 |  7 |
 *       +----+------+      +-''-+-''-+
 *       = 28 =   1  |      | 14 = 16 |
 *       +----+--''--+      +----+-''-+
 *                               = 13 |
 *       +----+------+           +-''-+
 *       =  2 =   3  |           | 11 |
 *       +----+--''--+           |    |
 *                          +----+----+
 *       +----+------+      |  9 | 10 |
 *       =  4 =   5  |      |    |    |
 *       +----+--''--+      +-''-+-''-+
 *                          =    8    |
 *                          +----+----+
 *
 * Map of tunnels' indices 1
 *
 *   ~~~~~~~~~~~~~~~~~~~~+---------+----+
 *   ~~~~~~~~~~~~~~~~~~~~|    49   = 50 |
 *   ~~~~~~~~~~~~~~~~~~~~+-''-+----+-''-+
 *   ~~~~~~~~~~~~~~~~~~~~| 48 |~~~~| 47 |
 *   ~~~~~~~~~~~~~~~~~~~~+-[]-+~~~~|    |
 *   +------+-------+----+----+----+    |
 *   |  45  =   41  =    40   | 46 =    |
 *   +-''-+-+--+-''-+------''-+-''-+----+
 *   | 44 = 43 = 42 |~~~~| 38 = 39 |~~~~~
 *   +----+----+----+~~~~+-''-+----+~~~~~
 *   ~~~~~~~~~~~~~~~~~~~~| 37 |~~~~~~~~~~
 *   ~~~~~~~~~~~~~~~~~~~~+-in-+~~~~~~~~~~
 *
 * Map of tunnels' indices 2
 *
 *   +----------+--------+--------+
 *   |    36    =   30   =   52   |
 *   +-''-+-----+--''----+---+-''-+
 *   | 35 |~~~~~~| 31 |~~~~~~| 51 |
 *   |    +------+-''-+~~~~~~|    |
 *   |    =  33  = 32 |~~~~~~|    |
 *   |    +------+----+~~~~~~|    |
 *   |    |~~~~~~~~~~~~~+----+    |
 *   +-''-+~~~~~~~~~~~~in 29 =    |
 *   | 34 |~~~~~~~~~~~~~+----+----+
 *   +-[]-+~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * I'm fairly sure that the visual of the above layout is actually
 * topologically impossible. e.g. if you take screenshots of every room and
 * attempt to combine them in an image editor it won't join up.
 *
 * Unused room indices: 6, 26, 27
 */

/**
 * Identifiers of game rooms.
 */
enum room
{
  room_0_outdoors,
  room_1_hut1right,
  room_2_hut2left,
  room_3_hut2right,
  room_4_hut3left,
  room_5_hut3right,
  room_6,             // unused
  room_7_corridor,
  room_8_corridor,
  room_9_crate,
  room_10_lockpick,
  room_11_papers,
  room_12_corridor,
  room_13_corridor,
  room_14_torch,
  room_15_uniform,
  room_16_corridor,
  room_17_corridor,
  room_18_radio,
  room_19_food,
  room_20_redcross,
  room_21_corridor,
  room_22_redkey,
  room_23_breakfast,
  room_24_solitary,
  room_25_breakfast,
  room_26,           // unused
  room_27,           // unused
  room_28_hut1left,
  room_29_secondtunnelstart,
  room_30,
  room_31,
  room_32,
  room_33,
  room_34,
  room_35,
  room_36,
  room_37,
  room_38,
  room_39,
  room_40,
  room_41,
  room_42,
  room_43,
  room_44,
  room_45,
  room_46,
  room_47,
  room_48,
  room_49,
  room_50_blocked_tunnel,
  room_51,
  room_52,
  room__LIMIT,
  room_NONE = 255,
};

/**
 * A game room.
 */
typedef enum room room_t;

#endif /* ROOMS_H */

