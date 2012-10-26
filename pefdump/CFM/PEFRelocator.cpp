//
//  PEFRelocator.cpp
//  pefdump
//
//  Created by Félix on 2012-10-24.
//  Copyright (c) 2012 Félix. All rights reserved.
//

#include "PEFRelocator.h"

namespace CFM
{
	PEFRelocator::PEFRelocator(FragmentManager& cfm, LoaderSection& loaderSection, InstantiableSection& section)
	: cfm(cfm), fixupSection(section), loaderSection(loaderSection)
	{
		data = reinterpret_cast<UInt32*>(section.Data);
	}
	
	void PEFRelocator::RelocBySectDWithSkip(uint32_t value)
	{
		int skipCount = (value >> 6) & 0xff;
		int relocCount = value & 0x3f;
		
		relocAddress += skipCount;
		for (int i = 0; i < relocCount; i++)
			Add(sectionD);
	}
	
	void PEFRelocator::RelocGroup(uint32_t value)
	{
		int subopcode = (value >> 9) & 0xf;
		int runLength = (value & 0x1ff) + 1;
		switch (subopcode)
		{
			case 0:
				for (int i = 0; i < runLength; i++)
					Add(sectionC);
				break;
				
			case 1:
				for (int i = 0; i < runLength; i++)
					Add(sectionD);
				break;
				
			case 2:
				for (int i = 0; i < runLength; i++)
				{
					Add(sectionC);
					Add(sectionD);
					relocAddress++;
				}
				break;
				
			case 3:
				for (int i = 0; i < runLength; i++)
				{
					Add(sectionC);
					Add(sectionD);
				}
				break;
				
			case 4:
				for (int i = 0; i < runLength; i++)
				{
					Add(sectionD);
					relocAddress++;
				}
				break;
				
			case 5:
				for (int i = 0; i < runLength; i++)
				{
					AddSymbol(importIndex);
					importIndex++;
				}
				break;
		}
	}
	
	void PEFRelocator::RelocSmallByIndex(uint32_t value)
	{
		int subOpcode = (value >> 9) & 0xf;
		int index = value & 0x1ff;
		RelocByIndex(subOpcode, index);
	}
	
	void PEFRelocator::RelocIncrementPosition(uint32_t value)
	{
		relocAddress += (value & 0xfff) + 1;
	}
	
	void PEFRelocator::RelocSmallRepeat(uint32_t value, Relocation::iterator current)
	{
		const int blockCount = ((value >> 8) & 0xff) + 1;
		const int repeatCount = (value & 0xf) + 1;
		Loop(current - blockCount, current, repeatCount);
	}
	
	void PEFRelocator::RelocSetPosition(uint32_t value)
	{
		relocAddress = value & 0xffffff;
	}
	
	void PEFRelocator::RelocLargeByImport(uint32_t value)
	{
		importIndex = value & 0xffffff;
		AddSymbol(importIndex);
		importIndex++;
	}
	
	void PEFRelocator::RelocLargeRepeat(uint32_t value, Relocation::iterator current)
	{
		const int repeatCount = 0x7fffff;
		const int blockCount = ((value >> 22) & 0xf) + 1;
		Loop(current - blockCount, current, repeatCount);
	}
	
	void PEFRelocator::RelocLargeSetOrBySection(uint32_t value)
	{
		int subOpcode = (value >> 22) & 0xf;
		int index = value & 0x7fffff;
		RelocByIndex(subOpcode, index);
	}
	
	void PEFRelocator::Execute(Relocation::iterator begin, Relocation::iterator end)
	{
		for (auto iter = begin; iter != end; iter++)
		{
			uint32_t value = *iter;
			if (value >> 14 == 0b00)
				RelocBySectDWithSkip(value);
			else if (value >> 13 == 0b010)
				RelocGroup(value);
			else if (value >> 13 == 0b011)
				RelocSmallByIndex(value);
			else if (value >> 12 == 0b1000)
				RelocIncrementPosition(value);
			else if (value >> 12 == 0b1001)
				RelocSmallRepeat(value, iter);
			else
			{
				iter++;
				value <<= 16;
				value |= *iter;
				if (value >> 26 == 0b101000)
					RelocSetPosition(value);
				else if (value >> 26 == 0b101001)
					RelocLargeByImport(value);
				else if (value >> 26 == 0b101100)
					RelocLargeRepeat(value, iter - 1);
				else if (value >> 26 == 0b101101)
					RelocLargeSetOrBySection(value);
				else
					throw std::logic_error("unknown instruction prefix");
			}
		}
	}
}