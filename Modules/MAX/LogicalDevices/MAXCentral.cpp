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

#include "MAXCentral.h"
#include "../GD.h"

namespace MAX {

MAXCentral::MAXCentral(IDeviceEventSink* eventHandler) : MAXDevice(eventHandler), BaseLib::Systems::Central(GD::bl, this)
{
	init();
}

MAXCentral::MAXCentral(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : MAXDevice(deviceID, serialNumber, address, eventHandler), Central(GD::bl, this)
{
	init();
}

MAXCentral::~MAXCentral()
{
	dispose();
	if(_pairingModeThread.joinable())
	{
		_stopPairingModeThread = true;
		_pairingModeThread.join();
	}
}

void MAXCentral::init()
{
	try
	{
		MAXDevice::init();

		_deviceType = (uint32_t)DeviceType::MAXCENTRAL;

		for(std::map<std::string, std::shared_ptr<IPhysicalInterface>>::iterator i = GD::physicalInterfaces.begin(); i != GD::physicalInterfaces.end(); ++i)
		{
			i->second->addEventHandler((IPhysicalInterface::IPhysicalInterfaceEventSink*)this);
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

void MAXCentral::setUpMAXMessages()
{
	try
	{
		//Don't call MAXDevice::setUpMAXMessages!
		_messages->add(std::shared_ptr<MAXMessage>(new MAXMessage(0x00, 0x04, this, ACCESSPAIREDTOSENDER, FULLACCESS, &MAXDevice::handlePairingRequest)));

		_messages->add(std::shared_ptr<MAXMessage>(new MAXMessage(0x02, 0x02, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &MAXDevice::handleAck)));

		_messages->add(std::shared_ptr<MAXMessage>(new MAXMessage(0x03, 0x0A, this, ACCESSPAIREDTOSENDER | ACCESSDESTISME, ACCESSPAIREDTOSENDER | ACCESSDESTISME, &MAXDevice::handleTimeRequest)));
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

void MAXCentral::worker()
{
	try
	{
		std::chrono::milliseconds sleepingTime(10);
		uint32_t counter = 0;
		int32_t lastPeer;
		lastPeer = 0;
		//One loop on the Raspberry Pi takes about 30Âµs
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
						int32_t windowTimePerPeer = _bl->settings.workerThreadWindow() / _peers.size();
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
						std::unordered_map<int32_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator nextPeer = _peers.find(lastPeer);
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
				std::shared_ptr<MAXPeer> peer(getPeer(lastPeer));
				if(peer && !peer->deleting) peer->worker();
				counter++;
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

bool MAXCentral::onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(_disposing) return false;
		std::shared_ptr<MAXPacket> maxPacket(std::dynamic_pointer_cast<MAXPacket>(packet));
		if(!maxPacket) return false;
		if(maxPacket->senderAddress() == _address) //Packet spoofed
		{
			std::shared_ptr<MAXPeer> peer(getPeer(maxPacket->destinationAddress()));
			if(peer)
			{
				if(senderID != peer->getPhysicalInterfaceID()) return true; //Packet we sent was received by another interface
				GD::out.printWarning("Warning: Central address of packet to peer " + std::to_string(peer->getID()) + " was spoofed. Packet was: " + packet->hexString());
				peer->serviceMessages->set("CENTRAL_ADDRESS_SPOOFED", 1, 0);
				std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string> { "CENTRAL_ADDRESS_SPOOFED" });
				std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values(new std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>> { std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)1)) });
				raiseRPCEvent(peer->getID(), 0, peer->getSerialNumber() + ":0", valueKeys, values);
				return true;
			}
			return false;
		}
		std::shared_ptr<IPhysicalInterface> physicalInterface = getPhysicalInterface(maxPacket->senderAddress());
		if(physicalInterface->getID() != senderID) return true;
		bool handled = MAXDevice::onPacketReceived(senderID, maxPacket);
		std::shared_ptr<MAXPeer> peer(getPeer(maxPacket->senderAddress()));
		if(!peer) return false;
		std::shared_ptr<MAXPeer> team;
		if(handled)
		{
			//This block is not necessary for teams as teams will never have queues.
			std::shared_ptr<PacketQueue> queue = _queueManager.get(maxPacket->senderAddress());
			if(queue && queue->getQueueType() != PacketQueueType::PEER)
			{
				peer->setLastPacketReceived();
				peer->serviceMessages->endUnreach();
				peer->setRSSIDevice(maxPacket->rssiDevice());
				return true; //Packet is handled by queue. Don't check if queue is empty!
			}
		}
		peer->packetReceived(maxPacket);
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
    return false;
}

void MAXCentral::reset(uint64_t id, bool defer)
{
	try
	{
		std::shared_ptr<MAXPeer> peer(getPeer(id));
		if(!peer) return;
		std::shared_ptr<PacketQueue> queue = _queueManager.createQueue(this, peer->getPhysicalInterface(), PacketQueueType::UNPAIRING, peer->getAddress());
		queue->peer = peer;
		std::shared_ptr<PacketQueue> pendingQueue(new PacketQueue(peer->getPhysicalInterface(), PacketQueueType::UNPAIRING));
		pendingQueue->noSending = true;

		//RESET
		std::vector<uint8_t> payload;
		payload.push_back(0);
		std::shared_ptr<MAXPacket> resetPacket(new MAXPacket(_messageCounter[0], 0xF0, 0, _address, peer->getAddress(), payload, false));
		pendingQueue->push(resetPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++; //Count resends in

		if(defer)
		{
			while(!peer->pendingQueues->empty()) peer->pendingQueues->pop();
			peer->pendingQueues->push(pendingQueue);
			peer->serviceMessages->setConfigPending(true);
			queue->push(peer->pendingQueues);
		}
		else queue->push(pendingQueue, true, true);
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

void MAXCentral::deletePeer(uint64_t id)
{
	try
	{
		std::shared_ptr<MAXPeer> peer(getPeer(id));
		if(!peer) return;
		peer->deleting = true;
		std::shared_ptr<BaseLib::RPC::RPCVariable> deviceAddresses(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		deviceAddresses->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(peer->getSerialNumber())));
		for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
		{
			deviceAddresses->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(peer->getSerialNumber() + ":" + std::to_string(i->first))));
		}
		std::shared_ptr<BaseLib::RPC::RPCVariable> deviceInfo(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		deviceInfo->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)peer->getID()))));
		std::shared_ptr<BaseLib::RPC::RPCVariable> channels(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		deviceInfo->structValue->insert(BaseLib::RPC::RPCStructElement("CHANNELS", channels));
		for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
		{
			channels->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(i->first)));
		}
		raiseRPCDeleteDevices(deviceAddresses, deviceInfo);
		_peersMutex.lock();
		if(_peersBySerial.find(peer->getSerialNumber()) != _peersBySerial.end()) _peersBySerial.erase(peer->getSerialNumber());
		if(_peersByID.find(id) != _peersByID.end()) _peersByID.erase(id);
		if(_peers.find(peer->getAddress()) != _peers.end()) _peers.erase(peer->getAddress());
		_peersMutex.unlock();
		peer->deleteFromDatabase();
		GD::out.printMessage("Removed peer " + std::to_string(peer->getID()));
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

std::string MAXCentral::handleCLICommand(std::string command)
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
		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "pairing on\t\tEnables pairing mode" << std::endl;
			stringStream << "pairing off\t\tDisables pairing mode" << std::endl;
			stringStream << "peers list\t\tList all peers" << std::endl;
			stringStream << "peers remove\t\tRemove a peer (without unpairing)" << std::endl;
			stringStream << "peers select\t\tSelect a peer" << std::endl;
			stringStream << "peers unpair\t\tUnpair a peer" << std::endl;
			stringStream << "unselect\t\tUnselect this device" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 10, "pairing on") == 0)
		{
			int32_t duration = 60;

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
					duration = BaseLib::HelperFunctions::getNumber(element, false);
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
			uint64_t peerID;

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
					peerID = BaseLib::HelperFunctions::getNumber(element, false);
					if(peerID == 0) return "Invalid id.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command removes a peer without trying to unpair it first." << std::endl;
				stringStream << "Usage: peers remove PEERID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to remove. Example: 513" << std::endl;
				return stringStream.str();
			}

			if(!peerExists(peerID)) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				if(_currentPeer && _currentPeer->getID() == peerID) _currentPeer.reset();
				deletePeer(peerID);
				stringStream << "Removed peer " << std::to_string(peerID) << "." << std::endl;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 12, "peers unpair") == 0)
		{
			uint64_t peerID;

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
					peerID = BaseLib::HelperFunctions::getNumber(element, false);
					if(peerID == 0) return "Invalid id.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command unpairs a peer." << std::endl;
				stringStream << "Usage: peers unpair PEERID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to unpair. Example: 513" << std::endl;
				return stringStream.str();
			}

			if(!peerExists(peerID)) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				if(_currentPeer && _currentPeer->getID() == peerID) _currentPeer.reset();
				stringStream << "Unpairing peer " << std::to_string(peerID) << std::endl;
				reset(peerID, true);
			}
			return stringStream.str();
		}
		else if(command.compare(0, 10, "peers list") == 0)
		{
			try
			{
				std::string filterType;
				std::string filterValue;

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
							index = -1;
							break;
						}
						filterType = BaseLib::HelperFunctions::toLower(element);
					}
					else if(index == 3) filterValue = element;
					index++;
				}
				if(index == -1)
				{
					stringStream << "Description: This command unpairs a peer." << std::endl;
					stringStream << "Usage: peers list [FILTERTYPE] [FILTERVALUE]" << std::endl << std::endl;
					stringStream << "Parameters:" << std::endl;
					stringStream << "  FILTERTYPE:\tSee filter types below." << std::endl;
					stringStream << "  FILTERVALUE:\tDepends on the filter type. If a number is required, it has to be in hexadecimal format." << std::endl << std::endl;
					stringStream << "Filter types:" << std::endl;
					stringStream << "  PEERID: Filter by id." << std::endl;
					stringStream << "      FILTERVALUE: The id of the peer to filter (e. g. 513)." << std::endl;
					stringStream << "  ADDRESS: Filter by address." << std::endl;
					stringStream << "      FILTERVALUE: The 4 byte address of the peer to filter (e. g. 001DA44D)." << std::endl;
					stringStream << "  SERIAL: Filter by serial number." << std::endl;
					stringStream << "      FILTERVALUE: The serial number of the peer to filter (e. g. JEQ0554309)." << std::endl;
					stringStream << "  TYPE: Filter by device type." << std::endl;
					stringStream << "      FILTERVALUE: The 2 byte device type in hexadecimal format." << std::endl;
					stringStream << "  UNREACH: List all unreachable peers." << std::endl;
					stringStream << "      FILTERVALUE: empty" << std::endl;
					return stringStream.str();
				}

				if(_peers.empty())
				{
					stringStream << "No peers are paired to this central." << std::endl;
					return stringStream.str();
				}
				std::string bar(" │ ");
				const int32_t idWidth = 8;
				const int32_t addressWidth = 7;
				const int32_t serialWidth = 13;
				const int32_t typeWidth1 = 4;
				const int32_t typeWidth2 = 25;
				const int32_t firmwareWidth = 8;
				const int32_t unreachWidth = 7;
				std::string typeStringHeader("Type String");
				typeStringHeader.resize(typeWidth2, ' ');
				stringStream << std::setfill(' ')
					<< std::setw(idWidth) << "ID" << bar
					<< std::setw(addressWidth) << "Address" << bar
					<< std::setw(serialWidth) << "Serial Number" << bar
					<< std::setw(typeWidth1) << "Type" << bar
					<< typeStringHeader << bar
					<< std::setw(firmwareWidth) << "Firmware" << bar
					<< std::setw(unreachWidth) << "Unreach"
					<< std::endl;
				stringStream << "─────────┼─────────┼───────────────┼──────┼───────────────────────────┼──────────┼────────" << std::endl;
				stringStream << std::setfill(' ')
					<< std::setw(idWidth) << " " << bar
					<< std::setw(addressWidth) << " " << bar
					<< std::setw(serialWidth) << " " << bar
					<< std::setw(typeWidth1) << " " << bar
					<< std::setw(typeWidth2) << " " << bar
					<< std::setw(firmwareWidth) << " " << bar
					<< std::setw(unreachWidth) << " "
					<< std::endl;
				_peersMutex.lock();
				for(std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersByID.begin(); i != _peersByID.end(); ++i)
				{
					if(filterType == "id")
					{
						uint64_t id = BaseLib::HelperFunctions::getNumber(filterValue, true);
						if(i->second->getID() != id) continue;
					}
					else if(filterType == "address")
					{
						int32_t address = BaseLib::HelperFunctions::getNumber(filterValue, true);
						if(i->second->getAddress() != address) continue;
					}
					else if(filterType == "serial")
					{
						if(i->second->getSerialNumber() != filterValue) continue;
					}
					else if(filterType == "type")
					{
						int32_t deviceType = BaseLib::HelperFunctions::getNumber(filterValue, true);
						if((int32_t)i->second->getDeviceType().type() != deviceType) continue;
					}
					else if(filterType == "unreach")
					{
						if(i->second->serviceMessages)
						{
							if(!i->second->serviceMessages->getUnreach()) continue;
						}
					}

					stringStream
						<< std::setw(idWidth) << std::setfill(' ') << std::to_string(i->second->getID()) << bar
						<< std::setw(addressWidth) << BaseLib::HelperFunctions::getHexString(i->second->getAddress(), 6) << bar
						<< std::setw(serialWidth) << i->second->getSerialNumber() << bar
						<< std::setw(typeWidth1) << BaseLib::HelperFunctions::getHexString(i->second->getDeviceType().type(), 4) << bar;
					if(i->second->rpcDevice)
					{
						std::shared_ptr<BaseLib::RPC::DeviceType> type = i->second->rpcDevice->getType(i->second->getDeviceType(), i->second->getFirmwareVersion());
						std::string typeID;
						if(type) typeID = type->id;
						typeID.resize(typeWidth2, ' ');
						stringStream << typeID  << bar;
					}
					else stringStream << std::setw(typeWidth2) << " " << bar;
					if(i->second->getFirmwareVersion() == 0) stringStream << std::setfill(' ') << std::setw(firmwareWidth) << "?" << bar;
					else if(i->second->firmwareUpdateAvailable())
					{
						stringStream << std::setfill(' ') << std::setw(firmwareWidth) << ("*" + BaseLib::HelperFunctions::getHexString(i->second->getFirmwareVersion() >> 4) + "." + BaseLib::HelperFunctions::getHexString(i->second->getFirmwareVersion() & 0x0F)) << bar;
					}
					else stringStream << std::setfill(' ') << std::setw(firmwareWidth) << (BaseLib::HelperFunctions::getHexString(i->second->getFirmwareVersion() >> 4) + "." + BaseLib::HelperFunctions::getHexString(i->second->getFirmwareVersion() & 0x0F)) << bar;
					if(i->second->serviceMessages)
					{
						std::string unreachable(i->second->serviceMessages->getUnreach() ? "Yes" : "No");
						stringStream << std::setw(unreachWidth) << unreachable;
					}
					stringStream << std::endl << std::dec;
				}
				_peersMutex.unlock();
				stringStream << "─────────┴─────────┴───────────────┴──────┴───────────────────────────┴──────────┴────────" << std::endl;

				return stringStream.str();
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
		else if(command.compare(0, 12, "peers select") == 0)
		{
			uint64_t id;

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
					id = BaseLib::HelperFunctions::getNumber(element, false);
					if(id == 0) return "Invalid id.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command selects a peer." << std::endl;
				stringStream << "Usage: peers select PEERID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to select. Example: 513" << std::endl;
				return stringStream.str();
			}

			_currentPeer = getPeer(id);
			if(!_currentPeer) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				stringStream << "Peer with id " << std::hex << std::to_string(id) << " and device type 0x" << (int32_t)_currentPeer->getDeviceType().type() << " selected." << std::dec << std::endl;
				stringStream << "For information about the peer's commands type: \"help\"" << std::endl;
			}
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

