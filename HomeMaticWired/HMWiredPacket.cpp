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

#include "HMWiredPacket.h"
#include "../Exception.h"
#include "../HelperFunctions.h"

namespace HMWired
{

HMWiredPacket::HMWiredPacket()
{
}

HMWiredPacket::HMWiredPacket(std::vector<uint8_t>& packet, int64_t timeReceived)
{
	_timeReceived = timeReceived;
	import(packet);
}

HMWiredPacket::~HMWiredPacket()
{
}

void HMWiredPacket::import(std::vector<uint8_t>& packet)
{
	try
	{
		if(packet.size() < 11)
		{
			HelperFunctions::printError("HomeMatic Wired packet is smaller than 11 bytes: " + HelperFunctions::getHexString(packet));
			return;
		}
		if(packet[0] != 0xFD && packet[0] != 0xFE)
		{
			HelperFunctions::printError("HomeMatic Wired packet has invalid start byte: " + HelperFunctions::getHexString(packet));
			return;
		}
		_length = packet[10]; //Frame length
		if(_length > 64 || _length + 11 > packet.size())
		{
			HelperFunctions::printError("HomeMatic Wired packet has invalid length: " + HelperFunctions::getHexString(packet));
			return;
		}
		_controlByte = packet[5];
		_senderAddress = (packet[6] << 24) + (packet[7] << 16) + (packet[8] << 8) + packet[9];
		_destinationAddress = (packet[1] << 24) + (packet[2] << 16) + (packet[3] << 8) + packet[4];
		if(_length >= 2)
		{
			_payload.clear();
			_payload.insert(_payload.end(), packet.begin() + 11, packet.end() - 2);
			_checksum.clear();
			_checksum.insert(_checksum.end(), packet.end() - 2, packet.end());
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredPacket::import(std::string packetHex)
{
	try
	{

	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

} /* namespace HMWired */
