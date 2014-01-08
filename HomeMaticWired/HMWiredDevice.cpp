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
#include "../GD.h"
#include "../HelperFunctions.h"

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

void HMWiredDevice::init()
{
	try
	{
		if(_initialized) return; //Prevent running init two times

		GD::physicalDevices.get(DeviceFamily::HomeMaticWired)->addLogicalDevice(this);

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

void HMWiredDevice::load()
{
	try
	{
		HelperFunctions::printDebug("Loading HomeMatic Wired device 0x" + HelperFunctions::getHexString(_address, 6));

		loadVariables();
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
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn((uint32_t)_deviceType.id())));
			int32_t result = GD::db.executeWriteCommand("REPLACE INTO devices VALUES(?, ?, ?, ?)", data);
			if(_deviceID == 0) _deviceID = result;
			_databaseMutex.unlock();
		}
		saveVariables();
	}
    catch(const std::exception& ex)
    {
    	_databaseMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_databaseMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_databaseMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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

void HMWiredDevice::saveVariables()
{
	try
	{
		if(_deviceID == 0) return;
		saveVariable(0, _firmwareVersion);
		saveVariable(1, _centralAddress);
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

void HMWiredDevice::sendPacket(std::shared_ptr<HMWiredPacket> packet, bool stealthy)
{
	try
	{
		/*uint32_t responseDelay = GD::physicalDevices.get(DeviceFamily::HomeMaticWired)->responseDelay();
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
		else HelperFunctions::printDebug("Debug: Sending packet " + packet->hexString() + " immediately, because it seems it is no response (no packet information found).", 7);*/
		GD::physicalDevices.get(DeviceFamily::HomeMaticWired)->sendPacket(packet);
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
}
