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

#include "Insteon-SD.h"
#include "../GD.h"

namespace Insteon
{
Insteon_SD::Insteon_SD(IDeviceEventSink* eventHandler) : InsteonDevice(eventHandler)
{
	init();
}

Insteon_SD::Insteon_SD(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : InsteonDevice(deviceID, serialNumber, address, eventHandler)
{
	init();
}

Insteon_SD::~Insteon_SD()
{
	dispose();
}

void Insteon_SD::init()
{
	try
	{
		InsteonDevice::init();

		_deviceType = (uint32_t)DeviceType::INSTEONSD;
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Insteon_SD::saveVariables()
{
	try
	{
		if(_deviceID == 0) return;
		InsteonDevice::saveVariables();
		saveVariable(1000, (int32_t)_enabled);
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Insteon_SD::loadVariables()
{
	try
	{
		InsteonDevice::loadVariables();
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetDeviceVariables();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 1000:
				_enabled = row->second.at(3)->intValue;
				break;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool Insteon_SD::onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!_enabled) return false;
		std::shared_ptr<InsteonPacket> insteonPacket(std::dynamic_pointer_cast<InsteonPacket>(packet));
		if(!insteonPacket) return false;
		std::cout << BaseLib::HelperFunctions::getTimeString(insteonPacket->timeReceived()) << " Insteon packet received: " + insteonPacket->hexString() << std::endl;
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return false;
}

std::string Insteon_SD::handleCLICommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;

		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "enable\t\t\tEnables the device if it was disabled" << std::endl;
			stringStream << "disable\t\t\tDisables the device" << std::endl;
			stringStream << "send\t\t\tSends a HomeMatic Wired packet" << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 6, "enable") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1)
				{
					index++;
					continue;
				}
				else if(index == 1)
				{
					if(element == "help") break;
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command enables the spy device if it was disabled." << std::endl;
				stringStream << "Usage: enable" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  There are no parameters." << std::endl;
				return stringStream.str();
			}

			if(!_enabled)
			{
				_enabled = true;
				stringStream << "Device is enabled now." << std::endl;
			}
			else stringStream << "Device already is enabled." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 7, "disable") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1)
				{
					index++;
					continue;
				}
				else if(index == 1)
				{
					if(element == "help") break;
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command disables processing of received packets." << std::endl;
				stringStream << "Usage: disable" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  There are no parameters." << std::endl;
				return stringStream.str();
			}

			if(_enabled)
			{
				_enabled = false;
				stringStream << "Device is disabled now." << std::endl;
			}
			else stringStream << "Device already is disabled." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 4, "send") == 0)
		{
			std::string packetHex;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1)
				{
					index++;
					continue;
				}
				else if(index == 1)
				{
					if(element == "help") break;
					packetHex = element;
				}
				index++;
			}
			if(index == 1)
			{
				stringStream << "Description: Sends an Insteon packet." << std::endl;
				stringStream << "Usage: send PACKET" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PACKET:\tThe packet to send" << std::endl;
				return stringStream.str();
			}

			std::shared_ptr<InsteonPacket> packet(new InsteonPacket(packetHex));
			sendPacket(GD::defaultPhysicalInterface, packet, false);
			stringStream << "Packet sent: " << packet->hexString() << std::endl;
			return stringStream.str();
		}
		else return "Unknown command.\n";
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}

}
