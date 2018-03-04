/* Copyright 2013-2017 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#include "DatabaseController.h"
#include "../User/User.h"
#include "../GD/GD.h"

DatabaseController::DatabaseController() : IQueue(GD::bl.get(), 1, 100000)
{
	_disposing = false;
}

DatabaseController::~DatabaseController()
{
	dispose();
}

void DatabaseController::dispose()
{
	if(_disposing) return;
	_disposing = true;
	stopQueue(0);
	_db.dispose();
	_systemVariables.clear();
	_metadata.clear();
}

void DatabaseController::init()
{
	if(!GD::bl)
	{
		GD::out.printCritical("Critical: Could not initialize database controller, because base library is not initialized.");
		return;
	}
	_rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, false));
	_rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get(), false, true));
	startQueue(0, true, 1, 0, SCHED_OTHER);
}

//General
void DatabaseController::open(std::string databasePath, std::string databaseFilename, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWALJournal, std::string backupPath, std::string backupFilename)
{
	_db.init(databasePath, databaseFilename, databaseSynchronous, databaseMemoryJournal, databaseWALJournal, backupPath, backupFilename);
}

void DatabaseController::hotBackup()
{
	_db.hotBackup();
}

void DatabaseController::initializeDatabase()
{
	try
	{
		_db.executeCommand("CREATE TABLE IF NOT EXISTS homegearVariables (variableID INTEGER PRIMARY KEY UNIQUE, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS homegearVariablesIndex ON homegearVariables (variableID, variableIndex)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS familyVariables (variableID INTEGER PRIMARY KEY UNIQUE, familyID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, variableName TEXT, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS familyVariablesIndex ON familyVariables (variableID, familyID, variableIndex, variableName)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS peers (peerID INTEGER PRIMARY KEY UNIQUE, parent INTEGER NOT NULL, address INTEGER NOT NULL, serialNumber TEXT NOT NULL, type INTEGER NOT NULL)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS peersIndex ON peers (peerID, parent, address, serialNumber, type)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS peerVariables (variableID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS peerVariablesIndex ON peerVariables (variableID, peerID, variableIndex)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS serviceMessages (variableID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS serviceMessagesIndex ON peerVariables (variableID, peerID, variableIndex)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS parameters (parameterID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, parameterSetType INTEGER NOT NULL, peerChannel INTEGER NOT NULL, remotePeer INTEGER, remoteChannel INTEGER, parameterName TEXT, value BLOB, room INTEGER, categories TEXT)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS parametersIndex ON parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS metadata (objectID TEXT, dataID TEXT, serializedObject BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS metadataIndex ON metadata (objectID, dataID)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS systemVariables (variableID TEXT PRIMARY KEY UNIQUE NOT NULL, serializedObject BLOB, room INTEGER, categories TEXT)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS systemVariablesIndex ON systemVariables (variableID)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS devices (deviceID INTEGER PRIMARY KEY UNIQUE, address INTEGER NOT NULL, serialNumber TEXT NOT NULL, deviceType INTEGER NOT NULL, deviceFamily INTEGER NOT NULL)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS devicesIndex ON devices (deviceID, address, deviceType, deviceFamily)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS deviceVariables (variableID INTEGER PRIMARY KEY UNIQUE, deviceID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS deviceVariablesIndex ON deviceVariables (variableID, deviceID, variableIndex)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS licenseVariables (variableID INTEGER PRIMARY KEY UNIQUE, moduleID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS licenseVariablesIndex ON licenseVariables (variableID, moduleID, variableIndex)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS users (userID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, password BLOB NOT NULL, salt BLOB NOT NULL, groups BLOB NOT NULL, metadata BLOB NOT NULL)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS usersIndex ON users (userID, name)");
        _db.executeCommand("CREATE TABLE IF NOT EXISTS groups (id INTEGER PRIMARY KEY UNIQUE, translations BLOB NOT NULL, acl BLOB NOT NULL)");
        _db.executeCommand("CREATE INDEX IF NOT EXISTS groupsIndex ON groups (id)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS events (eventID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, type INTEGER NOT NULL, peerID INTEGER, peerChannel INTEGER, variable TEXT, trigger INTEGER, triggerValue BLOB, eventMethod TEXT, eventMethodParameters BLOB, resetAfter INTEGER, initialTime INTEGER, timeOperation INTEGER, timeFactor REAL, timeLimit INTEGER, resetMethod TEXT, resetMethodParameters BLOB, eventTime INTEGER, endTime INTEGER, recurEvery INTEGER, lastValue BLOB, lastRaised INTEGER, lastReset INTEGER, currentTime INTEGER, enabled INTEGER)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS eventsIndex ON events (eventID, name, type, peerID, peerChannel)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS nodeData (node TEXT, key TEXT, value BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS nodeDataIndex ON nodeData (node, key)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS data (component TEXT, key TEXT, value BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS dataIndex ON data (component, key)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS rooms (id INTEGER PRIMARY KEY UNIQUE, translations BLOB, metadata BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS roomsIndex ON rooms (id)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS categories (id INTEGER PRIMARY KEY UNIQUE, translations BLOB, metadata BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS categoriesIndex ON categories (id)");

		//{{{ Create default groups
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT count(*) FROM groups");
		if(result->empty() || result->at(0).at(0)->intValue < 9)
		{
            if(result->at(0).at(0)->intValue < 1)
			{ //Administrators (1)
				BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
				translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("Administrators"));
				translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Administratoren"));
				std::vector<char> translationsBlob;
				_rpcEncoder->encodeResponse(translations, translationsBlob);

				BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
				BaseLib::PVariable methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
				methods->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
				acl->structValue->emplace("methods", methods);
                acl->structValue->emplace("eventServerMethods", methods);
				std::vector<char> aclBlob;
				_rpcEncoder->encodeResponse(acl, aclBlob);

				BaseLib::Database::DataRow data;
				data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
				data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
				data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
				_db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data);
			}

            if(result->at(0).at(0)->intValue < 2)
            { //Script engine (2)
                BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("Script Engine"));
                translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Skriptengine"));
                std::vector<char> translationsBlob;
                _rpcEncoder->encodeResponse(translations, translationsBlob);

                BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                BaseLib::PVariable methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                methods->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
                acl->structValue->emplace("methods", methods);
                acl->structValue->emplace("eventServerMethods", methods);
                std::vector<char> aclBlob;
                _rpcEncoder->encodeResponse(acl, aclBlob);

                BaseLib::Database::DataRow data;
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
                _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data);
            }

            if(result->at(0).at(0)->intValue < 3)
            { //IPC (3)
                BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("IPC"));
                translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("IPC"));
                std::vector<char> translationsBlob;
                _rpcEncoder->encodeResponse(translations, translationsBlob);

                BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                BaseLib::PVariable methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                methods->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
                acl->structValue->emplace("methods", methods);
                acl->structValue->emplace("eventServerMethods", methods);
                std::vector<char> aclBlob;
                _rpcEncoder->encodeResponse(acl, aclBlob);

                BaseLib::Database::DataRow data;
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
                _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data);
            }

            if(result->at(0).at(0)->intValue < 4)
            { //Node-BLUE (4)
                BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("Node-BLUE"));
                translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Node-BLUE"));
                std::vector<char> translationsBlob;
                _rpcEncoder->encodeResponse(translations, translationsBlob);

                BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                BaseLib::PVariable methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                methods->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
                acl->structValue->emplace("methods", methods);
                acl->structValue->emplace("eventServerMethods", methods);
                std::vector<char> aclBlob;
                _rpcEncoder->encodeResponse(acl, aclBlob);

                BaseLib::Database::DataRow data;
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
                _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data);
            }

            if(result->at(0).at(0)->intValue < 5)
            { //Event handler (5)
                BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("Event Handler"));
                translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Ereignisverarbeitung"));
                std::vector<char> translationsBlob;
                _rpcEncoder->encodeResponse(translations, translationsBlob);

                BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                BaseLib::PVariable methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                methods->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
                acl->structValue->emplace("methods", methods);
                acl->structValue->emplace("eventServerMethods", methods);
                std::vector<char> aclBlob;
                _rpcEncoder->encodeResponse(acl, aclBlob);

                BaseLib::Database::DataRow data;
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
                _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data);
            }

            if(result->at(0).at(0)->intValue < 6)
            { //MQTT (6)
                BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("MQTT"));
                translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("MQTT"));
                std::vector<char> translationsBlob;
                _rpcEncoder->encodeResponse(translations, translationsBlob);

                BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                BaseLib::PVariable methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                methods->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
                acl->structValue->emplace("methods", methods);
                acl->structValue->emplace("eventServerMethods", methods);
                std::vector<char> aclBlob;
                _rpcEncoder->encodeResponse(acl, aclBlob);

                BaseLib::Database::DataRow data;
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
                _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data);
            }

            if(result->at(0).at(0)->intValue < 7)
            { //Family Module (7)
                BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("Family Modules"));
                translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Familienmodule"));
                std::vector<char> translationsBlob;
                _rpcEncoder->encodeResponse(translations, translationsBlob);

                BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                BaseLib::PVariable methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                methods->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
                acl->structValue->emplace("methods", methods);
                acl->structValue->emplace("eventServerMethods", methods);
                std::vector<char> aclBlob;
                _rpcEncoder->encodeResponse(acl, aclBlob);

                BaseLib::Database::DataRow data;
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
                _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data);
            }

            { //No User (8)
                BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("No User"));
                translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Kein Benutzer"));
                std::vector<char> translationsBlob;
                _rpcEncoder->encodeResponse(translations, translationsBlob);

                BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                BaseLib::PVariable methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                methods->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
                acl->structValue->emplace("methods", methods);
                acl->structValue->emplace("eventServerMethods", methods);
                std::vector<char> aclBlob;
                _rpcEncoder->encodeResponse(acl, aclBlob);

                BaseLib::Database::DataRow data;
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
                _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data);
            }

            { //Unauthorized (9)
                BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("Unauthorized"));
                translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Unauthorisiert"));
                std::vector<char> translationsBlob;
                _rpcEncoder->encodeResponse(translations, translationsBlob);

                BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                BaseLib::PVariable methods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                methods->structValue->emplace("*", std::make_shared<BaseLib::Variable>(false));
                acl->structValue->emplace("methods", methods);
                acl->structValue->emplace("eventServerMethods", methods);
                std::vector<char> aclBlob;
                _rpcEncoder->encodeResponse(acl, aclBlob);

                BaseLib::Database::DataRow data;
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
                data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
                _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data);
            }
		}
		//}}}

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
		result = _db.executeCommand("SELECT 1 FROM homegearVariables WHERE variableIndex=?", data);
		if(result->empty())
		{
			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.7.1")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			_db.executeCommand("INSERT INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			std::string password;
			password.reserve(12);
			for(int32_t i = 0; i < 12; i++)
			{
				int32_t number = BaseLib::HelperFunctions::getRandomNumber(33, 126);
				if(number == 96) number--; //Exclude "`"
				password.push_back((char)number);
			}

			GD::out.printMessage("Default password for user \"homegear\" set to: " + password);
			std::string content = "Default password for user \"homegear\": " + password + "\nPlease write down the password and delete this file.\n";
			std::string path = GD::bl->settings.dataPath() + "defaultPassword.txt";
			BaseLib::Io::writeFile(path, content);

			std::vector<uint8_t> salt;
			std::vector<uint8_t> passwordHash = User::generateWHIRLPOOL(password, salt);

			std::string userName("homegear");
			std::vector<uint64_t> groups{1};
			createUser(userName, passwordHash, salt, groups);
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

void DatabaseController::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry)
{
	std::shared_ptr<QueueEntry> queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
	if(!queueEntry) return;
	_db.executeWriteCommand(queueEntry->getEntry());
}

bool DatabaseController::convertDatabase()
{
	try
	{
		std::string databasePath = GD::bl->settings.databasePath();
		if(databasePath.empty()) databasePath = GD::bl->settings.dataPath();
		std::string databaseFilename = "db.sql";
		std::string backupPath = GD::bl->settings.databaseBackupPath();
		if(backupPath.empty()) backupPath = databasePath;
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::string("table"))));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::string("homegearVariables"))));
		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT 1 FROM sqlite_master WHERE type=? AND name=?", data);
		//Cannot proceed, because table homegearVariables does not exist
		if(rows->empty()) return false;
		data.clear();
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM homegearVariables WHERE variableIndex=?", data);
		if(result->empty()) return false; //Handled in initializeDatabase
		std::string version = result->at(0).at(3)->textValue;
		if(version == "0.7.1") return false; //Up to date
		if(version != "0.3.1" && version != "0.4.3" && version != "0.5.0" && version != "0.5.1" && version != "0.6.0" && version != "0.6.1" && version != "0.7.0")
		{
			GD::out.printCritical("Critical: Unknown database version: " + version);
			return true; //Don't know, what to do
		}
		/*if(version == "0.0.7")
		{
			GD::out.printMessage("Converting database from version " + version + " to version 0.3.0...");
			db.init(GD::bl->settings.databasePath(), GD::bl->settings.databaseSynchronous(), GD::bl->settings.databaseMemoryJournal(), GD::bl->settings.databasePath() + ".old");

			db.executeCommand("ALTER TABLE events ADD COLUMN enabled INTEGER");
			db.executeCommand("UPDATE events SET enabled=1");

			loadDevicesFromDatabase(true);
			save(true);

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result.at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.3.0")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			GD::out.printMessage("Exiting Homegear after database conversion...");
			exit(0);
		}*/
		/*if(version == "0.3.0")
		{
			GD::out.printMessage("Converting database from version " + version + " to version 0.3.1...");
			db.init(GD::bl->settings.databasePath(), GD::bl->settings.databaseSynchronous(), GD::bl->settings.databaseMemoryJournal(), GD::bl->settings.databasePath() + ".old");

			db.executeCommand("ALTER TABLE devices ADD COLUMN deviceFamily INTEGER DEFAULT 0 NOT NULL");
			db.executeCommand("UPDATE devices SET deviceFamily=1 WHERE deviceType=4278190077");
			db.executeCommand("UPDATE devices SET deviceFamily=1 WHERE deviceType=4278190078");
			db.executeCommand("DROP TABLE events");

			loadDevicesFromDatabase(true);
			save(true);

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result.at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.3.1")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			GD::out.printMessage("Exiting Homegear after database conversion...");
			exit(0);
		}
		else*/
		_db.init(databasePath, databaseFilename, GD::bl->settings.databaseSynchronous(), GD::bl->settings.databaseMemoryJournal(), GD::bl->settings.databaseWALJournal(), backupPath + databaseFilename + ".0.6.0.old");
		if(version == "0.3.1")
		{
			GD::out.printMessage("Converting database from version " + version + " to version 0.4.3...");

			_db.executeCommand("DELETE FROM peerVariables WHERE variableIndex=16");

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.4.3")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			_db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
		}
		if(version == "0.4.3")
		{
			GD::out.printMessage("Converting database from version " + version + " to version 0.5.0...");

			_db.executeCommand("DELETE FROM peerVariables WHERE variableIndex=16");

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.5.0")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			_db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
		}
		if(version == "0.5.0")
		{
			GD::out.printMessage("Converting database from version " + version + " to version 0.5.1...");

			_db.executeCommand("DELETE FROM peerVariables WHERE variableIndex=15");

			_db.executeCommand("CREATE TABLE IF NOT EXISTS serviceMessages (variableID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
			_db.executeCommand("CREATE INDEX IF NOT EXISTS serviceMessagesIndex ON peerVariables (variableID, peerID, variableIndex)");

			_db.executeCommand("UPDATE peerVariables SET variableIndex=1001 WHERE variableIndex=0");
			_db.executeCommand("UPDATE peerVariables SET variableIndex=1002 WHERE variableIndex=3");

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.5.1")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			_db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
		}
		if(version == "0.5.1")
		{
			GD::out.printMessage("Converting database from version " + version + " to version 0.6.0...");

			_db.executeCommand("DELETE FROM devices WHERE deviceType!=4294967293 AND deviceType!=4278190077");
			_db.executeCommand("ALTER TABLE peers ADD COLUMN type INTEGER NOT NULL DEFAULT 0");
			_db.executeCommand("DROP INDEX IF EXISTS peersIndex");
			_db.executeCommand("CREATE INDEX peersIndex ON peers (peerID, parent, address, serialNumber, type)");

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.6.0")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			_db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
		}
		if(version == "0.6.0")
		{
			GD::out.printMessage("Converting database from version " + version + " to version 0.6.1...");

			_db.executeCommand("UPDATE devices SET deviceType=4294967293 WHERE deviceType=4278190077");

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.6.1")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			_db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
		}
        if(version == "0.6.1")
        {
            GD::out.printMessage("Converting database from version " + version + " to version 0.7.0...");

			BaseLib::PVariable groups = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
			groups->arrayValue->push_back(std::make_shared<BaseLib::Variable>(1));
			std::vector<char> groupBlob;
			_rpcEncoder->encodeResponse(groups, groupBlob);

            BaseLib::PVariable metadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
            std::vector<char> metadataBlob;
            _rpcEncoder->encodeResponse(metadata, metadataBlob);

            _db.executeCommand("ALTER TABLE users ADD COLUMN groups BLOB NOT NULL DEFAULT x'" + BaseLib::HelperFunctions::getHexString(groupBlob) + "'");
            _db.executeCommand("ALTER TABLE users ADD COLUMN metadata BLOB NOT NULL DEFAULT x'" + BaseLib::HelperFunctions::getHexString(metadataBlob) + "'");

            data.clear();
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
            //Don't forget to set new version in initializeDatabase!!!
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.7.0")));
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
            _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
        }
		if(version == "0.7.0")
		{
			GD::out.printMessage("Converting database from version " + version + " to version 0.7.1...");

			BaseLib::PVariable groups = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
			groups->arrayValue->push_back(std::make_shared<BaseLib::Variable>(1));
			std::vector<char> groupBlob;
			_rpcEncoder->encodeResponse(groups, groupBlob);

			BaseLib::PVariable metadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
			std::vector<char> metadataBlob;
			_rpcEncoder->encodeResponse(metadata, metadataBlob);

			_db.executeCommand("ALTER TABLE rooms ADD COLUMN metadata BLOB");
			_db.executeCommand("ALTER TABLE categories ADD COLUMN metadata BLOB");
			_db.executeCommand("ALTER TABLE parameters ADD COLUMN room INTEGER");
			_db.executeCommand("ALTER TABLE parameters ADD COLUMN categories TEXT");

			_db.executeCommand("CREATE TABLE IF NOT EXISTS systemVariables2 (variableID TEXT PRIMARY KEY UNIQUE NOT NULL, serializedObject BLOB, room INTEGER, categories TEXT)");
			std::shared_ptr<BaseLib::Database::DataTable> systemVariablesRows = _db.executeCommand("SELECT variableID, serializedObject FROM systemVariables");
			for(BaseLib::Database::DataTable::iterator i = rows->begin(); i != rows->end(); ++i)
			{
				if(i->second.size() < 2) continue;
				data.clear();
				data.push_back(i->second.at(0));
				data.push_back(i->second.at(1));
				_db.executeCommand("INSERT OR REPLACE INTO systemVariables2(variableID, serializedObject) VALUES(?, ?)", data);
			}
			_db.executeCommand("DROP TABLE systemVariables");
			_db.executeCommand("DROP INDEX systemVariablesIndex");
			_db.executeCommand("ALTER TABLE systemVariables2 RENAME TO systemVariables");
			_db.executeCommand("CREATE INDEX IF NOT EXISTS systemVariablesIndex ON systemVariables (variableID)");

			_db.executeCommand("ALTER TABLE systemVariables ADD COLUMN room INTEGER");
			_db.executeCommand("ALTER TABLE systemVariables ADD COLUMN categories TEXT");



			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.7.1")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			_db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
		}
		return false;
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
    return true;
}

