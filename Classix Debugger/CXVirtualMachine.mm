//
//  CXVirtualMachine.m
//  Classix
//
//  Created by Félix on 2012-11-25.
//  Copyright (c) 2012 Félix. All rights reserved.
//

#import "CXVirtualMachine.h"
#import "CXRegister.h"

#include "MachineState.h"
#include "FragmentManager.h"
#include "NativeAllocator.h"
#include "PEFLibraryResolver.h"
#include "Interpreter.h"
#include "DlfcnLibraryResolver.h"
#include "FancyDisassembler.h"
#include "CXObjcDisassemblyWriter.h"
#include <unordered_set>

NSNumber* CXVirtualMachineGPRKey = @(CXRegisterGPR);
NSNumber* CXVirtualMachineFPRKey = @(CXRegisterFPR);
NSNumber* CXVirtualMachineSPRKey = @(CXRegisterSPR);
NSNumber* CXVirtualMachineCRKey = @(CXRegisterCR);

NSString* CXErrorDomain = @"Classix Error Domain";
NSString* CXErrorFilePath = @"File URL";

struct ClassixCoreVM
{
	Common::IAllocator* allocator;
	PPCVM::MachineState state;
	CFM::FragmentManager cfm;
	CFM::PEFLibraryResolver pefResolver;
	ClassixCore::DlfcnLibraryResolver dlfcnResolver;
	PPCVM::Execution::Interpreter interp;
	PEF::Container* container;
	
	std::unordered_set<intptr_t> breakpoints;
	intptr_t nextPC;
	
	ClassixCoreVM(Common::IAllocator* allocator)
	: allocator(allocator)
	, state()
	, cfm()
	, pefResolver(allocator, cfm)
	, dlfcnResolver(allocator)
	, interp(allocator, &state)
	, container(nullptr)
	, nextPC(0)
	{
		dlfcnResolver.RegisterLibrary("StdCLib");
		cfm.LibraryResolvers.push_back(&pefResolver);
		cfm.LibraryResolvers.push_back(&dlfcnResolver);
	}
	
	bool LoadContainer(const std::string& path)
	{
		using namespace PEF;
		
		if (cfm.LoadContainer(path))
		{
			CFM::SymbolResolver* resolver = cfm.GetSymbolResolver(path);
			if (CFM::PEFSymbolResolver* pefResolver = dynamic_cast<CFM::PEFSymbolResolver*>(resolver))
			{
				container = &pefResolver->GetContainer();
				auto main = pefResolver->GetMainAddress();
				if (main.Universe == CFM::SymbolUniverse::PowerPC)
				{
					nextPC = main.Address;
					
					auto& section = container->GetSection(SectionForPC());
					if (section.GetSectionType() != SectionType::Code && section.GetSectionType() != SectionType::ExecutableData)
					{
						// then it's a transition vector
						const TransitionVector* vector = allocator->ToPointer<TransitionVector>(nextPC);
						nextPC = vector->EntryPoint;
					}
					return true;
				}
			}
		}
		return false;
	}
	
	int SectionForPC() const
	{
		void* ptr = allocator->ToPointer<void>(nextPC);
		for (int i = 0; i < container->Size(); i++)
		{
			auto& section = container->GetSection(i);
			if (section.Data <= ptr && section.Data + section.Size() > ptr)
				return i;
		}
		return -1;
	}
};

@interface CXVirtualMachine (Private)

-(void)refreshRegisters:(const PPCVM::MachineState*)oldState;

@end

@implementation CXVirtualMachine

@synthesize allRegisters = registers;
@synthesize breakpoints;
@synthesize pc;

-(NSArray*)gpr
{
	return [registers objectForKey:CXVirtualMachineGPRKey];
}

-(NSArray*)fpr
{
	return [registers objectForKey:CXVirtualMachineFPRKey];
}

-(NSArray*)spr
{
	return [registers objectForKey:CXVirtualMachineSPRKey];
}

-(NSArray*)cr
{
	return [registers objectForKey:CXVirtualMachineCRKey];
}

