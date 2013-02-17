//
// CXDocument.h
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

#import <Cocoa/Cocoa.h>
#import "CXDisassembly.h"
#import "CXVirtualMachine.h"
#import "CXDisassembly.h"
#import "CXDebugUIController.h"

@interface CXDocument : NSDocument
{
	CXVirtualMachine* vm;
	CXDisassembly* disassembly;
	
	CXDebugUIController* debugUIController;
	NSURL* executableURL;
	NSMutableArray* arguments;
	NSMutableDictionary* environment;
	
	NSPopUpButton* entryPoints;
}

@property (readonly) NSURL* executableURL;
@property (readonly) NSArray* arguments;
@property (readonly) NSMutableDictionary* environment;

@property (readonly) CXVirtualMachine* vm;
@property (readonly) CXDisassembly* disassembly;
@property (readonly) CXDebugUIController* debug;

@property (assign) IBOutlet NSPopUpButton* entryPoints;

-(BOOL)readExecutableFromURL:(NSURL*)url error:(NSError**)error;
-(BOOL)readDebugDocumentFromURL:(NSURL*)url error:(NSError**)error;
-(BOOL)useExecutableURL:(NSURL*)url error:(NSError**)error;

-(IBAction)start:(id)sender;

@end
