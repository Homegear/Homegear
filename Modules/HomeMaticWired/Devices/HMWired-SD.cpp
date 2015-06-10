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

#include "HMWired-SD.h"
#include "../GD.h"

namespace HMWired
{
HMWired_SD::HMWired_SD(IDeviceEventSink* eventHandler) : HMWiredDevice(eventHandler)
{
	init();
}

HMWired_SD::HMWired_SD(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : HMWiredDevice(deviceID, serialNumber, address, eventHandler)
{
	init();
}

HMWired_SD::~HMWired_SD()
{
	dispose();
}

void HMWired_SD::init()
{
	try
	{
		HMWiredDevice::init();

		_deviceType = (uint32_t)DeviceType::HMWIREDSD;
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

void HMWired_SD::saveVariables()
{
	try
	{
		if(_deviceID == 0) return;
		HMWiredDevice::saveVariables();
		saveVariable(1000, (int32_t)_enabled);
		saveFilters(); //1001
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

void HMWired_SD::loadVariables()
{
	try
	{
		HMWiredDevice::loadVariables();
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetDeviceVariables();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 1000:
				_enabled = row->second.at(3)->intValue;
				break;
			case 1001:
				unserializeFilters(row->second.at(5)->binaryValue);
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

void HMWired_SD::saveFilters()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializeFilters(serializedData);
		saveVariable(1001, serializedData);
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

void HMWired_SD::serializeFilters(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder(_bl);
		encoder.encodeInteger(encodedData, _filters.size());
		for(std::list<HMWired_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
		{
			encoder.encodeByte(encodedData, (int32_t)i->filterType);
			encoder.encodeInteger(encodedData, i->filterValue);
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

void HMWired_SD::unserializeFilters(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BaseLib::BinaryDecoder decoder(_bl);
		uint32_t position = 0;
		uint32_t filtersSize = decoder.decodeInteger(*serializedData, position);
		for(uint32_t i = 0; i < filtersSize; i++)
		{
			HMWired_SD_Filter filter;
			filter.filterType = (FilterType)decoder.decodeByte(*serializedData, position);
			filter.filterValue = decoder.decodeInteger(*serializedData, position);
			_filters.push_back(filter);
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

bool HMWired_SD::onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!_enabled) return false;
		std::shared_ptr<HMWiredPacket> wiredPacket(std::dynamic_pointer_cast<HMWiredPacket>(packet));
		if(!wiredPacket) return false;
		bool printPacket = false;
		for(std::list<HMWired_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
		{
			switch(i->filterType)
			{
				case FilterType::SenderAddress:
					if(wiredPacket->senderAddress() == i->filterValue) printPacket = true;
					break;
				case FilterType::DestinationAddress:
					if(wiredPacket->destinationAddress() == i->filterValue) printPacket = true;
					break;
			}
		}
		if(_filters.size() == 0) printPacket = true;
		if(printPacket) std::cout << BaseLib::HelperFunctions::getTimeString(wiredPacket->timeReceived()) << " HomeMatic Wired packet received: " + wiredPacket->hexString() << std::endl;
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

void HMWired_SD::addFilter(FilterType filterType, int32_t filterValue)
{
	try
	{
		HMWired_SD_Filter filter;
		filter.filterType = filterType;
		filter.filterValue = filterValue;
		_filters.push_back(filter);
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

void HMWired_SD::removeFilter(FilterType filterType, int32_t filterValue)
{
	try
	{
		_filters.remove_if([&](HMWired_SD_Filter filter){ return filter.filterType == filterType && filter.filterValue == filterValue; });
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

std::string HMWired_SD::handleCLICommand(std::string command)
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
			stringStream << "filters list\t\tLists all packet filters" << std::endl;
			stringStream << "filters add\t\tAdds a packet filter" << std::endl;
			stringStream << "filters remove\t\tRemoves a packet filter" << std::endl;
			stringStream << "send\t\t\tSends a HomeMatic Wired packet" << std::endl;
			stringStream << "sendraw\t\t\tSends arbitrary hex encoded data" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 12, "filters list") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command lists all HomeMatic Wired packet filters." << std::endl;
						stringStream << "Usage: filters list" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			if(_filters.empty())
			{
				stringStream << "No filters defined." << std::endl;
				stringStream.str();
			}
			for(std::list<HMWired_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
			{
				stringStream << "Filter type: " << std::hex << (int32_t)i->filterType << "\tFilter value: " << i->filterValue << std::dec << std::endl;
			}
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
		else if(command.compare(0, 11, "filters add") == 0)
		{
			int32_t filterType = -1;
			int32_t filterValue = -1;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					filterType = BaseLib::Math::getNumber(element, true);
					if(filterType != 0 && filterType != 1 && filterType != 3) return "Invalid filter type.\n";
				}
				else if(index == 3) filterValue = BaseLib::Math::getNumber(element, true);
				index++;
			}
			if(index < 4)
			{
				stringStream << "Description: This command adds a HomeMatic Wired packet filter. All filters are linked by \"or\". Currently there is no option to connect the filters by \"and\"" << std::endl;
				stringStream << "Usage: filters add FILTERTYPE FILTERVALUE" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  FILTERTYPE:\tSee filter types below." << std::endl;
				stringStream << "  FILTERVALUE:\tDepends on the filter type. The parameter needs to be in hexadecimal format." << std::endl << std::endl;
				stringStream << "Filter types:" << std::endl;
				stringStream << "  00: Filter by sender address." << std::endl;
				stringStream << "      FILTERVALUE: The 4 byte address of the peer to filter." << std::endl;
				stringStream << "  01: Filter by destination address." << std::endl;
				stringStream << "      FILTERVALUE: The 4 byte address of the peer to filter." << std::endl;
				return stringStream.str();
			}

			addFilter((FilterType)filterType, filterValue);
			stringStream << "Filter added." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 14, "filters remove") == 0)
		{
			int32_t filterType = -1;
			int32_t filterValue = -1;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					filterType = BaseLib::Math::getNumber(element, true);
					if(filterType != 0 && filterType != 1 && filterType != 3) return "Invalid filter type.\n";
				}
				else if(index == 3) filterValue = BaseLib::Math::getNumber(element, true);
				index++;
			}
			if(index < 4)
			{
				stringStream << "Description: This command removes a HomeMatic Wired packet filter." << std::endl;
				stringStream << "Usage: filters remove FILTERTYPE FILTERVALUE" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  FILTERTYPE:\tThe type of the filter to delete." << std::endl;
				stringStream << "  FILTERVALUE:\tThe value of the filter to delete." << std::endl;
				return stringStream.str();
			}

			uint32_t oldSize = _filters.size();
			removeFilter((FilterType)filterType, filterValue);
			if(_filters.size() != oldSize) stringStream << "Filter removed." << std::endl;
			else stringStream << "Filter not found." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 7, "sendraw") == 0)
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
				stringStream << "Description: Sends raw binary data." << std::endl;
				stringStream << "Usage: send PACKET" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PACKET:\tThe hex encoded packet to send" << std::endl;
				return stringStream.str();
			}

			std::vector<uint8_t> packet = BaseLib::HelperFunctions::hexToBin(packetHex);
			GD::physicalInterface->sendPacket(packet);
			stringStream << "Packet sent: " << BaseLib::HelperFunctions::getHexString(packet) << std::endl;
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
				stringStream << "Description: Sends a HomeMatic Wired packet." << std::endl;
				stringStream << "Usage: send PACKET" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PACKET:\tThe packet to send" << std::endl;
				return stringStream.str();
			}

			std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(packetHex));
			sendPacket(packet, false);
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
