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
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticCentral::setUpBidCoSMessages()
{
	//Don't call HomeMaticDevice::setUpBidCoSMessages!
	_messages->add(shared_ptr<BidCoSMessage>(new BidCoSMessage(0x00, this, ACCESSPAIREDTOSENDER, FULLACCESS, &HomeMaticDevice::handlePairingRequest)));

	_messages->add(shared_ptr<BidCoSMessage>(new BidCoSMessage(0x02, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleAck)));

	_messages->add(shared_ptr<BidCoSMessage>(new BidCoSMessage(0x10, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigParamResponse)));
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

void HomeMaticCentral::packetReceived(shared_ptr<BidCoSPacket> packet)
{
	try
	{
		HomeMaticDevice::packetReceived(packet);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

Peer HomeMaticCentral::createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter)
{
	Peer peer;
	peer.address = address;
	peer.firmwareVersion = firmwareVersion;
	peer.deviceType = deviceType;
	peer.serialNumber = serialNumber;
	peer.remoteChannel = remoteChannel;
	peer.messageCounter = messageCounter;
    return peer;
}

void HomeMaticCentral::handleCLICommand(std::string command)
{
	if(command == "pairing mode on")
	{
		_pairing = true;
		cout << "Pairing mode enabled." << endl;
	}
	else if(command == "pairing mode off")
	{
		_pairing = false;
		cout << "Pairing mode disabled." << endl;
	}
	else if(command == "unpair")
	{
		std::string input;
		cout << "Please enter the devices address: ";
		int32_t address = getHexInput();
		if(_peers.find(address) == _peers.end()) cout << "This device is not paired to this central." << endl;
		else
		{
			cout << "Unpairing device 0x" << std::hex << address << std::dec << endl;
			unpair(address);
		}
	}
	else if(command == "list peers")
	{
		for(std::unordered_map<int32_t, Peer>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			cout << "Address: 0x" << std::hex << i->second.address << "\tSerial number: " << i->second.serialNumber << "\tDevice type: " << (uint32_t)i->second.deviceType << endl << std::dec;
		}
	}
	else if(command == "select peer")
	{
		cout << "Please enter the devices address: ";
		int32_t address = getHexInput();
		if(_peers.find(address) == _peers.end()) cout << "This device is not paired to this central." << endl;
		else
		{
			_currentPeer = &_peers[address];
			cout << "Peer with address 0x" << std::hex << address << " and device type 0x" << (int32_t)_currentPeer->deviceType << " selected." << std::dec << endl;
		}
	}
	else if(command == "pending queues info")
	{
		if(_currentPeer == nullptr)
		{
			cout << "No peer selected." << endl;
			return;
		}
		cout << "Number of Pending queues:\t\t\t" << _currentPeer->pendingBidCoSQueues->size() << endl;
		cout << "First pending queue type:\t\t\t" << (int32_t)_currentPeer->pendingBidCoSQueues->front()->getQueueType() << endl;
		cout << "First packet of first pending queue:\t\t" << _currentPeer->pendingBidCoSQueues->front()->front()->getPacket()->hexString() << endl;
		cout << "Type of first entry of first pending queue:\t" << (int32_t)_currentPeer->pendingBidCoSQueues->front()->front()->getType() << endl;
	}
}

void HomeMaticCentral::unpair(int32_t address)
{
	try
	{
		if(_peers.find(address) == _peers.end()) return;
		Peer* peer = &_peers[address];
		BidCoSQueue* queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::UNPAIRING, address);
		shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::UNPAIRING));
		pendingQueue->noSending = true;

		//TODO Implement pending config
		std::vector<uint8_t> payload;

		//CONFIG_START
		payload.push_back(0);
		payload.push_back(0x05);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload));
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
		configPacket = shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		payload.clear();
		_messageCounter[0]++;

		//END_CONFIG
		payload.push_back(0);
		payload.push_back(0x06);
		configPacket = shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		payload.clear();
		_messageCounter[0]++;

		peer->pendingBidCoSQueues->push(pendingQueue);
		queue->push(peer->pendingBidCoSQueues);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
	catch(const Exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticCentral::handlePairingRequest(int32_t messageCounter, shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(packet->destinationAddress() != 0) return;
		Peer* peer = nullptr;
		std::vector<uint8_t> payload;
		shared_ptr<RPC::Device> device;
		BidCoSQueue* queue = nullptr;
		if(_pairing)
		{
			queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::PAIRING, packet->senderAddress());

			device = GD::rpcDevices.find(packet);
			if(device == nullptr)
			{
				if(GD::debugLevel >= 3) cout << "Warning: Device type not supported. Sender address 0x" << std::hex << packet->senderAddress() << std::dec << "." << std::endl;
				return;
			}

			std::string serialNumber;
			for(uint32_t i = 3; i < 13; i++)
				serialNumber.push_back((char)packet->payload()->at(i));
			if(_peers.find(packet->senderAddress()) == _peers.end())
			{
				queue->peer = createPeer(packet->senderAddress(), packet->payload()->at(0), (HMDeviceTypes)((packet->payload()->at(1) << 8) + packet->payload()->at(2)), serialNumber, 0, 0);
				queue->peer.xmlrpcDevice = device;
				queue->peer.initializeCentralConfig();
				peer = &queue->peer;
			}
			else peer = &_peers[packet->senderAddress()];

			//CONFIG_START
			payload.push_back(0);
			payload.push_back(0x05);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
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
			configPacket = shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();
			_messageCounter[0]++;

			//END_CONFIG
			payload.push_back(0);
			payload.push_back(0x06);
			configPacket = shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();
			_messageCounter[0]++;

			shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::UNPAIRING));
			pendingQueue->noSending = true;
			if(_peers.find(packet->senderAddress()) == _peers.end()) //Only request config when peer is not already paired to central
			{
				for(std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<double, XMLRPCConfigurationParameter>>>>::iterator i = peer->configCentral.begin(); i != peer->configCentral.end(); ++i)
				{
					int32_t channel = i->first;
					//Walk through all lists to request master config if necessary
					for(std::unordered_map<int32_t, std::unordered_map<double, XMLRPCConfigurationParameter>>::iterator j = i->second.at(RPC::ParameterSet::Type::Enum::master).begin(); j != i->second.at(RPC::ParameterSet::Type::Enum::master).end(); ++j)
					{
						int32_t list = j->first;
						payload.push_back(channel);
						payload.push_back(0x04);
						payload.push_back(0);
						payload.push_back(0);
						payload.push_back(0);
						payload.push_back(0);
						payload.push_back(list);
						configPacket = shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
						pendingQueue->push(configPacket);
						pendingQueue->push(_messages->find(DIRECTIONIN, 0x10, std::vector<std::pair<uint32_t, int32_t>>()));
						payload.clear();
						_messageCounter[0]++;
					}
					//Request peers if not received yet
					if(peer->xmlrpcDevice->channels[channel]->getParameterSet(RPC::ParameterSet::Type::link) != nullptr)
					{
						payload.push_back(channel);
						payload.push_back(0x03);
						configPacket = shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
						pendingQueue->push(configPacket);
						pendingQueue->push(_messages->find(DIRECTIONIN, 0x10, std::vector<std::pair<uint32_t, int32_t>>()));
						payload.clear();
						_messageCounter[0]++;
					}
				}
				peer->pendingBidCoSQueues->push(pendingQueue);
			}
		}
		else
		{
			if(_peers.find(packet->senderAddress()) == _peers.end()) return;
			peer = &_peers[packet->senderAddress()];
			queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::DEFAULT, packet->senderAddress());
		}
		if(peer == nullptr)
		{
			if(GD::debugLevel >= 2) cout << "Error: Peer is nullptr. This shouldn't have happened. Something went very wrong." << endl;
			return;
		}
		queue->push(peer->pendingBidCoSQueues); //This pushes the just generated queue and the already existent pending queue onto the queue
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
	catch(const Exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticCentral::handleConfigParamResponse(int32_t messageCounter, shared_ptr<BidCoSPacket> packet)
{
	try
	{
		BidCoSQueue* queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(queue == nullptr) return;
		if(_sentPacket.payload()->at(1) == 0x03) //Peer request
		{
			for(uint32_t i = 1; i < packet->payload()->size(); i += 4)
			{
				int32_t peerAddress = (packet->payload()->at(i) << 16) + (packet->payload()->at(i + 1) << 8) + packet->payload()->at(i + 2);
				if(peerAddress != 0)
				{
					int32_t peerChannel = packet->payload()->at(i + 3);
					PairedPeer peer;
					peer.address = peerAddress;
					_peers[packet->senderAddress()].peers[peerChannel].push_back(peer);
				}
			}
		}
		else if(_sentPacket.payload()->at(1) == 0x04) //Config request
		{
			if(packet->controlByte() & 0x20)
			{
				int32_t channel = _sentPacket.payload()->at(0);
				int32_t list = _sentPacket.payload()->at(6);
				Peer* peer = &_peers[packet->senderAddress()];
				int32_t startIndex = packet->payload()->at(1);
				int32_t endIndex = packet->payload()->size() - 1;
				if(peer->xmlrpcDevice->channels[channel]->getParameterSet(RPC::ParameterSet::Type::master) == nullptr)
				{
					if(GD::debugLevel >= 2) cout << "Error: Received config for non existant parameter set." << endl;
				}
				else
				{
					std::vector<shared_ptr<RPC::Parameter>> packetParameters = peer->xmlrpcDevice->channels[channel]->getParameterSet(RPC::ParameterSet::Type::master)->getIndices(startIndex, endIndex);
					for(std::vector<shared_ptr<RPC::Parameter>>::iterator i = packetParameters.begin(); i != packetParameters.end(); ++i)
					{
						peer->configCentral[channel][(uint32_t)RPC::ParameterSet::Type::master][list][(*i)->index].value = packet->getPosition((*i)->index, (*i)->size, (*i)->isSigned);
					}
				}
			}
			else
			{
				for(uint32_t i = 1; i < packet->payload()->size() - 2; i += 2)
				{
					_peers[packet->senderAddress()].configCentral[_sentPacket.payload()->at(0)][(uint32_t)RPC::ParameterSet::Type::master][_sentPacket.payload()->at(6)][packet->payload()->at(i)].value = packet->payload()->at(i + 1);
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
			queue->pop(); //Messages are not popped by default.
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
	catch(const Exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

//TODO Crash when double pairing
void HomeMaticCentral::handleAck(int32_t messageCounter, shared_ptr<BidCoSPacket> packet)
{
	try
	{
		BidCoSQueue* queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(queue == nullptr) return;
		if(queue->getQueueType() == BidCoSQueueType::PAIRING)
		{
			if(_sentPacket.messageType() == 0x01 && _sentPacket.payload()->at(0) == 0x00 && _sentPacket.payload()->at(1) == 0x06)
			{
				if(_peers.find(packet->senderAddress()) == _peers.end())
				{
					_peers[queue->peer.address] = queue->peer;
					cout << "Added device 0x" << std::hex << queue->peer.address << std::dec << endl;
				}
			}
		}
		else if(queue->getQueueType() == BidCoSQueueType::UNPAIRING)
		{
			if(_sentPacket.messageType() == 0x01 && _sentPacket.payload()->at(0) == 0x00 && _sentPacket.payload()->at(1) == 0x06)
			{
				if(_peers.find(packet->senderAddress()) != _peers.end())
				{
					_peers.erase(_peers.find(packet->senderAddress()));
					cout << "Removed device 0x" << std::hex << packet->senderAddress() << std::dec << endl;
				}
			}
		}
		queue->pop(); //Messages are not popped by default.
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
	catch(const Exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