-(id)init
{
	if (!(self = [super init]))
		return nil;
	
	vm = new ClassixCoreVM(Common::NativeAllocator::Instance);
	
	NSMutableArray* gpr = [NSMutableArray arrayWithCapacity:32];
	NSMutableArray* fpr = [NSMutableArray arrayWithCapacity:32];
	NSMutableArray* cr = [NSMutableArray arrayWithCapacity:8];
	NSMutableArray* spr = [NSMutableArray array];
	breakpoints = [[NSMutableArray alloc] init];
	changedRegisters = [[NSMutableSet alloc] initWithCapacity:75];
	
	for (int i = 0; i < 32; i++)
	{
		CXRegister* r = [CXRegister GPRNumber:i location:&vm->state.gpr[i]];
		CXRegister* fr = [CXRegister FPRNumber:i location:&vm->state.fpr[i]];
		[gpr addObject:r];
		[fpr addObject:fr];
	}
	
	for (int i = 0; i < 8; i++)
	{
		CXRegister* reg = [CXRegister CRNumber:i location:&vm->state.cr[i]];
		[cr addObject:reg];
	}
	
	[spr addObject:[CXRegister SPRName:@"xer" location:&vm->state.xer]];
	[spr addObject:[CXRegister SPRName:@"lr" location:&vm->state.lr]];
	[spr addObject:[CXRegister SPRName:@"ctr" location:&vm->state.ctr]];
	
	registers = [@{
		CXVirtualMachineGPRKey: gpr,
		CXVirtualMachineFPRKey: fpr,
		CXVirtualMachineSPRKey: spr,
		CXVirtualMachineCRKey: cr
	} retain];
	
	return self;
}

-(BOOL)loadClassicExecutable:(NSString *)executablePath arguments:(NSArray*)args environment:(NSDictionary*)env error:(NSError **)error
{
	// TODO check that we haven't already loaded an executable
	std::string path = [executablePath UTF8String];
	
	if (!vm->LoadContainer(path))
	{
		if (error != nullptr)
			*error = [NSError errorWithDomain:CXErrorDomain code:CXErrorCodeFileNotLoadable userInfo:@{CXErrorFilePath: executablePath}];
		return NO;
	}
	
	// do we have a main symbol? let's hope so
	auto resolver = vm->cfm.GetSymbolResolver(path);
	auto main = resolver->GetMainAddress();
	if (main.Universe != CFM::SymbolUniverse::PowerPC)
	{
		if (error != nullptr)
			*error = [NSError errorWithDomain:CXErrorDomain code:CXErrorCodeFileNotExecutable userInfo:@{CXErrorFilePath: executablePath}];
		return NO;
	}
	
	// from this point, nothing else should fail...
	auto mainVector = vm->allocator->ToPointer<const PEF::TransitionVector>(main.Address);
	
	// set argv and envp
	std::vector<size_t> argvOffsets;
	std::vector<size_t> envpOffsets;
	std::vector<char> argvStrings;
	std::vector<char> envpStrings;
	
	for (NSString* arg in args)
	{
		const char* begin = arg.UTF8String;
		const char* end = begin + strlen(begin) + 1;
		argvOffsets.push_back(argvStrings.size());
		argvStrings.insert(argvStrings.end(), begin, end);
	}
	
	for (NSString* key in env)
	{
		const char* keyBegin = key.UTF8String;
		const char* keyEnd = keyBegin + strlen(keyBegin);
		envpOffsets.push_back(envpStrings.size());
		envpStrings.insert(envpStrings.end(), keyBegin, keyEnd);
		envpStrings.push_back('=');
		
		NSString* value = [env objectForKey:key];
		const char* valueBegin = value.UTF8String;
		const char* valueEnd = valueBegin + strlen(valueBegin) + 1;
		envpStrings.insert(envpStrings.end(), valueBegin, valueEnd);
	}
	
	Common::AutoAllocation argvStringsArea = vm->allocator->AllocateAuto("Argument Values", argvStrings.size());
	Common::AutoAllocation envpStringsArea = vm->allocator->AllocateAuto("Environment Variables", envpStrings.size());
	Common::AutoAllocation argvAlloc = vm->allocator->AllocateAuto("argv", argvOffsets.size() * sizeof(uint32_t));
	Common::AutoAllocation envpAlloc = vm->allocator->AllocateAuto("envp", (envpOffsets.size() + 1) * sizeof(uint32_t));
	
	memcpy(*argvStringsArea, argvStrings.data(), argvStrings.size());
	memcpy(*envpStringsArea, envpStrings.data(), envpStrings.size());
	
	Common::UInt32* argvValues = argvAlloc.ToArray<Common::UInt32>(argvOffsets.size());
	for (size_t i = 0; i < argvOffsets.size(); i++)
		argvValues[i] = argvStringsArea.GetVirtualAddress() + argvOffsets[i];
	
	Common::UInt32* envpValues = envpAlloc.ToArray<Common::UInt32>(envpOffsets.size() + 1);
	for (size_t i = 0; i < envpOffsets.size(); i++)
		envpValues[i] = envpStringsArea.GetVirtualAddress() + envpOffsets[i];
	envpValues[envpOffsets.size()] = 0;
	
	Common::AutoAllocation stack = vm->allocator->AllocateAuto("Stack", 0x100000);
	
	// set the stage
	vm->state.r0 = 0;
	vm->state.r1 = stack.GetVirtualAddress();
	vm->state.r2 = mainVector->TableOfContents;
	vm->state.r3 = args.count;
	vm->state.r4 = argvAlloc.GetVirtualAddress();
	vm->state.r5 = envpAlloc.GetVirtualAddress();
	
	NSArray* gpr = self.gpr;
	NSArray* initialRegisters = @[
		[gpr objectAtIndex:1],
		[gpr objectAtIndex:2],
		[gpr objectAtIndex:3],
		[gpr objectAtIndex:4],
		[gpr objectAtIndex:5],
	];
	
	[changedRegisters removeAllObjects];
	[changedRegisters addObjectsFromArray:initialRegisters];
	
	self.pc = mainVector->EntryPoint;
	
	return YES;
}

