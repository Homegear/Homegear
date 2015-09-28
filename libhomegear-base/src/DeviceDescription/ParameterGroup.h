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

#ifndef PARAMETERGROUP_H_
#define PARAMETERGROUP_H_

#include "Parameter.h"
#include "Scenario.h"
#include "../Encoding/RapidXml/rapidxml.hpp"
#include <string>

using namespace rapidxml;

namespace BaseLib
{
namespace DeviceDescription
{

class Function;
class ParameterGroup;
class ConfigParameters;
class LinkParameters;
class Variables;

typedef std::shared_ptr<ParameterGroup> PParameterGroup;
typedef std::shared_ptr<ConfigParameters> PConfigParameters;
typedef std::shared_ptr<LinkParameters> PLinkParameters;
typedef std::shared_ptr<Variables> PVariables;
typedef std::map<uint32_t, std::vector<PParameter>> Lists;


class ParameterGroup
{
public:
	struct Type
	{
		enum Enum { none = 0, config = 1, variables = 2, link = 3 };
	};

	ParameterGroup(BaseLib::Obj* baseLib);
	ParameterGroup(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~ParameterGroup() {}

	//Attributes
	std::string id;
	int32_t memoryAddressStart = -1;
	int32_t memoryAddressStep = -1;

	//Elements
	Parameters parameters;
	std::vector<PParameter> parametersOrdered; //Needed for saving as the ordering of the parameters matters on some systems (e. g. HomeMatic CCU)
	Scenarios scenarios;

	//Helpers
	Lists lists;
	PParameter parameterGroupSelector;

	virtual Type::Enum type() = 0;
	static Type::Enum typeFromString(std::string& type);
	PParameter getParameter(std::string id);
	void getIndices(uint32_t startIndex, uint32_t endIndex, int32_t list, std::vector<PParameter>& result);
protected:
	BaseLib::Obj* _bl = nullptr;

	void parseAttributes(xml_node<>* node);
	void parseElements(xml_node<>* node);
};

class ConfigParameters : public ParameterGroup
{
public:
	ConfigParameters(BaseLib::Obj* baseLib);
	ConfigParameters(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~ConfigParameters() {}

	//Helpers
	Type::Enum type() { return Type::Enum::config; };
};

class Variables : public ParameterGroup
{
public:
	Variables(BaseLib::Obj* baseLib);
	Variables(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~Variables() {}

	//Helpers
	Type::Enum type() { return Type::Enum::variables; };
};

class LinkParameters : public ParameterGroup
{
public:
	LinkParameters(BaseLib::Obj* baseLib);
	LinkParameters(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~LinkParameters() {}

	//Helpers
	Type::Enum type() { return Type::Enum::link; };

	//Attributes
	int32_t peerChannelMemoryOffset = -1;
	int32_t channelMemoryOffset = -1;
	int32_t peerAddressMemoryOffset = -1;
	int32_t maxLinkCount = -1;
};

}
}

#endif
