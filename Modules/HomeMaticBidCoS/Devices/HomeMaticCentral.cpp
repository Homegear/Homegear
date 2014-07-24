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

#include "HomeMaticCentral.h"
#include "../PendingBidCoSQueues.h"
#include "../../Base/BaseLib.h"
#include "../GD.h"

namespace BidCoS
{

HomeMaticCentral::HomeMaticCentral(IDeviceEventSink* eventHandler) : HomeMaticDevice(eventHandler), Central(GD::bl, this)
{
	init();
}

HomeMaticCentral::HomeMaticCentral(uint32_t deviceType, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : HomeMaticDevice(deviceType, serialNumber, address, eventHandler), Central(GD::bl, this)
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
		if(_updateFirmwareThread.joinable()) _updateFirmwareThread.join();
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

void HomeMaticCentral::init()
{
	try
	{
		HomeMaticDevice::init();

		_deviceType = (uint32_t)DeviceType::HMCENTRAL;

		for(std::map<std::string, std::shared_ptr<IBidCoSInterface>>::iterator i = GD::physicalInterfaces.begin(); i != GD::physicalInterfaces.end(); ++i)
		{
			i->second->addEventHandler((BaseLib::Systems::IPhysicalInterface::IPhysicalInterfaceEventSink*)this);
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

void HomeMaticCentral::worker()
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
				std::shared_ptr<BidCoSPeer> peer(getPeer(lastPeer));
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

bool HomeMaticCentral::onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(_disposing) return false;
		std::shared_ptr<BidCoSPacket> bidCoSPacket(std::dynamic_pointer_cast<BidCoSPacket>(packet));
		if(!bidCoSPacket) return false;
		if(bidCoSPacket->senderAddress() == _address) //Packet spoofed
		{
			std::shared_ptr<BidCoSPeer> peer(getPeer(bidCoSPacket->destinationAddress()));
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
		std::shared_ptr<IBidCoSInterface> physicalInterface = getPhysicalInterface(bidCoSPacket->senderAddress());
		if(physicalInterface->getID() != senderID) return true;
		bool handled = HomeMaticDevice::onPacketReceived(senderID, bidCoSPacket);
		std::shared_ptr<BidCoSPeer> peer(getPeer(bidCoSPacket->senderAddress()));
		if(!peer) return false;
		std::shared_ptr<BidCoSPeer> team;
		if(peer->hasTeam() && bidCoSPacket->senderAddress() == peer->getTeamRemoteAddress()) team = getPeer(peer->getTeamRemoteSerialNumber());
		if(handled)
		{
			//This block is not necessary for teams as teams will never have queues.
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(bidCoSPacket->senderAddress());
			if(queue && queue->getQueueType() != BidCoSQueueType::PEER)
			{
				peer->setLastPacketReceived();
				peer->serviceMessages->endUnreach();
				peer->setRSSIDevice(bidCoSPacket->rssiDevice());
				return true; //Packet is handled by queue. Don't check if queue is empty!
			}
		}
		if(team)
		{
			team->packetReceived(bidCoSPacket);
			for(std::vector<std::pair<std::string, uint32_t>>::const_iterator i = team->teamChannels.begin(); i != team->teamChannels.end(); ++i)
			{
				getPeer(i->first)->packetReceived(bidCoSPacket);
			}
		}
		else peer->packetReceived(bidCoSPacket);
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

void HomeMaticCentral::enqueuePendingQueues(int32_t deviceAddress)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer = getPeer(deviceAddress);
		if(!peer || !peer->pendingBidCoSQueues) return;
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(deviceAddress);
		if(!queue) queue = _bidCoSQueueManager.createQueue(this, peer->getPhysicalInterface(), BidCoSQueueType::DEFAULT, deviceAddress);
		if(!queue) return;
		if(!queue->peer) queue->peer = peer;
		if(queue->pendingQueuesEmpty())
		{
			if(peer->getRXModes() & BaseLib::RPC::Device::RXModes::burst) peer->pendingBidCoSQueues->setWakeOnRadioBit();
			queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::enqueuePackets(int32_t deviceAddress, std::shared_ptr<BidCoSQueue> packets, bool pushPendingBidCoSQueues)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(deviceAddress));
		if(!peer) return;
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, peer->getPhysicalInterface(), BidCoSQueueType::DEFAULT, deviceAddress);
		queue->push(packets, true, true);
		if(pushPendingBidCoSQueues)
		{
			queue->push(peer->pendingBidCoSQueues);
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

std::shared_ptr<BidCoSPeer> HomeMaticCentral::createPeer(int32_t address, int32_t firmwareVersion, BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet, bool save)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(new BidCoSPeer(_deviceID, true, this));
		peer->setAddress(address);
		peer->setFirmwareVersion(firmwareVersion);
		peer->setDeviceType(deviceType);
		peer->setSerialNumber(serialNumber);
		peer->setRemoteChannel(remoteChannel);
		peer->setMessageCounter(messageCounter);
		if(packet)
		{
			peer->rpcDevice = GD::rpcDevices.find(deviceType, firmwareVersion, packet);
			//if(!peer->rpcDevice) peer->rpcDevice = GD::rpcDevices.find(deviceType.family(), deviceType.name(), packet);
		}
		else peer->rpcDevice = GD::rpcDevices.find(deviceType, firmwareVersion, -1);
		if(!peer->rpcDevice) return std::shared_ptr<BidCoSPeer>();
		if(peer->rpcDevice->countFromSysinfoIndex > -1) peer->setCountFromSysinfo(peer->rpcDevice->getCountFromSysinfo());
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
    return std::shared_ptr<BidCoSPeer>();
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
			stringStream << "peers add\t\tManually adds a peer (without pairing it! Only for testing)" << std::endl;
			stringStream << "peers remove\t\tRemove a peer (without unpairing)" << std::endl;
			stringStream << "peers reset\t\tUnpair a peer and reset it to factory defaults" << std::endl;
			stringStream << "peers select\t\tSelect a peer" << std::endl;
			stringStream << "peers unpair\t\tUnpair a peer" << std::endl;
			stringStream << "peers update\t\tUpdates a peer to the newest firmware version" << std::endl;
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
		else if(command.compare(0, 9, "peers add") == 0)
		{
			uint32_t deviceType;
			int32_t peerAddress = 0;
			std::string serialNumber;
			int32_t firmwareVersion = 0;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			std::shared_ptr<BidCoSPacket> packet;
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
					if(element.size() == 54)
					{
						index = 6;
						packet.reset(new BidCoSPacket());
						packet->import(element, false);
						peerAddress = packet->senderAddress();
						deviceType = (packet->payload()->at(1) << 8) + packet->payload()->at(2);
						firmwareVersion = packet->payload()->at(0);
						serialNumber.insert(serialNumber.end(), &packet->payload()->at(3), &packet->payload()->at(3) + 10);
						break;
					}
					else
					{
						int32_t temp = BaseLib::HelperFunctions::getNumber(element, true);
						if(temp == 0) return "Invalid device type. Device type has to be provided in hexadecimal format.\n";
						deviceType = temp;
					}
				}
				else if(index == 3)
				{
					peerAddress = BaseLib::HelperFunctions::getNumber(element, true);
					if(peerAddress == 0 || peerAddress != (peerAddress & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				else if(index == 4)
				{
					if(element.length() != 10) return "Invalid serial number. Please provide a serial number with a length of 10 characters.\n";
					serialNumber = element;
				}
				else if(index == 5)
				{
					firmwareVersion = BaseLib::HelperFunctions::getNumber(element, true);
					if(firmwareVersion == 0) return "Invalid firmware version. The firmware version has to be passed in hexadecimal format.\n";
				}
				index++;
			}
			if(index < 6)
			{
				stringStream << "Description: This command manually adds a peer without pairing. Please only use this command for testing." << std::endl;
				stringStream << "Usage: peers add DEVICETYPE ADDRESS SERIALNUMBER FIRMWAREVERSION" << std::endl;
				stringStream << "       peers add PAIRINGPACKET" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  DEVICETYPE:\t\tThe 2 byte device type of the peer to add in hexadecimal format. Example: 0039" << std::endl;
				stringStream << "  ADDRESS:\t\tThe 3 byte address of the peer to add in hexadecimal format. Example: 1A03FC" << std::endl;
				stringStream << "  SERIALNUMBER:\t\tThe 10 character long serial number of the peer to add. Example: JEQ0123456" << std::endl;
				stringStream << "  FIRMWAREVERSION:\tThe 1 byte firmware version of the peer to add in hexadecimal format. Example: 1F" << std::endl;
				stringStream << "  PAIRINGPACKET:\tThe 27 byte hex of a pairing packet. Example: 1A0080001F454D1D01231900044B45513030323236393510010100" << std::endl;
				return stringStream.str();
			}
			if(peerExists(peerAddress)) stringStream << "This peer is already paired to this central." << std::endl;
			else
			{
				std::shared_ptr<BidCoSPeer> peer = createPeer(peerAddress, firmwareVersion, BaseLib::Systems::LogicalDeviceType(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS, deviceType), serialNumber, 0, 0, packet, false);
				if(!peer || !peer->rpcDevice) return "Device type not supported.\n";
				try
				{
					_peersMutex.lock();
					_peers[peer->getAddress()] = peer;
					if(!peer->getSerialNumber().empty()) _peersBySerial[peer->getSerialNumber()] = peer;
					_peersMutex.unlock();
					peer->save(true, true, false);
					peer->initializeCentralConfig();
					_peersMutex.lock();
					_peersByID[peer->getID()] = peer;
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

				stringStream << "Added peer " + std::to_string(peer->getID()) + " with address 0x" << std::hex << peerAddress << " of type 0x" << (int32_t)deviceType << " with serial number " << serialNumber << " and firmware version 0x" << firmwareVersion << "." << std::dec << std::endl;
			}
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
					peerID = BaseLib::HelperFunctions::getNumber(element);
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
				stringStream << "Unpairing peer " << peerID << std::endl;
				unpair(peerID, true);
			}
			return stringStream.str();
		}
		else if(command.compare(0, 11, "peers reset") == 0)
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
					peerID = BaseLib::HelperFunctions::getNumber(element);
					if(peerID == 0) return "Invalid id.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command unpairs a peer and resets it to factory defaults." << std::endl;
				stringStream << "Usage: peers reset PEERID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to reset. Example: 513" << std::endl;
				return stringStream.str();
			}

			if(!peerExists(peerID)) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				if(_currentPeer && _currentPeer->getID() == peerID) _currentPeer.reset();
				stringStream << "Resetting peer " << std::to_string(peerID) << std::endl;
				reset(peerID, true);
			}
			return stringStream.str();
		}
		else if(command.compare(0, 12, "peers update") == 0)
		{
			uint64_t peerID;
			bool all = false;
			bool manually = false;

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
					else if(element == "all") all = true;
					else
					{
						peerID = BaseLib::HelperFunctions::getNumber(element, false);
						if(peerID == 0) return "Invalid id.\n";
					}
				}
				else if(index == 3)
				{
					manually = BaseLib::HelperFunctions::getNumber(element, false);
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command updates one or all peers to the newest firmware version available in \"" << _bl->settings.firmwarePath() << "\"." << std::endl;
				stringStream << "Usage: peers update PEERID" << std::endl;
				stringStream << "       peers update PEERID MANUALLY" << std::endl;
				stringStream << "       peers update all" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to update. Example: 513" << std::endl;
				stringStream << "  MANUALLY:\tIf \"1\" you need to enable the update mode on the device manually within 50 seconds after executing this command." << std::endl;
				return stringStream.str();
			}

			std::shared_ptr<BaseLib::RPC::RPCVariable> result;
			std::vector<uint64_t> ids;
			if(all)
			{
				_peersMutex.lock();
				for(std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersByID.begin(); i != _peersByID.end(); ++i)
				{
					std::shared_ptr<BidCoSPeer> peer(std::dynamic_pointer_cast<BidCoSPeer>(i->second));
					if(peer->firmwareUpdateAvailable()) ids.push_back(i->first);
				}
				_peersMutex.unlock();
				if(ids.empty())
				{
					stringStream << "All peers are up to date." << std::endl;
					return stringStream.str();
				}
				result = updateFirmware(ids, false);
			}
			else if(!peerExists(peerID)) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				std::shared_ptr<BidCoSPeer> peer = getPeer(peerID);
				if(!peer->firmwareUpdateAvailable())
				{
					stringStream << "Peer is up to date." << std::endl;
					return stringStream.str();
				}
				ids.push_back(peerID);
				result = updateFirmware(ids, manually);
			}
			if(!result) stringStream << "Unknown error." << std::endl;
			else if(result->errorStruct) stringStream << result->structValue->at("faultString")->stringValue << std::endl;
			else stringStream << "Started firmware update(s)... This might take a long time. Use the RPC function \"getUpdateProgress\" or see the log for details." << std::endl;
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
					stringStream << "      FILTERVALUE: The 3 byte address of the peer to filter (e. g. 1DA44D)." << std::endl;
					stringStream << "  SERIAL: Filter by serial number." << std::endl;
					stringStream << "      FILTERVALUE: The serial number of the peer to filter (e. g. JEQ0554309)." << std::endl;
					stringStream << "  TYPE: Filter by device type." << std::endl;
					stringStream << "      FILTERVALUE: The 2 byte device type in hexadecimal format." << std::endl;
					stringStream << "  CONFIGPENDING: List peers with pending config." << std::endl;
					stringStream << "      FILTERVALUE: empty" << std::endl;
					stringStream << "  UNREACH: List all unreachable peers." << std::endl;
					stringStream << "      FILTERVALUE: empty" << std::endl;
					return stringStream.str();
				}

				if(_peers.empty())
				{
					stringStream << "No peers are paired to this central." << std::endl;
					return stringStream.str();
				}
				bool firmwareUpdates = false;
				std::string bar(" │ ");
				const int32_t idWidth = 11;
				const int32_t addressWidth = 8;
				const int32_t serialWidth = 13;
				const int32_t typeWidth1 = 4;
				const int32_t typeWidth2 = 25;
				const int32_t firmwareWidth = 8;
				const int32_t configPendingWidth = 14;
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
					<< std::setw(configPendingWidth) << "Config Pending" << bar
					<< std::setw(unreachWidth) << "Unreach"
					<< std::endl;
				stringStream << "────────────┼──────────┼───────────────┼──────┼───────────────────────────┼──────────┼────────────────┼────────" << std::endl;
				stringStream << std::setfill(' ')
					<< std::setw(idWidth) << " " << bar
					<< std::setw(addressWidth) << " " << bar
					<< std::setw(serialWidth) << " " << bar
					<< std::setw(typeWidth1) << " " << bar
					<< std::setw(typeWidth2) << " " << bar
					<< std::setw(firmwareWidth) << " " << bar
					<< std::setw(configPendingWidth) << " " << bar
					<< std::setw(unreachWidth) << " "
					<< std::endl;
				_peersMutex.lock();
				for(std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersByID.begin(); i != _peersByID.end(); ++i)
				{
					std::shared_ptr<BidCoSPeer> peer(std::dynamic_pointer_cast<BidCoSPeer>(i->second));
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
					else if(filterType == "configpending")
					{
						if(i->second->serviceMessages)
						{
							if(!i->second->serviceMessages->getConfigPending()) continue;
						}
					}
					else if(filterType == "unreach")
					{
						if(i->second->serviceMessages)
						{
							if(!i->second->serviceMessages->getUnreach()) continue;
						}
					}

					uint64_t currentID = i->second->getID();
					std::string idString = (currentID > 999999) ? "0x" + BaseLib::HelperFunctions::getHexString(currentID, 8) : std::to_string(currentID);
					stringStream
						<< std::setw(idWidth) << std::setfill(' ') << idString << bar
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
						firmwareUpdates = true;
					}
					else stringStream << std::setfill(' ') << std::setw(firmwareWidth) << (BaseLib::HelperFunctions::getHexString(i->second->getFirmwareVersion() >> 4) + "." + BaseLib::HelperFunctions::getHexString(i->second->getFirmwareVersion() & 0x0F)) << bar;
					if(i->second->serviceMessages)
					{
						std::string configPending(i->second->serviceMessages->getConfigPending() ? "Yes" : "No");
						std::string unreachable(i->second->serviceMessages->getUnreach() ? "Yes" : "No");
						stringStream << std::setfill(' ') << std::setw(configPendingWidth) << configPending << bar;
						stringStream << std::setfill(' ') << std::setw(unreachWidth) << unreachable;
					}
					stringStream << std::endl << std::dec;
				}
				_peersMutex.unlock();
				stringStream << "────────────┴──────────┴───────────────┴──────┴───────────────────────────┴──────────┴────────────────┴────────" << std::endl;
				if(firmwareUpdates) stringStream << std::endl << "*: Firmware update available." << std::endl;

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
				stringStream << "Description: This command selects a peer." << std::endl;
				stringStream << "Usage: peers select PEERID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to select. Example: 513" << std::endl;
				return stringStream.str();
			}

			_currentPeer = getPeer(peerID);
			if(!_currentPeer) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				stringStream << "Peer with id " << peerID << " and device type 0x" << _bl->hf.getHexString(_currentPeer->getDeviceType().type()) << " selected." << std::dec << std::endl;
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

