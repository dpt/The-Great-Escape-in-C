/* sdlmain.c
 *
 * SDL front-end for The Great Escape.
 *
 * (c) David Thomas, 2017.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <SDL2/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "ZXSpectrum/Spectrum.h"
#include "ZXSpectrum/Keyboard.h"
#include "ZXSpectrum/Kempston.h"

#include "TheGreatEscape/TheGreatEscape.h"

// -----------------------------------------------------------------------------

// Configuration
//
#define FPS           10
#define GAMEWIDTH     256
#define GAMEHEIGHT    192
#define BORDER        32
#define WINDOWWIDTH   (GAMEWIDTH  + 2 * BORDER)
#define WINDOWHEIGHT  (GAMEHEIGHT + 2 * BORDER)

// -----------------------------------------------------------------------------

typedef struct
{
  zxspectrum_t *zx;
  tgestate_t   *game;

  zxkeyset_t    keys;
  zxkempston_t  kempston;

  int           paused; // bool

  int           quit; // bool

  SDL_Renderer *renderer;
  SDL_Texture  *texture;
  int           menu; // bool

  int           sleep_us; // us to sleep for on the next loop
}
state_t;

// -----------------------------------------------------------------------------

static void draw_handler(const zxbox_t *dirty,
                         void          *opaque)
{
  state_t  *state = opaque;
  uint32_t *pixels;

  pixels = zxspectrum_claim_screen(state->zx);
  SDL_UpdateTexture(state->texture, NULL, pixels, GAMEWIDTH * 4);
  zxspectrum_release_screen(state->zx);
}

static void stamp_handler(void *opaque)
{
  state_t *state = opaque;

  // TODO: Save timestamps.
}

static void sleep_handler(int durationTStates, void *opaque)
{
  state_t *state = opaque;

  // TODO: Sleep.
  //
  //usleep(durationTStates * 1000000 / 3500000);
  //emscripten_sleep(durationTStates * 1000000 / 3500000);
  //
  // Note that we can't sleep here: in this single-threaded model it would
  // stall the UI.  Instead we could store the timing details here and apply
  // them in the main_loop. Though that would still create lumpy effects due
  // to the way the game does not currently yield to its caller during
  // periods when it wants to sleep.
}

static int key_handler(uint16_t port, void *opaque)
{
  state_t *state = opaque;

  if (port == port_KEMPSTON_JOYSTICK)
    return state->kempston;
  else
    return zxkeyset_for_port(port, state->keys);
}

static void border_handler(int colour, void *opaque)
{
  state_t *state = opaque;

  // TODO: Set border colour.
}

static void speaker_handler(int on_off, void *opaque)
{
  state_t *state = opaque;

  // TODO: All sound.
}

// -----------------------------------------------------------------------------

void PrintEvent(const SDL_Event * event)
{
  if (event->type == SDL_WINDOWEVENT) {
    switch (event->window.event) {
      case SDL_WINDOWEVENT_SHOWN:
        SDL_Log("Window %d shown", event->window.windowID);
        break;
      case SDL_WINDOWEVENT_HIDDEN:
        SDL_Log("Window %d hidden", event->window.windowID);
        break;
      case SDL_WINDOWEVENT_EXPOSED:
        SDL_Log("Window %d exposed", event->window.windowID);
        break;
      case SDL_WINDOWEVENT_MOVED:
        SDL_Log("Window %d moved to %d,%d",
            event->window.windowID, event->window.data1,
            event->window.data2);
        break;
      case SDL_WINDOWEVENT_RESIZED:
        SDL_Log("Window %d resized to %dx%d",
            event->window.windowID, event->window.data1,
            event->window.data2);
        break;
      case SDL_WINDOWEVENT_SIZE_CHANGED:
        SDL_Log("Window %d size changed to %dx%d",
            event->window.windowID, event->window.data1,
            event->window.data2);
        break;
      case SDL_WINDOWEVENT_MINIMIZED:
        SDL_Log("Window %d minimized", event->window.windowID);
        break;
      case SDL_WINDOWEVENT_MAXIMIZED:
        SDL_Log("Window %d maximized", event->window.windowID);
        break;
      case SDL_WINDOWEVENT_RESTORED:
        SDL_Log("Window %d restored", event->window.windowID);
        break;
      case SDL_WINDOWEVENT_ENTER:
        SDL_Log("Mouse entered window %d",
            event->window.windowID);
        break;
      case SDL_WINDOWEVENT_LEAVE:
        SDL_Log("Mouse left window %d", event->window.windowID);
        break;
      case SDL_WINDOWEVENT_FOCUS_GAINED:
        SDL_Log("Window %d gained keyboard focus",
            event->window.windowID);
        break;
      case SDL_WINDOWEVENT_FOCUS_LOST:
        SDL_Log("Window %d lost keyboard focus",
            event->window.windowID);
        break;
      case SDL_WINDOWEVENT_CLOSE:
        SDL_Log("Window %d closed", event->window.windowID);
        break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
      case SDL_WINDOWEVENT_TAKE_FOCUS:
        SDL_Log("Window %d is offered a focus", event->window.windowID);
        break;
      case SDL_WINDOWEVENT_HIT_TEST:
        SDL_Log("Window %d has a special hit test", event->window.windowID);
        break;
#endif
      default:
        SDL_Log("Window %d got unknown event %d",
            event->window.windowID, event->window.event);
        break;
    }
  }
}

static void sdl_key_pressed(state_t *state, const SDL_KeyboardEvent *k)
{
  SDL_Keycode  sym;
  int          down;
  zxjoystick_t j;

  sym = k->keysym.sym;
  switch (sym)
  {
    case SDLK_LEFT:  j = zxjoystick_LEFT;    break;
    case SDLK_RIGHT: j = zxjoystick_RIGHT;   break;
    case SDLK_UP:    j = zxjoystick_UP;      break;
    case SDLK_DOWN:  j = zxjoystick_DOWN;    break;
    case '.':        j = zxjoystick_FIRE;    break;
    default:         j = zxjoystick_UNKNOWN; break;
  }

  down = (k->type == SDL_KEYDOWN);

  if (j != zxjoystick_UNKNOWN)
  {
    zxkempston_assign(&state->kempston, j, down);
  }
  else
  {
    zxkeyset_t *keys;

    keys = &state->keys;
    if (down)
      *keys = zxkeyset_setchar(*keys, k->keysym.sym);
    else
      *keys = zxkeyset_clearchar(*keys, k->keysym.sym);
  }
}

// type: em_arg_callback_func
static void main_loop(void *opaque)
{
  static const SDL_Rect dstrect = { BORDER, BORDER, GAMEWIDTH, GAMEHEIGHT };

  state_t *state = opaque;

  {
    SDL_Event event;

    // Consume all pending events
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_QUIT:
          state->quit = 1;
          SDL_Log("Quitting after %i ticks", event.quit.timestamp);
          break;

        case SDL_WINDOWEVENT:
          PrintEvent(&event);
          break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
          sdl_key_pressed(state, &event.key);
          break;

        case SDL_TEXTEDITING:
        case SDL_TEXTINPUT:
          break;

        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL:
          break;

        default:
          printf("Unhandled event code {%d}\n", event.type);
          break;
      }
    }

    if (state->quit)
      return;

    if (state->menu)
    {
      if (tge_menu(state->game) > 0)
      {
        tge_setup2(state->game);
        state->menu = 0;
      }
    }
    else
    {
      tge_main(state->game);
    }

    /* Update the texture and render it */

    /* Clear screen */
    // TODO: This ought to be the border colour, but TGE's is always black.
    SDL_SetRenderDrawColor(state->renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(state->renderer);

    /* Offset the image */
    // Note that this will inhibit image stretching.

    SDL_RenderCopy(state->renderer, state->texture, NULL, &dstrect);
    SDL_RenderPresent(state->renderer);

    SDL_Delay(1000 / FPS);
  }
}