void DatabaseController::createSavepointSynchronous(std::string& name)
{
	if(GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Creating savepoint (synchronous) " + name);
	BaseLib::Database::DataRow data;
	_db.executeWriteCommand("SAVEPOINT " + name, data);
}

void DatabaseController::releaseSavepointSynchronous(std::string& name)
{
	if(GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Releasing savepoint (synchronous) " + name);
	BaseLib::Database::DataRow data;
	_db.executeWriteCommand("RELEASE " + name, data);
}

void DatabaseController::createSavepointAsynchronous(std::string& name)
{
	if(GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Creating savepoint (asynchronous) " + name);
	BaseLib::Database::DataRow data;
	std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("SAVEPOINT " + name, data);
	enqueue(0, entry);
}

void DatabaseController::releaseSavepointAsynchronous(std::string& name)
{
	if(GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Releasing savepoint (asynchronous) " + name);
	BaseLib::Database::DataRow data;
	std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("RELEASE " + name, data);
	enqueue(0, entry);
}
//End general

//Homegear variables
bool DatabaseController::getHomegearVariableString(HomegearVariables::Enum id, std::string& value)
{
	BaseLib::Database::DataRow data;
	data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn((uint64_t)id)));
	std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT stringValue FROM homegearVariables WHERE variableIndex=?", data);
	if(result->empty() || result->at(0).empty()) return false;
	value = result->at(0).at(0)->textValue;
	return true;
}

void DatabaseController::setHomegearVariableString(HomegearVariables::Enum id, std::string& value)
{
	BaseLib::Database::DataRow data;
	data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn((uint64_t)id)));
	std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT variableID FROM homegearVariables WHERE variableIndex=?", data);
	data.clear();
	if(result->empty() || result->at(0).empty())
	{
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn((uint64_t)id)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
		//Don't forget to set new version in initializeDatabase!!!
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(value)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
		enqueue(0, entry);
	}
	else
	{
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn((uint64_t)id)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
		//Don't forget to set new version in initializeDatabase!!!
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(value)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
		enqueue(0, entry);
	}
}
//End Homegear variables

