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

#include "HmLogicalParameter.h"
#include "../../BaseLib.h"

namespace BaseLib
{
namespace HmDeviceDescription
{

ParameterOption::ParameterOption(BaseLib::Obj* baseLib, xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id") id = attributeValue;
		else if(attributeName == "default" && attributeValue == "true") isDefault = true;
		else if(attributeName == "index") index = Math::getNumber(attributeValue);
		else baseLib->out.printWarning("Warning: Unknown attribute for \"option\": " + attributeName);
	}
	for(xml_node<>* subnode = node->first_node(); subnode; subnode = subnode->next_sibling())
	{
		baseLib->out.printWarning("Warning: Unknown node in \"option\": " + std::string(subnode->name(), subnode->name_size()));
	}
}

std::shared_ptr<LogicalParameter> LogicalParameter::fromXML(BaseLib::Obj* baseLib, xml_node<>* node)
{
	std::shared_ptr<LogicalParameter> parameter;
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "type")
			{
				std::string attributeValue(attr->value());
				if(attributeValue == "option") parameter.reset(new LogicalParameterEnum(baseLib, node));
				else if(attributeValue == "integer") parameter.reset(new LogicalParameterInteger(baseLib, node));
				else if(attributeValue == "float") parameter.reset(new LogicalParameterFloat(baseLib, node));
				else if(attributeValue == "boolean") parameter.reset(new LogicalParameterBoolean(baseLib, node));
				else if(attributeValue == "string") parameter.reset(new LogicalParameterString(baseLib, node));
				else if(attributeValue == "action") parameter.reset(new LogicalParameterAction(baseLib, node));
				else baseLib->out.printWarning("Warning: Unknown logical parameter type: " + attributeValue);
			}
		}
		for(xml_node<>* subnode = node->first_node(); subnode; subnode = subnode->next_sibling())
		{
			if(std::string(subnode->name()) != "option" && std::string(subnode->name()) != "special_value") baseLib->out.printWarning("Warning: Unknown node in \"logical\": " + std::string(subnode->name(), subnode->name_size()));
		}
	}
    catch(const std::exception& ex)
    {
    	baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	baseLib->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return parameter;
}

LogicalParameter::LogicalParameter(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

std::shared_ptr<Variable> LogicalParameter::getEnforceValue()
{
	return std::shared_ptr<Variable>(new Variable(VariableType::tVoid));
}

std::shared_ptr<Variable> LogicalParameter::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(VariableType::tVoid));
}

LogicalParameterEnum::LogicalParameterEnum(BaseLib::Obj* baseLib) : LogicalParameter(baseLib)
{
	type = Type::Enum::typeEnum;
}

LogicalParameterEnum::LogicalParameterEnum(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalParameterEnum(baseLib)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "type") {}
			else if(attributeName == "unit") unit = attributeValue;
			else _bl->out.printWarning("Warning: Unknown attribute for \"logical\" with type enum: " + attributeName);
		}
		int32_t index = 0;
		for(xml_node<>* optionNode = node->first_node(); optionNode; optionNode = optionNode->next_sibling())
		{
			std::string nodeName(optionNode->name());
			if(nodeName == "option")
			{
				ParameterOption option(baseLib, optionNode);
				if(option.index > -1)
				{
					while((unsigned)option.index > options.size()) options.push_back(ParameterOption());
					index = option.index;
				}
				else option.index = index;
				options.push_back(option);
				if(options.back().isDefault)
				{
					defaultValueExists = true;
					defaultValue = index;
				}
				index++;
			}
			else baseLib->out.printWarning("Warning: Unknown node in \"logical\" with type enum: " + nodeName);
		}
		max = index - 1;
	}
    catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<Variable> LogicalParameterEnum::getEnforceValue()
{
	return std::shared_ptr<Variable>(new Variable(enforceValue));
}

std::shared_ptr<Variable> LogicalParameterEnum::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(defaultValue));
}

LogicalParameterInteger::LogicalParameterInteger(BaseLib::Obj* baseLib) : LogicalParameter(baseLib)
{
	type = Type::Enum::typeInteger;
}

LogicalParameterInteger::LogicalParameterInteger(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalParameterInteger(baseLib)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "type") {}
			else if(attributeName == "min") min = Math::getNumber(attributeValue);
			else if(attributeName == "max") max = Math::getNumber(attributeValue);
			else if(attributeName == "default")
			{
				defaultValue = Math::getNumber(attributeValue);
				defaultValueExists = true;
			}
			else if(attributeName == "unit") unit = attributeValue;
			else _bl->out.printWarning("Warning: Unknown attribute for \"logical\" with type integer: " + attributeName);
		}
		for(xml_node<>* logicalNode = node->first_node(); logicalNode; logicalNode = logicalNode->next_sibling())
		{
			std::string nodeName(logicalNode->name());
			if(nodeName == "special_value")
			{
				xml_attribute<>* attr1;
				xml_attribute<>* attr2;
				attr1 = logicalNode->first_attribute("id");
				attr2 = logicalNode->first_attribute("value");
				if(!attr1 || !attr2) continue;
				std::string valueString(attr2->value());
				specialValues[attr1->value()] = Math::getNumber(valueString);
			}
			else _bl->out.printWarning("Warning: Unknown node in \"logical\" with type integer: " + nodeName);
		}
	}
    catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<Variable> LogicalParameterInteger::getEnforceValue()
{
	return std::shared_ptr<Variable>(new Variable(enforceValue));
}

