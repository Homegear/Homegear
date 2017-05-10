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

DatabaseController::DatabaseController()
{
	_stopQueueProcessingThread = false;
}

DatabaseController::~DatabaseController()
{
	dispose();
}

void DatabaseController::dispose()
{
	if(_disposing) return;
	_disposing = true;
	_queueMutex.lock();
	//Make sure, bufferedWrite is finished
	_queueMutex.unlock();
	_stopQueueProcessingThread = true;
	_queueEntryAvailable = true;
	_queueConditionVariable.notify_one();
	GD::bl->threadManager.join(_queueProcessingThread);
	for(int32_t i = 0; i < _queueSize; ++i) _queue[i].reset(); //Just to make sure there are no valid shared pointers anymore
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
	_rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get()));
	_rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get()));

	_stopQueueProcessingThread = false;
	_queueEntryAvailable = false;
	_queueHead = 0;
	_queueTail = 0;
	GD::bl->threadManager.start(_queueProcessingThread, true, &DatabaseController::processQueueEntry, this);
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
		_db.executeCommand("CREATE TABLE IF NOT EXISTS parameters (parameterID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, parameterSetType INTEGER NOT NULL, peerChannel INTEGER NOT NULL, remotePeer INTEGER, remoteChannel INTEGER, parameterName TEXT, value BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS parametersIndex ON parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS metadata (objectID TEXT, dataID TEXT, serializedObject BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS metadataIndex ON metadata (objectID, dataID)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS systemVariables (variableID TEXT, serializedObject BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS systemVariablesIndex ON systemVariables (variableID)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS devices (deviceID INTEGER PRIMARY KEY UNIQUE, address INTEGER NOT NULL, serialNumber TEXT NOT NULL, deviceType INTEGER NOT NULL, deviceFamily INTEGER NOT NULL)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS devicesIndex ON devices (deviceID, address, deviceType, deviceFamily)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS deviceVariables (variableID INTEGER PRIMARY KEY UNIQUE, deviceID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS deviceVariablesIndex ON deviceVariables (variableID, deviceID, variableIndex)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS licenseVariables (variableID INTEGER PRIMARY KEY UNIQUE, moduleID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS licenseVariablesIndex ON licenseVariables (variableID, moduleID, variableIndex)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS users (userID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, password BLOB NOT NULL, salt BLOB NOT NULL)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS usersIndex ON users (userID, name)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS events (eventID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, type INTEGER NOT NULL, peerID INTEGER, peerChannel INTEGER, variable TEXT, trigger INTEGER, triggerValue BLOB, eventMethod TEXT, eventMethodParameters BLOB, resetAfter INTEGER, initialTime INTEGER, timeOperation INTEGER, timeFactor REAL, timeLimit INTEGER, resetMethod TEXT, resetMethodParameters BLOB, eventTime INTEGER, endTime INTEGER, recurEvery INTEGER, lastValue BLOB, lastRaised INTEGER, lastReset INTEGER, currentTime INTEGER, enabled INTEGER)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS eventsIndex ON events (eventID, name, type, peerID, peerChannel)");
		_db.executeCommand("CREATE TABLE IF NOT EXISTS data (component TEXT, key TEXT, value BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS dataIndex ON data (component, key)");

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
		std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT 1 FROM homegearVariables WHERE variableIndex=?", data);
		if(result->empty())
		{
			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.6.1")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			_db.executeCommand("INSERT INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			std::vector<uint8_t> salt;
			std::vector<uint8_t> passwordHash = User::generateWHIRLPOOL("homegear", salt);

			std::string userName("homegear");
			createUser(userName, passwordHash, salt);
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

void DatabaseController::bufferedWrite(std::string command, BaseLib::Database::DataRow& data)
{
	try
	{
		_queueMutex.lock();
		if(_disposing)
		{
			_queueMutex.unlock();
			return;
		}
		int32_t tempHead = _queueHead + 1;
		if(tempHead >= _queueSize) tempHead = 0;
		if(tempHead == _queueTail)
		{
			GD::out.printError("Error: More than " + std::to_string(_queueSize) + " entries are queued to be written into the database. Your data processing is too slow. Not writing entry.");
			_queueMutex.unlock();
			return;
		}

		_queue[_queueHead] = std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>>(new std::pair<std::string, BaseLib::Database::DataRow>(command, data));
		_queueHead++;
		if(_queueHead >= _queueSize)
		{
			_queueHead = 0;
		}
		_queueEntryAvailable = true;
		_queueMutex.unlock();

		_queueConditionVariable.notify_one();
	}
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void DatabaseController::processQueueEntry()
{
	while(!_stopQueueProcessingThread)
	{
		std::unique_lock<std::mutex> lock(_queueProcessingThreadMutex);
		try
		{
			_queueMutex.lock();
			if(_queueHead == _queueTail) //Only lock, when there is really no packet to process. This check is necessary, because the check of the while loop condition is outside of the mutex
			{
				_queueMutex.unlock();
				_queueConditionVariable.wait(lock, [&]{ return _queueEntryAvailable; });
			}
			else _queueMutex.unlock();

			while(_queueHead != _queueTail)
			{
				_queueMutex.lock();
				std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>> entry = _queue[_queueTail];
				_queue[_queueTail].reset();
				_queueTail++;
				if(_queueTail >= _queueSize) _queueTail = 0;
				if(_queueHead == _queueTail) _queueEntryAvailable = false; //Set here, because otherwise it might be set to "true" in raisePacketReceived and then set to false again after the while loop
				_queueMutex.unlock();
				if(entry) _db.executeWriteCommand(entry);
			}
		}
		catch(const std::exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(const BaseLib::Exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		lock.unlock();
	}
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
		if(version == "0.6.1") return false; //Up to date
		if(version != "0.3.1" && version != "0.4.3" && version != "0.5.0" && version != "0.5.1" && version != "0.6.0")
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
	bufferedWrite("SAVEPOINT " + name, data);
}

void DatabaseController::releaseSavepointAsynchronous(std::string& name)
{
	if(GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Releasing savepoint (asynchronous) " + name);
	BaseLib::Database::DataRow data;
	bufferedWrite("RELEASE " + name, data);
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
		bufferedWrite("INSERT INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
	}
	else
	{
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn((uint64_t)id)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
		//Don't forget to set new version in initializeDatabase!!!
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(value)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
		bufferedWrite("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
	}
}
//End Homegear variables

//data
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
		bufferedWrite("DELETE FROM data WHERE component=? AND key=?", data);

		std::vector<char> encodedValue;
		_rpcEncoder->encodeResponse(value, encodedValue);
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(encodedValue)));
		bufferedWrite("INSERT INTO data VALUES(?, ?, ?)", data);

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
		bufferedWrite(command, data);

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
//End data

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
		bufferedWrite("DELETE FROM metadata WHERE objectID=? AND dataID=?", data);

		std::vector<char> value;
		_rpcEncoder->encodeResponse(metadata, value);
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(value)));

		bufferedWrite("INSERT INTO metadata VALUES(?, ?, ?)", data);

#ifdef EVENTHANDLER
		GD::eventHandler->trigger(peerID, -1, dataID, metadata);
#endif
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{dataID});
		std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>{metadata});
