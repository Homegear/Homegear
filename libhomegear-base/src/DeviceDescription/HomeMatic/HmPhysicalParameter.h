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

#ifndef HMPHYSICALPARAMETER_H_
#define HMPHYSICALPARAMETER_H_

#include "../../Encoding/RapidXml/rapidxml.hpp"
#include "../../HelperFunctions/HelperFunctions.h"

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>

using namespace rapidxml;

namespace BaseLib
{

class Obj;

namespace HmDeviceDescription
{

class PhysicalParameterEvent
{
public:
	std::string frame;
	bool dominoEvent = false;
	int32_t dominoEventValue = 0;
	std::string dominoEventDelayID;

	PhysicalParameterEvent() {}
	virtual ~PhysicalParameterEvent() {}
};

class PhysicalParameterAddress
{
public:
	struct Operation
	{
		enum Enum { none, addition, substraction };
	};
	Operation::Enum operation = Operation::none;
	double index = 0;
	double step = 0;

	PhysicalParameterAddress() {}
	virtual ~PhysicalParameterAddress() {}
};

class SetRequestEx
{
public:
	struct BooleanOperator
	{
		enum Enum { none, e, g, l, ge, le };
	};

	std::string frame;
	BooleanOperator::Enum conditionOperator = BooleanOperator::Enum::none;
	int32_t value = 0;

	SetRequestEx() {}
	SetRequestEx(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~SetRequestEx() {}

	bool checkCondition(int32_t value);
};

class PhysicalParameter
{
public:
	struct Type
	{
		enum Enum { none, typeBoolean, typeInteger, typeString };
	};
	struct Interface
	{
		enum Enum { none, command, centralCommand, internal, config, configString, store, eeprom };
	};
	struct Endian
	{
		enum Enum { little, big };
	};
	Type::Enum type = Type::Enum::none;
	Interface::Enum interface = Interface::Enum::none;
	Endian::Enum endian = Endian::Enum::big;
	uint32_t list = 9999;
	double index = 0;
	uint32_t startIndex = 0;
	uint32_t endIndex = 0;
	bool sizeDefined = false;
	double size = 1.0;
	int32_t mask = -1;
	//int32_t readSize = -1;
	std::string valueID;
	//bool noInit = false;
	std::string getRequest;
	std::string getResponse;
	std::vector<std::shared_ptr<SetRequestEx>> setRequestsEx;
	std::string setRequest;

	//std::string counter;
	std::vector<std::shared_ptr<PhysicalParameterEvent>> eventFrames;
	std::vector<std::string> resetAfterSend;
	//bool isVolatile = false;
	std::string id;
	PhysicalParameterAddress address;

	PhysicalParameter();
	PhysicalParameter(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~PhysicalParameter() {}
};

}
}
#endif /* PHYSICALPARAMETER_H_ */
