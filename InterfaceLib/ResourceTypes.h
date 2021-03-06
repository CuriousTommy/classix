//
// ResourceTypes.h
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

#ifndef Classix_ResourceTypes_h
#define Classix_ResourceTypes_h

#include <string>
#include <vector>
#include <cassert>
#include <type_traits>
#include "CommonDefinitions.h"
#include "ResourceManager.h"

namespace InterfaceLib
{
namespace Resources
{
	struct __attribute__((packed)) Str255
	{
		uint8_t length;
		char string[0];
		
		inline operator std::string() const
		{
			return std::string(string, length);
		}
	};
	
	struct Unconstructible
	{
		Unconstructible() = delete;
		Unconstructible(const Unconstructible&) = delete;
		Unconstructible(Unconstructible&&) = delete;
		Unconstructible operator=(const Unconstructible&) = delete;
	};
	
	struct __attribute__((packed)) cfrg : Unconstructible
	{
		static const Common::FourCharCode key;
		
		struct Member : Unconstructible
		{
			Common::UInt32 archType;
			Common::SInt32 updateLevel;
			Common::UInt32 currentVersion;
			Common::UInt32 oldDefVersion;
			Common::UInt32 appStackSize;
			Common::UInt16 appSubFolderID;
			uint8_t usage;
			uint8_t where;
			Common::UInt32 offset;
			Common::UInt32 length;
			Common::UInt32 reserved;
			Common::UInt32 memberSize;
			uint8_t titleLength;
			
			std::string GetTitle() const;
		};
		
		Common::SInt32 fields[7];
		Common::SInt32 memberCount;
		
		const Member* GetFirstMember() const;
	};
	
	struct __attribute__((packed)) BNDL : Unconstructible
	{
		static const Common::FourCharCode key;
		
		struct __attribute__((packed)) IconArray
		{
			Common::UInt32 type;
			Common::UInt16 iconCount;
		};
		
		Common::UInt32 appSignature;
		Common::UInt16 version;
		Common::UInt16 arraySize;
	};
	
	struct __attribute__((packed)) WIND : Unconstructible
	{
		static const Common::FourCharCode key;
		
		Rect windowRect;
		Common::UInt16 procID;
		Common::UInt16 visible;
		Common::UInt16 goAwayFlag;
		Common::UInt32 refCon;
		Str255 title;
	};
	
	struct __attribute__((packed)) MENU : Unconstructible
	{
		static const Common::FourCharCode key;
		
		struct Item : Unconstructible
		{
			std::string GetTitle() const;
			uint8_t GetIconNumber() const;
			uint8_t GetKeyEquivalent() const;
			uint8_t GetCharacterMark() const;
			uint8_t GetTextStyle() const;
			
			const Item* GetNextItem() const;
		};
		
		Common::UInt16 menuId;
		Common::UInt16 width;
		Common::UInt16 height;
		Common::UInt16 definitionProcedure;
		Common::UInt16 zero;
		Common::UInt32 enableFlags;
		uint8_t titleLength;
		
		std::string GetTitle() const;
		const Item* GetFirstItem() const;
	};
	
	struct __attribute__((packed)) DLOG : Unconstructible
	{
		static const Common::FourCharCode key;
		
		InterfaceLib::Rect rect;
		Common::SInt16 procID;
		uint8_t visibility;
		uint8_t _padding1;
		uint8_t goesAway;
		uint8_t _padding2;
		Common::UInt32 refCon;
		Common::SInt16 dialogId;
		uint8_t titleLength;
		
		std::string GetTitle() const;
	};
	
	struct __attribute__((packed)) DITL : Unconstructible
	{
		static const Common::FourCharCode key;
		
		class Enumerator
		{
			friend class DITL;
			const uint8_t* ptr;
			int16_t index;
			const DITL& ditl;
			
			Enumerator(const DITL& ditl);
			
		public:
			bool HasItem() const;
			void MoveNext();
			Control GetControl() const;
		};
		
		int16_t GetCount() const;
		Enumerator EnumerateControls() const;
	};
	
	struct __attribute__((packed)) ALRT : Unconstructible
	{
		static const Common::FourCharCode key;
		
		InterfaceLib::Rect bounds;
		Common::UInt16 ditl;
		
		// TODO complete struct
	};
}
}

#endif
