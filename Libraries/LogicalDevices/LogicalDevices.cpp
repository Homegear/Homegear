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

#include "LogicalDevices.h"
#include "../GD/GD.h"

LogicalDevices::LogicalDevices()
{
}

LogicalDevices::~LogicalDevices()
{

}

void LogicalDevices::convertDatabase()
{
	try
	{
		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(std::string("table"))));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(std::string("homegearVariables"))));
		DataTable rows = GD::db.executeCommand("SELECT 1 FROM sqlite_master WHERE type=? AND name=?", data);
		//Cannot proceed, because table homegearVariables does not exist
		if(rows.empty()) return;
		data.clear();
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(0)));
		DataTable result = GD::db.executeCommand("SELECT * FROM homegearVariables WHERE variableIndex=?", data);
		if(result.empty()) return; //Handled in initializeDatabase
		std::string version = result.at(0).at(3)->textValue;
		if(version == "0.3.1") return; //Up to date
		if(version != "0.0.7" && version != "0.3.0")
		{
			Output::printCritical("Unknown database version: " + version);
			exit(1); //Don't know, what to do
		}
		if(version == "0.0.7")
		{
			Output::printMessage("Converting database from version " + version + " to version 0.3.0...");
			GD::db.init(GD::settings.databasePath(), GD::settings.databaseSynchronous(), GD::settings.databaseMemoryJournal(), GD::settings.databasePath() + ".old");

			GD::db.executeCommand("ALTER TABLE events ADD COLUMN enabled INTEGER");
			GD::db.executeCommand("UPDATE events SET enabled=1");

			loadDevicesFromDatabase(true);
			save(true);

			data.clear();
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(result.at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(0)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn("0.3.0")));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			GD::db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			Output::printMessage("Exiting Homegear after database conversion...");
			exit(0);
		}
		if(version == "0.3.0")
		{
			Output::printMessage("Converting database from version " + version + " to version 0.3.1...");
			GD::db.init(GD::settings.databasePath(), GD::settings.databaseSynchronous(), GD::settings.databaseMemoryJournal(), GD::settings.databasePath() + ".old");

			GD::db.executeCommand("ALTER TABLE devices ADD COLUMN deviceFamily INTEGER DEFAULT 0 NOT NULL");
			GD::db.executeCommand("UPDATE devices SET deviceFamily=1 WHERE deviceType=4278190077");
			GD::db.executeCommand("UPDATE devices SET deviceFamily=1 WHERE deviceType=4278190078");

			loadDevicesFromDatabase(true);
			save(true);

			data.clear();
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(result.at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(0)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn("0.3.1")));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			GD::db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			Output::printMessage("Exiting Homegear after database conversion...");
			exit(0);
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
		GD::db.executeCommand("CREATE TABLE IF NOT EXISTS devices (deviceID INTEGER PRIMARY KEY UNIQUE, address INTEGER NOT NULL, serialNumber TEXT NOT NULL, deviceType INTEGER NOT NULL, deviceFamily INTEGER NOT NULL)");
		GD::db.executeCommand("CREATE INDEX IF NOT EXISTS devicesIndex ON devices (deviceID, address, deviceType, deviceFamily)");
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
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn("0.3.1")));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			GD::db.executeCommand("INSERT INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
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

void LogicalDevices::loadDevicesFromDatabase(bool version_0_0_7)
{
	try
	{
		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			i->second->load(version_0_0_7);
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

void LogicalDevices::load()
{
	try
	{
		initializeDatabase();
		loadDevicesFromDatabase(false);
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

void LogicalDevices::dispose()
{
	try
	{
		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			i->second->dispose();
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

void LogicalDevices::save(bool full, bool crash)
{
	try
	{
		if(!crash)
		{
			Output::printMessage("(Shutdown) => Waiting for threads");
			//The disposing is necessary, because there is a small time gap between setting "_lastDutyCycleEvent" and the duty cycle message counter.
			//If saving takes place within this gap, the paired duty cycle devices are out of sync after restart of the program.
			dispose();
		}
		Output::printMessage("(Shutdown) => Saving devices");
		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			i->second->save(full);
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

std::string LogicalDevices::handleCLICommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if(command == "unselect" && _currentFamily && !_currentFamily->deviceSelected())
		{
			_currentFamily.reset();
			return "Device family unselected.\n";
		}
		else if(command.compare(0, 8, "families") != 0 && _currentFamily)
		{
			if(!_currentFamily) return "No device family selected.\n";
			return _currentFamily->handleCLICommand(command);
		}
		else if(command == "families help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "families list\t\tList all available device families" << std::endl;
			stringStream << "families select\t\tSelect a device family" << std::endl;
			return stringStream.str();
		}
		else if(command == "families list")
		{
			for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
			{
				if(i->first == DeviceFamilies::none || !i->second->available()) continue;
				stringStream << "ID: 0x" << std::hex << std::setfill('0') << std::setw(2) << (uint32_t)i->first << "\tName: " << i->second->getName() << std::endl << std::dec;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 15, "families select") == 0)
		{
			DeviceFamilies family = DeviceFamilies::none;

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
					family = (DeviceFamilies)HelperFunctions::getNumber(element, true);
					if(family == DeviceFamilies::none) return "Invalid family id.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command selects a device family." << std::endl;
				stringStream << "Usage: families select FAMILYID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  FAMILYID:\tThe id of the family to select in hexadecimal format. Example: 1" << std::endl;
				stringStream << "Supported families:" << std::endl;
				for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
				{
					if(i->first == DeviceFamilies::none || !i->second->available()) continue;
					stringStream << "  FAMILYID: 0x" << std::hex << std::setfill('0') << std::setw(2) << (uint32_t)i->first << ":\t" << i->second->getName() << std::endl << std::dec;
				}
				return stringStream.str();
			}
			if(GD::deviceFamilies.find(family) == GD::deviceFamilies.end())
			{
				stringStream << "Device family not found." << std::endl;
				return stringStream.str();
			}

			_currentFamily = GD::deviceFamilies.at(family);
			if(!_currentFamily) stringStream << "Device family not found." << std::endl;
			else
			{
				stringStream << "Device family \"" << _currentFamily->getName() << "\" selected." << std::endl;
				stringStream << "For information about the family's commands type: \"help\"" << std::endl;
			}

			return stringStream.str();
		}
		else return "Unknown command.\n";
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
    return "Error executing command. See log file for more details.\n";
}
