#include "Device.h"
#include "../HelperFunctions.h"
#include "../GD.h"

namespace XMLRPC {

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
		else if(attributeName == "type") type = std::stoll(attributeValue);
		else if(attributeName == "subtype") subtype = std::stoll(attributeValue);
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
			if(attr1 != nullptr && attr2 != nullptr) integerIntegerMap[std::stoll(attr1->value())] = std::stoll(attr2->value());
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown subnode for \"conversion\": " << nodeName << std::endl;
	}
}

Parameter::Parameter(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "index") index = std::stod(attributeValue);
		else if(attributeName == "size") size = std::stod(attributeValue);
		else if(attributeName == "cond_op")
		{
			HelperFunctions::toLower(HelperFunctions::trim(attributeValue));
			if(attributeValue == "e") conditionalOperator = ConditionalOperator::Enum::e;
			else if(attributeValue == "g") conditionalOperator = ConditionalOperator::Enum::g;
			else if(attributeValue == "l") conditionalOperator = ConditionalOperator::Enum::l;
			else if(attributeValue == "ge") conditionalOperator = ConditionalOperator::Enum::ge;
			else if(attributeValue == "le") conditionalOperator = ConditionalOperator::Enum::le;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"parameter\": " << attributeName << std::endl;
		}
		else if(attributeName == "const_value") constValue = std::stoll(attributeValue, 0, 16);
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
				if(element == "service") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::service);
				else if(element == "sticky") uiFlags = (UIFlags::Enum)(uiFlags | UIFlags::Enum::sticky);
			}
		}
	}
	for(xml_node<>* parameterNode = node->first_node(); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		std::string nodeName(parameterNode->name());
		if(nodeName == "logical")
		{
			LogicalParameter parameter;
			std::unique_ptr<LogicalParameter> pParameter = parameter.fromXML(parameterNode);
			if(pParameter.get() != nullptr) logicalParameters.push_back(*(pParameter.get()));
		}
		else if(nodeName == "physical")
		{
			physicalParameters.push_back(parameterNode);
		}
		else if(nodeName == "conversion")
		{
			conversions.push_back(parameterNode);
		}
		else if(nodeName == "description")
		{
			descriptions.push_back(parameterNode);
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown subnode for \"parameter\": " << nodeName << std::endl;
	}
}

bool DeviceType::matches(BidCoSPacket* packet)
{
	bla
}

DeviceType::DeviceType(xml_node<>* typeNode)
{
	for(xml_attribute<>* attr = typeNode->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "name") name = attributeValue;
		else if(attributeName == "id") id = attributeValue;
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

void ParameterSet::init(xml_node<>* parameterSetNode)
{
	for(xml_node<>* parameterNode = parameterSetNode->first_node("parameter"); parameterNode; parameterNode = parameterNode->next_sibling())
	{
		parameters.push_back(parameterNode);
	}
}

LinkRole::LinkRole(xml_node<>* node)
{
	for(xml_node<>* linkRoleNode = node->first_node(); linkRoleNode; linkRoleNode = linkRoleNode->next_sibling())
	{
		std::string nodeName(linkRoleNode->name());
		if(nodeName == "target")
		{
			xml_attribute<>* attr = node->first_attribute("name");
			if(attr != nullptr) targetName = std::string(attr->value());
		}
		else if(nodeName == "source")
		{
			xml_attribute<>* attr = node->first_attribute("name");
			if(attr != nullptr) sourceName = std::string(attr->value());
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
		else if(attributeName == "value" && attributeValue == "true") value = true;
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
			if(attributeValue == "internal") uiFlags = UIFlags::Enum::internal;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown ui flag for \"channel\": " << attributeValue << std::endl;
		}
		else if(attributeName == "class") channelClass = attributeValue;
		else if(attributeName == "type") type = attributeValue;
		else if(attributeName == "count") count = std::stoll(attributeValue);
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"device\": " << attributeName << std::endl;
	}
	for(xml_node<>* channelNode = node->first_node(); channelNode; channelNode = channelNode->next_sibling())
	{
		std::string nodeName(channelNode->name());
		if(nodeName == "paramset")
		{
			parameterSets.push_back(channelNode);
		}
		else if(nodeName == "link_roles")
		{
			linkRoles.push_back(channelNode);
		}
		else if(nodeName == "enforce_link")
		{
			for(xml_node<>* enforceLinkNode = channelNode->first_node("value"); enforceLinkNode; enforceLinkNode = enforceLinkNode->next_sibling("value"))
			{
				enforceLinks.push_back(enforceLinkNode);
			}
		}
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown node name for \"device\": " << nodeName << std::endl;
	}
}

Device::Device(std::string xmlFilename)
{
	load(xmlFilename);
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
		parseXML(&doc);
	}
	else
	{
		throw(errno);
	}
	doc.clear();
}

void Device::parseXML(xml_document<>* doc)
{
	try
	{
		//Device
		xml_node<>* node = doc->first_node("device");
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
			else if(attributeName == "cyclic_timeout") cyclicTimeout = std::stoll(attributeValue);
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"device\": " << attributeName << std::endl;
		}

		for(node = node->first_node(); node; node = node->next_sibling())
		{
			std::string nodeName(node->name());
			if(nodeName == "supported_types")
			{
				for(xml_node<>* typeNode = node->first_node("type"); typeNode; typeNode = typeNode->next_sibling())
				{
					supportedTypes.push_back(typeNode);
				}
			}
			else if(nodeName == "paramset")
			{
				for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
				{
					std::string attributeName(attr->name());
					std::string attributeValue(attr->value());
					HelperFunctions::toLower(HelperFunctions::trim(attributeValue));
					if(attributeName == "id") parameterSet.id = attributeValue;
					else if(attributeName == "type")
					{
						if(attributeValue == "MASTER") parameterSet.type = ParameterSet::Type::Enum::master;
						else if(attributeValue == "VALUES") parameterSet.type = ParameterSet::Type::Enum::values;
						else if(attributeValue == "LINK") parameterSet.type = ParameterSet::Type::Enum::values;
					}
					else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"paramset\": " << attributeName << std::endl;
				}
				parameterSet.init(node);
			}
			else if(nodeName == "channels")
			{
				for(xml_node<>* channelNode = node->first_node("channel"); channelNode; channelNode = channelNode->next_sibling())
				{
					channels.push_back(channelNode);
				}
			}
			else if(nodeName == "frames")
			{
				for(xml_node<>* frameNode = node->first_node("frame"); frameNode; frameNode = frameNode->next_sibling("frame"))
				{
					frames.push_back(frameNode);
				}
			}
			else if(nodeName == "paramset_defs")
			{
				if(node->first_node() != nullptr && GD::debugLevel >= 3) std::cout << "Warning: Unknown node name for \"paramset_defs\"" << std::endl;
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

}
