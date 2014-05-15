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

#include "HMWiredDevice.h"
#include "GD.h"

namespace HMWired
{

HMWiredDevice::HMWiredDevice(IDeviceEventSink* eventHandler) : LogicalDevice(eventHandler)
{
}

HMWiredDevice::HMWiredDevice(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : LogicalDevice(deviceID, serialNumber, address, eventHandler)
{
}

HMWiredDevice::~HMWiredDevice()
{
	try
	{
		dispose();
		//dispose might have been called by another thread, so wait until dispose is finished
		while(!_disposed) std::this_thread::sleep_for(std::chrono::milliseconds(50)); //Wait for received packets, without this, program sometimes SIGABRTs
	}
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }

}

void HMWiredDevice::dispose(bool wait)
{
	try
	{
		if(_disposing) return;
		_disposing = true;
		BaseLib::Output::printDebug("Removing device " + std::to_string(_deviceID) + " from physical device's event queue...");
		GD::physicalDevice->removeLogicalDevice(this);
		int64_t startTime = BaseLib::HelperFunctions::getTime();
		//stopThreads();
		int64_t timeDifference = BaseLib::HelperFunctions::getTime() - startTime;
		//Packets might still arrive, after removing this device from the rfDevice, so sleep a little bit
		//This is not necessary if the rfDevice doesn't listen anymore
		if(wait && timeDifference >= 0 && timeDifference < 2000) std::this_thread::sleep_for(std::chrono::milliseconds(2000 - timeDifference));
	}
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	_disposed = true;
}

void HMWiredDevice::init()
{
	try
	{
		if(_initialized) return; //Prevent running init two times

		GD::physicalDevice->addLogicalDevice(this);

		_messageCounter[0] = 0; //Broadcast message counter

		_initialized = true;
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool HMWiredDevice::isCentral()
{
	return _deviceType == (uint32_t)DeviceType::HMWIREDCENTRAL;
}

void HMWiredDevice::load()
{
	try
	{
		loadVariables();
	}
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredDevice::loadPeers(bool version_0_0_7)
{
	try
	{
		//Check for GD::devices for non unique access
		//Change peers identifier for device to id
		_peersMutex.lock();
		_databaseMutex.lock();
		BaseLib::DataTable rows = BaseLib::Obj::ins->db.executeCommand("SELECT * FROM peers WHERE parent=" + std::to_string(_deviceID));
		for(BaseLib::DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			int32_t peerID = row->second.at(0)->intValue;
			BaseLib::Output::printMessage("Loading HomeMatic Wired peer " + std::to_string(peerID));
			int32_t address = row->second.at(2)->intValue;
			std::shared_ptr<HMWiredPeer> peer(new HMWiredPeer(peerID, address, row->second.at(3)->textValue, _deviceID, isCentral(), this));
			if(!peer->load(this)) continue;
			if(!peer->rpcDevice) continue;
			_peers[peer->getAddress()] = peer;
			if(!peer->getSerialNumber().empty()) _peersBySerial[peer->getSerialNumber()] = peer;
			_peersByID[peerID] = peer;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
    _peersMutex.unlock();
}

void HMWiredDevice::loadVariables()
{
	try
	{
		_databaseMutex.lock();
		BaseLib::DataTable rows = BaseLib::Obj::ins->db.executeCommand("SELECT * FROM deviceVariables WHERE deviceID=" + std::to_string(_deviceID));
		for(BaseLib::DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
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
			}
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	_databaseMutex.unlock();
}

void HMWiredDevice::save(bool saveDevice)
{
	try
	{
		if(saveDevice)
		{
			_databaseMutex.lock();
			BaseLib::DataColumnVector data;
			if(_deviceID > 0) data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_deviceID)));
			else data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_address)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_serialNumber)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_deviceType)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn((uint32_t)BaseLib::Systems::DeviceFamilies::HomeMaticWired)));
			int32_t result = BaseLib::Obj::ins->db.executeWriteCommand("REPLACE INTO devices VALUES(?, ?, ?, ?, ?)", data);
			if(_deviceID == 0) _deviceID = result;
			_databaseMutex.unlock();
		}
		saveVariables();
	}
    catch(const std::exception& ex)
    {
    	_databaseMutex.unlock();
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_databaseMutex.unlock();
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_databaseMutex.unlock();
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredDevice::saveVariable(uint32_t index, int64_t intValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		BaseLib::DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(intValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_variableDatabaseIDs[index])));
			BaseLib::Obj::ins->db.executeWriteCommand("UPDATE deviceVariables SET integerValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(index)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(intValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			int32_t result = BaseLib::Obj::ins->db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void HMWiredDevice::saveVariable(uint32_t index, std::string& stringValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		BaseLib::DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(stringValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_variableDatabaseIDs[index])));
			BaseLib::Obj::ins->db.executeWriteCommand("UPDATE deviceVariables SET stringValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(index)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(stringValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			int32_t result = BaseLib::Obj::ins->db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void HMWiredDevice::saveVariable(uint32_t index, std::vector<uint8_t>& binaryValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		BaseLib::DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(binaryValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_variableDatabaseIDs[index])));
			BaseLib::Obj::ins->db.executeWriteCommand("UPDATE deviceVariables SET binaryValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(index)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(binaryValue)));
			int32_t result = BaseLib::Obj::ins->db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void HMWiredDevice::saveVariables()
{
	try
	{
		if(_deviceID == 0) return;
		saveVariable(0, _firmwareVersion);
		saveVariable(1, _centralAddress);
		saveMessageCounters(); //2
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredDevice::addPeer(std::shared_ptr<HMWiredPeer> peer)
{
	try
	{
		_peersMutex.lock();
		if(_peers.find(peer->getAddress()) == _peers.end()) _peers[peer->getAddress()] = peer;
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _peersMutex.unlock();
}

bool HMWiredDevice::peerExists(int32_t address)
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
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _peersMutex.unlock();
    return false;
}

bool HMWiredDevice::peerExists(uint64_t id)
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
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _peersMutex.unlock();
    return false;
}

std::shared_ptr<HMWiredPeer> HMWiredDevice::getPeer(int32_t address)
{
	try
	{
		_peersMutex.lock();
		if(_peers.find(address) != _peers.end())
		{
			std::shared_ptr<HMWiredPeer> peer(_peers.at(address));
			_peersMutex.unlock();
			return peer;
		}
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _peersMutex.unlock();
    return std::shared_ptr<HMWiredPeer>();
}

std::shared_ptr<HMWiredPeer> HMWiredDevice::getPeer(uint64_t id)
{
	try
	{
		_peersMutex.lock();
		if(_peersByID.find(id) != _peersByID.end())
		{
			std::shared_ptr<HMWiredPeer> peer(_peersByID.at(id));
			_peersMutex.unlock();
			return peer;
		}
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _peersMutex.unlock();
    return std::shared_ptr<HMWiredPeer>();
}

std::shared_ptr<HMWiredPeer> HMWiredDevice::getPeer(std::string serialNumber)
{
	try
	{
		_peersMutex.lock();
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end())
		{
			std::shared_ptr<HMWiredPeer> peer(_peersBySerial.at(serialNumber));
			_peersMutex.unlock();
			return peer;
		}
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _peersMutex.unlock();
    return std::shared_ptr<HMWiredPeer>();
}

std::shared_ptr<HMWiredPacket> HMWiredDevice::sendPacket(std::shared_ptr<HMWiredPacket> packet, bool resend, bool systemResponse)
{
	try
	{
		//First check if communication is in progress
		int64_t time = BaseLib::HelperFunctions::getTime();
		std::shared_ptr<HMWiredPacketInfo> rxPacketInfo;
		std::shared_ptr<HMWiredPacketInfo> txPacketInfo = _sentPackets.getInfo(packet->destinationAddress());
		int64_t timeDifference = 0;
		if(txPacketInfo) timeDifference = time - txPacketInfo->time;
		if((!txPacketInfo || timeDifference > 210) && packet->type() != HMWiredPacketType::discovery)
		{
			rxPacketInfo = _receivedPackets.getInfo(packet->destinationAddress());
			int64_t rxTimeDifference = 0;
			if(rxPacketInfo) rxTimeDifference = time - rxPacketInfo->time;
			if(!rxPacketInfo || rxTimeDifference > 50)
			{
				//Communication might be in progress. Wait a little
				if(BaseLib::Obj::ins->debugLevel > 4 && (time - GD::physicalDevice->lastPacketSent() < 210 || time - GD::physicalDevice->lastPacketReceived() < 210)) BaseLib::Output::printDebug("Debug: HomeMatic Wired Device 0x" + BaseLib::HelperFunctions::getHexString(_deviceID) + ": Waiting for RS485 bus to become free... (Packet: " + packet->hexString() + ")");
				while(time - GD::physicalDevice->lastPacketSent() < 210 || time - GD::physicalDevice->lastPacketReceived() < 210)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					time = BaseLib::HelperFunctions::getTime();
					if(time - GD::physicalDevice->lastPacketSent() >= 210 && time - GD::physicalDevice->lastPacketReceived() >= 210)
					{
						int32_t sleepingTime = BaseLib::HelperFunctions::getRandomNumber(0, 100);
						if(BaseLib::Obj::ins->debugLevel > 4) BaseLib::Output::printDebug("Debug: HomeMatic Wired Device 0x" + BaseLib::HelperFunctions::getHexString(_deviceID) + ": RS485 bus is free now. Waiting randomly for " + std::to_string(sleepingTime) + "ms... (Packet: " + packet->hexString() + ")");
						//Sleep random time
						std::this_thread::sleep_for(std::chrono::milliseconds(sleepingTime));
						time = BaseLib::HelperFunctions::getTime();
					}
				}
				if(BaseLib::Obj::ins->debugLevel > 4) BaseLib::Output::printDebug("Debug: HomeMatic Wired Device 0x" + BaseLib::HelperFunctions::getHexString(_deviceID) + ": RS485 bus is still free... sending... (Packet: " + packet->hexString() + ")");
			}
		}
		//RS485 bus should be free
		uint32_t responseDelay = GD::physicalDevice->responseDelay();
		_sentPackets.set(packet->destinationAddress(), packet);
		if(txPacketInfo)
		{
			timeDifference = time - txPacketInfo->time;
			if(timeDifference < responseDelay)
			{
				txPacketInfo->time += responseDelay - timeDifference; //Set to sending time
				std::this_thread::sleep_for(std::chrono::milliseconds(responseDelay - timeDifference));
				time = BaseLib::HelperFunctions::getTime();
			}
		}
		rxPacketInfo = _receivedPackets.getInfo(packet->destinationAddress());
		if(rxPacketInfo)
		{
			int64_t timeDifference = time - rxPacketInfo->time;
			if(timeDifference >= 0 && timeDifference < responseDelay)
			{
				int64_t sleepingTime = responseDelay - timeDifference;
				if(sleepingTime > 1) sleepingTime -= 1;
				packet->setTimeSending(time + sleepingTime + 1);
				std::this_thread::sleep_for(std::chrono::milliseconds(sleepingTime));
				time = BaseLib::HelperFunctions::getTime();
			}
			//Set time to now. This is necessary if two packets are sent after each other without a response in between
			rxPacketInfo->time = time;
		}
		else if(BaseLib::Obj::ins->debugLevel > 4) BaseLib::Output::printDebug("Debug: Sending HomeMatic Wired packet " + packet->hexString() + " immediately, because it seems it is no response (no packet information found).", 7);

		if(resend)
		{
			std::shared_ptr<HMWiredPacket> receivedPacket;
			for(int32_t retries = 0; retries < 3; retries++)
			{
				int64_t time = BaseLib::HelperFunctions::getTime();
				std::chrono::milliseconds sleepingTime(5);
				if(retries > 0) _sentPackets.keepAlive(packet->destinationAddress());
				GD::physicalDevice->sendPacket(packet);
				for(int32_t i = 0; i < 12; i++)
				{
					if(i == 5) sleepingTime = std::chrono::milliseconds(25);
					std::this_thread::sleep_for(sleepingTime);
					receivedPacket = systemResponse ? _receivedPackets.get(0) : _receivedPackets.get(packet->destinationAddress());
					if(receivedPacket && receivedPacket->timeReceived() >= time && receivedPacket->receiverMessageCounter() == packet->senderMessageCounter())
					{
						return receivedPacket;
					}
				}
			}
			std::shared_ptr<HMWiredPeer> peer = getPeer(packet->destinationAddress());
			if(peer) peer->serviceMessages->setUnreach(true);
		}
		else GD::physicalDevice->sendPacket(packet);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<HMWiredPacket>();
}

void HMWiredDevice::saveMessageCounters()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializeMessageCounters(serializedData);
		saveVariable(2, serializedData);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredDevice::serializeMessageCounters(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder;
		encoder.encodeInteger(encodedData, _messageCounter.size());
		for(std::unordered_map<int32_t, uint8_t>::const_iterator i = _messageCounter.begin(); i != _messageCounter.end(); ++i)
		{
			encoder.encodeInteger(encodedData, i->first);
			encoder.encodeByte(encodedData, i->second);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredDevice::unserializeMessageCounters(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BaseLib::BinaryDecoder decoder;
		uint32_t position = 0;
		uint32_t messageCounterSize = decoder.decodeInteger(serializedData, position);
		for(uint32_t i = 0; i < messageCounterSize; i++)
		{
			int32_t index = decoder.decodeInteger(serializedData, position);
			_messageCounter[index] = decoder.decodeByte(serializedData, position);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredDevice::deletePeersFromDatabase()
{
	try
	{
		_databaseMutex.lock();
		std::ostringstream command;
		command << "DELETE FROM peers WHERE parent=" << std::dec << _deviceID;
		BaseLib::Obj::ins->db.executeCommand(command.str());
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void HMWiredDevice::savePeers(bool full)
{
	try
	{
		_peersMutex.lock();
		_databaseMutex.lock();
		for(std::unordered_map<int32_t, std::shared_ptr<HMWiredPeer>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			//Necessary, because peers can be assigned to multiple virtual devices
			if(i->second->getParentID() != _deviceID) continue;
			//We are always printing this, because the init script needs it
			BaseLib::Output::printMessage("(Shutdown) => Saving HomeMatic Wired peer " + std::to_string(i->second->getID()));
			i->second->save(full, full, full);
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
	_peersMutex.unlock();
}

bool HMWiredDevice::packetReceived(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(_disposing) return false;
		std::shared_ptr<HMWiredPacket> hmWiredPacket(std::dynamic_pointer_cast<HMWiredPacket>(packet));
		if(!hmWiredPacket) return false;
		_receivedPackets.set(hmWiredPacket->senderAddress(), hmWiredPacket, hmWiredPacket->timeReceived());
	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

void HMWiredDevice::lockBus()
{
	try
	{
		std::vector<uint8_t> payload = { 0x7A };
		std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(HMWiredPacketType::iMessage, _address, 0xFFFFFFFF, true, _messageCounter[0]++, 0, 0, payload));
		sendPacket(packet, false);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		packet.reset(new HMWiredPacket(HMWiredPacketType::iMessage, _address, 0xFFFFFFFF, true, _messageCounter[0]++, 0, 0, payload));
		sendPacket(packet, false);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredDevice::unlockBus()
{
	try
	{
		std::vector<uint8_t> payload = { 0x5A };
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(HMWiredPacketType::iMessage, _address, 0xFFFFFFFF, true, _messageCounter[0]++, 0, 0, payload));
		sendPacket(packet, false);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		packet.reset(new HMWiredPacket(HMWiredPacketType::iMessage, _address, 0xFFFFFFFF, true, _messageCounter[0]++, 0, 0, payload));
		sendPacket(packet, false);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

uint8_t HMWiredDevice::getMessageCounter(int32_t destinationAddress)
{
	try
	{
		std::shared_ptr<HMWiredPeer> peer = getPeer(destinationAddress);
		uint8_t messageCounter = 0;
		if(peer)
		{
			messageCounter = peer->getMessageCounter();
			peer->setMessageCounter(messageCounter + 1);
		}
		else messageCounter = _messageCounter[destinationAddress]++;
		return messageCounter;
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return 0;
}

std::shared_ptr<HMWiredPacket> HMWiredDevice::getResponse(uint8_t command, int32_t destinationAddress, bool synchronizationBit)
{
	try
	{
		std::vector<uint8_t> payload({command});
		return getResponse(payload, destinationAddress, synchronizationBit);
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return std::shared_ptr<HMWiredPacket>();
}

std::shared_ptr<HMWiredPacket> HMWiredDevice::getResponse(std::vector<uint8_t>& payload, int32_t destinationAddress, bool synchronizationBit)
{
	std::shared_ptr<HMWiredPeer> peer = getPeer(destinationAddress);
	try
	{
		uint8_t messageCounter = 0;
		if(peer) peer->ignorePackets = true;
		std::shared_ptr<HMWiredPacket> request(new HMWiredPacket(HMWiredPacketType::iMessage, _address, destinationAddress, synchronizationBit, getMessageCounter(destinationAddress), 0, 0, payload));
		std::shared_ptr<HMWiredPacket> response = sendPacket(request, true);
		if(response && response->type() != HMWiredPacketType::ackMessage) sendOK(response->senderMessageCounter(), destinationAddress);
		if(peer) peer->ignorePackets = false;
		return response;
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	if(peer) peer->ignorePackets = false;
	return std::shared_ptr<HMWiredPacket>();
}

std::shared_ptr<HMWiredPacket> HMWiredDevice::getResponse(std::shared_ptr<HMWiredPacket> packet, bool systemResponse)
{
	std::shared_ptr<HMWiredPeer> peer = getPeer(packet->destinationAddress());
	try
	{
		if(peer) peer->ignorePackets = true;
		std::shared_ptr<HMWiredPacket> request(packet);
		std::shared_ptr<HMWiredPacket> response = sendPacket(request, true, systemResponse);
		if(response && response->type() != HMWiredPacketType::ackMessage && response->type() != HMWiredPacketType::system) sendOK(response->senderMessageCounter(), packet->destinationAddress());
		if(peer) peer->ignorePackets = false;
		return response;
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	if(peer) peer->ignorePackets = false;
	return std::shared_ptr<HMWiredPacket>();
}

std::vector<uint8_t> HMWiredDevice::readEEPROM(int32_t deviceAddress, int32_t eepromAddress)
{
	std::shared_ptr<HMWiredPeer> peer = getPeer(deviceAddress);
	try
	{
		if(peer) peer->ignorePackets = true;
		std::vector<uint8_t> payload;
		payload.push_back(0x52); //Command read EEPROM
		payload.push_back(eepromAddress >> 8);
		payload.push_back(eepromAddress & 0xFF);
		payload.push_back(0x10); //Bytes to read
		std::shared_ptr<HMWiredPacket> request(new HMWiredPacket(HMWiredPacketType::iMessage, _address, deviceAddress, false, getMessageCounter(deviceAddress), 0, 0, payload));
		std::shared_ptr<HMWiredPacket> response = sendPacket(request, true);
		if(response)
		{
			sendOK(response->senderMessageCounter(), deviceAddress);
			if(peer) peer->ignorePackets = false;
			return *response->payload();
		}
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	if(peer) peer->ignorePackets = false;
	return std::vector<uint8_t>();
}

bool HMWiredDevice::writeEEPROM(int32_t deviceAddress, int32_t eepromAddress, std::vector<uint8_t>& data)
{
	std::shared_ptr<HMWiredPeer> peer = getPeer(deviceAddress);
	try
	{
		if(data.size() > 32)
		{
			BaseLib::Output::printError("Error: HomeMatic Wired Device " + std::to_string(_deviceID) + ": Could not write data to EEPROM. Data size is larger than 32 bytes.");
			return false;
		}
		if(peer) peer->ignorePackets = true;
		std::vector<uint8_t> payload;
		payload.push_back(0x57); //Command write EEPROM
		payload.push_back(eepromAddress >> 8);
		payload.push_back(eepromAddress & 0xFF);
		payload.push_back(data.size()); //Bytes to write
		payload.insert(payload.end(), data.begin(), data.end());
		std::shared_ptr<HMWiredPacket> request(new HMWiredPacket(HMWiredPacketType::iMessage, _address, deviceAddress, false, getMessageCounter(deviceAddress), 0, 0, payload));
		std::shared_ptr<HMWiredPacket> response = sendPacket(request, true);
		if(response)
		{
			if(peer) peer->ignorePackets = false;
			return true;
		}
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	if(peer) peer->ignorePackets = false;
	return false;
}

void HMWiredDevice::sendOK(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		std::shared_ptr<HMWiredPacket> ackPacket(new HMWiredPacket(HMWiredPacketType::ackMessage, _address, destinationAddress, false, 0, messageCounter, 0, payload));
		sendPacket(ackPacket, false);
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

}
