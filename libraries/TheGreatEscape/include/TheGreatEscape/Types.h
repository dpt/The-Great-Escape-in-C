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
  character_0,
  character_1,
  character_2,
  character_3,
  character_4,
  character_5,
  character_6,
  character_7,
  character_8,
  character_9,
  character_10,
  character_11,
  character_12,
  character_13,
  character_14,
  character_15,
  character_16, /* Suspect this is a guard dog. */
  character_17, /* Suspect this is a guard dog. */
  character_18, /* Suspect this is a guard dog. */
  character_19, /* Suspect this is a guard dog. */
  character_20,
  character_21,
  character_22,
  character_23,
  character_24,
  character_25,
  character_26, /* Stove 1. */
  character_27, /* Stove 2. */
  character_28, /* Crate. */
  character__LIMIT,
  character_NONE = 255
}
character_t;

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
  message_NONE = 255
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
  vischar_BYTE0_EMPTY_SLOT             = 0xFF,
  vischar_BYTE0_MASK                   = 0x1F, // character index? 0..31

  vischar_BYTE1_EMPTY_SLOT             = 0xFF,
  vischar_BYTE1_MASK                   = 0x3F,
  vischar_BYTE1_PICKING_LOCK           = 1 << 0, /* Player only? */
  vischar_BYTE1_CUTTING_WIRE           = 1 << 1, /* Player only? */
  vischar_BYTE1_PERSUE                 = 1 << 0, /* AI only? */
  vischar_BYTE1_BIT1                   = 1 << 1, /* AI only? */
  vischar_BYTE1_BIT2                   = 1 << 2, /* 'Gone mad' (bribe) flag */
  vischar_BYTE1_BIT6                   = 1 << 6, // affects scaling
  vischar_BYTE1_BIT7                   = 1 << 7,

  vischar_BYTE2_MASK                   = 0x7F,
  vischar_BYTE2_BIT7                   = 1 << 7,

  vischar_BYTE7_MASK                   = 0x0F,
  vischar_BYTE7_MASK_HI                = 0xF0,
  vischar_BYTE7_BIT5                   = 1 << 5,
  vischar_BYTE7_BIT6                   = 1 << 6,
  vischar_BYTE7_BIT7                   = 1 << 7,

  vischar_BYTE12_MASK                  = 0x7F,

  vischar_BYTE13_MASK                  = 0x7F,
  vischar_BYTE13_BIT7                  = 1 << 7,

  vischar_BYTE14_CRAWL                 = 1 << 2
};

/**
 * Identifiers of character struct flags and masks.
 */
enum itemstruct_flags
{
  itemstruct_ITEM_MASK                 = 0x0F,
  itemstruct_ITEM_FLAG_POISONED        = 1 << 5,
  itemstruct_ITEM_FLAG_HELD            = 1 << 7, /* Set when the item is picked up (maybe). */

  itemstruct_ROOM_MASK                 = 0x3F,
  itemstruct_ROOM_FLAG_BIT6            = 1 << 6,
  itemstruct_ROOM_FLAG_ITEM_NEARBY     = 1 << 7  /* Set when the item is nearby. */
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
  characterstruct_BYTE0_BIT6           = 1 << 6, /* This disables the character. */
  characterstruct_BYTE0_MASK           = 0x1F,

  characterstruct_BYTE5_MASK           = 0x7F,

