#include "HM-CC-TC.h"
#include "../GD.h"

HM_CC_TC::HM_CC_TC() : HomeMaticDevice()
{
	init();
}

HM_CC_TC::HM_CC_TC(std::string serialNumber, int32_t address) : HomeMaticDevice(serialNumber, address)
{
	init();
	startDutyCycle(-1);
}

void HM_CC_TC::init()
{
	HomeMaticDevice::init();

	_deviceType = HMDeviceTypes::HMCCTC;
    _firmwareVersion = 0x21;
    _deviceClass = 0x58;
    _channelMin = 0x00;
    _channelMax = 0xFF;
    _deviceTypeChannels[0x3A] = 2;
    _lastPairingByte = 0xFF;

    setUpBidCoSMessages();
    setUpConfig();
}

int64_t HM_CC_TC::calculateLastDutyCycleEvent()
{
	int64_t now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	if(now - _lastDutyCycleEvent > 1800000000) return -1; //Duty cycle is out of sync anyway so don't bother to calculate
	int64_t nextDutyCycleEvent = _lastDutyCycleEvent;
	int64_t lastDutyCycleEvent = _lastDutyCycleEvent;
	_messageCounter[1]--; //The saved message counter is the current one, but the calculation has to use the last one
	while(nextDutyCycleEvent < now + 25000000)
	{
		lastDutyCycleEvent = nextDutyCycleEvent;
		nextDutyCycleEvent = lastDutyCycleEvent + (calculateCycleLength(_messageCounter[1]) * 250000);
		_messageCounter[1]++;
	}
	if(GD::debugLevel >= 5) std::cout << "Setting last duty cycle event to: " << lastDutyCycleEvent << std::endl;
	return lastDutyCycleEvent;
}

void HM_CC_TC::setUpConfig()
{
	HomeMaticDevice::setUpConfig();

	_config[0][0][1] = 0x00;
	_config[0][0][5] = 0x00;
	_config[0][0][15] = 0x00;

	//Possible temperature values are: 12 - 60 [= 6.0°C - 30.0°C]
	//Possible timeout values are: 1 - 144 (10 - 1440 minutes)
	_config[2][5][1] = 0x09; //Bit 0: DISPLAY_TEMPERATUR_HUMIDITY_CHANGE (0: TEMPERATUR_ONLY, 1: TEMPERATUR_AND_HUMIDITY), bit 1: DISPLAY_TEMPERATUR_INFORMATION (0: ACTUAL_VALVE, 1: SET_POINT), Bit 2: DISPLAY_TEMPERATUR_UNIT (0: CELSIUS, 1: FAHRENHEIT), Bit 3 - 4: MODE_TEMPERATUR_REGULATOR (0: MANUAL, 1: AUTO, 2: CENTRAL, 3: PARTY), bit 5 - 7: DECALCIFICATION_DAY (0: SAT, 1: SUN, 2: MON, 3: TUE, ...)
	_config[2][5][2] = 0x2A; //Bit 6 - 7: MODE_TEMPERATUR_VALVE (0: AUTO, 1: CLOSE_VALVE, 2: OPEN_VALVE)
	_config[2][5][3] = 0x2A; //Bit 0 - 5: TEMPERATUR_COMFORT_VALUE
	_config[2][5][4] = 0x22; //Bit 0 - 5: TEMPERATUR_LOWERING_VALUE
	_config[2][5][5] = 0x18;
	_config[2][5][6] = 0x28; //Bit 0 - 5: TEMPERATUR_PARTY_VALUE
	_config[2][5][7] = 0x00;
	_config[2][5][8] = 0x58; //Bit 0 - 2: DECALCIFICATION_MINUTE (0 - 5 [= 0 - 50 minutes]), bit 3 - 7: DECALCIFICATION_HOUR (0 - 23)
	_config[2][5][9] = 0x00;
	_config[2][5][10] = 0x00;
	_config[2][5][11] = 0x24; //TIMEOUT_SATURDAY_1
	_config[2][5][12] = 0x22; //TEMPERATUR_SATURDAY_1
	_config[2][5][13] = 0x48; //TIMEOUT_SATURDAY_2
	_config[2][5][14] = 0x2A; //TEMPERATUR_SATURDAY_2
	_config[2][5][15] = 0x8A; //TIMEOUT_SATURDAY_3
	_config[2][5][16] = 0x2A; //TEMPERATUR_SATURDAY_3
	_config[2][5][17] = 0x90; //TIMEOUT_SATURDAY_3
	_config[2][5][18] = 0x22; //TEMPERATUR_SATURDAY_3
	for(int i = 19; i <= 58; i+=2) //TIMEOUT AND TEMPERATURE 4 to 24
	{
		_config[2][5][i] = 0x90;
		_config[2][5][i+1] = 0x28;
	}
	for(int i = 59; i <= 106; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_SUNDAY
	{
		_config[2][5][i] = _config[2][5][i - 48];
		_config[2][5][i+1] = _config[2][5][i - 47];
	}
	for(int i = 107; i <= 154; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_MONDAY
	{
		_config[2][5][i] = _config[2][5][i - 48];
		_config[2][5][i+1] = _config[2][5][i - 47];
	}
	for(int i = 155; i <= 202; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_TUESDAY
	{
		_config[2][5][i] = _config[2][5][i - 48];
		_config[2][5][i+1] = _config[2][5][i - 47];
	}
	for(int i = 203; i <= 250; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_WEDNESDAY
	{
		_config[2][5][i] = _config[2][5][i - 48];
		_config[2][5][i+1] = _config[2][5][i - 47];
	}
	for(int i = 1; i <= 48; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_THURSDAY
	{
		_config[2][6][i] = _config[2][5][i + 10];
		_config[2][6][i+1] = _config[2][5][i + 11];
	}
	for(int i = 49; i <= 96; i+=2) //TIMEOUT_SUNDAY AND TEMPERATURE_FRIDAY
	{
		_config[2][6][i] = _config[2][6][i - 48];
		_config[2][6][i+1] = _config[2][6][i - 47];
	}
	//_config[2][6][97] = NOT DEFINED //Bit 0 - 5: PARTY_END_HOUR, Bit 7: PARTY_END_MINUTE (0: 0 minutes, 1: 30 minutes)
	//_config[2][6][98] = NOT DEFINED //PARTY_END_DAY (0 - 200 days)
}

void HM_CC_TC::setUpBidCoSMessages()
{
    HomeMaticDevice::setUpBidCoSMessages();

    //Outgoing
    _messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x00, 0xA0, this, &HomeMaticDevice::sendDirectedPairingRequest)));

    std::shared_ptr<BidCoSMessage> message(new BidCoSMessage(0x01, 0xA0, this, &HomeMaticDevice::sendRequestConfig));
    message->addSubtype(0x01, 0x04);
    _messages->add(message);

    //Incoming
    _messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x10, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigParamResponse)));
    _messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x11, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleSetPoint)));
    _messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x12, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleWakeUp)));
    _messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0xDD, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleSetValveState)));
}

