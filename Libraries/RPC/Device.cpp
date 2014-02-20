/* Copyright 2013-2014 Sathya Laufer
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

#include "Device.h"
#include "../GD/GD.h"

namespace RPC {

DescriptionField::DescriptionField(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id") id = attributeValue;
		else if(attributeName == "value") value = attributeValue;
		else Output::printWarning("Warning: Unknown attribute for \"field\": " + attributeName);
	}
}

ParameterDescription::ParameterDescription(xml_node<>* node)
{
	for(xml_node<>* descriptionNode = node->first_node(); descriptionNode; descriptionNode = descriptionNode->next_sibling())
	{
		std::string nodeName(descriptionNode->name());
		if(nodeName == "field")
		{
			fields.push_back(descriptionNode);
		}
		else Output::printWarning("Warning: Unknown subnode for \"description\": " + nodeName);
	}
}

DeviceFrame::DeviceFrame(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "direction")
		{
			if(attributeValue == "from_device") direction = Direction::Enum::fromDevice;
			else if(attributeValue == "to_device") direction = Direction::Enum::toDevice;
			else Output::printWarning("Warning: Unknown direction for \"frame\": " + attributeValue);
		}
		else if(attributeName == "allowed_receivers")
		{
			std::stringstream stream(attributeValue);
			std::string element;
			while(std::getline(stream, element, ','))
			{
				HelperFunctions::toLower(HelperFunctions::trim(element));
				if(element == "broadcast") allowedReceivers = (AllowedReceivers::Enum)(allowedReceivers | AllowedReceivers::Enum::broadcast);
				else if(element == "central") allowedReceivers = (AllowedReceivers::Enum)(allowedReceivers | AllowedReceivers::Enum::central);
				else if(element == "other") allowedReceivers = (AllowedReceivers::Enum)(allowedReceivers | AllowedReceivers::Enum::other);
			}
		}
		else if(attributeName == "id") id = attributeValue;
		else if(attributeName == "event") { if(attributeValue == "true") isEvent = true; }
		else if(attributeName == "type")
		{
			if(attributeValue.size() == 2 && attributeValue.at(0) == '#') type = (uint32_t)attributeValue.at(1);
			else type = HelperFunctions::getNumber(attributeValue);
		}
		else if(attributeName == "subtype") subtype = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "subtype_index") subtypeIndex = std::stoll(attributeValue);
		else if(attributeName == "channel_field")
		{
			if(channelField == -1) //Might already be set by receiver_channel_field. We don't need both
			{
				std::pair<std::string, std::string> splitString = HelperFunctions::split(attributeValue, ':');
				channelField = HelperFunctions::getNumber(splitString.first);
				if(!splitString.second.empty()) channelFieldSize = HelperFunctions::getDouble(splitString.second);
			}
		}
		else if(attributeName == "receiver_channel_field")
		{
			std::pair<std::string, std::string> splitString = HelperFunctions::split(attributeValue, ':');
			channelField = HelperFunctions::getNumber(splitString.first);
			if(!splitString.second.empty()) channelFieldSize = HelperFunctions::getDouble(splitString.second);
		}
		else if(attributeName == "fixed_channel")
		{
			if(attributeValue == "*") fixedChannel = -2;
			else fixedChannel = HelperFunctions::getNumber(attributeValue);
		}
		else Output::printWarning("Warning: Unknown attribute for \"frame\": " + attributeName);
	}
	for(xml_node<>* frameNode = node->first_node("parameter"); frameNode; frameNode = frameNode->next_sibling("parameter"))
	{
		parameters.push_back(frameNode);
	}
}

void ParameterConversion::fromPacket(std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		if(!value) return;
		if(type == Type::Enum::floatIntegerScale)
		{
			value->type = RPCVariableType::rpcFloat;
			value->floatValue = ((double)value->integerValue / factor) - offset;
		}
		else if(type == Type::Enum::integerIntegerScale)
		{
			value->type = RPCVariableType::rpcInteger;
			if(div > 0) value->integerValue *= div;
			if(mul > 0) value->integerValue /= mul;
		}
		else if(type == Type::Enum::integerIntegerMap || type == Type::Enum::optionInteger)
		{
			if(fromDevice && integerValueMapDevice.find(value->integerValue) != integerValueMapDevice.end()) value->integerValue = integerValueMapDevice[value->integerValue];
		}
		else if(type == Type::Enum::booleanInteger)
		{
			value->type = RPCVariableType::rpcBoolean;
			if(value->integerValue == valueFalse) value->booleanValue = false;
			if(value->integerValue == valueTrue || value->integerValue > threshold) value->booleanValue = true;
			if(invert) value->booleanValue = !value->booleanValue;
		}
		else if(type == Type::Enum::floatConfigTime)
		{
			value->type = RPCVariableType::rpcFloat;
			if(value < 0)
			{
				value->floatValue = 0;
				return;
			}
			if(valueSize > 0)
			{
				uint32_t bits = (uint32_t)std::floor(valueSize) * 8;
				bits += std::lround(valueSize * 10) % 10;
				if(bits == 0) bits = 7;
				uint32_t maxNumber = 1 << bits;
				if((unsigned)value->integerValue < maxNumber) value->floatValue = value->integerValue * factor;
				else value->floatValue = (value->integerValue - maxNumber) * factor2;
			}
			else
			{
				int32_t factorIndex = (value->integerValue & 0xFF) >> 5;
				double factor = 0;
				switch(factorIndex)
				{
				case 0:
					factor = 0.1;
					break;
				case 1:
					factor = 1;
					break;
				case 2:
					factor = 5;
					break;
				case 3:
					factor = 10;
					break;
				case 4:
					factor = 60;
					break;
				case 5:
					factor = 300;
					break;
				case 6:
					factor = 600;
					break;
				case 7:
					factor = 3600;
					break;
				}
				value->floatValue = (value->integerValue & 0x1F) * factor;
			}
		}
		else if(type == Type::Enum::integerTinyFloat)
		{
			value->type = RPCVariableType::rpcInteger;
			int32_t mantissa = (value->integerValue >> mantissaStart) & ((1 << mantissaSize) - 1);
			if(mantissaSize == 0) mantissa = 1;
			int32_t exponent = (value->integerValue >> exponentStart) & ((1 << exponentSize) - 1);
			value->integerValue = mantissa * (1 << exponent);
		}
		else if(type == Type::Enum::stringUnsignedInteger)
		{
			value->stringValue = std::to_string((uint32_t)value->integerValue);
		}
		else if(type == Type::Enum::blindTest)
		{
			value->integerValue = HelperFunctions::getNumber(stringValue);
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ParameterConversion::toPacket(std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		if(!value) return;
		if(type == Type::Enum::none)
		{
			if(value->type == RPCVariableType::rpcBoolean) value->integerValue = (int32_t)value->booleanValue;
		}
		else if(type == Type::Enum::floatIntegerScale)
		{
			value->integerValue = std::lround((value->floatValue + offset) * factor);
		}
		else if(type == Type::Enum::integerIntegerScale)
		{
			if(mul > 0) value->integerValue *= mul;
			//mul and div can be set, so no else if
			if(div > 0) value->integerValue /= div;
		}
		else if(type == Type::Enum::integerIntegerMap || type == Type::Enum::optionInteger)
		{
			//Value is not changed, when not in map. That is the desired behavior.
			if(toDevice && integerValueMapParameter.find(value->integerValue) != integerValueMapParameter.end()) value->integerValue = integerValueMapParameter[value->integerValue];
		}
		else if(type == Type::Enum::booleanInteger)
		{
			if(invert) value->booleanValue = !value->booleanValue;
			if(valueTrue == 0 && valueFalse == 0) value->integerValue = (int32_t)value->booleanValue;
			else if(value->booleanValue) value->integerValue = valueTrue;
			else value->integerValue = valueFalse;
		}
		else if(type == Type::Enum::floatConfigTime)
		{
			if(valueSize > 0)
			{
				uint32_t bits = (uint32_t)std::floor(valueSize) * 8;
				bits += std::lround(valueSize * 10) % 10;
				if(bits == 0) bits = 7;
				uint32_t maxNumber = 1 << bits;
				if(value->floatValue <= maxNumber - 1)
				{
					if(factor != 0) value->integerValue = std::lround(value->floatValue / factor);
				}
				else
				{
					if(factor2 != 0) value->integerValue = maxNumber + std::lround(value->floatValue / factor2);
				}
			}
			else
			{
				int32_t factorIndex = 0;
				double factor = 0.1;
				if(value->floatValue < 0) value->floatValue = 0;
				if(value->floatValue <= 3.1) { factorIndex = 0; factor = 0.1; }
				else if(value->floatValue <= 31) { factorIndex = 1; factor = 1; }
				else if(value->floatValue <= 155) { factorIndex = 2; factor = 5; }
				else if(value->floatValue <= 310) { factorIndex = 3; factor = 10; }
				else if(value->floatValue <= 1860) { factorIndex = 4; factor = 60; }
				else if(value->floatValue <= 9300) { factorIndex = 5; factor = 300; }
				else if(value->floatValue <= 18600) { factorIndex = 6; factor = 600; }
				else { factorIndex = 7; factor = 3600; }

				value->integerValue = ((factorIndex << 5) | std::lround(value->floatValue / factor)) & 0xFF;
			}
		}
		else if(type == Type::Enum::integerTinyFloat)
		{
			int64_t maxMantissa = ((1 << mantissaSize) - 1);
			int64_t maxExponent = ((1 << exponentSize) - 1);
			int64_t mantissa = value->integerValue;
			int64_t exponent = 0;
			if(maxMantissa > 0)
			{
				while(mantissa >= maxMantissa)
				{
					mantissa = mantissa >> 1;
					exponent++;
				}
			}
			if(mantissa > maxMantissa) mantissa = maxMantissa;
			if(exponent > maxExponent) exponent = maxExponent;
			exponent = exponent << exponentStart;
			value->integerValue = (mantissa << mantissaStart) | exponent;
		}
		else if(type == Type::Enum::stringUnsignedInteger)
		{
			value->integerValue = HelperFunctions::getUnsignedNumber(value->stringValue);
		}
		else if(type == Type::Enum::blindTest)
		{
			value->integerValue = HelperFunctions::getNumber(stringValue);
		}
		value->type = RPCVariableType::rpcInteger;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

ParameterConversion::ParameterConversion(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "type")
		{
			if(attributeValue == "float_integer_scale") type = Type::Enum::floatIntegerScale;
			else if(attributeValue == "integer_integer_scale") type = Type::Enum::integerIntegerScale;
			else if(attributeValue == "integer_integer_map") type = Type::Enum::integerIntegerMap;
			else if(attributeValue == "boolean_integer") type = Type::Enum::booleanInteger;
			else if(attributeValue == "float_configtime") type = Type::Enum::floatConfigTime;
			else if(attributeValue == "option_integer") type = Type::Enum::optionInteger;
			else if(attributeValue == "integer_tinyfloat") type = Type::Enum::integerTinyFloat;
			else if(attributeValue == "toggle") type = Type::Enum::toggle;
			else if(attributeValue == "string_unsigned_integer") type = Type::Enum::stringUnsignedInteger;
			else if(attributeValue == "action_key_counter") type = Type::Enum::none; //ignore, no conversion necessary
			else if(attributeValue == "action_key_same_counter") type = Type::Enum::none; //ignore, no conversion necessary
			else if(attributeValue == "rc19display") type = Type::Enum::none; //ignore, no conversion necessary
			else if(attributeValue == "blind_test") type = Type::Enum::blindTest;
			else if(attributeValue == "cfm") type = Type::Enum::cfm; //Used in "SUBMIT" of HM-OU-CFM-Pl
			else Output::printWarning("Warning: Unknown type for \"conversion\": " + attributeValue);
		}
		else if(attributeName == "factor") factor = HelperFunctions::getDouble(attributeValue);
		else if(attributeName == "factors")
		{
			std::pair<std::string, std::string> factors = HelperFunctions::split(attributeValue, ',');
			if(!factors.first.empty()) factor = HelperFunctions::getDouble(factors.first);
			if(!factors.second.empty()) factor2 = HelperFunctions::getDouble(factors.second);
		}
		else if(attributeName == "value_size") valueSize = HelperFunctions::getDouble(attributeValue);
		else if(attributeName == "threshold") threshold = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "false") valueFalse = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "true") valueTrue = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "div") div = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "mul") mul = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "offset") offset = HelperFunctions::getDouble(attributeValue);
		else if(attributeName == "value") stringValue = attributeValue;
		else if(attributeName == "mantissa_start") mantissaStart = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "mantissa_size") mantissaSize = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "exponent_start") exponentStart = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "exponent_size") exponentSize = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "sim_counter") {}
		else if(attributeName == "counter_size") {}
		else if(attributeName == "on") on = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "off") off = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "invert") { if(attributeValue == "true") invert = true; }
		else Output::printWarning("Warning: Unknown attribute for \"conversion\": " + attributeName);
	}
	for(xml_node<>* conversionNode = node->first_node(); conversionNode; conversionNode = conversionNode->next_sibling())
	{
		std::string nodeName(conversionNode->name());
		if(nodeName == "value_map" && (type == Type::Enum::integerIntegerMap || type == Type::Enum::optionInteger))
		{
			int32_t deviceValue = 0;
			int32_t parameterValue = 0;
			for(xml_attribute<>* valueMapAttr = conversionNode->first_attribute(); valueMapAttr; valueMapAttr = valueMapAttr->next_attribute())
			{
				std::string valueMapAttributeName(valueMapAttr->name());
				std::string valueMapAttributeValue(valueMapAttr->value());
				if(valueMapAttributeName == "device_value") deviceValue = HelperFunctions::getNumber(valueMapAttributeValue);
				else if(valueMapAttributeName == "parameter_value") parameterValue = HelperFunctions::getNumber(valueMapAttributeValue);
				else if(valueMapAttributeName == "from_device") { if(valueMapAttributeValue == "false") fromDevice = false; }
				else if(valueMapAttributeName == "to_device") { if(valueMapAttributeValue == "false") toDevice = false; }
				else Output::printWarning("Warning: Unknown attribute for \"value_map\": " + valueMapAttributeName);
			}
			integerValueMapDevice[deviceValue] = parameterValue;
			integerValueMapParameter[parameterValue] = deviceValue;
		}
		else Output::printWarning( "Warning: Unknown subnode for \"conversion\": " + nodeName);
	}
}

bool Parameter::checkCondition(int32_t value)
{
	try
	{
		switch(booleanOperator)
		{
		case BooleanOperator::Enum::e:
			return value == constValue;
			break;
		case BooleanOperator::Enum::g:
			return value > constValue;
			break;
		case BooleanOperator::Enum::l:
			return value < constValue;
			break;
		case BooleanOperator::Enum::ge:
			return value >= constValue;
			break;
		case BooleanOperator::Enum::le:
			return value <= constValue;
			break;
		default:
			Output::printWarning("Warning: Boolean operator is none.");
			break;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return false;
}

std::shared_ptr<RPCVariable> Parameter::convertFromPacket(const std::vector<uint8_t>& data, bool isEvent)
{
	try
	{
		if(logicalParameter->type == LogicalParameter::Type::Enum::typeEnum && conversion.empty())
		{
			return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcInteger, data));
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeBoolean && conversion.empty())
		{
			return std::shared_ptr<RPC::RPCVariable>(new RPCVariable(RPCVariableType::rpcBoolean, data));
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeString && conversion.empty())
		{
			if(!data.empty() && data.at(0) != 0)
			{
				int32_t size = data.back() == 0 ? data.size() - 1 : data.size();
				std::string string(&data.at(0), &data.at(0) + size);
				return std::shared_ptr<RPC::RPCVariable>(new RPCVariable(string));
			}
			return std::shared_ptr<RPC::RPCVariable>(new RPCVariable(RPCVariableType::rpcString));
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeAction)
		{
			if(isEvent) return std::shared_ptr<RPC::RPCVariable>(new RPCVariable(true));
			else return std::shared_ptr<RPC::RPCVariable>(new RPCVariable(false));
		}
		else if(id == "RSSI_DEVICE" || id == "RSSI_PEER")
		{
			//From CC1100 manual page 37:
			//1) Read the RSSI status register
			//2) Convert the reading from a hexadecimal
			//number to a decimal number (RSSI_dec)
			//3) If RSSI_dec ≥ 128 then RSSI_dBm =
			//(RSSI_dec - 256)/2 – RSSI_offset
			//4) Else if RSSI_dec < 128 then RSSI_dBm =
			//(RSSI_dec)/2 – RSSI_offset
			std::shared_ptr<RPCVariable> variable(new RPCVariable(RPCVariableType::rpcInteger, data));
			if(variable->integerValue >= 128) variable->integerValue = ((variable->integerValue - 256) / 2) - 74;
			else variable->integerValue = (variable->integerValue / 2) - 74;
			return variable;
		}
		else
		{
			std::shared_ptr<RPCVariable> variable(new RPCVariable(RPCVariableType::rpcInteger, data));
			if(isSigned && data.size() <= 4)
			{
				int32_t byteIndex = data.size() - std::lround(std::ceil(physicalParameter->size));
				int32_t bitSize = std::lround(physicalParameter->size * 10) % 10;
				int32_t signPosition = 0;
				if(bitSize == 0) signPosition = 7;
				else signPosition = bitSize - 1;
				if(data.at(byteIndex) & (1 << signPosition))
				{
					int32_t bits = (std::lround(std::floor(physicalParameter->size)) * 8) + bitSize;
					variable->integerValue -= (1 << bits);
				}
			}
			for(std::vector<std::shared_ptr<ParameterConversion>>::reverse_iterator i = conversion.rbegin(); i != conversion.rend(); ++i)
			{
				(*i)->fromPacket(variable);
			}
			return variable;
		}
		return std::shared_ptr<RPC::RPCVariable>(new RPCVariable(RPCVariableType::rpcInteger, data));
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<RPC::RPCVariable>(new RPCVariable(RPCVariableType::rpcInteger));
}

std::vector<uint8_t> Parameter::convertToPacket(std::string value)
{
	try
	{
		std::shared_ptr<RPCVariable> convertedValue;
		if(logicalParameter->type == LogicalParameter::Type::Enum::typeInteger) convertedValue.reset(new RPCVariable(HelperFunctions::getNumber(value)));
		if(logicalParameter->type == LogicalParameter::Type::Enum::typeEnum)
		{
			if(HelperFunctions::isNumber(value)) convertedValue.reset(new RPCVariable(HelperFunctions::getNumber(value)));
			else //value is id of enum element
			{
				LogicalParameterEnum* parameter = (LogicalParameterEnum*)logicalParameter.get();
				for(std::vector<ParameterOption>::iterator i = parameter->options.begin(); i != parameter->options.end(); ++i)
				{
					if(i->id == value)
					{
						convertedValue.reset(new RPCVariable(i->index));
						break;
					}
				}
				if(!convertedValue) convertedValue.reset(new RPCVariable(0));
			}
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeBoolean || logicalParameter->type == LogicalParameter::Type::Enum::typeAction)
		{
			convertedValue.reset(new RPCVariable(false));
			if(HelperFunctions::toLower(value) == "true") convertedValue->booleanValue = true;
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeFloat) convertedValue.reset(new RPCVariable(HelperFunctions::getDouble(value)));
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeString) convertedValue.reset(new RPCVariable(value));
		if(!convertedValue)
		{
			Output::printWarning("Warning: Could not convert parameter " + id + " from String.");
			return std::vector<uint8_t>();
		}
		return convertToPacket(convertedValue);
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::vector<uint8_t>();
}

std::vector<uint8_t> Parameter::convertToPacket(std::shared_ptr<RPCVariable> value)
{
	std::vector<uint8_t> data;
	try
	{
		if(!value) return data;
		if(logicalParameter->type == LogicalParameter::Type::Enum::typeEnum && conversion.empty())
		{
			LogicalParameterEnum* parameter = (LogicalParameterEnum*)logicalParameter.get();
			if(value->integerValue > parameter->max) value->integerValue = parameter->max;
			if(value->integerValue < parameter->min) value->integerValue = parameter->min;
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeAction && conversion.empty())
		{
			value->integerValue = (int32_t)value->booleanValue;
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeString && conversion.empty())
		{
			if(value->stringValue.size() > 0)
			{
				data.insert(data.end(), value->stringValue.begin(), value->stringValue.end());
			}
			if(data.size() < std::lround(physicalParameter->size)) data.push_back(0); //0 termination. Otherwise parts of old string will still be visible
		}
		else if(!conversion.empty() && conversion.at(0)->type == ParameterConversion::Type::cfm)
		{
			std::shared_ptr<RPCVariable> variable(new RPC::RPCVariable());
			*variable = *value;
			//Cannot currently easily be handled by ParameterConversion::toPacket
			data.resize(14, 0);
			if(variable->stringValue.empty() || variable->stringValue == "0") return data;
			std::istringstream stringStream(variable->stringValue);
			std::string element;

			for(uint32_t i = 0; std::getline(stringStream, element, ',') && i < 13; i++)
			{
				if(i == 0)
				{
					data.at(0) = std::lround(200 * HelperFunctions::getDouble(element));
				}
				else if(i == 1)
				{
					data.at(1) = HelperFunctions::getNumber(element);
				}
				else if(i == 2)
				{
					variable->integerValue = std::lround(HelperFunctions::getDouble(element) * 10);
					ParameterConversion conversion;
					conversion.type = ParameterConversion::Type::integerTinyFloat;
					conversion.toPacket(variable);
					std::vector<uint8_t> time;
					HelperFunctions::memcpyBigEndian(time, variable->integerValue);
					if(time.size() == 1) data.at(13) = time.at(0);
					else
					{
						data.at(12) = time.at(0);
						data.at(13) = time.at(1);
					}
				}
				else data.at(i - 1) = HelperFunctions::getNumber(element);
			}
			return data;
		}
		else
		{
			if(logicalParameter->type == LogicalParameter::Type::Enum::typeFloat)
			{
				LogicalParameterFloat* parameter = (LogicalParameterFloat*)logicalParameter.get();
				bool specialValue = (value->floatValue == parameter->defaultValue);
				if(!specialValue)
				{
					for(std::unordered_map<std::string, double>::const_iterator i = parameter->specialValues.begin(); i != parameter->specialValues.end(); ++i)
					{
						if(i->second == value->floatValue)
						{
							specialValue = true;
							break;
						}
					}
				}
				if(!specialValue)
				{
					if(value->floatValue > parameter->max) value->floatValue = parameter->max;
					else if(value->floatValue < parameter->min) value->floatValue = parameter->min;
				}
			}
			else if(logicalParameter->type == LogicalParameter::Type::Enum::typeInteger)
			{
				LogicalParameterInteger* parameter = (LogicalParameterInteger*)logicalParameter.get();
				bool specialValue = (value->integerValue == parameter->defaultValue);
				if(!specialValue)
				{
					for(std::unordered_map<std::string, int32_t>::const_iterator i = parameter->specialValues.begin(); i != parameter->specialValues.end(); ++i)
					{
						if(i->second == value->integerValue)
						{
							specialValue = true;
							break;
						}
					}
				}
				if(!specialValue)
				{
					if(value->integerValue > parameter->max) value->integerValue = parameter->max;
					else if(value->integerValue < parameter->min) value->integerValue = parameter->min;
				}
			}
			std::shared_ptr<RPCVariable> variable(new RPC::RPCVariable());
			*variable = *value;
			if(conversion.empty())
			{
				if(logicalParameter->type == LogicalParameter::Type::Enum::typeBoolean) variable->integerValue = (int32_t)variable->booleanValue;
			}
			else
			{
				for(std::vector<std::shared_ptr<ParameterConversion>>::iterator i = conversion.begin(); i != conversion.end(); ++i)
				{
					(*i)->toPacket(variable);
				}
			}
			value->integerValue = variable->integerValue;
		}
		if(physicalParameter->type != PhysicalParameter::Type::Enum::typeString)
		{
			if(physicalParameter->sizeDefined) //Values have no size defined. That is a problem if the value is larger than 1 byte.
			{
				//Crop to size. Most importantly for negative numbers
				uint32_t byteSize = std::lround(std::floor(physicalParameter->size));
				int32_t bitSize = std::lround(physicalParameter->size * 10) % 10;
				if(byteSize >= 4)
				{
					byteSize = 4;
					bitSize = 0;
				}
				int32_t valueMask = 0xFFFFFFFF >> (((4 - byteSize) * 8) - bitSize);
				value->integerValue &= valueMask;
			}
			HelperFunctions::memcpyBigEndian(data, value->integerValue);
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return data;
}

Parameter::Parameter(xml_node<>* node, bool checkForID) : Parameter()
{
	logicalParameter = std::shared_ptr<LogicalParameter>(new LogicalParameterInteger());
	physicalParameter = std::shared_ptr<PhysicalParameter>(new PhysicalParameter());
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "index") index = std::stod(attributeValue);
		else if(attributeName == "size") size = std::stod(attributeValue);
		else if(attributeName == "signed") { if(attributeValue == "true") isSigned = true; }
		else if(attributeName == "cond_op")
		{
			HelperFunctions::toLower(HelperFunctions::trim(attributeValue));
			if(attributeValue == "e" || attributeValue == "eq") booleanOperator = BooleanOperator::Enum::e;
			else if(attributeValue == "g") booleanOperator = BooleanOperator::Enum::g;
			else if(attributeValue == "l") booleanOperator = BooleanOperator::Enum::l;
			else if(attributeValue == "ge") booleanOperator = BooleanOperator::Enum::ge;
			else if(attributeValue == "le") booleanOperator = BooleanOperator::Enum::le;
			else Output::printWarning("Warning: Unknown attribute value for \"cond_op\" in node \"parameter\": " + attributeValue);
		}
		else if(attributeName == "const_value") constValue = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "id") id = attributeValue;
		else if(attributeName == "param") param = attributeValue;
		else if(attributeName == "PARAM") additionalParameter = attributeValue;
		else if(attributeName == "control") control = attributeValue;
		else if(attributeName == "loopback") { if(attributeValue == "true") loopback = true; }
		else if(attributeName == "hidden") { if(attributeValue == "true") hidden = true; } //Ignored actually
		else if(attributeName == "default") {} //Not necessary and not used
		else if(attributeName == "burst_suppression")
		{
			if(attributeValue != "0") Output::printWarning("Warning: Unknown value for \"burst_suppression\" in node \"parameter\": " + attributeValue);
		}
		else if(attributeName == "type")
		{
			if(attributeValue == "integer") type = PhysicalParameter::Type::Enum::typeInteger;
			else if(attributeValue == "boolean") type = PhysicalParameter::Type::Enum::typeBoolean;
			else if(attributeValue == "string") type = PhysicalParameter::Type::Enum::typeString;
			else if(attributeValue == "option") type = PhysicalParameter::Type::Enum::typeOption;
			else Output::printWarning("Warning: Unknown attribute value for \"type\" in node \"parameter\": " + attributeValue);
		}
		else if(attributeName == "omit_if")
		{
			if(type != PhysicalParameter::Type::Enum::typeInteger)
			{
				Output::printWarning("Warning: \"omit_if\" is only supported for type \"integer\" in node \"parameter\".");
				continue;
			}
			omitIfSet = true;
			omitIf = HelperFunctions::getNumber(attributeValue);
		}
		else if(attributeName == "operations")
		{
			operations = Operations::Enum::none;
			std::stringstream stream(attributeValue);
			std::string element;
			while(std::getline(stream, element, ','))
			{
				HelperFunctions::toLower(HelperFunctions::trim(element));
				if(element == "read") operations = (Operations::Enum)(operations | Operations::Enum::read);
				else if(element == "write") operations = (Operations::Enum)(operations | Operations::Enum::write);
				else if(element == "event") operations = (Operations::Enum)(operations | Operations::Enum::event);
			}
		}
		else if(attributeName == "ui_flags")
		{
			uiFlags = UIFlags::Enum::none; //Remove visible
			std::stringstream stream(attributeValue);
			std::string element;
			while(std::getline(stream, element, ','))
			{
				HelperFunctions::toLower(HelperFunctions::trim(element));
				if(element == "visible") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::visible);
				else if(element == "internal") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::internal);
				else if(element == "transform") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::transform);
				else if(element == "service") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::service);
				else if(element == "sticky") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::sticky);
			}
		}
		else Output::printWarning("Warning: Unknown attribute for \"parameter\": " + attributeName);
	}
	if(checkForID && id.empty() && GD::debugLevel >= 2)
	{
		Output::printError("Error: Parameter has no id. Index: " + std::to_string(index));
	}
	for(xml_node<>* parameterNode = node->first_node(); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		std::string nodeName(parameterNode->name());
		if(nodeName == "logical")
		{
			std::shared_ptr<LogicalParameter> parameter = LogicalParameter::fromXML(parameterNode);
			if(parameter) logicalParameter = parameter;
		}
		else if(nodeName == "physical")
		{
			physicalParameter.reset(new PhysicalParameter(parameterNode));
			for(std::vector<std::shared_ptr<PhysicalParameterEvent>>::iterator i = physicalParameter->eventFrames.begin(); i != physicalParameter->eventFrames.end(); ++i)
			{
				if((*i)->dominoEvent)
				{
					hasDominoEvents = true;
					break;
				}
			}
		}
		else if(nodeName == "conversion")
		{
			std::shared_ptr<ParameterConversion> parameterConversion(new ParameterConversion(parameterNode));
			if(parameterConversion && parameterConversion->type != ParameterConversion::Type::Enum::none) conversion.push_back(parameterConversion);
		}
		else if(nodeName == "description")
		{
			description = ParameterDescription(parameterNode);
		}
		else Output::printWarning("Warning: Unknown subnode for \"parameter\": " + nodeName);
	}
	if(logicalParameter->type == LogicalParameter::Type::Enum::typeFloat)
	{
		LogicalParameterFloat* parameter = (LogicalParameterFloat*)logicalParameter.get();
		if(parameter->min < 0 && parameter->min != std::numeric_limits<double>::min()) isSigned = true;
	}
}

void Parameter::adjustBitPosition(std::vector<uint8_t>& data)
{
	try
	{
		if(data.size() > 4 || data.empty()) return;
		int32_t value = 0;
		HelperFunctions::memcpyBigEndian(value, data);
		if(physicalParameter->size < 0)
		{
			Output::printError("Error: Negative size not allowed.");
			return;
		}
		double i = physicalParameter->index;
		i -= std::floor(i);
		double byteIndex = std::floor(i);
		if(byteIndex != i || physicalParameter->size < 0.8) //0.8 == 8 Bits
		{
			if(physicalParameter->size > 1)
			{
				Output::printError("Error: Can't set partial byte index > 1.");
				return;
			}
			data.clear();
			data.push_back(value << (std::lround(i * 10) % 10));
		}
		//Adjust data size. See for example ENDTIME_SATURDAY_1 and TEMPERATURE_SATURDAY_1 of HM-CC-RT-DN
		if((int32_t)physicalParameter->size > data.size())
		{
			uint32_t bytesMissing = (int32_t)physicalParameter->size - data.size();
			std::vector<uint8_t> oldData = data;
			data.clear();
			for(uint32_t i = 0; i < bytesMissing; i++) data.push_back(0);
			for(uint32_t i = 0; i < oldData.size(); i++) data.push_back(oldData.at(i));
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool DeviceType::matches(LogicalDeviceType deviceType, uint32_t firmwareVersion)
{
	try
	{
		if(parameters.empty() || (device && deviceType.family() != device->family)) return false;
		bool match = true;
		for(std::vector<Parameter>::iterator i = parameters.begin(); i != parameters.end(); ++i)
		{
			//When the device type is not at index 10 of the pairing packet, the device is not supported
			//The "priority" attribute is ignored, for the standard devices "priority" seems not important
			if(i->index == 10.0) { if(i->constValue != deviceType.type()) match = false; }
			else if(i->index == 9.0) { if(!i->checkCondition(firmwareVersion)) match = false; }
			else if(i->index == 0) { if((deviceType.type() >> 8) != i->constValue) match = false; }
			else if(i->index == 1.0) { if((deviceType.type() & 0xFF) != i->constValue) match = false; }
			else if(i->index == 2.0) { if(!i->checkCondition(firmwareVersion)) match = false; }
			else match = false; //Unknown index
		}
		if(match) return true;
	}
	catch(const std::exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
    return false;
}

bool DeviceType::matches(DeviceFamilies family, std::string typeID)
{
	try
	{
		if(device && family != device->family) return false;
		if(id == typeID) return true;
	}
	catch(const std::exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
    return false;
}

bool DeviceType::matches(DeviceFamilies family, std::shared_ptr<Packet> packet)
{
	try
	{
		if(parameters.empty() || (device && family != device->family)) return false;
		for(std::vector<Parameter>::iterator i = parameters.begin(); i != parameters.end(); ++i)
		{
			int32_t intValue = 0;
			std::vector<uint8_t> data = packet->getPosition(i->index, i->size, -1);
			HelperFunctions::memcpyBigEndian(intValue, data);
			if(!i->checkCondition(intValue)) return false;
		}
		return true;
	}
	catch(const std::exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
    return false;
}

DeviceType::DeviceType(xml_node<>* typeNode)
{
	for(xml_attribute<>* attr = typeNode->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "name") name = attributeValue;
		else if(attributeName == "id") id = attributeValue;
		else if(attributeName == "priority") priority = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "updatable") { if(attributeValue == "true") updatable = true; }
		else Output::printWarning("Warning: Unknown attribute for \"type\": " + attributeName);
	}
	for(xml_node<>* parameterNode = typeNode->first_node("parameter"); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		Parameter parameter(parameterNode);
		parameters.push_back(parameter);
	}
}

ParameterSet::ParameterSet(xml_node<>* parameterSetNode)
{
	init(parameterSetNode);
}

std::vector<std::shared_ptr<Parameter>> ParameterSet::getIndices(uint32_t startIndex, uint32_t endIndex, int32_t list)
{
	std::vector<std::shared_ptr<Parameter>> filteredParameters;
	try
	{
		if(list < 0) return filteredParameters;
		for(std::vector<std::shared_ptr<Parameter>>::iterator i = parameters.begin(); i != parameters.end(); ++i)
		{
			if((*i)->physicalParameter->list != (unsigned)list) continue;
			//if((*i)->physicalParameter->index >= startIndex && std::floor((*i)->physicalParameter->index) <= endIndex) filteredParameters.push_back(*i);
			if((*i)->physicalParameter->endIndex >= startIndex && (*i)->physicalParameter->startIndex <= endIndex) filteredParameters.push_back(*i);
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return filteredParameters;
}

std::shared_ptr<Parameter> ParameterSet::getIndex(double index)
{
	try
	{
		for(std::vector<std::shared_ptr<Parameter>>::iterator i = parameters.begin(); i != parameters.end(); ++i)
		{
			if((*i)->physicalParameter->index == index) return *i;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<Parameter>();
}

std::string ParameterSet::typeString()
{
	switch(type)
	{
	case Type::Enum::master:
		return "MASTER";
	case Type::Enum::values:
		return "VALUES";
	case Type::Enum::link:
		return "LINK";
	case Type::Enum::none:
		return "";
	}
	return "";
}

ParameterSet::Type::Enum ParameterSet::typeFromString(std::string type)
{
	HelperFunctions::toLower(HelperFunctions::trim(type));
	if(type == "master") return Type::Enum::master;
	else if(type == "values") return Type::Enum::values;
	else if(type == "link") return Type::Enum::link;
	return Type::Enum::none;
}

std::shared_ptr<Parameter> ParameterSet::getParameter(std::string id)
{
	try
	{
		for(std::vector<std::shared_ptr<Parameter>>::iterator i = parameters.begin(); i != parameters.end(); ++i)
		{
			if((*i)->id == id) return *i;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<Parameter>();
}

void ParameterSet::init(xml_node<>* parameterSetNode)
{
	for(xml_attribute<>* attr = parameterSetNode->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id") id = attributeValue;
		else if(attributeName == "type")
		{
			std::stringstream stream(attributeValue);
			type = typeFromString(attributeValue);
			if(type == Type::Enum::none) Output::printWarning("Warning: Unknown parameter set type: " + attributeValue);
		}
		else if(attributeName == "address_start") addressStart = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "address_step") addressStep = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "count") count = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "channel_offset") channelOffset = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "peer_address_offset") peerAddressOffset = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "peer_channel_offset") peerChannelOffset = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "link") {} //Ignored
		else Output::printWarning("Warning: Unknown attribute for \"paramset\": " + attributeName);
	}
	std::vector<std::pair<std::string, std::string>> enforce;
	for(xml_node<>* parameterNode = parameterSetNode->first_node(); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		std::string nodeName(parameterNode->name());
		if(nodeName == "parameter")
		{
			std::shared_ptr<Parameter> parameter(new Parameter(parameterNode, true));
			parameters.push_back(parameter);
			parameter->parentParameterSet = this;
			if(parameter->physicalParameter->list < 9999) lists[parameter->physicalParameter->list] = 1;
		}
		else if(nodeName == "enforce")
		{
			xml_attribute<>* attr1 = parameterNode->first_attribute("id");
			xml_attribute<>* attr2 = parameterNode->first_attribute("value");
			if(!attr1 || !attr2)
			{
				Output::printWarning("Warning: Could not parse \"enforce\". Attribute id or value not set.");
				continue;
			}
			enforce.push_back(std::pair<std::string, std::string>(std::string(attr1->value()), std::string(attr2->value())));
		}
		else if(nodeName == "subset")
		{
			xml_attribute<>* attr = parameterNode->first_attribute("ref");
			if(!attr)
			{
				Output::printWarning("Warning: Could not parse \"subset\". Attribute ref not set.");
				continue;
			}
			subsetReference = std::string(attr->value());
		}
		else if(nodeName == "default_values")
		{
			xml_attribute<>* attr = parameterNode->first_attribute("function");
			if(!attr)
			{
				Output::printWarning("Warning: Could not parse \"subset\". Attribute ref not set.");
				continue;
			}
			std::string function = std::string(attr->value());
			if(function.empty()) continue;
			for(xml_node<>* defaultValueNode = parameterNode->first_node(); defaultValueNode; defaultValueNode = defaultValueNode->next_sibling())
			{
				std::string nodeName(defaultValueNode->name());
				if(nodeName == "value")
				{
					xml_attribute<>* attr1 = defaultValueNode->first_attribute("id");
					xml_attribute<>* attr2 = defaultValueNode->first_attribute("value");
					if(!attr1 || !attr2)
					{
						Output::printWarning("Warning: Could not parse \"value\" (in default_values). Attribute id or value not set.");
						continue;
					}
					defaultValues[function].push_back(std::pair<std::string, std::string>(std::string(attr1->value()), std::string(attr2->value())));
				}
				else Output::printWarning("Warning: Unknown node name for \"default_values\": " + nodeName);
			}
		}
		else Output::printWarning("Warning: Unknown node name for \"paramset\": " + nodeName);
	}
	for(std::vector<std::pair<std::string, std::string>>::iterator i = enforce.begin(); i != enforce.end(); ++i)
	{
		if(i->first.empty() || i->second.empty()) continue;
		std::shared_ptr<Parameter> parameter = getParameter(i->first);
		if(!parameter) continue;
		parameter->logicalParameter->enforce = true;
		if(parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeInteger)
		{
			LogicalParameterInteger* logicalParameter = (LogicalParameterInteger*)parameter->logicalParameter.get();
			logicalParameter->enforceValue = HelperFunctions::getNumber(i->second);
		}
		else if(parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeBoolean)
		{
			LogicalParameterBoolean* logicalParameter = (LogicalParameterBoolean*)parameter->logicalParameter.get();
			if(HelperFunctions::toLower(i->second) == "true") logicalParameter->enforceValue = true;
		}
		else if(parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeFloat)
		{
			LogicalParameterFloat* logicalParameter = (LogicalParameterFloat*)parameter->logicalParameter.get();
			logicalParameter->enforceValue = HelperFunctions::getDouble(i->second);
		}
		else if(parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeAction)
		{
			LogicalParameterAction* logicalParameter = (LogicalParameterAction*)parameter->logicalParameter.get();
			if(HelperFunctions::toLower(i->second) == "true") logicalParameter->enforceValue = true;
		}
		else if(parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeEnum)
		{
			LogicalParameterEnum* logicalParameter = (LogicalParameterEnum*)parameter->logicalParameter.get();
			logicalParameter->enforceValue = HelperFunctions::getNumber(i->second);
		}
		else if(parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeString)
		{
			LogicalParameterString* logicalParameter = (LogicalParameterString*)parameter->logicalParameter.get();
			logicalParameter->enforceValue = i->second;
		}
	}
}

LinkRole::LinkRole(xml_node<>* node)
{
	for(xml_node<>* linkRoleNode = node->first_node(); linkRoleNode; linkRoleNode = linkRoleNode->next_sibling())
	{
		std::string nodeName(linkRoleNode->name());
		if(nodeName == "target")
		{
			xml_attribute<>* attr = linkRoleNode->first_attribute("name");
			if(attr != nullptr) targetNames.push_back(attr->value());
		}
		else if(nodeName == "source")
		{
			xml_attribute<>* attr = linkRoleNode->first_attribute("name");
			if(attr != nullptr) sourceNames.push_back(attr->value());
		}
		else Output::printWarning("Warning: Unknown node name for \"link_roles\": " + nodeName);
	}
}

EnforceLink::EnforceLink(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id") id = attributeValue;
		else if(attributeName == "value") value = attributeValue;
		else Output::printWarning("Warning: Unknown attribute for \"enforce_link - value\": " + attributeName);
	}
}

std::shared_ptr<RPCVariable> EnforceLink::getValue(LogicalParameter::Type::Enum type)
{
	try
	{
		RPCVariableType rpcType = (RPCVariableType)type;
		if(type == LogicalParameter::Type::Enum::typeEnum) rpcType = RPCVariableType::rpcInteger;
		else if(type == LogicalParameter::Type::Enum::typeAction) rpcType = RPCVariableType::rpcBoolean;
		return RPCVariable::fromString(value, rpcType);
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<RPCVariable>();
}

DeviceChannel::DeviceChannel(xml_node<>* node, uint32_t& index)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "index") index = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "physical_index_offset") physicalIndexOffset = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "ui_flags")
		{
			if(attributeValue == "visible") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::visible);
			else if(attributeValue == "internal") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::internal);
			else if(attributeValue == "dontdelete") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::dontdelete);
			else Output::printWarning("Warning: Unknown ui flag for \"channel\": " + attributeValue);
		}
		else if(attributeName == "direction")
		{
			if(attributeValue == "sender") direction = (Direction::Enum)(direction | Direction::Enum::sender);
			else if(attributeValue == "receiver") direction = (Direction::Enum)(direction | Direction::Enum::receiver);
			else Output::printWarning("Warning: Unknown direction for \"channel\": " + attributeValue);
		}
		else if(attributeName == "class") channelClass = attributeValue;
		else if(attributeName == "type") type = attributeValue;
		else if(attributeName == "hidden") { if(attributeValue == "true") hidden = true; }
		else if(attributeName == "autoregister") { if(attributeValue == "true") autoregister = true; }
		else if(attributeName == "count") count = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "has_team") { if(attributeValue == "true") hasTeam = true; }
		else if(attributeName == "aes_default") { if(attributeValue == "true") aesDefault = true; }
		else if(attributeName == "aes_always") { if(attributeValue == "true") aesDefault = true; }
		else if(attributeName == "aes_cbc") { if(attributeValue == "true") aesCBC = true; }
		else if(attributeName == "team_tag") teamTag = attributeValue;
		else if(attributeName == "paired") { if(attributeValue == "true") paired = true; }
		else if(attributeName == "function") function = attributeValue;
		else if(attributeName == "pair_function")
		{
			if(attributeValue.size() != 2)
			{
				Output::printWarning("Warning: pair_function does not consist of two functions.");
				continue;
			}
			pairFunction1 = attributeValue.substr(0, 1);
			pairFunction2 = attributeValue.substr(1, 1);
		}
		else if(attributeName == "count_from_sysinfo")
		{
			std::pair<std::string, std::string> splitValue = HelperFunctions::split(attributeValue, ':');
			if(!splitValue.first.empty())
			{
				countFromSysinfo = HelperFunctions::getDouble(splitValue.first);
				if(countFromSysinfo < 9)
				{
					Output::printError("Error: count_from_sysinfo has to be >= 9.");
					countFromSysinfo = -1;
				}
			}
			if(!splitValue.second.empty())
			{
				countFromSysinfoSize = HelperFunctions::getDouble(splitValue.second);
				if(countFromSysinfoSize > 1)
				{
					Output::printError("Error: The size of count_from_sysinfo has to be <= 1.");
					countFromSysinfoSize = 1;
				}
			}
		}
		else Output::printWarning("Warning: Unknown attribute for \"channel\": " + attributeName);
	}
	for(xml_node<>* channelNode = node->first_node(); channelNode; channelNode = channelNode->next_sibling())
	{
		std::string nodeName(channelNode->name());
		if(nodeName == "paramset")
		{
			std::shared_ptr<ParameterSet> parameterSet(new ParameterSet(channelNode));
			if(parameterSets.find(parameterSet->type) == parameterSets.end()) parameterSets[parameterSet->type] = parameterSet;
			else Output::printError("Error: Tried to add same parameter set type twice.");
		}
		else if(nodeName == "link_roles")
		{
			if(linkRoles) Output::printWarning("Warning: Multiple link roles are defined for channel " + std::to_string(index) + ".");
			linkRoles.reset(new LinkRole(channelNode));
		}
		else if(nodeName == "enforce_link")
		{
			for(xml_node<>* enforceLinkNode = channelNode->first_node("value"); enforceLinkNode; enforceLinkNode = enforceLinkNode->next_sibling("value"))
			{
				enforceLinks.push_back(std::shared_ptr<EnforceLink>(new EnforceLink(enforceLinkNode)));
			}
		}
		else if(nodeName == "special_parameter") specialParameter.reset(new Parameter(channelNode, true));
		else if(nodeName == "subconfig")
		{
			uint32_t index = 0;
			subconfig.reset(new DeviceChannel(channelNode, index));
		}
		else Output::printWarning("Warning: Unknown node name for \"device\": " + nodeName);
	}
}

Device::Device()
{
	parameterSet.reset(new ParameterSet());
}

Device::Device(std::string xmlFilename) : Device()
{
	try
	{
		load(xmlFilename);

		if(!_loaded || channels.empty()) return; //Happens when file couldn't be read

		std::shared_ptr<Parameter> parameter(new Parameter());
		parameter->id = "PAIRED_TO_CENTRAL";
		parameter->uiFlags = Parameter::UIFlags::Enum::hidden;
		parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeBoolean;
		parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
		parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeBoolean;
		parameter->physicalParameter->valueID = "PAIRED_TO_CENTRAL";
		parameter->physicalParameter->list = 0;
		parameter->physicalParameter->index = 2;
		channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);

		parameter.reset(new Parameter());
		parameter->id = "CENTRAL_ADDRESS_BYTE_1";
		parameter->uiFlags = Parameter::UIFlags::Enum::hidden;
		parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
		parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->valueID = "CENTRAL_ADDRESS_BYTE_1";
		parameter->physicalParameter->list = 0;
		parameter->physicalParameter->index = 10;
		channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);

		parameter.reset(new Parameter());
		parameter->id = "CENTRAL_ADDRESS_BYTE_2";
		parameter->uiFlags = Parameter::UIFlags::Enum::hidden;
		parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
		parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->valueID = "CENTRAL_ADDRESS_BYTE_2";
		parameter->physicalParameter->list = 0;
		parameter->physicalParameter->index = 11;
		channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);

		parameter.reset(new Parameter());
		parameter->id = "CENTRAL_ADDRESS_BYTE_3";
		parameter->uiFlags = Parameter::UIFlags::Enum::hidden;
		parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
		parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->valueID = "CENTRAL_ADDRESS_BYTE_1";
		parameter->physicalParameter->list = 0;
		parameter->physicalParameter->index = 12;
		channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

Device::~Device() {

}

void Device::load(std::string xmlFilename)
{
	xml_document<> doc;
	try
	{
		std::ifstream fileStream(xmlFilename, std::ios::in | std::ios::binary);
		if(fileStream)
		{
			uint32_t length;
			fileStream.seekg(0, std::ios::end);
			length = fileStream.tellg();
			fileStream.seekg(0, std::ios::beg);
			char buffer[length + 1];
			fileStream.read(&buffer[0], length);
			fileStream.close();
			buffer[length] = '\0';
			doc.parse<parse_no_entity_translation | parse_validate_closing_tags>(buffer);
			parseXML(doc.first_node("device"));
		}
		else Output::printError("Error reading file " + xmlFilename + ": " + strerror(errno));
		_loaded = true;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    doc.clear();
}

void Device::parseXML(xml_node<>* node)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "version") version = HelperFunctions::getNumber(attributeValue);
			else if(attributeName == "family")
			{
				HelperFunctions::toLower(HelperFunctions::trim(attributeValue));
				if(attributeValue == "homematicbidcos") family = DeviceFamilies::HomeMaticBidCoS;
				else if(attributeValue == "homematicwired") family = DeviceFamilies::HomeMaticWired;
			}
			else if(attributeName == "rx_modes")
			{
				std::stringstream stream(attributeValue);
				std::string element;
				rxModes = RXModes::Enum::none;
				while(std::getline(stream, element, ','))
				{
					HelperFunctions::toLower(HelperFunctions::trim(element));
					if(element == "wakeup") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::wakeUp);
					else if(element == "config") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::config);
					else if(element == "burst") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::burst);
					else if(element == "always") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::always);
					else if(element == "lazy_config") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::lazyConfig);
					else Output::printWarning("Warning: Unknown rx mode for \"device\": " + element);
				}
				if(rxModes == RXModes::Enum::none) rxModes = RXModes::Enum::always;
				if(rxModes != RXModes::Enum::always) hasBattery = true;
			}
			else if(attributeName == "class")
			{
				deviceClass = attributeValue;
			}
			else if(attributeName == "eep_size")
			{
				eepSize = HelperFunctions::getNumber(attributeValue);
			}
			else if(attributeName == "ui_flags")
			{
				if(attributeValue == "visible") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::visible);
				else if(attributeValue == "internal") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::internal);
				else if(attributeValue == "dontdelete") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::dontdelete);
				else Output::printWarning("Warning: Unknown ui flag for \"channel\": " + attributeValue);
			}
			else if(attributeName == "cyclic_timeout") cyclicTimeout = HelperFunctions::getNumber(attributeValue);
			else if(attributeName == "supports_aes") { if(attributeValue == "true") supportsAES = true; }
			else if(attributeName == "peering_sysinfo_expect_channel") { if(attributeValue == "false") peeringSysinfoExpectChannel = false; }
			else Output::printWarning("Warning: Unknown attribute for \"device\": " + attributeName);
		}

		std::map<std::string, std::shared_ptr<ParameterSet>> parameterSetDefinitions;
		for(node = node->first_node(); node; node = node->next_sibling())
		{
			std::string nodeName(node->name());
			if(nodeName == "supported_types")
			{
				for(xml_node<>* typeNode = node->first_node("type"); typeNode; typeNode = typeNode->next_sibling())
				{
					std::shared_ptr<DeviceType> deviceType(new DeviceType(typeNode));
					deviceType->device = this;
					supportedTypes.push_back(deviceType);
				}
			}
			else if(nodeName == "paramset")
			{
				for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
				{
					std::string attributeName(attr->name());
					std::string attributeValue(attr->value());
					HelperFunctions::toLower(HelperFunctions::trim(attributeValue));
					if(attributeName == "id") parameterSet->id = attributeValue;
					else if(attributeName == "type")
					{
						if(attributeValue == "master") parameterSet->type = ParameterSet::Type::Enum::master;
						else Output::printError("Error: Tried to add parameter set of type \"" + attributeValue + "\" to device. That is not allowed.");
					}
					else Output::printWarning("Warning: Unknown attribute for \"paramset\": " + attributeName);
				}
				parameterSet->init(node);
			}
			else if(nodeName == "paramset_defs")
			{
				for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
				{
					std::string attributeName(attr->name());
					std::string attributeValue(attr->value());
					HelperFunctions::toLower(HelperFunctions::trim(attributeValue));
					if(attributeName == "id") parameterSet->id = attributeValue;
					else Output::printWarning("Warning: Unknown attribute for \"paramset_defs\": " + attributeName);
				}

				for(xml_node<>* paramsetNode = node->first_node(); paramsetNode; paramsetNode = paramsetNode->next_sibling())
				{
					std::string nodeName(paramsetNode->name());
					if(nodeName == "paramset")
					{
						std::shared_ptr<ParameterSet> parameterSet(new ParameterSet(paramsetNode));
						parameterSetDefinitions[parameterSet->id] = parameterSet;
					}
					else Output::printWarning("Warning: Unknown node name for \"paramset_defs\": " + nodeName);
				}
			}
			else if(nodeName == "channels")
			{
				for(xml_node<>* channelNode = node->first_node("channel"); channelNode; channelNode = channelNode->next_sibling())
				{
					uint32_t index = 0;
					std::shared_ptr<DeviceChannel> channel(new DeviceChannel(channelNode, index));
					channel->parentDevice = this;
					for(uint32_t i = index; i < index + channel->count; i++)
					{
						if(channels.find(i) == channels.end()) channels[i] = channel;
						else Output::printError("Error: Tried to add channel with the same index twice. Index: " + std::to_string(i));
					}
					if(channel->countFromSysinfo)
					{
						if(countFromSysinfoIndex > -1) Output::printError("Error: count_from_sysinfo is defined for two channels. That is not allowed.");
						if(std::floor(channel->countFromSysinfo) != channel->countFromSysinfo)
						{
							Output::printError("Error: count_from_sysinfo has to start with index 0 of a byte.");
							continue;
						}
						countFromSysinfoIndex = (int32_t)channel->countFromSysinfo;
						countFromSysinfoSize = channel->countFromSysinfoSize;
						if(countFromSysinfoIndex < -1) countFromSysinfoIndex = -1;
						if(countFromSysinfoSize <= 0) countFromSysinfoSize = 1;
					}
				}
			}
			else if(nodeName == "frames")
			{
				for(xml_node<>* frameNode = node->first_node("frame"); frameNode; frameNode = frameNode->next_sibling("frame"))
				{
					std::shared_ptr<DeviceFrame> frame(new DeviceFrame(frameNode));
					framesByMessageType.insert(std::pair<uint32_t, std::shared_ptr<DeviceFrame>>(frame->type, frame));
					framesByID[frame->id] = frame;
				}
			}
			else if(nodeName == "team")
			{
				team.reset(new Device());
				team->parseXML(node);
			}
			else Output::printWarning("Warning: Unknown node name for \"device\": " + nodeName);
		}

		if(!parameterSetDefinitions.empty())
		{
			std::map<uint32_t, std::map<ParameterSet::Type::Enum, std::shared_ptr<ParameterSet>>> parameterSetsToAdd;
			for(std::map<uint32_t, std::shared_ptr<DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
			{
				for(std::map<ParameterSet::Type::Enum, std::shared_ptr<ParameterSet>>::iterator j = i->second->parameterSets.begin(); j != i->second->parameterSets.end(); ++j)
				{
					if(j->second->subsetReference.empty() || parameterSetDefinitions.find(j->second->subsetReference) == parameterSetDefinitions.end()) continue;
					std::shared_ptr<ParameterSet> parameterSet(new ParameterSet());
					*parameterSet = *parameterSetDefinitions.at(j->second->subsetReference);
					parameterSet->type = j->second->type;
					parameterSet->id = j->second->id;
					//We can't change the map while we are iterating through it, so we temporarily store the parameterSet
					parameterSetsToAdd[i->first][j->first] = parameterSet;
					for(std::vector<std::shared_ptr<Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
					{
						(*i)->parentParameterSet = parameterSet.get();
					}
				}
			}
			for(std::map<uint32_t, std::map<ParameterSet::Type::Enum, std::shared_ptr<ParameterSet>>>::iterator i = parameterSetsToAdd.begin(); i != parameterSetsToAdd.end(); ++i)
			{
				for(std::map<ParameterSet::Type::Enum, std::shared_ptr<ParameterSet>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					channels[i->first]->parameterSets[j->first] = j->second;
				}
			}
		}

		if(!channels[0]) channels[0] = std::shared_ptr<DeviceChannel>(new DeviceChannel());
		if(!channels[0]->parameterSets[ParameterSet::Type::Enum::master]) channels[0]->parameterSets[ParameterSet::Type::Enum::master] = std::shared_ptr<ParameterSet>(new ParameterSet());
		if(parameterSet->type == ParameterSet::Type::Enum::master && !parameterSet->parameters.empty())
		{
			if(channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.size() > 0)
			{
				Output::printError("Error: Master parameter set of channnel 0 has to be empty.");
			}
			channels[0]->parameterSets[ParameterSet::Type::Enum::master] = parameterSet;
		}

		for(std::map<uint32_t, std::shared_ptr<DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
		{
			if(!i->second || i->second->parameterSets.find(ParameterSet::Type::Enum::values) == i->second->parameterSets.end()) continue;
			for(std::vector<std::shared_ptr<Parameter>>::iterator j = i->second->parameterSets.at(ParameterSet::Type::Enum::values)->parameters.begin(); j != i->second->parameterSets.at(ParameterSet::Type::Enum::values)->parameters.end(); ++j)
			{
				if(!*j) continue;
				if(!(*j)->physicalParameter->getRequest.empty() && framesByID.find((*j)->physicalParameter->getRequest) != framesByID.end()) framesByID[(*j)->physicalParameter->getRequest]->associatedValues.push_back(*j);
				if(!(*j)->physicalParameter->setRequest.empty() && framesByID.find((*j)->physicalParameter->setRequest) != framesByID.end()) framesByID[(*j)->physicalParameter->setRequest]->associatedValues.push_back(*j);
				for(std::vector<std::shared_ptr<PhysicalParameterEvent>>::iterator k = (*j)->physicalParameter->eventFrames.begin(); k != (*j)->physicalParameter->eventFrames.end(); ++k)
				{
					if(framesByID.find((*k)->frame) != framesByID.end())
					{
						std::shared_ptr<DeviceFrame> frame = framesByID[(*k)->frame];
						frame->associatedValues.push_back(*j);
						frame->channelIndexOffset = i->second->physicalIndexOffset;
						//For float variables the frame is the only location to find out if it is signed
						for(std::vector<Parameter>::iterator l = frame->parameters.begin(); l != frame->parameters.end(); l++)
						{
							if((l->param == (*j)->id || l->additionalParameter == (*j)->id))
							{
								if(l->isSigned) (*j)->isSigned = true;
								if(l->size > 0) (*j)->physicalParameter->size = l->size;
							}
						}
					}
				}
			}
		}
	}
    catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

int32_t Device::getCountFromSysinfo(std::shared_ptr<Packet> packet)
{
	try
	{
		if(!packet) return -1;
		uint32_t bitSize = 8;
		if(countFromSysinfoSize < 1) bitSize = std::lround(countFromSysinfoSize * 10);
		if(bitSize > 8) bitSize = 8;
		int32_t bitMask = (1 << bitSize) - 1;
		if(countFromSysinfoIndex > -1 && ((uint32_t)(countFromSysinfoIndex - 9) < packet->payload()->size())) return packet->payload()->at(countFromSysinfoIndex - 9) & bitMask;
		return -1;
	}
    catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return -1;
}

void Device::setCountFromSysinfo(int32_t countFromSysinfo)
{
	try
	{
		_countFromSysinfo = countFromSysinfo;
		std::shared_ptr<DeviceChannel> channel;
		uint32_t index = 0;
		for(std::map<uint32_t, std::shared_ptr<DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
		{
			if(i->second->countFromSysinfo > -1)
			{
				index = i->first;
				channel = i->second;
				break;
			}
		}
		for(uint32_t i = index + 1; i < index + countFromSysinfo; i++)
		{
			if(channels.find(i) == channels.end()) channels[i] = channel;
			else Output::printError("Error: Tried to add channel with the same index twice. Index: " + std::to_string(i));
		}
	}
    catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<DeviceType> Device::getType(LogicalDeviceType deviceType, int32_t firmwareVersion)
{
	try
	{
		for(std::vector<std::shared_ptr<DeviceType>>::iterator j = supportedTypes.begin(); j != supportedTypes.end(); ++j)
		{
			if((*j)->matches(deviceType, firmwareVersion)) return *j;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<DeviceType>();
}

}
