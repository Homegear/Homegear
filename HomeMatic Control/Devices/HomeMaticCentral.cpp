#include "HomeMaticCentral.h"
#include "../GD.h"

HomeMaticCentral::HomeMaticCentral()
{

}

HomeMaticCentral::HomeMaticCentral(std::string serialNumber, int32_t address) : HomeMaticDevice(serialNumber, address)
{
	init();
}

HomeMaticCentral::HomeMaticCentral(std::string serializedObject) : HomeMaticDevice(serializedObject.substr(8, std::stoll(serializedObject.substr(0, 8), 0, 16)), 0, 0)
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
	_messages->add(BidCoSMessage(0x00, this, ACCESSPAIREDTOSENDER, FULLACCESS, &HomeMaticDevice::handlePairingRequest));

	_messages->add(BidCoSMessage(0x02, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleAck));

	_messages->add(BidCoSMessage(0x10, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigParamResponse));
}

std::string HomeMaticCentral::serialize()
{
	std::string serializedBase = HomeMaticDevice::serialize();
	std::ostringstream stringstream;
	stringstream << std::hex << std::uppercase << std::setfill('0');
	stringstream << std::setw(8) << serializedBase.size() << serializedBase;
	return  stringstream.str();
}

void HomeMaticCentral::packetReceived(BidCoSPacket* packet)
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
}

void HomeMaticCentral::unpair(int32_t address)
{
	try
	{
		if(_peers.find(address) == _peers.end()) return;
		newBidCoSQueue(BidCoSQueueType::UNPAIRING);
		Peer* peer = &_peers[address];
		if(peer->pendingBidCoSQueue.get() == nullptr) peer->pendingBidCoSQueue = shared_ptr<BidCoSQueue>(new BidCoSQueue(BidCoSQueueType::DEFAULT));

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
		BidCoSPacket configPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload);
		peer->pendingBidCoSQueue->push(configPacket);
		peer->pendingBidCoSQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<int32_t, int32_t>>()));
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
		configPacket = BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload);
		peer->pendingBidCoSQueue->push(configPacket);
		peer->pendingBidCoSQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<int32_t, int32_t>>()));
		payload.clear();
		_messageCounter[0]++;

		//END_CONFIG
		payload.push_back(0);
		payload.push_back(0x06);
		configPacket = BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload);
		peer->pendingBidCoSQueue->push(configPacket);
		peer->pendingBidCoSQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<int32_t, int32_t>>()));
		payload.clear();
		_messageCounter[0]++;

		peer->pendingBidCoSQueue->noSending = true;
		_bidCoSQueue->push(peer->pendingBidCoSQueue);
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

void HomeMaticCentral::handlePairingRequest(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		Peer* peer = nullptr;
		if(packet->destinationAddress() == 0)
		{
			std::vector<uint8_t> payload;
			XMLRPC::Device* device;
			if(_pairing)
			{
				newBidCoSQueue(BidCoSQueueType::PAIRING);

				device = GD::xmlrpcDevices.find(packet);
				if(device == nullptr)
				{
					if(GD::debugLevel >= 3) cout << "Warning: Device type not supported. Sender address 0x" << std::hex << packet->senderAddress() << std::dec << "." << std::endl;
					return;
				}

				std::string serialNumber;
				for(int i = 3; i < 13; i++)
					serialNumber += std::to_string(packet->payload()->at(i));
				if(_peers.find(packet->senderAddress()) == _peers.end())
				{
					_bidCoSQueue->peer = createPeer(packet->senderAddress(), packet->payload()->at(9), (HMDeviceTypes)((packet->payload()->at(10) << 8) + packet->payload()->at(11)), serialNumber, 0, 0);
					_bidCoSQueue->peer.xmlrpcDevice = device;
					_bidCoSQueue->peer.initializeCentralConfig();
					for(std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<double, XMLRPCConfigurationParameter>>>::iterator i = _bidCoSQueue->peer.configCentral.begin(); i != _bidCoSQueue->peer.configCentral.end(); ++i)
					{
						//Set peersReceived to false for all channels
						_bidCoSQueue->peer.peersReceived[i->first] = false;
					}
					peer = &_bidCoSQueue->peer;
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
				BidCoSPacket configPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload);
				_bidCoSQueue->push(configPacket);
				_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<int32_t, int32_t>>()));
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
				configPacket = BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload);
				_bidCoSQueue->push(configPacket);
				_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<int32_t, int32_t>>()));
				payload.clear();
				_messageCounter[0]++;

				//END_CONFIG
				payload.push_back(0);
				payload.push_back(0x06);
				configPacket = BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload);
				_bidCoSQueue->push(configPacket);
				_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<int32_t, int32_t>>()));
				payload.clear();
				_messageCounter[0]++;

				if(peer->pendingBidCoSQueue.get() == nullptr) peer->pendingBidCoSQueue = shared_ptr<BidCoSQueue>(new BidCoSQueue(BidCoSQueueType::DEFAULT));
				if(_peers.find(packet->senderAddress()) == _peers.end()) //Only request config when peer is not already paired to central
				{
					for(std::unordered_map<int32_t, std::unordered_map<int32_t, std::unordered_map<double, XMLRPCConfigurationParameter>>>::iterator i = peer->configCentral.begin(); i != peer->configCentral.end(); ++i)
					{
						int32_t channel = i->first;
						//Walk through all lists to request master config if necessary
						for(std::unordered_map<int32_t, std::unordered_map<double, XMLRPCConfigurationParameter>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
						{
							int32_t list = j->first;
							//Find out if there are items not received yet
							bool allEntriesReceived = true;
							for(std::unordered_map<double, XMLRPCConfigurationParameter>::iterator k = j->second.begin(); k != j->second.end(); ++k)
							{
								if(!k->second.initialized)
								{
									allEntriesReceived = false;
									break;
								}
							}
							if(allEntriesReceived) continue;
							payload.push_back(channel);
							payload.push_back(0x04);
							payload.push_back(0);
							payload.push_back(0);
							payload.push_back(0);
							payload.push_back(0);
							payload.push_back(list);
							BidCoSPacket configPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload);
							peer->pendingBidCoSQueue->push(configPacket);
							peer->pendingBidCoSQueue->push(_messages->find(DIRECTIONIN, 0x10, std::vector<std::pair<int32_t, int32_t>>()));
							payload.clear();
							_messageCounter[0]++;
						}
						//Request peers if not received yet
						if(peer->xmlrpcDevice->channels.at(channel).getParameterSet(XMLRPC::ParameterSet::Type::link) != nullptr && !peer->peersReceived[channel])
						{
							payload.push_back(channel);
							payload.push_back(0x03);
							BidCoSPacket configPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload);
							peer->pendingBidCoSQueue->push(configPacket);
							peer->pendingBidCoSQueue->push(_messages->find(DIRECTIONIN, 0x10, std::vector<std::pair<int32_t, int32_t>>()));
							payload.clear();
							_messageCounter[0]++;
						}
					}
				}
			}
			else
			{
				if(_peers.find(packet->senderAddress()) == _peers.end()) return;
				newBidCoSQueue(BidCoSQueueType::DEFAULT);
				peer = &_peers[packet->senderAddress()];
				if(peer->pendingBidCoSQueue.get() == nullptr) peer->pendingBidCoSQueue.reset(new BidCoSQueue(BidCoSQueueType::DEFAULT));
			}
			if(peer == nullptr)
			{
				if(GD::debugLevel >= 2) cout << "Error: Peer is nullptr. This shouldn't have happened. Something went very wrong." << endl;
				return;
			}
			peer->pendingBidCoSQueue->noSending = true;
			_bidCoSQueue->push(peer->pendingBidCoSQueue); //This pushes the just generated queue and the already existent pending queue onto the queue
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

