//
//  ZXGameView.m
//  The Great Escape
//
//  Created by David Thomas on 11/10/2014.
//  Copyright (c) 2014-2018 David Thomas. All rights reserved.
//

#import <ctype.h>
#import <sys/time.h>

#import <pthread/pthread.h>

#import <Foundation/Foundation.h>

#import <AudioUnit/AudioUnit.h>

#import <Cocoa/Cocoa.h>

#import <CoreImage/CoreImage.h>

#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/glu.h>

#import <GLUT/glut.h>

#import "ZXSpectrum/Spectrum.h"
#import "ZXSpectrum/Keyboard.h"
#import "ZXSpectrum/Kempston.h"

#import "TheGreatEscape/TheGreatEscape.h"

#import "ZXGameWindowController.h"

#import "bitfifo.h"

#import "ZXGameView.h"

// -----------------------------------------------------------------------------

// Configuration
//
#define DEFAULTWIDTH      256
#define DEFAULTHEIGHT     192
#define DEFAULTBORDERSIZE  32

#define MAXSTAMPS           4 /* depth of nested timestamp stack */

// -----------------------------------------------------------------------------

#pragma mark - UIView

@interface ZXGameView()
{
  tgeconfig_t     config;

  zxspectrum_t   *zx;
  tgestate_t     *game;

  pthread_t       thread;

  BOOL            doSetupDrawing;
  int             borderSize;
  BOOL            snap;
  BOOL            monochromatic;

  // -- Start of variables accessed from game thread must be @synchronized

  BOOL            quit;
  BOOL            paused;

  int             speed;    // percent

  struct timeval  stamps[MAXSTAMPS];
  int             nstamps;

  zxkeyset_t      keys;
  zxkempston_t    kempston;

  GLclampf        backgroundRed, backgroundGreen, backgroundBlue;

  struct
  {
    BOOL          enabled;
    int           index;    // index into samples
    bitfifo_t    *fifo;
    int           lastvol;  // most recently output volume
    AudioComponentInstance instance;
  }
  audio;

  // -- End of variables accessed from game thread must be @synchronized
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
    &stamp_handler,
    &sleep_handler,
    &key_handler,
    &border_handler,
    &speaker_handler
  };

  config.width  = DEFAULTWIDTH  / 8;
  config.height = DEFAULTHEIGHT / 8;

  zx              = NULL;
  game            = NULL;

  thread          = NULL;

  doSetupDrawing  = YES;
  borderSize      = DEFAULTBORDERSIZE;
  snap            = YES;
  monochromatic   = NO;

  quit            = NO;
  paused          = NO;

  speed           = 100;

  memset(stamps, 0, sizeof(stamps));
  nstamps = 0;

  keys            = 0ULL;
  kempston        = 0;

  backgroundRed = backgroundGreen = backgroundBlue = 0.0;

  _scale          = 1.0;

  zx = zxspectrum_create(&zxconfig);
  if (zx == NULL)
    goto failure;

  game = tge_create(zx, &config);
  if (game == NULL)
    goto failure;

  [self setupAudio];

  pthread_create(&thread,
                 NULL /* pthread_attr_t */,
                 tge_thread,
                 (__bridge void *)(self));

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

- (void)stop
{
  // Signal to the game thread to quit
  @synchronized(self)
  {
    quit = YES;
    paused = NO;
  }

  // Wait for it to yield
  // Is this is a bad idea to wait like this on the UI thread?
  pthread_join(thread, NULL);

  tge_destroy(game);
  zxspectrum_destroy(zx);

  [self teardownAudio];
}

// -----------------------------------------------------------------------------