void HomeMaticCentral::updateFirmwares(std::vector<uint64_t> ids, bool manual)
{
	try
	{
		if(_updateMode || _bl->deviceUpdateInfo.currentDevice > 0) return;
		_bl->deviceUpdateInfo.updateMutex.lock();
		_bl->deviceUpdateInfo.devicesToUpdate = ids.size();
		_bl->deviceUpdateInfo.currentUpdate = 0;
		for(std::vector<uint64_t>::iterator i = ids.begin(); i != ids.end(); ++i)
		{
			_bl->deviceUpdateInfo.currentDeviceProgress = 0;
			_bl->deviceUpdateInfo.currentUpdate++;
			_bl->deviceUpdateInfo.currentDevice = *i;
			updateFirmware(*i, manual);
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
	_bl->deviceUpdateInfo.reset();
	_bl->deviceUpdateInfo.updateMutex.unlock();
}

void HomeMaticCentral::updateFirmware(uint64_t id, bool manual)
{
	std::shared_ptr<IBidCoSInterface> physicalInterface;
	std::string oldPhysicalInterfaceID;
	std::shared_ptr<BidCoSPeer> peer;
	try
	{
		if(_updateMode) return;
		peer = getPeer(id);
		if(!peer) return;
		oldPhysicalInterfaceID = peer->getPhysicalInterfaceID();
		physicalInterface = peer->getPhysicalInterface();
		if(!physicalInterface->firmwareUpdatesSupported())
		{
			if(GD::defaultPhysicalInterface->firmwareUpdatesSupported())
			{
				GD::out.printInfo("Info: Using the default physical interface " + GD::defaultPhysicalInterface->getID() + " because the peer's interface doesn't support firmware updates.");
				physicalInterface = GD::defaultPhysicalInterface;
			}
			else
			{
				for(std::map<std::string, std::shared_ptr<IBidCoSInterface>>::iterator i = GD::physicalInterfaces.begin(); i != GD::physicalInterfaces.end(); ++i)
				{
					if(i->second->firmwareUpdatesSupported())
					{
						GD::out.printInfo("Info: Using physical interface " + i->second->getID() + " because the peer's interface doesn't support firmware updates.");
						physicalInterface = i->second;
						break;
					}
				}
			}
		}
		if(!physicalInterface->firmwareUpdatesSupported())
		{
			GD::out.printInfo("Info: Not updating peer with id " + std::to_string(id) + ". No physical interface supports firmware updates.");
			_bl->deviceUpdateInfo.results[id].first = 9;
			_bl->deviceUpdateInfo.results[id].second = "No physical interface supports firmware updates.";
			_updateMutex.unlock();
			_updateMode = false;
			return;
		}
		_updateMode = true;
		_updateMutex.lock();
		std::string filenamePrefix = BaseLib::HelperFunctions::getHexString((int32_t)BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS, 4) + "." + BaseLib::HelperFunctions::getHexString(peer->getDeviceType().type(), 8);
		std::string versionFile(_bl->settings.firmwarePath() + filenamePrefix + ".version");
		if(!BaseLib::HelperFunctions::fileExists(versionFile))
		{
			GD::out.printInfo("Info: Not updating peer with id " + std::to_string(id) + ". No version info file found.");
			_bl->deviceUpdateInfo.results[id].first = 2;
			_bl->deviceUpdateInfo.results[id].second = "No version file found.";
			_updateMutex.unlock();
			_updateMode = false;
			return;
		}
		std::string firmwareFile(_bl->settings.firmwarePath() + filenamePrefix + ".fw");
		if(!BaseLib::HelperFunctions::fileExists(firmwareFile))
		{
			GD::out.printInfo("Info: Not updating peer with id " + std::to_string(id) + ". No firmware file found.");
			_bl->deviceUpdateInfo.results[id].first = 3;
			_bl->deviceUpdateInfo.results[id].second = "No firmware file found.";
			_updateMutex.unlock();
			_updateMode = false;
			return;
		}
		int32_t firmwareVersion = peer->getNewFirmwareVersion();
		if(peer->getFirmwareVersion() >= firmwareVersion)
		{
			_bl->deviceUpdateInfo.results[id].first = 0;
			_bl->deviceUpdateInfo.results[id].second = "Already up to date.";
			GD::out.printInfo("Info: Not updating peer with id " + std::to_string(id) + ". Peer firmware is already up to date.");
			_updateMutex.unlock();
			_updateMode = false;
			return;
		}
		std::string oldVersionString = peer->getFirmwareVersionString(peer->getFirmwareVersion());
		std::string versionString = peer->getFirmwareVersionString(firmwareVersion);

		std::string firmwareHex;
		try
		{
			firmwareHex = BaseLib::HelperFunctions::getFileContent(firmwareFile);
		}
		catch(const std::exception& ex)
		{
			GD::out.printError("Error: Could not open firmware file: " + firmwareFile + ": " + ex.what());
			_bl->deviceUpdateInfo.results[id].first = 4;
			_bl->deviceUpdateInfo.results[id].second = "Could not open firmware file.";
			_updateMutex.unlock();
			_updateMode = false;
			return;
		}
		catch(...)
		{
			GD::out.printError("Error: Could not open firmware file: " + firmwareFile + ".");
			_bl->deviceUpdateInfo.results[id].first = 4;
			_bl->deviceUpdateInfo.results[id].second = "Could not open firmware file.";
			_updateMutex.unlock();
			_updateMode = false;
			return;
		}
		std::vector<uint8_t> firmware = _bl->hf.getUBinary(firmwareHex);
		GD::out.printDebug("Debug: Size of firmware is: " + std::to_string(firmware.size()) + " bytes.");
		if(firmware.size() < 4)
		{
			_bl->deviceUpdateInfo.results[id].first = 5;
			_bl->deviceUpdateInfo.results[id].second = "Firmware file has wrong format.";
			GD::out.printError("Error: Could not read firmware file: " + firmwareFile + ": Wrong format.");
			_updateMutex.unlock();
			_updateMode = false;
			return;
		}

		std::vector<uint8_t> currentBlock;
		std::vector<std::vector<uint8_t>> blocks;
		int32_t pos = 0;
		while(pos + 1 < firmware.size())
		{
			int32_t blockSize = (firmware.at(pos) << 8) + firmware.at(pos + 1);
			GD::out.printDebug("Debug: Current block size is: " + std::to_string(blockSize) + " bytes.");
			pos += 2;
			if(pos + blockSize > firmware.size() || blockSize > 1024)
			{
				_bl->deviceUpdateInfo.results[id].first = 5;
				_bl->deviceUpdateInfo.results[id].second = "Firmware file has wrong format.";
				GD::out.printError("Error: Could not read firmware file: " + firmwareFile + ": Wrong format.");
				_updateMutex.unlock();
				_updateMode = false;
				return;
			}
			currentBlock.clear();
			currentBlock.insert(currentBlock.begin(), firmware.begin() + pos, firmware.begin() + pos + blockSize);
			blocks.push_back(currentBlock);
			pos += blockSize;
		}

		int32_t waitIndex = 0;
		std::shared_ptr<BidCoSPacket> receivedPacket;
		if(peer->getPhysicalInterfaceID() != physicalInterface->getID()) peer->setPhysicalInterfaceID(physicalInterface->getID());

		if(!manual)
		{
			bool responseReceived = false;
			for(int32_t retries = 0; retries < 10; retries++)
			{
				std::vector<uint8_t> payload({0xCA});
				std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(_messageCounter[0]++, 0x30, 0x11, _address, peer->getAddress(), payload, true));
				int64_t time = BaseLib::HelperFunctions::getTime();
				physicalInterface->sendPacket(packet);
				waitIndex = 0;
				while(waitIndex < 100) //Wait, wait, wait. The WOR preamble alone needs 360ms with the CUL! And AES handshakes need time, too.
				{
					receivedPacket = _receivedPackets.get(peer->getAddress());
					if(receivedPacket && receivedPacket->timeReceived() > time && receivedPacket->payload()->size() >= 1 && receivedPacket->payload()->at(0) == 0 && receivedPacket->destinationAddress() == _address && receivedPacket->controlByte() == 0x80 && receivedPacket->messageType() == 2)
					{
						GD::out.printInfo("Info: Enter bootloader packet was accepted by peer.");
						responseReceived = true;
						break;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(20));
					waitIndex++;
				}
				if(responseReceived) break;
			}
			if(!responseReceived)
			{
				peer->setPhysicalInterfaceID(oldPhysicalInterfaceID);
				_updateMutex.unlock();
				_updateMode = false;
				_bl->deviceUpdateInfo.results[id].first = 6;
				_bl->deviceUpdateInfo.results[id].second = "Device did not respond to enter-bootloader packet.";
				GD::out.printWarning("Warning: Device did not enter bootloader.");
				return;
			}
		}

		int32_t retries = 0;
		for(retries = 0; retries < 10; retries++)
		{
			int64_t time = BaseLib::HelperFunctions::getTime();
			bool requestReceived = false;
			int32_t maxWaitIndex = manual ? 1000 : 100;
			while(waitIndex < 1000)
			{
				receivedPacket = _receivedPackets.get(peer->getAddress());
				if(receivedPacket && receivedPacket->timeReceived() > time && receivedPacket->payload()->size() > 1 && receivedPacket->payload()->at(0) == 0 && receivedPacket->destinationAddress() == 0 && receivedPacket->controlByte() == 0 && receivedPacket->messageType() == 0x10)
				{
					std::string serialNumber(&receivedPacket->payload()->at(1), &receivedPacket->payload()->at(1) + receivedPacket->payload()->size() - 1);
					if(serialNumber == peer->getSerialNumber())
					{
						requestReceived = true;
						break;
					}
					else GD::out.printWarning("Warning: Update request received, but serial number (" + serialNumber + ") does not match.");
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				waitIndex++;
			}
			if(!requestReceived || !receivedPacket)
			{
				peer->setPhysicalInterfaceID(oldPhysicalInterfaceID);
				_updateMutex.unlock();
				_updateMode = false;
				_bl->deviceUpdateInfo.results[id].first = 7;
				_bl->deviceUpdateInfo.results[id].second = "No update request received.";
				GD::out.printWarning("Warning: No update request received.");
				return;
			}

			std::vector<uint8_t> payload({0x10, 0x5B, 0x11, 0xF8, 0x15, 0x47}); //TI CC1100 register settings
			std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(0x42, 0, 0xCB, 0, peer->getAddress(), payload, true));
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			physicalInterface->sendPacket(packet);

			GD::out.printInfo("Info: Enabling update mode.");
			physicalInterface->enableUpdateMode();

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			packet = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(0x43, 0x20, 0xCB, 0, peer->getAddress(), payload, true));
			physicalInterface->sendPacket(packet);

			requestReceived = false;
			waitIndex = 0;
			while(waitIndex < 100)
			{
				receivedPacket = _receivedPackets.get(peer->getAddress());
				if(receivedPacket && receivedPacket->payload()->size() == 1 && receivedPacket->payload()->at(0) == 0 && receivedPacket->destinationAddress() == 0 && receivedPacket->controlByte() == 0 && receivedPacket->messageType() == 2)
				{
					requestReceived = true;
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				waitIndex++;
			}
			if(requestReceived) break;
			GD::out.printInfo("Info: Disabling update mode.");
			physicalInterface->disableUpdateMode();
			std::this_thread::sleep_for(std::chrono::milliseconds(4000));
		}
		if(retries == 10 || !receivedPacket)
		{
			peer->setPhysicalInterfaceID(oldPhysicalInterfaceID);
			_updateMutex.unlock();
			_updateMode = false;
			_bl->deviceUpdateInfo.results[id].first = 7;
			_bl->deviceUpdateInfo.results[id].second = "No update request received.";
			GD::out.printError("Error: No update request received.");
			std::this_thread::sleep_for(std::chrono::milliseconds(7000));
			return;
		}

		GD::out.printInfo("Info: Updating peer " + std::to_string(id) + " from version " + oldVersionString + " to version " + versionString + ".");

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		uint8_t messageCounter = receivedPacket->messageCounter() + 1;
		int32_t blockCounter = 1;
		for(std::vector<std::vector<uint8_t>>::iterator i = blocks.begin(); i != blocks.end(); ++i)
		{
			GD::out.printInfo("Info: Sending block " + std::to_string(blockCounter) + " of " + std::to_string(blocks.size()) + "...");
			_bl->deviceUpdateInfo.currentDeviceProgress = (blockCounter * 100) / blocks.size();
			blockCounter++;
			int32_t retries = 0;
			for(retries = 0; retries < 10; retries++)
			{
				int32_t pos = 0;
				std::vector<uint8_t> payload;
				while(pos < i->size())
				{
					payload.clear();
					if(pos == 0)
					{
						payload.push_back(i->size() >> 8);
						payload.push_back(i->size() & 0xFF);
					}
					if(i->size() - pos >= 35)
					{
						payload.insert(payload.end(), i->begin() + pos, i->begin() + pos + 35);
						pos += 35;
					}
					else
					{
						payload.insert(payload.end(), i->begin() + pos, i->begin() + pos + (i->size() - pos));
						pos += (i->size() - pos);
					}
					uint8_t controlByte = (pos < i->size()) ? 0 : 0x20;
					std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(messageCounter, controlByte, 0xCA, 0, peer->getAddress(), payload, true));
					physicalInterface->sendPacket(packet);
					std::this_thread::sleep_for(std::chrono::milliseconds(55));
				}
				waitIndex = 0;
				bool okReceived = false;
				while(waitIndex < 15)
				{
					receivedPacket = _receivedPackets.get(peer->getAddress());
					if(receivedPacket && receivedPacket->messageCounter() == messageCounter && receivedPacket->payload()->size() == 1 && receivedPacket->payload()->at(0) == 0 && receivedPacket->destinationAddress() == 0 && receivedPacket->controlByte() == 0 && receivedPacket->messageType() == 2)
					{
						messageCounter++;
						okReceived = true;
						break;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(20));
					waitIndex++;
				}
				if(okReceived) break;
			}
			if(retries == 10)
			{
				GD::out.printInfo("Info: Disabling update mode.");
				physicalInterface->disableUpdateMode();
				peer->setPhysicalInterfaceID(oldPhysicalInterfaceID);
				_updateMutex.unlock();
				_updateMode = false;
				_bl->deviceUpdateInfo.results[id].first = 8;
				_bl->deviceUpdateInfo.results[id].second = "Too many communication errors.";
				GD::out.printError("Error: Too many communication errors.");
				std::this_thread::sleep_for(std::chrono::milliseconds(7000));
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(55));
		}
		peer->setFirmwareVersion(firmwareVersion);
		_bl->deviceUpdateInfo.results[id].first = 0;
		_bl->deviceUpdateInfo.results[id].second = "Update successful.";
		GD::out.printInfo("Info: Peer " + std::to_string(id) + " was successfully updated to firmware version " + versionString + ".");
		GD::out.printInfo("Info: Disabling update mode.");
		physicalInterface->disableUpdateMode();
		if(peer) peer->setPhysicalInterfaceID(oldPhysicalInterfaceID);
		_updateMutex.unlock();
		_updateMode = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(7000));
		return;
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
    _bl->deviceUpdateInfo.results[id].first = 1;
	_bl->deviceUpdateInfo.results[id].second = "Unknown error.";
    GD::out.printInfo("Info: Disabling update mode.");
    peer->setPhysicalInterfaceID(oldPhysicalInterfaceID);
    if(physicalInterface) physicalInterface->disableUpdateMode();
    _updateMutex.unlock();
    _updateMode = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(7000));
}

