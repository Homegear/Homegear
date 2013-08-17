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
	stringstream << std::setw(8) << _states.size();
	for(std::map<uint32_t, bool>::iterator i = _states.begin(); i != _states.end(); ++i)
	{
		stringstream << std::setw(4) << (i->first & 0xFFFF);
		stringstream << std::setw(1) << i->second;
	}
	stringstream << std::setw(8) << _channelCount;
	return  stringstream.str();
}

void HM_LC_SWX_FM::unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent)
{
	try
	{
		int32_t baseLength = std::stoll(serializedObject.substr(0, 8), 0, 16);
		HomeMaticDevice::unserialize(serializedObject.substr(8, baseLength), dutyCycleMessageCounter, lastDutyCycleEvent);

		uint32_t pos = 8 + baseLength;
		uint32_t stateSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t i = 0; i < stateSize; i++)
		{
			uint32_t channel = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
			bool state = std::stol(serializedObject.substr(pos, 1)); pos += 1;
			_states[channel] = state;
		}
		_channelCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
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

std::string HM_LC_SWX_FM::handleCLICommand(std::string command)
{
	return HomeMaticDevice::handleCLICommand(command);
}

void HM_LC_SWX_FM::handleStateChange(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(packet->payload()->empty()) return;
		int32_t channel = packet->payload()->at(0) & 0x3F;
		if(channel > _channelCount) return;
		std::shared_ptr<HomeMaticCentral> central(GD::devices.getCentral());
		std::shared_ptr<Peer> peer(central->getPeer(packet->senderAddress()));
		//Check if channel is paired to me
		std::shared_ptr<BasicPeer> me(peer->getPeer(channel, _address, channel));
		if(!me) return;
		int32_t channelGroupedWith = peer->getChannelGroupedWith(channel);
		if(channelGroupedWith > -1)
		{
			//Check if grouped channel is also paired to me
			std::shared_ptr<BasicPeer> me2(peer->getPeer(channelGroupedWith, _address, channelGroupedWith));
			if(!me2) channelGroupedWith = -1;
		}
		if(channelGroupedWith > -1)
		{
			if(channelGroupedWith < channel) _states[channel] = true;
			else _states[channel] = false;
		}
		else _states[channel] = !_states[channel];

		if(packet->controlByte() & 0x20) sendStateChangeResponse(packet, channel);
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

void HM_LC_SWX_FM::sendStateChangeResponse(std::shared_ptr<BidCoSPacket> receivedPacket, uint32_t channel)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x01); //Subtype
		payload.push_back(channel); //Channel (doesn't necessarily have to be correct)
		if(_states[channel]) payload.push_back(0xC8);
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
