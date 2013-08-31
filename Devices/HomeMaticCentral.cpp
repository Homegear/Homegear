#include "HomeMaticCentral.h"
#include "../GD.h"
#include "../HelperFunctions.h"

HomeMaticCentral::HomeMaticCentral() : HomeMaticDevice()
{
	init();
}

HomeMaticCentral::HomeMaticCentral(uint32_t deviceID, std::string serialNumber, int32_t address) : HomeMaticDevice(deviceID, serialNumber, address)
{
	init();
}

HomeMaticCentral::~HomeMaticCentral()
{
	try
	{
		dispose();
		if(_pairingModeThread.joinable())
		{
			_stopPairingModeThread = true;
			_pairingModeThread.join();
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

void HomeMaticCentral::init()
{
	try
	{
		HomeMaticDevice::init();

		_deviceType = HMDeviceTypes::HMCENTRAL;
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

void HomeMaticCentral::unserialize_0_0_6(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent)
{
	HomeMaticDevice::unserialize_0_0_6(serializedObject.substr(8, std::stoll(serializedObject.substr(0, 8), 0, 16)), dutyCycleMessageCounter, lastDutyCycleEvent);
}

void HomeMaticCentral::setUpBidCoSMessages()
{
	try
	{
		//Don't call HomeMaticDevice::setUpBidCoSMessages!
		_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x00, this, ACCESSPAIREDTOSENDER, FULLACCESS, &HomeMaticDevice::handlePairingRequest)));

		_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x02, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleAck)));

		_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x10, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleConfigParamResponse)));

		_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x3F, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &HomeMaticDevice::handleTimeRequest)));
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

