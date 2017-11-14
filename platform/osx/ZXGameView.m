//
//  ZXGameView.m
//  The Great Escape
//
//  Created by David Thomas on 11/10/2014.
//  Copyright (c) 2014-2017 David Thomas. All rights reserved.
//

#import <ctype.h>

#import <pthread.h>

#import <Foundation/Foundation.h>

#import <Cocoa/Cocoa.h>

#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/glu.h>

#import <GLUT/glut.h>

#import "ZXSpectrum/Spectrum.h"
#import "ZXSpectrum/Keyboard.h"
#import "ZXSpectrum/Kempston.h"

#import "TheGreatEscape/TheGreatEscape.h"

#import "ZXGameWindowController.h"

#import "ZXGameView.h"

// -----------------------------------------------------------------------------

// Configuration
//
#define DEFAULTWIDTH      256
#define DEFAULTHEIGHT     192
#define DEFAULTBORDERSIZE  32

// -----------------------------------------------------------------------------

#pragma mark - UIView

@interface ZXGameView()
{
  tgeconfig_t   config;

  zxspectrum_t *zx;
  tgestate_t   *game;

  unsigned int *pixels;
  GLclampf      backgroundRed, backgroundGreen, backgroundBlue;
  NSRect        glViewContentRect;

  pthread_t     thread;
  zxkeyset_t    keys;
  zxkempston_t  kempston;

  BOOL          doSetupDrawing;
  int           borderSize;
  BOOL          snap;

  int           speed;
  BOOL          paused;

  BOOL          quit;
}

@property (nonatomic, readwrite) CGFloat scale;

@end

// -----------------------------------------------------------------------------

@implementation ZXGameView

// -----------------------------------------------------------------------------

#pragma mark - Standard methods

- (void)prepare
{
  // The GL context must be active for these functions to have an effect
  [[self openGLContext] makeCurrentContext];

  // Configure the view
  glShadeModel(GL_FLAT);
  glDisable(GL_DEPTH_TEST);

  // Reset the projection matrix
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // Reset the model view matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  // glTranslatef(0.375, 0.375, 0);
}

- (void)awakeFromNib
{
  const zxconfig_t zxconfig =
  {
    (__bridge void *)(self),
    &draw_handler,
    &sleep_handler,
    &key_handler,
    &border_handler
  };

  config.width  = DEFAULTWIDTH  / 8;
  config.height = DEFAULTHEIGHT / 8;

  zx              = NULL;
  game            = NULL;

  pixels          = NULL;
  backgroundRed = backgroundGreen = backgroundBlue = 0.0;
  glViewContentRect = NSMakeRect(0, 0, 0, 0);

  thread          = NULL;
  keys            = 0ULL;
  kempston        = 0;

  doSetupDrawing  = YES;
  borderSize      = DEFAULTBORDERSIZE;
  snap            = YES;

  speed           = 100;
  paused          = NO;

  quit            = NO;


  _scale          = 1.0;



  // TODO: allocate two window buffers


  zx = zxspectrum_create(&zxconfig);
  if (zx == NULL)
    goto failure;

  game = tge_create(zx, &config);
  if (game == NULL)
    goto failure;

  pthread_create(&thread, NULL /* pthread_attr_t */, tge_thread, (__bridge void *)(self));

  return;


failure:
  tge_destroy(game);
  zxspectrum_destroy(zx);
}

- (void)reshape
{
  doSetupDrawing = YES;
}

// -----------------------------------------------------------------------------

#pragma mark - UI

- (BOOL)acceptsFirstResponder
{
  return YES;
}

