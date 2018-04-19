// main.cpp
//
// Windows front-end for The Great Escape
//
// Copyright (c) David Thomas, 2016-2018. <dave@davespace.co.uk>
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

#include "ZXSpectrum/Spectrum.h"
#include "ZXSpectrum/Keyboard.h"

#include "TheGreatEscape/TheGreatEscape.h"

#include "resource.h"

///////////////////////////////////////////////////////////////////////////////

// Configuration
//
#define DEFAULTWIDTH      256
#define DEFAULTHEIGHT     192
#define DEFAULTBORDERSIZE  32

#define MAXSTAMPS           4 /* depth of nested timestamp stack */

///////////////////////////////////////////////////////////////////////////////

static const TCHAR szGameWindowClassName[] = _T("TheGreatEscapeWindowsApp");
static const TCHAR szGameWindowTitle[]     = _T("The Great Escape");

///////////////////////////////////////////////////////////////////////////////

typedef struct gamewin
{
  HINSTANCE       instance;
  HWND            window;
  BITMAPINFO      bitmapinfo;

  tgeconfig_t     config;
  zxspectrum_t   *zx;
  tgestate_t     *tge;

  HANDLE          thread;
  DWORD           threadId;

  bool            snap;

  zxkeyset_t      keys;

  int             speed;
  BOOL            paused;

  bool            quit;

  ULONGLONG       stamps[MAXSTAMPS];
  int             nstamps;
}
gamewin_t;

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

static void sleep_handler(int durationTStates, void *opaque)
{
  gamewin_t *gamewin = (gamewin_t *) opaque;

  // Unstack timestamps (even if we're paused)
  assert(gamewin->nstamps > 0);
  if (gamewin->nstamps <= 0)
    return;
  --gamewin->nstamps;

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
    duration = duration * 100 / gamewin->speed;

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
}

static int key_handler(uint16_t port, void *opaque)
{
  gamewin_t *gamewin = (gamewin_t *) opaque;

  return zxkeyset_for_port(port, gamewin->keys);
}

static void border_handler(int colour, void *opaque)
{
  // does nothing presently
}

static void speaker_handler(int on_off, void *opaque)
{
  // does nothing presently
}

///////////////////////////////////////////////////////////////////////////////

static DWORD WINAPI gamewin_thread(LPVOID lpParam)
{
  gamewin_t  *win  = (gamewin_t *) lpParam;
  tgestate_t *game = win->tge;

  tge_setup(game);

  // While in menu state
  while (!win->quit)
    if (tge_menu(game) > 0)
      break; // game begins

  // While in game state
  if (!win->quit)
  {
    tge_setup2(game);
    while (!win->quit)
    {
      tge_main(game);
      win->nstamps = 0; // reset all timing info
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

  gamewin->config.width  = DEFAULTWIDTH  / 8;
  gamewin->config.height = DEFAULTHEIGHT / 8;

  tge = tge_create(zx, &gamewin->config);
  if (tge == NULL)
    goto failure;

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
  bmih->biWidth         = DEFAULTWIDTH;
  bmih->biHeight        = -DEFAULTHEIGHT; // negative height flips the image
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

  gamewin->keys     = 0;

  gamewin->speed    = 100;
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

BOOL CreateGameWindow(HINSTANCE hInstance, int nCmdShow, gamewin_t **new_gamewin)
{
  gamewin_t *gamewin;
  RECT       rect;
  BOOL       result;
  HWND       window;

  *new_gamewin = NULL;

  gamewin = (gamewin_t *) calloc(sizeof(*gamewin), 1);
  if (gamewin == NULL)
    return FALSE;

  gamewin->instance = hInstance;

  // Required window dimensions
  rect.left   = 0;
  rect.top    = 0;
  rect.right  = DEFAULTWIDTH  + DEFAULTBORDERSIZE * 2;
  rect.bottom = DEFAULTHEIGHT + DEFAULTBORDERSIZE * 2;

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

  gamewin->window = window;

  *new_gamewin = gamewin;

  return TRUE;


failure:
  free(gamewin);

  return FALSE;
}

void DestroyGameWindow(gamewin_t *doomed)
{
  //DestroyWindow(doomed->window);
  free(doomed);
}

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

        ShellExecute(NULL, TEXT("open"), item.szUrl, NULL, NULL, SW_SHOW);
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

        gameWidth  = gamewin->config.width  * 8;
        gameHeight = gamewin->config.height * 8;

        pixels = zxspectrum_claim_screen(gamewin->zx);

        hdc = BeginPaint(hwnd, &ps);

        GetClientRect(hwnd, &clientrect);

        windowWidth  = clientrect.right  - clientrect.left;
        windowHeight = clientrect.bottom - clientrect.top;

        // How many natural-scale games fit comfortably into the window?
        // Try to fit while reducing the border if the window is very small
        reducedBorder = DEFAULTBORDERSIZE;
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
                      DEFAULTWIDTH, DEFAULTHEIGHT,
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
        bool down = (message == WM_KEYDOWN);

        if (wParam == VK_CONTROL)
          zxkeyset_assign(&gamewin->keys, zxkey_CAPS_SHIFT,   down);
        else if (wParam == VK_SHIFT)
          zxkeyset_assign(&gamewin->keys, zxkey_SYMBOL_SHIFT, down);
        else
        {
          if (down)
            gamewin->keys = zxkeyset_setchar(gamewin->keys, wParam);
          else
            gamewin->keys = zxkeyset_clearchar(gamewin->keys, wParam);
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
