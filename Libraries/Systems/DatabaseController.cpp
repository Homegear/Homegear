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

#include "DatabaseController.h"
#include "../User/User.h"
#include "../GD/GD.h"

DatabaseController::DatabaseController()
{
}

DatabaseController::~DatabaseController()
{

}

void DatabaseController::dispose()
{
	db.dispose();
}

void DatabaseController::init()
{
	if(!GD::bl)
	{
		GD::out.printCritical("Critical: Could not initialize database controller, because base library is not initialized.");
		return;
	}
	_rpcDecoder = std::unique_ptr<BaseLib::RPC::RPCDecoder>(new BaseLib::RPC::RPCDecoder(GD::bl.get()));
	_rpcEncoder = std::unique_ptr<BaseLib::RPC::RPCEncoder>(new BaseLib::RPC::RPCEncoder(GD::bl.get()));
}

//General
void DatabaseController::open(std::string databasePath, bool databaseSynchronous, bool databaseMemoryJournal, std::string backupPath)
{
	db.init(databasePath, databaseSynchronous, databaseMemoryJournal, backupPath);
}

void DatabaseController::initializeDatabase()
{
	try
	{
		db.executeCommand("CREATE TABLE IF NOT EXISTS homegearVariables (variableID INTEGER PRIMARY KEY UNIQUE, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		db.executeCommand("CREATE INDEX IF NOT EXISTS homegearVariablesIndex ON homegearVariables (variableID, variableIndex)");
		db.executeCommand("CREATE TABLE IF NOT EXISTS peers (peerID INTEGER PRIMARY KEY UNIQUE, parent INTEGER NOT NULL, address INTEGER NOT NULL, serialNumber TEXT NOT NULL)");
		db.executeCommand("CREATE INDEX IF NOT EXISTS peersIndex ON peers (peerID, parent, address, serialNumber)");
		db.executeCommand("CREATE TABLE IF NOT EXISTS peerVariables (variableID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		db.executeCommand("CREATE INDEX IF NOT EXISTS peerVariablesIndex ON peerVariables (variableID, peerID, variableIndex)");
		db.executeCommand("CREATE TABLE IF NOT EXISTS serviceMessages (variableID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		db.executeCommand("CREATE INDEX IF NOT EXISTS serviceMessagesIndex ON peerVariables (variableID, peerID, variableIndex)");
		db.executeCommand("CREATE TABLE IF NOT EXISTS parameters (parameterID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, parameterSetType INTEGER NOT NULL, peerChannel INTEGER NOT NULL, remotePeer INTEGER, remoteChannel INTEGER, parameterName TEXT, value BLOB)");
		db.executeCommand("CREATE INDEX IF NOT EXISTS parametersIndex ON parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName)");
		db.executeCommand("CREATE TABLE IF NOT EXISTS metadata (objectID TEXT, dataID TEXT, serializedObject BLOB)");
		db.executeCommand("CREATE INDEX IF NOT EXISTS metadataIndex ON metadata (objectID, dataID)");
		db.executeCommand("CREATE TABLE IF NOT EXISTS systemVariables (variableID TEXT, serializedObject BLOB)");
		db.executeCommand("CREATE INDEX IF NOT EXISTS systemVariablesIndex ON metadata (variableID)");
		db.executeCommand("CREATE TABLE IF NOT EXISTS devices (deviceID INTEGER PRIMARY KEY UNIQUE, address INTEGER NOT NULL, serialNumber TEXT NOT NULL, deviceType INTEGER NOT NULL, deviceFamily INTEGER NOT NULL)");
		db.executeCommand("CREATE INDEX IF NOT EXISTS devicesIndex ON devices (deviceID, address, deviceType, deviceFamily)");
		db.executeCommand("CREATE TABLE IF NOT EXISTS deviceVariables (variableID INTEGER PRIMARY KEY UNIQUE, deviceID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
		db.executeCommand("CREATE INDEX IF NOT EXISTS deviceVariablesIndex ON deviceVariables (variableID, deviceID, variableIndex)");
		db.executeCommand("CREATE TABLE IF NOT EXISTS users (userID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, password BLOB NOT NULL, salt BLOB NOT NULL)");
		db.executeCommand("CREATE INDEX IF NOT EXISTS usersIndex ON users (userID, name)");
		db.executeCommand("CREATE TABLE IF NOT EXISTS events (eventID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, type INTEGER NOT NULL, peerID INTEGER, peerChannel INTEGER, variable TEXT, trigger INTEGER, triggerValue BLOB, eventMethod TEXT, eventMethodParameters BLOB, resetAfter INTEGER, initialTime INTEGER, timeOperation INTEGER, timeFactor REAL, timeLimit INTEGER, resetMethod TEXT, resetMethodParameters BLOB, eventTime INTEGER, endTime INTEGER, recurEvery INTEGER, lastValue BLOB, lastRaised INTEGER, lastReset INTEGER, currentTime INTEGER, enabled INTEGER)");
		db.executeCommand("CREATE INDEX IF NOT EXISTS eventsIndex ON events (eventID, name, type, peerID, peerChannel)");

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
		std::shared_ptr<BaseLib::Database::DataTable> result = db.executeCommand("SELECT 1 FROM homegearVariables WHERE variableIndex=?", data);
		if(result->empty())
		{
			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.5.1")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			db.executeCommand("INSERT INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			std::vector<uint8_t> salt;
			std::vector<uint8_t> passwordHash = User::generatePBKDF2("homegear", salt);

			createUser("homegear", passwordHash, salt);
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

void DatabaseController::convertDatabase()
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::string("table"))));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(std::string("homegearVariables"))));
		std::shared_ptr<BaseLib::Database::DataTable> rows = db.executeCommand("SELECT 1 FROM sqlite_master WHERE type=? AND name=?", data);
		//Cannot proceed, because table homegearVariables does not exist
		if(rows->empty()) return;
		data.clear();
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
		std::shared_ptr<BaseLib::Database::DataTable> result = db.executeCommand("SELECT * FROM homegearVariables WHERE variableIndex=?", data);
		if(result->empty()) return; //Handled in initializeDatabase
		std::string version = result->at(0).at(3)->textValue;
		if(version == "0.5.1") return; //Up to date
		if(version != "0.3.1" && version != "0.4.3" && version != "0.5.0")
		{
			GD::out.printCritical("Unknown database version: " + version);
			exit(1); //Don't know, what to do
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
		if(version == "0.3.1")
		{
			GD::out.printMessage("Converting database from version " + version + " to version 0.4.3...");
			db.init(GD::bl->settings.databasePath(), GD::bl->settings.databaseSynchronous(), GD::bl->settings.databaseMemoryJournal(), GD::bl->settings.databasePath() + ".old");

			db.executeCommand("DELETE FROM peerVariables WHERE variableIndex=16");

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.4.3")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			GD::out.printMessage("Exiting Homegear after database conversion...");
			exit(0);
		}
		else if(version == "0.4.3")
		{
			GD::out.printMessage("Converting database from version " + version + " to version 0.5.0...");
			db.init(GD::bl->settings.databasePath(), GD::bl->settings.databaseSynchronous(), GD::bl->settings.databaseMemoryJournal(), GD::bl->settings.databasePath() + ".old");

			db.executeCommand("DELETE FROM peerVariables WHERE variableIndex=16");

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.5.0")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			GD::out.printMessage("Exiting Homegear after database conversion...");
			exit(0);
		}
		else if(version == "0.5.0")
		{
			GD::out.printMessage("Converting database from version " + version + " to version 0.5.1...");
			db.init(GD::bl->settings.databasePath(), GD::bl->settings.databaseSynchronous(), GD::bl->settings.databaseMemoryJournal(), GD::bl->settings.databasePath() + ".old");

			db.executeCommand("DELETE FROM peerVariables WHERE variableIndex=15");

			db.executeCommand("CREATE TABLE IF NOT EXISTS serviceMessages (variableID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)");
			db.executeCommand("CREATE INDEX IF NOT EXISTS serviceMessagesIndex ON peerVariables (variableID, peerID, variableIndex)");

			db.executeCommand("UPDATE peerVariables SET variableIndex=1001 WHERE variableIndex=0");
			db.executeCommand("UPDATE peerVariables SET variableIndex=1002 WHERE variableIndex=3");

			data.clear();
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(result->at(0).at(0)->intValue)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(0)));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			//Don't forget to set new version in initializeDatabase!!!
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn("0.5.1")));
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
			db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);

			GD::out.printMessage("Exiting Homegear after database conversion...");
			exit(0);
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

