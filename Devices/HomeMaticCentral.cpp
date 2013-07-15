#include "HomeMaticCentral.h"
#include "../GD.h"
#include "../HelperFunctions.h"

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
	if(_pairingModeThread.joinable())
	{
		_stopPairingModeThread = true;
		_pairingModeThread.join();
	}
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
	/********** TEST - DON'T FORGET TO SWITCH BACK COMMENTS **********/
	//_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x10, this, ACCESSPAIREDTOSENDER, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigParamResponse)));
	/********** TEST END **********/

	_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x3F, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleTimeRequest)));
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

void HomeMaticCentral::worker()
{
	std::chrono::milliseconds sleepingTime(7000);
	int32_t lastPeer;
	lastPeer = 0;
	while(!_stopWorkerThread)
	{
		try
		{
			std::this_thread::sleep_for(sleepingTime);
			if(!_peers.empty())
			{
				if(lastPeer == 0) lastPeer = _peers.begin()->first;
				_peers.at(lastPeer)->saveToDatabase(_address);
				std::unordered_map<int32_t, std::shared_ptr<Peer>>::iterator peer = _peers.find(lastPeer);
				if(peer != _peers.end())
				{
					peer++;
					if((peer == _peers.end() || !peer->second) && !_peers.empty())
					{
						peer = _peers.begin();
						lastPeer = peer->first;
					}
				}
				else if(!_peers.empty())
				{
					peer = _peers.begin();
					lastPeer = peer->first;
				}
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
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::DEFAULT, deviceAddress);
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

std::shared_ptr<Peer> HomeMaticCentral::createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, int32_t index23)
{
	std::shared_ptr<Peer> peer(new Peer());
	peer->address = address;
	peer->firmwareVersion = firmwareVersion;
	peer->deviceType = deviceType;
	peer->setSerialNumber(serialNumber);
	peer->remoteChannel = remoteChannel;
	peer->messageCounter = messageCounter;
	peer->rpcDevice = GD::rpcDevices.find(deviceType, firmwareVersion, index23);
	if(!peer->rpcDevice) peer->rpcDevice = GD::rpcDevices.find(HMDeviceType::getString(deviceType), index23);
	if(!peer->rpcDevice) return std::shared_ptr<Peer>();
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
			unpair(address, true);
		}
	}
	else if(command == "enable aes")
	{
		std::string input;
		std::cout << "Please enter the devices address: ";
		int32_t address = getHexInput();
		if(_peers.find(address) == _peers.end()) std::cout << "This device is not paired to this central." << std::endl;
		else
		{
			std::cout << "Enabling AES for device device 0x" << std::hex << address << std::dec << std::endl;
			sendEnableAES(address, 1);
		}
	}
	else if(command == "list peers")
	{
		for(std::unordered_map<int32_t, std::shared_ptr<Peer>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			std::cout << "Address: 0x" << std::hex << i->second->address << "\tSerial number: " << i->second->getSerialNumber() << "\tDevice type: " << (uint32_t)i->second->deviceType << std::endl << std::dec;
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

int32_t HomeMaticCentral::getUniqueAddress(int32_t seed)
{
	uint32_t i = 0;
	while((_peers.find(seed) != _peers.end() || GD::devices.get(seed)) && i++ < 200000)
	{
		seed += 9345;
	}
	return seed;
}

std::string HomeMaticCentral::getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber)
{
	if(seedPrefix.size() > 3) throw Exception("Length of seedPrefix is too large.");
	uint32_t i = 0;
	int32_t numberSize = 10 - seedPrefix.size();
	std::ostringstream stringstream;
	stringstream << seedPrefix << std::setw(numberSize) << std::setfill('0') << std::dec << seedNumber;
	std::string temp2 = stringstream.str();
	while((_peersBySerial.find(temp2) != _peersBySerial.end() || GD::devices.get(temp2)) && i++ < 100000)
	{
		stringstream.str(std::string());
		stringstream.clear();
		seedNumber += 73;
		std::ostringstream stringstream;
		stringstream << "VCD" << std::setw(numberSize) << std::setfill('0') << std::dec << seedNumber;
		temp2 = stringstream.str();
	}
	return temp2;
}

void HomeMaticCentral::addHomegearFeaturesHMCCVD(std::shared_ptr<Peer> peer)
{
	try
	{
		int32_t hmcctcAddress = getUniqueAddress((0x39 << 16) + (peer->address & 0xFF00) + (peer->address & 0xFF));
		if(peer->hasPeers(1) && !peer->getPeer(1, hmcctcAddress)) return; //Already linked to a HM-CC-TC
		std::string temp = peer->getSerialNumber().substr(3);
		std::string serialNumber = getUniqueSerialNumber("VCD", HelperFunctions::getNumber(temp));
		GD::devices.add(new HM_CC_TC(serialNumber, hmcctcAddress));
		std::shared_ptr<HomeMaticDevice> tc = GD::devices.get(hmcctcAddress);
		tc->addPeer(peer);
		std::shared_ptr<BasicPeer> hmcctc(new BasicPeer());
		hmcctc->address = tc->address();
		hmcctc->serialNumber = tc->serialNumber();
		hmcctc->channel = 1;
		hmcctc->hidden = true;
		peer->addPeer(1, hmcctc);
		peer->saveToDatabase(_address);

		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, peer->address);
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		std::vector<uint8_t> payload;
		//CONFIG_ADD_PEER
		payload.push_back(0x01);
		payload.push_back(0x01);
		payload.push_back(hmcctcAddress >> 16);
		payload.push_back((hmcctcAddress >> 8) & 0xFF);
		payload.push_back(hmcctcAddress & 0xFF);
		payload.push_back(0x02);
		payload.push_back(0);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, peer->address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		pendingQueue->serviceMessages = peer->serviceMessages;
		peer->serviceMessages->configPending = true;
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

void HomeMaticCentral::addHomegearFeatures(std::shared_ptr<Peer> peer)
{
	try
	{
		if(peer->deviceType == HMDeviceTypes::HMCCVD) addHomegearFeaturesHMCCVD(peer);
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

void HomeMaticCentral::deletePeer(int32_t address)
{
	if(_peers.find(address) != _peers.end())
	{
		std::shared_ptr<Peer> peer(_peers[address]);
		std::shared_ptr<RPC::RPCVariable> deviceAddresses(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		deviceAddresses->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(peer->getSerialNumber())));
		for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
		{
			deviceAddresses->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(peer->getSerialNumber() + ":" + std::to_string(i->first))));
		}
		GD::rpcClient.broadcastDeleteDevices(deviceAddresses);
		if(_peersBySerial.find(peer->getSerialNumber()) != _peersBySerial.end()) _peersBySerial.erase(peer->getSerialNumber());
		peer->deleteFromDatabase(_address);
		peer->deletePairedVirtualDevices();
		_peers.erase(address);
		std::cout << "Removed device 0x" << std::hex << address << std::dec << std::endl;
	}
}

void HomeMaticCentral::reset(int32_t address, bool defer)
{
	try
	{
		if(_peers.find(address) == _peers.end()) return;
		Peer* peer = _peers[address].get();
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::UNPAIRING, address);
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::UNPAIRING));
		pendingQueue->noSending = true;

		std::vector<uint8_t> payload;

		//CONFIG_START
		payload.push_back(0x04);
		payload.push_back(0x00);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x11, _address, address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		if(defer)
		{
			pendingQueue->serviceMessages = peer->serviceMessages;
			peer->serviceMessages->configPending = true;
			while(!peer->pendingBidCoSQueues->empty()) peer->pendingBidCoSQueues->pop();
			peer->pendingBidCoSQueues->push(pendingQueue);
			queue->push(peer->pendingBidCoSQueues);
		}
		else queue->push(pendingQueue, true, true);
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

void HomeMaticCentral::unpair(int32_t address, bool defer)
{
	try
	{
		if(_peers.find(address) == _peers.end()) return;
		Peer* peer = _peers[address].get();
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::UNPAIRING, address);
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

		if(defer)
		{
			pendingQueue->serviceMessages = peer->serviceMessages;
			peer->serviceMessages->configPending = true;
			while(!peer->pendingBidCoSQueues->empty()) peer->pendingBidCoSQueues->pop();
			peer->pendingBidCoSQueues->push(pendingQueue);
			queue->push(peer->pendingBidCoSQueues);
		}
		else queue->push(pendingQueue, true, true);
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

void HomeMaticCentral::sendEnableAES(int32_t address, int32_t channel)
{
	if(_peers.find(address) == _peers.end()) return;
	std::shared_ptr<Peer> peer = _peers[address];

	std::vector<uint8_t> payload;

	std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::DEFAULT));
	pendingQueue->noSending = true;

	//CONFIG_START
	payload.push_back((uint8_t)channel);
	payload.push_back(0x05);
	payload.push_back(0);
	payload.push_back(0);
	payload.push_back(0);
	payload.push_back(0);
	payload.push_back(0x01);
	std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload));
	pendingQueue->push(configPacket);
	pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
	payload.clear();
	_messageCounter[0]++;

	//CONFIG_WRITE_INDEX
	payload.push_back((uint8_t)channel);
	payload.push_back(0x08);
	payload.push_back(0x08);
	payload.push_back(0x01);
	configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload));
	pendingQueue->push(configPacket);
	pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
	payload.clear();
	_messageCounter[0]++;

	//END_CONFIG
	payload.push_back((uint8_t)channel);
	payload.push_back(0x06);
	configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload));
	pendingQueue->push(configPacket);
	pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
	payload.clear();
	_messageCounter[0]++;

	peer->pendingBidCoSQueues->push(pendingQueue);
	std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::DEFAULT, address);
	queue->push(peer->pendingBidCoSQueues);
}

