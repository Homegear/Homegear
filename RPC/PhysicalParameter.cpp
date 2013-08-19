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
			else if(attributeValue == "boolean") type = Type::Enum::typeBoolean;
			else if(attributeValue == "string") type = Type::Enum::typeString;
			else HelperFunctions::printWarning("Warning: Unknown physical type: " + attributeValue);
		}
		else if(attributeName == "interface")
		{
			if(attributeValue == "command") interface = Interface::Enum::command;
			else if(attributeValue == "central_command") interface = Interface::Enum::centralCommand;
			else if(attributeValue == "internal") interface = Interface::Enum::internal;
			else if(attributeValue == "config") interface = Interface::Enum::config;
			else if(attributeValue == "config_string") interface = Interface::Enum::configString;
			else if(attributeValue == "store") interface = Interface::Enum::store;
			else HelperFunctions::printWarning("Warning: Unknown interface for \"physical\": " + attributeValue);
		}
		else if(attributeName == "value_id") valueID = attributeValue;
		else if(attributeName == "no_init") { if(attributeValue == "true") noInit = true; }
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
		else if(attributeName == "size")
		{
			std::pair<std::string, std::string> splitValue = HelperFunctions::split(attributeValue, '.');
			size = 0;
			if(!splitValue.second.empty())
			{
				size += HelperFunctions::getNumber(splitValue.second);
				size /= 10;
			}
			size += HelperFunctions::getNumber(splitValue.first);
		}
		else if(attributeName == "counter") counter = attributeValue;
		else if(attributeName == "volatile") { if(attributeValue == "true") isVolatile = true; }
		else if(attributeName == "id") { id = attributeValue; }
		else if(attributeName == "save_on_change") {} //not necessary, all values are saved on change
		else if(attributeName == "mask") mask = HelperFunctions::getNumber(attributeValue);
		else HelperFunctions::printWarning("Warning: Unknown attribute for \"physical\": " + attributeName);
	}
	if(mask != -1 && fmod(index, 1) != 0) HelperFunctions::printWarning("Warning: mask combined with unaligned index not supported.");
	startIndex = std::lround(std::floor(index));
	int32_t intDiff = std::lround(std::floor(size)) - 1;
	if(intDiff < 0) intDiff = 0;
	endIndex = startIndex + intDiff;
	for(xml_node<>* physicalNode = node->first_node(); physicalNode; physicalNode = physicalNode->next_sibling())
	{
		std::string nodeName(physicalNode->name());
		xml_attribute<>* attr;
		if(nodeName == "set")
		{
			attr = physicalNode->first_attribute("request");
			if(attr) setRequest = std::string(attr->value());
		}
		else if(nodeName == "get")
		{
			attr = physicalNode->first_attribute("request");
			if(attr) getRequest = std::string(attr->value());
		}
		else if(nodeName == "event")
		{
			attr = physicalNode->first_attribute("frame");
			if(!attr) continue;
			std::shared_ptr<PhysicalParameterEvent> event(new PhysicalParameterEvent());
			event->frame = std::string(attr->value());
			if(type == Type::Enum::typeInteger)
			{
				for(xml_node<>* eventNode = physicalNode->first_node(); eventNode; eventNode = eventNode->next_sibling())
				{
					std::string eventNodeName(eventNode->name());
					if(eventNodeName == "domino_event")
					{
						xml_attribute<>* attr1 = eventNode->first_attribute("value");
						xml_attribute<>* attr2 = eventNode->first_attribute("delay_id");
						if(!attr1 || !attr2) continue;
						event->dominoEvent = true;
						std::string eventValue = std::string(attr1->value());
						event->dominoEventValue = HelperFunctions::getNumber(eventValue);
						event->dominoEventDelayID = std::string(attr2->value());
					}
					else HelperFunctions::printWarning("Warning: Unknown node for \"physical\\event\": " + eventNodeName);
				}
			}
			else HelperFunctions::printWarning("Warning: domino_event is only supported for physical type integer.");
			eventFrames.push_back(event);
		}
		else if(nodeName == "reset_after_send")
		{
			attr = physicalNode->first_attribute("param");
			if(attr) resetAfterSend.push_back(std::string(attr->value()));
		}
		else HelperFunctions::printWarning("Warning: Unknown node for \"physical\": " + nodeName);
	}
}

} /* namespace XMLRPC */
