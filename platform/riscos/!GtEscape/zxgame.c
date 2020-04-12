/* --------------------------------------------------------------------------
 *    Name: zxgame.c
 * Purpose: ZX game handling
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#include "kernel.h"
#include "swis.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oslib/types.h"
#include "oslib/colourtrans.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/osspriteop.h"
#include "oslib/wimp.h"
#include "oslib/wimpreadsysinfo.h"

#include "appengine/types.h"
#include "appengine/base/messages.h"
#include "appengine/base/os.h"
#include "appengine/base/oserror.h"
#include "appengine/base/strings.h"
#include "appengine/datastruct/list.h"
#include "appengine/dialogues/scale.h"
#include "appengine/geom/box.h"
#include "appengine/vdu/screen.h"
#include "appengine/vdu/sprite.h"
#include "appengine/wimp/event.h"
#include "appengine/wimp/help.h"
#include "appengine/wimp/icon.h"
#include "appengine/wimp/menu.h"
#include "appengine/wimp/window.h"

#include "ZXSpectrum/Kempston.h"
#include "ZXSpectrum/Keyboard.h"
#include "ZXSpectrum/Spectrum.h"

#include "TheGreatEscape/TheGreatEscape.h"

#include "globals.h"
#include "bitfifo.h"
#include "menunames.h"
#include "poll.h"
#include "ssbuffer.h"
#include "timermod.h"
#include "zxgame.h"
#include "zxgames.h"
#include "zxscale.h"

/* ----------------------------------------------------------------------- */

/* Configuration */

#define GAMEWIDTH       (256)   /* pixels */
#define GAMEHEIGHT      (192)   /* pixels */
#define GAMEBORDER      (16)    /* pixels */
#define GAMEEIG         (2)     /* natural scale of game (EIG 2 = 45dpi) */

#define MAXSTAMPS       (4)     /* max depth of timestamps stack */
#define MAXSPEED        (32767) /* fastest possible game (percent) */

/* ----------------------------------------------------------------------- */

/* Audio */

#define SAMPLE_RATE     (44100)
#define AVG             (5)     // average this many input bits to make an output sample {magic}
#define PERIOD          (10)    // process 10th sec at a time
#define BITFIFO_LENGTH_BITS (SAMPLE_RATE * AVG / PERIOD)
#define BITFIFO_LENGTH  (BITFIFO_LENGTH_BITS / 8)

/* ----------------------------------------------------------------------- */

typedef int scale_t;
#define SCALE_1         (512)   // 1.0 in scale_t units

typedef int fix16_t;
#define FIX16_1         (65536) // 1.0 in fix16_t units

/* ----------------------------------------------------------------------- */

/* Flags */

#define zxgame_FLAG_QUIT        (1u <<  0) /* quit the game */
#define zxgame_FLAG_PAUSED      (1u <<  1) /* game is paused */
#define zxgame_FLAG_FIRST       (1u <<  2) /* first render */
#define zxgame_FLAG_MENU        (1u <<  3) /* game menu running */
#define zxgame_FLAG_DODGE_NULLS (1u <<  4) /* don't react to nulls */
#define zxgame_FLAG_MONOCHROME  (1u <<  5) /* display as monochrome */
#define zxgame_FLAG_FIT         (1u <<  6) /* fit game to window */
#define zxgame_FLAG_SNAP        (1u <<  7) /* whole pixel snapping (in fit-to-window mode) */
#define zxgame_FLAG_HAVE_CARET  (1u <<  8) /* we own the caret */
#define zxgame_FLAG_BIG_WINDOW  (1u <<  9) /* size window to screen */
#define zxgame_FLAG_SOUND       (1u << 10) /* sound is enabled */
#define zxgame_FLAG_WIDE_CTRANS (1u << 11) /* use wide ColourTrans table */

/* ----------------------------------------------------------------------- */

struct zxgame
{
  list_t                list;           /* A game is a node in a linked list. */

  wimp_w                w;              /* The "primary key". */

  struct
  {
    int                 cur;//, prev;      /* Current and previous scale factors. */
  }
  scale;

  struct
  {
    os_colour           colour;
  }
  background;

  unsigned int          flags;

  zxspectrum_t         *zx;
  tgestate_t           *tge;

  int                   border_size;    // pixels
  int                   speed;          // percent

  timer_t               stamps[MAXSTAMPS];
  int                   nstamps;

  zxkeyset_t            keys;
  zxkempston_t          kempston;

  osspriteop_area      *sprite;
  os_factors            factors;
  osspriteop_trans_tab *trans_tab;

  int                   sleep_us; // us to sleep for on the next loop

  int                   win_width, win_height; // OS units
  os_box                extent; // extent of window w
  os_box                imgbox; // where to draw the image, positioned within the window extent

  struct
  {
    int                 index;
    bitfifo_t          *fifo;
    int                 lastvol;  // most recently output volume
    ssndbuf_t           buf;
    unsigned int       *data;
  }
  audio;
};

/* ----------------------------------------------------------------------- */

static void set_palette(zxgame_t *zxgame);
static void destroy_game(zxgame_t *zxgame);

/* ----------------------------------------------------------------------- */

/* Snap the given point to the current mode's pixel grid. */
static void snap2px(int *x, int *y)
{
  int xeig, yeig;

  read_current_mode_vars(&xeig, &yeig, NULL);
  *x &= ~((1 << xeig) - 1);
  *y &= ~((1 << yeig) - 1);
}

/* Snap the given os_box to the current mode's pixel grid. */
static void snapbox2px(os_box *b)
{
  int xeig, yeig;

  read_current_mode_vars(&xeig, &yeig, NULL);
  box_round(b, xeig, yeig);
}

/* ----------------------------------------------------------------------- */

/* Fill in a single edge. */
static void draw_edge(const wimp_draw *draw, const os_box *box)
{
  os_box clip;

  if (box_intersection(&draw->clip, box, &clip))
    return;

  screen_clip(&clip);
  os_writec(os_VDU_CLG);
}

