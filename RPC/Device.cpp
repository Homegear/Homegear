#include "Device.h"
#include "../HelperFunctions.h"
#include "../GD.h"

namespace RPC {

DescriptionField::DescriptionField(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id") id = attributeValue;
		else if(attributeName == "value") value = attributeValue;
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"field\": " << attributeName << std::endl;
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
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown subnode for \"description\": " << nodeName << std::endl;
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
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown direction for \"frame\": " << attributeValue << std::endl;
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
		else if(attributeName == "type") type = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "subtype") subtype = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "subtype_index") subtypeIndex = std::stoll(attributeValue);
		else if(attributeName == "channel_field")
		{
			std::pair<std::string, std::string> splitString = HelperFunctions::split(attributeValue, ':');
			channelField = HelperFunctions::getNumber(splitString.first);
			//Currently I'm ignoring the size
		}
		else if(attributeName == "fixed_channel") fixedChannel = HelperFunctions::getNumber(attributeValue);
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"frame\": " << attributeName << std::endl;
	}
	for(xml_node<>* frameNode = node->first_node("parameter"); frameNode; frameNode = frameNode->next_sibling("parameter"))
	{
		parameters.push_back(frameNode);
	}
}

void ParameterConversion::fromPacket(std::shared_ptr<RPC::RPCVariable> value)
{
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
		if(integerValueMapDevice.find(value->integerValue) != integerValueMapDevice.end()) value->integerValue = integerValueMapDevice[value->integerValue];
	}
	else if(type == Type::Enum::booleanInteger)
	{
		value->type = RPCVariableType::rpcBoolean;
		if(value->integerValue == valueFalse) value->booleanValue = false;
		if(value->integerValue == valueTrue || value->integerValue > threshold) value->booleanValue = true;;
	}
	else if(type == Type::Enum::floatConfigTime)
	{
		value->type = RPCVariableType::rpcFloat;
		if(value < 0)
		{
			value->floatValue = 0;
			return;
		}
		uint32_t bits = (uint32_t)std::floor(valueSize) * 8;
		bits += std::lround(valueSize * 10) % 10;
		if(bits == 0) bits = 7;
		uint32_t maxNumber = 1 << bits;
		if((unsigned)value->integerValue < maxNumber) value->floatValue = value->integerValue * factor;
		else value->floatValue = (value->integerValue - maxNumber) * factor2;
	}
	else if(type == Type::Enum::integerTinyFloat)
	{
		value->type = RPCVariableType::rpcInteger;
		int32_t mantissa = (value->integerValue >> mantissaStart) & ((1 << mantissaSize) - 1);
		if(mantissaSize == 0) mantissa = 1;
		int32_t exponent = (value->integerValue >> exponentStart) & ((1 << exponentSize) - 1);
		value->integerValue = mantissa * (1 << exponent);
	}
}

