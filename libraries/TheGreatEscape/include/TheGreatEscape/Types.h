#ifndef TYPES_H
#define TYPES_H

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

#include "TheGreatEscape/Items.h"
#include "TheGreatEscape/Map.h"
#include "TheGreatEscape/Rooms.h"
#include "TheGreatEscape/Sprites.h"
#include "TheGreatEscape/Tiles.h"
#include "TheGreatEscape/Utils.h"

#include "TheGreatEscape/TheGreatEscape.h"

/* ----------------------------------------------------------------------- */

/* LIMITS
 */

enum
{
  /** Byte limit of message queue. */
  message_queue_LENGTH = 19, /* == 9 * 2 + 1 */

  /** Limit of simultaneous visible characters. */
  vischars_LENGTH = 8,

  /** Available beds. */
  beds_LENGTH = 6
};

/* ----------------------------------------------------------------------- */

/* ENUMERATIONS
 */

/**
 * Identifiers of game characters.
 */
typedef enum character
{
  character_0_COMMANDANT,
  character_1_GUARD_1,
  character_2_GUARD_2, 
  character_3_GUARD_3, 
  character_4_GUARD_4, 
  character_5_GUARD_5, 
  character_6_GUARD_6, 
  character_7_GUARD_7, 
  character_8_GUARD_8, 
  character_9_GUARD_9, 
  character_10_GUARD_10,
  character_11_GUARD_11,
  character_12_GUARD_12,
  character_13_GUARD_13,
  character_14_GUARD_14,
  character_15_GUARD_15,
  character_16_GUARD_DOG_1,
  character_17_GUARD_DOG_2,
  character_18_GUARD_DOG_3,
  character_19_GUARD_DOG_4,
  character_20_PRISONER_1, 
  character_21_PRISONER_2, 
  character_22_PRISONER_3, 
  character_23_PRISONER_4, 
  character_24_PRISONER_5, 
  character_25_PRISONER_6, 
  character_26_STOVE_1,
  character_27_STOVE_2,
  character_28_CRATE,
  character__LIMIT,
  character_NONE = 255
}
character_t;

/**
 * Identifiers of character structs.
 */
enum
{
  character_structs__LIMIT = 26
};

/**
 * Identifiers of game messages.
 */
typedef enum message
{
  message_MISSED_ROLL_CALL,
  message_TIME_TO_WAKE_UP,
  message_BREAKFAST_TIME,
  message_EXERCISE_TIME,
  message_TIME_FOR_BED,
  message_THE_DOOR_IS_LOCKED,
  message_IT_IS_OPEN,
  message_INCORRECT_KEY,
  message_ROLL_CALL,
  message_RED_CROSS_PARCEL,
  message_PICKING_THE_LOCK,
  message_CUTTING_THE_WIRE,
  message_YOU_OPEN_THE_BOX,
  message_YOU_ARE_IN_SOLITARY,
  message_WAIT_FOR_RELEASE,
  message_MORALE_IS_ZERO,
  message_ITEM_DISCOVERED,
  message_HE_TAKES_THE_BRIBE,
  message_AND_ACTS_AS_DECOY,
  message_ANOTHER_DAY_DAWNS,
  message__LIMIT,
  message_QUEUE_END = 255
}
message_t;

/**
 * Named elements of movable_items[].
 */
enum movable_item_index
{
  movable_item_STOVE1,
  movable_item_CRATE,
  movable_item_STOVE2,
  movable_item__LIMIT
};

/**
 * Identifiers of input devices.
 */
typedef enum inputdevice
{
  inputdevice_KEYBOARD,
  inputdevice_KEMPSTON,
  inputdevice_SINCLAIR,
  inputdevice_PROTEK,
  inputdevice__LIMIT
}
inputdevice_t;

/**
 * Identifiers of zoombox tiles.
 */
enum zoombox_tiles
{
  zoombox_tile_TL,
  zoombox_tile_HZ,
  zoombox_tile_TR,
  zoombox_tile_VT,
  zoombox_tile_BR,
  zoombox_tile_BL,
};

/* ----------------------------------------------------------------------- */

/* FLAGS
 */

/**
 * Identifiers of input directions and actions.
 */
enum input_flags
{
  input_NONE                           = 0,
  input_UP                             = 1,
  input_DOWN                           = 2,
  input_LEFT                           = 3,
  input_RIGHT                          = 6,
  input_FIRE                           = 9,
  input_UP_FIRE                        = input_UP    + input_FIRE,
  input_DOWN_FIRE                      = input_DOWN  + input_FIRE,
  input_LEFT_FIRE                      = input_LEFT  + input_FIRE,
  input_RIGHT_FIRE                     = input_RIGHT + input_FIRE
};