/* Draw the window background by filling in the regions around the outside of
 * opaque bitmaps. This avoids flicker. */
static void draw_edges_only(zxgame_t  *zxgame,
                      const wimp_draw *draw,
                            int        x,
                            int        y)
{
  const os_box *extent;
  const os_box *imgbox;
  os_box        box;

  colourtrans_set_gcol(zxgame->background.colour,
                       colourtrans_SET_BG_GCOL,
                       os_ACTION_OVERWRITE,
                       NULL);

  extent = &zxgame->extent;
  imgbox = &zxgame->imgbox;

  /* Draw the edges in order: top, bottom, left, right. */

  box.x0 = extent->x0 + x;
  box.y0 = imgbox->y1 + y;
  box.x1 = extent->x1 + x;
  box.y1 = extent->y1 + y;
  draw_edge(draw, &box);

  colourtrans_set_gcol(os_COLOUR_GREEN,
                       colourtrans_SET_BG_GCOL,
                       os_ACTION_OVERWRITE,
                       NULL);

  /* Top and bottom share x coordinates. */
  box.y0 = extent->y0 + y;
  box.y1 = imgbox->y0 + y;
  draw_edge(draw, &box);

  colourtrans_set_gcol(os_COLOUR_YELLOW,
                       colourtrans_SET_BG_GCOL,
                       os_ACTION_OVERWRITE,
                       NULL);

  /* Left and bottom share x0 only. */
  box.y0 = imgbox->y0 + y;
  box.x1 = imgbox->x0 + x;
  box.y1 = imgbox->y1 + y;
  draw_edge(draw, &box);

  colourtrans_set_gcol(os_COLOUR_BLUE,
                       colourtrans_SET_BG_GCOL,
                       os_ACTION_OVERWRITE,
                       NULL);

  /* Right and left share y coordinates. */
  box.x0 = imgbox->x1 + x;
  box.x1 = extent->x1 + x;
  draw_edge(draw, &box);

  screen_clip(&draw->clip);
}

static void redrawfn(zxgame_t *zxgame, wimp_draw *draw, osbool more)
{
  osspriteop_action action;

  action = os_ACTION_OVERWRITE;
  if (zxgame->flags & zxgame_FLAG_WIDE_CTRANS)
    action |= osspriteop_GIVEN_WIDE_ENTRIES;

  for (; more; more = wimp_get_rectangle(draw))
  {
    int x,y;

    /* Calculate where the top left of the window would be on-screen */
    x = draw->box.x0 - draw->xscroll;
    y = draw->box.y1 - draw->yscroll;

    draw_edges_only(zxgame, draw, x, y);

    /* Position it */
    x += zxgame->imgbox.x0;
    y += zxgame->imgbox.y0;

    osspriteop_put_sprite_scaled(osspriteop_PTR,
                                 zxgame->sprite,
                 (osspriteop_id) sprite_select(zxgame->sprite, 0),
                                 x,y,
                                 action,
                                &zxgame->factors,
                                 zxgame->trans_tab);
  }
}

static void draw_handler(const zxbox_t *dirty,
                         void          *opaque)
{
  zxgame_t  *zxgame = opaque;
  uint32_t  *pixels;

  pixels = zxspectrum_claim_screen(zxgame->zx);

  if (zxgame->flags & zxgame_FLAG_FIRST)
  {
    /* First time this has been drawn - copy all. */
    static const zxbox_t all = { 0, 0, 256, 192 };
    zxgame->flags &= ~zxgame_FLAG_FIRST;
    dirty = &all;
  }

  /* Copy across the dirty region of the bitmap. */
  {
    char   *dst;
    char   *src;
    size_t  rowbytes;
    int     dx0, dy0, dx1, dy1;
    int     w;
    int     y;

    dst = (char *) zxgame->sprite + 16 + 44 + (16 * 4 * 2);
    src = (char *) pixels;
    rowbytes = 256 / 2;

    dx0 = (dirty->x0    ) >> 1; // round down to byte boundary
    dx1 = (dirty->x1 + 1) >> 1; // round up
    w = dx1 - dx0; // width in bytes

    // todo: double check these inversions
    dy0 = 192 - dirty->y1;
    dy1 = 192 - dirty->y0;

    for (y = dy0; y < dy1; y++)
      memcpy(dst + y * rowbytes + dx0, src + y * rowbytes + dx0, w);
  }

  zxspectrum_release_screen(zxgame->zx);

  /* Convert the dirty region into work area coordinates. */
  {
    int       x0, y0;
    scale_t   scale;
    wimp_draw draw;

    x0 = zxgame->imgbox.x0;
    y0 = zxgame->imgbox.y0;

    scale = zxgame->scale.cur * SCALE_1 / 100; // % -> scale_t

    draw.w = zxgame->w;
    draw.box.x0 = x0 + (dirty->x0 << GAMEEIG) * scale / SCALE_1;
    draw.box.y0 = y0 + (dirty->y0 << GAMEEIG) * scale / SCALE_1;
    draw.box.x1 = x0 + (dirty->x1 << GAMEEIG) * scale / SCALE_1;
    draw.box.y1 = y0 + (dirty->y1 << GAMEEIG) * scale / SCALE_1;
    snapbox2px(&draw.box);
    redrawfn(zxgame, &draw, wimp_update_window(&draw));
  }
}

static void stamp_handler(void *opaque)
{
  zxgame_t *zxgame = opaque;

  if (zxgame->nstamps < MAXSTAMPS)
  {
    read_timer(&zxgame->stamps[zxgame->nstamps]);
    zxgame->nstamps++;
  }
}

static int should_quit(const zxgame_t *zxgame)
{
  return (GLOBALS.flags & Flag_Quit) != 0 ||
         (zxgame->flags & zxgame_FLAG_QUIT) != 0;
}

