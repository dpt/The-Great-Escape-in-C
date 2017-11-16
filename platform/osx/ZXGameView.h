//
//  ZXGameView.h
//  The Great Escape
//
//  Created by David Thomas on 11/10/2014.
//  Copyright (c) 2014-2017 David Thomas. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface ZXGameView : NSOpenGLView
{
  IBOutlet NSMenuItem *snapMenuItem;
}

/// Current game scale factor
@property (nonatomic, readonly) CGFloat scale;

- (void)stop;

- (void)keyUp:(NSEvent *)event;
- (void)keyDown:(NSEvent *)event;
- (void)flagsChanged:(NSEvent *)event;

- (IBAction)setSpeed:(id)sender;
- (IBAction)toggleSnap:(id)sender;
- (IBAction)toggleSound:(id)sender;

- (void)getGameWidth:(int *)width height:(int *)height border:(int *)border;

@end
