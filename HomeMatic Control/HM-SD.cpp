#include "HM-SD.h"

HM_SD::HM_SD() : HomeMaticDevice("0000000000", 0)
{

}

HM_SD::~HM_SD()
{
}

void HM_SD::packetReceived(BidCoSPacket* packet)
{
	_receivedPacket = *packet;
    bool printPacket = false;
    for(std::list<HM_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
    {
        switch(i->filterType)
        {
            case FilterType::SenderAddress:
                if(_receivedPacket.senderAddress() == i->filterValue) printPacket = true;
                break;
            case FilterType::DestinationAddress:
                if(_receivedPacket.destinationAddress() == i->filterValue) printPacket = true;
                break;
            case FilterType::DeviceType:
                //Only possible for paired devices
                break;
            case FilterType::MessageType:
                if(_receivedPacket.messageType() == i->filterValue) printPacket = true;
                break;
        }
    }
    if(_filters.size() == 0) printPacket = true;
    std::chrono::time_point<std::chrono::system_clock> timepoint = std::chrono::system_clock::now();
    if(printPacket) cout << std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()).count() << " Received: " << _receivedPacket.hexString() << '\n';
    for(std::list<HM_SD_OverwriteResponse>::const_iterator i = _responsesToOverwrite.begin(); i != _responsesToOverwrite.end(); ++i)
    {
        std::string packetHex = _receivedPacket.hexString();
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
            GD::cul->sendPacket(packet);
        }
    }
    for(std::list<HM_SD_BlockResponse>::const_iterator i = _responsesToBlock.begin(); i != _responsesToBlock.end(); ++i)
    {
        std::string packetHex = _receivedPacket.hexString();
        if(packetHex.find(i->packetPartToCapture) != std::string::npos)
        {
            std::chrono::milliseconds sleepingTime(i->sendAfter); //Don't respond too fast
            std::this_thread::sleep_for(sleepingTime);
            std::vector<uint8_t> payload;
            for(int i = 0; i < 200; i++)
            {
                payload.push_back(i);
            }
            BidCoSPacket packet(_messageCounter[0], 0x84, 0xF0, _address, 0, payload);
            for(int i = 0; i < 100; i++)
            {
                GD::cul->sendPacket(_receivedPacket);
            }
            std::chrono::time_point<std::chrono::system_clock> timepoint = std::chrono::system_clock::now();
            cout << std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()).count() << " Blocked response to: " << i->packetPartToCapture << '\n';
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

void HM_SD::addBlockResponse(std::string packetPartToCapture, int32_t sendAfter)
{
    HM_SD_BlockResponse blockResponse;
    blockResponse.sendAfter = sendAfter;
    blockResponse.packetPartToCapture = packetPartToCapture;
    _responsesToBlock.push_back(blockResponse);
}
