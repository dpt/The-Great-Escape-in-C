//
//  AppDelegate.m
//  The Great Escape
//
//  Created by David Thomas on 11/09/2013.
//  Copyright (c) 2013-2017 David Thomas. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "ZXGameWindowController.h"

#import "AppDelegate.h"

@implementation AppDelegate
{
  /// Game window controllers live in here
  NSMutableArray *windowControllers;
}

- (void)awakeFromNib
{
  windowControllers = nil;
}

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification
{
  // Once the app's finished launching start a new game
  [self newGame:self];
}

- (IBAction)newGame:(id)sender
{
  ZXGameWindowController *game;

  if (windowControllers == nil)
    windowControllers = [[NSMutableArray alloc] initWithCapacity:1];

  game = [[ZXGameWindowController alloc] init];
  [windowControllers addObject:game];

  // Show the window, make it key and order it front
  // This will create the lazy loaded ZXGameWindow and its children
  [game showWindow:nil];
}

@end