void HomeMaticCentral::worker()
{
	try
	{
		std::chrono::milliseconds sleepingTime(10);
		uint32_t counter = 0;
		int32_t lastPeer;
		lastPeer = 0;
		//One loop on the Raspberry Pi takes about 30µs
		while(!_stopWorkerThread)
		{
			try
			{
				std::this_thread::sleep_for(sleepingTime);
				if(_stopWorkerThread) return;
				if(counter > 10000)
				{
					counter = 0;
					_peersMutex.lock();
					if(_peers.size() > 0)
					{
						int32_t windowTimePerPeer = GD::settings.workerThreadWindow() / _peers.size();
						if(windowTimePerPeer > 2) windowTimePerPeer -= 2;
						sleepingTime = std::chrono::milliseconds(windowTimePerPeer);
					}
					_peersMutex.unlock();
				}
				_peersMutex.lock();
				if(!_peers.empty())
				{
					if(!_peers.empty())
					{
						std::unordered_map<int32_t, std::shared_ptr<Peer>>::iterator nextPeer = _peers.find(lastPeer);
						if(nextPeer != _peers.end())
						{
							nextPeer++;
							if(nextPeer == _peers.end()) nextPeer = _peers.begin();
						}
						else nextPeer = _peers.begin();
						lastPeer = nextPeer->first;
					}
				}
				_peersMutex.unlock();
				std::shared_ptr<Peer> peer(getPeer(lastPeer));
				if(peer && !peer->deleting) peer->worker();
				counter++;
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

bool HomeMaticCentral::packetReceived(std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(_disposing) return false;
		bool handled = HomeMaticDevice::packetReceived(packet);
		std::shared_ptr<Peer> peer(getPeer(packet->senderAddress()));
		if(!peer) return false;
		std::shared_ptr<Peer> team;
		if(peer->hasTeam() && packet->senderAddress() == peer->getTeamRemoteAddress()) team = getPeer(peer->getTeamRemoteSerialNumber());
		if(handled)
		{
			//This block is not necessary for teams as teams will never have queues.
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(packet->senderAddress());
			if(queue && queue->getQueueType() != BidCoSQueueType::PEER)
			{
				peer->setLastPacketReceived();
				peer->serviceMessages->endUnreach();
				peer->setRSSI(packet->rssi());
				return true; //Packet is handled by queue. Don't check if queue is empty!
			}
		}
		if(team)
		{
			team->packetReceived(packet);
			for(std::vector<std::pair<std::string, uint32_t>>::const_iterator i = team->teamChannels.begin(); i != team->teamChannels.end(); ++i)
			{
				getPeer(i->first)->packetReceived(packet);
			}
		}
		else peer->packetReceived(packet);
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

void HomeMaticCentral::enqueuePendingQueues(int32_t deviceAddress)
{
	try
	{
		std::shared_ptr<Peer> peer = getPeer(deviceAddress);
		if(!peer) return;
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(deviceAddress);
		if(!queue) queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::DEFAULT, deviceAddress);
		if(!queue) return;
		if(!queue->peer) queue->peer = peer;
		if(queue->pendingQueuesEmpty()) queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::enqueuePackets(int32_t deviceAddress, std::shared_ptr<BidCoSQueue> packets, bool pushPendingBidCoSQueues)
{
	try
	{
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::DEFAULT, deviceAddress);
		queue->push(packets, true, true);
		if(pushPendingBidCoSQueues)
		{
			std::shared_ptr<Peer> peer(getPeer(deviceAddress));
			if(peer) queue->push(peer->pendingBidCoSQueues);
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

std::shared_ptr<Peer> HomeMaticCentral::createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet, bool save)
{
	try
	{
		std::shared_ptr<Peer> peer(new Peer(_address, true));
		peer->setAddress(address);
		peer->setFirmwareVersion(firmwareVersion);
		peer->setDeviceType(deviceType);
		peer->setSerialNumber(serialNumber);
		peer->setRemoteChannel(remoteChannel);
		peer->setMessageCounter(messageCounter);
		peer->rpcDevice = GD::rpcDevices.find(deviceType, firmwareVersion, packet);
		if(!peer->rpcDevice) peer->rpcDevice = GD::rpcDevices.find(HMDeviceType::getString(deviceType), packet);
		if(!peer->rpcDevice) return std::shared_ptr<Peer>();
		if(peer->rpcDevice->countFromSysinfoIndex > -1) peer->setCountFromSysinfo(peer->rpcDevice->getCountFromSysinfo());
		if(save) peer->save(true, true, false); //Save and create peerID
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

std::string HomeMaticCentral::handleCLICommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;
		if(_currentPeer)
		{
			if(command == "unselect")
			{
				_currentPeer.reset();
				return "Peer unselected.\n";
			}
			if(!_currentPeer) return "No peer selected.\n";
			return _currentPeer->handleCLICommand(command);
		}
		else if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "pairing on\t\tEnables pairing mode" << std::endl;
			stringStream << "pairing off\t\tDisables pairing mode" << std::endl;
			stringStream << "peers list\t\tList all peers" << std::endl;
			stringStream << "peers remove\t\tRemove a peer (without unpairing)" << std::endl;
			stringStream << "peers unpair\t\tUnpair a peer" << std::endl;
			stringStream << "peers select\t\tSelect a peer" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 10, "pairing on") == 0)
		{
			int32_t duration = 300;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command enables pairing mode." << std::endl;
						stringStream << "Usage: pairing on [DURATION]" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  DURATION:\tOptional duration in seconds to stay in pairing mode." << std::endl;
						return stringStream.str();
					}
					duration = HelperFunctions::getNumber(element, true);
					if(duration < 5 || duration > 3600) return "Invalid duration. Duration has to be greater than 5 and less than 3600.\n";
				}
				index++;
			}

			setInstallMode(true, duration, false);
			stringStream << "Pairing mode enabled." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 11, "pairing off") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command disables pairing mode." << std::endl;
						stringStream << "Usage: pairing off" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			setInstallMode(false, -1, false);
			stringStream << "Pairing mode disabled." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 12, "peers remove") == 0)
		{
			int32_t peerAddress;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					peerAddress = HelperFunctions::getNumber(element, true);
					if(peerAddress == 0 || peerAddress != (peerAddress & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command removes a peer without trying to unpair it first." << std::endl;
				stringStream << "Usage: peers remove ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe 3 byte address of the peer to remove in hexadecimal format. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			if(!peerExists(peerAddress)) stringStream << "This device is not paired to this central." << std::endl;
			else
			{
				if(_currentPeer && _currentPeer->getAddress() == peerAddress) _currentPeer.reset();
				deletePeer(peerAddress);
				stringStream << "Removed device 0x" << std::hex << peerAddress << "." << std::dec << std::endl;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 12, "peers unpair") == 0)
		{
			int32_t peerAddress;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					peerAddress = HelperFunctions::getNumber(element, true);
					if(peerAddress == 0 || peerAddress != (peerAddress & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command unpairs a peer." << std::endl;
				stringStream << "Usage: peers unpair ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe 3 byte address of the peer to unpair in hexadecimal format. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			if(!peerExists(peerAddress)) stringStream << "This device is not paired to this central." << std::endl;
			else
			{
				if(_currentPeer && _currentPeer->getAddress() == peerAddress) _currentPeer.reset();
				stringStream << "Unpairing device 0x" << std::hex << peerAddress << std::dec << std::endl;
				unpair(peerAddress, true);
			}
			return stringStream.str();
		}
		else if(command == "enable aes")
		{
			std::string input;
			stringStream << "Please enter the devices address: ";
			int32_t address = getHexInput();
			if(!peerExists(address)) stringStream << "This device is not paired to this central." << std::endl;
			else
			{
				stringStream << "Enabling AES for device device 0x" << std::hex << address << std::dec << std::endl;
				sendEnableAES(address, 1);
			}
			return stringStream.str();
		}
		else if(command == "peers list")
		{
			try
			{
				if(_peers.empty())
				{
					stringStream << "No peers are paired to this central." << std::endl;
					return stringStream.str();
				}
				_peersMutex.lock();
				for(std::unordered_map<int32_t, std::shared_ptr<Peer>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
				{
					stringStream << "Address: 0x" << std::hex << i->second->getAddress() << "\tSerial number: " << i->second->getSerialNumber() << "\tDevice type: " << (uint32_t)i->second->getDeviceType() << std::endl << std::dec;
				}
				_peersMutex.unlock();
				return stringStream.str();
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
		else if(command.compare(0, 12, "peers select") == 0)
		{
			int32_t address;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					address = HelperFunctions::getNumber(element, true);
					if(address == 0 || address != (address & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command selects a peer." << std::endl;
				stringStream << "Usage: peers select ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe 3 byte address of the peer to select in hexadecimal format. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			_currentPeer = getPeer(address);
			if(!_currentPeer) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				stringStream << "Peer with address 0x" << std::hex << address << " and device type 0x" << (int32_t)_currentPeer->getDeviceType() << " selected." << std::dec << std::endl;
				stringStream << "For information about the peer's commands type: \"help\"" << std::endl;
			}
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

int32_t HomeMaticCentral::getUniqueAddress(int32_t seed)
{
	try
	{
		uint32_t i = 0;
		while((_peers.find(seed) != _peers.end() || GD::devices.get(seed)) && i++ < 200000)
		{
			seed += 9345;
			if(seed > 16777215) seed -= 16777216;
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
	return seed;
}

void HomeMaticCentral::addPeersToVirtualDevices()
{
	try
	{
		_peersMutex.lock();
		for(std::unordered_map<std::string, std::shared_ptr<Peer>>::iterator i = _peersBySerial.begin(); i != _peersBySerial.end(); ++i)
		{
			std::shared_ptr<HomeMaticDevice> device = i->second->getHiddenPeerDevice();
			if(device) device->addPeer(i->second);
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

std::string HomeMaticCentral::getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber)
{
	try
	{
		if(seedPrefix.size() > 3) throw Exception("seedPrefix is too long.");
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
			if(seedNumber > 9999999) seedNumber -= 10000000;
			std::ostringstream stringstream;
			stringstream << seedPrefix << std::setw(numberSize) << std::setfill('0') << std::dec << seedNumber;
			temp2 = stringstream.str();
		}
		return temp2;
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

void HomeMaticCentral::addHomegearFeaturesHMCCVD(std::shared_ptr<Peer> peer, int32_t channel, bool pushPendingBidCoSQueues)
{
	try
	{
		std::shared_ptr<HomeMaticDevice> tc(peer->getHiddenPeerDevice());
		if(!tc)
		{
			int32_t hmcctcAddress = getUniqueAddress((0x39 << 16) + (peer->getAddress() & 0xFF00) + (peer->getAddress() & 0xFF));
			if(peer->hasPeers(1) && !peer->getPeer(1, hmcctcAddress)) return; //Already linked to a HM-CC-TC
			std::string temp = peer->getSerialNumber().substr(3);
			std::string serialNumber = getUniqueSerialNumber("VCD", HelperFunctions::getNumber(temp));
			GD::devices.add(new HM_CC_TC(0, serialNumber, hmcctcAddress));
			tc = GD::devices.get(hmcctcAddress);
			tc->addPeer(peer);
		}
		std::shared_ptr<BasicPeer> hmcctc(new BasicPeer());
		hmcctc->address = tc->getAddress();
		hmcctc->serialNumber = tc->getSerialNumber();
		hmcctc->channel = 1;
		hmcctc->hidden = true;
		peer->addPeer(1, hmcctc);

		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		std::vector<uint8_t> payload;
		//CONFIG_ADD_PEER
		payload.push_back(0x01);
		payload.push_back(0x01);
		payload.push_back(tc->getAddress() >> 16);
		payload.push_back((tc->getAddress() >> 8) & 0xFF);
		payload.push_back(tc->getAddress() & 0xFF);
		payload.push_back(0x02);
		payload.push_back(0);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, peer->getAddress(), payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		peer->serviceMessages->setConfigPending(true);
		peer->pendingBidCoSQueues->push(pendingQueue);
		if(pushPendingBidCoSQueues)
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, peer->getAddress());
			queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::addHomegearFeaturesRemote(std::shared_ptr<Peer> peer, int32_t channel, bool pushPendingBidCoSQueues)
{
	try
	{
		if(!peer) return;
		std::shared_ptr<HomeMaticDevice> sw(peer->getHiddenPeerDevice());
		std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>> channels;
		if(!sw)
		{
			int32_t actorAddress = getUniqueAddress((0x04 << 16) + (peer->getAddress() & 0xFF00) + (peer->getAddress() & 0xFF));
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
			{
				if(!peer->hasPeers(i->first) || peer->getPeer(i->first, actorAddress))
				{
					channels[i->first] = i->second;
				}
			}
			if(channels.empty()) return; //All channels are already paired to actors
			std::string temp = peer->getSerialNumber().substr(3);
			std::string serialNumber = getUniqueSerialNumber("VSW", HelperFunctions::getNumber(temp));
			GD::devices.add(new HM_LC_SWX_FM(0, serialNumber, actorAddress));
			sw = GD::devices.get(actorAddress);
			uint32_t channelCount = peer->rpcDevice->channels.size();
			sw->setChannelCount(channelCount);
			sw->addPeer(peer);
		}

		std::shared_ptr<BasicPeer> switchPeer;

		if(channel > -1)
		{
			switchPeer.reset(new BasicPeer());
			switchPeer->address = sw->getAddress();
			switchPeer->serialNumber = sw->getSerialNumber();
			switchPeer->channel = channel;
			switchPeer->hidden = true;
			peer->addPeer(channel, switchPeer);
		}
		else
		{
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
			{
				if(i->second->type != "KEY" && i->second->type != "MOTION_DETECTOR") continue;
				switchPeer.reset(new BasicPeer());
				switchPeer->address = sw->getAddress();
				switchPeer->serialNumber = sw->getSerialNumber();
				switchPeer->channel = i->first;
				switchPeer->hidden = true;
				peer->addPeer(i->first, switchPeer);
			}
		}

		std::shared_ptr<RPC::RPCVariable> paramset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		if(peer->getDeviceType() == HMDeviceTypes::HMRC19 || peer->getDeviceType() == HMDeviceTypes::HMRC19B || peer->getDeviceType() == HMDeviceTypes::HMRC19SW)
		{
			paramset->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("LCD_SYMBOL", 2)));
			paramset->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("LCD_LEVEL_INTERP", 1)));
		}

		std::vector<uint8_t> payload;
		if(channel > -1)
		{
			std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::CONFIG));
			pendingQueue->noSending = true;

			payload.clear();
			payload.push_back(channel);
			payload.push_back(0x01);
			payload.push_back(sw->getAddress() >> 16);
			payload.push_back((sw->getAddress() >> 8) & 0xFF);
			payload.push_back(sw->getAddress() & 0xFF);
			payload.push_back(channel);
			payload.push_back(0);
			std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, peer->getAddress(), payload));
			pendingQueue->push(configPacket);
			pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			_messageCounter[0]++;
			peer->serviceMessages->setConfigPending(true);
			peer->pendingBidCoSQueues->push(pendingQueue);

			//putParamset pushes the packets on pendingQueues, but does not send immediately
			if(!paramset->structValue->empty()) peer->putParamset(channel, RPC::ParameterSet::Type::Enum::link, sw->getSerialNumber(), channel, paramset, true, true);
		}
		else
		{
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
			{
				if(i->second->type != "KEY" && i->second->type != "MOTION_DETECTOR") continue;
				std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::CONFIG));
				pendingQueue->noSending = true;

				payload.clear();
				payload.push_back(i->first);
				payload.push_back(0x01);
				payload.push_back(sw->getAddress() >> 16);
				payload.push_back((sw->getAddress() >> 8) & 0xFF);
				payload.push_back(sw->getAddress() & 0xFF);
				payload.push_back(i->first);
				payload.push_back(0);
				std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, peer->getAddress(), payload));
				pendingQueue->push(configPacket);
				pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				_messageCounter[0]++;
				peer->serviceMessages->setConfigPending(true);
				peer->pendingBidCoSQueues->push(pendingQueue);

				//putParamset pushes the packets on pendingQueues, but does not send immediately
				if(!paramset->structValue->empty()) peer->putParamset(i->first, RPC::ParameterSet::Type::Enum::link, sw->getSerialNumber(), i->first, paramset, true, true);
			}
		}

		if(pushPendingBidCoSQueues)
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, peer->getAddress());
			queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::addHomegearFeaturesMotionDetector(std::shared_ptr<Peer> peer, int32_t channel, bool pushPendingBidCoSQueues)
{
	try
	{
		addHomegearFeaturesRemote(peer, channel, pushPendingBidCoSQueues);
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

void HomeMaticCentral::addHomegearFeatures(std::shared_ptr<Peer> peer, int32_t channel, bool pushPendingBidCoSQueues)
{
	try
	{
		HelperFunctions::printDebug("Debug: Adding homegear features. Device type: 0x" + HelperFunctions::getHexString((int32_t)peer->getDeviceType()));
		if(peer->getDeviceType() == HMDeviceTypes::HMCCVD) addHomegearFeaturesHMCCVD(peer, channel, pushPendingBidCoSQueues);
		else if(peer->getDeviceType() == HMDeviceTypes::HMPB4DISWM ||
				peer->getDeviceType() == HMDeviceTypes::HMRC4 ||
				peer->getDeviceType() == HMDeviceTypes::HMRC4B ||
				peer->getDeviceType() == HMDeviceTypes::HMRCP1 ||
				peer->getDeviceType() == HMDeviceTypes::HMRCSEC3 ||
				peer->getDeviceType() == HMDeviceTypes::HMRCSEC3B ||
				peer->getDeviceType() == HMDeviceTypes::HMRCKEY3 ||
				peer->getDeviceType() == HMDeviceTypes::HMRCKEY3B ||
				peer->getDeviceType() == HMDeviceTypes::HMPB4WM ||
				peer->getDeviceType() == HMDeviceTypes::HMPB2WM ||
				peer->getDeviceType() == HMDeviceTypes::HMRC12 ||
				peer->getDeviceType() == HMDeviceTypes::HMRC12B ||
				peer->getDeviceType() == HMDeviceTypes::HMRC12SW ||
				peer->getDeviceType() == HMDeviceTypes::HMRC19 ||
				peer->getDeviceType() == HMDeviceTypes::HMRC19B ||
				peer->getDeviceType() == HMDeviceTypes::HMRC19SW ||
				peer->getDeviceType() == HMDeviceTypes::RCH ||
				peer->getDeviceType() == HMDeviceTypes::ATENT ||
				peer->getDeviceType() == HMDeviceTypes::ZELSTGRMHS4) addHomegearFeaturesRemote(peer, channel, pushPendingBidCoSQueues);
		else if(peer->getDeviceType() == HMDeviceTypes::HMSECMDIR ||
				peer->getDeviceType() == HMDeviceTypes::HMSECMDIRSCHUECO ||
				peer->getDeviceType() == HMDeviceTypes::HMSENMDIRSM ||
				peer->getDeviceType() == HMDeviceTypes::HMSENMDIRO) addHomegearFeaturesMotionDetector(peer, channel, pushPendingBidCoSQueues);
		else HelperFunctions::printDebug("Debug: No homegear features to add.");
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

void HomeMaticCentral::deletePeer(int32_t address)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(address));
		if(!peer) return;
		peer->deleting = true;
		std::shared_ptr<RPC::RPCVariable> deviceAddresses(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		deviceAddresses->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(peer->getSerialNumber())));
		for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
		{
			deviceAddresses->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(peer->getSerialNumber() + ":" + std::to_string(i->first))));
		}
		GD::rpcClient.broadcastDeleteDevices(deviceAddresses);
		deleteMetadata(peer->getSerialNumber());
		if(peer->rpcDevice)
		{
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
			{
				deleteMetadata(peer->getSerialNumber() + ':' + std::to_string(i->first));
			}
		}
		_peersMutex.lock();
		if(_peersBySerial.find(peer->getSerialNumber()) != _peersBySerial.end()) _peersBySerial.erase(peer->getSerialNumber());
		_peersMutex.unlock();
		removePeerFromTeam(peer);
		peer->deleteFromDatabase();
		peer->deletePairedVirtualDevices();
		_peersMutex.lock();
		_peers.erase(address);
		_peersMutex.unlock();
		HelperFunctions::printMessage("Removed device 0x" + HelperFunctions::getHexString(address));
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

void HomeMaticCentral::reset(int32_t address, bool defer)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(address));
		if(!peer) return;
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::UNPAIRING, address);
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::UNPAIRING));
		pendingQueue->noSending = true;

		uint8_t configByte = 0xA0;
		if(peer->rpcDevice->rxModes & RPC::Device::RXModes::burst) configByte |= 0x10;

		std::vector<uint8_t> payload;

		//CONFIG_START
		payload.push_back(0x04);
		payload.push_back(0x00);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], configByte, 0x11, _address, address, payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		if(defer)
		{
			peer->serviceMessages->setConfigPending(true);
			while(!peer->pendingBidCoSQueues->empty()) peer->pendingBidCoSQueues->pop();
			peer->pendingBidCoSQueues->push(pendingQueue);
			queue->push(peer->pendingBidCoSQueues);
		}
		else queue->push(pendingQueue, true, true);
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

void HomeMaticCentral::unpair(int32_t address, bool defer)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(address));
		if(!peer) return;
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::UNPAIRING, address);
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::UNPAIRING));
		pendingQueue->noSending = true;

		uint8_t configByte = 0xA0;
		if(peer->rpcDevice->rxModes & RPC::Device::RXModes::burst) configByte |= 0x10;
		std::vector<uint8_t> payload;

		//CONFIG_START
		payload.push_back(0);
		payload.push_back(0x05);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], configByte, 0x01, _address, address, payload));
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
			peer->serviceMessages->setConfigPending(true);
			while(!peer->pendingBidCoSQueues->empty()) peer->pendingBidCoSQueues->pop();
			peer->pendingBidCoSQueues->push(pendingQueue);
			queue->push(peer->pendingBidCoSQueues);
		}
		else queue->push(pendingQueue, true, true);
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