HM_CC_TC::~HM_CC_TC()
{
}

void HM_CC_TC::stopThreads()
{
	HomeMaticDevice::stopThreads();
	_stopDutyCycleThread = true;
}

std::string HM_CC_TC::serialize()
{
	std::string serializedBase = HomeMaticDevice::serialize();
	std::ostringstream stringstream;
	stringstream << std::hex << std::uppercase << std::setfill('0');
	stringstream << std::setw(8) << serializedBase.size() << serializedBase;
	stringstream << std::setw(8) << _currentDutyCycleDeviceAddress;
	stringstream << std::setw(4) << _temperature;
	stringstream << std::setw(4) << _setPointTemperature;
	stringstream << std::setw(2) << _humidity;
	stringstream << std::setw(2) << _valveState;
	stringstream << std::setw(2) << _newValveState;
	return  stringstream.str();
}

void HM_CC_TC::unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent)
{
	int32_t baseLength = std::stoll(serializedObject.substr(0, 8), 0, 16);
	HomeMaticDevice::unserialize(serializedObject.substr(8, baseLength), dutyCycleMessageCounter, lastDutyCycleEvent);

	uint32_t pos = 8 + baseLength;
	_currentDutyCycleDeviceAddress = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	_temperature = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
	_setPointTemperature = std::stoll(serializedObject.substr(pos, 4), 0, 16); pos += 4;
	_humidity = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
	_valveState = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
	_newValveState = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;

	_messageCounter[1] = dutyCycleMessageCounter;
	_lastDutyCycleEvent = lastDutyCycleEvent;
	startDutyCycle(calculateLastDutyCycleEvent());
}