void HomeMaticCentral::handleConfigParamResponse(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		if(_bidCoSQueue.get() == nullptr) return;
		if(_sentPacket.payload()->at(1) == 0x03) //Peer request
		{
			for(uint32_t i = 1; i < packet->payload()->size(); i += 4)
			{
				int32_t peerAddress = (packet->payload()->at(i) << 16) + (packet->payload()->at(i + 1) << 8) + packet->payload()->at(i + 2);
				int32_t peerChannel = packet->payload()->at(i + 3);
				PairedPeer peer;
				peer.address = peerAddress;
				_peers[packet->senderAddress()].peers[peerChannel].push_back(peer);
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
				if(peer->xmlrpcDevice->channels[channel].getParameterSet(XMLRPC::ParameterSet::Type::master) == nullptr)
				{
					if(GD::debugLevel >= 2) cout << "Error: Received config for non existant parameter set." << endl;
				}
				else
				{
					std::vector<XMLRPC::Parameter*> packetParameters = peer->xmlrpcDevice->channels[channel].getParameterSet(XMLRPC::ParameterSet::Type::master)->getIndices(startIndex, endIndex);
					for(std::vector<XMLRPC::Parameter*>::iterator i = packetParameters.begin(); i != packetParameters.end(); ++i)
					{
						peer->configCentral[channel][list][(*i)->index].value = packet->getPosition((*i)->index, (*i)->size, (*i)->isSigned);
						peer->configCentral[channel][list][(*i)->index].initialized = true;
					}
				}
			}
			else
			{
				for(uint32_t i = 1; i < packet->payload()->size() - 2; i += 2)
				{
					_peers[packet->senderAddress()].configCentral[_sentPacket.payload()->at(0)][_sentPacket.payload()->at(6)][packet->payload()->at(i)].value = packet->payload()->at(i + 1);
					_peers[packet->senderAddress()].configCentral[_sentPacket.payload()->at(0)][_sentPacket.payload()->at(6)][packet->payload()->at(i)].initialized = true;
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
			_bidCoSQueue->pop(); //Messages are not popped by default.
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

void HomeMaticCentral::handleAck(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		if(_bidCoSQueue.get() == nullptr) return;
		if(_bidCoSQueue->getQueueType() == BidCoSQueueType::PAIRING)
		{
			if(_sentPacket.messageType() == 0x01 && _sentPacket.payload()->at(0) == 0x00 && _sentPacket.payload()->at(1) == 0x06)
			{
				if(_peers.find(packet->senderAddress()) == _peers.end())
				{
					_peers[_bidCoSQueue->peer.address] = _bidCoSQueue->peer;
					cout << "Added device 0x" << std::hex << _bidCoSQueue->peer.address << std::dec << endl;
				}
			}
		}
		else if(_bidCoSQueue->getQueueType() == BidCoSQueueType::UNPAIRING)
		{
			if(_peers.find(packet->senderAddress()) != _peers.end()) _peers.erase(_peers.find(packet->senderAddress()));
		}
		_bidCoSQueue->pop(); //Messages are not popped by default.
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

