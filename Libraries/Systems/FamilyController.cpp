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

#include "FamilyController.h"
#include "../GD/GD.h"
#include "../../Modules/Base/BaseLib.h"

ModuleLoader::ModuleLoader(std::string name, std::string path)
{
	try
	{
		_name = name;
		BaseLib::Output::printInfo("Info: Loading family module " + _name);

		void* moduleHandle = dlopen(path.c_str(), RTLD_NOW);
		if(!moduleHandle)
		{
			BaseLib::Output::printCritical("Critical: Could not open module \"" + path + "\": " + std::string(dlerror()));
			return;
		}

		BaseLib::Systems::SystemFactory* (*getFactory)();
		getFactory = (BaseLib::Systems::SystemFactory* (*)())dlsym(moduleHandle, "getFactory");
		if(!getFactory)
		{
			BaseLib::Output::printCritical("Critical: Could not open module \"" + path + "\". Symbol \"getFactory\" not found.");
			exit(4);
		}

		_handle = moduleHandle;
		_factory = std::unique_ptr<BaseLib::Systems::SystemFactory>(getFactory());
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

ModuleLoader::~ModuleLoader()
{
	try
	{
		BaseLib::Output::printInfo("Info: Disposing family module " + _name);
		BaseLib::Output::printDebug("Debug: Deleting factory pointer of module " + _name);
		delete _factory.release();
		BaseLib::Output::printDebug("Debug: Closing dynamic library module " + _name);
		dlclose(_handle);
		_handle = nullptr;
		BaseLib::Output::printDebug("Debug: Dynamic library " + _name + " disposed");
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

std::unique_ptr<BaseLib::Systems::DeviceFamily> ModuleLoader::createModule(BaseLib::Systems::DeviceFamily::IFamilyEventSink* eventHandler)
{
	if(!_factory) return std::unique_ptr<BaseLib::Systems::DeviceFamily>();
	return std::unique_ptr<BaseLib::Systems::DeviceFamily>(_factory->createDeviceFamily(BaseLib::Obj::ins, eventHandler));
}

FamilyController::FamilyController()
{
}

FamilyController::~FamilyController()
{
}

//Device event handling
void FamilyController::onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
{
	try
	{
		GD::rpcClient.broadcastEvent(id, channel, deviceAddress, valueKeys, values);
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

void FamilyController::onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	try
	{
		GD::rpcClient.broadcastUpdateDevice(id, channel, address, (RPC::Client::Hint::Enum)hint);
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

void FamilyController::onRPCNewDevices(std::shared_ptr<BaseLib::RPC::RPCVariable> deviceDescriptions)
{
	try
	{
		GD::rpcClient.broadcastNewDevices(deviceDescriptions);
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

void FamilyController::onRPCDeleteDevices(std::shared_ptr<BaseLib::RPC::RPCVariable> deviceAddresses, std::shared_ptr<BaseLib::RPC::RPCVariable> deviceInfo)
{
	try
	{
		GD::rpcClient.broadcastDeleteDevices(deviceAddresses, deviceInfo);
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

void FamilyController::onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
{
	try
	{
		GD::eventHandler.trigger(peerID, channel, variables, values);
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
//End Device event handling

bool FamilyController::familyAvailable(BaseLib::Systems::DeviceFamilies family)
{
	if(!GD::physicalDevices.get(family)) return false;
	return GD::physicalDevices.get(family)->isOpen();
}

void FamilyController::loadModules()
{
	try
	{
		BaseLib::Output::printDebug("Debug: Loading family modules");

		std::vector<std::string> files = BaseLib::HelperFunctions::getFiles(BaseLib::Obj::ins->settings.modulePath());
		if(files.empty())
		{
			BaseLib::Output::printCritical("Critical: No family modules found in \"" + BaseLib::Obj::ins->settings.modulePath() + "\".");
			exit(3);
		}
		for(std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i)
		{
			if(i->size() < 9) continue; //mod_?*.so
			std::string prefix = i->substr(0, 4);
			std::string extension = i->substr(i->size() - 3, 3);
			if(extension != ".so" || prefix != "mod_") continue;
			std::string path(BaseLib::Obj::ins->settings.modulePath() + *i);

			moduleLoaders.insert(std::pair<std::string, std::unique_ptr<ModuleLoader>>(*i, std::unique_ptr<ModuleLoader>(new ModuleLoader(*i, path))));

			std::unique_ptr<BaseLib::Systems::DeviceFamily> family = moduleLoaders.at(*i)->createModule(this);

			if(family) GD::deviceFamilies[family->getFamily()].swap(family);
		}
		if(GD::deviceFamilies.empty())
		{
			BaseLib::Output::printCritical("Critical: Could not load any family modules from \"" + BaseLib::Obj::ins->settings.modulePath() + "\".");
			exit(3);
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

void FamilyController::convertDatabase()
{
	try
	{
		BaseLib::DataColumnVector data;
		data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(std::string("table"))));
		data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(std::string("homegearVariables"))));
		BaseLib::DataTable rows = BaseLib::Obj::ins->db.executeCommand("SELECT 1 FROM sqlite_master WHERE type=? AND name=?", data);
		//Cannot proceed, because table homegearVariables does not exist
		if(rows.empty()) return;
		data.clear();
		data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(0)));
		BaseLib::DataTable result = BaseLib::Obj::ins->db.executeCommand("SELECT * FROM homegearVariables WHERE variableIndex=?", data);
		if(result.empty()) return; //Handled in initializeDatabase
		std::string version = result.at(0).at(3)->textValue;
		if(version == "0.4.3") return; //Up to date
		if(version != "0.3.0" && version != "0.3.1")
		{
			BaseLib::Output::printCritical("Unknown database version: " + version);
			exit(1); //Don't know, what to do
		}
		/*if(version == "0.0.7")
		{
			BaseLib::Output::printMessage("Converting database from version " + version + " to version 0.3.0...");
			BaseLib::Obj::ins->db.init(BaseLib::Obj::ins->settings.databasePath(), BaseLib::Obj::ins->settings.databaseSynchronous(), BaseLib::Obj::ins->settings.databaseMemoryJournal(), BaseLib::Obj::ins->settings.databasePath() + ".old");

			BaseLib::Obj::ins->db.executeCommand("ALTER TABLE events ADD COLUMN enabled INTEGER");
			BaseLib::Obj::ins->db.executeCommand("UPDATE events SET enabled=1");

			loadDevicesFromDatabase(true);
			save(true);

			data.clear();
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(result.at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(0)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn("0.3.0")));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			BaseLib::Obj::ins->db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			BaseLib::Output::printMessage("Exiting Homegear after database conversion...");
			exit(0);
		}*/
		if(version == "0.3.0")
		{
			BaseLib::Output::printMessage("Converting database from version " + version + " to version 0.3.1...");
			BaseLib::Obj::ins->db.init(BaseLib::Obj::ins->settings.databasePath(), BaseLib::Obj::ins->settings.databaseSynchronous(), BaseLib::Obj::ins->settings.databaseMemoryJournal(), BaseLib::Obj::ins->settings.databasePath() + ".old");

			BaseLib::Obj::ins->db.executeCommand("ALTER TABLE devices ADD COLUMN deviceFamily INTEGER DEFAULT 0 NOT NULL");
			BaseLib::Obj::ins->db.executeCommand("UPDATE devices SET deviceFamily=1 WHERE deviceType=4278190077");
			BaseLib::Obj::ins->db.executeCommand("UPDATE devices SET deviceFamily=1 WHERE deviceType=4278190078");
			BaseLib::Obj::ins->db.executeCommand("DROP TABLE events");

			loadDevicesFromDatabase(true);
			save(true);

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(result.at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn("0.3.1")));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			BaseLib::Obj::ins->db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			BaseLib::Output::printMessage("Exiting Homegear after database conversion...");
			exit(0);
		}
		else if(version == "0.3.1")
		{
			BaseLib::Output::printMessage("Converting database from version " + version + " to version 0.4.3...");
			BaseLib::Obj::ins->db.init(BaseLib::Obj::ins->settings.databasePath(), BaseLib::Obj::ins->settings.databaseSynchronous(), BaseLib::Obj::ins->settings.databaseMemoryJournal(), BaseLib::Obj::ins->settings.databasePath() + ".old");

			BaseLib::Obj::ins->db.executeCommand("DELETE FROM peerVariables WHERE variableIndex=16");

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(result.at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn("0.4.3")));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			BaseLib::Obj::ins->db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			BaseLib::Output::printMessage("Exiting Homegear after database conversion...");
			exit(0);
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

void FamilyController::initializeDatabase()
{
	try
	{
		BaseLib::Obj::ins->db.executeCommand("CREATE TABLE IF NOT EXISTS homegearVariables (variableID INTEGER PRIMARY KEY UNIQUE, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		BaseLib::Obj::ins->db.executeCommand("CREATE INDEX IF NOT EXISTS homegearVariablesIndex ON homegearVariables (variableID, variableIndex)");
		BaseLib::Obj::ins->db.executeCommand("CREATE TABLE IF NOT EXISTS peers (peerID INTEGER PRIMARY KEY UNIQUE, parent INTEGER NOT NULL, address INTEGER NOT NULL, serialNumber TEXT NOT NULL)");
		BaseLib::Obj::ins->db.executeCommand("CREATE INDEX IF NOT EXISTS peersIndex ON peers (peerID, parent, address, serialNumber)");
		BaseLib::Obj::ins->db.executeCommand("CREATE TABLE IF NOT EXISTS peerVariables (variableID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		BaseLib::Obj::ins->db.executeCommand("CREATE INDEX IF NOT EXISTS peerVariablesIndex ON peerVariables (variableID, peerID, variableIndex)");
		BaseLib::Obj::ins->db.executeCommand("CREATE TABLE IF NOT EXISTS parameters (parameterID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, parameterSetType INTEGER NOT NULL, peerChannel INTEGER NOT NULL, remotePeer INTEGER, remoteChannel INTEGER, parameterName TEXT, value BLOB)");
		BaseLib::Obj::ins->db.executeCommand("CREATE INDEX IF NOT EXISTS parametersIndex ON parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName)");
		BaseLib::Obj::ins->db.executeCommand("CREATE TABLE IF NOT EXISTS metadata (objectID TEXT, dataID TEXT, serializedObject BLOB)");
		BaseLib::Obj::ins->db.executeCommand("CREATE INDEX IF NOT EXISTS metadataIndex ON metadata (objectID, dataID)");
		BaseLib::Obj::ins->db.executeCommand("CREATE TABLE IF NOT EXISTS devices (deviceID INTEGER PRIMARY KEY UNIQUE, address INTEGER NOT NULL, serialNumber TEXT NOT NULL, deviceType INTEGER NOT NULL, deviceFamily INTEGER NOT NULL)");
		BaseLib::Obj::ins->db.executeCommand("CREATE INDEX IF NOT EXISTS devicesIndex ON devices (deviceID, address, deviceType, deviceFamily)");
		BaseLib::Obj::ins->db.executeCommand("CREATE TABLE IF NOT EXISTS deviceVariables (variableID INTEGER PRIMARY KEY UNIQUE, deviceID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		BaseLib::Obj::ins->db.executeCommand("CREATE INDEX IF NOT EXISTS deviceVariablesIndex ON deviceVariables (variableID, deviceID, variableIndex)");
		BaseLib::Obj::ins->db.executeCommand("CREATE TABLE IF NOT EXISTS users (userID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, password BLOB NOT NULL, salt BLOB NOT NULL)");
		BaseLib::Obj::ins->db.executeCommand("CREATE INDEX IF NOT EXISTS usersIndex ON users (userID, name)");
		BaseLib::Obj::ins->db.executeCommand("CREATE TABLE IF NOT EXISTS events (eventID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, type INTEGER NOT NULL, peerID INTEGER, peerChannel INTEGER, variable TEXT, trigger INTEGER, triggerValue BLOB, eventMethod TEXT, eventMethodParameters BLOB, resetAfter INTEGER, initialTime INTEGER, timeOperation INTEGER, timeFactor REAL, timeLimit INTEGER, resetMethod TEXT, resetMethodParameters BLOB, eventTime INTEGER, endTime INTEGER, recurEvery INTEGER, lastValue BLOB, lastRaised INTEGER, lastReset INTEGER, currentTime INTEGER, enabled INTEGER)");
		BaseLib::Obj::ins->db.executeCommand("CREATE INDEX IF NOT EXISTS eventsIndex ON events (eventID, name, type, peerID, peerChannel)");

		BaseLib::DataColumnVector data;
		data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(0)));
		BaseLib::DataTable result = BaseLib::Obj::ins->db.executeCommand("SELECT 1 FROM homegearVariables WHERE variableIndex=?", data);
		if(result.empty())
		{
			data.clear();
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn("0.4.3")));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			BaseLib::Obj::ins->db.executeCommand("INSERT INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
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

void FamilyController::loadDevicesFromDatabase(bool version_0_0_7)
{
	try
	{
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			i->second->load(version_0_0_7);
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

void FamilyController::load()
{
	try
	{
		initializeDatabase();
		loadDevicesFromDatabase(false);
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

void FamilyController::dispose()
{
	try
	{
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			if(!i->second)
			{
				BaseLib::Output::printError("Error: Disposing of device family with index " + std::to_string((int32_t)i->first) + " failed, because the pointer was empty.");
				continue;
			}
			i->second->dispose();
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

void FamilyController::save(bool full, bool crash)
{
	try
	{
		if(!crash)
		{
			BaseLib::Output::printMessage("(Shutdown) => Waiting for threads");
			//The disposing is necessary, because there is a small time gap between setting "_lastDutyCycleEvent" and the duty cycle message counter.
			//If saving takes place within this gap, the paired duty cycle devices are out of sync after restart of the program.
			dispose();
		}
		BaseLib::Output::printMessage("(Shutdown) => Saving devices");
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			i->second->save(full);
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

std::string FamilyController::handleCLICommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if(command == "unselect" && _currentFamily && !_currentFamily->deviceSelected())
		{
			_currentFamily = nullptr;
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
			std::string bar(" │ ");
			const int32_t idWidth = 5;
			const int32_t nameWidth = 30;
			std::string nameHeader = "Name";
			nameHeader.resize(nameWidth, ' ');
			stringStream << std::setfill(' ')
				<< std::setw(idWidth) << "ID" << bar
				<< nameHeader
				<< std::endl;
			stringStream << "──────┼───────────────────────────────" << std::endl;
			for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
			{
				if(i->first == BaseLib::Systems::DeviceFamilies::none || !familyAvailable(i->first)) continue;
				std::string name = i->second->getName();
				name.resize(nameWidth, ' ');
				stringStream
						<< std::setw(idWidth) << std::setfill(' ') << (int32_t)i->first << bar
						<< name << std::endl;
			}
			stringStream << "──────┴───────────────────────────────" << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 15, "families select") == 0)
		{
			BaseLib::Systems::DeviceFamilies family = BaseLib::Systems::DeviceFamilies::none;

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
					family = (BaseLib::Systems::DeviceFamilies)BaseLib::HelperFunctions::getNumber(element, false);
					if(family == BaseLib::Systems::DeviceFamilies::none) return "Invalid family id.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command selects a device family." << std::endl;
				stringStream << "Usage: families select FAMILYID" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  FAMILYID:\tThe id of the family to select. Example: 1" << std::endl;
				stringStream << "Supported families:" << std::endl;
				for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
				{
					if(i->first == BaseLib::Systems::DeviceFamilies::none || !familyAvailable(i->first)) continue;
					stringStream << "  FAMILYID: 0x" << std::hex << std::setfill('0') << std::setw(2) << (uint32_t)i->first << ":\t" << i->second->getName() << std::endl << std::dec;
				}
				return stringStream.str();
			}
			if(GD::deviceFamilies.find(family) == GD::deviceFamilies.end())
			{
				stringStream << "Device family not found." << std::endl;
				return stringStream.str();
			}

			_currentFamily = GD::deviceFamilies.at(family).get();
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
    return "Error executing command. See log file for more details.\n";
}
