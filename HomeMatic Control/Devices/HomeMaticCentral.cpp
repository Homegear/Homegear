#include "HomeMaticCentral.h"
#include "../GD.h"

HomeMaticCentral::HomeMaticCentral() : HomeMaticDevice()
{
	init();
}

HomeMaticCentral::HomeMaticCentral(std::string serialNumber, int32_t address) : HomeMaticDevice(serialNumber, address)
{
	init();
}

HomeMaticCentral::~HomeMaticCentral()
{

}

void HomeMaticCentral::init()
{
	try
	{
		HomeMaticDevice::init();

		_deviceType = HMDeviceTypes::HMCENTRAL;
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

void HomeMaticCentral::setUpBidCoSMessages()
{
	//Don't call HomeMaticDevice::setUpBidCoSMessages!
	_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x00, this, ACCESSPAIREDTOSENDER, FULLACCESS, &HomeMaticDevice::handlePairingRequest)));

	_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x02, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleAck)));

	_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x10, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigParamResponse)));
}

void HomeMaticCentral::unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent)
{
	HomeMaticDevice::unserialize(serializedObject.substr(8, std::stoll(serializedObject.substr(0, 8), 0, 16)), dutyCycleMessageCounter, lastDutyCycleEvent);
}

std::string HomeMaticCentral::serialize()
{
	std::string serializedBase = HomeMaticDevice::serialize();
	std::ostringstream stringstream;
	stringstream << std::hex << std::uppercase << std::setfill('0');
	stringstream << std::setw(8) << serializedBase.size() << serializedBase;
	return  stringstream.str();
}

