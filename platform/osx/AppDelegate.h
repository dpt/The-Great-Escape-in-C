//
//  AppDelegate.h
//  The Great Escape
//
//  Created by David Thomas on 11/09/2013.
//  Copyright (c) 2013-2020 David Thomas. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>

- (IBAction)newGame:(id)sender;
#ifdef TGE_SAVES
- (IBAction)open:(id)sender;
#endif

@end

// vim: ts=8 sts=2 sw=2 et