//{{{ Data
BaseLib::PVariable DatabaseController::getData(std::string& component, std::string& key)
{
	try
	{
		BaseLib::PVariable value;

		if(!key.empty())
		{
			std::lock_guard<std::mutex> dataGuard(_dataMutex);
			auto componentIterator = _data.find(component);
			if(componentIterator != _data.end())
			{
				auto keyIterator = componentIterator->second.find(key);
				if(keyIterator != componentIterator->second.end())
				{
					value = keyIterator->second;
					return value;
				}
			}
		}

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(component)));
		std::string command;
		if(!key.empty())
		{
			command = "SELECT value FROM data WHERE component=? AND key=?";
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(key)));
		}
		else command = "SELECT key, value FROM data WHERE component=?";

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand(command, data);
		if(rows->empty() || rows->at(0).empty()) return std::make_shared<BaseLib::Variable>();

		if(key.empty())
		{
			value = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
			for(auto& row : *rows)
			{
				value->structValue->emplace(row.second.at(0)->textValue, _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue));
			}
		}
		else
		{
			value = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
			std::lock_guard<std::mutex> dataGuard(_dataMutex);
			_data[component][key] = value;
		}

		return value;
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::setData(std::string& component, std::string& key, BaseLib::PVariable& value)
{
	try
	{
		if(!value) return BaseLib::Variable::createError(-32602, "Could not parse data.");
		if(component.empty()) return BaseLib::Variable::createError(-32602, "component is an empty string.");
		if(key.empty()) return BaseLib::Variable::createError(-32602, "key is an empty string.");
		if(component.size() > 250) return BaseLib::Variable::createError(-32602, "component has more than 250 characters.");
		if(key.size() > 250) return BaseLib::Variable::createError(-32602, "key has more than 250 characters.");
		//Don't check for type here, so base64, string and future data types that use stringValue are handled
		if(value->type != BaseLib::VariableType::tBase64 && value->type != BaseLib::VariableType::tString && value->type != BaseLib::VariableType::tInteger && value->type != BaseLib::VariableType::tInteger64 && value->type != BaseLib::VariableType::tFloat && value->type != BaseLib::VariableType::tBoolean && value->type != BaseLib::VariableType::tStruct && value->type != BaseLib::VariableType::tArray) return BaseLib::Variable::createError(-32602, "Type " + BaseLib::Variable::getTypeString(value->type) + " is currently not supported.");

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT COUNT(*) FROM data");
		if(rows->size() == 0 || rows->at(0).size() == 0)
		{
			return BaseLib::Variable::createError(-32500, "Error counting data in database.");
		}
		if(rows->at(0).at(0)->intValue > 1000000)
		{
			return BaseLib::Variable::createError(-32500, "Reached limit of 1000000 data entries. Please delete data before adding new entries.");
		}

		{
			std::lock_guard<std::mutex> dataGuard(_dataMutex);
			_data[component][key] = value;
		}

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(component)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(key)));
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM data WHERE component=? AND key=?", data);
		enqueue(0, entry);

		std::vector<char> encodedValue;
		_rpcEncoder->encodeResponse(value, encodedValue);
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(encodedValue)));
		entry = std::make_shared<QueueEntry>("INSERT INTO data VALUES(?, ?, ?)", data);
		enqueue(0, entry);

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteData(std::string& component, std::string& key)
{
	try
	{
		{
			std::lock_guard<std::mutex> dataGuard(_dataMutex);
			if(key.empty()) _data.erase(component);
			else
			{
				auto dataIterator = _data.find(component);
				if(dataIterator != _data.end()) dataIterator->second.erase(key);
			}
		}

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(component)));
		std::string command("DELETE FROM data WHERE component=?");
		if(!key.empty())
		{
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(key)));
			command.append(" AND key=?");
		}
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(command, data);
		enqueue(0, entry);

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//{{{ Rooms
BaseLib::PVariable DatabaseController::createRoom(BaseLib::PVariable translations, BaseLib::PVariable metadata)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>());

		std::vector<char> translationsBlob;
		_rpcEncoder->encodeResponse(translations, translationsBlob);
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

		std::vector<char> metadataBlob;
		_rpcEncoder->encodeResponse(metadata, metadataBlob);
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

		uint64_t result = _db.executeWriteCommand("REPLACE INTO rooms VALUES(?, ?, ?)", data);

		return std::make_shared<BaseLib::Variable>(result);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteRoom(uint64_t roomId)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
		if(_db.executeCommand("SELECT id FROM rooms WHERE id=?", data)->empty()) return BaseLib::Variable::createError(-1, "Unknown room.");

		_db.executeWriteCommand("DELETE FROM rooms WHERE id=?", data);

		return std::make_shared<BaseLib::Variable>();
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getRoomMetadata(uint64_t roomId)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
		auto rows = _db.executeCommand("SELECT metadata FROM rooms WHERE id=?", data);

		if(rows->empty()) return BaseLib::Variable::createError(-1, "Unknown room.");

		return _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getRooms(BaseLib::PRpcClientInfo clientInfo, std::string languageCode, bool checkAcls)
{
	try
	{
		BaseLib::PVariable rooms = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations, metadata FROM rooms");
		rooms->arrayValue->reserve(rows->size());
		for(auto row : *rows)
		{
            if(checkAcls && !clientInfo->acls->checkRoomReadAccess(row.second.at(0)->intValue)) continue;
			BaseLib::PVariable room = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
			room->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(row.second.at(0)->intValue));
			BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
			if(languageCode.empty()) room->structValue->emplace("TRANSLATIONS", translations);
			else
			{
				auto translationIterator = translations->structValue->find(languageCode);
				if(translationIterator != translations->structValue->end()) room->structValue->emplace("NAME", translationIterator->second);
				else
				{
					translationIterator = translations->structValue->find("en-US");
					if(translationIterator != translations->structValue->end()) room->structValue->emplace("NAME", translationIterator->second);
					else room->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
				}
			}
			if(!row.second.at(2)->binaryValue->empty())
			{
				BaseLib::PVariable metadata = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);
				room->structValue->emplace("METADATA", metadata);
			}
			else room->structValue->emplace("METADATA", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));

			rooms->arrayValue->push_back(room);
		}

		return rooms;
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool DatabaseController::roomExists(uint64_t roomId)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
		return !_db.executeCommand("SELECT id FROM rooms WHERE id=?", data)->empty();
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

