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
#include "../../GD/GD.h"
#include "HMWiredMessages.h"

namespace HMWired
{

HMWiredDevice::HMWiredDevice() : LogicalDevice()
{
}

HMWiredDevice::HMWiredDevice(uint32_t deviceID, std::string serialNumber, int32_t address) : LogicalDevice(deviceID, serialNumber, address)
{
}

HMWiredDevice::~HMWiredDevice()
{
	try
	{

	}
    catch(const std::exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }

}

void HMWiredDevice::init()
{
	try
	{
		if(_initialized) return; //Prevent running init two times
		_messages = std::shared_ptr<HMWiredMessages>(new HMWiredMessages());

		GD::physicalDevices.get(DeviceFamilies::HomeMaticWired)->addLogicalDevice(this);

		_messageCounter[0] = 0; //Broadcast message counter

		setUpHMWiredMessages();
		_initialized = true;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		Output::printDebug("Loading HomeMatic Wired device 0x" + HelperFunctions::getHexString(_address, 6));

		loadVariables();
	}
    catch(const std::exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		DataTable rows = GD::db.executeCommand("SELECT * FROM peers WHERE parent=" + std::to_string(_deviceID));
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			int32_t peerID = row->second.at(0)->intValue;
			int32_t address = row->second.at(2)->intValue;
			std::shared_ptr<HMWiredPeer> peer(new HMWiredPeer(peerID, address, row->second.at(3)->textValue, _deviceID, isCentral()));
			if(!peer->load(this)) continue;
			if(!peer->rpcDevice) continue;
			_peers[peer->getAddress()] = peer;
			if(!peer->getSerialNumber().empty()) _peersBySerial[peer->getSerialNumber()] = peer;
			_peersByID[peerID] = peer;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
    _peersMutex.unlock();
}

void HMWiredDevice::loadVariables()
{
	try
	{
		_databaseMutex.lock();
		DataTable rows = GD::db.executeCommand("SELECT * FROM deviceVariables WHERE deviceID=" + std::to_string(_deviceID));
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
			DataColumnVector data;
			if(_deviceID > 0) data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_deviceID)));
			else data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_address)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_serialNumber)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_deviceType)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn((uint32_t)DeviceFamilies::HomeMaticWired)));
			int32_t result = GD::db.executeWriteCommand("REPLACE INTO devices VALUES(?, ?, ?, ?, ?)", data);
			if(_deviceID == 0) _deviceID = result;
			_databaseMutex.unlock();
		}
		saveVariables();
	}
    catch(const std::exception& ex)
    {
    	_databaseMutex.unlock();
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_databaseMutex.unlock();
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_databaseMutex.unlock();
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredDevice::saveVariable(uint32_t index, int64_t intValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(intValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_variableDatabaseIDs[index])));
			GD::db.executeWriteCommand("UPDATE deviceVariables SET integerValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(index)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(intValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			int32_t result = GD::db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void HMWiredDevice::saveVariable(uint32_t index, std::string& stringValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(stringValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_variableDatabaseIDs[index])));
			GD::db.executeWriteCommand("UPDATE deviceVariables SET stringValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(index)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(stringValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			int32_t result = GD::db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void HMWiredDevice::saveVariable(uint32_t index, std::vector<uint8_t>& binaryValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(binaryValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_variableDatabaseIDs[index])));
			GD::db.executeWriteCommand("UPDATE deviceVariables SET binaryValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(index)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(binaryValue)));
			int32_t result = GD::db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _peersMutex.unlock();
    return std::shared_ptr<HMWiredPeer>();
}

std::shared_ptr<HMWiredPacket> HMWiredDevice::sendPacket(std::shared_ptr<HMWiredPacket> packet, bool resend, bool stealthy)
{
	try
	{
		//First check if communication is in progress
		int64_t time = HelperFunctions::getTime();
		std::shared_ptr<HMWiredPacketInfo> rxPacketInfo;
		std::shared_ptr<HMWiredPacketInfo> txPacketInfo = _sentPackets.getInfo(packet->destinationAddress());
		int64_t timeDifference = 0;
		if(txPacketInfo) timeDifference = time - txPacketInfo->time;
		std::shared_ptr<PhysicalDevices::PhysicalDevice> physicalDevice = GD::physicalDevices.get(DeviceFamilies::HomeMaticWired);
		if((!txPacketInfo || timeDifference > 210) && packet->type() != HMWiredPacketType::discovery)
		{
			rxPacketInfo = _receivedPackets.getInfo(packet->destinationAddress());
			int64_t rxTimeDifference = 0;
			if(rxPacketInfo) rxTimeDifference = time - rxPacketInfo->time;
			if(!rxPacketInfo || rxTimeDifference > 50)
			{
				//Communication might be in progress. Wait a little
				if(GD::debugLevel > 4 && (time - physicalDevice->lastPacketSent() < 210 || time - physicalDevice->lastPacketReceived() < 210)) Output::printDebug("Debug: HomeMatic Wired Device 0x" + HelperFunctions::getHexString(_deviceID) + ": Waiting for RS485 bus to become free...");
				while(time - physicalDevice->lastPacketSent() < 210 || time - physicalDevice->lastPacketReceived() < 210)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					time = HelperFunctions::getTime();
					if(time - physicalDevice->lastPacketSent() >= 210 && time - physicalDevice->lastPacketReceived() >= 210)
					{
						int32_t sleepingTime = HelperFunctions::getRandomNumber(0, 100);
						if(GD::debugLevel > 4) Output::printDebug("Debug: HomeMatic Wired Device 0x" + HelperFunctions::getHexString(_deviceID) + ": RS485 bus is free now. Waiting randomly for " + std::to_string(sleepingTime) + "ms...");
						//Sleep random time
						std::this_thread::sleep_for(std::chrono::milliseconds(sleepingTime));
						time = HelperFunctions::getTime();
					}
				}
				if(GD::debugLevel > 4) Output::printDebug("Debug: HomeMatic Wired Device 0x" + HelperFunctions::getHexString(_deviceID) + ": RS485 bus is still free... sending...");
			}
		}
		//RS485 bus should be free
		_sendMutex.lock();
		uint32_t responseDelay = physicalDevice->responseDelay();
		if(!stealthy) _sentPackets.set(packet->destinationAddress(), packet);
		if(txPacketInfo)
		{
			timeDifference = time - txPacketInfo->time;
			if(timeDifference < responseDelay)
			{
				txPacketInfo->time += responseDelay - timeDifference; //Set to sending time
				std::this_thread::sleep_for(std::chrono::milliseconds(responseDelay - timeDifference));
				time = HelperFunctions::getTime();
			}
		}
		if(stealthy) _sentPackets.keepAlive(packet->destinationAddress());
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
				time = HelperFunctions::getTime();
			}
			//Set time to now. This is necessary if two packets are sent after each other without a response in between
			rxPacketInfo->time = time;
		}
		else if(GD::debugLevel > 4) Output::printDebug("Debug: Sending HomeMatic Wired packet " + packet->hexString() + " immediately, because it seems it is no response (no packet information found).", 7);

		if(resend)
		{
			std::shared_ptr<HMWiredPacket> receivedPacket;
			for(int32_t retries = 0; retries < 3; retries++)
			{
				int64_t time = HelperFunctions::getTime();
				std::chrono::milliseconds sleepingTime(5);
				if(retries > 0) _sentPackets.keepAlive(packet->destinationAddress());
				physicalDevice->sendPacket(packet);
				for(int32_t i = 0; i < 12; i++)
				{
					if(i == 5) sleepingTime = std::chrono::milliseconds(25);
					std::this_thread::sleep_for(sleepingTime);
					receivedPacket = _receivedPackets.get(packet->destinationAddress());
					if(receivedPacket && receivedPacket->timeReceived() >= time)
					{
						_sendMutex.unlock();
						return receivedPacket;
					}
				}
			}
		}
		else physicalDevice->sendPacket(packet);
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _sendMutex.unlock();
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredDevice::serializeMessageCounters(std::vector<uint8_t>& encodedData)
{
	try
	{
		BinaryEncoder encoder;
		encoder.encodeInteger(encodedData, _messageCounter.size());
		for(std::unordered_map<int32_t, uint8_t>::const_iterator i = _messageCounter.begin(); i != _messageCounter.end(); ++i)
		{
			encoder.encodeInteger(encodedData, i->first);
			encoder.encodeByte(encodedData, i->second);
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredDevice::unserializeMessageCounters(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BinaryDecoder decoder;
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredDevice::deletePeersFromDatabase()
{
	try
	{
		_databaseMutex.lock();
		std::ostringstream command;
		command << "DELETE FROM peers WHERE parent=" << std::dec << _deviceID;
		GD::db.executeCommand(command.str());
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
			Output::printMessage("(Shutdown) => Saving HomeMatic Wired peer 0x" + HelperFunctions::getHexString(i->second->getAddress(), 6));
			i->second->save(full, full, full);
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
	_peersMutex.unlock();
}

bool HMWiredDevice::packetReceived(std::shared_ptr<Packet> packet)
{
	try
	{
		if(_disposing) return false;
		std::shared_ptr<HMWiredPacket> hmWiredPacket(std::dynamic_pointer_cast<HMWiredPacket>(packet));
		if(!hmWiredPacket) return false;
		_receivedPackets.set(hmWiredPacket->senderAddress(), hmWiredPacket, hmWiredPacket->timeReceived());
		if(hmWiredPacket->type() == HMWiredPacketType::ackMessage) handleAck(hmWiredPacket);
		else
		{
			std::shared_ptr<HMWiredMessage> message = _messages->find(DIRECTIONIN, hmWiredPacket);
			if(message && message->checkAccess(hmWiredPacket, _hmWiredQueueManager.get(hmWiredPacket->senderAddress())))
			{
				if(GD::debugLevel > 4) Output::printDebug("Debug: Device " + HelperFunctions::getHexString(_address) + ": Access granted for packet " + hmWiredPacket->hexString(), 6);
				message->invokeMessageHandlerIncoming(hmWiredPacket);
				return true;
			}
		}
	}
	catch(const std::exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
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
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return std::shared_ptr<HMWiredPacket>();
}

std::shared_ptr<HMWiredPacket> HMWiredDevice::getResponse(std::vector<uint8_t>& payload, int32_t destinationAddress, bool synchronizationBit)
{
	try
	{
		std::shared_ptr<HMWiredPacket> request(new HMWiredPacket(HMWiredPacketType::iMessage, _address, destinationAddress, synchronizationBit, _messageCounter[destinationAddress]++, 0, 0, payload));
		std::shared_ptr<HMWiredPacket> response = sendPacket(request, true);
		if(response) sendOK(response->senderMessageCounter(), destinationAddress);
		return response;
	}
	catch(const std::exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return std::shared_ptr<HMWiredPacket>();
}

std::vector<uint8_t> HMWiredDevice::readEEPROM(int32_t deviceAddress, int32_t eepromAddress)
{
	try
	{
		std::vector<uint8_t> payload;
		payload.push_back(0x52); //Command read EEPROM
		payload.push_back(eepromAddress >> 8);
		payload.push_back(eepromAddress & 0xFF);
		payload.push_back(0x10); //Bytes to read
		std::shared_ptr<HMWiredPacket> request(new HMWiredPacket(HMWiredPacketType::iMessage, _address, deviceAddress, false, _messageCounter[deviceAddress]++, 0, 0, payload));
		std::shared_ptr<HMWiredPacket> response = sendPacket(request, true);
		if(response)
		{
			sendOK(response->senderMessageCounter(), deviceAddress);
			return *response->payload();
		}
	}
	catch(const std::exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return std::vector<uint8_t>();
}

bool HMWiredDevice::writeEEPROM(int32_t deviceAddress, int32_t eepromAddress, std::vector<uint8_t>& data)
{
	try
	{
		if(data.size() > 32)
		{
			Output::printError("Error: HomeMatic Wired Device 0x" + HelperFunctions::getHexString(_address) + ": Could not write data to EEPROM. Data size is larger than 32 bytes.");
			return false;
		}
		std::vector<uint8_t> payload;
		payload.push_back(0x57); //Command write EEPROM
		payload.push_back(eepromAddress >> 8);
		payload.push_back(eepromAddress & 0xFF);
		payload.push_back(data.size()); //Bytes to write
		payload.insert(payload.end(), data.begin(), data.end());
		std::shared_ptr<HMWiredPacket> request(new HMWiredPacket(HMWiredPacketType::iMessage, _address, deviceAddress, false, _messageCounter[deviceAddress]++, 0, 0, payload));
		std::shared_ptr<HMWiredPacket> response = sendPacket(request, true);
		if(response) return true;
	}
	catch(const std::exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
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
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

}
