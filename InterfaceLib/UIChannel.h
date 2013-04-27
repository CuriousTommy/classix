//
// UIChannel.h
// Classix
//
// Copyright (C) 2013 Félix Cloutier
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

#ifndef __Classix__UIChannel__
#define __Classix__UIChannel__

#include <cstdint>
#include <type_traits>
#include <tuple>
#include <unistd.h>
#include "CommonDefinitions.h"

namespace InterfaceLib
{
	class UIChannel
	{
		union Pipe
		{
			int fd[2];
			struct
			{
				int read;
				int write;
			};
		};
		
		// reading to tuples
		// thanks Daniel Frey -- http://stackoverflow.com/q/16248828/variadic-read-tuples
		// the Indices::Next definition is pretty clever.
		template<typename TTupleType>
		struct TupleReader
		{
			int fd;
			TTupleType storage;
			
			template<size_t... IndexList>
			struct Indices
			{
				typedef Indices<IndexList..., sizeof...(IndexList)> Next;
			};
			
			template<size_t N>
			struct MakeIndices
			{
				typedef typename Indices<N - 1>::Next Type;
			};
			
			template<size_t... Ns>
			inline void ReadImpl(const Indices<Ns...>&)
			{
				PACK_EXPAND(::read(fd, &std::get<Ns>(storage), sizeof(typename std::tuple_element<Ns, TTupleType>::type)));
			}
			
			TupleReader(int fd) : fd(fd)
			{ }
			
			inline void Read()
			{
				ReadImpl(typename MakeIndices<std::tuple_size<TTupleType>::value>::Type());
			}
		};
		
		// utilities
		template<typename T>
		char WriteToPipe(const T& argument)
		{
			::write(write.write, &argument, sizeof argument);
			return 0;
		}
		
		template<typename T>
		T ReturnNonVoid(const uint8_t* buffer)
		{
			return *reinterpret_cast<const T*>(buffer);
		}
		
		// fields
		Pipe read;
		Pipe write;
		pid_t head;
		
	public:
		UIChannel();
		
		template<typename TReturnType, typename... TArgument>
		TReturnType PerformAction(IPCMessage message, TArgument&&... argument)
		{
			static_assert(!std::is_pointer<TReturnType>::value, "Using DoMessage with a pointer type");
			static_assert(std::is_void<TReturnType>::value || std::is_trivially_copy_constructible<TReturnType>::value,
						  "Using DoMessage with a non-trivial type");
			
			char doneReference[4] = {'D', 'O', 'N', 'E'};
			
			::write(write.write, &message, sizeof message);
			PACK_EXPAND(WriteToPipe(argument));
			::write(write.write, doneReference, sizeof doneReference);
			
			uint8_t response[sizeof(TReturnType)];
			if (!std::is_void<TReturnType>::value)
				::read(read.read, response, sizeof response);
			
			char done[4];
			::read(read.read, done, sizeof done);
			if (memcmp(done, doneReference, sizeof doneReference) != 0)
				throw std::logic_error("Wrong return type for action");
			
			return ReturnNonVoid<TReturnType>(response);
		}
		
		template<typename TTupleType, typename... TArgument>
		TTupleType PerformComplexAction(IPCMessage message, TArgument&&... argument)
		{
			char doneReference[4] = {'D', 'O', 'N', 'E'};
			
			::write(write.write, &message, sizeof message);
			PACK_EXPAND(WriteToPipe(argument));
			::write(write.write, doneReference, sizeof doneReference);
			
			TupleReader<TTupleType> reader(read.read);
			reader.Read();
			
			char done[4];
			::read(read.read, done, sizeof done);
			if (memcmp(done, doneReference, sizeof doneReference) != 0)
				throw std::logic_error("Wrong return type for action");
			
			return reader.storage;
		}
		
		~UIChannel();
	};
}

#endif /* defined(__Classix__UIChannel__) */
