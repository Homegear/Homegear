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
		else if(attributeName == "channel_field") channelField = std::stoll(attributeValue);
		else if(attributeName == "fixed_channel") fixedChannel = std::stoll(attributeValue);
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"frame\": " << attributeName << std::endl;
	}
	for(xml_node<>* frameNode = node->first_node("parameter"); frameNode; frameNode = frameNode->next_sibling("parameter"))
	{
		parameters.push_back(frameNode);
	}
}

std::shared_ptr<RPC::RPCVariable> ParameterConversion::fromPacket(int32_t value)
{
	if(type == Type::Enum::none)
	{
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(value));
	}
	else if(type == Type::Enum::floatIntegerScale)
	{
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((double)value / factor));
	}
	else if(type == Type::Enum::integerIntegerScale)
	{
		if(div > 0)
		{
			return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(value * div));
		}
		else if(mul > 0)
		{
			return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(value / mul));
		}
		else
		{
			return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(value));
		}
	}
	else if(type == Type::Enum::integerIntegerMap)
	{
		if(integerIntegerMapDevice.find(value) != integerIntegerMapDevice.end()) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(integerIntegerMapDevice[value]));
	}
	else if(type == Type::Enum::booleanInteger)
	{
		if(value == valueFalse) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(false));
		if(value == valueTrue || value > threshold) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true));
	}
	return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(value));
}

int32_t ParameterConversion::toPacket(std::shared_ptr<RPC::RPCVariable> value)
{
	if(type == Type::Enum::none)
	{
		return value->integerValue;
	}
	else if(type == Type::Enum::floatIntegerScale)
	{
		return std::lround(value->floatValue * factor);
	}
	else if(type == Type::Enum::integerIntegerScale)
	{
		if(div > 0)
		{
			return value->integerValue / div;
		}
		else if(mul > 0)
		{
			return value->integerValue * mul;
		}
		else
		{
			return value->integerValue;
		}
	}
	else if(type == Type::Enum::integerIntegerMap)
	{
		if(integerIntegerMapParameter.find(value->integerValue) != integerIntegerMapParameter.end()) return integerIntegerMapParameter[value->integerValue];
	}
	else if(type == Type::Enum::booleanInteger)
	{
		if(value->booleanValue) return valueTrue;
		else return valueFalse;
	}
	return value->integerValue;
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
		}
		else if(attributeName == "factor") factor = std::stod(attributeValue);
		else if(attributeName == "threshold") threshold = std::stoll(attributeValue);
		else if(attributeName == "false") valueFalse = std::stoll(attributeValue);
		else if(attributeName == "true") valueTrue = std::stoll(attributeValue);
		else if(attributeName == "div") div = std::stoll(attributeValue);
		else if(attributeName == "mul") mul = std::stoll(attributeValue);
		else if(attributeName == "offset") offset = std::stod(attributeValue);
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"conversion\": " << attributeName << std::endl;
	}
	for(xml_node<>* conversionNode = node->first_node(); conversionNode; conversionNode = conversionNode->next_sibling())
	{
		std::string nodeName(conversionNode->name());
		if(nodeName == "value_map" && type == Type::Enum::integerIntegerMap)
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
				integerIntegerMapDevice[deviceValue] = parameterValue;
				integerIntegerMapParameter[parameterValue] = deviceValue;
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
	else if(logicalParameter->type == LogicalParameter::Type::Enum::typeAction)
	{
		return false;
	}
	else
	{
		return conversion.fromPacket(value);
	}
	return std::shared_ptr<RPC::RPCVariable>(new RPCVariable(value));
}

int32_t Parameter::convertToPacket(std::shared_ptr<RPCVariable> value)
{
	if(logicalParameter->type == LogicalParameter::Type::Enum::typeEnum)
	{
		return value->integerValue;
	}
	else if(logicalParameter->type == LogicalParameter::Type::Enum::typeAction)
	{
		return true;
	}
	else
	{
		return conversion.toPacket(value);
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
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"parameter\": " << attributeName << std::endl;
		}
		else if(attributeName == "const_value") constValue = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "id") id = attributeValue;
		else if(attributeName == "param") param = attributeValue;
		else if(attributeName == "control") control = attributeValue;
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
		}
		else if(nodeName == "conversion")
		{
			conversion = ParameterConversion(parameterNode);
		}
		else if(nodeName == "description")
		{
			description = ParameterDescription(parameterNode);
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown subnode for \"parameter\": " << nodeName << std::endl;
	}
}