void HomeMaticCentral::packetReceived(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		HomeMaticDevice::packetReceived(packet);
		for(std::unordered_map<int32_t, std::shared_ptr<Peer>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			std::thread t(&Peer::packetReceived, i->second.get(), packet);
			t.detach();
		}
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

void HomeMaticCentral::enqueuePackets(int32_t deviceAddress, std::shared_ptr<BidCoSQueue> packets, bool pushPendingBidCoSQueues)
{
	try
	{
		BidCoSQueue* queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::DEFAULT, deviceAddress);
		queue->push(packets, true, true);
		if(_peers.find(deviceAddress) != _peers.end() && pushPendingBidCoSQueues) queue->push(_peers[deviceAddress]->pendingBidCoSQueues);
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

std::shared_ptr<Peer> HomeMaticCentral::createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter)
{
	std::shared_ptr<Peer> peer(new Peer());
	peer->address = address;
	peer->firmwareVersion = firmwareVersion;
	peer->deviceType = deviceType;
	peer->serialNumber = serialNumber;
	peer->remoteChannel = remoteChannel;
	peer->messageCounter = messageCounter;
    return peer;
}

void HomeMaticCentral::handleCLICommand(std::string command)
{
	if(command == "pairing mode on")
	{
		_pairing = true;
		std::cout << "Pairing mode enabled." << std::endl;
	}
	else if(command == "pairing mode off")
	{
		_pairing = false;
		std::cout << "Pairing mode disabled." << std::endl;
	}
	else if(command == "unpair")
	{
		std::string input;
		std::cout << "Please enter the devices address: ";
		int32_t address = getHexInput();
		if(_peers.find(address) == _peers.end()) std::cout << "This device is not paired to this central." << std::endl;
		else
		{
			std::cout << "Unpairing device 0x" << std::hex << address << std::dec << std::endl;
			unpair(address);
		}
	}
	else if(command == "list peers")
	{
		for(std::unordered_map<int32_t, std::shared_ptr<Peer>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			std::cout << "Address: 0x" << std::hex << i->second->address << "\tSerial number: " << i->second->serialNumber << "\tDevice type: " << (uint32_t)i->second->deviceType << std::endl << std::dec;
		}
	}
	else if(command == "select peer")
	{
		std::cout << "Please enter the devices address: ";
		int32_t address = getHexInput();
		if(_peers.find(address) == _peers.end()) std::cout << "This device is not paired to this central." << std::endl;
		else
		{
			_currentPeer = _peers[address];
			std::cout << "Peer with address 0x" << std::hex << address << " and device type 0x" << (int32_t)_currentPeer->deviceType << " selected." << std::dec << std::endl;
		}
	}
	else if(command == "pending queues info")
	{
		if(!_currentPeer)
		{
			std::cout << "No peer selected." << std::endl;
			return;
		}
		std::cout << "Number of Pending queues:\t\t\t" << _currentPeer->pendingBidCoSQueues->size() << std::endl;
		if(!_currentPeer->pendingBidCoSQueues->empty() && !_currentPeer->pendingBidCoSQueues->front()->isEmpty())
		{
			std::cout << "First pending queue type:\t\t\t" << (int32_t)_currentPeer->pendingBidCoSQueues->front()->getQueueType() << std::endl;
			if(_currentPeer->pendingBidCoSQueues->front()->front()->getPacket()) std::cout << "First packet of first pending queue:\t\t" << _currentPeer->pendingBidCoSQueues->front()->front()->getPacket()->hexString() << std::endl;
			std::cout << "Type of first entry of first pending queue:\t" << (int32_t)_currentPeer->pendingBidCoSQueues->front()->front()->getType() << std::endl;
		}
	}
	else if(command == "clear pending queues")
	{
		if(!_currentPeer)
		{
			std::cout << "No peer selected." << std::endl;
			return;
		}
		_currentPeer->pendingBidCoSQueues.reset(new std::queue<std::shared_ptr<BidCoSQueue>>());
		std::cout << "Pending queues cleared." << std::endl;
	}
	else if(command == "team info")
	{
		if(!_currentPeer)
		{
			std::cout << "No peer selected." << std::endl;
			return;
		}
		if(_currentPeer->team.serialNumber.empty()) std::cout << "This peer doesn't support teams." << std::endl;
		else std::cout << "Team address: 0x" << std::hex << _currentPeer->team.address << std::dec << " Team serial number: " << _currentPeer->team.serialNumber << std::endl;
	}
}

void HomeMaticCentral::unpair(int32_t address)
{
	try
	{
		if(_peers.find(address) == _peers.end()) return;
		Peer* peer = _peers[address].get();
		BidCoSQueue* queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::UNPAIRING, address);
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::UNPAIRING));
		pendingQueue->noSending = true;

		std::vector<uint8_t> payload;

		//CONFIG_START
		payload.push_back(0);
		payload.push_back(0x05);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		payload.clear();
		_messageCounter[0]++;

		//CONFIG_WRITE_INDEX
		payload.push_back(0);
		payload.push_back(0x08);
		payload.push_back(0x02);
		payload.push_back(0);
		payload.push_back(0x0A);
		payload.push_back(0);
		payload.push_back(0x0B);
		payload.push_back(0);
		payload.push_back(0x0C);
		payload.push_back(0);
		configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		payload.clear();
		_messageCounter[0]++;

		//END_CONFIG
		payload.push_back(0);
		payload.push_back(0x06);
		configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		payload.clear();
		_messageCounter[0]++;

		peer->pendingBidCoSQueues->push(pendingQueue);
		queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(packet->destinationAddress() != 0) return;
		Peer* peer = nullptr;
		std::vector<uint8_t> payload;
		std::shared_ptr<RPC::Device> device;
		BidCoSQueue* queue = nullptr;
		if(_pairing)
		{
			queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::PAIRING, packet->senderAddress());

			device = GD::rpcDevices.find(packet);
			if(device == nullptr)
			{
				if(GD::debugLevel >= 3) std::cout << "Warning: Device type not supported. Sender address 0x" << std::hex << packet->senderAddress() << std::dec << "." << std::endl;
				return;
			}

			std::string serialNumber;
			for(uint32_t i = 3; i < 13; i++)
				serialNumber.push_back((char)packet->payload()->at(i));
			if(_peers.find(packet->senderAddress()) == _peers.end())
			{
				queue->peer = createPeer(packet->senderAddress(), packet->payload()->at(0), (HMDeviceTypes)((packet->payload()->at(1) << 8) + packet->payload()->at(2)), serialNumber, 0, 0);
				queue->peer->rpcDevice = device;
				queue->peer->initializeCentralConfig();
				peer = queue->peer.get();
			}
			else peer = _peers[packet->senderAddress()].get();

			//CONFIG_START
			payload.push_back(0);
			payload.push_back(0x05);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();
			_messageCounter[0]++;

			//CONFIG_WRITE_INDEX
			payload.push_back(0);
			payload.push_back(0x08);
			payload.push_back(0x02);
			payload.push_back(0x01);
			payload.push_back(0x0A);
			payload.push_back(_address >> 16);
			payload.push_back(0x0B);
			payload.push_back((_address >> 8) & 0xFF);
			payload.push_back(0x0C);
			payload.push_back(_address & 0xFF);
			configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();
			_messageCounter[0]++;

			//END_CONFIG
			payload.push_back(0);
			payload.push_back(0x06);
			configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();
			_messageCounter[0]++;

			std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::DEFAULT));
			pendingQueue->noSending = true;
			if((device->rxModes & RPC::Device::RXModes::Enum::config) && _peers.find(packet->senderAddress()) == _peers.end()) //Only request config when peer is not already paired to central
			{
				for(std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::iterator i = peer->configCentral.begin(); i != peer->configCentral.end(); ++i)
				{
					int32_t channel = i->first;
					//Walk through all lists to request master config if necessary
					for(std::map<RPC::ParameterSet::Type::Enum, std::shared_ptr<RPC::ParameterSet>>::iterator j = device->channels.at(channel)->parameterSets.begin(); j != device->channels.at(channel)->parameterSets.end(); ++j)
					{
						for(std::map<uint32_t, uint32_t>::iterator k = j->second->lists.begin(); k != j->second->lists.end(); ++k)
						{
							payload.push_back(channel);
							payload.push_back(0x04);
							payload.push_back(0);
							payload.push_back(0);
							payload.push_back(0);
							payload.push_back(0);
							payload.push_back(k->first);
							configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
							pendingQueue->push(configPacket);
							pendingQueue->push(_messages->find(DIRECTIONIN, 0x10, std::vector<std::pair<uint32_t, int32_t>>()));
							payload.clear();
							_messageCounter[0]++;
						}
					}
					//Request peers if not received yet
					if(peer->rpcDevice->channels[channel]->parameterSets.find(RPC::ParameterSet::Type::link) != peer->rpcDevice->channels[channel]->parameterSets.end() && peer->rpcDevice->channels[channel]->parameterSets[RPC::ParameterSet::Type::link])
					{
						payload.push_back(channel);
						payload.push_back(0x03);
						configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
						pendingQueue->push(configPacket);
						pendingQueue->push(_messages->find(DIRECTIONIN, 0x10, std::vector<std::pair<uint32_t, int32_t>>()));
						payload.clear();
						_messageCounter[0]++;
					}
				}
				if(!pendingQueue->isEmpty()) peer->pendingBidCoSQueues->push(pendingQueue);
			}
		}
		else
		{
			if(_peers.find(packet->senderAddress()) == _peers.end()) return;
			peer = _peers[packet->senderAddress()].get();
			queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::DEFAULT, packet->senderAddress());
		}
		if(peer == nullptr)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Peer is nullptr. This shouldn't have happened. Something went very wrong." << std::endl;
			return;
		}
		queue->push(peer->pendingBidCoSQueues); //This pushes the just generated queue and the already existent pending queue onto the queue
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

