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

#include "fortify/fortify.h"

#include "oslib/types.h"
#include "oslib/colourtrans.h"
#include "oslib/hourglass.h"
#include "oslib/os.h"
#include "oslib/osbyte.h"
#include "oslib/osfile.h"
#include "oslib/osspriteop.h"
#include "oslib/wimp.h"
#include "oslib/wimpreadsysinfo.h"

#include "appengine/types.h"
#include "appengine/base/bitwise.h"
#include "appengine/base/bsearch.h"
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
#include "zxsave.h"

/* ----------------------------------------------------------------------- */

/* Configuration */

#define GAMEWIDTH       (256)   /* pixels */
#define GAMEHEIGHT      (192)   /* pixels */
#define GAMEBORDER      (16)    /* pixels */
#define GAMEEIG         (2)     /* natural scale of game (EIG 2 = 45dpi) */

#define MAXSTAMPS       (4)     /* max depth of timestamps stack */
#define SPEEDQ          (20)    /* smallest unit of speed (percent) */
#define NORMSPEED       (100)   /* normal speed (percent) */
#define MAXSPEED        (99999) /* fastest possible game (percent) */

/* ----------------------------------------------------------------------- */

/* Audio */

#define SAMPLE_RATE     (44100)
#define PERIOD          (10)    /* fraction of a second (10 => 0.1s) */
#define BUFFER_SAMPLES  (SAMPLE_RATE / PERIOD)
#define BITS_SAMPLE     (5)     /* magic value: we take the mean of this many input bits to make an output sample */
#define BITFIFO_LENGTH  (BUFFER_SAMPLES * BITS_SAMPLE) /* in bits */

#define MAXVOL          (32767 / 8)

/* ----------------------------------------------------------------------- */

/* Types */

typedef int scale_t;
#define SCALE_1         (1024)  /* 1.0 in scale_t units */

typedef int fix16_t;
#define FIX16_1         (65536) /* 1.0 in fix16_t units */

/* ----------------------------------------------------------------------- */

/* Flags */

#define zxgame_FLAG_QUIT        (1u <<  0) /* quit the game */
#define zxgame_FLAG_PAUSED      (1u <<  1) /* game is paused */
#define zxgame_FLAG_FIRST       (1u <<  2) /* first render */
#define zxgame_FLAG_MENU        (1u <<  3) /* game menu running */
#define zxgame_FLAG_SLEEPING    (1u <<  4) /* null shouldn't drive game */
#define zxgame_FLAG_MONOCHROME  (1u <<  5) /* display as monochrome */
#define zxgame_FLAG_FIT         (1u <<  6) /* fit game to window */
#define zxgame_FLAG_SNAP        (1u <<  7) /* whole pixel snapping (in fit-to-window mode) */
#define zxgame_FLAG_HAVE_CARET  (1u <<  8) /* we own the caret */
#define zxgame_FLAG_BIG_WINDOW  (1u <<  9) /* size window to screen */
#define zxgame_FLAG_HAVE_SOUND  (1u << 10) /* sound is available */
#define zxgame_FLAG_SOUND_ON    (1u << 11) /* sound is required */
#define zxgame_FLAG_WIDE_CTRANS (1u << 12) /* use wide ColourTrans table */

/* ----------------------------------------------------------------------- */

struct zxgame
{
  list_t                list;   /* A game is a node in a linked list. */

  wimp_w                w;      /* The "primary key". */

  struct
  {
    int                 cur;    /* Current and previous scale factors. */
    int                 prev;
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

  int                   border_size; /* Pixels */
  int                   speed;  /* Percent */

  timer_t               stamps[MAXSTAMPS];
  int                   nstamps;

  zxkeyset_t            keys;
  zxkempston_t          kempston;

  osspriteop_area      *sprite;
  os_factors            factors;
  osspriteop_trans_tab *trans_tab;

  int                   window_w, window_h; /* OS units */
  int                   xscroll, yscroll;

  os_box                extent; /* Extent of window w */
  os_box                imgbox; /* Ewhere to draw the image, positioned within the window extent */

  struct
  {
    int                 index;
    bitfifo_t          *fifo;
    ssndbuf_t           stream;
    unsigned int       *data;
  }
  audio;
};

/* ----------------------------------------------------------------------- */

static void set_palette(zxgame_t *zxgame);

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
    return; /* invalid intersection */

  screen_clip(&clip);
  os_writec(os_VDU_CLG);
}

/* Draw the window background by filling in the regions around the outside of
 * the image. This avoids flicker. */
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

  /* Top and bottom share x coordinates. */
  box.y0 = extent->y0 + y;
  box.y1 = imgbox->y0 + y;
  draw_edge(draw, &box);

  /* Left and bottom share x0 only. */
  box.y0 = imgbox->y0 + y;
  box.x1 = imgbox->x0 + x;
  box.y1 = imgbox->y1 + y;
  draw_edge(draw, &box);

  /* Right and left share y coordinates. */
  box.x0 = imgbox->x1 + x;
  box.x1 = extent->x1 + x;
  draw_edge(draw, &box);

  screen_clip(&draw->clip);
}

static void redrawfn(zxgame_t *zxgame, wimp_draw *draw, osbool more)
{
  osspriteop_action action;
  osspriteop_id     id;

  action = os_ACTION_OVERWRITE;
  if (zxgame->flags & zxgame_FLAG_WIDE_CTRANS)
    action |= osspriteop_GIVEN_WIDE_ENTRIES;

  id = (osspriteop_id) sprite_select(zxgame->sprite, 0);

  for (; more; more = wimp_get_rectangle(draw))
  {
    int x,y;

    /* Calculate where the top left of the window would be on-screen. */
    x = draw->box.x0 - draw->xscroll;
    y = draw->box.y1 - draw->yscroll;

    draw_edges_only(zxgame, draw, x, y);

    /* Position the sprite. */
    x += zxgame->imgbox.x0;
    y += zxgame->imgbox.y0;

    osspriteop_put_sprite_scaled(osspriteop_PTR,
                                 zxgame->sprite,
                                 id,
                                 x,y,
                                 action,
                                &zxgame->factors,
                                 zxgame->trans_tab);
  }
}