-(NSValue*)fragmentManager
{
	CFM::FragmentManager* cfm = &vm->cfm;
	return [NSValue value:&cfm withObjCType:@encode(typeof cfm)];
}

-(NSValue*)allocator
{
	Common::IAllocator* allocator = vm->allocator;
	return [NSValue value:&allocator withObjCType:@encode(typeof allocator)];
}

-(IBAction)run:(id)sender
{
	std::unordered_set<const void*> cppBreakpoints;
	for (NSNumber* number in breakpoints)
	{
		const void* address = vm->allocator->ToPointer<const void>(number.unsignedIntValue);
		cppBreakpoints.insert(address);
	}
	
	const void* eip = vm->allocator->ToPointer<const void>(pc);
	PPCVM::MachineState oldState = vm->state;
	eip = vm->interp.ExecuteUntil(eip, cppBreakpoints);
	
	self.pc = vm->allocator->ToIntPtr(eip);
	[self refreshRegisters:&oldState];
}

-(IBAction)stepOver:(id)sender
{
	NSLog(@"*** step over is not currently supported; stepping into instead");
	[self stepInto:sender];
}

-(IBAction)stepInto:(id)sender
{
	PPCVM::MachineState oldState = vm->state;
	const void* eip = vm->allocator->ToPointer<const void>(pc);
	eip = vm->interp.ExecuteOne(eip);
	
	self.pc = vm->allocator->ToIntPtr(eip);
	[self refreshRegisters:&oldState];
}

-(void)runTo:(uint32_t)location
{
	std::unordered_set<const void*> until = {vm->allocator->ToPointer<const void>(location)};
	const void* eip = vm->allocator->ToPointer<const void>(pc);
	PPCVM::MachineState oldState = vm->state;
	eip = vm->interp.ExecuteUntil(eip, until);
	
	self.pc = vm->allocator->ToIntPtr(eip);
	[self refreshRegisters:&oldState];
}

-(void)dealloc
{
	delete vm;
	[registers release];
	[breakpoints release];
	[changedRegisters release];
	[super dealloc];
}

#pragma mark -
#pragma mark NSOutlineView stuff
-(BOOL)outlineView:(NSOutlineView *)outlineView isGroupItem:(id)item
{
	return [outlineView parentForItem:item] == nil;
}

