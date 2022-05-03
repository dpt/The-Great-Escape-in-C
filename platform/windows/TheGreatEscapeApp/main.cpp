// main.cpp
//
// Windows front-end for The Great Escape
//
// Copyright (c) David Thomas, 2016-2022. <dave@davespace.co.uk>
//

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <list>
#include <vector>

#include <windows.h>
#include <tchar.h>
#include <CommCtrl.h>
#include <mmsystem.h>
#include <windowsx.h>

#include "ZXSpectrum/Spectrum.h"
#include "ZXSpectrum/Keyboard.h"
#include "ZXSpectrum/Kempston.h"

#include "TheGreatEscape/TheGreatEscape.h"

#include "resource.h"

#include "bitfifo.h"

#pragma comment(lib, "winmm.lib")

///////////////////////////////////////////////////////////////////////////////

// Configuration
//
#define GAMEWIDTH       (256)   // pixels
#define GAMEHEIGHT      (192)   // pixels
#define GAMEBORDER      (16)    // pixels

#define MAXSTAMPS       (4)     // max depth of timestamps stack
#define SPEEDQ          (20)    // smallest unit of speed (percent)
#define NORMSPEED       (100)   // normal speed (percent)
#define MAXSPEED        (99999) // fastest possible game (percent)

#define BUFSZ           (44100 / 50) // 1/50th sec at 44.1kHz

#define SAMPLE_RATE     (44100)
#define BITFIFO_LENGTH  (220500 / 4) // one seconds' worth of input bits (fifo will be ~27KiB)
#define AVG             (5)     // average this many input bits to make an output sample

///////////////////////////////////////////////////////////////////////////////

static const TCHAR szGameWindowClassName[] = _T("TheGreatEscapeWindowsApp");
static const TCHAR szGameWindowTitle[]     = _T("The Great Escape");

///////////////////////////////////////////////////////////////////////////////

typedef struct gamewin
{
  HINSTANCE       instance;
  HWND            window;
  BITMAPINFO      bitmapinfo;

  zxspectrum_t   *zx;
  tgestate_t     *tge;

  HANDLE          thread;
  DWORD           threadId;

  bool            snap;

  zxkeyset_t      keys;
  zxkempston_t    kempston;

  int             speed;  // percent
  BOOL            paused;

  bool            quit;

  ULONGLONG       stamps[MAXSTAMPS];
  int             nstamps;

  struct
  {
    CRITICAL_SECTION  soundLock;      // protects all following members
    HANDLE            soundDoneSema;  // signals that a sound has done playing
    HWAVEOUT          waveOut;        // device handle
    WAVEHDR           waveHdr;        // wave header
    UINT              bufferUsed;
    bitfifo_t        *fifo;
    signed short      lastvol;

    WORD              buffer[BUFSZ];  // 1/50th sec at 44.1kHz
  }
  sound;
}
gamewin_t;

///////////////////////////////////////////////////////////////////////////////

// https://msdn.microsoft.com/en-us/library/windows/desktop/dd797970(v=vs.85).aspx

static void CALLBACK SoundCallback(HWAVEOUT  hwo,
                                   UINT      uMsg,
                                   DWORD_PTR dwInstance,
                                   DWORD_PTR dwParam1,
                                   DWORD_PTR dwParam2);

static void EmitSound(gamewin_t *state);