void MAXCentral::enqueuePendingQueues(int32_t deviceAddress)
{
	try
	{
		std::shared_ptr<MAXPeer> peer = getPeer(deviceAddress);
		if(!peer || !peer->pendingQueues) return;
		std::shared_ptr<PacketQueue> queue = _queueManager.get(deviceAddress);
		if(!queue) queue = _queueManager.createQueue(this, peer->getPhysicalInterface(), PacketQueueType::DEFAULT, deviceAddress);
		if(!queue) return;
		if(!queue->peer) queue->peer = peer;
		if(queue->pendingQueuesEmpty()) queue->push(peer->pendingQueues);
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

std::shared_ptr<MAXPeer> MAXCentral::createPeer(int32_t address, int32_t firmwareVersion, BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber, bool save)
{
	try
	{
		std::shared_ptr<MAXPeer> peer(new MAXPeer(_deviceID, true, this));
		peer->setAddress(address);
		peer->setFirmwareVersion(firmwareVersion);
		peer->setDeviceType(deviceType);
		peer->setSerialNumber(serialNumber);
		peer->rpcDevice = GD::rpcDevices.find(deviceType, firmwareVersion, -1);
		if(!peer->rpcDevice) return std::shared_ptr<MAXPeer>();
		if(save) peer->save(true, true, false); //Save and create peerID
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
    return std::shared_ptr<MAXPeer>();
}

bool MAXCentral::knowsDevice(std::string serialNumber)
{
	if(serialNumber == _serialNumber) return true;
	_peersMutex.lock();
	try
	{
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end())
		{
			_peersMutex.unlock();
			return true;
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
	return false;
}

bool MAXCentral::knowsDevice(uint64_t id)
{
	_peersMutex.lock();
	try
	{
		if(_peersByID.find(id) != _peersByID.end())
		{
			_peersMutex.unlock();
			return true;
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
	return false;
}

//Packet handlers
void MAXCentral::handleAck(int32_t messageCounter, std::shared_ptr<MAXPacket> packet)
{
	try
	{
		std::shared_ptr<PacketQueue> queue = _queueManager.get(packet->senderAddress());
		if(!queue) return;
		std::shared_ptr<MAXPacket> sentPacket(_sentPackets.get(packet->senderAddress()));
		if(packet->payload()->size() > 1 && (packet->payload()->at(1) & 0x80))
		{
			if(_bl->debugLevel >= 2)
			{
				if(sentPacket) GD::out.printError("Error: NACK received from 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress(), 6) + " in response to " + sentPacket->hexString() + ".");
				else GD::out.printError("Error: NACK received from 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress(), 6));
				if(queue->getQueueType() == PacketQueueType::PAIRING) GD::out.printError("Try resetting the device to factory defaults before pairing it to this central.");
			}
			if(queue->getQueueType() == PacketQueueType::PAIRING) queue->clear(); //Abort
			else queue->pop(); //Otherwise the queue might persist forever. NACKS shouldn't be received when not pairing
			return;
		}
		if(queue->getQueueType() == PacketQueueType::PAIRING)
		{
			if(sentPacket && sentPacket->messageType() == 0xF1 && sentPacket->messageSubtype() == 0)
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
						queue->peer->initializeCentralConfig();
						_peersMutex.lock();
						_peersByID[queue->peer->getID()] = queue->peer;
						_peersMutex.unlock();
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
					std::shared_ptr<BaseLib::RPC::RPCVariable> deviceDescriptions(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
					deviceDescriptions->arrayValue = queue->peer->getDeviceDescriptions(true, std::map<std::string, bool>());
					raiseRPCNewDevices(deviceDescriptions);
					GD::out.printMessage("Added peer 0x" + BaseLib::HelperFunctions::getHexString(queue->peer->getAddress()) + ".");
				}
			}
		}
		else if(queue->getQueueType() == PacketQueueType::UNPAIRING)
		{
			if(sentPacket && sentPacket->messageType() == 0xF0 && sentPacket->messageSubtype() == 0)
			{
				std::shared_ptr<MAXPeer> peer = getPeer(packet->senderAddress());
				if(peer) deletePeer(peer->getID());
			}
		}
		queue->pop(); //Messages are not popped by default.
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

void MAXCentral::handlePairingRequest(int32_t messageCounter, std::shared_ptr<MAXPacket> packet)
{
	try
	{
		if(packet->destinationAddress() != 0 && packet->destinationAddress() != _address)
		{
			GD::out.printError("Error: Pairing packet rejected, because this peer is already paired to another central.");
			return;
		}
		if(packet->payload()->size() < 14)
		{
			GD::out.printError("Error: Pairing packet is too small (payload size has to be at least 14).");
			return;
		}

		std::string serialNumber(&packet->payload()->at(4), &packet->payload()->at(4) + 10);
		uint32_t rawType = packet->payload()->at(2);
		LogicalDeviceType deviceType(BaseLib::Systems::DeviceFamilies::MAX, rawType);

		std::shared_ptr<MAXPeer> peer(getPeer(packet->senderAddress()));
		if(peer && (peer->getSerialNumber() != serialNumber || peer->getDeviceType() != deviceType))
		{
			GD::out.printError("Error: Pairing packet rejected, because a peer with the same address but different serial number or device type is already paired to this central.");
			return;
		}

		std::vector<uint8_t> payload;

		std::shared_ptr<PacketQueue> queue;
		if(_pairing)
		{
			queue = _queueManager.createQueue(this, getPhysicalInterface(packet->senderAddress()), PacketQueueType::PAIRING, packet->senderAddress());

			if(!peer)
			{
				int32_t firmwareVersion = (packet->payload()->at(0) << 8) + packet->payload()->at(1);
				//Do not save here
				queue->peer = createPeer(packet->senderAddress(), firmwareVersion, deviceType, serialNumber, false);
				if(!queue->peer)
				{
					GD::out.printWarning("Warning: Device type " + GD::bl->hf.getHexString(deviceType.type(), 4) + " with firmware version 0x" + BaseLib::HelperFunctions::getHexString(firmwareVersion, 4) + " not supported. Sender address 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress(), 6) + ".");
					return;
				}
				peer = queue->peer;
			}

			if(!peer->rpcDevice)
			{
				GD::out.printWarning("Warning: Device type not supported. Sender address 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress(), 6) + ".");
				return;
			}

			//INCLUSION
			payload.push_back(0);
			payload.push_back(0);
			std::shared_ptr<MAXPacket> configPacket(new MAXPacket(_messageCounter[0], 0x01, 0, _address, packet->senderAddress(), payload, peer->getRXModes() & Device::RXModes::burst));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x02, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();
			_messageCounter[0]++;

			//WAKEUP
			payload.clear();
			payload.push_back(0);
			payload.push_back(0x3F);
			configPacket = std::shared_ptr<MAXPacket>(new MAXPacket(_messageCounter[0], 0xF1, 0, _address, packet->senderAddress(), payload, false));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x02, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();
			_messageCounter[0]++;

			if(peer->rpcDevice->needsTime)
			{
				//TIME
				const auto timePoint = std::chrono::system_clock::now();
				time_t t = std::chrono::system_clock::to_time_t(timePoint);
				tm* localTime = std::localtime(&t);
				t = std::chrono::system_clock::to_time_t(timePoint - std::chrono::seconds(localTime->tm_gmtoff));
				localTime = std::localtime(&t);

				payload.clear();
				payload.push_back(0);
				payload.push_back(localTime->tm_year % 100);
				int32_t gmtOff = localTime->tm_gmtoff / 1800;
				payload.push_back(localTime->tm_mday + ((gmtOff & 0x38) << 2));
				payload.push_back(localTime->tm_hour + ((gmtOff & 7) << 5));
				payload.push_back(localTime->tm_min + (((localTime->tm_mon + 1) & 0x0C) << 4));
				payload.push_back(localTime->tm_min + (((localTime->tm_mon + 1) & 3) << 6));

				configPacket = std::shared_ptr<MAXPacket>(new MAXPacket(_messageCounter[0], 0x03, 0, _address, packet->senderAddress(), payload, false));
				queue->push(configPacket);
				queue->push(_messages->find(DIRECTIONIN, 0x02, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				payload.clear();
				_messageCounter[0]++;
			}
		}
		//not in pairing mode
		else queue = _queueManager.createQueue(this, peer->getPhysicalInterface(), PacketQueueType::DEFAULT, packet->senderAddress());

		if(!peer)
		{
			GD::out.printError("Error handling pairing packet: Peer is nullptr. This shouldn't have happened. Something went very wrong.");
			return;
		}

		if(peer->pendingQueues && !peer->pendingQueues->empty()) GD::out.printInfo("Info: Pushing pending queues.");
		queue->push(peer->pendingQueues); //This pushes the just generated queue and the already existent pending queue onto the queue
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
//End packet handlers

//RPC functions
std::shared_ptr<BaseLib::RPC::RPCVariable> MAXCentral::deleteDevice(std::string serialNumber, int32_t flags)
{
	try
	{
		if(serialNumber.empty()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		if(serialNumber[0] == '*') return BaseLib::RPC::RPCVariable::createError(-2, "Cannot delete virtual device.");
		std::shared_ptr<MAXPeer> peer = getPeer(serialNumber);
		if(!peer) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
		return deleteDevice(peer->getID(), flags);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> MAXCentral::deleteDevice(uint64_t peerID, int32_t flags)
{
	try
	{
		if(peerID == 0) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		if(peerID & 0x80000000) return BaseLib::RPC::RPCVariable::createError(-2, "Cannot delete virtual device.");
		std::shared_ptr<MAXPeer> peer = getPeer(peerID);
		if(!peer) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
		uint64_t id = peer->getID();

		bool defer = flags & 0x04;
		bool force = flags & 0x02;
		std::thread t(&MAXCentral::reset, this, id, defer);
		t.detach();
		//Force delete
		if(force) deletePeer(peer->getID());
		else
		{
			int32_t waitIndex = 0;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			while(_queueManager.get(peer->getAddress()) && peerExists(id) && waitIndex < 20)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				waitIndex++;
			}
		}

		if(!defer && !force && peerExists(id)) return BaseLib::RPC::RPCVariable::createError(-1, "No answer from device.");

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> MAXCentral::getDeviceInfo(uint64_t id, std::map<std::string, bool> fields)
{
	try
	{
		if(id > 0)
		{
			std::shared_ptr<MAXPeer> peer(getPeer(id));
			if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");

			return peer->getDeviceInfo(fields);
		}
		else
		{
			std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));

			std::vector<std::shared_ptr<MAXPeer>> peers;
			//Copy all peers first, because listDevices takes very long and we don't want to lock _peersMutex too long
			_peersMutex.lock();
			for(std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersByID.begin(); i != _peersByID.end(); ++i)
			{
				peers.push_back(std::dynamic_pointer_cast<MAXPeer>(i->second));
			}
			_peersMutex.unlock();

			for(std::vector<std::shared_ptr<MAXPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				//listDevices really needs a lot of resources, so wait a little bit after each device
				std::this_thread::sleep_for(std::chrono::milliseconds(3));
				std::shared_ptr<BaseLib::RPC::RPCVariable> info = (*i)->getDeviceInfo(fields);
				if(!info) continue;
				array->arrayValue->push_back(info);
			}

			return array;
		}
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        _peersMutex.unlock();
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        _peersMutex.unlock();
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
        _peersMutex.unlock();
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> MAXCentral::getInstallMode()
{
	try
	{
		return std::shared_ptr<RPCVariable>(new BaseLib::RPC::RPCVariable(_timeLeftInPairingMode));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> MAXCentral::putParamset(std::string serialNumber, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> paramset)
{
	try
	{
		std::shared_ptr<MAXPeer> peer(getPeer(serialNumber));
		if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		uint64_t remoteID = 0;
		if(!remoteSerialNumber.empty())
		{
			std::shared_ptr<MAXPeer> remotePeer(getPeer(remoteSerialNumber));
			if(!remotePeer)
			{
				if(remoteSerialNumber != _serialNumber) return BaseLib::RPC::RPCVariable::createError(-3, "Remote peer is unknown.");
			}
			else remoteID = remotePeer->getID();
		}
		std::shared_ptr<BaseLib::RPC::RPCVariable> result = peer->putParamset(channel, type, remoteID, remoteChannel, paramset);
		if(result->errorStruct) return result;
		int32_t waitIndex = 0;
		while(_queueManager.get(peer->getAddress()) && waitIndex < 40)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			waitIndex++;
		}
		return result;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> MAXCentral::putParamset(uint64_t peerID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> paramset)
{
	try
	{
		std::shared_ptr<MAXPeer> peer(getPeer(peerID));
		if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		std::shared_ptr<BaseLib::RPC::RPCVariable> result = peer->putParamset(channel, type, remoteID, remoteChannel, paramset);
		if(result->errorStruct) return result;
		int32_t waitIndex = 0;
		while(_queueManager.get(peer->getAddress()) && waitIndex < 40)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			waitIndex++;
		}
		return result;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

void MAXCentral::pairingModeTimer(int32_t duration, bool debugOutput)
{
	try
	{
		_pairing = true;
		if(debugOutput) GD::out.printInfo("Info: Pairing mode enabled.");
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
		if(debugOutput) GD::out.printInfo("Info: Pairing mode disabled.");
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

std::shared_ptr<RPCVariable> MAXCentral::setInstallMode(bool on, uint32_t duration, bool debugOutput)
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
			_pairingModeThread = std::thread(&MAXCentral::pairingModeTimer, this, duration, debugOutput);
		}
		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> MAXCentral::setInterface(uint64_t peerID, std::string interfaceID)
{
	try
	{
		std::shared_ptr<MAXPeer> peer(getPeer(peerID));
		if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		return peer->setInterface(interfaceID);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}
//End RPC functions
}