void HomeMaticCentral::handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(packet->destinationAddress() != 0) return;
		Peer* peer = nullptr;
		std::vector<uint8_t> payload;

		std::shared_ptr<BidCoSQueue> queue;
		if(_pairing)
		{
			queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::PAIRING, packet->senderAddress());

			std::string serialNumber;
			for(uint32_t i = 3; i < 13; i++)
				serialNumber.push_back((char)packet->payload()->at(i));
			if(_peers.find(packet->senderAddress()) == _peers.end())
			{
				queue->peer = createPeer(packet->senderAddress(), packet->payload()->at(0), (HMDeviceTypes)((packet->payload()->at(1) << 8) + packet->payload()->at(2)), serialNumber, 0, 0, packet->payload()->at(16));
				queue->peer->initializeCentralConfig();
				peer = queue->peer.get();
			}
			else peer = _peers[packet->senderAddress()].get();

			if(!peer->rpcDevice)
			{
				if(GD::debugLevel >= 3) std::cout << "Warning: Device type not supported. Sender address 0x" << std::hex << packet->senderAddress() << std::dec << "." << std::endl;
				return;
			}

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
			std::shared_ptr<RPC::Parameter> internalKeysVisible = peer->rpcDevice->parameterSet->getParameter("INTERNAL_KEYS_VISIBLE");
			if(internalKeysVisible) payload.push_back(internalKeysVisible->getBytes(1) | 0x01);
			else payload.push_back(0x01);
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

			std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::CONFIG));
			pendingQueue->noSending = true;
			if((peer->rpcDevice->rxModes & RPC::Device::RXModes::Enum::config) && _peers.find(packet->senderAddress()) == _peers.end()) //Only request config when peer is not already paired to central
			{
				pendingQueue->serviceMessages = peer->serviceMessages;
				peer->serviceMessages->configPending = true;
				for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
				{
					int32_t channel = i->first;
					//Walk through all lists to request master config if necessary
					if(peer->rpcDevice->channels.at(channel)->parameterSets.find(RPC::ParameterSet::Type::Enum::master) != peer->rpcDevice->channels.at(channel)->parameterSets.end())
					{
						std::shared_ptr<RPC::ParameterSet> masterSet = peer->rpcDevice->channels.at(channel)->parameterSets[RPC::ParameterSet::Type::Enum::master];
						for(std::map<uint32_t, uint32_t>::iterator k = masterSet->lists.begin(); k != masterSet->lists.end(); ++k)
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

void HomeMaticCentral::sendRequestConfig(int32_t address, uint8_t localChannel, uint8_t list, int32_t remoteAddress, uint8_t remoteChannel)
{
	try
	{
		if(_peers.find(address) == _peers.end()) return;
		Peer* peer = _peers[address].get();
		bool oldQueue = true;
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(address);
		if(!queue)
		{
			oldQueue = false;
			queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, address);
		}
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		std::vector<uint8_t> payload;

		//CONFIG_WRITE_INDEX
		payload.push_back(localChannel);
		payload.push_back(0x04);
		payload.push_back(remoteAddress >> 16);
		payload.push_back((remoteAddress >> 8) & 0xFF);
		payload.push_back(remoteAddress & 0xFF);
		payload.push_back(remoteChannel);
		payload.push_back(list);
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, address, payload));
		pendingQueue->push(packet);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x10, std::vector<std::pair<uint32_t, int32_t>>()));
		payload.clear();
		_messageCounter[0]++;

		pendingQueue->serviceMessages = peer->serviceMessages;
		peer->serviceMessages->configPending = true;
		peer->pendingBidCoSQueues->push(pendingQueue);
		if(!oldQueue) queue->push(peer->pendingBidCoSQueues);
		else if(queue->pendingQueuesEmpty()) queue->push(peer->pendingBidCoSQueues);
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
		/********** TEST - DON'T FORGET TO REMOVE SOURCE CODE **********/
		/*if(packet->payload()->size() == 4 && packet->payload()->at(0) == 0x06 && packet->payload()->at(1) == 0x01 && packet->payload()->at(2) == 0xC8)
		{
			_messageCounter[0] = 0x2F;
			if(_peers.find(packet->senderAddress()) == _peers.end()) return;
			std::shared_ptr<Peer> peer = _peers[packet->senderAddress()];

			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::DEFAULT, packet->senderAddress());

			std::vector<uint8_t> payload;
			//CONFIG_START
			payload.push_back(0x01);
			payload.push_back(0x0E);
			std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x10, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();

			//CONFIG_WRITE_INDEX
			payload.push_back(0x04);
			payload.push_back(0x0E);
			payload.push_back(0x98);
			payload.push_back(0x3E);
			payload.push_back(0x70);
			payload.push_back(0x11);
			payload.push_back(0x2C);
			payload.push_back(0);
			configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x02, _address, packet->senderAddress(), payload));
			queue->push(configPacket);
			//queue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			return;
		}
		else
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(packet->senderAddress());
			if(!queue) return;
			queue->pop();
			return;
		}*/
		/********** TEST END **********/
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(!queue) return;
		std::shared_ptr<BidCoSPacket> sentPacket(_sentPackets.get(packet->senderAddress()));
		bool multiplePackets = false;
		bool multiPacketEnd = false;
		//Peer request
		if(sentPacket && sentPacket->payload()->at(1) == 0x03 && sentPacket->payload()->at(0) == packet->payload()->at(0))
		{
			Peer* peer = _peers[packet->senderAddress()].get();
			for(uint32_t i = 1; i < packet->payload()->size() - 1; i += 4)
			{
				int32_t peerAddress = (packet->payload()->at(i) << 16) + (packet->payload()->at(i + 1) << 8) + packet->payload()->at(i + 2);
				if(peerAddress != 0)
				{
					std::shared_ptr<BasicPeer> newPeer(new BasicPeer());
					newPeer->address = peerAddress;
					newPeer->channel = packet->payload()->at(i + 3);
					int32_t localChannel = sentPacket->payload()->at(0);
					_peers[packet->senderAddress()]->addPeer(localChannel, newPeer);
					if(peer->rpcDevice->channels.find(localChannel) == peer->rpcDevice->channels.end()) continue;
					std::shared_ptr<RPC::DeviceChannel> channel = peer->rpcDevice->channels[localChannel];
					if(channel->parameterSets.find(RPC::ParameterSet::Type::Enum::link) == channel->parameterSets.end()) continue;
					std::shared_ptr<RPC::ParameterSet> parameterSet = channel->parameterSets[RPC::ParameterSet::Type::Enum::link];
					if(parameterSet->parameters.empty()) continue;
					for(std::map<uint32_t, uint32_t>::iterator k = parameterSet->lists.begin(); k != parameterSet->lists.end(); ++k)
					{
						sendRequestConfig(peer->address, localChannel, k->first, newPeer->address, newPeer->channel);
					}
				}
			}
			if(packet->controlByte() & 0x20) sendOK(packet->messageCounter(), peer->address);
		}
		else if(sentPacket && sentPacket->payload()->at(1) == 0x04) //Config request
		{
			int32_t channel = sentPacket->payload()->at(0);
			int32_t list = sentPacket->payload()->at(6);
			int32_t remoteAddress = (sentPacket->payload()->at(2) << 16) + (sentPacket->payload()->at(3) << 8) + sentPacket->payload()->at(4);
			int32_t remoteChannel = sentPacket->payload()->at(5);
			RPC::ParameterSet::Type::Enum type = (remoteAddress != 0) ? RPC::ParameterSet::Type::link : RPC::ParameterSet::Type::master;
			Peer* peer = _peers[packet->senderAddress()].get();
			std::shared_ptr<RPC::RPCVariable> parametersToEnforce;
			if(!peer->pairingComplete) parametersToEnforce.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
			if((packet->controlByte() & 0x20) && (packet->payload()->at(0) == 3)) multiplePackets = true;
			//Some devices have a payload size of 3
			if(multiplePackets && packet->payload()->size() == 3 && packet->payload()->at(1) == 0 && packet->payload()->at(2) == 0) multiPacketEnd = true;
			//And some a payload size of 2
			if(multiplePackets && packet->payload()->size() == 2 && packet->payload()->at(1) == 0) multiPacketEnd = true;
			if(multiplePackets && !multiPacketEnd)
			{
				int32_t startIndex = packet->payload()->at(1);
				int32_t endIndex = startIndex + packet->payload()->size() - 3;
				if(peer->rpcDevice->channels[channel]->parameterSets.find(type) == peer->rpcDevice->channels[channel]->parameterSets.end() || !peer->rpcDevice->channels[channel]->parameterSets[type])
				{
					if(GD::debugLevel >= 2) std::cout << "Error: Received config for non existant parameter set." << std::endl;
				}
				else
				{
					std::vector<std::shared_ptr<RPC::Parameter>> packetParameters = peer->rpcDevice->channels[channel]->parameterSets[type]->getIndices(startIndex, endIndex, list);
					for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator i = packetParameters.begin(); i != packetParameters.end(); ++i)
					{
						if(!(*i)->id.empty())
						{
							double position = ((*i)->physicalParameter->index - startIndex) + 2 + 9;
							if(type == RPC::ParameterSet::Type::master)
							{
								peer->configCentral[channel][(*i)->id].value = packet->getPosition(position, (*i)->physicalParameter->size, false);
								if(!peer->pairingComplete && (*i)->logicalParameter->enforce)
								{
									parametersToEnforce->structValue->push_back((*i)->logicalParameter->getEnforceValue());
									parametersToEnforce->structValue->back()->name = (*i)->id;
								}
								if(GD::debugLevel >= 5) std::cout << "Parameter " << (*i)->id << " of device 0x" << std::hex << peer->address << std::dec << " at index " << std::to_string((*i)->physicalParameter->index) << " and packet index " << std::to_string(position) << " with size " << std::to_string((*i)->physicalParameter->size) << " was set to " << peer->configCentral[channel][(*i)->id].value << std::endl;
							}
							else if(peer->getPeer(channel, remoteAddress, remoteChannel))
							{
								peer->linksCentral[channel][remoteAddress][remoteChannel][(*i)->id].value = packet->getPosition(position, (*i)->physicalParameter->size, false);
								if(GD::debugLevel >= 5) std::cout << "Parameter " << (*i)->id << " of device 0x" << std::hex << peer->address << std::dec << " at index " << std::to_string((*i)->physicalParameter->index) << " and packet index " << std::to_string(position) << " with size " << std::to_string((*i)->physicalParameter->size) << " was set to " << peer->linksCentral[channel][remoteAddress][remoteChannel][(*i)->id].value << std::endl;
							}
						}
						else if(GD::debugLevel >= 2) std::cout << "Error: Device tried to set parameter without id. Device: " << std::hex << peer->address << std::dec << " Serial number: " << peer->getSerialNumber() << " Channel: " << channel << " List: " << (*i)->physicalParameter->list << " Parameter index: " << (*i)->index << std::endl;
					}
				}
			}
			else if(!multiPacketEnd)
			{
				if(peer->rpcDevice->channels[channel]->parameterSets.find(type) == peer->rpcDevice->channels[channel]->parameterSets.end() || !peer->rpcDevice->channels[channel]->parameterSets[type])
				{
					if(GD::debugLevel >= 2) std::cout << "Error: Received config for non existant parameter set." << std::endl;
				}
				else
				{
					for(uint32_t i = 1; i < packet->payload()->size() - 2; i += 2)
					{
						int32_t index = packet->payload()->at(i);
						std::vector<std::shared_ptr<RPC::Parameter>> packetParameters = peer->rpcDevice->channels[channel]->parameterSets[type]->getIndices(index, index, list);
						for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = packetParameters.begin(); j != packetParameters.end(); ++j)
						{
							if(!(*j)->id.empty())
							{
								double position = std::fmod((*j)->physicalParameter->index, 1) + 9 + i + 1;
								if(type == RPC::ParameterSet::Type::master)
								{
									peer->configCentral[channel][(*j)->id].value = packet->getPosition(position, (*j)->physicalParameter->size, false);
									if(!peer->pairingComplete && (*j)->logicalParameter->enforce)
									{
										parametersToEnforce->structValue->push_back((*j)->logicalParameter->getEnforceValue());
										parametersToEnforce->structValue->back()->name = (*j)->id;
									}
									if(GD::debugLevel >= 5) std::cout << "Parameter " << (*j)->id << " of device 0x" << std::hex << peer->address << std::dec << " at index " << std::to_string((*j)->physicalParameter->index) << " and packet index " << std::to_string(position) << " was set to " << peer->configCentral[channel][(*j)->id].value << std::endl;
								}
								else if(peer->getPeer(channel, remoteAddress, remoteChannel))
								{
									peer->linksCentral[channel][remoteAddress][remoteChannel][(*j)->id].value = packet->getPosition(position, (*j)->physicalParameter->size, false);
									if(GD::debugLevel >= 5) std::cout << "Parameter " << (*j)->id << " of device 0x" << std::hex << peer->address << std::dec << " at index " << std::to_string((*j)->physicalParameter->index) << " and packet index " << std::to_string(position) << " was set to " << peer->linksCentral[channel][remoteAddress][remoteChannel][(*j)->id].value << std::endl;
								}
							}
							else if(GD::debugLevel >= 2) std::cout << "Error: Device tried to set parameter without id. Device: " << std::hex << peer->address << std::dec << " Serial number: " << peer->getSerialNumber() << " Channel: " << channel << " List: " << (*j)->physicalParameter->list << " Parameter index: " << (*j)->index << std::endl;
						}
					}
				}
			}
			if(!peer->pairingComplete && !parametersToEnforce->structValue->empty()) peer->putParamset(channel, type, "", -1, parametersToEnforce);
		}
		if(multiplePackets && !multiPacketEnd) //Multiple config response packets
		{
			//StealthyOK does not set sentPacket
			sendStealthyOK(packet->messageCounter(), packet->senderAddress());
			//No popping from queue to stay at the same position until all response packets are received!!!
			//The popping is done with the last packet, which has no BIDI control bit set. See https://sathya.de/HMCWiki/index.php/Examples:Message_Counter
		}
		else
		{
			//Sometimes the peer requires an ok after the end packet
			if(multiPacketEnd && (packet->controlByte() & 0x20)) sendOK(packet->messageCounter(), packet->senderAddress());
			queue->pop(); //Messages are not popped by default.
		}
		if(queue->isEmpty())
		{
			if(!queue->peer) return;
			if(!queue->peer->pairingComplete)
			{
				addHomegearFeatures(queue->peer);
				queue->peer->pairingComplete = true;
			}
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
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(!queue) return;
		std::shared_ptr<BidCoSPacket> sentPacket(_sentPackets.get(packet->senderAddress()));
		if(packet->payload()->at(0) == 0x80)
		{
			if(GD::debugLevel >= 2)
			{
				if(sentPacket) std::cerr << "Error: NACK received from 0x" << std::hex << packet->senderAddress() << " in response to " << sentPacket->hexString() << "." << std::dec << std::endl;
				else std::cerr << "Error: NACK received from 0x" << std::hex << packet->senderAddress() << std::endl;
			}
			queue->pop(); //Otherwise the queue will persist forever
			return;
		}
		if(queue->getQueueType() == BidCoSQueueType::PAIRING)
		{
			if(sentPacket && sentPacket->messageType() == 0x01 && sentPacket->payload()->at(0) == 0x00 && sentPacket->payload()->at(1) == 0x06)
			{
				if(_peers.find(packet->senderAddress()) == _peers.end())
				{
					for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = queue->peer->rpcDevice->channels.begin(); i != queue->peer->rpcDevice->channels.end(); ++i)
					{
						if(i->second->hasTeam)
						{
							std::shared_ptr<Peer> team;
							std::string serialNumber('*' + queue->peer->getSerialNumber());
							if(_peersBySerial.find(serialNumber) == _peersBySerial.end())
							{
								team = createTeam(queue->peer->address, queue->peer->deviceType, serialNumber);
								team->rpcDevice = queue->peer->rpcDevice->team;
								_peersBySerial[team->getSerialNumber()] = team;
							}
							else team = _peersBySerial['*' + queue->peer->getSerialNumber()];
							queue->peer->team.address = team->address;
							queue->peer->team.serialNumber = team->getSerialNumber();
							_peersBySerial[team->getSerialNumber()]->teamChannels.push_back(std::pair<std::string, uint32_t>(queue->peer->getSerialNumber(), i->first));
							break;
						}
					}
					_peers[queue->peer->address] = queue->peer;
					if(!queue->peer->getSerialNumber().empty()) _peersBySerial[queue->peer->getSerialNumber()] = queue->peer;
					std::cout << "Added device 0x" << std::hex << queue->peer->address << std::dec << "." << std::endl;
					std::shared_ptr<RPC::RPCVariable> deviceDescriptions(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					deviceDescriptions->arrayValue = queue->peer->getDeviceDescription();
					GD::rpcClient.broadcastNewDevices(deviceDescriptions);
				}
			}
		}
		else if(queue->getQueueType() == BidCoSQueueType::UNPAIRING)
		{
			if((sentPacket && sentPacket->messageType() == 0x01 && sentPacket->payload()->at(0) == 0x00 && sentPacket->payload()->at(1) == 0x06) || (sentPacket && sentPacket->messageType() == 0x11 && sentPacket->payload()->at(0) == 0x04 && sentPacket->payload()->at(1) == 0x00))
			{
				deletePeer(packet->senderAddress());
			}
		}
		queue->pop(); //Messages are not popped by default.
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

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::addLink(std::string senderSerialNumber, int32_t senderChannelIndex, std::string receiverSerialNumber, int32_t receiverChannelIndex, std::string name, std::string description)
{
	try
	{
		if(senderSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given receiver address is empty.");
		if(_peersBySerial.find(senderSerialNumber) == _peersBySerial.end()) return RPC::RPCVariable::createError(-2, "Sender device not found.");
		if(_peersBySerial.find(receiverSerialNumber) == _peersBySerial.end()) return RPC::RPCVariable::createError(-2, "Receiver device not found.");
		if(senderChannelIndex < 0) senderChannelIndex = 0;
		if(receiverChannelIndex < 0) receiverChannelIndex = 0;
		std::shared_ptr<Peer> sender = _peersBySerial.at(senderSerialNumber);
		std::shared_ptr<Peer> receiver = _peersBySerial.at(receiverSerialNumber);
		if(sender->rpcDevice->channels.find(senderChannelIndex) == sender->rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Sender channel not found.");
		if(receiver->rpcDevice->channels.find(receiverChannelIndex) == receiver->rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Receiver channel not found.");
		std::shared_ptr<RPC::DeviceChannel> senderChannel = sender->rpcDevice->channels.at(senderChannelIndex);
		std::shared_ptr<RPC::DeviceChannel> receiverChannel = receiver->rpcDevice->channels.at(receiverChannelIndex);
		if(senderChannel->linkRoles->sourceNames.size() == 0 || receiverChannel->linkRoles->targetNames.size() == 0) RPC::RPCVariable::createError(-6, "Link not supported.");
		bool validLink = false;
		for(std::vector<std::string>::iterator i = senderChannel->linkRoles->sourceNames.begin(); i != senderChannel->linkRoles->sourceNames.end(); ++i)
		{
			for(std::vector<std::string>::iterator j = receiverChannel->linkRoles->targetNames.begin(); j != receiverChannel->linkRoles->targetNames.end(); ++j)
			{
				if(*i == *j)
				{
					validLink = true;
					break;
				}
			}
			if(validLink) break;
		}

		std::shared_ptr<BasicPeer> senderPeer(new BasicPeer());
		senderPeer->address = sender->address;
		senderPeer->channel = senderChannelIndex;
		senderPeer->serialNumber = sender->getSerialNumber();
		senderPeer->linkDescription = description;
		senderPeer->linkName = name;

		std::shared_ptr<BasicPeer> receiverPeer(new BasicPeer());
		receiverPeer->address = receiver->address;
		receiverPeer->channel = receiverChannelIndex;
		receiverPeer->serialNumber = receiver->getSerialNumber();
		receiverPeer->linkDescription = description;
		receiverPeer->linkName = name;

		sender->addPeer(senderChannelIndex, receiverPeer);

		receiver->addPeer(receiverChannelIndex, senderPeer);

		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, sender->address);
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		std::vector<uint8_t> payload;
		//CONFIG_ADD_PEER
		payload.push_back(senderChannelIndex);
		payload.push_back(0x01);
		payload.push_back(receiver->address >> 16);
		payload.push_back((receiver->address >> 8) & 0xFF);
		payload.push_back(receiver->address & 0xFF);
		payload.push_back(receiverChannelIndex);
		payload.push_back(0);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, sender->address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		pendingQueue->serviceMessages = sender->serviceMessages;
		sender->serviceMessages->configPending = true;
		sender->pendingBidCoSQueues->push(pendingQueue);

		if(!receiverChannel->enforceLinks.empty() && senderChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::link) != senderChannel->parameterSets.end())
		{
			std::shared_ptr<RPC::ParameterSet> linkset = senderChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::link);
			std::shared_ptr<RPC::RPCVariable> paramset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
			for(std::vector<std::shared_ptr<RPC::EnforceLink>>::iterator i = receiverChannel->enforceLinks.begin(); i != receiverChannel->enforceLinks.end(); ++i)
			{
				std::shared_ptr<RPC::Parameter> parameter = linkset->getParameter((*i)->id);
				if(parameter)
				{
					paramset->structValue->push_back((*i)->getValue((RPC::RPCVariableType)parameter->logicalParameter->type));
					paramset->structValue->back()->name = (*i)->id;
				}
			}
			//putParamset pushes the packets on pendingQueues, but does not send immediately
			sender->putParamset(senderChannelIndex, RPC::ParameterSet::Type::Enum::link, receiverSerialNumber, receiverChannelIndex, paramset, true);
		}

		queue->push(sender->pendingBidCoSQueues);


		queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, receiver->address);
		pendingQueue.reset(new BidCoSQueue(BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		payload.clear();
		//CONFIG_ADD_PEER
		payload.push_back(receiverChannelIndex);
		payload.push_back(0x01);
		payload.push_back(sender->address >> 16);
		payload.push_back((sender->address >> 8) & 0xFF);
		payload.push_back(sender->address & 0xFF);
		payload.push_back(senderChannelIndex);
		payload.push_back(0);
		configPacket.reset(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, receiver->address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		pendingQueue->serviceMessages = receiver->serviceMessages;
		receiver->serviceMessages->configPending = true;
		receiver->pendingBidCoSQueues->push(pendingQueue);

		if(!senderChannel->enforceLinks.empty() && receiverChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::link) != receiverChannel->parameterSets.end())
		{
			std::shared_ptr<RPC::ParameterSet> linkset = receiverChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::link);
			std::shared_ptr<RPC::RPCVariable> paramset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
			for(std::vector<std::shared_ptr<RPC::EnforceLink>>::iterator i = senderChannel->enforceLinks.begin(); i != senderChannel->enforceLinks.end(); ++i)
			{
				std::shared_ptr<RPC::Parameter> parameter = linkset->getParameter((*i)->id);
				if(parameter)
				{
					paramset->structValue->push_back((*i)->getValue((RPC::RPCVariableType)parameter->logicalParameter->type));
					paramset->structValue->back()->name = (*i)->id;
				}
			}
			//putParamset pushes the packets on pendingQueues, but does not send immediately
			receiver->putParamset(receiverChannelIndex, RPC::ParameterSet::Type::Enum::link, senderSerialNumber, senderChannelIndex, paramset, true);
		}

		queue->push(receiver->pendingBidCoSQueues);
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::removeLink(std::string senderSerialNumber, int32_t senderChannelIndex, std::string receiverSerialNumber, int32_t receiverChannelIndex)
{
	try
	{
		if(senderSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given receiver address is empty.");
		if(_peersBySerial.find(senderSerialNumber) == _peersBySerial.end()) return RPC::RPCVariable::createError(-2, "Sender device not found.");
		if(_peersBySerial.find(receiverSerialNumber) == _peersBySerial.end()) return RPC::RPCVariable::createError(-2, "Receiver device not found.");
		if(senderChannelIndex < 0) senderChannelIndex = 0;
		if(receiverChannelIndex < 0) receiverChannelIndex = 0;
		std::shared_ptr<Peer> sender = _peersBySerial.at(senderSerialNumber);
		std::shared_ptr<Peer> receiver = _peersBySerial.at(receiverSerialNumber);
		if(sender->rpcDevice->channels.find(senderChannelIndex) == sender->rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Sender channel not found.");
		if(receiver->rpcDevice->channels.find(receiverChannelIndex) == receiver->rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Receiver channel not found.");
		if(!sender->getPeer(senderChannelIndex, receiver->address) && !receiver->getPeer(receiverChannelIndex, sender->address)) return RPC::RPCVariable::createError(-6, "Devices are not paired to each other.");

		sender->removePeer(senderChannelIndex, receiver->address);
		receiver->removePeer(receiverChannelIndex, sender->address);

		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, sender->address);
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		std::vector<uint8_t> payload;
		//CONFIG_ADD_PEER
		payload.push_back(senderChannelIndex);
		payload.push_back(0x02);
		payload.push_back(receiver->address >> 16);
		payload.push_back((receiver->address >> 8) & 0xFF);
		payload.push_back(receiver->address & 0xFF);
		payload.push_back(receiverChannelIndex);
		payload.push_back(0);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, sender->address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		pendingQueue->serviceMessages = sender->serviceMessages;
		sender->serviceMessages->configPending = true;
		sender->pendingBidCoSQueues->push(pendingQueue);
		queue->push(sender->pendingBidCoSQueues);

		queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, receiver->address);
		pendingQueue.reset(new BidCoSQueue(BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		payload.clear();
		//CONFIG_ADD_PEER
		payload.push_back(receiverChannelIndex);
		payload.push_back(0x02);
		payload.push_back(sender->address >> 16);
		payload.push_back((sender->address >> 8) & 0xFF);
		payload.push_back(sender->address & 0xFF);
		payload.push_back(senderChannelIndex);
		payload.push_back(0);
		configPacket.reset(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, receiver->address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		pendingQueue->serviceMessages = receiver->serviceMessages;
		receiver->serviceMessages->configPending = true;
		receiver->pendingBidCoSQueues->push(pendingQueue);
		queue->push(receiver->pendingBidCoSQueues);
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::deleteDevice(std::string serialNumber, int32_t flags)
{
	try
	{
		if(_peersBySerial.find(serialNumber) == _peersBySerial.end()) return RPC::RPCVariable::createError(-2, "Unknown device.");
		std::shared_ptr<Peer> peer = _peersBySerial[serialNumber];

		bool defer = flags & 0x04;
		bool force = flags & 0x02;
		//Reset
		if(flags & 0x01) reset(peer->address, defer);
		else unpair(peer->address, defer);
		//Force delete
		if(force) deletePeer(peer->address);

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		if(!defer && !force && _peersBySerial.find(serialNumber) != _peersBySerial.end()) return RPC::RPCVariable::createError(-1, "No answer from device.");

		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::listDevices()
{
	return listDevices(std::shared_ptr<std::map<std::string, int32_t>>());
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::listDevices(std::shared_ptr<std::map<std::string, int32_t>> knownDevices)
{
	try
	{
		std::shared_ptr<RPC::RPCVariable> array(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));

		for(std::unordered_map<std::string, std::shared_ptr<Peer>>::iterator i = _peersBySerial.begin(); i != _peersBySerial.end(); ++i)
		{
			if(knownDevices && knownDevices->find(i->first) != knownDevices->end()) continue; //only add unknown devices
			std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> descriptions = i->second->getDeviceDescription();
			if(!descriptions) continue;
			for(std::vector<std::shared_ptr<RPC::RPCVariable>>::iterator j = descriptions->begin(); j != descriptions->end(); ++j)
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getDeviceDescription(std::string serialNumber, int32_t channel)
{
	try
	{
		if(_peersBySerial.find(serialNumber) == _peersBySerial.end()) return RPC::RPCVariable::createError(-2, "Unknown device.");

		return _peersBySerial[serialNumber]->getDeviceDescription(channel);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getInstallMode()
{
	try
	{
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_timeLeftInPairingMode));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::setLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description)
{
	try
	{
		if(senderSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given receiver address is empty.");
		std::shared_ptr<RPC::RPCVariable> result1;
		std::shared_ptr<RPC::RPCVariable> result2;
		if(_peersBySerial.find(senderSerialNumber) != _peersBySerial.end())
		{
			result1 = _peersBySerial[senderSerialNumber]->setLinkInfo(senderChannel, receiverSerialNumber, receiverChannel, name, description);
		}
		if(_peersBySerial.find(receiverSerialNumber) != _peersBySerial.end())
		{
			result2 = _peersBySerial[receiverSerialNumber]->setLinkInfo(receiverChannel, senderSerialNumber, senderChannel, name, description);
		}
		if(!result1 || !result2) RPC::RPCVariable::createError(-2, "At least one device was not found.");
		if(result1->errorStruct) return result1;
		if(result2->errorStruct) return result2;
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel)
{
	try
	{
		if(senderSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given receiver address is empty.");
		if(_peersBySerial.find(senderSerialNumber) == _peersBySerial.end())
		{
			if(_peersBySerial.find(receiverSerialNumber) != _peersBySerial.end()) return _peersBySerial[receiverSerialNumber]->getLinkInfo(receiverChannel, senderSerialNumber, senderChannel);
			else return RPC::RPCVariable::createError(-2, "Both devices not found.");
		}
		else return _peersBySerial[senderSerialNumber]->getLinkInfo(senderChannel, receiverSerialNumber, receiverChannel);
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getLinkPeers(std::string serialNumber, int32_t channel)
{
	try
	{
		if(_peersBySerial.find(serialNumber) == _peersBySerial.end()) return RPC::RPCVariable::createError(-2, "Unknown device.");
		return _peersBySerial[serialNumber]->getLinkPeers(channel);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getLinks(std::string serialNumber, int32_t channel, int32_t flags)
{
	try
	{
		std::shared_ptr<RPC::RPCVariable> array(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		std::shared_ptr<RPC::RPCVariable> element(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		if(serialNumber.empty())
		{
			for(std::unordered_map<std::string, std::shared_ptr<Peer>>::iterator i = _peersBySerial.begin(); i != _peersBySerial.end(); ++i)
			{
				element = i->second->getLink(channel, flags, true);
				array->arrayValue->insert(array->arrayValue->begin(), element->arrayValue->begin(), element->arrayValue->end());
			}
		}
		else
		{
			if(_peersBySerial.find(serialNumber) == _peersBySerial.end()) return RPC::RPCVariable::createError(-2, "Unknown device.");
			element = _peersBySerial[serialNumber]->getLink(channel, flags, false);
			array->arrayValue->insert(array->arrayValue->begin(), element->arrayValue->begin(), element->arrayValue->end());
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getParamsetId(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type)
{
	try
	{
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end()) return _peersBySerial[serialNumber]->getParamsetId(channel, type);
		return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::putParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> paramset)
{
	try
	{
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end()) return _peersBySerial[serialNumber]->putParamset(channel, type, remoteSerialNumber, remoteChannel, paramset);
		return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end()) return _peersBySerial[serialNumber]->getParamset(channel, type, remoteSerialNumber, remoteChannel);
		return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getParamsetDescription(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end()) return _peersBySerial[serialNumber]->getParamsetDescription(channel, type, remoteSerialNumber, remoteChannel);
		return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getServiceMessages()
{
	try
	{
		std::shared_ptr<RPC::RPCVariable> serviceMessages(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		for(std::unordered_map<int32_t, std::shared_ptr<Peer>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			if(!i->second) continue;
			std::shared_ptr<RPC::RPCVariable> messages = i->second->getServiceMessages();
			if(!messages->arrayValue->empty()) serviceMessages->arrayValue->insert(serviceMessages->arrayValue->end(), messages->arrayValue->begin(), messages->arrayValue->end());
		}
		return serviceMessages;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getValue(std::string serialNumber, uint32_t channel, std::string valueKey)
{
	try
	{
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end()) return _peersBySerial[serialNumber]->getValue(channel, valueKey);
		return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::setValue(std::string serialNumber, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end()) return _peersBySerial[serialNumber]->setValue(channel, valueKey, value);
		return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

void HomeMaticCentral::pairingModeTimer()
{
	try
	{
		_pairing = true;
		std::cout << "Pairing mode enabled." << std::endl;
		_timeLeftInPairingMode = 60;
		int64_t startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		int64_t timePassed = 0;
		while(timePassed < 60000 && !_stopPairingModeThread)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
			timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - startTime;
			_timeLeftInPairingMode = 60 - (timePassed / 1000);
		}
		_timeLeftInPairingMode = 0;
		_pairing = false;
		std::cout << "Pairing mode disabled." << std::endl;
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

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::setInstallMode(bool on)
{
	try
	{
		_stopPairingModeThread = true;
		if(_pairingModeThread.joinable()) _pairingModeThread.join();
		_stopPairingModeThread = false;
		_timeLeftInPairingMode = 0;
		if(on)
		{
			_pairingModeThread = std::thread(&HomeMaticCentral::pairingModeTimer, this);
		}
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}