bool DeviceType::matches(HMDeviceTypes deviceType, uint32_t firmwareVersion)
{
	try
	{
		for(std::vector<Parameter>::iterator i = parameters.begin(); i != parameters.end(); ++i)
		{
			bool match = true;
			//This might not be the optimal way to get the xml rpc device, because it assumes the device type is unique
			//When the device type is not at index 10 of the pairing packet, the device is not supported
			//The "priority" attribute is ignored, for the standard devices "priority" seems not important
			if(i->index == 10.0 && i->constValue != (int32_t)deviceType) match = false;
			if(i->index == 9.0 && !i->checkCondition(firmwareVersion)) match = false;
			if(match) return true;
		}
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
		bool match = true;
		for(std::vector<Parameter>::iterator i = parameters.begin(); i != parameters.end(); ++i)
		{
			if(!i->checkCondition(packet->getPosition(i->index, i->size, i->isSigned))) match = false;
		}
		return match;
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
		else if(attributeName == "priority") priority = std::stoll(attributeValue);
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"type\": " << attributeName << std::endl;
	}
	for(xml_node<>* parameterNode = typeNode->first_node("parameter"); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		Parameter parameter(parameterNode);
		parameters.push_back(parameter);
	}
}

ParameterSet::ParameterSet(int32_t channelNumber, xml_node<>* parameterSetNode)
{
	channel = channelNumber;
	init(parameterSetNode);
}

std::vector<std::shared_ptr<Parameter>> ParameterSet::getIndices(int32_t startIndex, int32_t endIndex)
{
	std::vector<std::shared_ptr<Parameter>> filteredParameters;
	for(std::vector<std::shared_ptr<Parameter>>::iterator i = parameters.begin(); i != parameters.end(); ++i)
	{
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
			HelperFunctions::toLower(HelperFunctions::trim(attributeValue));
			if(attributeValue == "master") type = Type::Enum::master;
			else if(attributeValue == "values") type = Type::Enum::values;
			else if(attributeValue == "link") type = Type::Enum::link;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown parameter set type: " << attributeValue << std::endl;
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"paramset\": " << attributeName << std::endl;
	}
	for(xml_node<>* parameterNode = parameterSetNode->first_node("parameter"); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		std::shared_ptr<Parameter> parameter(new Parameter(parameterNode, true));
		parameters.push_back(parameter);
		parameter->parentParameterSet = this;
		if(parameter->physicalParameter->list < 9999) lists[parameter->physicalParameter->list] = 1;
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
		if(attributeName == "id" && attributeValue == "PEER_NEEDS_BURST") id = ID::Enum::peerNeedsBurst;
		else if(attributeName == "value")
		{
			if(attributeValue == "true") value = 1;
			else value = stoll(attributeValue);
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"enforce_link - value\": " << attributeName << std::endl;
	}
}

DeviceChannel::DeviceChannel(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "index") index = std::stoll(attributeValue);
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
		else if(attributeName == "count") count = std::stoll(attributeValue);
		else if(attributeName == "has_team") { if(attributeValue == "true") hasTeam = true; }
		else if(attributeName == "aes_default") { if(attributeValue == "true") aesDefault = true; }
		else if(attributeName == "team_tag") teamTag = attributeValue;
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"channel\": " << attributeName << std::endl;
	}
	for(xml_node<>* channelNode = node->first_node(); channelNode; channelNode = channelNode->next_sibling())
	{
		std::string nodeName(channelNode->name());
		if(nodeName == "paramset")
		{
			std::shared_ptr<ParameterSet> parameterSet(new ParameterSet(index, channelNode));
			if(parameterSets.find(parameterSet->type) == parameterSets.end()) parameterSets[parameterSet->type] = parameterSet;
			else if(GD::debugLevel >= 2) std::cout << "Error: Tried to add same parameter set type twice." << std::endl;
		}
		else if(nodeName == "link_roles")
		{
			linkRoles.push_back(std::shared_ptr<LinkRole>(new LinkRole(channelNode)));
		}
		else if(nodeName == "enforce_link")
		{
			for(xml_node<>* enforceLinkNode = channelNode->first_node("value"); enforceLinkNode; enforceLinkNode = enforceLinkNode->next_sibling("value"))
			{
				enforceLinks.push_back(std::shared_ptr<EnforceLink>(new EnforceLink(channelNode)));
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
	if(parameterSet->type == ParameterSet::Type::Enum::master)
	{
		if(channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.size() > 0 && GD::debugLevel >= 2)
		{
			std::cout << "Error: Master parameter set of channnel 0 has to be empty." << std::endl;
		}
		channels[0]->parameterSets[ParameterSet::Type::Enum::master] = parameterSet;
	}
	std::shared_ptr<Parameter> parameter(new Parameter());
	parameter->id = "PAIRED_TO_CENTRAL";
	parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeBoolean;
	parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
	parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeBoolean;
	parameter->physicalParameter->valueID = "PAIRED_TO_CENTRAL";
	parameter->physicalParameter->list = 0;
	parameter->physicalParameter->index = 2;
	channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);

	parameter.reset(new Parameter());
	parameter->id = "CENTRAL_ADDRESS_BYTE_1";
	parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeInteger;
	parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
	parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeInteger;
	parameter->physicalParameter->valueID = "CENTRAL_ADDRESS_BYTE_1";
	parameter->physicalParameter->list = 0;
	parameter->physicalParameter->index = 10;
	channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);

	parameter.reset(new Parameter());
	parameter->id = "CENTRAL_ADDRESS_BYTE_2";
	parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeInteger;
	parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
	parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeInteger;
	parameter->physicalParameter->valueID = "CENTRAL_ADDRESS_BYTE_2";
	parameter->physicalParameter->list = 0;
	parameter->physicalParameter->index = 11;
	channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);

	parameter.reset(new Parameter());
	parameter->id = "CENTRAL_ADDRESS_BYTE_3";
	parameter->logicalParameter->type = LogicalParameter::Type::Enum::typeInteger;
	parameter->physicalParameter->interface = PhysicalParameter::Interface::Enum::internal;
	parameter->physicalParameter->type = PhysicalParameter::Type::Enum::typeInteger;
	parameter->physicalParameter->valueID = "CENTRAL_ADDRESS_BYTE_1";
	parameter->physicalParameter->list = 0;
	parameter->physicalParameter->index = 12;
	channels[0]->parameterSets[ParameterSet::Type::Enum::master]->parameters.push_back(parameter);

	for(std::map<uint32_t, std::shared_ptr<DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
	{
		for(std::vector<std::shared_ptr<Parameter>>::iterator j = i->second->parameterSets.at(ParameterSet::Type::Enum::values)->parameters.begin(); j != i->second->parameterSets.at(ParameterSet::Type::Enum::values)->parameters.end(); ++j)
		{
			if(!(*j)->physicalParameter->getRequest.empty() && framesByID.find((*j)->physicalParameter->getRequest) != framesByID.end()) framesByID[(*j)->physicalParameter->getRequest]->associatedValues.push_back(*j);
			for(std::vector<std::string>::iterator k = (*j)->physicalParameter->eventFrames.begin(); k != (*j)->physicalParameter->eventFrames.end(); ++k)
			{
				if(framesByID.find(*k) != framesByID.end()) framesByID[*k]->associatedValues.push_back(*j);
			}
		}
	}
}