/* Game callback. */
static void draw_handler(const zxbox_t *dirty,
                         void          *opaque)
{
  zxgame_t *zxgame = opaque;
  uint32_t *pixels;

  pixels = zxspectrum_claim_screen(zxgame->zx);

  if (zxgame->flags & zxgame_FLAG_FIRST)
  {
    /* The first time this image has been drawn - copy all. */
    static const zxbox_t all = { 0, 0, GAMEWIDTH, GAMEHEIGHT };
    zxgame->flags &= ~zxgame_FLAG_FIRST;
    dirty = &all;
  }

  /* Copy across the dirty region of the bitmap. */
  {
    char   *dst;
    char   *src;
    size_t  rowbytes;
    int     dx0, dy0, dx1, dy1;
    int     w, h;

    dst = sprite_data(sprite_select(zxgame->sprite, 0));
    src = (char *) pixels;
    rowbytes = GAMEWIDTH / 2;

    dx0 = (dirty->x0    ) >> 1; /* round down 4bpp to byte boundary (incl) */
    dx1 = (dirty->x1 + 1) >> 1; /* round up 4bpp to byte boundary (excl) */
    w = dx1 - dx0; /* width in bytes */

    // TODO: Double check these inversions (does excl to incl mean we're off by one?)
    dy0 = GAMEHEIGHT - dirty->y1;
    dy1 = GAMEHEIGHT - dirty->y0;
    h = dy1 - dy0;

    dst += dy0 * rowbytes + dx0;
    src += dy0 * rowbytes + dx0;
    while (h--)
    {
      memcpy(dst, src, w);
      dst += rowbytes;
      src += rowbytes;
    }
  }

  zxspectrum_release_screen(zxgame->zx);

  /* Convert the dirty region into work area coordinates. */
  {
    int       x0, y0;
    scale_t   scale;
    wimp_draw draw;

    x0 = zxgame->imgbox.x0;
    y0 = zxgame->imgbox.y0;

    scale = zxgame->scale.cur * SCALE_1 / 100; /* % -> scale_t */

    draw.w = zxgame->w;
    draw.box.x0 = x0 + (dirty->x0 << GAMEEIG) * scale / SCALE_1;
    draw.box.y0 = y0 + (dirty->y0 << GAMEEIG) * scale / SCALE_1;
    draw.box.x1 = x0 + (dirty->x1 << GAMEEIG) * scale / SCALE_1;
    draw.box.y1 = y0 + (dirty->y1 << GAMEEIG) * scale / SCALE_1;
    snapbox2px(&draw.box);
    redrawfn(zxgame, &draw, wimp_update_window(&draw));
  }
}

/* Game callback. */
static void stamp_handler(void *opaque)
{
  zxgame_t *zxgame = opaque;

  if (zxgame->nstamps >= MAXSTAMPS)
    return;

  read_timer(&zxgame->stamps[zxgame->nstamps]);
  zxgame->nstamps++;
}

static int should_quit(const zxgame_t *zxgame)
{
  return (GLOBALS.flags & Flag_Quit) != 0 ||
         (zxgame->flags & zxgame_FLAG_QUIT) != 0;
}

/* Game callback. */
static int sleep_handler(int durationTStates, void *opaque)
{
  zxgame_t *zxgame = opaque;

  //fprintf(stderr, "sleep @ %d (os_mono)\n", os_read_monotonic_time());

  /* Unstack timestamps. */
  assert(zxgame->nstamps > 0);
  if (zxgame->nstamps > 0)
    --zxgame->nstamps;

  if (should_quit(zxgame))
    return 1;

  /* Handle pausing. */
  if ((zxgame->flags & zxgame_FLAG_PAUSED) != 0)
  {
    os_t target_cs;
    int  quit;
    int  paused;

    do
    {
      target_cs = os_read_monotonic_time() + 100; /* sleep 1s */
      poll_set_target(target_cs);
      poll();
      quit = should_quit(zxgame);
      paused = (zxgame->flags & zxgame_FLAG_PAUSED) != 0;
    }
    while (!quit && paused && os_read_monotonic_time() < target_cs);

    if (quit)
      return 1;
  }

  /* Handle actual sleeping. */
  {
    const float tstatesPerSec = 3500000.0f;

    float          duration_s;
    const timer_t *then;
    timer_t        now;
    float          consumed_s;
    float          sleep_s;

    /* How much time should this sleep handler consume? */
    duration_s = durationTStates / tstatesPerSec; /* T-states -> secs */
    duration_s = duration_s * 100.0f / zxgame->speed; /* scale to match speed */

    /* How much time have we consumed so far? */
    then = &zxgame->stamps[zxgame->nstamps];
    read_timer(&now);
    consumed_s = diff_timer(&now, then);
    //fprintf(stderr, "consumed(sec)=%f, duration(sec)=%f\n", consumed_s, duration_s);

    /* How much remains? */
    sleep_s = duration_s - consumed_s;
    if (sleep_s > 0)
    {
      /* If we need to sleep then delay here by polling the Wimp. */

      os_t now_cs, target_cs;
      int  quit;

      now_cs    = os_read_monotonic_time();
      target_cs = now_cs + (os_t) (sleep_s * 100.0f); /* sec -> centisec */
      //fprintf(stderr, "sleep(sec)=%f now(csec)=%d target(csec)=%d\n", sleep_s, now_cs, target);
      zxgame->flags |= zxgame_FLAG_SLEEPING;
      do
      {
        poll_set_target(target_cs);
        poll();
        quit = should_quit(zxgame);
      }
      while (!quit && os_read_monotonic_time() < target_cs);
      zxgame->flags &= ~zxgame_FLAG_SLEEPING;
    }
  }

  return 0;
}

