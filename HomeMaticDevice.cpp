#include "HomeMaticDevice.h"
#include "GD.h"
#include "HelperFunctions.h"

HomeMaticDevice::HomeMaticDevice()
{
}

HomeMaticDevice::HomeMaticDevice(std::string serialNumber, int32_t address) : _address(address), _serialNumber(serialNumber)
{
}

void HomeMaticDevice::init()
{
	try
	{
		if(_initialized) return; //Prevent running init two times
		_messages = std::shared_ptr<BidCoSMessages>(new BidCoSMessages());
		_deviceType = HMDeviceTypes::HMUNKNOWN;

		GD::rfDevice->addHomeMaticDevice(this);

		_messageCounter[0] = 0; //Broadcast message counter
		_messageCounter[1] = 0; //Duty cycle message counter

		setUpBidCoSMessages();
		_workerThread = std::thread(&HomeMaticDevice::worker, this);
		HelperFunctions::setThreadPriority(_workerThread.native_handle(), 19);
		_initialized = true;
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

void HomeMaticDevice::setUpConfig()
{
	try
	{
		_config[0x00][0x00][0x02] = 0; //Paired to central (Possible values: 0 - false, 1 - true);
		_config[0x00][0x00][0x0A] = 0; //First byte of central address
		_config[0x00][0x00][0x0B] = 0; //Second byte of central address
		_config[0x00][0x00][0x0C] = 0; //Third byte of central address
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

void HomeMaticDevice::setUpBidCoSMessages()
{
	try
	{
		_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x00, this, NOACCESS, FULLACCESS, &HomeMaticDevice::handlePairingRequest)));

		std::shared_ptr<BidCoSMessage> message(new BidCoSMessage(0x11, this, ACCESSCENTRAL | ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleReset));
		message->addSubtype(0x00, 0x04);
		_messages->add(message);

		message = std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x01, this, ACCESSCENTRAL | ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigPeerAdd));
		message->addSubtype(0x01, 0x01);
		_messages->add(message);

		message = std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigPeerDelete));
		message->addSubtype(0x01, 0x02);
		_messages->add(message);

		message = std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigParamRequest));
		message->addSubtype(0x01, 0x04);
		_messages->add(message);

		message = std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, FULLACCESS, &HomeMaticDevice::handleConfigStart));
		message->addSubtype(0x01, 0x05);
		_messages->add(message);

		message = std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigWriteIndex));
		message->addSubtype(0x01, 0x08);
		_messages->add(message);

		message = std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x01, this, NOACCESS, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigWriteIndex));
		message->addSubtype(0x01, 0x08);
		message->addSubtype(0x02, 0x02);
		message->addSubtype(0x03, 0x01);
		_messages->add(message);

		message = std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x01, this, ACCESSCENTRAL | ACCESSDESTISME, &HomeMaticDevice::handleConfigWriteIndex));
		message->addSubtype(0x01, 0x08);
		message->addSubtype(0x02, 0x02);
		message->addSubtype(0x03, 0x00);
		_messages->add(message);

		message = std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME | ACCESSUNPAIRING, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigEnd));
		message->addSubtype(0x01, 0x06);
		_messages->add(message);

		message = std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x01, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigRequestPeers));
		message->addSubtype(0x01, 0x03);
		_messages->add(message);

		_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x3F, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleTimeRequest)));

		_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x02, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleAck)));
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