/**
 * Identifiers of visible character structure flags.
 */
enum vischar_flags
{
  vischar_CHARACTER_EMPTY_SLOT         = 0xFF,
  vischar_CHARACTER_MASK               = 0x1F, /* Character index mask. */

  vischar_FLAGS_EMPTY_SLOT             = 0xFF,
  vischar_FLAGS_MASK                   = 0x3F,
  vischar_FLAGS_PICKING_LOCK           = 1 << 0, /* Hero only? */
  vischar_FLAGS_CUTTING_WIRE           = 1 << 1, /* Hero only? */
  vischar_FLAGS_BRIBE_PENDING          = 1 << 0, /* NPC only? */
  vischar_FLAGS_BIT1                   = 1 << 1, /* NPC only? */ // dog+food flag
  vischar_FLAGS_BIT2                   = 1 << 2, /* 'Gone mad' (bribed) flag */
  vischar_FLAGS_BIT6                   = 1 << 6, // affects scaling
  vischar_FLAGS_BIT7                   = 1 << 7,

  vischar_BYTE2_MASK                   = 0x7F,   // target mask
  vischar_BYTE2_BIT7                   = 1 << 7, // target mask

  vischar_BYTE7_MASK                   = 0x0F,
  vischar_BYTE7_MASK_HI                = 0xF0,
  vischar_BYTE7_BIT5                   = 1 << 5, // set when can't move in X
  vischar_BYTE7_BIT6                   = 1 << 6,
  vischar_BYTE7_BIT7                   = 1 << 7, // stops locate_vischar_or_itemstruct considering a vischar

  vischar_BYTE12_MASK                  = 0x7F,
  vischar_BYTE12_BIT7                  = 1 << 7,

  vischar_BYTE13_MASK                  = 0x7F,
  vischar_BYTE13_BIT7                  = 1 << 7, // guess: input kicking flag

  vischar_BYTE14_CRAWL                 = 1 << 2
};

/**
 * Identifiers of character struct flags and masks.
 */
enum itemstruct_flags
{
  itemstruct_ITEM_MASK                 = 0x0F,
  itemstruct_ITEM_FLAG_UNKNOWN         = 1 << 4,
  itemstruct_ITEM_FLAG_POISONED        = 1 << 5,
  itemstruct_ITEM_FLAG_HELD            = 1 << 7, /* Set when the item is picked up for the first time. */

  itemstruct_ROOM_NONE                 = 0x3F,
  itemstruct_ROOM_MASK                 = 0x3F,
  itemstruct_ROOM_FLAG_NEARBY_6        = 1 << 6, /* Set when the item is nearby. Cleared when? */
  itemstruct_ROOM_FLAG_NEARBY_7        = 1 << 7  /* Set when the item is nearby. */
};

/**
 * Identifiers of gates and doors flags and masks.
 */
enum gates_and_doors_flags
{
  gates_and_doors_MASK                 = 0x7F,
  gates_and_doors_LOCKED               = 1 << 7
};

/**
 * Identifiers of character struct flags and masks.
 */
enum characterstruct_flags
{
  characterstruct_BYTE0_MASK           = 0x1F, /* Character index mask. */
  characterstruct_FLAG_DISABLED        = 1 << 6, /* This disables the character. */
  characterstruct_BYTE0_BIT7           = 1 << 7, // set in sub_A404

  characterstruct_BYTE5_MASK           = 0x7F, // target low byte

  characterstruct_BYTE6_MASK_HI        = 0xF8
};

/**
 * Identifiers of door position masks.
 */
enum doorposition_flags
{
  doorposition_BYTE0_MASK_LO           = 0x03,
  doorposition_BYTE0_MASK_HI           = 0xFC
};

/**
 * Identifiers of searchlight flags.
 */
enum searchlight_flags
{
  searchlight_STATE_1F                 = 0x1F, // seems to be set when the hero is caught in the spotlight
  searchlight_STATE_OFF                = 0xFF, /* Likely: Hunting for the hero. */
};

/**
 * Identifiers of items used during escape.
 */
enum escapeitem_flags
{
  escapeitem_COMPASS                   = 1 << 0,
  escapeitem_PAPERS                    = 1 << 1,
  escapeitem_PURSE                     = 1 << 2,
  escapeitem_UNIFORM                   = 1 << 3
};

