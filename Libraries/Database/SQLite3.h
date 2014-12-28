/* Copyright 2013-2015 Sathya Laufer
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

#ifndef SQLITE3_H_
#define SQLITE3_H_

#include "../../Modules/Base/Database/DatabaseTypes.h"

#include <mutex>

#include <sqlite3.h>

namespace BaseLib
{
namespace Database
{

class SQLite3
{
    public:
		SQLite3();
        SQLite3(std::string databasePath, bool databaseSynchronous, bool databaseMemoryJournal);
        virtual ~SQLite3();
        void dispose();
        void init(std::string databasePath, bool databaseSynchronous, bool databaseMemoryJournal, std::string backupPath = "");
        uint32_t executeWriteCommand(std::string command, DataRow& dataToEscape);
        std::shared_ptr<DataTable> executeCommand(std::string command);
        std::shared_ptr<DataTable> executeCommand(std::string command, DataRow& dataToEscape);
        bool isOpen() { return _database != nullptr; }
        void benchmark1();
        void benchmark2();
        void benchmark3();
        void benchmark4();
    protected:
    private:
        sqlite3* _database = nullptr;
        std::mutex _databaseMutex;

        void openDatabase(std::string databasePath, bool databaseSynchronous, bool databaseMemoryJournal);
        void closeDatabase();
        void getDataRows(sqlite3_stmt* statement, std::shared_ptr<DataTable>& dataRows);
        void bindData(sqlite3_stmt* statement, DataRow& dataToEscape);
};

}
}

#endif /* SQLITE3_H_ */
