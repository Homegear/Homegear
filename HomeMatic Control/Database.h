#ifndef DATABASE_H
#define DATABASE_H

using namespace std;

#include "Exception.h"
#include "GD.h"

#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>

#include <sqlite3.h>

enum DataType { NODATA, INTEGER, FLOAT, TEXT };

class DataColumn
{
    public:
        DataType dataType;
        int32_t index;
        int64_t intValue;
        double floatValue;
        std::string textValue;
};

class Database
{
    public:
		Database();
        Database(std::string databasePath);
        virtual ~Database();
        void init(std::string databasePath);
        std::vector<std::vector<DataColumn>> executeCommand(std::string command);
        std::vector<std::vector<DataColumn>> executeCommand(std::string command, std::vector<DataColumn> dataToEscape);
    protected:
    private:
        sqlite3* _database;
        std::mutex _databaseMutex;

        void openDatabase(std::string databasePath);
        void closeDatabase();
        void getDataRows(sqlite3_stmt* statement, std::vector<std::vector<DataColumn>>* dataRows);
};

#endif // DATABASE_H
