//
//  ZXGameView.h
//  The Great Escape
//
//  Created by David Thomas on 11/10/2014.
//  Copyright (c) 2014-2020 David Thomas. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@protocol ZXGameViewDelegate <NSObject>
- (NSURL *) getStartupGame;
@end

@interface ZXGameView : NSOpenGLView <NSOpenSavePanelDelegate>
{
  IBOutlet NSMenuItem *snapMenuItem;
}

@property (nonatomic, retain) id<ZXGameViewDelegate> delegate;

#ifdef TGE_SAVES
- (IBAction)saveDocumentAs:(id)sender;
#endif /* TGE_SAVES */

- (void)start;
- (void)stop;

- (void)keyUp:(NSEvent *)event;
- (void)keyDown:(NSEvent *)event;
- (void)flagsChanged:(NSEvent *)event;

- (IBAction)setSpeed:(id)sender;
- (IBAction)toggleSnap:(id)sender;
- (IBAction)toggleSound:(id)sender;

- (void)getDefaultViewSize:(CGSize *)size border:(int *)border;
- (void)getSuggestedViewSize:(CGSize *)size border:(int *)border forSize:(CGSize)maximum direction:(int)direction;

@end

// vim: ts=8 sts=2 sw=2 et
