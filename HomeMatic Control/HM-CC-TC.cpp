#include "HM-CC-TC.h"

HM_CC_TC::HM_CC_TC(std::string serialNumber, int32_t address) : HomeMaticDevice(serialNumber, address)
{
    _deviceType = 0x39;
    _firmwareVersion = 0x21;
    _deviceClass = 0x58;
    _channelMin = 0x00;
    _channelMax = 0xFF;
    _deviceTypeChannels[0x3A] = 2;
    _lastPairingByte = 0xFF;

    setUpBidCoSMessages();
    setUpConfig();
    startDutyCycle();
}

HM_CC_TC::HM_CC_TC(std::string serializedObject) : HomeMaticDevice(serializedObject.substr(serializedObject.find_first_of("|")))
{
	serializedObject = serializedObject.substr(0, serializedObject.find_first_of("|"));

	std::istringstream stringstream(serializedObject);
	std::string entry;
	int32_t i = 0;
	while(std::getline(stringstream, entry, ';'))
	{
		switch(i)
		{
		case 0:
			_currentDutyCycleDeviceAddress = std::stoi(entry, 0, 16);
			break;
		case 1:
			_temperature = std::stoi(entry, 0, 16);
			break;
		case 2:
			_setPointTemperature = std::stoi(entry, 0, 16);
			break;
		case 3:
			_humidity = std::stoi(entry, 0, 16);
			break;
		case 4:
			_valveState = std::stoi(entry, 0, 16);
			break;
		case 5:
			_newValveState = std::stoi(entry, 0, 16);
			break;
		}
		i++;
	}
}

