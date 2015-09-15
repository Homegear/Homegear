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

#include "DevicePacket.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace DeviceDescription
{

Packet::Packet(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

Packet::Packet(BaseLib::Obj* baseLib, xml_node<>* node) : Packet(baseLib)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		std::string attributeName(attr->name());
		std::string attributeValue(attr->value());
		if(attributeName == "id")
		{
			id = attributeValue;
		}
		else _bl->out.printWarning("Warning: Unknown attribute for \"packet\": " + attributeName);
	}
	for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
	{
		std::string nodeName(subNode->name());
		std::string value(subNode->value());
		if(nodeName == "direction")
		{
			if(value == "toCentral") direction = Direction::Enum::toCentral;
			else if(value == "fromCentral") direction = Direction::Enum::fromCentral;
			else _bl->out.printWarning("Warning: Unknown value for packet\\direction: " + value);
		}
		else if(nodeName == "length") length = Math::getNumber(value);
		else if(nodeName == "type") type = Math::getNumber(value);
		else if(nodeName == "subtype") subtype = Math::getNumber(value);
		else if(nodeName == "subtypeIndex")
		{
			std::pair<std::string, std::string> splitValue = HelperFunctions::split(value, ':');
			if(!splitValue.first.empty()) subtypeIndex = Math::getUnsignedNumber(splitValue.first);
			if(!splitValue.second.empty()) subtypeSize = Math::getDouble(splitValue.second);
		}
		else if(nodeName == "function1") function1 = value;
		else if(nodeName == "function2") function2 = value;
		else if(nodeName == "metaString1") metaString1 = value;
		else if(nodeName == "metaString2") metaString2 = value;
		else if(nodeName == "responseType") responseType = Math::getNumber(value);
		else if(nodeName == "responseSubtype") responseSubtype = Math::getNumber(value);
		else if(nodeName == "channel") channel = Math::getNumber(value);
		else if(nodeName == "channelIndex")
		{
			std::pair<std::string, std::string> splitValue = HelperFunctions::split(value, ':');
			if(!splitValue.first.empty()) channelIndex = Math::getUnsignedNumber(splitValue.first);
			if(!splitValue.second.empty()) channelSize = Math::getDouble(splitValue.second);
		}
		else if(nodeName == "doubleSend") { if(value == "true") doubleSend = true; }
		else if(nodeName == "splitAfter") splitAfter = Math::getNumber(value);
		else if(nodeName == "maxPackets") maxPackets = Math::getNumber(value);
		else if(nodeName == "binaryPayload")
		{
			for(xml_node<>* payloadNode = subNode->first_node("element"); payloadNode; payloadNode = payloadNode->next_sibling("element"))
			{
				PBinaryPayload payload(new BinaryPayload(_bl, payloadNode));
				binaryPayloads.push_back(payload);
			}
		}
		else if(nodeName == "jsonPayload")
		{
			for(xml_node<>* payloadNode = subNode->first_node("element"); payloadNode; payloadNode = payloadNode->next_sibling("element"))
			{
				PJsonPayload payload(new JsonPayload(_bl, payloadNode));
				jsonPayloads.push_back(payload);
			}
		}
		else if(nodeName == "httpPayload")
		{
			for(xml_node<>* payloadNode = subNode->first_node("element"); payloadNode; payloadNode = payloadNode->next_sibling("element"))
			{
				PHttpPayload payload(new HttpPayload(_bl, payloadNode));
				httpPayloads.push_back(payload);
			}
		}
		else _bl->out.printWarning("Warning: Unknown node in \"packet\": " + nodeName);
	}
}

}
}