void DatabaseController::createSavepoint(std::string name)
{
	db.executeCommand("SAVEPOINT " + name);
}

void DatabaseController::releaseSavepoint(std::string name)
{
	db.executeCommand("RELEASE " + name);
}
//End general

//Metadata
std::shared_ptr<BaseLib::RPC::RPCVariable> DatabaseController::getAllMetadata(std::string objectID)
{
	try
	{
		if(objectID.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(objectID)));

		std::shared_ptr<BaseLib::Database::DataTable> rows = db.executeCommand("SELECT dataID, serializedObject FROM metadata WHERE objectID=?", data);
		if(rows->empty())
		{
			return BaseLib::RPC::RPCVariable::createError(-1, "No metadata found.");
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> metadataStruct(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		for(BaseLib::Database::DataTable::iterator i = rows->begin(); i != rows->end(); ++i)
		{
			if(i->second.size() < 2) continue;
			std::shared_ptr<BaseLib::RPC::RPCVariable> metadata = _rpcDecoder->decodeResponse(i->second.at(1)->binaryValue);
			metadataStruct->structValue->insert(BaseLib::RPC::RPCStructElement(i->second.at(0)->textValue, metadata));
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
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> DatabaseController::getMetadata(std::string objectID, std::string dataID)
{
	try
	{
		if(objectID.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");
		if(dataID.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "dataID has more than 250 characters.");

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(objectID)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(dataID)));

		std::shared_ptr<BaseLib::Database::DataTable> rows = db.executeCommand("SELECT serializedObject FROM metadata WHERE objectID=? AND dataID=?", data);
		if(rows->empty() || rows->at(0).empty())
		{
			return BaseLib::RPC::RPCVariable::createError(-1, "No metadata found.");
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> metadata = _rpcDecoder->decodeResponse(rows->at(0).at(0)->binaryValue);
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
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> DatabaseController::setMetadata(std::string objectID, std::string dataID, std::shared_ptr<BaseLib::RPC::RPCVariable> metadata)
{
	try
	{
		if(!metadata) return BaseLib::RPC::RPCVariable::createError(-32602, "Could not parse data.");
		if(objectID.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");
		if(dataID.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "dataID has more than 250 characters.");
		//Don't check for type here, so base64, string and future data types that use stringValue are handled
		if(metadata->stringValue.size() > 1000) return BaseLib::RPC::RPCVariable::createError(-32602, "Data has more than 1000 characters.");
		if(metadata->type != BaseLib::RPC::RPCVariableType::rpcBase64 && metadata->type != BaseLib::RPC::RPCVariableType::rpcString && metadata->type != BaseLib::RPC::RPCVariableType::rpcInteger && metadata->type != BaseLib::RPC::RPCVariableType::rpcFloat && metadata->type != BaseLib::RPC::RPCVariableType::rpcBoolean) return BaseLib::RPC::RPCVariable::createError(-32602, "Type " + BaseLib::RPC::RPCVariable::getTypeString(metadata->type) + " is currently not supported.");

		std::shared_ptr<BaseLib::Database::DataTable> rows = db.executeCommand("SELECT COUNT(*) FROM metadata");
		if(rows->size() == 0 || rows->at(0).size() == 0)
		{
			return BaseLib::RPC::RPCVariable::createError(-32500, "Error counting metadata in database.");
		}
		if(rows->at(0).at(0)->intValue > 1000000)
		{
			return BaseLib::RPC::RPCVariable::createError(-32500, "Reached limit of 1000000 metadata entries. Please delete metadata before adding new entries.");
		}

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(objectID)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(dataID)));
		db.executeCommand("DELETE FROM metadata WHERE objectID=? AND dataID=?", data);

		std::shared_ptr<std::vector<char>> value = _rpcEncoder->encodeResponse(metadata);
		if(!value)
		{
			return BaseLib::RPC::RPCVariable::createError(-32700, "Could not encode data.");
		}
		if(value->size() > 1000)
		{
			return BaseLib::RPC::RPCVariable::createError(-32602, "Data is larger than 1000 bytes.");
		}
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(value)));

		db.executeCommand("INSERT INTO metadata VALUES(?, ?, ?)", data);

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
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
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> DatabaseController::deleteMetadata(std::string objectID, std::string dataID)
{
	try
	{
		if(objectID.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");
		if(dataID.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "dataID has more than 250 characters.");

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(objectID)));
		std::string command("DELETE FROM metadata WHERE objectID=?");
		if(!dataID.empty())
		{
			data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(dataID)));
			command.append(" AND dataID=?");
		}
		db.executeCommand(command, data);

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}
//End metadata

//System variables
std::shared_ptr<BaseLib::RPC::RPCVariable> DatabaseController::getAllSystemVariables()
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows = db.executeCommand("SELECT variableID, serializedObject FROM systemVariables");
		if(rows->empty())
		{
			return BaseLib::RPC::RPCVariable::createError(-1, "No system variables found.");
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> systemVariableStruct(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		for(BaseLib::Database::DataTable::iterator i = rows->begin(); i != rows->end(); ++i)
		{
			if(i->second.size() < 2) continue;
			std::shared_ptr<BaseLib::RPC::RPCVariable> value = _rpcDecoder->decodeResponse(i->second.at(1)->binaryValue);
			systemVariableStruct->structValue->insert(BaseLib::RPC::RPCStructElement(i->second.at(0)->textValue, value));
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
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> DatabaseController::getSystemVariable(std::string variableID)
{
	try
	{
		if(variableID.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "variableID has more than 250 characters.");

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(variableID)));

		std::shared_ptr<BaseLib::Database::DataTable> rows = db.executeCommand("SELECT serializedObject FROM systemVariables WHERE variableID=?", data);
		if(rows->empty() || rows->at(0).empty())
		{
			return BaseLib::RPC::RPCVariable::createError(-1, "System variable not found.");
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> value = _rpcDecoder->decodeResponse(rows->at(0).at(0)->binaryValue);
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
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> DatabaseController::setSystemVariable(std::string variableID, std::shared_ptr<BaseLib::RPC::RPCVariable> value)
{
	try
	{
		if(!value) return BaseLib::RPC::RPCVariable::createError(-32602, "Could not parse data.");
		if(variableID.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "variableID has more than 250 characters.");
		//Don't check for type here, so base64, string and future data types that use stringValue are handled
		if(value->stringValue.size() > 1000) return BaseLib::RPC::RPCVariable::createError(-32602, "Data has more than 1000 characters.");
		if(value->type != BaseLib::RPC::RPCVariableType::rpcBase64 && value->type != BaseLib::RPC::RPCVariableType::rpcString && value->type != BaseLib::RPC::RPCVariableType::rpcInteger && value->type != BaseLib::RPC::RPCVariableType::rpcFloat && value->type != BaseLib::RPC::RPCVariableType::rpcBoolean) return BaseLib::RPC::RPCVariable::createError(-32602, "Type " + BaseLib::RPC::RPCVariable::getTypeString(value->type) + " is currently not supported.");

		std::shared_ptr<BaseLib::Database::DataTable> rows = db.executeCommand("SELECT COUNT(*) FROM systemVariables");
		if(rows->size() == 0 || rows->at(0).size() == 0)
		{
			return BaseLib::RPC::RPCVariable::createError(-32500, "Error counting system variables in database.");
		}
		if(rows->at(0).at(0)->intValue > 1000000)
		{
			return BaseLib::RPC::RPCVariable::createError(-32500, "Reached limit of 1000000 system variable entries. Please delete system variables before adding new ones.");
		}

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(variableID)));
		db.executeCommand("DELETE FROM systemVariables WHERE variableID=?", data);

		std::shared_ptr<std::vector<char>> encodedValue = _rpcEncoder->encodeResponse(value);
		if(!encodedValue)
		{
			return BaseLib::RPC::RPCVariable::createError(-32700, "Could not encode data.");
		}
		if(encodedValue->size() > 1000)
		{
			return BaseLib::RPC::RPCVariable::createError(-32602, "Data is larger than 1000 bytes.");
		}
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(encodedValue)));

		db.executeCommand("INSERT INTO systemVariables VALUES(?, ?)", data);

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
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
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> DatabaseController::deleteSystemVariable(std::string variableID)
{
	try
	{
		if(variableID.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "variableID has more than 250 characters.");

		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(variableID)));
		std::string command("DELETE FROM systemVariables WHERE variableID=?");
		db.executeCommand(command, data);

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}
//End system variables

//Users
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getUsers()
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows = db.executeCommand("SELECT userID, name FROM users");
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

bool DatabaseController::createUser(std::string name, std::vector<uint8_t>& passwordHash, std::vector<uint8_t>& salt)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(name)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(passwordHash)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(salt)));
		db.executeCommand("INSERT INTO users VALUES(NULL, ?, ?, ?)", data);
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

bool DatabaseController::updateUser(uint64_t id, std::vector<uint8_t>& passwordHash, std::vector<uint8_t>& salt)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(passwordHash)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(salt)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id)));
		db.executeCommand("UPDATE users SET password=?, salt=? WHERE userID=?", data);

		std::shared_ptr<BaseLib::Database::DataTable> rows = db.executeCommand("SELECT userID FROM users WHERE password=? AND salt=? AND userID=?", data);
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
		db.executeCommand("DELETE FROM users WHERE userID=?", data);

		std::shared_ptr<BaseLib::Database::DataTable> rows = db.executeCommand("SELECT userID FROM users WHERE userID=?", data);
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