void HomeMaticCentral::handleConfigParamResponse(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		BidCoSQueue* queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(queue == nullptr) return;
		if(_sentPacket->payload()->at(1) == 0x03) //Peer request
		{
			for(uint32_t i = 1; i < packet->payload()->size(); i += 4)
			{
				int32_t peerAddress = (packet->payload()->at(i) << 16) + (packet->payload()->at(i + 1) << 8) + packet->payload()->at(i + 2);
				if(peerAddress != 0)
				{
					int32_t peerChannel = packet->payload()->at(i + 3);
					BasicPeer peer;
					peer.address = peerAddress;
					_peers[packet->senderAddress()]->peers[peerChannel].push_back(peer);
				}
			}
		}
		else if(_sentPacket->payload()->at(1) == 0x04) //Config request
		{
			if(packet->controlByte() & 0x20)
			{
				int32_t channel = _sentPacket->payload()->at(0);
				Peer* peer = _peers[packet->senderAddress()].get();
				int32_t startIndex = packet->payload()->at(1);
				int32_t endIndex = startIndex + packet->payload()->size() - 3;
				if(peer->rpcDevice->channels[channel]->parameterSets.find(RPC::ParameterSet::Type::master) == peer->rpcDevice->channels[channel]->parameterSets.end() || !peer->rpcDevice->channels[channel]->parameterSets[RPC::ParameterSet::Type::master])
				{
					if(GD::debugLevel >= 2) std::cout << "Error: Received config for non existant parameter set." << std::endl;
				}
				else
				{
					std::vector<std::shared_ptr<RPC::Parameter>> packetParameters = peer->rpcDevice->channels[channel]->parameterSets[RPC::ParameterSet::Type::master]->getIndices(startIndex, endIndex);
					for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator i = packetParameters.begin(); i != packetParameters.end(); ++i)
					{
						if(!(*i)->id.empty())
						{
							double position = ((*i)->physicalParameter->index - startIndex) + 2 + 9;
							peer->configCentral[channel][(*i)->id].value = packet->getPosition(position, (*i)->physicalParameter->size, false);
							if(GD::debugLevel >= 5) std::cout << "Parameter " << (*i)->id << " of device 0x" << std::hex << peer->address << std::dec << " at index " << std::to_string((*i)->physicalParameter->index) << " and packet index " << std::to_string(position) << " with size " << std::to_string((*i)->physicalParameter->size) << " was set to " << peer->configCentral[channel][(*i)->id].value << std::endl;
						}
						else if(GD::debugLevel >= 2) std::cout << "Error: Device tried to set parameter without id. Device: " << std::hex << peer->address << std::dec << " Serial number: " << peer->serialNumber << " Channel: " << channel << " List: " << (*i)->physicalParameter->list << " Parameter index: " << (*i)->index << std::endl;
					}
				}
			}
			else
			{
				Peer* peer = _peers[packet->senderAddress()].get();
				int32_t channel = _sentPacket->payload()->at(0);
				if(peer->rpcDevice->channels[channel]->parameterSets.find(RPC::ParameterSet::Type::master) == peer->rpcDevice->channels[channel]->parameterSets.end() || !peer->rpcDevice->channels[channel]->parameterSets[RPC::ParameterSet::Type::master])
				{
					if(GD::debugLevel >= 2) std::cout << "Error: Received config for non existant parameter set." << std::endl;
				}
				else
				{
					for(uint32_t i = 1; i < packet->payload()->size() - 2; i += 2)
					{
						int32_t index = packet->payload()->at(i);
						std::vector<std::shared_ptr<RPC::Parameter>> packetParameters = peer->rpcDevice->channels[channel]->parameterSets[RPC::ParameterSet::Type::master]->getIndices(index, index);
						for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = packetParameters.begin(); j != packetParameters.end(); ++j)
						{
							if(!(*j)->id.empty())
							{
								double position = std::fmod((*j)->physicalParameter->index, 1) + 9 + i + 1;
								peer->configCentral[channel][(*j)->id].value = packet->getPosition(position, (*j)->physicalParameter->size, false);
								if(GD::debugLevel >= 5) std::cout << "Parameter " << (*j)->id << " of device 0x" << std::hex << peer->address << std::dec << " at index " << std::to_string((*j)->physicalParameter->index) << " and packet index " << std::to_string(position) << " was set to " << peer->configCentral[channel][(*j)->id].value << std::endl;
							}
							else if(GD::debugLevel >= 2) std::cout << "Error: Device tried to set parameter without id. Device: " << std::hex << peer->address << std::dec << " Serial number: " << peer->serialNumber << " Channel: " << channel << " List: " << (*j)->physicalParameter->list << " Parameter index: " << (*j)->index << std::endl;
						}
					}
				}
			}
		}
		if(packet->controlByte() & 0x20) //Multiple config response packets
		{
			//StealthyOK does not set sentPacket
			sendStealthyOK(packet->messageCounter(), packet->senderAddress());
			//No popping from queue to stay at the same position until all response packets are received!!!
			//The popping is done with the last packet, which has no BIDI control bit set. See https://sathya.de/HMCWiki/index.php/Examples:Message_Counter
		}
		else
		{
			queue->pop(true); //Messages are not popped by default.
		}
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

