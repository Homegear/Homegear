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

#include "HMWiredCentral.h"
#include "../GD.h"

namespace HMWired {

HMWiredCentral::HMWiredCentral(IDeviceEventSink* eventHandler) : HMWiredDevice(eventHandler), BaseLib::Systems::Central(GD::bl, this)
{
	init();
}

HMWiredCentral::HMWiredCentral(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : HMWiredDevice(deviceID, serialNumber, address, eventHandler), Central(GD::bl, this)
{
	init();
}

HMWiredCentral::~HMWiredCentral()
{
	dispose();

	_updateFirmwareThreadMutex.lock();
	if(_updateFirmwareThread.joinable()) _updateFirmwareThread.join();
	_updateFirmwareThreadMutex.unlock();
}

void HMWiredCentral::init()
{
	try
	{
		HMWiredDevice::init();

		_deviceType = (uint32_t)DeviceType::HMWIREDCENTRAL;
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

void HMWiredCentral::worker()
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
				std::shared_ptr<HMWiredPeer> peer(getPeer(lastPeer));
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

bool HMWiredCentral::onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(_disposing) return false;
		HMWiredDevice::onPacketReceived(senderID, packet);
		std::shared_ptr<HMWiredPacket> hmWiredPacket(std::dynamic_pointer_cast<HMWiredPacket>(packet));
		if(!hmWiredPacket) return false;
		std::shared_ptr<HMWiredPeer> peer(getPeer(hmWiredPacket->senderAddress()));
		if(peer) peer->packetReceived(hmWiredPacket);
		else if(hmWiredPacket->messageType() == 0x41 && !_pairing) handleAnnounce(hmWiredPacket);
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

void HMWiredCentral::deletePeer(uint64_t id)
{
	try
	{
		std::shared_ptr<HMWiredPeer> peer(getPeer(id));
		if(!peer) return;
		peer->deleting = true;
		std::shared_ptr<BaseLib::RPC::Variable> deviceAddresses(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		deviceAddresses->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(peer->getSerialNumber())));
		for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
		{
			deviceAddresses->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(peer->getSerialNumber() + ":" + std::to_string(i->first))));
		}
		std::shared_ptr<BaseLib::RPC::Variable> deviceInfo(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
		deviceInfo->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)peer->getID()))));
		std::shared_ptr<BaseLib::RPC::Variable> channels(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		deviceInfo->structValue->insert(BaseLib::RPC::RPCStructElement("CHANNELS", channels));
		for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
		{
			channels->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(i->first)));
		}
		raiseRPCDeleteDevices(deviceAddresses, deviceInfo);
		peer->deleteFromDatabase();
		_peersMutex.lock();
		if(_peersBySerial.find(peer->getSerialNumber()) != _peersBySerial.end()) _peersBySerial.erase(peer->getSerialNumber());
		if(_peers.find(peer->getAddress()) != _peers.end()) _peers.erase(peer->getAddress());
		if(_peersByID.find(id) != _peersByID.end()) _peersByID.erase(id);
		_peersMutex.unlock();
		GD::out.printMessage("Removed HomeMatic Wired peer " + std::to_string(peer->getID()));
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

