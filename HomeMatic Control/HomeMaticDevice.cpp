#include "HomeMaticDevice.h"

HomeMaticDevice::HomeMaticDevice()
{
}

HomeMaticDevice::HomeMaticDevice(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent)
{
	if(GD::debugLevel == 5) cout << "Unserializing: " << serializedObject << endl;
	uint32_t pos = 0;
	_deviceType = (HMDeviceTypes)std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	_address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	_serialNumber = serializedObject.substr(pos, 10); pos += 10;
	_firmwareVersion = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
	_centralAddress = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	uint32_t messageCounterSize = std::stoll(serializedObject.substr(pos, 8), 0, 16) * 10; pos += 8;
	for(uint32_t i = pos; i < (pos + messageCounterSize); i += 10)
		_messageCounter[std::stoll(serializedObject.substr(i, 8), 0, 16)] = (uint8_t)std::stoll(serializedObject.substr(i + 8, 2), 0, 16);
	pos += messageCounterSize;
	uint32_t peersSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	for(uint32_t i = 0; i < peersSize; i++)
	{
		int32_t address = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		int32_t peerSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		_peers[address] = Peer(serializedObject.substr(pos, peerSize)); pos += peerSize;
	}
	uint32_t configSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	for(uint32_t i = 0; i < configSize; i++)
	{
		int32_t listIndex = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		uint32_t listSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t j = 0; j < listSize; j++)
		{
			_config[listIndex][std::stoll(serializedObject.substr(pos, 8), 0, 16)] = std::stoll(serializedObject.substr(pos + 8, 8), 0, 16); pos += 16;
		}
	}
}

HomeMaticDevice::HomeMaticDevice(std::string serialNumber, int32_t address) : _address(address), _serialNumber(serialNumber)
{
}

void HomeMaticDevice::init()
{
	try
	{
		if(_initialized) return; //Prevent running init two times
		_messages = new BidCoSMessages();
		_deviceType = HMDeviceTypes::HMUNKNOWN;

		GD::cul.addHomeMaticDevice(this);

		_messageCounter[0] = 0; //Broadcast message counter
		_messageCounter[1] = 0; //Duty cycle message counter

		if(_serialNumber.size() != 10)
		{
			throw(Exception("Serial number has an invalid length: " + _serialNumber));
		}

		setUpBidCoSMessages();
		_initialized = true;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::setUpConfig()
{
	_config[0x00][0x02] = 0; //Paired to central (Possible values: 0 - false, 1 - true);
	_config[0x00][0x0A] = 0; //First byte of central address
	_config[0x00][0x0B] = 0; //Second byte of central address
	_config[0x00][0x0C] = 0; //Third byte of central address
}

void HomeMaticDevice::setUpBidCoSMessages()
{
    _messages->add(BidCoSMessage(0x00, this, NOACCESS, FULLACCESS, &HomeMaticDevice::handlePairingRequest));

    BidCoSMessage message(0x11, this, ACCESSCENTRAL | ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleReset);
    message.addSubtype(0x00, 0x04);
    _messages->add(message);

    message = BidCoSMessage(0x01, this, ACCESSCENTRAL | ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigPeerAdd);
    message.addSubtype(0x01, 0x01);
    _messages->add(message);

    message = BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigPeerDelete);
    message.addSubtype(0x01, 0x02);
    _messages->add(message);

    message = BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigParamRequest);
    message.addSubtype(0x01, 0x04);
    _messages->add(message);

    message = BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, FULLACCESS, &HomeMaticDevice::handleConfigStart);
    message.addSubtype(0x01, 0x05);
    _messages->add(message);

    message = BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigWriteIndex);
    message.addSubtype(0x01, 0x08);
    _messages->add(message);

    message = BidCoSMessage(0x01, this, NOACCESS, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigWriteIndex);
    message.addSubtype(0x01, 0x08);
    message.addSubtype(0x02, 0x02);
    message.addSubtype(0x03, 0x01);
    _messages->add(message);

    message = BidCoSMessage(0x01, this, ACCESSCENTRAL | ACCESSDESTISME, &HomeMaticDevice::handleConfigWriteIndex);
    message.addSubtype(0x01, 0x08);
    message.addSubtype(0x02, 0x02);
    message.addSubtype(0x03, 0x00);
    _messages->add(message);

    message = BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME | ACCESSUNPAIRING, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigEnd);
    message.addSubtype(0x01, 0x06);
    _messages->add(message);

    message = BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigRequestPeers);
    message.addSubtype(0x01, 0x03);
    _messages->add(message);

    _messages->add(BidCoSMessage(0x02, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleAck));
}