void HM_CC_TC::handleCLICommand(std::string command)
{
	if(command == "get duty cycle counter")
	{
		std::cout << "Duty cycle counter: " << std::dec << _dutyCycleCounter << " (" << ((_dutyCycleCounter * 250) / 1000) << "s)" << std::endl;
	}
	HomeMaticDevice::handleCLICommand(command);
}

void HM_CC_TC::setValveState(int32_t valveState)
{
	valveState *= 256;
	//Round up if necessary. I don't use double for calculation, because hopefully this is faster.
	if(valveState % 100 >= 50) valveState = (valveState / 100) + 1; else valveState /= 100;
	_newValveState = valveState;
	if(_newValveState > 255) _newValveState = 255;
	if(_newValveState < 0) _newValveState = 0;
}

void HM_CC_TC::startDutyCycle(int64_t lastDutyCycleEvent)
{
	_dutyCycleThread = new std::thread(&HM_CC_TC::dutyCycleThread, this, lastDutyCycleEvent);
}

void HM_CC_TC::dutyCycleThread(int64_t lastDutyCycleEvent)
{
	int64_t nextDutyCycleEvent = (lastDutyCycleEvent == -1) ? std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() : lastDutyCycleEvent;
	_lastDutyCycleEvent = nextDutyCycleEvent;
	int64_t timePoint;
	int64_t cycleTime;
	int32_t cycleLength = calculateCycleLength(_messageCounter[1] - 1); //The calculation has to use the last message counter
	_dutyCycleCounter = (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - _lastDutyCycleEvent) / 250000;
	_dutyCycleCounter = (_dutyCycleCounter % 8 > 3) ? _dutyCycleCounter + (8 - (_dutyCycleCounter % 8)) : _dutyCycleCounter - (_dutyCycleCounter % 8);
	if(GD::debugLevel >= 5 && _dutyCycleCounter > 0) std::cout << "Skipping " << (_dutyCycleCounter * 250) << " ms of duty cycle." << std::endl;
	while(!_stopDutyCycleThread)
	{
		cycleTime = cycleLength * 250000;
		nextDutyCycleEvent += cycleTime + 3000; //Add 3ms every cycle. This is very important! Without it, 20% of the packets are sent too early.
		if(GD::debugLevel >= 5) std::cout << "Next duty cycle: " << (nextDutyCycleEvent / 1000) << " (in " << (cycleTime / 1000) << " ms) with message counter 0x" << std::hex << (int32_t)_messageCounter[1] << std::dec << std::endl;
		std::chrono::milliseconds sleepingTime(2000);
		while(!_stopDutyCycleThread && _dutyCycleCounter < cycleLength - 80)
		{
			std::this_thread::sleep_for(sleepingTime);
			_dutyCycleCounter += 8;
		}
		if(_stopDutyCycleThread) break;

		if(_dutyCycleBroadcast)
		{
			std::thread sendDutyCycleBroadcastThread(&HM_CC_TC::sendDutyCycleBroadcast, this);
			sendDutyCycleBroadcastThread.detach();
		}

		while(!_stopDutyCycleThread && _dutyCycleCounter < cycleLength - 40)
		{
			std::this_thread::sleep_for(sleepingTime);
			_dutyCycleCounter += 8;
		}
		if(_stopDutyCycleThread) break;

		timePoint = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		std::cout << "Correcting time mismatch of " << std::dec << ((nextDutyCycleEvent - 10000000 - timePoint) / 1000) << "ms." << std::endl;
		std::this_thread::sleep_for(std::chrono::microseconds(nextDutyCycleEvent - timePoint - 5000000));
		if(_stopDutyCycleThread) break;

		timePoint = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		std::this_thread::sleep_for(std::chrono::microseconds(nextDutyCycleEvent - timePoint - 2000000));
		if(_stopDutyCycleThread) break;

		std::thread sendDutyCyclePacketThread(&HM_CC_TC::sendDutyCyclePacket, this, _messageCounter[1], nextDutyCycleEvent);

		sched_param schedParam;
		int policy;
		pthread_getschedparam(sendDutyCyclePacketThread.native_handle(), &policy, &schedParam);
		schedParam.sched_priority = 99;
		if(!pthread_setschedparam(sendDutyCyclePacketThread.native_handle(), SCHED_FIFO, &schedParam)) throw(Exception("Error: Could not set thread priority."));
		sendDutyCyclePacketThread.detach();

		_lastDutyCycleEvent = nextDutyCycleEvent;
		cycleLength = calculateCycleLength(_messageCounter[1]);
		_messageCounter[1]++;
		std::ostringstream command;
		command << "UPDATE devices SET dutyCycleMessageCounter=" << std::dec << (int32_t)_messageCounter.at(1) << ",lastDutyCycle=" << _lastDutyCycleEvent;
		GD::db.executeCommand(command.str());

		_dutyCycleCounter = 0;
	}
}