/* Game callback. */
static int key_handler(uint16_t port, void *opaque)
{
  zxgame_t *zxgame = opaque;
  int       key_in;

  /* If our window lacks input focus then return the previous key state. */
  if ((zxgame->flags & zxgame_FLAG_HAVE_CARET) == 0)
    goto exit;

  /* Clear all keys. */
  zxkeyset_clear(&zxgame->keys);
  zxgame->kempston = 0;

  /* Scan pressed keys, starting at the lowest internal key number: Shift. */
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
      /* Don't consume Ctrl - reserve it for shortcuts. */
      case   1: goto exit;

      case  48: index = zxkey_1;                break; /* ZX row 1 */
      case  49: index = zxkey_2;                break;
      case  17: index = zxkey_3;                break;
      case  18: index = zxkey_4;                break;
      case  19: index = zxkey_5;                break;
      case  52: index = zxkey_6;                break;
      case  36: index = zxkey_7;                break;
      case  21: index = zxkey_8;                break;
      case  38: index = zxkey_9;                break;
      case  39: index = zxkey_0;                break;
      case  16: index = zxkey_Q;                break; /* ZX row 2 */
      case  33: index = zxkey_W;                break;
      case  34: index = zxkey_E;                break;
      case  51: index = zxkey_R;                break;
      case  35: index = zxkey_T;                break;
      case  68: index = zxkey_Y;                break;
      case  53: index = zxkey_U;                break;
      case  37: index = zxkey_I;                break;
      case  54: index = zxkey_O;                break;
      case  55: index = zxkey_P;                break;
      case  65: index = zxkey_A;                break; /* ZX row 3 */
      case  81: index = zxkey_S;                break;
      case  50: index = zxkey_D;                break;
      case  67: index = zxkey_F;                break;
      case  83: index = zxkey_G;                break;
      case  84: index = zxkey_H;                break;
      case  69: index = zxkey_J;                break;
      case  70: index = zxkey_K;                break;
      case  86: index = zxkey_L;                break;
      case  73: index = zxkey_ENTER;            break;
      case   0: index = zxkey_CAPS_SHIFT;       break; /* ZX row 4 - either Shift key */
      case  97: index = zxkey_Z;                break;
      case  66: index = zxkey_X;                break;
      case  82: index = zxkey_C;                break;
      case  99: index = zxkey_V;                break;
      case 100: index = zxkey_B;                break;
      case  85: index = zxkey_N;                break;
      case 101: index = zxkey_M;                break;
      case   2: index = zxkey_SYMBOL_SHIFT;     break; /* Either Alt key */
      case  98: index = zxkey_SPACE;            break;

      case  57: joystick = zxjoystick_UP;       break; /* Joystick */
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

/* Game callback. */
static void border_handler(int colour, void *opaque)
{
  zxgame_t *zxgame = opaque;
  os_colour c;

  switch (colour)
  {
    case 0:  c = os_COLOUR_BLACK;   break;
    case 1:  c = os_COLOUR_BLUE;    break;
    case 2:  c = os_COLOUR_RED;     break;
    case 3:  c = os_COLOUR_MAGENTA; break;
    case 4:  c = os_COLOUR_GREEN;   break;
    case 5:  c = os_COLOUR_CYAN;    break;
    case 6:  c = os_COLOUR_YELLOW;  break;
    case 7:  c = os_COLOUR_WHITE;   break;
    default: c = os_COLOUR_ORANGE;  break;
  }

  zxgame->background.colour = colour;

  zxgame_update(zxgame, zxgame_UPDATE_REDRAW);
}

static result_t setup_sound(zxgame_t *zxgame)
{
  result_t         err;
  _kernel_oserror *kerr;

  if ((zxgame->flags & zxgame_FLAG_HAVE_SOUND) == 0)
  {
    zxgame->flags &= ~zxgame_FLAG_SOUND_ON; /* ensure sound deselected */
    return error_NOT_SUPPORTED; /* no sound hardware */
  }

  if ((zxgame->flags & zxgame_FLAG_SOUND_ON) == 0)
    return error_NOT_SUPPORTED; /* not requested */

  if (zxgame->audio.stream != 0)
    return error_OK; /* already setup */

  zxgame->audio.fifo = bitfifo_create(BITFIFO_LENGTH);
  if (zxgame->audio.fifo == NULL)
  {
     err = error_OOM;
     goto Failure;
  }

  zxgame->audio.data = malloc(BUFFER_SAMPLES * 4);
  if (zxgame->audio.data == NULL)
  {
     err = error_OOM;
     goto Failure;
  }

  kerr = _swix(SharedSoundBuffer_OpenStream, _INR(0,1)|_OUT(0),
               0,
               message0("task"),
              &zxgame->audio.stream);
  if (kerr)
  {
    err = error_OS;
    goto Failure;
  }

  return error_OK;


Failure:
  free(zxgame->audio.data);
  zxgame->audio.data = NULL;

  bitfifo_destroy(zxgame->audio.fifo);
  zxgame->audio.fifo = NULL;

  return err;
}

static void teardown_sound(zxgame_t *zxgame)
{
  zxgame->flags &= ~zxgame_FLAG_SOUND_ON; /* ensure sound deselected */

  if ((zxgame->flags & zxgame_FLAG_HAVE_SOUND) == 0)
    return; /* no sound hardware */

  (void) _swix(SharedSoundBuffer_CloseStream, _IN(0),
               zxgame->audio.stream);
  zxgame->audio.stream = 0;

  free(zxgame->audio.data);
  zxgame->audio.data = NULL;

  bitfifo_destroy(zxgame->audio.fifo);
  zxgame->audio.fifo = NULL;
}

static void emit_sound(zxgame_t *zxgame)
{
  result_t      err;
  int           fetch;
  unsigned int *p;
  size_t        databytes;

  err = setup_sound(zxgame);
  if (err)
    return;

  fetch = BITS_SAMPLE * zxgame->speed / NORMSPEED;
  fetch = CLAMP(fetch, 1, 32);

  p = zxgame->audio.data;
  for (;;)
  {
    unsigned int bitqueue = 0;
    unsigned int vol      = 0;

    err = bitfifo_dequeue(zxgame->audio.fifo, &bitqueue, fetch);
    if (err)
      break;

    vol = countbits(bitqueue) * MAXVOL / fetch;
    *p++ = (vol << 0) | (vol << 16);
    if (p >= zxgame->audio.data + BUFFER_SAMPLES * 4)
      break;
  }

  databytes = (p - zxgame->audio.data) * 4;
  if (databytes)
    (void) _swix(SharedSoundBuffer_AddBlock, _INR(0,2), zxgame->audio.stream,
                                                        zxgame->audio.data,
                                                        databytes);

}

/* Game callback. */
static void speaker_handler(int on_off, void *opaque)
{
  const unsigned int SOUNDFLAGS = zxgame_FLAG_HAVE_SOUND | zxgame_FLAG_SOUND_ON;

  error         err;
  zxgame_t     *zxgame = opaque;
  unsigned int  bits;

  if ((zxgame->flags & SOUNDFLAGS) != SOUNDFLAGS)
    return;

  err = setup_sound(zxgame);
  if (err)
    return;

  bits = on_off;
  (void) bitfifo_enqueue(zxgame->audio.fifo, &bits, 0, 1);
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

typedef enum action
{
  SelectFixedScale,
  OpenScaleViewDialogue,
  ToggleBigWindow,
  SelectScaledToFit,
  ToggleSnapToPixels,
  FullScreen,
  ToggleMonochrome,
  SpeedNormal,
  SpeedMax,
  Faster,
  Slower,
  TogglePause,
  ToggleSound,
  OpenSaveGame,
  OpenSaveScreenshot,
  ZoomOut,
  ZoomIn,
  ToggleZoom,
  ResetZoom,
  Close
}
action_t;

static void action(action_t action)
{
  zxgame_t *zxgame = GLOBALS.current_zxgame;

  // TODO: Narrow these update flags down where possible.

  switch (action)
  {
    case SelectFixedScale:
      zxgame->flags &= ~zxgame_FLAG_FIT;
      zxgame_update(zxgame, zxgame_UPDATE_SCALING | zxgame_UPDATE_EXTENT | zxgame_UPDATE_REDRAW);
      break;

    case OpenScaleViewDialogue:
      dialogue_show(zxgamescale_dlg);
      break;

    case ToggleBigWindow:
      zxgame->flags ^= zxgame_FLAG_BIG_WINDOW;
      if ((zxgame->flags & zxgame_FLAG_FIT) == 0)
        zxgame_update(zxgame, zxgame_UPDATE_SCALING | zxgame_UPDATE_EXTENT | zxgame_UPDATE_REDRAW);
      break;

    case SelectScaledToFit:
      {
        wimp_window_state state;

        state.w = zxgame->w;
        wimp_get_window_state(&state);

        zxgame->window_w = state.visible.x1 - state.visible.x0;
        zxgame->window_h = state.visible.y1 - state.visible.y0;
        zxgame->xscroll  = state.xscroll;
        zxgame->yscroll  = state.yscroll;

        zxgame->flags |= zxgame_FLAG_FIT;
        zxgame_update(zxgame, zxgame_UPDATE_EXTENT | zxgame_UPDATE_WINDOW | zxgame_UPDATE_REDRAW);
      }
      break;

    case ToggleSnapToPixels:
      zxgame->flags ^= zxgame_FLAG_SNAP;
      if (zxgame->flags & zxgame_FLAG_FIT)
        zxgame_update(zxgame, zxgame_UPDATE_EXTENT | zxgame_UPDATE_WINDOW | zxgame_UPDATE_EXTENT | zxgame_UPDATE_REDRAW);
      break;

    case FullScreen:
      // TODO
      break;

    case ToggleMonochrome:
      zxgame->flags ^= zxgame_FLAG_MONOCHROME;
      set_palette(zxgame);
      zxgame_update(zxgame, zxgame_UPDATE_COLOURS | zxgame_UPDATE_REDRAW);
      break;

    case SpeedNormal:
      zxgame->speed = NORMSPEED;
      break;

    case SpeedMax:
      zxgame->speed = MAXSPEED;
      break;

    case Faster:
      zxgame->speed += SPEEDQ;
      if (zxgame->speed >= MAXSPEED)
        zxgame->speed = MAXSPEED;
      break;

    case Slower:
      zxgame->speed -= SPEEDQ;
      if (zxgame->speed <= SPEEDQ)
        zxgame->speed = SPEEDQ;
      break;

    case TogglePause:
      zxgame->flags ^= zxgame_FLAG_PAUSED;
      break;

    case ToggleSound:
      if (zxgame->flags & zxgame_FLAG_HAVE_SOUND)
      {
        zxgame->flags ^= zxgame_FLAG_SOUND_ON;
        if (zxgame->flags & zxgame_FLAG_SOUND_ON)
          setup_sound(zxgame);
        else
          teardown_sound(zxgame);
      }
      break;

    case OpenSaveGame:
      zxgamesave_show_game();
      break;

    case OpenSaveScreenshot:
      zxgamesave_show_screenshot();
      break;

    case ZoomOut:
      zxgame_set_scale(zxgame, zxgame_get_scale(zxgame) / 2);
      break;

    case ZoomIn:
      zxgame_set_scale(zxgame, zxgame_get_scale(zxgame) * 2);
      break;

    case ToggleZoom:
      zxgame_set_scale(zxgame, zxgame->scale.prev);
      break;

    case ResetZoom:
      zxgame_set_scale(zxgame, 100);
      break;

    case Close:
      zxgame->flags |= zxgame_FLAG_QUIT;
      break;
  }
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

  if (should_quit(zxgame))
  {
    /* Note: Deregistering event handlers within an event handler is ok. */
    zxgame_destroy(zxgame);
    return event_HANDLED;
  }

  /* Paused flag set => Game is paused by the user.
   * Sleeping flag set => Game is idling until next run. */
  if (zxgame->flags & (zxgame_FLAG_PAUSED | zxgame_FLAG_SLEEPING))
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

  emit_sound(zxgame);

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
    int window_w, window_h;

    /* Calculate the window's visible width and height (OS units). */
    window_w = open->visible.x1 - open->visible.x0;
    window_h = open->visible.y1 - open->visible.y0;

    if (window_w != zxgame->window_w || window_h != zxgame->window_h)
    {
      zxgame->window_w = window_w;
      zxgame->window_h = window_h;
      zxgame->xscroll  = open->xscroll;
      zxgame->yscroll  = open->yscroll;

      zxgame_update(zxgame, zxgame_UPDATE_WINDOW | zxgame_UPDATE_REDRAW);
    }

    /* Inhibit scrolling. */
    open->xscroll = open->yscroll = 0;
  }

  wimp_open_window(open);

  return event_HANDLED;
}

static int zxgame_event_close_window_request(wimp_event_no event_no,
                                             wimp_block   *block,
                                             void         *handle)
{
  zxgame_t *zxgame;

  NOT_USED(event_no);
  NOT_USED(block);

  zxgame = handle;

  zxgame->flags |= zxgame_FLAG_QUIT;

  return event_HANDLED;
}

static void tick(wimp_menu *m, int entry, int ticked)
{
  menu_set_menu_flags(m,
                      entry,
                      ticked ? wimp_MENU_TICKED : 0,
                      wimp_MENU_TICKED);
}

static void shade(wimp_menu *m, int entry, int shaded)
{
  menu_set_icon_flags(m,
                      entry,
                      shaded ? wimp_ICON_SHADED : 0,
                      wimp_ICON_SHADED);
}

static void zxgame_menu_update(void)
{
  zxgame_t  *zxgame;
  wimp_menu *m;

  zxgame = GLOBALS.current_zxgame;
  if (zxgame == NULL)
    return;

  /* "View" menu */

  m = GLOBALS.zxgame_m->entries[ZXGAME_VIEW].sub_menu;

  tick(m, VIEW_FIXED,      (zxgame->flags & zxgame_FLAG_FIT)        == 0);
  tick(m, VIEW_SCALED,     (zxgame->flags & zxgame_FLAG_FIT)        != 0);
  tick(m, VIEW_MONOCHROME, (zxgame->flags & zxgame_FLAG_MONOCHROME) != 0);

  /* "Fixed scale" menu */

  m = GLOBALS.zxgame_m->entries[ZXGAME_VIEW].sub_menu;
  m = m->entries[VIEW_FIXED].sub_menu;

  tick(m, FIXED_SELECTED,   (zxgame->flags & zxgame_FLAG_FIT)        == 0);
  tick(m, FIXED_BIG_WINDOW, (zxgame->flags & zxgame_FLAG_BIG_WINDOW) != 0);

  /* "Scaled to fit" menu */

  m = GLOBALS.zxgame_m->entries[ZXGAME_VIEW].sub_menu;
  m = m->entries[VIEW_SCALED].sub_menu;

  tick(m, SCALED_SELECTED, (zxgame->flags & zxgame_FLAG_FIT)  != 0);
  tick(m, SCALED_SNAP,     (zxgame->flags & zxgame_FLAG_SNAP) != 0);

  /* "Sound" menu */

  m = GLOBALS.zxgame_m->entries[ZXGAME_SOUND].sub_menu;

  shade(m, SOUND_ENABLED, (zxgame->flags & zxgame_FLAG_HAVE_SOUND) == 0);
  tick(m, SOUND_ENABLED, (zxgame->flags & zxgame_FLAG_SOUND_ON) != 0);

  /* "Speed" menu */

  m = GLOBALS.zxgame_m->entries[ZXGAME_SPEED].sub_menu;

  tick(m, SPEED_100PC,   (zxgame->speed == NORMSPEED));
  tick(m, SPEED_MAXIMUM, (zxgame->speed == MAXSPEED));
  tick(m, SPEED_PAUSE,   (zxgame->flags & zxgame_FLAG_PAUSED) != 0);
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
    wimp_set_caret_position((pointer->buttons == wimp_CLICK_SELECT) ? pointer->w : wimp_BACKGROUND,
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
    /* Menu actions */

    case 'F' - 64: /* Fixed scale > Selected */
      action(SelectFixedScale);
      break;
    case wimp_KEY_F11: /* Fixed scale > Scale view */
      action(OpenScaleViewDialogue);
      break;
    case 'G' - 64: /* Fixed scale > Big window */
      action(ToggleBigWindow);
      break;

    case 'V' - 64: /* Scaled to fit > Selected */
      action(SelectScaledToFit);
      break;
    case 'S' - 64: /* Scaled to fit > Snap to pixels */
      action(ToggleSnapToPixels);
      break;

    case wimp_KEY_F10: /* View > Full screen */
      action(FullScreen);
      break;
    case 'N' - 64: /* View > Monochrome */
      action(ToggleMonochrome);
      break;

    case wimp_KEY_F3: /* Save > Save */
      action(OpenSaveGame);
      break;
    case wimp_KEY_SHIFT | wimp_KEY_F3: /* Save > Screenshot */
      action(OpenSaveScreenshot);
      break;

    case 'O' - 64: /* Sound > Enabled */
      action(ToggleSound);
      break;

    case wimp_KEY_F5: /* Speed > Slower */
      action(Slower);
      break;
    case wimp_KEY_F6: /* Speed > 100% */
      action(SpeedNormal);
      break;
    case wimp_KEY_F7: /* Speed > Faster */
      action(Faster);
      break;
    case wimp_KEY_SHIFT | wimp_KEY_F7: /* Speed > Maximum */
      action(SpeedMax);
      break;
    case 'P' - 64: /* Speed > Pause */
      action(TogglePause);
      break;

    /* Non-menu actions */

    case 'Q' - 64: /* Zoom out */
      action(ZoomOut);
      break;
    case 'W' - 64: /* Zoom in */
      action(ZoomIn);
      break;
    case 'T' - 64: /* Toggle zoom */
      action(ToggleZoom);
      break;
    case 'D' - 64: /* Reset zoom */
      action(ResetZoom);
      break;

    case wimp_KEY_CONTROL | wimp_KEY_F2:
      action(Close);
      break;

    /* Others */

    default:
      if (isalnum(key->c)           ||
          key->c == wimp_KEY_RETURN ||
          key->c == ' '             ||
          key->c == wimp_KEY_UP     ||
          key->c == wimp_KEY_DOWN   ||
          key->c == wimp_KEY_LEFT   ||
          key->c == wimp_KEY_RIGHT)
      {
        /* Consume any key presses that the game would normally accept. */
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
#define PACK(a,b,c) ((((a+1) & 0xFF) << 16) | (((b+1) & 0xFF) << 8) | (((c+1) & 0xFF) << 0))

  static const struct
  {
    unsigned int items;
    action_t     action;
  }
  map[] =
  {
    { PACK(ZXGAME_VIEW,  VIEW_FIXED,       -1),               SelectFixedScale },
    { PACK(ZXGAME_VIEW,  VIEW_FIXED,       FIXED_SELECTED),   SelectFixedScale },
    { PACK(ZXGAME_VIEW,  VIEW_FIXED,       FIXED_BIG_WINDOW), ToggleBigWindow  },
    { PACK(ZXGAME_VIEW,  VIEW_SCALED,      -1),               SelectScaledToFit },
    { PACK(ZXGAME_VIEW,  VIEW_SCALED,      SCALED_SELECTED),  SelectScaledToFit },
    { PACK(ZXGAME_VIEW,  VIEW_SCALED,      SCALED_SNAP),      ToggleSnapToPixels },
    { PACK(ZXGAME_VIEW,  VIEW_FULL_SCREEN, -1),               FullScreen },
    { PACK(ZXGAME_VIEW,  VIEW_MONOCHROME,  -1),               ToggleMonochrome },
    { PACK(ZXGAME_SOUND, SOUND_ENABLED,    -1),               ToggleSound },
    { PACK(ZXGAME_SPEED, SPEED_100PC,      -1),               SpeedNormal },
    { PACK(ZXGAME_SPEED, SPEED_MAXIMUM,    -1),               SpeedMax },
    { PACK(ZXGAME_SPEED, SPEED_FASTER,     -1),               Faster },
    { PACK(ZXGAME_SPEED, SPEED_SLOWER,     -1),               Slower },
    { PACK(ZXGAME_SPEED, SPEED_PAUSE,      -1),               TogglePause },
  };

  const size_t stride = sizeof(map[0]);
  const size_t nelems = sizeof(map) / stride;

  wimp_selection *selection;
  wimp_menu      *last;
  wimp_pointer    p;
  unsigned int    item;
  int             i;

  NOT_USED(event_no);
  NOT_USED(handle);

  selection = &block->selection;

  /* We will receive this event on *any* menu selection. It's essential to
   * reject events not intended for us. */
  last = menu_last();
  if (last != GLOBALS.zxgame_m)
    return event_NOT_HANDLED;

  item = PACK(selection->items[0], selection->items[1], selection->items[2]);

  i = bsearch_uint(&map[0].items, nelems, stride, item);
  if (i >= 0)
    action(map[i].action);

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
  osspriteop_id           id;
  colourtrans_table_flags flags;
  int                     size;

  free(zxgame->trans_tab);

  id = (osspriteop_id) sprite_select(zxgame->sprite, 0);

  flags = colourtrans_GIVEN_SPRITE;
  if (zxgame->flags & zxgame_FLAG_WIDE_CTRANS)
    flags |= colourtrans_RETURN_WIDE_ENTRIES;

  size = colourtrans_generate_table_for_sprite(zxgame->sprite,
                                               id,
                                               os_CURRENT_MODE,
                                               colourtrans_CURRENT_PALETTE,
                                               NULL, /* return size */
                                               flags,
                                               NULL,
                                               NULL);

  zxgame->trans_tab = malloc(size);
  if (zxgame->trans_tab == NULL)
    return error_OOM;

  colourtrans_generate_table_for_sprite(zxgame->sprite,
                                        id,
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


  // put if (scaling/extent/window) ... around this big block

  {
    const int image_xeig = GAMEEIG, image_yeig = GAMEEIG;
    const int border = zxgame->border_size; /* pixels */

    scale_t   scale;
    int       screen_xeig, screen_yeig;
    int       extent_w, extent_h;

    scale = zxgame->scale.cur * SCALE_1 / 100; /* % -> scale_t */

    read_current_mode_vars(&screen_xeig, &screen_yeig, NULL);

    if ((zxgame->flags & zxgame_FLAG_FIT) == 0)
    {
      /* "Fixed scale" mode. */

      if (flags & zxgame_UPDATE_EXTENT)
      {
        int min_w, min_h;
        int game_w, game_h;
        int minsize;

        /* Size the window. If a big window is configured then use it as the
         * _minimum_ size of the window. */

        if (zxgame->flags & zxgame_FLAG_BIG_WINDOW)
          /* Not ideally named: reads max workarea size given current screen */
          read_max_visible_area(zxgame->w, &min_w, &min_h);
        else
          min_w = min_h = 0;

        /* Calculate dimensions of scaled game+border. */
        game_w = ((GAMEWIDTH  + border * 2) << image_xeig) * scale / SCALE_1;
        game_h = ((GAMEHEIGHT + border * 2) << image_yeig) * scale / SCALE_1;

        extent_w = MAX(min_w, game_w);
        extent_h = MAX(min_h, game_h);
        snap2px(&extent_w, &extent_h);

        minsize = window_set_extent2(zxgame->w, 0, -extent_h, extent_w, 0);
        // TODO: If we hit minsize then read the size we minned out at.

        /* Save the extent. */
        zxgame->extent.x0 = 0;
        zxgame->extent.y0 = -extent_h;
        zxgame->extent.x1 = extent_w;
        zxgame->extent.y1 = 0;
      }

      if (flags & zxgame_UPDATE_SCALING)
      {
        scale_t scaled_w, scaled_h;
        int     left_x, bottom_y;

        /* Calculate dimensions of scaled game only. */
        scaled_w = (GAMEWIDTH  << image_xeig) * scale / SCALE_1;
        scaled_h = (GAMEHEIGHT << image_yeig) * scale / SCALE_1;
        snap2px(&scaled_w, &scaled_h);

        /* Centre the box in the work area. */
        extent_w = zxgame->extent.x1 - zxgame->extent.x0;
        extent_h = zxgame->extent.y1 - zxgame->extent.y0;
        left_x   = zxgame->extent.x0 + (extent_w - scaled_w) / 2;
        bottom_y = zxgame->extent.y0 + (extent_h - scaled_h) / 2;
        snap2px(&left_x, &bottom_y);

        zxgame->imgbox.x0 = left_x;
        zxgame->imgbox.y0 = bottom_y;
        zxgame->imgbox.x1 = left_x   + scaled_w;
        zxgame->imgbox.y1 = bottom_y + scaled_h;
        snapbox2px(&zxgame->imgbox);

        /* Update sprite scaling. */
        os_factors_from_ratio(&zxgame->factors, scale, SCALE_1);
        zxgame->factors.xmul <<= image_xeig;
        zxgame->factors.ymul <<= image_yeig;
        zxgame->factors.xdiv <<= screen_xeig;
        zxgame->factors.ydiv <<= screen_yeig;
      }
    }
    else
    {
      /* "Scaled to fit" mode. */

      if (flags & zxgame_UPDATE_EXTENT)
      {
        int minsize;

        /* Window is to be sized to screen. */
        read_max_visible_area(zxgame->w, &extent_w, &extent_h);

        minsize = window_set_extent2(zxgame->w, 0, -extent_h, extent_w, 0);
        // TODO: If we hit minsize then read the size we minned out at.

        /* Save the extent. */
        zxgame->extent.x0 = 0;
        zxgame->extent.y0 = -extent_h;
        zxgame->extent.x1 = extent_w;
        zxgame->extent.y1 = 0;
      }

      if (flags & zxgame_UPDATE_WINDOW)
      {
        int     reduced_border_x, reduced_border_y;
        int     games_per_window;
        scale_t scaled_w, scaled_h;
        int     left_x, bottom_y;

        /* How many 1:1 games fit comfortably in the window (at this size)? */
        reduced_border_x = reduced_border_y = zxgame->border_size << GAMEEIG; /* pixels -> OS units */
        do
        {
          fix16_t game_widths_per_window, game_heights_per_window;

          game_widths_per_window  = (zxgame->window_w - reduced_border_x * 2) * FIX16_1 / (GAMEWIDTH  << GAMEEIG);
          game_heights_per_window = (zxgame->window_h - reduced_border_y * 2) * FIX16_1 / (GAMEHEIGHT << GAMEEIG);
          games_per_window = MIN(game_widths_per_window, game_heights_per_window);
          if (reduced_border_x >= screen_xeig) reduced_border_x -= screen_xeig;
          if (reduced_border_y >= screen_yeig) reduced_border_y -= screen_yeig;
        }
        /* Loop while there's borders to reduce and we're still struggling to
         * fit a whole game in the window. */
        while (games_per_window < FIX16_1 && (reduced_border_x > 0 || reduced_border_y > 0));

        /* Snap the game scale to whole units. */
        if (games_per_window >= FIX16_1 && (zxgame->flags & zxgame_FLAG_SNAP))
          scale = (games_per_window >> 16) * SCALE_1;
        else
          scale = (games_per_window * SCALE_1) >> 16;
        zxgame->scale.cur = scale * 100 / SCALE_1; /* scale_t -> % */
        zxgame->scale.cur = CLAMP(zxgame->scale.cur, 1, 8000);

        /* Calculate dimensions of scaled game only. */
        scaled_w = (GAMEWIDTH  << image_xeig) * scale / SCALE_1;
        scaled_h = (GAMEHEIGHT << image_yeig) * scale / SCALE_1;
        snap2px(&scaled_w, &scaled_h);

        /* Centre the box in the visible area. */
        left_x   = zxgame->xscroll + (zxgame->window_w - scaled_w) / 2;
        bottom_y = zxgame->yscroll + (zxgame->window_h - scaled_h) / 2;
        bottom_y -= zxgame->window_h;
        snap2px(&left_x, &bottom_y);

        zxgame->imgbox.x0 = left_x;
        zxgame->imgbox.y0 = bottom_y;
        zxgame->imgbox.x1 = left_x   + scaled_w;
        zxgame->imgbox.y1 = bottom_y + scaled_h;
        snapbox2px(&zxgame->imgbox);

        /* Update sprite scaling. */
        os_factors_from_ratio(&zxgame->factors, scale, SCALE_1);
        zxgame->factors.xmul <<= image_xeig;
        zxgame->factors.ymul <<= image_yeig;
        zxgame->factors.xdiv <<= screen_xeig;
        zxgame->factors.ydiv <<= screen_yeig;
      }
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

error zxgame_create(zxgame_t **new_zxgame, const char *startup_game)
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

  zxgame->flags = zxgame_FLAG_FIRST | zxgame_FLAG_MENU | zxgame_FLAG_SOUND_ON;

  if (GLOBALS.flags & Flag_HaveWideColourTrans)
    zxgame->flags |= zxgame_FLAG_WIDE_CTRANS;

  if (GLOBALS.flags & Flag_HaveSharedSoundBuffer)
    zxgame->flags |= zxgame_FLAG_HAVE_SOUND;

  zxgame->w = window_clone(GLOBALS.zxgame_w);
  if (zxgame->w == NULL)
    goto NoMem;

  zxgame->scale.cur = 100;
  zxgame->scale.prev = 50;
  zxgame->speed = NORMSPEED;
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

  zxgame->background.colour = os_COLOUR_BLUE;

  // =====

  zxconfig.opaque = zxgame;

  zxgame->zx = zxspectrum_create(&zxconfig);
  if (zxgame->zx == NULL)
    goto Failure;

  zxgame->tge = tge_create(zxgame->zx);
  if (zxgame->tge == NULL)
    goto Failure;

  tge_setup(zxgame->tge);

  set_handlers(zxgame);

  if (startup_game)
  {
    tge_setup2(zxgame->tge);
    zxgame->flags &= ~zxgame_FLAG_MENU;
    zxgame_load_game(zxgame, startup_game); // fixme errs
  }

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

  teardown_sound(zxgame);
  zxgame_remove(zxgame);

  /* Delete the window */
  window_delete_cloned(zxgame->w);

  release_handlers(zxgame);
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

  zxgame->flags &= ~zxgame_FLAG_FIT;

  zxgame->scale.prev = zxgame->scale.cur;
  zxgame->scale.cur = scale;

  zxgame_update(zxgame, zxgame_UPDATE_SCALING | zxgame_UPDATE_EXTENT | zxgame_UPDATE_REDRAW);
}

/* ----------------------------------------------------------------------- */

void zxgame_open(zxgame_t *zxgame)
{
  window_open_at(zxgame->w, AT_CENTRE);
}

/* ----------------------------------------------------------------------- */

error zxgame_load_game(zxgame_t *zxgame, const char *file_name)
{
  os_error *err;
  char     *errormsg;

  xhourglass_on();

  tge_load(zxgame->tge, file_name, &errormsg); // handle errors
  tge_disposeoferror(errormsg);

  xhourglass_off();

  return error_OK;
}

error zxgame_save_game(zxgame_t *zxgame, const char *file_name)
{
  os_error *err;

  xhourglass_on();

  tge_save(zxgame->tge, file_name); // handle errors
  osfile_set_type(file_name, APPFILETYPE);

  xhourglass_off();

  return error_OK;
}

error zxgame_save_screenshot(zxgame_t *zxgame, const char *file_name)
{
  os_error *err;

  err = xosspriteop_save_sprite_file(osspriteop_PTR,
                                     zxgame->sprite,
                                     file_name);
  if (err)
    return error_OS;

  return error_OK;
}

/* ----------------------------------------------------------------------- */

error zxgame_init(void)
{
  error err;

  /* Dependencies */

  err = help_init();
  if (err)
    return err;

  /* Handlers */

  register_single_handlers(1);

  GLOBALS.zxgame_w = window_create("zxgame");

  /* Internal dependencies */

  err = zxgamesave_dlg_init();
  if (!err)
    err = zxgamescale_dlg_init();
  if (err)
    return err;

  /* Menu */

  GLOBALS.zxgame_m = menu_create_from_desc(message0("menu.zxgame"),
                                           dialogue_get_window(zxgamescale_dlg),
                                           dialogue_get_window(zxgamesave_dlg),
                                           dialogue_get_window(zxgamesave_dlg));

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
  zxgamesave_dlg_fin();

  register_single_handlers(0);

  help_fin();
}

/* ----------------------------------------------------------------------- */

// vim: ts=8 sts=2 sw=2 et