int32_t HomeMaticCentral::getUniqueAddress(int32_t seed)
{
	try
	{
		uint32_t i = 0;
		while((_peers.find(seed) != _peers.end() || GD::family->get(seed)) && i++ < 200000)
		{
			seed += 9345;
			if(seed > 16777215) seed -= 16777216;
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
	return seed;
}

void HomeMaticCentral::addPeersToVirtualDevices()
{
	try
	{
		_peersMutex.lock();
		for(std::unordered_map<int32_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			std::shared_ptr<BidCoSPeer> peer(std::dynamic_pointer_cast<BidCoSPeer>(i->second));
			std::shared_ptr<HomeMaticDevice> device = peer->getHiddenPeerDevice();
			if(device && device->getID() != _deviceID) //Central can also be hidden peer. No check => deadlock
			{
				device->addPeer(peer);
			}
		}
		_peersMutex.unlock();
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

std::string HomeMaticCentral::getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber)
{
	try
	{
		if(seedPrefix.size() > 3) throw BaseLib::Exception("seedPrefix is too long.");
		uint32_t i = 0;
		int32_t numberSize = 10 - seedPrefix.size();
		std::ostringstream stringstream;
		stringstream << seedPrefix << std::setw(numberSize) << std::setfill('0') << std::dec << seedNumber;
		std::string temp2 = stringstream.str();
		while((_peersBySerial.find(temp2) != _peersBySerial.end() || GD::family->get(temp2)) && i++ < 100000)
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
	return "";
}

void HomeMaticCentral::addHomegearFeaturesHMCCVD(std::shared_ptr<BidCoSPeer> peer, int32_t channel, bool pushPendingBidCoSQueues)
{
	try
	{
		std::shared_ptr<HomeMaticDevice> tc(peer->getHiddenPeerDevice());
		if(!tc)
		{
			int32_t hmcctcAddress = getUniqueAddress((0x39 << 16) + (peer->getAddress() & 0xFF00) + (peer->getAddress() & 0xFF));
			if(peer->hasPeers(1) && !peer->getPeer(1, hmcctcAddress)) return; //Already linked to a HM-CC-TC
			std::string temp = peer->getSerialNumber().substr(3);
			std::string serialNumber = getUniqueSerialNumber("VCD", BaseLib::HelperFunctions::getNumber(temp));
			GD::family->add(std::shared_ptr<LogicalDevice>(new HM_CC_TC(0, serialNumber, hmcctcAddress, (IDeviceEventSink*)getEventHandler())));
			tc = getDevice(hmcctcAddress);
			tc->addPeer(peer);
		}
		std::shared_ptr<BaseLib::Systems::BasicPeer> hmcctc(new BaseLib::Systems::BasicPeer());
		hmcctc->address = tc->getAddress();
		hmcctc->serialNumber = tc->getSerialNumber();
		hmcctc->channel = 1;
		hmcctc->hidden = true;
		peer->addPeer(1, hmcctc);

		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(peer->getPhysicalInterface(), BidCoSQueueType::CONFIG));
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

		peer->pendingBidCoSQueues->push(pendingQueue);
		peer->serviceMessages->setConfigPending(true);
		if(pushPendingBidCoSQueues)
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, peer->getPhysicalInterface(), BidCoSQueueType::CONFIG, peer->getAddress());
			queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::addHomegearFeaturesRemote(std::shared_ptr<BidCoSPeer> peer, int32_t channel, bool pushPendingBidCoSQueues)
{
	try
	{
		if(!peer) return;
		std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>> channels;
		if(channel == -1)
		{
			for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
			{
				if(!peer->hasPeers(i->first) || peer->getPeer(i->first, _address))
				{
					channels[i->first] = i->second;
				}
			}
			if(channels.empty()) return; //All channels are already paired to actors
		}
		std::shared_ptr<BaseLib::Systems::BasicPeer> switchPeer;
		if(channel > -1)
		{
			switchPeer.reset(new BaseLib::Systems::BasicPeer());
			switchPeer->id = 0xFFFFFFFFFFFFFFFF;
			switchPeer->address = _address;
			switchPeer->serialNumber = _serialNumber;
			switchPeer->channel = channel;
			switchPeer->hidden = true;
			peer->addPeer(channel, switchPeer);
		}
		else
		{
			for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
			{
				if(i->second->type != "KEY" && i->second->type != "MOTION_DETECTOR" && i->second->type != "SWITCH_INTERFACE" && i->second->type != "PULSE_SENSOR") continue;
				switchPeer.reset(new BaseLib::Systems::BasicPeer());
				switchPeer->id = 0xFFFFFFFFFFFFFFFF;
				switchPeer->address = _address;
				switchPeer->serialNumber = _serialNumber;
				switchPeer->channel = i->first;
				switchPeer->hidden = true;
				peer->addPeer(i->first, switchPeer);
			}
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> paramset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		if(peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC19 || peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC19B || peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC19SW)
		{
			paramset->structValue->insert(BaseLib::RPC::RPCStructElement("LCD_SYMBOL", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(2))));
			paramset->structValue->insert(BaseLib::RPC::RPCStructElement("LCD_LEVEL_INTERP", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(1))));
		}

		std::vector<uint8_t> payload;
		if(channel > -1)
		{
			std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(peer->getPhysicalInterface(), BidCoSQueueType::CONFIG));
			pendingQueue->noSending = true;

			payload.clear();
			payload.push_back(channel);
			payload.push_back(0x01);
			payload.push_back(_address >> 16);
			payload.push_back((_address >> 8) & 0xFF);
			payload.push_back(_address & 0xFF);
			payload.push_back(channel);
			payload.push_back(0);
			std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, peer->getAddress(), payload));
			pendingQueue->push(configPacket);
			pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
			_messageCounter[0]++;
			peer->pendingBidCoSQueues->push(pendingQueue);
			peer->serviceMessages->setConfigPending(true);

			//putParamset pushes the packets on pendingQueues, but does not send immediately
			if(!paramset->structValue->empty()) peer->putParamset(channel, BaseLib::RPC::ParameterSet::Type::Enum::link, 0xFFFFFFFFFFFFFFFF, channel, paramset, true);
		}
		else
		{
			for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = channels.begin(); i != channels.end(); ++i)
			{
				if(i->second->type != "KEY" && i->second->type != "MOTION_DETECTOR" && i->second->type != "SWITCH_INTERFACE" && i->second->type != "PULSE_SENSOR") continue;
				std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(peer->getPhysicalInterface(), BidCoSQueueType::CONFIG));
				pendingQueue->noSending = true;

				payload.clear();
				payload.push_back(i->first);
				payload.push_back(0x01);
				payload.push_back(_address >> 16);
				payload.push_back((_address >> 8) & 0xFF);
				payload.push_back(_address & 0xFF);
				payload.push_back(i->first);
				payload.push_back(0);
				std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, peer->getAddress(), payload));
				pendingQueue->push(configPacket);
				pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
				_messageCounter[0]++;
				peer->pendingBidCoSQueues->push(pendingQueue);
				peer->serviceMessages->setConfigPending(true);

				//putParamset pushes the packets on pendingQueues, but does not send immediately
				if(!paramset->structValue->empty()) peer->putParamset(i->first, BaseLib::RPC::ParameterSet::Type::Enum::link, 0xFFFFFFFFFFFFFFFF, i->first, paramset, true);
			}
		}

		if(pushPendingBidCoSQueues)
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, peer->getPhysicalInterface(), BidCoSQueueType::CONFIG, peer->getAddress());
			queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::addHomegearFeaturesMotionDetector(std::shared_ptr<BidCoSPeer> peer, int32_t channel, bool pushPendingBidCoSQueues)
{
	try
	{
		addHomegearFeaturesRemote(peer, channel, pushPendingBidCoSQueues);
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

void HomeMaticCentral::addHomegearFeaturesHMCCRTDN(std::shared_ptr<BidCoSPeer> peer, int32_t channel, bool pushPendingBidCoSQueues)
{
	try
	{
		if(!peer) return;
		std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>> channels;
		if(channel == -1) channel = 3;
		if(peer->hasPeers(channel) && !peer->getPeer(channel, _address)) return;
		GD::out.printInfo("Info: Adding Homegear features to HM-CC-RT-DN.");
		std::shared_ptr<BaseLib::Systems::BasicPeer> switchPeer;

		switchPeer.reset(new BaseLib::Systems::BasicPeer());
		switchPeer->id = 0xFFFFFFFFFFFFFFFF;
		switchPeer->address = _address;
		switchPeer->serialNumber = _serialNumber;
		switchPeer->channel = channel;
		switchPeer->hidden = true;
		peer->addPeer(channel, switchPeer);

		std::vector<uint8_t> payload;
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(peer->getPhysicalInterface(), BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		payload.clear();
		payload.push_back(channel);
		payload.push_back(0x01);
		payload.push_back(_address >> 16);
		payload.push_back((_address >> 8) & 0xFF);
		payload.push_back(_address & 0xFF);
		payload.push_back(channel);
		payload.push_back(0);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, peer->getAddress(), payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;
		peer->pendingBidCoSQueues->push(pendingQueue);
		peer->serviceMessages->setConfigPending(true);

		if(pushPendingBidCoSQueues)
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, peer->getPhysicalInterface(), BidCoSQueueType::CONFIG, peer->getAddress());
			queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::addHomegearFeatures(std::shared_ptr<BidCoSPeer> peer, int32_t channel, bool pushPendingBidCoSQueues)
{
	try
	{
		GD::out.printDebug("Debug: Adding homegear features. Device type: 0x" + BaseLib::HelperFunctions::getHexString((int32_t)peer->getDeviceType().type()));
		if(peer->getDeviceType().type() == (uint32_t)DeviceType::HMCCVD) addHomegearFeaturesHMCCVD(peer, channel, pushPendingBidCoSQueues);
		else if(peer->getDeviceType().type() == (uint32_t)DeviceType::HMPB4DISWM ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC4 ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC4B ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRCP1 ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRCSEC3 ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRCSEC3B ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRCKEY3 ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRCKEY3B ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMPB4WM ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMPB2WM ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC12 ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC12B ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC12SW ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC19 ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC19B ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC19SW ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::RCH ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::ATENT ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::ZELSTGRMHS4 ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRC42 ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRCSEC42 ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMRCKEY42 ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMPB6WM55 ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMSWI3FM ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMSWI3FMSCHUECO ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMSWI3FMROTO ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMSENEP) addHomegearFeaturesRemote(peer, channel, pushPendingBidCoSQueues);
		else if(peer->getDeviceType().type() == (uint32_t)DeviceType::HMSECMDIR ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMSECMDIRSCHUECO ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMSENMDIRSM ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMSENMDIRO) addHomegearFeaturesMotionDetector(peer, channel, pushPendingBidCoSQueues);
		else if(peer->getDeviceType().type() == (uint32_t)DeviceType::HMCCRTDN ||
				peer->getDeviceType().type() == (uint32_t)DeviceType::HMCCRTDNBOM) addHomegearFeaturesHMCCRTDN(peer, channel, pushPendingBidCoSQueues);
		else GD::out.printDebug("Debug: No homegear features to add.");
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

void HomeMaticCentral::deletePeer(uint64_t id)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(id));
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
		if(peer->getPhysicalInterface()->needsPeers()) peer->getPhysicalInterface()->removePeer(peer->getAddress());
		_peersMutex.lock();
		if(_peersBySerial.find(peer->getSerialNumber()) != _peersBySerial.end()) _peersBySerial.erase(peer->getSerialNumber());
		if(_peersByID.find(id) != _peersByID.end()) _peersByID.erase(id);
		if(_peers.find(peer->getAddress()) != _peers.end()) _peers.erase(peer->getAddress());
		_peersMutex.unlock();
		removePeerFromTeam(peer);
		peer->deleteFromDatabase();
		peer->deletePairedVirtualDevices();
		GD::out.printMessage("Removed HomeMatic BidCoS peer " + std::to_string(peer->getID()));
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

void HomeMaticCentral::reset(uint64_t id, bool defer)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(id));
		if(!peer) return;
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, peer->getPhysicalInterface(), BidCoSQueueType::UNPAIRING, peer->getAddress());
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(peer->getPhysicalInterface(), BidCoSQueueType::UNPAIRING));
		pendingQueue->noSending = true;

		uint8_t configByte = 0xA0;
		if(peer->getRXModes() & BaseLib::RPC::Device::RXModes::burst) configByte |= 0x10;

		std::vector<uint8_t> payload;

		//CONFIG_START
		payload.push_back(0x04);
		payload.push_back(0x00);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], configByte, 0x11, _address, peer->getAddress(), payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		_messageCounter[0]++;

		if(defer)
		{
			while(!peer->pendingBidCoSQueues->empty()) peer->pendingBidCoSQueues->pop();
			peer->pendingBidCoSQueues->push(pendingQueue);
			peer->serviceMessages->setConfigPending(true);
			queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::unpair(uint64_t id, bool defer)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(id));
		if(!peer) return;
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, peer->getPhysicalInterface(), BidCoSQueueType::UNPAIRING, peer->getAddress());
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(peer->getPhysicalInterface(), BidCoSQueueType::UNPAIRING));
		pendingQueue->noSending = true;

		uint8_t configByte = 0xA0;
		if(peer->getRXModes() & BaseLib::RPC::Device::RXModes::burst) configByte |= 0x10;
		std::vector<uint8_t> payload;

		//CONFIG_START
		payload.push_back(0);
		payload.push_back(0x05);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		std::shared_ptr<BidCoSPacket> configPacket(new BidCoSPacket(_messageCounter[0], configByte, 0x01, _address, peer->getAddress(), payload));
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
		configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, peer->getAddress(), payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		payload.clear();
		_messageCounter[0]++;

		//END_CONFIG
		payload.push_back(0);
		payload.push_back(0x06);
		configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, peer->getAddress(), payload));
		pendingQueue->push(configPacket);
		pendingQueue->push(_messages->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		payload.clear();
		_messageCounter[0]++;

		if(defer)
		{
			while(!peer->pendingBidCoSQueues->empty()) peer->pendingBidCoSQueues->pop();
			peer->pendingBidCoSQueues->push(pendingQueue);
			peer->serviceMessages->setConfigPending(true);
			queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(packet->destinationAddress() != 0 && packet->destinationAddress() != _address) return;
		if(packet->payload()->size() < 17)
		{
			GD::out.printError("Error: Pairing packet is too small (payload size has to be at least 17).");
			return;
		}

		std::string serialNumber;
		for(uint32_t i = 3; i < 13; i++)
			serialNumber.push_back((char)packet->payload()->at(i));
		uint32_t rawType = (packet->payload()->at(1) << 8) + packet->payload()->at(2);
		BaseLib::Systems::LogicalDeviceType deviceType(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS, rawType);

		std::shared_ptr<BidCoSPeer> peer(getPeer(packet->senderAddress()));
		if(peer && (peer->getSerialNumber() != serialNumber || peer->getDeviceType() != deviceType))
		{
			GD::out.printError("Error: Pairing packet rejected, because a peer with the same address but different serial number or device type is already paired to this central.");
			return;
		}

		if((packet->controlByte() & 0x20) && packet->destinationAddress() == _address) sendOK(packet->messageCounter(), packet->senderAddress());

		std::vector<uint8_t> payload;

		std::shared_ptr<BidCoSQueue> queue;
		if(_pairing)
		{
			queue = _bidCoSQueueManager.createQueue(this, getPhysicalInterface(packet->senderAddress()), BidCoSQueueType::PAIRING, packet->senderAddress());

			if(!peer)
			{
				//Do not save here
				queue->peer = createPeer(packet->senderAddress(), packet->payload()->at(0), deviceType, serialNumber, 0, 0, packet, false);
				if(!queue->peer)
				{
					GD::out.printWarning("Warning: Device type not supported. Sender address 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress()) + ".");
					return;
				}
				peer = queue->peer;
				if(peer->getPhysicalInterface()->needsPeers()) peer->getPhysicalInterface()->addPeer(peer->getPeerInfo());
			}

			if(!peer->rpcDevice)
			{
				GD::out.printWarning("Warning: Device type not supported. Sender address 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress()) + ".");
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
			std::shared_ptr<BaseLib::RPC::Parameter> internalKeysVisible = peer->rpcDevice->parameterSet->getParameter("INTERNAL_KEYS_VISIBLE");
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
			//if(!peerExists(packet->senderAddress())) //Only request config when peer is not already paired to central
			//{
				for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
				{
					std::shared_ptr<BidCoSQueue> pendingQueue;
					int32_t channel = i->first;
					//Walk through all lists to request master config if necessary
					if(peer->rpcDevice->channels.at(channel)->parameterSets.find(BaseLib::RPC::ParameterSet::Type::Enum::master) != peer->rpcDevice->channels.at(channel)->parameterSets.end())
					{
						std::shared_ptr<BaseLib::RPC::ParameterSet> masterSet = peer->rpcDevice->channels.at(channel)->parameterSets[BaseLib::RPC::ParameterSet::Type::Enum::master];
						for(std::map<uint32_t, uint32_t>::iterator k = masterSet->lists.begin(); k != masterSet->lists.end(); ++k)
						{
							pendingQueue.reset(new BidCoSQueue(peer->getPhysicalInterface(), BidCoSQueueType::CONFIG));
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
							peer->pendingBidCoSQueues->push(pendingQueue);
							peer->serviceMessages->setConfigPending(true);
						}
					}

					if(peer->rpcDevice->channels[channel]->linkRoles && (!peer->rpcDevice->channels[channel]->linkRoles->sourceNames.empty() || !peer->rpcDevice->channels[channel]->linkRoles->targetNames.empty()))
					{
						pendingQueue.reset(new BidCoSQueue(peer->getPhysicalInterface(), BidCoSQueueType::CONFIG));
						pendingQueue->noSending = true;
						payload.push_back(channel);
						payload.push_back(0x03);
						configPacket = std::shared_ptr<BidCoSPacket>(new BidCoSPacket(_messageCounter[0], 0xA0, 0x01, _address, packet->senderAddress(), payload));
						pendingQueue->push(configPacket);
						pendingQueue->push(_messages->find(DIRECTIONIN, 0x10, std::vector<std::pair<uint32_t, int32_t>>()));
						payload.clear();
						_messageCounter[0]++;
						peer->pendingBidCoSQueues->push(pendingQueue);
						peer->serviceMessages->setConfigPending(true);
					}
				}
			//}
		}
		//not in pairing mode
		else queue = _bidCoSQueueManager.createQueue(this, peer->getPhysicalInterface(), BidCoSQueueType::DEFAULT, packet->senderAddress());

		if(!peer)
		{
			GD::out.printError("Error handling pairing packet: Peer is nullptr. This shouldn't have happened. Something went very wrong.");
			return;
		}

		if(peer->pendingBidCoSQueues && !peer->pendingBidCoSQueues->empty()) GD::out.printInfo("Info: Pushing pending queues.");
		queue->push(peer->pendingBidCoSQueues); //This pushes the just generated queue and the already existent pending queue onto the queue
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

void HomeMaticCentral::sendRequestConfig(int32_t address, uint8_t localChannel, uint8_t list, int32_t remoteAddress, uint8_t remoteChannel)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(address));
		if(!peer) return;
		bool oldQueue = true;
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(address);
		if(!queue)
		{
			oldQueue = false;
			queue = _bidCoSQueueManager.createQueue(this, peer->getPhysicalInterface(), BidCoSQueueType::CONFIG, address);
		}
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(peer->getPhysicalInterface(), BidCoSQueueType::CONFIG));
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

		peer->pendingBidCoSQueues->push(pendingQueue);
		peer->serviceMessages->setConfigPending(true);
		if(!oldQueue) queue->push(peer->pendingBidCoSQueues);
		else if(queue->pendingQueuesEmpty()) queue->push(peer->pendingBidCoSQueues);
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

void HomeMaticCentral::handleConfigParamResponse(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(packet->senderAddress()));
		if(!peer) return;
		//Config changed in device
		if(packet->payload()->size() > 7 && packet->payload()->at(0) == 0x05)
		{
			if(packet->controlByte() & 0x20) sendOK(packet->messageCounter(), packet->senderAddress());
			if(packet->payload()->size() == 8) return; //End packet
			int32_t list = packet->payload()->at(6);
			int32_t channel = packet->payload()->at(1); //??? Not sure if this really is the channel
			int32_t remoteAddress = (packet->payload()->at(2) << 16) + (packet->payload()->at(3) << 8) + packet->payload()->at(4);
			int32_t remoteChannel = (remoteAddress == 0) ? 0 : packet->payload()->at(5);
			BaseLib::RPC::ParameterSet::Type::Enum type = (remoteAddress != 0) ? BaseLib::RPC::ParameterSet::Type::link : BaseLib::RPC::ParameterSet::Type::master;
			int32_t startIndex = packet->payload()->at(7);
			int32_t endIndex = startIndex + packet->payload()->size() - 9;
			if(peer->rpcDevice->channels.find(channel) == peer->rpcDevice->channels.end() || peer->rpcDevice->channels[channel]->parameterSets.find(type) == peer->rpcDevice->channels[channel]->parameterSets.end() || !peer->rpcDevice->channels[channel]->parameterSets[type])
			{
				GD::out.printError("Error: Received config for non existant parameter set.");
			}
			else
			{
				std::vector<std::shared_ptr<BaseLib::RPC::Parameter>> packetParameters = peer->rpcDevice->channels[channel]->parameterSets[type]->getIndices(startIndex, endIndex, list);
				for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator i = packetParameters.begin(); i != packetParameters.end(); ++i)
				{
					if(!(*i)->id.empty())
					{
						double position = ((*i)->physicalParameter->index - startIndex) + 8 + 9;
						if(position < 0)
						{
							GD::out.printError("Error: Packet position is negative. Device: " + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " Serial number: " + peer->getSerialNumber() + " Channel: " + std::to_string(channel) + " List: " + std::to_string((*i)->physicalParameter->list) + " Parameter index: " + std::to_string((*i)->index));
							continue;
						}
						BaseLib::Systems::RPCConfigurationParameter* parameter = nullptr;
						if(type == BaseLib::RPC::ParameterSet::Type::master) parameter = &peer->configCentral[channel][(*i)->id];
						//type == link
						else if(peer->getPeer(channel, remoteAddress, remoteChannel)) parameter = &peer->linksCentral[channel][remoteAddress][remoteChannel][(*i)->id];
						if(!parameter) continue;

						if(position < 9 + 8)
						{
							uint32_t byteOffset = 9 + 8 - position;
							uint32_t missingBytes = (*i)->physicalParameter->size - byteOffset;
							if(missingBytes >= (*i)->physicalParameter->size)
							{
								GD::out.printError("Error: Device tried to set parameter with more bytes than specified. Device: " + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " Serial number: " + peer->getSerialNumber() + " Channel: " + std::to_string(channel) + " List: " + std::to_string((*i)->physicalParameter->list) + " Parameter index: " + std::to_string((*i)->index));
								continue;
							}
							while(parameter->partialData.size() < (*i)->physicalParameter->size) parameter->partialData.push_back(0);
							position = 9 + 8;
							std::vector<uint8_t> data = packet->getPosition(position, missingBytes, (*i)->physicalParameter->mask);
							for(uint32_t j = 0; j < byteOffset; j++) parameter->data.at(j) = parameter->partialData.at(j);
							for(uint32_t j = byteOffset; j < (*i)->physicalParameter->size; j++) parameter->data.at(j) = data.at(j - byteOffset);
							//Don't clear partialData - packet might be resent
							peer->saveParameter(parameter->databaseID, type, channel, (*i)->id, parameter->data, remoteAddress, remoteChannel);
							if(_bl->debugLevel >= 4) GD::out.printInfo("Info: Parameter " + (*i)->id + " of device 0x" + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " at index " + std::to_string((*i)->physicalParameter->index) + " and packet index " + std::to_string(position) + " with size " + std::to_string((*i)->physicalParameter->size) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(parameter->data) + " after being partially set in the last packet.");
						}
						else if(position + (int32_t)(*i)->physicalParameter->size >= packet->length())
						{
							parameter->partialData.clear();
							parameter->partialData = packet->getPosition(position, (*i)->physicalParameter->size, (*i)->physicalParameter->mask);
							if(_bl->debugLevel >= 4) GD::out.printInfo("Info: Parameter " + (*i)->id + " of device 0x" + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " at index " + std::to_string((*i)->physicalParameter->index) + " and packet index " + std::to_string(position) + " with size " + std::to_string((*i)->physicalParameter->size) + " was partially set to 0x" + BaseLib::HelperFunctions::getHexString(parameter->partialData) + ".");
						}
						else
						{
							parameter->data = packet->getPosition(position, (*i)->physicalParameter->size, (*i)->physicalParameter->mask);
							peer->saveParameter(parameter->databaseID, type, channel, (*i)->id, parameter->data, remoteAddress, remoteChannel);
							if(_bl->debugLevel >= 4) GD::out.printInfo("Info: Parameter " + (*i)->id + " of device 0x" + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " at index " + std::to_string((*i)->physicalParameter->index) + " and packet index " + std::to_string(position) + " with size " + std::to_string((*i)->physicalParameter->size) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(parameter->data) + ".");
						}
					}
					else GD::out.printError("Error: Device tried to set parameter without id. Device: " + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " Serial number: " + peer->getSerialNumber() + " Channel: " + std::to_string(channel) + " List: " + std::to_string((*i)->physicalParameter->list) + " Parameter index: " + std::to_string((*i)->index));
				}
			}
			return;
		}
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(!queue || queue->isEmpty()) return;
		if(packet->controlByte() & 0x04) return; //Ignore broadcast packets
		//Config was requested by central
		std::shared_ptr<BidCoSPacket> sentPacket(_sentPackets.get(packet->senderAddress()));
		bool continuousData = false;
		bool multiPacket = false;
		bool multiPacketEnd = false;
		//Peer request
		if(sentPacket && sentPacket->payload()->size() >= 2 && sentPacket->payload()->at(1) == 0x03)
		{
			int32_t localChannel = sentPacket->payload()->at(0);
			std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = peer->rpcDevice->channels[localChannel];
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
								GD::out.printInfo("Info: Adding peer 0x" + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " to team with address 0x" + BaseLib::HelperFunctions::getHexString(peerAddress) + ".");
								addPeerToTeam(peer, localChannel, peerAddress, remoteChannel);
							}
						}
						else //Normal peer
						{
							std::shared_ptr<BaseLib::Systems::BasicPeer> newPeer(new BaseLib::Systems::BasicPeer());
							newPeer->address = peerAddress;
							newPeer->channel = remoteChannel;
							peer->addPeer(localChannel, newPeer);
							if(peer->rpcDevice->channels.find(localChannel) == peer->rpcDevice->channels.end()) continue;

							if(rpcChannel->parameterSets.find(BaseLib::RPC::ParameterSet::Type::Enum::link) == rpcChannel->parameterSets.end()) continue;
							std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = rpcChannel->parameterSets[BaseLib::RPC::ParameterSet::Type::Enum::link];
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
			int32_t remoteChannel = (remoteAddress == 0) ? 0 : sentPacket->payload()->at(5);
			BaseLib::RPC::ParameterSet::Type::Enum type = (remoteAddress != 0) ? BaseLib::RPC::ParameterSet::Type::link : BaseLib::RPC::ParameterSet::Type::master;
			std::shared_ptr<BaseLib::RPC::RPCVariable> parametersToEnforce;
			if(!peer->getPairingComplete()) parametersToEnforce.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
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
				if(peer->rpcDevice->channels.find(channel) == peer->rpcDevice->channels.end() || peer->rpcDevice->channels[channel]->parameterSets.find(type) == peer->rpcDevice->channels[channel]->parameterSets.end() || !peer->rpcDevice->channels[channel]->parameterSets[type])
				{
					GD::out.printError("Error: Received config for non existant parameter set.");
				}
				else
				{
					std::vector<std::shared_ptr<BaseLib::RPC::Parameter>> packetParameters = peer->rpcDevice->channels[channel]->parameterSets[type]->getIndices(startIndex, endIndex, list);
					for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator i = packetParameters.begin(); i != packetParameters.end(); ++i)
					{
						if(!(*i)->id.empty())
						{
							double position = ((*i)->physicalParameter->index - startIndex) + 2 + 9;
							BaseLib::Systems::RPCConfigurationParameter* parameter = nullptr;
							if(type == BaseLib::RPC::ParameterSet::Type::master) parameter = &peer->configCentral[channel][(*i)->id];
							//type == link
							else if(peer->getPeer(channel, remoteAddress, remoteChannel)) parameter = &peer->linksCentral[channel][remoteAddress][remoteChannel][(*i)->id];
							if(!parameter) continue;
							if(position < 9 + 2)
							{
								uint32_t byteOffset = 9 + 2 - position;
								uint32_t missingBytes = (*i)->physicalParameter->size - byteOffset;
								if(missingBytes >= (*i)->physicalParameter->size)
								{
									GD::out.printError("Error: Device tried to set parameter with more bytes than specified. Device: " + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " Serial number: " + peer->getSerialNumber() + " Channel: " + std::to_string(channel) + " List: " + std::to_string((*i)->physicalParameter->list) + " Parameter index: " + std::to_string((*i)->index));
									continue;
								}
								while(parameter->partialData.size() < (*i)->physicalParameter->size) parameter->partialData.push_back(0);
								position = 9 + 2;
								std::vector<uint8_t> data = packet->getPosition(position, missingBytes, (*i)->physicalParameter->mask);
								for(uint32_t j = 0; j < byteOffset; j++)
								{
									if(j >= parameter->data.size()) parameter->data.push_back(parameter->partialData.at(j));
									else parameter->data.at(j) = parameter->partialData.at(j);
								}
								for(uint32_t j = byteOffset; j < (*i)->physicalParameter->size; j++)
								{
									if(j >= parameter->data.size()) parameter->data.push_back(data.at(j - byteOffset));
									else parameter->data.at(j) = data.at(j - byteOffset);
								}
								//Don't clear partialData - packet might be resent
								peer->saveParameter(parameter->databaseID, type, channel, (*i)->id, parameter->data, remoteAddress, remoteChannel);
								if(type == BaseLib::RPC::ParameterSet::Type::master && !peer->getPairingComplete() && (*i)->logicalParameter->enforce)
								{
									parametersToEnforce->structValue->insert(BaseLib::RPC::RPCStructElement((*i)->id, (*i)->logicalParameter->getEnforceValue()));
								}
								if(_bl->debugLevel >= 5) GD::out.printDebug("Debug: Parameter " + (*i)->id + " of device 0x" + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " at index " + std::to_string((*i)->physicalParameter->index) + " and packet index " + std::to_string(position) + " with size " + std::to_string((*i)->physicalParameter->size) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(parameter->data) + " after being partially set in the last packet.");
							}
							else if(position + (int32_t)(*i)->physicalParameter->size >= packet->length())
							{
								parameter->partialData.clear();
								parameter->partialData = packet->getPosition(position, (*i)->physicalParameter->size, (*i)->physicalParameter->mask);
								if(_bl->debugLevel >= 5) GD::out.printDebug("Debug: Parameter " + (*i)->id + " of device 0x" + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " at index " + std::to_string((*i)->physicalParameter->index) + " and packet index " + std::to_string(position) + " with size " + std::to_string((*i)->physicalParameter->size) + " was partially set to 0x" + BaseLib::HelperFunctions::getHexString(parameter->partialData) + ".");
							}
							else
							{
								parameter->data = packet->getPosition(position, (*i)->physicalParameter->size, (*i)->physicalParameter->mask);
								peer->saveParameter(parameter->databaseID, type, channel, (*i)->id, parameter->data, remoteAddress, remoteChannel);
								if(type == BaseLib::RPC::ParameterSet::Type::master && !peer->getPairingComplete() && (*i)->logicalParameter->enforce)
								{
									parametersToEnforce->structValue->insert(BaseLib::RPC::RPCStructElement((*i)->id, (*i)->logicalParameter->getEnforceValue()));
								}
								if(_bl->debugLevel >= 5) GD::out.printDebug("Debug: Parameter " + (*i)->id + " of device 0x" + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " at index " + std::to_string((*i)->physicalParameter->index) + " and packet index " + std::to_string(position) + " with size " + std::to_string((*i)->physicalParameter->size) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(parameter->data) + ".");
							}
						}
						else GD::out.printError("Error: Device tried to set parameter without id. Device: " + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " Serial number: " + peer->getSerialNumber() + " Channel: " + std::to_string(channel) + " List: " + std::to_string((*i)->physicalParameter->list) + " Parameter index: " + std::to_string((*i)->index));
					}
				}
			}
			else if(!multiPacketEnd)
			{
				if(peer->rpcDevice->channels.find(channel) == peer->rpcDevice->channels.end() || peer->rpcDevice->channels[channel]->parameterSets.find(type) == peer->rpcDevice->channels[channel]->parameterSets.end() || !peer->rpcDevice->channels[channel]->parameterSets[type])
				{
					GD::out.printError("Error: Received config for non existant parameter set.");
				}
				else
				{
					int32_t length = multiPacket ? packet->payload()->size() : packet->payload()->size() - 2;
					for(uint32_t i = 1; i < length; i += 2)
					{
						int32_t index = packet->payload()->at(i);
						std::vector<std::shared_ptr<BaseLib::RPC::Parameter>> packetParameters = peer->rpcDevice->channels[channel]->parameterSets[type]->getIndices(index, index, list);
						for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator j = packetParameters.begin(); j != packetParameters.end(); ++j)
						{
							if(!(*j)->id.empty())
							{
								double position = std::fmod((*j)->physicalParameter->index, 1) + 9 + i + 1;
								double size = (*j)->physicalParameter->size;
								if(size > 1.0) size = 1.0; //Reading more than one byte doesn't make any sense
								uint8_t data = packet->getPosition(position, size, (*j)->physicalParameter->mask).at(0);
								BaseLib::Systems::RPCConfigurationParameter* configParam = nullptr;
								if(type == BaseLib::RPC::ParameterSet::Type::master)
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
										parametersToEnforce->structValue->insert(BaseLib::RPC::RPCStructElement((*j)->id, (*j)->logicalParameter->getEnforceValue()));
									}
									peer->saveParameter(configParam->databaseID, type, channel, (*j)->id, configParam->data, remoteAddress, remoteChannel);
									if(_bl->debugLevel >= 5) GD::out.printDebug("Debug: Parameter " + (*j)->id + " of device 0x" + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " at index " + std::to_string((*j)->physicalParameter->index) + " and packet index " + std::to_string(position) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(data) + ".");
								}
							}
							else GD::out.printError("Error: Device tried to set parameter without id. Device: " + BaseLib::HelperFunctions::getHexString(peer->getAddress()) + " Serial number: " + peer->getSerialNumber() + " Channel: " + std::to_string(channel) + " List: " + std::to_string((*j)->physicalParameter->list) + " Parameter index: " + std::to_string((*j)->index));
						}
					}
				}
			}
			if(!peer->getPairingComplete() && !parametersToEnforce->structValue->empty()) peer->putParamset(channel, type, 0, -1, parametersToEnforce, true);
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