int main(void)
{
  state_t         state;
  zxconfig_t      zxconfig =
  {
    &state, /* opaque */
    &draw_handler,
    &stamp_handler,
    &sleep_handler,
    &key_handler,
    &border_handler,
    &speaker_handler
  };
  tgeconfig_t     config;
  SDL_Window     *window;

  printf("THE GREAT ESCAPE\n");
  printf("================\n");

  printf("Initialising...\n");

  config.width  = GAMEWIDTH  / 8;
  config.height = GAMEHEIGHT / 8;

  state.keys      = 0ULL;
  state.kempston  = 0;
  state.paused    = 0;
  state.quit      = 0;
  state.menu      = 1;

  state.zx = zxspectrum_create(&zxconfig);
  if (state.zx == NULL)
    goto failure;

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    fprintf(stderr, "Error: SDL_Init: %s\n", SDL_GetError());
    goto failure;
  }

  window = SDL_CreateWindow("The Great Escape",
                            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            WINDOWWIDTH, WINDOWHEIGHT,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (window == NULL)
  {
    fprintf(stderr, "Error: SDL_CreateWindow: %s\n", SDL_GetError());
    goto failure;
  }

  state.renderer = SDL_CreateRenderer(window,
                                      -1,
                                      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (state.renderer == NULL)
  {
    fprintf(stderr, "Error: SDL_CreateRenderer: %s\n", SDL_GetError());
    goto failure;
  }

  // native_format = SDL_GetWindowPixelFormat(window);

  state.texture = SDL_CreateTexture(state.renderer,
                                    SDL_PIXELFORMAT_ARGB8888, // fastest?
                                    SDL_TEXTUREACCESS_STREAMING,
                                    GAMEWIDTH, GAMEHEIGHT);
  if (state.texture == NULL)
  {
    fprintf(stderr, "Error: SDL_CreateTexture: %s\n", SDL_GetError());
    goto failure;
  }

  if (SDL_SetTextureBlendMode(state.texture, SDL_BLENDMODE_NONE) < 0)
  {
    fprintf(stderr, "Error: SDL_SetTextureBlendMode: %s\n", SDL_GetError());
    goto failure;
  }

  state.game = tge_create(state.zx, &config);
  if (state.game == NULL)
    goto failure;

  tge_setup(state.game);

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(main_loop, &state, FPS, 1 /* infinite loop */);
#else
  while (!state.quit)
    main_loop(&state);
#endif

  tge_destroy(state.game);
  zxspectrum_destroy(state.zx);

  SDL_DestroyTexture(state.texture);
  SDL_DestroyRenderer(state.renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();

  printf("(quit)\n");

  exit(EXIT_SUCCESS);


failure:

  exit(EXIT_FAILURE);
}

// vim: ts=8 sts=2 sw=2 et
