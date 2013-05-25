#include "PhysicalParameter.h"
#include "../GD.h"

namespace XMLRPC {

PhysicalParameter::PhysicalParameter(xml_node<>* node)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "type") {
			if(attributeValue == "integer") _type = Type::Enum::typeInteger;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown physical type: " << attributeValue << std::endl;
		}
		else if(attributeName == "interface")
		{
			if(attributeValue == "command") interface = Interface::Enum::command;
			else if(attributeValue == "internal") interface = Interface::Enum::internal;
			else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown interface for \"physical\": " << attributeValue << std::endl;
		}
		else if(attributeName == "value_id") valueID = attributeValue;
		else if(attributeName == "no_init" && attributeValue == "true") noInit = true;
		else if(GD::debugLevel >= 3) std::cout << "Warning: Unknown attribute for \"physical\": " << attributeName << std::endl;
	}
	for(xml_node<>* physicalNode = node->first_node(); physicalNode; physicalNode = physicalNode->next_sibling())
	{
		std::string nodeName(node->name());
		xml_attribute<>* attr;
		if(nodeName == "set")
		{
			attr = node->first_attribute("request");
			if(attr != nullptr) setRequest = std::string(attr->value());
		}
		else if(nodeName == "get")
		{
			attr = node->first_attribute("request");
			if(attr != nullptr) getRequest = std::string(attr->value());
		}
		else if(nodeName == "event")
		{
			attr = node->first_attribute("frame");
			if(attr != nullptr) eventFrames.push_back(std::string(attr->value()));
		}
	}
}

} /* namespace XMLRPC */
