#ifndef ROOMDEFS_H
#define ROOMDEFS_H

#include <stdint.h>

#include "TheGreatEscape/Rooms.h"

typedef uint8_t roomdef_t;

extern const roomdef_t *rooms_and_tunnels[room__LIMIT];

extern uint8_t roomdef_1_hut1_right[];
extern uint8_t roomdef_2_hut2_left[];
extern uint8_t roomdef_3_hut2_right[];
extern uint8_t roomdef_4_hut3_left[];
extern uint8_t roomdef_5_hut3_right[];
extern uint8_t roomdef_7_corridor[];
extern uint8_t roomdef_8_corridor[];
extern uint8_t roomdef_9_crate[];
extern uint8_t roomdef_10_lockpick[];
extern uint8_t roomdef_11_papers[];
extern uint8_t roomdef_12_corridor[];
extern uint8_t roomdef_13_corridor[];
extern uint8_t roomdef_14_torch[];
extern uint8_t roomdef_15_uniform[];
extern uint8_t roomdef_16_corridor[];
extern uint8_t roomdef_18_radio[];
extern uint8_t roomdef_19_food[];
extern uint8_t roomdef_20_redcross[];
extern uint8_t roomdef_22_red_key[];
extern uint8_t roomdef_23_breakfast[];
extern uint8_t roomdef_24_solitary[];
extern uint8_t roomdef_25_breakfast[];
extern uint8_t roomdef_28_hut1_left[];
extern uint8_t roomdef_29_second_tunnel_start[];
extern uint8_t roomdef_30[];
extern uint8_t roomdef_31[];
extern uint8_t roomdef_32[];
extern uint8_t roomdef_34[];
extern uint8_t roomdef_35[];
extern uint8_t roomdef_36[];
extern uint8_t roomdef_40[];
extern uint8_t roomdef_44[];
extern uint8_t roomdef_50_blocked_tunnel[];

#define roomdef_23_BENCH_A     (10 +  9 * 3)
#define roomdef_23_BENCH_B     (10 + 10 * 3)
#define roomdef_23_BENCH_C     (10 + 11 * 3)

#define roomdef_25_BENCH_D     ( 8 +  7 * 3)
#define roomdef_25_BENCH_E     ( 8 +  8 * 3)
#define roomdef_25_BENCH_F     ( 8 +  9 * 3)
#define roomdef_25_BENCH_G     ( 8 + 10 * 3)

#define roomdef_2_BED          (14 +  4 * 3)

#define roomdef_50_BLOCKAGE    (14 +  3 * 3)

#endif /* ROOMDEFS_H */
