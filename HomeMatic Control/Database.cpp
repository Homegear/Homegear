#include "Database.h"

Database GD::db;

Database::Database()
{

}

Database::Database(std::string databasePath)
{
	if(databasePath.size() == 0) return;
    openDatabase(databasePath);
}

void Database::init(std::string databasePath)
{
	if(databasePath.size() == 0) return;
    openDatabase(databasePath);
}

Database::~Database()
{
    closeDatabase();
}

void Database::openDatabase(std::string databasePath)
{
    int result = sqlite3_open(databasePath.c_str(), &_database);
    if(result)
    {
        throw(Exception("Can't open database: " + std::string(sqlite3_errmsg(_database))));
        sqlite3_close(_database);
    }
}

void Database::closeDatabase()
{
    sqlite3_close(_database);
}

void Database::getDataRows(sqlite3_stmt* statement, DataTable& dataRows)
{
	try
	{
		int32_t result;
		int32_t row = 0;
		while((result = sqlite3_step(statement)) == SQLITE_ROW)
		{
			for(int32_t i = 0; i < sqlite3_column_count(statement); i++)
			{
				std::shared_ptr<DataColumn> col(new DataColumn());
				col->index = i;
				int32_t columnType = sqlite3_column_type(statement, i);
				if(columnType == SQLITE_INTEGER)
				{
					col->dataType = DataColumn::DataType::Enum::INTEGER;
					col->intValue = sqlite3_column_int64(statement, i);
				}
				else if(columnType == SQLITE_FLOAT)
				{
					col->dataType = DataColumn::DataType::Enum::FLOAT;
					col->floatValue = sqlite3_column_double(statement, i);
				}
				else if(columnType == SQLITE_BLOB)
				{
					col->dataType = DataColumn::DataType::Enum::BLOB;
					char* binaryData = (char*)sqlite3_column_blob(statement, i);
					int32_t size = sqlite3_column_bytes(statement, i);
					col->binaryValue.reset(new std::vector<char>(binaryData, binaryData + size));
				}
				else if(columnType == SQLITE_NULL)
				{
					col->dataType = DataColumn::DataType::Enum::NODATA;
				}
				else if(columnType == SQLITE_TEXT) //or SQLITE3_TEXT. As we are not using SQLite version 2 it doesn't matter
				{
					col->dataType = DataColumn::DataType::Enum::TEXT;
					col->textValue = std::string((const char*)sqlite3_column_text(statement, i));
					if(GD::debugLevel >= 5)
					{
						std::cout << "Read text column from database: " << col->textValue << std::endl;;
					}
				}
				dataRows[row][i] = col;
			}
			row++;
		}
		if(result != SQLITE_DONE)
		{
			throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
}

DataTable Database::executeCommand(std::string command, DataColumnVector& dataToEscape)
{
	DataTable dataRows;
	try
	{
		if(GD::debugLevel >= 5) std::cout << "Executing SQL command: " << command << std::endl;
		_databaseMutex.lock();
		sqlite3_stmt* statement = 0;
		int result = sqlite3_prepare_v2(_database, command.c_str(), -1, &statement, NULL);
		if(result)
		{
			throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
		}
		int32_t index = 1;
		std::for_each(dataToEscape.begin(), dataToEscape.end(), [&](std::shared_ptr<DataColumn> col)
		{
			switch(col->dataType)
			{
				case DataColumn::DataType::Enum::NODATA:
					result = sqlite3_bind_null(statement, index);
					break;
				case DataColumn::DataType::Enum::INTEGER:
					result = sqlite3_bind_int64(statement, index, col->intValue);
					break;
				case DataColumn::DataType::Enum::FLOAT:
					result = sqlite3_bind_double(statement, index, col->floatValue);
					break;
				case DataColumn::DataType::Enum::BLOB:
					result = sqlite3_bind_blob(statement, index, &col->binaryValue->at(0), col->binaryValue->size(), SQLITE_STATIC);
					break;
				case DataColumn::DataType::Enum::TEXT:
					result = sqlite3_bind_text(statement, index, col->textValue.c_str(), -1, SQLITE_STATIC);
					break;
			}
			if(result)
			{
				throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
			}
			index++;
		});
		getDataRows(statement, dataRows);
		result = sqlite3_finalize(statement);
		if(result)
		{
			throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
	}
	_databaseMutex.unlock();
	return dataRows;
}

DataTable Database::executeCommand(std::string command)
{
    DataTable dataRows;
    try
    {
    	if(GD::debugLevel >= 5) std::cout << "Executing SQL command: " << command << std::endl;
    	_databaseMutex.lock();
		sqlite3_stmt* statement = 0;
		int result = sqlite3_prepare_v2(_database, command.c_str(), -1, &statement, NULL);
		if(result)
		{
			throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
		}
		getDataRows(statement, dataRows);
		result = sqlite3_finalize(statement);
		if(result)
		{
			throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
		}
    }
    catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << '\n';
    }
    _databaseMutex.unlock();
    return dataRows;
}
