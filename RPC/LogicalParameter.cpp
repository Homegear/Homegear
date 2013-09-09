/* Copyright 2013 Sathya Laufer
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

#include "LogicalParameter.h"
#include "../GD.h"
#include "../HelperFunctions.h"

namespace RPC {

ParameterOption::ParameterOption(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id") id = attributeValue;
		else if(attributeName == "default" && attributeValue == "true") isDefault = true;
		else if(attributeName == "index") index = HelperFunctions::getNumber(attributeValue);
		else HelperFunctions::printWarning("Warning: Unknown attribute for \"option\": " + attributeName);
	}
}

std::shared_ptr<LogicalParameter> LogicalParameter::fromXML(xml_node<>* node)
{
	std::shared_ptr<LogicalParameter> parameter;
	try
	{
		xml_attribute<>* attr = node->first_attribute("type");
		if(attr != nullptr)
		{
			std::string attributeValue(attr->value());
			if(attributeValue == "option") parameter.reset(new LogicalParameterEnum(node));
			else if(attributeValue == "integer") parameter.reset(new LogicalParameterInteger(node));
			else if(attributeValue == "float") parameter.reset(new LogicalParameterFloat(node));
			else if(attributeValue == "boolean") parameter.reset(new LogicalParameterBoolean(node));
			else if(attributeValue == "string") parameter.reset(new LogicalParameterString(node));
			else if(attributeValue == "action") parameter.reset(new LogicalParameterAction(node));
			else  parameter.reset(new LogicalParameterInteger(node));
		}
		else parameter.reset(new LogicalParameterInteger(node));
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return parameter;
}

LogicalParameterEnum::LogicalParameterEnum()
{
	type = Type::Enum::typeEnum;
}

LogicalParameterEnum::LogicalParameterEnum(xml_node<>* node)
{
	try
	{
		type = Type::Enum::typeEnum;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "type") {}
			else if(attributeName == "unit") unit = attributeValue;
			else HelperFunctions::printWarning("Warning: Unknown attribute for \"logical\" with type enum: " + attributeName);
		}
		int32_t index = 0;
		for(xml_node<>* optionNode = node->first_node("option"); optionNode; optionNode = optionNode->next_sibling())
		{
			ParameterOption option(optionNode);
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
		max = index - 1;
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

LogicalParameterInteger::LogicalParameterInteger()
{
	type = Type::Enum::typeInteger;
}

LogicalParameterInteger::LogicalParameterInteger(xml_node<>* node)
{
	try
	{
		type = Type::Enum::typeInteger;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "type") {}
			else if(attributeName == "min") min = HelperFunctions::getNumber(attributeValue);
			else if(attributeName == "max") max = HelperFunctions::getNumber(attributeValue);
			else if(attributeName == "default")
			{
				defaultValue = HelperFunctions::getNumber(attributeValue);
				defaultValueExists = true;
			}
			else if(attributeName == "unit") unit = attributeValue;
			else HelperFunctions::printWarning("Warning: Unknown attribute for \"logical\" with type integer: " + attributeName);
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
				specialValues[attr1->value()] = HelperFunctions::getNumber(valueString);
			}
			else HelperFunctions::printWarning("Warning: Unknown node in \"logical\" with type integer: " + nodeName);
		}
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

LogicalParameterFloat::LogicalParameterFloat()
{
	type = Type::Enum::typeFloat;
}

LogicalParameterFloat::LogicalParameterFloat(xml_node<>* node)
{
	try
	{
		type = Type::Enum::typeFloat;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "type") {}
			else if(attributeName == "min") min = HelperFunctions::getDouble(attributeValue);
			else if(attributeName == "max") max = HelperFunctions::getDouble(attributeValue);
			else if(attributeName == "default")
			{
				defaultValue = HelperFunctions::getDouble(attributeValue);
				defaultValueExists = true;
			}
			else if(attributeName == "unit") unit = attributeValue;
			else HelperFunctions::printWarning("Warning: Unknown attribute for \"logical\" with type float: " + attributeName);
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
				specialValues[attr1->value()] = HelperFunctions::getDouble(valueString);
			}
			else HelperFunctions::printWarning("Warning: Unknown node in \"logical\" with type float: " + nodeName);
		}
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

LogicalParameterBoolean::LogicalParameterBoolean()
{
	type = Type::Enum::typeBoolean;
}

LogicalParameterBoolean::LogicalParameterBoolean(xml_node<>* node)
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
			else if(attributeName != "type") HelperFunctions::printWarning("Warning: Unknown attribute for \"logical\" with type boolean: " + attributeName);
		}
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

LogicalParameterString::LogicalParameterString()
{
	type = Type::Enum::typeString;
}

LogicalParameterString::LogicalParameterString(xml_node<>* node)
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
			else if(attributeName != "type") HelperFunctions::printWarning("Warning: Unknown attribute for \"logical\" with type string: " + attributeName);
		}
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

LogicalParameterAction::LogicalParameterAction()
{
	type = Type::Enum::typeAction;
}

LogicalParameterAction::LogicalParameterAction(xml_node<>* node)
{
	try
	{
		type = Type::Enum::typeAction;
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			if(attributeName != "type") HelperFunctions::printWarning("Warning: Unknown attribute for \"logical\" with type boolean: " + attributeName);
		}
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

} /* namespace XMLRPC */
