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

#include "RunProgram.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace DeviceDescription
{

RunProgram::RunProgram(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

RunProgram::RunProgram(BaseLib::Obj* baseLib, xml_node<>* node) : RunProgram(baseLib)
{
	for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
	{
		_bl->out.printWarning("Warning: Unknown attribute for \"runProgram\": " + std::string(attr->name()));
	}
	for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
	{
		std::string nodeName(subNode->name());
		std::string value(subNode->value());
		if(nodeName == "path") path = value;
		else if(nodeName == "arguments")
		{
			for(xml_node<>* argumentNode = subNode->first_node(); argumentNode; argumentNode = argumentNode->next_sibling())
			{
				if(std::string(argumentNode->name()) == "argument") arguments.push_back(std::string(argumentNode->value()));
				else _bl->out.printWarning("Warning: Unknown node for \"runProgram\\arguments\": " + std::string(argumentNode->name()));
			}
		}
		else if(nodeName == "startType")
		{
			if(value == "once") startType = StartType::Enum::once;
			else if(value == "interval") startType = StartType::Enum::interval;
			else if(value == "permanent") startType = StartType::Enum::permanent;
			else _bl->out.printWarning("Warning: Unknown value for runProgram\\startType: " + value);
		}
		else if(nodeName == "interval") interval = Math::getUnsignedNumber(value);
		else _bl->out.printWarning("Warning: Unknown node in \"runProgram\": " + nodeName);
	}
}

}
}
