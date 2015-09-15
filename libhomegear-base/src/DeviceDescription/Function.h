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

#ifndef DEVICEFUNCTION_H_
#define DEVICEFUNCTION_H_

#include "ParameterGroup.h"
#include "../Encoding/RapidXml/rapidxml.hpp"
#include <string>
#include <set>

using namespace rapidxml;

namespace BaseLib
{
namespace DeviceDescription
{

class Function;

typedef std::set<std::string> LinkFunctionTypes;
typedef std::shared_ptr<Function> PFunction;
typedef std::map<uint32_t, PFunction> Functions;

class Function
{
public:
	struct Direction
	{
		enum Enum { none = 0, sender = 1, receiver = 2 };
	};

	Function(BaseLib::Obj* baseLib);
	Function(BaseLib::Obj* baseLib, xml_node<>* node, uint32_t& channel);
	virtual ~Function() {}

	//Attributes
	uint32_t channel = 0;
	std::string type;
	uint32_t channelCount = 1;

	//Properties
	bool encryptionEnabledByDefault = false;
	bool visible = true;
	bool deletable = false;
	bool internal = false;
	std::string countFromVariable;
	int32_t dynamicChannelCountIndex = -1;
	double dynamicChannelCountSize = 1;
	int32_t physicalChannelIndexOffset = 0;
	bool grouped = false;
	Direction::Enum direction = Direction::Enum::none;
	bool forceEncryption = false;
	std::string defaultLinkScenarioElementId;
	std::string defaultGroupedLinkScenarioElementId1;
	std::string defaultGroupedLinkScenarioElementId2;
	bool hasGroup = false;
	std::string groupId;
	LinkFunctionTypes linkSenderFunctionTypes;
	LinkFunctionTypes linkReceiverFunctionTypes;

	//Elements
	std::string configParametersId;
	std::string variablesId;
	std::string linkParametersId;
	PFunction alternativeFunction;

	//Helpers
	PParameter parameterGroupSelector;
	PConfigParameters configParameters;
	PVariables variables;
	PLinkParameters linkParameters;

	bool parameterSetDefined(ParameterGroup::Type::Enum type);
	PParameterGroup getParameterGroup(ParameterGroup::Type::Enum type);
protected:
	BaseLib::Obj* _bl = nullptr;
};
}
}

#endif
