//
// CXReverseAllocationDetails.cpp
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

#include "CXReverseAllocationDetails.h"
#include <sstream>
#include <cassert>

CXReverseAllocationDetails::CXReverseAllocationDetails(const std::string& name, uint32_t size)
: Common::AllocationDetails(name, size)
{ }

std::string CXReverseAllocationDetails::GetAllocationDetails(uint32_t offset) const
{
	assert(offset < Size() && "offset out of bounds");
	std::stringstream ss;
	ss << GetAllocationName() << " -" << Size() - offset;
	return ss.str();
}

Common::AllocationDetails* CXReverseAllocationDetails::ToHeapAlloc() const
{
	return new CXReverseAllocationDetails(*this);
}

CXReverseAllocationDetails::~CXReverseAllocationDetails()
{ }