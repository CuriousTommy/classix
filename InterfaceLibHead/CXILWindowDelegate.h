//
// CXILWindowDelegate.h
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

#import <Foundation/Foundation.h>
#import "CXILEventHandler.h"

typedef enum CXControlType
{
	CXControlButton,
	CXControlLabel,
} CXControlType;

// This may be kind of a misnomer. It's the delegate class that handles window stuff in lieu of CXILApplication.
@interface CXILWindowDelegate : NSObject

-(id)initWithMenuGate:(NSWindow*)window;

-(void)createWindow:(uint32_t)key withRect:(NSRect)rect surface:(IOSurfaceRef)surface title:(NSString*)title visible:(BOOL)visible behind:(uint32_t)behindKey;
-(void)createDialog:(uint32_t)key withRect:(NSRect)rect title:(NSString*)title visible:(BOOL)visible;
-(void)setDirtyRect:(CGRect)rect inWindow:(uint32_t)key;
-(void)destroyWindow:(uint32_t)windowID;

-(NSWindow*)windowForKey:(uint32_t)key;

-(id<CXILEventHandler>)startDragWindow:(uint32_t)windowKey mouseLocation:(NSPoint)location dragBounds:(NSRect)bounds;

-(uint32_t)keyOfFrontWindow;
-(uint32_t)keyOfWindow:(NSWindow*)window;
-(uint32_t)findWindowUnderPoint:(NSPoint)point area:(int16_t*)partCode;

@end
