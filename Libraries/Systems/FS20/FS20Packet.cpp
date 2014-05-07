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

#include "FS20Packet.h"
#include "../../GD/GD.h"

namespace FS20
{
FS20Packet::FS20Packet()
{
}

FS20Packet::FS20Packet(std::string packet, int64_t timeReceived)
{
	_timeReceived = timeReceived;
	import(packet);
}

FS20Packet::FS20Packet(std::vector<uint8_t>& packet, int64_t timeReceived)
{
	_timeReceived = timeReceived;
	import(packet);
}

FS20Packet::~FS20Packet()
{
}

void FS20Packet::import(std::vector<uint8_t>& packet)
{
	try
	{
		if(packet.empty()) return;
		if(packet.size() != 4 && packet.size() != 5)
		{
			GD::output->printWarning("Warning: Tried to import FS20 packet of wrong size (" + std::to_string(packet.size()) + " bytes).");
			return;
		}
		_packet = packet;
		_houseCode = (packet.at(0) << 8) + packet.at(1);
		_address = packet.at(2);
		_command = packet.at(3);
		if(packet.size() == 5)
		{
			_rssiDevice = packet.at(4);
			_packet.pop_back();
		}
	}
	catch(const std::exception& ex)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void FS20Packet::import(std::string packetHex, bool removeFirstCharacter)
{
	try
	{
		if(packetHex.size() > 256 || packetHex.size() < 9)
		{
			GD::output->printWarning("Warning: Tried to import FS20 packet larger than 128 bytes.");
			return;
		}
		int32_t tailLength = 0;
		if(packetHex.back() == '\n') tailLength = 2;
		if(removeFirstCharacter) packetHex = packetHex.substr(1, packetHex.length() - 1 - tailLength);
		else if(tailLength > 0) packetHex = packetHex.substr(0, packetHex.length() - tailLength);
		std::vector<uint8_t> packet(GD::helperFunctions->getUBinary(packetHex));
		import(packet);
	}
	catch(const std::exception& ex)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::vector<uint8_t> FS20Packet::byteArray()
{
	try
	{
		if(!_packet.empty()) return _packet;
		_packet.push_back(_houseCode >> 8);
		_packet.push_back(_houseCode & 0xFF);
		_packet.push_back(_address);
		_packet.push_back(_command);
	}
	catch(const std::exception& ex)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::vector<uint8_t>();
}

std::string FS20Packet::hexString()
{
	try
	{
		return GD::helperFunctions->getHexString(byteArray());
	}
	catch(const std::exception& ex)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "";
}

} /* namespace FS20 */
