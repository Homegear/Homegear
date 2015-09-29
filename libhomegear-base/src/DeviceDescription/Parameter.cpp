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

#include "Parameter.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace DeviceDescription
{

bool Parameter::Packet::checkCondition(int32_t lhs)
{
	switch(conditionOperator)
	{
	case ConditionOperator::Enum::e:
		return lhs == conditionValue;
	case ConditionOperator::Enum::g:
		return lhs > conditionValue;
	case ConditionOperator::Enum::l:
		return lhs < conditionValue;
	case ConditionOperator::Enum::ge:
		return lhs >= conditionValue;
	case ConditionOperator::Enum::le:
		return lhs <= conditionValue;
	default:
		return false;
	}
}

Parameter::Parameter(BaseLib::Obj* baseLib, ParameterGroup* parent)
{
	_bl = baseLib;
	_parent = parent;
	logical.reset(new LogicalInteger(baseLib));
	physical.reset(new PhysicalInteger(baseLib));
}

Parameter::Parameter(BaseLib::Obj* baseLib, xml_node<>* node, ParameterGroup* parent) : Parameter(baseLib, parent)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id")
		{
			id = attributeValue;
		}
		else _bl->out.printWarning("Warning: Unknown attribute for \"parameter\": " + attributeName);
	}
	for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
	{
		std::string nodeName(subNode->name());
		if(nodeName == "properties")
		{
			for(xml_node<>* propertyNode = subNode->first_node(); propertyNode; propertyNode = propertyNode->next_sibling())
			{
				std::string propertyName(propertyNode->name());
				std::string propertyValue(propertyNode->value());
				if(propertyName == "readable") { if(propertyValue == "false") readable = false; }
				else if(propertyName == "writeable") { if(propertyValue == "false") writeable = false; }
				else if(propertyName == "addonWriteable") { if(propertyValue == "false") addonWriteable = false; }
				else if(propertyName == "visible") { if(propertyValue == "false") visible = false; }
				else if(propertyName == "internal") { if(propertyValue == "true") internal = true; }
				else if(propertyName == "parameterGroupSelector") { if(propertyValue == "true") parameterGroupSelector = true; }
				else if(propertyName == "service") { if(propertyValue == "true") service = true; }
				else if(propertyName == "sticky") { if(propertyValue == "true") sticky = true; }
				else if(propertyName == "transform") { if(propertyValue == "true") transform = true; }
				else if(propertyName == "signed") { if(propertyValue == "true") isSigned = true; }
				else if(propertyName == "control") control = propertyValue;
				else if(propertyName == "unit") unit = propertyValue;
				else if(propertyName == "casts")
				{
					for(xml_attribute<>* attr = propertyNode->first_attribute(); attr; attr = attr->next_attribute())
					{
						_bl->out.printWarning("Warning: Unknown attribute for \"casts\": " + std::string(attr->name()));
					}
					for(xml_node<>* castNode = propertyNode->first_node(); castNode; castNode = castNode->next_sibling())
					{
						std::string castName(castNode->name());
						if(castName == "decimalIntegerScale") casts.push_back(PDecimalIntegerScale(new DecimalIntegerScale(_bl, castNode, this)));
						else if(castName == "integerIntegerScale") casts.push_back(PIntegerIntegerScale(new IntegerIntegerScale(_bl, castNode, this)));
						else if(castName == "integerIntegerMap") casts.push_back(PIntegerIntegerMap(new IntegerIntegerMap(_bl, castNode, this)));
						else if(castName == "booleanInteger") casts.push_back(PBooleanInteger(new BooleanInteger(_bl, castNode, this)));
						else if(castName == "booleanString") casts.push_back(PBooleanInteger (new BooleanInteger(_bl, castNode, this)));
						else if(castName == "decimalConfigTime") casts.push_back(PDecimalConfigTime (new DecimalConfigTime(_bl, castNode, this)));
						else if(castName == "integerTinyFloat") casts.push_back(PIntegerTinyFloat(new IntegerTinyFloat(_bl, castNode, this)));
						else if(castName == "stringUnsignedInteger") casts.push_back(PStringUnsignedInteger(new StringUnsignedInteger(_bl, castNode, this)));
						else if(castName == "blindTest") casts.push_back(PBlindTest(new BlindTest(_bl, castNode, this)));
						else if(castName == "optionString") casts.push_back(POptionString(new OptionString(_bl, castNode, this)));
						else if(castName == "optionInteger") casts.push_back(POptionInteger(new OptionInteger(_bl, castNode, this)));
						else if(castName == "stringJsonArrayDecimal") casts.push_back(PStringJsonArrayDecimal(new StringJsonArrayDecimal(_bl, castNode, this)));
						else if(castName == "rpcBinary") casts.push_back(PRpcBinary(new RpcBinary(_bl, castNode, this)));
						else if(castName == "toggle") casts.push_back(PToggle(new Toggle(_bl, castNode, this)));
						else if(castName == "cfm") casts.push_back(PCfm(new Cfm(_bl, castNode, this)));
						else if(castName == "ccrtdnParty") casts.push_back(PCcrtdnParty(new CcrtdnParty(_bl, castNode, this)));
						else if(castName == "stringReplace") casts.push_back(PStringReplace(new StringReplace(_bl, castNode, this)));
						else if(castName == "hexStringByteArray") casts.push_back(PHexStringByteArray(new HexStringByteArray(_bl, castNode, this)));
						else _bl->out.printWarning("Warning: Unknown cast: " + castName);
					}
				}
				else _bl->out.printWarning("Warning: Unknown parameter property: " + propertyName);
			}
		}
		else if(nodeName == "logicalBoolean") logical.reset(new LogicalBoolean(_bl, subNode));
		else if(nodeName == "logicalAction") logical.reset(new LogicalAction(_bl, subNode));
		else if(nodeName == "logicalArray") logical.reset(new LogicalArray(_bl, subNode));
		else if(nodeName == "logicalInteger") logical.reset(new LogicalInteger(_bl, subNode));
		else if(nodeName == "logicalDecimal") logical.reset(new LogicalDecimal(_bl, subNode));
		else if(nodeName == "logicalString") logical.reset(new LogicalString(_bl, subNode));
		else if(nodeName == "logicalStruct") logical.reset(new LogicalStruct(_bl, subNode));
		else if(nodeName == "logicalEnumeration") logical.reset(new LogicalEnumeration(_bl, subNode));
		else if(nodeName == "physicalInteger") physical.reset(new PhysicalInteger(_bl, subNode));
		else if(nodeName == "physicalBoolean") physical.reset(new PhysicalBoolean(_bl, subNode));
		else if(nodeName == "physicalString") physical.reset(new PhysicalString(_bl, subNode));
		else if(nodeName == "packets")
		{
			for(xml_node<>* packetsNode = subNode->first_node(); packetsNode; packetsNode = packetsNode->next_sibling())
			{
				std::string packetsNodeName(packetsNode->name());
				if(packetsNodeName == "packet")
				{
					std::shared_ptr<Packet> packet(new Packet());
					for(xml_attribute<>* attr = packetsNode->first_attribute(); attr; attr = attr->next_attribute())
					{
						std::string attributeName(attr->name());
						if(attributeName == "id") packet->id = std::string(attr->value());
						else _bl->out.printWarning("Warning: Unknown attribute for \"parameter\\packets\\packet\": " + std::string(attr->name()));
					}
					for(xml_node<>* packetNode = packetsNode->first_node(); packetNode; packetNode = packetNode->next_sibling())
					{
						std::string packetNodeName(packetNode->name());
						if(packetNodeName == "type")
						{
							for(xml_attribute<>* attr = packetNode->first_attribute(); attr; attr = attr->next_attribute())
							{
								_bl->out.printWarning("Warning: Unknown attribute for \"parameter\\packets\\packet\\type\": " + std::string(attr->name()));
							}
							std::string value(packetNode->value());
							if(value == "get")
							{
								packet->type = Packet::Type::Enum::get;
								getPackets.push_back(packet);
							}
							else if(value == "set")
							{
								packet->type = Packet::Type::Enum::set;
								setPackets.push_back(packet);
							}
							else if(value == "event")
							{
								packet->type = Packet::Type::Enum::event;
								eventPackets.push_back(packet);
							}
							else _bl->out.printWarning("Warning: Unknown value for \"parameter\\packets\\packet\\type\": " + value);
						}
						else if(packetNodeName == "responseId")
						{
							for(xml_attribute<>* attr = packetNode->first_attribute(); attr; attr = attr->next_attribute())
							{
								_bl->out.printWarning("Warning: Unknown attribute for \"parameter\\packets\\packet\\response\": " + std::string(attr->name()));
							}
							packet->responseId = std::string(packetNode->value());
						}
						else if(packetNodeName == "autoReset")
						{
							for(xml_attribute<>* attr = packetNode->first_attribute(); attr; attr = attr->next_attribute())
							{
								_bl->out.printWarning("Warning: Unknown attribute for \"parameter\\packets\\packet\\autoReset\": " + std::string(attr->name()));
							}
							for(xml_node<>* autoResetNode = packetNode->first_node(); autoResetNode; autoResetNode = autoResetNode->next_sibling())
							{
								std::string autoResetNodeName(autoResetNode->name());
								std::string autoResetNodeValue(autoResetNode->value());
								if(autoResetNodeValue.empty()) continue;
								if(autoResetNodeName == "parameterId") packet->autoReset.push_back(autoResetNodeValue);
								else _bl->out.printWarning("Warning: Unknown subnode for \"parameter\\packets\\packet\\autoReset\": " + packetsNodeName);
							}
						}
						else if(packetNodeName == "delayedAutoReset")
						{
							for(xml_attribute<>* attr = packetNode->first_attribute(); attr; attr = attr->next_attribute())
							{
								_bl->out.printWarning("Warning: Unknown attribute for \"parameter\\packets\\packet\\delayedAutoReset\": " + std::string(attr->name()));
							}
							for(xml_node<>* autoResetNode = packetNode->first_node(); autoResetNode; autoResetNode = autoResetNode->next_sibling())
							{
								std::string autoResetNodeName(autoResetNode->name());
								std::string autoResetNodeValue(autoResetNode->value());
								if(autoResetNodeValue.empty()) continue;
								if(autoResetNodeName == "resetDelayParameterId") packet->delayedAutoReset.first = autoResetNodeValue;
								else if(autoResetNodeName == "resetTo") packet->delayedAutoReset.second = Math::getNumber(autoResetNodeValue);
								else _bl->out.printWarning("Warning: Unknown subnode for \"parameter\\packets\\packet\\delayedAutoReset\": " + packetsNodeName);
								hasDelayedAutoResetParameters = true;
							}
						}
						else if(packetNodeName == "conditionOperator")
						{
							std::string value(packetNode->value());
							HelperFunctions::toLower(HelperFunctions::trim(value));
							if(value == "e" || value == "eq") packet->conditionOperator = Packet::ConditionOperator::Enum::e;
							else if(value == "g") packet->conditionOperator = Packet::ConditionOperator::Enum::g;
							else if(value == "l") packet->conditionOperator = Packet::ConditionOperator::Enum::l;
							else if(value == "ge") packet->conditionOperator = Packet::ConditionOperator::Enum::ge;
							else if(value == "le") packet->conditionOperator = Packet::ConditionOperator::Enum::le;
							else baseLib->out.printWarning("Warning: Unknown attribute value for \"cond\" in node \"setEx\": " + value);
						}
						else if(packetNodeName == "conditionValue")
						{
							std::string value(packetNode->value());
							packet->conditionValue = Math::getNumber(value);
						}
						else _bl->out.printWarning("Warning: Unknown subnode for \"parameter\\packets\\packet\": " + packetsNodeName);
					}
				}
				else _bl->out.printWarning("Warning: Unknown subnode for \"parameter\\packets\": " + packetsNodeName);
			}
		}
		else _bl->out.printWarning("Warning: Unknown node in \"parameter\": " + nodeName);
	}
	if(logical->type == ILogical::Type::Enum::tFloat)
	{
		LogicalDecimal* parameter = (LogicalDecimal*)logical.get();
		if(parameter->minimumValue < 0 && parameter->minimumValue != std::numeric_limits<double>::min()) isSigned = true;
	}
	else if(logical->type == ILogical::Type::Enum::tInteger)
	{
		LogicalInteger* parameter = (LogicalInteger*)logical.get();
		if(parameter->minimumValue < 0 && parameter->minimumValue != std::numeric_limits<int32_t>::min()) isSigned = true;
	}
}

