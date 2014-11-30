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

#include "COC.h"
#include "../BidCoSPacket.h"
#include "../../Base/BaseLib.h"
#include "../GD.h"

namespace BidCoS
{

COC::COC(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IBidCoSInterface(settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "COC \"" + settings->id + "\": ");

	stackPrefix = "";
	for(uint32_t i = 1; i < settings->stackPosition; i++)
	{
		stackPrefix.push_back('*');
	}
}

COC::~COC()
{
	try
	{
		if(_socket)
		{
			_socket->removeEventHandler(this);
			_socket->closeDevice();
			_socket.reset();
		}
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			_out.printWarning("Warning: Packet was nullptr.");
			return;
		}
		if(!_socket)
		{
			_out.printError("Error: Couldn't write to COC device, because the device descriptor is not valid: " + _settings->device);
			return;
		}
		if(packet->payload()->size() > 54)
		{
			if(_bl->debugLevel >= 2) _out.printError("Error: Tried to send packet larger than 64 bytes. That is not supported.");
			return;
		}
		if(_updateMode)
		{
			std::shared_ptr<BidCoSPacket> bidCoSPacket(std::dynamic_pointer_cast<BidCoSPacket>(packet));
			if(!bidCoSPacket) return;
			if(!bidCoSPacket->isUpdatePacket())
			{
				_out.printInfo("Info: Can't send packet to BidCoS peer with address 0x" + BaseLib::HelperFunctions::getHexString(packet->destinationAddress(), 6) + ", because update mode is enabled.");
				return;
			}
		}

		std::string packetHex = packet->hexString();
		if(_bl->debugLevel > 3) _out.printInfo("Info: Sending (" + _settings->id + "): " + packetHex);
		writeToDevice(stackPrefix + "As" + packetHex + "\n" + (_updateMode ? "" : stackPrefix + "Ar\n"));
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::enableUpdateMode()
{
	try
	{
		_updateMode = true;
		writeToDevice(stackPrefix + "AR\n");
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::disableUpdateMode()
{
	try
	{
		stopListening();
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		startListening();
		_updateMode = false;
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::writeToDevice(std::string data)
{
    try
    {
        if(!_socket)
		{
			_out.printError("Error: Couldn't write to COC device, because the device descriptor is not valid: " + _settings->device);
			return;
		}
        _socket->writeLine(data);
    }
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _lastPacketSent = BaseLib::HelperFunctions::getTime();
}

void COC::startListening()
{
	try
	{
		_socket = GD::bl->serialDeviceManager.get(_settings->device);
		if(!_socket) _socket = GD::bl->serialDeviceManager.create(_settings->device, 38400, O_RDWR | O_NOCTTY | O_NDELAY, true, 45);
		if(!_socket) return;
		_socket->addEventHandler(this);
		_socket->openDevice();
		if(gpioDefined(2))
		{
			openGPIO(2, false);
			if(!getGPIO(2)) setGPIO(2, true);
			closeGPIO(2);
		}
		if(gpioDefined(1))
		{
			openGPIO(1, false);
			if(!getGPIO(1))
			{
				setGPIO(1, false);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				setGPIO(1, true);
				std::this_thread::sleep_for(std::chrono::milliseconds(2000));
			}
			else std::this_thread::sleep_for(std::chrono::milliseconds(2000));
			closeGPIO(1);
		}
		writeToDevice(stackPrefix + "X21\n" + stackPrefix + "Ar\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		IPhysicalInterface::startListening();
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::stopListening()
{
	try
	{
		if(!_socket) return;
		_socket->removeEventHandler(this);
		_socket->closeDevice();
		_socket.reset();
		IPhysicalInterface::stopListening();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::lineReceived(const std::string& data)
{
    try
    {
    	std::string packetHex;
		if(stackPrefix.empty())
		{
			if(data.size() > 0 && data.at(0) == '*') return;
			packetHex = data;
		}
		else
		{
			if(data.size() + 1 <= stackPrefix.size()) return;
			if(data.substr(0, stackPrefix.size()) != stackPrefix || data.at(stackPrefix.size()) == '*') return;
			else packetHex = data.substr(stackPrefix.size());
		}
		if(packetHex.size() > 21) //21 is minimal packet length (=10 Byte + COC "A")
		{
			std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(packetHex, BaseLib::HelperFunctions::getTime()));
			raisePacketReceived(packet);
		}
		else if(!packetHex.empty())
		{
			if(packetHex == "LOVF") _out.printWarning("Warning: COC with id " + _settings->id + " reached 1% limit. You need to wait, before sending is allowed again.");
			else _out.printWarning("Warning: Too short packet received: " + packetHex);
		}
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void COC::setup(int32_t userID, int32_t groupID)
{
    try
    {
    	setDevicePermission(userID, groupID);
    	exportGPIO(1);
		setGPIOPermission(1, userID, groupID, false);
		setGPIODirection(1, GPIODirection::OUT);
		exportGPIO(2);
		setGPIOPermission(2, userID, groupID, false);
		setGPIODirection(2, GPIODirection::OUT);
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