// Simulate a black and white TV
- (IBAction)toggleMonochromatic:(id)sender
{
  if (monochromatic)
  {
    self.contentFilters = @[];
  }
  else
  {
    CIFilter       *filter;
    NSMutableArray *filters;

    filter = [CIFilter filterWithName:@"CIPhotoEffectMono"];
    [filter setDefaults];
    filters = [NSMutableArray arrayWithObject:filter];
    self.contentFilters = filters;
  }

  monochromatic = !monochromatic;
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
  else if ([anItem action] == @selector(toggleMonochromatic:))
  {
    menuItem = (NSMenuItem *) anItem;
    // Ensure that it's a menu item
    if ([menuItem respondsToSelector:@selector(setState:)])
      [menuItem setState:monochromatic ? NSOnState : NSOffState];
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

  // Ignore key repeat events. We're only interested in state changes so we can
  // set/clear bits in the zxkeyset bitfield. Also, if we don't ignore repeats
  // it will result in the self->mutex being taken too often which will starve
  // the main game thread.
  if (event.isARepeat)
    return;

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

  @synchronized(self)
  {
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

  @synchronized(self)
  {
    zxkeyset_assign(&keys, zxkey_CAPS_SHIFT,   shift);
    zxkeyset_assign(&keys, zxkey_SYMBOL_SHIFT, alt);
  }

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

  @synchronized(self)
  {
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

- (IBAction)toggleSound:(id)sender
{
  [self toggleAudio];
}

// -----------------------------------------------------------------------------

- (void)setupDrawing
{
  CGSize  gameSize;
  NSRect  baseRect;
  CGSize  viewSize;
  CGFloat gameWidthsPerView, gameHeightsPerView, gamesPerView;
  int     reducedBorder;
  GLsizei xOffset, yOffset;
  GLsizei drawWidth, drawHeight;

  gameSize.width  = config.width  * 8;
  gameSize.height = config.height * 8;

  // Convert up to window space, which is in pixel units
  baseRect = [self bounds];
  viewSize = baseRect.size;

  // How many 1:1 game windows fit comfortably into the view?
  // Try to fit while reducing the border if the view is very small
  reducedBorder = borderSize;
  do
  {
    gameWidthsPerView  = (viewSize.width  - reducedBorder * 2) / gameSize.width;
    gameHeightsPerView = (viewSize.height - reducedBorder * 2) / gameSize.height;
    gamesPerView = MIN(gameWidthsPerView, gameHeightsPerView);
    reducedBorder--;
  }
  while (reducedBorder >= 0 && gamesPerView < 1.0);

  // Snap the game scale to whole units
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
  uint32_t *pixels;

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

  pixels = zxspectrum_claim_screen(zx);

  // Clear the background
  // Do this every frame or you'll see junk in the border.
  glClearColor(backgroundRed, backgroundGreen, backgroundBlue, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  // Draw the image
  glPixelZoom(zsx, zsy);
  glDrawPixels(DEFAULTWIDTH, DEFAULTHEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Flush to screen
  glFinish();

  zxspectrum_release_screen(zx);
}

// -----------------------------------------------------------------------------

#pragma mark - Game thread

static void *tge_thread(void *arg)
{
  ZXGameView *view = (__bridge id) arg;
  tgestate_t *game = view->game;
  BOOL        quit;

  tge_setup(game);

  // While in menu state
  for (;;)
  {
    @synchronized(view)
    {
      quit = view->quit;
    }

    if (quit)
      break;

    if (tge_menu(game) > 0)
      break; // game begins
  }

  if (!quit)
  {
    tge_setup2(game);

    // While in game state
    for (;;)
    {
      @synchronized(view)
      {
        quit = view->quit;
      }

      if (quit)
        break;

      tge_main(game);
      view->nstamps = 0; // reset all timing info
    }
  }

  return NULL;
}

// -----------------------------------------------------------------------------

#pragma mark - Game thread callbacks

static void draw_handler(const zxbox_t *dirty,
                         void          *opaque)
{
  ZXGameView *view = (__bridge id) opaque;

  dispatch_async(dispatch_get_main_queue(), ^{
    // Odd: This refreshes the whole window no matter what size of rect is specified
    [view setNeedsDisplayInRect:NSMakeRect(0, 0, 0, 0)];
  });
}

static void stamp_handler(void *opaque)
{
  ZXGameView *view = (__bridge id) opaque;

  // Stack timestamps as they arrive
  assert(view->nstamps < MAXSTAMPS);
  if (view->nstamps >= MAXSTAMPS)
    return;
  gettimeofday(&view->stamps[view->nstamps++], NULL);
}

static void sleep_handler(int durationTStates, void *opaque)
{
  ZXGameView *view = (__bridge id) opaque;
  BOOL paused;

  @synchronized(view)
  {
    // Unstack timestamps (even if we're paused)
    assert(view->nstamps > 0);
    if (view->nstamps <= 0)
      return;
    --view->nstamps;

    paused = view->paused;
  }

  if (paused)
  {
    // Check twice per second for unpausing
    for (;;)
    {
      @synchronized(view)
      {
        paused = view->paused;
      }

      if (!paused)
        break;

      usleep(500000); /* 0.5s */
    }
  }
  else
  {
    // A Spectrum 48K has 69,888 T-states per frame and its Z80 runs at
    // 3.5MHz (~50Hz) for a total of 3,500,000 T-states per second.
    const double          tstatesPerSec = 3.5e6;

    struct timeval        now;
    double                duration; /* seconds */
    const struct timeval *then;
    struct timeval        delta;
    double                consumed; /* seconds */

    gettimeofday(&now, NULL); // get time now before anything else

    @synchronized(view)
    {
      // 'duration' tells us how long the operation should take since the previous mark call.
      // Turn T-state duration into seconds
      duration = durationTStates / tstatesPerSec;
      // Adjust the game speed
      duration = duration * 100 / view->speed;

      then = &view->stamps[view->nstamps];
    }

    delta.tv_sec  = now.tv_sec  - then->tv_sec;
    delta.tv_usec = now.tv_usec - then->tv_usec;

    consumed = delta.tv_sec + delta.tv_usec / 1e6;
    if (consumed < duration)
    {
      double     delay; /* seconds */
      useconds_t udelay;

      // We didn't take enough time - sleep for the remainder of our duration
      delay = duration - consumed;
      udelay = delay * 1e6;
      usleep(udelay);
    }
  }
}

static int key_handler(uint16_t port, void *opaque)
{
  ZXGameView *view = (__bridge id) opaque;
  int         k;

  @synchronized(view)
  {
    if (port == port_KEMPSTON_JOYSTICK)
      k = view->kempston;
    else
      k = zxkeyset_for_port(port, view->keys);
  }

  return k;
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

static void speaker_handler(int on_off, void *opaque)
{
  ZXGameView  *view = (__bridge id) opaque;
  unsigned int bit = on_off;

  @synchronized(view)
  {
    // There's nothing we can do if the buffer is full, so ignore errors
    (void) bitfifo_enqueue(view->audio.fifo, &bit, 0, 1);
  }
}

// -----------------------------------------------------------------------------

#pragma mark - Queries

- (void)getGameWidth:(int *)width height:(int *)height border:(int *)border
{
  *width  = config.width  * 8;
  *height = config.height * 8;
  *border = borderSize;
}

// -----------------------------------------------------------------------------

#pragma mark - Sound

// Use output bus zero
#define kOutputBus      (0)

// To make things easy we assume that the emulated ZX Spectrum is pumping out
// 220,500 samples per second into the fifo. We then sample this down by
// averaging five value to produce a 44.1KHz signal. This certainly sounds
// right but I'm not currently sure how accurate it really is.
// To produce a 22.05KHz signal instead: average ten values, and so on.
#define SAMPLE_RATE     (44100)
#define BITFIFO_LENGTH  (220500 / 4) // one seconds' worth of input bits (fifo will be ~27KiB)
#define AVG             (5) // average this many input bits to make an output sample

// Z80 @ 3.5MHz
// Each OUT takes 11 cycles
// >>> 3.5e6 / 11
// 318181.81 recurring OUTs/s theoretical max


// This is the same as a AURenderCallback except it's not a pointer type.
static OSStatus playbackCallback(void                       *inRefCon,
                                 AudioUnitRenderActionFlags *ioActionFlags,
                                 const AudioTimeStamp       *inTimeStamp,
                                 UInt32                      inBusNumber,
                                 UInt32                      inNumberFrames,
                                 AudioBufferList            *ioData);

- (void)setupAudio
{
  AudioComponentDescription   desc;
  AudioComponent              component;
  AudioComponentInstance      instance;
  OSStatus                    status;
  UInt32                      flag;
  AudioStreamBasicDescription audioFormat;
  AURenderCallbackStruct      input;

  // Describe audio component
  desc.componentType          = kAudioUnitType_Output;
  desc.componentSubType       = kAudioUnitSubType_DefaultOutput;
  desc.componentManufacturer  = kAudioUnitManufacturer_Apple;
  desc.componentFlags         = 0;
  desc.componentFlagsMask     = 0;

  // Find a matching audio component
  component = AudioComponentFindNext(NULL, &desc);
  if (component == NULL)
    return; // no matching audio component was found

  // Create a new instance of the audio component
  status = AudioComponentInstanceNew(component, &instance);
  if (status)
    goto failure;

  // Enable IO for playback
  flag = 1;
  status = AudioUnitSetProperty(instance,
                                kAudioOutputUnitProperty_EnableIO,
                                kAudioUnitScope_Output,
                                kOutputBus,
                                &flag,
                                sizeof(flag));
  if (status && status != kAudioUnitErr_InvalidProperty)
    goto failure;

  // Describe the format
  audioFormat.mSampleRate       = SAMPLE_RATE;
  audioFormat.mFormatID         = kAudioFormatLinearPCM;
  audioFormat.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger |
                                  kLinearPCMFormatFlagIsPacked;
  audioFormat.mBytesPerPacket   = 2;
  audioFormat.mFramesPerPacket  = 1;
  audioFormat.mBytesPerFrame    = 2;
  audioFormat.mChannelsPerFrame = 1;
  audioFormat.mBitsPerChannel   = 16 * 1;
  audioFormat.mReserved         = 0;

  // Apply the format
  status = AudioUnitSetProperty(instance,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                kOutputBus,
                                &audioFormat,
                                sizeof(audioFormat));
  if (status)
    goto failure;

  // Set the output callback
  input.inputProc       = playbackCallback;
  input.inputProcRefCon = (__bridge void *)(self);
  status = AudioUnitSetProperty(instance,
                                kAudioUnitProperty_SetRenderCallback,
                                kAudioUnitScope_Input,
                                kOutputBus,
                                &input,
                                sizeof(input));
  if (status)
    goto failure;

  // Initialise
  status = AudioUnitInitialize(instance);
  if (status)
    goto failure;

  // Setup audio
  audio.enabled  = YES;
  audio.index    = 0;
  audio.fifo     = NULL;
  audio.lastvol  = 0;
  audio.instance = instance;

  // Create a bit fifo
  audio.fifo = bitfifo_create(BITFIFO_LENGTH);
  if (audio.fifo == NULL)
    goto failure; // OOM

  // Start playing
  status = AudioOutputUnitStart(instance);
  if (status)
    goto failure;

  return;


failure:
  //  cout << "failure" << status << endl;

  return;
}

- (void)toggleAudio
{
  OSStatus status;

  audio.enabled = !audio.enabled;

  audio.index = 0; // flush the buffer

  if (audio.enabled)
    status = AudioOutputUnitStart(audio.instance);
  else
    status = AudioOutputUnitStop(audio.instance);

  // if (status)
  //   there was a problem;
}

- (void)teardownAudio
{
  AudioOutputUnitStop(audio.instance);
  AudioUnitUninitialize(audio.instance);
  AudioComponentInstanceDispose(audio.instance);
}

static
OSStatus playbackCallback(void                       *inRefCon,
                          AudioUnitRenderActionFlags *ioActionFlags,
                          const AudioTimeStamp       *inTimeStamp,
                          UInt32                      inBusNumber,
                          UInt32                      inNumberFrames,
                          AudioBufferList            *ioData)
{
  const int   bytesPerSample = 2;

  ZXGameView *view = (__bridge id) inRefCon;
  result_t    err;
  OSStatus    rc;
  UInt32      i;

  @synchronized(view)
  {
    // CHECK: Is this going to screw up with mNumberBuffers > 1 since i'm
    //        fetching from the fifo?

    for (i = 0; i < ioData->mNumberBuffers; i++)
    {
      assert(inNumberFrames == ioData->mBuffers[i].mDataByteSize / bytesPerSample);

      UInt32 outputCapacity;
      UInt32 outputUsed;

      outputCapacity = inNumberFrames;
      outputUsed     = 0;

      while (outputUsed < outputCapacity)
      {
        UInt32 outputCapacityRemaining;
        size_t bitsInFifo;
        size_t toCopy;

        outputCapacityRemaining = outputCapacity - outputUsed;
        bitsInFifo              = bitfifo_used(view->audio.fifo);
        toCopy                  = MIN(bitsInFifo, outputCapacityRemaining);

        if (bitsInFifo == 0)
          goto no_data;

        const signed short maxVolume = 32500; // tweak this down to quieten

        signed short      *p;
        signed short       vol;
        UInt32             j;

        p = (signed short *) ioData->mBuffers[i].mData + outputUsed;
        for (j = 0; j < toCopy; j++)
        {
          unsigned int bits;

          bits = 0;
          err = bitfifo_dequeue(view->audio.fifo, &bits, AVG);
          if (err == result_BITFIFO_INSUFFICIENT ||
              err == result_BITFIFO_EMPTY)
            // When the fifo empties maintain the most recent volume
            vol = view->audio.lastvol;
          else
            vol = (int) __builtin_popcount(bits) * maxVolume / AVG;
          *p++ = vol;

          view->audio.lastvol = vol;
        }

        outputUsed += toCopy;
      }
      ioData->mBuffers[i].mDataByteSize = outputUsed * bytesPerSample;

      assert(outputUsed == outputCapacity);
    }

    rc = noErr;
    goto exit;


  no_data:
    ioData->mBuffers[0].mDataByteSize = 0;
    rc = -1;

  exit:
    return rc;
  }
}

@end

// vim: ts=8 sts=2 sw=2 et
