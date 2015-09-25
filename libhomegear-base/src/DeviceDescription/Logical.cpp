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

#include "Logical.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace DeviceDescription
{

EnumerationValue::EnumerationValue(BaseLib::Obj* baseLib, xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		baseLib->out.printWarning("Warning: Unknown attribute for \"logicalEnumeration\\value\": " + std::string(attr->name()));
	}
	for(xml_node<>* subnode = node->first_node(); subnode; subnode = subnode->next_sibling())
	{
		std::string nodeName(subnode->name());
		std::string nodeValue(subnode->value());
		if(nodeName == "id") id = nodeValue;
		else if(nodeName == "index") index = Math::getNumber(nodeValue);
		else baseLib->out.printWarning("Warning: Unknown node in \"logicalEnumeration\\value\": " + std::string(subnode->name(), subnode->name_size()));
	}
}

ILogical::ILogical(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

ILogical::ILogical(BaseLib::Obj* baseLib, xml_node<>* node) : ILogical(baseLib)
{

}

LogicalEnumeration::LogicalEnumeration(BaseLib::Obj* baseLib) : ILogical(baseLib)
{
	type = Type::Enum::tEnum;
}

LogicalEnumeration::LogicalEnumeration(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalEnumeration(baseLib)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			_bl->out.printWarning("Warning: Unknown attribute for \"parameter\": " + std::string(attr->name()));
		}
		int32_t index = 0;
		for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
		{
			std::string nodeName(subNode->name());
			std::string nodeValue(subNode->value());
			if(nodeName == "value")
			{
				EnumerationValue value(baseLib, subNode);
				if(value.index > -1)
				{
					while((unsigned)value.index > values.size()) values.push_back(EnumerationValue());
					index = value.index;
				}
				else value.index = index;
				values.push_back(value);
				index++;
			}
			else if(nodeName == "defaultValue")
			{
				defaultValueExists = true;
				defaultValue = Math::getNumber(nodeValue);
			}
			else if(nodeName == "setToValueOnPairing")
			{
				setToValueOnPairingExists = true;
				setToValueOnPairing = Math::getNumber(nodeValue);
			}
			else baseLib->out.printWarning("Warning: Unknown node in \"logicalEnumeration\": " + nodeName);
		}
		maximumValue = index - 1;
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

std::shared_ptr<Variable> LogicalEnumeration::getSetToValueOnPairing()
{
	return std::shared_ptr<Variable>(new Variable(setToValueOnPairing));
}

std::shared_ptr<Variable> LogicalEnumeration::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(defaultValue));
}

LogicalInteger::LogicalInteger(BaseLib::Obj* baseLib) : ILogical(baseLib)
{
	type = Type::Enum::tInteger;
}

LogicalInteger::LogicalInteger(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalInteger(baseLib)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			_bl->out.printWarning("Warning: Unknown attribute for \"logicalInteger\": " + std::string(attr->name()));
		}
		for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
		{
			std::string nodeName(subNode->name());
			std::string nodeValue(subNode->value());
			if(nodeName == "minimumValue") minimumValue = Math::getNumber(nodeValue);
			else if(nodeName == "maximumValue") maximumValue = Math::getNumber(nodeValue);
			else if(nodeName == "defaultValue")
			{
				defaultValueExists = true;
				defaultValue = Math::getNumber(nodeValue);
			}
			else if(nodeName == "setToValueOnPairing")
			{
				setToValueOnPairingExists = true;
				setToValueOnPairing = Math::getNumber(nodeValue);
			}
			else if(nodeName == "specialValues")
			{
				for(xml_node<>* specialValueNode = subNode->first_node(); specialValueNode; specialValueNode = specialValueNode->next_sibling())
				{
					std::string specialValueName(specialValueNode->name());
					std::string specialValueString(specialValueNode->value());
					if(specialValueName == "specialValue")
					{
						std::string id;
						for(xml_attribute<>* attr = specialValueNode->first_attribute(); attr; attr = attr->next_attribute())
						{
							std::string attributeName(attr->name());
							if(attributeName == "id") id = std::string(attr->value());
							else _bl->out.printWarning("Warning: Unknown attribute for \"logicalInteger\\specialValues\\specialValue\": " + std::string(attr->name()));
						}

						if(id.empty()) _bl->out.printWarning("Warning: No id set for \"logicalInteger\\specialValues\\specialValue\"");
						int32_t specialValue = Math::getNumber(specialValueString);
						specialValuesStringMap[id] = specialValue;
						specialValuesIntegerMap[specialValue] = id;
					}
					else _bl->out.printWarning("Warning: Unknown node in \"logicalInteger\\specialValues\": " + nodeName);
				}
			}
			else _bl->out.printWarning("Warning: Unknown node in \"logicalInteger\": " + nodeName);
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

std::shared_ptr<Variable> LogicalInteger::getSetToValueOnPairing()
{
	return std::shared_ptr<Variable>(new Variable(setToValueOnPairing));
}

std::shared_ptr<Variable> LogicalInteger::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(defaultValue));
}