static int sleep_handler(int durationTStates, void *opaque)
{
  zxgame_t *zxgame = opaque;

  //fprintf(stderr, "sleep @ %d (os_mono)\n", os_read_monotonic_time());

  /* Unstack timestamps */
  assert(zxgame->nstamps > 0);
  if (zxgame->nstamps > 0)
    --zxgame->nstamps;

  if (should_quit(zxgame))
    return 1;

  if ((zxgame->flags & zxgame_FLAG_PAUSED) != 0)
  {
    int quit;
    int paused;

    zxgame->flags |= zxgame_FLAG_DODGE_NULLS; // could do this instead by disabling our null event handler (might need event lib changes to be efficient though)

    do
    {
      poll_set_target(os_read_monotonic_time() + 100); /* sleep 1s */
      poll();
      quit = should_quit(zxgame);
      paused = (zxgame->flags & zxgame_FLAG_PAUSED) != 0;
    }
    while (!quit && paused);

    zxgame->flags &= ~zxgame_FLAG_DODGE_NULLS;

    if (quit)
      return 1;
  }

  {
    const int tstatesPerSec = 3500000; // 3.5e6

    float               duration_s;
    timer_t             now;
    const timer_t      *then;
    float               consumed_s;
    float               sleep_s;

    // how much should we consume?
    duration_s = (float) durationTStates / tstatesPerSec; // T-states to secs
    duration_s = duration_s * 100.0f / zxgame->speed; // scale by speed - inverse percentage

    // how much time have we consumed so far?
    read_timer(&now);
    then = &zxgame->stamps[zxgame->nstamps];
    consumed_s = diff_timer(&now, then);
    //fprintf(stderr, "consumed(sec)=%f, duration(sec)=%f\n", consumed_s, duration_s);

    sleep_s = duration_s - consumed_s;
    if (sleep_s > 0) // if we need to sleep then delay
    {
      os_t now_cs = os_read_monotonic_time();
      os_t target;

      target = now_cs + sleep_s * 100.0f; // sec -> csec
      //fprintf(stderr, "sleep(sec)=%f now(csec)=%d target(csec)=%d\n", sleep_s, now_cs, target);
      zxgame->flags |= zxgame_FLAG_DODGE_NULLS; // could do this instead by disabling our null event handler (might need event lib changes to be efficient though)
      do
      {
        poll_set_target(target);
        poll();
      }
      while (os_read_monotonic_time() < target);
      zxgame->flags &= ~zxgame_FLAG_DODGE_NULLS;
      //fprintf(stderr, "resuming\n");
    }
  }

  // return immediately: run the game as fast as possible
  return 0;
}

static int key_handler(uint16_t port, void *opaque)
{
  zxgame_t *zxgame = opaque;
  int       key_in;

  /* if our window lacks input focus then return the previous state */
  if ((zxgame->flags & zxgame_FLAG_HAVE_CARET) == 0)
    goto exit;

  /* clear all keys */
  zxkeyset_clear(&zxgame->keys);
  zxgame->kempston = 0;

  /* scan pressed keys, starting at the lowest internal key number: Shift */
  for (key_in = 0; ; )
  {
    int          key_out;
    zxkey_t      index;
    zxjoystick_t joystick;

    key_out = osbyte1(osbyte_IN_KEY, key_in ^ 0x7F, 0xFF);
    if (key_out == 0xFF)
      break;

    index    = zxkey_UNKNOWN;
    joystick = zxjoystick_UNKNOWN;

    switch (key_out)
    {
      /* don't consume Ctrl - reserve it for shortcuts */
      case   1: goto exit;

      case  48: index = zxkey_1;                break; // ZX row 1
      case  49: index = zxkey_2;                break;
      case  17: index = zxkey_3;                break;
      case  18: index = zxkey_4;                break;
      case  19: index = zxkey_5;                break;
      case  52: index = zxkey_6;                break;
      case  36: index = zxkey_7;                break;
      case  21: index = zxkey_8;                break;
      case  38: index = zxkey_9;                break;
      case  39: index = zxkey_0;                break;
      case  16: index = zxkey_Q;                break; // ZX row 2
      case  33: index = zxkey_W;                break;
      case  34: index = zxkey_E;                break;
      case  51: index = zxkey_R;                break;
      case  35: index = zxkey_T;                break;
      case  68: index = zxkey_Y;                break;
      case  53: index = zxkey_U;                break;
      case  37: index = zxkey_I;                break;
      case  54: index = zxkey_O;                break;
      case  55: index = zxkey_P;                break;
      case  65: index = zxkey_A;                break; // ZX row 3
      case  81: index = zxkey_S;                break;
      case  50: index = zxkey_D;                break;
      case  67: index = zxkey_F;                break;
      case  83: index = zxkey_G;                break;
      case  84: index = zxkey_H;                break;
      case  69: index = zxkey_J;                break;
      case  70: index = zxkey_K;                break;
      case  86: index = zxkey_L;                break;
      case  73: index = zxkey_ENTER;            break;
      case   0: index = zxkey_CAPS_SHIFT;       break; // ZX row 4 // either Shift key
      case  97: index = zxkey_Z;                break;
      case  66: index = zxkey_X;                break;
      case  82: index = zxkey_C;                break;
      case  99: index = zxkey_V;                break;
      case 100: index = zxkey_B;                break;
      case  85: index = zxkey_N;                break;
      case 101: index = zxkey_M;                break;
      case   2: index = zxkey_SYMBOL_SHIFT;     break; // either Alt key
      case  98: index = zxkey_SPACE;            break;

      case  57: joystick = zxjoystick_UP;       break; // joystick
      case  41: joystick = zxjoystick_DOWN;     break;
      case  25: joystick = zxjoystick_LEFT;     break;
      case 121: joystick = zxjoystick_RIGHT;    break;
      case 103: joystick = zxjoystick_FIRE;     break;
    }

    if (index != zxkey_UNKNOWN)
      zxkeyset_assign(&zxgame->keys, index, 1 /* on */);
    if (joystick != zxjoystick_UNKNOWN)
      zxkempston_assign(&zxgame->kempston, joystick, 1 /* on */);

    key_in = key_out + 1;
  }

exit:
  if (port == port_KEMPSTON_JOYSTICK)
    return zxgame->kempston;
  else
    return zxkeyset_for_port(port, &zxgame->keys);
}