bool DatabaseController::userNameExists(std::string name)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(name)));
		return !db.executeCommand("SELECT userID FROM users WHERE name=?", data)->empty();
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

uint64_t DatabaseController::getUserID(std::string name)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(name)));
		std::shared_ptr<BaseLib::Database::DataTable> result = db.executeCommand("SELECT userID FROM users WHERE name=?", data);
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

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getPassword(std::string name)
{
	try
	{
		BaseLib::Database::DataRow dataSelect;
		dataSelect.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(name)));
		std::shared_ptr<BaseLib::Database::DataTable> rows = db.executeCommand("SELECT password, salt FROM users WHERE name=?", dataSelect);
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
		std::shared_ptr<BaseLib::Database::DataTable> result = db.executeCommand("SELECT * FROM events");
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

uint64_t DatabaseController::saveEvent(BaseLib::Database::DataRow event)
{
	try
	{
		createSavepoint("event");
		uint64_t result = db.executeWriteCommand("REPLACE INTO events VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", event);
		releaseSavepoint("event");
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
	releaseSavepoint("event");
	return 0;
}

void DatabaseController::deleteEvent(std::string name)
{
	try
	{
		createSavepoint("eventREMOVE");
		BaseLib::Database::DataRow data({std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(name))});
		db.executeCommand("DELETE FROM events WHERE name=?", data);
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
	releaseSavepoint("eventREMOVE");
}
//End events

//Device
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getDevices(uint32_t family)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(family)));
		std::shared_ptr<BaseLib::Database::DataTable> result = db.executeCommand("SELECT * FROM devices WHERE deviceFamily=?", data);
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
		db.executeCommand("DELETE FROM devices WHERE deviceID=?", data);
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

