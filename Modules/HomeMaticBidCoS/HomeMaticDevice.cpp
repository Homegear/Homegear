/* Copyright 2013-2015 Sathya Laufer
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

#include "HomeMaticDevice.h"
#include "Devices/HomeMaticCentral.h"
#include "../Base/BaseLib.h"
#include "GD.h"

namespace BidCoS
{

HomeMaticDevice::HomeMaticDevice(IDeviceEventSink* eventHandler) : LogicalDevice(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS, GD::bl, eventHandler)
{
	_physicalInterface = GD::defaultPhysicalInterface;
}

HomeMaticDevice::HomeMaticDevice(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : LogicalDevice(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS, GD::bl, deviceID, serialNumber, address, eventHandler)
{
	_physicalInterface = GD::defaultPhysicalInterface;
}

void HomeMaticDevice::init()
{
	try
	{
		if(_initialized) return; //Prevent running init two times
		_initialized = true;
		_messages = std::shared_ptr<BidCoSMessages>(new BidCoSMessages());

		if(_physicalInterface) _physicalInterface->addEventHandler((BaseLib::Systems::IPhysicalInterface::IPhysicalInterfaceEventSink*)this);

		_messageCounter[0] = 0; //Broadcast message counter
		_messageCounter[1] = 0; //Duty cycle message counter

		setUpBidCoSMessages();
		_workerThread = std::thread(&HomeMaticDevice::worker, this);
		BaseLib::Threads::setThreadPriority(_bl, _workerThread.native_handle(), _bl->settings.workerThreadPriority(), _bl->settings.workerThreadPolicy());
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

void HomeMaticDevice::setPhysicalInterfaceID(std::string id)
{
	if(id.empty() || (GD::physicalInterfaces.find(id) != GD::physicalInterfaces.end() && GD::physicalInterfaces.at(id)))
	{
		if(_physicalInterface) _physicalInterface->removeEventHandler((BaseLib::Systems::IPhysicalInterface::IPhysicalInterfaceEventSink*)this);
		_physicalInterfaceID = id;
		_physicalInterface = id.empty() ? GD::defaultPhysicalInterface : GD::physicalInterfaces.at(_physicalInterfaceID);
		_physicalInterface->addEventHandler((BaseLib::Systems::IPhysicalInterface::IPhysicalInterfaceEventSink*)this);
		saveVariable(4, _physicalInterfaceID);
	}
}

HomeMaticDevice::~HomeMaticDevice()
{
	try
	{
		dispose();
		//dispose might have been called by another thread, so wait until dispose is finished
		while(!_disposed) std::this_thread::sleep_for(std::chrono::milliseconds(50)); //Wait for received packets, without this, program sometimes SIGABRTs
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

void HomeMaticDevice::dispose(bool wait)
{
	try
	{
		if(_disposing) return;
		_disposing = true;
		GD::out.printDebug("Removing device " + std::to_string(_deviceID) + " from physical device's event queue...");
		for(std::map<std::string, std::shared_ptr<IBidCoSInterface>>::iterator i = GD::physicalInterfaces.begin(); i != GD::physicalInterfaces.end(); ++i)
		{
			//Just to make sure cycle through all physical devices. If event handler is not removed => segfault
			i->second->removeEventHandler((BaseLib::Systems::IPhysicalInterface::IPhysicalInterfaceEventSink*)this);
		}
		int64_t startTime = BaseLib::HelperFunctions::getTime();
		stopThreads();
		int64_t timeDifference = BaseLib::HelperFunctions::getTime() - startTime;
		//Packets might still arrive, after removing this device from the rfDevice, so sleep a little bit
		//This is not necessary if the rfDevice doesn't listen anymore
		if(wait && timeDifference >= 0 && timeDifference < 2000) std::this_thread::sleep_for(std::chrono::milliseconds(2000 - timeDifference));
		LogicalDevice::dispose(wait);
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
	_disposed = true;
}

void HomeMaticDevice::stopThreads()
{
	try
	{
		_bidCoSQueueManager.dispose(false);
		_receivedPackets.dispose(false);
		_sentPackets.dispose(false);

		_peersMutex.lock();
		for(std::unordered_map<int32_t, std::shared_ptr<BaseLib::Systems::Peer>>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			i->second->dispose();
		}
		_peersMutex.unlock();

		_stopWorkerThread = true;
		if(_workerThread.joinable())
		{
			GD::out.printDebug("Debug: Waiting for worker thread of device " + std::to_string(_deviceID) + "...");
			_workerThread.join();
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

bool HomeMaticDevice::isCentral()
{
	return _deviceType == (uint32_t)DeviceType::HMCENTRAL;
}

bool HomeMaticDevice::isSwitch(BaseLib::Systems::LogicalDeviceType type)
{
	switch((DeviceType)type.type())
	{
	case DeviceType::HMESPMSW1PL:
		return true;
	case DeviceType::HMLCSW1PL:
		return true;
	case DeviceType::HMLCSW1PL2:
		return true;
	case DeviceType::HMLCSW1SM:
		return true;
	case DeviceType::HMLCSW2SM:
		return true;
	case DeviceType::HMLCSW4SM:
		return true;
	case DeviceType::HMLCSW4PCB:
		return true;
	case DeviceType::HMLCSW4WM:
		return true;
	case DeviceType::HMLCSW1FM:
		return true;
	case DeviceType::HMLCSWSCHUECO:
		return true;
	case DeviceType::HMLCSWSCHUECO2:
		return true;
	case DeviceType::HMLCSW2FM:
		return true;
	case DeviceType::HMLCSW1PBFM:
		return true;
	case DeviceType::HMLCSW2PBFM:
		return true;
	case DeviceType::HMLCSW4DR:
		return true;
	case DeviceType::HMLCSW2DR:
		return true;
	case DeviceType::HMLCSW1PBUFM:
		return true;
	case DeviceType::HMLCSW4BAPCB:
		return true;
	case DeviceType::HMLCSW1BAPCB:
		return true;
	case DeviceType::HMLCSW1PLOM54:
		return true;
	case DeviceType::HMLCSW1SMATMEGA168:
		return true;
	case DeviceType::HMLCSW4SMATMEGA168:
		return true;
	case DeviceType::HMMODRE8:
		return true;
	default:
		return false;
	}
	return false;
}

bool HomeMaticDevice::isDimmer(BaseLib::Systems::LogicalDeviceType type)
{
	switch((DeviceType)type.type())
	{
	case DeviceType::HMLCDIM1TPL:
		return true;
	case DeviceType::HMLCDIM1TPL2:
		return true;
	case DeviceType::HMLCDIM1TCV:
		return true;
	case DeviceType::HMLCDIMSCHUECO:
		return true;
	case DeviceType::HMLCDIMSCHUECO2:
		return true;
	case DeviceType::HMLCDIM2TSM:
		return true;
	case DeviceType::HMLCDIM1TFM:
		return true;
	case DeviceType::HMLCDIM1LPL644:
		return true;
	case DeviceType::HMLCDIM1LCV644:
		return true;
	case DeviceType::HMLCDIM1PWMCV:
		return true;
	case DeviceType::HMLCDIM1TPL644:
		return true;
	case DeviceType::HMLCDIM1TCV644:
		return true;
	case DeviceType::HMLCDIM1TFM644:
		return true;
	case DeviceType::HMLCDIM1TPBUFM:
		return true;
	case DeviceType::HMLCDIM2LSM644:
		return true;
	case DeviceType::HMLCDIM2TSM644:
		return true;
	default:
		return false;
	}
	return false;
}

std::shared_ptr<BaseLib::Systems::Central> HomeMaticDevice::getCentral()
{
	try
	{
		if(_central) return _central;
		_central = GD::family->getCentral();
		return _central;
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
	return std::shared_ptr<BaseLib::Systems::Central>();
}

std::shared_ptr<HomeMaticDevice> HomeMaticDevice::getDevice(int32_t address)
{
	try
	{
		std::shared_ptr<HomeMaticDevice> device(std::dynamic_pointer_cast<HomeMaticDevice>(GD::family->get(address)));
		return device;
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
	return std::shared_ptr<HomeMaticDevice>();
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
			stringStream << "unselect\t\tUnselect this device" << std::endl;
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
					duration = BaseLib::Math::getNumber(element, true);
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

void HomeMaticDevice::loadPeers()
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetPeers();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			int32_t peerID = row->second.at(0)->intValue;
			GD::out.printMessage("Loading peer " + std::to_string(peerID));
			int32_t address = row->second.at(2)->intValue;
			std::shared_ptr<BidCoSPeer> peer(new BidCoSPeer(peerID, address, row->second.at(3)->textValue, _deviceID, isCentral(), this));
			if(!peer->load(this)) continue;
			if(!peer->rpcDevice) continue;
			_peersMutex.lock();
			_peers[peer->getAddress()] = peer;
			if(!peer->getSerialNumber().empty()) _peersBySerial[peer->getSerialNumber()] = peer;
			_peersByID[peerID] = peer;
			_peersMutex.unlock();
			if(peer->getPhysicalInterface()->needsPeers()) peer->getPhysicalInterface()->addPeer(peer->getPeerInfo());
			if(!peer->getTeamRemoteSerialNumber().empty())
			{
				_peersMutex.lock();
				if(_peersBySerial.find(peer->getTeamRemoteSerialNumber()) == _peersBySerial.end())
				{
					std::shared_ptr<BidCoSPeer> team = createTeam(peer->getTeamRemoteAddress(), peer->getDeviceType(), peer->getTeamRemoteSerialNumber());
					team->rpcDevice = peer->rpcDevice->team;
					team->initializeCentralConfig();
					team->setID(peer->getID() | (1 << 30));
					team->setInterface(peer->getPhysicalInterfaceID());
					_peersBySerial[team->getSerialNumber()] = team;
					_peersByID[team->getID()] = team;
				}
				_peersMutex.unlock();
				for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = peer->rpcDevice->channels.begin(); i != peer->rpcDevice->channels.end(); ++i)
				{
					if(i->second->hasTeam)
					{
						getPeer(peer->getTeamRemoteSerialNumber())->teamChannels.push_back(std::pair<std::string, uint32_t>(peer->getSerialNumber(), peer->getTeamRemoteChannel()));
						break;
					}
				}
			}
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
}

void HomeMaticDevice::saveVariables()
{
	try
	{
		if(_deviceID == 0) return;
		saveVariable(0, _firmwareVersion);
		saveVariable(1, _centralAddress);
		saveMessageCounters(); //2
		saveConfig(); //3
		saveVariable(4, _physicalInterfaceID);
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

void HomeMaticDevice::loadVariables()
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetDeviceVariables();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 0:
				_firmwareVersion = row->second.at(3)->intValue;
				break;
			case 1:
				_centralAddress = row->second.at(3)->intValue;
				break;
			case 2:
				unserializeMessageCounters(row->second.at(5)->binaryValue);
				break;
			case 3:
				unserializeConfig(row->second.at(5)->binaryValue);
				break;
			case 4:
				_physicalInterfaceID = row->second.at(4)->textValue;
				if(!_physicalInterfaceID.empty() && GD::physicalInterfaces.find(_physicalInterfaceID) != GD::physicalInterfaces.end()) _physicalInterface = GD::physicalInterfaces.at(_physicalInterfaceID);
				break;
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

void HomeMaticDevice::saveMessageCounters()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializeMessageCounters(serializedData);
		saveVariable(2, serializedData);
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

void HomeMaticDevice::saveConfig()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializeConfig(serializedData);
		saveVariable(3, serializedData);
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

void HomeMaticDevice::serializeMessageCounters(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder(_bl);
		encoder.encodeInteger(encodedData, _messageCounter.size());
		for(std::unordered_map<int32_t, uint8_t>::const_iterator i = _messageCounter.begin(); i != _messageCounter.end(); ++i)
		{
			encoder.encodeInteger(encodedData, i->first);
			encoder.encodeByte(encodedData, i->second);
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

void HomeMaticDevice::unserializeMessageCounters(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BaseLib::BinaryDecoder decoder(_bl);
		uint32_t position = 0;
		uint32_t messageCounterSize = decoder.decodeInteger(*serializedData, position);
		for(uint32_t i = 0; i < messageCounterSize; i++)
		{
			int32_t index = decoder.decodeInteger(*serializedData, position);
			_messageCounter[index] = decoder.decodeByte(*serializedData, position);
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

void HomeMaticDevice::serializeConfig(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder(_bl);
		encoder.encodeInteger(encodedData, _config.size());
		for(std::unordered_map<int32_t, std::unordered_map<int32_t, std::map<int32_t, int32_t>>>::const_iterator i = _config.begin(); i != _config.end(); ++i)
		{
			encoder.encodeInteger(encodedData, i->first); //Channel
			encoder.encodeInteger(encodedData, i->second.size());
			for(std::unordered_map<int32_t, std::map<int32_t, int32_t>>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				encoder.encodeInteger(encodedData, j->first); //List index
				encoder.encodeInteger(encodedData, j->second.size());
				for(std::map<int32_t, int32_t>::const_iterator k = j->second.begin(); k != j->second.end(); ++k)
				{
					encoder.encodeInteger(encodedData, k->first);
					encoder.encodeInteger(encodedData, k->second);
				}
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

void HomeMaticDevice::unserializeConfig(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BaseLib::BinaryDecoder decoder(_bl);
		uint32_t position = 0;
		uint32_t configSize = decoder.decodeInteger(*serializedData, position);
		for(uint32_t i = 0; i < configSize; i++)
		{
			int32_t channel = decoder.decodeInteger(*serializedData, position);
			uint32_t listCount = decoder.decodeInteger(*serializedData, position);
			for(uint32_t j = 0; j < listCount; j++)
			{
				int32_t listIndex = decoder.decodeInteger(*serializedData, position);
				uint32_t listSize = decoder.decodeInteger(*serializedData, position);
				for(uint32_t k = 0; k < listSize; k++)
				{
					int32_t parameterIndex = decoder.decodeInteger(*serializedData, position);
					_config[channel][listIndex][parameterIndex] = decoder.decodeInteger(*serializedData, position);
				}
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

void HomeMaticDevice::savePeers(bool full)
{
	try
	{
		_peersMutex.lock();
		//Don't change this to _peersByID, because then teams would be saved!
		for(std::unordered_map<int32_t, std::shared_ptr<BaseLib::Systems::Peer>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			//Necessary, because peers can be assigned to multiple virtual devices
			if(i->second->getParentID() != _deviceID)
			{
				GD::out.printDebug("Debug: Not saving peer " + std::to_string(i->second->getID()) + ", because the parent device ID does not match. This is normal, if the peer is paired to the central AND a virtual device.");
				continue;
			}
			//We are always printing this, because the init script needs it
			GD::out.printMessage("(Shutdown) => Saving HomeMatic BidCoS peer " + std::to_string(i->second->getID()) + " with address 0x" + BaseLib::HelperFunctions::getHexString(i->second->getAddress(), 6));
			i->second->save(full, full, full);
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

bool HomeMaticDevice::onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(_disposing) return false;
		std::shared_ptr<BidCoSPacket> bidCoSPacket(std::dynamic_pointer_cast<BidCoSPacket>(packet));
		if(!bidCoSPacket) return false;
		if(bidCoSPacket->senderAddress() == _address && GD::physicalInterfaces.size() > 1)
		{
			//Packet we sent was received by another interface
			return true;
		}
		if(_receivedPackets.set(bidCoSPacket->senderAddress(), bidCoSPacket, bidCoSPacket->timeReceived())) return true;
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(bidCoSPacket->senderAddress());
		if(queue && queue->getQueueType() == BidCoSQueueType::GETVALUE) return false; //Don't handle peer's get value responses
		std::shared_ptr<BidCoSMessage> message = _messages->find(DIRECTIONIN, bidCoSPacket);
		if(message && message->checkAccess(bidCoSPacket, queue))
		{
			if(_bl->debugLevel >= 6) GD::out.printDebug("Debug: Device " + std::to_string(_deviceID) + ": Access granted for packet " + bidCoSPacket->hexString());
			message->invokeMessageHandlerIncoming(bidCoSPacket);
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
    return false;
}

std::shared_ptr<IBidCoSInterface> HomeMaticDevice::getPhysicalInterface(int32_t peerAddress)
{
	try
	{
		std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.get(peerAddress);
		if(queue) return queue->getPhysicalInterface();
		std::shared_ptr<BidCoSPeer> peer = getPeer(peerAddress);
		return peer ? peer->getPhysicalInterface() : GD::defaultPhysicalInterface;
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
    return GD::defaultPhysicalInterface;
}

void HomeMaticDevice::sendPacket(std::shared_ptr<IBidCoSInterface> physicalInterface, std::shared_ptr<BidCoSPacket> packet, bool stealthy)
{
	try
	{
		if(!packet || !physicalInterface) return;
		uint32_t responseDelay = physicalInterface->responseDelay();
		std::shared_ptr<BidCoSPacketInfo> packetInfo = _sentPackets.getInfo(packet->destinationAddress());
		/*int64_t time = _bl->hf.getTime();
		if(_physicalInterface->autoResend())
		{
			if((packet->messageType() == 0x02 && packet->controlByte() == 0x80 && packet->payload()->size() == 1 && packet->payload()->at(0) == 0)
				|| !((packet->controlByte() & 0x01) && (packet->payload()->empty() || (packet->payload()->size() == 1 && packet->payload()->at(0) == 0))))
			{
				time -= 80;
			}
		}
		if(!stealthy) _sentPackets.set(packet->destinationAddress(), packet, time);*/
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
			int64_t time = BaseLib::HelperFunctions::getTime();
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
		else if(_bl->debugLevel > 4) GD::out.printDebug("Debug: Sending packet " + packet->hexString() + " immediately, because it seems it is no response (no packet information found).", 7);
		physicalInterface->sendPacket(packet);
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