void HomeMaticCentral::sendEnableAES(int32_t address, int32_t channel)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(address));
		if(!peer) return;

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

void HomeMaticCentral::handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(packet->destinationAddress() != 0 && packet->destinationAddress() != _address) return;
		if(packet->payload()->size() < 17)
		{
			HelperFunctions::printError("Error: Pairing packet is too small (payload size has to be at least 17).");
			return;
		}

		std::string serialNumber;
		for(uint32_t i = 3; i < 13; i++)
			serialNumber.push_back((char)packet->payload()->at(i));
		HMDeviceTypes deviceType = (HMDeviceTypes)((packet->payload()->at(1) << 8) + packet->payload()->at(2));

		std::shared_ptr<Peer> peer(getPeer(packet->senderAddress()));
		if(peer && (peer->getSerialNumber() != serialNumber || peer->getDeviceType() != deviceType))
		{
			HelperFunctions::printError("Error: Pairing packet rejected, because a peer with the same address but different serial number or device type is already paired to this central.");
			return;
		}

		std::vector<uint8_t> payload;

		std::shared_ptr<BidCoSQueue> queue;
		if(_pairing)
		{
			queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::PAIRING, packet->senderAddress());

			if(!peer)
			{
				//Do not save here
				queue->peer = createPeer(packet->senderAddress(), packet->payload()->at(0), deviceType, serialNumber, 0, 0, packet, false);
				if(!queue->peer)
				{
					HelperFunctions::printWarning("Warning: Device type not supported. Sender address 0x" + HelperFunctions::getHexString(packet->senderAddress()) + ".");
					return;
				}
				queue->peer->initializeCentralConfig();
				peer = queue->peer;
			}

			if(!peer->rpcDevice)
			{
				HelperFunctions::printWarning("Warning: Device type not supported. Sender address 0x" + HelperFunctions::getHexString(packet->senderAddress()) + ".");
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
			if(internalKeysVisible)
			{
				std::vector<uint8_t> data;
				data.push_back(1);
				internalKeysVisible->adjustBitPosition(data);
				payload.push_back(data.at(0) | 0x01);
			}
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

			//Don't check for rxModes here! All rxModes are allowed.
			if(!peerExists(packet->senderAddress())) //Only request config when peer is not already paired to central
			{
				for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
				{
					std::shared_ptr<BidCoSQueue> pendingQueue;
					int32_t channel = i->first;
					//Walk through all lists to request master config if necessary
					if(peer->rpcDevice->channels.at(channel)->parameterSets.find(RPC::ParameterSet::Type::Enum::master) != peer->rpcDevice->channels.at(channel)->parameterSets.end())
					{
						std::shared_ptr<RPC::ParameterSet> masterSet = peer->rpcDevice->channels.at(channel)->parameterSets[RPC::ParameterSet::Type::Enum::master];
						for(std::map<uint32_t, uint32_t>::iterator k = masterSet->lists.begin(); k != masterSet->lists.end(); ++k)
						{
							pendingQueue.reset(new BidCoSQueue(BidCoSQueueType::CONFIG));
							pendingQueue->noSending = true;
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
							peer->serviceMessages->setConfigPending(true);
							peer->pendingBidCoSQueues->push(pendingQueue);
						}
					}

					if(peer->rpcDevice->channels[channel]->linkRoles && (!peer->rpcDevice->channels[channel]->linkRoles->sourceNames.empty() || !peer->rpcDevice->channels[channel]->linkRoles->targetNames.empty()))
					{
						pendingQueue.reset(new BidCoSQueue(BidCoSQueueType::CONFIG));
						pendingQueue->noSending = true;
						payload.push_back(channel);
						payload.push_back(0x03);
						configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
						pendingQueue->push(configPacket);
						pendingQueue->push(_messages->find(DIRECTIONIN, 0x10, std::vector<std::pair<uint32_t, int32_t>>()));
						payload.clear();
						_messageCounter[0]++;
						peer->serviceMessages->setConfigPending(true);
						peer->pendingBidCoSQueues->push(pendingQueue);
					}
				}
			}
		}
		//not in pairing mode
		else queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::DEFAULT, packet->senderAddress());

		if(!peer)
		{
			HelperFunctions::printError("Error handling pairing packet: Peer is nullptr. This shouldn't have happened. Something went very wrong.");
			return;
		}

		if(peer->pendingBidCoSQueues && !peer->pendingBidCoSQueues->empty()) HelperFunctions::printInfo("Info: Pushing pending queues.");
		queue->push(peer->pendingBidCoSQueues); //This pushes the just generated queue and the already existent pending queue onto the queue
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

