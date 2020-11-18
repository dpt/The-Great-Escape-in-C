//
//  ZXGameWindowController.h
//  The Great Escape
//
//  Created by David Thomas on 03/08/2017.
//  Copyright © 2017-2020 David Thomas. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "ZXGameView.h"

@interface ZXGameWindowController : NSWindowController<ZXGameViewDelegate>
{
  IBOutlet ZXGameView *gameView;
}

- (void)setStartupGame:(NSURL *)url;

@end

// vim: ts=8 sts=2 sw=2 et
