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

#ifndef DATABASE_H
#define DATABASE_H

#include "Exception.h"
#include "GD.h"

#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <memory>

#include <sqlite3.h>

class DataColumn
{
    public:
		struct DataType
		{
			enum Enum { NODATA, INTEGER, FLOAT, TEXT, BLOB };
		};

        DataType::Enum dataType = DataType::Enum::NODATA;
        int32_t index = 0;
        int64_t intValue = 0;
        double floatValue = 0;
        std::string textValue;
        std::shared_ptr<std::vector<char>> binaryValue;

        DataColumn() { binaryValue.reset(new std::vector<char>()); }
        DataColumn(int64_t value) : DataColumn() { dataType = DataType::Enum::INTEGER; intValue = value; }
        DataColumn(uint64_t value) : DataColumn() { dataType = DataType::Enum::INTEGER; intValue = value; }
        DataColumn(int32_t value) : DataColumn() { dataType = DataType::Enum::INTEGER; intValue = value; }
        DataColumn(uint32_t value) : DataColumn() { dataType = DataType::Enum::INTEGER; intValue = value; }
        DataColumn(std::string value) : DataColumn() { dataType = DataType::Enum::TEXT; textValue = value; }
        DataColumn(double value) : DataColumn() { dataType = DataType::Enum::FLOAT; floatValue = value; }
        DataColumn(std::shared_ptr<std::vector<char>> value) : DataColumn() { dataType = DataType::Enum::BLOB; if(value) binaryValue = value; }
        DataColumn(std::vector<char>& value) : DataColumn()
        {
        	dataType = DataType::Enum::BLOB;
        	binaryValue.reset(new std::vector<char>());
        	*binaryValue = value;
        }
        DataColumn(std::vector<uint8_t>& value) : DataColumn()
        {
        	dataType = DataType::Enum::BLOB;
        	binaryValue.reset(new std::vector<char>());
        	binaryValue->insert(binaryValue->begin(), value.begin(), value.end());
        }
        virtual ~DataColumn() {}
};

typedef std::map<uint32_t, std::map<uint32_t, std::shared_ptr<DataColumn>>> DataTable;
typedef std::vector<std::shared_ptr<DataColumn>> DataColumnVector;

class Database
{
    public:
		Database();
        Database(std::string databasePath);
        virtual ~Database();
        void init(std::string databasePath, std::string backupPath = "");
        uint32_t executeWriteCommand(std::string command, DataColumnVector& dataToEscape);
        DataTable executeCommand(std::string command);
        DataTable executeCommand(std::string command, DataColumnVector& dataToEscape);
        bool isOpen() { return _database != nullptr; }
        void benchmark1();
        void benchmark2();
        void benchmark3();
        void benchmark4();
    protected:
    private:
        sqlite3* _database;
        std::mutex _databaseMutex;

        void openDatabase(std::string databasePath);
        void closeDatabase();
        void getDataRows(sqlite3_stmt* statement, DataTable& dataRows);
        void bindData(sqlite3_stmt* statement, DataColumnVector& dataToEscape);
};

#endif // DATABASE_H