void ParameterConversion::toPacket(std::shared_ptr<RPC::RPCVariable> value)
{
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
		if(integerValueMapParameter.find(value->integerValue) != integerValueMapParameter.end()) value->integerValue = integerValueMapParameter[value->integerValue];
	}
	else if(type == Type::Enum::booleanInteger)
	{
		if(valueTrue == 0 && valueFalse == 0) value->integerValue = (int32_t)value->booleanValue;
		else if(value->booleanValue) value->integerValue = valueTrue;
		else value->integerValue = valueFalse;
	}
	else if(type == Type::Enum::floatConfigTime)
	{
		uint32_t bits = (uint32_t)std::floor(valueSize) * 8;
		bits += std::lround(valueSize * 10) % 10;
		if(bits == 0) bits = 7;
		uint32_t maxNumber = 1 << bits;
		if(value->floatValue <= maxNumber - 1) value->integerValue = std::roundl(value->floatValue / factor);
		else value->integerValue = maxNumber + std::roundl(value->floatValue / factor2);
	}
	else if(type == Type::Enum::integerTinyFloat)
	{
		int64_t maxMantissa = ((1 << mantissaSize) - 1);
		int64_t maxExponent = ((1 << exponentSize) - 1);
		int64_t mantissa = value->integerValue;
		int64_t exponent = 0;
		if(maxMantissa > 0)
		{
			while(value->integerValue >= maxMantissa)
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
	value->type = RPCVariableType::rpcInteger;
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
			else if(attributeValue == "float_configtime") type = Type::Enum::booleanInteger;
			else if(attributeValue == "option_integer") type = Type::Enum::optionInteger;
			else if(attributeValue == "integer_tinyfloat") type = Type::Enum::integerTinyFloat;
			else if(attributeValue == "toggle") type = Type::Enum::toggle;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown type for \"conversion\": " << attributeValue << std::endl;
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
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"conversion\": " << attributeName << std::endl;
	}
	for(xml_node<>* conversionNode = node->first_node(); conversionNode; conversionNode = conversionNode->next_sibling())
	{
		std::string nodeName(conversionNode->name());
		if(nodeName == "value_map" && (type == Type::Enum::integerIntegerMap || type == Type::Enum::optionInteger))
		{
			xml_attribute<>* attr1;
			xml_attribute<>* attr2;
			attr1 = node->first_attribute("device_value");
			attr2 = node->first_attribute("parameter_value");
			if(attr1 != nullptr && attr2 != nullptr)
			{
				std::string attribute1(attr1->value());
				std::string attribute2(attr2->value());
				int32_t deviceValue = HelperFunctions::getNumber(attribute1);
				int32_t parameterValue = HelperFunctions::getNumber(attribute2);
				integerValueMapDevice[deviceValue] = parameterValue;
				integerValueMapParameter[parameterValue] = deviceValue;
			}
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown subnode for \"conversion\": " << nodeName << std::endl;
	}
}

bool Parameter::checkCondition(int64_t value)
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
		if(GD::debugLevel >= 3) std::cout << "Warning: Boolean operator is none." << std::endl;
		break;
	}
	return false;
}

std::shared_ptr<RPCVariable> Parameter::convertFromPacket(int32_t value)
{
	if(logicalParameter->type == LogicalParameter::Type::Enum::typeEnum)
	{
		return std::shared_ptr<RPCVariable>(new RPCVariable(value));
	}
	else if(logicalParameter->type == LogicalParameter::Type::Enum::typeBoolean && conversion.empty())
	{
		return std::shared_ptr<RPC::RPCVariable>(new RPCVariable((bool)value));
	}
	else if(logicalParameter->type == LogicalParameter::Type::Enum::typeString)
	{
		return std::shared_ptr<RPC::RPCVariable>(new RPCVariable(std::to_string(value)));
	}
	else if(logicalParameter->type == LogicalParameter::Type::Enum::typeAction)
	{
		return std::shared_ptr<RPC::RPCVariable>(new RPCVariable(false));
	}
	else
	{
		std::shared_ptr<RPCVariable> variable(new RPCVariable(value));
		for(std::vector<std::shared_ptr<ParameterConversion>>::reverse_iterator i = conversion.rbegin(); i != conversion.rend(); ++i)
		{
			(*i)->fromPacket(variable);
		}
		return variable;
	}
	return std::shared_ptr<RPC::RPCVariable>(new RPCVariable(value));
}