void HomeMaticCentral::sendRequestConfig(int32_t address, uint8_t localChannel, uint8_t list, int32_t remoteAddress, uint8_t remoteChannel)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(address));
		if(!peer) return;
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

		peer->serviceMessages->setConfigPending(true);
		peer->pendingBidCoSQueues->push(pendingQueue);
		if(!oldQueue) queue->push(peer->pendingBidCoSQueues);
		else if(queue->pendingQueuesEmpty()) queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::handleConfigParamResponse(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(!queue) return;
		std::shared_ptr<Peer> peer(getPeer(packet->senderAddress()));
		if(!peer) return;
		std::shared_ptr<BidCoSPacket> sentPacket(_sentPackets.get(packet->senderAddress()));
		bool continuousData = false;
		bool multiPacket = false;
		bool multiPacketEnd = false;
		//Peer request
		if(sentPacket && sentPacket->payload()->size() >= 2 && sentPacket->payload()->at(1) == 0x03)
		{
			int32_t localChannel = sentPacket->payload()->at(0);
			std::shared_ptr<RPC::DeviceChannel> rpcChannel = peer->rpcDevice->channels[localChannel];
			bool peerFound = false;
			if(packet->payload()->size() >= 5)
			{
				for(uint32_t i = 1; i < packet->payload()->size() - 1; i += 4)
				{
					int32_t peerAddress = (packet->payload()->at(i) << 16) + (packet->payload()->at(i + 1) << 8) + packet->payload()->at(i + 2);
					if(peerAddress != 0)
					{
						peerFound = true;
						int32_t remoteChannel = packet->payload()->at(i + 3);
						if(rpcChannel->hasTeam) //Peer is team
						{
							//Don't add team if address is peer's, because then the team already exists
							if(peerAddress != peer->getAddress())
							{
								HelperFunctions::printInfo("Info: Adding peer 0x" + HelperFunctions::getHexString(peer->getAddress()) + " to team with address 0x" + HelperFunctions::getHexString(peerAddress) + ".");
								addPeerToTeam(peer, localChannel, peerAddress, remoteChannel);
							}
						}
						else //Normal peer
						{
							std::shared_ptr<BasicPeer> newPeer(new BasicPeer());
							newPeer->address = peerAddress;
							newPeer->channel = remoteChannel;
							peer->addPeer(localChannel, newPeer);
							if(peer->rpcDevice->channels.find(localChannel) == peer->rpcDevice->channels.end()) continue;

							if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::link) == rpcChannel->parameterSets.end()) continue;
							std::shared_ptr<RPC::ParameterSet> parameterSet = rpcChannel->parameterSets[RPC::ParameterSet::Type::Enum::link];
							if(parameterSet->parameters.empty()) continue;
							for(std::map<uint32_t, uint32_t>::iterator k = parameterSet->lists.begin(); k != parameterSet->lists.end(); ++k)
							{
								sendRequestConfig(peer->getAddress(), localChannel, k->first, newPeer->address, newPeer->channel);
							}
						}
					}
				}
			}
			if(rpcChannel->hasTeam && !peerFound)
			{
				//Peer has no team yet so set it needs to be defined
				setTeam(peer->getSerialNumber(), localChannel, "", 0, true, false);
			}
			if(packet->payload()->size() >= 2 && packet->payload()->at(0) == 0x03 && packet->payload()->at(1) == 0x00)
			{
				//multiPacketEnd was received unexpectedly. Because of the delay it was received after the peer request packet was sent.
				//As the peer request is popped already, queue it again.
				//Example from HM-LC-Sw1PBU-FM:
				//1375093199833 Sending: 1072A001FD00011BD5D100040000000000
				//1375093199988 Received: 1472A0101BD5D1FD00010202810AFD0B000C0115FF <= not a multi packet response
				//1375093200079 Sending: 0A728002FD00011BD5D100 <= After the ok we are not expecting another packet
				//1375093200171 Sending: 0B73A001FD00011BD5D10103 <= So we are sending the peer request
				//1375093200227 Received: 0C73A0101BD5D1FD0001030000 <= And a little later receive multiPacketEnd unexpectedly
				std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(packet->senderAddress());
				if(queue)
				{
					queue->pushFront(sentPacket);
					return;
				}
			}
		}
		else if(sentPacket && sentPacket->payload()->size() >= 7 && sentPacket->payload()->at(1) == 0x04) //Config request
		{
			int32_t channel = sentPacket->payload()->at(0);
			int32_t list = sentPacket->payload()->at(6);
			int32_t remoteAddress = (sentPacket->payload()->at(2) << 16) + (sentPacket->payload()->at(3) << 8) + sentPacket->payload()->at(4);
			int32_t remoteChannel = sentPacket->payload()->at(5);
			RPC::ParameterSet::Type::Enum type = (remoteAddress != 0) ? RPC::ParameterSet::Type::link : RPC::ParameterSet::Type::master;
			std::shared_ptr<RPC::RPCVariable> parametersToEnforce;
			if(!peer->getPairingComplete()) parametersToEnforce.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
			if((packet->controlByte() & 0x20) && (packet->payload()->at(0) == 3)) continuousData = true;
			if(!continuousData && (packet->payload()->at(packet->payload()->size() - 2) != 0 || packet->payload()->at(packet->payload()->size() - 1) != 0)) multiPacket = true;
			//Some devices have a payload size of 3
			if(continuousData && packet->payload()->size() == 3 && packet->payload()->at(1) == 0 && packet->payload()->at(2) == 0) multiPacketEnd = true;
			//And some a payload size of 2
			if(continuousData && packet->payload()->size() == 2 && packet->payload()->at(1) == 0) multiPacketEnd = true;
			if(continuousData && !multiPacketEnd)
			{
				int32_t startIndex = packet->payload()->at(1);
				int32_t endIndex = startIndex + packet->payload()->size() - 3;
				if(peer->rpcDevice->channels[channel]->parameterSets.find(type) == peer->rpcDevice->channels[channel]->parameterSets.end() || !peer->rpcDevice->channels[channel]->parameterSets[type])
				{
					HelperFunctions::printError("Error: Received config for non existant parameter set.");
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
								//This is not working if one parameter is split over two or more packets.
								peer->configCentral[channel][(*i)->id].data = packet->getPosition(position, (*i)->physicalParameter->size, (*i)->physicalParameter->mask);
								if(!peer->getPairingComplete() && (*i)->logicalParameter->enforce)
								{
									parametersToEnforce->structValue->push_back((*i)->logicalParameter->getEnforceValue());
									parametersToEnforce->structValue->back()->name = (*i)->id;
								}
								if(GD::debugLevel >= 5) HelperFunctions::printDebug("Debug: Parameter " + (*i)->id + " of device 0x" + HelperFunctions::getHexString(peer->getAddress()) + " at index " + std::to_string((*i)->physicalParameter->index) + " and packet index " + std::to_string(position) + " with size " + std::to_string((*i)->physicalParameter->size) + " was set to 0x" + HelperFunctions::getHexString(peer->configCentral[channel][(*i)->id].data) + ".");
							}
							else if(peer->getPeer(channel, remoteAddress, remoteChannel))
							{
								peer->linksCentral[channel][remoteAddress][remoteChannel][(*i)->id].data = packet->getPosition(position, (*i)->physicalParameter->size, (*i)->physicalParameter->mask);
								if(GD::debugLevel >= 5) HelperFunctions::printDebug("Debug: Parameter " + (*i)->id + " of device 0x" + HelperFunctions::getHexString(peer->getAddress()) + " at index " + std::to_string((*i)->physicalParameter->index) + " and packet index " + std::to_string(position) + " with size " + std::to_string((*i)->physicalParameter->size) + " was set to 0x" + HelperFunctions::getHexString(peer->linksCentral[channel][remoteAddress][remoteChannel][(*i)->id].data) + ".");
							}
						}
						else HelperFunctions::printError("Error: Device tried to set parameter without id. Device: " + HelperFunctions::getHexString(peer->getAddress()) + " Serial number: " + peer->getSerialNumber() + " Channel: " + std::to_string(channel) + " List: " + std::to_string((*i)->physicalParameter->list) + " Parameter index: " + std::to_string((*i)->index));
					}
				}
			}
			else if(!multiPacketEnd)
			{
				if(peer->rpcDevice->channels[channel]->parameterSets.find(type) == peer->rpcDevice->channels[channel]->parameterSets.end() || !peer->rpcDevice->channels[channel]->parameterSets[type])
				{
					HelperFunctions::printError("Error: Received config for non existant parameter set.");
				}
				else
				{
					int32_t length = multiPacket ? packet->payload()->size() : packet->payload()->size() - 2;
					for(uint32_t i = 1; i < length; i += 2)
					{
						int32_t index = packet->payload()->at(i);
						std::vector<std::shared_ptr<RPC::Parameter>> packetParameters = peer->rpcDevice->channels[channel]->parameterSets[type]->getIndices(index, index, list);
						for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = packetParameters.begin(); j != packetParameters.end(); ++j)
						{
							if(!(*j)->id.empty())
							{
								double position = std::fmod((*j)->physicalParameter->index, 1) + 9 + i + 1;
								double size = (*j)->physicalParameter->size;
								if(size > 1.0) size = 1.0; //Reading more than one byte doesn't make any sense
								uint8_t data = packet->getPosition(position, size, (*j)->physicalParameter->mask).at(0);
								RPCConfigurationParameter* configParam = nullptr;
								if(type == RPC::ParameterSet::Type::master)
								{
									configParam = &peer->configCentral[channel][(*j)->id];
								}
								else if(peer->getPeer(channel, remoteAddress, remoteChannel))
								{
									configParam = &peer->linksCentral[channel][remoteAddress][remoteChannel][(*j)->id];
								}
								if(configParam)
								{
									while(index - (*j)->physicalParameter->startIndex >= configParam->data.size())
									{
										configParam->data.push_back(0);
									}
									configParam->data.at(index - (*j)->physicalParameter->startIndex) = data;
									if(!peer->getPairingComplete() && (*j)->logicalParameter->enforce)
									{
										parametersToEnforce->structValue->push_back((*j)->logicalParameter->getEnforceValue());
										parametersToEnforce->structValue->back()->name = (*j)->id;
									}
									if(GD::debugLevel >= 5) HelperFunctions::printDebug("Debug: Parameter " + (*j)->id + " of device 0x" + HelperFunctions::getHexString(peer->getAddress()) + " at index " + std::to_string((*j)->physicalParameter->index) + " and packet index " + std::to_string(position) + " was set to 0x" + HelperFunctions::getHexString(data) + ".");
								}
							}
							else HelperFunctions::printError("Error: Device tried to set parameter without id. Device: " + HelperFunctions::getHexString(peer->getAddress()) + " Serial number: " + peer->getSerialNumber() + " Channel: " + std::to_string(channel) + " List: " + std::to_string((*j)->physicalParameter->list) + " Parameter index: " + std::to_string((*j)->index));
						}
					}
				}
			}
			if(!peer->getPairingComplete() && !parametersToEnforce->structValue->empty()) peer->putParamset(channel, type, "", -1, parametersToEnforce, true, true);
		}
		if((continuousData || multiPacket) && !multiPacketEnd && (packet->controlByte() & 0x20)) //Multiple config response packets
		{
			//Sending stealthy does not set sentPacket
			//The packet is queued as it triggers the next response
			//If no response is coming, the packet needs to be resent
			std::vector<uint8_t> payload;
			payload.push_back(0x00);
			std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(packet->messageCounter(), 0x80, 0x02, _address, packet->senderAddress(), payload));
			queue->pushFront(ok, true, false, true);
			//No popping from queue to stay at the same position until all response packets are received!!!
			//The popping is done with the last packet, which has no BIDI control bit set. See https://sathya.de/HMCWiki/index.php/Examples:Message_Counter
		}
		else
		{
			//Sometimes the peer requires an ok after the end packet
			if((multiPacketEnd || !multiPacket) && (packet->controlByte() & 0x20))
			{
				//Send ok once. Do not queue it!!!
				sendOK(packet->messageCounter(), packet->senderAddress());
			}
			queue->pop(); //Messages are not popped by default.
		}
		if(queue->isEmpty())
		{
			if(!peer->getPairingComplete())
			{
				addHomegearFeatures(peer, -1, true);
				peer->setPairingComplete(true);
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

void HomeMaticCentral::resetTeam(std::shared_ptr<Peer> peer, uint32_t channel)
{
	try
	{
		removePeerFromTeam(peer);

		std::shared_ptr<Peer> team;
		std::string serialNumber('*' + peer->getSerialNumber());
		_peersMutex.lock();
		if(_peersBySerial.find(serialNumber) == _peersBySerial.end())
		{
			_peersMutex.unlock();
			team = createTeam(peer->getAddress(), peer->getDeviceType(), serialNumber);
			team->rpcDevice = peer->rpcDevice->team;
			team->initializeCentralConfig();
			_peersMutex.lock();
			_peersBySerial[team->getSerialNumber()] = team;
		}
		else team = _peersBySerial['*' + peer->getSerialNumber()];
		_peersMutex.unlock();
		peer->setTeamRemoteAddress(team->getAddress());
		peer->setTeamRemoteSerialNumber(team->getSerialNumber());
		peer->setTeamChannel(channel);
		for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator j = team->rpcDevice->channels.begin(); j != team->rpcDevice->channels.end(); ++j)
		{
			if(j->second->teamTag == peer->rpcDevice->channels.at(channel)->teamTag)
			{
				peer->setTeamRemoteChannel(j->first);
				break;
			}
		}
		team->teamChannels.push_back(std::pair<std::string, uint32_t>(peer->getSerialNumber(), channel));
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

void HomeMaticCentral::addPeerToTeam(std::shared_ptr<Peer> peer, int32_t channel, int32_t teamAddress, uint32_t teamChannel)
{
	try
	{
		std::shared_ptr<Peer> teamPeer(getPeer(teamAddress));
		if(teamPeer)
		{
			//Team should be known
			addPeerToTeam(peer, channel, teamChannel, '*' + teamPeer->getSerialNumber());
		}
		else
		{
			removePeerFromTeam(peer);

			peer->setTeamRemoteAddress(teamAddress);
			peer->setTeamChannel(channel);
			peer->setTeamRemoteChannel(teamChannel);
			peer->setTeamRemoteSerialNumber("");
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

void HomeMaticCentral::addPeerToTeam(std::shared_ptr<Peer> peer, int32_t channel, uint32_t teamChannel, std::string teamSerialNumber)
{
	try
	{
		_peersMutex.lock();
		if(_peersBySerial.find(teamSerialNumber) == _peersBySerial.end() || !_peersBySerial.at(teamSerialNumber))
		{
			_peersMutex.unlock();
			return;
		}
		std::shared_ptr<Peer> team = _peersBySerial[teamSerialNumber];
		_peersMutex.unlock();
		if(team->rpcDevice->channels.find(teamChannel) == team->rpcDevice->channels.end()) return;
		if(team->rpcDevice->channels[teamChannel]->teamTag != peer->rpcDevice->channels[channel]->teamTag) return;

		removePeerFromTeam(peer);

		peer->setTeamRemoteAddress(team->getAddress());
		peer->setTeamRemoteSerialNumber(teamSerialNumber);
		peer->setTeamChannel(channel);
		peer->setTeamRemoteChannel(teamChannel);
		team->teamChannels.push_back(std::pair<std::string, uint32_t>(peer->getSerialNumber(), channel));
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

void HomeMaticCentral::removePeerFromTeam(std::shared_ptr<Peer> peer)
{
	try
	{
		_peersMutex.lock();
		if(peer->getTeamRemoteSerialNumber().empty() || _peersBySerial.find(peer->getTeamRemoteSerialNumber()) == _peersBySerial.end())
		{
			_peersMutex.unlock();
			return;
		}
		std::shared_ptr<Peer> oldTeam = _peersBySerial[peer->getTeamRemoteSerialNumber()];
		_peersMutex.unlock();
		//Remove peer from old team
		for(std::vector<std::pair<std::string, uint32_t>>::iterator i = oldTeam->teamChannels.begin(); i != oldTeam->teamChannels.end(); ++i)
		{
			if(i->first == peer->getSerialNumber() && (signed)i->second == peer->getTeamChannel())
			{
				oldTeam->teamChannels.erase(i);
				break;
			}
		}
		//Delete team if there are no peers anymore
		if(oldTeam->teamChannels.empty())
		{
			_peersMutex.lock();
			_peersBySerial.erase(oldTeam->getSerialNumber());
			_peersMutex.unlock();
		}
		peer->setTeamRemoteSerialNumber("");
		peer->setTeamRemoteAddress(0);
		peer->setTeamRemoteChannel(0);
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

void HomeMaticCentral::handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(!queue) return;
		std::shared_ptr<BidCoSPacket> sentPacket(_sentPackets.get(packet->senderAddress()));
		if(packet->payload()->at(0) == 0x80 || packet->payload()->at(0) == 0x84)
		{
			if(GD::debugLevel >= 2)
			{
				if(sentPacket) HelperFunctions::printError("Error: NACK received from 0x" + HelperFunctions::getHexString(packet->senderAddress()) + " in response to " + sentPacket->hexString() + ".");
				else HelperFunctions::printError("Error: NACK received from 0x" + HelperFunctions::getHexString(packet->senderAddress()));
				if(queue->getQueueType() == BidCoSQueueType::PAIRING) HelperFunctions::printError("Try resetting the device to factory defaults before pairing it to this central.");
			}
			if(queue->getQueueType() == BidCoSQueueType::PAIRING) queue->clear(); //Abort
			else queue->pop(); //Otherwise the queue might persist forever. NACKS shouldn't be received when not pairing
			return;
		}
		if(queue->getQueueType() == BidCoSQueueType::PAIRING)
		{
			if(sentPacket && sentPacket->messageType() == 0x01 && sentPacket->payload()->at(0) == 0x00 && sentPacket->payload()->at(1) == 0x06)
			{
				if(!peerExists(packet->senderAddress()))
				{
					if(!queue->peer) return;
					try
					{
						_peersMutex.lock();
						_peers[queue->peer->getAddress()] = queue->peer;
						if(!queue->peer->getSerialNumber().empty()) _peersBySerial[queue->peer->getSerialNumber()] = queue->peer;
						_peersMutex.unlock();
						queue->peer->save(true, true, false);
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
					HelperFunctions::printMessage("Added peer 0x" + HelperFunctions::getHexString(queue->peer->getAddress()) + ".");
					for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = queue->peer->rpcDevice->channels.begin(); i != queue->peer->rpcDevice->channels.end(); ++i)
					{
						if(i->second->hasTeam)
						{
							HelperFunctions::printInfo("Info: Creating team for channel " + std::to_string(i->first) + ".");
							resetTeam(queue->peer, i->first);
							//Check if a peer exists with this devices team and if yes add it to the team.
							//As the serial number of the team was unknown up to this point, this couldn't
							//be done before.
							try
							{
								std::vector<std::shared_ptr<Peer>> peers;
								_peersMutex.lock();
								for(std::unordered_map<int32_t, std::shared_ptr<Peer>>::const_iterator j = _peers.begin(); j != _peers.end(); ++j)
								{
									if(j->second->getTeamRemoteAddress() == queue->peer->getAddress() && j->second->getAddress() != queue->peer->getAddress())
									{
										//Needed to avoid deadlocks
										peers.push_back(j->second);
									}
								}
								_peersMutex.unlock();
								for(std::vector<std::shared_ptr<Peer>>::iterator j = peers.begin(); j != peers.end(); ++j)
								{
									HelperFunctions::printInfo("Info: Adding device 0x" + HelperFunctions::getHexString((*j)->getAddress()) + " to team " + queue->peer->getTeamRemoteSerialNumber() + ".");
									addPeerToTeam(*j, (*j)->getTeamChannel(), (*j)->getTeamRemoteChannel(), queue->peer->getTeamRemoteSerialNumber());
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
							break;
						}
					}
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

		if(queue->isEmpty())
		{
			std::shared_ptr<Peer> peer = getPeer(packet->senderAddress());
			if(peer && !peer->getPairingComplete())
			{
				addHomegearFeatures(peer, -1, true);
				peer->setPairingComplete(true);
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

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::addDevice(std::string serialNumber)
{
	try
	{
		bool oldPairingModeState = _pairing;
		if(!_pairing) _pairing = true;

		if(serialNumber.empty()) return RPC::RPCVariable::createError(-2, "Serial number is empty.");
		if(serialNumber.size() > 10) return RPC::RPCVariable::createError(-2, "Serial number is too long.");

		std::vector<uint8_t> payload;
		payload.push_back(0x01);
		payload.push_back(serialNumber.size());
		payload.insert(payload.end(), serialNumber.begin(), serialNumber.end());
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(0, 0x84, 0x01, _address, 0, payload));

		int32_t i = 0;
		std::shared_ptr<Peer> peer;
		while(!peer && i < 3)
		{
			packet->setMessageCounter(_messageCounter[0]);
			_messageCounter[0]++;
			std::thread t(&HomeMaticDevice::sendPacket, this, packet, false);
			t.detach();
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			peer = getPeer(serialNumber);
			i++;
		}

		_pairing = oldPairingModeState;

		if(!peer)
		{
			return RPC::RPCVariable::createError(-1, "No response from device.");
		}
		else
		{
			return peer->getDeviceDescription(-1);
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::addLink(std::string senderSerialNumber, int32_t senderChannelIndex, std::string receiverSerialNumber, int32_t receiverChannelIndex, std::string name, std::string description)
{
	try
	{
		if(senderSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given receiver address is empty.");
		std::shared_ptr<Peer> sender = getPeer(senderSerialNumber);
		std::shared_ptr<Peer> receiver = getPeer(receiverSerialNumber);
		if(!sender) return RPC::RPCVariable::createError(-2, "Sender device not found.");
		if(!receiver) return RPC::RPCVariable::createError(-2, "Receiver device not found.");
		if(senderChannelIndex < 0) senderChannelIndex = 0;
		if(receiverChannelIndex < 0) receiverChannelIndex = 0;
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
		senderPeer->address = sender->getAddress();
		senderPeer->channel = senderChannelIndex;
		senderPeer->serialNumber = sender->getSerialNumber();
		senderPeer->linkDescription = description;
		senderPeer->linkName = name;

		std::shared_ptr<BasicPeer> receiverPeer(new BasicPeer());
		receiverPeer->address = receiver->getAddress();
		receiverPeer->channel = receiverChannelIndex;
		receiverPeer->serialNumber = receiver->getSerialNumber();
		receiverPeer->linkDescription = description;
		receiverPeer->linkName = name;

		sender->addPeer(senderChannelIndex, receiverPeer);
		receiver->addPeer(receiverChannelIndex, senderPeer);

		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, sender->getAddress());
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		std::vector<uint8_t> payload;

		uint8_t configByte = 0xA0;
		if(sender->rpcDevice->rxModes & RPC::Device::RXModes::burst) configByte |= 0x10;
		std::shared_ptr<BasicPeer> hiddenPeer(sender->getHiddenPeer(senderChannelIndex));
		if(hiddenPeer)
		{
			sender->removePeer(senderChannelIndex, hiddenPeer->address, hiddenPeer->channel);
			if(!sender->getHiddenPeerDevice()) GD::devices.remove(hiddenPeer->address);

			payload.clear();
			payload.push_back(senderChannelIndex);
			payload.push_back(0x02);
			payload.push_back(hiddenPeer->address >> 16);
			payload.push_back((hiddenPeer->address >> 8) & 0xFF);
			payload.push_back(hiddenPeer->address & 0xFF);
			payload.push_back(hiddenPeer->channel);
			payload.push_back(0);
			std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], configByte, 0x01, _address, sender->getAddress(), payload));
			pendingQueue->push(configPacket);
			pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			_messageCounter[0]++;
			configByte = 0xA0;
		}

		payload.clear();
		//CONFIG_ADD_PEER
		payload.push_back(senderChannelIndex);
		payload.push_back(0x01);
		payload.push_back(receiver->getAddress() >> 16);
		payload.push_back((receiver->getAddress() >> 8) & 0xFF);
		payload.push_back(receiver->getAddress() & 0xFF);
		payload.push_back(receiverChannelIndex);
		payload.push_back(0);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], configByte, 0x01, _address, sender->getAddress(), payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		sender->serviceMessages->setConfigPending(true);
		sender->pendingBidCoSQueues->push(pendingQueue);

		if(senderChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::link) != senderChannel->parameterSets.end())
		{
			std::shared_ptr<RPC::RPCVariable> paramset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
			std::unordered_map<std::string, RPCConfigurationParameter>* linkConfig = &sender->linksCentral.at(senderChannelIndex).at(receiver->getAddress()).at(receiverChannelIndex);
			for(std::unordered_map<std::string, RPCConfigurationParameter>::iterator i = linkConfig->begin(); i != linkConfig->end(); ++i)
			{
				paramset->structValue->push_back(i->second.rpcParameter->convertFromPacket(i->second.data));
				paramset->structValue->back()->name = i->first;
			}
			//putParamset pushes the packets on pendingQueues, but does not send immediately
			sender->putParamset(senderChannelIndex, RPC::ParameterSet::Type::Enum::link, receiverSerialNumber, receiverChannelIndex, paramset, true, true);

			if(!receiverChannel->enforceLinks.empty())
			{
				std::shared_ptr<RPC::ParameterSet> linkset = senderChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::link);
				paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
				for(std::vector<std::shared_ptr<RPC::EnforceLink>>::iterator i = receiverChannel->enforceLinks.begin(); i != receiverChannel->enforceLinks.end(); ++i)
				{
					std::shared_ptr<RPC::Parameter> parameter = linkset->getParameter((*i)->id);
					if(parameter)
					{
						paramset->structValue->push_back((*i)->getValue(parameter->logicalParameter->type));
						paramset->structValue->back()->name = (*i)->id;
					}
				}
				//putParamset pushes the packets on pendingQueues, but does not send immediately
				sender->putParamset(senderChannelIndex, RPC::ParameterSet::Type::Enum::link, receiverSerialNumber, receiverChannelIndex, paramset, true, true);
			}
		}

		queue->push(sender->pendingBidCoSQueues);

		GD::rpcClient.broadcastUpdateDevice(senderSerialNumber + ":" + std::to_string(senderChannelIndex), RPC::Client::Hint::Enum::updateHintLinks);

		while(_bidCoSQueueManager.get(sender->getAddress()))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, receiver->getAddress());
		pendingQueue.reset(new BidCoSQueue(BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		configByte = 0xA0;
		if(receiver->rpcDevice->rxModes & RPC::Device::RXModes::burst) configByte |= 0x10;

		hiddenPeer = receiver->getHiddenPeer(receiverChannelIndex);
		if(hiddenPeer)
		{
			sender->removePeer(receiverChannelIndex, hiddenPeer->address, hiddenPeer->channel);
			if(!sender->getHiddenPeerDevice()) GD::devices.remove(hiddenPeer->address);

			payload.clear();
			payload.push_back(receiverChannelIndex);
			payload.push_back(0x02);
			payload.push_back(hiddenPeer->address >> 16);
			payload.push_back((hiddenPeer->address >> 8) & 0xFF);
			payload.push_back(hiddenPeer->address & 0xFF);
			payload.push_back(hiddenPeer->channel);
			payload.push_back(0);
			std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], configByte, 0x01, _address, receiver->getAddress(), payload));
			pendingQueue->push(configPacket);
			pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			_messageCounter[0]++;
			configByte = 0xA0;
		}

		payload.clear();
		//CONFIG_ADD_PEER
		payload.push_back(receiverChannelIndex);
		payload.push_back(0x01);
		payload.push_back(sender->getAddress() >> 16);
		payload.push_back((sender->getAddress() >> 8) & 0xFF);
		payload.push_back(sender->getAddress() & 0xFF);
		payload.push_back(senderChannelIndex);
		payload.push_back(0);
		configPacket.reset(new BidCoSPacket(_messageCounter[0], configByte, 0x01, _address, receiver->getAddress(), payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		receiver->serviceMessages->setConfigPending(true);
		receiver->pendingBidCoSQueues->push(pendingQueue);

		if(receiverChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::link) != receiverChannel->parameterSets.end())
		{
			std::shared_ptr<RPC::RPCVariable> paramset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
			std::unordered_map<std::string, RPCConfigurationParameter>* linkConfig = &receiver->linksCentral.at(receiverChannelIndex).at(sender->getAddress()).at(senderChannelIndex);
			for(std::unordered_map<std::string, RPCConfigurationParameter>::iterator i = linkConfig->begin(); i != linkConfig->end(); ++i)
			{
				paramset->structValue->push_back(i->second.rpcParameter->convertFromPacket(i->second.data));
				paramset->structValue->back()->name = i->first;
			}
			//putParamset pushes the packets on pendingQueues, but does not send immediately
			receiver->putParamset(receiverChannelIndex, RPC::ParameterSet::Type::Enum::link, senderSerialNumber, senderChannelIndex, paramset, true, true);

			if(!senderChannel->enforceLinks.empty())
			{
				std::shared_ptr<RPC::ParameterSet> linkset = receiverChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::link);
				paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
				for(std::vector<std::shared_ptr<RPC::EnforceLink>>::iterator i = senderChannel->enforceLinks.begin(); i != senderChannel->enforceLinks.end(); ++i)
				{
					std::shared_ptr<RPC::Parameter> parameter = linkset->getParameter((*i)->id);
					if(parameter)
					{
						paramset->structValue->push_back((*i)->getValue(parameter->logicalParameter->type));
						paramset->structValue->back()->name = (*i)->id;
					}
				}
				//putParamset pushes the packets on pendingQueues, but does not send immediately
				receiver->putParamset(receiverChannelIndex, RPC::ParameterSet::Type::Enum::link, senderSerialNumber, senderChannelIndex, paramset, true, true);
			}
		}

		queue->push(receiver->pendingBidCoSQueues);

		GD::rpcClient.broadcastUpdateDevice(receiverSerialNumber + ":" + std::to_string(receiverChannelIndex), RPC::Client::Hint::Enum::updateHintLinks);

		while(_bidCoSQueueManager.get(receiver->getAddress()))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		//Check, if channel is part of a group and if that's the case add link for the grouped channel
		int32_t channelGroupedWith = sender->getChannelGroupedWith(senderChannelIndex);
		//I'm assuming that senderChannelIndex is always the first of the two grouped channels
		if(channelGroupedWith > senderChannelIndex)
		{
			return addLink(senderSerialNumber, channelGroupedWith, receiverSerialNumber, receiverChannelIndex, name, description);
		}
		else
		{
			return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::removeLink(std::string senderSerialNumber, int32_t senderChannelIndex, std::string receiverSerialNumber, int32_t receiverChannelIndex)
{
	try
	{
		if(senderSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given receiver address is empty.");
		std::shared_ptr<Peer> sender = getPeer(senderSerialNumber);
		std::shared_ptr<Peer> receiver = getPeer(receiverSerialNumber);
		if(!sender) return RPC::RPCVariable::createError(-2, "Sender device not found.");
		if(!receiver) return RPC::RPCVariable::createError(-2, "Receiver device not found.");
		if(senderChannelIndex < 0) senderChannelIndex = 0;
		if(receiverChannelIndex < 0) receiverChannelIndex = 0;
		if(sender->rpcDevice->channels.find(senderChannelIndex) == sender->rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Sender channel not found.");
		if(receiver->rpcDevice->channels.find(receiverChannelIndex) == receiver->rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Receiver channel not found.");
		if(!sender->getPeer(senderChannelIndex, receiver->getAddress()) && !receiver->getPeer(receiverChannelIndex, sender->getAddress())) return RPC::RPCVariable::createError(-6, "Devices are not paired to each other.");

		sender->removePeer(senderChannelIndex, receiver->getAddress(), receiverChannelIndex);
		receiver->removePeer(receiverChannelIndex, sender->getAddress(), senderChannelIndex);

		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, sender->getAddress());
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		uint8_t configByte = 0xA0;
		if(sender->rpcDevice->rxModes & RPC::Device::RXModes::burst) configByte |= 0x10;

		std::vector<uint8_t> payload;
		payload.push_back(senderChannelIndex);
		payload.push_back(0x02);
		payload.push_back(receiver->getAddress() >> 16);
		payload.push_back((receiver->getAddress() >> 8) & 0xFF);
		payload.push_back(receiver->getAddress() & 0xFF);
		payload.push_back(receiverChannelIndex);
		payload.push_back(0);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], configByte, 0x01, _address, sender->getAddress(), payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		sender->serviceMessages->setConfigPending(true);
		sender->pendingBidCoSQueues->push(pendingQueue);
		queue->push(sender->pendingBidCoSQueues);

		addHomegearFeatures(sender, senderChannelIndex, false);

		GD::rpcClient.broadcastUpdateDevice(senderSerialNumber + ":" + std::to_string(senderChannelIndex), RPC::Client::Hint::Enum::updateHintLinks);

		while(_bidCoSQueueManager.get(sender->getAddress()))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		queue = _bidCoSQueueManager.createQueue(this, BidCoSQueueType::CONFIG, receiver->getAddress());
		pendingQueue.reset(new BidCoSQueue(BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		configByte = 0xA0;
		if(receiver->rpcDevice->rxModes & RPC::Device::RXModes::burst) configByte |= 0x10;

		payload.clear();
		payload.push_back(receiverChannelIndex);
		payload.push_back(0x02);
		payload.push_back(sender->getAddress() >> 16);
		payload.push_back((sender->getAddress() >> 8) & 0xFF);
		payload.push_back(sender->getAddress() & 0xFF);
		payload.push_back(senderChannelIndex);
		payload.push_back(0);
		configPacket.reset(new BidCoSPacket(_messageCounter[0], configByte, 0x01, _address, receiver->getAddress(), payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		receiver->serviceMessages->setConfigPending(true);
		receiver->pendingBidCoSQueues->push(pendingQueue);
		queue->push(receiver->pendingBidCoSQueues);

		addHomegearFeatures(receiver, receiverChannelIndex, false);

		GD::rpcClient.broadcastUpdateDevice(receiverSerialNumber + ":" + std::to_string(receiverChannelIndex), RPC::Client::Hint::Enum::updateHintLinks);

		while(_bidCoSQueueManager.get(receiver->getAddress()))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::deleteDevice(std::string serialNumber, int32_t flags)
{
	try
	{
		if(serialNumber.empty()) return RPC::RPCVariable::createError(-2, "Unknown device.");
		if(serialNumber[0] == '*') return RPC::RPCVariable::createError(-2, "Cannot delete virtual device.");
		std::shared_ptr<Peer> peer = getPeer(serialNumber);
		if(!peer) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
		int32_t address = peer->getAddress();

		bool defer = flags & 0x04;
		bool force = flags & 0x02;
		//Reset
		if(flags & 0x01)
		{
			std::thread t(&HomeMaticCentral::reset, this, address, defer);
			t.detach();
		}
		else
		{
			std::thread t(&HomeMaticCentral::unpair, this, address, defer);
			t.detach();
		}
		//Force delete
		if(force) deletePeer(address);
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			while(_bidCoSQueueManager.get(address) && peerExists(address))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		if(!defer && !force && peerExists(address)) return RPC::RPCVariable::createError(-1, "No answer from device.");

		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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

		std::vector<std::shared_ptr<Peer>> peers;
		//Copy all peers first, because listDevices takes very long and we don't want to lock _peersMutex too long
		_peersMutex.lock();
		for(std::unordered_map<std::string, std::shared_ptr<Peer>>::iterator i = _peersBySerial.begin(); i != _peersBySerial.end(); ++i)
		{
			if(knownDevices && knownDevices->find(i->first) != knownDevices->end()) continue; //only add unknown devices
			peers.push_back(i->second);
		}
		_peersMutex.unlock();

		for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
		{
			//listDevices really needs a lot of ressources, so wait a little bit after each device
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> descriptions = (*i)->getDeviceDescription();
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::listTeams()
{
	try
	{
		std::shared_ptr<RPC::RPCVariable> array(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));

		std::vector<std::shared_ptr<Peer>> peers;
		//Copy all peers first, because listTeams takes very long and we don't want to lock _peersMutex too long
		_peersMutex.lock();
		for(std::unordered_map<std::string, std::shared_ptr<Peer>>::iterator i = _peersBySerial.begin(); i != _peersBySerial.end(); ++i)
		{
			peers.push_back(i->second);
		}
		_peersMutex.unlock();

		for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
		{
			//listTeams really needs a lot of ressources, so wait a little bit after each device
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::string serialNumber = (*i)->getSerialNumber();
			if(serialNumber.empty() || serialNumber.at(0) != '*') continue;
			std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> descriptions = (*i)->getDeviceDescription();
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::setTeam(std::string serialNumber, int32_t channel, std::string teamSerialNumber, int32_t teamChannel, bool force, bool burst)
{
	try
	{
		if(channel < 0) channel = 0;
		if(teamChannel < 0) teamChannel = 0;
		std::shared_ptr<Peer> peer(getPeer(serialNumber));
		if(!peer) return RPC::RPCVariable::createError(-2, "Unknown device.");
		if(peer->rpcDevice->channels.find(channel) == peer->rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(!peer->rpcDevice->channels[channel]->hasTeam) return RPC::RPCVariable::createError(-6, "Channel does not support teams.");
		int32_t oldTeamAddress = peer->getTeamRemoteAddress();
		int32_t oldTeamChannel = peer->getTeamRemoteChannel();
		if(oldTeamChannel < 0) oldTeamChannel = 0;
		if(teamSerialNumber.empty()) //Reset team to default
		{
			if(!force && !peer->getTeamRemoteSerialNumber().empty() && peer->getTeamRemoteSerialNumber().substr(1) == peer->getSerialNumber() && peer->getTeamChannel() == channel)
			{
				//Team already is default
				return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
			}

			int32_t newChannel = -1;
			//Get first channel which has a team
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
			{
				if(i->second->hasTeam)
				{
					newChannel = i->first;
					break;
				}
			}
			if(newChannel < 0) return RPC::RPCVariable::createError(-6, "There are no team channels for this device.");

			resetTeam(peer, newChannel);
		}
		else //Set new team
		{
			if(!force && !peer->getTeamRemoteSerialNumber().empty() && peer->getTeamRemoteSerialNumber() == teamSerialNumber && peer->getTeamChannel() == channel && peer->getTeamRemoteChannel() == teamChannel)
			{
				//Peer already is member of this team
				return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
			}
			//Don't create team if not existent!
			std::shared_ptr<Peer> team = getPeer(teamSerialNumber);
			if(!team) return RPC::RPCVariable::createError(-2, "Team does not exist.");
			if(team->rpcDevice->channels.find(teamChannel) == team->rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown team channel.");
			if(team->rpcDevice->channels[teamChannel]->teamTag != peer->rpcDevice->channels[channel]->teamTag) return RPC::RPCVariable::createError(-6, "Peer channel is not compatible to team channel.");

			addPeerToTeam(peer, channel, teamChannel, teamSerialNumber);
		}
		std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(BidCoSQueueType::CONFIG));
		queue->noSending = true;
		peer->serviceMessages->setConfigPending(true);

		uint8_t configByte = 0xA0;
		if(burst && (peer->rpcDevice->rxModes & RPC::Device::RXModes::burst)) configByte |= 0x10;

		std::vector<uint8_t> payload;
		if(oldTeamAddress != 0 && oldTeamAddress != peer->getTeamRemoteAddress())
		{
			payload.push_back(channel);
			payload.push_back(0x02);
			payload.push_back(oldTeamAddress >> 16);
			payload.push_back((oldTeamAddress >> 8) & 0xFF);
			payload.push_back(oldTeamAddress & 0xFF);
			payload.push_back(oldTeamChannel);
			payload.push_back(0);
			std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(peer->getMessageCounter(), configByte, 0x01, _address, peer->getAddress(), payload));
			peer->setMessageCounter(peer->getMessageCounter() + 1);
			queue->push(packet);
			queue->push(GD::devices.getCentral()->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			configByte = 0xA0;
		}

		payload.clear();
		payload.push_back(channel);
		payload.push_back(0x01);
		payload.push_back(peer->getTeamRemoteAddress() >> 16);
		payload.push_back((peer->getTeamRemoteAddress() >> 8) & 0xFF);
		payload.push_back(peer->getTeamRemoteAddress() & 0xFF);
		payload.push_back(peer->getTeamRemoteChannel());
		payload.push_back(0);
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(peer->getMessageCounter(), configByte, 0x01, _address, peer->getAddress(), payload));
		peer->setMessageCounter(peer->getMessageCounter() + 1);
		queue->push(packet);
		queue->push(GD::devices.getCentral()->getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));

		peer->pendingBidCoSQueues->push(queue);
		if((peer->rpcDevice->rxModes & RPC::Device::RXModes::Enum::always) || (peer->rpcDevice->rxModes & RPC::Device::RXModes::Enum::burst)) enqueuePendingQueues(peer->getAddress());
		else HelperFunctions::printDebug("Debug: Packet was queued and will be sent with next wake me up packet.");
		GD::rpcClient.broadcastUpdateDevice(serialNumber + ":" + std::to_string(channel), RPC::Client::Hint::Enum::updateHintAll);
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getDeviceDescription(std::string serialNumber, int32_t channel)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(serialNumber));
		if(!peer) return RPC::RPCVariable::createError(-2, "Unknown device.");

		return peer->getDeviceDescription(channel);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::setLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description)
{
	try
	{
		if(senderSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given receiver address is empty.");
		std::shared_ptr<Peer> sender(getPeer(senderSerialNumber));
		std::shared_ptr<Peer> receiver(getPeer(receiverSerialNumber));
		std::shared_ptr<RPC::RPCVariable> result1;
		std::shared_ptr<RPC::RPCVariable> result2;
		if(sender)
		{
			result1 = sender->setLinkInfo(senderChannel, receiverSerialNumber, receiverChannel, name, description);
		}
		if(receiver)
		{
			result2 = receiver->setLinkInfo(receiverChannel, senderSerialNumber, senderChannel, name, description);
		}
		if(!result1 || !result2) RPC::RPCVariable::createError(-2, "At least one device was not found.");
		if(result1->errorStruct) return result1;
		if(result2->errorStruct) return result2;
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel)
{
	try
	{
		if(senderSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return RPC::RPCVariable::createError(-2, "Given receiver address is empty.");
		std::shared_ptr<Peer> sender(getPeer(senderSerialNumber));
		std::shared_ptr<Peer> receiver(getPeer(receiverSerialNumber));
		if(!sender)
		{
			if(receiver) return receiver->getLinkInfo(receiverChannel, senderSerialNumber, senderChannel);
			else return RPC::RPCVariable::createError(-2, "Both devices not found.");
		}
		else return sender->getLinkInfo(senderChannel, receiverSerialNumber, receiverChannel);
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getLinkPeers(std::string serialNumber, int32_t channel)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(serialNumber));
		if(!peer) return RPC::RPCVariable::createError(-2, "Unknown device.");
		return peer->getLinkPeers(channel);
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
			try
			{
				std::vector<std::shared_ptr<Peer>> peers;
				//Copy all peers first, because getLinks takes very long and we don't want to lock _peersMutex too long
				_peersMutex.lock();
				for(std::unordered_map<std::string, std::shared_ptr<Peer>>::iterator i = _peersBySerial.begin(); i != _peersBySerial.end(); ++i)
				{
					peers.push_back(i->second);
				}
				_peersMutex.unlock();

				for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
				{
					//listDevices really needs a lot of ressources, so wait a little bit after each device
					std::this_thread::sleep_for(std::chrono::milliseconds(3));
					element = (*i)->getLink(channel, flags, true);
					array->arrayValue->insert(array->arrayValue->begin(), element->arrayValue->begin(), element->arrayValue->end());
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
		else
		{
			std::shared_ptr<Peer> peer(getPeer(serialNumber));
			if(!peer) return RPC::RPCVariable::createError(-2, "Unknown device.");
			element = peer->getLink(channel, flags, false);
			array->arrayValue->insert(array->arrayValue->begin(), element->arrayValue->begin(), element->arrayValue->end());
		}
		return array;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getParamsetId(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(serialNumber));
		if(peer) return peer->getParamsetId(channel, type, remoteSerialNumber, remoteChannel);
		return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::putParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> paramset)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(serialNumber));
		if(peer)
		{
			std::shared_ptr<RPC::RPCVariable> result = peer->putParamset(channel, type, remoteSerialNumber, remoteChannel, paramset);
			if(result->errorStruct) return result;
			while(_bidCoSQueueManager.get(peer->getAddress()))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			return result;
		}
		return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(serialNumber == "BidCoS-RF" && channel == 0 && type == RPC::ParameterSet::Type::Enum::master)
		{
			std::shared_ptr<RPC::RPCVariable> paramset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
			paramset->structValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable("AES_KEY", 1)));
			return paramset;
		}
		else
		{
			std::shared_ptr<Peer> peer(getPeer(serialNumber));
			if(peer) return peer->getParamset(channel, type, remoteSerialNumber, remoteChannel);
			return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getParamsetDescription(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(serialNumber));
		if(peer) return peer->getParamsetDescription(channel, type, remoteSerialNumber, remoteChannel);
		return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getServiceMessages()
{
	try
	{
		std::vector<std::shared_ptr<Peer>> peers;
		//Copy all peers first, because getServiceMessages takes very long and we don't want to lock _peersMutex too long
		_peersMutex.lock();
		for(std::unordered_map<std::string, std::shared_ptr<Peer>>::iterator i = _peersBySerial.begin(); i != _peersBySerial.end(); ++i)
		{
			peers.push_back(i->second);
		}
		_peersMutex.unlock();

		std::shared_ptr<RPC::RPCVariable> serviceMessages(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
		{
			if(!*i) continue;
			//getServiceMessages really needs a lot of ressources, so wait a little bit after each device
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<RPC::RPCVariable> messages = (*i)->getServiceMessages();
			if(!messages->arrayValue->empty()) serviceMessages->arrayValue->insert(serviceMessages->arrayValue->end(), messages->arrayValue->begin(), messages->arrayValue->end());
		}
		return serviceMessages;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::getValue(std::string serialNumber, uint32_t channel, std::string valueKey)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(serialNumber));
		if(peer) return peer->getValue(channel, valueKey);
		return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::setValue(std::string serialNumber, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		std::shared_ptr<Peer> peer(getPeer(serialNumber));
		if(peer) return peer->setValue(channel, valueKey, value);
		return RPC::RPCVariable::createError(-2, "Unknown device.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

void HomeMaticCentral::pairingModeTimer(int32_t duration, bool debugOutput)
{
	try
	{
		_pairing = true;
		if(debugOutput) HelperFunctions::printInfo("Info: Pairing mode enabled.");
		_timeLeftInPairingMode = duration;
		int64_t startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		int64_t timePassed = 0;
		while(timePassed < (duration * 1000) && !_stopPairingModeThread)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
			timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - startTime;
			_timeLeftInPairingMode = duration - (timePassed / 1000);
		}
		_timeLeftInPairingMode = 0;
		_pairing = false;
		if(debugOutput) HelperFunctions::printInfo("Info: Pairing mode disabled.");
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

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::deleteMetadata(std::string objectID, std::string dataID)
{
	try
	{
		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(objectID)));
		std::string command("DELETE FROM metadata WHERE objectID=?");
		if(!dataID.empty())
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(dataID)));
			command.append(" AND dataID=?");
		}
		GD::db.executeCommand(command, data);

		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HomeMaticCentral::setInstallMode(bool on, int32_t duration, bool debugOutput)
{
	try
	{
		_stopPairingModeThread = true;
		if(_pairingModeThread.joinable()) _pairingModeThread.join();
		_stopPairingModeThread = false;
		_timeLeftInPairingMode = 0;
		if(on && duration >= 5)
		{
			_timeLeftInPairingMode = duration; //It's important to set it here, because the thread often doesn't completely initialize before getInstallMode requests _timeLeftInPairingMode
			_pairingModeThread = std::thread(&HomeMaticCentral::pairingModeTimer, this, duration, debugOutput);
		}
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}