std::shared_ptr<Variable> LogicalParameterInteger::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(defaultValue));
}

LogicalParameterFloat::LogicalParameterFloat(BaseLib::Obj* baseLib) : LogicalParameter(baseLib)
{
	type = Type::Enum::typeFloat;
}

LogicalParameterFloat::LogicalParameterFloat(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalParameterFloat(baseLib)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "type") {}
			else if(attributeName == "min") min = Math::getDouble(attributeValue);
			else if(attributeName == "max") max = Math::getDouble(attributeValue);
			else if(attributeName == "default")
			{
				defaultValue = Math::getDouble(attributeValue);
				defaultValueExists = true;
			}
			else if(attributeName == "unit") unit = attributeValue;
			else _bl->out.printWarning("Warning: Unknown attribute for \"logical\" with type float: " + attributeName);
		}
		for(xml_node<>* logicalNode = node->first_node(); logicalNode; logicalNode = logicalNode->next_sibling())
		{
			std::string nodeName(logicalNode->name());
			if(nodeName == "special_value")
			{
				xml_attribute<>* attr1;
				xml_attribute<>* attr2;
				attr1 = logicalNode->first_attribute("id");
				attr2 = logicalNode->first_attribute("value");
				if(!attr1 || !attr2) continue;
				std::string valueString(attr2->value());
				specialValues[attr1->value()] = Math::getDouble(valueString);
			}
			else _bl->out.printWarning("Warning: Unknown node in \"logical\" with type float: " + nodeName);
		}
	}
    catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<Variable> LogicalParameterFloat::getEnforceValue()
{
	return std::shared_ptr<Variable>(new Variable(enforceValue));
}

std::shared_ptr<Variable> LogicalParameterFloat::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(defaultValue));
}

LogicalParameterBoolean::LogicalParameterBoolean(BaseLib::Obj* baseLib) : LogicalParameter(baseLib)
{
	type = Type::Enum::typeBoolean;
}

LogicalParameterBoolean::LogicalParameterBoolean(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalParameterBoolean(baseLib)
{
	try
	{
		type = Type::Enum::typeBoolean;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "default")
			{
				if(attributeValue == "true") defaultValue = true;
				defaultValueExists = true;
			}
			else if(attributeName == "unit") unit = attributeValue;
			else if(attributeName != "type") _bl->out.printWarning("Warning: Unknown attribute for \"logical\" with type boolean: " + attributeName);
		}
		for(xml_node<>* logicalNode = node->first_node(); logicalNode; logicalNode = logicalNode->next_sibling())
		{
			_bl->out.printWarning("Warning: Unknown node in \"logical\" with type boolean: " + std::string(logicalNode->name()));
		}
	}
    catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<Variable> LogicalParameterBoolean::getEnforceValue()
{
	return std::shared_ptr<Variable>(new Variable(enforceValue));
}

std::shared_ptr<Variable> LogicalParameterBoolean::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(defaultValue));
}

LogicalParameterString::LogicalParameterString(BaseLib::Obj* baseLib) : LogicalParameter(baseLib)
{
	type = Type::Enum::typeString;
}

LogicalParameterString::LogicalParameterString(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalParameterString(baseLib)
{
	try
	{
		type = Type::Enum::typeString;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "default")
			{
				defaultValue = attributeValue;
				defaultValueExists = true;
			}
			else if(attributeName == "unit") unit = attributeValue;
			else if(attributeName == "use_default_on_failure") {} //ignore, not necessary - all values are initialized
			else if(attributeName != "type") _bl->out.printWarning("Warning: Unknown attribute for \"logical\" with type string: " + attributeName);
		}
		for(xml_node<>* logicalNode = node->first_node(); logicalNode; logicalNode = logicalNode->next_sibling())
		{
			_bl->out.printWarning("Warning: Unknown node in \"logical\" with type string: " + std::string(logicalNode->name()));
		}
	}
    catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<Variable> LogicalParameterString::getEnforceValue()
{
	return std::shared_ptr<Variable>(new Variable(enforceValue));
}

std::shared_ptr<Variable> LogicalParameterString::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(defaultValue));
}

LogicalParameterAction::LogicalParameterAction(BaseLib::Obj* baseLib) : LogicalParameter(baseLib)
{
	type = Type::Enum::typeAction;
}

LogicalParameterAction::LogicalParameterAction(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalParameterAction(baseLib)
{
	try
	{
		type = Type::Enum::typeAction;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "unit") unit = attributeValue;
			else if(attributeName != "type") _bl->out.printWarning("Warning: Unknown attribute for \"logical\" with type boolean: " + attributeName);
		}
		for(xml_node<>* logicalNode = node->first_node(); logicalNode; logicalNode = logicalNode->next_sibling())
		{
			_bl->out.printWarning("Warning: Unknown node in \"logical\" with type action: " + std::string(logicalNode->name()));
		}
	}
    catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<Variable> LogicalParameterAction::getEnforceValue()
{
	return std::shared_ptr<Variable>(new Variable(enforceValue));
}

std::shared_ptr<Variable> LogicalParameterAction::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(defaultValue));
}

}
}