int32_t Parameter::convertToPacket(std::shared_ptr<RPCVariable> value)
{
	if(logicalParameter->type == LogicalParameter::Type::Enum::typeEnum)
	{
		LogicalParameterEnum* parameter = (LogicalParameterEnum*)logicalParameter.get();
		if(value->integerValue > parameter->max) return parameter->max;
		if(value->integerValue < parameter->min) return parameter->min;
		return value->integerValue;
	}
	else if(logicalParameter->type == LogicalParameter::Type::Enum::typeAction)
	{
		return true;
	}
	else if(logicalParameter->type == LogicalParameter::Type::Enum::typeString)
	{
		//String is only supported when it is a number (like UI_HINT)
		return HelperFunctions::getNumber(value->stringValue);
	}
	else
	{
		if(logicalParameter->type == LogicalParameter::Type::Enum::typeFloat)
		{
			LogicalParameterFloat* parameter = (LogicalParameterFloat*)logicalParameter.get();
			if(value->floatValue > parameter->max) value->floatValue = parameter->max;
			if(value->floatValue < parameter->min) value->floatValue = parameter->min;
		}
		else if(logicalParameter->type == LogicalParameter::Type::Enum::typeInteger)
		{
			LogicalParameterInteger* parameter = (LogicalParameterInteger*)logicalParameter.get();
			if(value->integerValue > parameter->max) value->integerValue = parameter->max;
			if(value->integerValue < parameter->min) value->integerValue = parameter->min;
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
		return variable->integerValue;
	}
	return value->integerValue;
}

Parameter::Parameter(xml_node<>* node, bool checkForID) : Parameter()
{
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
			if(attributeValue == "e") booleanOperator = BooleanOperator::Enum::e;
			else if(attributeValue == "g") booleanOperator = BooleanOperator::Enum::g;
			else if(attributeValue == "l") booleanOperator = BooleanOperator::Enum::l;
			else if(attributeValue == "ge") booleanOperator = BooleanOperator::Enum::ge;
			else if(attributeValue == "le") booleanOperator = BooleanOperator::Enum::le;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute value for \"cond_op\" in node \"parameter\": " << attributeValue << std::endl;
		}
		else if(attributeName == "const_value") constValue = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "id") id = attributeValue;
		else if(attributeName == "param") param = attributeValue;
		else if(attributeName == "PARAM") additionalParameter = attributeValue;
		else if(attributeName == "control") control = attributeValue;
		else if(attributeName == "loopback") { if(attributeValue == "true") loopback = true; }
		else if(attributeName == "type")
		{
			if(attributeValue == "integer") type = PhysicalParameter::Type::Enum::typeInteger;
			else if(attributeValue == "boolean") type = PhysicalParameter::Type::Enum::typeBoolean;
			else if(attributeValue == "option") type = PhysicalParameter::Type::Enum::typeOption;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute value for \"type\" in node \"parameter\": " << attributeValue << std::endl;
		}
		else if(attributeName == "omit_if")
		{
			if(type != PhysicalParameter::Type::Enum::typeInteger)
			{
				if(GD::debugLevel >= 3) std::cout << "Warning: \"omit_if\" is only supported for type \"integer\" in node \"parameter\"." << std::endl;
				continue;
			}
			omitIfSet = true;
			omitIf = HelperFunctions::getNumber(attributeValue);
		}
		else if(attributeName == "operations")
		{
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
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"parameter\": " << attributeName << std::endl;
	}
	if(checkForID && id.empty() && GD::debugLevel >= 2)
	{
		std::cout << "Error: Parameter has no id." << std::endl;
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
			conversion.push_back(std::shared_ptr<ParameterConversion>(new ParameterConversion(parameterNode)));
		}
		else if(nodeName == "description")
		{
			description = ParameterDescription(parameterNode);
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown subnode for \"parameter\": " << nodeName << std::endl;
	}
}

int64_t Parameter::getBytes(int32_t value)
{
	try
	{
		int64_t returnValue = 0;;
		if(physicalParameter->size < 0)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Negative size not allowed." << std::endl;
			return returnValue;
		}
		double i = physicalParameter->index;
		i -= std::floor(i);
		double byteIndex = std::floor(i);
		if(byteIndex != i || physicalParameter->size < 0.8) //0.8 == 8 Bits
		{
			if(physicalParameter->size > 1)
			{
				if(GD::debugLevel >= 2) std::cout << "Error: Can't set partial byte index > 1." << std::endl;
				return returnValue;
			}
			uint32_t bitSize = std::lround(physicalParameter->size * 10);
			if(value < 0) //has sign?
			{
				value = value & _bitmask[bitSize];
			}
			returnValue = value << (std::lround(i * 10) % 10);
		}
		else
		{
			uint32_t bytes = (uint32_t)std::ceil(physicalParameter->size);
			uint32_t bitSize = std::lround(physicalParameter->size * 10) % 10;
			if(bytes == 0) bytes = 1; //size is 0 - assume 1
			returnValue = ((value >> ((bytes - 1) * 8)) & _bitmask[bitSize]) << ((bytes - 1) * 8);
			for(uint32_t i = 1; i < bytes; i++)
			{
				returnValue |= (value >> ((bytes - i - 1) * 8)) << ((bytes - 1 - i) * 8);
			}
		}
		return returnValue;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return 0;
}

