#include <stdio.h>
#include <stdlib.h>

#include "ZXSpectrum/Spectrum.h"
#include "TheGreatEscape/TheGreatEscape.h"

// -----------------------------------------------------------------------------

// Configuration
//
#define DEFAULTWIDTH      256
#define DEFAULTHEIGHT     192

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
}

static int key_handler(uint16_t port, void *opaque)
{
    return 0;
}

static void border_handler(int colour, void *opaque)
{
}

static void speaker_handler(int on_off, void *opaque)
{
    // putc('0' + on_off, stderr);
}

// -----------------------------------------------------------------------------

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

    config.width  = DEFAULTWIDTH  / 8;
    config.height = DEFAULTHEIGHT / 8;

    zx = zxspectrum_create(&zxconfig);
    if (zx == NULL)
        goto failure;

    game = tge_create(zx, &config);
    if (game == NULL)
        goto failure;

    tge_setup(game);

    while (!quit)
        if (tge_menu(game) > 0)
            break;

    if (!quit)
    {
        tge_setup2(game);
        while (!quit)
            tge_main(game);
    }

    exit(EXIT_SUCCESS);


failure:

    exit(EXIT_FAILURE);
}