uint64_t DatabaseController::saveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family)
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
		int32_t result = db.executeWriteCommand("REPLACE INTO devices VALUES(?, ?, ?, ?, ?)", data);
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

uint64_t DatabaseController::saveDeviceVariable(BaseLib::Database::DataRow data)
{
	try
	{
		uint64_t result;
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not save device variable. Variable ID is \"0\".");
				return 0;
			}
			switch(data.at(0)->dataType)
			{
			case BaseLib::Database::DataColumn::DataType::INTEGER:
				result = db.executeWriteCommand("UPDATE deviceVariables SET integerValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::TEXT:
				result = db.executeWriteCommand("UPDATE deviceVariables SET stringValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::BLOB:
				result = db.executeWriteCommand("UPDATE deviceVariables SET binaryValue=? WHERE variableID=?", data);
				break;
			}
		}
		else
		{
			result = db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
		}
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

void DatabaseController::deletePeers(int32_t deviceID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(deviceID)));
		db.executeCommand("DELETE FROM peers WHERE parent=?", data);
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
		std::shared_ptr<BaseLib::Database::DataTable> result = db.executeCommand("SELECT * FROM peers WHERE parent=?", data);
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
		std::shared_ptr<BaseLib::Database::DataTable> result = db.executeCommand("SELECT * FROM deviceVariables WHERE deviceID=?", data);
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
		db.executeCommand("DELETE FROM parameters WHERE peerID=?", data);
		db.executeCommand("DELETE FROM peerVariables WHERE peerID=?", data);
		db.executeCommand("DELETE FROM peers WHERE peerID=?", data);
		db.executeCommand("DELETE FROM serviceMessages WHERE peerID=?", data);
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

