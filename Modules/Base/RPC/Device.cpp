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

#include "Device.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace RPC
{

DescriptionField::DescriptionField(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id") id = attributeValue;
		else if(attributeName == "value") value = attributeValue;
		else std::cout << "Warning: Unknown attribute for \"field\": " << attributeName << std::endl;
	}
}

ParameterDescription::ParameterDescription(xml_node<>* node)
{
	for(xml_node<>* descriptionNode = node->first_node(); descriptionNode; descriptionNode = descriptionNode->next_sibling())
	{
		std::string nodeName(descriptionNode->name());
		if(nodeName == "field")
		{
			DescriptionField field(descriptionNode);
			fields.push_back(field);
		}
		else std::cout << "Warning: Unknown subnode for \"description\": " << nodeName << std::endl;
	}
}

DeviceFrame::DeviceFrame(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

DeviceFrame::DeviceFrame(BaseLib::Obj* baseLib, xml_node<>* node) : DeviceFrame(baseLib)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "direction")
		{
			if(attributeValue == "from_device") direction = Direction::Enum::fromDevice;
			else if(attributeValue == "to_device") direction = Direction::Enum::toDevice;
			else _bl->out.printWarning("Warning: Unknown direction for \"frame\": " + attributeValue);
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
			else type = Math::getNumber(attributeValue);
		}
		else if(attributeName == "subtype") subtype = Math::getNumber(attributeValue);
		else if(attributeName == "subtype_index")
		{
			std::pair<std::string, std::string> splitString = HelperFunctions::split(attributeValue, ':');
			subtypeIndex = Math::getNumber(splitString.first);
			if(!splitString.second.empty()) subtypeFieldSize = Math::getDouble(splitString.second);
		}
		else if(attributeName == "response_type") responseType = Math::getNumber(attributeValue);
		else if(attributeName == "response_subtype") responseSubtype = Math::getNumber(attributeValue);
		else if(attributeName == "channel_field")
		{
			if(channelField == -1) //Might already be set by receiver_channel_field. We don't need both
			{
				std::pair<std::string, std::string> splitString = HelperFunctions::split(attributeValue, ':');
				channelField = Math::getNumber(splitString.first);
				if(!splitString.second.empty()) channelFieldSize = Math::getDouble(splitString.second);
			}
		}
		else if(attributeName == "receiver_channel_field")
		{
			std::pair<std::string, std::string> splitString = HelperFunctions::split(attributeValue, ':');
			channelField = Math::getNumber(splitString.first);
			if(!splitString.second.empty()) channelFieldSize = Math::getDouble(splitString.second);
		}
		else if(attributeName == "fixed_channel")
		{
			if(attributeValue == "*") fixedChannel = -2;
			else fixedChannel = Math::getNumber(attributeValue);
		}
		else if(attributeName == "size") size = Math::getNumber(attributeValue);
		else if(attributeName == "double_send") { if(attributeValue == "true") doubleSend = true; }
		else if(attributeName == "max_packets") maxPackets = Math::getNumber(attributeValue);
		else if(attributeName == "split_after") splitAfter = Math::getNumber(attributeValue);
		else _bl->out.printWarning("Warning: Unknown attribute for \"frame\": " + attributeName);
	}
	for(xml_node<>* frameNode = node->first_node("parameter"); frameNode; frameNode = frameNode->next_sibling("parameter"))
	{
		Parameter parameter(_bl, frameNode);
		parameters.push_back(parameter);
	}
}

DeviceProgram::DeviceProgram(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

DeviceProgram::DeviceProgram(BaseLib::Obj* baseLib, xml_node<>* node) : DeviceProgram(baseLib)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		_bl->out.printWarning("Warning: Unknown attribute for \"run_program\": " + std::string(attr->name()));
	}
	xml_node<>* pathNode = node->first_node("path");
	if(pathNode)
	{
		for(xml_attribute<>* attr = pathNode->first_attribute(); attr; attr = attr->next_attribute()) _bl->out.printWarning("Warning: Unknown attribute for \"path\": " + std::string(attr->name()));
		path = std::string(pathNode->value());
	}
	xml_node<>* argumentsNode = node->first_node("arguments");
	if(argumentsNode)
	{
		for(xml_attribute<>* attr = pathNode->first_attribute(); attr; attr = attr->next_attribute()) _bl->out.printWarning("Warning: Unknown attribute for \"arguments\": " + std::string(attr->name()));
		for(argumentsNode = argumentsNode->first_node(); argumentsNode; argumentsNode = argumentsNode->next_sibling())
		{
			if(std::string(argumentsNode->name()) == "argument") arguments.push_back(std::string(argumentsNode->value()));
			else _bl->out.printWarning("Warning: Unknown node for \"arguments\": " + std::string(argumentsNode->name()));
		}
	}
	xml_node<>* startTypeNode = node->first_node("start_type");
	if(startTypeNode)
	{
		for(xml_attribute<>* attr = pathNode->first_attribute(); attr; attr = attr->next_attribute()) _bl->out.printWarning("Warning: Unknown attribute for \"startType\": " + std::string(attr->name()));
		std::string value(startTypeNode->value());
		_bl->hf.toLower(value);
		if(value == "once") startType = StartType::Enum::once;
		else if(value == "interval") startType = StartType::Enum::interval;
		else if(value == "permanent") startType = StartType::Enum::permanent;
		else _bl->out.printWarning("Warning: Unknown start_type for \"run_program\": " + value);
	}
	xml_node<>* intervalNode = node->first_node("interval");
	if(intervalNode)
	{
		for(xml_attribute<>* attr = pathNode->first_attribute(); attr; attr = attr->next_attribute()) _bl->out.printWarning("Warning: Unknown attribute for \"interval\": " + std::string(attr->name()));
		std::string value(intervalNode->value());
		interval = Math::getUnsignedNumber(value);
	}
}