static void border_handler(int colour, void *opaque)
{
  NOT_USED(colour);
  NOT_USED(opaque);
}

static void speaker_handler(int on_off, void *opaque)
{
  zxgame_t *zxgame = opaque;

  if ((zxgame->flags & zxgame_FLAG_SOUND) == 0)
    return;

#if 0
  unsigned int bit = on_off;
  size_t       bitsInFifo;

  (void) bitfifo_enqueue(zxgame->audio.fifo, &bit, 0, 1);

  bitsInFifo = bitfifo_used(zxgame->audio.fifo);
  if (bitsInFifo < BITFIFO_LENGTH)
    return;

  static const unsigned char tab[32] =
  {
    0, 1, 1, 2,
    1, 2, 2, 3,
    1, 2, 2, 3,
    2, 3, 3, 4,
    1, 2, 2, 3,
    2, 3, 3, 4,
    2, 3, 3, 4,
    3, 4, 4, 5
  };

  p = buf;
  for (;;)
  {
    unsigned int bits;

    err = bitfifo_dequeue(zxgame->audio.fifo, &bits, AVG);
    if (err == result_BITFIFO_INSUFFICIENT || err == result_BITFIFO_EMPTY)
      break;

    vol = tab[bits] * maxVol / AVG;  // assumes avg is 5
    *p++ = (vol << 0) | (vol << 16);
  }

  zxgame->audio.lastvol = vol;

  _swi(SharedSoundBuffer_AddBlock, _INR(0,1), handle, buf, p - buf);
#endif
}

/* ----------------------------------------------------------------------- */

static event_wimp_handler zxgame_event_null_reason_code,
                          zxgame_event_redraw_window_request,
                          zxgame_event_open_window_request,
                          zxgame_event_close_window_request,
                          zxgame_event_mouse_click,
                          zxgame_event_key_pressed,
                          zxgame_event_menu_selection,
                          zxgame_event_losegain_caret;

/* ----------------------------------------------------------------------- */

static void register_handlers(int reg, const zxgame_t *zxgame)
{
  static const event_wimp_handler_spec wimp_handlers[] =
  {
    { wimp_NULL_REASON_CODE,      zxgame_event_null_reason_code      },
    { wimp_REDRAW_WINDOW_REQUEST, zxgame_event_redraw_window_request },
    { wimp_OPEN_WINDOW_REQUEST,   zxgame_event_open_window_request   },
    { wimp_CLOSE_WINDOW_REQUEST,  zxgame_event_close_window_request  },
    { wimp_MOUSE_CLICK,           zxgame_event_mouse_click           },
    { wimp_KEY_PRESSED,           zxgame_event_key_pressed           },
    { wimp_LOSE_CARET,            zxgame_event_losegain_caret        },
    { wimp_GAIN_CARET,            zxgame_event_losegain_caret        },
  };

  event_register_wimp_group(reg,
                            wimp_handlers,
                            NELEMS(wimp_handlers),
                            zxgame->w,
                            event_ANY_ICON,
                            zxgame);
}

static error set_handlers(zxgame_t *zxgame)
{
  error err;

  register_handlers(1, zxgame);

  err = help_add_window(zxgame->w, "zxgame");

  return err;
}

static void release_handlers(zxgame_t *zxgame)
{
  help_remove_window(zxgame->w);

  register_handlers(0, zxgame);
}

static void register_single_handlers(int reg)
{
  /* menu_selection doesn't associate with a specific window so should only
   * be registered once. */
  static const event_wimp_handler_spec wimp_handlers[] =
  {
    { wimp_MENU_SELECTION, zxgame_event_menu_selection },
  };

  event_register_wimp_group(reg,
                            wimp_handlers,
                            NELEMS(wimp_handlers),
                            event_ANY_WINDOW,
                            event_ANY_ICON,
                            NULL);
}

/* ----------------------------------------------------------------------- */

static int zxgame_event_null_reason_code(wimp_event_no event_no,
                                         wimp_block   *block,
                                         void         *handle)
{
  zxgame_t *zxgame;

  NOT_USED(event_no);
  NOT_USED(block);

  zxgame = handle;

  if (zxgame->flags & zxgame_FLAG_QUIT)
    return event_HANDLED;

  if (zxgame->flags & zxgame_FLAG_DODGE_NULLS)
    return event_PASS_ON;

  if (zxgame->flags & zxgame_FLAG_MENU)
  {
    if (tge_menu(zxgame->tge) > 0)
    {
      tge_setup2(zxgame->tge);
      zxgame->flags &= ~zxgame_FLAG_MENU;
    }
  }
  else
  {
    tge_main(zxgame->tge);
  }

  return event_PASS_ON;
}

static int zxgame_event_redraw_window_request(wimp_event_no event_no,
                                              wimp_block   *block,
                                              void         *handle)
{
  wimp_draw *draw;
  zxgame_t  *zxgame;

  NOT_USED(event_no);

  draw   = &block->redraw;
  zxgame = handle;

  redrawfn(zxgame, draw, wimp_redraw_window(draw));

  return event_HANDLED;
}

static int zxgame_event_open_window_request(wimp_event_no event_no,
                                            wimp_block   *block,
                                            void         *handle)
{
  wimp_open *open;
  zxgame_t  *zxgame;

  NOT_USED(event_no);

  open   = &block->open;
  zxgame = handle;

  if (zxgame->flags & zxgame_FLAG_FIT)
  {
    int win_width, win_height;

    /* Calculate the window's visible width and height (OS units) */
    win_width  = open->visible.x1 - open->visible.x0;
    win_height = open->visible.y1 - open->visible.y0;

    if (win_width != zxgame->win_width || win_height != zxgame->win_height)
    {
      zxgame->win_width  = win_width;
      zxgame->win_height = win_height;

      zxgame_update(zxgame, zxgame_UPDATE_EXTENT | zxgame_UPDATE_REDRAW);
    }

    /* Disable scrolling */
    open->xscroll = open->yscroll = 0;
  }

  wimp_open_window(open);

  return event_HANDLED;
}

