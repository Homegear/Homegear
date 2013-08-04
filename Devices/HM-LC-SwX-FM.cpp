#include "HM-LC-SwX-FM.h"
#include "../GD.h"

HM_LC_SWX_FM::HM_LC_SWX_FM() : HomeMaticDevice()
{
	init();
}

HM_LC_SWX_FM::HM_LC_SWX_FM(std::string serialNumber, int32_t address) : HomeMaticDevice(serialNumber, address)
{
	init();
}

void HM_LC_SWX_FM::init()
{
	HomeMaticDevice::init();

	_deviceType = HMDeviceTypes::HMLCSW1FM;
    _firmwareVersion = 0x16;

    setUpBidCoSMessages();
}

void HM_LC_SWX_FM::setUpBidCoSMessages()
{
    HomeMaticDevice::setUpBidCoSMessages();

    //Incoming
    _messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x40, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleStateChange)));
}

HM_LC_SWX_FM::~HM_LC_SWX_FM()
{
}

std::string HM_LC_SWX_FM::serialize()
{
	std::string serializedBase = HomeMaticDevice::serialize();
	std::ostringstream stringstream;
	stringstream << std::hex << std::uppercase << std::setfill('0');
	stringstream << std::setw(8) << serializedBase.size() << serializedBase;
	stringstream << std::setw(1) << _state;
	stringstream << std::setw(8) << _channelCount;
	return  stringstream.str();
}

void HM_LC_SWX_FM::unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent)
{
	int32_t baseLength = std::stoll(serializedObject.substr(0, 8), 0, 16);
	HomeMaticDevice::unserialize(serializedObject.substr(8, baseLength), dutyCycleMessageCounter, lastDutyCycleEvent);

	uint32_t pos = 8 + baseLength;
	_state = std::stol(serializedObject.substr(pos, 1)); pos += 1;
	_channelCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
}

void HM_LC_SWX_FM::handleCLICommand(std::string command)
{
	HomeMaticDevice::handleCLICommand(command);
}

void HM_LC_SWX_FM::handleStateChange(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(packet->channel() % 2) _state = true; else _state = false;
		sendStateChangeResponse(packet);
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

void HM_LC_SWX_FM::sendStateChangeResponse(std::shared_ptr<BidCoSPacket> receivedPacket)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x01); //Subtype
		payload.push_back(0x01); //Channel
		if(_state) payload.push_back(0xC8);
		else payload.push_back(0x00);
		payload.push_back(0x00); //STATE_FLAGS / WORKING
		payload.push_back(0xD3); //RSSI?
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(receivedPacket->messageCounter(), 0x80, 0x02, _address, receivedPacket->senderAddress(), payload));
		sendPacket(packet);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
}