BaseLib::PVariable DatabaseController::setRoomMetadata(uint64_t roomId, BaseLib::PVariable metadata)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
		if(_db.executeCommand("SELECT id FROM rooms WHERE id=?", data)->empty()) return BaseLib::Variable::createError(-1, "Unknown room.");

		std::vector<char> metadataBlob;
		_rpcEncoder->encodeResponse(metadata, metadataBlob);

		data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
		_db.executeCommand("UPDATE rooms SET metadata=? WHERE id=?", data);

		return std::make_shared<BaseLib::Variable>();
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::updateRoom(uint64_t roomId, BaseLib::PVariable translations, BaseLib::PVariable metadata)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
		if(_db.executeCommand("SELECT id FROM rooms WHERE id=?", data)->empty()) return BaseLib::Variable::createError(-1, "Unknown room.");

		std::vector<char> metadataBlob;
		_rpcEncoder->encodeResponse(metadata, metadataBlob);
		data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

		std::vector<char> translationsBlob;
		_rpcEncoder->encodeResponse(translations, translationsBlob);
		data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

		_db.executeCommand("UPDATE rooms SET translations=?, metadata=? WHERE id=?", data);

		return std::make_shared<BaseLib::Variable>();
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//{{{ Categories
BaseLib::PVariable DatabaseController::createCategory(BaseLib::PVariable translations, BaseLib::PVariable metadata)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>());

		std::vector<char> translationsBlob;
		_rpcEncoder->encodeResponse(translations, translationsBlob);
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

		std::vector<char> metadataBlob;
		_rpcEncoder->encodeResponse(metadata, metadataBlob);
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

		uint64_t result = _db.executeWriteCommand("REPLACE INTO categories VALUES(?, ?, ?)", data);

		return std::make_shared<BaseLib::Variable>(result);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteCategory(uint64_t categoryId)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categoryId));
		if(_db.executeCommand("SELECT id FROM categories WHERE id=?", data)->empty()) return BaseLib::Variable::createError(-1, "Unknown category.");

		_db.executeWriteCommand("DELETE FROM categories WHERE id=?", data);

		return std::make_shared<BaseLib::Variable>();
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getCategories(BaseLib::PRpcClientInfo clientInfo, std::string languageCode, bool checkAcls)
{
	try
	{
		BaseLib::PVariable categories = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations FROM categories");
		categories->arrayValue->reserve(rows->size());
		for(auto row : *rows)
		{
            if(checkAcls && !clientInfo->acls->checkCategoryReadAccess(row.second.at(0)->intValue)) continue;
			BaseLib::PVariable category = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
			category->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(row.second.at(0)->intValue));
			BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
			if(languageCode.empty()) category->structValue->emplace("TRANSLATIONS", translations);
			else
			{
				auto translationIterator = translations->structValue->find(languageCode);
				if(translationIterator != translations->structValue->end()) category->structValue->emplace("NAME", translationIterator->second);
				else
				{
					translationIterator = translations->structValue->find("en-US");
					if(translationIterator != translations->structValue->end()) category->structValue->emplace("NAME", translationIterator->second);
					else category->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
				}
			}
			if(!row.second.at(2)->binaryValue->empty())
			{
				BaseLib::PVariable metadata = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);
				category->structValue->emplace("METADATA", metadata);
			}
			else category->structValue->emplace("METADATA", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
			categories->arrayValue->push_back(category);
		}

		return categories;
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getCategoryMetadata(uint64_t categoryId)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categoryId));
		auto rows = _db.executeCommand("SELECT metadata FROM categories WHERE id=?", data);

		if(rows->empty()) return BaseLib::Variable::createError(-1, "Unknown category.");

		return _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool DatabaseController::categoryExists(uint64_t categoryId)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categoryId));
		return !_db.executeCommand("SELECT id FROM categories WHERE id=?", data)->empty();
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

BaseLib::PVariable DatabaseController::setCategoryMetadata(uint64_t categoryId, BaseLib::PVariable metadata)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categoryId));
		if(_db.executeCommand("SELECT id FROM categories WHERE id=?", data)->empty()) return BaseLib::Variable::createError(-1, "Unknown category.");

		std::vector<char> metadataBlob;
		_rpcEncoder->encodeResponse(metadata, metadataBlob);

		data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
		_db.executeCommand("UPDATE categories SET metadata=? WHERE id=?", data);

		return std::make_shared<BaseLib::Variable>();
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::updateCategory(uint64_t categoryId, BaseLib::PVariable translations, BaseLib::PVariable metadata)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categoryId));
		if(_db.executeCommand("SELECT id FROM categories WHERE id=?", data)->empty()) return BaseLib::Variable::createError(-1, "Unknown category.");

		std::vector<char> metadataBlob;
		_rpcEncoder->encodeResponse(metadata, metadataBlob);
		data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

		std::vector<char> translationsBlob;
		_rpcEncoder->encodeResponse(translations, translationsBlob);
		data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

		_db.executeCommand("UPDATE categories SET translations=?, metadata=? WHERE id=?", data);

		return std::make_shared<BaseLib::Variable>();
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//Node data
std::set<std::string> DatabaseController::getAllNodeDataNodes()
{
	try
	{
		std::set<std::string> nodeIds;
		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT node FROM nodeData");

		for(auto& row : *rows)
		{
			nodeIds.emplace(row.second.at(0)->textValue);
		}

		return nodeIds;
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
	return std::set<std::string>();
}

BaseLib::PVariable DatabaseController::getNodeData(std::string& node, std::string& key, bool requestFromTrustedServer)
{
	try
	{
		BaseLib::PVariable value;

		bool obfuscate = false;

		//Only return passwords if request comes from FlowsServer
		std::string lowerCharKey = key;
		BaseLib::HelperFunctions::toLower(lowerCharKey);
		if(!requestFromTrustedServer && lowerCharKey.size() >= 8 && (lowerCharKey.compare(lowerCharKey.size() - 8, 8, "password") == 0 || lowerCharKey.compare(lowerCharKey.size() - 11, 11, "private_key") == 0)) obfuscate = true;

		if(!key.empty())
		{
			std::lock_guard<std::mutex> dataGuard(_nodeDataMutex);
			auto componentIterator = _nodeData.find(node);
			if(componentIterator != _nodeData.end())
			{
				auto keyIterator = componentIterator->second.find(key);
				if(keyIterator != componentIterator->second.end())
				{
					value = keyIterator->second;
					if(obfuscate) value = value->stringValue.empty() ? std::make_shared<BaseLib::Variable>(std::string()) : std::make_shared<BaseLib::Variable>(std::string("*"));
					return value;
				}
			}
		}

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(node)));
		std::string command;
		if(!key.empty())
		{
			command = "SELECT value FROM nodeData WHERE node=? AND key=?";
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(key)));
		}
		else command = "SELECT key, value FROM nodeData WHERE node=?";

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand(command, data);
		if(rows->empty() || rows->at(0).empty()) return std::make_shared<BaseLib::Variable>();

		if(key.empty())
		{
			value = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
			for(auto& row : *rows)
			{
				std::string innerKey = row.second.at(0)->textValue;
				lowerCharKey = innerKey;
				BaseLib::HelperFunctions::toLower(lowerCharKey);
				BaseLib::PVariable innerValue;
				//Only return passwords if request comes from FlowsServer
				innerValue = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
				if(!requestFromTrustedServer && lowerCharKey.size() >= 8 && (lowerCharKey.compare(lowerCharKey.size() - 8, 8, "password") == 0 || lowerCharKey.compare(lowerCharKey.size() - 11, 11, "private_key") == 0))
				{
					innerValue = innerValue->stringValue.empty() ? std::make_shared<BaseLib::Variable>(std::string()) : std::make_shared<BaseLib::Variable>(std::string("*"));
				}
				value->structValue->emplace(innerKey, innerValue);
			}
		}
		else
		{
			value = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
			std::lock_guard<std::mutex> dataGuard(_nodeDataMutex);
			_nodeData[node][key] = value;
		}

		if(obfuscate) value = value->stringValue.empty() ? std::make_shared<BaseLib::Variable>(std::string()) : std::make_shared<BaseLib::Variable>(std::string("*"));
		return value;
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::setNodeData(std::string& node, std::string& key, BaseLib::PVariable& value)
{
	try
	{
		if(!value) return BaseLib::Variable::createError(-32602, "Could not parse data.");
		if(node.empty()) return BaseLib::Variable::createError(-32602, "component is an empty string.");
		if(key.empty()) return BaseLib::Variable::createError(-32602, "key is an empty string.");
		if(node.size() > 250) return BaseLib::Variable::createError(-32602, "node ID has more than 250 characters.");
		if(key.size() > 250) return BaseLib::Variable::createError(-32602, "key has more than 250 characters.");
		//Don't check for type here, so base64, string and future data types that use stringValue are handled
		if(value->type != BaseLib::VariableType::tBase64 && value->type != BaseLib::VariableType::tString && value->type != BaseLib::VariableType::tInteger && value->type != BaseLib::VariableType::tInteger64 && value->type != BaseLib::VariableType::tFloat && value->type != BaseLib::VariableType::tBoolean && value->type != BaseLib::VariableType::tStruct && value->type != BaseLib::VariableType::tArray) return BaseLib::Variable::createError(-32602, "Type " + BaseLib::Variable::getTypeString(value->type) + " is currently not supported.");

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT COUNT(*) FROM nodeData");
		if(rows->size() == 0 || rows->at(0).size() == 0)
		{
			return BaseLib::Variable::createError(-32500, "Error counting data in database.");
		}
		if(rows->at(0).at(0)->intValue > 1000000)
		{
			return BaseLib::Variable::createError(-32500, "Reached limit of 1000000 data entries. Please delete data before adding new entries.");
		}

		{
			std::lock_guard<std::mutex> dataGuard(_nodeDataMutex);
			_nodeData[node][key] = value;
		}

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(node)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(key)));
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM nodeData WHERE node=? AND key=?", data);
		enqueue(0, entry);

		std::vector<char> encodedValue;
		_rpcEncoder->encodeResponse(value, encodedValue);
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(encodedValue)));
		entry = std::make_shared<QueueEntry>("INSERT INTO nodeData VALUES(?, ?, ?)", data);
		enqueue(0, entry);

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteNodeData(std::string& node, std::string& key)
{
	try
	{
		{
			std::lock_guard<std::mutex> dataGuard(_nodeDataMutex);
			if(key.empty()) _nodeData.erase(node);
			else
			{
				auto dataIterator = _nodeData.find(node);
				if(dataIterator != _nodeData.end()) dataIterator->second.erase(key);
			}
		}

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(node)));
		std::string command("DELETE FROM nodeData WHERE node=?");
		if(!key.empty())
		{
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(key)));
			command.append(" AND key=?");
		}
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(command, data);
		enqueue(0, entry);

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//End node data

