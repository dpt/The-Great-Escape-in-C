//
//  TheGreatEscapeView.h
//  The Great Escape
//
//  Created by David Thomas on 11/10/2014.
//  Copyright (c) 2014 David Thomas. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface TheGreatEscapeView : NSOpenGLView

- (IBAction)zoom:(id)sender;

- (void)keyUp:(NSEvent*)event;
- (void)keyDown:(NSEvent*)event;

@end