bool DeviceType::matches(HMDeviceTypes deviceType, uint32_t firmwareVersion)
{
	try
	{
		if(parameters.empty()) return false;
		bool match = true;
		for(std::vector<Parameter>::iterator i = parameters.begin(); i != parameters.end(); ++i)
		{
			//This might not be the optimal way to get the xml rpc device, because it assumes the device type is unique
			//When the device type is not at index 10 of the pairing packet, the device is not supported
			//The "priority" attribute is ignored, for the standard devices "priority" seems not important
			if(i->index == 10.0) { if(i->constValue != (int32_t)deviceType) match = false; }
			else if(i->index == 9.0) { if(!i->checkCondition(firmwareVersion)) match = false; }
			else match = false; //Unknown index
		}
		if(match) return true;
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    return false;
}

bool DeviceType::matches(std::string typeID)
{
	try
	{
		if(id == typeID) return true;
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    return false;
}

bool DeviceType::matches(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(parameters.empty()) return false;
		for(std::vector<Parameter>::iterator i = parameters.begin(); i != parameters.end(); ++i)
		{
			if(!i->checkCondition(packet->getPosition(i->index, i->size, i->isSigned))) return false;
		}
		return true;
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
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
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"type\": " << attributeName << std::endl;
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

std::vector<std::shared_ptr<Parameter>> ParameterSet::getIndices(int32_t startIndex, int32_t endIndex, int32_t list)
{
	std::vector<std::shared_ptr<Parameter>> filteredParameters;
	if(list < 0) return filteredParameters;
	for(std::vector<std::shared_ptr<Parameter>>::iterator i = parameters.begin(); i != parameters.end(); ++i)
	{
		if((*i)->physicalParameter->list != (unsigned)list) continue;
		if((*i)->physicalParameter->index >= startIndex && std::floor((*i)->physicalParameter->index) <= endIndex) filteredParameters.push_back(*i);
	}
	return filteredParameters;
}

std::shared_ptr<Parameter> ParameterSet::getIndex(double index)
{
	std::vector<std::shared_ptr<Parameter>> filteredParameters;
	for(std::vector<std::shared_ptr<Parameter>>::iterator i = parameters.begin(); i != parameters.end(); ++i)
	{
		if((*i)->physicalParameter->index == index) return *i;
	}
	return nullptr;
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
	for(std::vector<std::shared_ptr<Parameter>>::iterator i = parameters.begin(); i != parameters.end(); ++i)
	{
		if((*i)->id == id) return *i;
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
			if(GD::debugLevel >= 3 && type == Type::Enum::none) std::cout << "Warning: Unknown parameter set type: " << attributeValue << std::endl;
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"paramset\": " << attributeName << std::endl;
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
				if(GD::debugLevel >= 3) std::cout << "Warning: Could not parse \"enforce\". Attribute id or value not set." << std::endl;
				continue;
			}
			enforce.push_back(std::pair<std::string, std::string>(std::string(attr1->value()), std::string(attr2->value())));
		}
		else if(nodeName == "subset")
		{
			xml_attribute<>* attr = parameterNode->first_attribute("ref");
			if(!attr)
			{
				if(GD::debugLevel >= 3) std::cout << "Warning: Could not parse \"subset\". Attribute ref not set." << std::endl;
				continue;
			}
			subsetReference = std::string(attr->value());
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown node name for \"paramset\": " << nodeName << std::endl;
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
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown node name for \"link_roles\": " << nodeName << std::endl;
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
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"enforce_link - value\": " << attributeName << std::endl;
	}
}

std::shared_ptr<RPCVariable> EnforceLink::getValue(RPCVariableType type)
{
	return RPCVariable::fromString(value, type);
}

DeviceChannel::DeviceChannel(xml_node<>* node, uint32_t& index)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "index") index = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "ui_flags")
		{
			if(attributeValue == "visible") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::visible);
			else if(attributeValue == "internal") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::internal);
			else if(attributeValue == "dontdelete") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::dontdelete);
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown ui flag for \"channel\": " << attributeValue << std::endl;
		}
		else if(attributeName == "direction")
		{
			if(attributeValue == "sender") direction = (Direction::Enum)(direction | Direction::Enum::sender);
			else if(attributeValue == "receiver") direction = (Direction::Enum)(direction | Direction::Enum::receiver);
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown direction for \"channel\": " << attributeValue << std::endl;
		}
		else if(attributeName == "class") channelClass = attributeValue;
		else if(attributeName == "type") type = attributeValue;
		else if(attributeName == "hidden") { if(attributeValue == "true") hidden = true; }
		else if(attributeName == "autoregister") { if(attributeValue == "true") autoregister = true; }
		else if(attributeName == "count") count = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "has_team") { if(attributeValue == "true") hasTeam = true; }
		else if(attributeName == "aes_default") { if(attributeValue == "true") aesDefault = true; }
		else if(attributeName == "team_tag") teamTag = attributeValue;
		else if(attributeName == "count_from_sysinfo")
		{
			std::pair<std::string, std::string> splitValue = HelperFunctions::split(attributeValue, ':');
			if(!splitValue.first.empty()) countFromSysinfo = HelperFunctions::getDouble(splitValue.first);
			if(!splitValue.second.empty()) countFromSysinfoSize = HelperFunctions::getDouble(splitValue.second);
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"channel\": " << attributeName << std::endl;
	}
	for(xml_node<>* channelNode = node->first_node(); channelNode; channelNode = channelNode->next_sibling())
	{
		std::string nodeName(channelNode->name());
		if(nodeName == "paramset")
		{
			std::shared_ptr<ParameterSet> parameterSet(new ParameterSet(channelNode));
			if(parameterSets.find(parameterSet->type) == parameterSets.end()) parameterSets[parameterSet->type] = parameterSet;
			else if(GD::debugLevel >= 2) std::cerr << "Error: Tried to add same parameter set type twice." << std::endl;
		}
		else if(nodeName == "link_roles")
		{
			if(linkRoles && GD::debugLevel >=3) std::cout << "Warning: Multiple link roles are defined for channel " << index << "." << std::endl;
			linkRoles.reset(new LinkRole(channelNode));
		}
		else if(nodeName == "enforce_link")
		{
			for(xml_node<>* enforceLinkNode = channelNode->first_node("value"); enforceLinkNode; enforceLinkNode = enforceLinkNode->next_sibling("value"))
			{
				enforceLinks.push_back(std::shared_ptr<EnforceLink>(new EnforceLink(enforceLinkNode)));
			}
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown node name for \"device\": " << nodeName << std::endl;
	}
}