Parameter::~Parameter()
{
}

PVariable Parameter::convertFromPacket(std::vector<uint8_t>& data, bool isEvent)
{
	try
	{
		std::vector<uint8_t> reversedData;
		std::vector<uint8_t>* value = nullptr;
		if(physical->endianess == IPhysical::Endianess::Enum::little)
		{
			value = &reversedData;
			reverseData(data, *value);
		}
		else value = &data;
		if(logical->type == ILogical::Type::Enum::tEnum && casts.empty())
		{
			int32_t integerValue = 0;
			_bl->hf.memcpyBigEndian(integerValue, *value);
			return std::shared_ptr<Variable>(new Variable(integerValue));
		}
		else if(logical->type == ILogical::Type::Enum::tBoolean && casts.empty())
		{
			int32_t integerValue = 0;
			_bl->hf.memcpyBigEndian(integerValue, *value);
			return PVariable(new Variable((bool)integerValue));
		}
		else if(logical->type == ILogical::Type::Enum::tString && casts.empty())
		{
			if(!value->empty() && value->at(0) != 0)
			{
				int32_t size = value->back() == 0 ? value->size() - 1 : value->size();
				std::string string((char*)&value->at(0), size);
				return PVariable(new Variable(string));
			}
			return PVariable(new Variable(VariableType::tString));
		}
		else if(logical->type == ILogical::Type::Enum::tAction)
		{
			if(isEvent) return PVariable(new Variable(true));
			else return PVariable(new Variable(false));
		}
		else if(id == "RSSI_DEVICE")
		{
			int32_t integerValue;
			_bl->hf.memcpyBigEndian(integerValue, *value);
			std::shared_ptr<Variable> variable(new Variable(integerValue * -1));
			return variable;
		}
		else
		{
			std::shared_ptr<Variable> variable;
			if(physical->type == IPhysical::Type::tString)
			{
				variable.reset(new Variable(VariableType::tString));
				variable->stringValue.insert(variable->stringValue.end(), value->begin(), value->end());
			}
			else if(value->size() <= 4)
			{
				int32_t integerValue;
				_bl->hf.memcpyBigEndian(integerValue, *value);
				variable.reset(new Variable(integerValue));
				if(isSigned && !value->empty() && value->size() <= 4)
				{
					int32_t byteIndex = value->size() - std::lround(std::ceil(physical->size));
					if(byteIndex >= 0 && byteIndex < (signed)value->size())
					{
						int32_t bitSize = std::lround(physical->size * 10) % 10;
						int32_t signPosition = 0;
						if(bitSize == 0) signPosition = 7;
						else signPosition = bitSize - 1;
						if(value->at(byteIndex) & (1 << signPosition))
						{
							int32_t bits = (std::lround(std::floor(physical->size)) * 8) + bitSize;
							variable->integerValue -= (1 << bits);
						}
					}
				}
			}
			if(!variable) variable.reset(new Variable(VariableType::tBinary));
			for(std::vector<PICast>::reverse_iterator i = casts.rbegin(); i != casts.rend(); ++i)
			{
				if((*i)->needsBinaryPacketData() && variable->binaryValue.empty()) variable->binaryValue = *value;
				(*i)->fromPacket(variable);
			}
			return variable;
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
    return PVariable(new Variable(VariableType::tInteger));
}

void Parameter::convertToPacket(std::string value, std::vector<uint8_t>& convertedValue)
{
	try
	{
		PVariable rpcValue;
		if(logical->type == ILogical::Type::Enum::tInteger) rpcValue.reset(new Variable(Math::getNumber(value)));
		if(logical->type == ILogical::Type::Enum::tEnum)
		{
			if(Math::isNumber(value)) rpcValue.reset(new Variable(Math::getNumber(value)));
			else //value is id of enum element
			{
				LogicalEnumeration* parameter = (LogicalEnumeration*)logical.get();
				for(std::vector<EnumerationValue>::iterator i = parameter->values.begin(); i != parameter->values.end(); ++i)
				{
					if(i->id == value)
					{
						rpcValue.reset(new Variable(i->index));
						break;
					}
				}
				if(!rpcValue) rpcValue.reset(new Variable(0));
			}
		}
		else if(logical->type == ILogical::Type::Enum::tBoolean || logical->type == ILogical::Type::Enum::tAction)
		{
			rpcValue.reset(new Variable(false));
			if(HelperFunctions::toLower(value) == "true") rpcValue->booleanValue = true;
		}
		else if(logical->type == ILogical::Type::Enum::tFloat) rpcValue.reset(new Variable(Math::getDouble(value)));
		else if(logical->type == ILogical::Type::Enum::tString) rpcValue.reset(new Variable(value));
		if(!rpcValue)
		{
			_bl->out.printWarning("Warning: Could not convert parameter " + id + " from String.");
			return;
		}
		return convertToPacket(rpcValue, convertedValue);
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

void Parameter::convertToPacket(const std::shared_ptr<Variable> value, std::vector<uint8_t>& convertedValue)
{
	try
	{
		convertedValue.clear();
		if(!value) return;
		PVariable variable(new Variable());
		*variable = *value;
		if(logical->type == ILogical::Type::Enum::tEnum && casts.empty())
		{
			LogicalEnumeration* parameter = (LogicalEnumeration*)logical.get();
			if(variable->integerValue > parameter->maximumValue) variable->integerValue = parameter->maximumValue;
			if(variable->integerValue < parameter->minimumValue) variable->integerValue = parameter->minimumValue;
		}
		else if(logical->type == ILogical::Type::Enum::tAction && casts.empty())
		{
			variable->integerValue = (int32_t)variable->booleanValue;
		}
		else if(logical->type == ILogical::Type::Enum::tString && casts.empty())
		{
			if(variable->stringValue.size() > 0)
			{
				convertedValue.insert(convertedValue.end(), variable->stringValue.begin(), variable->stringValue.end());
			}
			if((signed)convertedValue.size() < std::lround(physical->size)) convertedValue.push_back(0); //0 termination. Otherwise parts of old string will still be visible
		}
		else
		{
			if(logical->type == ILogical::Type::Enum::tFloat)
			{
				if(variable->floatValue == 0 && variable->integerValue != 0) variable->floatValue = variable->integerValue;
				else if(variable->integerValue == 0 && !variable->stringValue.empty()) variable->floatValue = Math::getDouble(variable->stringValue);
				LogicalDecimal* parameter = (LogicalDecimal*)logical.get();
				bool specialValue = (variable->floatValue == parameter->defaultValue);
				if(!specialValue)
				{
					for(std::unordered_map<std::string, double>::const_iterator i = parameter->specialValuesStringMap.begin(); i != parameter->specialValuesStringMap.end(); ++i)
					{
						if(i->second == variable->floatValue)
						{
							specialValue = true;
							break;
						}
					}
				}
				if(!specialValue)
				{
					if(variable->floatValue > parameter->maximumValue) variable->floatValue = parameter->maximumValue;
					else if(variable->floatValue < parameter->minimumValue) variable->floatValue = parameter->minimumValue;
				}
			}
			else if(logical->type == ILogical::Type::Enum::tInteger)
			{
				if(variable->integerValue == 0 && variable->floatValue != 0) variable->integerValue = variable->floatValue;
				else if(variable->integerValue == 0 && !variable->stringValue.empty()) variable->integerValue = Math::getNumber(variable->stringValue);
				LogicalInteger* parameter = (LogicalInteger*)logical.get();
				bool specialValue = (variable->integerValue == parameter->defaultValue);
				if(!specialValue)
				{
					for(std::unordered_map<std::string, int32_t>::const_iterator i = parameter->specialValuesStringMap.begin(); i != parameter->specialValuesStringMap.end(); ++i)
					{
						if(i->second == variable->integerValue)
						{
							specialValue = true;
							break;
						}
					}
				}
				if(!specialValue)
				{
					if(variable->integerValue > parameter->maximumValue) variable->integerValue = parameter->maximumValue;
					else if(variable->integerValue < parameter->minimumValue) variable->integerValue = parameter->minimumValue;
				}
			}
			else if(logical->type == ILogical::Type::Enum::tBoolean)
			{
				if(variable->booleanValue == false && variable->integerValue != 0) variable->booleanValue = true;
				else if(variable->booleanValue == false && variable->floatValue != 0) variable->booleanValue = true;
				else if(variable->booleanValue == false && variable->stringValue == "true") variable->booleanValue = true;
			}
			if(casts.empty())
			{
				if(logical->type == ILogical::Type::Enum::tBoolean)
				{
					if(variable->stringValue.size() > 0 && variable->stringValue == "true") variable->integerValue = 1;
					else variable->integerValue = (int32_t)variable->booleanValue;
				}
			}
			else
			{
				for(std::vector<PICast>::iterator i = casts.begin(); i != casts.end(); ++i)
				{
					(*i)->toPacket(variable);
				}
			}
		}
		if(variable->type == VariableType::tBinary)
		{
			convertedValue = variable->binaryValue;
		}
		else if(physical->type != IPhysical::Type::Enum::tString)
		{
			if(physical->sizeDefined) //Values have no size defined. That is a problem if the value is larger than 1 byte.
			{
				//Crop to size. Most importantly for negative numbers
				uint32_t byteSize = std::lround(std::floor(physical->size));
				int32_t bitSize = std::lround(physical->size * 10) % 10;
				if(byteSize >= 4)
				{
					byteSize = 4;
					bitSize = 0;
				}
				int32_t valueMask = 0xFFFFFFFF >> (((4 - byteSize) * 8) - bitSize);
				variable->integerValue &= valueMask;
			}
			_bl->hf.memcpyBigEndian(convertedValue, variable->integerValue);

			if(physical->endianess == IPhysical::Endianess::Enum::little)
			{
				std::vector<uint8_t> reversedData;
				reverseData(convertedValue, reversedData);
				convertedValue = reversedData;
			}
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

void Parameter::reverseData(const std::vector<uint8_t>& data, std::vector<uint8_t>& reversedData)
{
	try
	{
		reversedData.clear();
		int32_t size = std::lround(std::ceil(physical->size));
		if(size == 0) size = 1;
		int32_t j = data.size() - 1;
		for(int32_t i = 0; i < size; i++)
		{
			if(j < 0) reversedData.push_back(0);
			else reversedData.push_back(data.at(j));
			j--;
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

void Parameter::adjustBitPosition(std::vector<uint8_t>& data)
{
	try
	{
		if(data.size() > 4 || data.empty() || logical->type == ILogical::Type::Enum::tString) return;
		int32_t value = 0;
		_bl->hf.memcpyBigEndian(value, data);
		if(physical->size < 0)
		{
			_bl->out.printError("Error: Negative size not allowed.");
			return;
		}
		double i = physical->index;
		i -= std::floor(i);
		double byteIndex = std::floor(i);
		if(byteIndex != i || physical->size < 0.8) //0.8 == 8 Bits
		{
			if(physical->size > 1)
			{
				_bl->out.printError("Error: Can't set partial byte index > 1.");
				return;
			}
			data.clear();
			data.push_back(value << (std::lround(i * 10) % 10));
		}
		//Adjust data size. See for example ENDTIME_SATURDAY_1 and TEMPERATURE_SATURDAY_1 of HM-CC-RT-DN
		if((int32_t)physical->size > (signed)data.size())
		{
			uint32_t bytesMissing = (int32_t)physical->size - data.size();
			std::vector<uint8_t> oldData = data;
			data.clear();
			for(uint32_t i = 0; i < bytesMissing; i++) data.push_back(0);
			for(uint32_t i = 0; i < oldData.size(); i++) data.push_back(oldData.at(i));
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

ParameterGroup* Parameter::parent()
{
	return _parent;
}

}
}
