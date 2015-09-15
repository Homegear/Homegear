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

#ifndef DATABASETYPES_H
#define DATABASETYPES_H

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <memory>

namespace BaseLib
{
namespace Database
{

/**
 * Class to store data of a data column in.
 */
class DataColumn
{
    public:
		/**
		 * Enumeration of the data types which can be stored in this class.
		 */
		struct DataType
		{
			enum Enum { NODATA, INTEGER, FLOAT, TEXT, BLOB };
		};

		/**
		 * The data type of the column.
		 */
        DataType::Enum dataType = DataType::Enum::NODATA;

        /**
         * The column index.
         */
        int32_t index = 0;

        /**
         * The integer value.
         */
        int64_t intValue = 0;

        /**
         * The float value.
         */
        double floatValue = 0;

        /**
         * The text value.
         */
        std::string textValue;

        /**
         * The binary value.
         */
        std::shared_ptr<std::vector<char>> binaryValue;

        /**
         * Default constructor.
         */
        DataColumn() { binaryValue.reset(new std::vector<char>()); }

        /**
         * Constructor to create a data column of type INTEGER.
         *
         * @param value The column data.
         */
        DataColumn(int64_t value) : DataColumn() { dataType = DataType::Enum::INTEGER; intValue = value; }

        /**
         * Constructor to create a data column of type INTEGER.
         *
         * @param value The column data.
         */
        DataColumn(uint64_t value) : DataColumn() { dataType = DataType::Enum::INTEGER; intValue = value; }

        /**
         * Constructor to create a data column of type INTEGER.
         *
         * @param value The column data.
         */
        DataColumn(int32_t value) : DataColumn() { dataType = DataType::Enum::INTEGER; intValue = value; }

        /**
         * Constructor to create a data column of type INTEGER.
         *
         * @param value The column data.
         */
        DataColumn(uint32_t value) : DataColumn() { dataType = DataType::Enum::INTEGER; intValue = value; }

        /**
         * Constructor to create a data column of type TEXT.
         *
         * @param value The column data.
         */
        DataColumn(std::string value) : DataColumn() { dataType = DataType::Enum::TEXT; textValue = value; }

        /**
         * Constructor to create a data column of type FLOAT.
         *
         * @param value The column data.
         */
        DataColumn(double value) : DataColumn() { dataType = DataType::Enum::FLOAT; floatValue = value; }

        /**
         * Constructor to create a data column of type BLOB.
         *
         * @param value The column data. It is not copied! So make sure to not modify it as long as the DataColumn object exists.
         */
        DataColumn(std::shared_ptr<std::vector<char>> value) : DataColumn() { dataType = DataType::Enum::BLOB; if(value) binaryValue = value; }

        /**
         * Constructor to create a data column of type BLOB.
         *
         * @param value The column data. The data is copied.
         */
        DataColumn(const std::vector<char>& value) : DataColumn()
        {
        	dataType = DataType::Enum::BLOB;
        	binaryValue.reset(new std::vector<char>());
        	binaryValue->insert(binaryValue->begin(), value.begin(), value.end());
        }

        /**
         * Constructor to create a data column of type BLOB.
         *
         * @param value The column data. The data is copied.
         */
        DataColumn(const std::vector<uint8_t>& value) : DataColumn()
        {
        	dataType = DataType::Enum::BLOB;
        	binaryValue.reset(new std::vector<char>());
        	binaryValue->insert(binaryValue->begin(), value.begin(), value.end());
        }

        /**
         * Destructor. It does nothing.
         */
        virtual ~DataColumn() {}
};

/**
 * Type definition to easily create a DataTable.
 */
typedef std::map<uint32_t, std::map<uint32_t, std::shared_ptr<DataColumn>>> DataTable;

/**
 * Type definition to easily create a DataRow.
 */
typedef std::deque<std::shared_ptr<DataColumn>> DataRow;

}
}
#endif
