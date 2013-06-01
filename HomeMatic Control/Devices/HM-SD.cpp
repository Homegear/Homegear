#include "HM-SD.h"

HM_SD::HM_SD() : HomeMaticDevice()
{
}

HM_SD::HM_SD(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent) : HomeMaticDevice(serializedObject.substr(8, std::stoll(serializedObject.substr(0, 8), 0, 16)), dutyCycleMessageCounter, lastDutyCycleEvent)
{
	init();

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
	}
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
	stringstream << std::dec;
	return stringstream.str();
}

void HM_SD::packetReceived(BidCoSPacket* packet)
{
	BidCoSPacket receivedPacket = *packet;
    bool printPacket = false;
    for(std::list<HM_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
    {
        switch(i->filterType)
        {
            case FilterType::SenderAddress:
                if(receivedPacket.senderAddress() == i->filterValue) printPacket = true;
                break;
            case FilterType::DestinationAddress:
                if(receivedPacket.destinationAddress() == i->filterValue) printPacket = true;
                break;
            case FilterType::DeviceType:
                //Only possible for paired devices
                break;
            case FilterType::MessageType:
                if(receivedPacket.messageType() == i->filterValue) printPacket = true;
                break;
        }
    }
    if(_filters.size() == 0) printPacket = true;
    std::chrono::time_point<std::chrono::system_clock> timepoint = std::chrono::system_clock::now();
    if(printPacket) cout << std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()).count() << " Received: " << receivedPacket.hexString() << '\n';
    for(std::list<HM_SD_OverwriteResponse>::const_iterator i = _responsesToOverwrite.begin(); i != _responsesToOverwrite.end(); ++i)
    {
        std::string packetHex = receivedPacket.hexString();
        if(packetHex.find(i->packetPartToCapture) != std::string::npos)
        {
            std::chrono::milliseconds sleepingTime(i->sendAfter); //Don't respond too fast
            std::this_thread::sleep_for(sleepingTime);
            BidCoSPacket packet;
            std::stringstream stringstream;
            stringstream << std::hex << std::setw(2) << (i->response.size() / 2) + 1;
            std::string lengthHex = stringstream.str();
            packet.import("A" + lengthHex + packetHex.substr(2, 2) + i->response + "\r\n");
            std::chrono::time_point<std::chrono::system_clock> timepoint = std::chrono::system_clock::now();
            cout << std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()).count() << " Overwriting response: " << '\n';
            sendPacket(packet);
        }
    }
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

void HM_SD::removeOverwriteResponse(std::string packetPartToCapture)
{
    _responsesToOverwrite.remove_if([&](HM_SD_OverwriteResponse entry){ return entry.packetPartToCapture == packetPartToCapture; });
}

void HM_SD::handleCLICommand(std::string command)
{
	if(command == "add filter")
	{
		std::string input;
		cout << "Enter filter type or press \"l\" to list filter types: ";
		do
		{
			cin >> input;
			if(input == "l")
			{
				cout << "Filter types:" << endl;
				cout << "\t00: Sender address" << endl;
				cout << "\t01: Destination address" << endl;
				cout << "\t03: Message type" << endl << endl;
				cout << "Enter filter type or press \"l\" to list filter types: ";
			}
		}while(input == "l");
		int32_t filterType = -1;
		try	{ filterType = std::stoll(input, 0, 16); } catch(...) {}
		if(filterType < 0 || filterType > 3) cout << "Invalid filter type." << endl;
		else
		{
			cout << "Please enter a filter value in hexadecimal format: ";
			HM_SD_Filter filter;
			filter.filterType = (FilterType)filterType;
			filter.filterValue = getHexInput();
			_filters.push_back(filter);
			cout << "Filter added." << endl;
		}
	}
	else if(command == "remove filter")
	{
		std::string input;
		cout << "Enter filter type or press \"l\" to list filter types: ";
		while(input == "l")
		{
			cin >> input;
			cout << "Filter types:" << endl;
			cout << "\t00: Sender address" << endl;
			cout << "\t01: Destination address" << endl;
			cout << "\t03: Message type" << endl << endl;
			cout << "Enter filter type or press \"l\" to list filter types: ";
		}
		int32_t filterType = getHexInput();
		if(filterType < 0 || filterType > 3) cout << "Invalid filter type." << endl;
		else
		{
			cout << "Please enter a filter value in hexadecimal format: ";
			uint32_t oldSize = _filters.size();
			removeFilter((FilterType)filterType, getHexInput());
			if(_filters.size() != oldSize) cout << "Filter removed." << endl;
			else cout << "Filter not found." << endl;
		}
	}
	else if(command == "list filters")
	{
		for(std::list<HM_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
		{
			cout << "Filter type: " << std::hex << (int32_t)i->filterType << "\tFilter value: " << i->filterValue << std::dec << endl;
		}
	}
}