LogicalDecimal::LogicalDecimal(BaseLib::Obj* baseLib) : ILogical(baseLib)
{
	type = Type::Enum::tFloat;
}

LogicalDecimal::LogicalDecimal(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalDecimal(baseLib)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			_bl->out.printWarning("Warning: Unknown attribute for \"logicalDecimal\": " + std::string(attr->name()));
		}
		for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
		{
			std::string nodeName(subNode->name());
			std::string nodeValue(subNode->value());
			if(nodeName == "minimumValue") minimumValue = Math::getDouble(nodeValue);
			else if(nodeName == "maximumValue") maximumValue = Math::getDouble(nodeValue);
			else if(nodeName == "defaultValue")
			{
				defaultValueExists = true;
				defaultValue = Math::getDouble(nodeValue);
			}
			else if(nodeName == "setToValueOnPairing")
			{
				setToValueOnPairingExists = true;
				setToValueOnPairing = Math::getDouble(nodeValue);
			}
			else if(nodeName == "specialValues")
			{
				for(xml_node<>* specialValueNode = subNode->first_node(); specialValueNode; specialValueNode = specialValueNode->next_sibling())
				{
					std::string specialValueName(specialValueNode->name());
					std::string specialValueString(specialValueNode->value());
					if(specialValueName == "specialValue")
					{
						std::string id;
						for(xml_attribute<>* attr = specialValueNode->first_attribute(); attr; attr = attr->next_attribute())
						{
							std::string attributeName(attr->name());
							if(attributeName == "id") id = std::string(attr->value());
							else _bl->out.printWarning("Warning: Unknown attribute for \"logicalDecimal\\specialValues\\specialValue\": " + std::string(attr->name()));
						}

						if(id.empty()) _bl->out.printWarning("Warning: No id set for \"logicalDecimal\\specialValues\\specialValue\"");
						double specialValue = Math::getDouble(specialValueString);
						specialValuesStringMap[id] = specialValue;
						specialValuesFloatMap[specialValue] = id;
					}
					else _bl->out.printWarning("Warning: Unknown node in \"logicalDecimal\\specialValues\": " + nodeName);
				}
			}
			else _bl->out.printWarning("Warning: Unknown node in \"logicalDecimal\": " + nodeName);
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

std::shared_ptr<Variable> LogicalDecimal::getSetToValueOnPairing()
{
	return std::shared_ptr<Variable>(new Variable(setToValueOnPairing));
}

std::shared_ptr<Variable> LogicalDecimal::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(defaultValue));
}

LogicalBoolean::LogicalBoolean(BaseLib::Obj* baseLib) : ILogical(baseLib)
{
	type = Type::Enum::tBoolean;
}

LogicalBoolean::LogicalBoolean(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalBoolean(baseLib)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			_bl->out.printWarning("Warning: Unknown attribute for \"logicalBoolean\": " + std::string(attr->name()));
		}
		for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
		{
			std::string nodeName(subNode->name());
			std::string nodeValue(subNode->value());
			if(nodeName == "defaultValue")
			{
				defaultValueExists = true;
				defaultValue = (nodeValue == "true");
			}
			else if(nodeName == "setToValueOnPairing")
			{
				setToValueOnPairingExists = true;
				setToValueOnPairing = (nodeValue == "true");
			}
			else _bl->out.printWarning("Warning: Unknown node in \"logicalBoolean\": " + nodeName);
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

std::shared_ptr<Variable> LogicalBoolean::getSetToValueOnPairing()
{
	return std::shared_ptr<Variable>(new Variable(setToValueOnPairing));
}

std::shared_ptr<Variable> LogicalBoolean::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(defaultValue));
}

