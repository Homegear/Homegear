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
        DataTable executeCommand(std::string command);
        DataTable executeCommand(std::string command, DataColumnVector& dataToEscape);
    protected:
    private:
        sqlite3* _database;
        std::mutex _databaseMutex;

        void openDatabase(std::string databasePath);
        void closeDatabase();
        void getDataRows(sqlite3_stmt* statement, DataTable& dataRows);
};

#endif // DATABASE_H
