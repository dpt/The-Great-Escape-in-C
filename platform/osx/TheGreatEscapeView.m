//
//  TheGreatEscapeView.m
//  The Great Escape
//
//  Created by David Thomas on 11/10/2014.
//  Copyright (c) 2014 David Thomas. All rights reserved.
//

#import <ctype.h>

#import <pthread.h>

#import <Foundation/Foundation.h>

#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/glu.h>

#import <GLUT/glut.h>

#import "ZXSpectrum/ZXSpectrum.h"
#import "TheGreatEscape/TheGreatEscape.h"

#define SCALE 2.5f
#import "speckey.h"

#import "TheGreatEscapeView.h"

// -----------------------------------------------------------------------------

// move this into state
static speckeyfield_t keys;

#pragma mark - UIView

@interface TheGreatEscapeView()
{
  ZXSpectrum_t *zx;
  tgestate_t   *game;
  
  unsigned int *pixels;
  pthread_t     thread;
}

@end

// -----------------------------------------------------------------------------

@implementation TheGreatEscapeView

// -----------------------------------------------------------------------------

#pragma mark - Game thread callbacks

static void draw_handler(unsigned int *pixels, void *opaque)
{
  [(__bridge id) opaque setPixels:pixels];
}

static void sleep_handler(int duration, sleeptype_t sleeptype, void *opaque)
{
  usleep(duration); // duration is taken literally for now
}

static int key_handler(uint16_t port, void *opaque)
{
  return port_to_keyfield(port, keys);
}

// -----------------------------------------------------------------------------

#pragma mark - Game thread

static void *tge_thread(void *arg)
{
  tgestate_t *game = arg;
  
  tge_setup(game);

  for (;;) // while (!quit)
    tge_main(game);
  
  //tge_destroy(game);
  
  //ZXSpectrum_destroy(zx);
  
  return NULL;
}

// -----------------------------------------------------------------------------

#pragma mark - blah

- (void)setPixels:(unsigned int *)data
{
  pixels = data;
  [self setNeedsDisplayInRect:NSMakeRect(0, 0, 1000, 1000)];
}

- (void)awakeFromNib
{
  const ZXSpectrum_config_t zxconfig =
  {
    (__bridge void *)(self),
    &draw_handler,
    &sleep_handler,
    &key_handler,
  };

  static const tgeconfig_t tgeconfig =
  {
    256 / 8, 192 / 8
  };


  NSWindow *w = [self window];

  NSRect contentRect;
  contentRect.origin.x    = 0.0;
  contentRect.origin.y    = 0.0;
  contentRect.size.width  = tgeconfig.width  * 8 * SCALE;
  contentRect.size.height = tgeconfig.height * 8 * SCALE;

  [w setFrame:[w frameRectForContentRect:contentRect] display:YES];

  [self setFrame:NSMakeRect(0, 0, contentRect.size.width, contentRect.size.height)];


  zx     = NULL;
  game   = NULL;
  pixels = NULL;
  
  zx = ZXSpectrum_create(&zxconfig);
  if (zx == NULL)
    goto failure;
  
  game = tge_create(zx, &tgeconfig);
  if (game == NULL)
    goto failure;
  
  pthread_create(&thread, NULL /* pthread_attr_t */, tge_thread, game);
  
  return;
  
  
failure:
  tge_destroy(game);
  ZXSpectrum_destroy(zx);
}

// -----------------------------------------------------------------------------

- (id)initWithCoder:(NSCoder *)coder
{
  self = [super initWithCoder:coder];
  if (self) {
    [self prepare];
  }
  return self;
}

// -----------------------------------------------------------------------------

- (void)prepare
{
  NSLog(@"prepare");

  // The GL context must be active for these functions to have an effect
  [[self openGLContext] makeCurrentContext];
  
  // Configure the view
  glShadeModel(GL_FLAT); // Selects flat shading
  
  glMatrixMode(GL_PROJECTION); // Applies subsequent matrix operations to the projection matrix stack
  glLoadIdentity(); // Replace the current matrix with the identity matrix
  //  glOrtho(0, 640, 480, 0, 0, 1); // Multiply the current matrix with an orthographic matrix
  glDisable(GL_DEPTH_TEST); // Don't update the depth buffer.
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  //  glTranslatef(0.375, 0.375, 0);
}

// -----------------------------------------------------------------------------

//- (void)animate
//{
//  /* redraw just the region we've invalidated */
//  [self setNeedsDisplayInRect:NSMakeRect(0, 0, 1000, 1000)];
//}
//
//- (void)reshape
//{
//  // Convert up to window space, which is in pixel units.
//  NSRect baseRect = [self convertRectToBase:[self bounds]];
//
//  // Now the result is glViewport()-compatible.
//  glViewport(0, 0, (GLsizei) baseRect.size.width, (GLsizei) baseRect.size.height);
//}

// -----------------------------------------------------------------------------

//- (void)setTimer
//{
//  [NSTimer scheduledTimerWithTimeInterval:1.0 / 30 /* 30fps */
//                                   target:self
//                                 selector:@selector(onTick:)
//                                 userInfo:nil
//                                  repeats:YES];
//}
//
//- (void)onTick:(NSTimer *)timer
//{
//  (void) timer;
//
//  [self animate];
//}

// -----------------------------------------------------------------------------

- (void)drawRect:(NSRect)dirtyRect
{
  static float zsx =  SCALE;
  static float zsy = -SCALE;

  (void) dirtyRect;
  
  // Clear the background
  glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  
  if (pixels)
  {
    // Draw the image
    glRasterPos2f(-1.0f, 1.0f);
    glPixelZoom(zsx, zsy);
    glDrawPixels(256, 192, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  }
  
  // Flush to screen
  glFinish();
}

// -----------------------------------------------------------------------------

#pragma mark - Key handling

- (BOOL)acceptsFirstResponder
{
  return YES;
}

- (BOOL)becomeFirstResponder
{
  return YES;
}

- (void)keyDown:(NSEvent*)event
{
  NSString *chars = [event characters];
  
  if ([chars length] > 0)
  {
    unichar u = [chars characterAtIndex:0];
    
    if (u < 256)
      keys = set_speckey(keys, u);
  }
  
  // NSLog(@"Key pressed: %@", event);
}

- (void)keyUp:(NSEvent*)event
{
  NSString *chars = [event characters];
  
  if ([chars length] > 0)
  {
    unichar u = [chars characterAtIndex:0];
    
    if (u < 256)
      keys = clear_speckey(keys, u);
  }
  
  // NSLog(@"Key released: %@", event);
}

- (void)flagsChanged:(NSEvent*)event
{
  /* Unlike keyDown and keyUp, flagsChanged is a single event delivered when any
   * one of the modifier key states change, down or up. */

  bool shift = ([event modifierFlags] & NSShiftKeyMask) != 0;
  bool alt   = ([event modifierFlags] & NSAlternateKeyMask) != 0;

  /* For reference:
   *
   * bool control = ([event modifierFlags] & NSControlKeyMask) != 0;
   * bool command = ([event modifierFlags] & NSCommandKeyMask) != 0;
   */

  keys = assign_speckey(keys, speckey_CAPS_SHIFT,   shift);
  keys = assign_speckey(keys, speckey_SYMBOL_SHIFT, alt);

  // NSLog(@"Key shift=%d control=%d alt=%d command=%d", shift, control, alt, command);
}

// -----------------------------------------------------------------------------

@end