uint64_t DatabaseController::savePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber)
{
	try
	{
		BaseLib::Database::DataRow data;
		if(id > 0) data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(id)));
		else data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn()));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(parentID)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(address)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(serialNumber)));
		uint64_t result = db.executeWriteCommand("REPLACE INTO peers VALUES(?, ?, ?, ?)", data);
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

uint64_t DatabaseController::savePeerParameter(uint64_t peerID, BaseLib::Database::DataRow data)
{
	try
	{
		createSavepoint("peerParameter" + std::to_string(peerID));
		int32_t result;
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not save peer parameter. Parameter ID is \"0\".");
				releaseSavepoint("peerParameter" + std::to_string(peerID));
				return 0;
			}
			result = db.executeWriteCommand("UPDATE parameters SET value=? WHERE parameterID=?", data);
		}
		else
		{
			result = db.executeWriteCommand("REPLACE INTO parameters VALUES(?, ?, ?, ?, ?, ?, ?, ?)", data);
		}
		releaseSavepoint("peerParameter" + std::to_string(peerID));
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
	releaseSavepoint("peerParameter" + std::to_string(peerID));
	return 0;
}

uint64_t DatabaseController::savePeerVariable(uint64_t peerID, BaseLib::Database::DataRow data)
{
	try
	{
		createSavepoint("peerVariable" + std::to_string(peerID));
		uint64_t result;
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not save peer variable. Variable ID is \"0\".");
				releaseSavepoint("peerVariable" + std::to_string(peerID));
				return 0;
			}
			switch(data.at(0)->dataType)
			{
			case BaseLib::Database::DataColumn::DataType::INTEGER:
				result = db.executeWriteCommand("UPDATE peerVariables SET integerValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::TEXT:
				result = db.executeWriteCommand("UPDATE peerVariables SET stringValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::BLOB:
				result = db.executeWriteCommand("UPDATE peerVariables SET binaryValue=? WHERE variableID=?", data);
				break;
			}
		}
		else
		{
			result = db.executeWriteCommand("REPLACE INTO peerVariables VALUES(?, ?, ?, ?, ?, ?)", data);
		}
		releaseSavepoint("peerVariable" + std::to_string(peerID));
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
	releaseSavepoint("peerVariable" + std::to_string(peerID));
	return 0;
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getPeerParameters(uint64_t peerID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(peerID)));
		std::shared_ptr<BaseLib::Database::DataTable> result = db.executeCommand("SELECT * FROM parameters WHERE peerID=?", data);
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
		std::shared_ptr<BaseLib::Database::DataTable> result = db.executeCommand("SELECT * FROM peerVariables WHERE peerID=?", data);
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

