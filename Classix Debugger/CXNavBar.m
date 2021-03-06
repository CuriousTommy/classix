//
// CXNavBar.m
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

#import "CXNavBar.h"

@interface CXNavBar (Private)

-(void)popUpMenu:(NSMenu*)menu inRect:(NSRect)rect;
-(void)menuItemSelected:(NSMenuItem*)sender;
-(void)setMenuActions:(NSMenu*)menu;

@end

static NSImage* arrow;

@implementation CXNavBar

@synthesize selectionChanged;

+(void)initialize
{
	NSString* arrowPath = [[NSBundle mainBundle] pathForResource:@"arrow" ofType:@"png"];
	arrow = [[NSImage alloc] initWithContentsOfFile:arrowPath];
}

-(NSMenu*)menu
{
	return menu;
}

-(void)setMenu:(NSMenu *)_menu
{
	NSMenu* newMenu = [_menu copy];
	[menu release];
	menu = newMenu;
	
	selectedItem = nil;
	[self setMenuActions:menu];
	[self setNeedsDisplay:YES];
}

-(void)setMenuActions:(NSMenu *)submenu
{
	for (NSMenuItem* item in submenu.itemArray)
	{
		item.onStateImage = nil;
		if (item.hasSubmenu)
		{
			item.target = nil;
			item.action = NULL;
			[self setMenuActions:item.submenu];
		}
		else
		{
			item.target = self;
			item.action = @selector(menuItemSelected:);
			
			if (selectedItem == nil)
			{
				selectedItem = item;
				if (selectionChanged != nil)
					selectionChanged(self, item);
			}
		}
	}
}

-(void)menuItemSelected:(NSMenuItem *)sender
{
	selectedItem = sender;
	[self setNeedsDisplay:YES];
	if (selectionChanged != nil)
		selectionChanged(self, sender);
}

-(BOOL)acceptsFirstResponder
{
	return YES;
}

-(void)mouseDown:(NSEvent *)theEvent
{
	NSPoint location = [self convertPoint:theEvent.locationInWindow fromView:nil];
	for (NSValue* key in hitBoxes)
	{
		NSRect hitBox;
		[key getValue:&hitBox];
		if (NSPointInRect(location, hitBox))
		{
			[self popUpMenu:[hitBoxes objectForKey:key] inRect:hitBox];
			break;
		}
	}
}

-(void)drawRect:(NSRect)dirtyRect
{
	if (menu == nil || selectedItem == nil)
		return;
	
	NSRect bounds = self.bounds;
	NSDictionary* textDrawingAttributes = @{NSFontAttributeName : [NSFont systemFontOfSize:11]};
	
	// menu items
	NSMutableArray* selectionHierarchy = [NSMutableArray array];
	NSMenuItem* item = selectedItem;
	do
	{
		[selectionHierarchy insertObject:item atIndex:0];
		item = item.parentItem;
	} while (item != nil);
	
	NSMutableDictionary* mutableHitBoxes = [NSMutableDictionary dictionaryWithCapacity:selectionHierarchy.count];
	NSUInteger i = 0;
	CGFloat outputLocation = bounds.origin.x;
	const NSSize arrowSize = arrow.size;
	const CGFloat height = bounds.size.height - 8;
	const NSRect arrowBaseFrame = NSMakeRect(0, bounds.origin.y + 4, height * arrowSize.width / arrowSize.height, height);
	const NSRect arrowBounds = NSMakeRect(0, 0, arrowSize.width, arrowSize.height);
	for (NSMenuItem* item in selectionHierarchy)
	{
		outputLocation = round(outputLocation);
		if (i != 0)
		{
			outputLocation += 3;
			NSRect arrowFrame = arrowBaseFrame;
			arrowFrame.origin.x = outputLocation;
			[arrow drawInRect:arrowFrame fromRect:arrowBounds operation:NSCompositeSourceAtop fraction:1];
			outputLocation += arrowFrame.size.width + 7;
		}
		
		NSRect hitBox = NSMakeRect(outputLocation, 0, 0, bounds.size.height);
		NSImage* icon = item.image;
		if (icon != nil)
		{
			NSSize iconSize = icon.size;
			NSRect iconBounds = NSMakeRect(0, 0, iconSize.width, iconSize.height);
			NSRect iconFrame = NSMakeRect(outputLocation, bounds.origin.y + 4, height * iconSize.height / iconSize.width, height);
			[icon drawInRect:iconFrame fromRect:iconBounds operation:NSCompositeSourceAtop fraction:1];
			outputLocation += iconFrame.size.width + 5;
		}
		
		NSString* label = item.title;
		NSSize stringSize = [item.title sizeWithAttributes:textDrawingAttributes];
		NSPoint location = NSMakePoint(outputLocation, bounds.origin.y + (bounds.size.height - stringSize.height) / 2);
		[label drawAtPoint:location withAttributes:textDrawingAttributes];
		outputLocation += stringSize.width + 3;
		
		hitBox.size.width = outputLocation - hitBox.origin.x;
		[mutableHitBoxes setObject:item.menu forKey:[NSValue valueWithRect:hitBox]];
		
		i++;
	}
	[hitBoxes release];
	hitBoxes = [mutableHitBoxes copy];
}

-(void)dealloc
{
	self.menu = nil;
	self.selectionChanged = nil;
	[super dealloc];
}

#pragma mark -
#pragma mark Private

-(void)popUpMenu:(NSMenu*)menuToPop inRect:(NSRect)rect
{
	// location changes to align the menu on the bar
	// (this is right as of Mountain Lion on a non-Retina screen)
	rect.origin.x -= 11;
	rect.origin.y -= 1;
	
	NSPopUpButtonCell *popUpButtonCell = [[[NSPopUpButtonCell alloc] initTextCell:@"" pullsDown:NO] autorelease];
	[popUpButtonCell setMenu:menuToPop];
	[popUpButtonCell performClickWithFrame:rect inView:self];
}

@end