static int zxgame_event_close_window_request(wimp_event_no event_no,
                                             wimp_block   *block,
                                             void         *handle)
{
  wimp_close *close;
  zxgame_t   *zxgame;

  NOT_USED(event_no);

  close  = &block->close;
  zxgame = handle;

//  if (zxgame_query_unload(zxgame))
    zxgame_destroy(zxgame);

  return event_HANDLED;
}

static void tick(wimp_menu *m, int entry, int ticked)
{
  menu_set_menu_flags(m,
                      entry,
                      ticked ? wimp_MENU_TICKED : 0,
                      wimp_MENU_TICKED);
}

static void zxgame_menu_update(void)
{
  zxgame_t  *zxgame;
  wimp_menu *m;

  zxgame = GLOBALS.current_zxgame;
  if (zxgame == NULL)
    return;

  /* View menu */

  m = GLOBALS.zxgame_m->entries[ZXGAME_VIEW].sub_menu;

  tick(m, VIEW_MONOCHROME, (zxgame->flags & zxgame_FLAG_MONOCHROME) != 0);

  /* Game menu */

  m = GLOBALS.zxgame_m->entries[ZXGAME_VIEW].sub_menu;
  m = m->entries[VIEW_GAME].sub_menu;

  tick(m, GAME_FIT_WINDOW,  (zxgame->flags & zxgame_FLAG_FIT)  != 0);
  tick(m, GAME_SNAP_PIXELS, (zxgame->flags & zxgame_FLAG_SNAP) != 0);

  /* Window menu */

  m = GLOBALS.zxgame_m->entries[ZXGAME_VIEW].sub_menu;
  m = m->entries[VIEW_WINDOW].sub_menu;

  tick(m, WINDOW_FIT_GAME,   (zxgame->flags & zxgame_FLAG_BIG_WINDOW) == 0);
  tick(m, WINDOW_FIT_SCREEN, (zxgame->flags & zxgame_FLAG_BIG_WINDOW) != 0);

  /* Speed menu */

  m = GLOBALS.zxgame_m->entries[ZXGAME_SPEED].sub_menu;

  tick(m, SPEED_PAUSE,   (zxgame->flags & zxgame_FLAG_PAUSED) != 0);
  tick(m, SPEED_100PC,   (zxgame->speed == 100));
  tick(m, SPEED_MAXIMUM, (zxgame->speed == MAXSPEED));

  /* Sound menu */

  m = GLOBALS.zxgame_m->entries[ZXGAME_SOUND].sub_menu;

  tick(m, SOUND_ENABLED, (zxgame->flags & zxgame_FLAG_SOUND) != 0);
}

static int zxgame_event_mouse_click(wimp_event_no event_no,
                                    wimp_block   *block,
                                    void         *handle)
{
  wimp_pointer *pointer;
  zxgame_t     *zxgame;

  NOT_USED(event_no);
  NOT_USED(block);

  pointer = &block->pointer;
  zxgame  = handle;

  GLOBALS.current_zxgame = zxgame;

  if (pointer->buttons & wimp_CLICK_MENU)
  {
    zxgame_menu_update();

    menu_open(GLOBALS.zxgame_m, pointer->pos.x - 64, pointer->pos.y);
  }
  else
  {
    wimp_set_caret_position((zxgame->flags & zxgame_FLAG_HAVE_CARET) ? wimp_BACKGROUND : pointer->w,
                            wimp_ICON_WINDOW,
                            0,0,
                            1 << 25, /* invisible */
                            0);
  }

  return event_HANDLED;
}

static int zxgame_event_key_pressed(wimp_event_no event_no,
                                    wimp_block   *block,
                                    void         *handle)
{
  wimp_key *key;
  zxgame_t *zxgame;

  NOT_USED(event_no);

  key    = &block->key;
  zxgame = handle;

  switch (key->c)
  {
    case wimp_KEY_CONTROL | wimp_KEY_F2:
      zxgame->flags |= zxgame_FLAG_QUIT; // doesn't clean up properly
      // needs to zxgame_destroy(zxgame); once the quit is processed
      break;

    default:
      if (isalnum(key->c)           ||
          key->c == wimp_KEY_RETURN ||
          key->c == ' '             ||
          key->c == wimp_KEY_UP     ||
          key->c == wimp_KEY_DOWN   ||
          key->c == wimp_KEY_LEFT   ||
          key->c == wimp_KEY_RIGHT)
      {
        /* consume any key presses that the game would normally accept */
      }
      else
      {
        wimp_process_key(key->c);
      }
      break;
  }

  return event_HANDLED;
}

