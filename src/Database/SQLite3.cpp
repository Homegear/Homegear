#include <utility>

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

#include "SQLite3.h"
#include "../GD/GD.h"

namespace Homegear {

SQLite3::SQLite3() = default;

SQLite3::SQLite3(const std::string &databasePath, const std::string &databaseFilename, const std::string &maintenanceDatabasePath, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWALJournal) : SQLite3() {
  if (databasePath.empty()) return;
  _databaseSynchronous = databaseSynchronous;
  _databaseMemoryJournal = databaseMemoryJournal;
  _databaseWALJournal = databaseWALJournal;
  _databasePath = databasePath;
  _maintenanceDatabasePath = maintenanceDatabasePath;
  if (_maintenanceDatabasePath == _databasePath) {
    GD::out.printError("Error: Maintenance database path is the same as database path. This is not allowed. Disabling maintenance database.");
    _maintenanceDatabasePath = "";
  }
  _databaseFilename = databaseFilename;
  _backupPath = "";
  _backupFilename = "";
  openDatabase(true);
}

void SQLite3::init(const std::string &databasePath,
                   const std::string &databaseFilename,
                   const std::string &maintenanceDatabasePath,
                   bool databaseSynchronous,
                   bool databaseMemoryJournal,
                   bool databaseWALJournal,
                   const std::string &backupPath,
                   const std::string &maintenanceDatabaseBackupPath,
                   const std::string &backupFilename) {
  if (databasePath.empty()) return;
  _databaseSynchronous = databaseSynchronous;
  _databaseMemoryJournal = databaseMemoryJournal;
  _databaseWALJournal = databaseWALJournal;
  _databasePath = databasePath;
  _maintenanceDatabasePath = maintenanceDatabasePath;
  if (_maintenanceDatabasePath == _databasePath) {
    GD::out.printError("Error: Maintenance database path is the same as database path. This is not allowed. Disabling maintenance database.");
    _maintenanceDatabasePath = "";
  }
  _databaseFilename = databaseFilename;
  _backupPath = backupPath;
  _maintenanceDatabaseBackupPath = maintenanceDatabaseBackupPath;
  if (_maintenanceDatabaseBackupPath == _backupPath) {
    GD::out.printError("Error: Maintenance database backup path is the same as database backup path. This is not allowed. Disabling backup of maintenance database.");
    _maintenanceDatabaseBackupPath = "";
  }
  _backupFilename = backupFilename;
  hotBackup(false);
  openDatabase(true);
}

SQLite3::~SQLite3() {
  closeDatabase(true);
}

void SQLite3::dispose() {
  closeDatabase(true);
  closeMaintenanceDatabase(true);
}

void SQLite3::hotBackup(bool maintenanceDatabase) {
  try {
    std::string databasePath;
    std::string backupPath;
    if (maintenanceDatabase) {
      if (_maintenanceDatabasePath.empty() || _maintenanceDatabaseBackupPath.empty() || _databaseFilename.empty()) {
        return;
      }
      databasePath = _maintenanceDatabasePath;
      backupPath = _maintenanceDatabaseBackupPath;
    } else {
      if (_databasePath.empty() || _databaseFilename.empty()) {
        GD::out.printError(std::string("Error: Can't backup ") + (maintenanceDatabase ? "maintenance " : "") + "database: databasePath or databaseFilename is empty.");
        return;
      }
      databasePath = _databasePath;
      backupPath = _backupPath;
    }
    std::lock_guard<std::mutex> databaseGuard(_databaseMutex);
    bool databaseWasOpen = false;
    if (maintenanceDatabase) {
      if (_maintenanceDatabase) databaseWasOpen = true;
      closeMaintenanceDatabase(false);
    } else {
      if (_database) databaseWasOpen = true;
      closeDatabase(false);
    }
    if (BaseLib::Io::fileExists(databasePath + _databaseFilename)) {
      if (!checkIntegrity(databasePath + _databaseFilename)) {
        GD::out.printCritical(std::string("Critical: Integrity check on ") + (maintenanceDatabase ? "maintenance " : "") + "database failed.");
        if (!backupPath.empty() && !_backupFilename.empty()) {
          GD::out.printCritical(std::string("Critical: Backing up corrupted ") + (maintenanceDatabase ? "maintenance " : "") + "database file to: " + backupPath + _databaseFilename + ".broken");
          GD::bl->io.copyFile(databasePath + _databaseFilename, backupPath + _databaseFilename + ".broken");
          bool restored = false;
          for (int32_t i = 0; i <= 10000; i++) {
            if (BaseLib::Io::fileExists(backupPath + _backupFilename + std::to_string(i)) && checkIntegrity(backupPath + _backupFilename + std::to_string(i))) {
              GD::out.printCritical(std::string("Critical: Restoring ") + (maintenanceDatabase ? "maintenance " : "") + "database file: " + backupPath + _backupFilename + std::to_string(i));
              if (GD::bl->io.copyFile(backupPath + _backupFilename + std::to_string(i), databasePath + _databaseFilename)) {
                restored = true;
                break;
              }
            }
          }
          if (!restored) {
            GD::out.printCritical(std::string("Critical: Could not restore ") + (maintenanceDatabase ? "maintenance " : "") + "database.");
            return;
          }
        } else return;
      } else {
        if (!backupPath.empty() && !_backupFilename.empty()) {
          GD::out.printInfo(std::string("Info: Backing up ") + (maintenanceDatabase ? "maintenance " : "") + "database...");
          if (GD::bl->settings.databaseMaxBackups() > 1) {
            if (BaseLib::Io::fileExists(backupPath + _backupFilename + std::to_string(GD::bl->settings.databaseMaxBackups() - 1))) {
              if (!BaseLib::Io::deleteFile(backupPath + _backupFilename + std::to_string(GD::bl->settings.databaseMaxBackups() - 1))) {
                GD::out.printError("Error: Cannot delete file: " + backupPath + _backupFilename + std::to_string(GD::bl->settings.databaseMaxBackups() - 1));
              }
            }
            for (int32_t i = GD::bl->settings.databaseMaxBackups() - 2; i >= 0; i--) {
              if (BaseLib::Io::fileExists(backupPath + _backupFilename + std::to_string(i))) {
                if (!BaseLib::Io::moveFile(backupPath + _backupFilename + std::to_string(i), backupPath + _backupFilename + std::to_string(i + 1))) {
                  GD::out.printError("Error: Cannot move file: " + backupPath + _backupFilename + std::to_string(i));
                }
              }
            }
          }
          if (GD::bl->settings.databaseMaxBackups() > 0) {
            if (!GD::bl->io.copyFile(databasePath + _databaseFilename, backupPath + _backupFilename + '0')) {
              GD::out.printError("Error: Cannot copy file: " + backupPath + _backupFilename + '0');
            }
          }
        } else GD::out.printError(std::string("Error: Can't backup ") + (maintenanceDatabase ? "maintenance " : "") + "database: backupPath or _backupFilename is empty.");
      }
    } else {
      if (maintenanceDatabase) {
        GD::out.printWarning("Warning: No maintenance database found.");
      } else {
        GD::out.printWarning("Warning: No database found. Trying to restore backup.");
        if (!backupPath.empty() && !_backupFilename.empty()) {
          bool restored = false;
          for (int32_t i = 0; i <= 10000; i++) {
            if (BaseLib::Io::fileExists(backupPath + _backupFilename + std::to_string(i)) && checkIntegrity(backupPath + _backupFilename + std::to_string(i))) {
              GD::out.printWarning("Warning: Restoring database file: " + backupPath + _backupFilename + std::to_string(i));
              if (GD::bl->io.copyFile(backupPath + _backupFilename + std::to_string(i), databasePath + _databaseFilename)) {
                restored = true;
                break;
              }
            }
          }
          if (restored) GD::out.printWarning("Warning: Database successfully restored.");
          else GD::out.printWarning("Warning: Database could not be restored. Creating new database.");
        }
      }
    }
    if (databaseWasOpen) {
      if (maintenanceDatabase) {
        openMaintenanceDatabase(false);
      } else {
        openDatabase(false);
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

bool SQLite3::enableMaintenanceMode() {
  try {
    GD::out.printInfo("Enabling maintenance mode...");
    hotBackup(true);
    openMaintenanceDatabase(true);
    return _maintenanceDatabase;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

bool SQLite3::disableMaintenanceMode() {
  try {
    GD::out.printInfo("Disabling maintenance mode...");
    if (!_maintenanceDatabase) return false;
    closeMaintenanceDatabase(true);
    if (!checkIntegrity(_maintenanceDatabasePath + _databaseFilename)) {
      GD::out.printCritical("Critical: Integrity check on maintenance database failed.");
      if (!_maintenanceDatabaseBackupPath.empty() && !_backupFilename.empty()) {
        GD::out.printCritical("Critical: Backing up corrupted maintenance database file to: " + _maintenanceDatabaseBackupPath + _databaseFilename + ".broken");
        GD::bl->io.copyFile(_maintenanceDatabasePath + _databaseFilename, _maintenanceDatabaseBackupPath + _databaseFilename + ".broken");
        bool restored = false;
        for (int32_t i = 0; i <= 10000; i++) {
          if (BaseLib::Io::fileExists(_maintenanceDatabaseBackupPath + _backupFilename + std::to_string(i)) && checkIntegrity(_maintenanceDatabaseBackupPath + _backupFilename + std::to_string(i))) {
            GD::out.printCritical("Critical: Restoring maintenance database file: " + _maintenanceDatabaseBackupPath + _backupFilename + std::to_string(i));
            if (GD::bl->io.copyFile(_maintenanceDatabaseBackupPath + _backupFilename + std::to_string(i), _maintenanceDatabasePath + _databaseFilename)) {
              restored = true;
              break;
            }
          }
        }
        if (!restored) {
          GD::out.printCritical("Critical: Could not restore maintenance database.");
          return false;
        }
      } else return false;
    }
    return true;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

bool SQLite3::checkIntegrity(const std::string &databasePath) {
  sqlite3 *database = nullptr;
  try {
    int32_t result = sqlite3_open(databasePath.c_str(), &database);
    if (result || !database) {
      if (database) sqlite3_close(database);
      return true;
    }

    std::shared_ptr<BaseLib::Database::DataTable> integrityResult(new BaseLib::Database::DataTable());

    sqlite3_stmt *statement = nullptr;
    result = sqlite3_prepare_v2(database, "PRAGMA integrity_check", -1, &statement, nullptr);
    if (result) {
      sqlite3_close(database);
      return false;
    }
    try {
      getDataRows(statement, integrityResult);
    }
    catch (const std::exception &ex) {
      sqlite3_close(database);
      return false;
    }
    sqlite3_clear_bindings(statement);
    result = sqlite3_finalize(statement);
    if (result) {
      sqlite3_close(database);
      return false;
    }

    if (integrityResult->size() != 1 || integrityResult->at(0).empty() || integrityResult->at(0).at(0)->textValue != "ok") {
      sqlite3_close(database);
      return false;
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  if (database) sqlite3_close(database);
  return true;
}

void SQLite3::openDatabase(bool lockMutex) {
  try {
    std::unique_lock<std::mutex> databaseGuard(_databaseMutex, std::defer_lock);
    if (lockMutex) databaseGuard.lock();
    char *errorMessage = nullptr;
    std::string fullDatabasePath = _databasePath + _databaseFilename;
    int result = sqlite3_open(fullDatabasePath.c_str(), &_database);
    if (result || !_database) {
      GD::out.printCritical("Can't open database: " + std::string(sqlite3_errmsg(_database)));
      if (_database) sqlite3_close(_database);
      _database = nullptr;
      return;
    }
    sqlite3_extended_result_codes(_database, 1);

    if (!_databaseSynchronous) {
      sqlite3_exec(_database, "PRAGMA synchronous=OFF", nullptr, nullptr, &errorMessage);
      if (errorMessage) {
        GD::out.printError("Can't execute \"PRAGMA synchronous=OFF\": " + std::string(errorMessage));
        sqlite3_free(errorMessage);
      }
    }

    //Reset to default journal mode, because WAL stays active.
    //Also I'm not sure if VACUUM works with journal_mode=WAL.
    sqlite3_exec(_database, "PRAGMA journal_mode=DELETE", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      GD::out.printError("Can't execute \"PRAGMA journal_mode=DELETE\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }

    sqlite3_exec(_database, "VACUUM", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      GD::out.printWarning("Warning: Can't execute \"VACUUM\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }

    if (_databaseMemoryJournal) {
      sqlite3_exec(_database, "PRAGMA journal_mode=MEMORY", nullptr, nullptr, &errorMessage);
      if (errorMessage) {
        GD::out.printError("Can't execute \"PRAGMA journal_mode=MEMORY\": " + std::string(errorMessage));
        sqlite3_free(errorMessage);
      }
    }

    if (_databaseWALJournal) {
      sqlite3_exec(_database, "PRAGMA journal_mode=WAL", nullptr, nullptr, &errorMessage);
      if (errorMessage) {
        GD::out.printError("Can't execute \"PRAGMA journal_mode=WAL\": " + std::string(errorMessage));
        sqlite3_free(errorMessage);
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void SQLite3::openMaintenanceDatabase(bool lockMutex) {
  try {
    std::unique_lock<std::mutex> databaseGuard(_databaseMutex, std::defer_lock);
    if (lockMutex) databaseGuard.lock();
    char *errorMessage = nullptr;
    std::string fullDatabasePath = _maintenanceDatabasePath + _databaseFilename;
    int result = sqlite3_open(fullDatabasePath.c_str(), &_maintenanceDatabase);
    if (result || !_maintenanceDatabase) {
      GD::out.printCritical("Can't open database: " + std::string(sqlite3_errmsg(_maintenanceDatabase)));
      if (_maintenanceDatabase) sqlite3_close(_maintenanceDatabase);
      _maintenanceDatabase = nullptr;
      return;
    }
    sqlite3_extended_result_codes(_maintenanceDatabase, 1);

    if (!_databaseSynchronous) {
      sqlite3_exec(_maintenanceDatabase, "PRAGMA synchronous=OFF", nullptr, nullptr, &errorMessage);
      if (errorMessage) {
        GD::out.printError("Can't execute \"PRAGMA synchronous=OFF\": " + std::string(errorMessage));
        sqlite3_free(errorMessage);
      }
    }

    //Reset to default journal mode, because WAL stays active.
    //Also I'm not sure if VACUUM works with journal_mode=WAL.
    sqlite3_exec(_maintenanceDatabase, "PRAGMA journal_mode=DELETE", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      GD::out.printError("Can't execute \"PRAGMA journal_mode=DELETE\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }

    sqlite3_exec(_maintenanceDatabase, "VACUUM", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      GD::out.printWarning("Warning: Can't execute \"VACUUM\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }

    if (_databaseMemoryJournal) {
      sqlite3_exec(_maintenanceDatabase, "PRAGMA journal_mode=MEMORY", nullptr, nullptr, &errorMessage);
      if (errorMessage) {
        GD::out.printError("Can't execute \"PRAGMA journal_mode=MEMORY\": " + std::string(errorMessage));
        sqlite3_free(errorMessage);
      }
    }

    if (_databaseWALJournal) {
      sqlite3_exec(_maintenanceDatabase, "PRAGMA journal_mode=WAL", nullptr, nullptr, &errorMessage);
      if (errorMessage) {
        GD::out.printError("Can't execute \"PRAGMA journal_mode=WAL\": " + std::string(errorMessage));
        sqlite3_free(errorMessage);
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void SQLite3::closeDatabase(bool lockMutex) {
  try {
    if (!_database) return;
    std::unique_lock<std::mutex> databaseGuard(_databaseMutex, std::defer_lock);
    if (lockMutex) databaseGuard.lock();
    GD::out.printInfo("Closing database...");
    char *errorMessage = nullptr;
    sqlite3_exec(_database, "COMMIT", nullptr, nullptr, &errorMessage); //Release all savepoints
    if (errorMessage) {
      //Normally error is: No transaction is active, so no real error
      GD::out.printDebug("Debug: Can't execute \"COMMIT\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }
    sqlite3_exec(_database, "PRAGMA synchronous = FULL", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      GD::out.printError("Error: Can't execute \"PRAGMA synchronous = FULL\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }
    sqlite3_exec(_database, "PRAGMA journal_mode = DELETE", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      GD::out.printError("Error: Can't execute \"PRAGMA journal_mode = DELETE\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }
    sqlite3_close(_database);
    _database = nullptr;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void SQLite3::closeMaintenanceDatabase(bool lockMutex) {
  try {
    if (!_maintenanceDatabase) return;
    std::unique_lock<std::mutex> databaseGuard(_databaseMutex, std::defer_lock);
    if (lockMutex) databaseGuard.lock();
    GD::out.printInfo("Closing maintenance database...");
    char *errorMessage = nullptr;
    sqlite3_exec(_maintenanceDatabase, "COMMIT", nullptr, nullptr, &errorMessage); //Release all savepoints
    if (errorMessage) {
      //Normally error is: No transaction is active, so no real error
      GD::out.printDebug("Debug: Can't execute \"COMMIT\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }
    sqlite3_exec(_maintenanceDatabase, "PRAGMA synchronous = FULL", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      GD::out.printError("Error: Can't execute \"PRAGMA synchronous = FULL\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }
    sqlite3_exec(_maintenanceDatabase, "PRAGMA journal_mode = DELETE", nullptr, nullptr, &errorMessage);
    if (errorMessage) {
      GD::out.printError("Error: Can't execute \"PRAGMA journal_mode = DELETE\": " + std::string(errorMessage));
      sqlite3_free(errorMessage);
    }
    sqlite3_close(_maintenanceDatabase);
    _maintenanceDatabase = nullptr;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void SQLite3::getDataRows(sqlite3_stmt *statement, std::shared_ptr<BaseLib::Database::DataTable> &dataRows) {
  int32_t result;
  int32_t row = 0;
  while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
    for (int32_t i = 0; i < sqlite3_column_count(statement); i++) {
      std::shared_ptr<BaseLib::Database::DataColumn> col(new BaseLib::Database::DataColumn());
      col->index = i;
      int32_t columnType = sqlite3_column_type(statement, i);
      if (columnType == SQLITE_INTEGER) {
        col->dataType = BaseLib::Database::DataColumn::DataType::Enum::INTEGER;
        col->intValue = sqlite3_column_int64(statement, i);
      } else if (columnType == SQLITE_FLOAT) {
        col->dataType = BaseLib::Database::DataColumn::DataType::Enum::FLOAT;
        col->floatValue = sqlite3_column_double(statement, i);
      } else if (columnType == SQLITE_BLOB) {
        col->dataType = BaseLib::Database::DataColumn::DataType::Enum::BLOB;
        char *binaryData = (char *)sqlite3_column_blob(statement, i);
        int32_t size = sqlite3_column_bytes(statement, i);
        if (size > 0) col->binaryValue.reset(new std::vector<char>(binaryData, binaryData + size));
      } else if (columnType == SQLITE_NULL) {
        col->dataType = BaseLib::Database::DataColumn::DataType::Enum::NODATA;
      } else if (columnType == SQLITE_TEXT) //or SQLITE3_TEXT. As we are not using SQLite version 2 it doesn't matter
      {
        col->dataType = BaseLib::Database::DataColumn::DataType::Enum::TEXT;
        col->textValue = std::string((const char *)sqlite3_column_text(statement, i));
      }
      if (i == 0) dataRows->insert(std::pair<uint32_t, std::map<uint32_t, std::shared_ptr<BaseLib::Database::DataColumn>>>(row, std::map<uint32_t, std::shared_ptr<BaseLib::Database::DataColumn>>()));
      dataRows->at(row)[i] = col;
    }
    row++;
  }
  if (result != SQLITE_DONE) {
    throw BaseLib::Exception("Can't execute command (Error-no.: " + std::to_string(result) + "): " + std::string(sqlite3_errmsg(_database)));
  }
}

bool SQLite3::bindData(sqlite3_stmt *statement, BaseLib::Database::DataRow &dataToEscape) {
  //There is no try/catch block on purpose!
  int32_t result = 0;
  int32_t index = 1;
  for (auto &col : dataToEscape) {
    switch (col->dataType) {
      case BaseLib::Database::DataColumn::DataType::Enum::NODATA: {
        result = sqlite3_bind_null(statement, index);
        break;
      }
      case BaseLib::Database::DataColumn::DataType::Enum::INTEGER: {
        result = sqlite3_bind_int64(statement, index, col->intValue);
        break;
      }
      case BaseLib::Database::DataColumn::DataType::Enum::FLOAT: {
        result = sqlite3_bind_double(statement, index, col->floatValue);
        break;
      }
      case BaseLib::Database::DataColumn::DataType::Enum::BLOB: {
        if (col->binaryValue->empty()) result = sqlite3_bind_null(statement, index);
        else result = sqlite3_bind_blob(statement, index, &col->binaryValue->at(0), col->binaryValue->size(), SQLITE_STATIC);
        break;
      }
      case BaseLib::Database::DataColumn::DataType::Enum::TEXT: {
        result = sqlite3_bind_text(statement, index, col->textValue.c_str(), -1, SQLITE_STATIC);
        break;
      }
    }
    if (result) return false;
    index++;
  }
  return true;
}

uint64_t SQLite3::executeWriteCommand(const std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>> &command, bool maintenanceDatabase) {
  try {
    if (!command) return 0;
    std::lock_guard<std::mutex> databaseGuard(_databaseMutex);
    if (!_database) {
      GD::out.printError("Error: Could not write to database. No database handle.");
      return 0;
    }
    if (maintenanceDatabase && !_maintenanceDatabase) return 0;

    auto database = maintenanceDatabase ? _maintenanceDatabase : _database;

    sqlite3_stmt *statement = nullptr;
    int32_t result = sqlite3_prepare_v2(database, command->first.c_str(), -1, &statement, nullptr);
    if (result) {
      GD::out.printError("Can't execute command \"" + command->first + "\": " + std::string(sqlite3_errmsg(database)));
      return 0;
    }
    if (!command->second.empty()) {
      if (!bindData(statement, command->second)) {
        GD::out.printError("Error binding data: " + std::string(sqlite3_errmsg(database)));
      }
    }
    result = sqlite3_step(statement);
    if (result != SQLITE_DONE) {
      GD::out.printError("Can't execute command: " + std::string(sqlite3_errmsg(database)));
      return 0;
    }
    sqlite3_clear_bindings(statement);
    result = sqlite3_finalize(statement);
    if (result) {
      GD::out.printError("Can't execute command \"" + command->first + "\": " + std::string(sqlite3_errmsg(database)));
      return 0;
    }
    uint64_t rowID = sqlite3_last_insert_rowid(database);

    return rowID;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}

uint64_t SQLite3::executeWriteCommand(const std::string &command, BaseLib::Database::DataRow &dataToEscape, bool maintenanceDatabase) {
  try {
    std::lock_guard<std::mutex> databaseGuard(_databaseMutex);
    if (!_database) {
      GD::out.printError("Error: Could not write to database. No database handle.");
      return 0;
    }
    if (maintenanceDatabase && !_maintenanceDatabase) return 0;

    auto database = maintenanceDatabase ? _maintenanceDatabase : _database;

    sqlite3_stmt *statement = nullptr;
    int32_t result = sqlite3_prepare_v2(database, command.c_str(), -1, &statement, nullptr);
    if (result) {
      GD::out.printError("Can't execute command \"" + command + "\": " + std::string(sqlite3_errmsg(database)));
      return 0;
    }
    if (!dataToEscape.empty()) {
      if (!bindData(statement, dataToEscape)) {
        GD::out.printError("Error binding data: " + std::string(sqlite3_errmsg(database)));
      }
    }
    result = sqlite3_step(statement);
    if (result != SQLITE_DONE) {
      GD::out.printError("Can't execute command \"" + command + "\": " + std::string(sqlite3_errmsg(database)));
      return 0;
    }
    sqlite3_clear_bindings(statement);
    result = sqlite3_finalize(statement);
    if (result) {
      GD::out.printError("Can't execute command \"" + command + "\": " + std::string(sqlite3_errmsg(database)));
      return 0;
    }
    uint64_t rowID = sqlite3_last_insert_rowid(database);

    return rowID;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}

std::shared_ptr<BaseLib::Database::DataTable> SQLite3::executeCommand(const std::string &command, BaseLib::Database::DataRow &dataToEscape, bool maintenanceDatabase) {
  auto dataRows = std::make_shared<BaseLib::Database::DataTable>();
  try {
    std::lock_guard<std::mutex> databaseGuard(_databaseMutex);
    if (!_database) {
      GD::out.printError("Error: Could not write to database. No database handle.");
      return dataRows;
    }
    if (maintenanceDatabase && !_maintenanceDatabase) return dataRows;

    auto database = maintenanceDatabase ? _maintenanceDatabase : _database;

    sqlite3_stmt *statement = nullptr;
    int32_t result = sqlite3_prepare_v2(database, command.c_str(), -1, &statement, nullptr);
    if (result) {
      GD::out.printError("Can't execute command \"" + command + "\": " + std::string(sqlite3_errmsg(database)));
      return dataRows;
    }
    if (!bindData(statement, dataToEscape)) {
      GD::out.printError("Error binding data: " + std::string(sqlite3_errmsg(database)));
    }
    try {
      getDataRows(statement, dataRows);
    }
    catch (const std::exception &ex) {
      if (command.compare(0, 7, "RELEASE") == 0) {
        GD::out.printInfo(std::string("Info: ") + ex.what());
        sqlite3_clear_bindings(statement);
        return dataRows;
      } else GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    sqlite3_clear_bindings(statement);
    result = sqlite3_finalize(statement);
    if (result) GD::out.printError("Can't execute command \"" + command + "\" (Error-no.: " + std::to_string(result) + "): " + std::string(sqlite3_errmsg(database)));
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return dataRows;
}

std::shared_ptr<BaseLib::Database::DataTable> SQLite3::executeCommand(const std::string &command, bool maintenanceDatabase) {
  auto dataRows = std::make_shared<BaseLib::Database::DataTable>();
  try {
    std::lock_guard<std::mutex> databaseGuard(_databaseMutex);
    if (!_database) {
      GD::out.printError("Error: Could not write to database. No database handle.");
      return dataRows;
    }
    if (maintenanceDatabase && !_maintenanceDatabase) return dataRows;

    auto database = maintenanceDatabase ? _maintenanceDatabase : _database;

    sqlite3_stmt *statement = nullptr;
    int32_t result = sqlite3_prepare_v2(database, command.c_str(), -1, &statement, nullptr);
    if (result) {
      GD::out.printError("Can't execute command \"" + command + "\": " + std::string(sqlite3_errmsg(database)));
      return dataRows;
    }
    try {
      getDataRows(statement, dataRows);
    }
    catch (const std::exception &ex) {
      if (command.compare(0, 7, "RELEASE") == 0) {
        GD::out.printInfo(std::string("Info: ") + ex.what());
        sqlite3_clear_bindings(statement);
        return dataRows;
      } else GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    sqlite3_clear_bindings(statement);
    result = sqlite3_finalize(statement);
    if (result) {
      GD::out.printError("Can't execute command \"" + command + "\" (Error-no.: " + std::to_string(result) + "): " + std::string(sqlite3_errmsg(database)));
      return dataRows;
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return dataRows;
}

/*
void SQLite3::benchmark1()
{
	//Duration for REPLACE in ms: 17836
	//Duration for UPDATE in ms: 14973
	std::vector<uint8_t> value({0});
	int64_t startTime = HelperFunctions::getTime();
	std::map<uint32_t, uint32_t> ids;
	for(uint32_t i = 0; i < 10000; ++i)
	{
		DataRow data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(10000000)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(1)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(2)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn("TEST" + std::to_string(i))));
		value[0] = (i % 256);
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
		ids[i] = executeWriteCommand("REPLACE INTO parameters VALUES(?, ?, ?, ?, ?, ?, ?, ?)", data);
	}
	int64_t duration = HelperFunctions::getTime() - startTime;
	std::cerr << "Duration for REPLACE in ms: " << duration << std::endl;
	startTime = HelperFunctions::getTime();
	for(uint32_t i = 0; i < 10000; ++i)
	{
		DataRow data;
		value[0] = (i % 256);
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(ids[i])));
		executeWriteCommand("UPDATE parameters SET value=? WHERE parameterID=?", data);
	}
	duration = HelperFunctions::getTime() - startTime;
	std::cerr << "Duration for UPDATE in ms: " << duration << std::endl;
	executeCommand("DELETE FROM parameters WHERE peerID=10000000");
}

void SQLite3::benchmark2()
{
	try
	{
		//Duration for REPLACE in ms: 17002
		//Duration for UPDATE in ms: 14391
		sqlite3_stmt* statement = nullptr;
		int32_t result = sqlite3_prepare_v2(_database, "REPLACE INTO parameters VALUES(?, ?, ?, ?, ?, ?, ?, ?)", -1, &statement, NULL);
		if(result)
		{
			throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
		}
		std::vector<uint8_t> value({0});
		int64_t startTime = HelperFunctions::getTime();
		std::map<uint32_t, uint32_t> ids;
		for(uint32_t i = 0; i < 10000; ++i)
		{
			DataRow data;
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(10000000)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(1)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(2)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn("TEST" + std::to_string(i))));
			value[0] = (i % 256);
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
			bindData(statement, data);
			ids[i] = sqlite3_step(statement);
			sqlite3_clear_bindings(statement);
			sqlite3_reset(statement);
		}
		sqlite3_finalize(statement);
		statement = nullptr;
		sqlite3_prepare_v2(_database, "UPDATE parameters SET value=? WHERE parameterID=?", -1, &statement, NULL);
		int64_t duration = HelperFunctions::getTime() - startTime;
		std::cerr << "Duration for REPLACE in ms: " << duration << std::endl;
		startTime = HelperFunctions::getTime();
		for(uint32_t i = 0; i < 10000; ++i)
		{
			DataRow data;
			value[0] = (i % 256);
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(ids[i])));
			bindData(statement, data);
			ids[i] = sqlite3_step(statement);
			sqlite3_clear_bindings(statement);
			sqlite3_reset(statement);
		}
		sqlite3_finalize(statement);
		duration = HelperFunctions::getTime() - startTime;
		std::cerr << "Duration for UPDATE in ms: " << duration << std::endl;
		executeCommand("DELETE FROM parameters WHERE peerID=10000000");
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void SQLite3::benchmark3()
{
	try
	{
		sqlite3_stmt* statement = nullptr;
		int32_t result = sqlite3_prepare_v2(_database, "REPLACE INTO parameters VALUES(?, ?, ?, ?, ?, ?, ?, ?)", -1, &statement, NULL);
		if(result)
		{
			throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
		}
		std::vector<uint8_t> value({0});
		executeCommand("BEGIN IMMEDIATE TRANSACTION");
		int64_t startTime = HelperFunctions::getTime();
		std::map<uint32_t, uint32_t> ids;
		for(uint32_t i = 0; i < 10000; ++i)
		{
			DataRow data;
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(10000000)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(1)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(2)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn("TEST" + std::to_string(i))));
			value[0] = (i % 256);
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
			bindData(statement, data);
			ids[i] = sqlite3_step(statement);
			sqlite3_clear_bindings(statement);
			sqlite3_reset(statement);
		}
		sqlite3_finalize(statement);
		statement = nullptr;
		executeCommand("COMMIT TRANSACTION");
		int64_t duration = HelperFunctions::getTime() - startTime;
		std::cerr << "Duration for REPLACE in ms: " << duration << std::endl;
		sqlite3_prepare_v2(_database, "UPDATE parameters SET value=? WHERE parameterID=?", -1, &statement, NULL);
		executeCommand("BEGIN IMMEDIATE TRANSACTION");
		startTime = HelperFunctions::getTime();
		for(uint32_t i = 0; i < 10000; ++i)
		{
			DataRow data;
			value[0] = (i % 256);
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(ids[i])));
			bindData(statement, data);
			ids[i] = sqlite3_step(statement);
			sqlite3_clear_bindings(statement);
			sqlite3_reset(statement);
		}
		sqlite3_finalize(statement);
		executeCommand("COMMIT TRANSACTION");
		duration = HelperFunctions::getTime() - startTime;
		std::cerr << "Duration for UPDATE in ms: " << duration << std::endl;
		executeCommand("DELETE FROM parameters WHERE peerID=10000000");
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void SQLite3::benchmark4()
{
	try
	{
		sqlite3_stmt* statement = nullptr;
		int32_t result = sqlite3_prepare_v2(_database, "REPLACE INTO parameters VALUES(?, ?, ?, ?, ?, ?, ?, ?)", -1, &statement, NULL);
		if(result)
		{
			throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
		}
		std::vector<uint8_t> value({0});
		executeCommand("SAVEPOINT peerConfig");
		int64_t startTime = HelperFunctions::getTime();
		std::map<uint32_t, uint32_t> ids;
		for(uint32_t i = 0; i < 10000; ++i)
		{
			DataRow data;
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(10000000)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(1)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(2)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn("TEST" + std::to_string(i))));
			value[0] = (i % 256);
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
			bindData(statement, data);
			ids[i] = sqlite3_step(statement);
			sqlite3_clear_bindings(statement);
			sqlite3_reset(statement);
		}
		sqlite3_finalize(statement);
		statement = nullptr;
		executeCommand("RELEASE peerConfig");
		int64_t duration = HelperFunctions::getTime() - startTime;
		std::cerr << "Duration for REPLACE in ms: " << duration << std::endl;
		sqlite3_prepare_v2(_database, "UPDATE parameters SET value=? WHERE parameterID=?", -1, &statement, NULL);
		executeCommand("SAVEPOINT peerConfig");
		startTime = HelperFunctions::getTime();
		for(uint32_t i = 0; i < 10000; ++i)
		{
			DataRow data;
			value[0] = (i % 256);
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(ids[i])));
			bindData(statement, data);
			ids[i] = sqlite3_step(statement);
			sqlite3_clear_bindings(statement);
			sqlite3_reset(statement);
		}
		sqlite3_finalize(statement);
		executeCommand("RELEASE peerConfig");
		duration = HelperFunctions::getTime() - startTime;
		std::cerr << "Duration for UPDATE in ms: " << duration << std::endl;
		executeCommand("DELETE FROM parameters WHERE peerID=10000000");
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
*/

}
