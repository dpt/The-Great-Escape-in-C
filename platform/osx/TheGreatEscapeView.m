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

#import "TheGreatEscapeView.h"

// -----------------------------------------------------------------------------

#pragma mark Spectrum keys utility

/**
 * A code for every Spectrum key.
 */
typedef enum speckey
{
  speckey_SPACE,
  speckey_SYMBOL_SHIFT,
  speckey_M,
  speckey_N,
  speckey_B,

  speckey_ENTER,
  speckey_L,
  speckey_K,
  speckey_J,
  speckey_H,

  speckey_P,
  speckey_O,
  speckey_I,
  speckey_U,
  speckey_Y,

  speckey_0,
  speckey_9,
  speckey_8,
  speckey_7,
  speckey_6,

  speckey_1,
  speckey_2,
  speckey_3,
  speckey_4,
  speckey_5,

  speckey_Q,
  speckey_W,
  speckey_E,
  speckey_R,
  speckey_T,

  speckey_A,
  speckey_S,
  speckey_D,
  speckey_F,
  speckey_G,

  speckey_CAPS_SHIFT,
  speckey_Z,
  speckey_X,
  speckey_C,
  speckey_V,

  speckey__LIMIT,
  
  speckey_UNKNOWN = -1
}
speckey_t;

/**
 * A bitfield large enough to hold all 40 Spectrum keys using one bit each.
 */
typedef unsigned long long speckeyfield_t;

/**
 * Mark or unmark the given key.
 */
static __inline speckeyfield_t assign_speckey(speckeyfield_t keystate,
                                              speckey_t      index,
                                              bool           on_off)
{
  return (keystate & (~1ULL << index)) | ((unsigned long long) on_off << index);
}

/**
 * Extract the current key state for the specified port and return it.
 */
static __inline int port_to_keyfield(uint16_t port, speckeyfield_t keystate)
{
  int clz = __builtin_clz(~port << 16);
  return (~keystate >> clz * 5) & 0x1F;
}

/**
 * Convert the given character to a speckey_t.
 */
static speckey_t char_to_speckey(int c)
{
  switch (toupper(c))
  {
    case ' ':
      return speckey_SPACE;
//  case XXX:
//    return speckey_SYMBOL_SHIFT;
    case 'M':
      return speckey_M;
    case 'N':
      return speckey_N;
    case 'B':
      return speckey_B;

    case '\r':
    case '\n':
      return speckey_ENTER;
    case 'L':
      return speckey_L;
    case 'K':
      return speckey_K;
    case 'J':
      return speckey_J;
    case 'H':
      return speckey_H;

    case 'P':
      return speckey_P;
    case 'O':
      return speckey_O;
    case 'I':
      return speckey_I;
    case 'U':
      return speckey_U;
    case 'Y':
      return speckey_Y;

    case '0':
      return speckey_0;
    case '9':
      return speckey_9;
    case '8':
      return speckey_8;
    case '7':
      return speckey_7;
    case '6':
      return speckey_6;

    case '1':
      return speckey_1;
    case '2':
      return speckey_2;
    case '3':
      return speckey_3;
    case '4':
      return speckey_4;
    case '5':
      return speckey_5;

    case 'Q':
      return speckey_Q;
    case 'W':
      return speckey_W;
    case 'E':
      return speckey_E;
    case 'R':
      return speckey_R;
    case 'T':
      return speckey_T;

    case 'A':
      return speckey_A;
    case 'S':
      return speckey_S;
    case 'D':
      return speckey_D;
    case 'F':
      return speckey_F;
    case 'G':
      return speckey_G;

//  case XXX:
//    return speckey_CAPS_SHIFT;
    case 'Z':
      return speckey_Z;
    case 'X':
      return speckey_X;
    case 'C':
      return speckey_C;
    case 'V':
      return speckey_V;
      
    default:
      return speckey_UNKNOWN;
  }
}

/**
 * Mark the given character c as a held down key.
 */
static __inline speckeyfield_t set_speckey(speckeyfield_t keystate, int c)
{
  speckey_t sk = char_to_speckey(c);
  if (sk != speckey_UNKNOWN)
    keystate |= 1ULL << sk;
  return keystate;
}

/**
 * Mark the given character c as a released key.
 */
static __inline speckeyfield_t clear_speckey(speckeyfield_t keystate, int c)
{
  speckey_t sk = char_to_speckey(c);
  if (sk != speckey_UNKNOWN)
    keystate &= ~(1ULL << sk);
  return keystate;
}

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

static void sleep_handler(int duration, void *opaque)
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
  
  //tge_main(game);
  
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
    32, 16,
  };
  
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
  (void) dirtyRect;
  
  // Clear the background
  glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  
  if (pixels)
  {
    // Draw the image
    glRasterPos2f(-1.0f, 1.0f);
    glPixelZoom(1.0f, -1.0f);
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

  unsigned long long shift   = ([event modifierFlags] & NSShiftKeyMask) != 0;
  unsigned long long alt     = ([event modifierFlags] & NSAlternateKeyMask) != 0;

  /* For reference:
   *
   * control = ([event modifierFlags] & NSControlKeyMask) != 0;
   * command = ([event modifierFlags] & NSCommandKeyMask) != 0;
   */

  assign_speckey(keys, speckey_CAPS_SHIFT,   shift);
  assign_speckey(keys, speckey_SYMBOL_SHIFT, alt);

  // NSLog(@"Key shift=%d control=%d alt=%d command=%d", shift, control, alt, command);
}

// -----------------------------------------------------------------------------

@end
