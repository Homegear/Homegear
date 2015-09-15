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

#ifndef DEVICEPACKET_H_
#define DEVICEPACKET_H_

#include "JsonPayload.h"
#include "BinaryPayload.h"
#include "HttpPayload.h"
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

class Packet;

typedef std::shared_ptr<Packet> PPacket;
typedef std::multimap<uint32_t, PPacket> PacketsByMessageType;
typedef std::map<std::string, PPacket> PacketsById;
typedef std::multimap<std::string, PPacket> PacketsByFunction;
typedef std::map<int32_t, std::map<std::string, PPacket>> ValueRequestPackets;

class Packet
{
public:
	struct Direction
	{
		enum Enum { none = 0, toCentral = 1, fromCentral = 2 };
	};

	Packet(BaseLib::Obj* baseLib);
	Packet(BaseLib::Obj* baseLib, xml_node<>* node);
	virtual ~Packet() {}

	//Attributes
	std::string id;

	//Elements
	Direction::Enum direction = Direction::Enum::none;
	int32_t length = -1;
	int32_t type = -1;
	int32_t subtype = -1;
	int32_t subtypeIndex = -1;
	double subtypeSize = -1;
	std::string function1;
	std::string function2;
	std::string metaString1;
	std::string metaString2;
	int32_t responseType = -1;
	int32_t responseSubtype = -1;
	int32_t channel = -1;
	int32_t channelIndex = -1;
	double channelSize = 1;
	bool doubleSend = false;
	int32_t splitAfter = -1;
	int32_t maxPackets = -1;
	JsonPayloads jsonPayloads;
	BinaryPayloads binaryPayloads;
	HttpPayloads httpPayloads;

	//Helpers
	int32_t channelIndexOffset = 0;
	std::vector<PParameter> associatedVariables;
protected:
	BaseLib::Obj* _bl = nullptr;
};
}
}

#endif