Device::Device()
{
	parameterSet.reset(new ParameterSet());
}

Device::Device(std::string xmlFilename) : Device()
{
	load(xmlFilename);

	if(!channels[0]) channels[0] = std::shared_ptr<DeviceChannel>(new DeviceChannel());
	if(!channels[0]->parameterSets[ParameterSet::Type::Enum::master]) channels[0]->parameterSets[ParameterSet::Type::Enum::master] = std::shared_ptr<ParameterSet>(new ParameterSet());
	if(parameterSet->type == ParameterSet::Type::Enum::master && !parameterSet->parameters.empty())
	{
		if(channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.size() > 0 && GD::debugLevel >= 2)
		{
			std::cout << "Error: Master parameter set of channnel 0 has to be empty." << std::endl;
		}
		channels[0]->parameterSets[ParameterSet::Type::Enum::master] = parameterSet;
	}

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
				if(framesByID.find((*k)->frame) != framesByID.end()) framesByID[(*k)->frame]->associatedValues.push_back(*j);
			}
		}
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
		else throw Exception("Error reading file " + xmlFilename + ". Error number: " + std::to_string(errno));
		_loaded = true;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
			else if(attributeName == "rx_modes")
			{
				std::stringstream stream(attributeValue);
				std::string element;
				while(std::getline(stream, element, ','))
				{
					HelperFunctions::toLower(HelperFunctions::trim(element));
					if(element == "wakeup") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::wakeUp);
					else if(element == "config") rxModes = (RXModes::Enum)(rxModes | RXModes::Enum::config);
				}
			}
			else if(attributeName == "class")
			{
				deviceClass = attributeValue;
			}
			else if(attributeName == "ui_flags")
			{
				if(attributeValue == "visible") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::visible);
				else if(attributeValue == "internal") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::internal);
				else if(attributeValue == "dontdelete") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::dontdelete);
				else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown ui flag for \"channel\": " << attributeValue << std::endl;
			}
			else if(attributeName == "cyclic_timeout") cyclicTimeout = HelperFunctions::getNumber(attributeValue);
			else if(attributeName == "supports_aes") { if(attributeValue == "true") supportsAES = true; }
			else if(attributeName == "peering_sysinfo_expect_channel") { if(attributeValue == "false") peeringSysinfoExpectChannel = false; }
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"device\": " << attributeName << std::endl;
		}

		std::map<std::string, std::shared_ptr<ParameterSet>> parameterSetDefinitions;
		for(node = node->first_node(); node; node = node->next_sibling())
		{
			std::string nodeName(node->name());
			if(nodeName == "supported_types")
			{
				for(xml_node<>* typeNode = node->first_node("type"); typeNode; typeNode = typeNode->next_sibling())
				{
					supportedTypes.push_back(std::shared_ptr<DeviceType>(new DeviceType(typeNode)));
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
						else if(GD::debugLevel >= 2) std::cout << "Error: Tried to add parameter set of type \"" << attributeValue << "\" to device. That is not allowed." << std::endl;
					}
					else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"paramset\": " << attributeName << std::endl;
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
					else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"paramset_defs\": " << attributeName << std::endl;
				}

				for(xml_node<>* paramsetNode = node->first_node(); paramsetNode; paramsetNode = paramsetNode->next_sibling())
				{
					std::string nodeName(paramsetNode->name());
					if(nodeName == "paramset")
					{
						std::shared_ptr<ParameterSet> parameterSet(new ParameterSet(paramsetNode));
						parameterSetDefinitions[parameterSet->id] = parameterSet;
					}
					else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown node name for \"paramset_defs\": " << nodeName << std::endl;
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
						else if(GD::debugLevel >= 2) std::cout << "Error: Tried to add channel with the same index twice. Index: " << i << std::endl;
					}
					if(channel->countFromSysinfo)
					{
						if(countFromSysinfoIndex > -1 && GD::debugLevel >= 2) std::cout << "Error: count_from_sysinfo is defined for two channels. That is not allowed." << std::endl;
						if(std::floor(channel->countFromSysinfo) != channel->countFromSysinfo && GD::debugLevel >= 2)
						{
							std::cout << "Error: count_from_sysinfo has to start with index 0 of a byte." << std::endl;
							continue;
						}
						countFromSysinfoIndex = (int32_t)channel->countFromSysinfo;
						if(countFromSysinfoIndex < -1) countFromSysinfoIndex = -1;
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
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown node name for \"device\": " << nodeName << std::endl;
		}
		if(parameterSetDefinitions.empty()) return;
		for(std::map<uint32_t, std::shared_ptr<DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
		{
			for(std::map<ParameterSet::Type::Enum, std::shared_ptr<ParameterSet>>::iterator j = i->second->parameterSets.begin(); j != i->second->parameterSets.end(); ++j)
			{
				if(j->second->subsetReference.empty() || parameterSetDefinitions.find(j->second->subsetReference) == parameterSetDefinitions.end()) continue;
				std::shared_ptr<ParameterSet> parameterSet(new ParameterSet());
				*parameterSet = *parameterSetDefinitions.at(j->second->subsetReference);
				parameterSet->type = j->second->type;
				parameterSet->id = j->second->id;
				i->second->parameterSets[parameterSet->type] = parameterSet;
				for(std::vector<std::shared_ptr<Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
				{
					(*i)->parentParameterSet = parameterSet.get();
				}
			}
		}
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
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
		for(uint32_t i = index; i < index + countFromSysinfo; i++)
		{
			if(channels.find(i) == channels.end()) channels[i] = channel;
			else if(GD::debugLevel >= 2) std::cout << "Error: Tried to add channel with the same index twice. Index: " << i << std::endl;
		}
	}
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

std::shared_ptr<DeviceType> Device::getType(HMDeviceTypes deviceType, int32_t firmwareVersion)
{
	for(std::vector<std::shared_ptr<DeviceType>>::iterator j = supportedTypes.begin(); j != supportedTypes.end(); ++j)
	{
		if((*j)->matches(deviceType, firmwareVersion)) return *j;
	}
	return std::shared_ptr<DeviceType>();
}

}
