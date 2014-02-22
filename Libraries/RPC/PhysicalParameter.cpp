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

#include "PhysicalParameter.h"

namespace RPC {

PhysicalParameter::PhysicalParameter(xml_node<>* node)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "type") {
				if(attributeValue == "integer") type = Type::Enum::typeInteger;
				else if(attributeValue == "boolean") type = Type::Enum::typeBoolean;
				else if(attributeValue == "string") type = Type::Enum::typeString;
				else Output::printWarning("Warning: Unknown physical type: " + attributeValue);
			}
			else if(attributeName == "interface")
			{
				if(attributeValue == "command") interface = Interface::Enum::command;
				else if(attributeValue == "central_command") interface = Interface::Enum::centralCommand;
				else if(attributeValue == "internal") interface = Interface::Enum::internal;
				else if(attributeValue == "config") interface = Interface::Enum::config;
				else if(attributeValue == "config_string") interface = Interface::Enum::configString;
				else if(attributeValue == "store") interface = Interface::Enum::store;
				else if(attributeValue == "eeprom") interface = Interface::Enum::eeprom;
				else Output::printWarning("Warning: Unknown interface for \"physical\": " + attributeValue);
			}
			else if(attributeName == "endian")
			{
				if(attributeValue == "little") endian = Endian::Enum::little;
				else if(attributeValue == "big") endian = Endian::Enum::big; //default
				else Output::printWarning("Warning: Unknown endianess for \"physical\": " + attributeValue);
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
				sizeDefined = true;
			}
			else if(attributeName == "read_size") readSize = HelperFunctions::getNumber(attributeValue);
			else if(attributeName == "counter") counter = attributeValue;
			else if(attributeName == "volatile") { if(attributeValue == "true") isVolatile = true; }
			else if(attributeName == "id") { id = attributeValue; }
			else if(attributeName == "save_on_change") {} //not necessary, all values are saved on change
			else if(attributeName == "mask") mask = HelperFunctions::getNumber(attributeValue);
			else if(attributeName == "read_size") {} //not necessary, because size can be determined through index
			else Output::printWarning("Warning: Unknown attribute for \"physical\": " + attributeName);
		}
		if(mask != -1 && fmod(index, 1) != 0) Output::printWarning("Warning: mask combined with unaligned index not supported.");
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
				//ignore "response"
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
						else Output::printWarning("Warning: Unknown node for \"physical\\event\": " + eventNodeName);
					}
				}
				else Output::printWarning("Warning: domino_event is only supported for physical type integer.");
				eventFrames.push_back(event);
			}
			else if(nodeName == "address")
			{
				for(xml_attribute<>* addressAttr = physicalNode->first_attribute(); addressAttr; addressAttr = addressAttr->next_attribute())
				{
					std::string attributeName(addressAttr->name());
					std::string attributeValue(addressAttr->value());
					if(attributeName == "index")
					{
						if(attributeValue.substr(0, 1) == "+")
						{
							address.operation = PhysicalParameterAddress::Operation::addition;
							attributeValue.erase(0, 1);
						}
						else if(attributeValue.substr(0, 1) == "-")
						{
							address.operation = PhysicalParameterAddress::Operation::substraction;
							attributeValue.erase(0, 1);
						}
						std::pair<std::string, std::string> splitValue = HelperFunctions::split(attributeValue, '.');
						address.index = 0;
						if(!splitValue.second.empty())
						{
							address.index += HelperFunctions::getNumber(splitValue.second);
							address.index /= 10;
						}
						address.index += HelperFunctions::getNumber(splitValue.first);
						if(std::lround(address.index * 10) % 10 >= 8) address.index += 0.2; //e. g. 15.9 => 16.1
						index = address.index;
					}
					else if(attributeName == "step")
					{
						address.step = HelperFunctions::getDouble(attributeValue);
					}
					else Output::printWarning("Warning: Unknown attribute for \"address\": " + attributeName);
				}
			}
			else if(nodeName == "reset_after_send")
			{
				attr = physicalNode->first_attribute("param");
				if(attr) resetAfterSend.push_back(std::string(attr->value()));
			}
			else Output::printWarning("Warning: Unknown node for \"physical\": " + nodeName);
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

} /* namespace XMLRPC */