HomeMaticDevice::~HomeMaticDevice()
{
	if(GD::debugLevel == 5) cout << "Removing device from CUL event queue..." << endl;
    GD::cul.removeHomeMaticDevice(this);
    if(_resetQueueThread.joinable())
	{
    	if(GD::debugLevel == 5) cout << "Stopping reset queue thread..." << endl;
		_stopResetQueueThread = true;
		_resetQueueThread.join();
	}
    if(_messages != nullptr)
    {
    	if(GD::debugLevel == 5) cout << "Deleting BidCoS messages array..." << endl;
    	delete(_messages);
    	_messages = nullptr;
    }
}

int32_t HomeMaticDevice::getHexInput()
{
	std::string input;
	cin >> input;
	int32_t intInput = -1;
	try	{ intInput = std::stoll(input, 0, 16); } catch(...) {}
    return intInput;
}

void HomeMaticDevice::handleCLICommand(std::string command)
{
	if(command == "pair")
	{
		if(pairDevice(10000)) cout << "Pairing successful." << endl; else cout << "Pairing not successful." << endl;
	}
	else if(command == "reset")
	{
		reset();
		cout << "Device reset." << endl;
	}
}

std::string HomeMaticDevice::serialize()
{
	std::ostringstream stringstream;
	stringstream << std::hex << std::uppercase << std::setfill('0');
	stringstream << std::setw(8) << (uint32_t)_deviceType;
	stringstream << std::setw(8) << _address;
	if(_serialNumber.size() != 10) stringstream << "0000000000"; else stringstream << _serialNumber;
	stringstream << std::setw(2) << _firmwareVersion;
	stringstream << std::setw(8) << _centralAddress;
	stringstream << std::setw(8) << _messageCounter.size();
	for(std::unordered_map<int32_t, uint8_t>::const_iterator i = _messageCounter.begin(); i != _messageCounter.end(); ++i)
	{
		stringstream << std::setw(8) << i->first;
		stringstream << std::setw(2) << (int32_t)i->second;
	}
	stringstream << std::setw(8) << _peers.size();
	for(std::unordered_map<int32_t, Peer>::iterator i = _peers.begin(); i != _peers.end(); ++i)
	{
		stringstream << std::setw(8) << i->first;
		std::string peer = i->second.serialize();
		stringstream << std::setw(8) << peer.size();
		stringstream << peer;
	}
	stringstream << std::setw(8) << _config.size();
	for(std::unordered_map<int32_t, std::map<int32_t, int32_t>>::const_iterator i = _config.begin(); i != _config.end(); ++i)
	{
		stringstream << std::setw(8) << i->first; //List index
		stringstream << std::setw(8) << i->second.size();
		for(std::map<int32_t, int32_t>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
		{
			stringstream << std::setw(8) << j->first;
			stringstream << std::setw(8) << j->second;
		}
	}
	stringstream << std::dec;
	return stringstream.str();
}

std::string HomeMaticDevice::serialNumber()
{
    return _serialNumber;
}

int32_t HomeMaticDevice::address()
{
    return _address;
}

int32_t HomeMaticDevice::firmwareVersion()
{
    return _firmwareVersion;
}

HMDeviceTypes HomeMaticDevice::deviceType()
{
    return _deviceType;
}

xmlrpc_c::value HomeMaticDevice::getValue(const xmlrpc_c::paramList& paramList)
{
	return xmlrpc_c::value_nil();
}

bool HomeMaticDevice::pairDevice(int32_t timeout)
{
    _pairing = true;
    int32_t i = 0;
    std::chrono::milliseconds sleepingTime(500);
    sendPairingRequest();
    while(_pairing && i < timeout)
    {
        std::this_thread::sleep_for(sleepingTime);
        i += std::chrono::duration_cast<std::chrono::milliseconds>(sleepingTime).count();
    }
    if(_pairing)
    {
        _pairing = false;
        return false;
    }
    return true;
}

void HomeMaticDevice::packetReceived(BidCoSPacket* packet)
{
	try
	{
		_receivedPacketMutex.lock();
		_receivedPacket = *packet; //Make a copy of packet
		BidCoSMessage* message = _messages->find(DIRECTIONIN, &_receivedPacket);
		if(message != nullptr && message->checkAccess(&_receivedPacket, _bidCoSQueue.get()))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(90));  //Don't respond too fast
			_messageCounter[_receivedPacket.senderAddress()] = _receivedPacket.messageCounter();
			message->invokeMessageHandlerIncoming(&_receivedPacket);
			_lastReceivedMessage = message;
		}
		if(message == nullptr && _receivedPacket.destinationAddress() == _address) cout << "Could not process message. Unknown message type: " << _receivedPacket.hexString() << endl;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
	_receivedPacketMutex.unlock();
}