void HM_CC_TC::sendDutyCycleBroadcast()
{

	std::vector<uint8_t> payload;
	payload.push_back((_temperature & 0xFF00) >> 8);
	payload.push_back(_temperature & 0xFF);
	payload.push_back(_humidity);
	std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(_messageCounter[1], 0x86, 0x70, _address, 0, payload));
	sendPacket(packet);
}

void HM_CC_TC::sendDutyCyclePacket(uint8_t messageCounter, int64_t sendingTime)
{
	try
	{
		int32_t address = getNextDutyCycleDeviceAddress();
		if(GD::debugLevel >= 5)	std::cout << "Next HM-CC-VD is 0x" << std::hex << address << std::dec << std::endl;
		if(address < 1)
		{
			if(GD::debugLevel >= 5) std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << " Not sending duty cycle packet, because no valve drives are paired to me." << std::endl;
			return;
		}
		std::vector<uint8_t> payload;
		payload.push_back(getAdjustmentCommand());
		payload.push_back(_newValveState);
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(messageCounter, 0xA2, 0x58, _address, address, payload));
		int64_t nanoseconds = (sendingTime - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - 1000000) * 1000;
		int32_t seconds = nanoseconds / 1000000000;
		nanoseconds -= seconds * 1000000000;
		struct timespec timeToSleep;
		timeToSleep.tv_sec = seconds;
		timeToSleep.tv_nsec = nanoseconds;
		nanosleep(&timeToSleep, NULL);

		nanoseconds = (sendingTime - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - 500000) * 1000;
		timeToSleep.tv_sec = 0;
		timeToSleep.tv_nsec = nanoseconds;
		nanosleep(&timeToSleep, NULL);

		nanoseconds = (sendingTime - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - 100000) * 1000;
		timeToSleep.tv_nsec = nanoseconds;
		nanosleep(&timeToSleep, NULL);

		nanoseconds = (sendingTime - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - 30000) * 1000;
		timeToSleep.tv_nsec = nanoseconds;
		nanosleep(&timeToSleep, NULL);

		nanoseconds = (sendingTime - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) * 1000;
		timeToSleep.tv_nsec = nanoseconds;
		nanosleep(&timeToSleep, NULL);
		if(_stopDutyCycleThread) return;

		int64_t timePoint = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		GD::rfDevice->sendPacket(packet);
		_valveState = _newValveState;
		int64_t timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - timePoint;
		std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() << ": Sending took " << timePassed << "ms." << std::endl;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
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
	try
	{
		_peersMutex.lock();
		if(_peers.size() == 0)
		{
			_peersMutex.unlock();
			return -1;
		}
		int i = 0;
		std::unordered_map<int32_t, std::shared_ptr<Peer>>::iterator j = (_currentDutyCycleDeviceAddress == -1) ? _peers.begin() : _peers.find(_currentDutyCycleDeviceAddress);
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
			if(j->second->deviceType == HMDeviceTypes::HMCCVD)
			{
				_currentDutyCycleDeviceAddress = j->first;
				_peersMutex.unlock();
				return _currentDutyCycleDeviceAddress;
			}
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
	_peersMutex.unlock();
	return -1;
}

void HM_CC_TC::sendConfigParams(int32_t messageCounter, int32_t destinationAddress, std::shared_ptr<BidCoSPacket> packet)
{
    HomeMaticDevice::sendConfigParams(messageCounter, destinationAddress, packet);
}

std::shared_ptr<Peer> HM_CC_TC::createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
    std::shared_ptr<Peer> peer(new Peer(false));
    peer->address = address;
    peer->firmwareVersion = firmwareVersion;
    peer->deviceType = deviceType;
    peer->messageCounter = 0;
    peer->remoteChannel = remoteChannel;
    if(deviceType == HMDeviceTypes::HMCCVD || deviceType == HMDeviceTypes::HMUNKNOWN) peer->localChannel = 2; else peer->localChannel = 0;
    peer->setSerialNumber(serialNumber);
    return peer;
}

void HM_CC_TC::reset()
{
    HomeMaticDevice::reset();
}

