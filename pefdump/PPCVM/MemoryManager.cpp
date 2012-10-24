//
//  MemoryManager.cpp
//  pefdump
//
//  Created by Félix on 2012-10-23.
//  Copyright (c) 2012 Félix. All rights reserved.
//

#include "MemoryManager.h"
#include <algorithm>
#include <cassert>

namespace
{
	const uint32_t DeadPattern = 0xcccccccc;
	
	struct HeapHole
	{
		uint8_t* address;
		size_t size;
		
		HeapHole(uint8_t* address, size_t size)
		{
			this->address = address;
			this->size = size;
		}
		
		uint8_t* finish()
		{
			return address + size;
		}
	};
}

namespace PPCVM
{
	MemoryManager::Allocation::Allocation()
	{
		address = nullptr;
		size = 0;
		lockCount = 0;
	}
	
	MemoryManager::Allocation::Allocation(uint8_t* address, size_t size)
	{
		this->address = address;
		this->size = size;
		this->lockCount = 0;
	}
	
	MemoryManager::AddressLock::AddressLock(Allocation& allocation)
	{
		this->allocation = &allocation;
		this->allocation->lockCount++;
	}
	
	MemoryManager::AddressLock::AddressLock(AddressLock&& that)
	{
		this->allocation = that.allocation;
		that.allocation = nullptr;
	}
	
	uint8_t* MemoryManager::AddressLock::GetAddress()
	{
		return allocation->address;
	}
	
	const uint8_t* MemoryManager::AddressLock::GetAddress() const
	{
		return allocation->address;
	}
	
	MemoryManager::AddressLock::~AddressLock()
	{
		if (allocation != nullptr)
			allocation->lockCount--;
	}
	
	MemoryManager::MemoryManager()
	{
		begin = nullptr;
		nextEmptyBlock = nullptr;
		sizeLocked = false;
		currentSize = 0;
		ScribbleFreedMemory = false;
	}
	
	const uint8_t* MemoryManager::GetBaseAddress() const
	{
		if (!sizeLocked)
			throw std::logic_error("cannot get base address until size is locked");
		
		return begin;
	}
	
	size_t MemoryManager::GetReservedSize() const
	{
		return currentSize;
	}
	
	void MemoryManager::Reserve(size_t size)
	{
		uint8_t* oldAddress = begin;
		uint8_t* newAddress = static_cast<uint8_t*>(malloc(size));
		if (newAddress == nullptr)
		{
			// stay calm and PANIIIIIIC
			throw std::bad_alloc();
		}
		
		ptrdiff_t difference = newAddress - oldAddress;
		for (auto pair : allocations)
			pair.second.address += difference;
		
		memcpy(newAddress, oldAddress, std::min(size, currentSize));
		free(oldAddress);
		
		begin = newAddress;
		currentSize = size;
		nextEmptyBlock = oldAddress + difference;
	}
	
	void MemoryManager::ReserveAdditional(size_t size)
	{
		Reserve(currentSize + size);
	}
	
	void MemoryManager::LockReservedSize()
	{
		sizeLocked = true;
	}
	
	uint32_t MemoryManager::Allocate(size_t size)
	{
		bool heapCompacted = false;
		do
		{
			if (nextEmptyBlock + size <= begin + currentSize)
			{
				uint32_t index;
				if (recycledHandles.size() > 0)
				{
					index = recycledHandles.front();
					recycledHandles.pop_front();
				}
				else
				{
					// the +1 makes sure that the handle 0 is never used
					index = allocations.size() + 1;
				}
				
				Allocation& newAlloc = allocations[index];
				newAlloc = Allocation(nextEmptyBlock, size);
				allocationsByAddress[nextEmptyBlock] = index;
				
				nextEmptyBlock += size;
				return index;
			}
			else if (!heapCompacted)
			{
				CompactHeap();
				heapCompacted = true;
			}
			else if (!sizeLocked)
			{
				ReserveAdditional(size);
			}
			else
			{
				return 0;
			}
		}
		while (true);
	}
	
	void MemoryManager::Deallocate(uint32_t handle)
	{
		auto iter = allocations.find(handle);
		if (iter != allocations.end())
		{
			if (ScribbleFreedMemory)
			{
				uint8_t* address = iter->second.address;
				size_t size = iter->second.size;
				memset_pattern4(address, &DeadPattern, size);
			}
			recycledHandles.push_back(handle);
			allocations.erase(iter);
		}
	}
	
	void MemoryManager::Deallocate(uint8_t* address)
	{
		if (address == nullptr) return;
		
		auto iter = allocationsByAddress.find(address);
		if (iter == allocationsByAddress.end())
		{
			assert(!"Deallocating memory that was not allocated");
		}
		else
		{
			Deallocate(iter->second);
		}
	}
	
	uint8_t* MemoryManager::Lock(uint32_t handle)
	{
		auto iter = allocations.find(handle);
		if (iter != allocations.end())
		{
			iter->second.lockCount++;
			return iter->second.address;
		}
		return nullptr;
	}
	
	void MemoryManager::Unlock(uint32_t handle)
	{
		auto iter = allocations.find(handle);
		if (iter != allocations.end())
		{
			assert(iter->second.lockCount > 0 && "lock count underflow");
			iter->second.lockCount--;
		}
	}
	
	void MemoryManager::CompactHeap()
	{
		if (allocations.size() == 0) return;
		
		typedef std::pair<uint8_t*, uint8_t*> Relocation;
		std::list<HeapHole> holes;
		std::list<Relocation> relocated;
		
		auto iter = allocationsByAddress.begin();
		Allocation* lastAllocation = &allocations[iter->second];
		for (iter++; iter != allocationsByAddress.end(); iter++)
		{
			auto& allocation = allocations[iter->second];
			uint8_t* lastAllocationEnd = lastAllocation->address + lastAllocation->size;
			ptrdiff_t difference = allocation.address - lastAllocationEnd;
			if (difference != 0)
			{
				auto& lastHole = holes.back();
				if (lastHole.address == lastAllocationEnd)
				{
					lastHole.size += difference;
				}
				else
				{
					holes.emplace_back(lastAllocationEnd, difference);
				}
			}
			
			if (!allocation.IsLocked())
			{
				for (auto holeIter = holes.begin(); holeIter != holes.end(); iter++)
				{
					if (holeIter->size >= allocation.size || holeIter->finish() == allocation.address)
					{
						relocated.push_back(Relocation(allocation.address, holeIter->address));
						memmove(holeIter->address, allocation.address, allocation.size);
						holeIter->size -= allocation.size;
						if (holeIter->size == 0)
						{
							holes.erase(holeIter);
						}
						else
						{
							holeIter->address += allocation.size;
						}
						break;
					}
				}
			}
			lastAllocation = &allocations[iter->second];
		}
		
		nextEmptyBlock = lastAllocation->address + lastAllocation->size;
		
		for (auto iter = relocated.begin(); iter != relocated.end(); iter++)
		{
			auto& relocation = *iter;
			auto allocationIter = allocationsByAddress.find(relocation.first);
			auto allocation = allocationIter->second;
			assert(allocationIter != allocationsByAddress.end() && "how did we relocate a nonexistant allocation?");
			allocationsByAddress.erase(allocationIter);
			allocationsByAddress[relocation.second] = allocation;
		}
	}
	
	MemoryManager::~MemoryManager()
	{
		free(begin);
	}
}
