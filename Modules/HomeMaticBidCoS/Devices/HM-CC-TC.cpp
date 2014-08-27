/* Copyright 2013-2014 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "HM-CC-TC.h"
#include "../../Base/BaseLib.h"
#include "../GD.h"

namespace BidCoS
{
HM_CC_TC::HM_CC_TC(IDeviceEventSink* eventHandler) : HomeMaticDevice(eventHandler)
{
	init();
}

HM_CC_TC::HM_CC_TC(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : HomeMaticDevice(deviceID, serialNumber, address, eventHandler)
{
	init();
	if(deviceID == 0) startDutyCycle(-1); //Device is newly created
}

void HM_CC_TC::init()
{
	try
	{
		HomeMaticDevice::init();

		_deviceType = (uint32_t)DeviceType::HMCCTC;
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
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

int64_t HM_CC_TC::calculateLastDutyCycleEvent()
{
	try
	{
		if(_lastDutyCycleEvent < 0) _lastDutyCycleEvent = 0;
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
		GD::out.printDebug("Debug: Setting last duty cycle event to: " + std::to_string(lastDutyCycleEvent));
		return lastDutyCycleEvent;
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

HM_CC_TC::~HM_CC_TC()
{
	try
	{
		dispose();
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CC_TC::dispose()
{
	try
	{
		_stopDutyCycleThread = true;
		HomeMaticDevice::dispose();
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CC_TC::stopThreads()
{
	try
	{
		HomeMaticDevice::stopThreads();
		_stopDutyCycleThread = true;
		if(_dutyCycleThread.joinable()) _dutyCycleThread.join();
		if(_sendDutyCyclePacketThread.joinable()) _sendDutyCyclePacketThread.join();
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CC_TC::saveVariables()
{
	try
	{
		if(_deviceID == 0) return;
		HomeMaticDevice::saveVariables();
		saveVariable(1000, _currentDutyCycleDeviceAddress);
		saveVariable(1001, _temperature);
		saveVariable(1002, _setPointTemperature);
		saveVariable(1003, _humidity);
		saveVariable(1004, _valveState);
		saveVariable(1005, _newValveState);
		saveVariable(1006, _lastDutyCycleEvent);
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CC_TC::loadVariables()
{
	try
	{
		HomeMaticDevice::loadVariables();
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetDeviceVariables();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 1000:
				_currentDutyCycleDeviceAddress = row->second.at(3)->intValue;
				break;
			case 1001:
				_temperature = row->second.at(3)->intValue;
				break;
			case 1002:
				_setPointTemperature = row->second.at(3)->intValue;
				break;
			case 1003:
				_humidity = row->second.at(3)->intValue;
				break;
			case 1004:
				_valveState = row->second.at(3)->intValue;
				break;
			case 1005:
				_newValveState = row->second.at(3)->intValue;
				break;
			case 1006:
				_lastDutyCycleEvent = row->second.at(3)->intValue;
				break;
			}
		}
		startDutyCycle(calculateLastDutyCycleEvent());
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CC_TC::startDutyCycle(int64_t lastDutyCycleEvent)
{
	try
	{
		if(_dutyCycleThread.joinable())
		{
			GD::out.printCritical("HomeMatic BidCoS device " + std::to_string(_deviceID) + ": Duty cycle thread already started. Something went very wrong.");
			return;
		}
		_dutyCycleThread = std::thread(&HM_CC_TC::dutyCycleThread, this, lastDutyCycleEvent);
		BaseLib::Threads::setThreadPriority(_bl, _dutyCycleThread.native_handle(), 35);
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CC_TC::dutyCycleThread(int64_t lastDutyCycleEvent)
{
	try
	{
		int64_t nextDutyCycleEvent = (lastDutyCycleEvent < 0) ? std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() : lastDutyCycleEvent;
		_lastDutyCycleEvent = nextDutyCycleEvent;
		int64_t timePoint;
		int64_t cycleTime;
		uint32_t cycleLength = calculateCycleLength(_messageCounter[1] - 1); //The calculation has to use the last message counter
		_dutyCycleCounter = (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - _lastDutyCycleEvent) / 250000;
		if(_dutyCycleCounter < 0) _dutyCycleCounter = 0;
		if(_dutyCycleCounter > 0) GD::out.printDebug("Debug: Skipping " + std::to_string(_dutyCycleCounter * 250) + " ms of duty cycle.");
		//_dutyCycleCounter = (_dutyCycleCounter % 8 > 3) ? _dutyCycleCounter + (8 - (_dutyCycleCounter % 8)) : _dutyCycleCounter - (_dutyCycleCounter % 8);
		while(!_stopDutyCycleThread)
		{
			try
			{
				cycleTime = cycleLength * 250000;
				nextDutyCycleEvent += cycleTime + _dutyCycleTimeOffset; //Add offset every cycle. This is very important! Without it, 20% of the packets are sent too early.
				GD::out.printDebug("Next duty cycle: " + std::to_string(nextDutyCycleEvent / 1000) + " (in " + std::to_string(cycleTime / 1000) + " ms) with message counter 0x" + BaseLib::HelperFunctions::getHexString(_messageCounter[1]));

				std::chrono::milliseconds sleepingTime(250);
				while(!_stopDutyCycleThread && _dutyCycleCounter < (signed)cycleLength - 80)
				{
					std::this_thread::sleep_for(sleepingTime);
					_dutyCycleCounter += 1;
				}
				if(_stopDutyCycleThread) break;

				if(_dutyCycleBroadcast)
				{
					if(_sendDutyCyclePacketThread.joinable()) _sendDutyCyclePacketThread.join();
					_sendDutyCyclePacketThread = std::thread(&HM_CC_TC::sendDutyCycleBroadcast, this);
				}

				while(!_stopDutyCycleThread && _dutyCycleCounter < (signed)cycleLength - 40)
				{
					std::this_thread::sleep_for(sleepingTime);
					_dutyCycleCounter += 1;
				}
				if(_stopDutyCycleThread) break;

				setDecalcification();

				timePoint = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
				GD::out.printDebug("Correcting time mismatch of " + std::to_string((nextDutyCycleEvent - 10000000 - timePoint) / 1000) + "ms.");
				std::this_thread::sleep_for(std::chrono::microseconds(nextDutyCycleEvent - timePoint - 5000000));
				if(_stopDutyCycleThread) break;

				timePoint = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
				std::this_thread::sleep_for(std::chrono::microseconds(nextDutyCycleEvent - timePoint - 2000000));
				if(_stopDutyCycleThread) break;

				if(_sendDutyCyclePacketThread.joinable()) _sendDutyCyclePacketThread.join();
				_sendDutyCyclePacketThread = std::thread(&HM_CC_TC::sendDutyCyclePacket, this, _messageCounter[1], nextDutyCycleEvent);
				BaseLib::Threads::setThreadPriority(_bl, _sendDutyCyclePacketThread.native_handle(), 99);

				_lastDutyCycleEvent = nextDutyCycleEvent;
				cycleLength = calculateCycleLength(_messageCounter[1]);
				_messageCounter[1]++;
				saveVariable(1006, _lastDutyCycleEvent);
				saveMessageCounters();

				_dutyCycleCounter = 0;
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
				for(std::unordered_map<int32_t, std::shared_ptr<BaseLib::Systems::Peer>>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
				{
					std::shared_ptr<BidCoSPeer> peer(std::dynamic_pointer_cast<BidCoSPeer>(i->second));
					peer->config[0xFFFF] = 4;
				}
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			_peersMutex.unlock();
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		sendPacket(_physicalInterface, packet);
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CC_TC::sendDutyCyclePacket(uint8_t messageCounter, int64_t sendingTime)
{
	try
	{
		if(sendingTime < 0) sendingTime = 2000000;
		if(_stopDutyCycleThread) return;
		int32_t address = getNextDutyCycleDeviceAddress();
		GD::out.printDebug("Debug: HomeMatic BidCoS device " + std::to_string(_deviceID) + ": Next HM-CC-VD is 0x" + BaseLib::HelperFunctions::getHexString(_address));
		if(address < 1)
		{
			GD::out.printDebug("Debug: Not sending duty cycle packet, because no valve drives are paired to me.");
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

		_physicalInterface->sendPacket(packet);
		_valveState = _newValveState;
		int64_t timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() - timePoint;
		GD::out.printDebug("Debug: HomeMatic BidCoS device " + std::to_string(_deviceID) + ": Sending took " + std::to_string(timePassed) + "ms.");
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

int32_t HM_CC_TC::getAdjustmentCommand(int32_t peerAddress)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(peerAddress));
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
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return 0;
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
		std::unordered_map<int32_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator j = (_currentDutyCycleDeviceAddress == -1) ? _peers.begin() : _peers.find(_currentDutyCycleDeviceAddress);
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
			if(j->second && j->second->getDeviceType().type() == (uint32_t)DeviceType::HMCCVD)
			{
				_currentDutyCycleDeviceAddress = j->first;
				_peersMutex.unlock();
				return _currentDutyCycleDeviceAddress;
			}
		}
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<BidCoSPeer> HM_CC_TC::createPeer(int32_t address, int32_t firmwareVersion, BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet, bool save)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(new BidCoSPeer(_deviceID, false, this));
		peer->setAddress(address);
		peer->setFirmwareVersion(firmwareVersion);
		peer->setDeviceType(deviceType);
		peer->setMessageCounter(0);
		peer->setRemoteChannel(remoteChannel);
		if(deviceType.type() == (uint32_t)DeviceType::HMCCVD || deviceType.type() == (uint32_t)DeviceType::none) peer->setLocalChannel(2); else peer->setLocalChannel(0);
		peer->setSerialNumber(serialNumber);
		if(save) peer->save(true, true, false);
		return peer;
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<BidCoSPeer>();
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
		GD::out.printDebug("Debug: HomeMatic BidCoS device " + std::to_string(_deviceID) + ": New valve state: " + std::to_string(_newValveState));
		sendOK(messageCounter, packet->senderAddress());
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
			_peers[address]->setDeviceType(BaseLib::Systems::LogicalDeviceType(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS, (uint32_t)DeviceType::HMCCVD));
			_peersMutex.unlock();
			std::shared_ptr<BidCoSPeer> peer = getPeer(address);
			peer->save(true, true, false);
			GD::out.printMessage("HomeMatic BidCoS device " + std::to_string(_deviceID) + ": Added HM-CC-VD " + std::to_string(peer->getID()));
		}
	}
	catch(const std::exception& ex)
    {
		_peersMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_peersMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_peersMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
			sendPacket(_physicalInterface, response);
		}
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		BaseLib::Systems::LogicalDeviceType deviceType(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS, (packet->payload()->at(1) << 8) + packet->payload()->at(2));
		std::shared_ptr<BidCoSPeer> peer = createPeer(packet->senderAddress(), (int32_t)packet->payload()->at(0), deviceType, std::string(), (int32_t)packet->payload()->at(15), 0, packet);
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, _physicalInterface, BidCoSQueueType::PAIRING, packet->senderAddress());
		queue->peer = peer;
		queue->push(_messages->find(DIRECTIONOUT, 0x00, std::vector<std::pair<uint32_t, int32_t>>()), packet);
		queue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		queue->push(_messages->find(DIRECTIONOUT, 0x01, std::vector<std::pair<uint32_t, int32_t>> { std::pair<uint32_t, int32_t>(0x01, 0x04) }), packet);
		//The 0x10 response with the config does not have to be part of the queue.
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CC_TC::handleConfigParamResponse(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		HomeMaticDevice::handleConfigParamResponse(messageCounter, packet);

		_currentList = packet->payload()->at(6);
		std::shared_ptr<BidCoSPeer> peer = getPeer(packet->senderAddress());
		if(!peer) return;
		for(int i = 7; i < (signed)packet->payload()->size() - 2; i+=2)
		{
			int32_t index = packet->payload()->at(i);
			if(peer->config.find(index) == peer->config.end()) continue;
			peer->config.at(index) = packet->payload()->at(i + 1);
			GD::out.printInfo("Info: HomeMatic BidCoS device " + std::to_string(_deviceID) + ": Config of device with address 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress()) + " at index " + std::to_string(packet->payload()->at(i)) + " set to 0x" + BaseLib::HelperFunctions::getHexString(packet->payload()->at(i + 1)));
		}
		sendOK(messageCounter, packet->senderAddress());
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CC_TC::sendRequestConfig(int32_t messageCounter, int32_t controlByte, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(_bidCoSQueueManager.get(packet->senderAddress()) == nullptr) return;
		std::vector<uint8_t> payload;
		payload.push_back(_bidCoSQueueManager.get(packet->senderAddress())->peer->getRemoteChannel());
		payload.push_back(0x04);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0x05);
		std::shared_ptr<BidCoSPacket> requestConfig(new BidCoSPacket(messageCounter, 0xA0, 0x01, _address, packet->senderAddress(), payload));
		sendPacket(_physicalInterface, requestConfig);
	}
    catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CC_TC::handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(packet->senderAddress()));
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
			_peers[queue->peer->getAddress()] = queue->peer;
			_peersMutex.unlock();
			queue->peer->save(true, true, false);
			_pairing = false;
		}
	}
	catch(const std::exception& ex)
    {
		_peersMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_peersMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_peersMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
}
