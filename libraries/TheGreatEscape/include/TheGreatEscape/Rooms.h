/**
 * Rooms.h
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

#ifndef ROOMS_H
#define ROOMS_H

#include <stdint.h>

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
  room_0_OUTDOORS,
  room_1_HUT1RIGHT,
  room_2_HUT2LEFT,
  room_3_HUT2RIGHT,
  room_4_HUT3LEFT,
  room_5_HUT3RIGHT,
  room_6,             // unused as a room, but used numerically
  room_7_CORRIDOR,
  room_8_CORRIDOR,
  room_9_CRATE,
  room_10_LOCKPICK,
  room_11_PAPERS,
  room_12_CORRIDOR,
  room_13_CORRIDOR,
  room_14_TORCH,
  room_15_UNIFORM,
  room_16_CORRIDOR,
  room_17_CORRIDOR,
  room_18_RADIO,
  room_19_FOOD,
  room_20_REDCROSS,
  room_21_CORRIDOR,
  room_22_REDKEY,
  room_23_MESS_HALL,
  room_24_SOLITARY,
  room_25_MESS_HALL,
  room_26,           // unused
  room_27,           // unused
  room_28_HUT1LEFT,
  room_29_SECOND_TUNNEL_START,
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
  room_50_BLOCKED_TUNNEL,
  room_51,
  room_52,
  room__LIMIT,
  room_NONE = 255,
};

/**
 * A game room.
 */
typedef uint8_t room_t;

#endif /* ROOMS_H */

// vim: ts=8 sts=2 sw=2 et
