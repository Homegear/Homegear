#include "HM-SD.h"
#include "../GD.h"
#include "../HelperFunctions.h"

HM_SD::HM_SD() : HomeMaticDevice()
{
	init();
}

HM_SD::HM_SD(std::string serialNumber, int32_t address) : HomeMaticDevice(serialNumber, address)
{
	init();
}

HM_SD::~HM_SD()
{
}

void HM_SD::init()
{
	HomeMaticDevice::init();

	_deviceType = HMDeviceTypes::HMSD;
}

void HM_SD::unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent)
{
	try
	{
		HomeMaticDevice::unserialize(serializedObject.substr(8, std::stoll(serializedObject.substr(0, 8), 0, 16)), dutyCycleMessageCounter, lastDutyCycleEvent);

		uint32_t pos = 8 + std::stoll(serializedObject.substr(0, 8), 0, 16);
		uint32_t filtersSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t i = 0; i < filtersSize; i++)
		{
			HM_SD_Filter filter;
			filter.filterType = (FilterType)std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			filter.filterValue = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			_filters.push_back(filter);
		}
		uint32_t responsesToOverwriteSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t i = 0; i < responsesToOverwriteSize; i++)
		{
			HM_SD_OverwriteResponse responseToOverwrite;
			uint32_t size = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			responseToOverwrite.packetPartToCapture = serializedObject.substr(pos, size); pos += size;
			size = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			responseToOverwrite.response = serializedObject.substr(pos, size); pos += size;
			responseToOverwrite.sendAfter = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			_responsesToOverwrite.push_back(responseToOverwrite);
		}
		_enabled = std::stol(serializedObject.substr(pos, 1)); pos += 1;
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

std::string HM_SD::serialize()
{
	std::string serializedBase = HomeMaticDevice::serialize();
	std::ostringstream stringstream;
	stringstream << std::hex << std::uppercase << std::setfill('0');
	stringstream << std::setw(8) << serializedBase.size() << serializedBase;
	stringstream << std::setw(8) << _filters.size();
	for(std::list<HM_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
	{
		stringstream << std::setw(8) << (int32_t)i->filterType;
		stringstream << std::setw(8) << i->filterValue;
	}
	stringstream << std::setw(8) << _responsesToOverwrite.size();
	for(std::list<HM_SD_OverwriteResponse>::const_iterator i = _responsesToOverwrite.begin(); i != _responsesToOverwrite.end(); ++i)
	{
		stringstream << std::setw(8) << i->packetPartToCapture.size();
		stringstream << i->packetPartToCapture;
		stringstream << std::setw(8) << i->response.size();
		stringstream << i->response;
		stringstream << std::setw(8) << i->sendAfter;
	}
	stringstream << std::setw(1) << (int32_t)_enabled;
	stringstream << std::dec;
	return stringstream.str();
}

bool HM_SD::packetReceived(std::shared_ptr<BidCoSPacket> packet)
{
	if(!_enabled) return false;
    bool printPacket = false;
    for(std::list<HM_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
    {
        switch(i->filterType)
        {
            case FilterType::SenderAddress:
                if(packet->senderAddress() == i->filterValue) printPacket = true;
                break;
            case FilterType::DestinationAddress:
                if(packet->destinationAddress() == i->filterValue) printPacket = true;
                break;
            case FilterType::DeviceType:
                //Only possible for paired devices
                break;
            case FilterType::MessageType:
                if(packet->messageType() == i->filterValue) printPacket = true;
                break;
        }
    }
    if(_filters.size() == 0) printPacket = true;
    std::chrono::time_point<std::chrono::system_clock> timepoint = std::chrono::system_clock::now();
    if(printPacket) std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()).count() << " Received: " << packet->hexString() << '\n';
    for(std::list<HM_SD_OverwriteResponse>::const_iterator i = _responsesToOverwrite.begin(); i != _responsesToOverwrite.end(); ++i)
    {
        std::string packetHex = packet->hexString();
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
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()).count() << " Overwriting response." << '\n';
            GD::rfDevice->sendPacket(packet);
        }
    }
    return false;
}

void HM_SD::addFilter(FilterType filterType, int32_t filterValue)
{
    HM_SD_Filter filter;
    filter.filterType = filterType;
    filter.filterValue = filterValue;
    _filters.push_back(filter);
}

void HM_SD::removeFilter(FilterType filterType, int32_t filterValue)
{
    _filters.remove_if([&](HM_SD_Filter filter){ return filter.filterType == filterType && filter.filterValue == filterValue; });
}

void HM_SD::addOverwriteResponse(std::string packetPartToCapture, std::string response, int32_t sendAfter)
{
    HM_SD_OverwriteResponse overwriteResponse;
    overwriteResponse.sendAfter = sendAfter;
    overwriteResponse.packetPartToCapture = packetPartToCapture;
    overwriteResponse.response = response;
    _responsesToOverwrite.push_back(overwriteResponse);
}

void removeCapture(std::string packetPartToCapture);

void HM_SD::removeOverwriteResponse(std::string packetPartToCapture)
{
    _responsesToOverwrite.remove_if([&](HM_SD_OverwriteResponse entry){ return entry.packetPartToCapture == packetPartToCapture; });
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
			stringStream << "enable\t\tEnables the device if it was disabled" << std::endl;
			stringStream << "disable\t\tDisables the device" << std::endl;
			stringStream << "filters list\t\tLists all packet filters" << std::endl;
			stringStream << "filters add\t\tAdds a packet filter" << std::endl;
			stringStream << "filters remove\t\tRemoves a packet filter" << std::endl;
			stringStream << "captures list\t\tLists all packet captures" << std::endl;
			stringStream << "captures add\t\tAdds a packet to capture" << std::endl;
			stringStream << "captures remove\t\tRemoves a packet to capture" << std::endl;
			stringStream << "send\t\tSends a BidCoS packet" << std::endl;
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
					filterType = HelperFunctions::getNumber(element, true);
					if(filterType != 0 && filterType != 1 && filterType != 3) return "Invalid filter type.\n";
				}
				else if(index == 3) filterValue = HelperFunctions::getNumber(element, true);
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
					filterType = HelperFunctions::getNumber(element, true);
					if(filterType != 0 && filterType != 1 && filterType != 3) return "Invalid filter type.\n";
				}
				else if(index == 3) filterValue = HelperFunctions::getNumber(element, true);
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
					sendAfter = HelperFunctions::getNumber(element);
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
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					packetHex = element;
				}
				index++;
			}
			if(index == 2)
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
		else return "Unknown command.\n";
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    return "Error executing command. See log file for more details.\n";
}
