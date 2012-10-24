//
//  main.cpp
//  pefdump
//
//  Created by Félix on 2012-10-20.
//  Copyright (c) 2012 Félix. All rights reserved.
//

#include <cstdio>
#include <iostream>

#include "FragmentManager.h"
#include "PEFLibraryResolver.h"
#include "MemoryManager.h"
#include "MemoryManagerAllocator.h"

const char endline = '\n';

int main(int argc, const char * argv[])
{
	if (argc < 2)
	{
		std::cerr << "usage: pefdump pef-file" << std::endl;
		return 1;
	}
	
	try
	{
		PPCVM::MemoryManager memoryManager;
		PPCVM::MemoryManagerAllocator allocator(memoryManager);
		
		CFM::FragmentManager fragmentManager;
		CFM::PEFLibraryResolver pefResolver(&allocator, fragmentManager);
		fragmentManager.Resolvers.push_back(&pefResolver);
		
		fragmentManager.LoadContainer(argv[1]);
		
		memoryManager.ReserveAdditional(0x2000000); // 32 MB
		memoryManager.LockReservedSize();
		std::cout << "Successfully loaded container " << argv[1] << endline;
	}
	catch (std::logic_error error)
	{
		std::cerr << "operation failed: " << error.what() << endline;
	}
}