//Metadata
BaseLib::PVariable DatabaseController::getAllMetadata(uint64_t peerID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::to_string(peerID))));

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT dataID, serializedObject FROM metadata WHERE objectID=?", data);

		BaseLib::PVariable metadataStruct(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		for(BaseLib::Database::DataTable::iterator i = rows->begin(); i != rows->end(); ++i)
		{
			if(i->second.size() < 2) continue;
			BaseLib::PVariable metadata = _rpcDecoder->decodeResponse(*i->second.at(1)->binaryValue);
			metadataStruct->structValue->insert(BaseLib::StructElement(i->second.at(0)->textValue, metadata));
		}

		//getAllMetadata is called repetitively for all central peers. That takes a lot of ressources, so we wait a little after each call.
		std::this_thread::sleep_for(std::chrono::milliseconds(3));

		return metadataStruct;
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getMetadata(uint64_t peerID, std::string& dataID)
{
	try
	{
		if(dataID.size() > 250) return BaseLib::Variable::createError(-32602, "dataID has more than 250 characters.");

		BaseLib::PVariable metadata;
		_metadataMutex.lock();
		std::map<uint64_t, std::map<std::string, BaseLib::PVariable>>::iterator peerIterator = _metadata.find(peerID);
		if(peerIterator != _metadata.end())
		{
			std::map<std::string, BaseLib::PVariable>::iterator dataIterator = peerIterator->second.find(dataID);
			if(dataIterator != peerIterator->second.end())
			{
				 metadata = dataIterator->second;
				_metadataMutex.unlock();
				return metadata;
			}
		}
		_metadataMutex.unlock();

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::to_string(peerID))));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(dataID)));

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT serializedObject FROM metadata WHERE objectID=? AND dataID=?", data);
		if(rows->empty() || rows->at(0).empty()) return std::make_shared<BaseLib::Variable>();

		metadata = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
		_metadataMutex.lock();
		_metadata[peerID][dataID] = metadata;
		_metadataMutex.unlock();
		return metadata;
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::setMetadata(uint64_t peerID, std::string& serialNumber, std::string& dataID, BaseLib::PVariable& metadata)
{
	try
	{
		if(!metadata) return BaseLib::Variable::createError(-32602, "Could not parse data.");
		if(dataID.empty()) return BaseLib::Variable::createError(-32602, "dataID is an empty string.");
		if(dataID.size() > 250) return BaseLib::Variable::createError(-32602, "dataID has more than 250 characters.");
		//Don't check for type here, so base64, string and future data types that use stringValue are handled
		if(metadata->type != BaseLib::VariableType::tBase64 && metadata->type != BaseLib::VariableType::tString && metadata->type != BaseLib::VariableType::tInteger && metadata->type != BaseLib::VariableType::tInteger64 && metadata->type != BaseLib::VariableType::tFloat && metadata->type != BaseLib::VariableType::tBoolean && metadata->type != BaseLib::VariableType::tStruct && metadata->type != BaseLib::VariableType::tArray) return BaseLib::Variable::createError(-32602, "Type " + BaseLib::Variable::getTypeString(metadata->type) + " is currently not supported.");

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT COUNT(*) FROM metadata");
		if(rows->size() == 0 || rows->at(0).size() == 0)
		{
			return BaseLib::Variable::createError(-32500, "Error counting metadata in database.");
		}
		if(rows->at(0).at(0)->intValue > 1000000)
		{
			return BaseLib::Variable::createError(-32500, "Reached limit of 1000000 metadata entries. Please delete metadata before adding new entries.");
		}

		_metadataMutex.lock();
		_metadata[peerID][dataID] = metadata;
		_metadataMutex.unlock();

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::to_string(peerID))));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(dataID)));
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM metadata WHERE objectID=? AND dataID=?", data);
		enqueue(0, entry);

		std::vector<char> value;
		_rpcEncoder->encodeResponse(metadata, value);
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(value)));

		entry = std::make_shared<QueueEntry>("INSERT INTO metadata VALUES(?, ?, ?)", data);
		enqueue(0, entry);

#ifdef EVENTHANDLER
		GD::eventHandler->trigger(peerID, -1, dataID, metadata);
#endif
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{dataID});
		std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>{metadata});
		if(GD::nodeBlueServer) GD::nodeBlueServer->broadcastEvent(peerID, -1, valueKeys, values);
#ifndef NO_SCRIPTENGINE
		GD::scriptEngineServer->broadcastEvent(peerID, -1, valueKeys, values);
#endif
		if(GD::ipcServer) GD::ipcServer->broadcastEvent(peerID, -1, valueKeys, values);
		GD::rpcClient->broadcastEvent(peerID, -1, serialNumber, valueKeys, values);

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteMetadata(uint64_t peerID, std::string& serialNumber, std::string& dataID)
{
	try
	{
		if(dataID.size() > 250) return BaseLib::Variable::createError(-32602, "dataID has more than 250 characters.");

		_metadataMutex.lock();
		if(dataID.empty())
		{
			if(_metadata.find(peerID) != _metadata.end()) _metadata.erase(peerID);
		}
		else
		{
			if(_metadata.find(peerID) != _metadata.end() && _metadata[peerID].find(dataID) != _metadata[peerID].end()) _metadata[peerID].erase(dataID);
		}
		_metadataMutex.unlock();

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::to_string(peerID))));
		std::string command("DELETE FROM metadata WHERE objectID=?");
		if(!dataID.empty())
		{
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(dataID)));
			command.append(" AND dataID=?");
		}
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(command, data);
		enqueue(0, entry);

		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{dataID});
		std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>());
		BaseLib::PVariable value(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		value->structValue->insert(BaseLib::StructElement("TYPE", BaseLib::PVariable(new BaseLib::Variable(1))));
		value->structValue->insert(BaseLib::StructElement("CODE", BaseLib::PVariable(new BaseLib::Variable(1))));
		values->push_back(value);
#ifdef EVENTHANDLER
		GD::eventHandler->trigger(peerID, -1, dataID, value);
#endif
		if(GD::nodeBlueServer) GD::nodeBlueServer->broadcastEvent(peerID, -1, valueKeys, values);
#ifndef NO_SCRIPTENGINE
		GD::scriptEngineServer->broadcastEvent(peerID, -1, valueKeys, values);
#endif
		if(GD::ipcServer) GD::ipcServer->broadcastEvent(peerID, -1, valueKeys, values);
		GD::rpcClient->broadcastEvent(peerID, -1, serialNumber, valueKeys, values);

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//End metadata

//System variables
BaseLib::PVariable DatabaseController::getAllSystemVariables()
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT variableID, serializedObject FROM systemVariables");

		BaseLib::PVariable systemVariableStruct(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		if(rows->empty()) return systemVariableStruct;
		for(BaseLib::Database::DataTable::iterator i = rows->begin(); i != rows->end(); ++i)
		{
			if(i->second.size() < 2) continue;
			BaseLib::PVariable value = _rpcDecoder->decodeResponse(*i->second.at(1)->binaryValue);
			systemVariableStruct->structValue->insert(BaseLib::StructElement(i->second.at(0)->textValue, value));
		}

		return systemVariableStruct;
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getSystemVariable(std::string& variableID)
{
	try
	{
		if(variableID.size() > 250) return BaseLib::Variable::createError(-32602, "variableID has more than 250 characters.");

		BaseLib::PVariable value;
		_systemVariableMutex.lock();
		if(_systemVariables.find(variableID) != _systemVariables.end())
		{
			 value = _systemVariables.at(variableID);
			_systemVariableMutex.unlock();
			return value;
		}
		_systemVariableMutex.unlock();

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(variableID)));

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT serializedObject FROM systemVariables WHERE variableID=?", data);
		if(rows->empty() || rows->at(0).empty()) return std::make_shared<BaseLib::Variable>();

		value = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
		_systemVariableMutex.lock();
		_systemVariables[variableID] = value;
		_systemVariableMutex.unlock();
		return value;
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::setSystemVariable(std::string& variableID, BaseLib::PVariable& value)
{
	try
	{
		if(!value) return BaseLib::Variable::createError(-32602, "Could not parse data.");
		if(variableID.empty()) return BaseLib::Variable::createError(-32602, "variableID is an empty string.");
		if(variableID.size() > 250) return BaseLib::Variable::createError(-32602, "variableID has more than 250 characters.");
		//Don't check for type here, so base64, string and future data types that use stringValue are handled
		if(value->type != BaseLib::VariableType::tBase64 && value->type != BaseLib::VariableType::tString && value->type != BaseLib::VariableType::tInteger && value->type != BaseLib::VariableType::tInteger64 && value->type != BaseLib::VariableType::tFloat && value->type != BaseLib::VariableType::tBoolean && value->type != BaseLib::VariableType::tStruct && value->type != BaseLib::VariableType::tArray) return BaseLib::Variable::createError(-32602, "Type " + BaseLib::Variable::getTypeString(value->type) + " is currently not supported.");

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT COUNT(*) FROM systemVariables");
		if(rows->size() == 0 || rows->at(0).size() == 0)
		{
			return BaseLib::Variable::createError(-32500, "Error counting system variables in database.");
		}
		if(rows->at(0).at(0)->intValue > 1000000)
		{
			return BaseLib::Variable::createError(-32500, "Reached limit of 1000000 system variable entries. Please delete system variables before adding new ones.");
		}

		{
			std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
			_systemVariables[variableID] = value;
		}

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(variableID)));
		std::vector<char> encodedValue;
		_rpcEncoder->encodeResponse(value, encodedValue);
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(encodedValue)));

		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT OR REPLACE INTO systemVariables(variableID, serializedObject) VALUES(?, ?)", data);
		enqueue(0, entry);

#ifdef EVENTHANDLER
		GD::eventHandler->trigger(variableID, value);
#endif
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{variableID});
		std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>{value});
		if(GD::nodeBlueServer) GD::nodeBlueServer->broadcastEvent(0, -1, valueKeys, values);
#ifndef NO_SCRIPTENGINE
		GD::scriptEngineServer->broadcastEvent(0, -1, valueKeys, values);
