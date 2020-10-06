//
//  ZXGameWindowController.m
//  The Great Escape
//
//  Created by David Thomas on 03/08/2017.
//  Copyright © 2017-2018 David Thomas. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "ZXGameView.h"

#import "ZXGameWindowController.h"

@implementation ZXGameWindowController
{
  int gameWidth, gameHeight, gameBorder;
  NSURL *saveGame;
}

static int ngames;

#pragma mark - Standard methods

- (id)init
{
  self = [super initWithWindowNibName:@"ZXGameWindow" owner:self];
  ngames++;

  return self;
}

- (void)windowDidLoad
{
  [super windowDidLoad];

  assert(gameView != nil);

  if (ngames >= 2)
    [[self window] setTitle:[NSString stringWithFormat:@"The Great Escape %d", ngames]];

  // shouldCascadeWindows does magic to offset each window when we first open
  // it - even if we try to centre it manually
  self.shouldCascadeWindows = YES; // (default)

  // Get the configured game dimensions
  [gameView getGameWidth:&gameWidth height:&gameHeight border:&gameBorder];

  [self resizeAndCentreGameWindow];

  gameView.delegate = self; // too late?
  [gameView start];

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(windowWillCloseNotification:)
                                               name:NSWindowWillCloseNotification
                                             object:[self window]]; //[self window] will cause the gameview to load, so leave it to here
}

// -----------------------------------------------------------------------------

#pragma mark - Notification handlers

- (void)windowWillCloseNotification:(NSNotification *)notification
{
  [gameView stop];
}

// -----------------------------------------------------------------------------

#pragma mark - IBAction handlers

- (IBAction)setZoomLevel:(id)sender
{
  NSInteger tag;
  int       direction;

  tag = [sender tag];
  direction = 0; // Actual size
  if (tag == 1)
    direction = +1; // Zoom in
  else if (tag == 2)
    direction = -1; // Zoom out
  [self scaleWindow:direction];
}

// -----------------------------------------------------------------------------

#pragma mark - Window handling

/// Resize and centre the window frame.
///
/// Note: [NSWindow center] did not correctly centre the window on the Y axis
/// when I tried it, so this does it manually.
///
- (void)resizeAndCentreGameWindow
{
  NSWindow *window;
  int       borderedWidth, borderedHeight;
  NSRect    contentRect;
  NSRect    frame;
  int       nwidth, nheight;
  NSRect    screenFrame;
  CGFloat   x,y;
  NSRect    centeredFrame;

  window = [self window];

  borderedWidth  = gameWidth  + gameBorder * 2;
  borderedHeight = gameHeight + gameBorder * 2;
  contentRect = NSMakeRect(0, 0, borderedWidth, borderedHeight);
  frame = [window frameRectForContentRect:contentRect];
  nwidth  = frame.size.width;
  nheight = frame.size.height;

  screenFrame = [[NSScreen mainScreen] frame];
  x = screenFrame.origin.x + (screenFrame.size.width  - nwidth)  / 2;
  y = screenFrame.origin.y + (screenFrame.size.height - nheight) / 2;
  centeredFrame = NSMakeRect(x, y, nwidth, nheight);

  [window setFrame:centeredFrame display:YES animate:NO];
}

/// Rescale the window frame size either up or down by one game width.
///
/// We calculate and set the window frame dimensions but don't set the 'scale' value.
///
- (void)scaleWindow:(int)direction
{
  NSWindow *window;
  NSRect    frameRect;
  NSRect    currentContentRect;
  CGFloat   scale;
  NSRect    newContentRect;
  NSRect    newFrameRect;
  CGFloat   x,y;
  NSRect    centeredFrame;

  window = [self window];
  frameRect = [window frame];
  currentContentRect = [window contentRectForFrameRect:frameRect];

  scale = gameView.scale;

  // Choose the new scale by rounding up/down
  if (direction > 0)
    scale = ceil(scale + 0.001); // bump up
  else if (direction < 0)
    scale = floor(scale - 0.001); // bump down
  else
    scale = 1.0; // reset

  // Clamp to 25% minimum to avoid the window becoming too small
  if (scale < 0.25)
    scale = 0.25;

  // Calculate the new window frame
  newContentRect.origin = currentContentRect.origin;
  newContentRect.size.width  = gameWidth  * scale + gameBorder * 2;
  newContentRect.size.height = gameHeight * scale + gameBorder * 2;
  newFrameRect = [window frameRectForContentRect:newContentRect];

  // Centre the window frame over its previous position
  x = frameRect.origin.x + (frameRect.size.width  - newFrameRect.size.width)  / 2;
  y = frameRect.origin.y + (frameRect.size.height - newFrameRect.size.height) / 2;
  centeredFrame = NSMakeRect(x, y, newFrameRect.size.width,
                                   newFrameRect.size.height);

  [window setFrame:centeredFrame display:YES animate:NO];
}

// -----------------------------------------------------------------------------

#pragma mark - (we're fed savegame locations via this)

- (void)setStartupGame:(NSURL *)saveGameURL
{
  saveGame = saveGameURL;
}

// -----------------------------------------------------------------------------

#pragma mark - Load/Save Games (ZXGameViewDelegate)

- (NSURL *)getStartupGame {
  return saveGame;
}

// -----------------------------------------------------------------------------

@end

// vim: ts=8 sts=2 sw=2 et