- (BOOL)mouseDownCanMoveWindow
{
  return YES;
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
{
  NSMenuItem *menuItem;

  if ([anItem action] == @selector(toggleSnap:))
  {
    menuItem = (NSMenuItem *) anItem;
    // Ensure that it's a menu item
    if ([menuItem respondsToSelector:@selector(setState:)])
      [menuItem setState:snap ? NSOnState : NSOffState];
  }

  return YES;
}

// -----------------------------------------------------------------------------

#pragma mark - Key handling

- (void)keyUpOrDown:(NSEvent *)event isDown:(BOOL)down
{
  NSEventModifierFlags  modifierFlags;
  NSEventModifierFlags  modifiersToReject;
  NSString             *chars;
  unichar               u;
  zxjoystick_t          j;

  modifierFlags = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
  modifiersToReject = NSEventModifierFlagControl |
  NSEventModifierFlagOption  |
  NSEventModifierFlagCommand;
  if (modifierFlags & modifiersToReject)
    return;

  chars = [event characters];
  if ([chars length] == 0)
    return;

  u = [chars characterAtIndex:0];
  switch (u)
  {
    case NSUpArrowFunctionKey:    j = zxjoystick_UP;      break;
    case NSDownArrowFunctionKey:  j = zxjoystick_DOWN;    break;
    case NSLeftArrowFunctionKey:  j = zxjoystick_LEFT;    break;
    case NSRightArrowFunctionKey: j = zxjoystick_RIGHT;   break;
    case '.':                     j = zxjoystick_FIRE;    break;
    default:                      j = zxjoystick_UNKNOWN; break;
  }

  if (j != zxjoystick_UNKNOWN)
  {
    zxkempston_assign(&kempston, j, down);
  }
  else
  {
    // zxkeyset_set/clearchar converts generic keypresses for us
    if (down)
      keys = zxkeyset_setchar(keys, u);
    else
      keys = zxkeyset_clearchar(keys, u);
  }
}

- (void)keyUp:(NSEvent *)event
{
  [self keyUpOrDown:event isDown:NO];
  // NSLog(@"Key released: %@", event);
}

- (void)keyDown:(NSEvent *)event
{
  [self keyUpOrDown:event isDown:YES];
  // NSLog(@"Key pressed: %@", event);
}

- (void)flagsChanged:(NSEvent *)event
{
  /* Unlike keyDown and keyUp, flagsChanged is a single event delivered when
   * any one of the modifier key states change, down or up. */

  NSEventModifierFlags modifierFlags = [event modifierFlags];
  bool shift = (modifierFlags & NSEventModifierFlagShift)  != 0;
  bool alt   = (modifierFlags & NSEventModifierFlagOption) != 0;

  /* For reference:
   *
   * bool control = (modifierFlags & NSControlKeyMask) != 0;
   * bool command = (modifierFlags & NSCommandKeyMask) != 0;
   */

  zxkeyset_assign(&keys, zxkey_CAPS_SHIFT,   shift);
  zxkeyset_assign(&keys, zxkey_SYMBOL_SHIFT, alt);

  // NSLog(@"Key shift=%d control=%d alt=%d command=%d", shift, control, alt, command);
}

// -----------------------------------------------------------------------------

#pragma mark - Action handlers

- (IBAction)setSpeed:(id)sender
{
  const int min_speed = 10;     // percent
  const int max_speed = 100000; // percent

  NSInteger tag;

  tag = [sender tag];

  if (tag == 0) // pause/unpause
  {
    paused = !paused;
    return;
  }

  // any other action results in unpausing
  paused = NO;

  switch (tag)
  {
    default:
    case 100: // normal speed
      speed = 100;
      break;

    case -1: // maximum speed
      speed = max_speed;
      break;

    case 1: // increase speed
      speed += 25;
      break;

    case 2: // decrease speed
      speed -= 25;
      break;
  }

  if (speed < min_speed)
    speed = min_speed;
  else if (speed > max_speed)
    speed = max_speed;
}

- (IBAction)toggleSnap:(id)sender
{
  snap = !snap;

  // (un)tick the menu entry
  [sender setState:snap ? NSOnState : NSOffState];

  // invalidate drawing setup
  doSetupDrawing = YES;

  [self displayIfNeeded];
}

// -----------------------------------------------------------------------------

- (void)setupDrawing
{
  CGSize  gameSize;
  NSRect  baseRect;
  CGSize  viewSize;
  CGFloat gameWidthsPerView, gameHeightsPerView, gamesPerView;
  GLsizei xOffset, yOffset;
  GLsizei drawWidth, drawHeight;

  gameSize.width  = config.width  * 8;
  gameSize.height = config.height * 8;

  // Convert up to window space, which is in pixel units
  baseRect = [self convertRectToBacking:[self bounds]];
  viewSize = baseRect.size;

  // How many game windows fit comfortably into the view?
  gameWidthsPerView  = (viewSize.width  - borderSize * 2) / gameSize.width;
  gameHeightsPerView = (viewSize.height - borderSize * 2) / gameSize.height;
  gamesPerView = MIN(gameWidthsPerView, gameHeightsPerView);

  // Try again without the border if the view is very small
  if (gamesPerView < 1.0)
  {
    gameWidthsPerView  = viewSize.width  / gameSize.width;
    gameHeightsPerView = viewSize.height / gameSize.height;
    gamesPerView = MIN(gameWidthsPerView, gameHeightsPerView);
  }

  // Force scaling if a full game won't fit
  if (gamesPerView > 1.0 && snap)
  {
      const CGFloat snapSteps = 1.0; // set to 2.0 for scales of 1.0 / 1.5 / 2.0 etc.
      gamesPerView = floor(gamesPerView * snapSteps) / snapSteps;
  }

  drawWidth  = (config.width  * 8) * gamesPerView;
  drawHeight = (config.height * 8) * gamesPerView;
  _scale = gamesPerView;

  // Centre the game within the view (note that conversion to int floors the values)
  xOffset = (viewSize.width  - drawWidth)  / 2;
  yOffset = (viewSize.height - drawHeight) / 2;

  // Set the affine transformation of x and y from normalized device coordinates to window coordinates
  glViewport(0, 0, viewSize.width, viewSize.height);
  // Set which matrix stack is the target for subsequent matrix operations
  glMatrixMode(GL_PROJECTION);
  // Replace the current matrix with the identity matrix
  glLoadIdentity();
  // Multiply the current matrix with an orthographic matrix
  glOrtho(0, viewSize.width, 0, viewSize.height, 0.1, 1);
  // Set the raster position for pixel operations
  glRasterPos3f(xOffset, viewSize.height - yOffset, -0.3);
}

- (void)drawRect:(NSRect)dirtyRect
{
  // Note: NSOpenGLViews would appear to never receive a graphics context, so
  // we must draw exclusively in terms of GL ops.

  if (_scale == 0.0)
    return;

  if (doSetupDrawing)
  {
    [self setupDrawing];
    doSetupDrawing = NO;
  }

  GLfloat zsx =  _scale;
  GLfloat zsy = -_scale;

  (void) dirtyRect;

  // Clear the background
  // Do this every frame or you'll see junk in the border.
  glClearColor(backgroundRed, backgroundGreen, backgroundBlue, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  if (pixels)
  {
    // Draw the image
    glPixelZoom(zsx, zsy);
    glDrawPixels(DEFAULTWIDTH, DEFAULTHEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  }

  // Flush to screen
  glFinish();
}

// -----------------------------------------------------------------------------

#pragma mark - Game thread

static void *tge_thread(void *arg)
{
  ZXGameView *view = (__bridge id) arg;
  tgestate_t *game = view->game;

  tge_setup(game);

  // While in menu state
  while (!view->quit)
    if (tge_menu(game) > 0)
      break; // game begins

  // While in game state
  if (!view->quit)
  {
    tge_setup2(game);
    while (!view->quit)
      tge_main(game);
  }

  return NULL;
}

// -----------------------------------------------------------------------------

#pragma mark - Game thread callbacks

static void draw_handler(unsigned int  *pixels,
                         const zxbox_t *dirty,
                         void          *opaque)
{
  ZXGameView *view = (__bridge id) opaque;
  NSRect      rect;

  view->pixels = pixels;
  rect = view->glViewContentRect;

  dispatch_async(dispatch_get_main_queue(), ^{
    // Odd: This refreshes the whole window no matter what size of rect is specified
    [view setNeedsDisplayInRect:rect];
  });
}

static void sleep_handler(int duration, sleeptype_t sleeptype, void *opaque)
{
  ZXGameView *view = (__bridge id) opaque;

  if (view->paused)
  {
    // Check twice per second for unpausing
    // FIXME: Slow spinwait without any synchronisation
    while (view->paused)
      usleep(500000);
  }
  else
  {
    usleep(duration * 100 / view->speed);
  }
}

static int key_handler(uint16_t port, void *opaque)
{
  ZXGameView *view = (__bridge id) opaque;

  if (port == 0x001F)
    return view->kempston;
  else
    return zxkeyset_for_port(port, view->keys);
}

static void border_handler(int colour, void *opaque)
{
  ZXGameView *view = (__bridge id) opaque;
  GLclampf    r, g, b;

  switch (colour)
  {
    case 0: r = 0; g = 0; b = 0; break;
    case 1: r = 0; g = 0; b = 1; break;
    case 2: r = 1; g = 0; b = 0; break;
    case 3: r = 1; g = 0; b = 1; break;
    case 4: r = 0; g = 1; b = 0; break;
    case 5: r = 0; g = 1; b = 1; break;
    case 6: r = 1; g = 1; b = 0; break;
    case 7: r = 1; g = 1; b = 1; break;
    default: return;
  }

  dispatch_async(dispatch_get_main_queue(), ^{
    view->backgroundRed   = r;
    view->backgroundGreen = g;
    view->backgroundBlue  = b;
  });
}

// -----------------------------------------------------------------------------

#pragma mark - Queries

- (void)getGameWidth:(int *)width height:(int *)height border:(int *)border
{
  *width  = config.width  * 8;
  *height = config.height * 8;
  *border = borderSize;
}

@end
