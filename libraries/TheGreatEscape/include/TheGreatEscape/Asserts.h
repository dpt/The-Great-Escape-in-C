/**
 * Asserts.h
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

#ifndef ASSERTS_H
#define ASSERTS_H

/*
 * State assertions.
 */

#define ASSERT_SCREEN_PTR_VALID(p)                    \
do {                                                  \
  assert((p) >= &state->speccy->screen.pixels[0]);    \
  assert((p) < &state->speccy->screen.pixels[SCREEN_BITMAP_LENGTH]); \
} while (0)

#define ASSERT_SCREEN_ATTRIBUTES_PTR_VALID(p)         \
do {                                                  \
  assert((p) >= &state->speccy->screen.attributes[0]); \
  assert((p) < &state->speccy->screen.attributes[SCREEN_ATTRIBUTES_LENGTH]); \
} while (0)

#define ASSERT_MASK_BUF_PTR_VALID(p)                  \
do {                                                  \
  assert((p) >= &state->mask_buffer[0]);              \
  assert((p) < &state->mask_buffer[MASK_BUFFER_LENGTH]); \
} while (0)

#define ASSERT_TILE_BUF_PTR_VALID(p)                  \
do {                                                  \
  assert((p) >= state->tile_buf);                     \
  assert((p) < state->tile_buf + state->tile_buf_size); \
} while (0)

#define ASSERT_WINDOW_BUF_PTR_VALID(p, overshoot)                       \
do {                                                                    \
  assert((p) >= state->window_buf - overshoot);                         \
  assert((p) < state->window_buf + state->window_buf_size + overshoot); \
} while (0)

#define ASSERT_MAP_BUF_PTR_VALID(p)                   \
do {                                                  \
  assert((p) >= state->map_buf);                      \
  assert((p) < state->map_buf + state->map_buf_size); \
} while (0)

#define ASSERT_VISCHAR_VALID(p)                       \
do {                                                  \
  assert((p) >= &state->vischars[0]);                 \
  assert((p) < &state->vischars[vischars_LENGTH]);    \
} while (0)

#define ASSERT_ITEMSTRUCT_VALID(p)                    \
do {                                                  \
  assert((p) >= &state->item_structs[0]);             \
  assert((p) < &state->item_structs[item__LIMIT]);    \
} while (0)

/*
 * Type assertions.
 */

#define ASSERT_CHARACTER_VALID(c)                             \
do {                                                          \
  assert(c >= 0 && c < character__LIMIT);                     \
} while (0)

#define ASSERT_ROOM_VALID(r)                                  \
do {                                                          \
  assert((r) == room_NONE ||                                  \
         ((r) >= 0 && (r) < room__LIMIT));                    \
} while (0)

#define ASSERT_ITEM_VALID(i)                                  \
do {                                                          \
  assert(i >= 0 && i < item__LIMIT);                          \
} while (0)

#define ASSERT_INTERIOR_TILES_VALID(p)                          \
do {                                                            \
  assert(p >= &interior_tiles[0].row[0]);                       \
  assert(p <= &interior_tiles[interiortile__LIMIT - 1].row[7]); \
} while (0)

#define ASSERT_DOORS_VALID(p)                                 \
do {                                                          \
  assert(p >= &state->interior_doors[0]);                     \
  assert(p < &state->interior_doors[4]);                      \
} while (0)

#define ASSERT_SUPERTILE_PTR_VALID(p)                            \
do {                                                             \
  assert(p >= &supertiles[0].tiles[0]);                          \
  assert(p <= &supertiles[supertileindex__LIMIT - 1].tiles[15]); \
} while (0)

#define ASSERT_MAP_PTR_VALID(p)                               \
do {                                                          \
  assert(p >= &map[0]);                                       \
  assert(p < &map[MAPX * MAPY]);                              \
} while (0)

/* These limits were determined by checking the original game and cover the main
 * map only. They'll need adjusting. */
#define ASSERT_MAP_POSITION_VALID(p)                          \
do {                                                          \
  assert(p.x >= 0);                                           \
  assert(p.x < 200);                                          \
  assert(p.y >= 6);                                           \
  assert(p.y < 130);                                          \
} while (0)

#define ASSERT_ROUTE_VALID(r)                                 \
do                                                            \
{                                                             \
  assert((r).index == routeindex_255_WANDER ||                \
        ((r).index & ~ROUTEINDEX_REVERSE_FLAG) < routeindex__LIMIT); \
}                                                             \
while (0)

#endif /* ASSERTS_H */

// vim: ts=8 sts=2 sw=2 et