#endif
		if(GD::ipcServer) GD::ipcServer->broadcastEvent(0, -1, valueKeys, values);
		GD::rpcClient->broadcastEvent(0, -1, "", valueKeys, values);

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteSystemVariable(std::string& variableID)
{
	try
	{
		if(variableID.size() > 250) return BaseLib::Variable::createError(-32602, "variableID has more than 250 characters.");

		{
			std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
			if(_systemVariables.find(variableID) != _systemVariables.end()) _systemVariables.erase(variableID);
		}

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(variableID)));
		std::string command("DELETE FROM systemVariables WHERE variableID=?");
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(command, data);
		enqueue(0, entry);

		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{variableID});
		std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>());
		BaseLib::PVariable value(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		value->structValue->insert(BaseLib::StructElement("TYPE", BaseLib::PVariable(new BaseLib::Variable(0))));
		value->structValue->insert(BaseLib::StructElement("CODE", BaseLib::PVariable(new BaseLib::Variable(1))));
		values->push_back(value);
#ifdef EVENTHANDLER
		GD::eventHandler->trigger(variableID, value);
#endif
		if(GD::nodeBlueServer) GD::nodeBlueServer->broadcastEvent(0, -1, valueKeys, values);
#ifndef NO_SCRIPTENGINE
		GD::scriptEngineServer->broadcastEvent(0, -1, valueKeys, values);
#endif
		if(GD::ipcServer) GD::ipcServer->broadcastEvent(0, -1, valueKeys, values);
		GD::rpcClient->broadcastEvent(0, -1, "", valueKeys, values);

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//End system variables

//Users
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getUsers()
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT userID, name, groups, metadata FROM users");
		return rows;
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
	return std::shared_ptr<BaseLib::Database::DataTable>();
}

bool DatabaseController::createUser(const std::string& name, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt, const std::vector<uint64_t>& groups)
{
	try
	{
        if(groups.empty())
        {
            GD::out.printError("Error: Could not create user. Groups are not specified.");
            return false;
        }

		BaseLib::PVariable groupArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
		groupArray->arrayValue->reserve(groups.size());
		for(auto& groupId : groups)
		{
			groupArray->arrayValue->push_back(std::make_shared<BaseLib::Variable>(groupId));
		}
		std::vector<char> groupBlob;
		_rpcEncoder->encodeResponse(groupArray, groupBlob);

        BaseLib::PVariable metadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        std::vector<char> metadataBlob;
        _rpcEncoder->encodeResponse(metadata, metadataBlob);

		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(name));
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(passwordHash));
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(salt));
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupBlob));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
		_db.executeCommand("INSERT INTO users VALUES(NULL, ?, ?, ?, ?, ?)", data);
		if(userNameExists(name)) return true;
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

bool DatabaseController::updateUser(uint64_t id, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt, const std::vector<uint64_t>& groups)
{
	try
	{
		BaseLib::PVariable groupArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
		groupArray->arrayValue->reserve(groups.size());
		for(auto& groupId : groups)
		{
			groupArray->arrayValue->push_back(std::make_shared<BaseLib::Variable>(groupId));
		}
		std::vector<char> groupBlob;
		_rpcEncoder->encodeResponse(groupArray, groupBlob);

        std::shared_ptr<BaseLib::Database::DataTable> rows;
        if(!passwordHash.empty() && !groups.empty())
        {
            if(salt.empty()) return false;
            BaseLib::Database::DataRow data;
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(passwordHash)));
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(salt)));
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(groupBlob)));
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id)));
            _db.executeCommand("UPDATE users SET password=?, salt=?, groups=? WHERE userID=?", data);

            rows = _db.executeCommand("SELECT userID FROM users WHERE password=? AND salt=? AND userID=?", data);
        }
        else if(!passwordHash.empty())
        {
            if(salt.empty()) return false;
            BaseLib::Database::DataRow data;
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(passwordHash)));
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(salt)));
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id)));
            _db.executeCommand("UPDATE users SET password=?, salt=? WHERE userID=?", data);

            rows = _db.executeCommand("SELECT userID FROM users WHERE password=? AND salt=? AND userID=?", data);
        }
        else if(!groups.empty())
        {
            BaseLib::Database::DataRow data;
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(groupBlob)));
            data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id)));
            _db.executeCommand("UPDATE users SET groups=? WHERE userID=?", data);

            rows = _db.executeCommand("SELECT userID FROM users WHERE userID=?", data);
        }
        else return true; //Nothing to update

		return !rows->empty();
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

bool DatabaseController::deleteUser(uint64_t id)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id)));
		_db.executeCommand("DELETE FROM users WHERE userID=?", data);

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT userID FROM users WHERE userID=?", data);
		return rows->empty();
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

bool DatabaseController::userNameExists(const std::string& name)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(name)));
		return !_db.executeCommand("SELECT userID FROM users WHERE name=?", data)->empty();
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

uint64_t DatabaseController::getUserId(const std::string& name)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(name)));
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT userID FROM users WHERE name=?", data);
		if(result->empty()) return 0;
		return result->at(0).at(0)->intValue;
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
	return 0;
}

std::vector<uint64_t> DatabaseController::getUsersGroups(uint64_t userId)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(userId)));
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT groups FROM users WHERE userID=?", data);
		if(result->empty()) return std::vector<uint64_t>();
		auto decodedData = _rpcDecoder->decodeResponse(*result->at(0).at(0)->binaryValue);
		std::vector<uint64_t> groups;
		groups.reserve(decodedData->arrayValue->size());
		for(auto& group : *decodedData->arrayValue)
		{
			groups.push_back(group->integerValue64);
		}
		return groups;
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
	return std::vector<uint64_t>();
}

