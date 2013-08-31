#include "HM-LC-SwX-FM.h"
#include "../GD.h"
#include "../HelperFunctions.h"

HM_LC_SWX_FM::HM_LC_SWX_FM() : HomeMaticDevice()
{
	init();
}

HM_LC_SWX_FM::HM_LC_SWX_FM(uint32_t deviceID, std::string serialNumber, int32_t address) : HomeMaticDevice(deviceID, serialNumber, address)
{
	init();
}

void HM_LC_SWX_FM::init()
{
	try
	{
		HomeMaticDevice::init();

		_deviceType = HMDeviceTypes::HMLCSW1FM;
		_firmwareVersion = 0x16;

		setUpBidCoSMessages();
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LC_SWX_FM::setUpBidCoSMessages()
{
	try
	{
		HomeMaticDevice::setUpBidCoSMessages();

		//Incoming
		_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x40, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleStateChangeRemote)));
		_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x41, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleStateChangeMotionDetector)));
	}
    catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

HM_LC_SWX_FM::~HM_LC_SWX_FM()
{
	dispose();
}

void HM_LC_SWX_FM::saveVariables()
{
	try
	{
		if(_deviceID == 0) return;
		HomeMaticDevice::saveVariables();
		saveVariable(1000, _channelCount);
		saveStates(); //1001
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LC_SWX_FM::loadVariables()
{
	try
	{
		HomeMaticDevice::loadVariables();
		_databaseMutex.lock();
		DataTable rows = GD::db.executeCommand("SELECT * FROM deviceVariables WHERE deviceID=" + std::to_string(_deviceID));
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 1000:
				_channelCount = row->second.at(3)->intValue;
				break;
			case 1001:
				unserializeStates(row->second.at(5)->binaryValue);
				break;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	_databaseMutex.unlock();
}

void HM_LC_SWX_FM::saveStates()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializeStates(serializedData);
		saveVariable(1001, serializedData);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LC_SWX_FM::serializeStates(std::vector<uint8_t>& encodedData)
{
	try
	{
		BinaryEncoder encoder;
		encoder.encodeInteger(encodedData, _states.size());
		for(std::map<uint32_t, bool>::iterator i = _states.begin(); i != _states.end(); ++i)
		{
			encoder.encodeInteger(encodedData, i->first);
			encoder.encodeBoolean(encodedData, i->second);
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LC_SWX_FM::unserializeStates(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BinaryDecoder decoder;
		uint32_t position = 0;
		uint32_t stateSize = decoder.decodeInteger(serializedData, position);
		for(uint32_t i = 0; i < stateSize; i++)
		{
			uint32_t channel = decoder.decodeInteger(serializedData, position);
			_states[channel] = decoder.decodeBoolean(serializedData, position);
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LC_SWX_FM::unserialize_0_0_6(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent)
{
	try
	{
		int32_t baseLength = std::stoll(serializedObject.substr(0, 8), 0, 16);
		HomeMaticDevice::unserialize_0_0_6(serializedObject.substr(8, baseLength), dutyCycleMessageCounter, lastDutyCycleEvent);

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
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string HM_LC_SWX_FM::handleCLICommand(std::string command)
{
	return HomeMaticDevice::handleCLICommand(command);
}

void HM_LC_SWX_FM::handleStateChangeRemote(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
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
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_LC_SWX_FM::handleStateChangeMotionDetector(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
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
		_states[channel] = true;

		if(packet->controlByte() & 0x20) sendStateChangeResponse(packet, channel);
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
