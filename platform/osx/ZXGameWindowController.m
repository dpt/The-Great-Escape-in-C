//
//  ZXGameWindowController.m
//  The Great Escape
//
//  Created by David Thomas on 03/08/2017.
//  Copyright © 2017-2020 David Thomas. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "ZXGameView.h"

#import "ZXGameWindowController.h"

@implementation ZXGameWindowController
{
  CGSize defaultGameSize;
  int    defaultGameBorder;
  NSURL *startupGame;
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
  [gameView getDefaultViewSize:&defaultGameSize border:&defaultGameBorder];

  [self resizeAndCentreGameWindow];

  gameView.delegate = self;
  [gameView start];

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(windowWillCloseNotification:)
                                               name:NSWindowWillCloseNotification
                                             object:[self window]];
}

// -----------------------------------------------------------------------------

#pragma mark - Public Interface

- (void)setStartupGame:(NSURL *)url
{
  startupGame = url;
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
  int       w, h;
  NSRect    screenFrame;
  CGFloat   x,y;
  NSRect    centeredFrame;

  window = [self window];

  borderedWidth  = defaultGameSize.width  + defaultGameBorder * 2;
  borderedHeight = defaultGameSize.height + defaultGameBorder * 2;
  contentRect = NSMakeRect(0, 0, borderedWidth, borderedHeight);
  frame = [window frameRectForContentRect:contentRect];
  w = frame.size.width;
  h = frame.size.height;

  screenFrame = [[NSScreen mainScreen] frame];
  x = screenFrame.origin.x + (screenFrame.size.width  - w) / 2;
  y = screenFrame.origin.y + (screenFrame.size.height - h) / 2;
  centeredFrame = NSMakeRect(x, y, w, h);

  [window setFrame:centeredFrame display:YES animate:NO];
}

/// Rescale the window frame size either up or down by one game width.
///
/// We calculate and set the window frame dimensions but don't set the 'scale' value directly - instead relying
/// on ZXGameView::setupDrawing to react to our changes.
///
- (void)scaleWindow:(int)direction
{
  NSWindow *window;
  NSRect    frameRect;
  CGSize    contentSize;
  int       gameBorder;
  CGFloat   x,y;
  NSRect    centeredFrame;

  window    = [self window];
  frameRect = [window frame];

  if (direction)
  {
    // Take the current content rect then ask the game to suggest a better size
    contentSize = [window contentRectForFrameRect:frameRect].size;
    [gameView getSuggestedViewSize:&contentSize
                            border:&gameBorder
                           forSize:contentSize
                         direction:direction];
  }
  else
  {
    // Get the basic game dimensions
    [gameView getDefaultViewSize:&contentSize border:&gameBorder];
  }

  // Add in the border
  contentSize.width  += gameBorder * 2;
  contentSize.height += gameBorder * 2;

  // Centre the window frame over its previous position
  x = frameRect.origin.x + (frameRect.size.width  - contentSize.width)  / 2;
  y = frameRect.origin.y + (frameRect.size.height - contentSize.height) / 2;
  centeredFrame = NSMakeRect(x, y, contentSize.width, contentSize.height);

  // Clamp to screen size
  centeredFrame = NSIntersectionRect(centeredFrame, [[window screen] visibleFrame]);

  [window setFrame:centeredFrame display:YES animate:NO];
}

// -----------------------------------------------------------------------------

#pragma mark - Load/Save Games (ZXGameViewDelegate)

- (NSURL *)getStartupGame
{
  return startupGame;
}

// -----------------------------------------------------------------------------

@end

// vim: ts=8 sts=2 sw=2 et