void DatabaseController::deletePeerParameter(uint64_t peerID, BaseLib::Database::DataRow data)
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
			db.executeCommand("DELETE FROM parameters WHERE peerID=? AND parameterID=?", data);
		}
		else if(data.size() == 4)
		{
			db.executeCommand("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND parameterName=?", data);
		}
		else if(data.size() == 5)
		{
			db.executeCommand("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND remotePeer=? AND remoteChannel=?", data);
		}
		else
		{
			db.executeCommand("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND parameterName=? AND remotePeer=? AND remoteChannel=?", data);
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
//End peer

//Service messages
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getServiceMessages(uint64_t peerID)
{
	try
	{
		BaseLib::Database::DataRow data;
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(peerID)));
		std::shared_ptr<BaseLib::Database::DataTable> result = db.executeCommand("SELECT * FROM serviceMessages WHERE peerID=?", data);
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

uint64_t DatabaseController::saveServiceMessage(uint64_t peerID, BaseLib::Database::DataRow data)
{
	try
	{
		createSavepoint("serviceMessage" + std::to_string(peerID));
		uint64_t result;
		if(data.size() == 2)
		{
			if(data.at(1)->intValue == 0)
			{
				GD::out.printError("Error: Could not save service message. Variable ID is \"0\".");
				releaseSavepoint("serviceMessage" + std::to_string(peerID));
				return 0;
			}
			switch(data.at(0)->dataType)
			{
			case BaseLib::Database::DataColumn::DataType::INTEGER:
				result = db.executeWriteCommand("UPDATE serviceMessages SET integerValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::TEXT:
				result = db.executeWriteCommand("UPDATE serviceMessages SET stringValue=? WHERE variableID=?", data);
				break;
			case BaseLib::Database::DataColumn::DataType::BLOB:
				result = db.executeWriteCommand("UPDATE serviceMessages SET binaryValue=? WHERE variableID=?", data);
				break;
			}
		}
		else if(data.size() == 4)
		{
			if(data.at(3)->intValue == 0)
			{
				GD::out.printError("Error: Could not save service message. Variable ID is \"0\".");
				releaseSavepoint("serviceMessage" + std::to_string(peerID));
				return 0;
			}
			result = db.executeWriteCommand("UPDATE serviceMessages SET integerValue=?, stringValue=?, binaryValue=? WHERE variableID=?", data);
		}
		else
		{
			result = db.executeWriteCommand("REPLACE INTO serviceMessages VALUES(?, ?, ?, ?, ?, ?)", data);
		}
		releaseSavepoint("serviceMessage" + std::to_string(peerID));
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
	releaseSavepoint("serviceMessage" + std::to_string(peerID));
	return 0;
}

void DatabaseController::deleteServiceMessage(uint64_t databaseID)
{
	try
	{
		BaseLib::Database::DataRow data({std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(databaseID))});
		db.executeCommand("DELETE FROM serviceMessages WHERE variableID=?", data);
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