void HomeMaticDevice::sendPacket(BidCoSPacket& packet)
{
	_sentPacket = packet;
	GD::cul.sendPacket(packet);
}

void HomeMaticDevice::sendPacket(BidCoSPacket* packet)
{
	_sentPacket = *packet;
	GD::cul.sendPacket(packet);
}

void HomeMaticDevice::handleReset(int32_t messageCounter, BidCoSPacket* packet)
{
    sendOK(messageCounter, packet->senderAddress());
    reset();
}

void HomeMaticDevice::reset()
{
    _messageCounter.clear();
    _centralAddress = 0;
    _pairing = false;
    _justPairedToOrThroughCentral = false;
    _peers.clear();
}

void HomeMaticDevice::handlePairingRequest(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		if(packet->destinationAddress() == 0x00)
		{
			sendPairingRequest();
		}
		else if(packet->destinationAddress() == _address)
		{
			if(_centralAddress) //The central does not send packets with message type 0x00, so we don't have to allow pairing from the central here
			{
				sendNOK(messageCounter, packet->senderAddress());
				return;
			}
			if(_bidCoSQueue.get() == nullptr) return;
			Peer peer = createPeer(packet->senderAddress(), packet->payload()->at(0), (HMDeviceTypes)((packet->payload()->at(1) << 8) + packet->payload()->at(2)), "", packet->payload()->at(15), 0);
			_peers[packet->senderAddress()] = _bidCoSQueue->peer;
			_pairing = false;
			sendOK(messageCounter, packet->senderAddress());
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::handleWakeUp(int32_t messageCounter, BidCoSPacket * packet)
{
	sendOK(messageCounter, packet->senderAddress());
}

Peer HomeMaticDevice::createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter)
{
    return Peer();
}

void HomeMaticDevice::handleConfigRequestPeers(int32_t messageCounter, BidCoSPacket* packet)
{
    if(_peers.find(packet->senderAddress()) == _peers.end() || packet->destinationAddress() != _address) return;
    sendPeerList(messageCounter, packet->senderAddress(), packet->payload()->at(0));
}

void HomeMaticDevice::handleConfigPeerDelete(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		int32_t address = (packet->payload()->at(2) << 16) + (packet->payload()->at(3) << 8) + (packet->payload()->at(4));
		if(_peers.find(address) == _peers.end()) return;
		cout << "0x" << std::setw(6) << std::hex << _address;
		cout << ": Unpaired from peer 0x" << address << std::dec << endl;
		sendOK(messageCounter, packet->senderAddress());
		_peersMutex.lock();
		try
		{
			if(_peers[address].deviceType != HMDeviceTypes::HMRCV50) _peers.erase(address); //Unpair. Unpairing of HMRCV50 is done through CONFIG_WRITE_INDEX
		}
		catch(const std::exception& ex)
		{
			std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
		}
		_peersMutex.unlock();
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::handleConfigEnd(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		sendOK(messageCounter, packet->senderAddress());
		if(_bidCoSQueue.get() == nullptr) return;
		if(_bidCoSQueue->getQueueType() == BidCoSQueueType::PAIRINGCENTRAL)
		{
			_peersMutex.lock();
			try
			{
				_peers[_bidCoSQueue->peer.address] = _bidCoSQueue->peer;
				_centralAddress = _bidCoSQueue->peer.address;
			}
			catch(const std::exception& ex)
			{
				std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
			}
			_peersMutex.unlock();
			_justPairedToOrThroughCentral = true;
			_pairing = false;
		}
		else if(_bidCoSQueue->getQueueType() == BidCoSQueueType::UNPAIRING)
		{
			_peersMutex.lock();
			try
			{
				_peers.erase(packet->senderAddress());
			}
			catch(const std::exception& ex)
			{
				std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
			}
			_peersMutex.unlock();
			_centralAddress = 0;
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::handleConfigParamRequest(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		_currentList = packet->payload()->at(6);
		sendConfigParams(messageCounter, packet->senderAddress());
    }
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::handleConfigPeerAdd(int32_t messageCounter, BidCoSPacket* packet)
{
    int32_t address = (packet->payload()->at(2) << 16) + (packet->payload()->at(3) << 8) + (packet->payload()->at(4));
    if(_peers.find(address) == _peers.end())
    {
        Peer peer = createPeer(address, -1, HMDeviceTypes::HMUNKNOWN, "", packet->payload()->at(5), 0);
        _peersMutex.lock();
        try
        {
        	_peers[address] = peer;
        }
		catch(const std::exception& ex)
		{
			std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
		}
		_peersMutex.unlock();
    }
    _justPairedToOrThroughCentral = true;
    sendOK(messageCounter, packet->senderAddress());
}

void HomeMaticDevice::newBidCoSQueue(BidCoSQueueType queueType)
{
	if(_resetQueueThread.joinable())
	{
		_stopResetQueueThread = true;
		_resetQueueThread.join();
	}
	_stopResetQueueThread = false;
	if(_bidCoSQueue.get() != nullptr) _bidCoSQueue.reset(new BidCoSQueue(queueType));
	else _bidCoSQueue = unique_ptr<BidCoSQueue>(new BidCoSQueue(queueType));
	_bidCoSQueue->device = this;
	_resetQueueThread = std::thread(&HomeMaticDevice::resetQueue, this);
	_resetQueueThread.detach();
}

void HomeMaticDevice::resetQueue()
{
	std::chrono::milliseconds sleepingTime(500);
	int i = 0;
	while(!_stopResetQueueThread && i < 30)
	{
		std::this_thread::sleep_for(sleepingTime);
		i++;
	}
	_bidCoSQueue.reset();
}

void HomeMaticDevice::handleConfigStart(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		if(_pairing)
		{
			newBidCoSQueue(BidCoSQueueType::PAIRINGCENTRAL);
			Peer peer;
			peer.address = packet->senderAddress();
			peer.deviceType = HMDeviceTypes::HMRCV50;
			peer.messageCounter = 0x00; //Unknown at this point
			_bidCoSQueue->peer = peer;
			_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<int32_t, int32_t>> { std::pair<int32_t, int32_t>(0x01, 0x05) }));
			_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<int32_t, int32_t>> { std::pair<int32_t, int32_t>(0x01, 0x08), std::pair<int32_t, int32_t>(0x02, 0x02), std::pair<int32_t, int32_t>(0x03, 0x01) }));
			_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<int32_t, int32_t>> { std::pair<int32_t, int32_t>(0x01, 0x06) }));
			_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<int32_t, int32_t>> { std::pair<int32_t, int32_t>(0x01, 0x04) }));
		}
		_currentList = packet->payload()->at(6);
		sendOK(messageCounter, packet->senderAddress());
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::handleConfigWriteIndex(int32_t messageCounter, BidCoSPacket* packet)
{
	if(packet->payload()->at(2) == 0x02 && packet->payload()->at(3) == 0x00)
    {
        newBidCoSQueue(BidCoSQueueType::UNPAIRING);
        _bidCoSQueue->peer = _peers[packet->senderAddress()];
        _bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<int32_t, int32_t>> { std::pair<int32_t, int32_t>(0x01, 0x08), std::pair<int32_t, int32_t>(0x02, 0x02), std::pair<int32_t, int32_t>(0x03, 0x00) }));
        _bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<int32_t, int32_t>> { std::pair<int32_t, int32_t>(0x01, 0x06) }));
    }
	for(int i = 2; i < (signed)packet->payload()->size(); i+=2)
	{
		_config[_currentList][(int32_t)packet->payload()->at(i)] = packet->payload()->at(i + 1);
		cout << "0x" << std::setw(6) << std::hex << _address;
		cout << ": Config at index " << std::setw(2) << (int32_t)(packet->payload()->at(i)) << " set to " << std::setw(2) << (int32_t)(packet->payload()->at(i + 1)) << std::dec << endl;
	}
    sendOK(messageCounter, packet->senderAddress());
}

