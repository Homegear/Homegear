#include "PhysicalParameter.h"
#include "../GD.h"
#include "../HelperFunctions.h"

namespace RPC {

PhysicalParameter::PhysicalParameter(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "type") {
			if(attributeValue == "integer") type = Type::Enum::typeInteger;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown physical type: " << attributeValue << std::endl;
		}
		else if(attributeName == "interface")
		{
			if(attributeValue == "command") interface = Interface::Enum::command;
			else if(attributeValue == "central_command") interface = Interface::Enum::centralCommand;
			else if(attributeValue == "internal") interface = Interface::Enum::internal;
			else if(attributeValue == "config") interface = Interface::Enum::config;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown interface for \"physical\": " << attributeValue << std::endl;
		}
		else if(attributeName == "value_id") valueID = attributeValue;
		else if(attributeName == "no_init" && attributeValue == "true") noInit = true;
		else if(attributeName == "list") list = HelperFunctions::getNumber(attributeValue);
		else if(attributeName == "index")
		{
			std::pair<std::string, std::string> splitValue = HelperFunctions::split(attributeValue, '.');
			index = 0;
			if(!splitValue.second.empty())
			{
				index += HelperFunctions::getNumber(splitValue.second);
				index /= 10;
			}
			index += HelperFunctions::getNumber(splitValue.first);
		}
		else if(attributeName == "size") size = HelperFunctions::getDouble(attributeValue);
		else if(attributeName == "counter") counter = attributeValue;
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"physical\": " << attributeName << std::endl;
	}
	for(xml_node<>* physicalNode = node->first_node(); physicalNode; physicalNode = physicalNode->next_sibling())
	{
		std::string nodeName(physicalNode->name());
		xml_attribute<>* attr;
		if(nodeName == "set")
		{
			attr = physicalNode->first_attribute("request");
			if(attr != nullptr) setRequest = std::string(attr->value());
		}
		else if(nodeName == "get")
		{
			attr = physicalNode->first_attribute("request");
			if(attr != nullptr) getRequest = std::string(attr->value());
		}
		else if(nodeName == "event")
		{
			attr = physicalNode->first_attribute("frame");
			if(attr != nullptr) eventFrames.push_back(std::string(attr->value()));
		}
	}
}

} /* namespace XMLRPC */