HomeMaticDevice::~HomeMaticDevice()
{
	try
	{
		dispose();
		std::this_thread::sleep_for(std::chrono::milliseconds(20)); //Wait for received packets, without this, program sometimes SIGABRTs
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

void HomeMaticDevice::dispose()
{
	if(_disposing) return;
	_disposing = true;
	HelperFunctions::printDebug("Removing device 0x" + HelperFunctions::getHexString(_address) + " from CUL event queue...");
	GD::rfDevice->removeHomeMaticDevice(this);
	int64_t startTime = HelperFunctions::getTime();
	stopThreads();
	int64_t timeDifference = HelperFunctions::getTime() - startTime;
	//Packets might still arrive, after removing this device from the rfDevice, so sleep a little bit
	if(timeDifference >= 0 && timeDifference < 100) std::this_thread::sleep_for(std::chrono::milliseconds(100 - timeDifference));
}

void HomeMaticDevice::stopThreads()
{
	try
	{
		_bidCoSQueueManager.dispose(false);
		_receivedPackets.dispose(false);
		_sentPackets.dispose(false);

		_peersMutex.lock();
		for(std::unordered_map<int32_t, std::shared_ptr<Peer>>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			i->second->dispose();
		}
		_peersMutex.unlock();

		_stopWorkerThread = true;
		if(_workerThread.joinable())
		{
			HelperFunctions::printDebug("Debug: Waiting for worker thread of device 0x" + HelperFunctions::getHexString(_address) + "...");
			_workerThread.join();
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

void HomeMaticDevice::checkForDeadlock()
{
	try
	{
		if(!_peersMutex.try_lock_for(std::chrono::seconds(3)))
		{
			HelperFunctions::printError("Critical: _peersMutex is deadlocked. Unlocking... This might crash the program.");
			_peersMutex.unlock();
		}
		else _peersMutex.unlock();
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

void HomeMaticDevice::worker()
{
	try
	{
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

int32_t HomeMaticDevice::getHexInput()
{
	std::string input;
	std::cin >> input;
	int32_t intInput = -1;
	try	{ intInput = std::stoll(input, 0, 16); } catch(...) {}
    return intInput;
}

std::string HomeMaticDevice::handleCLICommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;
		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "pair\t\t\tEnables pairing mode" << std::endl;
			stringStream << "reset\t\t\tResets the device to it's default settings" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 4, "pair") == 0)
		{
			int32_t duration = 20;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1)
				{
					index++;
					continue;
				}
				else if(index == 1)
				{
					if(element == "help")
					{
						stringStream << "Description: This command enables pairing mode." << std::endl;
						stringStream << "Usage: pair [DURATION]" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  DURATION:\tOptional duration in seconds to stay in pairing mode." << std::endl;
						return stringStream.str();
					}
					duration = HelperFunctions::getNumber(element, true);
					if(duration < 5 || duration > 3600) return "Invalid duration. Duration has to be greater than 5 and less than 3600.\n";
				}
				index++;
			}

			std::thread t(&HomeMaticDevice::pairDevice, this, duration * 1000);
			t.detach();
			stringStream << "Pairing mode enabled." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 5, "reset") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1)
				{
					index++;
					continue;
				}
				else if(index == 1)
				{
					if(element == "help")
					{
						stringStream << "Description: This command resets the device to it's default settings." << std::endl;
						stringStream << "Usage: reset" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			reset();
			stringStream << "Device reset." << std::endl;
			return stringStream.str();
		}
		else return "";
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

void HomeMaticDevice::unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent)
{
	try
	{
		HelperFunctions::printDebug("Debug: Unserializing: " + serializedObject);
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
		uint32_t configSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
		for(uint32_t i = 0; i < configSize; i++)
		{
			int32_t channel = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			uint32_t listCount = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
			for(uint32_t j = 0; j < listCount; j++)
			{
				int32_t listIndex = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				uint32_t listSize = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
				for(uint32_t k = 0; k < listSize; k++)
				{
					_config[channel][listIndex][std::stoll(serializedObject.substr(pos, 8), 0, 16)] = std::stoll(serializedObject.substr(pos + 8, 8), 0, 16); pos += 16;
				}
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

void HomeMaticDevice::saveToDatabase()
{
	try
	{
		std::ostringstream command;
		command << "SELECT 1 FROM devices WHERE address=" << std::dec << _address;
		_databaseMutex.lock();
		DataTable result = GD::db.executeCommand(command.str());
		if(result.empty())
		{
			std::ostringstream command2;
			command2 << "INSERT INTO devices VALUES(" << _address << "," << (uint32_t)_deviceType << ",'" << serialize() << "'," << (int32_t)_messageCounter.at(1) << "," << _lastDutyCycleEvent << ")";
			GD::db.executeCommand(command2.str());
		}
		else
		{
			std::ostringstream command2;
			command2 << "UPDATE devices SET serializedObject='" << serialize() << "',dutyCycleMessageCounter=" << (int32_t)_messageCounter.at(1) << ",lastDutyCycle=" << _lastDutyCycleEvent << " WHERE address=" << std::dec << _address;
			GD::db.executeCommand(command2.str());
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

void HomeMaticDevice::loadPeersFromDatabase()
{
	try
	{
		_peersMutex.lock();
		_databaseMutex.lock();
		std::ostringstream command;
		command << "SELECT * FROM peers WHERE parent=" << std::dec << std::to_string(_address);
		DataTable rows = GD::db.executeCommand(command.str());
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			if(row->second.at(2)->textValue.empty()) continue;
			std::shared_ptr<Peer> peer(new Peer(row->second.at(2)->textValue, this, isCentral()));
			if(!peer->rpcDevice) continue;
			_peers[peer->address] = peer;
			if(!peer->getSerialNumber().empty()) _peersBySerial[peer->getSerialNumber()] = peer;
			if(!peer->team.serialNumber.empty())
			{
				if(_peersBySerial.find(peer->team.serialNumber) == _peersBySerial.end())
				{
					std::shared_ptr<Peer> team = createTeam(peer->team.address, peer->deviceType, peer->team.serialNumber);
					team->rpcDevice = peer->rpcDevice->team;
					team->initializeCentralConfig();
					_peersBySerial[team->getSerialNumber()] = team;
				}
				for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
				{
					if(i->second->hasTeam)
					{
						_peersBySerial[peer->team.serialNumber]->teamChannels.push_back(std::pair<std::string, uint32_t>(peer->getSerialNumber(), peer->team.channel));
						break;
					}
				}
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
    _peersMutex.unlock();
}

void HomeMaticDevice::deletePeersFromDatabase()
{
	try
	{
		_databaseMutex.lock();
		std::ostringstream command;
		command << "DELETE FROM peers WHERE parent=" << std::dec << _address;
		GD::db.executeCommand(command.str());
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

void HomeMaticDevice::savePeersToDatabase()
{
	try
	{
		_peersMutex.lock();
		_databaseMutex.lock();
		for(std::unordered_map<int32_t, std::shared_ptr<Peer>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			//We are always printing this, because the init script needs it
			HelperFunctions::printMessage(" - Saving peer 0x" + HelperFunctions::getHexString(i->second->address, 6) + "...");
			i->second->saveToDatabase(_address);
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
	_peersMutex.unlock();
}

std::string HomeMaticDevice::serialize()
{
	try
	{
		std::ostringstream stringstream;
		stringstream << std::hex << std::uppercase << std::setfill('0');
		stringstream << std::setw(8) << (uint32_t)_deviceType;
		stringstream << std::setw(8) << _address;
		if(_serialNumber.size() != 10) stringstream << "0000000000"; else stringstream << _serialNumber;
		stringstream << std::setw(2) << _firmwareVersion;
		stringstream << std::setw(8) << _centralAddress;
		//cleanUpMessageCounters();
		stringstream << std::setw(8) << _messageCounter.size();
		for(std::unordered_map<int32_t, uint8_t>::const_iterator i = _messageCounter.begin(); i != _messageCounter.end(); ++i)
		{
			stringstream << std::setw(8) << i->first;
			stringstream << std::setw(2) << (int32_t)i->second;
		}
		stringstream << std::setw(8) << _config.size();
		for(std::unordered_map<int32_t, std::unordered_map<int32_t, std::map<int32_t, int32_t>>>::const_iterator i = _config.begin(); i != _config.end(); ++i)
		{
			stringstream << std::setw(8) << i->first; //channel
			stringstream << std::setw(8) << i->second.size();
			for(std::unordered_map<int32_t, std::map<int32_t, int32_t>>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringstream << std::setw(8) << j->first; //List index
				stringstream << std::setw(8) << j->second.size();
				for(std::map<int32_t, int32_t>::const_iterator k = j->second.begin(); k != j->second.end(); ++k)
				{
					stringstream << std::setw(8) << k->first;
					stringstream << std::setw(8) << k->second;
				}
			}
		}
		stringstream << std::dec;
		return stringstream.str();
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

bool HomeMaticDevice::pairDevice(int32_t timeout)
{
	try
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
    return false;
}

bool HomeMaticDevice::packetReceived(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(_disposing) return false;
		_receivedPackets.set(packet->senderAddress(), packet, packet->timeReceived());
		std::shared_ptr<BidCoSMessage> message = _messages->find(DIRECTIONIN, packet);
		if(message && message->checkAccess(packet, _bidCoSQueueManager.get(packet->senderAddress())))
		{
			HelperFunctions::printDebug("Debug: Device " + HelperFunctions::getHexString(_address) + ": Access granted for packet " + packet->hexString(), 6);
			message->invokeMessageHandlerIncoming(packet);
			return true;
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
    return false;
}

void HomeMaticDevice::sendPacket(std::shared_ptr<BidCoSPacket> packet, bool stealthy)
{
	try
	{
		uint32_t responseDelay = GD::settings.bidCoSResponseDelay();
		std::shared_ptr<BidCoSPacketInfo> packetInfo = _sentPackets.getInfo(packet->destinationAddress());
		if(!stealthy) _sentPackets.set(packet->destinationAddress(), packet);
		if(packetInfo)
		{
			int64_t timeDifference = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - packetInfo->time;
			if(timeDifference < responseDelay)
			{
				packetInfo->time += responseDelay - timeDifference; //Set to sending time
				std::this_thread::sleep_for(std::chrono::milliseconds(responseDelay - timeDifference));
			}
		}
		if(stealthy) _sentPackets.keepAlive(packet->destinationAddress());
		packetInfo = _receivedPackets.getInfo(packet->destinationAddress());
		if(packetInfo)
		{
			int64_t time = HelperFunctions::getTime();
			int64_t timeDifference = time - packetInfo->time;
			if(timeDifference >= 0 && timeDifference < responseDelay)
			{
				int64_t sleepingTime = responseDelay - timeDifference;
				if(sleepingTime > 1) sleepingTime -= 1;
				packet->setTimeSending(time + sleepingTime + 1);
				std::this_thread::sleep_for(std::chrono::milliseconds(sleepingTime));
			}
			//Set time to now. This is necessary if two packets are sent after each other without a response in between
			packetInfo->time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}
		else HelperFunctions::printDebug("Debug: Sending packet " + packet->hexString() + " immediately, because it seems it is no response (no packet information found).", 7);
		GD::rfDevice->sendPacket(packet);
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

void HomeMaticDevice::sendPacketMultipleTimes(std::shared_ptr<BidCoSPacket> packet, int32_t peerAddress, int32_t count, int32_t delay, bool useCentralMessageCounter, bool isThread)
{
	try
	{
		if(!isThread)
		{
			std::thread t(&HomeMaticDevice::sendPacketMultipleTimes, this, packet, peerAddress, count, delay, useCentralMessageCounter, true);
			t.detach();
			return;
		}
		std::shared_ptr<Peer> peer = getPeer(peerAddress);
		if(!peer) return;
		for(uint32_t i = 0; i < count; i++)
		{
			_sentPackets.set(packet->destinationAddress(), packet);
			int64_t start = HelperFunctions::getTime();
			GD::rfDevice->sendPacket(packet);
			if(useCentralMessageCounter)
			{
				packet->setMessageCounter(_messageCounter[0]);
				_messageCounter[0]++;
			}
			else
			{
				packet->setMessageCounter(peer->messageCounter);
				peer->messageCounter++;
			}
			int32_t difference = HelperFunctions::getTime() - start;
			if(difference < delay - 10) std::this_thread::sleep_for(std::chrono::milliseconds(delay - difference));
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

void HomeMaticDevice::handleReset(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
    sendOK(messageCounter, packet->senderAddress());
    reset();
}

void HomeMaticDevice::reset()
{
	try
	{
		_messageCounter.clear();
		_centralAddress = 0;
		_pairing = false;
		_justPairedToOrThroughCentral = false;
		_peersMutex.lock();
		_peers.clear();
		_peersMutex.unlock();
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

void HomeMaticDevice::handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
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
			if(_bidCoSQueueManager.get(packet->senderAddress()) == nullptr) return;
			_peersMutex.lock();
			_peers[packet->senderAddress()] = _bidCoSQueueManager.get(packet->senderAddress())->peer;
			_peersMutex.unlock();
			_pairing = false;
			sendOK(messageCounter, packet->senderAddress());
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

void HomeMaticDevice::handleWakeUp(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	sendOK(messageCounter, packet->senderAddress());
}

std::shared_ptr<Peer> HomeMaticDevice::createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
    return std::shared_ptr<Peer>(new Peer(isCentral()));
}

std::shared_ptr<Peer> HomeMaticDevice::createTeam(int32_t address, HMDeviceTypes deviceType, std::string serialNumber)
{
	try
	{
		std::shared_ptr<Peer> team(new Peer(isCentral()));
		team->address = address;
		team->deviceType = deviceType;
		team->setSerialNumber(serialNumber);
		return team;
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

void HomeMaticDevice::handleConfigRequestPeers(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
    if(!peerExists(packet->senderAddress()) || packet->destinationAddress() != _address) return;
    sendPeerList(messageCounter, packet->senderAddress(), packet->payload()->at(0));
}

void HomeMaticDevice::handleConfigPeerDelete(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		int32_t address = (packet->payload()->at(2) << 16) + (packet->payload()->at(3) << 8) + (packet->payload()->at(4));
		if(!peerExists(address)) return;
		HelperFunctions::printMessage("0x" + HelperFunctions::getHexString(_address) + ": Unpaired from peer 0x" + HelperFunctions::getHexString(address));
		sendOK(messageCounter, packet->senderAddress());
		_peersMutex.lock();
		try
		{
			if(_peers[address]->deviceType != HMDeviceTypes::HMRCV50)
			{
				_peers[address]->deleteFromDatabase(_address);
				_peers.erase(address); //Unpair. Unpairing of HMRCV50 is done through CONFIG_WRITE_INDEX
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

void HomeMaticDevice::handleConfigEnd(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		sendOK(messageCounter, packet->senderAddress());
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(!queue) return;
		if(queue->getQueueType() == BidCoSQueueType::PAIRINGCENTRAL)
		{
			_peersMutex.lock();
			try
			{
				_peers[queue->peer->address] = queue->peer;
				_centralAddress = queue->peer->address;
			}
			catch(const std::exception& ex)
			{
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			_peersMutex.unlock();
			_justPairedToOrThroughCentral = true;
			_pairing = false;
		}
		else if(queue->getQueueType() == BidCoSQueueType::UNPAIRING)
		{
			_peersMutex.lock();
			try
			{
				_peers[packet->senderAddress()]->deleteFromDatabase(_address);
				_peers.erase(packet->senderAddress());
			}
			catch(const std::exception& ex)
			{
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			_peersMutex.unlock();
			_centralAddress = 0;
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

void HomeMaticDevice::handleConfigParamRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		_currentList = packet->payload()->at(6);
		sendConfigParams(messageCounter, packet->senderAddress(), packet);
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

void HomeMaticDevice::handleTimeRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x02);
		payload.push_back(0x04);
		uint32_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() - 946684800;
		payload.push_back(time >> 24);
		payload.push_back((time >> 16) & 0xFF);
		payload.push_back((time >> 8) & 0xFF);
		payload.push_back(time & 0xFF);
		std::shared_ptr<BidCoSPacket> timePacket(new BidCoSPacket(messageCounter, 0x80, 0x3F, _address, packet->senderAddress(), payload));
		sendPacket(timePacket);
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

void HomeMaticDevice::handleConfigPeerAdd(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		int32_t address = (packet->payload()->at(2) << 16) + (packet->payload()->at(3) << 8) + (packet->payload()->at(4));
		if(!peerExists(address))
		{
			std::shared_ptr<Peer> peer = createPeer(address, -1, HMDeviceTypes::HMUNKNOWN, "", packet->payload()->at(5), 0);
			_peersMutex.lock();
			try
			{
				_peers[address] = peer;
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
		_justPairedToOrThroughCentral = true;
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

void HomeMaticDevice::handleConfigStart(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(_pairing)
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::PAIRINGCENTRAL, packet->senderAddress());
			std::shared_ptr<Peer> peer(new Peer(isCentral()));
			peer->address = packet->senderAddress();
			peer->deviceType = HMDeviceTypes::HMRCV50;
			peer->messageCounter = 0x00; //Unknown at this point
			queue->peer = peer;
			queue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<uint32_t, int32_t>> { std::pair<uint32_t, int32_t>(0x01, 0x05) }));
			queue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<uint32_t, int32_t>> { std::pair<uint32_t, int32_t>(0x01, 0x08), std::pair<int32_t, int32_t>(0x02, 0x02), std::pair<int32_t, int32_t>(0x03, 0x01) }));
			queue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<uint32_t, int32_t>> { std::pair<uint32_t, int32_t>(0x01, 0x06) }));
			queue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<uint32_t, int32_t>> { std::pair<uint32_t, int32_t>(0x01, 0x04) }));
		}
		_currentList = packet->payload()->at(6);
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

void HomeMaticDevice::handleConfigWriteIndex(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(packet->payload()->at(2) == 0x02 && packet->payload()->at(3) == 0x00)
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::UNPAIRING, packet->senderAddress());
			queue->peer = getPeer(packet->senderAddress());
			queue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<uint32_t, int32_t>> { std::pair<uint32_t, int32_t>(0x01, 0x08), std::pair<uint32_t, int32_t>(0x02, 0x02), std::pair<uint32_t, int32_t>(0x03, 0x00) }));
			queue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<uint32_t, int32_t>> { std::pair<uint32_t, int32_t>(0x01, 0x06) }));
		}
		uint32_t channel = packet->payload()->at(0);
		for(int i = 2; i < (signed)packet->payload()->size(); i+=2)
		{
			int32_t index = (int32_t)packet->payload()->at(i);
			if(_config.find(channel) == _config.end() || _config.at(channel).find(_currentList) == _config.at(channel).end() || _config.at(channel).at(_currentList).find(index) == _config.at(channel).at(_currentList).end()) continue;
			_config[channel][_currentList][(int32_t)packet->payload()->at(i)] = packet->payload()->at(i + 1);
			HelperFunctions::printInfo("Info: 0x" + HelperFunctions::getHexString(_address) + ": Config at index " + std::to_string(packet->payload()->at(i)) + " set to " + std::to_string(packet->payload()->at(i + 1)));
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

void HomeMaticDevice::sendPeerList(int32_t messageCounter, int32_t destinationAddress, int32_t channel)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x01); //I think it's always channel 1
		_peersMutex.lock();
		for(std::unordered_map<int32_t, std::shared_ptr<Peer>>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			if(i->second->deviceType == HMDeviceTypes::HMRCV50) continue;
			if(i->second->localChannel != channel) continue;
			payload.push_back(i->first >> 16);
			payload.push_back((i->first >> 8) & 0xFF);
			payload.push_back(i->first & 0xFF);
			payload.push_back(i->second->remoteChannel); //Channel
		}
		_peersMutex.unlock();
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(messageCounter, 0x80, 0x10, _address, destinationAddress, payload));
		sendPacket(packet);
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

void HomeMaticDevice::sendStealthyOK(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		//As there is no action in the queue when sending stealthy ok's, we need to manually keep it alive
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(destinationAddress);
		if(queue) queue->keepAlive();
		_sentPackets.keepAlive(destinationAddress);
		std::vector<uint8_t> payload;
		payload.push_back(0x00);
		std::this_thread::sleep_for(std::chrono::milliseconds(GD::settings.bidCoSResponseDelay()));
		std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(messageCounter, 0x80, 0x02, _address, destinationAddress, payload));
		GD::rfDevice->sendPacket(ok);
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

void HomeMaticDevice::sendOK(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x00);
		std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(messageCounter, 0x80, 0x02, _address, destinationAddress, payload));
		sendPacket(ok);
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

void HomeMaticDevice::sendOKWithPayload(int32_t messageCounter, int32_t destinationAddress, std::vector<uint8_t> payload, bool isWakeMeUpPacket)
{
	try
	{
		uint8_t controlByte = 0x80;
		if(isWakeMeUpPacket) controlByte &= 0x02;
		std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(messageCounter, 0x80, 0x02, _address, destinationAddress, payload));
		sendPacket(ok);
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

void HomeMaticDevice::sendNOK(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x80);
		std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(messageCounter, 0x80, 0x02, _address, destinationAddress, payload));
		sendPacket(ok);
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

void HomeMaticDevice::sendNOKTargetInvalid(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x84);
		std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(messageCounter, 0x80, 0x02, _address, destinationAddress, payload));
		sendPacket(ok);
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

void HomeMaticDevice::sendConfigParams(int32_t messageCounter, int32_t destinationAddress, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		uint32_t channel = packet->payload()->at(0);
		if(_config[channel][_currentList].size() >= 8)
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::DEFAULT, destinationAddress);
			queue->peer = getPeer(destinationAddress);

			//Add request config packet in case the request is being repeated
			queue->push(_messages->find(DIRECTIONIN, 0x01, std::vector<std::pair<uint32_t, int32_t>> { std::pair<uint32_t, int32_t>(0x01, 0x04) }));

			std::vector<uint8_t> payload;
			std::map<int32_t, int32_t>::const_iterator i = _config[channel][_currentList].begin();
			while(true)
			{
				if(i->first % 15 == 1 || i == _config[channel][_currentList].end()) //New packet
				{
					if(!payload.empty())
					{
						std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(messageCounter, 0xA0, 0x10, _address, destinationAddress, payload));
						queue->push(packet);
						messageCounter++;
						queue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
						payload.clear();
					}
					if(i == _config[channel][_currentList].end()) break;
					payload.push_back((packet->payload()->at(0) == 2) ? 3 : 2);
					payload.push_back(i->first);
				}
				payload.push_back(i->second);
				i++;
			}
			payload.push_back((packet->payload()->at(0) == 2) ? 3 : 2);
			payload.push_back(0x00);
			std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(messageCounter, 0x80, 0x10, _address, destinationAddress, payload));
			queue->push(packet);
		}
		else
		{
			std::vector<unsigned char> payload;
			payload.push_back((packet->payload()->at(0) == 2) ? 3 : 2); //No idea how to determine the destination channel
			for(std::map<int32_t, int32_t>::const_iterator i = _config[channel][_currentList].begin(); i != _config[channel][_currentList].end(); ++i)
			{
				payload.push_back(i->first);
				payload.push_back(i->second);
			}
			payload.push_back(0); //I think the packet always ends with two zero bytes
			payload.push_back(0);
			std::shared_ptr<BidCoSPacket> config(new BidCoSPacket(messageCounter, 0x80, 0x10, _address, destinationAddress, payload));
			sendPacket(config);
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
		std::shared_ptr<BidCoSPacket> pairingRequest(new BidCoSPacket(_messageCounter[0], 0x84, 0x00, _address, 0x00, payload));
		_messageCounter[0]++;
		sendPacket(pairingRequest);
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

void HomeMaticDevice::sendDirectedPairingRequest(int32_t messageCounter, int32_t controlByte, std::shared_ptr<BidCoSPacket> packet)
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
		std::shared_ptr<BidCoSPacket> pairingRequest(new BidCoSPacket(messageCounter, 0xA0, 0x00, _address, packet->senderAddress(), payload));
		sendPacket(pairingRequest);
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

void HomeMaticDevice::setLowBattery(bool lowBattery)
{
    _lowBattery = lowBattery;
}

void HomeMaticDevice::sendDutyCycleResponse(int32_t destinationAddress) {}

bool HomeMaticDevice::peerExists(int32_t address)
{
	try
	{
		_peersMutex.lock();
		if(_peers.find(address) != _peers.end())
		{
			_peersMutex.unlock();
			return true;
		}
		_peersMutex.unlock();
		return false;
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

void HomeMaticDevice::addPeer(std::shared_ptr<Peer> peer)
{
	try
	{
		_peersMutex.lock();
		if(_peers.find(peer->address) == _peers.end())
		{
			_peers[peer->address] = peer;
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

std::shared_ptr<Peer> HomeMaticDevice::getPeer(int32_t address)
{
	try
	{
		_peersMutex.lock();
		if(_peers.find(address) != _peers.end())
		{
			std::shared_ptr<Peer> peer(_peers.at(address));
			_peersMutex.unlock();
			return peer;
		}
		_peersMutex.unlock();
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
    return std::shared_ptr<Peer>();
}

std::shared_ptr<Peer> HomeMaticDevice::getPeer(std::string serialNumber)
{
	try
	{
		_peersMutex.lock();
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end())
		{
			std::shared_ptr<Peer> peer(_peersBySerial.at(serialNumber));
			_peersMutex.unlock();
			return peer;
		}
		_peersMutex.unlock();
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
    return std::shared_ptr<Peer>();
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