BaseLib::PVariable DatabaseController::getUserMetadata(uint64_t userId)
{
    try
    {
        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
        auto rows = _db.executeCommand("SELECT metadata FROM users WHERE userID=?", data);

        if(rows->empty()) return BaseLib::Variable::createError(-1, "Unknown user.");

        return _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getPassword(const std::string& name)
{
	try
	{
		BaseLib::Database::DataRow dataSelect;
		dataSelect.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(name)));
		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT password, salt FROM users WHERE name=?", dataSelect);
		return rows;
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
	return std::shared_ptr<BaseLib::Database::DataTable>();
}

BaseLib::PVariable DatabaseController::setUserMetadata(uint64_t userId, BaseLib::PVariable metadata)
{
    try
    {
        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
        if(_db.executeCommand("SELECT userID FROM users WHERE userID=?", data)->empty()) return BaseLib::Variable::createError(-1, "Unknown user.");

        std::vector<char> metadataBlob;
        _rpcEncoder->encodeResponse(metadata, metadataBlob);

        data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
        _db.executeCommand("UPDATE users SET metadata=? WHERE userID=?", data);

        return std::make_shared<BaseLib::Variable>();
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//End users

//Groups
BaseLib::PVariable DatabaseController::createGroup(BaseLib::PVariable translations, BaseLib::PVariable acl)
{
    try
    {
        std::shared_ptr<BaseLib::Database::DataTable> idResult = _db.executeCommand("SELECT id FROM groups ORDER BY id DESC LIMIT 1");
        if(idResult->empty())
        {
            GD::out.printError("Error: Could not retrieve largest group ID.");
            return BaseLib::Variable::createError(-32500, "Unknown application error.");
        }
        uint64_t largestId = idResult->at(0).at(0)->intValue;

        std::vector<char> translationsBlob;
        _rpcEncoder->encodeResponse(translations, translationsBlob);

        std::vector<char> aclBlob;
        _rpcEncoder->encodeResponse(acl, aclBlob);

        BaseLib::Database::DataRow data;
        if(largestId < 100) data.push_back(std::make_shared<BaseLib::Database::DataColumn>(100));
        else data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
        uint64_t result = _db.executeWriteCommand("REPLACE INTO groups VALUES(?, ?, ?)", data);

        return std::make_shared<BaseLib::Variable>(result);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteGroup(uint64_t groupId)
{
    try
    {
        if(groupId < 100) return BaseLib::Variable::createError(-1, "Can't delete system group.");

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupId));
        if(_db.executeCommand("SELECT id FROM groups WHERE id=?", data)->empty()) return BaseLib::Variable::createError(-1, "Unknown group.");

        _db.executeWriteCommand("DELETE FROM groups WHERE id=?", data);

        return std::make_shared<BaseLib::Variable>();
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getAcl(uint64_t groupId)
{
    try
    {
        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupId));
        std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT acl FROM groups WHERE id=?", data);
        if(rows->empty()) return BaseLib::Variable::createError(-1, "Unknown group ID.");

        auto acls = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
        if(acls->structValue->empty()) return BaseLib::Variable::createError(-1, "Group has no ACLs.");

        return acls;
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getGroup(uint64_t groupId, std::string languageCode)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupId));
		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations, acl FROM groups WHERE id=?", data);

		if(rows->empty()) return BaseLib::Variable::createError(-1, "Unknown group.");

		BaseLib::PVariable group = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

		group->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(rows->at(0).at(0)->intValue));

		BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*rows->at(0).at(1)->binaryValue);
		if(languageCode.empty()) group->structValue->emplace("TRANSLATIONS", translations);
		else
		{
			auto translationIterator = translations->structValue->find(languageCode);
			if(translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
			else
			{
				translationIterator = translations->structValue->find("en-US");
				if(translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
				else group->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
			}
		}

		BaseLib::PVariable acl = _rpcDecoder->decodeResponse(*rows->at(0).at(2)->binaryValue);
		group->structValue->emplace("ACL", acl);

		return group;
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getGroups(std::string languageCode)
{
    try
    {
        BaseLib::PVariable groups = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

        std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations, acl FROM groups");
        groups->arrayValue->reserve(rows->size());
        for(auto row : *rows)
        {
            BaseLib::PVariable group = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

            group->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(row.second.at(0)->intValue));

            BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
            if(languageCode.empty()) group->structValue->emplace("TRANSLATIONS", translations);
            else
            {
                auto translationIterator = translations->structValue->find(languageCode);
                if(translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
                else
                {
                    translationIterator = translations->structValue->find("en-US");
                    if(translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
                    else group->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
                }
            }

            BaseLib::PVariable acl = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);
            group->structValue->emplace("ACL", acl);

            groups->arrayValue->push_back(group);
        }

        return groups;
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool DatabaseController::groupExists(uint64_t groupId)
{
    try
    {
        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupId));
        return !_db.executeCommand("SELECT id FROM groups WHERE id=?", data)->empty();
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

BaseLib::PVariable DatabaseController::updateGroup(uint64_t groupId, BaseLib::PVariable translations, BaseLib::PVariable acl)
{
    try
    {
        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupId));
        if(_db.executeCommand("SELECT id FROM groups WHERE id=?", data)->empty()) return BaseLib::Variable::createError(-1, "Unknown group.");

        std::vector<char> translationsBlob;
        if(!translations->structValue->empty()) _rpcEncoder->encodeResponse(translations, translationsBlob);

        std::vector<char> aclBlob;
        _rpcEncoder->encodeResponse(acl, aclBlob);

        data.push_front(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
        if(!translationsBlob.empty())
		{
			data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
			_db.executeCommand("UPDATE groups SET translations=?, acl=? WHERE id=?", data);
		}
		else _db.executeCommand("UPDATE groups SET acl=? WHERE id=?", data);

        return std::make_shared<BaseLib::Variable>();
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//End groups

//Events
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getEvents()
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM events");
		return result;
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
	return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::saveEventAsynchronous(BaseLib::Database::DataRow& event)
{
	if(event.size() == 24)
	{
		event.push_front(event.at(0));
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT OR REPLACE INTO events (eventID, name, type, peerID, peerChannel, variable, trigger, triggerValue, eventMethod, eventMethodParameters, resetAfter, initialTime, timeOperation, timeFactor, timeLimit, resetMethod, resetMethodParameters, eventTime, endTime, recurEvery, lastValue, lastRaised, lastReset, currentTime, enabled) VALUES((SELECT eventID FROM events WHERE name=?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", event);
		enqueue(0, entry);
	}
	else if(event.size() == 25 && event.at(0)->intValue != 0)
	{
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO events VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", event);
		enqueue(0, entry);
	}
	else GD::out.printError("Error: Either eventID is 0 or the number of columns is invalid.");
}

void DatabaseController::deleteEvent(std::string& name)
{
	try
	{
		BaseLib::Database::DataRow data({std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(name))});
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM events WHERE name=?", data);
		enqueue(0, entry);
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
//End events

// {{{ Family
	void DatabaseController::deleteFamily(int32_t familyId)
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(familyId)));
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM familyVariables WHERE familyID=?", data);
		enqueue(0, entry);
	}

	void DatabaseController::saveFamilyVariableAsynchronous(int32_t familyId, BaseLib::Database::DataRow& data)
	{
		try
		{
			if(data.size() == 2)
			{
				if(data.at(1)->intValue == 0)
				{
					GD::out.printError("Error: Could not save family variable. Variable ID is \"0\".");
					return ;
				}
				switch(data.at(0)->dataType)
				{
					case BaseLib::Database::DataColumn::DataType::INTEGER:
						{
							std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE familyVariables SET integerValue=? WHERE variableID=?", data);
							enqueue(0, entry);
						}
						break;
					case BaseLib::Database::DataColumn::DataType::TEXT:
						{
							std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE familyVariables SET stringValue=? WHERE variableID=?", data);
							enqueue(0, entry);
						}
						break;
					case BaseLib::Database::DataColumn::DataType::BLOB:
						{
							std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE familyVariables SET binaryValue=? WHERE variableID=?", data);
							enqueue(0, entry);
						}
						break;
					case BaseLib::Database::DataColumn::DataType::NODATA:
						GD::out.printError("Error: Tried to store data of type NODATA in family variable table.");
						break;
					case BaseLib::Database::DataColumn::DataType::FLOAT:
						GD::out.printError("Error: Tried to store data of type FLOAT in family variable table.");
						break;
				}
			}
			else
			{
				if(data.size() == 9)
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT OR REPLACE INTO familyVariables (variableID, familyID, variableIndex, variableName, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM familyVariables WHERE familyID=? AND variableIndex=? AND variableName=?), ?, ?, ?, ?, ?, ?)", data);
					enqueue(0, entry);
				}
				else if(data.size() == 7 && data.at(0)->intValue != 0)
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO familyVariables VALUES(?, ?, ?, ?, ?, ?, ?)", data);
					enqueue(0, entry);
				}
				else GD::out.printError("Error: Either variableID is 0 or the number of columns is invalid.");
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

	std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getFamilyVariables(int32_t familyId)
	{
		try
		{
			BaseLib::Database::DataRow data;
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(familyId)));
			std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM familyVariables WHERE familyID=?", data);
			return result;
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
		return std::shared_ptr<BaseLib::Database::DataTable>();
	}

	void DatabaseController::deleteFamilyVariable(BaseLib::Database::DataRow& data)
	{
		try
		{
			if(data.size() == 1)
			{
				if(data.at(0)->intValue == 0)
				{
					GD::out.printError("Error: Could not delete family variable. Variable ID is \"0\".");
					return;
				}
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM familyVariables WHERE variableID=?", data);
				enqueue(0, entry);
			}
			else if(data.size() == 2 && data.at(1)->dataType == BaseLib::Database::DataColumn::DataType::Enum::INTEGER)
			{
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM familyVariables WHERE familyID=? AND variableIndex=?", data);
				enqueue(0, entry);
			}
			else if(data.size() == 2 && data.at(1)->dataType == BaseLib::Database::DataColumn::DataType::Enum::TEXT)
			{
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM familyVariables WHERE familyID=? AND variableName=?", data);
				enqueue(0, entry);
			}
			else if(data.size() == 3)
			{
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM familyVariables WHERE familyID=? AND variableIndex=? AND variableName=?", data);
				enqueue(0, entry);
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
// }}}

//Device
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getDevices(uint32_t family)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(family)));
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM devices WHERE deviceFamily=?", data);
		return result;
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
	return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::deleteDevice(uint64_t id)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id)));
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM devices WHERE deviceID=?", data);
		enqueue(0, entry);
		entry = std::make_shared<QueueEntry>("DELETE FROM deviceVariables WHERE deviceID=?", data);
		enqueue(0, entry);
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

uint64_t DatabaseController::saveDevice(uint64_t id, int32_t address, std::string& serialNumber, uint32_t type, uint32_t family)
{
	try
	{
		BaseLib::Database::DataRow data;
		if(id != 0) data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id)));
		else data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(address)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(serialNumber)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(type)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(family)));
		int32_t result = _db.executeWriteCommand("REPLACE INTO devices VALUES(?, ?, ?, ?, ?)", data);
		return result;
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
	return 0;
}

void DatabaseController::saveDeviceVariableAsynchronous(BaseLib::Database::DataRow& data)
{
	try
	{
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not save device variable. Variable ID is \"0\".");
				return;
			}
			switch(data.at(0)->dataType)
			{
			case BaseLib::Database::DataColumn::DataType::INTEGER:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE deviceVariables SET integerValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::TEXT:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE deviceVariables SET stringValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::BLOB:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE deviceVariables SET binaryValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::NODATA:
				GD::out.printError("Error: Tried to store data of type NODATA in variable table.");
				break;
			case BaseLib::Database::DataColumn::DataType::FLOAT:
				GD::out.printError("Error: Tried to store data of type FLOAT in variable table.");
				break;
			}
		}
		else
		{
			if(data.size() == 5)
			{
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT OR REPLACE INTO deviceVariables (variableID, deviceID, variableIndex, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM deviceVariables WHERE deviceID=" + std::to_string(data.at(0)->intValue) + " AND variableIndex=" + std::to_string(data.at(1)->intValue) + "), ?, ?, ?, ?, ?)", data);
				enqueue(0, entry);
			}
			else if(data.size() == 6 && data.at(0)->intValue != 0)
			{
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
				enqueue(0, entry);
			}
			else GD::out.printError("Error: Either variableID is 0 or the number of columns is invalid.");
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

void DatabaseController::deletePeers(int32_t deviceID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(deviceID)));
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM peers WHERE parent=?", data);
		enqueue(0, entry);
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

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getPeers(uint64_t deviceID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(deviceID)));
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM peers WHERE parent=?", data);
		return result;
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
	return std::shared_ptr<BaseLib::Database::DataTable>();
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getDeviceVariables(uint64_t deviceID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(deviceID)));
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM deviceVariables WHERE deviceID=?", data);
		return result;
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
	return std::shared_ptr<BaseLib::Database::DataTable>();
}
//End device

//Peer
void DatabaseController::deletePeer(uint64_t id)
{
	try
	{
		BaseLib::Database::DataRow data({std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id))});
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM parameters WHERE peerID=?", data);
		enqueue(0, entry);
		entry = std::make_shared<QueueEntry>("DELETE FROM peerVariables WHERE peerID=?", data);
		enqueue(0, entry);
		entry = std::make_shared<QueueEntry>("DELETE FROM peers WHERE peerID=?", data);
		enqueue(0, entry);
		entry = std::make_shared<QueueEntry>("DELETE FROM serviceMessages WHERE peerID=?", data);
		enqueue(0, entry);
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

uint64_t DatabaseController::savePeer(uint64_t id, uint32_t parentID, int32_t address, std::string& serialNumber, uint32_t type)
{
	try
	{
		BaseLib::Database::DataRow data;
		if(id > 0) data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id)));
		else data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(parentID)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(address)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(serialNumber)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(type)));
		uint64_t result = _db.executeWriteCommand("REPLACE INTO peers VALUES(?, ?, ?, ?, ?)", data);
		return result;
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
	return 0;
}

