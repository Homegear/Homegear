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

#include "Physical.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace DeviceDescription
{

IPhysical::IPhysical(BaseLib::Obj* baseLib, Type::Enum type)
{
	_bl = baseLib;
	this->type = type;
}

IPhysical::IPhysical(BaseLib::Obj* baseLib, Type::Enum type, xml_node<>* node) : IPhysical(baseLib, type)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			if(attributeName == "groupId") groupId = std::string(attr->value());
			else _bl->out.printWarning("Warning: Unknown attribute for \"parameter\\physical*\": " + std::string(attr->name()));
		}
		for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
		{
			std::string nodeName(subNode->name());
			std::string nodeValue(subNode->value());
			if(nodeName == "operationType")
			{
				if(nodeValue == "command") operationType = OperationType::Enum::command;
				else if(nodeValue == "centralCommand") operationType = OperationType::Enum::centralCommand;
				else if(nodeValue == "internal") operationType = OperationType::Enum::internal;
				else if(nodeValue == "config") operationType = OperationType::Enum::config;
				else if(nodeValue == "configString") operationType = OperationType::Enum::configString;
				else if(nodeValue == "store") operationType = OperationType::Enum::store;
				else if(nodeValue == "memory") operationType = OperationType::Enum::memory;
				else baseLib->out.printWarning("Warning: Unknown interface for \"parameter\\physical*\": " + nodeValue);
			}
			else if(nodeName == "endianess")
			{
				if(nodeValue == "little") endianess = Endianess::Enum::little;
				else if(nodeValue == "big") endianess = Endianess::Enum::big;
				else baseLib->out.printWarning("Warning: Unknown endianess for \"parameter\\physical*\": " + nodeValue);
			}
			else if(nodeName == "list") list = Math::getNumber(nodeValue);
			else if(nodeName == "index") index = Math::getDouble(nodeValue);
			else if(nodeName == "size")
			{
				size = Math::getDouble(nodeValue);
				sizeDefined = true;
			}
			else if(nodeName == "mask") mask = Math::getNumber(nodeValue);
			else if(nodeName == "memoryIndex") memoryIndex = Math::getDouble(nodeValue);
			else if(nodeName == "memoryIndexOperation")
			{
				if(nodeValue == "none") memoryIndexOperation = MemoryIndexOperation::Enum::none;
				else if(nodeValue == "addition") memoryIndexOperation = MemoryIndexOperation::Enum::addition;
				else if(nodeValue == "subtraction") memoryIndexOperation = MemoryIndexOperation::Enum::subtraction;
				else baseLib->out.printWarning("Warning: Unknown memoryIndexOperation for \"parameter\\physical*\": " + nodeValue);
			}
			else if(nodeName == "memoryChannelStep") memoryChannelStep = Math::getDouble(nodeValue);
			else baseLib->out.printWarning("Warning: Unknown node in \"parameter\\physical*\": " + nodeName);
		}
		if(mask != -1 && fmod(index, 1) != 0) baseLib->out.printWarning("Warning: mask combined with unaligned index not supported.");
		startIndex = std::lround(std::floor(index));
		int32_t intDiff = std::lround(std::floor(size)) - 1;
		if(intDiff < 0) intDiff = 0;
		endIndex = startIndex + intDiff;
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

PhysicalInteger::PhysicalInteger(BaseLib::Obj* baseLib) : IPhysical(baseLib, Type::Enum::tInteger)
{

}

PhysicalInteger::PhysicalInteger(BaseLib::Obj* baseLib, xml_node<>* node) : IPhysical(baseLib, Type::Enum::tInteger, node)
{

}

PhysicalBoolean::PhysicalBoolean(BaseLib::Obj* baseLib) : IPhysical(baseLib, Type::Enum::tBoolean)
{

}

PhysicalBoolean::PhysicalBoolean(BaseLib::Obj* baseLib, xml_node<>* node) : IPhysical(baseLib, Type::Enum::tBoolean, node)
{

}

PhysicalString::PhysicalString(BaseLib::Obj* baseLib) : IPhysical(baseLib, Type::Enum::tString)
{

}

PhysicalString::PhysicalString(BaseLib::Obj* baseLib, xml_node<>* node) : IPhysical(baseLib, Type::Enum::tString, node)
{

}

}
}