static int zxgame_event_menu_selection(wimp_event_no event_no,
                                       wimp_block   *block,
                                       void         *handle)
{
  wimp_selection *selection;
  wimp_menu      *last;
  wimp_pointer    p;

  NOT_USED(event_no);
  NOT_USED(handle);

  selection = &block->selection;

  /* We will receive this event on *any* menu selection. It's essential to
   * reject events not intended for us. */
  last = menu_last();
  if (last != GLOBALS.zxgame_m)
    return event_NOT_HANDLED;

  switch (selection->items[0])
  {
  case ZXGAME_VIEW:
    switch (selection->items[1])
    {
    case VIEW_GAME:
      switch (selection->items[2])
      {
      case GAME_SCALE: // is a submenu only
        break;

      case GAME_FIT_WINDOW:
        GLOBALS.current_zxgame->flags ^= zxgame_FLAG_FIT;
        zxgame_update(GLOBALS.current_zxgame, zxgame_UPDATE_EXTENT | zxgame_UPDATE_REDRAW);
        break;

      case GAME_SNAP_PIXELS:
        GLOBALS.current_zxgame->flags ^= zxgame_FLAG_SNAP;
        zxgame_update(GLOBALS.current_zxgame, zxgame_UPDATE_EXTENT | zxgame_UPDATE_REDRAW);
        break;
      }
      break;

    case VIEW_WINDOW:
      switch (selection->items[2])
      {
      case WINDOW_FIT_GAME:
      case WINDOW_FIT_SCREEN:
        GLOBALS.current_zxgame->flags &= ~zxgame_FLAG_BIG_WINDOW;
        if (selection->items[2] == WINDOW_FIT_SCREEN)
          GLOBALS.current_zxgame->flags ^= zxgame_FLAG_BIG_WINDOW;
        zxgame_update(GLOBALS.current_zxgame, zxgame_UPDATE_EXTENT | zxgame_UPDATE_REDRAW);
        break;
      }
      break;

    case VIEW_MONOCHROME:
      GLOBALS.current_zxgame->flags ^= zxgame_FLAG_MONOCHROME;
      set_palette(GLOBALS.current_zxgame);
      zxgame_update(GLOBALS.current_zxgame, zxgame_UPDATE_COLOURS | zxgame_UPDATE_REDRAW);
      break;
    }
    break;

  case ZXGAME_SPEED:
    switch (selection->items[1])
    {
    case SPEED_PAUSE:
      GLOBALS.current_zxgame->flags ^= zxgame_FLAG_PAUSED;
      break;

    case SPEED_100PC:
      GLOBALS.current_zxgame->speed = 100;
      break;

    case SPEED_MAXIMUM:
      GLOBALS.current_zxgame->speed = MAXSPEED;
      break;

    case SPEED_FASTER:
      GLOBALS.current_zxgame->speed += 50;
      if (GLOBALS.current_zxgame->speed >= MAXSPEED)
        GLOBALS.current_zxgame->speed = MAXSPEED;
      break;

    case SPEED_SLOWER:
      GLOBALS.current_zxgame->speed -= 50;
      if (GLOBALS.current_zxgame->speed <= 0)
        GLOBALS.current_zxgame->speed = 1;
      break;
    }
    break;

  case ZXGAME_SOUND:
    switch (selection->items[1])
    {
    case SOUND_ENABLED:
      GLOBALS.current_zxgame->flags ^= zxgame_FLAG_SOUND;
      break;
    }
    break;
  }

  wimp_get_pointer_info(&p);
  if (p.buttons & wimp_CLICK_ADJUST)
  {
    zxgame_menu_update();
    menu_reopen();
  }

  return event_HANDLED;
}

static int zxgame_event_losegain_caret(wimp_event_no event_no,
                                       wimp_block   *block,
                                       void         *handle)
{
  wimp_caret *caret;
  zxgame_t   *zxgame;

  NOT_USED(event_no);

  caret  = &block->caret;
  zxgame = handle;

  if (event_no == wimp_GAIN_CARET)
    zxgame->flags |= zxgame_FLAG_HAVE_CARET;
  else
    zxgame->flags &= ~zxgame_FLAG_HAVE_CARET;

  return event_HANDLED;
}

/* ----------------------------------------------------------------------- */

static error gentranstab(zxgame_t *zxgame)
{
  colourtrans_table_flags flags;

  free(zxgame->trans_tab);
  zxgame->trans_tab = malloc(16 * 4); // CHECK is this the worst case?
  if (zxgame->trans_tab == NULL)
    return error_OOM;

  flags = colourtrans_GIVEN_SPRITE;
  if (zxgame->flags & zxgame_FLAG_WIDE_CTRANS)
    flags |= colourtrans_RETURN_WIDE_ENTRIES;

  colourtrans_generate_table_for_sprite(zxgame->sprite,
                        (osspriteop_id) sprite_select(zxgame->sprite, 0),
                                        os_CURRENT_MODE,
                                        colourtrans_CURRENT_PALETTE,
                                        zxgame->trans_tab,
                                        flags,
                                        NULL,
                                        NULL);
  return error_OK;
}