-(NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
	if (item == nil) return registers.count;
	if ([item respondsToSelector:@selector(count)]) return [item count];
	return 0;
}

-(BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
{
	return [self outlineView:outlineView numberOfChildrenOfItem:item] != 0;
}

-(id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item
{
	if (item == nil)
		return [registers objectForKey:@(index)];
	
	return [item objectAtIndex:index];
}

-(void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
	if ([changedRegisters containsObject:item])
	{
		[cell setTextColor:NSColor.redColor];
	}
	else
	{
		[cell setTextColor:NSColor.blackColor];
	}
}

-(id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
	if ([outlineView parentForItem:item] == nil)
	{
		if ([tableColumn.identifier isEqualToString:@"Register"])
		{
			static NSString* headers[] = {@"GPR", @"FPR", @"CR", @"SPR"};
			int index = [[[registers allKeysForObject:item] objectAtIndex:0] intValue];
			return headers[index];
		}
		return nil;
	}
	
	NSString* identifier = tableColumn.identifier;
	if ([identifier isEqualToString:@"Register"])
		return [item name];
	else if ([identifier isEqualToString:@"Value"])
		return [item value];
	
	return nil;
}

-(void)outlineView:(NSOutlineView *)outlineView setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
	if ([item isKindOfClass:CXRegister.class])
	{
		NSString* value = object;
		CXRegister* regObject = item;
		const char* numberString = value.UTF8String;
		char* end;
		
		if (value.length > 2 && [value characterAtIndex:0] == '0')
		{
			long result;
			switch ([value characterAtIndex:1])
			{
				case 'b': result = strtol(numberString + 2, &end, 2); break;
				case 'x': result = strtol(numberString + 2, &end, 16); break;
				default: result = strtol(numberString + 1, &end, 8); break;
			}
			
			if (*end != 0)
			{
				NSLog(@"*** trying to set %@ to invalid string %@", regObject.name, value);
				return;
			}
			regObject.value = @(result);
		}
		else
		{
			NSNumberFormatter* formatter = [[[NSNumberFormatter alloc] init] autorelease];
			formatter.numberStyle = NSNumberFormatterDecimalStyle;
			NSNumber* result = [formatter numberFromString:value];
			if (result == nil)
			{
				NSLog(@"*** trying to set %@ to invalid string %@", regObject.name, value);
				return;
			}
			regObject.value = result;
		}
	}
}

#pragma mark -
#pragma mark Private
-(void)refreshRegisters:(const PPCVM::MachineState *)oldState
{
	[changedRegisters removeAllObjects];
	// notify observers for value changes
	for (int i = 0; i < 8; i++)
	{
		if (oldState->cr[i] != vm->state.cr[i])
		{
			CXRegister* cr = [self.cr objectAtIndex:i];
			cr.value = @(vm->state.cr[i]);
			[changedRegisters addObject:cr];
		}
	}
	
	for (int i = 0; i < 32; i++)
	{
		if (oldState->gpr[i] != vm->state.gpr[i])
		{
			CXRegister* gpr = [self.gpr objectAtIndex:i];
			gpr.value = @(vm->state.gpr[i]);
			[changedRegisters addObject:gpr];
		}
		
		if (oldState->fpr[i] != vm->state.fpr[i])
		{
			CXRegister* fpr = [self.fpr objectAtIndex:i];
			fpr.value = @(vm->state.fpr[i]);
			[changedRegisters addObject:fpr];
		}
	}
	
	if (oldState->xer != vm->state.xer)
	{
		CXRegister* xer = [self.spr objectAtIndex:CXVirtualMachineSPRXERIndex];
		xer.value = @(vm->state.xer);
		[changedRegisters addObject:xer];
	}
	
	if (oldState->ctr != vm->state.ctr)
	{
		CXRegister* ctr = [self.spr objectAtIndex:CXVirtualMachineSPRCTRIndex];
		ctr.value = @(vm->state.ctr);
		[changedRegisters addObject:ctr];
	}
	
	if (oldState->lr != vm->state.lr)
	{
		CXRegister* lr = [self.spr objectAtIndex:CXVirtualMachineSPRLRIndex];
		lr.value = @(vm->state.lr);
		[changedRegisters addObject:lr];
	}
}

@end