std::string HMWiredCentral::handleCLICommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;
		if(_currentPeer)
		{
			if(command == "unselect" || command == "u")
			{
				_currentPeer.reset();
				return "Peer unselected.\n";
			}
			return _currentPeer->handleCLICommand(command);
		}
		if(command == "help" || command == "h")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "peers list (ls)\t\tList all peers" << std::endl;
			stringStream << "peers reset (pr)\tUnpair a peer and reset it to factory defaults" << std::endl;
			stringStream << "peers select (ps)\tSelect a peer" << std::endl;
			stringStream << "peers setname (pn)\tName a peer" << std::endl;
			stringStream << "peers unpair (pup)\tUnpair a peer" << std::endl;
			stringStream << "peers update (pud)\tUpdates a peer to the newest firmware version" << std::endl;
			stringStream << "search (sp)\t\tSearches for new devices on the bus" << std::endl;
			stringStream << "unselect (u)\t\tUnselect this device" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 6, "search") == 0 || command.compare(0, 2, "sp") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'p') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help")
					{
						stringStream << "Description: This command searches for new devices on the bus." << std::endl;
						stringStream << "Usage: search" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			std::shared_ptr<BaseLib::RPC::Variable> result = searchDevices();
			if(result->errorStruct) stringStream << "Error: " << result->structValue->at("faultString")->stringValue << std::endl;
			else stringStream << "Search completed successfully." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 12, "peers unpair") == 0 || command.compare(0, 3, "pup") == 0)
		{
			uint64_t peerID = 0;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'u') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					peerID = BaseLib::Math::getNumber(element, false);
					if(peerID == 0) return "Invalid id.\n";
				}
				index++;
			}
			if(index == 1 + offset)
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
				deletePeer(peerID);
			}
			return stringStream.str();
		}
		else if(command.compare(0, 11, "peers reset") == 0 || command.compare(0, 3, "prs") == 0)
		{
			uint64_t peerID = 0;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'r') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					peerID = BaseLib::Math::getNumber(element, false);
					if(peerID == 0) return "Invalid id.\n";
				}
				index++;
			}
			if(index == 1 + offset)
			{
				stringStream << "Description: This command resets and unpairs a peer." << std::endl;
				stringStream << "Usage: peers reset PEERID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to reset. Example: 513" << std::endl;
				return stringStream.str();
			}

			std::shared_ptr<HMWiredPeer> peer = getPeer(peerID);
			if(!peer) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				if(_currentPeer && _currentPeer->getID() == peerID) _currentPeer.reset();
				stringStream << "Resetting peer " << std::to_string(peerID) << std::endl;
				peer->reset();
				deletePeer(peerID);
			}
			return stringStream.str();
		}
		else if(command.compare(0, 10, "peers list") == 0 || command.compare(0, 2, "pl") == 0 || command.compare(0, 2, "ls") == 0)
		{
			try
			{
				std::string filterType;
				std::string filterValue;

				std::stringstream stream(command);
				std::string element;
				int32_t offset = (command.at(1) == 'l' || command.at(1) == 's') ? 0 : 1;
				int32_t index = 0;
				while(std::getline(stream, element, ' '))
				{
					if(index < 1 + offset)
					{
						index++;
						continue;
					}
					else if(index == 1 + offset)
					{
						if(element == "help")
						{
							index = -1;
							break;
						}
						filterType = BaseLib::HelperFunctions::toLower(element);
					}
					else if(index == 2 + offset)
					{
						filterValue = element;
						if(filterType == "name") BaseLib::HelperFunctions::toLower(filterValue);
					}
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
					stringStream << "  NAME: Filter by name." << std::endl;
					stringStream << "      FILTERVALUE: The part of the name to search for (e. g. \"1st floor\")." << std::endl;
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
				bool firmwareUpdates = false;
				std::string bar(" │ ");
				const int32_t idWidth = 8;
				const int32_t nameWidth = 25;
				const int32_t addressWidth = 8;
				const int32_t serialWidth = 13;
				const int32_t typeWidth1 = 4;
				const int32_t typeWidth2 = 25;
				const int32_t firmwareWidth = 8;
				const int32_t unreachWidth = 7;
				std::string nameHeader("Name");
				nameHeader.resize(nameWidth, ' ');
				std::string typeStringHeader("Type String");
				typeStringHeader.resize(typeWidth2, ' ');
				stringStream << std::setfill(' ')
					<< std::setw(idWidth) << "ID" << bar
					<< nameHeader << bar
					<< std::setw(addressWidth) << "Address" << bar
					<< std::setw(serialWidth) << "Serial Number" << bar
					<< std::setw(typeWidth1) << "Type" << bar
					<< typeStringHeader << bar
					<< std::setw(firmwareWidth) << "Firmware" << bar
					<< std::setw(unreachWidth) << "Unreach"
					<< std::endl;
				stringStream << "─────────┼───────────────────────────┼──────────┼───────────────┼──────┼───────────────────────────┼──────────┼────────" << std::endl;
				stringStream << std::setfill(' ')
					<< std::setw(idWidth) << " " << bar
					<< std::setw(nameWidth) << " " << bar
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
						uint64_t id = BaseLib::Math::getNumber(filterValue, true);
						if(i->second->getID() != id) continue;
					}
					else if(filterType == "name")
					{
						std::string name = i->second->getName();
						if((signed)BaseLib::HelperFunctions::toLower(name).find(filterValue) == (signed)std::string::npos) continue;
					}
					else if(filterType == "address")
					{
						int32_t address = BaseLib::Math::getNumber(filterValue, true);
						if(i->second->getAddress() != address) continue;
					}
					else if(filterType == "serial")
					{
						if(i->second->getSerialNumber() != filterValue) continue;
					}
					else if(filterType == "type")
					{
						int32_t deviceType = BaseLib::Math::getNumber(filterValue, true);
						if((int32_t)i->second->getDeviceType().type() != deviceType) continue;
					}
					else if(filterType == "unreach")
					{
						if(i->second->serviceMessages)
						{
							if(!i->second->serviceMessages->getUnreach()) continue;
						}
					}

					stringStream << std::setw(idWidth) << std::setfill(' ') << std::to_string(i->second->getID()) << bar;
					std::string name = i->second->getName();
					size_t nameSize = BaseLib::HelperFunctions::utf8StringSize(name);
					if(nameSize > (unsigned)nameWidth)
					{
						name = BaseLib::HelperFunctions::utf8Substring(name, 0, nameWidth - 3);
						name += "...";
					}
					else name.resize(nameWidth + (name.size() - nameSize), ' ');
					stringStream << name << bar
						<< std::setw(addressWidth) << BaseLib::HelperFunctions::getHexString(i->second->getAddress(), 8) << bar
						<< std::setw(serialWidth) << i->second->getSerialNumber() << bar
						<< std::setw(typeWidth1) << BaseLib::HelperFunctions::getHexString(i->second->getDeviceType().type(), 4) << bar;
					if(i->second->rpcDevice)
					{
						std::shared_ptr<BaseLib::RPC::DeviceType> type = i->second->rpcDevice->getType(i->second->getDeviceType(), i->second->getFirmwareVersion());
						std::string typeID;
						if(type) typeID = type->id;
						if(typeID.size() > (unsigned)typeWidth2)
						{
							typeID.resize(typeWidth2 - 3);
							typeID += "...";
						}
						stringStream << std::setw(typeWidth2) << typeID << bar;
					}
					else stringStream << std::setw(typeWidth2) << " " << bar;
					if(i->second->getFirmwareVersion() == 0) stringStream << std::setfill(' ') << std::setw(firmwareWidth) << "?" << bar;
					else if(i->second->firmwareUpdateAvailable())
					{
						stringStream << std::setfill(' ') << std::setw(firmwareWidth) << ("*" + BaseLib::HelperFunctions::getHexString(i->second->getFirmwareVersion() >> 8) + "." + std::to_string(i->second->getFirmwareVersion() & 0xFF)) << bar;
						firmwareUpdates = true;
					}
					else stringStream << std::setfill(' ') << std::setw(firmwareWidth) << (BaseLib::HelperFunctions::getHexString(i->second->getFirmwareVersion() >> 8) + "." + std::to_string(i->second->getFirmwareVersion() & 0xFF)) << bar;
					if(i->second->serviceMessages)
					{
						std::string unreachable(i->second->serviceMessages->getUnreach() ? "Yes" : "No");
						stringStream << std::setw(unreachWidth) << unreachable;
					}
					stringStream << std::endl << std::dec;
				}
				_peersMutex.unlock();
				stringStream << "─────────┴───────────────────────────┴──────────┴───────────────┴──────┴───────────────────────────┴──────────┴────────" << std::endl;
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
		else if(command.compare(0, 13, "peers setname") == 0 || command.compare(0, 2, "pn") == 0)
		{
			uint64_t peerID = 0;
			std::string name;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'n') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					else
					{
						peerID = BaseLib::Math::getNumber(element, false);
						if(peerID == 0) return "Invalid id.\n";
					}
				}
				else if(index == 2 + offset) name = element;
				else name += ' ' + element;
				index++;
			}
			if(index == 1 + offset)
			{
				stringStream << "Description: This command sets or changes the name of a peer to identify it more easily." << std::endl;
				stringStream << "Usage: peers setname PEERID NAME" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to set the name for. Example: 513" << std::endl;
				stringStream << "  NAME:\tThe name to set. Example: \"1st floor light switch\"." << std::endl;
				return stringStream.str();
			}

			if(!peerExists(peerID)) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				std::shared_ptr<HMWiredPeer> peer = getPeer(peerID);
				peer->setName(name);
				stringStream << "Name set to \"" << name << "\"." << std::endl;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 12, "peers update") == 0 || command.compare(0, 3, "pud") == 0)
		{
			uint64_t peerID;
			bool all = false;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 'u') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					else if(element == "all") all = true;
					else
					{
						peerID = BaseLib::Math::getNumber(element, false);
						if(peerID == 0) return "Invalid id.\n";
					}
				}
				index++;
			}
			if(index == 1 + offset)
			{
				stringStream << "Description: This command updates one or all peers to the newest firmware version available in \"" << _bl->settings.firmwarePath() << "\"." << std::endl;
				stringStream << "Usage: peers update PEERID" << std::endl;
				stringStream << "       peers update all" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to update. Example: 513" << std::endl;
				return stringStream.str();
			}

			std::shared_ptr<BaseLib::RPC::Variable> result;
			std::vector<uint64_t> ids;
			if(all)
			{
				_peersMutex.lock();
				for(std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersByID.begin(); i != _peersByID.end(); ++i)
				{
					if(i->second->firmwareUpdateAvailable()) ids.push_back(i->first);
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
				std::shared_ptr<HMWiredPeer> peer = getPeer(peerID);
				if(!peer->firmwareUpdateAvailable())
				{
					stringStream << "Peer is up to date." << std::endl;
					return stringStream.str();
				}
				ids.push_back(peerID);
				result = updateFirmware(ids, false);
			}
			if(!result) stringStream << "Unknown error." << std::endl;
			else if(result->errorStruct) stringStream << result->structValue->at("faultString")->stringValue << std::endl;
			else stringStream << "Started firmware update(s)... This might take a long time. Use the RPC function \"getUpdateProgress\" or see the log for details." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 12, "peers select") == 0 || command.compare(0, 2, "ps") == 0)
		{
			uint64_t id;

			std::stringstream stream(command);
			std::string element;
			int32_t offset = (command.at(1) == 's') ? 0 : 1;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 1 + offset)
				{
					index++;
					continue;
				}
				else if(index == 1 + offset)
				{
					if(element == "help") break;
					id = BaseLib::Math::getNumber(element, false);
					if(id == 0) return "Invalid id.\n";
				}
				index++;
			}
			if(index == 1 + offset)
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
				stringStream << "Peer with id " << std::hex << std::to_string(id) << " and device type 0x" << _bl->hf.getHexString(_currentPeer->getDeviceType().type()) << " selected." << std::dec << std::endl;
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

