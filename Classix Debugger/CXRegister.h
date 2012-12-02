//
// CXRegister.h
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

typedef enum
{
	CXRegisterGPR,
	CXRegisterFPR,
	CXRegisterCR,
	CXRegisterSPR
} CXRegisterType;

@interface CXRegister : NSObject
{
	NSString* name;
	CXRegisterType type;
	void* address;
}

@property (readonly) NSString* name;
@property (readonly) CXRegisterType type;
@property (copy) NSNumber* value;

+(id)GPRNumber:(int)number location:(uint32_t*)location;
+(id)FPRNumber:(int)number location:(double*)location;
+(id)CRNumber:(int)number location:(uint8_t*)location;
+(id)SPRName:(NSString*)name location:(uint32_t*)location;

-(id)initWithName:(NSString*)name address:(void*)address type:(CXRegisterType)size;

-(NSString*)description;

@end
