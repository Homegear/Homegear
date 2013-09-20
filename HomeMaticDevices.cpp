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

#include "HomeMaticDevices.h"
#include "GD.h"
#include "Devices/HM-CC-VD.h"
#include "Devices/HM-CC-TC.h"
#include "Devices/HM-SD.h"
#include "HelperFunctions.h"

HomeMaticDevices::HomeMaticDevices()
{
}

HomeMaticDevices::~HomeMaticDevices()
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

void HomeMaticDevices::convertDatabase()
{
	try
	{
		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(std::string("table"))));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(std::string("homegearVariables"))));
		DataTable rows = GD::db.executeCommand("SELECT 1 FROM sqlite_master WHERE type=? AND name=?", data);
		if(!rows.empty()) return;
		data.at(1) = std::shared_ptr<DataColumn>(new DataColumn(std::string("peers")));
		rows = GD::db.executeCommand("SELECT 1 FROM sqlite_master WHERE type=? AND name=?", data);
		if(rows.empty()) return;
		//Create a backup
		GD::db.init(GD::settings.databasePath(), GD::settings.databasePath() + ".old");
		//Database is version 0.0.6
		HelperFunctions::printMessage("Converting database (version 0.0.6)...");
		loadDevicesFromDatabase_0_0_6();
		GD::db.executeCommand("DROP TABLE IF EXISTS peers");
		GD::db.executeCommand("DROP TABLE IF EXISTS devices");
		initializeDatabase();
		save(true, false);
		HelperFunctions::printMessage("Database converted. Stopping Homegear.");
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

void HomeMaticDevices::initializeDatabase()
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
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS events (eventID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, type INTEGER NOT NULL, address TEXT, variable TEXT, trigger INTEGER, eventMethod TEXT, eventMethodParameters BLOB, resetAfter INTEGER, initialTime INTEGER, operation INTEGER, factor DOUBLE, limit INTEGER, resetMethod TEXT, resetMethodParameters BLOB, eventTime INTEGER, recurEvery INTEGER, lastValue BLOB, lastRaised INTEGER)");
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

