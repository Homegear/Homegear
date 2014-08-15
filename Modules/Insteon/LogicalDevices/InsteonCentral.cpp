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

#include "InsteonCentral.h"
#include "../GD.h"

namespace Insteon {

InsteonCentral::InsteonCentral(IDeviceEventSink* eventHandler) : InsteonDevice(eventHandler), BaseLib::Systems::Central(GD::bl, this)
{
	init();
}

InsteonCentral::InsteonCentral(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : InsteonDevice(deviceID, serialNumber, address, eventHandler), Central(GD::bl, this)
{
	init();
}

InsteonCentral::~InsteonCentral()
{
	dispose();
	_pairingModeThreadMutex.lock();
	if(_pairingModeThread.joinable())
	{
		_stopPairingModeThread = true;
		_pairingModeThread.join();
	}
	_pairingModeThreadMutex.unlock();
}

void InsteonCentral::init()
{
	try
	{
		InsteonDevice::init();

		_deviceType = (uint32_t)DeviceType::INSTEONCENTRAL;

		for(std::map<std::string, std::shared_ptr<IInsteonInterface>>::iterator i = GD::physicalInterfaces.begin(); i != GD::physicalInterfaces.end(); ++i)
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

void InsteonCentral::setUpInsteonMessages()
{
	try
	{
		//Don't call InsteonDevice::setUpInsteonMessages!
		_messages->add(std::shared_ptr<InsteonMessage>(new InsteonMessage(0x01, 0x00, InsteonPacketFlags::Broadcast, this, ACCESSPAIREDTOSENDER, FULLACCESS, &InsteonDevice::handlePairingRequest)));

		_messages->add(std::shared_ptr<InsteonMessage>(new InsteonMessage(0x09, 0x01, InsteonPacketFlags::DirectAck, this, ACCESSPAIREDTOSENDER, FULLACCESS, &InsteonDevice::handleLinkingModeResponse)));

		_messages->add(std::shared_ptr<InsteonMessage>(new InsteonMessage(0x2F, -1, InsteonPacketFlags::Direct, this, ACCESSPAIREDTOSENDER, FULLACCESS, &InsteonDevice::handleDatabaseOpResponse)));

		_messages->add(std::shared_ptr<InsteonMessage>(new InsteonMessage(0x2F, -1, InsteonPacketFlags::DirectAck, this, ACCESSPAIREDTOSENDER, FULLACCESS, &InsteonDevice::handleDatabaseOpResponse)));
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

void InsteonCentral::worker()
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
				std::shared_ptr<InsteonPeer> peer(getPeer(lastPeer));
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

bool InsteonCentral::onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(_disposing) return false;
		std::shared_ptr<InsteonPacket> InsteonPacket(std::dynamic_pointer_cast<InsteonPacket>(packet));
		if(!InsteonPacket) return false;
		if(InsteonPacket->senderAddress() == _address) //Packet spoofed
		{
			std::shared_ptr<InsteonPeer> peer(getPeer(InsteonPacket->destinationAddress()));
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
		std::shared_ptr<IPhysicalInterface> physicalInterface = getPhysicalInterface(InsteonPacket->senderAddress());
		if(physicalInterface->getID() != senderID) return true;
		bool handled = InsteonDevice::onPacketReceived(senderID, InsteonPacket);
		std::shared_ptr<InsteonPeer> peer(getPeer(InsteonPacket->senderAddress()));
		if(!peer) return false;
		std::shared_ptr<InsteonPeer> team;
		if(handled)
		{
			//This block is not necessary for teams as teams will never have queues.
			std::shared_ptr<PacketQueue> queue = _queueManager.get(InsteonPacket->senderAddress());
			if(queue && queue->getQueueType() != PacketQueueType::PEER)
			{
				peer->setLastPacketReceived();
				peer->serviceMessages->endUnreach();
				return true; //Packet is handled by queue. Don't check if queue is empty!
			}
		}
		peer->packetReceived(InsteonPacket);
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

void InsteonCentral::unpair(uint64_t id)
{
	try
	{
		std::shared_ptr<InsteonPeer> peer(getPeer(id));
		if(!peer) return;

		while(!peer->pendingQueues->empty()) peer->pendingQueues->pop();
		peer->serviceMessages->setConfigPending(true);

		std::shared_ptr<PacketQueue> queue = _queueManager.createQueue(this, getPhysicalInterface(peer->getAddress()), PacketQueueType::UNPAIRING, peer->getAddress());
		queue->peer = peer;

		peer->getPhysicalInterface()->removePeer(peer->getAddress());

		std::vector<uint8_t> payload;
		payload.push_back(0); //Write
		payload.push_back(2); //Write
		payload.push_back(0x0F); //First byte of link database address
		payload.push_back(0xF7); //Second byte of link database address
		payload.push_back(0x08); //Entry size?
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0xC1);
		std::shared_ptr<InsteonPacket> configPacket(new InsteonPacket(0x2F, 0, peer->getAddress(), 3, 3, InsteonPacketFlags::Direct, true, payload));
		queue->push(configPacket);
		queue->push(_messages->find(DIRECTIONIN, 0x2F, 0, InsteonPacketFlags::DirectAck, std::vector<std::pair<uint32_t, int32_t>>()));
		payload.clear();

		payload.push_back(0); //Write
		payload.push_back(2); //Write
		payload.push_back(0x0F); //First byte of link database address
		payload.push_back(0xFF); //Second byte of link database address
		payload.push_back(0x08); //Entry size?
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0xB9);
		configPacket = std::shared_ptr<InsteonPacket>(new InsteonPacket(0x2F, 0, peer->getAddress(), 3, 3, InsteonPacketFlags::Direct, true, payload));
		queue->push(configPacket);
		queue->push(_messages->find(DIRECTIONIN, 0x2F, 0, InsteonPacketFlags::DirectAck, std::vector<std::pair<uint32_t, int32_t>>()));
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

void InsteonCentral::deletePeer(uint64_t id)
{
	try
	{
		std::shared_ptr<InsteonPeer> peer(getPeer(id));
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

std::string InsteonCentral::handleCLICommand(std::string command)
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
				unpair(peerID);
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

void InsteonCentral::enqueuePendingQueues(int32_t deviceAddress)
{
	try
	{
		std::shared_ptr<InsteonPeer> peer = getPeer(deviceAddress);
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

std::shared_ptr<InsteonPeer> InsteonCentral::createPeer(int32_t address, int32_t firmwareVersion, BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber, bool save)
{
	try
	{
		std::shared_ptr<InsteonPeer> peer(new InsteonPeer(_deviceID, true, this));
		peer->setAddress(address);
		peer->setFirmwareVersion(firmwareVersion);
		peer->setDeviceType(deviceType);
		peer->setSerialNumber(serialNumber);
		peer->rpcDevice = GD::rpcDevices.find(deviceType, firmwareVersion, -1);
		if(!peer->rpcDevice) return std::shared_ptr<InsteonPeer>();
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
    return std::shared_ptr<InsteonPeer>();
}

bool InsteonCentral::knowsDevice(std::string serialNumber)
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

bool InsteonCentral::knowsDevice(uint64_t id)
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

void InsteonCentral::addHomegearFeatures(std::shared_ptr<InsteonPeer> peer)
{
	try
	{
		if(!peer) return;
		//if(peer->getDeviceType().type() == (uint32_t)DeviceType::INTEONBLA) addHomegearFeaturesValveDrive(peer);
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

//Packet handlers
void InsteonCentral::handleNak(std::shared_ptr<InsteonPacket> packet)
{
	try
	{
		std::shared_ptr<PacketQueue> queue = _queueManager.get(packet->senderAddress());
		if(!queue) return;
		std::shared_ptr<InsteonPacket> sentPacket(_sentPackets.get(packet->senderAddress()));
		if(queue->getQueueType() == PacketQueueType::PAIRING)
		{
			//Handle Nak in response to first pairing queue packet
			if(_bl->debugLevel >= 5)
			{
				if(sentPacket) GD::out.printDebug("Debug: NACK received from 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress(), 6) + " in response to " + sentPacket->hexString() + ".");
				else GD::out.printDebug("Debug: NACK received from 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress(), 6));
			}

			setInstallMode(true, 60, false);
			if(!queue->isEmpty() && queue->front()->getType() == QueueEntryType::PACKET) queue->pop(); //Pop sent packet
			queue->pop();
		}
		else if(queue->getQueueType() == PacketQueueType::UNPAIRING)
		{
			if(!queue->isEmpty() && queue->front()->getType() == QueueEntryType::PACKET) queue->pop(); //Pop sent packet
			queue->pop();
			if(queue->isEmpty())
			{
				//Just remove the peer. Probably the link database packet was sent twice, that's why we receive a NAK.
				std::shared_ptr<InsteonPeer> peer = getPeer(packet->senderAddress());
				if(peer) deletePeer(peer->getID());
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

void InsteonCentral::handleDatabaseOpResponse(std::shared_ptr<InsteonPacket> packet)
{
	try
	{
		std::shared_ptr<PacketQueue> queue = _queueManager.get(packet->senderAddress());
		if(!queue) return;
		std::shared_ptr<InsteonPacket> sentPacket(_sentPackets.get(packet->senderAddress()));

		if(queue->getQueueType() == PacketQueueType::PAIRING && sentPacket && sentPacket->payload()->size() > 5 && sentPacket->payload()->at(5) == 0xE2)
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
				queue->peer->getPhysicalInterface()->addPeer(queue->peer->getAddress());
				std::shared_ptr<BaseLib::RPC::RPCVariable> deviceDescriptions(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
				deviceDescriptions->arrayValue = queue->peer->getDeviceDescriptions(true, std::map<std::string, bool>());
				raiseRPCNewDevices(deviceDescriptions);
				GD::out.printMessage("Added peer 0x" + BaseLib::HelperFunctions::getHexString(queue->peer->getAddress()) + ".");
				addHomegearFeatures(queue->peer);
			}
		}
		else if(queue->getQueueType() == PacketQueueType::UNPAIRING && sentPacket && sentPacket->payload()->size() > 3 && sentPacket->payload()->at(3) == 0xFF)
		{
			std::shared_ptr<InsteonPeer> peer = getPeer(packet->senderAddress());
			if(peer) deletePeer(peer->getID());
		}

		queue->pop(true); //Messages are not popped by default.

		if(queue->getQueueType() == PacketQueueType::PAIRING && !queue->isEmpty() && queue->front()->getType() == QueueEntryType::PACKET && queue->front()->getPacket()->messageType() == 0x09)
		{
			//Remove packets needed for a NAK as first response
			while(!queue->isEmpty())
			{
				if(queue->front()->getType() == QueueEntryType::PACKET && queue->front()->getPacket()->payload()->size() == 14 && queue->front()->getPacket()->payload()->at(1) == 2 && queue->front()->getPacket()->messageType() == 0x2F)
				{
					queue->processCurrentQueueEntry(); //Necessary for the packet to be sent after silent popping
					break;
				}

				queue->pop(true);
				continue;
			}
			return;
		}
		else queue->processCurrentQueueEntry(); //We popped silently before the "if"
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

void InsteonCentral::handleLinkingModeResponse(std::shared_ptr<InsteonPacket> packet)
{
	try
	{
		std::shared_ptr<PacketQueue> queue = _queueManager.get(packet->senderAddress());
		if(queue && queue->getQueueType() == PacketQueueType::PAIRING) queue->pop(); //Messages are not popped by default.
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

void InsteonCentral::handlePairingRequest(std::shared_ptr<InsteonPacket> packet)
{
	try
	{
		uint32_t rawType = packet->destinationAddress() >> 8;
		LogicalDeviceType deviceType(BaseLib::Systems::DeviceFamilies::Insteon, rawType);

		std::shared_ptr<InsteonPeer> peer(getPeer(packet->senderAddress()));
		if(peer && peer->getDeviceType() != deviceType)
		{
			GD::out.printError("Error: Pairing packet rejected, because a peer with the same address but different device type is already paired to this central.");
			return;
		}

		if(_pairing)
		{
			std::shared_ptr<PacketQueue> queue = _queueManager.get(packet->senderAddress());
			if(queue)
			{
				if(queue->getQueueType() == PacketQueueType::PAIRING) queue->pop();
				return;
			}

			_stopPairingModeThread = true;
			queue = _queueManager.createQueue(this, getPhysicalInterface(packet->senderAddress()), PacketQueueType::PAIRING, packet->senderAddress());

			if(!peer)
			{
				int32_t firmwareVersion = packet->destinationAddress() & 0xFF;
				std::string serialNumber = BaseLib::HelperFunctions::getHexString(packet->senderAddress(), 6);
				//Do not save here
				queue->peer = createPeer(packet->senderAddress(), firmwareVersion, deviceType, serialNumber, false);
				if(!queue->peer)
				{
					GD::out.printWarning("Warning: Device type 0x" + GD::bl->hf.getHexString(deviceType.type(), 4) + " with firmware version 0x" + BaseLib::HelperFunctions::getHexString(firmwareVersion, 4) + " not supported. Sender address 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress(), 6) + ".");
					return;
				}
				peer = queue->peer;
			}

			if(!peer->rpcDevice)
			{
				GD::out.printWarning("Warning: Device type not supported. Sender address 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress(), 6) + ".");
				return;
			}

			std::vector<uint8_t> payload;
			payload.push_back(1); //Write
			payload.push_back(0); //Write
			payload.push_back(0x0F); //First byte of link database address
			payload.push_back(0xFF); //Second byte of link database address
			payload.push_back(0x01); //Entry size?
			std::shared_ptr<InsteonPacket> configPacket(new InsteonPacket(0x2F, 0, packet->senderAddress(), 3, 3, InsteonPacketFlags::DirectAck, true, payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x2F, -1, InsteonPacketFlags::Direct, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();

			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0xF6);
			configPacket = std::shared_ptr<InsteonPacket>(new InsteonPacket(0x09, 0x01, packet->senderAddress(), 3, 3, InsteonPacketFlags::DirectAck, true, payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x09, 0x01, InsteonPacketFlags::DirectAck, std::vector<std::pair<uint32_t, int32_t>>()));
			queue->push(_messages->find(DIRECTIONIN, 0x01, 0x00, InsteonPacketFlags::Broadcast, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();

			payload.push_back(1); //Write
			payload.push_back(0); //Write
			payload.push_back(0x0F); //First byte of link database address
			payload.push_back(0xFF); //Second byte of link database address
			payload.push_back(0x01); //Entry size?
			configPacket = std::shared_ptr<InsteonPacket>(new InsteonPacket(0x2F, 0, packet->senderAddress(), 3, 3, InsteonPacketFlags::DirectAck, true, payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x2F, -1, InsteonPacketFlags::Direct, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();

			payload.push_back(0); //Write
			payload.push_back(2); //Write
			payload.push_back(0x0F); //First byte of link database address
			payload.push_back(0xFF); //Second byte of link database address
			payload.push_back(0x08); //Entry size?
			payload.push_back(0xA2); //Bit 7 => Record in use, Bit 6 => Controller, Bit 5 => ACK required, Bit 4, 3, 2 => reserved, Bit 1 => Record has been used before, Bit 0 => unused
			payload.push_back(0x00); //Group
			payload.push_back(GD::defaultPhysicalInterface->address() >> 16);
			payload.push_back((GD::defaultPhysicalInterface->address() >> 8) & 0xFF);
			payload.push_back(GD::defaultPhysicalInterface->address() & 0xFF);
			payload.push_back(0xFF); //Data1 => Level in this case
			payload.push_back(0x1F); //Data2 => Ramp rate?
			payload.push_back(0x01); //Data3
			payload.push_back(0x9F); //?
			configPacket = std::shared_ptr<InsteonPacket>(new InsteonPacket(0x2F, 0, packet->senderAddress(), 3, 3, InsteonPacketFlags::Direct, true, payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x2F, 0, InsteonPacketFlags::DirectAck, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();

			payload.push_back(1); //Write
			payload.push_back(0); //Write
			payload.push_back(0x0F); //First byte of link database address
			payload.push_back(0xFF); //Second byte of link database address
			payload.push_back(0x01); //Entry size?
			configPacket = std::shared_ptr<InsteonPacket>(new InsteonPacket(0x2F, 0, packet->senderAddress(), 3, 3, InsteonPacketFlags::DirectAck, true, payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x2F, 0, InsteonPacketFlags::Direct, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();

			payload.push_back(1); //Write
			payload.push_back(0); //Write
			payload.push_back(0x0F); //First byte of link database address
			payload.push_back(0xF7); //Second byte of link database address
			payload.push_back(0x01); //Entry size?
			configPacket = std::shared_ptr<InsteonPacket>(new InsteonPacket(0x2F, 0, packet->senderAddress(), 3, 3, InsteonPacketFlags::DirectAck, true, payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x2F, 0, InsteonPacketFlags::Direct, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();

			payload.push_back(0); //Write
			payload.push_back(2); //Write
			payload.push_back(0x0F); //First byte of link database address
			payload.push_back(0xF7); //Second byte of link database address
			payload.push_back(0x08); //Entry size?
			payload.push_back(0xE2); //Bit 7 => Record in use, Bit 6 => Controller, Bit 5 => ACK required, Bit 4, 3, 2 => reserved, Bit 1 => Record has been used before, Bit 0 => unused
			payload.push_back(0x01); //Group
			payload.push_back(GD::defaultPhysicalInterface->address() >> 16);
			payload.push_back((GD::defaultPhysicalInterface->address() >> 8) & 0xFF);
			payload.push_back(GD::defaultPhysicalInterface->address() & 0xFF);
			payload.push_back(0x03); //Data1
			payload.push_back(0x1F); //Data2
			payload.push_back(0x01); //Data3
			payload.push_back(0x62); //?
			configPacket = std::shared_ptr<InsteonPacket>(new InsteonPacket(0x2F, 0, packet->senderAddress(), 3, 3, InsteonPacketFlags::Direct, true, payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x2F, 0, InsteonPacketFlags::DirectAck, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();

			payload.push_back(1); //Write
			payload.push_back(0); //Write
			payload.push_back(0x0F); //First byte of link database address
			payload.push_back(0xF7); //Second byte of link database address
			payload.push_back(0x01); //Entry size?
			configPacket = std::shared_ptr<InsteonPacket>(new InsteonPacket(0x2F, 0, packet->senderAddress(), 3, 3, InsteonPacketFlags::DirectAck, true, payload));
			queue->push(configPacket);
			queue->push(_messages->find(DIRECTIONIN, 0x2F, 0, InsteonPacketFlags::Direct, std::vector<std::pair<uint32_t, int32_t>>()));
			payload.clear();
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
//End packet handlers

//RPC functions
std::shared_ptr<BaseLib::RPC::RPCVariable> InsteonCentral::deleteDevice(std::string serialNumber, int32_t flags)
{
	try
	{
		if(serialNumber.empty()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		if(serialNumber[0] == '*') return BaseLib::RPC::RPCVariable::createError(-2, "Cannot delete virtual device.");
		std::shared_ptr<InsteonPeer> peer = getPeer(serialNumber);
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

std::shared_ptr<BaseLib::RPC::RPCVariable> InsteonCentral::deleteDevice(uint64_t peerID, int32_t flags)
{
	try
	{
		if(peerID == 0) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		if(peerID & 0x80000000) return BaseLib::RPC::RPCVariable::createError(-2, "Cannot delete virtual device.");
		std::shared_ptr<InsteonPeer> peer = getPeer(peerID);
		if(!peer) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
		uint64_t id = peer->getID();

		bool defer = flags & 0x04;
		bool force = flags & 0x02;
		std::thread t(&InsteonCentral::unpair, this, id);
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

std::shared_ptr<BaseLib::RPC::RPCVariable> InsteonCentral::getDeviceInfo(uint64_t id, std::map<std::string, bool> fields)
{
	try
	{
		if(id > 0)
		{
			std::shared_ptr<InsteonPeer> peer(getPeer(id));
			if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");

			return peer->getDeviceInfo(fields);
		}
		else
		{
			std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));

			std::vector<std::shared_ptr<InsteonPeer>> peers;
			//Copy all peers first, because listDevices takes very long and we don't want to lock _peersMutex too long
			_peersMutex.lock();
			for(std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersByID.begin(); i != _peersByID.end(); ++i)
			{
				peers.push_back(std::dynamic_pointer_cast<InsteonPeer>(i->second));
			}
			_peersMutex.unlock();

			for(std::vector<std::shared_ptr<InsteonPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
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

std::shared_ptr<RPCVariable> InsteonCentral::getInstallMode()
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

std::shared_ptr<BaseLib::RPC::RPCVariable> InsteonCentral::putParamset(std::string serialNumber, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> paramset)
{
	try
	{
		std::shared_ptr<InsteonPeer> peer(getPeer(serialNumber));
		if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		uint64_t remoteID = 0;
		if(!remoteSerialNumber.empty())
		{
			std::shared_ptr<InsteonPeer> remotePeer(getPeer(remoteSerialNumber));
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

std::shared_ptr<BaseLib::RPC::RPCVariable> InsteonCentral::putParamset(uint64_t peerID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> paramset)
{
	try
	{
		std::shared_ptr<InsteonPeer> peer(getPeer(peerID));
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

void InsteonCentral::pairingModeTimer(int32_t duration, bool debugOutput)
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
		GD::defaultPhysicalInterface->disablePairingMode();
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

std::shared_ptr<RPCVariable> InsteonCentral::setInstallMode(bool on, uint32_t duration, bool debugOutput)
{
	try
	{
		_pairingModeThreadMutex.lock();
		_stopPairingModeThread = true;
		if(_pairingModeThread.joinable()) _pairingModeThread.join();
		_stopPairingModeThread = false;
		_timeLeftInPairingMode = 0;
		if(on && duration >= 5)
		{
			_timeLeftInPairingMode = duration; //It's important to set it here, because the thread often doesn't completely initialize before getInstallMode requests _timeLeftInPairingMode
			GD::defaultPhysicalInterface->enablePairingMode();
			_pairingModeThread = std::thread(&InsteonCentral::pairingModeTimer, this, duration, debugOutput);
		}
		_pairingModeThreadMutex.unlock();
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
    _pairingModeThreadMutex.unlock();
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> InsteonCentral::setInterface(uint64_t peerID, std::string interfaceID)
{
	try
	{
		std::shared_ptr<InsteonPeer> peer(getPeer(peerID));
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
