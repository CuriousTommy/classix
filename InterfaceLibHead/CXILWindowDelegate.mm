//
// CXILWindowDelegate.cpp
// Classix
//
// Copyright (C) 2012 Félix Cloutier
//
// This file is part of Classix.
//
// Classix is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// Classix is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// Classix. If not, see http://www.gnu.org/licenses/.
//

#import "CXILWindowDelegate.h"

@interface CXILWindowDelegate (Private)

-(NSWindow*)windowForID:(uint32_t)windowID;

@end

@implementation CXILWindowDelegate
{
	NSMutableDictionary* windows;
}

-(id)init
{
	if (!(self = [super init]))
		return nil;
	
	windows = [NSMutableDictionary dictionary];
	
	return self;
}

-(void)createWindow:(uint32_t)key withRect:(NSRect)rect title:(NSString *)title visible:(BOOL)visible behind:(uint32_t)windowKey
{
	NSUInteger windowStyle = NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask;
	NSRect actualRect = [NSWindow contentRectForFrameRect:rect styleMask:windowStyle];
	NSWindow* window = [[NSWindow alloc] initWithContentRect:actualRect styleMask:windowStyle backing:NSBackingStoreRetained defer:NO];
	window.title = title;
	
	if (visible)
	{
		if (windowKey == -1)
		{
			[window makeKeyAndOrderFront:self];
		}
		else
		{
			if (NSWindow* above = [self windowForID:windowKey])
			{
				[window orderWindow:NSWindowBelow relativeTo:above.orderedIndex];
				[window makeKeyWindow];
			}
			else
			{
				NSLog(@"*** trying to place window above dead reference; ordering front");
				[window makeKeyAndOrderFront:self];
			}
		}
	}
	
	[windows setObject:window forKey:@(key)];
}

-(void)destroyWindow:(uint32_t)windowID
{
	[windows removeObjectForKey:@(windowID)];
}

@end