void HomeMaticDevice::sendPacketMultipleTimes(std::shared_ptr<IBidCoSInterface> physicalInterface, std::shared_ptr<BidCoSPacket> packet, int32_t peerAddress, int32_t count, int32_t delay, bool useCentralMessageCounter, bool isThread)
{
	try
	{
		if(!isThread)
		{
			std::thread t(&HomeMaticDevice::sendPacketMultipleTimes, this, physicalInterface, packet, peerAddress, count, delay, useCentralMessageCounter, true);
			t.detach();
			return;
		}
		if(!packet || !physicalInterface) return;
		if(physicalInterface->autoResend() && (packet->controlByte() & 0x20) && delay < 700) delay = 700;
		std::shared_ptr<BidCoSPeer> peer = getPeer(peerAddress);
		if(!peer) return;
		for(int32_t i = 0; i < count; i++)
		{
			_sentPackets.set(packet->destinationAddress(), packet);
			int64_t start = BaseLib::HelperFunctions::getTime();
			physicalInterface->sendPacket(packet);
			if(useCentralMessageCounter)
			{
				packet->setMessageCounter(_messageCounter[0]);
				_messageCounter[0]++;
			}
			else
			{
				packet->setMessageCounter(peer->getMessageCounter());
				peer->setMessageCounter(peer->getMessageCounter() + 1);
			}
			int32_t difference = BaseLib::HelperFunctions::getTime() - start;
			if(difference < delay - 10) std::this_thread::sleep_for(std::chrono::milliseconds(delay - difference));
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
		setCentralAddress(0);
		_pairing = false;
		_justPairedToOrThroughCentral = false;
		_peersMutex.lock();
		_peers.clear();
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
    _peersMutex.unlock();
}

void HomeMaticDevice::handleWakeUp(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	sendOK(messageCounter, packet->senderAddress());
}

std::shared_ptr<BidCoSPeer> HomeMaticDevice::createPeer(int32_t address, int32_t firmwareVersion, BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet, bool save)
{
    return std::shared_ptr<BidCoSPeer>(new BidCoSPeer(_deviceID, isCentral(), this));
}

std::shared_ptr<BidCoSPeer> HomeMaticDevice::createTeam(int32_t address, BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber)
{
	try
	{
		std::shared_ptr<BidCoSPeer> team(new BidCoSPeer(_deviceID, isCentral(), this));
		team->setAddress(address);
		team->setDeviceType(deviceType);
		team->setSerialNumber(serialNumber);
		//Do not save team!!!
		return team;
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
		std::shared_ptr<BidCoSPeer> peer = getPeer(address);
		if(!peer) return;

		GD::out.printMessage("HomeMatic BidCoS Device " + std::to_string(_deviceID) + ": Unpaired from peer " + std::to_string(peer->getID()));
		sendOK(messageCounter, packet->senderAddress());
		_peersMutex.lock();
		try
		{
			if(peer->getDeviceType().type() != (uint32_t)DeviceType::HMRCV50)
			{
				peer->deleteFromDatabase();
				_peers.erase(address); //Unpair. Unpairing of HMRCV50 is done through CONFIG_WRITE_INDEX
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
				_peers[queue->peer->getAddress()] = queue->peer;
				queue->peer->save(true, true, false);
				setCentralAddress(queue->peer->getAddress());
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
				_peers[packet->senderAddress()]->deleteFromDatabase();
				_peers.erase(packet->senderAddress());
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			_peersMutex.unlock();
			setCentralAddress(0);
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

void HomeMaticDevice::handleConfigParamRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		_currentList = packet->payload()->at(6);
		sendConfigParams(messageCounter, packet->senderAddress(), packet);
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

void HomeMaticDevice::handleTimeRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x02);
		const auto timePoint = std::chrono::system_clock::now();
		time_t t = std::chrono::system_clock::to_time_t(timePoint);
		tm* localTime = std::localtime(&t);
		uint32_t time = (uint32_t)(t - 946684800);
		payload.push_back(localTime->tm_gmtoff / 1800);
		payload.push_back(time >> 24);
		payload.push_back((time >> 16) & 0xFF);
		payload.push_back((time >> 8) & 0xFF);
		payload.push_back(time & 0xFF);
		std::shared_ptr<BidCoSPacket> timePacket(new BidCoSPacket(messageCounter, 0x80, 0x3F, _address, packet->senderAddress(), payload));
		sendPacket(getPhysicalInterface(packet->senderAddress()), timePacket);
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

void HomeMaticDevice::handleConfigPeerAdd(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		int32_t address = (packet->payload()->at(2) << 16) + (packet->payload()->at(3) << 8) + (packet->payload()->at(4));
		if(!peerExists(address))
		{
			std::shared_ptr<BidCoSPeer> peer = createPeer(address, -1, BaseLib::Systems::LogicalDeviceType(), "", packet->payload()->at(5), 0);
			_peersMutex.lock();
			try
			{
				_peers[address] = peer;
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
		_justPairedToOrThroughCentral = true;
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

void HomeMaticDevice::handleConfigStart(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(_pairing)
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, getPhysicalInterface(packet->senderAddress()), BidCoSQueueType::PAIRINGCENTRAL, packet->senderAddress());
			std::shared_ptr<BidCoSPeer> peer(new BidCoSPeer(_deviceID, isCentral(), this));
			peer->setAddress(packet->senderAddress());
			peer->setDeviceType(BaseLib::Systems::LogicalDeviceType(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS, (uint32_t)DeviceType::HMRCV50));
			peer->setMessageCounter(0); //Unknown at this point
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

void HomeMaticDevice::handleConfigWriteIndex(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		if(packet->payload()->at(2) == 0x02 && packet->payload()->at(3) == 0x00)
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, getPhysicalInterface(packet->senderAddress()), BidCoSQueueType::UNPAIRING, packet->senderAddress());
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
			GD::out.printInfo("Info: HomeMatic BidCoS device " + std::to_string(_deviceID) + ": Config at index " + std::to_string(packet->payload()->at(i)) + " set to " + std::to_string(packet->payload()->at(i + 1)));
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

void HomeMaticDevice::sendPeerList(int32_t messageCounter, int32_t destinationAddress, int32_t channel)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x01); //I think it's always channel 1
		_peersMutex.lock();
		for(std::unordered_map<int32_t, std::shared_ptr<BaseLib::Systems::Peer>>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			std::shared_ptr<BidCoSPeer> peer(std::dynamic_pointer_cast<BidCoSPeer>(i->second));
			if(peer->getDeviceType().type() == (uint32_t)DeviceType::HMRCV50) continue;
			if(peer->getLocalChannel() != channel) continue;
			payload.push_back(i->first >> 16);
			payload.push_back((i->first >> 8) & 0xFF);
			payload.push_back(i->first & 0xFF);
			payload.push_back(peer->getRemoteChannel()); //Channel
		}
		_peersMutex.unlock();
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		std::shared_ptr<BidCoSPacket> packet(new BidCoSPacket(messageCounter, 0x80, 0x10, _address, destinationAddress, payload));
		sendPacket(getPhysicalInterface(destinationAddress), packet);
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
		std::shared_ptr<IBidCoSInterface> physicalInterface = getPhysicalInterface(destinationAddress);
		std::this_thread::sleep_for(std::chrono::milliseconds(physicalInterface->responseDelay()));
		std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(messageCounter, 0x80, 0x02, _address, destinationAddress, payload));
		physicalInterface->sendPacket(ok);
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

void HomeMaticDevice::sendOK(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x00);
		std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(messageCounter, 0x80, 0x02, _address, destinationAddress, payload));
		sendPacket(getPhysicalInterface(destinationAddress), ok);
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

void HomeMaticDevice::sendOKWithPayload(int32_t messageCounter, int32_t destinationAddress, std::vector<uint8_t> payload, bool isWakeMeUpPacket)
{
	try
	{
		uint8_t controlByte = 0x80;
		if(isWakeMeUpPacket) controlByte &= 0x02;
		std::shared_ptr<BidCoSPacket> ok(new BidCoSPacket(messageCounter, 0x80, 0x02, _address, destinationAddress, payload));
		sendPacket(getPhysicalInterface(destinationAddress), ok);
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

void HomeMaticDevice::sendNOK(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x80);
		std::shared_ptr<BidCoSPacket> nok(new BidCoSPacket(messageCounter, 0x80, 0x02, _address, destinationAddress, payload));
		sendPacket(getPhysicalInterface(destinationAddress), nok);
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

void HomeMaticDevice::sendNOKTargetInvalid(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x84);
		std::shared_ptr<BidCoSPacket> nok(new BidCoSPacket(messageCounter, 0x80, 0x02, _address, destinationAddress, payload));
		sendPacket(getPhysicalInterface(destinationAddress), nok);
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

void HomeMaticDevice::sendConfigParams(int32_t messageCounter, int32_t destinationAddress, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		uint32_t channel = packet->payload()->at(0);
		if(_config[channel][_currentList].size() >= 8)
		{
			std::shared_ptr<BidCoSQueue> queue = _bidCoSQueueManager.createQueue(this, getPhysicalInterface(destinationAddress), BidCoSQueueType::DEFAULT, destinationAddress);
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
			sendPacket(getPhysicalInterface(destinationAddress), config);
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

void HomeMaticDevice::sendPairingRequest()
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(_firmwareVersion);
		payload.push_back(_deviceType >> 8);
		payload.push_back(_deviceType & 0xFF);
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
		sendPacket(GD::defaultPhysicalInterface, pairingRequest);
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

void HomeMaticDevice::sendDirectedPairingRequest(int32_t messageCounter, int32_t controlByte, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		int32_t deviceType = (packet->payload()->at(1) << 8) + packet->payload()->at(2);
		if(_deviceTypeChannels.find(deviceType) == _deviceTypeChannels.end()) return;
		std::vector<uint8_t> payload;
		payload.push_back(_firmwareVersion);
		payload.push_back(_deviceType >> 8);
		payload.push_back(_deviceType & 0xFF);
		std::for_each(_serialNumber.begin(), _serialNumber.end(), [&](unsigned char byte)
		{
			payload.push_back(byte);
		});
		payload.push_back(_deviceClass);
		payload.push_back(_channelMin);
		payload.push_back(_deviceTypeChannels[deviceType]);
		payload.push_back(0x00);
		std::shared_ptr<BidCoSPacket> pairingRequest(new BidCoSPacket(messageCounter, 0xA0, 0x00, _address, packet->senderAddress(), payload));
		sendPacket(getPhysicalInterface(packet->senderAddress()), pairingRequest);
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

bool HomeMaticDevice::peerExists(uint64_t id)
{
	try
	{
		_peersMutex.lock();
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

void HomeMaticDevice::addPeer(std::shared_ptr<BidCoSPeer> peer)
{
	try
	{
		_peersMutex.lock();
		if(_peers.find(peer->getAddress()) == _peers.end()) _peers[peer->getAddress()] = peer;
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

std::shared_ptr<BidCoSPeer> HomeMaticDevice::getPeer(int32_t address)
{
	try
	{
		_peersMutex.lock();
		if(_peers.find(address) != _peers.end())
		{
			std::shared_ptr<BidCoSPeer> peer(std::dynamic_pointer_cast<BidCoSPeer>(_peers.at(address)));
			_peersMutex.unlock();
			return peer;
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
    return std::shared_ptr<BidCoSPeer>();
}

std::shared_ptr<BidCoSPeer> HomeMaticDevice::getPeer(uint64_t id)
{
	try
	{
		_peersMutex.lock();
		if(_peersByID.find(id) != _peersByID.end())
		{
			std::shared_ptr<BidCoSPeer> peer(std::dynamic_pointer_cast<BidCoSPeer>(_peersByID.at(id)));
			_peersMutex.unlock();
			return peer;
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
    return std::shared_ptr<BidCoSPeer>();
}

std::shared_ptr<BidCoSPeer> HomeMaticDevice::getPeer(std::string serialNumber)
{
	try
	{
		_peersMutex.lock();
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end())
		{
			std::shared_ptr<BidCoSPeer> peer(std::dynamic_pointer_cast<BidCoSPeer>(_peersBySerial.at(serialNumber)));
			_peersMutex.unlock();
			return peer;
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
    return std::shared_ptr<BidCoSPeer>();
}

std::shared_ptr<BidCoSQueue> HomeMaticDevice::getQueue(int32_t address)
{
	return _bidCoSQueueManager.get(address);
}

int32_t HomeMaticDevice::calculateCycleLength(uint8_t messageCounter)
{
	int32_t result = (((_address << 8) | messageCounter) * 1103515245 + 12345) >> 16;
	return (result & 0xFF) + 480;
}

//BidCoSQueueManager event handling
void HomeMaticDevice::onQueueCreateSavepoint(std::string name)
{
	raiseCreateSavepoint(name);
}

void HomeMaticDevice::onQueueReleaseSavepoint(std::string name)
{
	raiseReleaseSavepoint(name);
}
//End BidCoSQueueManager event handling

}