#ifndef NO_SCRIPTENGINE
		GD::scriptEngineServer->broadcastEvent(peerID, -1, valueKeys, values);
#endif
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
		bufferedWrite(command, data);

		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{dataID});
		std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>());
		BaseLib::PVariable value(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		value->structValue->insert(BaseLib::StructElement("TYPE", BaseLib::PVariable(new BaseLib::Variable(1))));
		value->structValue->insert(BaseLib::StructElement("CODE", BaseLib::PVariable(new BaseLib::Variable(1))));
		values->push_back(value);
#ifdef EVENTHANDLER
		GD::eventHandler->trigger(peerID, -1, dataID, value);
#endif
#ifndef NO_SCRIPTENGINE
		GD::scriptEngineServer->broadcastEvent(peerID, -1, valueKeys, values);
#endif
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

		_systemVariableMutex.lock();
		_systemVariables[variableID] = value;
		_systemVariableMutex.unlock();

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(variableID)));
		bufferedWrite("DELETE FROM systemVariables WHERE variableID=?", data);

		std::vector<char> encodedValue;
		_rpcEncoder->encodeResponse(value, encodedValue);
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(encodedValue)));

		bufferedWrite("INSERT INTO systemVariables VALUES(?, ?)", data);

#ifdef EVENTHANDLER
		GD::eventHandler->trigger(variableID, value);