void HM_CC_TC::handleSetValveState(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		_newValveState = packet->payload()->at(0);
		if(GD::debugLevel >= 5) std::cout << "New valve state: " << _newValveState << std::endl;
		sendOK(messageCounter, packet->senderAddress());
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
}

void HM_CC_TC::handleConfigPeerAdd(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		HomeMaticDevice::handleConfigPeerAdd(messageCounter, packet);

		uint8_t channel = packet->payload()->at(0);
		int32_t address = (packet->payload()->at(2) << 16) + (packet->payload()->at(3) << 8) + (packet->payload()->at(4));
		if(channel == 2 && _peers.size() < 20)
		{
			_peersMutex.lock();
			_peers[address]->deviceType = HMDeviceTypes::HMCCVD;
			_peersMutex.unlock();
			if(GD::debugLevel >= 5) std::cout << "Added HM-CC-VD with address 0x" << std::hex << address << std::dec << std::endl;
		}
	}
	catch(const std::exception& ex)
    {
		_peersMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_peersMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_peersMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}

void HM_CC_TC::handleSetPoint(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
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
			payload.push_back(0);
			payload.push_back(0x4D); //RSSI
			std::shared_ptr<BidCoSPacket> response(new BidCoSPacket(packet->messageCounter(), 0x80, 0x02, _address, packet->senderAddress(), payload));
			sendPacket(response);
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
}

void HM_CC_TC::handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		//Block pairing, when paired to central
		if(_centralAddress) //The central does not send packets with message type 0x00, so we don't have to allow pairing from the central here
		{
			sendNOK(messageCounter, packet->senderAddress());
			return;
		}
		std::shared_ptr<Peer> peer = createPeer(packet->senderAddress(), (int32_t)packet->payload()->at(0), (HMDeviceTypes)((packet->payload()->at(1) << 8) + packet->payload()->at(2)), std::string(), (int32_t)packet->payload()->at(15), 0, packet);
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::PAIRING, packet->senderAddress());
		queue->peer = peer;
		queue->push(_messages->find(DIRECTIONOUT, 0x00, std::vector<std::pair<uint32_t, int32_t>>()), packet);
		queue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		queue->push(_messages->find(DIRECTIONOUT, 0x01, std::vector<std::pair<uint32_t, int32_t>> { std::pair<uint32_t, int32_t>(0x01, 0x04) }), packet);
		//The 0x10 response with the config does not have to be part of the queue.
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
}

void HM_CC_TC::handleConfigParamResponse(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		HomeMaticDevice::handleConfigParamResponse(messageCounter, packet);

		_currentList = packet->payload()->at(6);
		std::shared_ptr<Peer> peer = getPeer(packet->senderAddress());
		if(!peer) return;
		for(int i = 7; i < (signed)packet->payload()->size() - 2; i+=2)
		{
			int32_t index = packet->payload()->at(i);
			if(peer->config.find(index) == peer->config.end()) continue;
			peer->config.at(index) = packet->payload()->at(i + 1);
			std::cout << "0x" << std::setw(6) << std::hex << _address;
			std::cout << ": Config of device 0x" << std::setw(6) << packet->senderAddress() << " at index " << std::setw(2) << (int32_t)(packet->payload()->at(i)) << " set to " << std::setw(2) << (int32_t)(packet->payload()->at(i + 1)) << std::dec << std::endl;
		}
		sendOK(messageCounter, packet->senderAddress());
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
}

void HM_CC_TC::sendRequestConfig(int32_t messageCounter, int32_t controlByte, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(_bidCoSQueueManager.get(packet->senderAddress()) == nullptr) return;
		std::vector<uint8_t> payload;
		payload.push_back(_bidCoSQueueManager.get(packet->senderAddress())->peer->remoteChannel);
		payload.push_back(0x04);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0x05);
		std::shared_ptr<BidCoSPacket> requestConfig(new BidCoSPacket(messageCounter, 0xA0, 0x01, _address, packet->senderAddress(), payload));
		sendPacket(requestConfig);
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
}

void HM_CC_TC::handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(!queue) return;
		queue->pop(); //Messages are not popped by default.
		if(queue->getQueueType() == BidCoSQueueType::PAIRING)
		{
			_peersMutex.lock();
			if(_peers.size() > 20)
			{
				_peersMutex.unlock();
				return;
			}
			_peers[queue->peer->address] = queue->peer;
			_peersMutex.unlock();
			_pairing = false;
		}
	}
	catch(const std::exception& ex)
    {
		_peersMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_peersMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_peersMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
}