Device::~Device() {

}

void Device::load(std::string xmlFilename)
{
	xml_document<> doc;
	std::ifstream fileStream(xmlFilename, std::ios::in | std::ios::binary);
	if(fileStream)
	{
		uint32_t length;
		fileStream.seekg(0, std::ios::end);
		length = fileStream.tellg();
		fileStream.seekg(0, std::ios::beg);
		char buffer[length];
		fileStream.read(&buffer[0], length);
		fileStream.close();
		doc.parse<0>(buffer);
		parseXML(doc.first_node("device"));
	}
	else
	{
		throw(errno);
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
			if(attributeName == "version") version = std::stoll(attributeValue);
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
			else if(attributeName == "cyclic_timeout") cyclicTimeout = std::stoll(attributeValue);
			else if(attributeName == "supports_aes") { if(attributeValue == "true") supportsAES = true; }
			else if(attributeName == "peering_sysinfo_expect_channel") { if(attributeValue == "false") peeringSysinfoExpectChannel = false; }
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"device\": " << attributeName << std::endl;
		}

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
			else if(nodeName == "channels")
			{
				for(xml_node<>* channelNode = node->first_node("channel"); channelNode; channelNode = channelNode->next_sibling())
				{
					std::shared_ptr<DeviceChannel> channel(new DeviceChannel(channelNode));
					channel->parentDevice = this;
					if(channels.find(channel->index) == channels.end()) channels[channel->index] = channel;
					else if(GD::debugLevel >= 2) std::cout << "Error: Tried to add channel with the same index twice." << std::endl;
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
			else if(nodeName == "paramset_defs")
			{
				if(node->first_node() != nullptr && GD::debugLevel >= 3) std::cout << "Warning: Unknown node name for \"paramset_defs\"" << std::endl;
			}
			else if(nodeName == "team")
			{
				team.reset(new Device());
				team->parseXML(node);
			}
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown node name for \"device\": " << nodeName << std::endl;
		}
	}
    catch(const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
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
