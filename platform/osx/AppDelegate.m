//
//  AppDelegate.m
//  The Great Escape
//
//  Created by David Thomas on 11/09/2013.
//  Copyright (c) 2013-2020 David Thomas. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "ZXGameWindowController.h"

#import "AppDelegate.h"

@implementation AppDelegate
{
  /// Game window controllers live in here
  NSMutableArray *windowControllers;
}

// MARK: - Lifecycle

- (void)awakeFromNib
{
  windowControllers = nil;
}

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification
{
  // Once the app's finished launching, start a new game
  [self newGame:self];
}

// MARK: - IBAction handlers

- (IBAction)newGame:(id)sender
{
  [self createGame:nil];
}

// MARK: - Game

/// Create a new game with optional save game specified
/// @param url Save game URL, or nil
- (void)createGame:(NSURL *)url
{
  ZXGameWindowController *game;

  game = [[ZXGameWindowController alloc] init];

  if (windowControllers == nil)
    windowControllers = [[NSMutableArray alloc] initWithCapacity:1];
  [windowControllers addObject:game];

  if (url)
    [game setStartupGame:url];

  // Show the window, make it key and order it front
  // This will create the lazily-loaded ZXGameWindow and its children
  [game showWindow:nil];
}

#ifdef TGE_SAVES
- (IBAction)open:(id)sender
{
  NSOpenPanel *openPanel;

  openPanel = [NSOpenPanel openPanel];
  openPanel.allowedFileTypes = @[@"tge"];
  openPanel.allowsMultipleSelection = NO;
  openPanel.canChooseDirectories = NO;
  openPanel.canChooseFiles = YES;
  [openPanel beginWithCompletionHandler:^(NSInteger result) {
    if (result == NSModalResponseOK) {
      NSURL *url = [[openPanel URLs] objectAtIndex:0];
      [self createGame:url];
    }
  }];
}
#endif

@end

// vim: ts=8 sts=2 sw=2 et
