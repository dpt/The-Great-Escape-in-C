/**
 * Menu.c
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

#include <assert.h>
#include <string.h>

#include "TheGreatEscape/Main.h"
#include "TheGreatEscape/Menu.h"
#include "TheGreatEscape/Music.h"
#include "TheGreatEscape/Screen.h"
#include "TheGreatEscape/State.h"
#include "TheGreatEscape/Text.h"

/* When enabled will start the game immediately in Kempston input mode. */
// #define IMMEDIATE_START

//static
//int check_menu_keys(tgestate_t *state);
//
//static
//void wipe_game_window(tgestate_t *state);

static void choose_keys(tgestate_t *state);

static uint8_t menu_keyscan(tgestate_t *state);

/**
 * $F271: Menu screen key handling.
 *
 * Scan for a keypress which starts the game or selects an input device. If
 * an input device is chosen, update the menu highlight to match and record
 * which input device was chosen.
 *
 * If the game is started and keyboard input device is selected then call
 * choose_keys().
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0 => no keypress, 1 => keypress handled, -1 => start the game
 */
static int check_menu_keys(tgestate_t *state)
{
  uint8_t keycode; /* was A */

  assert(state != NULL);

  keycode = menu_keyscan(state);
  if (keycode == 0xFF)
#ifdef IMMEDIATE_START
    keycode = 2;
#else
    return 0; /* no keypress */
#endif

  if (keycode)
  {
    /* 1..4 -> 0..3 */
    keycode--;

    /* Clear old selection. */
    set_menu_item_attributes(state,
                             state->chosen_input_device,
                             attribute_WHITE_OVER_BLACK);

    /* Highlight new selection. */
    state->chosen_input_device = keycode;
    set_menu_item_attributes(state,
                             keycode,
                             attribute_BRIGHT_YELLOW_OVER_BLACK);

#ifdef IMMEDIATE_START
    return -1;
#else
    return 1;
#endif
  }
  else
  {
    /* Zero pressed to start game. */

    /* Conv: At this point the original game copies the selected input
     * routine to $F075. */

    if (state->chosen_input_device == inputdevice_KEYBOARD)
      choose_keys(state); /* Keyboard was selected. */

    return -1; /* Start the game */
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F335: Wipe the game screen.
 *
 * \param[in] state Pointer to game state.
 */
static void wipe_game_window(tgestate_t *state)
{
  assert(state != NULL);
  assert(state->speccy != NULL);

  uint8_t *const  screen = &state->speccy->screen.pixels[0];
  const uint16_t *poffsets; /* was SP */
  uint8_t         iters;    /* was A */

  poffsets = &state->game_window_start_offsets[0]; /* points to offsets */
  iters = (state->rows - 1) * 8;
  do
  {
    uint8_t *const p = screen + *poffsets++;

    ASSERT_SCREEN_PTR_VALID(p);
    memset(p, 0, state->columns - 1); /* 23 columns (not 24 like the window buffer) */
  }
  while (--iters);

  /* Conv: Invalidation added over the original game. */
  invalidate_bitmap(state,
                    screen + state->game_window_start_offsets[0],
                    state->columns * 8,
                    (state->rows - 1) * 8);
}

/* ----------------------------------------------------------------------- */

/**
 * $F350: Choose keys.
 *
 * \param[in] state Pointer to game state.
 */
static void choose_keys(tgestate_t *state)
{
  /** $F2AD: Key choice prompt strings. */
  static const screenlocstring_t choose_key_prompts[6] =
  {
    { 0x006D, 11, "CHOOSE KEYS" },
    { 0x00CD,  5, "LEFT." },
    { 0x080D,  6, "RIGHT." },
    { 0x084D,  3, "UP." },
    { 0x088D,  5, "DOWN." },
    { 0x08CD,  5, "FIRE." },
  };

  /** $F2E1: Table of keyscan high bytes. */
  static const uint8_t keyboard_port_hi_bytes[10] =
  {
    0x24, /* unused */
    0xF7, 0xEF, 0xFB, 0xDF, 0xFD, 0xBF, 0xFE, 0x7F,
    0x00 /* terminator */
  };

  /** $F2EB: Table of special key name strings, prefixed by their length. */
  static const char special_key_names[] = "\x05" "ENTER"
                                          "\x04" "CAPS"   /* CAPS SHIFT */
                                          "\x06" "SYMBOL" /* SYMBOL SHIFT */
                                          "\x05" "SPACE";

  /* Macro to encode special_key_names in below keycode_to_glyph table. */
#define O(n) ((n) | (1 << 7))

  /**
   * $F303: Table mapping key codes to glyph indices.
   *
   * Each table entry is a character (in original code: a glyph index) OR if
   * its top bit is set then bottom seven bits are an index into
   * special_key_names.
   */
  static const unsigned char keycode_to_glyph[8][5] =
  {
    { '1',   '2',   '3',   '4',   '5', }, /* table_12345 */
    { '0',   '9',   '8',   '7',   '6', }, /* table_09876 */
    { 'Q',   'W',   'E',   'R',   'T', }, /* table_QWERT */
    { 'P',   'O',   'I',   'U',   'Y', }, /* table_POIUY */
    { 'A',   'S',   'D',   'F',   'G', }, /* table_ASDFG */
    { O(0),  'L',   'K',   'J',   'H', }, /* table_ENTERLKJH */
    { O(6),  'Z',   'X',   'C',   'V', }, /* table_SHIFTZXCV */
    { O(18), O(11), 'M',   'N',   'B', }, /* table_SPACESYMSHFTMNB */
  };

#undef O

  /**
   * $F32B: Screen offsets where to plot key names.
   * Conv: In the original code these are absolute addresses.
   */
  static const uint16_t key_name_screen_offsets[5] =
  {
    0x00D5,
    0x0815,
    0x0855,
    0x0895,
    0x08D5,
  };

  assert(state != NULL);
  assert(state->speccy != NULL);

  uint8_t *const screen = &state->speccy->screen.pixels[0]; /* Conv: Added */

  /* Loop while the user does not confirm. */
  for (;;) // FIXME: Loop which doesn't check the quit flag.
  {
    uint8_t                  prompt_iters; /* was B */
    const screenlocstring_t *prompt;       /* was HL */

    /* Clear the game window. */
    wipe_game_window(state);
    set_game_window_attributes(state, attribute_WHITE_OVER_BLACK);

    /* Draw key choice prompts. */
    prompt_iters = NELEMS(choose_key_prompts);
    prompt = &choose_key_prompts[0];
    do
    {
      uint8_t    *screenptr; /* was DE */
      uint8_t     iters;     /* was B */
      const char *string;    /* was HL */

      screenptr = &state->speccy->screen.pixels[prompt->screenloc];
      iters  = prompt->length;
      string = prompt->string;
      do
      {
        // A = *HLstring; /* Conv: Present in original code but this is redundant when calling plot_glyph(). */
        ASSERT_SCREEN_PTR_VALID(screenptr);
        screenptr = plot_glyph(state, string, screenptr);
        string++;
      }
      while (--iters);
      prompt++; /* Conv: Original has all data contiguous, but we need this in addition. */
    }
    while (--prompt_iters);

    /* Wipe key definitions. */
    memset(&state->keydefs.defs[0], 0, 5 * 2);

    uint8_t Adash = 0; /* was A' */ // initialised to zero
    {
      const uint16_t *poffset; /* was HL */

      prompt_iters = NELEMS(key_name_screen_offsets); /* L/R/U/D/F => 5 */
      poffset = &key_name_screen_offsets[0];
      do
      {
        uint16_t  screenoff;  /* was $F3E9 */
        uint8_t   A; /* was A */

        // this block moved up for scope
        keydef_t *keydef;  /* was HL */
        uint8_t   port;    /* was B */
        uint8_t   mask;    /* was C */

        screenoff = *poffset++; // self modify screen addr
        A = 0xFF; /* debounce */

        {
          const uint8_t *hi_bytes;    /* was HL */
          uint8_t        index;       /* was D */
          int            carry = 0;
          // uint8_t        hi_byte;     /* was A' */
          // uint8_t        storedport;  /* was A' */
          uint8_t        keyflags;    /* was E */

for_loop:
          for (;;)
          {
            SWAP(uint8_t, A, Adash);
            hi_bytes = &keyboard_port_hi_bytes[0]; /* Byte 0 is unused. */
            index = 0xFF;
try_next_port:
            hi_bytes++;
            index++;
            A = *hi_bytes;
            if (A == 0) /* Hit end of keyboard_port_hi_bytes. */
              goto for_loop;

            port = A; // saved so it can be stored later
            A = ~state->speccy->in(state->speccy, (port << 8) | 0xFE);
            keyflags = A;
            mask = 1 << 5;
key_loop:
            SRL(mask);
            if (carry)
              goto try_next_port; /* Masking loop ran out. Move to the next keyboard port. */

            A = mask & keyflags;
            if (A == 0) // temps: A'
              goto key_loop; /* Key was not pressed. Move to next key. */

            SWAP(uint8_t, A, Adash);

            /* Keep looping until we've done a full keyscan with no result - debounce */
            if (A)
              goto for_loop;

            /* ... */

            A = index;

            SWAP(uint8_t, A, Adash);

            /* Check for an already defined key. */
            keydef = &state->keydefs.defs[0] - 1;
            do
            {
              keydef++;

              A = keydef->port;
              if (A == 0) /* If an empty slot, assign */
                goto assign_keydef;
            }
            while (A != port || keydef->mask != mask);

            /* Conv: Timing: The original game keyscans as fast as it can. We
             * can't have that so instead we introduce a short delay. */
            state->speccy->stamp(state->speccy);
            state->speccy->sleep(state->speccy, 3500000 / 10); /* 10/sec */
          } // for_loop
        }

assign_keydef:
        keydef->port = port;
        keydef->mask = mask;

        /* Plot */
        {
          const unsigned char *pglyph;          /* was HL */
          const char          *pkeyname;        /* was HL */
          int                  carry = 0;
          uint8_t              length;          /* was B */
          uint8_t              glyph_and_flags; /* was A */
          unsigned char       *screenptr;       /* was DE */

          SWAP(uint8_t, A, Adash);

          pglyph = &keycode_to_glyph[A][0] - 1; /* Off by one to compensate for pre-increment */
          /* Skip entries until 'mask' carries out. */
          do
          {
            pglyph++;
            RR(mask);
          }
          while (!carry);

          // plot the byte at HL, string length is 1
          length = 1;
          glyph_and_flags = *pglyph;
          pkeyname = (const char *) pglyph; // Conv: Added. For type reasons.

          if (glyph_and_flags & (1 << 7))
          {
            /* If the top bit was set then it's a special key,
             * like BREAK or ENTER. */
            pkeyname = &special_key_names[glyph_and_flags & ~(1 << 7)];
            length = *pkeyname++;
          }

          /* Plot. */
          screenptr = screen + screenoff; // self modified // screen offset
          do
          {
            // glyph_and_flags = *pkeyname; // Conv: dead code? similar to other instances of calls to plot_glyph
            ASSERT_SCREEN_PTR_VALID(screenptr);
            screenptr = plot_glyph(state, pkeyname, screenptr);
            pkeyname++;
          }
          while (--length);
        }
      }
      while (--prompt_iters);
    }

    /* Conv: Timing: This was a delay loop counting to 0xFFFF. */
    state->speccy->stamp(state->speccy);
    state->speccy->sleep(state->speccy, 3500000 / 10); /* 10/sec */

    /* Wait for user's input */
    if (user_confirm(state) == 0) /* Confirmed - return */
      return; /* Start the game */
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F408: Set the screen attributes of the specified menu item.
 *
 * \param[in] state Pointer to game state.
 * \param[in] index Item index. (was A)
 * \param[in] attrs Screen attributes. (was E)
 */
void set_menu_item_attributes(tgestate_t *state,
                              int         index,
                              attribute_t attrs)
{
  attribute_t *pattr;

  assert(state != NULL);
  assert(index < 4); // a guess
  assert(attrs <= attribute_BRIGHT_WHITE_OVER_BLACK);
  assert(state->speccy != NULL);

  pattr = &state->speccy->screen.attributes[(0x590D - SCREEN_ATTRIBUTES_START_ADDRESS)];

  /* Skip to the item's row */
  pattr += index * 2 * state->width; /* two rows */

  /* Draw */
  ASSERT_SCREEN_ATTRIBUTES_PTR_VALID(pattr);
  memset(pattr, attrs, 10);

  /* Conv: Invalidation added over the original game. */
  invalidate_attrs(state, pattr, 10 * 8, 8);
}

/* ----------------------------------------------------------------------- */

/**
 * $F41C: Scan for keys to select an input device.
 *
 * \param[in] state Pointer to game state.
 *
 * \return 0/1/2/3/4 = keypress, or 255 = no keypress
 */
static uint8_t menu_keyscan(tgestate_t *state)
{
  uint8_t count;   /* was E */
  uint8_t keymask; /* was A */
  uint8_t iters;   /* was B */

  assert(state != NULL);
  assert(state->speccy != NULL);

  count = 0;
  /* Keys 1..4 only. */
  keymask = ~state->speccy->in(state->speccy, port_KEYBOARD_12345) & 0xF;
  if (keymask)
  {
    iters = 4;
    do
    {
      /* Conv: Reordered from original to avoid carry flag test */
      count++;
      if (keymask & 1)
        break;
      keymask >>= 1;
    }
    while (--iters);

    return count;
  }
  else
  {
    /* Key 0 only. */
    if ((state->speccy->in(state->speccy, port_KEYBOARD_09876) & 1) == 0)
      return count; /* always zero */

    return 0xFF; /* no keypress */
  }
}

/* ----------------------------------------------------------------------- */

/**
 * $F4B7: Run the menu screen.
 *
 * Waits for user to select an input device, waves the morale flag and plays
 * the title tune.
 *
 * \param[in] state Pointer to game state.
 *
 * \return Non-zero when the game should begin, zero otherwise.
 */
int menu_screen(tgestate_t *state)
{
  uint16_t counter_0;       /* was BC */
  uint16_t counter_1;       /* was BC' */
  uint16_t frequency_0;     /* was DE */
  uint16_t frequency_1;     /* was DE' */
  uint8_t  datum;           /* was A */
  uint8_t  major_delay;     /* was A */
  uint8_t  minor_delay;     /* was H */
  uint16_t channel0_index;  /* was HL */
  uint16_t channel1_index;  /* was HL' */
  uint8_t  speaker0;        /* was L */
  uint8_t  speaker1;        /* was L' */
  uint8_t  B;               /* was B */
  uint8_t  C;               /* was C */
  uint8_t  Bdash;           /* was B' */
  uint8_t  Cdash;           /* was C' */
  uint8_t  bit = 0; // most recently emitted bit

  assert(state != NULL);

  state->speccy->stamp(state->speccy);

  /* Conv: Menu driving loop was removed and the routine changed to return
   * non-zero when the game should begin. */
  if (check_menu_keys(state) < 0)
  {
    /* Cancel the above stamp. */
    state->speccy->sleep(state->speccy, 0);
    return 1; /* Start the game */
  }

  wave_morale_flag(state);

  /* Play music */

  channel0_index = state->music_channel0_index + 1;
  for (;;)
  {
    state->music_channel0_index = channel0_index;
    datum = music_channel0_data[channel0_index];
    if (datum != 0xFF) /* end marker */
      break;
    channel0_index = 0;
  }
  frequency_0 = counter_0 = frequency_for_semitone(datum, &speaker0);

  channel1_index = state->music_channel1_index + 1;
  for (;;)
  {
    state->music_channel1_index = channel1_index;
    datum = music_channel1_data[channel1_index];
    if (datum != 0xFF) /* end marker */
      break;
    channel1_index = 0;
  }
  frequency_1 = counter_1 = frequency_for_semitone(datum, &speaker1);

  /* When the second channel is silent use the first channel's frequency. */
  if ((counter_1 >> 8) == 0xFF) // (BCdash >> 8) was Bdash;
    frequency_1 = counter_1 = counter_0;

  major_delay = 24; /* overall tune speed (a delay: lower values are faster) */
  do
  {
    minor_delay = 255;
    do
    {
      int to_emit = 3; // Conv: Whatever we do always emit this many bits

      // B,C are a pair of counters? half pulse length?
      // B = lo, C = hi  (in this routine)

      B = counter_0 >> 8;
      C = counter_0 & 0xFF;
      if (--B == 0 && --C == 0)
      {
        speaker0 ^= port_MASK_EAR;
        bit = speaker0 & port_MASK_EAR;
        state->speccy->out(state->speccy, port_BORDER_EAR_MIC, speaker0);
        to_emit--;
        counter_0 = frequency_0;
      }
      else
      {
        counter_0 = (B << 8) | C;
      }

      Bdash = counter_1 >> 8;
      Cdash = counter_1 & 0xFF;
      if (--Bdash == 0 && --Cdash == 0)
      {
        speaker1 ^= port_MASK_EAR;
        bit = speaker1 & port_MASK_EAR;
        state->speccy->out(state->speccy, port_BORDER_EAR_MIC, speaker1);
        to_emit--;
        counter_1 = frequency_1;
      }
      else
      {
        counter_1 = (Bdash << 8) | Cdash;
      }

      /* Conv: We simulate the EAR port holding its level by adding extra
       * OUTs here. */
      // FIXME: The timing needs working out here: 3 OUTs per loop works,
      // but why?
      while (to_emit-- > 0)
        state->speccy->out(state->speccy, port_BORDER_EAR_MIC, bit);
    }
    while (--minor_delay);
  }
  while (--major_delay);

  /* Conv: Timing: Calibrated to original game. */
  state->speccy->sleep(state->speccy, 300365);

  return 0;
}

// vim: ts=8 sts=2 sw=2 et

