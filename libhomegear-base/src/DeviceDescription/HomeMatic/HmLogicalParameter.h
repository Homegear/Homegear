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

#ifndef HMLOGICALPARAMETER_H_
#define HMLOGICALPARAMETER_H_

#include "../../Variable.h"
#include "../../Encoding/RapidXml/rapidxml.hpp"

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <limits>

using namespace rapidxml;

namespace BaseLib
{

class Obj;

namespace HmDeviceDescription
{

class ParameterOption
{
public:
	std::string id;
	bool isDefault = false;
	int32_t index = -1;

	ParameterOption() {}
	ParameterOption(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~ParameterOption() {}
};

class LogicalParameter
{
public:
	struct Type
	{
		enum Enum { none = 0x00, typeInteger = 0x01, typeBoolean = 0x02, typeString = 0x03, typeFloat = 0x04, typeEnum = 0x20, typeAction = 0x30 };
	};
	std::string unit;
	bool defaultValueExists = false;
	bool enforce = false;
	Type::Enum type = Type::none;

	LogicalParameter(BaseLib::Obj* baseLib);
	virtual ~LogicalParameter() {}
	static std::shared_ptr<LogicalParameter> fromXML(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual std::shared_ptr<Variable> getEnforceValue();
	virtual std::shared_ptr<Variable> getDefaultValue();
protected:
	BaseLib::Obj* _bl;
};

class LogicalParameterInteger : public LogicalParameter
{
public:
	int32_t min = -2147483648;
	int32_t max = 2147483647;
	int32_t defaultValue = 0;
	int32_t enforceValue = 0;
	std::unordered_map<std::string, int32_t> specialValues;

	LogicalParameterInteger(BaseLib::Obj* baseLib);
	LogicalParameterInteger(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~LogicalParameterInteger() {}
	virtual std::shared_ptr<Variable> getEnforceValue();
	virtual std::shared_ptr<Variable> getDefaultValue();
};

class LogicalParameterFloat : public LogicalParameter
{
public:
	double min = 1.175494351e-38f;
	double max = 3.40282347e+38f;
	double defaultValue = 0;
	double enforceValue = 0;
	std::unordered_map<std::string, double> specialValues;

	LogicalParameterFloat(BaseLib::Obj* baseLib);
	LogicalParameterFloat(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~LogicalParameterFloat() {}
	virtual std::shared_ptr<Variable> getEnforceValue();
	virtual std::shared_ptr<Variable> getDefaultValue();
};

class LogicalParameterEnum : public LogicalParameter
{
public:
	int32_t min = 0;
	int32_t max = 0;
	int32_t defaultValue = 0;
	int32_t enforceValue = 0;

	LogicalParameterEnum(BaseLib::Obj* baseLib);
	LogicalParameterEnum(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~LogicalParameterEnum() {}
	std::vector<ParameterOption> options;
	virtual std::shared_ptr<Variable> getEnforceValue();
	virtual std::shared_ptr<Variable> getDefaultValue();
};

class LogicalParameterBoolean : public LogicalParameter
{
public:
	bool min = false;
	bool max = true;
	bool defaultValue = false;
	bool enforceValue = false;

	LogicalParameterBoolean(BaseLib::Obj* baseLib);
	LogicalParameterBoolean(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~LogicalParameterBoolean() {}
	virtual std::shared_ptr<Variable> getEnforceValue();
	virtual std::shared_ptr<Variable> getDefaultValue();
};

class LogicalParameterString : public LogicalParameter
{
public:
	std::string min;
	std::string max;
	std::string defaultValue;
	std::string enforceValue;

	LogicalParameterString(BaseLib::Obj* baseLib);
	LogicalParameterString(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~LogicalParameterString() {}
	virtual std::shared_ptr<Variable> getEnforceValue();
	virtual std::shared_ptr<Variable> getDefaultValue();
};

class LogicalParameterAction : public LogicalParameter
{
public:
	bool min = false;
	bool max = true;
	bool defaultValue = false;
	bool enforceValue = false;

	LogicalParameterAction(BaseLib::Obj* baseLib);
	LogicalParameterAction(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~LogicalParameterAction() {}
	virtual std::shared_ptr<Variable> getEnforceValue();
	virtual std::shared_ptr<Variable> getDefaultValue();
};

}
}
#endif /* LOGICALPARAMETER_H_ */