static int OpenSound(gamewin_t *state)
{
  const WORD   nChannels      = 1; // mono
  const DWORD  nSamplesPerSec = SAMPLE_RATE;
  const WORD   wBitsPerSample = 16; // 8 or 16

  const WORD   nBlockAlign    = nChannels * wBitsPerSample / 8;

  WAVEFORMATEX waveFormatEx; // wave format specifier
  HWAVEOUT     waveOut;      // device handle

  // initialise in structure order
  waveFormatEx.wFormatTag      = WAVE_FORMAT_PCM;
  waveFormatEx.nChannels       = nChannels;
  waveFormatEx.nSamplesPerSec  = nSamplesPerSec;
  waveFormatEx.nAvgBytesPerSec = nSamplesPerSec * nBlockAlign;
  waveFormatEx.nBlockAlign     = nBlockAlign;
  waveFormatEx.wBitsPerSample  = wBitsPerSample;
  waveFormatEx.cbSize          = 0;

  // reset this: it's tested in CloseSound()
  state->sound.waveHdr.dwFlags = 0;

  state->sound.bufferUsed = 0;

  state->sound.fifo = bitfifo_create(BITFIFO_LENGTH);
  if (state->sound.fifo == NULL)
    goto failure; // OOM

  InitializeCriticalSection(&state->sound.soundLock);

  // initial counter = 1, max counter = 1
  state->sound.soundDoneSema = CreateSemaphore(NULL, 1, 1, NULL);

  if (waveOutOpen(&waveOut,
                  WAVE_MAPPER,
                  &waveFormatEx,
                  (DWORD_PTR) SoundCallback, // dwCallback
                  (DWORD_PTR) state, // dwCallbackInstance
                  CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
  {
    MessageBox(NULL, _T("unable to open WAVE_MAPPER device"), _T("Some Caption"), MB_TOPMOST);  // check
    return -1;
  }

  state->sound.waveOut = waveOut;

  assert(state->sound.fifo);

  return 0;

failure:
  return 1;
}

static void CloseSound(gamewin_t *state)
{
  MMRESULT result;

  result = waveOutReset(state->sound.waveOut);
  if (result)
  {
    return;
  }

  if (state->sound.waveHdr.dwFlags & WHDR_PREPARED)
  {
    result = waveOutUnprepareHeader(state->sound.waveOut, &state->sound.waveHdr, sizeof(WAVEHDR));
    if (result)
    {
      return;
    }
  }

  result = waveOutClose(state->sound.waveOut);
  if (result)
  {
    return;
  }

  CloseHandle(state->sound.soundDoneSema);

  DeleteCriticalSection(&state->sound.soundLock);

  bitfifo_destroy(state->sound.fifo);
}

static void EmitSound(gamewin_t *state)
{
  MMRESULT mmresult;

  // wait for the previous sound to clear
  WaitForSingleObject(state->sound.soundDoneSema, INFINITE);

  {
    result_t err;
    UINT     outputCapacity;
    UINT     outputUsed;

    outputCapacity = BUFSZ;
    outputUsed     = 0;

    // try to fill the buffer
    while (state->sound.bufferUsed < outputCapacity)
    {
      UINT   outputCapacityRemaining;
      size_t bitsInFifo;
      size_t toCopy;

      outputCapacityRemaining = outputCapacity - state->sound.bufferUsed;
      bitsInFifo              = bitfifo_used(state->sound.fifo);
      toCopy                  = min(bitsInFifo, outputCapacityRemaining);

      if (bitsInFifo == 0)
        goto no_data;

      const signed short maxVolume = 32500; // tweak this down to quieten

      signed short      *p;
      signed short       vol;
      UINT               j;

      p = (signed short *) &state->sound.buffer[outputUsed];
      for (j = 0; j < toCopy; j++)
      {
        unsigned int bits;

        bits = 0;
        err = bitfifo_dequeue(state->sound.fifo, &bits, AVG);
        if (err == result_BITFIFO_INSUFFICIENT ||
            err == result_BITFIFO_EMPTY)
          // When the fifo empties maintain the most recent volume
          vol = state->sound.lastvol;
        else
          vol = (int) __popcnt(bits) * maxVolume / AVG;
        *p++ = vol;

        state->sound.lastvol = vol;
      }

      outputUsed += toCopy;
    }

    return;

  no_data:
    return;
  }

  if (state->sound.waveHdr.dwFlags & WHDR_PREPARED)
  {
    mmresult = waveOutUnprepareHeader(state->sound.waveOut, &state->sound.waveHdr, sizeof(WAVEHDR));
    if (mmresult)
    {
      printf("blah 1\n");
    }
  }

  // prepare the waveform audio data block for playback
  state->sound.waveHdr.lpData = (LPSTR) &state->sound.buffer[0];
  state->sound.waveHdr.dwBufferLength = 2 * 1/*channels*/ * BUFSZ;
  state->sound.waveHdr.dwFlags = 0;
  state->sound.waveHdr.dwLoops = 0;
  mmresult = waveOutPrepareHeader(state->sound.waveOut, &state->sound.waveHdr, sizeof(WAVEHDR));
  if (mmresult)
  {
    printf("blah 2\n");
  }

  mmresult = waveOutWrite(state->sound.waveOut, &state->sound.waveHdr, sizeof(WAVEHDR));
  if (mmresult)
  {
    printf("blah 3\n");
  }
}

static void CALLBACK SoundCallback(HWAVEOUT  hwo,
                                   UINT      uMsg,
                                   DWORD_PTR dwInstance,
                                   DWORD_PTR dwParam1,
                                   DWORD_PTR dwParam2)
{
  gamewin_t *state = (gamewin_t *) dwInstance;

  if (uMsg == WOM_DONE)
  {
    // we can't do anything complex in the callback, so just release the semaphore
    EnterCriticalSection(&state->sound.soundLock);
    ReleaseSemaphore(state->sound.soundDoneSema, 1, NULL);
    LeaveCriticalSection(&state->sound.soundLock);
  }
}

///////////////////////////////////////////////////////////////////////////////

static void draw_handler(const zxbox_t *dirty,
                         void          *opaque)
{
  gamewin_t *gamewin = (gamewin_t *) opaque;

  // Invalidate the entire client area and don't erase the background
  InvalidateRect(gamewin->window, NULL, FALSE);
}

static void stamp_handler(void *opaque)
{
  gamewin_t *gamewin = (gamewin_t *) opaque;

  // Stack timestamps as they arrive
  assert(gamewin->nstamps < MAXSTAMPS);
  if (gamewin->nstamps >= MAXSTAMPS)
    return;
  gamewin->stamps[gamewin->nstamps++] = GetTickCount64();
}

static int sleep_handler(int durationTStates, void *opaque)
{
  gamewin_t *gamewin = (gamewin_t *) opaque;

  // Unstack timestamps (even if we're paused)
  assert(gamewin->nstamps > 0);
  if (gamewin->nstamps <= 0)
    return gamewin->quit;
  --gamewin->nstamps;

  // Quit straight away if signalled
  if (gamewin->quit)
    return TRUE;

  if (gamewin->paused)
  {
    // Check twice per second for unpausing
    // FIXME: Slow spinwait without any synchronisation
    while (gamewin->paused)
      Sleep(500); /* 0.5s */
  }
  else
  {
    // A Spectrum 48K has 69,888 T-states per frame and its Z80 runs at
    // 3.5MHz (~50Hz) for a total of 3,500,000 T-states per second.
    const double tstatesPerSec = 3.5e6;

    ULONGLONG now;
    double    duration; /* seconds */
    ULONGLONG then;
    ULONGLONG delta;
    double    consumed; /* seconds */

    now = GetTickCount64(); // get time now before anything else

    // 'duration' tells us how long the operation should take since the previous mark call.
    // Turn T-state duration into seconds
    duration = durationTStates / tstatesPerSec;
    // Adjust the game speed
    duration = duration * NORMSPEED / gamewin->speed;

    then = gamewin->stamps[gamewin->nstamps];

    delta = now - then;

    consumed = delta / 1e6;
    if (consumed < duration)
    {
      double delay; /* seconds */
      DWORD  udelay; /* milliseconds */

      // We didn't take enough time - sleep for the remainder of our duration
      delay = duration - consumed;
      udelay = (DWORD)(delay * 1e3);
      Sleep(udelay);
    }
  }

  return FALSE;
}

static int key_handler(uint16_t port, void *opaque)
{
  gamewin_t *gamewin = (gamewin_t *) opaque;
  int        k;

  if (port == port_KEMPSTON_JOYSTICK)
    k = gamewin->kempston;
  else
    k = zxkeyset_for_port(port, &gamewin->keys);

  return k;
}

static void border_handler(int colour, void *opaque)
{
  // does nothing presently
}

static void speaker_handler(int on_off, void *opaque)
{
  gamewin_t   *gamewin = (gamewin_t *) opaque;
  unsigned int bit = on_off;

  return;

  EnterCriticalSection(&gamewin->sound.soundLock);

  // There's nothing we can do if the buffer is full, so ignore errors
  (void) bitfifo_enqueue(gamewin->sound.fifo, &bit, 0, 1);

  LeaveCriticalSection(&gamewin->sound.soundLock);
}

///////////////////////////////////////////////////////////////////////////////

static DWORD WINAPI gamewin_thread(LPVOID lpParam)
{
  gamewin_t  *win  = (gamewin_t *) lpParam;
  tgestate_t *game = win->tge;

  tge_setup(game);

  // While in menu state
  while (!win->quit)
  {
    if (tge_menu(game) > 0)
      break; // game begins
    // EmitSound(win);
  }

  // While in game state
  if (!win->quit)
  {
    tge_setup2(game);
    while (!win->quit)
    {
      tge_main(game);
      win->nstamps = 0; // reset all timing info
     // EmitSound(win);
    }
  }

  return NULL;
}

static int CreateGame(gamewin_t *gamewin)
{
  zxconfig_t        zxconfig;
  zxspectrum_t     *zx;
  tgestate_t       *tge;
  HANDLE            thread;
  DWORD             threadId;
  BITMAPINFOHEADER *bmih;

  zxconfig.width  = GAMEWIDTH / 8;
  zxconfig.height = GAMEHEIGHT / 8;
  zxconfig.opaque = gamewin;
  zxconfig.draw   = draw_handler;
  zxconfig.stamp  = stamp_handler;
  zxconfig.sleep  = sleep_handler;
  zxconfig.key    = key_handler;
  zxconfig.border = border_handler;
  zxconfig.speaker = speaker_handler;

  zx = zxspectrum_create(&zxconfig);
  if (zx == NULL)
    goto failure;

  tge = tge_create(zx);
  if (tge == NULL)
    goto failure;

  //OpenSound(gamewin);

  thread = CreateThread(NULL,           // default security attributes
                        0,              // use default stack size
                        gamewin_thread, // thread function name
                        gamewin,        // argument to thread function
                        0,              // use default creation flags
                        &threadId);     // returns the thread identifier
  if (thread == NULL)
    goto failure;

  bmih = &gamewin->bitmapinfo.bmiHeader;
  bmih->biSize          = sizeof(BITMAPINFOHEADER);
  bmih->biWidth         = GAMEWIDTH;
  bmih->biHeight        = -GAMEHEIGHT; // negative height flips the image
  bmih->biPlanes        = 1;
  bmih->biBitCount      = 32;
  bmih->biCompression   = BI_RGB;
  bmih->biSizeImage     = 0; // set to zero for BI_RGB bitmaps
  bmih->biXPelsPerMeter = 0;
  bmih->biYPelsPerMeter = 0;
  bmih->biClrUsed       = 0;
  bmih->biClrImportant  = 0;

  gamewin->zx       = zx;
  gamewin->tge      = tge;

  gamewin->thread   = thread;
  gamewin->threadId = threadId;

  gamewin->snap     = true;

  zxkeyset_clear(&gamewin->keys);
  gamewin->kempston = 0;

  gamewin->speed    = NORMSPEED;
  gamewin->paused   = false;

  gamewin->quit     = false;

  gamewin->nstamps  = 0;

  return 0;


failure:
  return 1;
}

static void DestroyGame(gamewin_t *doomed)
{
  doomed->quit = true;

  WaitForSingleObject(doomed->thread, INFINITE);
  CloseHandle(doomed->thread);

  tge_destroy(doomed->tge);
  zxspectrum_destroy(doomed->zx);
}

///////////////////////////////////////////////////////////////////////////////

BOOL CreateGameWindow(HINSTANCE hInstance, int nCmdShow, gamewin_t **new_gamewin);
void DestroyGameWindow(gamewin_t *doomed);

///////////////////////////////////////////////////////////////////////////////

// naming gets confused here - what are games, game windows, etc.

static std::list<gamewin_t *> AllGameWindows;

static int CreateNewGame()
{
  gamewin_t *game;
  int        rc;

  rc = CreateGameWindow(GetModuleHandle(NULL), SW_NORMAL, &game);
  if (!rc)
    return 0;

  AllGameWindows.push_back(game);

  return rc;
}

static void DestroySingleGame(gamewin_t *doomed)
{
  DestroyGame(doomed);
  DestroyGameWindow(doomed);
}

static void DestroyAllGameWindows()
{
  for (auto it = AllGameWindows.begin(); it != AllGameWindows.end(); it++)
    DestroySingleGame(*it);
  AllGameWindows.clear();
}

static void DestroyGameWindowThenQuit(gamewin_t *doomed)
{
  DestroySingleGame(doomed);
  AllGameWindows.remove(doomed);

  // If no games remain then shut down the app
  // Check this is needed
  if (AllGameWindows.empty())
    PostQuitMessage(0);
}

///////////////////////////////////////////////////////////////////////////////

static bool FillOutVersionFields(HWND hwnd)
{
  TCHAR   executableFilename[MAX_PATH + 1];
  DWORD   dataSize;
  LPVOID  productName;
  UINT    productNameLen;
  LPVOID  productVersion;
  UINT    productVersionLen;

  if (GetModuleFileName(NULL, executableFilename, MAX_PATH) == 0)
  {
    return false;
  }

  dataSize = GetFileVersionInfoSize(executableFilename, NULL);
  if (dataSize == 0)
  {
    return false;
  }

  std::vector<BYTE> data(dataSize);

  if (GetFileVersionInfo(executableFilename, NULL, dataSize, &data[0]) == 0)
  {
    return false;
  }

  if (VerQueryValue(&data[0], _T("\\StringFileInfo\\080904b0\\ProductName"), &productName, &productNameLen) == 0 ||
      VerQueryValue(&data[0], _T("\\StringFileInfo\\080904b0\\ProductVersion"), &productVersion, &productVersionLen) == 0)
  {
    return false;
  }

  SetDlgItemText(hwnd, IDC_ABOUTNAME, (LPCTSTR) productName);
  SetDlgItemText(hwnd, IDC_ABOUTVERSION, (LPCTSTR) productVersion);

  return true;
}

INT_PTR CALLBACK AboutDialogueProcedure(HWND   hwnd,
                                        UINT   message,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
  switch(message)
  {
  case WM_NOTIFY:
    switch (((LPNMHDR) lParam)->code)
    {
    case NM_CLICK:
    case NM_RETURN:
      switch (LOWORD(wParam))
      {
      case IDC_ABOUTLINK:
      {
        PNMLINK pNMLink = (PNMLINK) lParam;
        LITEM   item    = pNMLink->item;

        ShellExecuteW(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
        return TRUE;
      }
      }
      break;
    }
    return FALSE;

  case WM_INITDIALOG:
    FillOutVersionFields(hwnd);
    return TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDOK:
      EndDialog(hwnd, 0);
      return TRUE;
    }
    return FALSE;
  }

  return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

// todo: synchronisation
static void SetSpeed(gamewin_t *gamewin, int tag)
{
  const int min_speed = 10;      // percent
  const int max_speed = 1000000; // percent
  int       newspeed;

  if (tag == 0)
  {
    gamewin->paused = !gamewin->paused;
    return;
  }

  gamewin->paused = false;

  switch (tag)
  {
  default:
  case 100: // normal speed
    newspeed = 100;
    break;

  case -1: // maximum speed
    newspeed = max_speed;
    break;

  case 1: // increase speed
    newspeed = gamewin->speed + 25;
    break;

  case 2: // decrease speed
    newspeed = gamewin->speed - 25;
    break;
  }

  if (newspeed < min_speed)
    newspeed = min_speed;
  else if (newspeed > max_speed)
    newspeed = max_speed;

  gamewin->speed = newspeed;
}

LRESULT CALLBACK GameWindowProcedure(HWND   hwnd,
                                     UINT   message,
                                     WPARAM wParam,
                                     LPARAM lParam)
{
  gamewin_t *gamewin;

  gamewin = (gamewin_t *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

  switch (message)
  {
    case WM_CREATE:
      {
        CREATESTRUCT *pCreateStruct = reinterpret_cast<CREATESTRUCT *>(lParam);
        gamewin = reinterpret_cast<gamewin_t *>(pCreateStruct->lpCreateParams);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) gamewin);

        /////////

        int rc;

        rc = CreateGame(gamewin);
        // MessageBox()
        // if (rc)
        //   error;
      }
      break;

    case WM_DESTROY:
      DestroyGameWindowThenQuit(gamewin);
      break;

    case WM_PAINT:
      {
        int           gameWidth, gameHeight;
        unsigned int *pixels;
        PAINTSTRUCT   ps;
        HDC           hdc;
        RECT          clientrect;
        float         windowWidth, windowHeight;
        float         gameWidthsPerWindow, gameHeightsPerWindow;
        float         gamesPerWindow;
        int           reducedBorder;
        int           drawWidth, drawHeight;
        int           xOffset, yOffset;

        gameWidth  = gamewin->zx->screen.width  * 8;
        gameHeight = gamewin->zx->screen.height * 8;

        pixels = zxspectrum_claim_screen(gamewin->zx);

        hdc = BeginPaint(hwnd, &ps);

        GetClientRect(hwnd, &clientrect);

        windowWidth  = clientrect.right  - clientrect.left;
        windowHeight = clientrect.bottom - clientrect.top;

        // How many natural-scale games fit comfortably into the window?
        // Try to fit while reducing the border if the window is very small
        reducedBorder = GAMEBORDER;
        do
        {
          gameWidthsPerWindow  = (windowWidth  - reducedBorder * 2) / gameWidth;
          gameHeightsPerWindow = (windowHeight - reducedBorder * 2) / gameHeight;
          gamesPerWindow = min(gameWidthsPerWindow, gameHeightsPerWindow);
          reducedBorder--;
        }
        while (reducedBorder >= 0 && gamesPerWindow < 1.0);

        // Snap the game scale to whole units
        if (gamesPerWindow > 1.0 && gamewin->snap)
        {
          const float snapSteps = 1.0; // set to 2.0 for scales of 1.0 / 1.5 / 2.0 etc.
          gamesPerWindow = floor(gamesPerWindow * snapSteps) / snapSteps;
        }

        drawWidth  = gameWidth  * gamesPerWindow;
        drawHeight = gameHeight * gamesPerWindow;

        // Centre the game within the window (note that conversion to int floors the values)
        xOffset = (windowWidth  - drawWidth)  / 2;
        yOffset = (windowHeight - drawHeight) / 2;

        StretchDIBits(hdc,
                      xOffset, yOffset,
                      drawWidth, drawHeight,
                      0, 0,
                      GAMEWIDTH, GAMEHEIGHT,
                      pixels,
                      &gamewin->bitmapinfo,
                      DIB_RGB_COLORS,
                      SRCCOPY);

        EndPaint(hwnd, &ps);

        zxspectrum_release_screen(gamewin->zx);
      }
      break;

    case WM_CLOSE:
      if (MessageBox(hwnd,
                     TEXT("Really quit?"),
                     szGameWindowTitle,
                     MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
        DestroyGameWindowThenQuit(gamewin);
      break;

    case WM_KEYDOWN:
    case WM_KEYUP:
      {
        bool         down = (message == WM_KEYDOWN);
        zxjoystick_t j;

        switch (wParam)
        {
        case VK_UP:         j = zxjoystick_UP;      break;
        case VK_DOWN:       j = zxjoystick_DOWN;    break;
        case VK_LEFT:       j = zxjoystick_LEFT;    break;
        case VK_RIGHT:      j = zxjoystick_RIGHT;   break;
        case VK_OEM_PERIOD: j = zxjoystick_FIRE;    break;
        default:            j = zxjoystick_UNKNOWN; break;
        }

        if (j != zxjoystick_UNKNOWN)
        {
          zxkempston_assign(&gamewin->kempston, j, down);
        }
        else
        {
          if (wParam == VK_CONTROL)
            zxkeyset_assign(&gamewin->keys, zxkey_CAPS_SHIFT, down);
          else if (wParam == VK_SHIFT)
            zxkeyset_assign(&gamewin->keys, zxkey_SYMBOL_SHIFT, down);
          else
          {
            if (down)
              zxkeyset_setchar(&gamewin->keys, wParam);
            else
              zxkeyset_clearchar(&gamewin->keys, wParam);
          }
        }
      }
      break;

    case WM_COMMAND:
    {
      int wmId = LOWORD(wParam);
      switch (wmId)
      {
      case ID_HELP_ABOUT:
      {
        (void) DialogBox(gamewin->instance,
                         MAKEINTRESOURCE(IDD_ABOUT),
                         hwnd,
                         AboutDialogueProcedure);
        break;
      }
      break;

      case ID_FILE_EXIT:
      DestroyGameWindowThenQuit(gamewin);
      break;

      case ID_FILE_NEW:
      CreateNewGame();
      break;

      case ID_GAME_DUPLICATE:
      // NYI
      break;

      case ID_VIEW_ACTUALSIZE:
      {
      }
      break;

      case ID_VIEW_ZOOMIN:
      {
      }
      break;

      case ID_VIEW_ZOOMOUT:
      {
      }
      break;

      case ID_VIEW_SNAPTOWHOLEPIXELS:
      gamewin->snap = !gamewin->snap;
      // TODO: refresh full window
      break;

      case ID_SOUND_ENABLED:
      // NYI
      break;

      case ID_SPEED_PAUSE:
      case ID_SPEED_100:
      case ID_SPEED_MAXIMUM:
      case ID_SPEED_FASTER:
      case ID_SPEED_SLOWER:
      {
        int s;

        switch (wmId)
        {
        case ID_SPEED_PAUSE:   s =   0; break;
        default:
        case ID_SPEED_100:     s = 100; break;
        case ID_SPEED_MAXIMUM: s =  -1; break;
        case ID_SPEED_FASTER:  s =   1; break;
        case ID_SPEED_SLOWER:  s =   2; break;
        }
        SetSpeed(gamewin, s);
      }
      break;

      default:
        return DefWindowProc(hwnd, message, wParam, lParam);
      }
    }
    break;

    default:
      return DefWindowProc(hwnd, message, wParam, lParam);
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////

BOOL RegisterGameWindowClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcx;
  ATOM       atom;

  wcx.cbSize        = sizeof(wcx);
  wcx.style         = CS_HREDRAW | CS_VREDRAW; // redraw the entire window whenever its size changes
  wcx.lpfnWndProc   = GameWindowProcedure;
  wcx.cbClsExtra    = 0; // no extra bytes after this class
  wcx.cbWndExtra    = 0;
  wcx.hInstance     = hInstance;
  wcx.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
  wcx.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wcx.hbrBackground = CreateSolidBrush(0x00000000);
  wcx.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
  wcx.lpszClassName = szGameWindowClassName;
  wcx.hIconSm       = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

  atom = RegisterClassEx(&wcx);
  if (!atom)
    return FALSE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

BOOL CreateGameWindow(HINSTANCE hInstance, int nCmdShow, gamewin_t **new_gamewin)
{
  gamewin_t *gamewin;
  RECT       rect;
  BOOL       result;
  HWND       window;

  *new_gamewin = NULL;

  gamewin = (gamewin_t *) calloc(1, sizeof(*gamewin));
  if (gamewin == NULL)
    return FALSE;

  // Required window dimensions
  rect.left   = 0;
  rect.top    = 0;
  rect.right  = GAMEWIDTH  + GAMEBORDER * 2;
  rect.bottom = GAMEHEIGHT + GAMEBORDER * 2;

  // Adjust window dimensions for window furniture
  result = AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
  if (result == 0)
    goto failure;

  window = CreateWindowEx(0,
                          szGameWindowClassName,
                          szGameWindowTitle,
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          rect.right - rect.left, rect.bottom - rect.top,
                          NULL,
                          NULL,
                          hInstance,
                          gamewin);
  if (!window)
    goto failure;

  ShowWindow(window, nCmdShow);
  UpdateWindow(window);

  gamewin->instance = hInstance;
  gamewin->window = window;

  *new_gamewin = gamewin;

  return TRUE;


failure:
  free(gamewin);

  return FALSE;
}

void DestroyGameWindow(gamewin_t *doomed)
{
  // DestroyWindow(doomed->window); // required?
  free(doomed);
}

///////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(__in     HINSTANCE hInstance,
                   __in_opt HINSTANCE hPrevInstance,
                   __in     LPSTR     lpszCmdParam,
                   __in     int       nCmdShow)
{
  static const INITCOMMONCONTROLSEX iccex =
  {
    sizeof(INITCOMMONCONTROLSEX),
    ICC_LINK_CLASS
  };

  HACCEL hAccelTable;
  int    rc;
  MSG    msg;

  hAccelTable = LoadAccelerators(hInstance,
                                 MAKEINTRESOURCE(IDR_ACCELERATOR1));

  InitCommonControlsEx(&iccex);

  rc = RegisterGameWindowClass(hInstance);
  if (!rc)
    return 0;

  rc = CreateNewGame();
  if (!rc)
    return 0;

  while (GetMessage(&msg, NULL, 0, 0) > 0)
  {
    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  DestroyAllGameWindows();

  return msg.wParam;
}

// vim: ts=8 sts=2 sw=2 et