void HomeMaticDevice::sendPeerList(int32_t messageCounter, int32_t destinationAddress, int32_t channel)
{
    std::vector<uint8_t> payload;
    payload.push_back(0x01); //I think it's always channel 1
    for(std::unordered_map<int32_t, Peer>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
    {
        if(i->second.deviceType == HMDeviceTypes::HMRCV50) continue;
        if(i->second.localChannel != channel) continue;
        payload.push_back(i->first >> 16);
        payload.push_back((i->first >> 8) & 0xFF);
        payload.push_back(i->first & 0xFF);
        payload.push_back(i->second.remoteChannel); //Channel
    }
    payload.push_back(0x00);
    payload.push_back(0x00);
    payload.push_back(0x00);
    payload.push_back(0x00);
    BidCoSPacket packet(messageCounter, 0x80, 0x10, _address, destinationAddress, payload);
    sendPacket(packet);
}

void HomeMaticDevice::sendStealthyOK(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x00);
		BidCoSPacket ok(messageCounter, 0x80, 0x02, _address, destinationAddress, payload);
		GD::cul.sendPacket(ok);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::sendOK(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x00);
		BidCoSPacket ok(messageCounter, 0x80, 0x02, _address, destinationAddress, payload);
		sendPacket(ok);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::sendOKWithPayload(int32_t messageCounter, int32_t destinationAddress, std::vector<uint8_t> payload, bool isWakeMeUpPacket)
{
	try
	{
		uint8_t controlByte = 0x80;
		if(isWakeMeUpPacket) controlByte &= 0x02;
		BidCoSPacket ok(messageCounter, 0x80, 0x02, _address, destinationAddress, payload);
		sendPacket(ok);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::sendNOK(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x80);
		BidCoSPacket ok(messageCounter, 0x80, 0x02, _address, destinationAddress, payload);
		sendPacket(ok);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::sendNOKTargetInvalid(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x84);
		BidCoSPacket ok(messageCounter, 0x80, 0x02, _address, destinationAddress, payload);
		sendPacket(ok);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::sendConfigParams(int32_t messageCounter, int32_t destinationAddress)
{
	if(_config[_currentList].size() >= 8)
	{
		newBidCoSQueue(BidCoSQueueType::DEFAULT);
		//Actually destinationAddress should always be in _peers at this point
		if(_peers.find(destinationAddress) != _peers.end()) _bidCoSQueue->peer = _peers[destinationAddress];

		//Add request config packet in case the request is being repeated
		_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<int32_t, int32_t>> { std::pair<int32_t, int32_t>(0x01, 0x04) }));

		std::vector<uint8_t> payload;
		std::map<int32_t, int32_t>::const_iterator i = _config[_currentList].begin();
		while(true)
		{
			if(i->first % 15 == 1 || i == _config[_currentList].end()) //New packet
			{
				if(!payload.empty())
				{
					BidCoSPacket packet(messageCounter, 0xA0, 0x10, _address, destinationAddress, payload);
					_bidCoSQueue->push(packet);
					_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<int32_t, int32_t>>()));
					payload.clear();
				}
				if(i == _config[_currentList].end()) break;
				payload.push_back((_receivedPacket.payload()->at(0) == 2) ? 3 : 2);
				payload.push_back(i->first);
			}
			payload.push_back(i->second);
			i++;
		}
		payload.push_back((_receivedPacket.payload()->at(0) == 2) ? 3 : 2);
		payload.push_back(0x00);
		BidCoSPacket packet(messageCounter, 0x80, 0x10, _address, destinationAddress, payload);
		_bidCoSQueue->push(packet);
	}
	else
	{
		std::vector<unsigned char> payload;
		payload.push_back((_receivedPacket.payload()->at(0) == 2) ? 3 : 2); //No idea how to determine the destination channel
		for(std::map<int32_t, int32_t>::const_iterator i = _config[_currentList].begin(); i != _config[_currentList].end(); ++i)
		{
			payload.push_back(i->first);
			payload.push_back(i->second);
		}
		payload.push_back(0); //I think the packet always ends with two zero bytes
		payload.push_back(0);
		BidCoSPacket config(messageCounter, 0x80, 0x10, _address, destinationAddress, payload);
		sendPacket(config);
	}
}