#endif
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{variableID});
		std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>{value});
#ifndef NO_SCRIPTENGINE
		GD::scriptEngineServer->broadcastEvent(0, -1, valueKeys, values);
#endif
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

		_systemVariableMutex.lock();
		if(_systemVariables.find(variableID) != _systemVariables.end()) _systemVariables.erase(variableID);
		_systemVariableMutex.unlock();

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(variableID)));
		std::string command("DELETE FROM systemVariables WHERE variableID=?");
		bufferedWrite(command, data);

		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{variableID});
		std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>());
		BaseLib::PVariable value(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		value->structValue->insert(BaseLib::StructElement("TYPE", BaseLib::PVariable(new BaseLib::Variable(0))));
		value->structValue->insert(BaseLib::StructElement("CODE", BaseLib::PVariable(new BaseLib::Variable(1))));
		values->push_back(value);
#ifdef EVENTHANDLER
		GD::eventHandler->trigger(variableID, value);
#endif
#ifndef NO_SCRIPTENGINE
		GD::scriptEngineServer->broadcastEvent(0, -1, valueKeys, values);
#endif
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
		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT userID, name FROM users");
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

bool DatabaseController::createUser(const std::string& name, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(name)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(passwordHash)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(salt)));
		_db.executeCommand("INSERT INTO users VALUES(NULL, ?, ?, ?)", data);
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

bool DatabaseController::updateUser(uint64_t id, const std::vector<uint8_t>& passwordHash, const std::vector<uint8_t>& salt)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(passwordHash)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(salt)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id)));
		_db.executeCommand("UPDATE users SET password=?, salt=? WHERE userID=?", data);

		std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT userID FROM users WHERE password=? AND salt=? AND userID=?", data);
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

uint64_t DatabaseController::getUserID(const std::string& name)
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
//End users

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
		bufferedWrite("INSERT OR REPLACE INTO events (eventID, name, type, peerID, peerChannel, variable, trigger, triggerValue, eventMethod, eventMethodParameters, resetAfter, initialTime, timeOperation, timeFactor, timeLimit, resetMethod, resetMethodParameters, eventTime, endTime, recurEvery, lastValue, lastRaised, lastReset, currentTime, enabled) VALUES((SELECT eventID FROM events WHERE name=?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", event);
	}
	else if(event.size() == 25 && event.at(0)->intValue != 0)
	{
		bufferedWrite("REPLACE INTO events VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", event);
	}
	else GD::out.printError("Error: Either eventID is 0 or the number of columns is invalid.");
}