void zxgame_update(zxgame_t *zxgame, zxgame_update_flags flags)
{
  if (flags & zxgame_UPDATE_COLOURS)
    gentranstab(zxgame);

  if (flags & (zxgame_UPDATE_SCALING | zxgame_UPDATE_EXTENT))
  {
    const int   image_xeig = GAMEEIG, image_yeig = GAMEEIG;
    const int   border = zxgame->border_size; /* pixels */

    scale_t     scale;
    int         screen_xeig, screen_yeig;

    int         extent_w, extent_h;
    scale_t     scaled_w, scaled_h;
    int         left_x, bottom_y;

    scale = zxgame->scale.cur * SCALE_1 / 100; // % -> scale_t

    read_current_mode_vars(&screen_xeig, &screen_yeig, NULL);

    // set this flag if any of its controlling vars change (e.g. scale)
    if (flags & zxgame_UPDATE_EXTENT)
    {
      if (zxgame->flags & zxgame_FLAG_FIT)
      {
        // fit game to window mode

        int reduced_border_x, reduced_border_y;
        int games_per_window;

        /* How many 1:1 games fit comfortably in the window? */
        reduced_border_x = reduced_border_y = zxgame->border_size << GAMEEIG; /* pixels to OS units */
        do
        {
          fix16_t game_widths_per_window, game_heights_per_window;

          game_widths_per_window  = (zxgame->win_width  - reduced_border_x * 2) * FIX16_1 / (GAMEWIDTH  << GAMEEIG);
          game_heights_per_window = (zxgame->win_height - reduced_border_y * 2) * FIX16_1 / (GAMEHEIGHT << GAMEEIG);
          games_per_window = MIN(game_widths_per_window, game_heights_per_window);
          if (reduced_border_x > 0) reduced_border_x -= screen_xeig;
          if (reduced_border_y > 0) reduced_border_y -= screen_yeig;
        }
        // loop while there's borders to reduce and we're still struggling to fit a whole game in the window
        while (games_per_window < FIX16_1 && (reduced_border_x > 0 || reduced_border_y > 0));

        /* Snap the game scale to whole units */
        if (games_per_window >= 1.0 && (zxgame->flags & zxgame_FLAG_SNAP))
          scale = (games_per_window >> 16) * SCALE_1;
        else
          scale = (games_per_window * SCALE_1) >> 16;

        scale = CLAMP(scale, 1, 8000);

        scaled_w = ((GAMEWIDTH  + reduced_border_x * 2) << image_xeig) * scale / SCALE_1;
        scaled_h = ((GAMEHEIGHT + reduced_border_y * 2) << image_yeig) * scale / SCALE_1;
//        scaled_w = (scaled_w + 3) & ~3; // temp
//        scaled_h = (scaled_h + 3) & ~3; // temp

        zxgame->scale.cur = scale * 100 / SCALE_1; // scale_t -> %

        flags |= zxgame_UPDATE_SCALING;

        /* centre the box */
        left_x   =   (zxgame->win_width  - scaled_w) / 2;
        bottom_y = -((zxgame->win_height - scaled_h) / 2 + scaled_h);
      }
      else
      {
        // non-fit mode

        if (zxgame->flags & zxgame_FLAG_BIG_WINDOW) // window is to be sized to screen
        {
          read_max_visible_area(zxgame->w, &extent_w, &extent_h); // read max workarea size
        }
        else // window is to be sized to game+border
        {
          extent_w = ((GAMEWIDTH  + border * 2) << image_xeig) * scale / SCALE_1;
          extent_h = ((GAMEHEIGHT + border * 2) << image_yeig) * scale / SCALE_1;
          snap2px(&extent_w, &extent_h);
        }

        // TODO: make use of the returned minsize flag
        (void) window_set_extent2(zxgame->w, 0, -extent_h, extent_w, 0);

        // save the extent
        zxgame->extent.x0 = 0;
        zxgame->extent.y0 = -extent_h;
        zxgame->extent.x1 = extent_w;
        zxgame->extent.y1 = 0;

        scaled_w = (GAMEWIDTH  << image_xeig) * scale / SCALE_1;
        scaled_h = (GAMEHEIGHT << image_yeig) * scale / SCALE_1;
        snap2px(&scaled_w, &scaled_h);

        // centre the box
        left_x   = zxgame->extent.x0 + (extent_w - scaled_w) / 2;
        bottom_y = zxgame->extent.y0 + (extent_h - scaled_h) / 2;
        snap2px(&left_x, &bottom_y);
      }
    }

    zxgame->imgbox.x0 = left_x;
    zxgame->imgbox.y0 = bottom_y;
    zxgame->imgbox.x1 = left_x   + scaled_w;
    zxgame->imgbox.y1 = bottom_y + scaled_h;
    snapbox2px(&zxgame->imgbox);

    // update sprite scaling
    if (flags & zxgame_UPDATE_SCALING)
    {
      os_factors_from_ratio(&zxgame->factors, scale, SCALE_1);
      zxgame->factors.xmul <<= image_xeig;
      zxgame->factors.ymul <<= image_yeig;
      zxgame->factors.xdiv <<= screen_xeig;
      zxgame->factors.ydiv <<= screen_yeig;
      fprintf(stderr, "zxgame_update: factors: mul %d %d div %d %d\n",
              zxgame->factors.xmul, zxgame->factors.ymul,
              zxgame->factors.xdiv, zxgame->factors.ydiv);
    }
  }

  if (flags & zxgame_UPDATE_REDRAW)
    window_redraw(zxgame->w);
}

/* ----------------------------------------------------------------------- */

static void set_palette(zxgame_t *zxgame)
{
#define BLK (os_COLOUR_BLACK   / 0xFFu)
#define BLU (os_COLOUR_BLUE    / 0xFFu)
#define RED (os_COLOUR_RED     / 0xFFu)
#define MAG (os_COLOUR_MAGENTA / 0xFFu)
#define GRN (os_COLOUR_GREEN   / 0xFFu)
#define CYN (os_COLOUR_CYAN    / 0xFFu)
#define YLW (os_COLOUR_YELLOW  / 0xFFu)
#define WHT (os_COLOUR_WHITE   / 0xFFu)

#define DEFINE_PALETTE \
  BRIGHT(BLK), \
  BRIGHT(BLU), \
  BRIGHT(RED), \
  BRIGHT(MAG), \
  BRIGHT(GRN), \
  BRIGHT(CYN), \
  BRIGHT(YLW), \
  BRIGHT(WHT),

  static const os_SPRITE_PALETTE(16) stdpalette =
  {{
#define BRIGHT(C) { C * 0xAA, C * 0xAA }
    DEFINE_PALETTE
#undef BRIGHT
#define BRIGHT(C) { C * 0xFF, C * 0xFF }
    DEFINE_PALETTE
#undef BRIGHT
  }};

  int                palette_size;
  os_sprite_palette *palette;

  osspriteop_read_palette_size(osspriteop_PTR,
                               zxgame->sprite,
               (osspriteop_id) sprite_select(zxgame->sprite, 0),
                              &palette_size,
                              &palette,
                               NULL);

  assert(palette_size * 8 == sizeof(stdpalette));
  memcpy(palette, &stdpalette, sizeof(stdpalette));

  if (zxgame->flags & zxgame_FLAG_MONOCHROME)
  {
    /* Convert the palette to monochrome. */

    const int red_weight   = 19595; /* 0.29900 * 65536 (rounded down) */
    const int green_weight = 38470; /* 0.58700 * 65536 (rounded up)   */
    const int blue_weight  =  7471; /* 0.11400 * 65536 (rounded down) */

    int i;

    for (i = 0; i < 16; i++)
    {
      os_colour c;
      int       r, g, b;
      int       grey;

      /* 0xBBGGRR00 */

      c = palette->entries[i].on;
      r = (c >> 8)  & 0xFF;
      g = (c >> 16) & 0xFF;
      b = (c >> 24) & 0xFF;

      grey = (r * red_weight + g * green_weight + b * blue_weight) >> 16;
      c = (grey << 24) | (grey << 16) | (grey << 8);
      palette->entries[i].on = palette->entries[i].off = c;
    }
  }
}

