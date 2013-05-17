#include "Database.h"

Database::Database(std::string databasePath)
{
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

void Database::getDataRows(sqlite3_stmt* statement, std::vector<std::vector<DataColumn>>* dataRows)
{
	try
	{
		int result;
		while((result = sqlite3_step(statement)) == SQLITE_ROW)
		{
			std::vector<DataColumn> row;
			for(int i = 0; i < sqlite3_column_count(statement); i++)
			{
				DataColumn col;
				col.index = i;
				switch(sqlite3_column_type(statement, i))
				{
					case SQLITE_INTEGER:
						col.dataType = INTEGER;
						col.intValue = sqlite3_column_int(statement, i);
						break;
					case SQLITE_FLOAT:
						col.dataType = FLOAT;
						col.floatValue = sqlite3_column_double(statement, i);
						break;
					case SQLITE_BLOB:
						col.dataType = TEXT;
						col.textValue = std::string((const char*)sqlite3_column_text(statement, i));
						break;
					case SQLITE_NULL:
						col.dataType = NODATA;
						break;
					case SQLITE_TEXT: //or SQLITE3_TEXT. As we are not using SQLite version 2 it doesn't matter
						col.dataType = TEXT;
						col.textValue = std::string((const char*)sqlite3_column_text(statement, i));
						break;
				}
				row.push_back(col);
			}
			dataRows->push_back(row);
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

std::vector<std::vector<DataColumn>> Database::executeCommand(std::string command, std::vector<DataColumn> dataToEscape)
{
	std::vector<std::vector<DataColumn>> dataRows;
	try
	{
		sqlite3_stmt* statement = 0;
		int result = sqlite3_prepare_v2(_database, command.c_str(), -1, &statement, NULL);
		if(result)
		{
			throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
		}
		std::for_each(dataToEscape.begin(), dataToEscape.end(), [&](DataColumn col)
		{
			switch(col.dataType)
			{
				case NODATA:
					result = sqlite3_bind_null(statement, col.index);
					break;
				case INTEGER:
					result = sqlite3_bind_int(statement, col.index, col.intValue);
					break;
				case FLOAT:
					result = sqlite3_bind_double(statement, col.index, col.floatValue);
					break;
				case TEXT:
					result = sqlite3_bind_text(statement, col.index, col.textValue.c_str(), -1, SQLITE_TRANSIENT);
					break;
			}
			if(result)
			{
				throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
			}
		});
		getDataRows(statement, &dataRows);
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
	return dataRows;
}

std::vector<std::vector<DataColumn>> Database::executeCommand(std::string command)
{
    std::vector<std::vector<DataColumn>> dataRows;
    try
    {
		sqlite3_stmt* statement = 0;
		int result = sqlite3_prepare_v2(_database, command.c_str(), -1, &statement, NULL);
		if(result)
		{
			throw(Exception("Can't execute command: " + std::string(sqlite3_errmsg(_database))));
		}
		getDataRows(statement, &dataRows);
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
    return dataRows;
}