void HomeMaticDevices::loadDevicesFromDatabase_0_0_6()
{
	try
	{
		DataTable rows = GD::db.executeCommand("SELECT * FROM devices");
		bool spyDeviceExists = false;
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			HMDeviceTypes deviceType = HMDeviceTypes::HMUNKNOWN;
			std::string serializedObject;
			uint8_t dutyCycleMessageCounter = 0;
			for(std::map<uint32_t, std::shared_ptr<DataColumn>>::iterator col = row->second.begin(); col != row->second.end(); ++col)
			{
				if(col->second->index == 1)
				{
					deviceType = (HMDeviceTypes)col->second->intValue;
				}
				else if(col->second->index == 2)
				{
					serializedObject = col->second->textValue;
				}
				else if(col->second->index == 3)
				{
					dutyCycleMessageCounter = col->second->intValue;
				}
				else if(col->second->index == 4)
				{
					std::shared_ptr<HomeMaticDevice> device;
					switch(deviceType)
					{
					case HMDeviceTypes::HMCCTC:
						device = std::shared_ptr<HomeMaticDevice>(new HM_CC_TC());
						break;
					case HMDeviceTypes::HMLCSW1FM:
						device = std::shared_ptr<HomeMaticDevice>(new HM_LC_SWX_FM());
						break;
					case HMDeviceTypes::HMCCVD:
						device = std::shared_ptr<HomeMaticDevice>(new HM_CC_VD());
						break;
					case HMDeviceTypes::HMCENTRAL:
						_central = std::shared_ptr<HomeMaticCentral>(new HomeMaticCentral());
						device = _central;
						break;
					case HMDeviceTypes::HMSD:
						spyDeviceExists = true;
						device = std::shared_ptr<HomeMaticDevice>(new HM_SD());
						break;
					default:
						break;
					}
					if(device)
					{
						device->unserialize_0_0_6(serializedObject, dutyCycleMessageCounter, col->second->intValue);
						device->loadPeers_0_0_6();
						_devicesMutex.lock();
						_devices.push_back(device);
						_devicesMutex.unlock();
					}
				}
			}
		}
		if(!_central) createCentral();
		if(!spyDeviceExists) createSpyDevice();
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

void HomeMaticDevices::loadDevicesFromDatabase()
{
	try
	{
		DataTable rows = GD::db.executeCommand("SELECT * FROM devices");
		bool spyDeviceExists = false;
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			uint32_t deviceID = row->second.at(0)->intValue;
			int32_t address = row->second.at(1)->intValue;
			std::string serialNumber = row->second.at(2)->textValue;
			HMDeviceTypes deviceType = (HMDeviceTypes)row->second.at(3)->intValue;

			std::shared_ptr<HomeMaticDevice> device;
			switch(deviceType)
			{
			case HMDeviceTypes::HMCCTC:
				device = std::shared_ptr<HomeMaticDevice>(new HM_CC_TC(deviceID, serialNumber, address));
				break;
			case HMDeviceTypes::HMLCSW1FM:
				device = std::shared_ptr<HomeMaticDevice>(new HM_LC_SWX_FM(deviceID, serialNumber, address));
				break;
			case HMDeviceTypes::HMCCVD:
				device = std::shared_ptr<HomeMaticDevice>(new HM_CC_VD(deviceID, serialNumber, address));
				break;
			case HMDeviceTypes::HMCENTRAL:
				_central = std::shared_ptr<HomeMaticCentral>(new HomeMaticCentral(deviceID, serialNumber, address));
				device = _central;
				break;
			case HMDeviceTypes::HMSD:
				spyDeviceExists = true;
				device = std::shared_ptr<HomeMaticDevice>(new HM_SD(deviceID, serialNumber, address));
				break;
			default:
				break;
			}
			if(device)
			{
				device->load();
				device->loadPeers();
				_devicesMutex.lock();
				_devices.push_back(device);
				_devicesMutex.unlock();
			}
		}
		if(!_central) createCentral();
		if(!spyDeviceExists) createSpyDevice();
		if(_central) _central->addPeersToVirtualDevices();
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

void HomeMaticDevices::load()
{
	try
	{
		initializeDatabase();
		loadDevicesFromDatabase();
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

void HomeMaticDevices::createSpyDevice()
{
	try
	{
		if(!_central) return;

		int32_t seed = 0xfe0000 + HelperFunctions::getRandomNumber(1, 500);

		int32_t address = _central->getUniqueAddress(seed);
		std::string serialNumber(_central->getUniqueSerialNumber("VSD", HelperFunctions::getRandomNumber(1, 9999999)));

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


void HomeMaticDevices::createCentral()
{
	try
	{
		if(_central) return;

		int32_t address = getUniqueAddress(0xfd);
		std::string serialNumber(getUniqueSerialNumber("VCL"));

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

std::shared_ptr<HomeMaticCentral> HomeMaticDevices::getCentral()
{
	try
	{
		_devicesMutex.lock();
		if(!_central)
		{
			_devicesMutex.unlock();
			return std::shared_ptr<HomeMaticCentral>();
		}

		std::shared_ptr<HomeMaticCentral> central(_central);
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

int32_t HomeMaticDevices::getUniqueAddress(uint8_t firstByte)
{
	int32_t prefix = firstByte << 16;
	int32_t seed = HelperFunctions::getRandomNumber(1, 9999);
	uint32_t i = 0;
	while(get(prefix + seed) && i++ < 10000)
	{
		seed += 13;
		if(seed > 9999) seed -= 10000;
	}
	return prefix + seed;
}

std::string HomeMaticDevices::getUniqueSerialNumber(std::string seedPrefix)
{
	if(seedPrefix.size() != 3) throw Exception("seedPrefix must have a size of 3.");
	uint32_t i = 0;
	int32_t seedNumber = HelperFunctions::getRandomNumber(1, 9999999);
	std::ostringstream stringstream;
	stringstream << seedPrefix << std::setw(7) << std::setfill('0') << std::dec << seedNumber;
	std::string temp2 = stringstream.str();
	while(get(temp2) && i++ < 100000)
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

void HomeMaticDevices::dispose()
{
	try
	{
		std::vector<std::shared_ptr<HomeMaticDevice>> devices;
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			HelperFunctions::printDebug("Debug: Disposing device 0x" + HelperFunctions::getHexString((*i)->getAddress()));
			devices.push_back(*i);
		}
		_devicesMutex.unlock();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = devices.begin(); i != devices.end(); ++i)
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

void HomeMaticDevices::save(bool full, bool crash)
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
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			HelperFunctions::printMessage("(Shutdown) => Saving device 0x" + HelperFunctions::getHexString((*i)->getAddress(), 6));
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

void HomeMaticDevices::add(HomeMaticDevice* device)
{
	try
	{
		if(!device) return;
		_devicesMutex.lock();
		device->save(true);
		if(device->getDeviceType() == HMDeviceTypes::HMCENTRAL)
		{
			_central = std::shared_ptr<HomeMaticCentral>((HomeMaticCentral*)device);
			_devices.push_back(_central);
		}
		else
		{
			_devices.push_back(std::shared_ptr<HomeMaticDevice>(device));
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

void HomeMaticDevices::remove(int32_t address)
{
	try
	{
		if(_removeThread.joinable()) _removeThread.join();
		_removeThread = std::thread(&HomeMaticDevices::removeThread, this, address);
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

void HomeMaticDevices::removeThread(int32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getAddress() == address)
			{
				HelperFunctions::printDebug("Removing device pointer from device array...");
				std::shared_ptr<HomeMaticDevice> device = *i;
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

std::shared_ptr<HomeMaticDevice> HomeMaticDevices::get(int32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
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
	return std::shared_ptr<HomeMaticDevice>();
}

std::vector<std::shared_ptr<HomeMaticDevice>> HomeMaticDevices::getDevices()
{
	try
	{
		_devicesMutex.lock();
		std::vector<std::shared_ptr<HomeMaticDevice>> devices;
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
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
	return std::vector<std::shared_ptr<HomeMaticDevice>>();
}

std::shared_ptr<HomeMaticDevice> HomeMaticDevices::get(std::string serialNumber)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
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

std::string HomeMaticDevices::handleCLICommand(std::string& command)
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
			std::vector<std::shared_ptr<HomeMaticDevice>> devices;
			for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
			{
				stringStream << "Address: 0x" << std::hex << (*i)->getAddress() << "\tSerial number: " << (*i)->getSerialNumber() << "\tDevice type: " << (uint32_t)(*i)->getDeviceType() << std::endl << std::dec;
			}
			_devicesMutex.unlock();
			return stringStream.str();
		}
		else if(command.compare(0, 14, "devices create") == 0)
		{
			int32_t address;
			int32_t deviceType;
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
					if(address == 0 || address != (address & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				else if(index == 3)
				{
					serialNumber = element;
					if(serialNumber.size() > 10) return "Serial number too long.\n";
				}
				else if(index == 4) deviceType = HelperFunctions::getNumber(element, true);
				index++;
			}
			if(index < 5)
			{
				stringStream << "Description: This command creates a new virtual device." << std::endl;
				stringStream << "Usage: devices create ADDRESS SERIALNUMBER DEVICETYPE" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tAny unused 3 byte address in hexadecimal format. Example: 1A03FC" << std::endl;
				stringStream << "  SERIALNUMBER:\tAny unused serial number with a maximum size of 10 characters. Don't use special characters. Example: VTC9179403" << std::endl;
				stringStream << "  DEVICETYPE:\tThe type of the device to create. Example: FFFFFFFD" << std::endl << std::endl;
				stringStream << "Currently supported virtual device types:" << std::endl;
				stringStream << "  FFFFFFFD:\tCentral device" << std::endl;
				stringStream << "  FFFFFFFE:\tSpy device" << std::endl;
				stringStream << "  39:\t\tHM-CC-TC" << std::endl;
				stringStream << "  3A:\t\tHM-CC-VD" << std::endl;
				return stringStream.str();
			}

			switch(deviceType)
			{
			case (uint32_t)HMDeviceTypes::HMCCTC:
				add(new HM_CC_TC(0, serialNumber, address));
				stringStream << "Created HM_CC_TC with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			case (uint32_t)HMDeviceTypes::HMCCVD:
				add(new HM_CC_VD(0, serialNumber, address));
				stringStream << "Created HM_CC_VD with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				break;
			case (uint32_t)HMDeviceTypes::HMCENTRAL:
				if(getCentral())
				{
					stringStream << "Cannot create more than one central device." << std::endl;
				}
				else
				{
					add(new HomeMaticCentral(0, serialNumber, address));
					stringStream << "Created HMCENTRAL with address 0x" << std::hex << address << std::dec << " and serial number " << serialNumber << std::endl;
				}
				break;
			case (uint32_t)HMDeviceTypes::HMSD:
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
					if(address == 0 || address != (address & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command removes a virtual device." << std::endl;
				stringStream << "Usage: devices remove ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe 3 byte address of the device to delete in hexadecimal format. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			if(_currentDevice && _currentDevice->getAddress() == address) _currentDevice.reset();
			if(get(address))
			{
				if(_central && address == _central->getAddress()) _central.reset();
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
						if(address == 0 || address != (address & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
					}
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command selects a virtual device." << std::endl;
				stringStream << "Usage: devices select ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe 3 byte address of the device to select in hexadecimal format or \"central\" as a shortcut to select the central device. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			_currentDevice = central ? getCentral() : get(address);
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