LogicalString::LogicalString(BaseLib::Obj* baseLib) : ILogical(baseLib)
{
	type = Type::Enum::tString;
}

LogicalString::LogicalString(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalString(baseLib)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			_bl->out.printWarning("Warning: Unknown attribute for \"logicalString\": " + std::string(attr->name()));
		}
		for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
		{
			std::string nodeName(subNode->name());
			std::string nodeValue(subNode->value());
			if(nodeName == "defaultValue")
			{
				defaultValueExists = true;
				defaultValue = nodeValue;
			}
			else if(nodeName == "setToValueOnPairing")
			{
				setToValueOnPairingExists = true;
				setToValueOnPairing = nodeValue;
			}
			else _bl->out.printWarning("Warning: Unknown node in \"logicalString\": " + nodeName);
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

std::shared_ptr<Variable> LogicalString::getSetToValueOnPairing()
{
	return std::shared_ptr<Variable>(new Variable(setToValueOnPairing));
}

std::shared_ptr<Variable> LogicalString::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable(defaultValue));
}

LogicalAction::LogicalAction(BaseLib::Obj* baseLib) : ILogical(baseLib)
{
	type = Type::Enum::tAction;
}

LogicalAction::LogicalAction(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalAction(baseLib)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			_bl->out.printWarning("Warning: Unknown attribute for \"logicalAction\": " + std::string(attr->name()));
		}
		for(xml_node<>* logicalNode = node->first_node(); logicalNode; logicalNode = logicalNode->next_sibling())
		{
			_bl->out.printWarning("Warning: Unknown node in \"logicalAction\": " + std::string(logicalNode->name()));
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

std::shared_ptr<Variable> LogicalAction::getSetToValueOnPairing()
{
	return std::shared_ptr<Variable>(new Variable());
}

std::shared_ptr<Variable> LogicalAction::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable());
}

LogicalArray::LogicalArray(BaseLib::Obj* baseLib) : ILogical(baseLib)
{
	type = Type::Enum::tArray;
}

LogicalArray::LogicalArray(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalArray(baseLib)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			_bl->out.printWarning("Warning: Unknown attribute for \"logicalArray\": " + std::string(attr->name()));
		}
		for(xml_node<>* logicalNode = node->first_node(); logicalNode; logicalNode = logicalNode->next_sibling())
		{
			_bl->out.printWarning("Warning: Unknown node in \"logicalArray\": " + std::string(logicalNode->name()));
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

std::shared_ptr<Variable> LogicalArray::getSetToValueOnPairing()
{
	return std::shared_ptr<Variable>(new Variable());
}

std::shared_ptr<Variable> LogicalArray::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable());
}

LogicalStruct::LogicalStruct(BaseLib::Obj* baseLib) : ILogical(baseLib)
{
	type = Type::Enum::tStruct;
}

LogicalStruct::LogicalStruct(BaseLib::Obj* baseLib, xml_node<>* node) : LogicalStruct(baseLib)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			_bl->out.printWarning("Warning: Unknown attribute for \"logicalStruct\": " + std::string(attr->name()));
		}
		for(xml_node<>* logicalNode = node->first_node(); logicalNode; logicalNode = logicalNode->next_sibling())
		{
			_bl->out.printWarning("Warning: Unknown node in \"logicalStruct\": " + std::string(logicalNode->name()));
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

std::shared_ptr<Variable> LogicalStruct::getSetToValueOnPairing()
{
	return std::shared_ptr<Variable>(new Variable());
}

std::shared_ptr<Variable> LogicalStruct::getDefaultValue()
{
	return std::shared_ptr<Variable>(new Variable());
}

}
}
