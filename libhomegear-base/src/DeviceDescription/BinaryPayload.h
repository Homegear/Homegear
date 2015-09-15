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

#ifndef BINARYPAYLOAD_H_
#define BINARYPAYLOAD_H_

#include "../Encoding/RapidXml/rapidxml.hpp"
#include <string>
#include <vector>
#include <memory>

using namespace rapidxml;

namespace BaseLib
{

class Obj;

namespace DeviceDescription
{

class BinaryPayload;

typedef std::shared_ptr<BinaryPayload> PBinaryPayload;
typedef std::vector<PBinaryPayload> BinaryPayloads;

class BinaryPayload
{
public:
	BinaryPayload(BaseLib::Obj* baseLib);
	BinaryPayload(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~BinaryPayload() {}

	double index = 0;
	double size = 1.0;
	double index2 = 0;
	double size2 = 0;
	int32_t index2Offset = -1;
	int32_t constValueInteger = -1;
	double constValueDecimal = -1;
	std::string constValueString;
	bool isSigned = false;
	bool omitIfSet = false;
	int32_t omitIf = 0;
	std::string parameterId;
protected:
	BaseLib::Obj* _bl = nullptr;
};
}
}

#endif
