/**
 * RoomDefs.h
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

#ifndef ROOM_DEFS_H
#define ROOM_DEFS_H

#include "TheGreatEscape/Types.h"
#include "TheGreatEscape/Rooms.h"

#define roomdef_2_BED          (14 +  4 * 3)

#define roomdef_3_BED_A        (20 +  3 * 3)
#define roomdef_3_BED_B        (20 +  4 * 3)
#define roomdef_3_BED_C        (20 +  5 * 3)

#define roomdef_5_BED_D        (20 +  3 * 3)
#define roomdef_5_BED_E        (20 +  4 * 3)
#define roomdef_5_BED_F        (20 +  5 * 3)

#define roomdef_23_BENCH_A     (10 +  9 * 3)
#define roomdef_23_BENCH_B     (10 + 10 * 3)
#define roomdef_23_BENCH_C     (10 + 11 * 3)

#define roomdef_25_BENCH_D     ( 8 +  7 * 3)
#define roomdef_25_BENCH_E     ( 8 +  8 * 3)
#define roomdef_25_BENCH_F     ( 8 +  9 * 3)
#define roomdef_25_BENCH_G     ( 8 + 10 * 3)

#define roomdef_50_BOUNDARY    ( 2)
#define roomdef_50_BLOCKAGE    (14 +  3 * 3)

int get_roomdef(tgestate_t *state, room_t room_index, int offset);
void set_roomdef(tgestate_t *state,
                 room_t      room_index,
                 int         offset,
                 int         new_byte);

#endif /* ROOM_DEFS_H */
