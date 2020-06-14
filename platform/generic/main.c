/* main.c
 *
 * Headless front-end for The Great Escape.
 *
 * This does nothing more than run the game a fast as possible for a specified
 * number of iterations with no display or sound output.
 *
 * (c) David Thomas, 2017-2020.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "ZXSpectrum/Spectrum.h"
#include "ZXSpectrum/Keyboard.h"

#include "TheGreatEscape/TheGreatEscape.h"

// -----------------------------------------------------------------------------

// Configuration
//
#define GAMEWIDTH     256
#define GAMEHEIGHT    192

#define MAXITERS      100000

// -----------------------------------------------------------------------------

static int keystroke_time;

// -----------------------------------------------------------------------------

static void draw_handler(const zxbox_t *dirty,
                         void          *opaque)
{
}

static void stamp_handler(void *opaque)
{
}

static int sleep_handler(int durationTStates, void *opaque)
{
  // return immediately: run the game as fast as possible
  return 0;
}

static int key_handler(uint16_t port, void *opaque)
{
  if (port == port_KEMPSTON_JOYSTICK)
    return 0; // active high (zeroes by default)

  keystroke_time++;

  // first send a '2' to select Kempston joystick mode
  if (keystroke_time < 3 && port == port_KEYBOARD_12345)
    return 0x1F ^ 2;

  // then send a '0' to start the game
  if (keystroke_time < 6 && port == port_KEYBOARD_09876)
    return 0x1F ^ 1;

  return 0x1F;
}

static void border_handler(int colour, void *opaque)
{
}

static void speaker_handler(int on_off, void *opaque)
{
  // dump the speaker pulses in binary
  // putc('0' + on_off, stderr);
}

// -----------------------------------------------------------------------------

static int get_ms(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main(void)
{
  static const zxconfig_t zxconfig =
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
  zxspectrum_t *zx;
  tgestate_t *game;
  int quit = 0;
  int iters;
  int start, end;

  printf("THE GREAT ESCAPE\n");
  printf("================\n");

  printf("Initialising...\n");

  zx = zxspectrum_create(&zxconfig);
  if (zx == NULL)
    goto failure;

  game = tge_create(zx);
  if (game == NULL)
    goto failure;

  printf("Running setup 1...\n");
  tge_setup(game);

  printf("Running menu...\n");
  iters = 0;
  while (!quit)
  {
    if (tge_menu(game) > 0)
      break;
    iters++;
  }
  printf("(ran %d iterations)\n", iters);

  if (!quit)
  {
    printf("Running setup 2...\n");
    tge_setup2(game);

    printf("Running game...\n");
    iters = 0;
    start = get_ms();
    while (!quit)
    {
      tge_main(game);
      iters++;
      if (iters >= MAXITERS)
        break;

      if ((iters % 1000) == 0)
      {
        printf("+");
        fflush(stdout);
      }
    }
    end = get_ms();
    printf("\n");

    printf("%d iterations in %dms = %.2fiters/sec\n",
        iters,
        end - start,
        (double) iters / ((end - start) / 1000.0));
  }

  tge_destroy(game);
  zxspectrum_destroy(zx);

  printf("(quit)\n");

  exit(EXIT_SUCCESS);


failure:

  exit(EXIT_FAILURE);
}

// vim: ts=8 sts=2 sw=2 et