std::shared_ptr<HMWiredPeer> HMWiredCentral::createPeer(int32_t address, int32_t firmwareVersion, BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber, bool save)
{
	try
	{
		std::shared_ptr<HMWiredPeer> peer(new HMWiredPeer(_deviceID, true, this));
		peer->setAddress(address);
		peer->setFirmwareVersion(firmwareVersion);
		peer->setDeviceType(deviceType);
		peer->setSerialNumber(serialNumber);
		peer->rpcDevice = GD::rpcDevices.find(deviceType, firmwareVersion, -1);
		if(!peer->rpcDevice) return std::shared_ptr<HMWiredPeer>();
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
    return std::shared_ptr<HMWiredPeer>();
}

void HMWiredCentral::updateFirmwares(std::vector<uint64_t> ids)
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
			updateFirmware(*i);
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

void HMWiredCentral::updateFirmware(uint64_t id)
{
	try
	{
		if(_updateMode) return;
		std::shared_ptr<HMWiredPeer> peer = getPeer(id);
		if(!peer) return;
		_updateMode = true;
		_updateMutex.lock();
		std::string filenamePrefix = BaseLib::HelperFunctions::getHexString((int32_t)BaseLib::Systems::DeviceFamilies::HomeMaticWired, 4) + "." + BaseLib::HelperFunctions::getHexString(peer->getDeviceType().type(), 8);
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
		std::string oldVersionString = BaseLib::HelperFunctions::getHexString(peer->getFirmwareVersion() >> 8) + "." + BaseLib::HelperFunctions::getHexString(peer->getFirmwareVersion() & 0xFF, 2);
		std::string versionString = BaseLib::HelperFunctions::getHexString(firmwareVersion >> 8) + "." + BaseLib::HelperFunctions::getHexString(firmwareVersion & 0xFF, 2);

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

		std::stringstream stream(firmwareHex);
		std::string line;
		int32_t currentAddress = 0;
		std::vector<uint8_t> firmware;
		while(std::getline(stream, line))
		{
			if(line.at(0) != ':' || line.size() < 11)
			{
				_bl->deviceUpdateInfo.results[id].first = 5;
				_bl->deviceUpdateInfo.results[id].second = "Firmware file has wrong format.";
				GD::out.printError("Error: Could not read firmware file: " + firmwareFile + ": Wrong format (no colon at position 0 or line too short).");
				_updateMutex.unlock();
				_updateMode = false;
				return;
			}
			std::string hex = line.substr(1, 2);
			int32_t bytes = BaseLib::Math::getNumber(hex, true);
			hex = line.substr(7, 2);
			int32_t recordType = BaseLib::Math::getNumber(hex, true);
			if(recordType == 1) break; //End of file
			if(recordType != 0)
			{
				_bl->deviceUpdateInfo.results[id].first = 5;
				_bl->deviceUpdateInfo.results[id].second = "Firmware file has wrong format.";
				GD::out.printError("Error: Could not read firmware file: " + firmwareFile + ": Wrong format (wrong record type).");
				_updateMutex.unlock();
				_updateMode = false;
				return;
			}
			hex = line.substr(3, 4);
			int32_t address = BaseLib::Math::getNumber(hex, true);
			if(address != currentAddress || (11 + bytes * 2) > (signed)line.size())
			{
				_bl->deviceUpdateInfo.results[id].first = 5;
				_bl->deviceUpdateInfo.results[id].second = "Firmware file has wrong format.";
				GD::out.printError("Error: Could not read firmware file: " + firmwareFile + ": Wrong format (address does not match).");
				_updateMutex.unlock();
				_updateMode = false;
				return;
			}
			currentAddress += bytes;
			std::vector<uint8_t> data = _bl->hf.getUBinary(line.substr(9, bytes * 2));
			hex = line.substr(9 + bytes * 2, 2);
			int32_t checkSum = BaseLib::Math::getNumber(hex, true);
			int32_t calculatedCheckSum = bytes + (address >> 8) + (address & 0xFF) + recordType;
			for(std::vector<uint8_t>::iterator i = data.begin(); i != data.end(); ++i)
			{
				calculatedCheckSum += *i;
			}
			calculatedCheckSum = (((calculatedCheckSum & 0xFF) ^ 0xFF) + 1) & 0xFF;
			if(calculatedCheckSum != checkSum)
			{
				_bl->deviceUpdateInfo.results[id].first = 5;
				_bl->deviceUpdateInfo.results[id].second = "Firmware file has wrong format.";
				GD::out.printError("Error: Could not read firmware file: " + firmwareFile + ": Wrong format (check sum failed).");
				_updateMutex.unlock();
				_updateMode = false;
				return;
			}
			firmware.insert(firmware.end(), data.begin(), data.end());
		}

		lockBus();

		std::shared_ptr<HMWiredPacket> response = getResponse(0x75, peer->getAddress(), true);
		if(!response || response->type() != HMWiredPacketType::ackMessage)
		{
			unlockBus();
			_bl->deviceUpdateInfo.results[id].first = 6;
			_bl->deviceUpdateInfo.results[id].second = "Device did not respond to enter-bootloader packet.";
			GD::out.printWarning("Warning: Device did not enter bootloader.");
			_updateMutex.unlock();
			_updateMode = false;
			return;
		}

		//Wait for the device to enter bootloader
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		std::vector<uint8_t> payload;
		payload.push_back(0x75);
		std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(HMWiredPacketType::iMessage, 0, peer->getAddress(), false, getMessageCounter(peer->getAddress()), 0, 0, payload));
		response = getResponse(packet, true);
		if(!response || response->type() != HMWiredPacketType::system)
		{
			unlockBus();
			_bl->deviceUpdateInfo.results[id].first = 6;
			_bl->deviceUpdateInfo.results[id].second = "Device did not respond to enter-bootloader packet.";
			GD::out.printWarning("Warning: Device did not enter bootloader.");
			_updateMutex.unlock();
			_updateMode = false;
			return;
		}

		payload.clear();
		payload.push_back(0x70);
		packet.reset(new HMWiredPacket(HMWiredPacketType::iMessage, 0, peer->getAddress(), false, getMessageCounter(peer->getAddress()), 0, 0, payload));
		response = getResponse(packet, true);
		int32_t packetSize = 0;
		if(response && response->payload()->size() == 2) packetSize = (response->payload()->at(0) << 8) + response->payload()->at(1);
		if(!response || response->type() != HMWiredPacketType::system || response->payload()->size() != 2 || packetSize > 128 || packetSize == 0)
		{
			unlockBus();
			_bl->deviceUpdateInfo.results[id].first = 8;
			_bl->deviceUpdateInfo.results[id].second = "Too many communication errors (block size request failed).";
			GD::out.printWarning("Error: Block size request failed.");
			_updateMutex.unlock();
			_updateMode = false;
			return;
		}

		std::vector<uint8_t> data;
		for(int32_t i = 0; i < (signed)firmware.size(); i += packetSize)
		{
			_bl->deviceUpdateInfo.currentDeviceProgress = (i * 100) / firmware.size();
			int32_t currentPacketSize = (i + packetSize < (signed)firmware.size()) ? packetSize : firmware.size() - i;
			data.clear();
			data.push_back(0x77); //Type
			data.push_back(i >> 8); //Address
			data.push_back(i & 0xFF); //Address
			data.push_back(currentPacketSize); //Length
			data.insert(data.end(), firmware.begin() + i, firmware.begin() + i + currentPacketSize);

			std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(HMWiredPacketType::iMessage, 0, peer->getAddress(), false, getMessageCounter(peer->getAddress()), 0, 0, data));
			response = getResponse(packet, true);
			if(!response || response->type() != HMWiredPacketType::system || response->payload()->size() != 2)
			{
				unlockBus();
				_bl->deviceUpdateInfo.results[id].first = 8;
				_bl->deviceUpdateInfo.results[id].second = "Too many communication errors.";
				GD::out.printWarning("Error: Block size request failed.");
				_updateMutex.unlock();
				_updateMode = false;
				return;
			}
			int32_t receivedBytes = (response->payload()->at(0) << 8) + response->payload()->at(1);
			if(receivedBytes != currentPacketSize)
			{
				unlockBus();
				_bl->deviceUpdateInfo.results[id].first = 8;
				_bl->deviceUpdateInfo.results[id].second = "Too many communication errors (device received wrong number of bytes).";
				GD::out.printWarning("Error: Block size request failed.");
				_updateMutex.unlock();
				_updateMode = false;
				return;
			}
		}

		payload.clear();
		payload.push_back(0x67);
		packet.reset(new HMWiredPacket(HMWiredPacketType::iMessage, 0, peer->getAddress(), false, getMessageCounter(peer->getAddress()), 0, 0, payload));
		for(int32_t i = 0; i < 3; i++)
		{
			sendPacket(packet, false);
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		unlockBus();

		peer->setFirmwareVersion(firmwareVersion);
		_bl->deviceUpdateInfo.results[id].first = 0;
		_bl->deviceUpdateInfo.results[id].second = "Update successful.";
		GD::out.printInfo("Info: Peer " + std::to_string(id) + " was successfully updated to firmware version " + versionString + ".");
		_updateMutex.unlock();
		_updateMode = false;
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
    unlockBus();
    _bl->deviceUpdateInfo.results[id].first = 1;
	_bl->deviceUpdateInfo.results[id].second = "Unknown error.";
    _updateMutex.unlock();
    _updateMode = false;
}

bool HMWiredCentral::knowsDevice(std::string serialNumber)
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

bool HMWiredCentral::knowsDevice(uint64_t id)
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

void HMWiredCentral::handleAnnounce(std::shared_ptr<HMWiredPacket> packet)
{
	try
	{
		_peerInitMutex.lock();
		if(getPeer(packet->senderAddress()))
		{
			_peerInitMutex.unlock();
			return;
		}
		GD::out.printInfo("Info: New device detected on bus.");
		if(packet->payload()->size() != 16)
		{
			GD::out.printWarning("Warning: Could not interpret announce packet: Packet has unknown size (payload size has to be 16).");
			_peerInitMutex.unlock();
			return;
		}
		int32_t deviceType = (packet->payload()->at(2) << 8) + packet->payload()->at(3);
		int32_t firmwareVersion = (packet->payload()->at(4) << 8) + packet->payload()->at(5);
		std::string serialNumber(&packet->payload()->at(6), &packet->payload()->at(6) + 10);

		std::shared_ptr<HMWiredPeer> peer = createPeer(packet->senderAddress(), firmwareVersion, BaseLib::Systems::LogicalDeviceType(BaseLib::Systems::DeviceFamilies::HomeMaticWired, deviceType), serialNumber, true);
		if(!peer)
		{
			GD::out.printError("Error: HomeMatic Wired Central: Could not pair device with address 0x" + BaseLib::HelperFunctions::getHexString(packet->senderAddress(), 8) + ". No matching XML file was found.");
			_peerInitMutex.unlock();
			return;
		}

		if(peerInit(peer))
		{
			std::shared_ptr<BaseLib::RPC::Variable> deviceDescriptions(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
			peer->restoreLinks();
			std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> descriptions = peer->getDeviceDescriptions(true, std::map<std::string, bool>());
			if(!descriptions)
			{
				_peerInitMutex.unlock();
				return;
			}
			for(std::vector<std::shared_ptr<BaseLib::RPC::Variable>>::iterator j = descriptions->begin(); j != descriptions->end(); ++j)
			{
				deviceDescriptions->arrayValue->push_back(*j);
			}
			raiseRPCNewDevices(deviceDescriptions);
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
	_peerInitMutex.unlock();
}

bool HMWiredCentral::peerInit(std::shared_ptr<HMWiredPeer> peer)
{
	try
	{
		peer->initializeCentralConfig();

		int32_t address = peer->getAddress();

		peer->binaryConfig[0].data = readEEPROM(address, 0);
		peer->saveParameter(peer->binaryConfig[0].databaseID, 0, peer->binaryConfig[0].data);
		if(peer->binaryConfig[0].data.size() != 0x10)
		{
			peer->deleteFromDatabase();
			GD::out.printError("Error: HomeMatic Wired Central: Could not pair device with address 0x" + BaseLib::HelperFunctions::getHexString(address, 8) + ". Could not read master config from EEPROM.");
			return false;
		}

		for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator j = peer->rpcDevice->parameterSet->parameters.begin(); j != peer->rpcDevice->parameterSet->parameters.end(); ++j)
		{
			if((*j)->logicalParameter->enforce)
			{
				std::vector<uint8_t> enforceValue;
				(*j)->convertToPacket((*j)->logicalParameter->getEnforceValue(), enforceValue);
				peer->setConfigParameter((*j)->physicalParameter->address.index, (*j)->physicalParameter->size, enforceValue);
			}
		}

		if(!writeEEPROM(address, 0, peer->binaryConfig[0].data))
		{
			GD::out.printError("Error: Could not pair device with address 0x" + BaseLib::HelperFunctions::getHexString(address, 8) + ".");
			peer->deleteFromDatabase();
			return false;
		}

		//Read all config
		std::vector<uint8_t> command({0x45, 0, 0, 0x10, 0x40}); //Request used EEPROM blocks; start address 0x0000, block size 0x10, blocks 0x40
		std::shared_ptr<HMWiredPacket> response = getResponse(command, address);
		if(!response || response->payload()->empty() || response->payload()->size() != 12 || response->payload()->at(0) != 0x65 || response->payload()->at(1) != 0 || response->payload()->at(2) != 0 || response->payload()->at(3) != 0x10)
		{
			GD::out.printError("Error: HomeMatic Wired Central: Could not pair device with address 0x" + BaseLib::HelperFunctions::getHexString(address, 8) + ". Could not determine EEPROM blocks to read.");
			peer->deleteFromDatabase();
			return false;
		}

		int32_t configIndex = 0;
		for(int32_t j = 0; j < 8; j++)
		{
			for(int32_t k = 0; k < 8; k++)
			{
				if(response->payload()->at(j + 4) & (1 << k))
				{
					if(peer->binaryConfig.find(configIndex) == peer->binaryConfig.end())
					{
						peer->binaryConfig[configIndex].data = readEEPROM(peer->getAddress(), configIndex);
						peer->saveParameter(peer->binaryConfig[configIndex].databaseID, configIndex, peer->binaryConfig[configIndex].data);
						if(peer->binaryConfig[configIndex].data.size() != 0x10) GD::out.printError("Error: HomeMatic Wired Central: Error reading config from device with address 0x" + BaseLib::HelperFunctions::getHexString(address, 8) + ". Size is not 16 bytes.");
					}
				}
				configIndex += 0x10;
			}
		}
		_peersMutex.lock();
		try
		{
			_peers[address] = peer;
			if(!peer->getSerialNumber().empty()) _peersBySerial[peer->getSerialNumber()] = peer;
			_peersByID[peer->getID()] = peer;
			peer->setMessageCounter(_messageCounter[address]);
			_messageCounter.erase(address);
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
		return true;
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
	peer->deleteFromDatabase();
	return false;
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::addLink(std::string senderSerialNumber, int32_t senderChannelIndex, std::string receiverSerialNumber, int32_t receiverChannelIndex, std::string name, std::string description)
{
	try
	{
		if(senderSerialNumber.empty()) return BaseLib::RPC::Variable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return BaseLib::RPC::Variable::createError(-2, "Given receiver address is empty.");
		std::shared_ptr<HMWiredPeer> sender = getPeer(senderSerialNumber);
		std::shared_ptr<HMWiredPeer> receiver = getPeer(receiverSerialNumber);
		if(!sender) return BaseLib::RPC::Variable::createError(-2, "Sender device not found.");
		if(!receiver) return BaseLib::RPC::Variable::createError(-2, "Receiver device not found.");
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
	return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::addLink(uint64_t senderID, int32_t senderChannelIndex, uint64_t receiverID, int32_t receiverChannelIndex, std::string name, std::string description)
{
	try
	{
		if(senderID == 0) return BaseLib::RPC::Variable::createError(-2, "Given sender id is not set.");
		if(receiverID == 0) return BaseLib::RPC::Variable::createError(-2, "Given receiver id is not set.");
		std::shared_ptr<HMWiredPeer> sender = getPeer(senderID);
		std::shared_ptr<HMWiredPeer> receiver = getPeer(receiverID);
		if(!sender) return BaseLib::RPC::Variable::createError(-2, "Sender device not found.");
		if(!receiver) return BaseLib::RPC::Variable::createError(-2, "Receiver device not found.");
		if(senderChannelIndex < 0) senderChannelIndex = 0;
		if(receiverChannelIndex < 0) receiverChannelIndex = 0;
		if(sender->rpcDevice->channels.find(senderChannelIndex) == sender->rpcDevice->channels.end()) return BaseLib::RPC::Variable::createError(-2, "Sender channel not found.");
		if(receiver->rpcDevice->channels.find(receiverChannelIndex) == receiver->rpcDevice->channels.end()) return BaseLib::RPC::Variable::createError(-2, "Receiver channel not found.");
		std::shared_ptr<BaseLib::RPC::DeviceChannel> senderChannel = sender->rpcDevice->channels.at(senderChannelIndex);
		std::shared_ptr<BaseLib::RPC::DeviceChannel> receiverChannel = receiver->rpcDevice->channels.at(receiverChannelIndex);
		if(senderChannel->linkRoles->sourceNames.size() == 0 || receiverChannel->linkRoles->targetNames.size() == 0) return BaseLib::RPC::Variable::createError(-6, "Link not supported.");
		if(sender->getPeer(senderChannelIndex, receiver->getID(), receiverChannelIndex) || receiver->getPeer(receiverChannelIndex, sender->getID(), senderChannelIndex)) return BaseLib::RPC::Variable::createError(-6, "Link already exists.");
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
		if(!validLink) return BaseLib::RPC::Variable::createError(-6, "Link not supported.");

		std::shared_ptr<BaseLib::Systems::BasicPeer> senderPeer(new BaseLib::Systems::BasicPeer());
		senderPeer->isSender = true;
		senderPeer->id = sender->getID();
		senderPeer->address = sender->getAddress();
		senderPeer->channel = senderChannelIndex;
		senderPeer->physicalIndexOffset = senderChannel->physicalIndexOffset;
		senderPeer->serialNumber = sender->getSerialNumber();
		senderPeer->linkDescription = description;
		senderPeer->linkName = name;
		senderPeer->configEEPROMAddress = receiver->getFreeEEPROMAddress(receiverChannelIndex, false);
		if(senderPeer->configEEPROMAddress == -1) return BaseLib::RPC::Variable::createError(-32500, "Can't get free eeprom address to store config.");

		std::shared_ptr<BaseLib::Systems::BasicPeer> receiverPeer(new BaseLib::Systems::BasicPeer());
		receiverPeer->id = receiver->getID();
		receiverPeer->address = receiver->getAddress();
		receiverPeer->channel = receiverChannelIndex;
		receiverPeer->physicalIndexOffset = receiverChannel->physicalIndexOffset;
		receiverPeer->serialNumber = receiver->getSerialNumber();
		receiverPeer->linkDescription = description;
		receiverPeer->linkName = name;
		receiverPeer->configEEPROMAddress = sender->getFreeEEPROMAddress(senderChannelIndex, true);
		if(receiverPeer->configEEPROMAddress == -1) return BaseLib::RPC::Variable::createError(-32500, "Can't get free eeprom address to store config.");

		sender->addPeer(senderChannelIndex, receiverPeer);
		sender->initializeLinkConfig(senderChannelIndex, receiverPeer);
		raiseRPCUpdateDevice(sender->getID(), senderChannelIndex, sender->getSerialNumber() + ":" + std::to_string(senderChannelIndex), 1);

		receiver->addPeer(receiverChannelIndex, senderPeer);
		receiver->initializeLinkConfig(receiverChannelIndex, senderPeer);
		raiseRPCUpdateDevice(receiver->getID(), receiverChannelIndex, receiver->getSerialNumber() + ":" + std::to_string(receiverChannelIndex), 1);

		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
	return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::deleteDevice(std::string serialNumber, int32_t flags)
{
	try
	{
		if(serialNumber.empty()) return BaseLib::RPC::Variable::createError(-2, "Unknown device.");
		std::shared_ptr<HMWiredPeer> peer = getPeer(serialNumber);
		if(!peer) return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));

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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::deleteDevice(uint64_t peerID, int32_t flags)
{
	try
	{
		if(peerID == 0) return BaseLib::RPC::Variable::createError(-2, "Unknown device.");
		std::shared_ptr<HMWiredPeer> peer = getPeer(peerID);
		if(!peer) return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
		uint64_t id = peer->getID();

		//Reset
		if(flags & 0x01) peer->reset();
		deletePeer(id);

		if(knowsDevice(id)) return BaseLib::RPC::Variable::createError(-1, "Error deleting peer. See log for more details.");

		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

/*std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::getDeviceDescriptionCentral()
{
	try
	{
		std::shared_ptr<BaseLib::RPC::Variable> description(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(_deviceID))));
		description->structValue->insert(BaseLib::RPC::RPCStructElement("ADDRESS", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(_serialNumber))));

		std::shared_ptr<BaseLib::RPC::Variable> variable = std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		description->structValue->insert(BaseLib::RPC::RPCStructElement("CHILDREN", variable));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("FIRMWARE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string(VERSION)))));

		int32_t uiFlags = (int32_t)RPC::Device::UIFlags::dontdelete | (int32_t)RPC::Device::UIFlags::visible;
		description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(uiFlags))));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("INTERFACE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(_serialNumber))));

		variable = std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		description->structValue->insert(BaseLib::RPC::RPCStructElement("PARAMSETS", variable));
		variable->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string("MASTER")))); //Always MASTER

		description->structValue->insert(BaseLib::RPC::RPCStructElement("PARENT", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string("")))));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("PHYSICAL_ADDRESS", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::to_string(_address)))));
		description->structValue->insert(BaseLib::RPC::RPCStructElement("RF_ADDRESS", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::to_string(_address)))));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("ROAMING", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(0))));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string("Homegear HomeMatic Wired Central")))));

		description->structValue->insert(BaseLib::RPC::RPCStructElement("VERSION", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((int32_t)10))));

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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}*/

std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::getDeviceInfo(uint64_t id, std::map<std::string, bool> fields)
{
	try
	{
		if(id > 0)
		{
			std::shared_ptr<HMWiredPeer> peer(getPeer(id));
			if(!peer) return BaseLib::RPC::Variable::createError(-2, "Unknown device.");

			return peer->getDeviceInfo(fields);
		}
		else
		{
			std::shared_ptr<BaseLib::RPC::Variable> array(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));

			std::vector<std::shared_ptr<HMWiredPeer>> peers;
			//Copy all peers first, because listDevices takes very long and we don't want to lock _peersMutex too long
			_peersMutex.lock();
			for(std::map<uint64_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peersByID.begin(); i != _peersByID.end(); ++i)
			{
				peers.push_back(std::dynamic_pointer_cast<HMWiredPeer>(i->second));
			}
			_peersMutex.unlock();

			for(std::vector<std::shared_ptr<HMWiredPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				//listDevices really needs a lot of resources, so wait a little bit after each device
				std::this_thread::sleep_for(std::chrono::milliseconds(3));
				std::shared_ptr<BaseLib::RPC::Variable> info = (*i)->getDeviceInfo(fields);
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::putParamset(std::string serialNumber, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::Variable> paramset)
{
	try
	{
		std::shared_ptr<HMWiredPeer> peer(getPeer(serialNumber));
		uint64_t remoteID = 0;
		if(!remoteSerialNumber.empty())
		{
			std::shared_ptr<HMWiredPeer> remotePeer(getPeer(remoteSerialNumber));
			if(!remotePeer) return BaseLib::RPC::Variable::createError(-3, "Remote peer is unknown.");
			remoteID = remotePeer->getID();
		}
		if(peer) return peer->putParamset(channel, type, remoteID, remoteChannel, paramset);
		return BaseLib::RPC::Variable::createError(-2, "Unknown device.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::putParamset(uint64_t peerID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::Variable> paramset)
{
	try
	{
		std::shared_ptr<HMWiredPeer> peer(getPeer(peerID));
		if(peer) return peer->putParamset(channel, type, remoteID, remoteChannel, paramset);
		return BaseLib::RPC::Variable::createError(-2, "Unknown device.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::removeLink(std::string senderSerialNumber, int32_t senderChannelIndex, std::string receiverSerialNumber, int32_t receiverChannelIndex)
{
	try
	{
		if(senderSerialNumber.empty()) return BaseLib::RPC::Variable::createError(-2, "Given sender address is empty.");
		if(receiverSerialNumber.empty()) return BaseLib::RPC::Variable::createError(-2, "Given receiver address is empty.");
		std::shared_ptr<HMWiredPeer> sender = getPeer(senderSerialNumber);
		std::shared_ptr<HMWiredPeer> receiver = getPeer(receiverSerialNumber);
		if(!sender) return BaseLib::RPC::Variable::createError(-2, "Sender device not found.");
		if(!receiver) return BaseLib::RPC::Variable::createError(-2, "Receiver device not found.");
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
	return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::removeLink(uint64_t senderID, int32_t senderChannelIndex, uint64_t receiverID, int32_t receiverChannelIndex)
{
	try
	{
		if(senderID == 0) return BaseLib::RPC::Variable::createError(-2, "Sender id is not set.");
		if(receiverID == 0) return BaseLib::RPC::Variable::createError(-2, "Receiver id is not set.");
		std::shared_ptr<HMWiredPeer> sender = getPeer(senderID);
		std::shared_ptr<HMWiredPeer> receiver = getPeer(receiverID);
		if(!sender) return BaseLib::RPC::Variable::createError(-2, "Sender device not found.");
		if(!receiver) return BaseLib::RPC::Variable::createError(-2, "Receiver device not found.");
		if(senderChannelIndex < 0) senderChannelIndex = 0;
		if(receiverChannelIndex < 0) receiverChannelIndex = 0;
		if(sender->rpcDevice->channels.find(senderChannelIndex) == sender->rpcDevice->channels.end()) return BaseLib::RPC::Variable::createError(-2, "Sender channel not found.");
		if(receiver->rpcDevice->channels.find(receiverChannelIndex) == receiver->rpcDevice->channels.end()) return BaseLib::RPC::Variable::createError(-2, "Receiver channel not found.");
		if(!sender->getPeer(senderChannelIndex, receiver->getID()) && !receiver->getPeer(receiverChannelIndex, sender->getID())) return BaseLib::RPC::Variable::createError(-6, "Devices are not paired to each other.");

		sender->removePeer(senderChannelIndex, receiver->getID(), receiverChannelIndex);
		receiver->removePeer(receiverChannelIndex, sender->getID(), senderChannelIndex);

		raiseRPCUpdateDevice(sender->getID(), senderChannelIndex, sender->getSerialNumber() + ":" + std::to_string(senderChannelIndex), 1);
		raiseRPCUpdateDevice(receiver->getID(), receiverChannelIndex, receiver->getSerialNumber() + ":" + std::to_string(receiverChannelIndex), 1);

		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
	return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::searchDevices()
{
	try
	{
		lockBus();
		_pairing = true;
		std::vector<int32_t> foundDevices;
		GD::physicalInterface->search(foundDevices);
		unlockBus();
		GD::out.printInfo("Info: Search completed. Found " + std::to_string(foundDevices.size()) + " devices.");
		std::vector<std::shared_ptr<HMWiredPeer>> newPeers;
		_peerInitMutex.lock();
		try
		{
			for(std::vector<int32_t>::iterator i = foundDevices.begin(); i != foundDevices.end(); ++i)
			{
				if(getPeer(*i)) continue;

				//Get device type:
				std::shared_ptr<HMWiredPacket> response = getResponse(0x68, *i, true);
				if(!response || response->payload()->size() != 2)
				{
					GD::out.printError("Error: HomeMatic Wired Central: Could not pair device with address 0x" + BaseLib::HelperFunctions::getHexString(*i, 8) + ". Device type request failed.");
					continue;
				}
				int32_t deviceType = (response->payload()->at(0) << 8) + response->payload()->at(1);

				//Get firmware version:
				response = getResponse(0x76, *i);
				if(!response || response->payload()->size() != 2)
				{
					GD::out.printError("Error: HomeMatic Wired Central: Could not pair device with address 0x" + BaseLib::HelperFunctions::getHexString(*i, 8) + ". Firmware version request failed.");
					continue;
				}
				int32_t firmwareVersion = (response->payload()->at(0) << 8) + response->payload()->at(1);

				//Get serial number:
				response = getResponse(0x6E, *i);
				if(!response || response->payload()->empty())
				{
					GD::out.printError("Error: HomeMatic Wired Central: Could not pair device with address 0x" + BaseLib::HelperFunctions::getHexString(*i, 8) + ". Serial number request failed.");
					continue;
				}
				std::string serialNumber(&response->payload()->at(0), &response->payload()->at(0) + response->payload()->size());

				std::shared_ptr<HMWiredPeer> peer = createPeer(*i, firmwareVersion, BaseLib::Systems::LogicalDeviceType(BaseLib::Systems::DeviceFamilies::HomeMaticWired, deviceType), serialNumber, true);
				if(!peer)
				{
					GD::out.printError("Error: HomeMatic Wired Central: Could not pair device with address 0x" + BaseLib::HelperFunctions::getHexString(*i, 8) + ". No matching XML file was found.");
					continue;
				}

				if(peerInit(peer)) newPeers.push_back(peer);;
			}

			if(newPeers.size() > 0)
			{
				std::shared_ptr<BaseLib::RPC::Variable> deviceDescriptions(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
				for(std::vector<std::shared_ptr<HMWiredPeer>>::iterator i = newPeers.begin(); i != newPeers.end(); ++i)
				{
					(*i)->restoreLinks();
					std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> descriptions = (*i)->getDeviceDescriptions(true, std::map<std::string, bool>());
					if(!descriptions) continue;
					for(std::vector<std::shared_ptr<BaseLib::RPC::Variable>>::iterator j = descriptions->begin(); j != descriptions->end(); ++j)
					{
						deviceDescriptions->arrayValue->push_back(*j);
					}
				}
				raiseRPCNewDevices(deviceDescriptions);
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
		_peerInitMutex.unlock();
		_pairing = false;
		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((uint32_t)newPeers.size()));
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
	_pairing = false;
	unlockBus();
	return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredCentral::updateFirmware(std::vector<uint64_t> ids, bool manual)
{
	try
	{
		if(_updateMode || _bl->deviceUpdateInfo.currentDevice > 0) return BaseLib::RPC::Variable::createError(-32500, "Central is already already updating a device. Please wait until the current update is finished.");
		_updateFirmwareThreadMutex.lock();
		if(_disposing)
		{
			_updateFirmwareThreadMutex.unlock();
			return BaseLib::RPC::Variable::createError(-32500, "Central is disposing.");
		}
		if(_updateFirmwareThread.joinable()) _updateFirmwareThread.join();
		_updateFirmwareThread = std::thread(&HMWiredCentral::updateFirmwares, this, ids);
		_updateFirmwareThreadMutex.unlock();
		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}
} /* namespace HMWired */
