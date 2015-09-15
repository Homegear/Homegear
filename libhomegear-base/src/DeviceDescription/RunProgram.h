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

#ifndef RUNPROGRAM_H_
#define RUNPROGRAM_H_

#include "JsonPayload.h"
#include "BinaryPayload.h"
#include "Parameter.h"
#include "../Encoding/RapidXml/rapidxml.hpp"
#include <string>
#include <memory>
#include <vector>
#include <map>

using namespace rapidxml;

namespace BaseLib
{

class Obj;

namespace DeviceDescription
{

class RunProgram;

typedef std::shared_ptr<RunProgram> PRunProgram;

class RunProgram
{
public:
	struct StartType
	{
		enum Enum { none, once, interval, permanent };
	};

	RunProgram(BaseLib::Obj* baseLib);
	RunProgram(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~RunProgram() {}

	//Elements
	std::string path;
	std::vector<std::string> arguments;
	StartType::Enum startType = StartType::none;
	uint32_t interval = 0;

protected:
	BaseLib::Obj* _bl = nullptr;
};
}
}

#endif
