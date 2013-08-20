#include "HM-CC-TC.h"
#include "../GD.h"
#include "../HelperFunctions.h"

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
	try
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

int64_t HM_CC_TC::calculateLastDutyCycleEvent()
{
	try
	{
		int64_t now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if(now - _lastDutyCycleEvent > 1800000000) return -1; //Duty cycle is out of sync anyway so don't bother to calculate
		int64_t nextDutyCycleEvent = _lastDutyCycleEvent;
		int64_t lastDutyCycleEvent = _lastDutyCycleEvent;
		_messageCounter[1]--; //The saved message counter is the current one, but the calculation has to use the last one
		while(nextDutyCycleEvent < now + 25000000)
		{
			lastDutyCycleEvent = nextDutyCycleEvent;
			nextDutyCycleEvent = lastDutyCycleEvent + (calculateCycleLength(_messageCounter[1]) * 250000) + _dutyCycleTimeOffset;
			_messageCounter[1]++;
		}
		HelperFunctions::printDebug("Debug: Setting last duty cycle event to: " + std::to_string(lastDutyCycleEvent));
		return lastDutyCycleEvent;
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
    return 0;
}

void HM_CC_TC::setUpConfig()
{
	try
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

void HM_CC_TC::setUpBidCoSMessages()
{
	try
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

HM_CC_TC::~HM_CC_TC()
{
	try
	{
		stopHMCCTCThreads();
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

void HM_CC_TC::stopHMCCTCThreads()
{
	try
	{
		HomeMaticDevice::stopThreads();
		_stopDutyCycleThread = true;
		if(_dutyCycleThread && _dutyCycleThread->joinable())
		{
			_dutyCycleThread->join();
			//Wait a little bit, so subthreads are stopped, too (e. g. sendDutyCyclePacket)
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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

std::string HM_CC_TC::serialize()
{
	try
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
    return "";
}

void HM_CC_TC::unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent)
{
	try
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

std::string HM_CC_TC::handleCLICommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;
		std::string response(HomeMaticDevice::handleCLICommand(command));
		if(command == "help")
		{
			stringStream << response << "duty cycle counter\tPrints the value of the duty cycle counter" << std::endl;
			return stringStream.str();
		}
		else if(!response.empty()) return response;
		else if(command == "duty cycle counter")
		{
			stringStream << "Duty cycle counter: " << std::dec << _dutyCycleCounter << " (" << ((_dutyCycleCounter * 250) / 1000) << "s)" << std::endl;
			return stringStream.str();
		}
		else return "Unknown command.\n";
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
    return "Error executing command. See log file for more details.\n";
}

void HM_CC_TC::setValveState(int32_t valveState)
{
	try
	{
		valveState *= 256;
		//Round up if necessary. I don't use double for calculation, because hopefully this is faster.
		if(valveState % 100 >= 50) valveState = (valveState / 100) + 1; else valveState /= 100;
		_newValveState = valveState;
		if(_newValveState > 255) _newValveState = 255;
		if(_newValveState < 0) _newValveState = 0;
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

void HM_CC_TC::startDutyCycle(int64_t lastDutyCycleEvent)
{
	try
	{
		_dutyCycleThread.reset(new std::thread(&HM_CC_TC::dutyCycleThread, this, lastDutyCycleEvent));
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

void HM_CC_TC::dutyCycleThread(int64_t lastDutyCycleEvent)
{
	try
	{
		int64_t nextDutyCycleEvent = (lastDutyCycleEvent == -1) ? std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() : lastDutyCycleEvent;
		_lastDutyCycleEvent = nextDutyCycleEvent;
		int64_t timePoint;
		int64_t cycleTime;
		int32_t cycleLength = calculateCycleLength(_messageCounter[1] - 1); //The calculation has to use the last message counter
		_dutyCycleCounter = (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - _lastDutyCycleEvent) / 250000;
		_dutyCycleCounter = (_dutyCycleCounter % 8 > 3) ? _dutyCycleCounter + (8 - (_dutyCycleCounter % 8)) : _dutyCycleCounter - (_dutyCycleCounter % 8);
		if(_dutyCycleCounter > 0) HelperFunctions::printDebug("Debug: Skipping " + std::to_string(_dutyCycleCounter * 250) + " ms of duty cycle.");
		while(!_stopDutyCycleThread)
		{
			try
			{
				cycleTime = cycleLength * 250000;
				nextDutyCycleEvent += cycleTime + _dutyCycleTimeOffset; //Add offset every cycle. This is very important! Without it, 20% of the packets are sent too early.
				HelperFunctions::printDebug("Next duty cycle: " + std::to_string(nextDutyCycleEvent / 1000) + " (in " + std::to_string(cycleTime / 1000) + " ms) with message counter 0x" + HelperFunctions::getHexString(_messageCounter[1]));

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

				setDecalcification();

				timePoint = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
				HelperFunctions::printDebug("Correcting time mismatch of " + std::to_string((nextDutyCycleEvent - 10000000 - timePoint) / 1000) + "ms.");
				std::this_thread::sleep_for(std::chrono::microseconds(nextDutyCycleEvent - timePoint - 5000000));
				if(_stopDutyCycleThread) break;

				timePoint = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
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

void HM_CC_TC::setDecalcification()
{
	try
	{
		std::time_t time1 = std::time(nullptr);
		std::tm* time2 = std::localtime(&time1);
		if(time2->tm_wday == 6 && time2->tm_hour == 14 && time2->tm_min >= 0 && time2->tm_min <= 3)
		{
			try
			{
				_peersMutex.lock();
				for(std::unordered_map<int32_t, std::shared_ptr<Peer>>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
				{
					i->second->config[0xFFFF] = 4;
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
			_peersMutex.unlock();
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

void HM_CC_TC::sendDutyCycleBroadcast()
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back((_temperature & 0xFF00) >> 8);
		payload.push_back(_temperature & 0xFF);
		payload.push_back(_humidity);
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(_messageCounter[1], 0x86, 0x70, _address, 0, payload));
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

void HM_CC_TC::sendDutyCyclePacket(uint8_t messageCounter, int64_t sendingTime)
{
	try
	{
		if(_stopDutyCycleThread) return;
		int32_t address = getNextDutyCycleDeviceAddress();
		HelperFunctions::printDebug("Debug: 0x" + HelperFunctions::getHexString(_address) + ": Next HM-CC-VD is 0x" + HelperFunctions::getHexString(_address));
		if(address < 1)
		{
			HelperFunctions::printDebug("Debug: Not sending duty cycle packet, because no valve drives are paired to me.");
			return;
		}
		std::vector<uint8_t> payload;
		payload.push_back(getAdjustmentCommand(address));
		payload.push_back(_newValveState);
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(messageCounter, 0xA2, 0x58, _address, address, payload));
		int64_t nanoseconds = (sendingTime - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - 1000000) * 1000;
		int32_t seconds = nanoseconds / 1000000000;
		nanoseconds -= seconds * 1000000000;
		struct timespec timeToSleep;
		timeToSleep.tv_sec = seconds;
		timeToSleep.tv_nsec = nanoseconds;
		nanosleep(&timeToSleep, NULL);
		if(_stopDutyCycleThread) return;

		nanoseconds = (sendingTime - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - 500000) * 1000;
		timeToSleep.tv_sec = 0;
		timeToSleep.tv_nsec = nanoseconds;
		nanosleep(&timeToSleep, NULL);
		if(_stopDutyCycleThread) return;

		nanoseconds = (sendingTime - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - 100000) * 1000;
		timeToSleep.tv_nsec = nanoseconds;
		nanosleep(&timeToSleep, NULL);

		nanoseconds = (sendingTime - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - 30000) * 1000;
		timeToSleep.tv_nsec = nanoseconds;
		nanosleep(&timeToSleep, NULL);

		nanoseconds = (sendingTime - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()) * 1000;
		timeToSleep.tv_nsec = nanoseconds;
		nanosleep(&timeToSleep, NULL);
		if(_stopDutyCycleThread) return;

		int64_t timePoint = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

		GD::rfDevice->sendPacket(packet);
		_valveState = _newValveState;
		int64_t timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - timePoint;
		HelperFunctions::printDebug("Debug: " + HelperFunctions::getHexString(_address) + " Sending took " + std::to_string(timePassed) + "ms.");
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

int32_t HM_CC_TC::getAdjustmentCommand(int32_t peerAddress)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(peerAddress));
		if(peer && peer->config[0xFFFF] == 4) return 4;
		else if(_setPointTemperature == 0) return 2; //OFF
		else if(_setPointTemperature == 60) return 3; //ON
		else
		{
			if(_newValveState != _valveState) return 3; else return 0;
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
			if(j->second && j->second->deviceType == HMDeviceTypes::HMCCVD)
			{
				_currentDutyCycleDeviceAddress = j->first;
				_peersMutex.unlock();
				return _currentDutyCycleDeviceAddress;
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
	_peersMutex.unlock();
	return -1;
}

void HM_CC_TC::sendConfigParams(int32_t messageCounter, int32_t destinationAddress, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		HomeMaticDevice::sendConfigParams(messageCounter, destinationAddress, packet);
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

std::shared_ptr<Peer> HM_CC_TC::createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
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
    return std::shared_ptr<Peer>();
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
		HelperFunctions::printDebug("Debug: " + HelperFunctions::getHexString(_address) + ": New valve state: " + std::to_string(_newValveState));
		sendOK(messageCounter, packet->senderAddress());
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
			HelperFunctions::printMessage("0x" + HelperFunctions::getHexString(_address) + ": Added HM-CC-VD with address 0x" + HelperFunctions::getHexString(address));
		}
	}
	catch(const std::exception& ex)
    {
		_peersMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_peersMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_peersMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
			HelperFunctions::printInfo("Info: 0x" + HelperFunctions::getHexString(_address) + ": Config of device 0x" + HelperFunctions::getHexString(packet->senderAddress()) + " at index " + std::to_string(packet->payload()->at(i)) + " set to 0x" + HelperFunctions::getHexString(packet->payload()->at(i + 1)));
		}
		sendOK(messageCounter, packet->senderAddress());
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

void HM_CC_TC::handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(packet->senderAddress()));
		if(peer) peer->config[0xFFFF] = 0; //Decalcification done
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
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_peersMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_peersMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
