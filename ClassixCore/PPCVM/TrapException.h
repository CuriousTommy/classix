//
// TrapException.h
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

#ifndef TRAPEXCEPTION_H
#define TRAPEXCEPTION_H

#include <exception>
#include <string>

namespace PPCVM
{
	class TrapException : public std::exception
	{
		std::string trapName;
	public:
		TrapException(const std::string& trapName);
		
		virtual const char* what() const noexcept;
	};
}

#endif