void HomeMaticDevice::sendPairingRequest()
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(_firmwareVersion);
		payload.push_back(((uint32_t)_deviceType) >> 8);
		payload.push_back((uint32_t)_deviceType & 0xFF);
		std::for_each(_serialNumber.begin(), _serialNumber.end(), [&](unsigned char byte)
		{
			payload.push_back(byte);
		});
		payload.push_back(_deviceClass);
		payload.push_back(_channelMin);
		payload.push_back(_channelMax);
		payload.push_back(_lastPairingByte);
		BidCoSPacket pairingRequest(_messageCounter[0], 0x84, 0x00, _address, 0x00, payload);
		_messageCounter[0]++;
		sendPacket(pairingRequest);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::sendDirectedPairingRequest(int32_t messageCounter, int32_t controlByte, BidCoSPacket* packet)
{
	try
	{
		int32_t deviceType = (packet->payload()->at(1) << 8) + packet->payload()->at(2);
		if(_deviceTypeChannels.find(deviceType) == _deviceTypeChannels.end()) return;
		std::vector<uint8_t> payload;
		payload.push_back(_firmwareVersion);
		payload.push_back(((uint32_t)_deviceType) >> 8);
		payload.push_back((uint32_t)_deviceType & 0xFF);
		std::for_each(_serialNumber.begin(), _serialNumber.end(), [&](unsigned char byte)
		{
			payload.push_back(byte);
		});
		payload.push_back(_deviceClass);
		payload.push_back(_channelMin);
		payload.push_back(_deviceTypeChannels[deviceType]);
		payload.push_back(0x00);
		BidCoSPacket pairingRequest(messageCounter, 0xA0, 0x00, _address, packet->senderAddress(), payload);
		sendPacket(pairingRequest);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HomeMaticDevice::setLowBattery(bool lowBattery)
{
    _lowBattery = lowBattery;
}

void HomeMaticDevice::sendDutyCycleResponse(int32_t destinationAddress) {}

std::unordered_map<int32_t, Peer>* HomeMaticDevice::getPeers()
{
    return &_peers;
}

int32_t HomeMaticDevice::getCentralAddress()
{
    return _centralAddress;
}

int32_t HomeMaticDevice::calculateCycleLength(uint8_t messageCounter)
{
	int32_t result = (((_address << 8) | messageCounter) * 1103515245 + 12345) >> 16;
	return (result & 0xFF) + 480;
}