void DatabaseController::savePeerParameterAsynchronous(BaseLib::Database::DataRow& data)
{
	try
	{
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not save peer parameter. Parameter ID is \"0\".");
				return ;
			}
			std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE parameters SET value=? WHERE parameterID=?", data);
			enqueue(0, entry);
		}
		else
		{
			if(data.size() == 7)
			{
				data.push_front(data.at(5));
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT OR REPLACE INTO parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName, value) VALUES((SELECT parameterID FROM parameters WHERE peerID=" + std::to_string(data.at(1)->intValue) + " AND parameterSetType=" + std::to_string(data.at(2)->intValue) + " AND peerChannel=" + std::to_string(data.at(3)->intValue) + " AND remotePeer=" + std::to_string(data.at(4)->intValue) + " AND remoteChannel=" + std::to_string(data.at(5)->intValue) + " AND parameterName=?), ?, ?, ?, ?, ?, ?, ?)", data);
				enqueue(0, entry);
			}
			else if(data.size() == 8 && data.at(0)->intValue != 0)
			{
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName, value) VALUES(?, ?, ?, ?, ?, ?, ?, ?)", data);
				enqueue(0, entry);
			}
			else GD::out.printError("Error: Either parameterID is 0 or the number of columns is invalid.");
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

void DatabaseController::savePeerParameterRoomAsynchronous(BaseLib::Database::DataRow& data)
{
	try
	{
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not save room of peer parameter. Parameter ID is \"0\".");
				return ;
			}
			std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE parameters SET room=? WHERE parameterID=?", data);
			enqueue(0, entry);
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

void DatabaseController::savePeerParameterCategoriesAsynchronous(BaseLib::Database::DataRow& data)
{
	try
	{
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not save categories of peer parameter. Parameter ID is \"0\".");
				return ;
			}
			std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE parameters SET categories=? WHERE parameterID=?", data);
			enqueue(0, entry);
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

void DatabaseController::savePeerVariableAsynchronous(BaseLib::Database::DataRow& data)
{
	try
	{
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not save peer variable. Variable ID is \"0\".");
				return ;
			}
			switch(data.at(0)->dataType)
			{
			case BaseLib::Database::DataColumn::DataType::INTEGER:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE peerVariables SET integerValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::TEXT:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE peerVariables SET stringValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::BLOB:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE peerVariables SET binaryValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::NODATA:
				GD::out.printError("Error: Tried to store data of type NODATA in peer variable table.");
				break;
			case BaseLib::Database::DataColumn::DataType::FLOAT:
				GD::out.printError("Error: Tried to store data of type FLOAT in peer variable table.");
				break;
			}
		}
		else
		{
			if(data.size() == 5)
			{
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT OR REPLACE INTO peerVariables (variableID, peerID, variableIndex, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM peerVariables WHERE peerID=" + std::to_string(data.at(0)->intValue) + " AND variableIndex=" + std::to_string(data.at(1)->intValue) + "), ?, ?, ?, ?, ?)", data);
				enqueue(0, entry);
			}
			else if(data.size() == 6 && data.at(0)->intValue != 0)
			{
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO peerVariables VALUES(?, ?, ?, ?, ?, ?)", data);
				enqueue(0, entry);
			}
			else GD::out.printError("Error: Either variableID is 0 or the number of columns is invalid.");
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

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getPeerParameters(uint64_t peerID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(peerID)));
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM parameters WHERE peerID=?", data);
		return result;
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
	return std::shared_ptr<BaseLib::Database::DataTable>();
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getPeerVariables(uint64_t peerID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(peerID)));
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM peerVariables WHERE peerID=?", data);
		return result;
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
	return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::deletePeerParameter(uint64_t peerID, BaseLib::Database::DataRow& data)
{
	try
	{
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not delete parameter. Parameter ID is \"0\".");
				return;
			}
			std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM parameters WHERE peerID=? AND parameterID=?", data);
			enqueue(0, entry);
		}
		else if(data.size() == 4)
		{
			std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND parameterName=?", data);
			enqueue(0, entry);
		}
		else if(data.size() == 5)
		{
			std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND remotePeer=? AND remoteChannel=?", data);
			enqueue(0, entry);
		}
		else
		{
			std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND parameterName=? AND remotePeer=? AND remoteChannel=?", data);
			enqueue(0, entry);
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

bool DatabaseController::peerExists(uint64_t id)
{
    BaseLib::Database::DataRow data({std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id))});
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT 1 FROM peers WHERE peerID=?", data);
    return !result->empty();
}

bool DatabaseController::setPeerID(uint64_t oldPeerID, uint64_t newPeerID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(newPeerID)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(oldPeerID)));
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE peers SET peerID=? WHERE peerID=?", data);
		enqueue(0, entry);
		entry = std::make_shared<QueueEntry>("UPDATE parameters SET peerID=? WHERE peerID=?", data);
		enqueue(0, entry);
		entry = std::make_shared<QueueEntry>("UPDATE peerVariables SET peerID=? WHERE peerID=?", data);
		enqueue(0, entry);
		entry = std::make_shared<QueueEntry>("UPDATE serviceMessages SET peerID=? WHERE peerID=?", data);
		enqueue(0, entry);
		entry = std::make_shared<QueueEntry>("UPDATE events SET peerID=? WHERE peerID=?", data);
		enqueue(0, entry);
		_metadataMutex.lock();
		_metadata.erase(oldPeerID);
		_metadataMutex.unlock();
		data.clear();
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::to_string(newPeerID))));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::to_string(oldPeerID))));
		entry = std::make_shared<QueueEntry>("UPDATE metadata SET objectID=? WHERE objectID=?", data);
		enqueue(0, entry);
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
//End peer

//Service messages
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getServiceMessages(uint64_t peerID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(peerID)));
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM serviceMessages WHERE peerID=?", data);
		return result;
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
	return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::saveServiceMessageAsynchronous(uint64_t peerID, BaseLib::Database::DataRow& data)
{
	try
	{
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not save service message. Variable ID is \"0\".");
				return;
			}
			switch(data.at(0)->dataType)
			{
			case BaseLib::Database::DataColumn::DataType::INTEGER:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE serviceMessages SET integerValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::TEXT:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE serviceMessages SET stringValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::BLOB:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE serviceMessages SET binaryValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::NODATA:
				GD::out.printError("Error: Tried to store data of type NODATA in service message table.");
				break;
			case BaseLib::Database::DataColumn::DataType::FLOAT:
				GD::out.printError("Error: Tried to store data of type FLOAT in service message table.");
				break;
			}
		}
		else if(data.size() == 4)
		{
			if(data.at(3)->intValue == 0)
			{
				GD::out.printError("Error: Could not save service message. Variable ID is \"0\".");
				return;
			}
			std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE serviceMessages SET integerValue=?, stringValue=?, binaryValue=? WHERE variableID=?", data);
			enqueue(0, entry);
		}
		else if(data.size() == 5)
		{
			std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT OR REPLACE INTO serviceMessages (variableID, peerID, variableIndex, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM serviceMessages WHERE peerID=" + std::to_string(data.at(0)->intValue) + " AND variableIndex=" + std::to_string(data.at(1)->intValue) + "), ?, ?, ?, ?, ?)", data);
			enqueue(0, entry);
		}
		else  if(data.size() == 6 && data.at(0)->intValue != 0)
		{
			std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO serviceMessages VALUES(?, ?, ?, ?, ?, ?)", data);
			enqueue(0, entry);
		}
		else GD::out.printError("Error: Either variableID is 0 or the number of columns is invalid.");
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

void DatabaseController::deleteServiceMessage(uint64_t databaseID)
{
	try
	{
		BaseLib::Database::DataRow data({std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(databaseID))});
		_db.executeCommand("DELETE FROM serviceMessages WHERE variableID=?", data);
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
//End service messages

// {{{ License modules
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getLicenseVariables(int32_t moduleId)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(moduleId)));
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM licenseVariables WHERE moduleID=?", data);
		return result;
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
	return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::saveLicenseVariable(int32_t moduleId, BaseLib::Database::DataRow& data)
{
	try
	{
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not save license module variable. Variable ID is \"0\".");
				return ;
			}
			switch(data.at(0)->dataType)
			{
			case BaseLib::Database::DataColumn::DataType::INTEGER:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE licenseVariables SET integerValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::TEXT:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE licenseVariables SET stringValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::BLOB:
				{
					std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE licenseVariables SET binaryValue=? WHERE variableID=?", data);
					enqueue(0, entry);
				}
				break;
			case BaseLib::Database::DataColumn::DataType::NODATA:
				GD::out.printError("Error: Tried to store data of type NODATA in license module variable table.");
				break;
			case BaseLib::Database::DataColumn::DataType::FLOAT:
				GD::out.printError("Error: Tried to store data of type FLOAT in license module variable table.");
				break;
			}
		}
		else
		{
			if(data.size() == 5)
			{
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT OR REPLACE INTO licenseVariables (variableID, moduleID, variableIndex, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM licenseVariables WHERE moduleID=" + std::to_string(data.at(0)->intValue) + " AND variableIndex=" + std::to_string(data.at(1)->intValue) + "), ?, ?, ?, ?, ?)", data);
				enqueue(0, entry);
			}
			else if(data.size() == 6 && data.at(0)->intValue != 0)
			{
				std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO licenseVariables VALUES(?, ?, ?, ?, ?, ?)", data);
				enqueue(0, entry);
			}
			else GD::out.printError("Error: Either variableID is 0 or the number of columns is invalid.");
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

void DatabaseController::deleteLicenseVariable(int32_t moduleId, uint64_t mapKey)
{
	try
	{
		_db.executeCommand("DELETE FROM licenseVariables WHERE variableIndex=" + std::to_string(mapKey));
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
// }}}
