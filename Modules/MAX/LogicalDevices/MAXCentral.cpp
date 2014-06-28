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
}

void MAXCentral::init()
{
	try
	{
		MAXDevice::init();

		_deviceType = (uint32_t)DeviceType::MAXCENTRAL;
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

void MAXCentral::deletePeer(uint64_t id)
{

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
			stringStream << "peers list\t\tList all peers" << std::endl;
			stringStream << "peers reset\t\tUnpair a peer and reset it to factory defaults" << std::endl;
			stringStream << "peers select\t\tSelect a peer" << std::endl;
			stringStream << "peers unpair\t\tUnpair a peer" << std::endl;
			stringStream << "peers update\t\tUpdates a peer to the newest firmware version" << std::endl;
			stringStream << "search\t\t\tSearches for new devices on the bus" << std::endl;
			stringStream << "unselect\t\tUnselect this device" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 6, "search") == 0)
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
						stringStream << "Description: This command searches for new devices on the bus." << std::endl;
						stringStream << "Usage: search" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			std::shared_ptr<BaseLib::RPC::RPCVariable> result = searchDevices();
			if(result->errorStruct) stringStream << "Error: " << result->structValue->at("faultString")->stringValue << std::endl;
			else stringStream << "Search completed successfully." << std::endl;
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
				deletePeer(peerID);
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
					peerID = BaseLib::HelperFunctions::getNumber(element, false);
					if(peerID == 0) return "Invalid id.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command resets and unpairs a peer." << std::endl;
				stringStream << "Usage: peers reset PEERID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to reset. Example: 513" << std::endl;
				return stringStream.str();
			}

			std::shared_ptr<MAXPeer> peer = getPeer(peerID);
			if(!peer) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				if(_currentPeer && _currentPeer->getID() == peerID) _currentPeer.reset();
				stringStream << "Resetting peer " << std::to_string(peerID) << std::endl;
				//peer->reset();
				deletePeer(peerID);
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
				bool firmwareUpdates = false;
				std::string bar(" │ ");
				const int32_t idWidth = 8;
				const int32_t addressWidth = 8;
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
				stringStream << "─────────┼──────────┼───────────────┼──────┼───────────────────────────┼──────────┼────────" << std::endl;
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
						<< std::setw(addressWidth) << BaseLib::HelperFunctions::getHexString(i->second->getAddress(), 8) << bar
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
				stringStream << "─────────┴──────────┴───────────────┴──────┴───────────────────────────┴──────────┴────────" << std::endl;
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
		else if(command.compare(0, 12, "peers update") == 0)
		{
			uint64_t peerID;
			bool all = false;

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
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command updates one or all peers to the newest firmware version available in \"" << _bl->settings.firmwarePath() << "\"." << std::endl;
				stringStream << "Usage: peers update PEERID" << std::endl;
				stringStream << "       peers update all" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  PEERID:\tThe id of the peer to update. Example: 513" << std::endl;
				return stringStream.str();
			}

			std::shared_ptr<BaseLib::RPC::RPCVariable> result;
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
				std::shared_ptr<MAXPeer> peer = getPeer(peerID);
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
}