error zxgame_create(zxgame_t **new_zxgame)
{
  static const zxconfig_t zxconfigconsts =
  {
    GAMEWIDTH / 8, GAMEHEIGHT / 8,
    NULL, /* opaque */
    &draw_handler,
    &stamp_handler,
    &sleep_handler,
    &key_handler,
    &border_handler,
    &speaker_handler
  };

  error      err      = error_OK;
  zxgame_t  *zxgame   = NULL;
  size_t     sprareasz;
  zxconfig_t zxconfig = zxconfigconsts;

  *new_zxgame = NULL;

  zxgame = calloc(1, sizeof(*zxgame));
  if (zxgame == NULL)
    goto NoMem;

  zxgame->flags = zxgame_FLAG_FIRST | zxgame_FLAG_MENU;

  // 1.64 is the RISC OS 3.6 version (hoist to main())
  if (xos_cli("RMEnsure ColourTrans 1.64") == NULL)
    zxgame->flags |= zxgame_FLAG_WIDE_CTRANS;

  if (0) { // if shared sound is available (hoist to main())
    zxgame->flags |= zxgame_FLAG_SOUND;
  }

  zxgame->w = window_clone(GLOBALS.zxgame_w);
  if (zxgame->w == NULL)
    goto NoMem;

  zxgame->scale.cur = 100;
  //zxgame->scale.prev = 100;
  zxgame->speed = 100;
  zxgame->border_size = GAMEBORDER;

  sprareasz = sprite_size(GAMEWIDTH, GAMEHEIGHT, 4, TRUE);

  zxgame->sprite = malloc(sprareasz);
  if (zxgame->sprite == NULL)
    goto NoMem;

  zxgame->sprite->size  = sprareasz;
  zxgame->sprite->first = 16;
  osspriteop_clear_sprites(osspriteop_PTR, zxgame->sprite);

  osspriteop_create_sprite(osspriteop_PTR,
                           zxgame->sprite,
                          "zxgame",
                           TRUE, /* paletted */
                           GAMEWIDTH, GAMEHEIGHT,
                           os_MODE4BPP45X45);

  set_palette(zxgame);

  zxgame->trans_tab = NULL;
  gentranstab(zxgame);

  zxgame_update(zxgame, zxgame_UPDATE_ALL); // FIXME: won't need _ALL

  zxgame->background.colour = os_COLOUR_MID_LIGHT_GREY;

  // =====

  zxconfig.opaque = zxgame;

  zxgame->zx = zxspectrum_create(&zxconfig);
  if (zxgame->zx == NULL)
    goto Failure;

  zxgame->tge = tge_create(zxgame->zx);
  if (zxgame->tge == NULL)
    goto Failure;

  // maybe just do all the sound init lazily
  zxgame->audio.fifo = bitfifo_create(BITFIFO_LENGTH); // fixme: iff sound
  if (zxgame->audio.fifo == NULL)
    goto NoMem;

  if (zxgame->flags & zxgame_FLAG_SOUND)
  {
    _swi(SharedSoundBuffer_OpenStream, _INR(0,1)|_OUT(0),
         0,
         message0("task"),
        &zxgame->audio.buf);
  }

  tge_setup(zxgame->tge);

  register_handlers(1, zxgame);

  zxgame_add(zxgame);

  *new_zxgame = zxgame;

  return error_OK;


NoMem:

  err = error_OOM;

  goto Failure;


Failure:

  // more cleanup

  free(zxgame);

  error_report(err);

  return err;
}

void zxgame_destroy(zxgame_t *zxgame)
{
  if (zxgame == NULL)
    return;

  bitfifo_destroy(zxgame->audio.fifo);

  if (zxgame->flags & zxgame_FLAG_SOUND)
  {
    _swi(SharedSoundBuffer_CloseStream, _IN(0),
         zxgame->audio.buf); // stop audio dead
  }

  zxgame_remove(zxgame);

  /* Delete the window */
  window_delete_cloned(zxgame->w);

  register_handlers(0, zxgame);
  tge_destroy(zxgame->tge);
  zxspectrum_destroy(zxgame->zx);
  free(zxgame->trans_tab);
  free(zxgame->sprite);
  free(zxgame);
}

/* ----------------------------------------------------------------------- */

int zxgame_get_scale(zxgame_t *zxgame)
{
  return zxgame->scale.cur;
}

void zxgame_set_scale(zxgame_t *zxgame, int scale)
{
  if (scale == zxgame->scale.cur)
    return;

  zxgame->scale.cur = scale;

// check: how were scaling and extent different?
  zxgame_update(zxgame, zxgame_UPDATE_SCALING | zxgame_UPDATE_EXTENT | zxgame_UPDATE_REDRAW);
}

/* ----------------------------------------------------------------------- */

void zxgame_open(zxgame_t *zxgame)
{
  window_open_at(zxgame->w, AT_CENTRE);
}

/* ----------------------------------------------------------------------- */

error zxgame_init(void)
{
  error err;

  /* main? Dependencies */

  err = help_init();
  if (err)
    return err;

  /* Handlers */

  register_single_handlers(1);

  GLOBALS.zxgame_w = window_create("zxgame");

  /* Internal dependencies */

  err = zxgamescale_dlg_init();
  if (err)
    return err;

  /* Menu */

  GLOBALS.zxgame_m = menu_create_from_desc(message0("menu.zxgame"),
                                           dialogue_get_window(zxgamescale_dlg));

  err = help_add_menu(GLOBALS.zxgame_m, "zxgame");
  if (err)
    return err;

  return error_OK;
}

void zxgame_fin(void)
{
  help_remove_menu(GLOBALS.zxgame_m);

  menu_destroy(GLOBALS.zxgame_m);

  zxgamescale_dlg_fin();

  register_single_handlers(0);

  help_fin();
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et