/* ----------------------------------------------------------------------- */

/* CONSTANTS
 */

/* These are offsets from the start of the screen bank. */
#define score_address                 ((uint16_t) 0x1094)
#define screen_text_start_address     ((uint16_t) 0x10E0)

/* These are offsets from the start of the attributes bank. */
#define morale_flag_attributes_offset ((uint16_t) 0x0042)

/* These are absolute addresses. Here for reference. */
#define visible_tiles_start_address   ((uint16_t) 0xF0F8)
#define visible_tiles_end_address     ((uint16_t) 0xF28F) /* inclusive */
#define visible_tiles_length          (24 * 17)

/* These are absolute addresses. Here for reference. */
#define screen_buffer_start_address   ((uint16_t) 0xF290)
#define screen_buffer_end_address     ((uint16_t) 0xFF4F) /* inclusive */
#define screen_buffer_length          (24 * 8 * 17)

/**
 * Identifiers of morale.
 */
enum morale
{
  morale_MIN                           = 0,
  morale_MAX                           = 112,
};

/**
 * Identifiers of map locations.
 */
enum location
{
  location_0005                        = 0x0005, /* used in wake_up */
  location_000E                        = 0x000E, /* used at exercise time (prior) */
  location_0010                        = 0x0010,
  location_001A                        = 0x001A, /* used in go_to_roll_call */
  location_0026                        = 0x0026, /* used in event_search_light */
  location_002A                        = 0x002A, /* used at wake up time */
  location_002B                        = 0x002B,
  location_002C                        = 0x002C,
  location_002D                        = 0x002D, /* used at roll call */
  location_012C                        = 0x012C, /* used at night time */
  location_0285                        = 0x0285, /* used at bed time */
  location_0390                        = 0x0390, /* used at breakfast time */
  location_03A6                        = 0x03A6, /* used in event_time_for_bed */
  location_048E                        = 0x048E, /* used at exercise time */
};

/**
 * Holds a location.
 */
typedef uint16_t location_t;

/**
 * Identifiers of map locations.
 *
 * These are /ranges/ of locations.
 */
enum
{
  map_MAIN_GATE_X                      = 0x696D, /* coords: 0x69..0x6D */
  map_MAIN_GATE_Y                      = 0x494B,
  map_ROLL_CALL_X                      = 0x727C,
  map_ROLL_CALL_Y                      = 0x6A72,
};

/**
 * Identifiers of sounds.
 *
 * High byte is iterations, low byte is delay.
 */
typedef enum sound
{
  sound_CHARACTER_ENTERS_1             = 0x2030,
  sound_CHARACTER_ENTERS_2             = 0x2040,
  sound_BELL_RINGER                    = 0x2530,
  sound_PICK_UP_ITEM                   = 0x3030,
  sound_DROP_ITEM                      = 0x3040,
}
sound_t;

/**
 * Identifiers of bell ringing states.
 */
enum bellring
{
  bell_RING_PERPETUAL                  = 0,
  bell_RING_40_TIMES                   = 40,
  bell_STOP                            = 0xFF,
};

/**
 * Holds a bell ringing counter.
 */
typedef uint8_t bellring_t;

/* ----------------------------------------------------------------------- */

/* TYPES
 *
 * Note: Descriptions are written as if prefixed with "This type ...".
 */

/**
 * Holds a game time value.
 */
typedef uint8_t gametime_t;

/**
 * Holds a game event time value.
 */
typedef uint8_t eventtime_t;

/**
 * Holds a position and height.
 */
typedef struct pos
{
  uint16_t x, y, height;
}
pos_t;

/**
 * Holds a smaller scale version of pos_t
 */
typedef struct tinypos
{
  uint8_t x, y, height;
}
tinypos_t;

/**
 * Defines a movable item.
 * This is a sub-struct of vischar (from 'pos' onwards).
 * Unknown as yet its intent. Just calling it a "movable item" for now.
 */
typedef struct movableitem
{
  pos_t           pos;
  const sprite_t *spriteset;
  uint8_t         flip_sprite; // sprite set offset / flip flag
}
movableitem_t;

/**
 * Defines a visible (on-screen) character.
 */