  characterstruct_BYTE6_MASK_HI        = 0xF8,
  characterstruct_BYTE6_MASK_LO        = 0x07
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
  searchlight_STATE_00                 = 0x00,
  searchlight_STATE_1F                 = 0x1F,
  searchlight_STATE_OFF                = 0xFF, /* Likely: Hunting for player. */
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
typedef enum location
{
  location_0E00                        = 0x0E00, /* used at exercise time (prior) */
  location_1000                        = 0x1000,
  location_2A00                        = 0x2A00, /* used at wake up time */
  location_2B00                        = 0x2B00,
  location_2C00                        = 0x2C00,
  location_2C01                        = 0x2C01, /* used at night time */
  location_2D00                        = 0x2D00, /* used at roll call */
  location_8502                        = 0x8502, /* used at bed time */
  location_8E04                        = 0x8E04, /* used at exercise time */
  location_9003                        = 0x9003, /* used at breakfast time */
}
location_t;

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
 * Holds a position and vertical offset / height (not yet fully understood).
 */
typedef struct pos
{
  uint16_t y, x, vo;
}
pos_t;

/**
 * Holds a smaller scale version of pos_t
 */
typedef struct tinypos
{
  uint8_t y, x, vo;
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
  uint8_t         b17;
}
movableitem_t;

/**
 * Defines a visible (on-screen) character.
 */
typedef struct vischar
{
  uint8_t       character; /* $8000 char index */
  uint8_t       flags;     /* $8001 flags */
  location_t    target;    /* $8002 target location */
  tinypos_t     p04;       /* $8004 position */
  uint8_t       b07;       /* $8007 more flags */
  uint16_t      w08;       /* $8008 */ // only ever read in called_from_main_loop_9
  uint16_t      w0A;       /* $800A */ // must be a pointer
  uint8_t       b0C;       /* $800C */ // used with above?
  uint8_t       b0D;       /* $800D movement */ // compared to flags?
  uint8_t       b0E;       /* $800E walk/crawl flag */
  movableitem_t mi;        /* $800F movable item (position, current character sprite set, b17) */
  uint16_t      w18;       /* $8018 */ // coord
  uint16_t      w1A;       /* $801A */ // coord
  room_t        room;      /* $801C room index */
  uint8_t       b1D;       /* $801D */ // no references
  uint8_t       b1E;       /* $801E spotlight */
  uint8_t       b1F;       /* $801F */ // compared to but never written? only used by sub_BAF7
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
  uint8_t a, b, c, d, e, f; // TBD
}
wall_t;

/**
 * Defines a screen location and string, for drawing strings.
 */
typedef struct screenlocstring
{
  uint16_t  screenloc; /* screen offset (pointer in original code) */
  uint8_t   length;
  char     *string;    /* string pointer (embedded in original code) */
}
screenlocstring_t;

/**
 * Defines a character.
 */
typedef struct characterstruct
{
  character_t character;
  room_t      room;
  tinypos_t   pos;
  uint8_t     t1, t2; // seems to be a copy of target
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
  uint8_t               time;
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
  item_t    item;
  room_t    room; // if has flags should be room_and_flags
  tinypos_t pos;
  uint8_t   t1, t2;
}
itemstruct_t;

/**
 * Maps a character to an event.
 */
typedef struct charactereventmap
{
  character_t character_and_flag;
  uint8_t     handler;
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
  uint8_t room_and_flags; // top 6 bits are room, bottom 2 are ?
  // replace y,x,z with a tinypos_t?
  uint8_t y, x;
  uint8_t z;  // suspect: height
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
  uint8_t a, b, c, d; // TBD which of (a,b,c,d) is (x0,y0,x1,y1).
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
  uint8_t y;
  uint8_t x;
}
default_item_location_t;

/**
 * Seven byte structures. Unknown yet. Likely indoor mask data.
 */
typedef struct sevenbyte
{
  uint8_t a, b, c, d, e, f, g;
}
sevenbyte_t;

/**
 * Eight byte structures. Unknown yet. Likely outdoor mask data.
 */
typedef struct eightbyte
{
  uint8_t eb_a, eb_b, eb_c, eb_d, eb_e, eb_f, eb_g, eb_h;
}
eightbyte_t;

/**
 * Four byte structures. Unknown yet. Likely character meta data.
 */
typedef struct character_meta_data
{
  const uint8_t  **character;
  const sprite_t  *sprite;
}
character_meta_data_t;

/* ----------------------------------------------------------------------- */

#endif /* TYPES_H */

