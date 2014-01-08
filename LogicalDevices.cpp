/* Copyright 2013 Sathya Laufer
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

#include "LogicalDevices.h"
#include "GD.h"
#include "HomeMaticBidCoS/Devices/HM-CC-VD.h"
#include "HomeMaticBidCoS/Devices/HM-CC-TC.h"
#include "HomeMaticBidCoS/Devices/HM-SD.h"
#include "HomeMaticWired/Devices/HMWired-SD.h"
#include "HelperFunctions.h"

LogicalDevices::LogicalDevices()
{
}

LogicalDevices::~LogicalDevices()
{
	try
	{
		if(_removeThread.joinable()) _removeThread.join();
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

void LogicalDevices::convertDatabase()
{
	try
	{
		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(0)));
		DataTable result = GD::db.executeCommand("SELECT * FROM homegearVariables WHERE variableIndex=?", data);
		if(result.empty()) return; //Handled in initializeDatabase
		std::string version = result.at(0).at(3)->textValue;
		if(version == "0.3.0") return; //Up to date
		if(version != "0.0.7")
		{
			HelperFunctions::printCritical("Unknown database version: " + version);
			exit(1); //Don't know, what to do
		}
		HelperFunctions::printMessage("Converting database from version " + version + " to version 0.3.0...");
		GD::db.init(GD::settings.databasePath(), GD::settings.databasePath() + ".old");

		GD::db.executeCommand("ALTER TABLE events ADD COLUMN enabled INTEGER");
		GD::db.executeCommand("UPDATE events SET enabled=1");

		loadDevicesFromDatabase(true);
		save(true);

		data.clear();
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(result.at(0).at(0)->intValue)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(0)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn("0.3.0")));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
		GD::db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

		HelperFunctions::printMessage("Exiting Homegear after database conversion...");
		exit(0);
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

void LogicalDevices::initializeDatabase()
{
	try
	{
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS homegearVariables (variableID INTEGER PRIMARY KEY UNIQUE, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		GD::db.executeCommand("CREATE INDEX IF NOT EXISTS homegearVariablesIndex ON homegearVariables (variableID, variableIndex)");
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS peers (peerID INTEGER PRIMARY KEY UNIQUE, parent INTEGER NOT NULL, address INTEGER NOT NULL, serialNumber TEXT NOT NULL)");
		GD::db.executeCommand("CREATE INDEX IF NOT EXISTS peersIndex ON peers (peerID, parent, address, serialNumber)");
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS peerVariables (variableID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		GD::db.executeCommand("CREATE INDEX IF NOT EXISTS peerVariablesIndex ON peerVariables (variableID, peerID, variableIndex)");
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS parameters (parameterID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, parameterSetType INTEGER NOT NULL, peerChannel INTEGER NOT NULL, remotePeer INTEGER, remoteChannel INTEGER, parameterName TEXT, value BLOB)");
		GD::db.executeCommand("CREATE INDEX IF NOT EXISTS parametersIndex ON parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName)");
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS metadata (objectID TEXT, dataID TEXT, serializedObject BLOB)");
		GD::db.executeCommand("CREATE INDEX IF NOT EXISTS metadataIndex ON metadata (objectID, dataID)");
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS devices (deviceID INTEGER PRIMARY KEY UNIQUE, address INTEGER NOT NULL, serialNumber TEXT NOT NULL, deviceType INTEGER NOT NULL)");
		GD::db.executeCommand("CREATE INDEX IF NOT EXISTS devicesIndex ON devices (deviceID, address, deviceType)");
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS deviceVariables (variableID INTEGER PRIMARY KEY UNIQUE, deviceID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		GD::db.executeCommand("CREATE INDEX IF NOT EXISTS deviceVariablesIndex ON deviceVariables (variableID, deviceID, variableIndex)");
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS users (userID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, password BLOB NOT NULL, salt BLOB NOT NULL)");
		GD::db.executeCommand("CREATE INDEX IF NOT EXISTS usersIndex ON users (userID, name)");
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS events (eventID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, type INTEGER NOT NULL, address TEXT, variable TEXT, trigger INTEGER, triggerValue BLOB, eventMethod TEXT, eventMethodParameters BLOB, resetAfter INTEGER, initialTime INTEGER, timeOperation INTEGER, timeFactor REAL, timeLimit INTEGER, resetMethod TEXT, resetMethodParameters BLOB, eventTime INTEGER, endTime INTEGER, recurEvery INTEGER, lastValue BLOB, lastRaised INTEGER, lastReset INTEGER, currentTime INTEGER, enabled INTEGER)");
		GD::db.executeCommand("CREATE INDEX IF NOT EXISTS eventsIndex ON events (eventID, name, type, address)");

		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(0)));
		DataTable result = GD::db.executeCommand("SELECT 1 FROM homegearVariables WHERE variableIndex=?", data);
		if(result.empty())
		{
			data.clear();
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(0)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn("0.0.7")));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			GD::db.executeCommand("INSERT INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
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

void LogicalDevices::loadDevicesFromDatabase(bool version_0_0_7)
{
	try
	{
		DataTable rows = GD::db.executeCommand("SELECT * FROM devices");
		bool bidCoSSpyDeviceExists = false;
		bool hmWiredSpyDeviceExists = false;
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			uint32_t deviceID = row->second.at(0)->intValue;
			int32_t address = row->second.at(1)->intValue;
			std::string serialNumber = row->second.at(2)->textValue;
			DeviceID deviceTypeID = (DeviceID)row->second.at(3)->intValue;

			std::shared_ptr<LogicalDevice> device;
			switch(deviceTypeID)
			{
			case DeviceID::HMCCTC:
				device = std::shared_ptr<LogicalDevice>(new HM_CC_TC(deviceID, serialNumber, address));
				break;
			case DeviceID::HMLCSW1FM:
				device = std::shared_ptr<LogicalDevice>(new HM_LC_SWX_FM(deviceID, serialNumber, address));
				break;
			case DeviceID::HMCCVD:
				device = std::shared_ptr<LogicalDevice>(new HM_CC_VD(deviceID, serialNumber, address));
				break;
			case DeviceID::HMCENTRAL:
				_homeMaticCentral = std::shared_ptr<HomeMaticCentral>(new HomeMaticCentral(deviceID, serialNumber, address));
				device = _homeMaticCentral;
				break;
			case DeviceID::HMSD:
				bidCoSSpyDeviceExists = true;
				device = std::shared_ptr<LogicalDevice>(new HM_SD(deviceID, serialNumber, address));
				break;
			case DeviceID::HMWIREDSD:
				hmWiredSpyDeviceExists = true;
				device = std::shared_ptr<LogicalDevice>(new HMWired::HMWired_SD(deviceID, serialNumber, address));
			default:
				break;
			}
			if(device)
			{
				device->load();
				device->loadPeers(version_0_0_7);
				_devicesMutex.lock();
				_devices.push_back(device);
				_devicesMutex.unlock();
			}
		}
		if(!_homeMaticCentral) createBidCoSCentral();
		if(!bidCoSSpyDeviceExists) createBidCoSSpyDevice();
		if(_homeMaticCentral) _homeMaticCentral->addPeersToVirtualDevices();
		if(!hmWiredSpyDeviceExists) createHMWiredSpyDevice();
	}
	catch(const std::exception& ex)
    {
		_devicesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_devicesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_devicesMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void LogicalDevices::load()
{
	try
	{
		initializeDatabase();
		loadDevicesFromDatabase(false);
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

void LogicalDevices::createBidCoSSpyDevice()
{
	try
	{
		if(!_homeMaticCentral) return;

		int32_t seed = 0xfe0000 + HelperFunctions::getRandomNumber(1, 500);

		int32_t address = _homeMaticCentral->getUniqueAddress(seed);
		std::string serialNumber(getUniqueHomeMaticSerialNumber("VSD", HelperFunctions::getRandomNumber(1, 9999999)));

		add(new HM_SD(0, serialNumber, address));
		HelperFunctions::printMessage("Created HM_SD with address 0x" + HelperFunctions::getHexString(address) + " and serial number " + serialNumber);
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

void LogicalDevices::createHMWiredSpyDevice()
{
	try
	{
		uint32_t seed = 0xfe000000 + HelperFunctions::getRandomNumber(1, 65535);

		//ToDo: Use HMWiredCentral to get unique address
		int32_t address = getUniqueHMWiredAddress(seed);
		std::string serialNumber(getUniqueHomeMaticSerialNumber("VSD", HelperFunctions::getRandomNumber(1, 9999999)));

		add(new HMWired::HMWired_SD(0, serialNumber, address));
		HelperFunctions::printMessage("Created HMWired_SD with address 0x" + HelperFunctions::getHexString(address) + " and serial number " + serialNumber);
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


void LogicalDevices::createBidCoSCentral()
{
	try
	{
		if(_homeMaticCentral) return;

		int32_t address = getUniqueBidCoSAddress(0xfd);
		std::string serialNumber(getUniqueHomeMaticSerialNumber("VCL", HelperFunctions::getRandomNumber(1, 9999999)));

		add(new HomeMaticCentral(0, serialNumber, address));
		HelperFunctions::printMessage("Created HMCENTRAL with address 0x" + HelperFunctions::getHexString(address) + " and serial number " + serialNumber);
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

std::shared_ptr<HomeMaticCentral> LogicalDevices::getHomeMaticCentral()
{
	try
	{
		_devicesMutex.lock();
		if(!_homeMaticCentral)
		{
			_devicesMutex.unlock();
			return std::shared_ptr<HomeMaticCentral>();
		}

		std::shared_ptr<HomeMaticCentral> central(_homeMaticCentral);
		_devicesMutex.unlock();
		return central;
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
    _devicesMutex.unlock();
    return std::shared_ptr<HomeMaticCentral>();
}

int32_t LogicalDevices::getUniqueBidCoSAddress(uint8_t firstByte)
{
	int32_t prefix = firstByte << 16;
	int32_t seed = HelperFunctions::getRandomNumber(1, 9999);
	uint32_t i = 0;
	while(getHomeMatic(prefix + seed) && i++ < 10000)
	{
		seed += 13;
		if(seed > 9999) seed -= 10000;
	}
	return prefix + seed;
}

uint32_t LogicalDevices::getUniqueHMWiredAddress(uint32_t seed)
{
	uint32_t prefix = seed << 24;
	seed = HelperFunctions::getRandomNumber(1, 999999);
	uint32_t i = 0;
	while(getHMWired(prefix + seed) && i++ < 10000)
	{
		seed += 131;
		if(seed > 999999) seed -= 1000000;
	}
	return prefix + seed;
}

std::string LogicalDevices::getUniqueHomeMaticSerialNumber(std::string seedPrefix, uint32_t seedNumber)
{
	if(seedPrefix.size() != 3) throw Exception("seedPrefix must have a size of 3.");
	uint32_t i = 0;
	std::ostringstream stringstream;
	stringstream << seedPrefix << std::setw(7) << std::setfill('0') << std::dec << seedNumber;
	std::string temp2 = stringstream.str();
	while((getHomeMatic(temp2) || getHMWired(temp2)) && i++ < 100000)
	{
		stringstream.str(std::string());
		stringstream.clear();
		seedNumber += 73;
		if(seedNumber > 9999999) seedNumber -= 10000000;
		std::ostringstream stringstream;
		stringstream << seedPrefix << std::setw(7) << std::setfill('0') << std::dec << seedNumber;
		temp2 = stringstream.str();
	}
	return temp2;
}

void LogicalDevices::dispose()
{
	try
	{
		std::vector<std::shared_ptr<LogicalDevice>> devices;
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			HelperFunctions::printDebug("Debug: Disposing device 0x" + HelperFunctions::getHexString((*i)->getAddress()));
			devices.push_back(*i);
		}
		_devicesMutex.unlock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = devices.begin(); i != devices.end(); ++i)
		{
			(*i)->dispose(false);
		}
	}
	catch(const std::exception& ex)
    {
		_devicesMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_devicesMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_devicesMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

void LogicalDevices::save(bool full, bool crash)
{
	try
	{
		if(!crash)
		{
			HelperFunctions::printMessage("(Shutdown) => Waiting for threads");
			//The disposing is necessary, because there is a small time gap between setting "_lastDutyCycleEvent" and the duty cycle message counter.
			//If saving takes place within this gap, the paired duty cycle devices are out of sync after restart of the program.
			dispose();
		}
		HelperFunctions::printMessage("(Shutdown) => Saving devices");
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			HelperFunctions::printMessage("(Shutdown) => Saving " + DeviceFamilies::getName((*i)->deviceFamily()) + " device 0x" + HelperFunctions::getHexString((*i)->getAddress(), 6));
			(*i)->save(full);
			(*i)->savePeers(full);
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
    _devicesMutex.unlock();
}

void LogicalDevices::add(LogicalDevice* device)
{
	try
	{
		if(!device) return;
		_devicesMutex.lock();
		device->save(true);
		if(device->getDeviceType().id() == DeviceID::HMCENTRAL)
		{
			_homeMaticCentral = std::shared_ptr<HomeMaticCentral>((HomeMaticCentral*)device);
			_devices.push_back(_homeMaticCentral);
		}
		else
		{
			_devices.push_back(std::shared_ptr<LogicalDevice>(device));
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
    _devicesMutex.unlock();
}

void LogicalDevices::remove(int32_t address)
{
	try
	{
		if(_removeThread.joinable()) _removeThread.join();
		_removeThread = std::thread(&LogicalDevices::removeThread, this, address);
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

void LogicalDevices::removeThread(int32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getAddress() == address)
			{
				HelperFunctions::printDebug("Removing device pointer from device array...");
				std::shared_ptr<LogicalDevice> device = *i;
				_devices.erase(i);
				_devicesMutex.unlock();
				HelperFunctions::printDebug("Disposing device...");
				device->dispose(true);
				HelperFunctions::printDebug("Deleting peers from database...");
				device->deletePeersFromDatabase();
				HelperFunctions::printDebug("Deleting database entry...");
				DataColumnVector data;
				data.push_back(std::shared_ptr<DataColumn>(new DataColumn(address)));
				GD::db.executeCommand("DELETE FROM devices WHERE address=?", data);
				return;
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
    _devicesMutex.unlock();
}

std::shared_ptr<LogicalDevice> LogicalDevices::get(int32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getAddress() == address)
			{
				_devicesMutex.unlock();
				return (*i);
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
    _devicesMutex.unlock();
	return std::shared_ptr<LogicalDevice>();
}

std::shared_ptr<HomeMaticDevice> LogicalDevices::getHomeMatic(int32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->deviceFamily() == DeviceFamily::HomeMaticBidCoS && (*i)->getAddress() == address)
			{
				std::shared_ptr<HomeMaticDevice> device(std::dynamic_pointer_cast<HomeMaticDevice>(*i));
				if(!device) continue;
				_devicesMutex.unlock();
				return device;
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
    _devicesMutex.unlock();
	return std::shared_ptr<HomeMaticDevice>();
}

std::shared_ptr<HMWired::HMWiredDevice> LogicalDevices::getHMWired(int32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->deviceFamily() == DeviceFamily::HomeMaticWired && (*i)->getAddress() == address)
			{
				std::shared_ptr<HMWired::HMWiredDevice> device(std::dynamic_pointer_cast<HMWired::HMWiredDevice>(*i));
				if(!device) continue;
				_devicesMutex.unlock();
				return device;
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
    _devicesMutex.unlock();
	return std::shared_ptr<HMWired::HMWiredDevice>();
}

std::vector<std::shared_ptr<LogicalDevice>> LogicalDevices::getDevices()
{
	try
	{
		_devicesMutex.lock();
		std::vector<std::shared_ptr<LogicalDevice>> devices;
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			devices.push_back(*i);
		}
		_devicesMutex.unlock();
		return devices;
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
    _devicesMutex.unlock();
	return std::vector<std::shared_ptr<LogicalDevice>>();
}

std::shared_ptr<LogicalDevice> LogicalDevices::get(std::string serialNumber)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getSerialNumber() == serialNumber)
			{
				_devicesMutex.unlock();
				return (*i);
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
    _devicesMutex.unlock();
	return std::shared_ptr<HomeMaticDevice>();
}

std::shared_ptr<HomeMaticDevice> LogicalDevices::getHomeMatic(std::string serialNumber)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->deviceFamily() == DeviceFamily::HomeMaticBidCoS && (*i)->getSerialNumber() == serialNumber)
			{
				std::shared_ptr<HomeMaticDevice> device(std::dynamic_pointer_cast<HomeMaticDevice>(*i));
				if(!device) continue;
				_devicesMutex.unlock();
				return device;
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
    _devicesMutex.unlock();
	return std::shared_ptr<HomeMaticDevice>();
}


std::shared_ptr<HMWired::HMWiredDevice> LogicalDevices::getHMWired(std::string serialNumber)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->deviceFamily() == DeviceFamily::HomeMaticWired && (*i)->getSerialNumber() == serialNumber)
			{
				std::shared_ptr<HMWired::HMWiredDevice> device(std::dynamic_pointer_cast<HMWired::HMWiredDevice>(*i));
				if(!device) continue;
				_devicesMutex.unlock();
				return device;
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
    _devicesMutex.unlock();
	return std::shared_ptr<HMWired::HMWiredDevice>();
}
std::string LogicalDevices::handleCLICommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if(command == "unselect" && _currentDevice && !_currentDevice->peerSelected())
		{
			_currentDevice.reset();
			return "Device unselected.\n";
		}
		else if(command.compare(0, 7, "devices") != 0 && _currentDevice)
		{
			if(!_currentDevice) return "No device selected.\n";
			return _currentDevice->handleCLICommand(command);
		}
		else if(command == "devices help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "devices list\t\tList all devices" << std::endl;
			stringStream << "devices create\t\tCreate a virtual device" << std::endl;
			stringStream << "devices remove\t\tRemove a virtual device" << std::endl;
			stringStream << "devices select\t\tSelect a virtual device" << std::endl;
			return stringStream.str();
		}
		else if(command == "devices list")
		{
			_devicesMutex.lock();
			std::vector<std::shared_ptr<LogicalDevice>> devices;
			for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
			{
				stringStream << "Address: 0x" << std::hex << (*i)->getAddress() << "\tSerial number: " << (*i)->getSerialNumber() << "\tDevice type: " << (uint32_t)(*i)->getDeviceType().type() << "\tDevice id: " << (uint32_t)(*i)->getDeviceType().id() << std::endl << std::dec;
			}
			_devicesMutex.unlock();
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices create") == 0)
		{
			int32_t address;
			int32_t deviceID;
			std::string serialNumber;

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
					if(address == 0) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 4 bytes. A value of \"0\" is not allowed.\n";
				}
				else if(index == 3)
				{
					serialNumber = element;
					if(serialNumber.size() > 10) return "Serial number too long.\n";
				}
				else if(index == 4) deviceID = HelperFunctions::getNumber(element, true);
				index++;
			}
			if(index < 5)
			{
				stringStream << "Description: This command creates a new virtual device." << std::endl;
				stringStream << "Usage: devices create ADDRESS SERIALNUMBER DEVICETYPE" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tAny unused 3 byte address in hexadecimal format. Example: 1A03FC" << std::endl;
				stringStream << "  SERIALNUMBER:\tAny unused serial number with a maximum size of 10 characters. Don't use special characters. Example: VTC9179403" << std::endl;
				stringStream << "  DEVICEID:\tThe id of the device to create. Example: FFFFFFFD" << std::endl << std::endl;
				stringStream << "Currently supported virtual device id's:" << std::endl;
				stringStream << "  FFFFFFFD:\tCentral device" << std::endl;
				stringStream << "  FFFFFFFE:\tSpy device" << std::endl;
				stringStream << "  39:\t\tHM-CC-TC" << std::endl;
				stringStream << "  3A:\t\tHM-CC-VD" << std::endl;
				return stringStream.str();
			}

			switch(deviceID)
			{
			case (uint32_t)DeviceID::HMCCTC:
				add(new HM_CC_TC(0, serialNumber, address));
				stringStream << "Created HM_CC_TC with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			case (uint32_t)DeviceID::HMCCVD:
				add(new HM_CC_VD(0, serialNumber, address));
				stringStream << "Created HM_CC_VD with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			case (uint32_t)DeviceID::HMCENTRAL:
				if(getHomeMaticCentral())
				{
					stringStream << "Cannot create more than one HomeMatic BidCoS central device." << std::endl;
				}
				else
				{
					add(new HomeMaticCentral(0, serialNumber, address));
					stringStream << "Created HMCENTRAL with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				}
				break;
			case (uint32_t)DeviceID::HMSD:
				add(new HM_SD(0, serialNumber, address));
				stringStream << "Created HM_SD with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			default:
				return "Unknown device type.\n";
			}
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices remove") == 0)
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
					if(address == 0) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 4 bytes. A value of \"0\" is not allowed.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command removes a virtual device." << std::endl;
				stringStream << "Usage: devices remove ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe address of the device to delete in hexadecimal format. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			if(_currentDevice && _currentDevice->getAddress() == address) _currentDevice.reset();
			if(get(address))
			{
				if(_homeMaticCentral && address == _homeMaticCentral->getAddress()) _homeMaticCentral.reset();
				remove(address);
				stringStream << "Removing device." << std::endl;
			}
			else stringStream << "Device not found." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices select") == 0)
		{
			int32_t address = 0;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			bool central = false;
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
					if(element == "central") central = true;
					else
					{
						address = HelperFunctions::getNumber(element, true);
						if(address == 0) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 4 bytes. A value of \"0\" is not allowed.\n";
					}
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command selects a virtual device." << std::endl;
				stringStream << "Usage: devices select ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe address of the device to select in hexadecimal format or \"central\" as a shortcut to select the central device. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			_currentDevice = central ? getHomeMaticCentral() : get(address);
			if(!_currentDevice) stringStream << "Device not found." << std::endl;
			else
			{
				stringStream << "Device selected." << std::endl;
				stringStream << "For information about the device's commands type: \"help\"" << std::endl;
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