void ParameterConversion::fromPacket(std::shared_ptr<RPC::Variable> value)
{
	try
	{
		if(!value) return;
		if(type == Type::Enum::floatIntegerScale)
		{
			value->type = VariableType::rpcFloat;
			value->floatValue = ((double)value->integerValue / factor) - offset;
		}
		else if(type == Type::Enum::integerIntegerScale)
		{
			value->type = VariableType::rpcInteger;
			if(div > 0) value->integerValue *= div;
			if(mul > 0) value->integerValue /= mul;
		}
		else if(type == Type::Enum::integerIntegerMap || type == Type::Enum::optionInteger)
		{
			if(fromDevice && integerValueMapDevice.find(value->integerValue) != integerValueMapDevice.end()) value->integerValue = integerValueMapDevice[value->integerValue];
		}
		else if(type == Type::Enum::booleanInteger)
		{
			value->type = VariableType::rpcBoolean;
			if(valueTrue == 0 && valueFalse == 0)
			{
				if(value->integerValue >= threshold) value->booleanValue = true;
				else value->booleanValue = false;
			}
			else
			{
				if(value->integerValue == valueFalse) value->booleanValue = false;
				if(value->integerValue == valueTrue || value->integerValue >= threshold) value->booleanValue = true;
			}
			if(invert) value->booleanValue = !value->booleanValue;
		}
		else if(type == Type::Enum::floatConfigTime)
		{
			value->type = VariableType::rpcFloat;
			if(value < 0)
			{
				value->floatValue = 0;
				return;
			}
			if(valueSize > 0 && factors.size() > 0)
			{
				uint32_t bits = std::lround(std::floor(valueSize)) * 8;
				bits += std::lround(valueSize * 10) % 10;
				double factor = factors.at(value->integerValue >> bits);
				value->floatValue = (value->integerValue & (0xFFFFFFFF >> (32 - bits))) * factor;
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
			value->type = VariableType::rpcInteger;
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
			value->integerValue = Math::getNumber(stringValue);
		}
		else if(type == Type::Enum::stringJsonArrayFloat)
		{
			if(_parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeString)
			{
				if(value->arrayValue->size() > 0) value->stringValue = std::to_string(value->arrayValue->at(0)->floatValue);
				if(value->arrayValue->size() > 1)
				{
					for(std::vector<std::shared_ptr<Variable>>::iterator i = value->arrayValue->begin() + 1; i != value->arrayValue->end(); ++i)
					{
						value->stringValue += ';' + std::to_string((*i)->floatValue);
					}
				}
				value->arrayValue->clear();
				value->type = VariableType::rpcString;
			}
			else _bl->out.printWarning("Warning: Only strings can be created from Json arrays.");
		}
		else if(type == Type::Enum::optionString)
		{
			LogicalParameterEnum* logicalEnum = (LogicalParameterEnum*)_parameter->logicalParameter.get();
			value->integerValue = -1;
			for(std::vector<ParameterOption>::iterator i = logicalEnum->options.begin(); i != logicalEnum->options.end(); ++i)
			{
				if(i->id == value->stringValue)
				{
					value->integerValue = i->index;
					break;
				}
			}
			if(value->integerValue < 0)
			{
				_bl->out.printWarning("Warning: Cannot convert json string to enum, because no matching element could be found.");
				value->integerValue = 0;
			}
			value->stringValue = "";
			value->type = VariableType::rpcInteger;
		}
		else if(type == Type::Enum::hexstringBytearray)
		{
			value->stringValue = _bl->hf.getHexString(value->stringValue);
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

void ParameterConversion::toPacket(std::shared_ptr<RPC::Variable> value)
{
	try
	{
		if(!value) return;
		if(type == Type::Enum::floatIntegerScale)
		{
			value->integerValue = std::lround((value->floatValue + offset) * factor);
			value->type = VariableType::rpcInteger;
		}
		else if(type == Type::Enum::integerIntegerScale)
		{
			if(mul > 0) value->integerValue *= mul;
			//mul and div can be set, so no else if
			if(div > 0) value->integerValue /= div;
			value->type = VariableType::rpcInteger;
		}
		else if(type == Type::Enum::integerIntegerMap || type == Type::Enum::optionInteger)
		{
			//Value is not changed, when not in map. That is the desired behavior.
			if(toDevice && integerValueMapParameter.find(value->integerValue) != integerValueMapParameter.end())
			{
				value->integerValue = integerValueMapParameter[value->integerValue];
			}
			value->type = VariableType::rpcInteger;
		}
		else if(type == Type::Enum::booleanInteger)
		{
			if(value->stringValue.size() > 0 && value->stringValue == "true") value->booleanValue = true;
			if(invert) value->booleanValue = !value->booleanValue;
			if(valueTrue == 0 && valueFalse == 0) value->integerValue = (int32_t)value->booleanValue;
			else if(value->booleanValue) value->integerValue = valueTrue;
			else value->integerValue = valueFalse;
			value->type = VariableType::rpcInteger;
		}
		else if(type == Type::Enum::floatConfigTime)
		{
			if(valueSize > 0 && factors.size() > 0)
			{
				uint32_t bits = std::lround(std::floor(valueSize)) * 8;
				bits += std::lround(valueSize * 10) % 10;
				if(value->floatValue < 0) value->floatValue = 0;
				int32_t maxNumber = (1 << bits) - 1;
				int32_t factorIndex = 0;
				while(factorIndex < (signed)factors.size() && (value->floatValue / factors.at(factorIndex)) > maxNumber) factorIndex++;

				value->integerValue = (factorIndex << bits) | std::lround(value->floatValue / factors.at(factorIndex));
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
			value->type = VariableType::rpcInteger;
		}
		else if(type == Type::Enum::integerTinyFloat)
		{
			int64_t maxMantissa = (((int64_t)1 << mantissaSize) - 1);
			int64_t maxExponent = (((int64_t)1 << exponentSize) - 1);
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
			value->type = VariableType::rpcInteger;
		}
		else if(type == Type::Enum::stringUnsignedInteger)
		{
			value->integerValue = Math::getUnsignedNumber(value->stringValue);
			value->type = VariableType::rpcInteger;
		}
		else if(type == Type::Enum::blindTest)
		{
			value->integerValue = Math::getNumber(stringValue);
			value->type = VariableType::rpcInteger;
		}
		else if(type == Type::Enum::optionString)
		{
			if(_parameter->logicalParameter->type == LogicalParameter::Type::typeEnum)
			{
				LogicalParameterEnum* logicalEnum = (LogicalParameterEnum*)_parameter->logicalParameter.get();
				if(value->integerValue >= 0 && value->integerValue < (signed)logicalEnum->options.size())
				{
					value->stringValue = logicalEnum->options.at(value->integerValue).id;
				}
				else _bl->out.printWarning("Warning: Cannot convert variable, because enum index is not valid.");
				value->type = VariableType::rpcString;
				value->integerValue = 0;
			}
		}
		else if(type == Type::Enum::stringJsonArrayFloat)
		{
			if(_parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeString)
			{
				std::vector<std::string> arrayElements = HelperFunctions::splitAll(value->stringValue, ';');
				for(std::vector<std::string>::iterator i = arrayElements.begin(); i != arrayElements.end(); ++i)
				{
					value->arrayValue->push_back(std::shared_ptr<Variable>(new Variable(Math::getDouble(*i))));
				}
				value->type = VariableType::rpcArray;
				value->stringValue = "";
			}
			else _bl->out.printWarning("Warning: Only strings can be converted to Json arrays.");
		}
		else if(type == Type::Enum::hexstringBytearray)
		{
			if(_parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeString)
			{
				value->stringValue = _bl->hf.getBinaryString(value->stringValue);
			}
			else _bl->out.printWarning("Warning: Only strings can be converted to byte arrays.");
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

ParameterConversion::ParameterConversion(BaseLib::Obj* baseLib, Parameter* parameter)
{
	_bl = baseLib;
	_parameter = parameter;
}

ParameterConversion::ParameterConversion(BaseLib::Obj* baseLib, Parameter* parameter, xml_node<>* node) : ParameterConversion(baseLib, parameter)
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
			else if(attributeValue == "ccrtdn_party") type = Type::Enum::ccrtdnParty; //Used in "SUBMIT" of HM-CC-RT-DN
			else if(attributeValue == "rpc_binary") type = Type::Enum::rpcBinary;
			else if(attributeValue == "option_string") type = Type::Enum::optionString;
			else if(attributeValue == "string_json_array_float") type = Type::Enum::stringJsonArrayFloat;
			else if(attributeValue == "hexstring_bytearray") type = Type::Enum::hexstringBytearray;
			else _bl->out.printWarning("Warning: Unknown type for \"conversion\": " + attributeValue);
		}
		else if(attributeName == "factor") factor = Math::getDouble(attributeValue);
		else if(attributeName == "factors")
		{
			std::stringstream stream(attributeValue);
			std::string element;
			factors.clear();
			while(std::getline(stream, element, ',')) factors.push_back(Math::getDouble(element));
		}
		else if(attributeName == "value_size") valueSize = Math::getDouble(attributeValue);
		else if(attributeName == "threshold") threshold = Math::getNumber(attributeValue);
		else if(attributeName == "false") valueFalse = Math::getNumber(attributeValue);
		else if(attributeName == "true") valueTrue = Math::getNumber(attributeValue);
		else if(attributeName == "div") div = Math::getNumber(attributeValue);
		else if(attributeName == "mul") mul = Math::getNumber(attributeValue);
		else if(attributeName == "offset") offset = Math::getDouble(attributeValue);
		else if(attributeName == "value") stringValue = attributeValue;
		else if(attributeName == "mantissa_start") mantissaStart = Math::getNumber(attributeValue);
		else if(attributeName == "mantissa_size") mantissaSize = Math::getNumber(attributeValue);
		else if(attributeName == "exponent_start") exponentStart = Math::getNumber(attributeValue);
		else if(attributeName == "exponent_size") exponentSize = Math::getNumber(attributeValue);
		else if(attributeName == "sim_counter") {}
		else if(attributeName == "counter_size") {}
		else if(attributeName == "on") on = Math::getNumber(attributeValue);
		else if(attributeName == "off") off = Math::getNumber(attributeValue);
		else if(attributeName == "invert") { if(attributeValue == "true") invert = true; }
		else _bl->out.printWarning("Warning: Unknown attribute for \"conversion\": " + attributeName);
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
				if(valueMapAttributeName == "device_value") deviceValue = Math::getNumber(valueMapAttributeValue);
				else if(valueMapAttributeName == "parameter_value") parameterValue = Math::getNumber(valueMapAttributeValue);
				else if(valueMapAttributeName == "from_device") { if(valueMapAttributeValue == "false") fromDevice = false; }
				else if(valueMapAttributeName == "to_device") { if(valueMapAttributeValue == "false") toDevice = false; }
				else if(valueMapAttributeName == "mask") {} //ignore, not needed
				else _bl->out.printWarning("Warning: Unknown attribute for \"value_map\": " + valueMapAttributeName);
			}
			integerValueMapDevice[deviceValue] = parameterValue;
			integerValueMapParameter[parameterValue] = deviceValue;
		}
		else _bl->out.printWarning( "Warning: Unknown subnode for \"conversion\": " + nodeName);
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
			_bl->out.printWarning("Warning: Boolean operator is none.");
			break;
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
	return false;
}

void Parameter::reverseData(const std::vector<uint8_t>& data, std::vector<uint8_t>& reversedData)
{
	try
	{
		reversedData.clear();
		int32_t size = std::lround(std::ceil(physicalParameter->size));
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

std::shared_ptr<Variable> Parameter::convertFromPacket(std::vector<uint8_t>& data, bool isEvent)
{
	try
	{
		std::vector<uint8_t> reversedData;
		std::vector<uint8_t>* value = nullptr;
		if(physicalParameter->endian == PhysicalParameter::Endian::Enum::little)
		{
			value = &reversedData;
			reverseData(data, *value);
		}
		else value = &data;
		if(logicalParameter->type == LogicalParameter::Type::Enum::typeEnum && conversion.empty())
		{
			int32_t integerValue = 0;
			_bl->hf.memcpyBigEndian(integerValue, *value);
			return std::shared_ptr<Variable>(new Variable(integerValue));
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeBoolean && conversion.empty())
		{
			int32_t integerValue = 0;
			_bl->hf.memcpyBigEndian(integerValue, *value);
			return std::shared_ptr<RPC::Variable>(new Variable((bool)integerValue));
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeString && conversion.empty())
		{
			if(!value->empty() && value->at(0) != 0)
			{
				int32_t size = value->back() == 0 ? value->size() - 1 : value->size();
				std::string string((char*)&value->at(0), size);
				return std::shared_ptr<RPC::Variable>(new Variable(string));
			}
			return std::shared_ptr<RPC::Variable>(new Variable(VariableType::rpcString));
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeAction)
		{
			if(isEvent) return std::shared_ptr<RPC::Variable>(new Variable(true));
			else return std::shared_ptr<RPC::Variable>(new Variable(false));
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
			if(physicalParameter->type == PhysicalParameter::Type::typeString)
			{
				variable.reset(new Variable(VariableType::rpcString));
				variable->stringValue.insert(variable->stringValue.end(), value->begin(), value->end());
			}
			else if(value->size() <= 4)
			{
				int32_t integerValue;
				_bl->hf.memcpyBigEndian(integerValue, *value);
				variable.reset(new Variable(integerValue));
				if(isSigned && !value->empty() && value->size() <= 4)
				{
					int32_t byteIndex = value->size() - std::lround(std::ceil(physicalParameter->size));
					if(byteIndex >= 0 && byteIndex < (signed)value->size())
					{
						int32_t bitSize = std::lround(physicalParameter->size * 10) % 10;
						int32_t signPosition = 0;
						if(bitSize == 0) signPosition = 7;
						else signPosition = bitSize - 1;
						if(value->at(byteIndex) & (1 << signPosition))
						{
							int32_t bits = (std::lround(std::floor(physicalParameter->size)) * 8) + bitSize;
							variable->integerValue -= (1 << bits);
						}
					}
				}
			}
			for(std::vector<std::shared_ptr<ParameterConversion>>::reverse_iterator i = conversion.rbegin(); i != conversion.rend(); ++i)
			{
				if((*i)->type == ParameterConversion::Type::Enum::rpcBinary)
				{
					if(!_binaryDecoder) _binaryDecoder = std::shared_ptr<BaseLib::RPC::RPCDecoder>(new BaseLib::RPC::RPCDecoder(_bl));
					variable = _binaryDecoder->decodeResponse(*value);
				}
				else
				{
					(*i)->fromPacket(variable);
				}
			}
			if(!variable)
			{
				_bl->out.printError("Error converting value: Variable " + id + " is nullptr.");
				variable.reset(new Variable(VariableType::rpcInteger));
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
    return std::shared_ptr<RPC::Variable>(new Variable(VariableType::rpcInteger));
}

void Parameter::convertToPacket(std::string value, std::vector<uint8_t>& convertedValue)
{
	try
	{
		std::shared_ptr<Variable> rpcValue;
		if(logicalParameter->type == LogicalParameter::Type::Enum::typeInteger) rpcValue.reset(new Variable(Math::getNumber(value)));
		if(logicalParameter->type == LogicalParameter::Type::Enum::typeEnum)
		{
			if(Math::isNumber(value)) rpcValue.reset(new Variable(Math::getNumber(value)));
			else //value is id of enum element
			{
				LogicalParameterEnum* parameter = (LogicalParameterEnum*)logicalParameter.get();
				for(std::vector<ParameterOption>::iterator i = parameter->options.begin(); i != parameter->options.end(); ++i)
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
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeBoolean || logicalParameter->type == LogicalParameter::Type::Enum::typeAction)
		{
			rpcValue.reset(new Variable(false));
			if(HelperFunctions::toLower(value) == "true") rpcValue->booleanValue = true;
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeFloat) rpcValue.reset(new Variable(Math::getDouble(value)));
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeString) rpcValue.reset(new Variable(value));
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
		std::shared_ptr<Variable> variable(new RPC::Variable());
		*variable = *value;
		if(logicalParameter->type == LogicalParameter::Type::Enum::typeEnum && conversion.empty())
		{
			LogicalParameterEnum* parameter = (LogicalParameterEnum*)logicalParameter.get();
			if(variable->integerValue > parameter->max) variable->integerValue = parameter->max;
			if(variable->integerValue < parameter->min) variable->integerValue = parameter->min;
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeAction && conversion.empty())
		{
			variable->integerValue = (int32_t)variable->booleanValue;
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeString && conversion.empty())
		{
			if(variable->stringValue.size() > 0)
			{
				convertedValue.insert(convertedValue.end(), variable->stringValue.begin(), variable->stringValue.end());
			}
			if((signed)convertedValue.size() < std::lround(physicalParameter->size)) convertedValue.push_back(0); //0 termination. Otherwise parts of old string will still be visible
		}
		else if(!conversion.empty() && conversion.at(0)->type == ParameterConversion::Type::cfm)
		{
			//Cannot currently easily be handled by ParameterConversion::toPacket
			convertedValue.resize(14, 0);
			if(variable->stringValue.empty() || variable->stringValue == "0") return;
			std::istringstream stringStream(variable->stringValue);
			std::string element;

			for(uint32_t i = 0; std::getline(stringStream, element, ',') && i < 13; i++)
			{
				if(i == 0)
				{
					convertedValue.at(0) = std::lround(200 * Math::getDouble(element));
				}
				else if(i == 1)
				{
					convertedValue.at(1) = Math::getNumber(element);
				}
				else if(i == 2)
				{
					variable->integerValue = std::lround(Math::getDouble(element) * 10);
					ParameterConversion conversion(_bl, this);
					conversion.type = ParameterConversion::Type::integerTinyFloat;
					conversion.toPacket(variable);
					std::vector<uint8_t> time;
					_bl->hf.memcpyBigEndian(time, variable->integerValue);
					if(time.size() == 1) convertedValue.at(13) = time.at(0);
					else
					{
						convertedValue.at(12) = time.at(0);
						convertedValue.at(13) = time.at(1);
					}
				}
				else convertedValue.at(i - 1) = Math::getNumber(element);
			}
			if(physicalParameter->endian == PhysicalParameter::Endian::Enum::little)
			{
				std::vector<uint8_t> reversedData;
				reverseData(convertedValue, reversedData);
				convertedValue = reversedData;
				return;
			}
			return;
		}
		else if(!conversion.empty() && conversion.at(0)->type == ParameterConversion::Type::ccrtdnParty)
		{
			//Cannot currently easily be handled by ParameterConversion::toPacket
			convertedValue.resize(8, 0);
			if(variable->stringValue.empty()) return;
			std::istringstream stringStream(variable->stringValue);
			std::string element;

			//"TEMP,START_TIME,DAY,MONTH,YEAR,END_TIME,DAY,MONTH,YEAR"
			for(uint32_t i = 0; std::getline(stringStream, element, ',') && i < 9; i++)
			{
				//Temperature
				if(i == 0) convertedValue.at(0) = std::lround(2 * Math::getDouble(element));
				//Start time
				else if(i == 1) convertedValue.at(1) = Math::getNumber(element) / 30;
				//Start day
				else if(i == 2) convertedValue.at(2) = Math::getNumber(element);
				//Start month
				else if(i == 3) convertedValue.at(7) = Math::getNumber(element) << 4;
				//Start year
				else if(i == 4) convertedValue.at(3) = Math::getNumber(element);
				//End time
				else if(i == 5) convertedValue.at(4) = Math::getNumber(element) / 30;
				//End day
				else if(i == 6) convertedValue.at(5) = Math::getNumber(element);
				//End month
				else if(i == 7) convertedValue.at(7) |= Math::getNumber(element);
				//End year
				else if(i == 8) convertedValue.at(6) = Math::getNumber(element);
			}
			return;
		}
		else
		{
			if(logicalParameter->type == LogicalParameter::Type::Enum::typeFloat)
			{
				LogicalParameterFloat* parameter = (LogicalParameterFloat*)logicalParameter.get();
				bool specialValue = (variable->floatValue == parameter->defaultValue);
				if(!specialValue)
				{
					for(std::unordered_map<std::string, double>::const_iterator i = parameter->specialValues.begin(); i != parameter->specialValues.end(); ++i)
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
					if(variable->floatValue > parameter->max) variable->floatValue = parameter->max;
					else if(variable->floatValue < parameter->min) variable->floatValue = parameter->min;
				}
			}
			else if(logicalParameter->type == LogicalParameter::Type::Enum::typeInteger)
			{
				LogicalParameterInteger* parameter = (LogicalParameterInteger*)logicalParameter.get();
				bool specialValue = (variable->integerValue == parameter->defaultValue);
				if(!specialValue)
				{
					for(std::unordered_map<std::string, int32_t>::const_iterator i = parameter->specialValues.begin(); i != parameter->specialValues.end(); ++i)
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
					if(variable->integerValue > parameter->max) variable->integerValue = parameter->max;
					else if(variable->integerValue < parameter->min) variable->integerValue = parameter->min;
				}
			}
			if(conversion.empty())
			{
				if(logicalParameter->type == LogicalParameter::Type::Enum::typeBoolean)
				{
					if(variable->stringValue.size() > 0 && variable->stringValue == "true") variable->integerValue = 1;
					else variable->integerValue = (int32_t)variable->booleanValue;
				}
			}
			else
			{
				for(std::vector<std::shared_ptr<ParameterConversion>>::iterator i = conversion.begin(); i != conversion.end(); ++i)
				{
					if((*i)->type == ParameterConversion::Type::Enum::rpcBinary)
					{
						if(!_binaryEncoder) _binaryEncoder = std::shared_ptr<BaseLib::RPC::RPCEncoder>(new BaseLib::RPC::RPCEncoder(_bl));
						_binaryEncoder->encodeResponse(variable, convertedValue); //No more conversions after this point.
						return;
					}
					else (*i)->toPacket(variable);
				}
			}
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
				variable->integerValue &= valueMask;
			}
			_bl->hf.memcpyBigEndian(convertedValue, variable->integerValue);
		}
		else
		{
			convertedValue.insert(convertedValue.end(), variable->stringValue.begin(), variable->stringValue.end());
		}

		if(physicalParameter->endian == PhysicalParameter::Endian::Enum::little)
		{
			std::vector<uint8_t> reversedData;
			reverseData(convertedValue, reversedData);
			convertedValue = reversedData;
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

Parameter::Parameter(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
	logicalParameter = std::shared_ptr<LogicalParameter>(new LogicalParameterInteger(baseLib));
	physicalParameter = std::shared_ptr<PhysicalParameter>(new PhysicalParameter());
}

Parameter::Parameter(BaseLib::Obj* baseLib, xml_node<>* node, bool checkForID) : Parameter(baseLib)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "index") index = std::stod(attributeValue);
		else if(attributeName == "size") size = std::stod(attributeValue);
		else if(attributeName == "index2") index2 = std::stod(attributeValue);
		else if(attributeName == "size2") size2 = std::stod(attributeValue);
		else if(attributeName == "index2_offset") index2Offset = Math::getNumber(attributeValue);
		else if(attributeName == "signed") { if(attributeValue == "true") isSigned = true; }
		else if(attributeName == "cond_op")
		{
			HelperFunctions::toLower(HelperFunctions::trim(attributeValue));
			if(attributeValue == "e" || attributeValue == "eq") booleanOperator = BooleanOperator::Enum::e;
			else if(attributeValue == "g") booleanOperator = BooleanOperator::Enum::g;
			else if(attributeValue == "l") booleanOperator = BooleanOperator::Enum::l;
			else if(attributeValue == "ge") booleanOperator = BooleanOperator::Enum::ge;
			else if(attributeValue == "le") booleanOperator = BooleanOperator::Enum::le;
			else _bl->out.printWarning("Warning: Unknown attribute value for \"cond_op\" in node \"parameter\": " + attributeValue);
		}
		else if(attributeName == "const_value") constValue = Math::getNumber(attributeValue);
		else if(attributeName == "id") id = attributeValue;
		else if(attributeName == "param") param = attributeValue;
		else if(attributeName == "PARAM") additionalParameter = attributeValue;
		else if(attributeName == "control") control = attributeValue;
		else if(attributeName == "loopback") { if(attributeValue == "true") loopback = true; }
		else if(attributeName == "hidden") { if(attributeValue == "true") hidden = true; } //Ignored actually
		else if(attributeName == "default") {} //Not necessary and not used
		else if(attributeName == "burst_suppression")
		{
			if(attributeValue != "0") _bl->out.printWarning("Warning: Unknown value for \"burst_suppression\" in node \"parameter\": " + attributeValue);
		}
		else if(attributeName == "type")
		{
			if(attributeValue == "integer") type = PhysicalParameter::Type::Enum::typeInteger;
			else if(attributeValue == "boolean") type = PhysicalParameter::Type::Enum::typeBoolean;
			else if(attributeValue == "string") type = PhysicalParameter::Type::Enum::typeString;
			else _bl->out.printWarning("Warning: Unknown attribute value for \"type\" in node \"parameter\": " + attributeValue);
		}
		else if(attributeName == "omit_if")
		{
			if(type != PhysicalParameter::Type::Enum::typeInteger)
			{
				_bl->out.printWarning("Warning: \"omit_if\" is only supported for type \"integer\" in node \"parameter\".");
				continue;
			}
			omitIfSet = true;
			omitIf = Math::getNumber(attributeValue);
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
				else if(element == "invisible") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::invisible);
				else _bl->out.printWarning("Warning: Unknown ui flag for \"parameter\": " + attributeValue);
			}
		}
		else if(attributeName == "mask") mask = Math::getNumber(attributeValue);
		else if(attributeName == "field") field = attributeValue;
		else if(attributeName == "subfield") subfield = attributeValue;
		else _bl->out.printWarning("Warning: Unknown attribute for \"parameter\": " + attributeName);
	}
	if(checkForID && id.empty()) _bl->out.printError("Error: Parameter has no id. Index: " + std::to_string(index));
	for(xml_node<>* parameterNode = node->first_node(); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		std::string nodeName(parameterNode->name());
		if(nodeName == "logical")
		{
			std::shared_ptr<LogicalParameter> parameter = LogicalParameter::fromXML(baseLib, parameterNode);
			if(parameter) logicalParameter = parameter;
		}
		else if(nodeName == "physical")
		{
			physicalParameter.reset(new PhysicalParameter(baseLib, parameterNode));
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
			std::shared_ptr<ParameterConversion> parameterConversion(new ParameterConversion(baseLib, this, parameterNode));
			if(parameterConversion && parameterConversion->type != ParameterConversion::Type::Enum::none) conversion.push_back(parameterConversion);
		}
		else if(nodeName == "description")
		{
			description = ParameterDescription(parameterNode);
		}
		else _bl->out.printWarning("Warning: Unknown subnode for \"parameter\": " + nodeName);
	}
	if(logicalParameter->type == LogicalParameter::Type::Enum::typeFloat)
	{
		LogicalParameterFloat* parameter = (LogicalParameterFloat*)logicalParameter.get();
		if(parameter->min < 0 && parameter->min != std::numeric_limits<double>::min()) isSigned = true;
	}
	else if(logicalParameter->type == LogicalParameter::Type::Enum::typeInteger)
	{
		LogicalParameterInteger* parameter = (LogicalParameterInteger*)logicalParameter.get();
		if(parameter->min < 0 && parameter->min != std::numeric_limits<int32_t>::min()) isSigned = true;
	}
}

void Parameter::adjustBitPosition(std::vector<uint8_t>& data)
{
	try
	{
		if(data.size() > 4 || data.empty() || logicalParameter->type == LogicalParameter::Type::Enum::typeString) return;
		int32_t value = 0;
		_bl->hf.memcpyBigEndian(value, data);
		if(physicalParameter->size < 0)
		{
			_bl->out.printError("Error: Negative size not allowed.");
			return;
		}
		double i = physicalParameter->index;
		i -= std::floor(i);
		double byteIndex = std::floor(i);
		if(byteIndex != i || physicalParameter->size < 0.8) //0.8 == 8 Bits
		{
			if(physicalParameter->size > 1)
			{
				_bl->out.printError("Error: Can't set partial byte index > 1.");
				return;
			}
			data.clear();
			data.push_back(value << (std::lround(i * 10) % 10));
		}
		//Adjust data size. See for example ENDTIME_SATURDAY_1 and TEMPERATURE_SATURDAY_1 of HM-CC-RT-DN
		if((int32_t)physicalParameter->size > (signed)data.size())
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

bool DeviceType::matches(Systems::LogicalDeviceType deviceType, uint32_t firmwareVersion)
{
	try
	{
		if((device && deviceType.family() != device->family)) return false;
		if(typeID != -1)
		{
			if((signed)deviceType.type() == typeID && (firmware == -1 || checkFirmwareVersion(firmwareVersion))) return true;
		}
		else
		{
			if(parameters.empty()) return false;
			bool match = true;
			for(std::vector<Parameter>::iterator i = parameters.begin(); i != parameters.end(); ++i)
			{
				//When the device type is not at index 10 of the pairing packet, the device is not supported
				//The "priority" attribute is ignored, for the standard devices "priority" seems not important
				if(i->index == 10.0) { if(i->constValue != (signed)deviceType.type()) match = false; }
				else if(i->index == 9.0) { if(!i->checkCondition(firmwareVersion)) match = false; }
				else if(i->index == 0) { if(((signed)deviceType.type() >> 8) != i->constValue) match = false; }
				else if(i->index == 1.0) { if(((signed)deviceType.type() & 0xFF) != i->constValue) match = false; }
				else if(i->index == 2.0) { if(!i->checkCondition(firmwareVersion)) match = false; }
				else match = false; //Unknown index
			}
			if(match) return true;
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
    return false;
}

bool DeviceType::matches(Systems::DeviceFamilies family, std::string typeID)
{
	try
	{
		if(device && family != device->family) return false;
		if(id == typeID) return true;
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
    return false;
}

bool DeviceType::matches(Systems::DeviceFamilies family, std::shared_ptr<Systems::Packet> packet)
{
	try
	{
		if(parameters.empty() || (device && family != device->family)) return false;
		for(std::vector<Parameter>::iterator i = parameters.begin(); i != parameters.end(); ++i)
		{
			int32_t intValue = 0;
			std::vector<uint8_t> data = packet->getPosition(i->index, i->size, -1);
			_bl->hf.memcpyBigEndian(intValue, data);
			if(!i->checkCondition(intValue)) return false;
		}
		return true;
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
    return false;
}

bool DeviceType::checkFirmwareVersion(int32_t version)
{
	try
	{
		switch(booleanOperator)
		{
		case BooleanOperator::Enum::e:
			return version == firmware;
			break;
		case BooleanOperator::Enum::g:
			return version > firmware;
			break;
		case BooleanOperator::Enum::l:
			return version < firmware;
			break;
		case BooleanOperator::Enum::ge:
			return version >= firmware;
			break;
		case BooleanOperator::Enum::le:
			return version <= firmware;
			break;
		default:
			_bl->out.printWarning("Warning: Boolean operator is none.");
			break;
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
	return false;
}

DeviceType::DeviceType(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

DeviceType::DeviceType(BaseLib::Obj* baseLib, xml_node<>* typeNode) : DeviceType(baseLib)
{
	for(xml_attribute<>* attr = typeNode->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "name") name = attributeValue;
		else if(attributeName == "id") id = attributeValue;
		else if(attributeName == "priority") priority = Math::getNumber(attributeValue);
		else if(attributeName == "updatable") { if(attributeValue == "true") updatable = true; }
		else _bl->out.printWarning("Warning: Unknown attribute for \"type\": " + attributeName);
	}
	for(xml_node<>* parameterNode = typeNode->first_node("parameter"); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		Parameter parameter(baseLib, parameterNode);
		parameters.push_back(parameter);
	}
	xml_node<>* typeIDNode = typeNode->first_node("type_id");
	if(typeIDNode)
	{
		std::string value(typeIDNode->value());
		if(!value.empty()) typeID = Math::getNumber(value);
	}
	xml_node<>* firmwareNode = typeNode->first_node("firmware");
	if(firmwareNode)
	{
		for(xml_attribute<>* attr = firmwareNode->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "cond_op")
			{
				HelperFunctions::toLower(HelperFunctions::trim(attributeValue));
				if(attributeValue == "e" || attributeValue == "eq") booleanOperator = BooleanOperator::Enum::e;
				else if(attributeValue == "g") booleanOperator = BooleanOperator::Enum::g;
				else if(attributeValue == "l") booleanOperator = BooleanOperator::Enum::l;
				else if(attributeValue == "ge") booleanOperator = BooleanOperator::Enum::ge;
				else if(attributeValue == "le") booleanOperator = BooleanOperator::Enum::le;
				else _bl->out.printWarning("Warning: Unknown attribute value for \"cond_op\" in node \"parameter\": " + attributeValue);
			}
			else _bl->out.printWarning("Warning: Unknown attribute for \"firmware\": " + attributeName);
		}
		std::string value(firmwareNode->value());
		if(!value.empty()) firmware = Math::getNumber(value);
	}
}

ParameterSet::ParameterSet(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

ParameterSet::ParameterSet(BaseLib::Obj* baseLib, xml_node<>* parameterSetNode) : ParameterSet(baseLib)
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
	return filteredParameters;
}

std::vector<std::shared_ptr<Parameter>> ParameterSet::getList(int32_t list)
{
	std::vector<std::shared_ptr<Parameter>> filteredParameters;
	try
	{
		if(list < 0) return filteredParameters;
		for(std::vector<std::shared_ptr<Parameter>>::iterator i = parameters.begin(); i != parameters.end(); ++i)
		{
			if((*i)->physicalParameter->list == (unsigned)list) filteredParameters.push_back(*i);
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
			if(type == Type::Enum::none) _bl->out.printWarning("Warning: Unknown parameter set type: " + attributeValue);
		}
		else if(attributeName == "address_start") addressStart = Math::getNumber(attributeValue);
		else if(attributeName == "address_step") addressStep = Math::getNumber(attributeValue);
		else if(attributeName == "count") count = Math::getNumber(attributeValue);
		else if(attributeName == "channel_offset") channelOffset = Math::getNumber(attributeValue);
		else if(attributeName == "peer_address_offset") peerAddressOffset = Math::getNumber(attributeValue);
		else if(attributeName == "peer_channel_offset") peerChannelOffset = Math::getNumber(attributeValue);
		else if(attributeName == "link") {} //Ignored
		else _bl->out.printWarning("Warning: Unknown attribute for \"paramset\": " + attributeName);
	}
	std::vector<std::pair<std::string, std::string>> enforce;
	for(xml_node<>* parameterNode = parameterSetNode->first_node(); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		std::string nodeName(parameterNode->name());
		if(nodeName == "parameter")
		{
			std::shared_ptr<Parameter> parameter(new Parameter(_bl, parameterNode, true));
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
				_bl->out.printWarning("Warning: Could not parse \"enforce\". Attribute id or value not set.");
				continue;
			}
			enforce.push_back(std::pair<std::string, std::string>(std::string(attr1->value()), std::string(attr2->value())));
		}
		else if(nodeName == "subset")
		{
			xml_attribute<>* attr = parameterNode->first_attribute("ref");
			if(!attr)
			{
				_bl->out.printWarning("Warning: Could not parse \"subset\". Attribute ref not set.");
				continue;
			}
			subsetReference = std::string(attr->value());
		}
		else if(nodeName == "default_values")
		{
			xml_attribute<>* attr = parameterNode->first_attribute("function");
			if(!attr)
			{
				_bl->out.printWarning("Warning: Could not parse \"subset\". Attribute function not set.");
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
						_bl->out.printWarning("Warning: Could not parse \"value\" (in default_values). Attribute id or value not set.");
						continue;
					}
					defaultValues[function].push_back(std::pair<std::string, std::string>(std::string(attr1->value()), std::string(attr2->value())));
				}
				else _bl->out.printWarning("Warning: Unknown node name for \"default_values\": " + nodeName);
			}
		}
		else _bl->out.printWarning("Warning: Unknown node name for \"paramset\": " + nodeName);
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
			logicalParameter->enforceValue = Math::getNumber(i->second);
		}
		else if(parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeBoolean)
		{
			LogicalParameterBoolean* logicalParameter = (LogicalParameterBoolean*)parameter->logicalParameter.get();
			if(HelperFunctions::toLower(i->second) == "true") logicalParameter->enforceValue = true;
		}
		else if(parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeFloat)
		{
			LogicalParameterFloat* logicalParameter = (LogicalParameterFloat*)parameter->logicalParameter.get();
			logicalParameter->enforceValue = Math::getDouble(i->second);
		}
		else if(parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeAction)
		{
			LogicalParameterAction* logicalParameter = (LogicalParameterAction*)parameter->logicalParameter.get();
			if(HelperFunctions::toLower(i->second) == "true") logicalParameter->enforceValue = true;
		}
		else if(parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeEnum)
		{
			LogicalParameterEnum* logicalParameter = (LogicalParameterEnum*)parameter->logicalParameter.get();
			logicalParameter->enforceValue = Math::getNumber(i->second);
		}
		else if(parameter->logicalParameter->type == LogicalParameter::Type::Enum::typeString)
		{
			LogicalParameterString* logicalParameter = (LogicalParameterString*)parameter->logicalParameter.get();
			logicalParameter->enforceValue = i->second;
		}
	}
}

LinkRole::LinkRole(BaseLib::Obj* baseLib, xml_node<>* node)
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
		else baseLib->out.printWarning("Warning: Unknown node name for \"link_roles\": " + nodeName);
	}
}

EnforceLink::EnforceLink(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

EnforceLink::EnforceLink(BaseLib::Obj* baseLib, xml_node<>* node) : EnforceLink(baseLib)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id") id = attributeValue;
		else if(attributeName == "value") value = attributeValue;
		else baseLib->out.printWarning("Warning: Unknown attribute for \"enforce_link - value\": " + attributeName);
	}
}

std::shared_ptr<Variable> EnforceLink::getValue(LogicalParameter::Type::Enum type)
{
	try
	{
		VariableType rpcType = VariableType::rpcVoid;
		switch(type)
		{
		case LogicalParameter::Type::Enum::typeEnum:
			rpcType = VariableType::rpcInteger;
			break;
		case LogicalParameter::Type::Enum::typeAction:
			rpcType = VariableType::rpcBoolean;
			break;
		case LogicalParameter::Type::Enum::typeBoolean:
			rpcType = VariableType::rpcBoolean;
			break;
		case LogicalParameter::Type::Enum::typeFloat:
			rpcType = VariableType::rpcFloat;
			break;
		case LogicalParameter::Type::Enum::typeInteger:
			rpcType = VariableType::rpcInteger;
			break;
		case LogicalParameter::Type::Enum::typeString:
			rpcType = VariableType::rpcString;
			break;
		case LogicalParameter::Type::Enum::none:
			rpcType = VariableType::rpcString;
			break;
		}
		return Variable::fromString(value, rpcType);
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
    return std::shared_ptr<Variable>();
}

DeviceChannel::DeviceChannel(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

DeviceChannel::DeviceChannel(BaseLib::Obj* baseLib, xml_node<>* node, uint32_t& index) : DeviceChannel(baseLib)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "index")
		{
			startIndex = Math::getNumber(attributeValue);
			index = startIndex;
		}
		else if(attributeName == "physical_index_offset") physicalIndexOffset = Math::getNumber(attributeValue);
		else if(attributeName == "ui_flags")
		{
			if(attributeValue == "visible") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::visible);
			else if(attributeValue == "internal") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::internal);
			else if(attributeValue == "dontdelete") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::dontdelete);
			else _bl->out.printWarning("Warning: Unknown ui flag for \"channel\": " + attributeValue);
		}
		else if(attributeName == "direction")
		{
			if(attributeValue == "sender") direction = (Direction::Enum)(direction | Direction::Enum::sender);
			else if(attributeValue == "receiver") direction = (Direction::Enum)(direction | Direction::Enum::receiver);
			else _bl->out.printWarning("Warning: Unknown direction for \"channel\": " + attributeValue);
		}
		else if(attributeName == "class") channelClass = attributeValue;
		else if(attributeName == "type") type = attributeValue;
		else if(attributeName == "hidden") { if(attributeValue == "true") hidden = true; }
		else if(attributeName == "autoregister") { if(attributeValue == "true") autoregister = true; }
		else if(attributeName == "count") count = Math::getNumber(attributeValue);
		else if(attributeName == "has_team") { if(attributeValue == "true") hasTeam = true; }
		else if(attributeName == "aes_default") { if(attributeValue == "true") aesDefault = true; }
		else if(attributeName == "aes_always") { if(attributeValue == "true") { aesDefault = true; aesAlways = true; } }
		else if(attributeName == "aes_cbc") { if(attributeValue == "true") aesCBC = true; }
		else if(attributeName == "team_tag") teamTag = attributeValue;
		else if(attributeName == "paired") { if(attributeValue == "true") paired = true; }
		else if(attributeName == "function") function = attributeValue;
		else if(attributeName == "pair_function")
		{
			if(attributeValue.size() != 2)
			{
				_bl->out.printWarning("Warning: pair_function does not consist of two functions.");
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
				countFromSysinfo = Math::getDouble(splitValue.first);
				if(countFromSysinfo < 9)
				{
					_bl->out.printError("Error: count_from_sysinfo has to be >= 9.");
					countFromSysinfo = -1;
				}
			}
			if(!splitValue.second.empty())
			{
				countFromSysinfoSize = Math::getDouble(splitValue.second);
				if(countFromSysinfoSize > 1)
				{
					_bl->out.printError("Error: The size of count_from_sysinfo has to be <= 1.");
					countFromSysinfoSize = 1;
				}
			}
		}
		else _bl->out.printWarning("Warning: Unknown attribute for \"channel\": " + attributeName);
	}
	for(xml_node<>* channelNode = node->first_node(); channelNode; channelNode = channelNode->next_sibling())
	{
		std::string nodeName(channelNode->name());
		if(nodeName == "parameters" || nodeName == "paramset")
		{
			std::shared_ptr<ParameterSet> parameterSet(new ParameterSet(baseLib, channelNode));
			if(parameterSets.find(parameterSet->type) == parameterSets.end()) parameterSets[parameterSet->type] = parameterSet;
			else _bl->out.printError("Error: Tried to add same parameter set type twice.");
			//Set physical settings of special_parameter
			if(specialParameter && parameterSet->type == ParameterSet::Type::master)
			{
				std::shared_ptr<Parameter> parameter = parameterSet->getParameter(specialParameter->id);
				if(parameter)
				{
					std::string valueID = parameter->physicalParameter->valueID;
					parameter->physicalParameter = specialParameter->physicalParameter;
					parameter->physicalParameter->valueID = valueID;
				}
			}
		}
		else if(nodeName == "link_roles")
		{
			if(linkRoles) _bl->out.printWarning("Warning: Multiple link roles are defined for channel " + std::to_string(index) + ".");
			linkRoles.reset(new LinkRole(baseLib, channelNode));
		}
		else if(nodeName == "enforce_link")
		{
			for(xml_node<>* enforceLinkNode = channelNode->first_node("value"); enforceLinkNode; enforceLinkNode = enforceLinkNode->next_sibling("value"))
			{
				enforceLinks.push_back(std::shared_ptr<EnforceLink>(new EnforceLink(baseLib, enforceLinkNode)));
			}
		}
		else if(nodeName == "special_parameter") specialParameter.reset(new Parameter(baseLib, channelNode, true));
		else if(nodeName == "subconfig")
		{
			uint32_t index = 0;
			subconfig.reset(new DeviceChannel(baseLib, channelNode, index));
			//Set physical settings of special_parameter
			if(specialParameter && subconfig->parameterSets.find(ParameterSet::Type::Enum::master) != subconfig->parameterSets.end())
			{
				std::shared_ptr<Parameter> parameter = subconfig->parameterSets.at(ParameterSet::Type::Enum::master)->getParameter(specialParameter->id);
				if(parameter)
				{
					std::string valueID = parameter->physicalParameter->valueID;
					parameter->physicalParameter = specialParameter->physicalParameter;
					parameter->physicalParameter->valueID = valueID;
				}
			}
		}
		else _bl->out.printWarning("Warning: Unknown node name for \"device\": " + nodeName);
	}
	if(parameterSets.find(ParameterSet::Type::Enum::master) == parameterSets.end() || !parameterSets[ParameterSet::Type::Enum::master])
	{
		//Create master parameter set if not existant
		std::shared_ptr<ParameterSet> masterSet(new ParameterSet(_bl));
		masterSet->type = ParameterSet::Type::Enum::master;
		parameterSets[ParameterSet::Type::Enum::master] = masterSet;
	}
}

Device::Device(BaseLib::Obj* baseLib, Systems::DeviceFamilies deviceFamily)
{
	_bl = baseLib;
	family = deviceFamily;
	parameterSet.reset(new ParameterSet(baseLib));
}

Device::Device(BaseLib::Obj* baseLib, Systems::DeviceFamilies deviceFamily, std::string xmlFilename) : Device(baseLib, deviceFamily)
{
	try
	{
		load(xmlFilename);

		if(!_loaded || channels.empty()) return; //Happens when file couldn't be read

		std::shared_ptr<Parameter> parameter(new Parameter(baseLib));
		parameter->id = "PAIRED_TO_CENTRAL";
		parameter->uiFlags = Parameter::UIFlags::Enum::invisible;
		parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeBoolean;
		parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
		parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeBoolean;
		parameter->physicalParameter->valueID = "PAIRED_TO_CENTRAL";
		parameter->physicalParameter->list = 0;
		parameter->physicalParameter->index = 2;
		channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);

		parameter.reset(new Parameter(baseLib));
		parameter->id = "CENTRAL_ADDRESS_BYTE_1";
		parameter->uiFlags = Parameter::UIFlags::Enum::invisible;
		parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
		parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->valueID = "CENTRAL_ADDRESS_BYTE_1";
		parameter->physicalParameter->list = 0;
		parameter->physicalParameter->index = 10;
		channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);

		parameter.reset(new Parameter(baseLib));
		parameter->id = "CENTRAL_ADDRESS_BYTE_2";
		parameter->uiFlags = Parameter::UIFlags::Enum::invisible;
		parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
		parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->valueID = "CENTRAL_ADDRESS_BYTE_2";
		parameter->physicalParameter->list = 0;
		parameter->physicalParameter->index = 11;
		channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);

		parameter.reset(new Parameter(baseLib));
		parameter->id = "CENTRAL_ADDRESS_BYTE_3";
		parameter->uiFlags = Parameter::UIFlags::Enum::invisible;
		parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
		parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeInteger;
		parameter->physicalParameter->valueID = "CENTRAL_ADDRESS_BYTE_3";
		parameter->physicalParameter->list = 0;
		parameter->physicalParameter->index = 12;
		channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);

		if(!supportsAES) return;
		//For HomeMatic BidCoS: Set AES default value to "false", so a new AES key is set on pairing
		for(std::map<uint32_t, std::shared_ptr<DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
		{
			if(!i->second || i->second->parameterSets.find(ParameterSet::Type::Enum::master) == i->second->parameterSets.end() || !i->second->parameterSets.at(ParameterSet::Type::Enum::master) || i->first == 0) continue;
			parameter = i->second->parameterSets[ParameterSet::Type::Enum::master]->getParameter("AES_ACTIVE");
			if(!parameter)
			{
				parameter.reset(new Parameter(baseLib));
				i->second->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);
			}
			parameter->id = "AES_ACTIVE";
			parameter->uiFlags = Parameter::UIFlags::Enum::internal;
			parameter->conversion.clear();
			parameter->conversion.push_back(std::shared_ptr<ParameterConversion>(new ParameterConversion(baseLib, parameter.get())));
			parameter->conversion.back()->type = ParameterConversion::Type::Enum::booleanInteger;
			std::shared_ptr<LogicalParameterBoolean> logicalParameter(new LogicalParameterBoolean(baseLib));
			logicalParameter->defaultValueExists = true;
			logicalParameter->defaultValue = false;
			parameter->logicalParameter = logicalParameter;
			parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::config;
			parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeInteger;
			parameter->physicalParameter->valueID = "AES_ACTIVE";
			parameter->physicalParameter->list = 1;
			parameter->physicalParameter->index = 8;
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
			if(!doc.first_node("device"))
			{
				_bl->out.printError("Error: Device XML file \"" + xmlFilename + "\" does not start with \"device\".");
				return;
			}
			parseXML(doc.first_node("device"));
		}
		else _bl->out.printError("Error reading file " + xmlFilename + ": " + strerror(errno));
		_loaded = true;
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
			if(attributeName == "version") version = Math::getNumber(attributeValue);
			else if(attributeName == "rx_modes")
			{
				std::stringstream stream(attributeValue);
				std::string element;
				rxModes = RXModes::Enum::none;
				while(std::getline(stream, element, ','))
				{
					HelperFunctions::toLower(HelperFunctions::trim(element));
					if(element == "wakeup") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::wakeUp);
					else if(element == "wakeup2") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::wakeUp2);
					else if(element == "config") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::config);
					else if(element == "burst") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::burst);
					else if(element == "always") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::always);
					else if(element == "lazy_config") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::lazyConfig);
					else _bl->out.printWarning("Warning: Unknown rx mode for \"device\": " + element);
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
				eepSize = Math::getNumber(attributeValue);
			}
			else if(attributeName == "rx_default") {} //not needed
			else if(attributeName == "ui_flags")
			{
				if(attributeValue == "visible") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::visible);
				else if(attributeValue == "internal") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::internal);
				else if(attributeValue == "dontdelete") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::dontdelete);
				else _bl->out.printWarning("Warning: Unknown ui flag for \"channel\": " + attributeValue);
			}
			else if(attributeName == "cyclic_timeout") cyclicTimeout = Math::getNumber(attributeValue);
			else if(attributeName == "supports_aes") { if(attributeValue == "true") supportsAES = true; }
			else if(attributeName == "peering_sysinfo_expect_channel") { if(attributeValue == "false") peeringSysinfoExpectChannel = false; }
			else if(attributeName == "needs_time") { if(attributeValue == "true") needsTime = true; }
			else _bl->out.printWarning("Warning: Unknown attribute for \"device\": " + attributeName);
		}

		std::map<std::string, std::shared_ptr<ParameterSet>> parameterSetDefinitions;
		for(node = node->first_node(); node; node = node->next_sibling())
		{
			std::string nodeName(node->name());
			if(nodeName == "types" || nodeName == "supported_types")
			{
				for(xml_node<>* typeNode = node->first_node("type"); typeNode; typeNode = typeNode->next_sibling())
				{
					std::shared_ptr<DeviceType> deviceType(new DeviceType(_bl, typeNode));
					deviceType->device = this;
					supportedTypes.push_back(deviceType);
				}
			}
			else if(nodeName == "paramset" || nodeName == "parameters")
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
						else _bl->out.printError("Error: Tried to add parameter set of type \"" + attributeValue + "\" to device. That is not allowed.");
					}
					else _bl->out.printWarning("Warning: Unknown attribute for \"paramset\": " + attributeName);
				}
				parameterSet->init(node);
			}
			else if(nodeName == "paramset_defs")
			{
				for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
				{
					std::string attributeName(attr->name());
					_bl->out.printWarning("Warning: Unknown attribute for \"paramset_defs\": " + attributeName);
				}

				for(xml_node<>* paramsetNode = node->first_node(); paramsetNode; paramsetNode = paramsetNode->next_sibling())
				{
					std::string nodeName(paramsetNode->name());
					if(nodeName == "paramset" || nodeName == "parameters")
					{
						std::shared_ptr<ParameterSet> parameterSetDefinition(new ParameterSet(_bl, paramsetNode));
						parameterSetDefinitions[parameterSetDefinition->id] = parameterSetDefinition;
					}
					else _bl->out.printWarning("Warning: Unknown node name for \"paramset_defs\": " + nodeName);
				}
			}
			else if(nodeName == "channels")
			{
				for(xml_node<>* channelNode = node->first_node("channel"); channelNode; channelNode = channelNode->next_sibling())
				{
					uint32_t index = 0;
					std::shared_ptr<DeviceChannel> channel(new DeviceChannel(_bl, channelNode, index));
					channel->parentDevice = this;
					for(uint32_t i = index; i < index + channel->count; i++)
					{
						if(channels.find(i) == channels.end()) channels[i] = channel;
						else _bl->out.printError("Error: Tried to add channel with the same index twice. Index: " + std::to_string(i));
					}
					if(channel->countFromSysinfo)
					{
						if(countFromSysinfoIndex > -1) _bl->out.printError("Error: count_from_sysinfo is defined for two channels. That is not allowed.");
						if(std::floor(channel->countFromSysinfo) != channel->countFromSysinfo)
						{
							_bl->out.printError("Error: count_from_sysinfo has to start with index 0 of a byte.");
							continue;
						}
						countFromSysinfoIndex = (int32_t)channel->countFromSysinfo;
						countFromSysinfoSize = channel->countFromSysinfoSize;
						if(countFromSysinfoIndex < -1) countFromSysinfoIndex = -1;
						if(countFromSysinfoSize <= 0) countFromSysinfoSize = 1;
					}
				}
			}
			else if(nodeName == "packets" || nodeName == "frames")
			{
				for(xml_node<>* frameNode = node->first_node("frame"); frameNode; frameNode = frameNode->next_sibling("frame"))
				{
					std::shared_ptr<DeviceFrame> frame(new DeviceFrame(_bl, frameNode));
					framesByMessageType.insert(std::pair<uint32_t, std::shared_ptr<DeviceFrame>>(frame->type, frame));
					framesByID[frame->id] = frame;
				}
				for(xml_node<>* frameNode = node->first_node("packet"); frameNode; frameNode = frameNode->next_sibling("packet"))
				{
					std::shared_ptr<DeviceFrame> frame(new DeviceFrame(_bl, frameNode));
					framesByMessageType.insert(std::pair<uint32_t, std::shared_ptr<DeviceFrame>>(frame->type, frame));
					framesByID[frame->id] = frame;
				}
			}
			else if(nodeName == "run_program")
			{
				runProgram.reset(new DeviceProgram(_bl, node));
			}
			else if(nodeName == "team")
			{
				team.reset(new Device(_bl, family));
				team->parseXML(node);
			}
			else _bl->out.printWarning("Warning: Unknown node name for \"device\": " + nodeName);
		}

		if(!parameterSetDefinitions.empty())
		{
			std::map<uint32_t, std::map<ParameterSet::Type::Enum, std::shared_ptr<ParameterSet>>> parameterSetsToAdd;
			for(std::map<uint32_t, std::shared_ptr<DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
			{
				for(std::map<ParameterSet::Type::Enum, std::shared_ptr<ParameterSet>>::iterator j = i->second->parameterSets.begin(); j != i->second->parameterSets.end(); ++j)
				{
					if(j->second->subsetReference.empty() || parameterSetDefinitions.find(j->second->subsetReference) == parameterSetDefinitions.end()) continue;
					std::shared_ptr<ParameterSet> parameterSet(new ParameterSet(_bl));
					*parameterSet = *parameterSetDefinitions.at(j->second->subsetReference);
					parameterSet->type = j->second->type;
					parameterSet->id = j->second->id;
					for(std::vector<std::shared_ptr<Parameter>>::iterator k = j->second->parameters.begin(); k != j->second->parameters.end(); ++k)
					{
						if(!*k) continue;
						parameterSet->parameters.push_back(*k);
					}
					//We can't change the map while we are iterating through it, so we temporarily store the parameterSet
					parameterSetsToAdd[i->first][j->first] = parameterSet;
					for(std::vector<std::shared_ptr<Parameter>>::iterator k = parameterSet->parameters.begin(); k != parameterSet->parameters.end(); ++k)
					{
						(*k)->parentParameterSet = parameterSet.get();
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

		if(!channels[0]) channels[0] = std::shared_ptr<DeviceChannel>(new DeviceChannel(_bl));
		if(!channels[0]->parameterSets[ParameterSet::Type::Enum::master]) channels[0]->parameterSets[ParameterSet::Type::Enum::master] = std::shared_ptr<ParameterSet>(new ParameterSet(_bl));
		if(parameterSet->type == ParameterSet::Type::Enum::master && !parameterSet->parameters.empty())
		{
			if(channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.size() > 0)
			{
				_bl->out.printError("Error: Master parameter set of channnel 0 has to be empty.");
			}
			channels[0]->parameterSets[ParameterSet::Type::Enum::master] = parameterSet;
		}

		for(std::map<uint32_t, std::shared_ptr<DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
		{
			if(!i->second || i->second->parameterSets.find(ParameterSet::Type::Enum::values) == i->second->parameterSets.end()) continue;
			for(std::vector<std::shared_ptr<Parameter>>::iterator j = i->second->parameterSets.at(ParameterSet::Type::Enum::values)->parameters.begin(); j != i->second->parameterSets.at(ParameterSet::Type::Enum::values)->parameters.end(); ++j)
			{
				if(!*j) continue;
				if(!(*j)->physicalParameter->getRequest.empty() && framesByID.find((*j)->physicalParameter->getRequest) != framesByID.end())
				{
					framesByID[(*j)->physicalParameter->getRequest]->associatedValues.push_back(*j);
					valueRequestFrames[i->first][(*j)->physicalParameter->getRequest] = framesByID[(*j)->physicalParameter->getRequest];
				}
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

int32_t Device::getCountFromSysinfo(std::shared_ptr<Systems::Packet> packet)
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
			if(!i->second) continue;
			if(i->second->countFromSysinfo > -1)
			{
				index = i->first;
				channel = i->second;
				break;
			}
		}
		std::shared_ptr<ParameterSet> valueSet = (channel->parameterSets.find(ParameterSet::Type::Enum::values) == channel->parameterSets.end()) ? nullptr : channel->parameterSets.at(ParameterSet::Type::Enum::values);

		for(uint32_t i = index + 1; i < index + countFromSysinfo; i++)
		{
			if(channels.find(i) == channels.end())
			{
				channels[i] = channel;
				if(!valueSet) continue;
				for(std::vector<std::shared_ptr<Parameter>>::iterator j = valueSet->parameters.begin(); j != valueSet->parameters.end(); ++j)
				{
					if(!*j) continue;
					if(!(*j)->physicalParameter->getRequest.empty() && framesByID.find((*j)->physicalParameter->getRequest) != framesByID.end())
					{
						valueRequestFrames[i][(*j)->physicalParameter->getRequest] = framesByID[(*j)->physicalParameter->getRequest];
					}
				}
			}
			else _bl->out.printError("Error: Tried to add channel with the same index twice. Index: " + std::to_string(i));
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

std::shared_ptr<DeviceType> Device::getType(Systems::LogicalDeviceType deviceType, int32_t firmwareVersion)
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
	return std::shared_ptr<DeviceType>();
}

}
}
