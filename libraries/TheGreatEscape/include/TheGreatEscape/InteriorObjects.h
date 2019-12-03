/**
 * InteriorObjects.h
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

#ifndef INTERIOR_OBJECTS_H
#define INTERIOR_OBJECTS_H

/**
 * Identifiers of objects used to build interiors.
 *
 * Compass directions are suffixed to indicate where the object faces e.g.
 * interiorobject_WIDE_WINDOW_FACING_SE faces south-east.
 *
 * A double suffix like SW_NE means that an object goes south-west to
 * north-east in screen space (or vice versa).
 */
enum interior_object
{
  interiorobject_STRAIGHT_TUNNEL_SW_NE,
  interiorobject_SMALL_TUNNEL_ENTRANCE,
  interiorobject_ROOM_OUTLINE_22x12_A,
  interiorobject_STRAIGHT_TUNNEL_NW_SE,
  interiorobject_TUNNEL_T_JOIN_NW_SE,
  interiorobject_PRISONER_SAT_MID_TABLE,
  interiorobject_TUNNEL_T_JOIN_SW_NE,
  interiorobject_TUNNEL_CORNER_SW_SE,
  interiorobject_WIDE_WINDOW_FACING_SE,
  interiorobject_EMPTY_BED_FACING_SE,
  interiorobject_SHORT_WARDROBE_FACING_SW,
  interiorobject_CHEST_OF_DRAWERS_FACING_SW,
  interiorobject_TUNNEL_CORNER_NW_NE,
  interiorobject_EMPTY_BENCH,
  interiorobject_TUNNEL_CORNER_NE_SE,
  interiorobject_DOOR_FRAME_SE,
  interiorobject_DOOR_FRAME_SW,
  interiorobject_TUNNEL_CORNER_NW_SW,
  interiorobject_TUNNEL_ENTRANCE,
  interiorobject_PRISONER_SAT_END_TABLE,
  interiorobject_COLLAPSED_TUNNEL_SW_NE,
  interiorobject_UNUSED_21, // unused by game, draws as interiorobject_ROOM_OUTLINE_22x12_A
  interiorobject_CHAIR_FACING_SE,
  interiorobject_OCCUPIED_BED,
  interiorobject_ORNATE_WARDROBE_FACING_SW,
  interiorobject_CHAIR_FACING_SW,
  interiorobject_CUPBOARD_FACING_SE,
  interiorobject_ROOM_OUTLINE_18x10_A,
  interiorobject_UNUSED_28, // unused by game, draws as interiorobject_TABLE
  interiorobject_TABLE,
  interiorobject_STOVE_PIPE,
  interiorobject_PAPERS_ON_FLOOR,
  interiorobject_TALL_WARDROBE_FACING_SW,
  interiorobject_SMALL_SHELF_FACING_SE,
  interiorobject_SMALL_CRATE,
  interiorobject_SMALL_WINDOW_WITH_BARS_FACING_SE,
  interiorobject_TINY_DOOR_FRAME_NE,  // tunnel entrance
  interiorobject_NOTICEBOARD_FACING_SE,
  interiorobject_DOOR_FRAME_NW,
  interiorobject_UNUSED_39, // unused by game, draws as interiorobject_END_DOOR_FRAME_NW_SE
  interiorobject_DOOR_FRAME_NE,
  interiorobject_ROOM_OUTLINE_15x8,
  interiorobject_CUPBOARD_FACING_SW,
  interiorobject_MESS_BENCH,
  interiorobject_MESS_TABLE,
  interiorobject_MESS_BENCH_SHORT,
  interiorobject_ROOM_OUTLINE_18x10_B,
  interiorobject_ROOM_OUTLINE_22x12_B,
  interiorobject_TINY_TABLE,
  interiorobject_TINY_DRAWERS_FACING_SE,
  interiorobject_TALL_DRAWERS_FACING_SW,
  interiorobject_DESK_FACING_SW,
  interiorobject_SINK_FACING_SE,
  interiorobject_KEY_RACK_FACING_SE,
  interiorobject__LIMIT
};

/**
 * An object used to build interiors.
 */
typedef enum interior_object object_t;

#endif /* INTERIOR_OBJECTS_H */

// vim: ts=8 sts=2 sw=2 et