void DatabaseController::deleteEvent(std::string& name)
{
	try
	{
		BaseLib::Database::DataRow data({std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(name))});
		bufferedWrite("DELETE FROM events WHERE name=?", data);
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
		bufferedWrite("DELETE FROM familyVariables WHERE familyID=?", data);
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
						bufferedWrite("UPDATE familyVariables SET integerValue=? WHERE variableID=?", data);
						break;
					case BaseLib::Database::DataColumn::DataType::TEXT:
						bufferedWrite("UPDATE familyVariables SET stringValue=? WHERE variableID=?", data);
						break;
					case BaseLib::Database::DataColumn::DataType::BLOB:
						bufferedWrite("UPDATE familyVariables SET binaryValue=? WHERE variableID=?", data);
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
					bufferedWrite("INSERT OR REPLACE INTO familyVariables (variableID, familyID, variableIndex, variableName, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM familyVariables WHERE familyID=? AND variableIndex=? AND variableName=?), ?, ?, ?, ?, ?, ?)", data);
				}
				else if(data.size() == 7 && data.at(0)->intValue != 0)
				{
					bufferedWrite("REPLACE INTO familyVariables VALUES(?, ?, ?, ?, ?, ?, ?)", data);
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
				bufferedWrite("DELETE FROM familyVariables WHERE variableID=?", data);
			}
			else if(data.size() == 2 && data.at(1)->dataType == BaseLib::Database::DataColumn::DataType::Enum::INTEGER)
			{
				bufferedWrite("DELETE FROM familyVariables WHERE familyID=? AND variableIndex=?", data);
			}
			else if(data.size() == 2 && data.at(1)->dataType == BaseLib::Database::DataColumn::DataType::Enum::TEXT)
			{
				bufferedWrite("DELETE FROM familyVariables WHERE familyID=? AND variableName=?", data);
			}
			else if(data.size() == 3)
			{
				bufferedWrite("DELETE FROM familyVariables WHERE familyID=? AND variableIndex=? AND variableName=?", data);
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
		bufferedWrite("DELETE FROM devices WHERE deviceID=?", data);
		bufferedWrite("DELETE FROM deviceVariables WHERE deviceID=?", data);
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
				bufferedWrite("UPDATE deviceVariables SET integerValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::TEXT:
				bufferedWrite("UPDATE deviceVariables SET stringValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::BLOB:
				bufferedWrite("UPDATE deviceVariables SET binaryValue=? WHERE variableID=?", data);
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
				bufferedWrite("INSERT OR REPLACE INTO deviceVariables (variableID, deviceID, variableIndex, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM deviceVariables WHERE deviceID=" + std::to_string(data.at(0)->intValue) + " AND variableIndex=" + std::to_string(data.at(1)->intValue) + "), ?, ?, ?, ?, ?)", data);
			}
			else if(data.size() == 6 && data.at(0)->intValue != 0)
			{
				bufferedWrite("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
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
		bufferedWrite("DELETE FROM peers WHERE parent=?", data);
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
		bufferedWrite("DELETE FROM parameters WHERE peerID=?", data);
		bufferedWrite("DELETE FROM peerVariables WHERE peerID=?", data);
		bufferedWrite("DELETE FROM peers WHERE peerID=?", data);
		bufferedWrite("DELETE FROM serviceMessages WHERE peerID=?", data);
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

void DatabaseController::savePeerParameterAsynchronous(uint64_t peerID, BaseLib::Database::DataRow& data)
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
			bufferedWrite("UPDATE parameters SET value=? WHERE parameterID=?", data);
		}
		else
		{
			if(data.size() == 7)
			{
				data.push_front(data.at(5));
				bufferedWrite("INSERT OR REPLACE INTO parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName, value) VALUES((SELECT parameterID FROM parameters WHERE peerID=" + std::to_string(data.at(1)->intValue) + " AND parameterSetType=" + std::to_string(data.at(2)->intValue) + " AND peerChannel=" + std::to_string(data.at(3)->intValue) + " AND remotePeer=" + std::to_string(data.at(4)->intValue) + " AND remoteChannel=" + std::to_string(data.at(5)->intValue) + " AND parameterName=?), ?, ?, ?, ?, ?, ?, ?)", data);
			}
			else if(data.size() == 8 && data.at(0)->intValue != 0)
			{
				bufferedWrite("REPLACE INTO parameters VALUES(?, ?, ?, ?, ?, ?, ?, ?)", data);
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

void DatabaseController::savePeerVariableAsynchronous(uint64_t peerID, BaseLib::Database::DataRow& data)
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
				bufferedWrite("UPDATE peerVariables SET integerValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::TEXT:
				bufferedWrite("UPDATE peerVariables SET stringValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::BLOB:
				bufferedWrite("UPDATE peerVariables SET binaryValue=? WHERE variableID=?", data);
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
				bufferedWrite("INSERT OR REPLACE INTO peerVariables (variableID, peerID, variableIndex, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM peerVariables WHERE peerID=" + std::to_string(data.at(0)->intValue) + " AND variableIndex=" + std::to_string(data.at(1)->intValue) + "), ?, ?, ?, ?, ?)", data);
			}
			else if(data.size() == 6 && data.at(0)->intValue != 0)
			{
				bufferedWrite("REPLACE INTO peerVariables VALUES(?, ?, ?, ?, ?, ?)", data);
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
			bufferedWrite("DELETE FROM parameters WHERE peerID=? AND parameterID=?", data);
		}
		else if(data.size() == 4)
		{
			bufferedWrite("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND parameterName=?", data);
		}
		else if(data.size() == 5)
		{
			bufferedWrite("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND remotePeer=? AND remoteChannel=?", data);
		}
		else
		{
			bufferedWrite("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND parameterName=? AND remotePeer=? AND remoteChannel=?", data);
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

bool DatabaseController::setPeerID(uint64_t oldPeerID, uint64_t newPeerID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(newPeerID)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(oldPeerID)));
		bufferedWrite("UPDATE peers SET peerID=? WHERE peerID=?", data);
		bufferedWrite("UPDATE parameters SET peerID=? WHERE peerID=?", data);
		bufferedWrite("UPDATE peerVariables SET peerID=? WHERE peerID=?", data);
		bufferedWrite("UPDATE serviceMessages SET peerID=? WHERE peerID=?", data);
		bufferedWrite("UPDATE events SET peerID=? WHERE peerID=?", data);
		_metadataMutex.lock();
		_metadata.erase(oldPeerID);
		_metadataMutex.unlock();
		data.clear();
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::to_string(newPeerID))));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::to_string(oldPeerID))));
		bufferedWrite("UPDATE metadata SET objectID=? WHERE objectID=?", data);
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
				bufferedWrite("UPDATE serviceMessages SET integerValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::TEXT:
				bufferedWrite("UPDATE serviceMessages SET stringValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::BLOB:
				bufferedWrite("UPDATE serviceMessages SET binaryValue=? WHERE variableID=?", data);
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
			bufferedWrite("UPDATE serviceMessages SET integerValue=?, stringValue=?, binaryValue=? WHERE variableID=?", data);
		}
		else if(data.size() == 5)
		{
			bufferedWrite("INSERT OR REPLACE INTO serviceMessages (variableID, peerID, variableIndex, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM serviceMessages WHERE peerID=" + std::to_string(data.at(0)->intValue) + " AND variableIndex=" + std::to_string(data.at(1)->intValue) + "), ?, ?, ?, ?, ?)", data);
		}
		else  if(data.size() == 6 && data.at(0)->intValue != 0)
		{
			bufferedWrite("REPLACE INTO serviceMessages VALUES(?, ?, ?, ?, ?, ?)", data);
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
				bufferedWrite("UPDATE licenseVariables SET integerValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::TEXT:
				bufferedWrite("UPDATE licenseVariables SET stringValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::BLOB:
				bufferedWrite("UPDATE licenseVariables SET binaryValue=? WHERE variableID=?", data);
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
				bufferedWrite("INSERT OR REPLACE INTO licenseVariables (variableID, moduleID, variableIndex, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM licenseVariables WHERE moduleID=" + std::to_string(data.at(0)->intValue) + " AND variableIndex=" + std::to_string(data.at(1)->intValue) + "), ?, ?, ?, ?, ?)", data);
			}
			else if(data.size() == 6 && data.at(0)->intValue != 0)
			{
				bufferedWrite("REPLACE INTO licenseVariables VALUES(?, ?, ?, ?, ?, ?)", data);
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