typedef struct vischar
{
  uint8_t         character;    /* $8000 char index */
  uint8_t         flags;        /* $8001 flags */
  location_t      target;       /* $8002 target location */
  tinypos_t       p04;          /* $8004 position */
  uint8_t         b07;          /* $8007 top nibble = flags, bottom nibble = counter? */
  const uint8_t **w08;          /* $8008 pointer to character_related_pointers (assigned once only) */
  const uint8_t  *w0A;          /* $800A */
  uint8_t         b0C;          /* $800C */ // used with above?
  uint8_t         b0D;          /* $800D movement */ // compared to flags?
  uint8_t         b0E;          /* $800E walk/crawl flag */
  movableitem_t   mi;           /* $800F movable item (position, current character sprite set, flip_sprite) */
  uint16_t        scrx, scry;   /* $8018,$801A screen x,y coord */
  room_t          room;         /* $801C room index */
  uint8_t         unused;       /* $801D unused */
  uint8_t         width_bytes;  /* $801E copy of sprite width in bytes + 1 */
  uint8_t         height;       /* $801F copy of sprite height in rows */
}
vischar_t;

/**
 * Holds a key definition.
 */
typedef struct keydef
{
  uint8_t port, mask;
}
keydef_t;

/**
 * Holds all key definitions.
 */
typedef struct keydefs
{
  keydef_t defs[5]; /* left, right, up, down, fire */
}
keydefs_t;

/**
 * Holds input directions and actions.
 */
typedef unsigned int input_t;

/**
 * Holds bitmask of items checked during an escape attempt.
 */
typedef unsigned int escapeitem_t;

/**
 * Defines a boundary such as a wall or fence.
 */
typedef struct wall
{
  uint8_t minx, maxx;
  uint8_t miny, maxy;
  uint8_t minheight, maxheight;
}
wall_t;

/**
 * Defines a screen location and string, for drawing strings.
 */
typedef struct screenlocstring
{
  uint16_t  screenloc; /* screen offset (pointer in original code) */
  uint8_t   length;
  char     *string;    /* string pointer (embedded array in original code) */
}
screenlocstring_t;

/**
 * Defines a character.
 */
typedef struct characterstruct
{
  character_t character; // and flags
  room_t      room;
  tinypos_t   pos;
  location_t  target;
}
characterstruct_t;

/**
 * Handles a timed event.
 */
typedef void (timedevent_handler_t)(tgestate_t *state);

/**
 * Defines a timed event.
 */
typedef struct timedevent
{
  eventtime_t           time;
  timedevent_handler_t *handler;
}
timedevent_t;

/**
 * Defines an item.
 *
 * This has the same structure layout as characterstruct.
 */
typedef struct itemstruct
{
  item_t     item_and_flags;
  room_t     room_and_flags;
  tinypos_t  pos;
  location_t target;
}
itemstruct_t;

/**
 * Maps a character to an event.
 */
typedef struct charactereventmap
{
  uint8_t something;
  uint8_t handler;
}
charactereventmap_t;

/**
 * Defines a character event handler.
 */
typedef void (charevnt_handler_t)(tgestate_t  *state,
                                  character_t *charptr,
                                  vischar_t   *vischar);

/**
 * Defines a door's position.
 */
typedef struct doorpos
{
  uint8_t   room_and_flags; // top 6 bits are room, bottom 2 are ?
  tinypos_t pos;
}
doorpos_t;

/**
 * Holds a door index.
 */
typedef uint8_t doorindex_t;

/**
 * Handles item actions.
 */
typedef void (*item_action_t)(tgestate_t *state);

/**
 * Stores a byte-to-pointer mapping.
 */
typedef struct byte_to_pointer
{
  uint8_t        byte;
  const uint8_t *pointer;
}
byte_to_pointer_t;

/**
 * Holds a boundary.
 */
typedef struct bounds
{
  uint8_t x0, x1, y0, y1;
}
bounds_t;

/**
 * Signature of a player input routine.
 */
typedef input_t (*inputroutine_t)(tgestate_t *state);

/**
 * Holds default item locations.
 */
typedef struct default_item_location
{
  uint8_t room_and_flags;
  uint8_t x, y;
}
default_item_location_t;

/**
 * Holds mask data.
 */
typedef struct mask
{
  uint8_t   index;  /**< Index into mask_pointers. */
  bounds_t  bounds;
  tinypos_t pos;
}
mask_t;

/**
 * Holds character meta data.
 */
typedef struct character_meta_data
{
  const uint8_t  **data;
  const sprite_t  *spriteset;
}
character_meta_data_t;

/* ----------------------------------------------------------------------- */

#endif /* TYPES_H */