void HomeMaticCentral::resetTeam(std::shared_ptr<BidCoSPeer> peer, uint32_t channel)
{
	try
	{
		removePeerFromTeam(peer);

		std::shared_ptr<BidCoSPeer> team;
		std::string serialNumber('*' + peer->getSerialNumber());
		_peersMutex.lock();
		if(_peersBySerial.find(serialNumber) == _peersBySerial.end())
		{
			_peersMutex.unlock();
			team = createTeam(peer->getAddress(), peer->getDeviceType(), serialNumber);
			team->rpcDevice = peer->rpcDevice->team;
			team->setID(peer->getID() | (1 << 30));
			team->initializeCentralConfig();
			_peersMutex.lock();
			_peersBySerial[team->getSerialNumber()] = team;
			_peersByID[team->getID()] = team;
		}
		else team = getPeer('*' + peer->getSerialNumber());
		_peersMutex.unlock();
		peer->setTeamRemoteAddress(team->getAddress());
		peer->setTeamRemoteSerialNumber(team->getSerialNumber());
		peer->setTeamChannel(channel);
		for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator j = team->rpcDevice->channels.begin(); j != team->rpcDevice->channels.end(); ++j)
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

void HomeMaticCentral::addPeerToTeam(std::shared_ptr<BidCoSPeer> peer, int32_t channel, int32_t teamAddress, uint32_t teamChannel)
{
	try
	{
		std::shared_ptr<BidCoSPeer> teamPeer(getPeer(teamAddress));
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

void HomeMaticCentral::addPeerToTeam(std::shared_ptr<BidCoSPeer> peer, int32_t channel, uint32_t teamChannel, std::string teamSerialNumber)
{
	try
	{
		_peersMutex.lock();
		if(_peersBySerial.find(teamSerialNumber) == _peersBySerial.end() || !_peersBySerial.at(teamSerialNumber))
		{
			_peersMutex.unlock();
			return;
		}
		std::shared_ptr<BidCoSPeer> team = getPeer(teamSerialNumber);
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

void HomeMaticCentral::removePeerFromTeam(std::shared_ptr<BidCoSPeer> peer)
{
	try
	{
		_peersMutex.lock();
		if(peer->getTeamRemoteSerialNumber().empty() || _peersBySerial.find(peer->getTeamRemoteSerialNumber()) == _peersBySerial.end())
		{
			_peersMutex.unlock();
			return;
		}
		std::shared_ptr<BidCoSPeer> oldTeam = getPeer(peer->getTeamRemoteSerialNumber());
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
			_peersByID.erase(oldTeam->getID());
			_peersMutex.unlock();
		}
		peer->setTeamRemoteSerialNumber("");
		peer->setTeamRemoteAddress(0);
		peer->setTeamRemoteChannel(0);
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

void HomeMaticCentral::handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(packet->senderAddress());
		if(!queue) return;
		std::shared_ptr<BidCoSPacket> sentPacket(_sentPackets.get(packet->senderAddress()));
		if(packet->payload()->at(0) == 0x80 || packet->payload()->at(0) == 0x84)
		{
			if(_bl->debugLevel >= 2)
			{
				if(sentPacket) GD::out.printError("Error: NACK received from 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress()) + " in response to " + sentPacket->hexString() + ".");
				else GD::out.printError("Error: NACK received from 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress()));
				if(queue->getQueueType() == BidCoSQueueType::PAIRING) GD::out.printError("Try resetting the device to factory defaults before pairing it to this central.");
			}
			if(queue->getQueueType() == BidCoSQueueType::PAIRING) queue->clear(); //Abort
			else queue->pop(); //Otherwise the queue might persist forever. NACKS shouldn't be received when not pairing
			return;
		}
		bool aesKeyChanged = false;
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
					if(queue->peer->getRXModes() & BaseLib::RPC::Device::RXModes::burst) queue->setWakeOnRadioBit();
					std::shared_ptr<BaseLib::RPC::RPCVariable> deviceDescriptions(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
					deviceDescriptions->arrayValue = queue->peer->getDeviceDescriptions(true, std::map<std::string, bool>());
					raiseRPCNewDevices(deviceDescriptions);
					GD::out.printMessage("Added peer 0x" + BaseLib::HelperFunctions::getHexString(queue->peer->getAddress()) + ".");
					for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = queue->peer->rpcDevice->channels.begin(); i != queue->peer->rpcDevice->channels.end(); ++i)
					{
						if(queue->peer->getPhysicalInterface()->aesSupported() && i->second->aesDefault)
						{
							queue->peer->peerInfoPacketsEnabled = false;
							std::shared_ptr<BaseLib::RPC::RPCVariable> variables(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
							variables->structValue->insert(BaseLib::RPC::RPCStructElement("AES_ACTIVE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(true))));
							queue->peer->putParamset(i->first, BaseLib::RPC::ParameterSet::Type::master, 0, -1, variables, true);
							queue->peer->peerInfoPacketsEnabled = true;
						}
						if(i->second->hasTeam)
						{
							GD::out.printInfo("Info: Creating team for channel " + std::to_string(i->first) + ".");
							resetTeam(queue->peer, i->first);
							//Check if a peer exists with this devices team and if yes add it to the team.
							//As the serial number of the team was unknown up to this point, this couldn't
							//be done before.
							try
							{
								std::vector<std::shared_ptr<BidCoSPeer>> peers;
								_peersMutex.lock();
								for(std::unordered_map<int32_t, std::shared_ptr<BaseLib::Systems::Peer>>::const_iterator j = _peers.begin(); j != _peers.end(); ++j)
								{
									std::shared_ptr<BidCoSPeer> peer(std::dynamic_pointer_cast<BidCoSPeer>(j->second));
									if(peer->getTeamRemoteAddress() == queue->peer->getAddress() && j->second->getAddress() != queue->peer->getAddress())
									{
										//Needed to avoid deadlocks
										peers.push_back(peer);
									}
								}
								_peersMutex.unlock();
								for(std::vector<std::shared_ptr<BidCoSPeer>>::iterator j = peers.begin(); j != peers.end(); ++j)
								{
									GD::out.printInfo("Info: Adding device 0x" + BaseLib::HelperFunctions::getHexString((*j)->getAddress()) + " to team " + queue->peer->getTeamRemoteSerialNumber() + ".");
									addPeerToTeam(*j, (*j)->getTeamChannel(), (*j)->getTeamRemoteChannel(), queue->peer->getTeamRemoteSerialNumber());
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
							break;
						}
					}
				}
			}
		}
		else if(queue->getQueueType() == BidCoSQueueType::UNPAIRING)
		{
			if((sentPacket && sentPacket->messageType() == 0x01 && sentPacket->payload()->at(0) == 0x00 && sentPacket->payload()->at(1) == 0x06) || (sentPacket && sentPacket->messageType() == 0x11 && sentPacket->payload()->at(0) == 0x04 && sentPacket->payload()->at(1) == 0x00))
			{
				std::shared_ptr<BidCoSPeer> peer = getPeer(packet->senderAddress());
				if(peer) deletePeer(peer->getID());
			}
		}
		else if(queue->getQueueType() == BidCoSQueueType::SETAESKEY)
		{
			std::shared_ptr<BidCoSPeer> peer = getPeer(packet->senderAddress());
			if(peer && queue->getQueue()->size() < 2)
			{
				peer->setAESKeyIndex(peer->getPhysicalInterface()->getCurrentRFKeyIndex());
				aesKeyChanged = true;
				GD::out.printInfo("Info: Successfully changed AES key of peer " + std::to_string(peer->getID()) + ". Key index now is: " + std::to_string(peer->getAESKeyIndex()));
			}
		}
		queue->pop(); //Messages are not popped by default.
		if(aesKeyChanged)
		{
			std::shared_ptr<BidCoSPeer> peer = getPeer(packet->senderAddress());
			peer->getPhysicalInterface()->addPeer(peer->getPeerInfo());
		}

		if(queue->isEmpty())
		{
			std::shared_ptr<BidCoSPeer> peer = getPeer(packet->senderAddress());
			if(peer && !peer->getPairingComplete())
			{
				addHomegearFeatures(peer, -1, true);
				peer->setPairingComplete(true);
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

bool HomeMaticCentral::knowsDevice(std::string serialNumber)
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

bool HomeMaticCentral::knowsDevice(uint64_t id)
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::addDevice(std::string serialNumber)
{
	try
	{
		bool oldPairingModeState = _pairing;
		if(!_pairing) _pairing = true;

		if(serialNumber.empty()) return BaseLib::RPC::RPCVariable::createError(-2, "Serial number is empty.");
		if(serialNumber.size() > 10) return BaseLib::RPC::RPCVariable::createError(-2, "Serial number is too long.");

		std::vector<uint8_t> payload;
		payload.push_back(0x01);
		payload.push_back(serialNumber.size());
		payload.insert(payload.end(), serialNumber.begin(), serialNumber.end());
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(0, 0x84, 0x01, _address, 0, payload));

		int32_t i = 0;
		std::shared_ptr<BidCoSPeer> peer;
		while(!peer && i < 3)
		{
			packet->setMessageCounter(_messageCounter[0]);
			_messageCounter[0]++;
			std::thread t(&HomeMaticDevice::sendPacket, this, GD::defaultPhysicalInterface, packet, false);
			t.detach();
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));
			peer = getPeer(serialNumber);
			i++;
		}

		_pairing = oldPairingModeState;

		if(!peer)
		{
			return BaseLib::RPC::RPCVariable::createError(-1, "No response from device.");
		}
		else
		{
			return peer->getDeviceDescription(-1, std::map<std::string, bool>());
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
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::addLink(std::string senderSerialNumber, int32_t senderChannelIndex, std::string receiverSerialNumber, int32_t receiverChannelIndex, std::string name, std::string description)
{
	try
	{
		if(senderSerialNumber.empty()) return BaseLib::RPC::RPCVariable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return BaseLib::RPC::RPCVariable::createError(-2, "Given receiver address is empty.");
		std::shared_ptr<BidCoSPeer> sender = getPeer(senderSerialNumber);
		std::shared_ptr<BidCoSPeer> receiver = getPeer(receiverSerialNumber);
		if(!sender) return BaseLib::RPC::RPCVariable::createError(-2, "Sender device not found.");
		if(!receiver) return BaseLib::RPC::RPCVariable::createError(-2, "Receiver device not found.");
		return addLink(sender->getID(), senderChannelIndex, receiver->getID(), receiverChannelIndex, name, description);
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::addLink(uint64_t senderID, int32_t senderChannelIndex, uint64_t receiverID, int32_t receiverChannelIndex, std::string name, std::string description)
{
	try
	{
		if(senderID == 0) return BaseLib::RPC::RPCVariable::createError(-2, "Sender id is not set.");
		if(receiverID == 0) return BaseLib::RPC::RPCVariable::createError(-2, "Receiver is not set.");
		std::shared_ptr<BidCoSPeer> sender = getPeer(senderID);
		std::shared_ptr<BidCoSPeer> receiver = getPeer(receiverID);
		if(!sender) return BaseLib::RPC::RPCVariable::createError(-2, "Sender device not found.");
		if(!receiver) return BaseLib::RPC::RPCVariable::createError(-2, "Receiver device not found.");
		if(senderChannelIndex < 0) senderChannelIndex = 0;
		if(receiverChannelIndex < 0) receiverChannelIndex = 0;
		if(sender->rpcDevice->channels.find(senderChannelIndex) == sender->rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Sender channel not found.");
		if(receiver->rpcDevice->channels.find(receiverChannelIndex) == receiver->rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Receiver channel not found.");
		std::shared_ptr<BaseLib::RPC::DeviceChannel> senderChannel = sender->rpcDevice->channels.at(senderChannelIndex);
		std::shared_ptr<BaseLib::RPC::DeviceChannel> receiverChannel = receiver->rpcDevice->channels.at(receiverChannelIndex);
		if(senderChannel->linkRoles->sourceNames.size() == 0 || receiverChannel->linkRoles->targetNames.size() == 0) BaseLib::RPC::RPCVariable::createError(-6, "Link not supported.");
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

		std::shared_ptr<BaseLib::Systems::BasicPeer> senderPeer(new BaseLib::Systems::BasicPeer());
		senderPeer->address = sender->getAddress();
		senderPeer->channel = senderChannelIndex;
		senderPeer->id = sender->getID();
		senderPeer->serialNumber = sender->getSerialNumber();
		senderPeer->linkDescription = description;
		senderPeer->linkName = name;

		std::shared_ptr<BaseLib::Systems::BasicPeer> receiverPeer(new BaseLib::Systems::BasicPeer());
		receiverPeer->address = receiver->getAddress();
		receiverPeer->channel = receiverChannelIndex;
		receiverPeer->id = receiver->getID();
		receiverPeer->serialNumber = receiver->getSerialNumber();
		receiverPeer->linkDescription = description;
		receiverPeer->linkName = name;

		sender->addPeer(senderChannelIndex, receiverPeer);
		receiver->addPeer(receiverChannelIndex, senderPeer);

		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, sender->getPhysicalInterface(), BidCoSQueueType::CONFIG, sender->getAddress());
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(sender->getPhysicalInterface(), BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		std::vector<uint8_t> payload;

		uint8_t configByte = 0xA0;
		if(sender->getRXModes() & BaseLib::RPC::Device::RXModes::burst) configByte |= 0x10;
		std::shared_ptr<BaseLib::Systems::BasicPeer> hiddenPeer(sender->getHiddenPeer(senderChannelIndex));
		if(hiddenPeer)
		{
			sender->removePeer(senderChannelIndex, hiddenPeer->address, hiddenPeer->channel);
			if(!sender->getHiddenPeerDevice())
			{
				std::shared_ptr<HomeMaticDevice> device = getDevice(hiddenPeer->address);
				if(device && !device->isCentral()) GD::family->remove(device->getID());
			}

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

		sender->pendingBidCoSQueues->push(pendingQueue);
		sender->serviceMessages->setConfigPending(true);

		if(senderChannel->parameterSets.find(BaseLib::RPC::ParameterSet::Type::Enum::link) != senderChannel->parameterSets.end())
		{
			std::shared_ptr<BaseLib::RPC::RPCVariable> paramset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
			std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>* linkConfig = &sender->linksCentral.at(senderChannelIndex).at(receiver->getAddress()).at(receiverChannelIndex);
			for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator i = linkConfig->begin(); i != linkConfig->end(); ++i)
			{
				paramset->structValue->insert(BaseLib::RPC::RPCStructElement(i->first, i->second.rpcParameter->convertFromPacket(i->second.data)));
			}
			//putParamset pushes the packets on pendingQueues, but does not send immediately
			sender->putParamset(senderChannelIndex, BaseLib::RPC::ParameterSet::Type::Enum::link, receiverID, receiverChannelIndex, paramset, true);

			if(!receiverChannel->enforceLinks.empty())
			{
				std::shared_ptr<BaseLib::RPC::ParameterSet> linkset = senderChannel->parameterSets.at(BaseLib::RPC::ParameterSet::Type::Enum::link);
				paramset.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
				for(std::vector<std::shared_ptr<BaseLib::RPC::EnforceLink>>::iterator i = receiverChannel->enforceLinks.begin(); i != receiverChannel->enforceLinks.end(); ++i)
				{
					std::shared_ptr<BaseLib::RPC::Parameter> parameter = linkset->getParameter((*i)->id);
					if(parameter)
					{
						paramset->structValue->insert(BaseLib::RPC::RPCStructElement((*i)->id, (*i)->getValue(parameter->logicalParameter->type)));
					}
				}
				//putParamset pushes the packets on pendingQueues, but does not send immediately
				sender->putParamset(senderChannelIndex, BaseLib::RPC::ParameterSet::Type::Enum::link, receiverID, receiverChannelIndex, paramset, true);
			}
		}

		queue->push(sender->pendingBidCoSQueues);

		raiseRPCUpdateDevice(sender->getID(), senderChannelIndex, sender->getSerialNumber() + ":" + std::to_string(senderChannelIndex), 1);

		int32_t waitIndex = 0;
		while(_bidCoSQueueManager.get(sender->getAddress()) && waitIndex < 20)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			waitIndex++;
		}

		queue = _bidCoSQueueManager.createQueue(this, receiver->getPhysicalInterface(), BidCoSQueueType::CONFIG, receiver->getAddress());
		pendingQueue.reset(new BidCoSQueue(receiver->getPhysicalInterface(), BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		configByte = 0xA0;
		if(receiver->getRXModes() & BaseLib::RPC::Device::RXModes::burst) configByte |= 0x10;

		hiddenPeer = receiver->getHiddenPeer(receiverChannelIndex);
		if(hiddenPeer)
		{
			sender->removePeer(receiverChannelIndex, hiddenPeer->address, hiddenPeer->channel);
			if(!sender->getHiddenPeerDevice())
			{
				std::shared_ptr<HomeMaticDevice> device = getDevice(hiddenPeer->address);
				if(device && !device->isCentral()) GD::family->remove(device->getID());
			}

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

		receiver->pendingBidCoSQueues->push(pendingQueue);
		receiver->serviceMessages->setConfigPending(true);

		if(receiverChannel->parameterSets.find(BaseLib::RPC::ParameterSet::Type::Enum::link) != receiverChannel->parameterSets.end())
		{
			std::shared_ptr<BaseLib::RPC::RPCVariable> paramset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
			std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>* linkConfig = &receiver->linksCentral.at(receiverChannelIndex).at(sender->getAddress()).at(senderChannelIndex);
			for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator i = linkConfig->begin(); i != linkConfig->end(); ++i)
			{
				paramset->structValue->insert(BaseLib::RPC::RPCStructElement(i->first, i->second.rpcParameter->convertFromPacket(i->second.data)));
			}
			//putParamset pushes the packets on pendingQueues, but does not send immediately
			receiver->putParamset(receiverChannelIndex, BaseLib::RPC::ParameterSet::Type::Enum::link, senderID, senderChannelIndex, paramset, true);

			if(!senderChannel->enforceLinks.empty())
			{
				std::shared_ptr<BaseLib::RPC::ParameterSet> linkset = receiverChannel->parameterSets.at(BaseLib::RPC::ParameterSet::Type::Enum::link);
				paramset.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
				for(std::vector<std::shared_ptr<BaseLib::RPC::EnforceLink>>::iterator i = senderChannel->enforceLinks.begin(); i != senderChannel->enforceLinks.end(); ++i)
				{
					std::shared_ptr<BaseLib::RPC::Parameter> parameter = linkset->getParameter((*i)->id);
					if(parameter)
					{
						paramset->structValue->insert(BaseLib::RPC::RPCStructElement((*i)->id, (*i)->getValue(parameter->logicalParameter->type)));
					}
				}
				//putParamset pushes the packets on pendingQueues, but does not send immediately
				receiver->putParamset(receiverChannelIndex, BaseLib::RPC::ParameterSet::Type::Enum::link, senderID, senderChannelIndex, paramset, true);
			}
		}

		queue->push(receiver->pendingBidCoSQueues);

		raiseRPCUpdateDevice(receiver->getID(), receiverChannelIndex, receiver->getSerialNumber() + ":" + std::to_string(receiverChannelIndex), 1);

		waitIndex = 0;
		while(_bidCoSQueueManager.get(receiver->getAddress()) && waitIndex < 20)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			waitIndex++;
		}

		//Check, if channel is part of a group and if that's the case add link for the grouped channel
		int32_t channelGroupedWith = sender->getChannelGroupedWith(senderChannelIndex);
		//I'm assuming that senderChannelIndex is always the first of the two grouped channels
		if(channelGroupedWith > senderChannelIndex)
		{
			return addLink(senderID, channelGroupedWith, receiverID, receiverChannelIndex, name, description);
		}
		else
		{
			return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
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
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::removeLink(std::string senderSerialNumber, int32_t senderChannelIndex, std::string receiverSerialNumber, int32_t receiverChannelIndex)
{
	try
	{
		if(senderSerialNumber.empty()) return BaseLib::RPC::RPCVariable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return BaseLib::RPC::RPCVariable::createError(-2, "Given receiver address is empty.");
		std::shared_ptr<BidCoSPeer> sender = getPeer(senderSerialNumber);
		std::shared_ptr<BidCoSPeer> receiver = getPeer(receiverSerialNumber);
		if(!sender) return BaseLib::RPC::RPCVariable::createError(-2, "Sender device not found.");
		if(!receiver) return BaseLib::RPC::RPCVariable::createError(-2, "Receiver device not found.");
		return removeLink(sender->getID(), senderChannelIndex, receiver->getID(), receiverChannelIndex);
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::removeLink(uint64_t senderID, int32_t senderChannelIndex, uint64_t receiverID, int32_t receiverChannelIndex)
{
	try
	{
		if(senderID == 0) return BaseLib::RPC::RPCVariable::createError(-2, "Sender id is not set.");
		if(receiverID == 0) return BaseLib::RPC::RPCVariable::createError(-2, "Receiver id is not set.");
		std::shared_ptr<BidCoSPeer> sender = getPeer(senderID);
		std::shared_ptr<BidCoSPeer> receiver = getPeer(receiverID);
		if(!sender) return BaseLib::RPC::RPCVariable::createError(-2, "Sender device not found.");
		if(!receiver) return BaseLib::RPC::RPCVariable::createError(-2, "Receiver device not found.");
		if(senderChannelIndex < 0) senderChannelIndex = 0;
		if(receiverChannelIndex < 0) receiverChannelIndex = 0;
		std::string senderSerialNumber = sender->getSerialNumber();
		std::string receiverSerialNumber = receiver->getSerialNumber();
		if(sender->rpcDevice->channels.find(senderChannelIndex) == sender->rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Sender channel not found.");
		if(receiver->rpcDevice->channels.find(receiverChannelIndex) == receiver->rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Receiver channel not found.");
		if(!sender->getPeer(senderChannelIndex, receiver->getAddress()) && !receiver->getPeer(receiverChannelIndex, sender->getAddress())) return BaseLib::RPC::RPCVariable::createError(-6, "Devices are not paired to each other.");

		sender->removePeer(senderChannelIndex, receiver->getAddress(), receiverChannelIndex);
		receiver->removePeer(receiverChannelIndex, sender->getAddress(), senderChannelIndex);

		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, sender->getPhysicalInterface(), BidCoSQueueType::CONFIG, sender->getAddress());
		std::shared_ptr<BidCoSQueue> pendingQueue(new BidCoSQueue(sender->getPhysicalInterface(), BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		uint8_t configByte = 0xA0;
		if(sender->getRXModes() & BaseLib::RPC::Device::RXModes::burst) configByte |= 0x10;

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

		sender->pendingBidCoSQueues->push(pendingQueue);
		sender->serviceMessages->setConfigPending(true);
		queue->push(sender->pendingBidCoSQueues);

		addHomegearFeatures(sender, senderChannelIndex, false);

		raiseRPCUpdateDevice(sender->getID(), senderChannelIndex, senderSerialNumber + ":" + std::to_string(senderChannelIndex), 1);

		int32_t waitIndex = 0;
		while(_bidCoSQueueManager.get(sender->getAddress()) && waitIndex < 20)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			waitIndex++;
		}

		queue = _bidCoSQueueManager.createQueue(this, receiver->getPhysicalInterface(), BidCoSQueueType::CONFIG, receiver->getAddress());
		pendingQueue.reset(new BidCoSQueue(receiver->getPhysicalInterface(), BidCoSQueueType::CONFIG));
		pendingQueue->noSending = true;

		configByte = 0xA0;
		if(receiver->getRXModes() & BaseLib::RPC::Device::RXModes::burst) configByte |= 0x10;

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

		receiver->pendingBidCoSQueues->push(pendingQueue);
		receiver->serviceMessages->setConfigPending(true);
		queue->push(receiver->pendingBidCoSQueues);

		addHomegearFeatures(receiver, receiverChannelIndex, false);

		raiseRPCUpdateDevice(receiver->getID(), receiverChannelIndex, receiverSerialNumber + ":" + std::to_string(receiverChannelIndex), 1);

		waitIndex = 0;
		while(_bidCoSQueueManager.get(receiver->getAddress()) && waitIndex < 20)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			waitIndex++;
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::deleteDevice(std::string serialNumber, int32_t flags)
{
	try
	{
		if(serialNumber.empty()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		if(serialNumber[0] == '*') return BaseLib::RPC::RPCVariable::createError(-2, "Cannot delete virtual device.");
		std::shared_ptr<BidCoSPeer> peer = getPeer(serialNumber);
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::deleteDevice(uint64_t peerID, int32_t flags)
{
	try
	{
		if(peerID == 0) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		if(peerID & 0x80000000) return BaseLib::RPC::RPCVariable::createError(-2, "Cannot delete virtual device.");
		std::shared_ptr<BidCoSPeer> peer = getPeer(peerID);
		if(!peer) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
		uint64_t id = peer->getID();

		bool defer = flags & 0x04;
		bool force = flags & 0x02;
		//Reset
		if(flags & 0x01)
		{
			std::thread t(&HomeMaticCentral::reset, this, id, defer);
			t.detach();
		}
		else
		{
			std::thread t(&HomeMaticCentral::unpair, this, id, defer);
			t.detach();
		}
		//Force delete
		if(force) deletePeer(peer->getID());
		else
		{
			int32_t waitIndex = 0;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			while(_bidCoSQueueManager.get(peer->getAddress()) && peerExists(id) && waitIndex < 20)
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::listTeams()
{
	try
	{
		std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));

		std::vector<std::shared_ptr<BidCoSPeer>> peers;
		//Copy all peers first, because listTeams takes very long and we don't want to lock _peersMutex too long
		_peersMutex.lock();
		for(std::unordered_map<std::string, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersBySerial.begin(); i != _peersBySerial.end(); ++i)
		{
			peers.push_back(std::dynamic_pointer_cast<BidCoSPeer>(i->second));
		}
		_peersMutex.unlock();

		for(std::vector<std::shared_ptr<BidCoSPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
		{
			//listTeams really needs a lot of ressources, so wait a little bit after each device
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::string serialNumber = (*i)->getSerialNumber();
			if(serialNumber.empty() || serialNumber.at(0) != '*') continue;
			std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> descriptions = (*i)->getDeviceDescriptions(true, std::map<std::string, bool>());
			if(!descriptions) continue;
			for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator j = descriptions->begin(); j != descriptions->end(); ++j)
			{
				array->arrayValue->push_back(*j);
			}
		}
		return array;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::setTeam(std::string serialNumber, int32_t channel, std::string teamSerialNumber, int32_t teamChannel, bool force, bool burst)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(serialNumber));
		if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		uint64_t teamID = 0;
		if(!teamSerialNumber.empty())
		{
			std::shared_ptr<BidCoSPeer> team(getPeer(teamSerialNumber));
			if(!team) return BaseLib::RPC::RPCVariable::createError(-2, "Team does not exist.");
			teamID = team->getID();
		}
		return setTeam(peer->getID(), channel, teamID, teamChannel);
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::setTeam(uint64_t peerID, int32_t channel, uint64_t teamID, int32_t teamChannel, bool force, bool burst)
{
	try
	{
		if(channel < 0) channel = 0;
		if(teamChannel < 0) teamChannel = 0;
		std::shared_ptr<BidCoSPeer> peer(getPeer(peerID));
		if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		if(peer->rpcDevice->channels.find(channel) == peer->rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(!peer->rpcDevice->channels[channel]->hasTeam) return BaseLib::RPC::RPCVariable::createError(-6, "Channel does not support teams.");
		int32_t oldTeamAddress = peer->getTeamRemoteAddress();
		int32_t oldTeamChannel = peer->getTeamRemoteChannel();
		if(oldTeamChannel < 0) oldTeamChannel = 0;
		if(teamID == 0) //Reset team to default
		{
			if(!force && !peer->getTeamRemoteSerialNumber().empty() && peer->getTeamRemoteSerialNumber().substr(1) == peer->getSerialNumber() && peer->getTeamChannel() == channel)
			{
				//Team already is default
				return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
			}

			int32_t newChannel = -1;
			//Get first channel which has a team
			for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
			{
				if(i->second->hasTeam)
				{
					newChannel = i->first;
					break;
				}
			}
			if(newChannel < 0) return BaseLib::RPC::RPCVariable::createError(-6, "There are no team channels for this device.");

			resetTeam(peer, newChannel);
		}
		else //Set new team
		{
			//Don't create team if not existent!
			std::shared_ptr<BidCoSPeer> team = getPeer(teamID);
			if(!team) return BaseLib::RPC::RPCVariable::createError(-2, "Team does not exist.");
			if(!force && !peer->getTeamRemoteSerialNumber().empty() && peer->getTeamRemoteSerialNumber() == team->getSerialNumber() && peer->getTeamChannel() == channel && peer->getTeamRemoteChannel() == teamChannel)
			{
				//Peer already is member of this team
				return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
			}
			if(team->rpcDevice->channels.find(teamChannel) == team->rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown team channel.");
			if(team->rpcDevice->channels[teamChannel]->teamTag != peer->rpcDevice->channels[channel]->teamTag) return BaseLib::RPC::RPCVariable::createError(-6, "Peer channel is not compatible to team channel.");

			addPeerToTeam(peer, channel, teamChannel, team->getSerialNumber());
		}
		std::shared_ptr<BidCoSQueue> queue(new BidCoSQueue(peer->getPhysicalInterface(), BidCoSQueueType::CONFIG));
		queue->noSending = true;

		uint8_t configByte = 0xA0;
		if(burst && (peer->getRXModes() & BaseLib::RPC::Device::RXModes::burst)) configByte |= 0x10;

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
			queue->push(getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
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
		queue->push(getMessages()->find(DIRECTIONIN, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));

		peer->pendingBidCoSQueues->push(queue);
		peer->serviceMessages->setConfigPending(true);
		if((peer->getRXModes() & BaseLib::RPC::Device::RXModes::Enum::always) || (peer->getRXModes() & BaseLib::RPC::Device::RXModes::Enum::burst)) enqueuePendingQueues(peer->getAddress());
		else GD::out.printDebug("Debug: Packet was queued and will be sent with next wake me up packet.");
		raiseRPCUpdateDevice(peer->getID(), channel, peer->getSerialNumber() + ":" + std::to_string(channel), 0);
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

/*std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::getDeviceDescriptionCentral()
{
	try
	{
		std::shared_ptr<BaseLib::RPC::RPCVariable> description(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_deviceID))));
		description->structValue->insert(BaseLib::RPC::RPCStructElement("ADDRESS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_serialNumber))));

		std::shared_ptr<BaseLib::RPC::RPCVariable> variable = std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		description->structValue->insert(BaseLib::RPC::RPCStructElement("CHILDREN", variable));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("FIRMWARE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::string(VERSION)))));

		int32_t uiFlags = (int32_t)BaseLib::RPC::Device::UIFlags::dontdelete | (int32_t)BaseLib::RPC::Device::UIFlags::visible;
		description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(uiFlags))));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("INTERFACE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_serialNumber))));

		variable = std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		description->structValue->insert(BaseLib::RPC::RPCStructElement("PARAMSETS", variable));
		variable->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::string("MASTER")))); //Always MASTER

		description->structValue->insert(BaseLib::RPC::RPCStructElement("PARENT", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::string("")))));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("PHYSICAL_ADDRESS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::to_string(_address)))));
		description->structValue->insert(BaseLib::RPC::RPCStructElement("RF_ADDRESS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::to_string(_address)))));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("ROAMING", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(0))));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::string("Homegear HomeMatic BidCoS Central")))));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("VERSION", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((int32_t)10))));

		return description;
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
}*/

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::getDeviceInfo(uint64_t id, std::map<std::string, bool> fields)
{
	try
	{
		if(id > 0)
		{
			std::shared_ptr<BidCoSPeer> peer(getPeer(id));
			if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");

			return peer->getDeviceInfo(fields);
		}
		else
		{
			std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));

			std::vector<std::shared_ptr<BidCoSPeer>> peers;
			//Copy all peers first, because listDevices takes very long and we don't want to lock _peersMutex too long
			_peersMutex.lock();
			for(std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersByID.begin(); i != _peersByID.end(); ++i)
			{
				peers.push_back(std::dynamic_pointer_cast<BidCoSPeer>(i->second));
			}
			_peersMutex.unlock();

			for(std::vector<std::shared_ptr<BidCoSPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::getInstallMode()
{
	try
	{
		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(_timeLeftInPairingMode));
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

void HomeMaticCentral::pairingModeTimer(int32_t duration, bool debugOutput)
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::putParamset(std::string serialNumber, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> paramset)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(serialNumber));
		if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		uint64_t remoteID = 0;
		if(!remoteSerialNumber.empty())
		{
			std::shared_ptr<BidCoSPeer> remotePeer(getPeer(remoteSerialNumber));
			if(!remotePeer) return BaseLib::RPC::RPCVariable::createError(-3, "Remote peer is unknown.");
			remoteID = remotePeer->getID();
		}
		std::shared_ptr<BaseLib::RPC::RPCVariable> result = peer->putParamset(channel, type, remoteID, remoteChannel, paramset);
		if(result->errorStruct) return result;
		int32_t waitIndex = 0;
		while(_bidCoSQueueManager.get(peer->getAddress()) && waitIndex < 20)
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::putParamset(uint64_t peerID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> paramset)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(peerID));
		if(!peer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown device.");
		std::shared_ptr<BaseLib::RPC::RPCVariable> result = peer->putParamset(channel, type, remoteID, remoteChannel, paramset);
		if(result->errorStruct) return result;
		int32_t waitIndex = 0;
		while(_bidCoSQueueManager.get(peer->getAddress()) && waitIndex < 20)
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::setInstallMode(bool on, uint32_t duration, bool debugOutput)
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::updateFirmware(std::vector<uint64_t> ids, bool manual)
{
	try
	{
		if(_updateMode || _bl->deviceUpdateInfo.currentDevice > 0) return BaseLib::RPC::RPCVariable::createError(-32500, "Central is already already updating a device. Please wait until the current update is finished.");
		if(_updateFirmwareThread.joinable()) _updateFirmwareThread.join();
		_updateFirmwareThread = std::thread(&HomeMaticCentral::updateFirmwares, this, ids, manual);
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

std::shared_ptr<BaseLib::RPC::RPCVariable> HomeMaticCentral::setInterface(uint64_t peerID, std::string interfaceID)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(getPeer(peerID));
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
}
