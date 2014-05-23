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

#include "HM-SD.h"
#include "../../Base/BaseLib.h"
#include "../GD.h"

namespace BidCoS
{
HM_SD::HM_SD(IDeviceEventSink* eventHandler) : HomeMaticDevice(eventHandler)
{
	init();
}

HM_SD::HM_SD(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : HomeMaticDevice(deviceID, serialNumber, address, eventHandler)
{
	init();
}

HM_SD::~HM_SD()
{
	dispose();
}

void HM_SD::init()
{
	try
	{
		HomeMaticDevice::init();

		_deviceType = (uint32_t)DeviceType::HMSD;
	}
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_SD::saveVariables()
{
	try
	{
		if(_deviceID == 0) return;
		HomeMaticDevice::saveVariables();
		saveVariable(1000, (int32_t)_enabled);
		saveFilters(); //1001
		saveResponsesToOverwrite(); //1002
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_SD::loadVariables()
{
	try
	{
		HomeMaticDevice::loadVariables();
		BaseLib::Database::DataTable rows = raiseGetDeviceVariables();
		for(BaseLib::Database::DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
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
			case 1002:
				unserializeResponsesToOverwrite(row->second.at(5)->binaryValue);
				break;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_SD::saveFilters()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializeFilters(serializedData);
		saveVariable(1001, serializedData);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_SD::saveResponsesToOverwrite()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializeResponsesToOverwrite(serializedData);
		saveVariable(1002, serializedData);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_SD::serializeFilters(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder;
		encoder.encodeInteger(encodedData, _filters.size());
		for(std::list<HM_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
		{
			encoder.encodeByte(encodedData, (int32_t)i->filterType);
			encoder.encodeInteger(encodedData, i->filterValue);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_SD::unserializeFilters(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BaseLib::BinaryDecoder decoder;
		uint32_t position = 0;
		uint32_t filtersSize = decoder.decodeInteger(serializedData, position);
		for(uint32_t i = 0; i < filtersSize; i++)
		{
			HM_SD_Filter filter;
			filter.filterType = (FilterType)decoder.decodeByte(serializedData, position);
			filter.filterValue = decoder.decodeInteger(serializedData, position);
			_filters.push_back(filter);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_SD::serializeResponsesToOverwrite(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder;
		encoder.encodeInteger(encodedData, _responsesToOverwrite.size());
		for(std::list<HM_SD_OverwriteResponse>::iterator i = _responsesToOverwrite.begin(); i != _responsesToOverwrite.end(); ++i)
		{
			encoder.encodeString(encodedData, i->packetPartToCapture);
			encoder.encodeString(encodedData, i->response);
			encoder.encodeInteger(encodedData, i->sendAfter);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_SD::unserializeResponsesToOverwrite(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BaseLib::BinaryDecoder decoder;
		uint32_t position = 0;
		uint32_t responsesToOverwriteSize = decoder.decodeInteger(serializedData, position);
		for(uint32_t i = 0; i < responsesToOverwriteSize; i++)
		{
			HM_SD_OverwriteResponse responseToOverwrite;
			responseToOverwrite.packetPartToCapture = decoder.decodeString(serializedData, position);
			responseToOverwrite.response = decoder.decodeString(serializedData, position);
			responseToOverwrite.sendAfter = decoder.decodeInteger(serializedData, position);
			_responsesToOverwrite.push_back(responseToOverwrite);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool HM_SD::onPacketReceived(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!_enabled) return false;
		std::shared_ptr<BidCoSPacket> bidCoSPacket(std::dynamic_pointer_cast<BidCoSPacket>(packet));
		if(!bidCoSPacket) return false;
		bool printPacket = false;
		for(std::list<HM_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
		{
			switch(i->filterType)
			{
				case FilterType::SenderAddress:
					if(bidCoSPacket->senderAddress() == i->filterValue) printPacket = true;
					break;
				case FilterType::DestinationAddress:
					if(bidCoSPacket->destinationAddress() == i->filterValue) printPacket = true;
					break;
				case FilterType::DeviceType:
					//Only possible for paired devices
					break;
				case FilterType::MessageType:
					if(bidCoSPacket->messageType() == i->filterValue) printPacket = true;
					break;
			}
		}
		if(_filters.size() == 0) printPacket = true;
		if(_hack)
		{
			int32_t addressMotionDetector = 0x1A4EFE;
			int32_t addressKeyMatic = 0x1F454D;
			int32_t addressRemote = 0x24C0EE;
			int32_t addressCentral = 0x1C6940;

			if(bidCoSPacket->senderAddress() == addressMotionDetector)
			{
				if(bidCoSPacket->messageType() == 0x10)
				{
					_messageCounter[1]++;
					std::vector<uint8_t> payload;
					payload.push_back(0x03);
					payload.push_back(_messageCounter[1]);
					std::shared_ptr<BidCoSPacket> packet1(new BidCoSPacket(bidCoSPacket->messageCounter(), 0xA4, 0x40, addressRemote, addressKeyMatic, payload));
					packet1->setTimeSending(BaseLib::HelperFunctions::getTime());
					GD::physicalDevice->sendPacket(packet1);
				}
				else if(bidCoSPacket->messageType() == 0x03 && bidCoSPacket->controlByte() == 0xA0)
				{
					std::vector<uint8_t> payload = *bidCoSPacket->payload();
					std::shared_ptr<BidCoSPacket> packet3(new BidCoSPacket(bidCoSPacket->messageCounter(), 0xA0, 0x03, addressRemote, addressKeyMatic, payload));
					packet3->setTimeSending(BaseLib::HelperFunctions::getTime());
					GD::physicalDevice->sendPacket(packet3);
				}
			}
			else if(bidCoSPacket->senderAddress() == addressKeyMatic)
			{
				if(bidCoSPacket->messageType() == 0x02 && bidCoSPacket->controlByte() == 0xA0)
				{
					std::vector<uint8_t> payload = *bidCoSPacket->payload();
					std::shared_ptr<BidCoSPacket> packet2(new BidCoSPacket(bidCoSPacket->messageCounter(), 0xA0, 0x02, addressCentral, addressMotionDetector, payload));
					packet2->setTimeSending(BaseLib::HelperFunctions::getTime());
					GD::physicalDevice->sendPacket(packet2);
				}
				else if(bidCoSPacket->messageType() == 0x02 && bidCoSPacket->controlByte() == 0x80 && bidCoSPacket->payload()->size() == 5)
				{
					std::vector<uint8_t> payload = *bidCoSPacket->payload();
					std::shared_ptr<BidCoSPacket> packet4(new BidCoSPacket(bidCoSPacket->messageCounter(), 0x80, 0x02, addressCentral, addressMotionDetector, payload));
					packet4->setTimeSending(BaseLib::HelperFunctions::getTime());
					GD::physicalDevice->sendPacket(packet4);
				}
			}
		}
		if(printPacket) std::cout << BaseLib::HelperFunctions::getTimeString(bidCoSPacket->timeReceived()) << " HomeMatic BidCoS packet received: " + bidCoSPacket->hexString() << std::endl;
		for(std::list<HM_SD_OverwriteResponse>::const_iterator i = _responsesToOverwrite.begin(); i != _responsesToOverwrite.end(); ++i)
		{
			std::string packetHex = bidCoSPacket->hexString();
			if(packetHex.find(i->packetPartToCapture) != std::string::npos)
			{
				std::chrono::milliseconds sleepingTime(i->sendAfter); //Don't respond too fast
				std::this_thread::sleep_for(sleepingTime);
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket());
				std::stringstream stringstream;
				stringstream << std::hex << std::setfill('0') << std::setw(2) << (i->response.size() / 2) + 1;
				std::string lengthHex = stringstream.str();
				std::string packetString(lengthHex + packetHex.substr(2, 2) + i->response);
				packet->import(packetString, false);
				std::chrono::time_point<std::chrono::system_clock> timepoint = std::chrono::system_clock::now();
				BaseLib::Output::printMessage("Captured: " + packetHex + " Responding with: " + packet->hexString());
				GD::physicalDevice->sendPacket(packet);
			}
		}
	}
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return false;
}

void HM_SD::addFilter(FilterType filterType, int32_t filterValue)
{
	try
	{
		HM_SD_Filter filter;
		filter.filterType = filterType;
		filter.filterValue = filterValue;
		_filters.push_back(filter);
	}
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_SD::removeFilter(FilterType filterType, int32_t filterValue)
{
	try
	{
		_filters.remove_if([&](HM_SD_Filter filter){ return filter.filterType == filterType && filter.filterValue == filterValue; });
	}
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_SD::addOverwriteResponse(std::string packetPartToCapture, std::string response, int32_t sendAfter)
{
	try
	{
		HM_SD_OverwriteResponse overwriteResponse;
		overwriteResponse.sendAfter = sendAfter;
		overwriteResponse.packetPartToCapture = packetPartToCapture;
		overwriteResponse.response = response;
		_responsesToOverwrite.push_back(overwriteResponse);
	}
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_SD::removeOverwriteResponse(std::string packetPartToCapture)
{
	try
	{
		_responsesToOverwrite.remove_if([&](HM_SD_OverwriteResponse entry){ return entry.packetPartToCapture == packetPartToCapture; });
	}
    catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string HM_SD::handleCLICommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;

		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "enable\t\t\tEnables the device if it was disabled" << std::endl;
			stringStream << "disable\t\t\tDisables the device" << std::endl;
			stringStream << "filters list\t\tLists all packet filters" << std::endl;
			stringStream << "filters add\t\tAdds a packet filter" << std::endl;
			stringStream << "filters remove\t\tRemoves a packet filter" << std::endl;
			stringStream << "captures list\t\tLists all packet captures" << std::endl;
			stringStream << "captures add\t\tAdds a packet to capture" << std::endl;
			stringStream << "captures remove\t\tRemoves a packet to capture" << std::endl;
			stringStream << "send\t\t\tSends a BidCoS packet" << std::endl;
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
						stringStream << "Description: This command lists all BidCoS packet filters." << std::endl;
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
			for(std::list<HM_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
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
			int32_t filterType;
			int32_t filterValue;

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
					filterType = BaseLib::HelperFunctions::getNumber(element, true);
					if(filterType != 0 && filterType != 1 && filterType != 3) return "Invalid filter type.\n";
				}
				else if(index == 3) filterValue = BaseLib::HelperFunctions::getNumber(element, true);
				index++;
			}
			if(index < 4)
			{
				stringStream << "Description: This command adds a BidCoS packet filter. All filters are linked by \"or\". Currently there is no option to connect the filters by \"and\"" << std::endl;
				stringStream << "Usage: filters add FILTERTYPE FILTERVALUE" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  FILTERTYPE:\tSee filter types below." << std::endl;
				stringStream << "  FILTERVALUE:\tDepends on the filter type. The parameter needs to be in hexadecimal format." << std::endl << std::endl;
				stringStream << "Filter types:" << std::endl;
				stringStream << "  00: Filter by sender address." << std::endl;
				stringStream << "      FILTERVALUE: The 3 byte address of the peer to filter." << std::endl;
				stringStream << "  01: Filter by destination address." << std::endl;
				stringStream << "      FILTERVALUE: The 3 byte address of the peer to filter." << std::endl;
				stringStream << "  03: Filter by message type." << std::endl;
				stringStream << "      FILTERVALUE: The 1 byte message type of the packet to filter." << std::endl;
				return stringStream.str();
			}

			addFilter((FilterType)filterType, filterValue);
			stringStream << "Filter added." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 14, "filters remove") == 0)
		{
			int32_t filterType;
			int32_t filterValue;

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
					filterType = BaseLib::HelperFunctions::getNumber(element, true);
					if(filterType != 0 && filterType != 1 && filterType != 3) return "Invalid filter type.\n";
				}
				else if(index == 3) filterValue = BaseLib::HelperFunctions::getNumber(element, true);
				index++;
			}
			if(index < 4)
			{
				stringStream << "Description: This command removes a BidCoS packet filter." << std::endl;
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
		else if(command.compare(0, 13, "captures list") == 0)
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
						stringStream << "Description: This command lists all BidCoS packet captures." << std::endl;
						stringStream << "Usage: captures list" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			if(_responsesToOverwrite.empty())
			{
				stringStream << "No captures defined." << std::endl;
				stringStream.str();
			}
			for(std::list<HM_SD_OverwriteResponse>::const_iterator i = _responsesToOverwrite.begin(); i != _responsesToOverwrite.end(); ++i)
			{
				stringStream << "Packet part to capture: " << std::hex << i->packetPartToCapture << "\tResponse: " << i->response << std::dec << "\tSend after: " << i->sendAfter << " ms" << std::endl;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 12, "captures add") == 0)
		{
			std::string toCapture;
			std::string response;
			int32_t sendAfter;

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
					toCapture = element;
				}
				else if(index == 3) response = element;
				else if(index == 4)
				{
					sendAfter = BaseLib::HelperFunctions::getNumber(element);
					if(sendAfter < 5) stringStream << "Invalid value for SENDAFTER. SENDAFTER needs to be longer than 5 ms." << std::endl;
				}
				index++;
			}
			if(index < 5)
			{
				stringStream << "Description: Sends a custom response after receiving the specified packet." << std::endl;
				stringStream << "Usage: captures add CAPTURE RESPONSE SENDAFTER" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  CAPTURE:\tAny part of the packet to capture in hexadecimal format." << std::endl;
				stringStream << "  RESPONSE:\tThe packet to send as a response in hexadecimal format without length byte and message counter." << std::endl;
				stringStream << "  SENDAFTER:\tThe time in milliseconds to wait before sending the response (typically 80 to 120 ms)." << std::endl;
				return stringStream.str();
			}

			addOverwriteResponse(toCapture, response, sendAfter);
			stringStream << "Capture added." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 15, "captures remove") == 0)
		{
			std::string toCapture;

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
					toCapture = element;
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: Removes a packet capture." << std::endl;
				stringStream << "Usage: captures remove CAPTURE" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  CAPTURE:\tThe part of the packet to capture which should be removed from the list of captures." << std::endl;
				return stringStream.str();
			}


			uint32_t oldSize = _responsesToOverwrite.size();
			removeOverwriteResponse(toCapture);
			if(_responsesToOverwrite.size() != oldSize) stringStream << "Capture removed." << std::endl;
			else stringStream << "Capture not found." << std::endl;
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
				stringStream << "Description: Sends a BidCoS packet." << std::endl;
				stringStream << "Usage: send PACKET" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PACKET:\tThe packet to send with length byte and message counter." << std::endl;
				return stringStream.str();
			}

			if(packetHex.size() < 18) return "Packet is too short. Please provide at least 9 bytes.\n";
			std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket());
			packet->import(packetHex, false);
			sendPacket(packet);
			stringStream << "Packet sent: " << packet->hexString() << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 4, "hack") == 0)
		{
			if(_hack)
			{
				_hack = false;
				return "Hack disabled.\n";
			}
			else
			{
				_hack = true;
				return "Hack enabled.\n";
			}
		}
		else return "Unknown command.\n";
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}
}
