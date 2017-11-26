/* main.c
 *
 * Minimal front-end for The Great Escape.
 *
 * (c) David Thomas, 2017.
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
#define DEFAULTWIDTH      256
#define DEFAULTHEIGHT     192

#define MAXITERS          100000

// -----------------------------------------------------------------------------

static int keystroke_time;

// -----------------------------------------------------------------------------

static void draw_handler(unsigned int  *pixels,
                         const zxbox_t *dirty,
                         void          *opaque)
{
}

static void stamp_handler(void *opaque)
{
}

static void sleep_handler(int durationTStates, void *opaque)
{
    // return immediately: run the game as fast as possible
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
        NULL, /* opaque */
        &draw_handler,
        &stamp_handler,
        &sleep_handler,
        &key_handler,
        &border_handler,
        &speaker_handler
    };
    tgeconfig_t config;
    zxspectrum_t *zx;
    tgestate_t *game;
    int quit = 0;
    int iters;
    int start, end;

    printf("THE GREAT ESCAPE\n");
    printf("================\n");

    printf("Initialising...\n");

    config.width  = DEFAULTWIDTH  / 8;
    config.height = DEFAULTHEIGHT / 8;

    zx = zxspectrum_create(&zxconfig);
    if (zx == NULL)
        goto failure;

    game = tge_create(zx, &config);
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

    printf("(quit)\n");

    exit(EXIT_SUCCESS);


failure:

    exit(EXIT_FAILURE);
}
