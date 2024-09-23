/* Copyright 2013-2020 Homegear GmbH
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

#include <sys/stat.h>

#include <memory>

namespace Homegear {

DatabaseController::DatabaseController() : IQueue(GD::bl.get(), 1, 100000) {
}

DatabaseController::~DatabaseController() {
  dispose();
}

void DatabaseController::dispose() {
  if (_disposing) return;
  _disposing = true;
  stopQueue(0);
  _db.dispose();
  _metadata.clear();
}

void DatabaseController::init() {
  if (!GD::bl) {
    GD::out.printCritical("Critical: Could not initialize database controller, because base library is not initialized.");
    return;
  }

  _rpcDecoder = std::make_unique<BaseLib::Rpc::RpcDecoder>(GD::bl.get(), false, false);
  _rpcEncoder = std::make_unique<BaseLib::Rpc::RpcEncoder>(GD::bl.get(), false, true);

  startQueue(0, true, 1, 0, SCHED_OTHER);
}

//General
void DatabaseController::open(const std::string &databasePath,
                              const std::string &databaseFilename,
                              const std::string &factoryDatabasePath,
                              bool databaseSynchronous,
                              bool databaseMemoryJournal,
                              bool databaseWALJournal,
                              const std::string &backupPath,
                              const std::string &factoryBackupPath,
                              const std::string &backupFilename) {
  _db.init(databasePath, databaseFilename, factoryDatabasePath, databaseSynchronous, databaseMemoryJournal, databaseWALJournal, backupPath, factoryBackupPath, backupFilename);
}

void DatabaseController::hotBackup() {
  _db.hotBackup(false);
  _db.hotBackup(true);
}

void DatabaseController::initializeDatabase() {
  try {
    _db.executeCommand("CREATE TABLE IF NOT EXISTS homegearVariables (variableID INTEGER PRIMARY KEY UNIQUE, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS homegearVariablesIndex ON homegearVariables (variableID, variableIndex)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS familyVariables (variableID INTEGER PRIMARY KEY UNIQUE, familyID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, variableName TEXT, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)",
                       false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS familyVariablesIndex ON familyVariables (variableID, familyID, variableIndex, variableName)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS peers (peerID INTEGER PRIMARY KEY UNIQUE, parent INTEGER NOT NULL, address INTEGER NOT NULL, serialNumber TEXT NOT NULL, type INTEGER NOT NULL)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS peersIndex ON peers (peerID, parent, address, serialNumber, type)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS peerVariables (variableID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS peerVariablesIndex ON peerVariables (variableID, peerID, variableIndex)", false);
    _db.executeCommand(
        "CREATE TABLE IF NOT EXISTS serviceMessages (variableID INTEGER PRIMARY KEY UNIQUE, familyID INTEGER NOT NULL, interface TEXT, peerID INTEGER NOT NULL, messageID INTEGER NOT NULL, messageSubID TEXT, timestamp INTEGER, integerValue INTEGER, message TEXT, variables BLOB, binaryData BLOB, priority INTEGER)",
        false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS serviceMessagesIndex ON serviceMessages (variableID, familyID, interface, peerID, messageID, messageSubID, timestamp, priority)", false);
    _db.executeCommand(
        "CREATE TABLE IF NOT EXISTS parameters (parameterID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, parameterSetType INTEGER NOT NULL, peerChannel INTEGER NOT NULL, remotePeer INTEGER, remoteChannel INTEGER, parameterName TEXT, value BLOB, room INTEGER, buildingPart INTEGER, categories TEXT, roles TEXT, specialType INTEGER, metadata BLOB)",
        false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS parametersIndex ON parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName, specialType)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS metadata (objectID TEXT, dataID TEXT, serializedObject BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS metadataIndex ON metadata (objectID, dataID)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS systemVariables (variableID TEXT PRIMARY KEY UNIQUE NOT NULL, serializedObject BLOB, room INTEGER, categories TEXT, flags INTEGER, roles TEXT)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS systemVariablesIndex ON systemVariables (variableID)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS devices (deviceID INTEGER PRIMARY KEY UNIQUE, address INTEGER NOT NULL, serialNumber TEXT NOT NULL, deviceType INTEGER NOT NULL, deviceFamily INTEGER NOT NULL)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS devicesIndex ON devices (deviceID, address, deviceType, deviceFamily)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS deviceVariables (variableID INTEGER PRIMARY KEY UNIQUE, deviceID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS deviceVariablesIndex ON deviceVariables (variableID, deviceID, variableIndex)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS licenseVariables (variableID INTEGER PRIMARY KEY UNIQUE, moduleID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS licenseVariablesIndex ON licenseVariables (variableID, moduleID, variableIndex)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS users (userID INTEGER PRIMARY KEY UNIQUE, name TEXT NOT NULL, password BLOB NOT NULL, salt BLOB NOT NULL, groups BLOB NOT NULL, metadata BLOB NOT NULL, keyIndex1 INTEGER, keyIndex2 INTEGER)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS usersIndex ON users (userID, name)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS userData (userID INTEGER, component TEXT, key TEXT, value BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS userDataIndex ON userData (userID, component, key)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS groups (id INTEGER PRIMARY KEY UNIQUE, translations BLOB NOT NULL, acl BLOB NOT NULL)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS groupsIndex ON groups (id)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS nodeData (node TEXT, key TEXT, value BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS nodeDataIndex ON nodeData (node, key)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS data (component TEXT, key TEXT, value BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS dataIndex ON data (component, key)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS rooms (id INTEGER PRIMARY KEY UNIQUE, translations BLOB, metadata BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS roomsIndex ON rooms (id)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS stories (id INTEGER PRIMARY KEY UNIQUE, translations BLOB, rooms TEXT, metadata BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS storiesIndex ON stories (id)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS buildingParts (id INTEGER PRIMARY KEY UNIQUE, translations BLOB, metadata BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS buildingPartsIndex ON buildingParts (id)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS buildings (id INTEGER PRIMARY KEY UNIQUE, translations BLOB, stories TEXT, buildingParts TEXT, metadata BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS buildingsIndex ON buildings (id)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS categories (id INTEGER PRIMARY KEY UNIQUE, translations BLOB, metadata BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS categoriesIndex ON categories (id)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS roles (id INTEGER PRIMARY KEY UNIQUE, translations BLOB, metadata BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS rolesIndex ON roles (id)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS uiElements (id INTEGER PRIMARY KEY UNIQUE, element TEXT, data BLOB, metadata BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS uiElementsIndex ON uiElements (id, element)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS uiNotifications (id INTEGER PRIMARY KEY UNIQUE, data BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS uiNotificationsIndex ON uiElements (id)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS variableProfiles (id INTEGER PRIMARY KEY UNIQUE, translations BLOB, profile BLOB)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS variableProfilesIndex ON variableProfiles (id)", false);

    //{{{ Create default groups
    {
      if (_db.executeCommand("SELECT id FROM groups WHERE id=1", false)->empty()) { //Administrators (1)
        BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        translations->structValue->emplace("en", std::make_shared<BaseLib::Variable>("Administrators"));
        translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("Administrators"));
        translations->structValue->emplace("de", std::make_shared<BaseLib::Variable>("Administratoren"));
        translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Administratoren"));
        std::vector<char> translationsBlob;
        _rpcEncoder->encodeResponse(translations, translationsBlob);

        BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        BaseLib::PVariable grantAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        grantAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
        acl->structValue->emplace("methods", grantAll);
        acl->structValue->emplace("eventServerMethods", grantAll);
        acl->structValue->emplace("services", grantAll);
        std::vector<char> aclBlob;
        _rpcEncoder->encodeResponse(acl, aclBlob);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(1));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
        _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data, false);
      }

      if (_db.executeCommand("SELECT id FROM groups WHERE id=2", false)->empty()) { //Script engine (2)
        BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        translations->structValue->emplace("en", std::make_shared<BaseLib::Variable>("Script Engine"));
        translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("Script Engine"));
        translations->structValue->emplace("de", std::make_shared<BaseLib::Variable>("Skriptengine"));
        translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Skriptengine"));
        std::vector<char> translationsBlob;
        _rpcEncoder->encodeResponse(translations, translationsBlob);

        BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        BaseLib::PVariable grantAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        grantAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
        acl->structValue->emplace("methods", grantAll);
        acl->structValue->emplace("eventServerMethods", grantAll);
        acl->structValue->emplace("services", grantAll);
        std::vector<char> aclBlob;
        _rpcEncoder->encodeResponse(acl, aclBlob);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(2));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
        _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data, false);
      }

      if (_db.executeCommand("SELECT id FROM groups WHERE id=3", false)->empty()) { //IPC (3)
        BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        translations->structValue->emplace("en", std::make_shared<BaseLib::Variable>("IPC"));
        translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("IPC"));
        translations->structValue->emplace("de", std::make_shared<BaseLib::Variable>("IPC"));
        translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("IPC"));
        std::vector<char> translationsBlob;
        _rpcEncoder->encodeResponse(translations, translationsBlob);

        BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        BaseLib::PVariable grantAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        grantAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
        acl->structValue->emplace("methods", grantAll);
        acl->structValue->emplace("eventServerMethods", grantAll);
        acl->structValue->emplace("services", grantAll);
        std::vector<char> aclBlob;
        _rpcEncoder->encodeResponse(acl, aclBlob);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(3));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
        _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data, false);
      }

      if (_db.executeCommand("SELECT id FROM groups WHERE id=4", false)->empty()) { //Node-BLUE (4)
        BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        translations->structValue->emplace("en", std::make_shared<BaseLib::Variable>("Node-BLUE"));
        translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("Node-BLUE"));
        translations->structValue->emplace("de", std::make_shared<BaseLib::Variable>("Node-BLUE"));
        translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Node-BLUE"));
        std::vector<char> translationsBlob;
        _rpcEncoder->encodeResponse(translations, translationsBlob);

        BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        BaseLib::PVariable grantAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        grantAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
        acl->structValue->emplace("methods", grantAll);
        acl->structValue->emplace("eventServerMethods", grantAll);
        acl->structValue->emplace("services", grantAll);
        std::vector<char> aclBlob;
        _rpcEncoder->encodeResponse(acl, aclBlob);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(4));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
        _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data, false);
      }

      if (_db.executeCommand("SELECT id FROM groups WHERE id=6", false)->empty()) { //MQTT (6)
        BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        translations->structValue->emplace("en", std::make_shared<BaseLib::Variable>("MQTT"));
        translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("MQTT"));
        translations->structValue->emplace("de", std::make_shared<BaseLib::Variable>("MQTT"));
        translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("MQTT"));
        std::vector<char> translationsBlob;
        _rpcEncoder->encodeResponse(translations, translationsBlob);

        BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        BaseLib::PVariable grantAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        grantAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
        acl->structValue->emplace("methods", grantAll);
        acl->structValue->emplace("eventServerMethods", grantAll);
        acl->structValue->emplace("services", grantAll);
        std::vector<char> aclBlob;
        _rpcEncoder->encodeResponse(acl, aclBlob);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(6));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
        _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data, false);
      }

      if (_db.executeCommand("SELECT id FROM groups WHERE id=7", false)->empty()) { //Family Module (7)
        BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        translations->structValue->emplace("en", std::make_shared<BaseLib::Variable>("Family Modules"));
        translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("Family Modules"));
        translations->structValue->emplace("de", std::make_shared<BaseLib::Variable>("Familienmodule"));
        translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Familienmodule"));
        std::vector<char> translationsBlob;
        _rpcEncoder->encodeResponse(translations, translationsBlob);

        BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        BaseLib::PVariable grantAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        grantAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
        acl->structValue->emplace("methods", grantAll);
        acl->structValue->emplace("eventServerMethods", grantAll);
        acl->structValue->emplace("services", grantAll);
        std::vector<char> aclBlob;
        _rpcEncoder->encodeResponse(acl, aclBlob);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(7));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
        _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data, false);
      }

      if (_db.executeCommand("SELECT id FROM groups WHERE id=8", false)->empty()) { //No User (8)
        BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        translations->structValue->emplace("en", std::make_shared<BaseLib::Variable>("No User"));
        translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("No User"));
        translations->structValue->emplace("de", std::make_shared<BaseLib::Variable>("Kein Benutzer"));
        translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Kein Benutzer"));
        std::vector<char> translationsBlob;
        _rpcEncoder->encodeResponse(translations, translationsBlob);

        BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        BaseLib::PVariable grantAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        grantAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
        acl->structValue->emplace("methods", grantAll);
        acl->structValue->emplace("eventServerMethods", grantAll);
        acl->structValue->emplace("services", grantAll);
        std::vector<char> aclBlob;
        _rpcEncoder->encodeResponse(acl, aclBlob);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(8));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
        _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data, false);
      }

      if (_db.executeCommand("SELECT id FROM groups WHERE id=9", false)->empty()) { //Unauthorized (9)
        BaseLib::PVariable translations = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        translations->structValue->emplace("en", std::make_shared<BaseLib::Variable>("Unauthorized"));
        translations->structValue->emplace("en-US", std::make_shared<BaseLib::Variable>("Unauthorized"));
        translations->structValue->emplace("de", std::make_shared<BaseLib::Variable>("Unauthorisiert"));
        translations->structValue->emplace("de-DE", std::make_shared<BaseLib::Variable>("Unauthorisiert"));
        std::vector<char> translationsBlob;
        _rpcEncoder->encodeResponse(translations, translationsBlob);

        BaseLib::PVariable acl = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        BaseLib::PVariable denyAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        denyAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(false));
        acl->structValue->emplace("methods", denyAll);
        acl->structValue->emplace("eventServerMethods", denyAll);
        acl->structValue->emplace("services", denyAll);
        std::vector<char> aclBlob;
        _rpcEncoder->encodeResponse(acl, aclBlob);

        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(9));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
        _db.executeCommand("INSERT INTO groups VALUES(?, ?, ?)", data, false);
      }
    }
    //}}}

    createDefaultRoles();

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
    auto result = _db.executeCommand("SELECT 1 FROM homegearVariables WHERE variableIndex=?", data, false);
    if (result->empty()) {
      data.clear();
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.8.5"));
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
      _db.executeCommand("INSERT INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, false);

      std::string password;
      password.reserve(12);
      for (int32_t i = 0; i < 12; i++) {
        int32_t number = BaseLib::HelperFunctions::getRandomNumber(33, 126);
        if (number == 96) number--; //Exclude "`"
        password.push_back((char)number);
      }

      GD::out.printMessage("Default password for user \"homegear\" set to: " + password);
      std::string content = "Default password for user \"homegear\": " + password + "\nPlease write down the password and delete this file.\n";
      std::string path = GD::bl->settings.writeableDataPath() + "defaultPassword.txt";
      BaseLib::Io::writeFile(path, content);
      uid_t userId = BaseLib::HelperFunctions::userId(GD::bl->settings.dataPathUser());
      gid_t groupId = BaseLib::HelperFunctions::groupId(GD::bl->settings.dataPathGroup());
      if (((int32_t)userId) == -1 || ((int32_t)groupId) == -1) {
        userId = GD::bl->userId;
        groupId = GD::bl->groupId;
      }
      if (chown(path.c_str(), userId, groupId) == -1) GD::out.printError("Could not set owner on " + path);
      if (chmod(path.c_str(), GD::bl->settings.dataPathPermissions()) == -1) GD::out.printError("Could not set permissions on " + path);

      std::vector<uint8_t> salt;
      std::vector<uint8_t> passwordHash = User::generateWHIRLPOOL(password, salt);

      std::string userName("homegear");
      std::vector<uint64_t> groups{1};
      createUser(userName, passwordHash, salt, groups);
    }

    std::string homegearInstanceId;
    if (!getHomegearVariableString(DatabaseController::HomegearVariables::uniqueid, homegearInstanceId)) {
      homegearInstanceId = BaseLib::HelperFunctions::getTimeUuid();
      setHomegearVariableString(DatabaseController::HomegearVariables::uniqueid, homegearInstanceId);
      GD::out.printMessage("Homegear instance ID is " + homegearInstanceId);
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) {
  std::shared_ptr<QueueEntry> queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
  if (!queueEntry) return;
  _db.executeWriteCommand(queueEntry->getEntry(), false);
  _db.executeWriteCommand(queueEntry->getEntry(), true);
}

bool DatabaseController::convertDatabase(const std::string &databasePath,
                                         const std::string &databaseFilename,
                                         const std::string &factoryDatabasePath,
                                         bool databaseSynchronous,
                                         bool databaseMemoryJournal,
                                         bool databaseWALJournal,
                                         const std::string &backupPath,
                                         const std::string &factoryBackupPath,
                                         const std::string &backupFilename) {
  try {
    for (int32_t i = 0; i < 2; i++) {
      if (i == 1 && factoryDatabasePath.empty()) break;
      bool factoryDatabase = (i != 0);
      std::string backupPathCopy = backupPath;
      if (backupPathCopy.empty()) backupPathCopy = databasePath;
      BaseLib::Database::DataRow data;
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::string("table")));
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::string("homegearVariables")));
      std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT 1 FROM sqlite_master WHERE type=? AND name=?", data, factoryDatabase);
      //Cannot proceed, because table homegearVariables does not exist
      if (rows->empty()) return false;
      data.clear();
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
      std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM homegearVariables WHERE variableIndex=?", data, factoryDatabase);
      if (result->empty()) return false; //Handled in initializeDatabase
      int64_t versionId = result->at(0).at(0)->intValue;
      std::string version = result->at(0).at(3)->textValue;

      static const std::string kCurrentVersion("0.8.5");

      if (version == kCurrentVersion) return false; //Up to date
      /*if(version == "0.0.7")
          {
              GD::out.printMessage("Converting database from version " + version + " to version 0.3.0...");
              db.init(GD::bl->settings.databasePath(), GD::bl->settings.databaseSynchronous(), GD::bl->settings.databaseMemoryJournal(), GD::bl->settings.databasePath() + ".old");

              db.executeCommand("ALTER TABLE events ADD COLUMN enabled INTEGER");
              db.executeCommand("UPDATE events SET enabled=1");

              loadDevicesFromDatabase(true);
              save(true);

              data.clear();
              data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(versionId)));
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
              data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(versionId)));
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
      _db.init(databasePath, databaseFilename, factoryDatabasePath, databaseSynchronous, databaseMemoryJournal, databaseWALJournal, backupPathCopy + backupFilename + ".0.6.0.old");
      if (version == "0.3.1") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.4.3...");

        _db.executeCommand("DELETE FROM peerVariables WHERE variableIndex=16", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.4.3"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.4.3";
      }
      if (version == "0.4.3") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.5.0...");

        _db.executeCommand("DELETE FROM peerVariables WHERE variableIndex=16", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.5.0"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.5.0";
      }
      if (version == "0.5.0") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.5.1...");

        _db.executeCommand("DELETE FROM peerVariables WHERE variableIndex=15", factoryDatabase);

        _db.executeCommand("CREATE TABLE IF NOT EXISTS serviceMessages (variableID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)", factoryDatabase);
        _db.executeCommand("CREATE INDEX IF NOT EXISTS serviceMessagesIndex ON peerVariables (variableID, peerID, variableIndex)", factoryDatabase);

        _db.executeCommand("UPDATE peerVariables SET variableIndex=1001 WHERE variableIndex=0", factoryDatabase);
        _db.executeCommand("UPDATE peerVariables SET variableIndex=1002 WHERE variableIndex=3", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.5.1"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.5.1";
      }
      if (version == "0.5.1") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.6.0...");

        _db.executeCommand("DELETE FROM devices WHERE deviceType!=4294967293 AND deviceType!=4278190077", factoryDatabase);
        _db.executeCommand("ALTER TABLE peers ADD COLUMN type INTEGER NOT NULL DEFAULT 0", factoryDatabase);
        _db.executeCommand("DROP INDEX IF EXISTS peersIndex", factoryDatabase);
        _db.executeCommand("CREATE INDEX peersIndex ON peers (peerID, parent, address, serialNumber, type)", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.6.0"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.6.0";
      }
      if (version == "0.6.0") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.6.1...");

        _db.executeCommand("UPDATE devices SET deviceType=4294967293 WHERE deviceType=4278190077", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.6.1"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.6.1";
      }
      if (version == "0.6.1") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.0...");

        BaseLib::PVariable groups = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        groups->arrayValue->push_back(std::make_shared<BaseLib::Variable>(1));
        std::vector<char> groupBlob;
        _rpcEncoder->encodeResponse(groups, groupBlob);

        BaseLib::PVariable metadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        std::vector<char> metadataBlob;
        _rpcEncoder->encodeResponse(metadata, metadataBlob);

        _db.executeCommand("ALTER TABLE users ADD COLUMN groups BLOB NOT NULL DEFAULT x'" + BaseLib::HelperFunctions::getHexString(groupBlob) + "'", factoryDatabase);
        _db.executeCommand("ALTER TABLE users ADD COLUMN metadata BLOB NOT NULL DEFAULT x'" + BaseLib::HelperFunctions::getHexString(metadataBlob) + "'", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.0"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.0";
      }
      if (version == "0.7.0") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.1...");

        BaseLib::PVariable groups = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        groups->arrayValue->push_back(std::make_shared<BaseLib::Variable>(1));
        std::vector<char> groupBlob;
        _rpcEncoder->encodeResponse(groups, groupBlob);

        BaseLib::PVariable metadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        std::vector<char> metadataBlob;
        _rpcEncoder->encodeResponse(metadata, metadataBlob);

        _db.executeCommand("ALTER TABLE rooms ADD COLUMN metadata BLOB", factoryDatabase);
        _db.executeCommand("ALTER TABLE categories ADD COLUMN metadata BLOB", factoryDatabase);
        _db.executeCommand("ALTER TABLE parameters ADD COLUMN room INTEGER", factoryDatabase);
        _db.executeCommand("ALTER TABLE parameters ADD COLUMN categories TEXT", factoryDatabase);

        _db.executeCommand("CREATE TABLE IF NOT EXISTS systemVariables2 (variableID TEXT PRIMARY KEY UNIQUE NOT NULL, serializedObject BLOB, room INTEGER, categories TEXT)", factoryDatabase);
        std::shared_ptr<BaseLib::Database::DataTable> systemVariablesRows = _db.executeCommand("SELECT variableID, serializedObject FROM systemVariables", factoryDatabase);
        for (auto &i: *systemVariablesRows) {
          if (i.second.size() < 2) continue;
          data.clear();
          data.push_back(i.second.at(0));
          data.push_back(i.second.at(1));
          _db.executeCommand("INSERT OR REPLACE INTO systemVariables2(variableID, serializedObject) VALUES(?, ?)", data, factoryDatabase);
        }
        _db.executeCommand("DROP INDEX systemVariablesIndex", factoryDatabase);
        _db.executeCommand("DROP TABLE systemVariables", factoryDatabase);
        _db.executeCommand("ALTER TABLE systemVariables2 RENAME TO systemVariables", factoryDatabase);
        _db.executeCommand("CREATE INDEX IF NOT EXISTS systemVariablesIndex ON systemVariables (variableID)", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.1"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.1";
      }
      if (version == "0.7.1" || version == "0.7.2") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.3...");

        _db.executeCommand("DROP INDEX serviceMessagesIndex", factoryDatabase);
        _db.executeCommand("DROP TABLE serviceMessages", factoryDatabase);
        _db.executeCommand(
            "CREATE TABLE IF NOT EXISTS serviceMessages (variableID INTEGER PRIMARY KEY UNIQUE, familyID INTEGER NOT NULL, peerID INTEGER NOT NULL, variableIndex INTEGER NOT NULL, timestamp INTEGER, integerValue INTEGER, stringValue TEXT, binaryValue BLOB)",
            factoryDatabase);
        _db.executeCommand("CREATE INDEX IF NOT EXISTS serviceMessagesIndex ON serviceMessages (variableID, peerID, variableIndex, timestamp)", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.3"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.3";
      }
      if (version == "0.7.3") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.4...");

        for (int32_t i = 1; i <= 8; i++) {
          auto aclStruct = getAcl(i);
          if (!aclStruct || aclStruct->errorStruct) continue;

          BaseLib::PVariable grantAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
          grantAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));
          aclStruct->structValue->emplace("services", grantAll);

          updateGroup(i, BaseLib::PVariable(), aclStruct);
        }

        {
          auto aclStruct = getAcl(9);
          if (aclStruct && !aclStruct->errorStruct) {
            BaseLib::PVariable denyAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
            denyAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(false));
            aclStruct->structValue->emplace("services", denyAll);

            updateGroup(9, BaseLib::PVariable(), aclStruct);
          }
        }

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.4"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.4";
      }
      if (version == "0.7.4") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.5...");

        data.clear();
        _db.executeWriteCommand("DELETE FROM serviceMessages", data, factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.5"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.5";
      }
      if (version == "0.7.5") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.6...");

        data.clear();
        _db.executeCommand("ALTER TABLE users ADD COLUMN keyIndex1 INTEGER", factoryDatabase);
        _db.executeCommand("ALTER TABLE users ADD COLUMN keyIndex2 INTEGER", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.6"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.6";
      }
      if (version == "0.7.6") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.7...");

        data.clear();
        _db.executeCommand("DROP INDEX serviceMessagesIndex", factoryDatabase);
        _db.executeCommand("DROP TABLE serviceMessages", factoryDatabase);
        _db.executeCommand(
            "CREATE TABLE IF NOT EXISTS serviceMessages (variableID INTEGER PRIMARY KEY UNIQUE, familyID INTEGER NOT NULL, peerID INTEGER NOT NULL, messageID INTEGER NOT NULL, messageSubID TEXT, timestamp INTEGER, integerValue INTEGER, message TEXT, variables BLOB, binaryData BLOB)",
            factoryDatabase);
        _db.executeCommand("CREATE INDEX IF NOT EXISTS serviceMessagesIndex ON serviceMessages (variableID, familyID, peerID, messageID, messageSubID, timestamp)", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.7"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.7";
      }
      if (version == "0.7.7") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.8...");

        data.clear();
        _db.executeCommand("ALTER TABLE systemVariables ADD COLUMN flags INTEGER", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.8"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.8";
      }
      if (version == "0.7.8") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.9...");

        data.clear();
        _db.executeCommand("ALTER TABLE parameters ADD COLUMN roles TEXT", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.9"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.9";
      }
      if (version == "0.7.9") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.10...");

        data.clear();
        _db.executeCommand("ALTER TABLE parameters ADD COLUMN specialType INTEGER", factoryDatabase);
        _db.executeCommand("ALTER TABLE parameters ADD COLUMN metadata BLOB", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.10"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.10";
      }
      if (version == "0.7.10") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.11...");

        data.clear();
        _db.executeCommand("ALTER TABLE systemVariables ADD COLUMN roles TEXT", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.11"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.11";
      }
      if (version == "0.7.11") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.7.12...");

        data.clear();
        _db.executeCommand("ALTER TABLE uiElements ADD COLUMN metadata BLOB", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.7.12"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.7.12";
      }
      if (version == "0.7.12") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.8.0...");

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("Base.doorHandle"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("Base.doorContact"));
        _db.executeWriteCommand("UPDATE uiElements SET element=? WHERE element=?", data, factoryDatabase);
        data.at(0)->textValue = "Base.heatingSliderModeWindowhandle";
        data.at(1)->textValue = "Base.heatingIsStateSliderModeWindowContact";
        _db.executeWriteCommand("UPDATE uiElements SET element=? WHERE element=?", data, factoryDatabase);
        data.at(0)->textValue = "Base.heatingSliderModeWindowhandle";
        data.at(1)->textValue = "Base.heatingIsStateSliderModeWindow";
        _db.executeWriteCommand("UPDATE uiElements SET element=? WHERE element=?", data, factoryDatabase);
        data.at(0)->textValue = "Base.heatingSliderModeWindowhandle";
        data.at(1)->textValue = "Base.heatingIsStateSliderModeWindowHandle";
        _db.executeWriteCommand("UPDATE uiElements SET element=? WHERE element=?", data, factoryDatabase);
        data.at(0)->textValue = "Base.heatingSliderMode";
        data.at(1)->textValue = "Base.heatingIsStateSliderMode";
        _db.executeWriteCommand("UPDATE uiElements SET element=? WHERE element=?", data, factoryDatabase);
        data.at(0)->textValue = "Base.heatingTemperature";
        data.at(1)->textValue = "Base.heatingIsState";
        _db.executeWriteCommand("UPDATE uiElements SET element=? WHERE element=?", data, factoryDatabase);
        data.at(0)->textValue = "Base.windowHandle";
        data.at(1)->textValue = "Base.windowContact";
        _db.executeWriteCommand("UPDATE uiElements SET element=? WHERE element=?", data, factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.8.0"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.8.0";
      }
      if (version == "0.8.0") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.8.1...");

        data.clear();
        _db.executeCommand("DROP INDEX serviceMessagesIndex", factoryDatabase);
        _db.executeCommand("DROP TABLE serviceMessages", factoryDatabase);
        _db.executeCommand(
            "CREATE TABLE IF NOT EXISTS serviceMessages (variableID INTEGER PRIMARY KEY UNIQUE, familyID INTEGER NOT NULL, interface TEXT, peerID INTEGER NOT NULL, messageID INTEGER NOT NULL, messageSubID TEXT, timestamp INTEGER, integerValue INTEGER, message TEXT, variables BLOB, binaryData BLOB)",
            factoryDatabase);
        _db.executeCommand("CREATE INDEX IF NOT EXISTS serviceMessagesIndex ON serviceMessages (variableID, familyID, interface, peerID, messageID, messageSubID, timestamp)", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.8.1"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.8.1";
      }
      if (version == "0.8.1") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.8.2...");

        data.clear();
        _db.executeCommand("DROP INDEX serviceMessagesIndex", factoryDatabase);
        _db.executeCommand("DROP TABLE serviceMessages", factoryDatabase);
        _db.executeCommand(
            "CREATE TABLE IF NOT EXISTS serviceMessages (variableID INTEGER PRIMARY KEY UNIQUE, familyID INTEGER NOT NULL, interface TEXT, peerID INTEGER NOT NULL, messageID INTEGER NOT NULL, messageSubID TEXT, timestamp INTEGER, integerValue INTEGER, message TEXT, variables BLOB, binaryData BLOB, priority INTEGER)",
            factoryDatabase);
        _db.executeCommand("CREATE INDEX IF NOT EXISTS serviceMessagesIndex ON serviceMessages (variableID, familyID, interface, peerID, messageID, messageSubID, timestamp, priority)", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.8.2"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.8.2";
      }
      if (version == "0.8.2") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.8.3...");

        data.clear();
        _db.executeCommand("ALTER TABLE buildings ADD COLUMN buildingParts TEXT", factoryDatabase);

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.8.3"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.8.3";
      }
      if (version == "0.8.3" || version == "0.8.4") {
        GD::out.printMessage("Converting database from version " + version + " to version 0.8.5...");

        auto rows2 = _db.executeCommand("SELECT * FROM parameters", factoryDatabase);

        data.clear();
        _db.executeCommand("DROP TABLE parameters", factoryDatabase);
        _db.executeCommand(
            "CREATE TABLE IF NOT EXISTS parameters (parameterID INTEGER PRIMARY KEY UNIQUE, peerID INTEGER NOT NULL, parameterSetType INTEGER NOT NULL, peerChannel INTEGER NOT NULL, remotePeer INTEGER, remoteChannel INTEGER, parameterName TEXT, value BLOB, room INTEGER, buildingPart INTEGER, categories TEXT, roles TEXT, specialType INTEGER, metadata BLOB)",
            factoryDatabase);

        for (auto &row: *rows2) {
          data.clear();
          for (uint32_t column_index = 0; column_index < 9; column_index++) {
            data.push_back(row.second.at(column_index));
          }
          data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
          for (uint32_t column_index = 10; column_index < 14; column_index++) {
            data.push_back(row.second.at(column_index - 1));
          }

          _db.executeWriteCommand("REPLACE INTO parameters VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", data, factoryDatabase);
        }

        data.clear();
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(versionId));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        //Don't forget to set new version in initializeDatabase!!!
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>("0.8.5"));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
        _db.executeWriteCommand("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data, factoryDatabase);

        version = "0.8.5";
      }

      if (version != kCurrentVersion) {
        GD::out.printCritical("Critical: Unknown database version: " + version);
        return true; //Don't know, what to do
      }
    }

    return false;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return true;
}

bool DatabaseController::enableMaintenanceMode() {
  try {
    GD::bl->maintenanceMode = true;

    return _db.enableMaintenanceMode();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

bool DatabaseController::disableMaintenanceMode() {
  try {
    if (!GD::bl->maintenanceMode) return false;
    GD::bl->maintenanceMode = false;
    bool result = _db.disableMaintenanceMode();
    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

void DatabaseController::createSavepointSynchronous(std::string &name) {
  if (GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Creating savepoint (synchronous) " + name);
  BaseLib::Database::DataRow data;
  _db.executeWriteCommand("SAVEPOINT " + name, data, false);
  _db.executeWriteCommand("SAVEPOINT " + name, data, true);
}

void DatabaseController::releaseSavepointSynchronous(std::string &name) {
  if (GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Releasing savepoint (synchronous) " + name);
  BaseLib::Database::DataRow data;
  _db.executeWriteCommand("RELEASE " + name, data, false);
  _db.executeWriteCommand("RELEASE " + name, data, true);
}

void DatabaseController::createSavepointAsynchronous(std::string &name) {
  if (GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Creating savepoint (asynchronous) " + name);
  BaseLib::Database::DataRow data;
  std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("SAVEPOINT " + name, data);
  enqueue(0, entry);
}

void DatabaseController::releaseSavepointAsynchronous(std::string &name) {
  if (GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Releasing savepoint (asynchronous) " + name);
  BaseLib::Database::DataRow data;
  std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("RELEASE " + name, data);
  enqueue(0, entry);
}
//End general

//Homegear variables
bool DatabaseController::getHomegearVariableString(HomegearVariables::Enum id, std::string &value) {
  BaseLib::Database::DataRow data;
  data.push_back(std::make_shared<BaseLib::Database::DataColumn>((uint64_t)id));
  std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT stringValue FROM homegearVariables WHERE variableIndex=?", data, false);
  if (result->empty() || result->at(0).empty()) return false;
  value = result->at(0).at(0)->textValue;
  return true;
}

void DatabaseController::setHomegearVariableString(HomegearVariables::Enum id, std::string &value) {
  BaseLib::Database::DataRow data;
  data.push_back(std::make_shared<BaseLib::Database::DataColumn>((uint64_t)id));
  std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT variableID FROM homegearVariables WHERE variableIndex=?", data, false);
  data.clear();
  if (result->empty() || result->at(0).empty()) {
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>((uint64_t)id));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
    //Don't forget to set new version in initializeDatabase!!!
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(value));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
    enqueue(0, entry);
  } else {
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(result->at(0).at(0)->intValue));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>((uint64_t)id));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
    //Don't forget to set new version in initializeDatabase!!!
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(value));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO homegearVariables VALUES(?, ?, ?, ?, ?)", data);
    enqueue(0, entry);
  }
}
//End Homegear variables

//{{{ Data
BaseLib::PVariable DatabaseController::getData(std::string &component, std::string &key) {
  try {
    BaseLib::PVariable value;

    if (!key.empty()) {
      std::lock_guard<std::mutex> dataGuard(_dataMutex);
      auto componentIterator = _data.find(component);
      if (componentIterator != _data.end()) {
        auto keyIterator = componentIterator->second.find(key);
        if (keyIterator != componentIterator->second.end()) {
          value = keyIterator->second;
          return value;
        }
      }
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(component));
    std::string command;
    if (!key.empty()) {
      command = "SELECT value FROM data WHERE component=? AND key=?";
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(key));
    } else command = "SELECT key, value FROM data WHERE component=?";

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand(command, data, false);
    if (rows->empty() || rows->at(0).empty()) return std::make_shared<BaseLib::Variable>();

    if (key.empty()) {
      value = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      for (auto &row: *rows) {
        value->structValue->emplace(row.second.at(0)->textValue, _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue));
      }
    } else {
      value = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
      std::lock_guard<std::mutex> dataGuard(_dataMutex);
      _data[component][key] = value;
    }

    return value;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::setData(std::string &component, std::string &key, BaseLib::PVariable &value) {
  try {
    if (!value) return BaseLib::Variable::createError(-32602, "Could not parse data.");
    if (component.empty()) return BaseLib::Variable::createError(-32602, "component is an empty string.");
    if (key.empty()) return BaseLib::Variable::createError(-32602, "key is an empty string.");
    if (component.size() > 250) return BaseLib::Variable::createError(-32602, "component has more than 250 characters.");
    if (key.size() > 250) return BaseLib::Variable::createError(-32602, "key has more than 250 characters.");
    //Don't check for type here, so base64, string and future data types that use stringValue are handled
    if (value->type != BaseLib::VariableType::tBase64 && value->type != BaseLib::VariableType::tString && value->type != BaseLib::VariableType::tInteger && value->type != BaseLib::VariableType::tInteger64
        && value->type != BaseLib::VariableType::tFloat && value->type != BaseLib::VariableType::tBoolean && value->type != BaseLib::VariableType::tStruct && value->type != BaseLib::VariableType::tArray)
      return BaseLib::Variable::createError(-32602, "Type " + BaseLib::Variable::getTypeString(value->type) + " is currently not supported.");

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT COUNT(*) FROM data", false);
    if (rows->empty() || rows->at(0).empty()) {
      return BaseLib::Variable::createError(-32500, "Error counting data in database.");
    }
    if (rows->at(0).at(0)->intValue > 1000000) {
      return BaseLib::Variable::createError(-32500, "Reached limit of 1000000 data entries. Please delete data before adding new entries.");
    }

    {
      std::lock_guard<std::mutex> dataGuard(_dataMutex);
      _data[component][key] = value;
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(component));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(key));
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM data WHERE component=? AND key=?", data);
    enqueue(0, entry);

    std::vector<char> encodedValue;
    _rpcEncoder->encodeResponse(value, encodedValue);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(encodedValue));
    entry = std::make_shared<QueueEntry>("INSERT INTO data VALUES(?, ?, ?)", data);
    enqueue(0, entry);

    return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tVoid);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteData(std::string &component, std::string &key) {
  try {
    {
      std::lock_guard<std::mutex> dataGuard(_dataMutex);
      if (key.empty()) _data.erase(component);
      else {
        auto dataIterator = _data.find(component);
        if (dataIterator != _data.end()) dataIterator->second.erase(key);
      }
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(component));
    std::string command("DELETE FROM data WHERE component=?");
    if (!key.empty()) {
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(key));
      command.append(" AND key=?");
    }
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(command, data);
    enqueue(0, entry);

    return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tVoid);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//{{{ UI
uint64_t DatabaseController::addUiElement(const std::string &elementStringId, const BaseLib::PVariable &data, const BaseLib::PVariable &metadata) {
  try {
    BaseLib::Database::DataRow rowData;
    rowData.push_back(std::make_shared<BaseLib::Database::DataColumn>());

    rowData.push_back(std::make_shared<BaseLib::Database::DataColumn>(elementStringId));

    std::vector<char> dataBlob;
    _rpcEncoder->encodeResponse(data, dataBlob);
    rowData.push_back(std::make_shared<BaseLib::Database::DataColumn>(dataBlob));

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);
    rowData.push_back(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

    uint64_t result = _db.executeWriteCommand("REPLACE INTO uiElements VALUES(?, ?, ?, ?)", rowData, false);
    rowData.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand("REPLACE INTO uiElements VALUES(?, ?, ?, ?)", rowData, true);

    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getUiElements() {
  try {
    return _db.executeCommand("SELECT id, element, data, metadata FROM uiElements", false);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

BaseLib::PVariable DatabaseController::getUiElementMetadata(uint64_t databaseId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(databaseId));
    auto rows = _db.executeCommand("SELECT metadata FROM uiElements WHERE id=?", data, false);

    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown UI element.");

    return _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void DatabaseController::removeUiElement(uint64_t databaseId) {
  try {
    BaseLib::Database::DataRow rowData;
    rowData.push_back(std::make_shared<BaseLib::Database::DataColumn>(databaseId));

    _db.executeWriteCommand("DELETE FROM uiElements WHERE id=?", rowData, false);
    _db.executeWriteCommand("DELETE FROM uiElements WHERE id=?", rowData, true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

BaseLib::PVariable DatabaseController::setUiElementMetadata(uint64_t databaseId, const BaseLib::PVariable &metadata) {
  try {
    {
      BaseLib::Database::DataRow data;
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(databaseId));
      if (_db.executeCommand("SELECT id FROM uiElements WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown UI element.");

      std::vector<char> metadataBlob;
      _rpcEncoder->encodeResponse(metadata, metadataBlob);

      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
      _db.executeCommand("UPDATE uiElements SET metadata=? WHERE id=?", data, false);
    }

    {
      BaseLib::Database::DataRow data;
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(databaseId));
      if (_db.executeCommand("SELECT id FROM uiElements WHERE id=?", data, true)->empty()) return std::make_shared<BaseLib::Variable>();

      std::vector<char> metadataBlob;
      _rpcEncoder->encodeResponse(metadata, metadataBlob);

      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
      _db.executeCommand("UPDATE uiElements SET metadata=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//{{{ UI notifications
uint64_t DatabaseController::createUiNotification(const BaseLib::PVariable &notificationDescription) {
  try {
    BaseLib::Database::DataRow rowData;
    rowData.push_back(std::make_shared<BaseLib::Database::DataColumn>());

    //{
    //  "type": "bla", //An arbitrary string to identify the notification
    //  "flags": {
    //    "closeable": true, //Shows a button when shown as an overlay to close the notification
    //    "hasModal": true, //Does the notification have a modal with additional information?
    //    "overlayNotification": true, //Makes the notification appear as an overlay
    //    "overlayModal": true //The user needs to interact with the modal to continue
    //  }
    //  "title": {
    //    "en-US": "Please do something",
    //    .
    //    .
    //    .
    //  },
    //  "overlayContent": {
    //    "en-US": "Content",
    //    .
    //    .
    //    .
    //  },
    //  "modalTitle": {
    //    "en-US": "My Modal",
    //    .
    //    .
    //    .
    //  }
    //  "modalContent": {
    //    "en-US": "<p>My HTML</p>"
    //  "}
    //  "buttons": [ //Modal and notification overlay buttons. The buttons are only displayed in the notification overlay, when there is no modal defined.
    //    {
    //      "id": 0, //A number to identify the button
    //      "type": "success", //"success", "warning", "error" - used for setting CSS classes
    //      "reloadUi": true, //Reload UI on button press?
    //      "closeModal": true, //Closes the modal
    //      "label": {
    //        "en-US": "OK"
    //      },
    //      "icon": "my-icon"
    //    },
    //    .
    //    .
    //    .
    //  ]
    //}

    std::vector<char> dataBlob;
    _rpcEncoder->encodeResponse(notificationDescription, dataBlob);
    rowData.push_back(std::make_shared<BaseLib::Database::DataColumn>(dataBlob));

    uint64_t result = _db.executeWriteCommand("REPLACE INTO uiNotifications VALUES(?, ?)", rowData, false);
    rowData.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand("REPLACE INTO uiNotifications VALUES(?, ?)", rowData, true);

    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return 0;
}

BaseLib::PVariable DatabaseController::getUiNotification(uint64_t databaseId, const std::string &languageCode) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(databaseId));
    auto rows = _db.executeCommand("SELECT data FROM uiNotifications WHERE id=?", data, false);

    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown UI notification.");

    auto shortLanguageCode = BaseLib::HelperFunctions::splitFirst(languageCode, '-').first;

    auto notification = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);

    auto contentIterator = notification->structValue->find("title");
    if (contentIterator != notification->structValue->end()) {
      auto languageIterator = contentIterator->second->structValue->find(languageCode);
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find(shortLanguageCode);
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en");
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en-US");
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->begin();
      if (languageIterator != contentIterator->second->structValue->end()) {
        contentIterator->second->stringValue = languageIterator->second->stringValue;
      }
      contentIterator->second->type = BaseLib::VariableType::tString;
    }

    contentIterator = notification->structValue->find("overlayContent");
    if (contentIterator != notification->structValue->end()) {
      auto languageIterator = contentIterator->second->structValue->find(languageCode);
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find(shortLanguageCode);
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en");
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en-US");
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->begin();
      if (languageIterator != contentIterator->second->structValue->end()) {
        contentIterator->second->stringValue = languageIterator->second->stringValue;
      }
      contentIterator->second->type = BaseLib::VariableType::tString;
    }

    contentIterator = notification->structValue->find("modalTitle");
    if (contentIterator != notification->structValue->end()) {
      auto languageIterator = contentIterator->second->structValue->find(languageCode);
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find(shortLanguageCode);
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en");
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en-US");
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->begin();
      if (languageIterator != contentIterator->second->structValue->end()) {
        contentIterator->second->stringValue = languageIterator->second->stringValue;
      }
      contentIterator->second->type = BaseLib::VariableType::tString;
    }

    contentIterator = notification->structValue->find("modalContent");
    if (contentIterator != notification->structValue->end()) {
      auto languageIterator = contentIterator->second->structValue->find(languageCode);
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find(shortLanguageCode);
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en");
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en-US");
      if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->begin();
      if (languageIterator != contentIterator->second->structValue->end()) {
        contentIterator->second->stringValue = languageIterator->second->stringValue;
      }
      contentIterator->second->type = BaseLib::VariableType::tString;
    }

    auto buttonsIterator = notification->structValue->find("buttons");
    if (buttonsIterator != notification->structValue->end()) {
      for (auto &button: *buttonsIterator->second->arrayValue) {
        auto labelIterator = button->structValue->find("label");
        if (labelIterator != button->structValue->end()) {
          auto languageIterator = labelIterator->second->structValue->find(languageCode);
          if (languageIterator == labelIterator->second->structValue->end()) languageIterator = labelIterator->second->structValue->find(shortLanguageCode);
          if (languageIterator == labelIterator->second->structValue->end()) languageIterator = labelIterator->second->structValue->find("en");
          if (languageIterator == labelIterator->second->structValue->end()) languageIterator = labelIterator->second->structValue->find("en-US");
          if (languageIterator == labelIterator->second->structValue->end()) languageIterator = labelIterator->second->structValue->begin();
          if (languageIterator != labelIterator->second->structValue->end()) {
            labelIterator->second->stringValue = languageIterator->second->stringValue;
          }
          labelIterator->second->type = BaseLib::VariableType::tString;
        }
      }
    }

    return notification;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getUiNotifications(const std::string &languageCode) {
  try {
    auto rows = _db.executeCommand("SELECT id, data FROM uiNotifications", false);

    auto shortLanguageCode = BaseLib::HelperFunctions::splitFirst(languageCode, '-').first;

    auto notifications = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    notifications->arrayValue->reserve(rows->size());

    for (auto &row: *rows) {
      auto id = (uint64_t)row.second.at(0)->intValue;
      auto notification = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);

      notification->structValue->emplace("id", std::make_shared<BaseLib::Variable>(id));

      auto contentIterator = notification->structValue->find("title");
      if (contentIterator != notification->structValue->end()) {
        auto languageIterator = contentIterator->second->structValue->find(languageCode);
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find(shortLanguageCode);
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en");
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en-US");
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->begin();
        if (languageIterator != contentIterator->second->structValue->end()) {
          contentIterator->second->stringValue = languageIterator->second->stringValue;
        }
        contentIterator->second->type = BaseLib::VariableType::tString;
      }

      contentIterator = notification->structValue->find("overlayContent");
      if (contentIterator != notification->structValue->end()) {
        auto languageIterator = contentIterator->second->structValue->find(languageCode);
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find(shortLanguageCode);
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en");
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en-US");
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->begin();
        if (languageIterator != contentIterator->second->structValue->end()) {
          contentIterator->second->stringValue = languageIterator->second->stringValue;
        }
        contentIterator->second->type = BaseLib::VariableType::tString;
      }

      contentIterator = notification->structValue->find("modalTitle");
      if (contentIterator != notification->structValue->end()) {
        auto languageIterator = contentIterator->second->structValue->find(languageCode);
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find(shortLanguageCode);
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en");
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en-US");
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->begin();
        if (languageIterator != contentIterator->second->structValue->end()) {
          contentIterator->second->stringValue = languageIterator->second->stringValue;
        }
        contentIterator->second->type = BaseLib::VariableType::tString;
      }

      contentIterator = notification->structValue->find("modalContent");
      if (contentIterator != notification->structValue->end()) {
        auto languageIterator = contentIterator->second->structValue->find(languageCode);
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find(shortLanguageCode);
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en");
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->find("en-US");
        if (languageIterator == contentIterator->second->structValue->end()) languageIterator = contentIterator->second->structValue->begin();
        if (languageIterator != contentIterator->second->structValue->end()) {
          contentIterator->second->stringValue = languageIterator->second->stringValue;
        }
        contentIterator->second->type = BaseLib::VariableType::tString;
      }

      auto buttonsIterator = notification->structValue->find("buttons");
      if (buttonsIterator != notification->structValue->end()) {
        for (auto &button: *buttonsIterator->second->arrayValue) {
          auto labelIterator = button->structValue->find("label");
          if (labelIterator != button->structValue->end()) {
            auto languageIterator = labelIterator->second->structValue->find(languageCode);
            if (languageIterator == labelIterator->second->structValue->end()) languageIterator = labelIterator->second->structValue->find(shortLanguageCode);
            if (languageIterator == labelIterator->second->structValue->end()) languageIterator = labelIterator->second->structValue->find("en");
            if (languageIterator == labelIterator->second->structValue->end()) languageIterator = labelIterator->second->structValue->find("en-US");
            if (languageIterator == labelIterator->second->structValue->end()) languageIterator = labelIterator->second->structValue->begin();
            if (languageIterator != labelIterator->second->structValue->end()) {
              labelIterator->second->stringValue = languageIterator->second->stringValue;
            }
            labelIterator->second->type = BaseLib::VariableType::tString;
          }
        }
      }

      notifications->arrayValue->emplace_back(std::move(notification));
    }

    return notifications;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void DatabaseController::removeUiNotification(uint64_t databaseId) {
  try {
    BaseLib::Database::DataRow rowData;
    rowData.push_back(std::make_shared<BaseLib::Database::DataColumn>(databaseId));

    _db.executeWriteCommand("DELETE FROM uiNotifications WHERE id=?", rowData, false);
    _db.executeWriteCommand("DELETE FROM uiNotifications WHERE id=?", rowData, true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}
//}}}

//{{{ Buildings
BaseLib::PVariable DatabaseController::addBuildingPartToBuilding(uint64_t buildingId, uint64_t buildingPartId) {
  try {
    if (!buildingPartExists(buildingPartId)) return BaseLib::Variable::createError(-2, "Unknown building part.");
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingId));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT buildingParts FROM buildings WHERE id=?", data, false);
    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown building.");

    std::vector<std::string> buildingPartStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(0)->textValue, ',');
    bool containsBuildingPart = false;

    std::ostringstream buildingPartStream;
    for (auto &buildingPartString: buildingPartStrings) {
      auto buildingPart = (uint64_t)BaseLib::Math::getNumber64(buildingPartString);
      if (buildingPart == buildingPartId) containsBuildingPart = true;
      buildingPartStream << std::to_string(buildingPart) << ",";
    }
    if (!containsBuildingPart) {
      buildingPartStream << std::to_string(buildingPartId) << ",";
      std::string buildingPartString = buildingPartStream.str();
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(buildingPartString));
      _db.executeCommand("UPDATE buildings SET buildingParts=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE buildings SET buildingParts=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::addStoryToBuilding(uint64_t buildingId, uint64_t storyId) {
  try {
    if (!storyExists(storyId)) return BaseLib::Variable::createError(-2, "Unknown story.");
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingId));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT stories FROM buildings WHERE id=?", data, false);
    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown building.");

    std::vector<std::string> storyStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(0)->textValue, ',');
    bool containsStory = false;

    std::ostringstream storyStream;
    for (auto &storyString: storyStrings) {
      auto story = (uint64_t)BaseLib::Math::getNumber64(storyString);
      if (story == storyId) containsStory = true;
      storyStream << std::to_string(story) << ",";
    }
    if (!containsStory) {
      storyStream << std::to_string(storyId) << ",";
      std::string storyString = storyStream.str();
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(storyString));
      _db.executeCommand("UPDATE buildings SET stories=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE buildings SET stories=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::createBuilding(BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::string()));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::string()));

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

    uint64_t result = _db.executeWriteCommand("REPLACE INTO buildings VALUES(?, ?, ?, ?, ?)", data, false);
    data.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand("REPLACE INTO buildings VALUES(?, ?, ?, ?, ?)", data, true);

    return std::make_shared<BaseLib::Variable>(result);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteBuilding(uint64_t buildingId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingId));
    if (_db.executeCommand("SELECT id FROM buildings WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown building.");

    _db.executeWriteCommand("DELETE FROM buildings WHERE id=?", data, false);
    _db.executeWriteCommand("DELETE FROM buildings WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getBuildingPartsInBuilding(BaseLib::PRpcClientInfo clientInfo, uint64_t buildingId, bool checkAcls) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingId));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT buildingParts FROM buildings WHERE id=?", data, false);
    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown building.");
    std::multimap<int32_t, uint64_t> sortedBuildingParts;
    int32_t pos = 0;
    std::vector<std::string> buildingPartStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(0)->textValue, ',');
    for (auto &getBuildingPartString: buildingPartStrings) {
      auto buildingPart = (uint64_t)BaseLib::Math::getNumber64(getBuildingPartString);
      if (buildingPart != 0) {
        auto metadata = getStoryMetadata(buildingPart);
        auto positionIterator = metadata->structValue->find("position");
        if (positionIterator != metadata->structValue->end()) sortedBuildingParts.emplace(positionIterator->second->integerValue64, buildingPart);
        else sortedBuildingParts.emplace(pos++, buildingPart);
      }
    }

    auto buildingParts = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    buildingParts->arrayValue->reserve(sortedBuildingParts.size());
    for (auto &story: sortedBuildingParts) {
      buildingParts->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(story.second));
    }
    return buildingParts;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getStoriesInBuilding(BaseLib::PRpcClientInfo clientInfo, uint64_t buildingId, bool checkAcls) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingId));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT stories FROM buildings WHERE id=?", data, false);
    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown building.");
    std::multimap<int32_t, uint64_t> sortedStories;
    int32_t pos = 0;
    std::vector<std::string> storyStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(0)->textValue, ',');
    for (auto &storyString: storyStrings) {
      auto story = (uint64_t)BaseLib::Math::getNumber64(storyString);
      if (story != 0) {
        auto metadata = getStoryMetadata(story);
        auto positionIterator = metadata->structValue->find("position");
        if (positionIterator != metadata->structValue->end()) sortedStories.emplace(positionIterator->second->integerValue64, story);
        else sortedStories.emplace(pos++, story);
      }
    }

    auto stories = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    stories->arrayValue->reserve(sortedStories.size());
    for (auto &story: sortedStories) {
      stories->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(story.second));
    }
    return stories;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getBuildingMetadata(uint64_t buildingId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingId));
    auto rows = _db.executeCommand("SELECT metadata FROM buildings WHERE id=?", data, false);

    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown story.");

    return _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getBuildings(std::string languageCode) {
  try {
    std::multimap<int32_t, BaseLib::PVariable> sortedBuildings;
    int32_t buildingPos = 0;

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations, stories, buildingParts, metadata FROM buildings", false);
    for (auto &row: *rows) {
      BaseLib::PVariable building = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      building->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(row.second.at(0)->intValue));
      BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
      if (languageCode.empty()) building->structValue->emplace("TRANSLATIONS", translations);
      else {
        auto translationIterator = translations->structValue->find(languageCode);
        if (translationIterator != translations->structValue->end()) building->structValue->emplace("NAME", translationIterator->second);
        else {
          auto shortLanguageCode = BaseLib::HelperFunctions::splitFirst(languageCode, '-').first;
          translationIterator = translations->structValue->find(shortLanguageCode);
          if (translationIterator != translations->structValue->end()) building->structValue->emplace("NAME", translationIterator->second);
          else {
            translationIterator = translations->structValue->find("en");
            if (translationIterator != translations->structValue->end()) building->structValue->emplace("NAME", translationIterator->second);
            else {
              translationIterator = translations->structValue->find("en-US");
              if (translationIterator != translations->structValue->end()) building->structValue->emplace("NAME", translationIterator->second);
              else if (!translations->structValue->empty()) building->structValue->emplace("NAME", translations->structValue->begin()->second);
              else building->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
            }
          }
        }
      }

      {
        std::multimap<int32_t, uint64_t> sortedStories;
        int32_t storyPos = 0;
        std::vector<std::string> storyStrings = BaseLib::HelperFunctions::splitAll(row.second.at(2)->textValue, ',');
        for (auto &storyString: storyStrings) {
          if (storyString.empty()) continue;
          auto story = (uint64_t)BaseLib::Math::getNumber64(storyString);
          if (story != 0) {
            auto metadata = getStoryMetadata(story);
            auto positionIterator = metadata->structValue->find("position");
            if (positionIterator != metadata->structValue->end()) sortedStories.emplace(positionIterator->second->integerValue64, story);
            else sortedStories.emplace(storyPos++, story);
          }
        }
        BaseLib::PVariable stories = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        stories->arrayValue->reserve(sortedStories.size());
        for (auto &story: sortedStories) {
          stories->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(story.second));
        }
        building->structValue->emplace("STORIES", stories);
      }

      {
        std::multimap<int32_t, uint64_t> sortedBuildingParts;
        int32_t buildingPartPos = 0;
        std::vector<std::string> buildingPartStrings = BaseLib::HelperFunctions::splitAll(row.second.at(3)->textValue, ',');
        for (auto &buildingPartString: buildingPartStrings) {
          if (buildingPartString.empty()) continue;
          auto buildingPart = (uint64_t)BaseLib::Math::getNumber64(buildingPartString);
          if (buildingPart != 0) {
            auto metadata = getStoryMetadata(buildingPart);
            auto positionIterator = metadata->structValue->find("position");
            if (positionIterator != metadata->structValue->end()) sortedBuildingParts.emplace(positionIterator->second->integerValue64, buildingPart);
            else sortedBuildingParts.emplace(buildingPartPos++, buildingPart);
          }
        }
        BaseLib::PVariable buildingParts = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        buildingParts->arrayValue->reserve(sortedBuildingParts.size());
        for (auto &buildingPart: sortedBuildingParts) {
          buildingParts->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(buildingPart.second));
        }
        building->structValue->emplace("BUILDING_PARTS", buildingParts);
      }

      if (!row.second.at(4)->binaryValue->empty()) {
        BaseLib::PVariable metadata = _rpcDecoder->decodeResponse(*row.second.at(4)->binaryValue);
        building->structValue->emplace("METADATA", metadata);

        auto positionIterator = metadata->structValue->find("position");
        if (positionIterator != metadata->structValue->end()) sortedBuildings.emplace(positionIterator->second->integerValue64, building);
        else sortedBuildings.emplace(buildingPos++, building);
      } else {
        building->structValue->emplace("METADATA", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
        sortedBuildings.emplace(buildingPos++, building);
      }
    }

    BaseLib::PVariable buildings = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    buildings->arrayValue->reserve(sortedBuildings.size());
    for (auto &building: sortedBuildings) {
      buildings->arrayValue->emplace_back(building.second);
    }
    return buildings;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::removeBuildingPartFromBuildings(uint64_t buildingPartId) {
  try {
    if (buildingPartId == 0) return std::make_shared<BaseLib::Variable>(false);
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, buildingParts FROM buildings", false);

    for (auto &i: *rows) {
      std::vector<std::string> buildingPartStrings = BaseLib::HelperFunctions::splitAll(i.second.at(1)->textValue, ',');
      bool containsBuildingPart = false;

      std::ostringstream buildingPartStream;
      for (auto &buildingPartString: buildingPartStrings) {
        auto buildingPart = (uint64_t)BaseLib::Math::getNumber64(buildingPartString);
        if (buildingPart == buildingPartId) containsBuildingPart = true;
        else buildingPartStream << std::to_string(buildingPart) << ",";
      }

      if (containsBuildingPart) {
        std::string buildingPartString = buildingPartStream.str();
        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingPartString));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(i.second.at(0)->intValue));
        _db.executeCommand("UPDATE buildings SET buildingParts=? WHERE id=?", data, false);
        _db.executeCommand("UPDATE buildings SET buildingParts=? WHERE id=?", data, true);
      }
    }

    return std::make_shared<BaseLib::Variable>(true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::removeBuildingPartFromBuilding(uint64_t buildingId, uint64_t buildingPartId) {
  try {
    if (buildingPartId == 0) return BaseLib::Variable::createError(-2, "Invalid story ID.");
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingId));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT buildingParts FROM buildings WHERE id=?", data, false);
    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown building.");

    std::vector<std::string> buildingPartStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(0)->textValue, ',');
    bool containsBuildingPart = false;

    std::ostringstream buildingPartStream;
    for (auto &buildingPartString: buildingPartStrings) {
      auto buildingPart = (uint64_t)BaseLib::Math::getNumber64(buildingPartString);
      if (buildingPart == buildingPartId) containsBuildingPart = true;
      else buildingPartStream << std::to_string(buildingPart) << ",";
    }

    if (containsBuildingPart) {
      std::string buildingPartString = buildingPartStream.str();
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(buildingPartString));
      _db.executeCommand("UPDATE buildings SET buildingParts=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE buildings SET buildingParts=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::removeStoryFromBuildings(uint64_t storyId) {
  try {
    if (storyId == 0) return std::make_shared<BaseLib::Variable>(false);
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, stories FROM buildings", false);

    for (auto &i: *rows) {
      std::vector<std::string> storyStrings = BaseLib::HelperFunctions::splitAll(i.second.at(1)->textValue, ',');
      bool containsStory = false;

      std::ostringstream storyStream;
      for (auto &storyString: storyStrings) {
        auto story = (uint64_t)BaseLib::Math::getNumber64(storyString);
        if (story == storyId) containsStory = true;
        else storyStream << std::to_string(story) << ",";
      }

      if (containsStory) {
        std::string storyString = storyStream.str();
        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(storyString));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(i.second.at(0)->intValue));
        _db.executeCommand("UPDATE buildings SET stories=? WHERE id=?", data, false);
        _db.executeCommand("UPDATE buildings SET stories=? WHERE id=?", data, true);
      }
    }

    return std::make_shared<BaseLib::Variable>(true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::removeStoryFromBuilding(uint64_t buildingId, uint64_t storyId) {
  try {
    if (storyId == 0) return BaseLib::Variable::createError(-2, "Invalid story ID.");
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingId));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT stories FROM buildings WHERE id=?", data, false);
    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown building.");

    std::vector<std::string> storyStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(0)->textValue, ',');
    bool containsStory = false;

    std::ostringstream storyStream;
    for (auto &storyString: storyStrings) {
      auto story = (uint64_t)BaseLib::Math::getNumber64(storyString);
      if (story == storyId) containsStory = true;
      else storyStream << std::to_string(story) << ",";
    }

    if (containsStory) {
      std::string storyString = storyStream.str();
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(storyString));
      _db.executeCommand("UPDATE buildings SET stories=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE buildings SET stories=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool DatabaseController::buildingExists(uint64_t buildingId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingId));
    return !_db.executeCommand("SELECT id FROM buildings WHERE id=?", data, false)->empty();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

BaseLib::PVariable DatabaseController::setBuildingMetadata(uint64_t buildingId, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingId));
    if (_db.executeCommand("SELECT id FROM buildings WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown building.");

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);

    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
    _db.executeCommand("UPDATE buildings SET metadata=? WHERE id=?", data, false);
    _db.executeCommand("UPDATE buildings SET metadata=? WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::updateBuilding(uint64_t buildingId, BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingId));
    if (_db.executeCommand("SELECT id FROM buildings WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown building.");

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    if (!metadata->structValue->empty()) {
      std::vector<char> metadataBlob;
      _rpcEncoder->encodeResponse(metadata, metadataBlob);
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
      _db.executeCommand("UPDATE buildings SET metadata=?, translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE buildings SET metadata=?, translations=? WHERE id=?", data, true);
    } else {
      _db.executeCommand("UPDATE buildings SET translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE buildings SET translations=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//{{{ Building parts
BaseLib::PVariable DatabaseController::createBuildingPart(BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

    uint64_t result = _db.executeWriteCommand("REPLACE INTO buildingParts VALUES(?, ?, ?)", data, false);
    data.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand("REPLACE INTO buildingParts VALUES(?, ?, ?)", data, true);

    return std::make_shared<BaseLib::Variable>(result);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteBuildingPart(uint64_t buildingPartId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingPartId));
    if (_db.executeCommand("SELECT id FROM buildingParts WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown building part.");

    _db.executeWriteCommand("DELETE FROM buildingParts WHERE id=?", data, false);
    _db.executeWriteCommand("DELETE FROM buildingParts WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getBuildingPartMetadata(uint64_t buildingPartId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingPartId));
    auto rows = _db.executeCommand("SELECT metadata FROM buildingParts WHERE id=?", data, false);

    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown building part.");

    return _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getBuildingParts(BaseLib::PRpcClientInfo clientInfo, std::string languageCode, bool checkAcls) {
  try {
    std::multimap<int32_t, BaseLib::PVariable> sortedBuildingParts;
    int32_t buildingPartPos = 0;

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations, metadata FROM buildingParts", false);
    for (auto &row: *rows) {
      if (checkAcls && !clientInfo->acls->checkBuildingPartReadAccess(row.second.at(0)->intValue)) continue;
      BaseLib::PVariable buildingPart = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      buildingPart->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(row.second.at(0)->intValue));
      BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
      if (languageCode.empty()) buildingPart->structValue->emplace("TRANSLATIONS", translations);
      else {
        auto translationIterator = translations->structValue->find(languageCode);
        if (translationIterator != translations->structValue->end()) buildingPart->structValue->emplace("NAME", translationIterator->second);
        else {
          auto shortLanguageCode = BaseLib::HelperFunctions::splitFirst(languageCode, '-').first;
          translationIterator = translations->structValue->find(shortLanguageCode);
          if (translationIterator != translations->structValue->end()) buildingPart->structValue->emplace("NAME", translationIterator->second);
          else {
            translationIterator = translations->structValue->find("en");
            if (translationIterator != translations->structValue->end()) buildingPart->structValue->emplace("NAME", translationIterator->second);
            else {
              translationIterator = translations->structValue->find("en-US");
              if (translationIterator != translations->structValue->end()) buildingPart->structValue->emplace("NAME", translationIterator->second);
              else if (!translations->structValue->empty()) buildingPart->structValue->emplace("NAME", translations->structValue->begin()->second);
              else buildingPart->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
            }
          }
        }
      }

      if (!row.second.at(2)->binaryValue->empty()) {
        BaseLib::PVariable metadata = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);
        buildingPart->structValue->emplace("METADATA", metadata);

        auto positionIterator = metadata->structValue->find("position");
        if (positionIterator != metadata->structValue->end()) sortedBuildingParts.emplace(positionIterator->second->integerValue64, buildingPart);
        else sortedBuildingParts.emplace(buildingPartPos++, buildingPart);
      } else {
        buildingPart->structValue->emplace("METADATA", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
        sortedBuildingParts.emplace(buildingPartPos++, buildingPart);
      }
    }

    BaseLib::PVariable buildingParts = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    buildingParts->arrayValue->reserve(sortedBuildingParts.size());
    for (auto &buildingPart: sortedBuildingParts) {
      buildingParts->arrayValue->emplace_back(buildingPart.second);
    }
    return buildingParts;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool DatabaseController::buildingPartExists(uint64_t buildingPartId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingPartId));
    return !_db.executeCommand("SELECT id FROM buildingParts WHERE id=?", data, false)->empty();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

BaseLib::PVariable DatabaseController::setBuildingPartMetadata(uint64_t buildingPartId, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingPartId));
    if (_db.executeCommand("SELECT id FROM buildingParts WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown building part.");

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);

    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
    _db.executeCommand("UPDATE buildingParts SET metadata=? WHERE id=?", data, false);
    _db.executeCommand("UPDATE buildingParts SET metadata=? WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::updateBuildingPart(uint64_t buildingPartId, BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(buildingPartId));
    if (_db.executeCommand("SELECT id FROM buildingParts WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown building part.");

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    if (!metadata->structValue->empty()) {
      std::vector<char> metadataBlob;
      _rpcEncoder->encodeResponse(metadata, metadataBlob);
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
      _db.executeCommand("UPDATE buildingParts SET metadata=?, translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE buildingParts SET metadata=?, translations=? WHERE id=?", data, true);
    } else {
      _db.executeCommand("UPDATE buildingParts SET translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE buildingParts SET translations=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//{{{ Stories
BaseLib::PVariable DatabaseController::addRoomToStory(uint64_t storyId, uint64_t roomId) {
  try {
    if (!roomExists(roomId)) return BaseLib::Variable::createError(-2, "Unknown room.");
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(storyId));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT rooms FROM stories WHERE id=?", data, false);
    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown story.");

    std::vector<std::string> roomStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(0)->textValue, ',');
    bool containsRoom = false;

    std::ostringstream roomStream;
    for (auto &roomString: roomStrings) {
      auto room = (uint64_t)BaseLib::Math::getNumber64(roomString);
      if (room == roomId) containsRoom = true;
      roomStream << std::to_string(room) << ",";
    }
    if (!containsRoom) {
      roomStream << std::to_string(roomId) << ",";
      std::string roomString = roomStream.str();
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(roomString));
      _db.executeCommand("UPDATE stories SET rooms=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE stories SET rooms=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::createStory(BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::string()));

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

    uint64_t result = _db.executeWriteCommand("REPLACE INTO stories VALUES(?, ?, ?, ?)", data, false);
    data.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand("REPLACE INTO stories VALUES(?, ?, ?, ?)", data, true);

    return std::make_shared<BaseLib::Variable>(result);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteStory(uint64_t storyId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(storyId));
    if (_db.executeCommand("SELECT id FROM stories WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown story.");

    _db.executeWriteCommand("DELETE FROM stories WHERE id=?", data, false);
    _db.executeWriteCommand("DELETE FROM stories WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getRoomsInStory(BaseLib::PRpcClientInfo clientInfo, uint64_t storyId, bool checkAcls) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(storyId));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT rooms FROM stories WHERE id=?", data, false);
    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown story.");
    std::multimap<int32_t, uint64_t> sortedRooms;
    int32_t pos = 0;
    std::vector<std::string> roomStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(0)->textValue, ',');
    for (auto &roomString: roomStrings) {
      auto room = (uint64_t)BaseLib::Math::getNumber64(roomString);
      if (checkAcls && !clientInfo->acls->checkRoomReadAccess(room)) continue;
      if (room != 0) {
        auto metadata = getRoomMetadata(room);
        auto positionIterator = metadata->structValue->find("position");
        if (positionIterator != metadata->structValue->end()) sortedRooms.emplace(positionIterator->second->integerValue64, room);
        else sortedRooms.emplace(pos++, room);
      }
    }

    auto rooms = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    rooms->arrayValue->reserve(sortedRooms.size());
    for (auto &room: sortedRooms) {
      rooms->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(room.second));
    }
    return rooms;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getStoryMetadata(uint64_t storyId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(storyId));
    auto rows = _db.executeCommand("SELECT metadata FROM stories WHERE id=?", data, false);

    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown story.");

    return _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getStories(std::string languageCode) {
  try {
    std::multimap<int32_t, BaseLib::PVariable> sortedStories;
    int32_t storyPos = 0;

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations, rooms, metadata FROM stories", false);
    for (auto &row: *rows) {
      BaseLib::PVariable story = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      story->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(row.second.at(0)->intValue));
      BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
      if (languageCode.empty()) story->structValue->emplace("TRANSLATIONS", translations);
      else {
        auto translationIterator = translations->structValue->find(languageCode);
        if (translationIterator != translations->structValue->end()) story->structValue->emplace("NAME", translationIterator->second);
        else {
          auto shortLanguageCode = BaseLib::HelperFunctions::splitFirst(languageCode, '-').first;
          translationIterator = translations->structValue->find(shortLanguageCode);
          if (translationIterator != translations->structValue->end()) story->structValue->emplace("NAME", translationIterator->second);
          else {
            translationIterator = translations->structValue->find("en");
            if (translationIterator != translations->structValue->end()) story->structValue->emplace("NAME", translationIterator->second);
            else {
              translationIterator = translations->structValue->find("en-US");
              if (translationIterator != translations->structValue->end()) story->structValue->emplace("NAME", translationIterator->second);
              else if (!translations->structValue->empty()) story->structValue->emplace("NAME", translations->structValue->begin()->second);
              else story->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
            }
          }
        }
      }

      std::multimap<int32_t, uint64_t> sortedRooms;
      int32_t roomPos = 0;
      std::vector<std::string> roomStrings = BaseLib::HelperFunctions::splitAll(row.second.at(2)->textValue, ',');
      for (auto &roomString: roomStrings) {
        if (roomString.empty()) continue;
        auto room = (uint64_t)BaseLib::Math::getNumber64(roomString);
        if (room != 0) {
          auto metadata = getRoomMetadata(room);
          auto positionIterator = metadata->structValue->find("position");
          if (positionIterator != metadata->structValue->end()) sortedRooms.emplace(positionIterator->second->integerValue64, room);
          else sortedRooms.emplace(roomPos++, room);
        }
      }
      BaseLib::PVariable rooms = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      rooms->arrayValue->reserve(sortedRooms.size());
      for (auto &room: sortedRooms) {
        rooms->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(room.second));
      }
      story->structValue->emplace("ROOMS", rooms);

      if (!row.second.at(3)->binaryValue->empty()) {
        BaseLib::PVariable metadata = _rpcDecoder->decodeResponse(*row.second.at(3)->binaryValue);
        story->structValue->emplace("METADATA", metadata);

        auto positionIterator = metadata->structValue->find("position");
        if (positionIterator != metadata->structValue->end()) sortedStories.emplace(positionIterator->second->integerValue64, story);
        else sortedStories.emplace(storyPos++, story);
      } else {
        story->structValue->emplace("METADATA", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
        sortedStories.emplace(storyPos++, story);
      }
    }

    BaseLib::PVariable stories = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    stories->arrayValue->reserve(sortedStories.size());
    for (auto &story: sortedStories) {
      stories->arrayValue->emplace_back(story.second);
    }
    return stories;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::removeRoomFromStories(uint64_t roomId) {
  try {
    if (roomId == 0) return std::make_shared<BaseLib::Variable>(false);
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, rooms FROM stories", false);

    for (auto &i: *rows) {
      std::vector<std::string> roomStrings = BaseLib::HelperFunctions::splitAll(i.second.at(1)->textValue, ',');
      bool containsRoom = false;

      std::ostringstream roomStream;
      for (auto &roomString: roomStrings) {
        auto room = (uint64_t)BaseLib::Math::getNumber64(roomString);
        if (room == roomId) containsRoom = true;
        else roomStream << std::to_string(room) << ",";
      }

      if (containsRoom) {
        std::string roomString = roomStream.str();
        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomString));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(i.second.at(0)->intValue));
        _db.executeCommand("UPDATE stories SET rooms=? WHERE id=?", data, false);
        _db.executeCommand("UPDATE stories SET rooms=? WHERE id=?", data, true);
      }
    }

    return std::make_shared<BaseLib::Variable>(true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::removeRoomFromStory(uint64_t storyId, uint64_t roomId) {
  try {
    if (roomId == 0) return BaseLib::Variable::createError(-2, "Invalid room ID.");
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(storyId));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT rooms FROM stories WHERE id=?", data, false);
    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown story.");

    std::vector<std::string> roomStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(0)->textValue, ',');
    bool containsRoom = false;

    std::ostringstream roomStream;
    for (auto &roomString: roomStrings) {
      auto room = (uint64_t)BaseLib::Math::getNumber64(roomString);
      if (room == roomId) containsRoom = true;
      else roomStream << std::to_string(room) << ",";
    }

    if (containsRoom) {
      std::string roomString = roomStream.str();
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(roomString));
      _db.executeCommand("UPDATE stories SET rooms=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE stories SET rooms=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool DatabaseController::storyExists(uint64_t storyId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(storyId));
    return !_db.executeCommand("SELECT id FROM stories WHERE id=?", data, false)->empty();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

BaseLib::PVariable DatabaseController::setStoryMetadata(uint64_t storyId, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(storyId));
    if (_db.executeCommand("SELECT id FROM stories WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown story.");

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);

    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
    _db.executeCommand("UPDATE stories SET metadata=? WHERE id=?", data, false);
    _db.executeCommand("UPDATE stories SET metadata=? WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::updateStory(uint64_t storyId, BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(storyId));
    if (_db.executeCommand("SELECT id FROM stories WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown story.");

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    if (!metadata->structValue->empty()) {
      std::vector<char> metadataBlob;
      _rpcEncoder->encodeResponse(metadata, metadataBlob);
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
      _db.executeCommand("UPDATE stories SET metadata=?, translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE stories SET metadata=?, translations=? WHERE id=?", data, true);
    } else {
      _db.executeCommand("UPDATE stories SET translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE stories SET translations=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//{{{ Rooms
BaseLib::PVariable DatabaseController::createRoom(BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

    uint64_t result = _db.executeWriteCommand("REPLACE INTO rooms VALUES(?, ?, ?)", data, false);
    data.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand("REPLACE INTO rooms VALUES(?, ?, ?)", data, true);

    return std::make_shared<BaseLib::Variable>(result);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteRoom(uint64_t roomId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
    if (_db.executeCommand("SELECT id FROM rooms WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown room.");

    _db.executeWriteCommand("DELETE FROM rooms WHERE id=?", data, false);
    _db.executeWriteCommand("DELETE FROM rooms WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

std::string DatabaseController::getRoomName(BaseLib::PRpcClientInfo clientInfo, uint64_t roomId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
    auto rows = _db.executeCommand("SELECT translations FROM rooms WHERE id=?", data, false);

    if (rows->empty()) return "";

    auto translations = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
    auto language = clientInfo->language;
    if (language.empty()) language = "en";

    auto translationsIterator = translations->structValue->find(language);
    if (translationsIterator != translations->structValue->end()) return translationsIterator->second->stringValue;
    else {
      auto shortLanguageCode = BaseLib::HelperFunctions::splitFirst(language, '-').first;
      translationsIterator = translations->structValue->find(shortLanguageCode);
      if (translationsIterator != translations->structValue->end()) return translationsIterator->second->stringValue;
      else {
        translationsIterator = translations->structValue->find("en");
        if (translationsIterator != translations->structValue->end()) return translationsIterator->second->stringValue;
        else {
          translationsIterator = translations->structValue->find("en-US");
          if (translationsIterator != translations->structValue->end()) return translationsIterator->second->stringValue;
          else if (!translations->structValue->empty()) return translations->structValue->begin()->second->stringValue;
          else return "";
        }
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return "";
}

BaseLib::PVariable DatabaseController::getRoomMetadata(uint64_t roomId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
    auto rows = _db.executeCommand("SELECT metadata FROM rooms WHERE id=?", data, false);

    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown room.");

    return _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getRooms(BaseLib::PRpcClientInfo clientInfo, std::string languageCode, bool checkAcls) {
  try {
    std::multimap<int32_t, BaseLib::PVariable> sortedRooms;
    int32_t pos = 0;

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations, metadata FROM rooms", false);
    for (auto &row: *rows) {
      if (checkAcls && !clientInfo->acls->checkRoomReadAccess(row.second.at(0)->intValue)) continue;
      BaseLib::PVariable room = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      room->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(row.second.at(0)->intValue));
      BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
      if (languageCode.empty()) room->structValue->emplace("TRANSLATIONS", translations);
      else {
        auto translationIterator = translations->structValue->find(languageCode);
        if (translationIterator != translations->structValue->end()) room->structValue->emplace("NAME", translationIterator->second);
        else {
          auto shortLanguageCode = BaseLib::HelperFunctions::splitFirst(languageCode, '-').first;
          translationIterator = translations->structValue->find(shortLanguageCode);
          if (translationIterator != translations->structValue->end()) room->structValue->emplace("NAME", translationIterator->second);
          else {
            translationIterator = translations->structValue->find("en");
            if (translationIterator != translations->structValue->end()) room->structValue->emplace("NAME", translationIterator->second);
            {
              translationIterator = translations->structValue->find("en-US");
              if (translationIterator != translations->structValue->end()) room->structValue->emplace("NAME", translationIterator->second);
              else if (!translations->structValue->empty()) room->structValue->emplace("NAME", translations->structValue->begin()->second);
              else room->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
            }
          }
        }
      }
      if (!row.second.at(2)->binaryValue->empty()) {
        BaseLib::PVariable metadata = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);
        room->structValue->emplace("METADATA", metadata);

        auto positionIterator = metadata->structValue->find("position");
        if (positionIterator != metadata->structValue->end()) sortedRooms.emplace(positionIterator->second->integerValue64, room);
        else sortedRooms.emplace(pos++, room);
      } else {
        room->structValue->emplace("METADATA", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
        sortedRooms.emplace(pos++, room);
      }
    }

    BaseLib::PVariable rooms = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    rooms->arrayValue->reserve(sortedRooms.size());
    for (auto &room: sortedRooms) {
      rooms->arrayValue->emplace_back(room.second);
    }
    return rooms;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool DatabaseController::roomExists(uint64_t roomId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
    return !_db.executeCommand("SELECT id FROM rooms WHERE id=?", data, false)->empty();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

BaseLib::PVariable DatabaseController::setRoomMetadata(uint64_t roomId, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
    if (_db.executeCommand("SELECT id FROM rooms WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown room.");

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);

    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
    _db.executeCommand("UPDATE rooms SET metadata=? WHERE id=?", data, false);
    _db.executeCommand("UPDATE rooms SET metadata=? WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::updateRoom(uint64_t roomId, BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
    if (_db.executeCommand("SELECT id FROM rooms WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown room.");

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    if (!metadata->structValue->empty()) {
      std::vector<char> metadataBlob;
      _rpcEncoder->encodeResponse(metadata, metadataBlob);
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
      _db.executeCommand("UPDATE rooms SET metadata=?, translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE rooms SET metadata=?, translations=? WHERE id=?", data, true);
    } else {
      _db.executeCommand("UPDATE rooms SET translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE rooms SET translations=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//{{{ Categories
BaseLib::PVariable DatabaseController::createCategory(BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

    uint64_t result = _db.executeWriteCommand("REPLACE INTO categories VALUES(?, ?, ?)", data, false);
    data.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand("REPLACE INTO categories VALUES(?, ?, ?)", data, true);

    return std::make_shared<BaseLib::Variable>(result);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteCategory(uint64_t categoryId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categoryId));
    if (_db.executeCommand("SELECT id FROM categories WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown category.");

    _db.executeWriteCommand("DELETE FROM categories WHERE id=?", data, false);
    _db.executeWriteCommand("DELETE FROM categories WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getCategories(BaseLib::PRpcClientInfo clientInfo, std::string languageCode, bool checkAcls) {
  try {
    std::multimap<int32_t, BaseLib::PVariable> sortedCategories;
    int32_t pos = 0;

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations, metadata FROM categories", false);
    for (auto &row: *rows) {
      if (checkAcls && !clientInfo->acls->checkCategoryReadAccess(row.second.at(0)->intValue)) continue;
      BaseLib::PVariable category = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      category->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(row.second.at(0)->intValue));
      BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
      if (languageCode.empty()) category->structValue->emplace("TRANSLATIONS", translations);
      else {
        auto translationIterator = translations->structValue->find(languageCode);
        if (translationIterator != translations->structValue->end()) category->structValue->emplace("NAME", translationIterator->second);
        else {
          auto shortLanguageCode = BaseLib::HelperFunctions::splitFirst(languageCode, '-').first;
          translationIterator = translations->structValue->find(shortLanguageCode);
          if (translationIterator != translations->structValue->end()) category->structValue->emplace("NAME", translationIterator->second);
          else {
            translationIterator = translations->structValue->find("en");
            if (translationIterator != translations->structValue->end()) category->structValue->emplace("NAME", translationIterator->second);
            {
              translationIterator = translations->structValue->find("en-US");
              if (translationIterator != translations->structValue->end()) category->structValue->emplace("NAME", translationIterator->second);
              else if (!translations->structValue->empty()) category->structValue->emplace("NAME", translations->structValue->begin()->second);
              else category->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
            }
          }
        }
      }
      if (!row.second.at(2)->binaryValue->empty()) {
        BaseLib::PVariable metadata = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);
        category->structValue->emplace("METADATA", metadata);

        auto positionIterator = metadata->structValue->find("position");
        if (positionIterator != metadata->structValue->end()) sortedCategories.emplace(positionIterator->second->integerValue64, category);
        else sortedCategories.emplace(pos++, category);
      } else {
        category->structValue->emplace("METADATA", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
        sortedCategories.emplace(pos++, category);
      }
    }

    BaseLib::PVariable categories = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    categories->arrayValue->reserve(sortedCategories.size());
    for (auto &category: sortedCategories) {
      categories->arrayValue->emplace_back(category.second);
    }
    return categories;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getCategoryMetadata(uint64_t categoryId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categoryId));
    auto rows = _db.executeCommand("SELECT metadata FROM categories WHERE id=?", data, false);

    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown category.");

    return _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool DatabaseController::categoryExists(uint64_t categoryId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categoryId));
    return !_db.executeCommand("SELECT id FROM categories WHERE id=?", data, false)->empty();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

BaseLib::PVariable DatabaseController::setCategoryMetadata(uint64_t categoryId, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categoryId));
    if (_db.executeCommand("SELECT id FROM categories WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown category.");

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);

    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
    _db.executeCommand("UPDATE categories SET metadata=? WHERE id=?", data, false);
    _db.executeCommand("UPDATE categories SET metadata=? WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::updateCategory(uint64_t categoryId, BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categoryId));
    if (_db.executeCommand("SELECT id FROM categories WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown category.");

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    if (!metadata->structValue->empty()) {
      std::vector<char> metadataBlob;
      _rpcEncoder->encodeResponse(metadata, metadataBlob);
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
      _db.executeCommand("UPDATE categories SET metadata=?, translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE categories SET metadata=?, translations=? WHERE id=?", data, true);
    } else {
      _db.executeCommand("UPDATE categories SET translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE categories SET translations=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//{{{ Roles
void DatabaseController::createDefaultRoles() {
  try {
    auto result = _db.executeCommand("SELECT count(*) FROM roles", false);
    if ((!result->empty() && result->begin()->second.begin()->second->intValue == 0) || GD::bl->settings.reloadRolesOnStartup()) {
      std::string defaultRolesFile = GD::bl->settings.dataPath() + "defaultRoles.json";
      if (BaseLib::Io::fileExists(defaultRolesFile)) {
        auto rawRoles = BaseLib::Io::getFileContent(defaultRolesFile);
        if (!BaseLib::HelperFunctions::trim(rawRoles).empty()) {
          BaseLib::Rpc::JsonDecoder jsonDecoder(GD::bl.get());
          BaseLib::PVariable roles;
          try {
            roles = BaseLib::Rpc::JsonDecoder::decode(rawRoles);

            //Make sure, file exists and JSON is valid before deleting old roles
            _db.executeCommand("DELETE FROM roles", false);
            _db.executeCommand("DELETE FROM roles", true);

            for (auto &roleEntry: *roles->arrayValue) {
              auto idIterator = roleEntry->structValue->find("id");
              auto translationsIterator = roleEntry->structValue->find("translations");
              if (idIterator == roleEntry->structValue->end() || translationsIterator == roleEntry->structValue->end()) continue;

              int64_t id = 0;
              if (idIterator->second->type == BaseLib::VariableType::tInteger64 || idIterator->second->type == BaseLib::VariableType::tInteger) {
                id = idIterator->second->integerValue64;
              } else {
                id = BaseLib::Math::getNumber64(BaseLib::HelperFunctions::stringReplace(idIterator->second->stringValue, ".", ""));
              }
              if (id <= 0) continue;

              auto metadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

              auto typesIterator = roleEntry->structValue->find("types");
              if (typesIterator != roleEntry->structValue->end()) {
                auto typesArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
                typesArray->arrayValue->reserve(typesIterator->second->arrayValue->size());
                for (auto &type: *typesIterator->second->arrayValue) {
                  auto typeValue = (int32_t)BaseLib::VariableType::tVoid;
                  if (type->stringValue == "bool") typeValue = (int32_t)BaseLib::VariableType::tBoolean;
                  else if (type->stringValue == "int") typeValue = (int32_t)BaseLib::VariableType::tInteger;
                  else if (type->stringValue == "int64") typeValue = (int32_t)BaseLib::VariableType::tInteger64;
                  else if (type->stringValue == "float" || type->stringValue == "num") typeValue = (int32_t)BaseLib::VariableType::tFloat;
                  else if (type->stringValue == "string" || type->stringValue == "str") typeValue = (int32_t)BaseLib::VariableType::tString;
                  else if (type->stringValue == "bin") typeValue = (int32_t)BaseLib::VariableType::tBinary;
                  else if (type->stringValue == "array") typeValue = (int32_t)BaseLib::VariableType::tArray;
                  else if (type->stringValue == "object") typeValue = (int32_t)BaseLib::VariableType::tStruct;
                  else continue;
                  typesArray->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(typeValue));
                }
                metadata->structValue->emplace("types", typesArray);
              }

              auto aggregationIterator = roleEntry->structValue->find("aggregation");
              if (aggregationIterator != roleEntry->structValue->end()) {
                metadata->structValue->emplace("aggregation", aggregationIterator->second);
              }

              auto addVariablesIterator = roleEntry->structValue->find("addVariables");
              if (addVariablesIterator != roleEntry->structValue->end()) {
                metadata->structValue->emplace("addVariables", addVariablesIterator->second);
              }

              auto uiIterator = roleEntry->structValue->find("ui");
              if (uiIterator != roleEntry->structValue->end()) {
                metadata->structValue->emplace("ui", uiIterator->second);
              }

              auto uiRefIterator = roleEntry->structValue->find("uiRef");
              if (uiRefIterator != roleEntry->structValue->end()) {
                metadata->structValue->emplace("uiRef", uiRefIterator->second);
              }

              auto metadataIterator = roleEntry->structValue->find("metadata");
              if (metadataIterator != roleEntry->structValue->end()) {
                for (auto &metadataEntry: *metadataIterator->second->structValue) {
                  if (metadataEntry.first == "addVariables" || metadataEntry.first == "ui") continue;
                  metadata->structValue->emplace(metadataEntry.first, metadataEntry.second);
                }
              }

              createRoleInternal(id, translationsIterator->second, metadata);
            }
          }
          catch (std::exception &ex) {
            GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
          }
        }
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void DatabaseController::createRoleInternal(uint64_t roleId, const BaseLib::PVariable &translations, const BaseLib::PVariable &metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roleId));

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

    _db.executeWriteCommand("REPLACE INTO roles VALUES(?, ?, ?)", data, false);
    _db.executeWriteCommand("REPLACE INTO roles VALUES(?, ?, ?)", data, true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

BaseLib::PVariable DatabaseController::createRole(BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));

    uint64_t result = _db.executeWriteCommand("REPLACE INTO roles VALUES(?, ?, ?)", data, false);
    data.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand("REPLACE INTO roles VALUES(?, ?, ?)", data, true);

    return std::make_shared<BaseLib::Variable>(result);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteRole(uint64_t roleId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roleId));
    if (_db.executeCommand("SELECT id FROM roles WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown role.");

    _db.executeWriteCommand("DELETE FROM roles WHERE id=?", data, false);
    _db.executeWriteCommand("DELETE FROM roles WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void DatabaseController::deleteAllRoles() {
  try {
    _db.executeCommand("DROP INDEX rolesIndex", false);
    _db.executeCommand("DROP INDEX rolesIndex", true);
    _db.executeCommand("DROP TABLE roles", false);
    _db.executeCommand("DROP TABLE roles", true);

    _db.executeCommand("CREATE TABLE IF NOT EXISTS roles (id INTEGER PRIMARY KEY UNIQUE, translations BLOB, metadata BLOB)", false);
    _db.executeCommand("CREATE TABLE IF NOT EXISTS roles (id INTEGER PRIMARY KEY UNIQUE, translations BLOB, metadata BLOB)", true);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS rolesIndex ON roles (id)", false);
    _db.executeCommand("CREATE INDEX IF NOT EXISTS rolesIndex ON roles (id)", true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

BaseLib::PVariable DatabaseController::getRoles(BaseLib::PRpcClientInfo clientInfo, std::string languageCode, bool checkAcls) {
  try {
    std::multimap<int32_t, BaseLib::PVariable> sortedRoles;
    int32_t pos = 0;

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations, metadata FROM roles", false);
    for (auto &row: *rows) {
      if (checkAcls && !clientInfo->acls->checkRoleReadAccess(row.second.at(0)->intValue)) continue;
      BaseLib::PVariable role = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      role->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(row.second.at(0)->intValue));
      BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
      if (languageCode.empty()) role->structValue->emplace("TRANSLATIONS", translations);
      else {
        auto translationIterator = translations->structValue->find(languageCode);
        if (translationIterator != translations->structValue->end()) role->structValue->emplace("NAME", translationIterator->second);
        else {
          auto shortLanguageCode = BaseLib::HelperFunctions::splitFirst(languageCode, '-').first;
          translationIterator = translations->structValue->find(shortLanguageCode);
          if (translationIterator != translations->structValue->end()) role->structValue->emplace("NAME", translationIterator->second);
          else {
            translationIterator = translations->structValue->find("en");
            if (translationIterator != translations->structValue->end()) role->structValue->emplace("NAME", translationIterator->second);
            {
              translationIterator = translations->structValue->find("en-US");
              if (translationIterator != translations->structValue->end()) role->structValue->emplace("NAME", translationIterator->second);
              else if (!translations->structValue->empty()) role->structValue->emplace("NAME", translations->structValue->begin()->second);
              else role->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
            }
          }
        }
      }
      if (!row.second.at(2)->binaryValue->empty()) {
        BaseLib::PVariable metadata = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);
        role->structValue->emplace("METADATA", metadata);

        auto positionIterator = metadata->structValue->find("position");
        if (positionIterator != metadata->structValue->end()) sortedRoles.emplace(positionIterator->second->integerValue64, role);
        else sortedRoles.emplace(pos++, role);
      } else {
        role->structValue->emplace("METADATA", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
        sortedRoles.emplace(pos++, role);
      }
    }

    BaseLib::PVariable roles = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    roles->arrayValue->reserve(sortedRoles.size());
    for (auto &role: sortedRoles) {
      roles->arrayValue->emplace_back(role.second);
    }
    return roles;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getRoleMetadata(uint64_t roleId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roleId));
    auto rows = _db.executeCommand("SELECT metadata FROM roles WHERE id=?", data, false);

    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown role.");

    return _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool DatabaseController::roleExists(uint64_t roleId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roleId));
    return !_db.executeCommand("SELECT id FROM roles WHERE id=?", data, false)->empty();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

BaseLib::PVariable DatabaseController::setRoleMetadata(uint64_t roleId, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roleId));
    if (_db.executeCommand("SELECT id FROM roles WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown role.");

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);

    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
    _db.executeCommand("UPDATE roles SET metadata=? WHERE id=?", data, false);
    _db.executeCommand("UPDATE roles SET metadata=? WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::updateRole(uint64_t roleId, BaseLib::PVariable translations, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roleId));
    if (_db.executeCommand("SELECT id FROM roles WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown role.");

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    if (!metadata->structValue->empty()) {
      std::vector<char> metadataBlob;
      _rpcEncoder->encodeResponse(metadata, metadataBlob);
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
      _db.executeCommand("UPDATE roles SET metadata=?, translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE roles SET metadata=?, translations=? WHERE id=?", data, true);
    } else {
      _db.executeCommand("UPDATE roles SET translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE roles SET translations=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//Node data
std::set<std::string> DatabaseController::getAllNodeDataNodes() {
  try {
    std::set<std::string> nodeIds;
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT node FROM nodeData", false);

    for (auto &row: *rows) {
      nodeIds.emplace(row.second.at(0)->textValue);
    }

    return nodeIds;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::set<std::string>();
}

BaseLib::PVariable DatabaseController::getNodeData(const std::string &node, const std::string &key, bool requestFromTrustedServer) {
  try {
    BaseLib::PVariable value;

    bool obfuscate = false;

    //Only return passwords if request comes from FlowsServer
    std::string lowerCharKey = key;
    BaseLib::HelperFunctions::toLower(lowerCharKey);
    if (!requestFromTrustedServer && lowerCharKey.size() >= 8 && (lowerCharKey.compare(lowerCharKey.size() - 8, 8, "password") == 0 || (lowerCharKey.size() >= 11 && lowerCharKey.compare(lowerCharKey.size() - 11, 11, "private_key") == 0)))
      obfuscate = true;

    if (!key.empty()) {
      std::lock_guard<std::mutex> dataGuard(_nodeDataMutex);
      auto componentIterator = _nodeData.find(node);
      if (componentIterator != _nodeData.end()) {
        auto keyIterator = componentIterator->second.find(key);
        if (keyIterator != componentIterator->second.end()) {
          value = keyIterator->second;
          if (obfuscate) value = value->stringValue.empty() ? std::make_shared<BaseLib::Variable>(std::string()) : std::make_shared<BaseLib::Variable>(std::string("*"));
          return value;
        }
      }
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(node));
    std::string command;
    if (!key.empty()) {
      command = "SELECT value FROM nodeData WHERE node=? AND key=?";
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(key));
    } else command = "SELECT key, value FROM nodeData WHERE node=?";

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand(command, data, false);
    if (rows->empty() || rows->at(0).empty()) return std::make_shared<BaseLib::Variable>();

    if (key.empty()) {
      value = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      for (auto &row: *rows) {
        std::string innerKey = row.second.at(0)->textValue;
        lowerCharKey = innerKey;
        BaseLib::HelperFunctions::toLower(lowerCharKey);
        BaseLib::PVariable innerValue;
        //Only return passwords if request comes from FlowsServer
        innerValue = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
        if (!requestFromTrustedServer && lowerCharKey.size() >= 8 && (lowerCharKey.compare(lowerCharKey.size() - 8, 8, "password") == 0 || (lowerCharKey.size() >= 11 && lowerCharKey.compare(lowerCharKey.size() - 11, 11, "private_key") == 0))) {
          innerValue = innerValue->stringValue.empty() ? std::make_shared<BaseLib::Variable>(std::string()) : std::make_shared<BaseLib::Variable>(std::string("*"));
        }
        value->structValue->emplace(innerKey, innerValue);
      }
    } else {
      value = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
      std::lock_guard<std::mutex> dataGuard(_nodeDataMutex);
      _nodeData[node][key] = value;
    }

    if (obfuscate) value = value->stringValue.empty() ? std::make_shared<BaseLib::Variable>(std::string()) : std::make_shared<BaseLib::Variable>(std::string("*"));
    return value;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::setNodeData(const std::string &node, const std::string &key, const BaseLib::PVariable &value) {
  try {
    if (!value) return BaseLib::Variable::createError(-32602, "Could not parse data.");
    if (node.empty()) return BaseLib::Variable::createError(-32602, "component is an empty string.");
    if (key.empty()) return BaseLib::Variable::createError(-32602, "key is an empty string.");
    if (node.size() > 250) return BaseLib::Variable::createError(-32602, "node ID has more than 250 characters.");
    if (key.size() > 250) return BaseLib::Variable::createError(-32602, "key has more than 250 characters.");
    //Don't check for type here, so base64, string and future data types that use stringValue are handled
    if (value->type != BaseLib::VariableType::tBase64 && value->type != BaseLib::VariableType::tString && value->type != BaseLib::VariableType::tInteger && value->type != BaseLib::VariableType::tInteger64
        && value->type != BaseLib::VariableType::tFloat && value->type != BaseLib::VariableType::tBoolean && value->type != BaseLib::VariableType::tStruct && value->type != BaseLib::VariableType::tArray)
      return BaseLib::Variable::createError(-32602, "Type " + BaseLib::Variable::getTypeString(value->type) + " is currently not supported.");

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT COUNT(*) FROM nodeData", false);
    if (rows->empty() || rows->at(0).empty()) {
      return BaseLib::Variable::createError(-32500, "Error counting data in database.");
    }
    if (rows->at(0).at(0)->intValue > 1000000) {
      return BaseLib::Variable::createError(-32500, "Reached limit of 1000000 data entries. Please delete data before adding new entries.");
    }

    {
      std::lock_guard<std::mutex> dataGuard(_nodeDataMutex);
      _nodeData[node][key] = value;
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(node));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(key));
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM nodeData WHERE node=? AND key=?", data);
    enqueue(0, entry);

    std::vector<char> encodedValue;
    _rpcEncoder->encodeResponse(value, encodedValue);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(encodedValue));
    entry = std::make_shared<QueueEntry>("INSERT INTO nodeData VALUES(?, ?, ?)", data);
    enqueue(0, entry);

    return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tVoid);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteNodeData(const std::string &node, const std::string &key) {
  try {
    {
      std::lock_guard<std::mutex> dataGuard(_nodeDataMutex);
      if (key.empty()) _nodeData.erase(node);
      else {
        auto dataIterator = _nodeData.find(node);
        if (dataIterator != _nodeData.end()) dataIterator->second.erase(key);
      }
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(node));
    std::string command("DELETE FROM nodeData WHERE node=?");
    if (!key.empty()) {
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(key));
      command.append(" AND key=?");
    }
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(command, data);
    enqueue(0, entry);

    return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tVoid);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//End node data

//Metadata
BaseLib::PVariable DatabaseController::getAllMetadata(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<BaseLib::Systems::Peer> peer, bool checkAcls) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::to_string(peer->getID())));

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT dataID, serializedObject FROM metadata WHERE objectID=?", data, false);

    BaseLib::PVariable metadataStruct(new BaseLib::Variable(BaseLib::VariableType::tStruct));
    for (auto &i: *rows) {
      if (i.second.size() < 2) continue;
      if (checkAcls && !clientInfo->acls->checkVariableReadAccess(peer, -2, i.second.at(0)->textValue)) continue;
      BaseLib::PVariable metadata = _rpcDecoder->decodeResponse(*i.second.at(1)->binaryValue);
      metadataStruct->structValue->insert(BaseLib::StructElement(i.second.at(0)->textValue, metadata));
    }

    return metadataStruct;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getMetadata(uint64_t peerID, std::string &dataID) {
  try {
    if (dataID.size() > 250) return BaseLib::Variable::createError(-32602, "dataID has more than 250 characters.");

    BaseLib::PVariable metadata;

    {
      std::lock_guard<std::mutex> metadataGuard(_metadataMutex);
      auto peerIterator = _metadata.find(peerID);
      if (peerIterator != _metadata.end()) {
        auto dataIterator = peerIterator->second.find(dataID);
        if (dataIterator != peerIterator->second.end()) {
          metadata = dataIterator->second;
          return metadata;
        }
      }
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::to_string(peerID)));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(dataID));

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT serializedObject FROM metadata WHERE objectID=? AND dataID=?", data, false);
    if (rows->empty() || rows->at(0).empty()) return std::make_shared<BaseLib::Variable>();

    metadata = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
    std::lock_guard<std::mutex> metadataGuard(_metadataMutex);
    _metadata[peerID][dataID] = metadata;
    return metadata;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::setMetadata(BaseLib::PRpcClientInfo clientInfo, uint64_t peerID, std::string &serialNumber, std::string &dataID, BaseLib::PVariable &metadata) {
  try {
    if (!metadata) return BaseLib::Variable::createError(-32602, "Could not parse data.");
    if (dataID.empty()) return BaseLib::Variable::createError(-32602, "dataID is an empty string.");
    if (dataID.size() > 250) return BaseLib::Variable::createError(-32602, "dataID has more than 250 characters.");
    //Don't check for type here, so base64, string and future data types that use stringValue are handled
    if (metadata->type != BaseLib::VariableType::tBase64 && metadata->type != BaseLib::VariableType::tString && metadata->type != BaseLib::VariableType::tInteger && metadata->type != BaseLib::VariableType::tInteger64
        && metadata->type != BaseLib::VariableType::tFloat && metadata->type != BaseLib::VariableType::tBoolean && metadata->type != BaseLib::VariableType::tStruct && metadata->type != BaseLib::VariableType::tArray)
      return BaseLib::Variable::createError(-32602, "Type " + BaseLib::Variable::getTypeString(metadata->type) + " is currently not supported.");

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT COUNT(*) FROM metadata", false);
    if (rows->empty() || rows->at(0).empty()) {
      return BaseLib::Variable::createError(-32500, "Error counting metadata in database.");
    }
    if (rows->at(0).at(0)->intValue > 1000000) {
      return BaseLib::Variable::createError(-32500, "Reached limit of 1000000 metadata entries. Please delete metadata before adding new entries.");
    }

    {
      std::lock_guard<std::mutex> metadataGuard(_metadataMutex);
      _metadata[peerID][dataID] = metadata;
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::to_string(peerID)));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(dataID));
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM metadata WHERE objectID=? AND dataID=?", data);
    enqueue(0, entry);

    std::vector<char> value;
    _rpcEncoder->encodeResponse(metadata, value);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(value));

    entry = std::make_shared<QueueEntry>("INSERT INTO metadata VALUES(?, ?, ?)", data);
    enqueue(0, entry);

    std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{dataID});
    std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>{metadata});
    std::string &source = clientInfo->initInterfaceId;
    if (GD::nodeBlueServer) GD::nodeBlueServer->broadcastEvent(source, peerID, -1, valueKeys, values);
#ifndef NO_SCRIPTENGINE
    GD::scriptEngineServer->broadcastEvent(source, peerID, -1, valueKeys, values);
#endif
    if (GD::ipcServer) GD::ipcServer->broadcastEvent(source, peerID, -1, valueKeys, values);
    GD::rpcClient->broadcastEvent(source, peerID, -1, serialNumber, valueKeys, values);

    return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tVoid);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteMetadata(uint64_t peerID, std::string &serialNumber, std::string &dataID) {
  try {
    if (dataID.size() > 250) return BaseLib::Variable::createError(-32602, "dataID has more than 250 characters.");

    {
      std::lock_guard<std::mutex> metadataGuard(_metadataMutex);
      if (dataID.empty()) {
        if (_metadata.find(peerID) != _metadata.end()) _metadata.erase(peerID);
      } else {
        if (_metadata.find(peerID) != _metadata.end() && _metadata[peerID].find(dataID) != _metadata[peerID].end()) _metadata[peerID].erase(dataID);
      }
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::to_string(peerID)));
    std::string command("DELETE FROM metadata WHERE objectID=?");
    if (!dataID.empty()) {
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(dataID));
      command.append(" AND dataID=?");
    }
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(command, data);
    enqueue(0, entry);

    std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{dataID});
    std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>());
    BaseLib::PVariable value(new BaseLib::Variable(BaseLib::VariableType::tStruct));
    value->structValue->insert(BaseLib::StructElement("TYPE", std::make_shared<BaseLib::Variable>(1)));
    value->structValue->insert(BaseLib::StructElement("CODE", std::make_shared<BaseLib::Variable>(1)));
    values->push_back(value);
    std::string source;
    if (GD::nodeBlueServer) GD::nodeBlueServer->broadcastEvent(source, peerID, -1, valueKeys, values);
#ifndef NO_SCRIPTENGINE
    GD::scriptEngineServer->broadcastEvent(source, peerID, -1, valueKeys, values);
#endif
    if (GD::ipcServer) GD::ipcServer->broadcastEvent(source, peerID, -1, valueKeys, values);
    GD::rpcClient->broadcastEvent(source, peerID, -1, serialNumber, valueKeys, values);

    return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tVoid);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//End metadata

//System variables
void DatabaseController::deleteSystemVariable(std::string &variableId) {
  try {
    if (variableId.size() > 250) return;

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(variableId));
    std::string command("DELETE FROM systemVariables WHERE variableID=?");
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(command, data);
    enqueue(0, entry);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getAllSystemVariables() {
  try {
    return _db.executeCommand("SELECT variableID, serializedObject, room, categories, roles, flags FROM systemVariables", false);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getSystemVariable(const std::string &variableId) {
  try {
    if (variableId.size() > 250) return std::shared_ptr<BaseLib::Database::DataTable>();

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(variableId));
    return _db.executeCommand("SELECT serializedObject, room, categories, roles, flags FROM systemVariables WHERE variableID=?", data, false);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::removeCategoryFromSystemVariables(uint64_t categoryId) {
  try {
    if (categoryId == 0) return;
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT variableID, categories FROM systemVariables", false);

    for (auto &i: *rows) {
      std::vector<std::string> categoryStrings = BaseLib::HelperFunctions::splitAll(i.second.at(1)->textValue, ',');
      bool containsCategory = false;

      std::ostringstream categoryStream;
      for (auto &categoryString: categoryStrings) {
        uint64_t category = BaseLib::Math::getUnsignedNumber64(categoryString);
        if (category == categoryId) containsCategory = true;
        else categoryStream << std::to_string(category) << ",";
      }

      if (containsCategory) {
        std::string categoryString = categoryStream.str();
        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categoryString));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(i.second.at(0)->intValue));
        _db.executeCommand("UPDATE systemVariables SET categories=? WHERE variableID=?", data, false);
        _db.executeCommand("UPDATE systemVariables SET categories=? WHERE variableID=?", data, true);
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void DatabaseController::removeRoleFromSystemVariables(uint64_t roleId) {
  try {
    if (roleId == 0) return;
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT variableID, roles FROM systemVariables", false);

    for (auto &i: *rows) {
      std::vector<std::string> roleStrings = BaseLib::HelperFunctions::splitAll(i.second.at(1)->textValue, ',');
      bool containsRole = false;

      std::ostringstream roleStream;
      for (auto &roleString: roleStrings) {
        uint64_t role = BaseLib::Math::getUnsignedNumber64(roleString);
        if (role == roleId) containsRole = true;
        else roleStream << std::to_string(role) << ",";
      }

      if (containsRole) {
        std::string roleString = roleStream.str();
        BaseLib::Database::DataRow data;
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roleString));
        data.push_back(std::make_shared<BaseLib::Database::DataColumn>(i.second.at(0)->intValue));
        _db.executeCommand("UPDATE systemVariables SET roles=? WHERE variableID=?", data, false);
        _db.executeCommand("UPDATE systemVariables SET roles=? WHERE variableID=?", data, true);
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void DatabaseController::removeRoomFromSystemVariables(uint64_t roomId) {
  try {
    if (roomId == 0) return;
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
    _db.executeCommand("UPDATE systemVariables SET room=0 WHERE room=?", data, false);
    _db.executeCommand("UPDATE systemVariables SET room=0 WHERE room=?", data, true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getSystemVariablesInRoom(uint64_t roomId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
    return _db.executeCommand("SELECT variableID, serializedObject, categories, roles, flags FROM systemVariables WHERE room=?", data, false);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

BaseLib::PVariable DatabaseController::setSystemVariable(std::string &variableId, BaseLib::PVariable &value, uint64_t roomId, const std::string &categories, const std::string &roles, int32_t flags) {
  try {
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT COUNT(*) FROM systemVariables", false);
    if (rows->empty() || rows->at(0).empty()) {
      return BaseLib::Variable::createError(-32500, "Error counting system variables in database.");
    }
    if (rows->at(0).at(0)->intValue > 1000000) {
      return BaseLib::Variable::createError(-32500, "Reached limit of 1000000 system variable entries. Please delete system variables before adding new ones.");
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(variableId));
    std::vector<char> encodedValue;
    _rpcEncoder->encodeResponse(value, encodedValue);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(encodedValue));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categories));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roles));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(flags));

    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT OR REPLACE INTO systemVariables(variableID, serializedObject, room, categories, roles, flags) VALUES(?, ?, ?, ?, ?, ?)", data);
    enqueue(0, entry);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::setSystemVariableCategories(std::string &variableId, const std::string &categories) {
  try {
    if (variableId.empty()) return BaseLib::Variable::createError(-32602, "variableId is an empty string.");
    if (variableId.size() > 250) return BaseLib::Variable::createError(-32602, "variableId has more than 250 characters.");

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(categories));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(variableId));
    _db.executeCommand("UPDATE systemVariables SET categories=? WHERE variableID=?", data, false);
    _db.executeCommand("UPDATE systemVariables SET categories=? WHERE variableID=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::setSystemVariableRoles(std::string &variableId, const std::string &roles) {
  try {
    if (variableId.empty()) return BaseLib::Variable::createError(-32602, "variableId is an empty string.");
    if (variableId.size() > 250) return BaseLib::Variable::createError(-32602, "variableId has more than 250 characters.");

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roles));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(variableId));
    _db.executeCommand("UPDATE systemVariables SET roles=? WHERE variableID=?", data, false);
    _db.executeCommand("UPDATE systemVariables SET roles=? WHERE variableID=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::setSystemVariableRoom(std::string &variableId, uint64_t roomId) {
  try {
    if (variableId.empty()) return BaseLib::Variable::createError(-32602, "variableId is an empty string.");
    if (variableId.size() > 250) return BaseLib::Variable::createError(-32602, "variableId has more than 250 characters.");

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(roomId));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(variableId));
    _db.executeCommand("UPDATE systemVariables SET room=? WHERE variableID=?", data, false);
    _db.executeCommand("UPDATE systemVariables SET room=? WHERE variableID=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//End system variables

//{{{ Users
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getUsers() {
  try {
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT userID, name, groups, metadata FROM users", false);
    return rows;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

bool DatabaseController::createUser(const std::string &name, const std::vector<uint8_t> &passwordHash, const std::vector<uint8_t> &salt, const std::vector<uint64_t> &groups) {
  try {
    if (groups.empty()) {
      GD::out.printError("Error: Could not create user. Groups are not specified.");
      return false;
    }

    if (name == "rpcserver" || name == "gateway-client") {
      GD::out.printError("Error: Could not create user. User name is reserved.");
      return false;
    }

    BaseLib::PVariable groupArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    groupArray->arrayValue->reserve(groups.size());
    for (auto &groupId: groups) {
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
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(0));
    _db.executeCommand("INSERT INTO users VALUES(NULL, ?, ?, ?, ?, ?, ?, ?)", data, false);
    _db.executeCommand("INSERT INTO users VALUES(NULL, ?, ?, ?, ?, ?, ?, ?)", data, true);
    if (userNameExists(name)) return true;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

bool DatabaseController::updateUser(uint64_t id, const std::vector<uint8_t> &passwordHash, const std::vector<uint8_t> &salt, const std::vector<uint64_t> &groups) {
  try {
    BaseLib::PVariable groupArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    groupArray->arrayValue->reserve(groups.size());
    for (auto &groupId: groups) {
      groupArray->arrayValue->push_back(std::make_shared<BaseLib::Variable>(groupId));
    }
    std::vector<char> groupBlob;
    _rpcEncoder->encodeResponse(groupArray, groupBlob);

    std::shared_ptr<BaseLib::Database::DataTable> rows;
    if (!passwordHash.empty() && !groups.empty()) {
      if (salt.empty()) return false;
      BaseLib::Database::DataRow data;
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(passwordHash));
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(salt));
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupBlob));
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(id));
      _db.executeCommand("UPDATE users SET password=?, salt=?, groups=? WHERE userID=?", data, false);
      _db.executeCommand("UPDATE users SET password=?, salt=?, groups=? WHERE userID=?", data, true);

      rows = _db.executeCommand("SELECT userID FROM users WHERE password=? AND salt=? AND groups=? AND userID=?", data, false);
    } else if (!passwordHash.empty()) {
      if (salt.empty()) return false;
      BaseLib::Database::DataRow data;
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(passwordHash));
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(salt));
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(id));
      _db.executeCommand("UPDATE users SET password=?, salt=? WHERE userID=?", data, false);
      _db.executeCommand("UPDATE users SET password=?, salt=? WHERE userID=?", data, true);

      rows = _db.executeCommand("SELECT userID FROM users WHERE password=? AND salt=? AND userID=?", data, false);
    } else if (!groups.empty()) {
      BaseLib::Database::DataRow data;
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupBlob));
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(id));
      _db.executeCommand("UPDATE users SET groups=? WHERE userID=?", data, false);
      _db.executeCommand("UPDATE users SET groups=? WHERE userID=?", data, true);

      rows = _db.executeCommand("SELECT userID FROM users WHERE groups=? AND userID=?", data, false);
    } else return true; //Nothing to update

    return !rows->empty();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

bool DatabaseController::deleteUser(uint64_t id) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(id));
    _db.executeCommand("DELETE FROM userData WHERE userID=?", data, false);
    _db.executeCommand("DELETE FROM userData WHERE userID=?", data, true);
    _db.executeCommand("DELETE FROM users WHERE userID=?", data, false);
    _db.executeCommand("DELETE FROM users WHERE userID=?", data, true);

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT userID FROM users WHERE userID=?", data, false);
    return rows->empty();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

bool DatabaseController::userNameExists(const std::string &name) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(name));
    return !_db.executeCommand("SELECT userID FROM users WHERE name=?", data, false)->empty();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

uint64_t DatabaseController::getUserId(const std::string &name) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(name));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT userID FROM users WHERE name=?", data, false);
    if (result->empty()) return 0;
    return (uint64_t)result->at(0).at(0)->intValue;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}

int64_t DatabaseController::getUserKeyIndex1(uint64_t userId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT keyIndex1 FROM users WHERE userID=?", data, false);
    if (result->empty()) return 0;
    return result->at(0).at(0)->intValue;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}

int64_t DatabaseController::getUserKeyIndex2(uint64_t userId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT keyIndex2 FROM users WHERE userID=?", data, false);
    if (result->empty()) return 0;
    return result->at(0).at(0)->intValue;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}

std::vector<uint64_t> DatabaseController::getUsersGroups(uint64_t userId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT groups FROM users WHERE userID=?", data, false);
    if (result->empty()) return std::vector<uint64_t>();
    auto decodedData = _rpcDecoder->decodeResponse(*result->at(0).at(0)->binaryValue);
    std::vector<uint64_t> groups;
    groups.reserve(decodedData->arrayValue->size());
    for (auto &group: *decodedData->arrayValue) {
      groups.push_back(group->integerValue64);
    }
    return groups;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::vector<uint64_t>();
}

BaseLib::PVariable DatabaseController::getUserMetadata(uint64_t userId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
    auto rows = _db.executeCommand("SELECT metadata FROM users WHERE userID=?", data, false);

    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown user.");

    return _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getPassword(const std::string &name) {
  try {
    BaseLib::Database::DataRow dataSelect;
    dataSelect.push_back(std::make_shared<BaseLib::Database::DataColumn>(name));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT password, salt FROM users WHERE name=?", dataSelect, false);
    return rows;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::setUserKeyIndex1(uint64_t userId, int64_t keyIndex) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
    if (_db.executeCommand("SELECT userID FROM users WHERE userID=?", data, false)->empty()) return;

    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(keyIndex));
    _db.executeCommand("UPDATE users SET keyIndex1=? WHERE userID=?", data, false);
    _db.executeCommand("UPDATE users SET keyIndex1=? WHERE userID=?", data, true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::setUserKeyIndex2(uint64_t userId, int64_t keyIndex) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
    if (_db.executeCommand("SELECT userID FROM users WHERE userID=?", data, false)->empty()) return;

    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(keyIndex));
    _db.executeCommand("UPDATE users SET keyIndex2=? WHERE userID=?", data, false);
    _db.executeCommand("UPDATE users SET keyIndex2=? WHERE userID=?", data, true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

BaseLib::PVariable DatabaseController::setUserMetadata(uint64_t userId, BaseLib::PVariable metadata) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
    if (_db.executeCommand("SELECT userID FROM users WHERE userID=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown user.");

    std::vector<char> metadataBlob;
    _rpcEncoder->encodeResponse(metadata, metadataBlob);

    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(metadataBlob));
    _db.executeCommand("UPDATE users SET metadata=? WHERE userID=?", data, false);
    _db.executeCommand("UPDATE users SET metadata=? WHERE userID=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//{{{ User data
BaseLib::PVariable DatabaseController::deleteUserData(uint64_t userId, const std::string &component, const std::string &key) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
    if (_db.executeCommand("SELECT userID FROM users WHERE userID=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown user.");

    {
      std::lock_guard<std::mutex> dataGuard(_dataMutex);
      if (key.empty()) _data.erase(component);
      else {
        auto dataIterator = _data.find(component);
        if (dataIterator != _data.end()) dataIterator->second.erase(key);
      }
    }

    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(component));
    std::string command("DELETE FROM userData WHERE userID=? AND component=?");
    if (!key.empty()) {
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(key));
      command.append(" AND key=?");
    }
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(command, data);
    enqueue(0, entry);

    return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tVoid);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getUserData(uint64_t userId, const std::string &component, const std::string &key) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
    if (_db.executeCommand("SELECT userID FROM users WHERE userID=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown user.");

    BaseLib::PVariable value;

    if (!key.empty()) {
      std::lock_guard<std::mutex> dataGuard(_dataMutex);
      auto componentIterator = _data.find(component);
      if (componentIterator != _data.end()) {
        auto keyIterator = componentIterator->second.find(key);
        if (keyIterator != componentIterator->second.end()) {
          value = keyIterator->second;
          return value;
        }
      }
    }

    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(component));
    std::string command;
    if (!key.empty()) {
      command = "SELECT value FROM userData WHERE userID=? AND component=? AND key=?";
      data.push_back(std::make_shared<BaseLib::Database::DataColumn>(key));
    } else command = "SELECT key, value FROM userData WHERE userID=? AND component=?";

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand(command, data, false);
    if (rows->empty() || rows->at(0).empty()) return std::make_shared<BaseLib::Variable>();

    if (key.empty()) {
      value = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      for (auto &row: *rows) {
        value->structValue->emplace(row.second.at(0)->textValue, _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue));
      }
    } else {
      value = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
      std::lock_guard<std::mutex> dataGuard(_dataMutex);
      _data[component][key] = value;
    }

    return value;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::setUserData(uint64_t userId, const std::string &component, const std::string &key, const BaseLib::PVariable &value) {
  try {
    if (!value) return BaseLib::Variable::createError(-32602, "Could not parse data.");
    if (component.empty()) return BaseLib::Variable::createError(-32602, "component is an empty string.");
    if (key.empty()) return BaseLib::Variable::createError(-32602, "key is an empty string.");
    if (component.size() > 250) return BaseLib::Variable::createError(-32602, "component has more than 250 characters.");
    if (key.size() > 250) return BaseLib::Variable::createError(-32602, "key has more than 250 characters.");
    //Don't check for type here, so base64, string and future data types that use stringValue are handled
    if (value->type != BaseLib::VariableType::tBase64 && value->type != BaseLib::VariableType::tString && value->type != BaseLib::VariableType::tInteger && value->type != BaseLib::VariableType::tInteger64
        && value->type != BaseLib::VariableType::tFloat && value->type != BaseLib::VariableType::tBoolean && value->type != BaseLib::VariableType::tStruct && value->type != BaseLib::VariableType::tArray)
      return BaseLib::Variable::createError(-32602, "Type " + BaseLib::Variable::getTypeString(value->type) + " is currently not supported.");

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(userId));
    if (_db.executeCommand("SELECT userID FROM users WHERE userID=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown user.");

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT COUNT(*) FROM userData", false);
    if (rows->empty() || rows->at(0).empty()) {
      return BaseLib::Variable::createError(-32500, "Error counting data in database.");
    }
    if (rows->at(0).at(0)->intValue > 1000000) {
      return BaseLib::Variable::createError(-32500, "Reached limit of 1000000 data entries. Please delete data before adding new entries.");
    }

    {
      std::lock_guard<std::mutex> dataGuard(_dataMutex);
      _data[component][key] = value;
    }

    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(component));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(key));
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM userData WHERE userID=? AND component=? AND key=?", data);
    enqueue(0, entry);

    std::vector<char> encodedValue;
    _rpcEncoder->encodeResponse(value, encodedValue);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(encodedValue));
    entry = std::make_shared<QueueEntry>("INSERT INTO userData VALUES(?, ?, ?, ?)", data);
    enqueue(0, entry);

    return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tVoid);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//}}}

//Groups
BaseLib::PVariable DatabaseController::createGroup(BaseLib::PVariable translations, BaseLib::PVariable aclStruct) {
  try {
    std::shared_ptr<BaseLib::Database::DataTable> idResult = _db.executeCommand("SELECT id FROM groups ORDER BY id DESC LIMIT 1", false);
    if (idResult->empty()) {
      GD::out.printError("Error: Could not retrieve largest group ID.");
      return BaseLib::Variable::createError(-32500, "Unknown application error.");
    }
    uint64_t largestId = (uint64_t)idResult->at(0).at(0)->intValue;

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);

    try {
      BaseLib::Security::Acl acl;
      acl.fromVariable(aclStruct);
    }
    catch (BaseLib::Security::AclException &ex) {
      return BaseLib::Variable::createError(-1, "Error in ACL: " + std::string(ex.what()));
    }

    std::vector<char> aclBlob;
    _rpcEncoder->encodeResponse(aclStruct, aclBlob);

    BaseLib::Database::DataRow data;
    if (largestId < 100) data.push_back(std::make_shared<BaseLib::Database::DataColumn>(100));
    else data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
    uint64_t result = _db.executeWriteCommand("REPLACE INTO groups VALUES(?, ?, ?)", data, false);
    if (data.at(0)->dataType != BaseLib::Database::DataColumn::DataType::Enum::INTEGER) data.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand("REPLACE INTO groups VALUES(?, ?, ?)", data, true);

    return std::make_shared<BaseLib::Variable>(result);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::deleteGroup(uint64_t groupId) {
  try {
    if (groupId < 100) return BaseLib::Variable::createError(-1, "Can't delete system group.");

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupId));
    if (_db.executeCommand("SELECT id FROM groups WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown group.");

    _db.executeWriteCommand("DELETE FROM groups WHERE id=?", data, false);
    _db.executeWriteCommand("DELETE FROM groups WHERE id=?", data, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getAcl(uint64_t groupId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupId));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT acl FROM groups WHERE id=?", data, false);
    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown group ID.");

    auto acls = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);
    if (acls->structValue->empty()) return BaseLib::Variable::createError(-1, "Group has no ACLs.");

    return acls;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getGroup(uint64_t groupId, std::string languageCode) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupId));
    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations, acl FROM groups WHERE id=?", data, false);

    if (rows->empty()) return BaseLib::Variable::createError(-1, "Unknown group.");

    BaseLib::PVariable group = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

    group->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(rows->at(0).at(0)->intValue));

    BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*rows->at(0).at(1)->binaryValue);
    if (languageCode.empty()) group->structValue->emplace("TRANSLATIONS", translations);
    else {
      auto translationIterator = translations->structValue->find(languageCode);
      if (translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
      else {
        auto shortLanguageCode = BaseLib::HelperFunctions::splitFirst(languageCode, '-').first;
        translationIterator = translations->structValue->find(shortLanguageCode);
        if (translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
        else {
          translationIterator = translations->structValue->find("en");
          if (translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
          {
            translationIterator = translations->structValue->find("en-US");
            if (translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
            else if (!translations->structValue->empty()) group->structValue->emplace("NAME", translations->structValue->begin()->second);
            else group->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
          }
        }
      }
    }

    BaseLib::PVariable acl = _rpcDecoder->decodeResponse(*rows->at(0).at(2)->binaryValue);
    group->structValue->emplace("ACL", acl);

    return group;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable DatabaseController::getGroups(std::string languageCode) {
  try {
    BaseLib::PVariable groups = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

    std::shared_ptr<BaseLib::Database::DataTable> rows = _db.executeCommand("SELECT id, translations, acl FROM groups", false);
    groups->arrayValue->reserve(rows->size());
    for (auto row: *rows) {
      BaseLib::PVariable group = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

      group->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(row.second.at(0)->intValue));

      BaseLib::PVariable translations = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
      if (languageCode.empty()) group->structValue->emplace("TRANSLATIONS", translations);
      else {
        auto translationIterator = translations->structValue->find(languageCode);
        if (translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
        else {
          auto shortLanguageCode = BaseLib::HelperFunctions::splitFirst(languageCode, '-').first;
          translationIterator = translations->structValue->find(shortLanguageCode);
          if (translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
          else {
            translationIterator = translations->structValue->find("en");
            if (translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
            {
              translationIterator = translations->structValue->find("en-US");
              if (translationIterator != translations->structValue->end()) group->structValue->emplace("NAME", translationIterator->second);
              else if (!translations->structValue->empty()) group->structValue->emplace("NAME", translations->structValue->begin()->second);
              else group->structValue->emplace("NAME", std::make_shared<BaseLib::Variable>(""));
            }
          }
        }
      }

      BaseLib::PVariable acl = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);
      group->structValue->emplace("ACL", acl);

      groups->arrayValue->push_back(group);
    }

    return groups;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool DatabaseController::groupExists(uint64_t groupId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupId));
    return !_db.executeCommand("SELECT id FROM groups WHERE id=?", data, false)->empty();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

BaseLib::PVariable DatabaseController::updateGroup(uint64_t groupId, BaseLib::PVariable translations, BaseLib::PVariable aclStruct) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(groupId));
    if (_db.executeCommand("SELECT id FROM groups WHERE id=?", data, false)->empty()) return BaseLib::Variable::createError(-1, "Unknown group.");

    std::vector<char> translationsBlob;
    if (translations && !translations->structValue->empty()) _rpcEncoder->encodeResponse(translations, translationsBlob);

    try {
      BaseLib::Security::Acl acl;
      acl.fromVariable(aclStruct);
    }
    catch (BaseLib::Security::AclException &ex) {
      return BaseLib::Variable::createError(-1, "Error in ACL: " + std::string(ex.what()));
    }

    std::vector<char> aclBlob;
    _rpcEncoder->encodeResponse(aclStruct, aclBlob);

    data.push_front(std::make_shared<BaseLib::Database::DataColumn>(aclBlob));
    if (!translationsBlob.empty()) {
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
      _db.executeCommand("UPDATE groups SET translations=?, acl=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE groups SET translations=?, acl=? WHERE id=?", data, true);
    } else {
      _db.executeCommand("UPDATE groups SET acl=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE groups SET acl=? WHERE id=?", data, true);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
//End groups

// {{{ Family
void DatabaseController::deleteFamily(int32_t familyId) {
  BaseLib::Database::DataRow data;
  data.push_back(std::make_shared<BaseLib::Database::DataColumn>(familyId));
  std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM familyVariables WHERE familyID=?", data);
  enqueue(0, entry);
}

void DatabaseController::saveFamilyVariableAsynchronous(int32_t familyId, BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 2) {
      if (data.at(1)->intValue == 0) {
        GD::out.printError("Error: Could not save family variable. Variable ID is \"0\".");
        return;
      }
      switch (data.at(0)->dataType) {
        case BaseLib::Database::DataColumn::DataType::INTEGER: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE familyVariables SET integerValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::TEXT: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE familyVariables SET stringValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::BLOB: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE familyVariables SET binaryValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::NODATA:GD::out.printError("Error: Tried to store data of type NODATA in family variable table.");
          break;
        case BaseLib::Database::DataColumn::DataType::FLOAT:GD::out.printError("Error: Tried to store data of type FLOAT in family variable table.");
          break;
      }
    } else {
      if (data.size() == 9) {
        std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(
            "INSERT OR REPLACE INTO familyVariables (variableID, familyID, variableIndex, variableName, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM familyVariables WHERE familyID=? AND variableIndex=? AND variableName=?), ?, ?, ?, ?, ?, ?)",
            data);
        enqueue(0, entry);
      } else if (data.size() == 7 && data.at(0)->intValue != 0) {
        std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO familyVariables VALUES(?, ?, ?, ?, ?, ?, ?)", data);
        enqueue(0, entry);
      } else GD::out.printError("Error: Either variableID is 0 or the number of columns is invalid.");
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getFamilyVariables(int32_t familyId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(familyId));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM familyVariables WHERE familyID=?", data, false);
    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::deleteFamilyVariable(BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 1) {
      if (data.at(0)->intValue == 0) {
        GD::out.printError("Error: Could not delete family variable. Variable ID is \"0\".");
        return;
      }
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM familyVariables WHERE variableID=?", data);
      enqueue(0, entry);
    } else if (data.size() == 2 && data.at(1)->dataType == BaseLib::Database::DataColumn::DataType::Enum::INTEGER) {
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM familyVariables WHERE familyID=? AND variableIndex=?", data);
      enqueue(0, entry);
    } else if (data.size() == 2 && data.at(1)->dataType == BaseLib::Database::DataColumn::DataType::Enum::TEXT) {
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM familyVariables WHERE familyID=? AND variableName=?", data);
      enqueue(0, entry);
    } else if (data.size() == 3) {
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM familyVariables WHERE familyID=? AND variableIndex=? AND variableName=?", data);
      enqueue(0, entry);
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}
// }}}

//Device
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getDevices(uint32_t family) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(family));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM devices WHERE deviceFamily=?", data, false);
    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::deleteDevice(uint64_t id) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(id));
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM devices WHERE deviceID=?", data);
    enqueue(0, entry);
    entry = std::make_shared<QueueEntry>("DELETE FROM deviceVariables WHERE deviceID=?", data);
    enqueue(0, entry);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

uint64_t DatabaseController::saveDevice(uint64_t id, int32_t address, std::string &serialNumber, uint32_t type, uint32_t family) {
  try {
    BaseLib::Database::DataRow data;
    if (id != 0) data.push_back(std::make_shared<BaseLib::Database::DataColumn>(id));
    else data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(address));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(serialNumber));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(type));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(family));
    auto result = _db.executeWriteCommand("REPLACE INTO devices VALUES(?, ?, ?, ?, ?)", data, false);
    if (id == 0) data.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand("REPLACE INTO devices VALUES(?, ?, ?, ?, ?)", data, true);

    return (uint64_t)result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}

void DatabaseController::saveDeviceVariableAsynchronous(BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 2) {
      if (data.at(1)->intValue == 0) {
        GD::out.printError("Error: Could not save device variable. Variable ID is \"0\".");
        return;
      }
      switch (data.at(0)->dataType) {
        case BaseLib::Database::DataColumn::DataType::INTEGER: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE deviceVariables SET integerValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::TEXT: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE deviceVariables SET stringValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::BLOB: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE deviceVariables SET binaryValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::NODATA:GD::out.printError("Error: Tried to store data of type NODATA in variable table.");
          break;
        case BaseLib::Database::DataColumn::DataType::FLOAT:GD::out.printError("Error: Tried to store data of type FLOAT in variable table.");
          break;
      }
    } else {
      if (data.size() == 5) {
        std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(
            "INSERT OR REPLACE INTO deviceVariables (variableID, deviceID, variableIndex, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM deviceVariables WHERE deviceID=" + std::to_string(data.at(0)->intValue)
                + " AND variableIndex=" + std::to_string(data.at(1)->intValue) + "), ?, ?, ?, ?, ?)",
            data);
        enqueue(0, entry);
      } else if (data.size() == 6 && data.at(0)->intValue != 0) {
        std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
        enqueue(0, entry);
      } else GD::out.printError("Error: Either variableID is 0 or the number of columns is invalid.");
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::deletePeers(int32_t deviceID) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(deviceID));
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM peers WHERE parent=?", data);
    enqueue(0, entry);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getPeers(uint64_t deviceID) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(deviceID));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM peers WHERE parent=?", data, false);
    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getDeviceVariables(uint64_t deviceID) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(deviceID));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM deviceVariables WHERE deviceID=?", data, false);
    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}
//End device

//Peer
void DatabaseController::deletePeer(uint64_t id) {
  try {
    BaseLib::Database::DataRow data({std::make_shared<BaseLib::Database::DataColumn>(id)});
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM parameters WHERE peerID=?", data);
    enqueue(0, entry);
    entry = std::make_shared<QueueEntry>("DELETE FROM peerVariables WHERE peerID=?", data);
    enqueue(0, entry);
    entry = std::make_shared<QueueEntry>("DELETE FROM peers WHERE peerID=?", data);
    enqueue(0, entry);
    entry = std::make_shared<QueueEntry>("DELETE FROM serviceMessages WHERE peerID=?", data);
    enqueue(0, entry);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

uint64_t DatabaseController::savePeer(uint64_t id, uint32_t parentID, int32_t address, std::string &serialNumber, uint32_t type) {
  BaseLib::Database::DataRow data;
  if (id > 0) data.push_back(std::make_shared<BaseLib::Database::DataColumn>(id));
  else data.push_back(std::make_shared<BaseLib::Database::DataColumn>());
  data.push_back(std::make_shared<BaseLib::Database::DataColumn>(parentID));
  data.push_back(std::make_shared<BaseLib::Database::DataColumn>(address));
  data.push_back(std::make_shared<BaseLib::Database::DataColumn>(serialNumber));
  data.push_back(std::make_shared<BaseLib::Database::DataColumn>(type));
  uint64_t result = _db.executeWriteCommand("REPLACE INTO peers VALUES(?, ?, ?, ?, ?)", data, false);
  if (result == 0) throw BaseLib::Exception("Error saving peer to database. See previous errors in log for more information.");
  else if (result > BaseLib::Systems::Peer::kMaximumPeerId) throw BaseLib::Exception("Error: Reached maximum possible peer ID. Please move peers to lower IDs or delete peers.");
  if (id == 0) data.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
  _db.executeWriteCommand("REPLACE INTO peers VALUES(?, ?, ?, ?, ?)", data, true);
  return result;
}

uint64_t DatabaseController::savePeerParameterSynchronous(BaseLib::Database::DataRow &data) {
  if (data.size() == 7) {
    data.push_front(data.at(5));
    auto command = "REPLACE INTO parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName, value) VALUES((SELECT parameterID FROM parameters WHERE peerID=" + std::to_string(data.at(1)->intValue)
        + " AND parameterSetType=" + std::to_string(data.at(2)->intValue) + " AND peerChannel=" + std::to_string(data.at(3)->intValue) + " AND remotePeer=" + std::to_string(data.at(4)->intValue) + " AND remoteChannel="
        + std::to_string(data.at(5)->intValue) + " AND parameterName=?), ?, ?, ?, ?, ?, ?, ?)";
    uint64_t result = _db.executeWriteCommand(command, data, false);
    if (result == 0) throw BaseLib::Exception("Error saving peer parameter to database. See previous errors in log for more information.");
    data.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand(command, data, true);
    return result;
  } else GD::out.printError("Error: The number of columns is invalid.");
  return 0;
}

void DatabaseController::savePeerParameterAsynchronous(BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 2) {
      if (data.at(1)->intValue == 0) {
        GD::out.printError("Error: Could not save peer parameter. Parameter ID is \"0\".");
        return;
      }
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE parameters SET value=? WHERE parameterID=?", data);
      enqueue(0, entry);
    } else {
      if (data.size() == 6) {
        data.push_front(data.at(3));
        std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(
            "INSERT OR REPLACE INTO parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName, value, specialType) VALUES((SELECT parameterID FROM parameters WHERE peerID="
                + std::to_string(data.at(1)->intValue) + " AND parameterSetType=" + std::to_string(data.at(2)->intValue) + " AND peerChannel=" + std::to_string(data.at(3)->intValue) + " AND parameterName=?), ?, ?, ?, ?, ?, ?)",
            data);
        enqueue(0, entry);
      } else if (data.size() == 7) {
        data.push_front(data.at(5));
        std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(
            "INSERT OR REPLACE INTO parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName, value) VALUES((SELECT parameterID FROM parameters WHERE peerID=" + std::to_string(data.at(1)->intValue)
                + " AND parameterSetType=" + std::to_string(data.at(2)->intValue) + " AND peerChannel=" + std::to_string(data.at(3)->intValue) + " AND remotePeer=" + std::to_string(data.at(4)->intValue) + " AND remoteChannel="
                + std::to_string(data.at(5)->intValue) + " AND parameterName=?), ?, ?, ?, ?, ?, ?, ?)",
            data);
        enqueue(0, entry);
      } else if (data.size() == 8 && data.at(0)->intValue != 0) {
        std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO parameters (parameterID, peerID, parameterSetType, peerChannel, remotePeer, remoteChannel, parameterName, value) VALUES(?, ?, ?, ?, ?, ?, ?, ?)", data);
        enqueue(0, entry);
      } else GD::out.printError("Error: Either parameterID is 0 or the number of columns is invalid.");
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::saveSpecialPeerParameterAsynchronous(BaseLib::Database::DataRow &data) {
  try {
    if (data.size() != 8) {
      GD::out.printError("Error: Invalid number of columns.");
      return;
    }

    data.push_front(data.at(3));
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(
        "INSERT OR REPLACE INTO parameters (parameterID, peerID, parameterSetType, peerChannel, parameterName, value, specialType, metadata, roles) VALUES((SELECT parameterID FROM parameters WHERE peerID=" + std::to_string(data.at(1)->intValue)
            + " AND parameterSetType=" + std::to_string(data.at(2)->intValue) + " AND peerChannel=" + std::to_string(data.at(3)->intValue) + " AND parameterName=?), ?, ?, ?, ?, ?, ?, ?, ?)",
        data);
    enqueue(0, entry);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::savePeerParameterRoomAsynchronous(BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 2) {
      if (data.at(1)->intValue == 0) {
        GD::out.printError("Error: Could not save room of peer parameter. Parameter ID is \"0\".");
        return;
      }
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE parameters SET room=? WHERE parameterID=?", data);
      enqueue(0, entry);
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::savePeerParameterBuildingPartAsynchronous(BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 2) {
      if (data.at(1)->intValue == 0) {
        GD::out.printError("Error: Could not save building part of peer parameter. Parameter ID is \"0\".");
        return;
      }
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE parameters SET buildingPart=? WHERE parameterID=?", data);
      enqueue(0, entry);
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::savePeerParameterCategoriesAsynchronous(BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 2) {
      if (data.at(1)->intValue == 0) {
        GD::out.printError("Error: Could not save categories of peer parameter. Parameter ID is \"0\".");
        return;
      }
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE parameters SET categories=? WHERE parameterID=?", data);
      enqueue(0, entry);
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::savePeerParameterRolesAsynchronous(BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 2) {
      if (data.at(1)->intValue == 0) {
        GD::out.printError("Error: Could not save roles of peer parameter. Parameter ID is \"0\".");
        return;
      }
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE parameters SET roles=? WHERE parameterID=?", data);
      enqueue(0, entry);
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::savePeerVariableAsynchronous(BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 2) {
      if (data.at(1)->intValue == 0) {
        GD::out.printError("Error: Could not save peer variable. Variable ID is \"0\".");
        return;
      }
      switch (data.at(0)->dataType) {
        case BaseLib::Database::DataColumn::DataType::INTEGER: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE peerVariables SET integerValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::TEXT: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE peerVariables SET stringValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::BLOB: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE peerVariables SET binaryValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::NODATA:GD::out.printError("Error: Tried to store data of type NODATA in peer variable table.");
          break;
        case BaseLib::Database::DataColumn::DataType::FLOAT:GD::out.printError("Error: Tried to store data of type FLOAT in peer variable table.");
          break;
      }
    } else {
      if (data.size() == 5) {
        std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(
            "INSERT OR REPLACE INTO peerVariables (variableID, peerID, variableIndex, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM peerVariables WHERE peerID=" + std::to_string(data.at(0)->intValue) + " AND variableIndex="
                + std::to_string(data.at(1)->intValue) + "), ?, ?, ?, ?, ?)",
            data);
        enqueue(0, entry);
      } else if (data.size() == 6 && data.at(0)->intValue != 0) {
        std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO peerVariables VALUES(?, ?, ?, ?, ?, ?)", data);
        enqueue(0, entry);
      } else GD::out.printError("Error: Either variableID is 0 or the number of columns is invalid.");
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getPeerParameters(uint64_t peerID) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(peerID));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM parameters WHERE peerID=?", data, false);
    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getPeerVariables(uint64_t peerID) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(peerID));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM peerVariables WHERE peerID=?", data, false);
    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::deletePeerParameter(uint64_t peerID, BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 2) {
      if (data.at(1)->intValue == 0) {
        GD::out.printError("Error: Could not delete parameter. Parameter ID is \"0\".");
        return;
      }
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM parameters WHERE peerID=? AND parameterID=?", data);
      enqueue(0, entry);
    } else if (data.size() == 4) {
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND parameterName=?", data);
      enqueue(0, entry);
    } else if (data.size() == 5) {
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND remotePeer=? AND remoteChannel=?", data);
      enqueue(0, entry);
    } else {
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND parameterName=? AND remotePeer=? AND remoteChannel=?", data);
      enqueue(0, entry);
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

bool DatabaseController::peerExists(uint64_t id) {
  BaseLib::Database::DataRow data({std::make_shared<BaseLib::Database::DataColumn>(id)});
  std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT 1 FROM peers WHERE peerID=?", data, false);
  return !result->empty();
}

bool DatabaseController::peerExists(const std::string &serial_number) {
  BaseLib::Database::DataRow data({std::make_shared<BaseLib::Database::DataColumn>(serial_number)});
  std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT 1 FROM peers WHERE serialNumber=?", data, false);
  return !result->empty();
}

bool DatabaseController::setPeerID(uint64_t oldPeerID, uint64_t newPeerID) {
  try {
    if (oldPeerID > BaseLib::Systems::Peer::kMaximumPeerId ||
        newPeerID > BaseLib::Systems::Peer::kMaximumPeerId) {
      return false;
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(newPeerID));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(oldPeerID));
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE peers SET peerID=? WHERE peerID=?", data);
    enqueue(0, entry);
    entry = std::make_shared<QueueEntry>("UPDATE parameters SET peerID=? WHERE peerID=?", data);
    enqueue(0, entry);
    entry = std::make_shared<QueueEntry>("UPDATE peerVariables SET peerID=? WHERE peerID=?", data);
    enqueue(0, entry);
    entry = std::make_shared<QueueEntry>("UPDATE serviceMessages SET peerID=? WHERE peerID=?", data);
    enqueue(0, entry);

    {
      std::lock_guard<std::mutex> metadataGuard(_metadataMutex);
      _metadata.erase(oldPeerID);
    }

    data.clear();
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::to_string(newPeerID)));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(std::to_string(oldPeerID)));
    entry = std::make_shared<QueueEntry>("UPDATE metadata SET objectID=? WHERE objectID=?", data);
    enqueue(0, entry);
    return true;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

bool DatabaseController::setPeerSerialNumber(uint64_t peer_id, const std::string &serial_number) {
  try {
    if (peer_id > BaseLib::Systems::Peer::kMaximumPeerId) {
      return false;
    }

    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(serial_number));
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(peer_id));
    std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE peers SET serialNumber=? WHERE peerID=?", data);
    enqueue(0, entry);

    return true;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}
//End peer

//Service messages
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getServiceMessages(uint64_t peerID) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(peerID));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM serviceMessages WHERE peerID=?", data, false);
    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::saveServiceMessageAsynchronous(uint64_t peerID, BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 3) {
      if (data.at(2)->intValue == 0) {
        GD::out.printError("Error: Could not save service message. Variable ID is \"0\".");
        return;
      }
      switch (data.at(1)->dataType) {
        case BaseLib::Database::DataColumn::DataType::INTEGER: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE serviceMessages SET timestamp=?, integerValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::TEXT: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE serviceMessages SET timestamp=?, message=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::BLOB: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE serviceMessages SET timestamp=?, binaryData=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::NODATA:GD::out.printError("Error: Tried to store data of type NODATA in service message table.");
          break;
        case BaseLib::Database::DataColumn::DataType::FLOAT:GD::out.printError("Error: Tried to store data of type FLOAT in service message table.");
          break;
      }
    } else if (data.size() == 5) {
      if (data.at(4)->intValue == 0) {
        GD::out.printError("Error: Could not save service message. Variable ID is \"0\".");
        return;
      }
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE serviceMessages SET timestamp=?, integerValue=?, message=?, binaryData=? WHERE variableID=?", data);
      enqueue(0, entry);
    } else if (data.size() == 10) {
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(
          "INSERT OR REPLACE INTO serviceMessages (variableID, familyID, peerID, messageID, messageSubID, timestamp, integerValue, message, variables, binaryData, priority) VALUES((SELECT variableID FROM serviceMessages WHERE peerID="
              + std::to_string(data.at(1)->intValue) + " AND messageID=" + std::to_string(data.at(2)->intValue) + "), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
          data);
      enqueue(0, entry);
    } else if (data.size() == 11 && data.at(0)->intValue != 0) {
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO serviceMessages VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", data);
      enqueue(0, entry);
    } else GD::out.printError("Error: Either variableID is 0 or the number of columns is invalid.");
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::saveGlobalServiceMessageAsynchronous(BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 13) {
      std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(
          "INSERT OR REPLACE INTO serviceMessages (variableID, familyID, interface, peerID, messageID, messageSubID, timestamp, integerValue, message, variables, binaryData, priority) VALUES((SELECT variableID FROM serviceMessages WHERE familyID="
              + std::to_string(data.at(2)->intValue) + " AND messageID=" + std::to_string(data.at(4)->intValue) + " AND messageSubID=? AND message=?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
          data);
      enqueue(0, entry);
    } else GD::out.printError("Error: Either variableID is 0 or the number of columns is invalid.");
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::deleteServiceMessage(uint64_t databaseID) {
  try {
    BaseLib::Database::DataRow data({std::make_shared<BaseLib::Database::DataColumn>(databaseID)});
    _db.executeCommand("DELETE FROM serviceMessages WHERE variableID=?", data, false);
    _db.executeCommand("DELETE FROM serviceMessages WHERE variableID=?", data, true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::deleteGlobalServiceMessage(int32_t familyId, int32_t messageId, std::string &messageSubId, std::string &message) {
  try {
    BaseLib::Database::DataRow data({std::make_shared<BaseLib::Database::DataColumn>(familyId), std::make_shared<BaseLib::Database::DataColumn>(messageId), std::make_shared<BaseLib::Database::DataColumn>(messageSubId),
                                     std::make_shared<BaseLib::Database::DataColumn>(message)});
    _db.executeCommand("DELETE FROM serviceMessages WHERE familyID=? AND messageID=? AND messageSubID=? AND message=?", data, false);
    _db.executeCommand("DELETE FROM serviceMessages WHERE familyID=? AND messageID=? AND messageSubID=? AND message=?", data, true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}
//End service messages

// {{{ License modules
std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getLicenseVariables(int32_t moduleId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(moduleId));
    std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT * FROM licenseVariables WHERE moduleID=?", data, false);
    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

void DatabaseController::saveLicenseVariable(int32_t moduleId, BaseLib::Database::DataRow &data) {
  try {
    if (data.size() == 2) {
      if (data.at(1)->intValue == 0) {
        GD::out.printError("Error: Could not save license module variable. Variable ID is \"0\".");
        return;
      }
      switch (data.at(0)->dataType) {
        case BaseLib::Database::DataColumn::DataType::INTEGER: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE licenseVariables SET integerValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::TEXT: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE licenseVariables SET stringValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::BLOB: {
          std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("UPDATE licenseVariables SET binaryValue=? WHERE variableID=?", data);
          enqueue(0, entry);
        }
          break;
        case BaseLib::Database::DataColumn::DataType::NODATA:GD::out.printError("Error: Tried to store data of type NODATA in license module variable table.");
          break;
        case BaseLib::Database::DataColumn::DataType::FLOAT:GD::out.printError("Error: Tried to store data of type FLOAT in license module variable table.");
          break;
      }
    } else {
      if (data.size() == 5) {
        std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>(
            "INSERT OR REPLACE INTO licenseVariables (variableID, moduleID, variableIndex, integerValue, stringValue, binaryValue) VALUES((SELECT variableID FROM licenseVariables WHERE moduleID=" + std::to_string(data.at(0)->intValue)
                + " AND variableIndex=" + std::to_string(data.at(1)->intValue) + "), ?, ?, ?, ?, ?)",
            data);
        enqueue(0, entry);
      } else if (data.size() == 6 && data.at(0)->intValue != 0) {
        std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("REPLACE INTO licenseVariables VALUES(?, ?, ?, ?, ?, ?)", data);
        enqueue(0, entry);
      } else GD::out.printError("Error: Either variableID is 0 or the number of columns is invalid.");
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void DatabaseController::deleteLicenseVariable(int32_t moduleId, uint64_t mapKey) {
  try {
    _db.executeCommand("DELETE FROM licenseVariables WHERE variableIndex=" + std::to_string(mapKey), false);
    _db.executeCommand("DELETE FROM licenseVariables WHERE variableIndex=" + std::to_string(mapKey), true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}
// }}}

// {{{ Variable profiles
uint64_t DatabaseController::addVariableProfile(const BaseLib::PVariable &translations, const BaseLib::PVariable &profile) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>());

    std::vector<char> translationsBlob;
    _rpcEncoder->encodeResponse(translations, translationsBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));

    std::vector<char> profileBlob;
    _rpcEncoder->encodeResponse(profile, profileBlob);
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(profileBlob));

    uint64_t result = _db.executeWriteCommand("REPLACE INTO variableProfiles VALUES(?, ?, ?)", data, false);
    data.at(0) = std::make_shared<BaseLib::Database::DataColumn>(result);
    _db.executeWriteCommand("REPLACE INTO variableProfiles VALUES(?, ?, ?)", data, true);

    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return 0;
}

void DatabaseController::deleteVariableProfile(uint64_t profileId) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(profileId));

    _db.executeWriteCommand("DELETE FROM variableProfiles WHERE id=?", data, false);
    _db.executeWriteCommand("DELETE FROM variableProfiles WHERE id=?", data, true);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

std::shared_ptr<BaseLib::Database::DataTable> DatabaseController::getVariableProfiles() {
  try {
    return _db.executeCommand("SELECT id, translations, profile FROM variableProfiles", false);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::shared_ptr<BaseLib::Database::DataTable>();
}

bool DatabaseController::updateVariableProfile(uint64_t profileId, const BaseLib::PVariable &translations, const BaseLib::PVariable &profile) {
  try {
    BaseLib::Database::DataRow data;
    data.push_back(std::make_shared<BaseLib::Database::DataColumn>(profileId));
    auto profileRows = _db.executeCommand("SELECT translations FROM variableProfiles WHERE id=?", data, false);
    if (profileRows->empty()) return false;

    auto translationsBlob = std::move(*profileRows->at(0).at(0)->binaryValue);
    auto oldTranslations = _rpcDecoder->decodeResponse(translationsBlob);
    oldTranslations->type = BaseLib::VariableType::tStruct;

    for (auto &translation: *translations->structValue) {
      oldTranslations->structValue->emplace(translation.first, translation.second);
    }

    if (!oldTranslations->structValue->empty()) _rpcEncoder->encodeResponse(oldTranslations, translationsBlob);

    std::vector<char> profileBlob;
    if (!profile->structValue->empty()) _rpcEncoder->encodeResponse(profile, profileBlob);

    if (!translationsBlob.empty() && !profileBlob.empty()) {
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(profileBlob));
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
      _db.executeCommand("UPDATE variableProfiles SET translations=?, profile=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE variableProfiles SET translations=?, profile=? WHERE id=?", data, true);
    } else if (!translationsBlob.empty()) {
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(translationsBlob));
      _db.executeCommand("UPDATE variableProfiles SET translations=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE variableProfiles SET translations=? WHERE id=?", data, true);
    } else if (!profileBlob.empty()) {
      data.push_front(std::make_shared<BaseLib::Database::DataColumn>(profileBlob));
      _db.executeCommand("UPDATE variableProfiles SET profile=? WHERE id=?", data, false);
      _db.executeCommand("UPDATE variableProfiles SET profile=? WHERE id=?", data, true);
    }

    return true;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}
// }}}
}