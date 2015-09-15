/* Copyright 2013-2015 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef DEVICEPARAMETERPHYSICAL_H_
#define DEVICEPARAMETERPHYSICAL_H_

#include "../Encoding/RapidXml/rapidxml.hpp"
#include <string>
#include <memory>

using namespace rapidxml;

namespace BaseLib
{

class Obj;

namespace DeviceDescription
{

class PhysicalInteger;
class PhysicalBoolean;
class PhysicalString;

typedef std::shared_ptr<PhysicalInteger> PPhysicalInteger;
typedef std::shared_ptr<PhysicalBoolean> PPhysicalBoolean;
typedef std::shared_ptr<PhysicalString> PPhysicalString;

class IPhysical
{
public:
	struct Type
	{
		enum Enum { none = 0x00, tInteger = 0x01, tBoolean = 0x02, tString = 0x03 };
	};

	struct MemoryIndexOperation
	{
		enum Enum { none, addition, subtraction };
	};

	struct OperationType
	{
		enum Enum { none = 0, command = 1, centralCommand = 2, internal = 3, config = 4, configString = 5, store = 6, memory = 7 };
	};

	struct Endianess
	{
		enum Enum { unset = 0, big = 1, little = 2 };
	};

	std::string groupId;
	Type::Enum type = Type::none;
	OperationType::Enum operationType = OperationType::none;
	Endianess::Enum endianess = Endianess::big;
	double index = 0;
	bool sizeDefined = false;
	double size = 1.0;
	int32_t mask = -1;
	int32_t list = -1;
	double memoryIndex = 0;
	MemoryIndexOperation::Enum memoryIndexOperation = MemoryIndexOperation::Enum::none;
	double memoryChannelStep = 0;

	IPhysical(BaseLib::Obj* baseLib, Type::Enum type);
	IPhysical(BaseLib::Obj* baseLib, Type::Enum type, xml_node<>* node);
	virtual ~IPhysical() {}

	//Helpers
	uint32_t startIndex = 0;
	uint32_t endIndex = 0;
protected:
	BaseLib::Obj* _bl = nullptr;
};

class PhysicalInteger : public IPhysical
{
public:
	PhysicalInteger(BaseLib::Obj* baseLib);
	PhysicalInteger(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~PhysicalInteger() {}
};

class PhysicalBoolean : public IPhysical
{
public:
	PhysicalBoolean(BaseLib::Obj* baseLib);
	PhysicalBoolean(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~PhysicalBoolean() {}
};

class PhysicalString : public IPhysical
{
public:
	PhysicalString(BaseLib::Obj* baseLib);
	PhysicalString(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~PhysicalString() {}
};

}
}

#endif
