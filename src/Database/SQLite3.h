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

#ifndef SQLITE3_H_
#define SQLITE3_H_

#include "homegear-base/Database/DatabaseTypes.h"

#include <mutex>

#include <sqlite3.h>

namespace Homegear {

class SQLite3 {
 public:
  SQLite3();
  SQLite3(const std::string &databasePath, const std::string &databaseFilename, const std::string &factoryDatabasePath, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWALJournal);
  virtual ~SQLite3();
  void dispose();
  void init(const std::string &databasePath, const std::string &databaseFilename, const std::string &factoryDatabasePath, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWALJournal, const std::string &backupPath = "", const std::string &factoryDatabaseBackupPath = "", const std::string &backupFilename = "");
  void hotBackup(bool factoryDatabase);
  bool enableMaintenanceMode();
  bool disableMaintenanceMode();
  uint64_t executeWriteCommand(const std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>>& command, bool factoryDatabase);
  uint64_t executeWriteCommand(const std::string& command, BaseLib::Database::DataRow &dataToEscape, bool factoryDatabase);
  std::shared_ptr<BaseLib::Database::DataTable> executeCommand(const std::string& command, bool factoryDatabase);
  std::shared_ptr<BaseLib::Database::DataTable> executeCommand(const std::string& command, BaseLib::Database::DataRow &dataToEscape, bool factoryDatabase);
  bool isOpen() { return _database != nullptr; }
  /*void benchmark1();
  void benchmark2();
  void benchmark3();
  void benchmark4();*/
 protected:
 private:
  std::string _databasePath;
  std::string _databaseFilename;
  std::string _factoryDatabasePath;
  std::string _backupPath;
  std::string _factoryDatabaseBackupPath;
  std::string _backupFilename;
  bool _databaseSynchronous = true;
  bool _databaseMemoryJournal = false;
  bool _databaseWALJournal = true;
  sqlite3 *_database = nullptr;
  sqlite3 *_factoryDatabase = nullptr;
  std::mutex _databaseMutex;

  bool checkIntegrity(const std::string &databasePath);
  void openDatabase(bool lockMutex);
  void openFactoryDatabase(bool lockMutex);
  void closeDatabase(bool lockMutex);
  void closeFactoryDatabase(bool lockMutex);
  void getDataRows(sqlite3_stmt *statement, std::shared_ptr<BaseLib::Database::DataTable> &dataRows);
  static bool bindData(sqlite3_stmt *statement, BaseLib::Database::DataRow &dataToEscape);
};

}

#endif /* SQLITE3_H_ */
