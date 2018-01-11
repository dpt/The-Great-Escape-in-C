#ifndef ROOMDEFS_H
#define ROOMDEFS_H

#include <stdint.h>

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

#endif /* ROOMDEFS_H */