void HM_CC_TC::setUpConfig()
{
	HomeMaticDevice::setUpConfig();

	_config[0][1] = 0x00;
	_config[0][5] = 0x00;
	_config[0][15] = 0x00;

	//Possible temperature values are: 12 - 60 [= 6.0°C - 30.0°C]
	//Possible timeout values are: 1 - 144 (10 - 1440 minutes)
	_config[5][1] = 0x09; //Bit 0: DISPLAY_TEMPERATUR_HUMIDITY_CHANGE (0: TEMPERATUR_ONLY, 1: TEMPERATUR_AND_HUMIDITY), bit 1: DISPLAY_TEMPERATUR_INFORMATION (0: ACTUAL_VALVE, 1: SET_POINT), Bit 2: DISPLAY_TEMPERATUR_UNIT (0: CELSIUS, 1: FAHRENHEIT), Bit 3 - 4: MODE_TEMPERATUR_REGULATOR (0: MANUAL, 1: AUTO, 2: CENTRAL, 3: PARTY), bit 5 - 7: DECALCIFICATION_DAY (0: SAT, 1: SUN, 2: MON, 3: TUE, ...)
	_config[5][2] = 0x2A; //Bit 6 - 7: MODE_TEMPERATUR_VALVE (0: AUTO, 1: CLOSE_VALVE, 2: OPEN_VALVE)
	_config[5][3] = 0x2A; //Bit 0 - 5: TEMPERATUR_COMFORT_VALUE
	_config[5][4] = 0x22; //Bit 0 - 5: TEMPERATUR_LOWERING_VALUE
	_config[5][5] = 0x18;
	_config[5][6] = 0x28; //Bit 0 - 5: TEMPERATUR_PARTY_VALUE
	_config[5][7] = 0x00;
	_config[5][8] = 0x58; //Bit 0 - 2: DECALCIFICATION_MINUTE (0 - 5 [= 0 - 50 minutes]), bit 3 - 7: DECALCIFICATION_HOUR (0 - 23)
	_config[5][9] = 0x00;
	_config[5][10] = 0x00;
	_config[5][11] = 0x24; //TIMEOUT_SATURDAY_1
	_config[5][12] = 0x22; //TEMPERATUR_SATURDAY_1
	_config[5][13] = 0x48; //TIMEOUT_SATURDAY_2
	_config[5][14] = 0x2A; //TEMPERATUR_SATURDAY_2
	_config[5][15] = 0x8A; //TIMEOUT_SATURDAY_3
	_config[5][16] = 0x2A; //TEMPERATUR_SATURDAY_3
	_config[5][17] = 0x90; //TIMEOUT_SATURDAY_3
	_config[5][18] = 0x22; //TEMPERATUR_SATURDAY_3
	for(int i = 19; i <= 58; i+=2) //TIMEOUT AND TEMPERATURE 4 to 24
	{
		_config[5][i] = 0x90;
		_config[5][i+1] = 0x28;
	}
	for(int i = 59; i <= 106; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_SUNDAY
	{
		_config[5][i] = _config[5][i - 48];
		_config[5][i+1] = _config[5][i - 47];
	}
	for(int i = 107; i <= 154; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_MONDAY
	{
		_config[5][i] = _config[5][i - 48];
		_config[5][i+1] = _config[5][i - 47];
	}
	for(int i = 155; i <= 202; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_TUESDAY
	{
		_config[5][i] = _config[5][i - 48];
		_config[5][i+1] = _config[5][i - 47];
	}
	for(int i = 203; i <= 250; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_WEDNESDAY
	{
		_config[5][i] = _config[5][i - 48];
		_config[5][i+1] = _config[5][i - 47];
	}
	for(int i = 1; i <= 48; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_THURSDAY
	{
		_config[6][i] = _config[5][i + 10];
		_config[6][i+1] = _config[5][i + 11];
	}
	for(int i = 49; i <= 96; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_FRIDAY
	{
		_config[6][i] = _config[6][i - 48];
		_config[6][i+1] = _config[6][i - 47];
	}
	//_config[6][97] = NOT DEFINED //Bit 0 - 5: PARTY_END_HOUR, Bit 7: PARTY_END_MINUTE (0: 0 minutes, 1: 30 minutes)
	//_config[6][98] = NOT DEFINED //PARTY_END_DAY (0 - 200 days)
}

void HM_CC_TC::setUpBidCoSMessages()
{
    HomeMaticDevice::setUpBidCoSMessages();

    //Outgoing
    _messages->add(BidCoSMessage(0x00, 0xA0, this, &HomeMaticDevice::sendDirectedPairingRequest));

    BidCoSMessage message(0x01, 0xA0, this, &HomeMaticDevice::sendRequestConfig);
    message.addSubtype(0x01, 0x04);
    _messages->add(message);

    //Incoming
    _messages->add(BidCoSMessage(0x10, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigParamResponse));
    _messages->add(BidCoSMessage(0x11, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleSetPoint));
    _messages->add(BidCoSMessage(0x12, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleWakeUp));
    _messages->add(BidCoSMessage(0xDD, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleSetValveState));
}

HM_CC_TC::~HM_CC_TC()
{
	_stopDutyCycleThread = true;
	if(_dutyCycleThread != nullptr && _dutyCycleThread->joinable())	_dutyCycleThread->join();
}

std::string HM_CC_TC::serialize()
{
	std::string serializedObject = HomeMaticDevice::serialize();
	std::ostringstream stringstream;
	stringstream << std::hex;
	stringstream << _currentDutyCycleDeviceAddress << ";";
	stringstream << _temperature << ";";
	stringstream << _setPointTemperature << ";";
	stringstream << _humidity << ";";
	stringstream << _valveState << ";";
	stringstream << _newValveState << "|"; //The "|" seperates the base class from the inherited class part
	stringstream << std::dec;
	return stringstream.str() + serializedObject;
}

void HM_CC_TC::startDutyCycle()
{
	_dutyCycleThread = new std::thread(&HM_CC_TC::dutyCycleThread, this);
	_dutyCycleThread->detach();
}

void HM_CC_TC::dutyCycleThread()
{
	int64_t lastDutyCycleEvent;
	int64_t nextDutyCycleEvent = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	int64_t timePassed;
	int64_t cycleTime;
	int64_t delay = 0; //TODO sathyaRemove if no problems
	while(!_stopDutyCycleThread)
	{
		lastDutyCycleEvent = nextDutyCycleEvent;
		int32_t cycleLength = calculateCycleLength();
		cycleTime = cycleLength * 250;
		nextDutyCycleEvent += cycleTime;

		std::chrono::milliseconds sleepingTime(2000);
		int32_t cycleCounter = 0;
		while(!_stopDutyCycleThread && cycleCounter < cycleLength - 80)
		{
			std::this_thread::sleep_for(sleepingTime);
			cycleCounter += 8;
		}
		if(_stopDutyCycleThread) break;

		std::thread sendDutyCycleBroadcastThread(&HM_CC_TC::sendDutyCycleBroadcast, this);
		sendDutyCycleBroadcastThread.detach();

		while(!_stopDutyCycleThread && cycleCounter < cycleLength - 40)
		{
			std::this_thread::sleep_for(sleepingTime);
			cycleCounter += 8;
		}

		timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - lastDutyCycleEvent;
		cout << "Time mismatch: " << (timePassed + 10000 - cycleTime) << endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(cycleTime - timePassed - 5000));

		timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - lastDutyCycleEvent;
		cout << "Time mismatch: " << (timePassed + 5000 - cycleTime) << endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(cycleTime - timePassed - 2000));

		timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - lastDutyCycleEvent;
		cout << "Time mismatch: " << (timePassed + 2000 - cycleTime) << endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(cycleTime - timePassed - 1000));

		timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - lastDutyCycleEvent;
		cout << "Time mismatch: " << (timePassed + 1000 - cycleTime) << endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(cycleTime - timePassed - 500));

		timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - lastDutyCycleEvent;
		cout << "Time mismatch: " << (timePassed + 500 - cycleTime) << endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(cycleTime - timePassed + delay));

		std::thread sendDutyCyclePacketThread(&HM_CC_TC::sendDutyCyclePacket, this);

		sched_param schedParam;
		int policy;
		pthread_getschedparam(sendDutyCyclePacketThread.native_handle(), &policy, &schedParam);
		schedParam.sched_priority = 99;
		if(!pthread_setschedparam(sendDutyCyclePacketThread.native_handle(), policy, &schedParam)) throw(new Exception("Error: Could not set thread priority."));
		sendDutyCyclePacketThread.detach();

		_messageCounter[1]++;
	}
}

void HM_CC_TC::sendDutyCycleBroadcast()
{

	std::vector<uint8_t> payload;
	payload.push_back((_temperature & 0xFF00) >> 8);
	payload.push_back(_temperature & 0xFF);
	payload.push_back(_humidity);
	BidCoSPacket packet(_messageCounter[1], 0x86, 0x70, _address, 0, payload);
	GD::cul->sendPacket(packet);
}

void HM_CC_TC::sendDutyCyclePacket()
{
	int64_t timePoint = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	int32_t address = getNextDutyCycleDeviceAddress();
#if DEBUG_LEVEL==5
	cout << "Next HM-CC-VD is 0x" << std::hex << address << std::dec << endl;
#endif
	if(address < 1) return;
	std::vector<uint8_t> payload;
	payload.push_back(getAdjustmentCommand());
	payload.push_back(_newValveState);
	BidCoSPacket packet(_messageCounter[1], 0xA2, 0x58, _address, address, payload);
	GD::cul->sendPacket(packet);
	_valveState = _newValveState;
	int64_t timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - timePoint;
	cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << ": Sending took " << timePassed << "ms." << endl;
}

int32_t HM_CC_TC::getAdjustmentCommand()
{
	if(_setPointTemperature == 0) //OFF
	{
		return 2;
	}
	else if(_setPointTemperature == 60) //ON
	{
		return 3;
	}
	else
	{
		if(_newValveState != _valveState) return 3; else return 0;
	}
}

int32_t HM_CC_TC::getNextDutyCycleDeviceAddress()
{
	_peersMutex.lock(); //I already had the problem that _peers was accessed while a peer was being deleted from it
	try
	{
		if(_peers.size() == 0)
		{
			_peersMutex.unlock();
			return -1;
		}
		int i = 0;
		std::unordered_map<int32_t, Peer>::iterator j = (_currentDutyCycleDeviceAddress == -1) ? _peers.begin() : _peers.find(_currentDutyCycleDeviceAddress);
		if(j == _peers.end()) //_currentDutyCycleDeviceAddress does not exist anymore in peers
		{
			j = _peers.begin();
		}
		while(i <= (signed)_peers.size()) //<= because it might be there is only one HM-CC-VD
		{
			j++;
			if(j == _peers.end())
			{
				j = _peers.begin();
			}
			if(j->second.deviceType == HMCCVD)
			{
				_currentDutyCycleDeviceAddress = j->first;
				_peersMutex.unlock();
				return j->first;
			}
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
	_peersMutex.unlock();
	return -1;
}

void HM_CC_TC::sendConfigParams(int32_t messageCounter, int32_t destinationAddress)
{
    HomeMaticDevice::sendConfigParams(messageCounter, destinationAddress);
}

Peer HM_CC_TC::createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter)
{
    Peer peer;
    peer.address = address;
    peer.firmwareVersion = firmwareVersion;
    peer.deviceType = deviceType;
    peer.messageCounter = 0;
    peer.remoteChannel = remoteChannel;
    if(deviceType == HMCCVD || deviceType == HMUNKNOWN) peer.localChannel = 2; else peer.localChannel = 0;
    peer.serialNumber = serialNumber;
    return peer;
}

void HM_CC_TC::reset()
{
    HomeMaticDevice::reset();
}

void HM_CC_TC::handleSetValveState(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		_newValveState = packet->payload()->at(0);
#if DEBUG_LEVEL==5
		cout << "New valve state: " << _newValveState << endl;
#endif
		sendOK(messageCounter, packet->senderAddress());
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HM_CC_TC::handleConfigPeerAdd(int32_t messageCounter, BidCoSPacket* packet)
{
    HomeMaticDevice::handleConfigPeerAdd(messageCounter, packet);

    uint8_t channel = packet->payload()->at(0);
    int32_t address = (packet->payload()->at(2) << 16) + (packet->payload()->at(3) << 8) + (packet->payload()->at(4));
    if(channel == 2)
    {
    	_peers[address].deviceType = HMCCVD;
#if DEBUG_LEVEL==5
    	cout << "Added HM-CC-VD with address 0x" << std::hex << address << std::dec << endl;
#endif
    }
}

void HM_CC_TC::handleSetPoint(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		if(packet->payload()->at(1) == 2) //Is the subtype correct? Though I don't think there are other subtypes
		{
			_setPointTemperature = packet->payload()->at(2);
			std::vector<uint8_t> payload;
			payload.push_back(0x01); //Channel
			payload.push_back(0x02); //Subtype
			payload.push_back(_setPointTemperature);
			payload.push_back(0x4D); //RSSI
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HM_CC_TC::handlePairingRequest(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		//Block pairing, when paired to central
		if(_centralAddress) //The central does not send packets with message type 0x00, so we don't have to allow pairing from the central here
		{
			sendNOK(messageCounter, packet->senderAddress());
			return;
		}
		Peer peer = createPeer(packet->senderAddress(), packet->payload()->at(0), (HMDeviceTypes)((packet->payload()->at(1) << 8) + packet->payload()->at(2)), "", packet->payload()->at(15), 0);
		newBidCoSQueue(BidCoSQueueType::PAIRING);
		_bidCoSQueue->peer = peer;
		_bidCoSQueue->push(_messages->find(DIRECTIONOUT, 0x00, std::vector<std::pair<int32_t, int32_t>>()));
		_bidCoSQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<int32_t, int32_t>>()));
		_bidCoSQueue->push(_messages->find(DIRECTIONOUT, 0x01, std::vector<std::pair<int32_t, int32_t>> { std::pair<int32_t, int32_t>(0x01, 0x04) }));
		//The 0x10 response with the config does not have to be part of the queue.
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HM_CC_TC::handleConfigParamResponse(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		HomeMaticDevice::handleConfigParamResponse(messageCounter, packet);

		_currentList = packet->payload()->at(6);
		for(int i = 7; i < (signed)packet->payload()->size() - 2; i+=2)
		{
			_peers[packet->senderAddress()].config[packet->payload()->at(i)] = packet->payload()->at(i + 1);
			cout << "0x" << std::setw(6) << std::hex << _address;
			cout << ": Config of device 0x" << std::setw(6) << packet->senderAddress() << " at index " << std::setw(2) << (int32_t)(packet->payload()->at(i)) << " set to " << std::setw(2) << (int32_t)(packet->payload()->at(i + 1)) << std::dec << endl;
		}
		sendOK(messageCounter, packet->senderAddress());
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HM_CC_TC::sendRequestConfig(int32_t messageCounter, int32_t controlByte, BidCoSPacket* packet)
{
	try
	{
		if(_bidCoSQueue.get() == nullptr) return;
		std::vector<uint8_t> payload;
		payload.push_back(_bidCoSQueue->peer.remoteChannel);
		payload.push_back(0x04);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0x05);
		BidCoSPacket requestConfig(messageCounter, 0xA0, 0x01, _address, packet->senderAddress(), payload);
		GD::cul->sendPacket(requestConfig);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}

void HM_CC_TC::handleAck(int32_t messageCounter, BidCoSPacket* packet)
{
	try
	{
		if(_bidCoSQueue.get() == nullptr) return;
		_bidCoSQueue->pop(); //Messages are not popped by default.
		if(_bidCoSQueue->getQueueType() == BidCoSQueueType::PAIRING)
		{
			_peers[_bidCoSQueue->peer.address] = _bidCoSQueue->peer;
			_pairing = false;
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << endl;
	}
}
