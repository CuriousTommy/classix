//
//  NativeAllocator.h
//  pefdump
//
//  Created by Félix on 2012-10-24.
//  Copyright (c) 2012 Félix. All rights reserved.
//

#ifndef __pefdump__NativeAllocator__
#define __pefdump__NativeAllocator__

#include "IAllocator.h"

namespace Common
{
	class NativeAllocator : public virtual IAllocator
	{
	public:
		static NativeAllocator* Instance;
		
		NativeAllocator() = delete;
		NativeAllocator(const NativeAllocator& that) = delete;
		NativeAllocator(NativeAllocator&& that) = delete;
		
		virtual uint8_t* Allocate(size_t size);
		virtual void Deallocate(void* address);
		virtual ~NativeAllocator();
	};
}

#endif /* defined(__pefdump__NativeAllocator__) */