void HomeMaticCentral::handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		BidCoSQueue* queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(queue == nullptr) return;
		if(queue->getQueueType() == BidCoSQueueType::PAIRING)
		{
			if(_sentPacket->messageType() == 0x01 && _sentPacket->payload()->at(0) == 0x00 && _sentPacket->payload()->at(1) == 0x06)
			{
				if(_peers.find(packet->senderAddress()) == _peers.end())
				{
					for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = queue->peer->rpcDevice->channels.begin(); i != queue->peer->rpcDevice->channels.end(); ++i)
					{
						if(i->second->hasTeam)
						{
							std::shared_ptr<Peer> team;
							std::string serialNumber('*' + queue->peer->serialNumber);
							if(_peersBySerial.find(serialNumber) == _peersBySerial.end())
							{
								team = createTeam(queue->peer->address, queue->peer->deviceType, serialNumber);
								team->rpcDevice = queue->peer->rpcDevice->team;
								_peersBySerial[team->serialNumber] = team;
							}
							else team = _peersBySerial['*' + queue->peer->serialNumber];
							queue->peer->team.address = team->address;
							queue->peer->team.serialNumber = team->serialNumber;
							_peersBySerial[team->serialNumber]->teamChannels.push_back(std::pair<std::string, uint32_t>(queue->peer->serialNumber, i->first));
							break;
						}
					}
					_peers[queue->peer->address] = queue->peer;
					if(queue->peer->serialNumber.size() == 10) _peersBySerial[queue->peer->serialNumber] = queue->peer;
					std::cout << "Added device 0x" << std::hex << queue->peer->address << std::dec << "." << std::endl;
				}
			}
		}
		else if(queue->getQueueType() == BidCoSQueueType::UNPAIRING)
		{
			if(_sentPacket->messageType() == 0x01 && _sentPacket->payload()->at(0) == 0x00 && _sentPacket->payload()->at(1) == 0x06)
			{
				if(_peers.find(packet->senderAddress()) != _peers.end())
				{
					if(_peersBySerial.find(_peers[packet->senderAddress()]->serialNumber) != _peersBySerial.end()) _peersBySerial.erase(_peers[packet->senderAddress()]->serialNumber);
					_peers.erase(_peers.find(packet->senderAddress()));
					std::cout << "Removed device 0x" << std::hex << packet->senderAddress() << std::dec << std::endl;
				}
			}
		}
		queue->pop(true); //Messages are not popped by default.
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

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::listDevices()
{
	try
	{
		std::shared_ptr<RPC::RPCVariable> array(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		for(std::unordered_map<std::string, std::shared_ptr<Peer>>::iterator i = _peersBySerial.begin(); i != _peersBySerial.end(); ++i)
		{
			std::vector<std::shared_ptr<RPC::RPCVariable>> descriptions = i->second->getDeviceDescription();
			for(std::vector<std::shared_ptr<RPC::RPCVariable>>::iterator j = descriptions.begin(); j != descriptions.end(); ++j)
			{
				array->arrayValue->push_back(*j);
			}
		}
		return array;
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
    return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable());
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getParamsetDescription(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type)
{
	try
	{
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end()) return _peersBySerial[serialNumber]->getParamsetDescription(channel, type);
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
    return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable());
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getValue(std::string serialNumber, uint32_t channel, std::string valueKey)
{
	try
	{
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end()) return _peersBySerial[serialNumber]->getValue(channel, valueKey);
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
    return RPC::RPCVariable::createError(-2, "unknown device");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::setValue(std::string serialNumber, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end()) return _peersBySerial[serialNumber]->setValue(channel, valueKey, value);
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
    return RPC::RPCVariable::createError(-2, "unknown device");
}
